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

#include "am_hdi_subtitle.h"
#include "aml_subtitle.h"

#define TAG  "subtitle_dtvkit"
#define SUB_LOG(x,...) DTV_LOG(ANDROID_LOG_INFO, TAG, x, ##__VA_ARGS__ )

static int mInited = 0;

static int parse_subtitle_type(int dvb_type) {
    int type = AM_SUBT_TYPE_DVB;
    switch (dvb_type) {
        default:
        case TYPE_DVB:
            type = AM_SUBT_TYPE_DVB;
            break;
        case TYPE_TTX:
        case TYPE_TTX_SUBTITLE:
            type = AM_SUBT_TYPE_TELETEXT;
            break;
        case TYPE_SCTE:
            type = AM_SUBT_TYPE_SCTE27;
            break;
        case TYPE_ARIB:
            type = AM_SUBT_TYPE_ARIB_B24;
            break;
        case TYPE_CC:
            type = AM_SUBT_TYPE_CLOSED_CAPTION;
            break;
    }
    return type;
}

void aml_subtitle_open(int type, aml_subtitle_param_t *p) {
    int sub_type;
    am_hdi_ttx_start_params_s hdi_sub_param;
    am_hdi_subt_source_params_t hdi_source_param;
    am_hdi_teletext_param_t hdi_teletext_param;

    if (p->pid == 0x1fff)
        return;

    if (!mInited) {
        int ret = am_hdi_subt_init();
        mInited = ret ? 0 : 1;
    }

    if (!mInited) {
        SUB_LOG("am_hdi_subt_init failed");
        return;
    }

    SUB_LOG("Open subtitle (type:%d,pid:%u,dmx:%d, arg1:%d, arg2:%d)",
        type, p->pid, p->dmx_id, p->arg1, p->arg2);

    sub_type = parse_subtitle_type(type);
    hdi_sub_param.m_pid = p->pid;
    hdi_sub_param.type = (am_hdi_subt_type_t)sub_type;
    hdi_sub_param.ancilpage_id = p->arg1;
    hdi_sub_param.comppage_id = p->arg2;

    if (p->dmx_id != -1) {
        hdi_source_param.m_demux_id = (am_hdi_dmx_id_t)(0x1 << p->dmx_id);
        am_hdi_subt_set_source(&hdi_source_param);
    }

    if (type == TYPE_CC) {
        hdi_sub_param.m_pid = 0;
        hdi_sub_param.channel_id = (p->dmx_id == -1) ? (p->pid | 0x100) : p->pid;
    }

    if (am_hdi_subt_start(&hdi_sub_param) == 0) {
        hdi_teletext_param.magazine = p->arg1;
        hdi_teletext_param.page = p->arg2;
        hdi_teletext_param.region_id = p->flag1;
        hdi_teletext_param.sub_page_no = -1;
        hdi_teletext_param.page_dir = 0;
        hdi_teletext_param.sub_page_dir = 0;
        hdi_teletext_param.flag = -1;
        if (type == TYPE_TTX_SUBTITLE) {
            hdi_teletext_param.event = AM_HDI_TT_EVENT_GO_TO_SUBTITLE;
            am_hdi_subtitle_teletext_control(&hdi_teletext_param);
        } else if (type == TYPE_TTX_SUBTITLE) {
            hdi_teletext_param.event = AM_HDI_TT_EVENT_GO_TO_PAGE;
            am_hdi_subtitle_teletext_control(&hdi_teletext_param);
        }

        SUB_LOG("Open subtitle, update initial decoder id(%d), sync_id(%d)",
            p->decoder_id, p->sync_id);
        am_subt_set_player_id(p->decoder_id);
        am_subt_set_sync_id(p->sync_id);
    } else {
        SUB_LOG("Open subtitle failed");
    }
}

void aml_subtitle_close() {
    SUB_LOG("Close subtitle");

    if (mInited) {
        am_hdi_subt_stop();
    }
}

void aml_subtitle_pause() {
    if (mInited) {
        am_hdi_subt_hide();
    }
}

void aml_subtitle_resume() {
    if (mInited) {
        am_hdi_subt_show();
    }
}

void aml_subtitle_ttx_control(int event) {
    am_hdi_teletext_param_t hdi_teletext_param;

    SUB_LOG("send teletext event %d", event);

    if (mInited) {
        hdi_teletext_param.event = (am_hdi_teletext_event_t)event;
        am_hdi_subtitle_teletext_control(&hdi_teletext_param);
    }
}

void aml_subtitle_set(int type, int arg1, int arg2, int arg3) {
    if (mInited) {
        SUB_LOG("Update pip mode: (type:%d, data:%d)", type, arg1);
        if (type == 0)
            am_subt_set_player_id(arg1);
        else if (type == 1)
            am_subt_set_sync_id(arg1);
    }
}

void aml_subtitle_set_region_id(int region) {
    am_hdi_teletext_param_t hdi_teletext_param;

    SUB_LOG("set teletext region id %d", region);

    if (mInited) {
        hdi_teletext_param.region_id = region;
        hdi_teletext_param.event = AM_HDI_TT_EVENT_SET_REGION_ID;
        am_hdi_subtitle_teletext_control(&hdi_teletext_param);
    }

}

