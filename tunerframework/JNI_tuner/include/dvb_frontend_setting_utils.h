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

#ifndef _DVB_FRONTEND_SETTING_UTILS_
#define _DVB_FRONTEND_SETTING_UTILS_
#include <jni.h>
#include <utils/Log.h>
#include <utils/RefBase.h>
#include <assert.h>
#include <string>

using namespace android;
using ::android::sp;
using ::android::RefBase;

#define DVBT_FRONTEND_SETTING_CLASS "android/media/tv/tuner/frontend/DvbtFrontendSettings"
#define DVBC_FRONTEND_SETTING_CLASS "android/media/tv/tuner/frontend/DvbcFrontendSettings"
#define DVBS_FRONTEND_SETTING_CLASS "android/media/tv/tuner/frontend/DvbsFrontendSettings"

enum DVBT_TRANSMISSION_MODE {
    DVBT_TRANSMISSION_MODE_UNDEFINED = 0,
    DVBT_TRANSMISSION_MODE_AUTO = 1 << 0,
    DVBT_TRANSMISSION_MODE_2K = 1 << 1,
    DVBT_TRANSMISSION_MODE_8K = 1 << 2,
    DVBT_TRANSMISSION_MODE_4K = 1 << 3,
    DVBT_TRANSMISSION_MODE_1K = 1 << 4,
    DVBT_TRANSMISSION_MODE_16K = 1 << 5,
    DVBT_TRANSMISSION_MODE_32K = 1 << 6,
};

enum DVBT_BANDWIDTH {
    DVBT_BANDWIDTH_UNDEFINED = 0,
    DVBT_BANDWIDTH_AUTO = 1 << 0,
    DVBT_BANDWIDTH_8MHZ = 1 << 1,
    DVBT_BANDWIDTH_7MHZ = 1 << 2,
    DVBT_BANDWIDTH_6MHZ = 1 << 3,
    DVBT_BANDWIDTH_5MHZ = 1 << 4,
    DVBT_BANDWIDTH_1_7MHZ = 1 << 5,
    DVBT_BANDWIDTH_10MHZ = 1 << 6,
};

enum DVBT_STANDARD {
    DVBT_STANDARD_UNDEFINED = 0,
    DVBT_STANDARD_AUTO = 1 << 0,
    DVBT_STANDARD_T = 1 << 1,
    DVBT_STANDARD_T2 = 1 << 2,
};

enum DVBT_CONSTELLATION {
    DVBT_CONSTELLATION_UNDEFINED = 0,
    DVBT_CONSTELLATION_AUTO = 1 << 0,
    DVBT_CONSTELLATION_QPSK = 1 << 1,
    DVBT_CONSTELLATION_16QAM = 1 << 2,
    DVBT_CONSTELLATION_64QAM = 1 << 3,
    DVBT_CONSTELLATION_256QAM = 1 << 4,
};

enum DVBT_HIERARCHY {
    DVBT_HIERARCHY_UNDEFINED = 0,
    DVBT_HIERARCHY_AUTO = 1 << 0,
    DVBT_HIERARCHY_NON_NATIVE = 1 << 1,
    DVBT_HIERARCHY_1_NATIVE = 1 << 2,
    DVBT_HIERARCHY_2_NATIVE = 1 << 3,
    DVBT_HIERARCHY_4_NATIVE = 1 << 4,
    DVBT_HIERARCHY_NON_INDEPTH = 1 << 5,
    DVBT_HIERARCHY_1_INDEPTH = 1 << 6,
    DVBT_HIERARCHY_2_INDEPTH = 1 << 7,
    DVBT_HIERARCHY_4_INDEPTH = 1 << 8,
};

