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

#ifndef _ATSC_FRONTEND_SETTING_UTILS_
#define _ATSC_FRONTEND_SETTING_UTILS_

#include <jni.h>

using namespace android;

typedef enum {
    ATSC_MODULATION_UNDEFINED = 0,
    ATSC_MODULATION_AUTO = 1 << 0,
    ATSC_MODULATION_8VSB = 1 << 2,
    ATSC_MODULATION_16VSB = 1 << 3,
} ATSC_MODULATION;

typedef struct
{
    unsigned long frequency;
    int modulation;
} Atsc_Frontend_Settings;

/**
 * get Atsc frontend settings jobject
 *
 * @param atscFrontendSettings java class AtscFrontendSettings map structure Atsc_Frontend_Settings.
 * @return analog frontend settings jobject or null.
 */

jobject atsc_utils_getAtscFrontendSettingsObject(JNIEnv *env, Atsc_Frontend_Settings atscFrontendSettings);

#endif/*_ATSC_FRONTEND_SETTING_UTILS_*/