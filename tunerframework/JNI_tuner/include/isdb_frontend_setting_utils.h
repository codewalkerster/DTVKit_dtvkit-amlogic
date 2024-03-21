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

#ifndef _ISDB_FRONTEND_SETTING_UTILS_
#define _ISDB_FRONTEND_SETTING_UTILS_
#include <jni.h>
#include <utils/Log.h>
#include <utils/RefBase.h>
#include <assert.h>
#include <string>

using namespace android;
using ::android::RefBase;
using ::android::sp;

enum ISDBT_MODULATION
{
    ISDBT_MODULATION_UNDEFINED = 0,
    ISDBT_MODULATION_AUTO = 1 << 0,
    ISDBT_MODULATION_MOD_DQPSK = 1 << 1,
    ISDBT_MODULATION_MOD_QPSK = 1 << 2,
    ISDBT_MODULATION_MOD_16QAM = 1 << 3,
    ISDBT_MODULATION_MOD_64QAM = 1 << 4,
};

enum ISDBT_MODE
{
    ISDBT_MODE_UNDEFINED = 0,
    ISDBT_MODE_AUTO = 1 << 0,
    ISDBT_MODE_1 = 1 << 1,
    ISDBT_MODE_2 = 1 << 2,
    ISDBT_MODE_3 = 1 << 3,
};

enum ISDBT_BANDWIDTH
{
    ISDBT_BANDWIDTH_UNDEFINED = 0,
    ISDBT_BANDWIDTH_AUTO = 1 << 0,
    ISDBT_BANDWIDTH_8M = 1 << 1,
    ISDBT_BANDWIDTH_7M = 1 << 2,
    ISDBT_BANDWIDTH_6M = 1 << 3,
};

enum ISDBT_PARTIAL_RECEPTION_FLAG
{
    ISDBT_PARTIAL_RECEPTION_FLAG_UNDEFINED = 0,
    ISDBT_PARTIAL_RECEPTION_FLAG_AUTO = 1 << 0,
    ISDBT_PARTIAL_RECEPTION_FLAG_FALSE = 1 << 1,
    ISDBT_PARTIAL_RECEPTION_FLAG_TRUE = 1 << 2,
};

enum ISDBT_TIME_INTERLEAVE_MODE
{
    ISDBT_TIME_INTERLEAVE_MODE_UNDEFINED = 0,
    ISDBT_TIME_INTERLEAVE_MODE_AUTO = 1 << 0,
    ISDBT_TIME_INTERLEAVE_MODE_1_0 = 1 << 1,
    ISDBT_TIME_INTERLEAVE_MODE_1_4 = 1 << 2,
    ISDBT_TIME_INTERLEAVE_MODE_1_8 = 1 << 3,
    ISDBT_TIME_INTERLEAVE_MODE_1_16 = 1 << 4,
    ISDBT_TIME_INTERLEAVE_MODE_2_0 = 1 << 5,
    ISDBT_TIME_INTERLEAVE_MODE_2_2 = 1 << 6,
    ISDBT_TIME_INTERLEAVE_MODE_2_4 = 1 << 7,
    ISDBT_TIME_INTERLEAVE_MODE_2_8 = 1 << 8,
    ISDBT_TIME_INTERLEAVE_MODE_3_0 = 1 << 9,
    ISDBT_TIME_INTERLEAVE_MODE_3_1 = 1 << 10,
    ISDBT_TIME_INTERLEAVE_MODE_3_2 = 1 << 11,
    ISDBT_TIME_INTERLEAVE_MODE_3_4 = 1 << 12,
};

typedef struct
{
    int modulation;
    int timeInterleaveMode;
    int codeRate;
    int numOfSegments;
} IsdbtLayerSettings;

typedef struct
{
    long frequency;
    int bandwidth;
    int mode;
    int guardInterval;
    int serviceAreaId;
    int partialReceptionFlag;
    IsdbtLayerSettings layerSettings[1];
} Isdbt_Frontend_Settings;

/**
 * get Isdbt frontend settings jobject
 *
 * @param Isdbt_Frontend_Settings java class IsdbtFrontendSettings map structure Isdbt_Frontend_Settings.
 * @return Isdbt frontend settings jobject or null.
 */

jobject isdb_utils_getIsdbtFrontendSettingsObject(JNIEnv *env, Isdbt_Frontend_Settings dvbtFrontendSettings);

#endif /*_ISDB_FRONTEND_SETTING_UTILS_*/