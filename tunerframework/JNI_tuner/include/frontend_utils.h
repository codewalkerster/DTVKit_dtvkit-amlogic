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

#ifndef _FRONTEND_UTILS_
#define _FRONTEND_UTILS_
#include <jni.h>
#include <utils/Log.h>
#include <utils/RefBase.h>
#include <assert.h>
#include <string>
#include <vector>

using namespace android;
using ::android::sp;
using ::android::RefBase;

#define FRONTEND_STATUS_CLASS "android/media/tv/tuner/frontend/FrontendStatus"

enum FRONTEND_STATUS_TYPE {
    FRONTEND_STATUS_TYPE_DEMOD_LOCK = 0,
    FRONTEND_STATUS_TYPE_SNR = 1,
    FRONTEND_STATUS_TYPE_BER = 2,
    FRONTEND_STATUS_TYPE_PER = 3,
    FRONTEND_STATUS_TYPE_PRE_BER = 4,
    FRONTEND_STATUS_TYPE_SIGNAL_QUALITY = 5,
    FRONTEND_STATUS_TYPE_SIGNAL_STRENGTH = 6,
    FRONTEND_STATUS_TYPE_SYMBOL_RATE = 7,
    FRONTEND_STATUS_TYPE_FEC = 8,
    FRONTEND_STATUS_TYPE_MODULATION = 9,
    FRONTEND_STATUS_TYPE_SPECTRAL = 10,
    FRONTEND_STATUS_TYPE_LNB_VOLTAGE = 11,
    FRONTEND_STATUS_TYPE_PLP_ID = 12,
    FRONTEND_STATUS_TYPE_EWBS = 13,
    FRONTEND_STATUS_TYPE_AGC = 14,
    FRONTEND_STATUS_TYPE_LNA = 15,
    FRONTEND_STATUS_TYPE_LAYER_ERROR = 16,
    FRONTEND_STATUS_TYPE_MER = 17,
    FRONTEND_STATUS_TYPE_FREQ_OFFSET = 18,
    FRONTEND_STATUS_TYPE_HIERARCHY = 19,
    FRONTEND_STATUS_TYPE_RF_LOCK = 20,
    FRONTEND_STATUS_TYPE_ATSC3_PLP_INFO = 21,
//For Android T
    FRONTEND_STATUS_TYPE_MODULATIONS = 22,
    FRONTEND_STATUS_TYPE_BERS = 23,
    FRONTEND_STATUS_TYPE_CODERATES = 24,
    FRONTEND_STATUS_TYPE_BANDWIDTH = 25,
    FRONTEND_STATUS_TYPE_GUARD_INTERVAL = 26,
    FRONTEND_STATUS_TYPE_TRANSMISSION_MODE = 27,
    FRONTEND_STATUS_TYPE_UEC = 28,
    FRONTEND_STATUS_TYPE_T2_SYSTEM_ID = 29,
    FRONTEND_STATUS_TYPE_INTERLEAVINGS = 30,
    FRONTEND_STATUS_TYPE_ISDBT_SEGMENTS = 31,
    FRONTEND_STATUS_TYPE_TS_DATA_RATES = 32,
    FRONTEND_STATUS_TYPE_ROLL_OFF = 33,
    FRONTEND_STATUS_TYPE_IS_MISO = 34,
    FRONTEND_STATUS_TYPE_IS_LINEAR = 35,
    FRONTEND_STATUS_TYPE_IS_SHORT_FRAMES  = 36,
    FRONTEND_STATUS_TYPE_ISDBT_MODE = 37,
    FRONTEND_STATUS_TYPE_ISDBT_PARTIAL_RECEPTION_FLAG = 38,
    FRONTEND_STATUS_TYPE_STREAM_ID_LIST = 39,
    FRONTEND_STATUS_TYPE_DVBT_CELL_IDS = 40,
    FRONTEND_STATUS_TYPE_ATSC3_ALL_PLP_INFO = 41,
};

enum TUNE_EVENT {
    TUNE_EVENT_SIGNAL_LOCKED = 0,
    TUNE_EVENT_SIGNAL_NO_SIGNAL = 1,
    TUNE_EVENT_SIGNAL_LOST_LOCK = 2,
};

enum SCAN_TYPE {
    SCAN_TYPE_UNDEFINED = 0,
    SCAN_TYPE_AUTO = 1 << 0,
    SCAN_TYPE_BLIND = 1 << 1,
};

enum SCAN_MESSAGE_TYPE {
    SCAN_MESSAGE_LOCKED = 0,
    SCAN_MESSAGE_UNLOCK = 1,
    SCAN_MESSAGE_END = 2,
    SCAN_MESSAGE_PROGRESS_PERCENT = 3,
    SCAN_MESSAGE_FREQUENCY = 4,
    SCAN_MESSAGE_SYMBOL_RATE = 5,
    SCAN_MESSAGE_PLP_IDS = 6,
    SCAN_MESSAGE_GROUP_IDS = 7,
    SCAN_MESSAGE_INPUT_STREAM_IDS = 8,
    SCAN_MESSAGE_DVBS_STANDARD = 9,
    SCAN_MESSAGE_DVBT_STANDARD = 10,
    SCAN_MESSAGE_ANALOG_TYPE = 11,
    SCAN_MESSAGE_HIERARCHY = 12,
    SCAN_MESSAGE_SIGNAL_TYPE = 13,
    SCAN_MESSAGE_DVBT_CELL_IDS = 14,
};

typedef struct
{
    int plp_id;
    int uec;
    bool is_locked;
}Atsc3_Plp_Tuning_Info;

typedef struct
{
    int snr;
    int ber;
    int per;
    int per_ber;
    int signal_quality;
    int signal_strength;
    int symbol_rate;
    int modulation;
    int inversion;
    int lnb_voltage;
    int plp_id;
    int agc;
    int mer;
    long freq_offset;
    int hierarchy;
    long inner_fec;
    bool is_demod_locked;
    bool is_ewbs;
    bool is_lna_on;
    bool is_rf_locked;
    char * pis_layer_errors;
    Atsc3_Plp_Tuning_Info *patsc3_info;
    std::vector<int> code_rate;
    std::vector<int> dvbt_cell_ids;
}Frontend_Status;

typedef struct {
    int scanMessageType;
    std::vector<int> integer_value;
}Scan_Callback_Message;

/**
 * parse FrontendStatus class to Frontend_Status struct
 *
 * @param jFrontendStatus java class FrontendStatus map structure Dvbt_Frontend_Settings.
 *
 * @param pFrontendStatus java class FrontendStatus map structure Frontend_Status.
 *
 * @return true parse success, false parse fail .
 */
bool frontend_utils_parseFrontendStatus(JNIEnv *env, jobject jFrontendStatus, Frontend_Status *pFrontendStatus);

/**
 * parse ScanCallback class to Scan_Callback_Message struct
 *
 * @param scanMessageType scan callback message type.
 *
 * @param Scan_Callback_Message scan callback message value.
 *
 * @return true parse success, false parse fail .
 */
bool frontend_utils_parseScanCallbackMessage(JNIEnv *env, int scanMessageType, jobjectArray scanMessage, Scan_Callback_Message *pScanCallbackMessage);

#endif/*_FRONTEND_UTILS_*/

