/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _FILTER_UTILS_
#define _FILTER_UTILS_
#include <jni.h>
#include <utils/Log.h>
#include <utils/RefBase.h>
#include <assert.h>
#include <string>

using namespace android;
using ::android::sp;
using ::android::RefBase;

#define FILTER_CONFIGURATION_CLASS "android/media/tv/tuner/filter/FilterConfiguration"
#define TS_FILTER_CONFIGURATION_CLASS "android/media/tv/tuner/filter/TsFilterConfiguration"
#define SECTION_SETTING_CLASS "android/media/tv/tuner/filter/SectionSettingsWithSectionBits"
#define SECTION_EVENT_CLASS "android/media/tv/tuner/filter/SectionEvent"

#define AV_SETTING_CLASS "android/media/tv/tuner/filter/AvSettings"

enum FILTER_MAIN_TYPE {
    MAIN_TYPE_UNDEFINED = 0,
    MAIN_TYPE_TS = 1,
    MAIN_TYPE_MMTP = 2,
    MAIN_TYPE_IP = 4,
    MAIN_TYPE_TLV = 8,
    MAIN_TYPE_ALP = 16,
};

enum FILTER_SUB_TYPE {
    SUBTYPE_UNDEFINED = 0,
    SUBTYPE_SECTION = 1,
    SUBTYPE_PES = 2,
    SUBTYPE_AUDIO = 3,
    SUBTYPE_VIDEO = 4,
    SUBTYPE_DOWNLOAD = 5,
    SUBTYPE_RECORD = 6,
    SUBTYPE_TS = 7,
    SUBTYPE_PCR = 8,
    SUBTYPE_TEMI = 9,
    SUBTYPE_MMTP = 10,
    SUBTYPE_NTP = 11,
    SUBTYPE_IP_PAYLOAD = 12,
    SUBTYPE_IP = 13,
    SUBTYPE_PAYLOAD_THROUGH = 14,
    SUBTYPE_TLV = 15,
    SUBTYPE_PTP = 16,
};

enum FILTER_STATUS {
    STATUS_UNDEFINED = 0,
    STATUS_DATA_READY = 1,
    STATUS_LOW_WATER = 2,
    STATUS_HIGH_WATER = 4,
    STATUS_OVERFLOW = 8,
};

enum VIDEO_STREAM_TYPE {
    VIDEO_STREAM_TYPE_UNDEFINED,
    VIDEO_STREAM_TYPE_RESERVED,   // ITU-T | ISO/IEC Reserved
    VIDEO_STREAM_TYPE_MPEG1,      // ISO/IEC 11172
    VIDEO_STREAM_TYPE_MPEG2,      // ITU-T Rec.H.262 and ISO/IEC 13818-2
    VIDEO_STREAM_TYPE_MPEG4P2,    // ISO/IEC 14496-2 (MPEG-4 H.263 based video)
    VIDEO_STREAM_TYPE_AVC,        // ITU-T Rec.H.264 and ISO/IEC 14496-10
    VIDEO_STREAM_TYPE_HEVC,       // ITU-T Rec. H.265 and ISO/IEC 23008-2
    VIDEO_STREAM_TYPE_VC1,        // Microsoft VC.1
    VIDEO_STREAM_TYPE_VP8,        // Google VP8
    VIDEO_STREAM_TYPE_VP9,        // Google VP9
    VIDEO_STREAM_TYPE_AV1,        // AOMedia Video 1
    VIDEO_STREAM_TYPE_AVS,        // Chinese Standard
    VIDEO_STREAM_TYPE_AVS2,       // New Chinese Standard
};

enum AUDIO_STREAM_TYPE {
    AUDIO_STREAM_TYPE_UNDEFINED,
    AUDIO_STREAM_TYPE_PCM,        // Uncompressed Audio
    AUDIO_STREAM_TYPE_MP3,        // MPEG Audio Layer III versions
    AUDIO_STREAM_TYPE_MPEG1,      // ISO/IEC 11172 Audio
    AUDIO_STREAM_TYPE_MPEG2,      // ISO/IEC 13818-3
    AUDIO_STREAM_TYPE_MPEGH,      // ISO/IEC 23008-3 (MPEG-H Part 3)
    AUDIO_STREAM_TYPE_AAC,        //ISO/IEC 14496-3
    AUDIO_STREAM_TYPE_AC3,        //Dolby Digital
    AUDIO_STREAM_TYPE_EAC3,       // Dolby Digital Plus
    AUDIO_STREAM_TYPE_AC4,        //Dolby AC-4
    AUDIO_STREAM_TYPE_DTS,        //Basic DTS
    AUDIO_STREAM_TYPE_DTS_HD,     //High Resolution DTS
    AUDIO_STREAM_TYPE_WMA,        //Windows Media Audio
    AUDIO_STREAM_TYPE_OPUS,       // Opus Interactive Audio Codec
    AUDIO_STREAM_TYPE_VORBIS,     // VORBIS Interactive Audio Codec
    AUDIO_STREAM_TYPE_DRA,        // SJ/T 11368-2006
    AUDIO_STREAM_TYPE_AAC_ADTS,   // AAC with ADTS (Audio Data Transport Format).
    AUDIO_STREAM_TYPE_AAC_LATM,   // AAC with ADTS with LATM (Low-overhead MPEG-4 Audio Transport Multiplex).
    AUDIO_STREAM_TYPE_AAC_HE_ADTS,// High-Efficiency AAC (HE-AAC) with ADTS (Audio Data Transport Format).
    AUDIO_STREAM_TYPE_AAC_HE_LATM,// High-Efficiency AAC (HE-AAC) with LATM (Low-overhead MPEG-4 Audio Transport Multiplex).
};

typedef struct {
    bool crc_enable;
    bool is_repeat;
    bool is_raw;
    int filter_length;
    int mask_length;
    int mode_length;
    char* filter;
    char* mask;
    char* mode;
}Section_Setting;

/*
typedef struct {
    int type;
    int pid;
    Section_Setting section_setting;
}TS_Filter_Configuration;
*/

typedef struct {
    int tableId;
    int version;
    int sectionNum;
    int dataLength;
}Section_Event;

typedef struct {
    bool is_passthrough;
    bool is_audio;
    bool is_use_secure_memory;
    int audio_strem_type;
    int video_strem_type;
}AV_Setting;

typedef struct {
    int type;
    int pid;
    union {
        Section_Setting section_setting;
        AV_Setting av_setting;
    }setting;
}TS_Filter_Configuration;

/**
 * get Section TsFilterConfiguration jobject from TS_FILTER_CONFIGURATION structure
 *
 * @param tsFilterConfiguration java class TsFilterConfiguration map structure TS_FILTER_CONFIGURATION.
 * @return TsFilterConfiguration jobject for section filter conifg or null .
 */
jobject filter_utils_getSectionTsFilterConfiguration(JNIEnv *env, TS_Filter_Configuration tsFilterConfiguration);

/**
 * get AV TsFilterConfiguration jobject from TS_FILTER_CONFIGURATION structure
 *
 * @param tsFilterConfiguration java class TsFilterConfiguration map structure TS_FILTER_CONFIGURATION.
 * @return TsFilterConfiguration jobject for av filter conifg or null.
 */
jobject filter_utils_getAVTsFilterConfiguration(JNIEnv *env, TS_Filter_Configuration tsFilterConfiguration);



bool filter_utils_getSectionEvent(JNIEnv *env, jobject sectionEventObject, Section_Event *pSectionEvent);

#endif/*_FILTER_UTILS_*/