enum DVBT_CODERATE {
    DVBT_CODERATE_UNDEFINED = 0,
    DVBT_CODERATE_AUTO = 1 << 0,
    DVBT_CODERATE_1_2 = 1 << 1,
    DVBT_CODERATE_2_3 = 1 << 2,
    DVBT_CODERATE_3_4 = 1 << 3,
    DVBT_CODERATE_5_6 = 1 << 4,
    DVBT_CODERATE_7_8 = 1 << 5,
    DVBT_CODERATE_3_5 = 1 << 6,
    DVBT_CODERATE_4_5 = 1 << 7,
    DVBT_CODERATE_6_7 = 1 << 8,
    DVBT_CODERATE_8_9 = 1 << 9,
};

enum DVBT_GUARD_INTERVAL {
    DVBT_GUARD_INTERVAL_UNDEFINED = 0,
    DVBT_GUARD_INTERVAL_AUTO = 1 << 0,
    DVBT_GUARD_INTERVAL_1_32 = 1 << 1,
    DVBT_GUARD_INTERVAL_1_16 = 1 << 2,
    DVBT_GUARD_INTERVAL_1_8 = 1 << 3,
    DVBT_GUARD_INTERVAL_1_4 = 1 << 4,
    DVBT_GUARD_INTERVAL_1_128 = 1 << 5,
    DVBT_GUARD_INTERVAL_19_128 = 1 << 6,
    DVBT_GUARD_INTERVAL_19_256 = 1 << 7,
};

enum DVBT_PLP_MODE {
    DVBT_PLP_MODE_UNDEFINED = 0,
    DVBT_PLP_MODE_AUTO = 1 << 0,
    DVBT_PLP_MODE_MANUAL = 1 << 1,
};

enum DVBC_MODULATION {
    DVBC_MODULATION_UNDEFINED = 0,
    DVBC_MODULATION_AUTO = 1 << 0,
    DVBC_MODULATION_MOD_16QAM = 1 << 1,
    DVBC_MODULATION_MOD_32QAM = 1 << 2,
    DVBC_MODULATION_MOD_64QAM = 1 << 3,
    DVBC_MODULATION_MOD_128QAM = 1 << 4,
    DVBC_MODULATION_MOD_256QAM = 1 << 5,
};

enum DVBC_OUTER_FEC {
    DVBC_OUTER_FEC_UNDEFINED = 0,
    DVBC_OUTER_FEC_OUTER_FEC_NONE = 1 << 0,
    DVBC_OUTER_FEC_OUTER_FEC_RS = 1 << 1,
};

enum DVBC_ANNEX {
    DVBC_ANNEX_UNDEFINED = 0,
    DVBC_ANNEX_A = 1 << 0,
    DVBC_ANNEX_B = 1 << 1,
    DVBC_ANNEX_C = 1 << 2,
};

enum DVBC_SPECTRAL_INVERSION {
    DVBC_SPECTRAL_INVERSION_UNDEFINED = 0,
    DVBC_SPECTRAL_INVERSION_NORMAL = 1 << 0,
    DVBC_SPECTRAL_INVERSION_INVERTED = 1 << 1,
};

enum DVBS_SCAN_TYPE {
    DVBS_SCAN_TYPE_UNDEFINED = 0,
    DVBS_SCAN_TYPE_DIRECT    = 1,
    DVBS_SCAN_TYPE_DISEQC    = 2,
    DVBS_SCAN_TYPE_UNICABLE  = 3,
    DVBS_SCAN_TYPE_JESS      = 4,
};

enum DVBS_MODULATION {
    DVBS_MODULATION_UNDEFINED    = 0,
    DVBS_MODULATION_AUTO         = 1 << 0,
    DVBS_MODULATION_MOD_QPSK     = 1 << 1,
    DVBS_MODULATION_MOD_8PSK     = 1 << 2,
    DVBS_MODULATION_MOD_16QAM    = 1 << 3,
    DVBS_MODULATION_MOD_16PSK    = 1 << 4,
    DVBS_MODULATION_MOD_32PSK    = 1 << 5,
    DVBS_MODULATION_MOD_ACM      = 1 << 6,
    DVBS_MODULATION_MOD_8APSK    = 1 << 7,
    DVBS_MODULATION_MOD_16APSK   = 1 << 8,
    DVBS_MODULATION_MOD_32APSK   = 1 << 9,
    DVBS_MODULATION_MOD_64APSK   = 1 << 10,
    DVBS_MODULATION_MOD_128APSK  = 1 << 11,
    DVBS_MODULATION_MOD_256APSK  = 1 << 12,
    DVBS_MODULATION_MOD_RESERVED = 1 << 13,
};

