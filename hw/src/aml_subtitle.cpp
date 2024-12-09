// Copyright (C) 2015 Amlogic, Inc. All rights reserved.
//
// All information contained herein is Amlogic confidential.
//
// This software is provided to you pursuant to Software License
// Agreement (SLA) with Amlogic Inc ("Amlogic"). This software may be
// used only in accordance with the terms of this agreement.
//
// Redistribution and use in source and binary forms, with or without
// modification is strictly prohibited without prior written permission
// from Amlogic.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <string>
#include <mutex>

extern "C" {
    #include "techtype.h"
    #include "stbhwini.h"
    #include "stbhwos.h"
}
#include "dtv_log.h"

#include "SubtitleNativeAPI.h"
#include "aml_subtitle.h"


#define TAG  "subtitle_dtvkit"
#define SUB_LOG(x,...) DTV_LOG(ANDROID_LOG_INFO, TAG, x, ##__VA_ARGS__ )

static subtitle_context_t sub_context = {nullptr, 0, TYPE_NONE};

static std::mutex sub_mutex;

/*---global variable definitions---------------------------------------------*/
extern OSD_OverlayDraw_Func g_OverlayDraw_Func;

static int parse_subtitle_type(int dvb_type) {
    int type = TYPE_SUBTITLE_DVB;
    switch (dvb_type) {
        default:
        case TYPE_DVB:
            type = TYPE_SUBTITLE_DVB;
            break;
        case TYPE_TTX:
        case TYPE_TTX_SUBTITLE:
            type = TYPE_SUBTITLE_DVB_TELETEXT;
            break;
        case TYPE_SCTE:
            type = TYPE_SUBTITLE_SCTE27;
            break;
        case TYPE_ARIB:
            type = TYPE_SUBTITLE_ARIB_B24;
            break;
        case TYPE_CC:
            type = TYPE_SUBTITLE_CLOSED_CAPTION;
            break;
    }
    return type;
}

static void on_subtitle_data(const char* data,
                                  int size,
                                  AmlSubDataType type,
                                  int x,
                                  int y,
                                  int width,
                                  int height,
                                  int video_width,
                                  int video_height,
                                  int show) {
    unsigned char extra;
    int h_extra;
    int draw_type;
    int width_fixed = width;
    int height_fixed = height;

    //SUB_LOG("on_subtitle_data: %p (type:%d, show:%d, [%d,%d,%d,%d] in (%d,%d)",
    //    data, type, show, x, y, width, height, video_width, video_height);

    if (show == 1 && size == 0) {
        SUB_LOG("skip show no size data");
        return;
    }

    if (type == SUB_DATA_TYPE_STRING || type == SUB_DATA_TYPE_CC_JSON) {
        width_fixed = size;
        height_fixed = 1;
    }

    //add extra info for subtitle
    //bit 27 - bit 20
    //extra 0x80
    //show 0x40
    //type 0x0- 0x3f
    if (g_OverlayDraw_Func && type <= SUB_DATA_TYPE_POSITION_BITMAP) {
        draw_type = parse_subtitle_type(sub_context.type);
        extra = 0x80 | (show << 6) | draw_type;
        h_extra = extra << 20 | video_height;
        g_OverlayDraw_Func(
            width_fixed,
            height_fixed,
            x,
            y,
            video_width,
            h_extra,
            (const unsigned char *)data);
    }
}

static void on_cc_channel_event(int event, int id) {
    SUB_LOG("on_cc_channel_event: (event:%d, id:%d)", event, id);
    if (event == SUBTITLE_CC_EVENT_CHANNELS_CHANGED)
    {
        subtitle_data_event_t cc_event = {
            .type = event,
            .data = id,
            .arg1 = 0,
            .arg2 = 0,
            .user_data = NULL,
        };
        STB_OSSendEvent(FALSE, HW_EV_CLASS_SUBTITLE, HW_EV_TYPE_TRUE,
            &cc_event, sizeof(subtitle_data_event_t));
    }
}

