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

#ifndef _AML_SUBTITLE_DTV_H_
#define _AML_SUBTITLE_DTV_H_

#ifdef __cplusplus
extern "C" {
#endif

#define CAPTION_CC_1 1
#define CAPTION_CS_1 9
#define CAPTION_MAX  14

#define SUBTITLE_CC_EVENT_CHANNELS_CHANGED -2

typedef enum {
    TYPE_NONE,
    TYPE_DVB,
    TYPE_TTX_SUBTITLE,
    TYPE_SCTE,
    TYPE_TTX,
    TYPE_ARIB,
    TYPE_CC
} subtitle_type;

typedef struct {
    unsigned int pid;//pid or cc channel
    int dmx_id;
    int arg1;//ancillary or magazing
    int arg2;//composition or page
    int flag;//to decide whether restart vbi
    int flag1;//subtitle region id
    int decoder_id;
    int sync_id;
    unsigned char ca_flag;
    int reserved;
} aml_subtitle_param_t;

typedef struct {
    void* handle;
    int paused;
    subtitle_type type;
} subtitle_context_t;

typedef struct {
    int type;
    int data;
    int arg1;
    int arg2;
    void* user_data;
} subtitle_data_event_t;

void aml_subtitle_open(int type, aml_subtitle_param_t *p);
void aml_subtitle_close();
void aml_subtitle_pause();
void aml_subtitle_resume();
void aml_subtitle_ttx_control(int event);
void aml_subtitle_set(int type, int arg1, int arg2, int arg3);
void aml_subtitle_set_region_id(int region);

#ifdef __cplusplus
}
#endif

#endif /*#define _AML_SUBTITLE_DTV_H_*/