enum DVBS_ROLLOFF {
    DVBS_ROLLOFF_UNDEFINED = 0,
    DVBS_ROLLOFF_0_35      = 1,
    DVBS_ROLLOFF_0_25      = 2,
    DVBS_ROLLOFF_0_20      = 3,
    DVBS_ROLLOFF_0_15      = 4,
    DVBS_ROLLOFF_0_10      = 5,
    DVBS_ROLLOFF_0_5       = 6,
};

enum DVBS_PILOT {
    DVBS_PILOT_UNDEFINED = 0,
    DVBS_PILOT_ON        = 1,
    DVBS_PILOT_OFF       = 2,
    DVBS_PILOT_AUTO      = 3,
};

enum DVBS_STANDARD {
    DVBS_STANDARD_AUTO = 1 << 0,
    DVBS_STANDARD_S    = 1 << 1,
    DVBS_STANDARD_S2   = 2 << 2,
    DVBS_STANDARD_S2X  = 3 << 3,
};

enum DVBS_VCM_MODE {
    DVBS_VCM_MODE_UNDEFINED = 0,
    DVBS_VCM_MODE_AUTO      = 1,
    DVBS_VCM_MODE_MANUAL    = 2,
};

enum DVBS_LNB_VOLTAGE {
    DVBS_LNB_VOLTAGE_NONE = 0,
    DVBS_LNB_VOLTAGE_5V  = 1,
    DVBS_LNB_VOLTAGE_11V = 2,
    DVBS_LNB_VOLTAGE_12V = 3,
    DVBS_LNB_VOLTAGE_13V = 4,
    DVBS_LNB_VOLTAGE_14V = 5,
    DVBS_LNB_VOLTAGE_15V = 6,
    DVBS_LNB_VOLTAGE_18V = 7,
    DVBS_LNB_VOLTAGE_19V = 8,
};

enum DVBS_LNB_TONE {
    DVBS_LNB_TONE_NONE        = 0,
    DVBS_LNB_TONE_CONTINUOUS  = 1,
};

enum DVBS_LNB_POSITION {
    DVBS_LNB_POSITION_UNDEFINED  = 0,
    DVBS_LNB_POSITION_POSITION_A = 1,
    DVBS_LNB_POSITION_POSITION_B = 2,
};

enum DVBS_INNER_FEC {
    DVBS_FEC_UNDEFINED = 0,
    DVBS_FEC_AUTO      = 1 << 0,
    DVBS_FEC_1_2       = 1 << 1,
    DVBS_FEC_1_3       = 1 << 2,
    DVBS_FEC_1_4       = 1 << 3,
    DVBS_FEC_1_5       = 1 << 4,
    DVBS_FEC_2_3       = 1 << 5,
    DVBS_FEC_2_5       = 1 << 6,
    DVBS_FEC_2_9       = 1 << 7,
    DVBS_FEC_3_4       = 1 << 8,
    DVBS_FEC_3_5       = 1 << 9,
    DVBS_FEC_4_5       = 1 << 10,
    DVBS_FEC_4_15      = 1 << 11,
    DVBS_FEC_5_6       = 1 << 12,
    DVBS_FEC_5_9       = 1 << 13,
    DVBS_FEC_6_7       = 1 << 14,
    DVBS_FEC_7_8       = 1 << 15,
    DVBS_FEC_7_9       = 1 << 16,
    DVBS_FEC_7_15      = 1 << 17,
    DVBS_FEC_8_9       = 1 << 18,
    DVBS_FEC_8_15      = 1 << 19,
    DVBS_FEC_9_10      = 1 << 20,
    DVBS_FEC_9_20      = 1 << 21,
    DVBS_FEC_11_15     = 1 << 22,
    DVBS_FEC_11_20     = 1 << 23,
    DVBS_FEC_11_45     = 1 << 24,
    DVBS_FEC_13_18     = 1 << 25,
    DVBS_FEC_13_45     = 1 << 26,
    DVBS_FEC_14_45     = 1 << 27,
    DVBS_FEC_23_36     = 1 << 28,
    DVBS_FEC_25_36     = 1 << 29,
    DVBS_FEC_26_45     = 1 << 30,
    DVBS_FEC_28_45     = 1 << 31,
    DVBS_FEC_29_45     = 1 << 32,
    DVBS_FEC_31_45     = 1 << 33,
    DVBS_FEC_32_45     = 1 << 34,
    DVBS_FEC_77_90     = 1 << 35,
};