void aml_subtitle_open(int type, aml_subtitle_param_t *p) {
    AmlSubtitleParam sub_param;
    int status;
    int sub_type;

    if (p->pid == 0x1fff)
        return;

    std::lock_guard<std::mutex> lock(sub_mutex);

    SUB_LOG("Open subtitle start");
    if (!sub_context.handle) {
        sub_context.handle = amlsub_Create();
#ifndef RDK_COMPILE
        amlsub_RegistOnDataCB(sub_context.handle, (AmlSubtitleDataCb)on_subtitle_data);
#endif
        amlsub_RegistOnChannelUpdateCb(sub_context.handle, (AmlChannelUpdateCb)on_cc_channel_event);
    } else {
        amlsub_Close(sub_context.handle);
    }
    sub_context.type = (subtitle_type)type;

    SUB_LOG("Open subtitle (type:%d,pid:%u,dmx:%d, arg1:%d, arg2:%d)",
        type, p->pid, p->dmx_id, p->arg1, p->arg2);

    sub_type = parse_subtitle_type(type);
    //memset(&sub_param, 0, sizeof(AmlSubtitleParam));
    sub_param.ioSource = E_SUBTITLE_DEMUX;
    sub_param.subtitleType = sub_type;
    sub_param.pid = p->pid;
    sub_param.dmxId = p->dmx_id;
    sub_param.flag = (int)p->ca_flag;

    //amlsub_SetPip(sub_context.handle, SCRAMBLE_TYPE, p->ca_flag);

    switch (type) {
        case TYPE_DVB:
        case TYPE_SCTE:
        case TYPE_ARIB: {
            sub_param.ancillaryPageId = p->arg1;
            sub_param.compositionPageId = p->arg2;
            status = amlsub_Open(sub_context.handle, &sub_param);
        }
        break;
        case TYPE_TTX_SUBTITLE:
        case TYPE_TTX: {
            status = amlsub_Open(sub_context.handle, &sub_param);
            if (status == SUB_STAT_OK) {
                AmlTeletextCtrlParam ttx_param;
                ttx_param.magazine = p->arg1;
                ttx_param.page = p->arg2;
                if (type == TYPE_TTX) {
                    ttx_param.regionid = p->flag1;
                } else {
                    //ttx subtitle use orig info of stream
                    ttx_param.regionid = -1;
                }

                if (type == TYPE_TTX_SUBTITLE)
                    ttx_param.event = TT_EVENT_GO_TO_SUBTITLE;
                else
                    ttx_param.event = TT_EVENT_GO_TO_PAGE;
                amlsub_TeletextControl(sub_context.handle, &ttx_param);
            }
        }
        break;
        case TYPE_CC: {
            int cc_Id = p->pid;
            bool is_atv = (p->dmx_id == -1);
            int vbi_src_flag = is_atv ? 1 : 0;
            sub_param.channelId = (cc_Id | (vbi_src_flag << 8));
            sub_param.pid = 0;
            status = amlsub_Open(sub_context.handle, &sub_param);
        }
        break;
    }

    if (status != SUB_STAT_OK)
        SUB_LOG("Open subtitle failed (%d)", status);
    else {
        SUB_LOG("Open subtitle, update initial decoder id(%d), sync_id(%d)",
            p->decoder_id, p->sync_id);
        amlsub_SetPip(sub_context.handle, MODE_SUBTITLE_PIP_PLAYER, p->decoder_id);
        amlsub_SetPip(sub_context.handle, MODE_SUBTITLE_PIP_MEDIASYNC, p->sync_id);
    }

    SUB_LOG("Open subtitle and update end");
}

void aml_subtitle_close() {
    SUB_LOG("Close subtitle");

#ifndef RDK_COMPILE
    //clear draw
    if (g_OverlayDraw_Func) {
        int draw_type = parse_subtitle_type(sub_context.type);
        int h = (0x80 | draw_type) << 20;
        g_OverlayDraw_Func(0, 0, 0, 0, 9999, h, NULL);
    }
#endif

    std::lock_guard<std::mutex> lock(sub_mutex);

    if (sub_context.handle) {
#ifdef RDK_COMPILE
        amlsub_Close(sub_context.handle);
#else
        amlsub_RegistOnDataCB(sub_context.handle, nullptr);
        amlsub_RegistOnChannelUpdateCb(sub_context.handle, nullptr);
        amlsub_Close(sub_context.handle);
        amlsub_Destroy(sub_context.handle);
        sub_context.handle = nullptr;
#endif
        sub_context.paused = 0;
        sub_context.type = TYPE_NONE;
    }
    SUB_LOG("close subtitle end");
}

void aml_subtitle_pause() {
    std::lock_guard<std::mutex> lock(sub_mutex);

#ifndef RDK_COMPILE
    //clear draw
    if (g_OverlayDraw_Func) {
        int draw_type = parse_subtitle_type(sub_context.type);
        int h = (0x80 | draw_type) << 20;
        g_OverlayDraw_Func(0, 0, 0, 0, 9999, h, NULL);
    }
#endif

    sub_context.paused = 1;
}

void aml_subtitle_resume() {
    std::lock_guard<std::mutex> lock(sub_mutex);

    sub_context.paused = 0;
}

void aml_subtitle_ttx_control(int event) {
    AmlTeletextCtrlParam p;

    SUB_LOG("send teletext event %d", event);
    std::lock_guard<std::mutex> lock(sub_mutex);

    if (sub_context.handle) {
        p.event = (AmlTeletextEvent)event;
        p.magazine = -1;
        p.page = -1;
        p.regionid = -1;
        amlsub_TeletextControl(sub_context.handle, &p);
    }
}

void aml_subtitle_set(int type, int arg1, int arg2, int arg3) {
    std::lock_guard<std::mutex> lock(sub_mutex);

    if (sub_context.handle) {
        SUB_LOG("Update pip mode: (type:%d, data:%d)", type, arg1);
        amlsub_SetPip(sub_context.handle, (AmlSubtitlePipMode)(type + 1), arg1);
        SUB_LOG("Update pip mode: (type:%d, data:%d) end", type, arg1);
    }
}

void aml_subtitle_set_region_id(int region) {
    AmlTeletextCtrlParam p;

    SUB_LOG("set teletext region id %d", region);
    std::lock_guard<std::mutex> lock(sub_mutex);

    if (sub_context.handle) {
        p.event = TT_EVENT_SET_REGION_ID;
        p.magazine = -1;
        p.page = -1;
        p.regionid = region;
        amlsub_TeletextControl(sub_context.handle, &p);
    }

    SUB_LOG("set teletext region id %d end", region);
}

