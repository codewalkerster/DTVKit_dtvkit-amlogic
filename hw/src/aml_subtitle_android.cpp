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
}
#include "dtv_log.h"

#include "SubtitleServerClient.h"
#include "SubtitleNativeAPI.h"
#include "aml_subtitle.h"

#define TAG  "subtitle_dtvkit"
#define SUB_LOG(x,...) DTV_LOG(ANDROID_LOG_INFO, TAG, x, ##__VA_ARGS__ )


#define AML_SUB_PARSE_TYPE_OFFSET 4
#define AML_SUB_DEMUX_SOURCE 4

static subtitle_context_t sub_context = {nullptr, 0, TYPE_NONE};
static sp<amlogic::SubtitleServerClient> sub_handle = nullptr;

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

class SubtitleDataListenerImpl : public amlogic::SubtitleListener {
    public:
        SubtitleDataListenerImpl() {}
        ~SubtitleDataListenerImpl() {}

        virtual void onSubtitleEvent(const char *data, int size, int parserType,
                int x, int y, int width, int height,
                int videoWidth, int videoHeight, int cmd,int objectSegmetnId) {
            unsigned char extra;
            int h_extra;
            int width_fixed = width;
            int height_fixed = height;

            //SUB_LOG("on_subtitle_data: %p (type:%d, show:%d, [%d,%d,%d,%d] in (%d,%d)",
            //    data, parserType, cmd, x, y, width, height, videoWidth, videoHeight);

            if (cmd == 1 && size == 0) {
                SUB_LOG("skip show no size data");
                return;
            }

            if (parserType == TYPE_SUBTITLE_ARIB_B24 ||
                    parserType == TYPE_SUBTITLE_CLOSED_CAPTION ||
                    parserType == TYPE_SUBTITLE_TTML ||
                    parserType == TYPE_SUBTITLE_SMPTE_TTML) {
                width_fixed = size;
                height_fixed = 1;//mean string type
            }

            if (data == NULL || size == 0) {
                width_fixed = 0;
                height_fixed = 0;
            }

            //add extra info for subtitle
            //bit 27 - bit 20
            //extra 0x80
            //show 0x40
            //type 0x0- 0x3f
            if (g_OverlayDraw_Func && parserType >= TYPE_SUBTITLE_DVB) {
                extra = 0x80 | (cmd << 6) | parserType;
                h_extra = extra << 20 | videoHeight;
                g_OverlayDraw_Func(
                    width_fixed,
                    height_fixed,
                    x,
                    y,
                    videoWidth,
                    h_extra,
                    (const unsigned char *)data);
            }
        };

        virtual void onSubtitleDataEvent(int event, int id) {
            SUB_LOG("on_cc_channel_event: (event:%d, id:%d)", event, id);
        };

        void onSubtitleAvail(int avail) {};
        void onSubtitleAfdEvent(int dec_id, int afd) {};
        void onSubtitleDimension(int width, int height) {};
        void onSubtitleLanguage(std::string lang) {};
        void onSubtitleInfo(int what, int extra) {};
        void onMixVideoEvent(int val) {
            SUB_LOG("onMixVideoEvent: %d", val);
            if (g_OverlayDraw_Func) {
                g_OverlayDraw_Func(0, val, 0, 0, 0, 0x8000000, NULL);
            }
        };
        virtual void onServerDied() {
            SUB_LOG("onServerDied from subtitle!");
        };
        void onSubtitleUIEvent(int uiCmd, const std::vector<int> &params) {};
};

void aml_subtitle_open(int type, aml_subtitle_param_t *p) {
    int dmx_id;
    int atv_flag;
    int io_type;
    int sub_type;

    if (p->pid == 0x1fff)
        return;

    std::lock_guard<std::mutex> lock(sub_mutex);

    if (!sub_handle) {
        sub_handle = new amlogic::SubtitleServerClient(
                        false, new SubtitleDataListenerImpl(), OpenType::TYPE_APPSDK);
    } else {
        sub_handle->close();
    }
    sub_context.type = (subtitle_type)type;

    SUB_LOG("Open subtitle (type:%d,pid:%u,dmx:%d, arg1:%d, arg2:%d)",
        type, p->pid, p->dmx_id, p->arg1, p->arg2);

    dmx_id = p->dmx_id < 0 ? 0 : p->dmx_id;
    atv_flag = (p->dmx_id == -1) ? 1 : 0;
    io_type = (dmx_id << 16) | AML_SUB_DEMUX_SOURCE;

    sub_type = type + AML_SUB_PARSE_TYPE_OFFSET;
    if (type == TYPE_TTX)
        sub_type = TYPE_TTX_SUBTITLE + AML_SUB_PARSE_TYPE_OFFSET;

    sub_handle->setSubType(sub_type);

    if (type == TYPE_CC)
        sub_handle->selectCcChannel(p->pid | (atv_flag << 8));
    else
        sub_handle->setSubPid(p->pid, p->flag, p->flag);

    sub_handle->setSecureLevel(p->ca_flag > 0);

    if (sub_handle->open("", io_type)) {
        if (type == TYPE_TTX)
            sub_handle->ttControl(TT_EVENT_GO_TO_PAGE, p->arg1, p->arg2, p->flag1, 0);
        else if (type == TYPE_TTX_SUBTITLE)
            sub_handle->ttControl(TT_EVENT_GO_TO_SUBTITLE, p->arg1, p->arg2, -1, 0);
        else if (type == TYPE_DVB) {
            sub_handle->setAncillaryPageId(p->arg1);
            sub_handle->setCompositionPageId(p->arg2);
        }
    }

    sub_handle->setPipId(MODE_SUBTITLE_PIP_PLAYER, p->decoder_id);
    sub_handle->setPipId(MODE_SUBTITLE_PIP_MEDIASYNC, p->sync_id);
}

void aml_subtitle_close() {
    int draw_type;
    int h;

    SUB_LOG("Close subtitle");

    //clear draw
    if (g_OverlayDraw_Func) {
        draw_type = parse_subtitle_type(sub_context.type);
        h = (0x80 | draw_type) << 20;
        g_OverlayDraw_Func(0, 0, 0, 0, 9999, h, NULL);
    }

    std::lock_guard<std::mutex> lock(sub_mutex);

    if (sub_handle) {
        sub_handle->close();
        sub_handle = nullptr;
        sub_context.handle = nullptr;
        sub_context.paused = 0;
        sub_context.type = TYPE_NONE;
    }
}

void aml_subtitle_pause() {
    int draw_type;
    int h;

    std::lock_guard<std::mutex> lock(sub_mutex);

    //clear draw
    if (g_OverlayDraw_Func) {
        draw_type = parse_subtitle_type(sub_context.type);
        h = (0x80 | draw_type) << 20;
        g_OverlayDraw_Func(0, 0, 0, 0, 9999, h, NULL);
    }

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

    if (sub_handle) {
        sub_handle->ttControl(event, -1, -1, -1, -1);
    }
}

void aml_subtitle_set(int type, int arg1, int arg2, int arg3) {
    std::lock_guard<std::mutex> lock(sub_mutex);

    if (sub_handle) {
        SUB_LOG("Update pip mode: (type:%d, data:%d)", type, arg1);
        sub_handle->setPipId(type + 1, arg1);
    }
}

void aml_subtitle_set_region_id(int region) {
    SUB_LOG("set teletext region id %d", region);
    std::lock_guard<std::mutex> lock(sub_mutex);

    if (sub_handle) {
        sub_handle->ttControl(TT_EVENT_SET_REGION_ID, -1, -1, region, -1);
    }
}