typedef struct
{
    int frequency;
    int transmissionMode; /*DVBT_TRANSMISSION_MODE*/
    int bandwidth;        /*DVBT_BANDWIDTH*/
    int constellation;    /*DVBT_CONSTELLATION*/
    int hierarchy;        /*DVBT_HIERARCHY*/
    int hpCodeRate;       /*DVBT_CODERATE*/
    int lpCodeRate;       /*DVBT_CODERATE*/
    int guardInterval;    /*DVBT_GUARD_INTERVAL*/
    int standard;         /*DVBT_STANDARD*/
    int plpMode;          /*DVBT_PLP_MODE*/
    int plpId;
    int plpGroupId;
    bool isHighPriority;
    bool isMiso;
}Dvbt_Frontend_Settings;

typedef struct
{
    int frequency;
    int modulation;        /*DVBC_MODULATION*/
    int symbolRate;
    int outerFec;          /*DVBC_OUTER_FEC*/
    int annex;             /*DVBC_ANNEX*/
    int spectralInversion; /*DVBC_SPECTRAL_INVERSION*/
    long innerFec;
}Dvbc_Frontend_Settings;

typedef struct
{
    long fec;
    bool isLinear;
    bool isShortFrames;
    int bitsPer1000Symbol;
}Dvbs_Code_Rate;

typedef struct
{
    bool isDiseqcRxMessage;
    int frequency;
    int modulation;
    int symbol_rate;
    int roll_off;
    int pilot;
    int input_streamId;
    int standard;
    int vcm;
    int scan_type;
    unsigned long end_frequency;
    Dvbs_Code_Rate code_rate;
}Dvbs_Frontend_Settings;

/**
 * get dvbt frontend settings jobject
 *
 * @param dvbtFrontendSettings java class DvbtFrontendSettings map structure Dvbt_Frontend_Settings.
 * @return dvbt frontend settings jobject or null.
 */

jobject dvb_utils_getDvbtFrontendSettingsObject(JNIEnv *env, Dvbt_Frontend_Settings dvbtFrontendSettings);

/**
 * get dvbc frontend settings jobject
 *
 * @param dvbcFrontendSettings java class DvbcFrontendSettings map structure Dvbc_Frontend_Settings.
 * @return dvbc frontend settings jobject or null.
 */
jobject dvb_utils_getDvbcFrontendSettingsObject(JNIEnv *env, Dvbc_Frontend_Settings dvbcFrontendSettings);

/**
 * get dvbs frontend settings jobject
 *
 * @param dvbsFrontendSettings java class DvbsFrontendSettings map structure Dvbs_Frontend_Settings.
 * @return dvbs frontend settings jobject or null.
 */
jobject dvb_utils_getDvbsFrontendSettingsObject(JNIEnv *env, Dvbs_Frontend_Settings dvbsFrontendSettings);
#endif/*_DVB_FRONTEND_SETTING_UTILS_*/

