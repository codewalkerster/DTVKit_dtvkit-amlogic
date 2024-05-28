#include "wrapper_frontend.h"
#include <map>
#include <vector>
#include <jni.h>
#include "JNI_tuner.h"
#include "filter_utils.h"
#include "dvb_frontend_setting_utils.h"
#include "isdb_frontend_setting_utils.h"
#include "atsc_frontend_settings_utils.h"
#include "type_change_utils.h"
#include "frontend_utils.h"

#define TAG "wrapper_frontend"

#define MAP_INSERT_ITEM(__MAP__, __KEY__, __VALUE__) __MAP__.insert(std::make_pair(__KEY__, __VALUE__))
#define KEY_CONTAINED_IN_MAP(__MAP__, __KEY__) (__MAP__.find(__KEY__) != __MAP__.end())

typedef enum  {
    FRONTEND_PARAM_MIN_SRATE,
    FRONTEND_PARAM_MAX_SRATE,
    FRONTEND_PARAM_MIN_FREQ,
    FRONTEND_PARAM_MAX_FREQ,
} WRAPPER_FRONTEND_PARAM;

typedef struct
{
    S32BIT frontend_fd = INVALID_FD;
    U16BIT tuner_client = INVALID_TUNER_ID;
    U32BIT frequency = 0;
    BOOLEAN current_tuning = FALSE;
    E_TTYPE signal_type = E_TERR_TYPE_UNKNOWN;
    EW_STB_TUNE_SYSTEM_TYPE sys_type = WRAPPER_TUNE_SYSTEM_TYPE_UNKNOWN;
    EW_STB_TUNE_TBWIDTH tbwidth = WRAPPER_TUNE_TBWIDTH_8MHZ;
    EW_STB_TUNE_TMODE tmode = WRAPPER_TUNE_MODE_COFDM_UNDEFINED;
    EW_STB_TUNE_CMODE cable_mode = WRAPPER_TUNE_MODE_QAM_UNDEFINED;
    EW_STB_TUNE_MODULATION tune_modulation = WRAPPER_TUNE_MOD_AUTO;
    std::vector<U8BIT> t2_plp_list;

    EW_STB_TUNE_LNB_VOLTAGE tune_voltage = WRAPPER_LNB_VOLTAGE_OFF;
    EW_STB_TUNE_FEC fec = WRAPPER_TUNE_FEC_AUTOMATIC;
    jobject tuner_lnb = NULL;
    U8BIT tuner_plp = 0;
    U8BIT frontend_usage = 0;
    S32BIT tuner_lo_freq = 0;
    U32BIT tuner_srate = 0;
    BOOLEAN tuner_search_mode = FALSE;
    BOOLEAN use_22khz = FALSE;
    BOOLEAN tuning_params_changed = FALSE;
    BOOLEAN auto_relock= FALSE;
    BOOLEAN tune_lock = FALSE;

    BOOLEAN blindscan_mode = FALSE;
    Wrapper_Tune_BlindCallback_t blindscan_event_cb = NULL;
    void* blindscan_cb_user_data = NULL;
    std::vector<U32BIT> blindscan_tp_freq;
    std::vector<U32BIT> blindscan_tp_srate;
} WRAPPER_TUNER_STATUS;

typedef std::map<U8BIT/*path*/, WRAPPER_TUNER_STATUS/*tuner status*/> TUNER_STATUS_MAP;

static TUNER_STATUS_MAP tuner_status_map;
static std::vector<int> frontend_list;
static Wrapper_SendEvent lock_SendEvent;



static U16BIT findTunerClient(U8BIT path)
{
    return KEY_CONTAINED_IN_MAP(tuner_status_map, path) ? tuner_status_map[path].tuner_client
                                                        : INVALID_TUNER_ID;
}

static inline BOOLEAN isCurrentTuning(U8BIT path)
{
    return KEY_CONTAINED_IN_MAP(tuner_status_map, path) ? tuner_status_map[path].current_tuning
                                                        : FALSE;
}

static E_TTYPE getSignalType(U8BIT path)
{
    return KEY_CONTAINED_IN_MAP(tuner_status_map, path) ? tuner_status_map[path].signal_type
                                                        : E_TERR_TYPE_UNKNOWN;
}

static U8BIT getTunerPath(U16BIT tuner_client)
{
    for (const auto& status : tuner_status_map) {
        if (tuner_client == status.second.tuner_client) {
            return status.first;
        }
    }

    return INVALID_TUNER_PATH;
}

static TUNER_TYPE getTunerType(U8BIT path)
{
    switch (path)
    {
        case 0:
            return TUNER_TYPE_LIVE_0;
        case 1:
            return TUNER_TYPE_LIVE_1;
        case 2:
            return TUNER_TYPE_LIVE_2;
        default:
            return TUNER_TYPE_LIVE_0;
    }

    return TUNER_TYPE_LIVE_0;
}

static jclass getValueClass(JNIEnv *env, jobject valueObject, const char *name)
{
   if ((NULL == env) || (NULL == valueObject)) {
        ALOGE("%s: input parameter error", __FUNCTION__);
        return NULL;
    }

    if (env->IsSameObject(valueObject, nullptr)) {
        ALOGE("%s: value Object is nullptr", __FUNCTION__);
        return NULL;
    }
    jclass valueClazz = env->FindClass(name);
    if (JNI_TRUE != env->IsInstanceOf(valueObject, valueClazz)) {
        ALOGD("%s: value object not check value class", __FUNCTION__);
        return NULL;
    }
    return valueClazz;
}

static BOOLEAN getFrontendIds(U8BIT path)
{
    frontend_list.clear();

    U16BIT tuner_client = (U16BIT)Am_tuner_getTunerClientIdByType(getTunerType(path));
    if (tuner_client == INVALID_TUNER_ID) {
        ALOGE("%s: path: %d, Invalid tuner id", __FUNCTION__, path);
        return FALSE;
    }

    jobject frontend_obj_list = Am_tuner_getFrontendIds(tuner_client);
    if (NULL == frontend_obj_list) {
        ALOGE("%s: frontend_obj_list is null", __FUNCTION__);
        return FALSE;
    }

    bool attached = false;
    JNIEnv *env = Am_tuner_getJNIEnv(&attached);
    if (NULL == env) {
        ALOGE("%s: env is null", __FUNCTION__);
        return FALSE;
    }

    jclass list_class = env->FindClass("java/util/List");
    if (JNI_TRUE != env->IsInstanceOf(frontend_obj_list, list_class)) {
        ALOGE("%s: no frontend object list", __FUNCTION__);
        if (attached) {
            Am_tuner_detachJNIEnv();
        }
        return FALSE;
    }

    jmethodID list_size = env->GetMethodID(list_class, "size", "()I");
    jmethodID list_get = env->GetMethodID(list_class, "get", "(I)Ljava/lang/Object;");

    int frontend_size = env->CallIntMethod(frontend_obj_list, list_size);
    ALOGD("%s: frontend size: %d", __FUNCTION__, frontend_size);
    for (int i=0; i < frontend_size; i++) {
        jobject id = env->CallObjectMethod(frontend_obj_list, list_get, i);
        jclass integer_class = env->FindClass("java/lang/Integer");
        if (JNI_TRUE != env->IsInstanceOf(id, integer_class)) {
            ALOGE("%s: frontend id is not integer", __FUNCTION__);
            if (attached) {
                Am_tuner_detachJNIEnv();
            }
            return FALSE;
        }
        jmethodID intValue = env->GetMethodID(integer_class, "intValue", "()I");
        int frontend_id = env->CallIntMethod(id, intValue);
        ALOGD("%s: frontendId: %d", __FUNCTION__, frontend_id);
        frontend_list.push_back(frontend_id);
    }

    env->DeleteWeakGlobalRef(frontend_obj_list);

    if (attached) {
        Am_tuner_detachJNIEnv();
    }


    return (frontend_list.size() > 0);
}

static S64BIT getCurrentFrontendParameter(U8BIT path, WRAPPER_FRONTEND_PARAM param)
{
    U16BIT tuner_client = (U16BIT)Am_tuner_getTunerClientIdByType(getTunerType(path));

    if (tuner_client == INVALID_TUNER_ID) {
        ALOGE("%s: path %d is invalid", __FUNCTION__, path);
        return 0;
    }

    bool attached = false;
    JNIEnv *env = Am_tuner_getJNIEnv(&attached);
    if (NULL == env) {
        ALOGE("%s: env is null", __FUNCTION__);
        return 0;
    }

    S64BIT param_value = 0;
    for (int frontendId : frontend_list)
    {
        ALOGI("%s: frontendId : %d", __FUNCTION__, frontendId);
        jobject frontendInfo = Am_tuner_getFrontendInfoById(tuner_client, frontendId);
        if (NULL == frontendInfo) {
            ALOGE("%s: FrontendInfo is null", __FUNCTION__);
            if (attached) {
                Am_tuner_detachJNIEnv();
            }
            return 0;
        }
        //1.Test jobject class
        jclass fe_info_class = env->FindClass("android/media/tv/tuner/frontend/FrontendInfo");
        if (JNI_TRUE != env->IsInstanceOf(frontendInfo, fe_info_class)) {
            ALOGE("%s: not FrontendInfo object", __FUNCTION__);
            if (attached) {
                Am_tuner_detachJNIEnv();
            }
            return 0;
        }

        //2.show Frontend info
        jfieldID fType = env->GetFieldID(fe_info_class, "mType", "I");
        E_TTYPE type = (E_TTYPE)(env->GetIntField(frontendInfo, fType));
        if (getSignalType(path) == type) {
            std::string get_param_range;
            std::string ValueClass_type;
            if (param == FRONTEND_PARAM_MAX_FREQ || param == FRONTEND_PARAM_MIN_FREQ) {
                get_param_range = "getFrequencyRangeLong";
                ValueClass_type = LONG_CLASS;
            }
            else if (param == FRONTEND_PARAM_MAX_SRATE || param == FRONTEND_PARAM_MIN_SRATE) {
                get_param_range = "getSymbolRateRange";
                ValueClass_type = INTEGER_CLASS;
            }

            jmethodID value_range_id = env->GetMethodID(fe_info_class, get_param_range.c_str(), "()Landroid/util/Range;");
            jobject value_range_obj = env->CallObjectMethod(frontendInfo, value_range_id);
            jclass value_range_class = env->FindClass("android/util/Range");
            if (JNI_TRUE != env->IsInstanceOf(value_range_obj, value_range_class)) {
                ALOGE("%s: not Range object", __FUNCTION__);
                if (attached) {
                    Am_tuner_detachJNIEnv();
                }
                return 0;
            }

            std::string get_value_func;
            if (param == FRONTEND_PARAM_MIN_SRATE|| param == FRONTEND_PARAM_MIN_FREQ) {
                get_value_func = "getLower";
            }
            else if (param == FRONTEND_PARAM_MAX_SRATE || param == FRONTEND_PARAM_MAX_FREQ) {
                get_value_func = "getUpper";
            }

            jmethodID get_value_id = env->GetMethodID(value_range_class, get_value_func.c_str(), "()Ljava/lang/Comparable;");
            jobject get_value_obj = env->CallObjectMethod(value_range_obj, get_value_id);
            jclass value_class = getValueClass(env, get_value_obj, ValueClass_type.c_str());
            if (NULL == value_class) {
                if (attached) {
                    Am_tuner_detachJNIEnv();
                }
                return 0;
            }

            if (param == FRONTEND_PARAM_MAX_FREQ || param == FRONTEND_PARAM_MIN_FREQ)
            {
                jmethodID value_id = env->GetMethodID(value_class, "longValue", "()J");
                param_value = env->CallLongMethod(get_value_obj, value_id);
            }
            else if (param == FRONTEND_PARAM_MAX_SRATE || param == FRONTEND_PARAM_MIN_SRATE)
            {
                jmethodID value_id = env->GetMethodID(value_class, "intValue", "()I");
                param_value = env->CallIntMethod(get_value_obj, value_id);
            }
            ALOGI("%s: type:%d param:%d value:%lld", __FUNCTION__, type, param, param_value);
            env->DeleteLocalRef(get_value_obj);
            env->DeleteLocalRef(value_range_obj);
            env->DeleteWeakGlobalRef(frontendInfo);
            break;
        }

        env->DeleteWeakGlobalRef(frontendInfo);
    }

    if (attached) {
        Am_tuner_detachJNIEnv();
    }
    return param_value;
}

static Frontend_Status getFrontendStatus(U16BIT tuner_client, FRONTEND_STATUS_TYPE data_type)
{
    Frontend_Status stfrontendStatus;
    memset(&stfrontendStatus, 0, sizeof(Frontend_Status));
    if (INVALID_TUNER_ID == tuner_client) {
        ALOGE("%s: tuner_client is invalid", __FUNCTION__);
        return stfrontendStatus;
    }

    bool attached = false;
    JNIEnv *env = Am_tuner_getJNIEnv(&attached);
    if (NULL == env) {
        ALOGE("%s: env is null", __FUNCTION__);
        return stfrontendStatus;
    }

    FRONTEND_STATUS_TYPE status_type = data_type;
    jintArray jaStatusTypes = TypeChangeUtils::getJNIArray(env, (int*)&status_type, 1);
    jobject frontendStatusObject = Am_tuner_getFrontendStatus(tuner_client, jaStatusTypes);
    frontend_utils_parseFrontendStatus(env, frontendStatusObject, &stfrontendStatus);
    env->DeleteWeakGlobalRef(frontendStatusObject);
    if (attached) {
        Am_tuner_detachJNIEnv();
    }


    return stfrontendStatus;
}

static DVBT_BANDWIDTH getTerrBwidth(EW_STB_TUNE_TBWIDTH tbwidth)
{
    DVBT_BANDWIDTH bwidth = DVBT_BANDWIDTH_UNDEFINED;
    switch (tbwidth)
        {
        case WRAPPER_TUNE_TBWIDTH_8MHZ:
            bwidth = DVBT_BANDWIDTH_8MHZ;
            break;
        case WRAPPER_TUNE_TBWIDTH_7MHZ:
            bwidth = DVBT_BANDWIDTH_7MHZ;
            break;
        case WRAPPER_TUNE_TBWIDTH_6MHZ:
            bwidth = DVBT_BANDWIDTH_6MHZ;
            break;
        case WRAPPER_TUNE_TBWIDTH_5MHZ:
            bwidth = DVBT_BANDWIDTH_5MHZ;
            break;
        case WRAPPER_TUNE_TBWIDTH_10MHZ:
            bwidth = DVBT_BANDWIDTH_10MHZ;
            break;
        default:
            bwidth = DVBT_BANDWIDTH_AUTO;
            break;
        }
    return bwidth;
}

static DVBT_TRANSMISSION_MODE getTransmissionMode(EW_STB_TUNE_TMODE tmode)
{
    DVBT_TRANSMISSION_MODE mode = DVBT_TRANSMISSION_MODE_UNDEFINED;
    switch (tmode)
        {
        case WRAPPER_TUNE_MODE_COFDM_1K:
            mode = DVBT_TRANSMISSION_MODE_1K;
            break;
        case WRAPPER_TUNE_MODE_COFDM_2K:
            mode = DVBT_TRANSMISSION_MODE_2K;
            break;
        case WRAPPER_TUNE_MODE_COFDM_4K:
            mode = DVBT_TRANSMISSION_MODE_4K;
            break;
        case WRAPPER_TUNE_MODE_COFDM_8K:
            mode = DVBT_TRANSMISSION_MODE_8K;
            break;
        case WRAPPER_TUNE_MODE_COFDM_16K:
            mode = DVBT_TRANSMISSION_MODE_16K;
            break;
        case WRAPPER_TUNE_MODE_COFDM_32K:
            mode = DVBT_TRANSMISSION_MODE_32K;
            break;
        default:
            mode = DVBT_TRANSMISSION_MODE_AUTO;
            break;
        }
    return mode;
}

static DVBC_MODULATION getCableModulation(EW_STB_TUNE_CMODE cmode)
{
    DVBC_MODULATION modulation = DVBC_MODULATION_AUTO;
    switch (cmode) {
        // case WRAPPER_TUNE_MODE_QAM_4:
        // case WRAPPER_TUNE_MODE_QAM_8:
        case WRAPPER_TUNE_MODE_QAM_16:
            modulation = DVBC_MODULATION_MOD_16QAM;
            break;
        case WRAPPER_TUNE_MODE_QAM_32:
            modulation = DVBC_MODULATION_MOD_32QAM;
            break;
        case WRAPPER_TUNE_MODE_QAM_64:
            modulation = DVBC_MODULATION_MOD_64QAM;
            break;
        case WRAPPER_TUNE_MODE_QAM_128:
            modulation = DVBC_MODULATION_MOD_128QAM;
            break;
        case WRAPPER_TUNE_MODE_QAM_256:
            modulation = DVBC_MODULATION_MOD_256QAM;
            break;
        case WRAPPER_TUNE_MODE_QAM_UNDEFINED:
        default:
            modulation = DVBC_MODULATION_AUTO;
            break;
    }
    return modulation;
}

static DVBS_INNER_FEC getSatelliteFec(EW_STB_TUNE_FEC fec)
{
    DVBS_INNER_FEC inner_fec = DVBS_FEC_AUTO;
    switch (fec) {
        case WRAPPER_TUNE_FEC_1_2:
            inner_fec = DVBS_FEC_1_2;
            break;
        case WRAPPER_TUNE_FEC_2_3:
            inner_fec = DVBS_FEC_2_3;
            break;
        case WRAPPER_TUNE_FEC_3_4:
            inner_fec = DVBS_FEC_3_4;
            break;
        case WRAPPER_TUNE_FEC_5_6:
            inner_fec = DVBS_FEC_5_6;
            break;
        case WRAPPER_TUNE_FEC_7_8:
            inner_fec = DVBS_FEC_7_8;
            break;
        case WRAPPER_TUNE_FEC_1_4:
            inner_fec = DVBS_FEC_1_4;
            break;
        case WRAPPER_TUNE_FEC_1_3:
            inner_fec = DVBS_FEC_1_3;
            break;
        case WRAPPER_TUNE_FEC_2_5:
            inner_fec = DVBS_FEC_2_5;
            break;
        case WRAPPER_TUNE_FEC_8_9:
            inner_fec = DVBS_FEC_8_9;
            break;
        case WRAPPER_TUNE_FEC_9_10:
            inner_fec = DVBS_FEC_9_10;
            break;
        case WRAPPER_TUNE_FEC_3_5:
            inner_fec = DVBS_FEC_3_5;
            break;
        case WRAPPER_TUNE_FEC_4_5:
            inner_fec = DVBS_FEC_4_5;
            break;
        case WRAPPER_TUNE_FEC_AUTOMATIC:
        default:
            inner_fec = DVBS_FEC_AUTO;
            break;
    }
    return inner_fec;
}

static ISDBT_BANDWIDTH getIsdbtBwidth(EW_STB_TUNE_TBWIDTH tbwidth)
{
    ISDBT_BANDWIDTH ret = ISDBT_BANDWIDTH_UNDEFINED;
    switch (tbwidth)
    {
        case WRAPPER_TUNE_TBWIDTH_8MHZ:
        {
            ret = ISDBT_BANDWIDTH_8M;
            break;
        }
        case WRAPPER_TUNE_TBWIDTH_7MHZ:
        {
            ret = ISDBT_BANDWIDTH_7M;
            break;
        }
        case WRAPPER_TUNE_TBWIDTH_6MHZ:
        {
            ret = ISDBT_BANDWIDTH_6M;
            break;
        }
        default:
        {
            ret = ISDBT_BANDWIDTH_AUTO;
            break;
        }
    }
    return ret;
}

static void lnbCallback(jobject lnb, int tuner_client, int eventType, jbyteArray diseqcMessage) {
    ALOGD("%s: lnb: %p tuner_client:%d eventType:%d", __FUNCTION__, lnb, tuner_client, eventType);

    bool attached = false;
    JNIEnv *env = Am_tuner_getJNIEnv(&attached);
    if (NULL == env) {
        ALOGE("%s: env is null", __FUNCTION__);
        return;
    }

    if (NULL != diseqcMessage) {
        std::vector<char> message;
        TypeChangeUtils::getCharVector(env, diseqcMessage, &message);
        for (int i = 0; i < message.size(); i++) {
            ALOGD("%s: message: 0x%x", __FUNCTION__, message[i]);
        }
    }

    if (attached) {
        Am_tuner_detachJNIEnv();
    }
}

static BOOLEAN openLnb(U8BIT path)
{
    U16BIT client_id = (U16BIT)Am_tuner_getTunerClientIdByType(getTunerType(path));
    if (!KEY_CONTAINED_IN_MAP(tuner_status_map, path)) {
        ALOGE("%s: path:%d isn't contained in map, client_id:%d", __FUNCTION__, path, client_id);
        return FALSE;
    }

    if (tuner_status_map[path].tuner_lnb == NULL) {
        ALOGD("%s: path:%d open lnb", __FUNCTION__, path);
        tuner_status_map[path].tuner_client = client_id;
        tuner_status_map[path].tuner_lnb = Am_tuner_openLnb(client_id, (long)lnbCallback);
        if (tuner_status_map[path].tuner_lnb == NULL) {
            ALOGE("%s: tuner_lnb is open failed", __FUNCTION__);
            return FALSE;
        }
    }
    else if (tuner_status_map[path].tuner_client != client_id) {
        ALOGD("%s: path:%d client_id change(%d->%d) open lnb",
              __FUNCTION__, path, tuner_status_map[path].tuner_client, client_id);
        Am_lnb_close(tuner_status_map[path].tuner_lnb);
        tuner_status_map[path].tuner_client = client_id;
        tuner_status_map[path].tuner_lnb = Am_tuner_openLnb(client_id, (long)lnbCallback);
        if (tuner_status_map[path].tuner_lnb == NULL) {
            ALOGE("%s: tuner_lnb is open failed", __FUNCTION__);
            return FALSE;
        }
    }
    else {
        ALOGD("%s: path:%d lnb is already open", __FUNCTION__, path);
    }

    return TRUE;
}

static BOOLEAN closeLnb(U8BIT path)
{
    U16BIT client_id = findTunerClient(path);
    if (INVALID_TUNER_ID == client_id) {
        ALOGE("%s: path(%d) is invalid", __FUNCTION__, path);
        return FALSE;
    }

    ALOGD("%s: path:%d tuner_client:%d", __FUNCTION__, path, client_id);

    if (tuner_status_map[path].tuner_lnb != NULL) {
        Am_lnb_close(tuner_status_map[path].tuner_lnb);
        tuner_status_map[path].tuner_lnb = NULL;
    }
    else
    {
        ALOGD("%s: path:%d lnb is already closed", __FUNCTION__, path);
    }

    return TRUE;
}

static void setLnbVoltage(jobject lnb, EW_STB_TUNE_LNB_VOLTAGE voltage)
{
    DVBS_LNB_VOLTAGE lnb_voltage = DVBS_LNB_VOLTAGE_NONE;
    if (WRAPPER_LNB_VOLTAGE_14V == voltage) {
        lnb_voltage = DVBS_LNB_VOLTAGE_14V;
    }
    else if (WRAPPER_LNB_VOLTAGE_18V == voltage) {
        lnb_voltage = DVBS_LNB_VOLTAGE_18V;
    }
    else {
        lnb_voltage = DVBS_LNB_VOLTAGE_NONE;
    }
    Am_lnb_setVoltage(lnb, lnb_voltage);
}

static void setLnbTone(jobject lnb, BOOLEAN use_22khz)
{
    if (use_22khz == TRUE) {
        Am_lnb_setTone(lnb, DVBS_LNB_TONE_CONTINUOUS);
    }
    else {
        Am_lnb_setTone(lnb, DVBS_LNB_TONE_NONE);
    }
}

static void tuneCallback(int tuner_client, int event) {
    ALOGD("%s: tuner_client:%d event:%d", __FUNCTION__, tuner_client, event);

    if (INVALID_TUNER_ID == tuner_client) {
        ALOGE("%s: tuner_client is invalid", __FUNCTION__);
        return;
    }

    U8BIT tuner_path = getTunerPath(tuner_client);
    if (INVALID_TUNER_PATH == tuner_path) {
        ALOGE("%s: tuner_path is invalid", __FUNCTION__);
        return;
    }

    U16BIT event_type = WRPPER_HW_EV_TYPE_NOTLOCKED;
    if (0 == event) {
        event_type = WRPPER_HW_EV_TYPE_LOCKED;
        tuner_status_map[tuner_path].tune_lock = TRUE;
        ALOGD("%s: tuner lock", __FUNCTION__);
    }
    else {
        tuner_status_map[tuner_path].tune_lock = FALSE;
        ALOGD("%s: tuner unlock", __FUNCTION__);
    }

    lock_SendEvent(FALSE, WRPPER_HW_EV_CLASS_TUNER, event_type, &tuner_path, sizeof(U8BIT));
}

static void scanCallback(int tuner_client, int scanCallbackMessageType, jobjectArray scanCallbackMessage) {
    ALOGD("%s: tuner_client:%d scanCallbackMessageType:%d", __FUNCTION__, tuner_client, scanCallbackMessageType);

    if (INVALID_TUNER_ID == tuner_client) {
        ALOGE("%s: tuner_client is invalid", __FUNCTION__);
        return;
    }

    U8BIT tuner_path = getTunerPath(tuner_client);
    if (INVALID_TUNER_PATH == tuner_path) {
        ALOGE("%s: tuner_path is invalid", __FUNCTION__);
        return;
    }

    Scan_Callback_Message scanMessage;
    memset(&scanMessage, 0, sizeof(Scan_Callback_Message));
    if (NULL != scanCallbackMessage) {
        bool attached = false;
        JNIEnv *env = Am_tuner_getJNIEnv(&attached);
        if (NULL == env) {
            ALOGE("%s: env is null", __FUNCTION__);
            return;
        }

        frontend_utils_parseScanCallbackMessage(env, scanCallbackMessageType, scanCallbackMessage, &scanMessage);
        ALOGD("%s: Scan_Callback_Message scanMessageType : %d", __FUNCTION__, scanMessage.scanMessageType);
        for (int value : scanMessage.integer_value) {
            ALOGD("%s: message: %d", __FUNCTION__, value);
        }

        if (attached) {
            Am_tuner_detachJNIEnv();
        }
    }

    switch (scanCallbackMessageType)
    {
        case SCAN_MESSAGE_LOCKED:
            tuner_status_map[tuner_path].tune_lock = TRUE;
            ALOGD("%s: tuner lock", __FUNCTION__);
            lock_SendEvent(FALSE, WRPPER_HW_EV_CLASS_TUNER, WRPPER_HW_EV_TYPE_LOCKED, &tuner_path, sizeof(U8BIT));
            break;
        case SCAN_MESSAGE_UNLOCK:
            tuner_status_map[tuner_path].tune_lock = FALSE;
            ALOGD("%s: tuner unlock", __FUNCTION__);
            lock_SendEvent(FALSE, WRPPER_HW_EV_CLASS_TUNER, WRPPER_HW_EV_TYPE_NOTLOCKED, &tuner_path, sizeof(U8BIT));
            break;
        case SCAN_MESSAGE_END:
            break;
        case SCAN_MESSAGE_PROGRESS_PERCENT:
            break;
        case SCAN_MESSAGE_FREQUENCY:
            if (scanMessage.integer_value.size() == 1)
            {
                tuner_status_map[tuner_path].frequency = scanMessage.integer_value[0];
            }
            break;
        case SCAN_MESSAGE_SYMBOL_RATE:
            if (scanMessage.integer_value.size() == 1)
            {
                tuner_status_map[tuner_path].tuner_srate = scanMessage.integer_value[0];
            }
            break;

        case SCAN_MESSAGE_PLP_IDS:
            for (int value : scanMessage.integer_value) {
                tuner_status_map[tuner_path].t2_plp_list.push_back(static_cast<U8BIT>(value));
            }
            break;
        case SCAN_MESSAGE_GROUP_IDS:
            break;
        case SCAN_MESSAGE_INPUT_STREAM_IDS:
            break;
        case SCAN_MESSAGE_DVBS_STANDARD:
            break;
        case SCAN_MESSAGE_DVBT_STANDARD:
            break;
        case SCAN_MESSAGE_ANALOG_TYPE:
            break;
        case SCAN_MESSAGE_HIERARCHY:
            break;
        case SCAN_MESSAGE_SIGNAL_TYPE:
            break;
        case SCAN_MESSAGE_DVBT_CELL_IDS:
            break;
        default:
            //ALOGD("%s: message default");
        break;
    }
}


static BOOLEAN IsAlreadyTuned(U8BIT path, U16BIT client_id,
    U32BIT freq, U32BIT srate, EW_STB_TUNE_FEC fec, EW_STB_TUNE_TMODE tmode, EW_STB_TUNE_TBWIDTH tbwidth, EW_STB_TUNE_CMODE cmode)
{
    if (tuner_status_map[path].tuner_client != client_id) {
        ALOGD("%s: Tuner client(%u) is different(%u %u)", __FUNCTION__, path, tuner_status_map[path].tuner_client, client_id);
        return FALSE;
    }

    if (!tuner_status_map[path].tune_lock) {
        ALOGD("%s: path:%d tuner is unlock", __FUNCTION__, path);
        return FALSE;
    }

    if (tuner_status_map[path].tuning_params_changed) {
        ALOGD("%s: path:%d tuning_params_changed is true", __FUNCTION__, path);
        return FALSE;
    }

    E_TTYPE signal_type = tuner_status_map[path].signal_type;
    EW_STB_TUNE_SYSTEM_TYPE sys_type = tuner_status_map[path].sys_type;
    EW_STB_TUNE_SIGNAL_TYPE required_signal = WRAPPER_TUNE_SIGNAL_NONE;

    if (signal_type == E_TERR_TYPE_DVBT &&
        (sys_type == WRAPPER_TUNE_SYSTEM_TYPE_DVBT || sys_type == WRAPPER_TUNE_SYSTEM_TYPE_DVBT2)) {
        required_signal = WRAPPER_TUNE_SIGNAL_COFDM;
    }
    else if (signal_type == E_TERR_TYPE_DVBS &&
             (sys_type == WRAPPER_TUNE_SYSTEM_TYPE_DVBS || sys_type == WRAPPER_TUNE_SYSTEM_TYPE_DVBS2)) {
        required_signal = WRAPPER_TUNE_SIGNAL_QPSK;
    }
    else if (signal_type == E_TERR_TYPE_DVBC && sys_type == WRAPPER_TUNE_SYSTEM_TYPE_DVBC) {
        required_signal = WRAPPER_TUNE_SIGNAL_QAM;
    }
    else if (signal_type == E_TERR_TYPE_ISDBT && sys_type == WRAPPER_TUNE_SYSTEM_TYPE_ISDBT) {
        required_signal = WRAPPER_TUNE_SIGNAL_ISDBT;
    }
    else if (signal_type == E_TERR_TYPE_VSB && sys_type == WRAPPER_TUNE_SYSTEM_TYPE_VSB)
    {
        required_signal = WRAPPER_TUNE_SIGNAL_VSB;
    }
    else if (signal_type == E_TERR_TYPE_QAMB && sys_type == WRAPPER_TUNE_SYSTEM_TYPE_QAMB)
    {
        required_signal = WRAPPER_TUNE_SIGNAL_QAMB;
    }

    E_TTYPE fe_signal = Wrapper_TuneGetActualSignalType(path);
    if ((required_signal == WRAPPER_TUNE_SIGNAL_COFDM && fe_signal == E_TERR_TYPE_DVBT) ||
        (required_signal == WRAPPER_TUNE_SIGNAL_QPSK && fe_signal == E_TERR_TYPE_DVBS) ||
        (required_signal == WRAPPER_TUNE_SIGNAL_QAM && fe_signal == E_TERR_TYPE_DVBC) ||
        (required_signal == WRAPPER_TUNE_SIGNAL_ISDBT && fe_signal == E_TERR_TYPE_ISDBT) ||
        (required_signal == WRAPPER_TUNE_SIGNAL_VSB && fe_signal == E_TERR_TYPE_VSB) ||
        (required_signal == WRAPPER_TUNE_SIGNAL_QAMB && fe_signal == E_TERR_TYPE_QAMB)) {
        // nothing
    }
    else {
        ALOGD("%s: Signal type(%u) is different(%u %u)", __FUNCTION__, path, required_signal, fe_signal);
        return FALSE;
    }

    if (tuner_status_map[path].frequency != freq)
    {
        ALOGD("%s: Frequency(%u) is different(%u %u)", __FUNCTION__, path, tuner_status_map[path].frequency, freq);
        return FALSE;
    }

    switch (required_signal)
    {
        case WRAPPER_TUNE_SIGNAL_COFDM:
            if (tuner_status_map[path].tmode != tmode || tuner_status_map[path].tbwidth != tbwidth)
            {
                ALOGD("%s: tmode or tbwidth(%u) is different(%u %u, %u %u)", __FUNCTION__, path,
                      tuner_status_map[path].tmode, tmode, tuner_status_map[path].tbwidth, tbwidth);
                return FALSE;
            }
            break;
        case WRAPPER_TUNE_SIGNAL_QAM:
            if (tuner_status_map[path].cable_mode != cmode || tuner_status_map[path].tuner_srate != srate)
            {
                ALOGD("%s: cmode or srate(%u) is different(%u %u, %u %u)", __FUNCTION__, path,
                      tuner_status_map[path].cable_mode, cmode, tuner_status_map[path].tuner_srate, srate);
                return FALSE;
            }
            break;
        case WRAPPER_TUNE_SIGNAL_QPSK:
            if (tuner_status_map[path].fec != fec || tuner_status_map[path].tuner_srate != srate)
            {
                ALOGD("%s: fec or srate(%u) is different(%u %u, %u %u)", __FUNCTION__, path,
                      tuner_status_map[path].fec, fec, tuner_status_map[path].tuner_srate, srate);
                return FALSE;
            }
            break;
        case WRAPPER_TUNE_SIGNAL_ISDBT:
            if (tuner_status_map[path].tbwidth != tbwidth)
            {
                ALOGD("%s: tbwidth(%u) is different(%u %u)", __FUNCTION__, path,
                      tuner_status_map[path].tbwidth, tbwidth);
                return FALSE;
            }
            break;
       case WRAPPER_TUNE_SIGNAL_VSB:
            return TRUE;
            break;
       case WRAPPER_TUNE_SIGNAL_QAMB:
            if (tuner_status_map[path].cable_mode != cmode || tuner_status_map[path].tuner_srate != srate)
            {
                ALOGD("%s: cmode or srate(%u) is different(%u %u, %u %u)", __FUNCTION__, path,
                  tuner_status_map[path].cable_mode, cmode, tuner_status_map[path].tuner_srate, srate);
                return FALSE;
            }
            break;
        default:
            {
                return FALSE;
            }
            break;
    }

    return TRUE;
}

static void blindscanCallback(int tuner_client, int scanCallbackMessageType, jobjectArray scanCallbackMessage) {
    ALOGD("%s: tuner_client:%d scanCallbackMessageType:%d", __FUNCTION__, tuner_client, scanCallbackMessageType);

    if (INVALID_TUNER_ID == tuner_client) {
        ALOGE("%s: tuner_client is invalid", __FUNCTION__);
        return;
    }

    U8BIT tuner_path = getTunerPath(tuner_client);
    if (INVALID_TUNER_PATH == tuner_path) {
        ALOGE("%s: tuner_path is invalid", __FUNCTION__);
        return;
    }

    ALOGD("%s: scanCallbackMessageType :%d ", __FUNCTION__, scanCallbackMessageType);

    Scan_Callback_Message scanMessage;
    memset(&scanMessage, 0, sizeof(Scan_Callback_Message));
    if (NULL != scanCallbackMessage) {
        bool attached = false;
        JNIEnv *env = Am_tuner_getJNIEnv(&attached);
        if (NULL == env) {
            ALOGE("%s: env is null", __FUNCTION__);
            return;
        }

        frontend_utils_parseScanCallbackMessage(env, scanCallbackMessageType, scanCallbackMessage, &scanMessage);
        ALOGD("%s: Scan_Callback_Message scanMessageType : %d", __FUNCTION__, scanMessage.scanMessageType);
        for (int value : scanMessage.integer_value) {
            ALOGD("%s: message: %d", __FUNCTION__, value);
        }

        if (attached) {
            Am_tuner_detachJNIEnv();
        }
    }

    switch (scanMessage.scanMessageType)
    {
        case SCAN_MESSAGE_PROGRESS_PERCENT:
            if (scanMessage.integer_value.size() == 1)
            {
                U8BIT progress = scanMessage.integer_value[0];
                if (tuner_status_map[tuner_path].blindscan_event_cb != NULL)
                {
                    EW_STB_TUNE_BlindEvent_t evt;
                    evt.status = WRAPPER_AM_FEND_BLIND_UPDATEPROCESS;
                    evt.process = progress;
                    tuner_status_map[tuner_path].blindscan_event_cb(
                        tuner_path, &evt, tuner_status_map[tuner_path].blindscan_cb_user_data);
                }
            }
            break;
        case SCAN_MESSAGE_FREQUENCY:
            if (scanMessage.integer_value.size() == 1)
            {
                U32BIT freq = scanMessage.integer_value[0];
                tuner_status_map[tuner_path].blindscan_tp_freq.push_back(freq);

            }
            break;
        case SCAN_MESSAGE_SYMBOL_RATE:
            if (scanMessage.integer_value.size() == 1)
            {
                U32BIT srate = scanMessage.integer_value[0];
                tuner_status_map[tuner_path].blindscan_tp_srate.push_back(srate);
            }
            break;

        case SCAN_MESSAGE_END:
            break;
        default:
            break;
    }
}

void Wrapper_RegisterCallback(Wrapper_SendEvent callback)
{
    lock_SendEvent = callback;
}

void Wrapper_TuneStartTuner(U8BIT path, U32BIT freq, U32BIT srate, EW_STB_TUNE_FEC fec, EW_STB_TUNE_TMODE tmode, EW_STB_TUNE_TBWIDTH tbwidth, EW_STB_TUNE_CMODE cmode)
{
    if (!KEY_CONTAINED_IN_MAP(tuner_status_map, path)) {
        ALOGE("%s: path:%d isn't contained in map", __FUNCTION__, path);
        return;
    }

    U16BIT client_id = (U16BIT)Am_tuner_getTunerClientIdByType(getTunerType(path));
    BOOLEAN search_mode = tuner_status_map[path].tuner_search_mode;
    ALOGD("%s path:%d client_id:%d search:%u freq:%d cmode:%d srate:%d tbwidth:%u",
          __FUNCTION__, path, client_id, search_mode, freq, cmode, srate, tbwidth);

    if (IsAlreadyTuned(path, client_id, freq, srate, fec, tmode, tbwidth, cmode)) {
        ALOGD("%s path:%d Already_tuned", __FUNCTION__, path);
        tuner_status_map[path].current_tuning = TRUE;
        tuner_status_map[path].tuning_params_changed = FALSE;
        tuner_status_map[path].tune_lock = TRUE;
        lock_SendEvent(FALSE, WRPPER_HW_EV_CLASS_TUNER, WRPPER_HW_EV_TYPE_LOCKED, &path, sizeof(U8BIT));
        return;
    }

    tuner_status_map[path].tuner_client = client_id;
    tuner_status_map[path].tune_lock = FALSE;

    bool attached = false;
    JNIEnv *env = Am_tuner_getJNIEnv(&attached);
    if (NULL == env)
    {
        ALOGE("%s: env is null", __FUNCTION__);
        return;
    }

    tuner_status_map[path].frequency = freq;
    tuner_status_map[path].tmode = tmode;
    tuner_status_map[path].tbwidth = tbwidth;
    tuner_status_map[path].tuner_srate = srate;
    tuner_status_map[path].cable_mode = cmode;
    tuner_status_map[path].fec = fec;

    jobject frontendSettingObject = NULL;
    if (tuner_status_map[path].signal_type == E_TERR_TYPE_DVBT) {

        Dvbt_Frontend_Settings dvbtFrontendSettings;
        memset(&dvbtFrontendSettings, 0, sizeof(Dvbt_Frontend_Settings));
        dvbtFrontendSettings.frequency = freq;
        dvbtFrontendSettings.transmissionMode = getTransmissionMode(tmode);
        dvbtFrontendSettings.bandwidth = getTerrBwidth(tbwidth);
        if (tuner_status_map[path].sys_type == WRAPPER_TUNE_SYSTEM_TYPE_DVBT) {
            dvbtFrontendSettings.standard = DVBT_STANDARD_T;
        }
        else if (tuner_status_map[path].sys_type == WRAPPER_TUNE_SYSTEM_TYPE_DVBT2) {
            dvbtFrontendSettings.standard = DVBT_STANDARD_T2;
            dvbtFrontendSettings.plpId = tuner_status_map[path].tuner_plp;
        }

        frontendSettingObject = dvb_utils_getDvbtFrontendSettingsObject(env, dvbtFrontendSettings);
    }
    else if (tuner_status_map[path].signal_type == E_TERR_TYPE_DVBC || tuner_status_map[path].signal_type == E_TERR_TYPE_QAMB) {
        Dvbc_Frontend_Settings dvbcFrontendSettings;
        memset(&dvbcFrontendSettings, 0, sizeof(Dvbc_Frontend_Settings));
        dvbcFrontendSettings.frequency = freq;
        dvbcFrontendSettings.modulation = getCableModulation(cmode);
        dvbcFrontendSettings.symbolRate = srate;
        if (tuner_status_map[path].signal_type == E_TERR_TYPE_DVBC)
        {
            dvbcFrontendSettings.annex = DVBC_ANNEX_A;
        }
        else if (tuner_status_map[path].signal_type == E_TERR_TYPE_QAMB)
        {
            dvbcFrontendSettings.annex = DVBC_ANNEX_B;
        }

        frontendSettingObject = dvb_utils_getDvbcFrontendSettingsObject(env, dvbcFrontendSettings);
    }
    else if (tuner_status_map[path].signal_type == E_TERR_TYPE_DVBS) {
        Dvbs_Frontend_Settings dvbsFrontendSettings;
        memset(&dvbsFrontendSettings, 0, sizeof(Dvbs_Frontend_Settings));
        dvbsFrontendSettings.frequency = freq*1000;
        dvbsFrontendSettings.symbol_rate = srate;
        dvbsFrontendSettings.modulation = DVBS_MODULATION_AUTO;
        dvbsFrontendSettings.scan_type= DVBS_SCAN_TYPE_DISEQC;
        dvbsFrontendSettings.pilot = DVBS_PILOT_AUTO;
        dvbsFrontendSettings.code_rate.fec = getSatelliteFec(fec);
        dvbsFrontendSettings.code_rate.isLinear = FALSE;
        dvbsFrontendSettings.code_rate.isShortFrames = true;
        dvbsFrontendSettings.code_rate.bitsPer1000Symbol = 0;
        dvbsFrontendSettings.standard = DVBS_STANDARD_AUTO;

        frontendSettingObject = dvb_utils_getDvbsFrontendSettingsObject(env, dvbsFrontendSettings);
    }
    else if (tuner_status_map[path].signal_type == E_TERR_TYPE_ISDBT) {
        Isdbt_Frontend_Settings isdbtFrontendSettings;
        memset(&isdbtFrontendSettings, 0, sizeof(Isdbt_Frontend_Settings));
        isdbtFrontendSettings.frequency = freq;
        isdbtFrontendSettings.bandwidth = getIsdbtBwidth(tbwidth);

        frontendSettingObject = isdb_utils_getIsdbtFrontendSettingsObject(env, isdbtFrontendSettings);
    }
    else if (tuner_status_map[path].signal_type == E_TERR_TYPE_VSB) {
        Atsc_Frontend_Settings atscFrontendSettings;
        memset(&atscFrontendSettings, 0, sizeof(Atsc_Frontend_Settings));
        atscFrontendSettings.frequency = freq;
        atscFrontendSettings.modulation = ATSC_MODULATION_8VSB;

        frontendSettingObject = atsc_utils_getAtscFrontendSettingsObject(env, atscFrontendSettings);
    }
    else {
        ALOGE("%s: error signal type %u", __FUNCTION__, tuner_status_map[path].signal_type);
    }

    if (frontendSettingObject != NULL)
    {
        if (search_mode) {
            if (tuner_status_map[path].sys_type == WRAPPER_TUNE_SYSTEM_TYPE_DVBT2) {
                tuner_status_map[path].t2_plp_list.clear();
            }
            Am_tuner_scan(client_id, frontendSettingObject, SCAN_TYPE_AUTO, (long)scanCallback);
        }
        else {
            Am_tuner_setOnTuneEventListener(client_id, (long)tuneCallback);
            Am_tuner_tune(client_id, frontendSettingObject);
        }

        tuner_status_map[path].current_tuning = TRUE;
        tuner_status_map[path].tuning_params_changed = FALSE;
    }
    else {
        ALOGE("%s: not get dvb frontend settings object", __FUNCTION__);
    }

    if (attached) {
        Am_tuner_detachJNIEnv();
    }
}
void Wrapper_TuneStopTuner(U8BIT path)
{
    U16BIT tuner_client = findTunerClient(path);
    if (tuner_client == INVALID_TUNER_ID) {
        ALOGE("%s path:%d is invalid", __FUNCTION__, path);
        return;
    }

    if (tuner_status_map[path].tuner_search_mode) {
        Am_tuner_cancelScanning(tuner_client);

        tuner_status_map[path].tuner_client = INVALID_TUNER_ID;
        tuner_status_map[path].current_tuning = FALSE;
        tuner_status_map[path].tune_lock = FALSE;
    }
}
U32BIT Wrapper_TuneGetSignalStrength(U8BIT path)
{
    U16BIT tuner_client = findTunerClient(path);
    if (INVALID_TUNER_ID == tuner_client) {
        ALOGE("%s: path %d is invalid", __FUNCTION__, path);
        return 0;
    }

    if (!isCurrentTuning(path)) {
        ALOGE("%s: path %d is not tuning", __FUNCTION__, path);
        return 0;
    }

    ALOGD("%s: tuner_client: %d", __FUNCTION__, tuner_client);

    Frontend_Status stfrontendStatus = getFrontendStatus(tuner_client, FRONTEND_STATUS_TYPE_SIGNAL_STRENGTH);

    return stfrontendStatus.signal_strength;
}

U32BIT Wrapper_TuneGetDataIntegrity(U8BIT path)
{
    U16BIT tuner_client = findTunerClient(path);
    if (INVALID_TUNER_ID == tuner_client) {
        ALOGE("%s: path %d is invalid", __FUNCTION__, path);
        return 0;
    }

    if (!isCurrentTuning(path)) {
        ALOGE("%s: path %d is not tuning", __FUNCTION__, path);
        return 0;
    }

    ALOGD("%s: tuner_client: %d", __FUNCTION__, tuner_client);

    Frontend_Status stfrontendStatus = getFrontendStatus(tuner_client, FRONTEND_STATUS_TYPE_BER);

    return stfrontendStatus.ber;
}

U32BIT Wrapper_TuneGetSignalQuality(U8BIT path)
{
    U16BIT tuner_client = findTunerClient(path);
    if (INVALID_TUNER_ID == tuner_client) {
        ALOGE("%s: path %d is invalid", __FUNCTION__, path);
        return 0;
    }

    if (!isCurrentTuning(path)) {
        ALOGE("%s: path %d is not tuning", __FUNCTION__, path);
        return 0;
    }

    ALOGD("%s: tuner_client: %d", __FUNCTION__, tuner_client);

    Frontend_Status stfrontendStatus = getFrontendStatus(tuner_client, FRONTEND_STATUS_TYPE_SIGNAL_QUALITY);

    return stfrontendStatus.signal_quality;
}

/**
 * @brief   Returns the ewbs flag
 * @param   path the tuner path to query
 * @return  ewbs flag
 */
BOOLEAN Wrapper_TuneGetEwbsFlag(U8BIT path)
{
    U16BIT tuner_client = findTunerClient(path);
    if (INVALID_TUNER_ID == tuner_client) {
        ALOGE("%s: path %d is invalid", __FUNCTION__, path);
        return 0;
    }

    if (!isCurrentTuning(path)) {
        ALOGE("%s: path %d is not tuning", __FUNCTION__, path);
        return 0;
    }

    ALOGD("%s: tuner_client: %d", __FUNCTION__, tuner_client);

    Frontend_Status stfrontendStatus = getFrontendStatus(tuner_client, FRONTEND_STATUS_TYPE_EWBS);
    return stfrontendStatus.is_ewbs;
}

U32BIT Wrapper_TuneGetActualTerrFrequency(U8BIT path)
{
    U16BIT tuner_client = findTunerClient(path);
    if (INVALID_TUNER_ID == tuner_client) {
        ALOGE("%s: path %d is invalid", __FUNCTION__, path);
        return 0;
    }

    if (!isCurrentTuning(path)) {
        ALOGE("%s: path %d is not tuning", __FUNCTION__, path);
        return 0;
    }

    ALOGD("%s: tuner_client: %d", __FUNCTION__, tuner_client);

    U32BIT terr_fre = 0;
    if (WRAPPER_TUNER_STATE_LOCKED == Wrapper_TuneGetLockStatus(path))  {
        terr_fre = tuner_status_map[path].frequency;
        ALOGD("%s: terr_fre %d", __FUNCTION__, terr_fre);
    }
    else {
        ALOGD("%s: unlock", __FUNCTION__);
    }

    return terr_fre;
}
S8BIT Wrapper_TuneGetActualTerrFreqOffset(U8BIT path)
{
    U16BIT tuner_client = findTunerClient(path);
    if (INVALID_TUNER_ID == tuner_client) {
        ALOGE("%s: path %d is invalid", __FUNCTION__, path);
        return 0;
    }

    if (!isCurrentTuning(path)) {
        ALOGE("%s: path %d is not tuning", __FUNCTION__, path);
        return 0;
    }

    ALOGD("%s: tuner_client: %d", __FUNCTION__, tuner_client);

    S8BIT fre_offset = 0;
    if (WRAPPER_TUNER_STATE_LOCKED == Wrapper_TuneGetLockStatus(path)) {
        Frontend_Status stfrontendStatus = getFrontendStatus(tuner_client, FRONTEND_STATUS_TYPE_FREQ_OFFSET);
        fre_offset = stfrontendStatus.freq_offset;
        ALOGD("%s: fre_offset %d", __FUNCTION__, fre_offset);
    }
    else {
        ALOGD("%s: unlock", __FUNCTION__);
    }

    return fre_offset;
}
EW_TUNER_EVENT Wrapper_TuneGetLockStatus(U8BIT path)
{
    U16BIT tuner_client = findTunerClient(path);
    if (INVALID_TUNER_ID == tuner_client) {
        ALOGE("%s: path %d is invalid", __FUNCTION__, path);
        return WRAPPER_TUNER_STATE_UNKNOWN;
    }

    if (!isCurrentTuning(path)) {
        ALOGE("%s: path %d is not tuning", __FUNCTION__, path);
        return WRAPPER_TUNER_STATE_UNKNOWN;
    }

    ALOGD("%s: tuner_client: %d", __FUNCTION__, tuner_client);

    EW_TUNER_EVENT lock_st = WRAPPER_TUNER_STATE_UNKNOWN;

    if (TRUE == tuner_status_map[path].tune_lock) {
        lock_st = WRAPPER_TUNER_STATE_LOCKED;
    }
    else {
        lock_st = WRAPPER_TUNER_STATE_TIMEOUT;
    }

    ALOGD("%s: lock_st: %d", __FUNCTION__, lock_st);

    return lock_st;
}

BOOLEAN Wrapper_TuneOpen(U8BIT path)
{
    U16BIT tuner_client = INVALID_TUNER_ID;
    if (KEY_CONTAINED_IN_MAP(tuner_status_map, path)) {
        ALOGD("%s: path:%d is contained in map", __FUNCTION__, path);
        tuner_client = (U16BIT)Am_tuner_getTunerClientIdByType(getTunerType(path));
        if (INVALID_TUNER_ID == tuner_client) {
            tuner_client = (U16BIT)Am_tuner_getTunerClientIdByType(getTunerType(path));
            if (INVALID_TUNER_ID == tuner_client)
            {
                ALOGE("%s: tuner_client is invalid", __FUNCTION__);
                return FALSE;
            }

            ALOGD("%s: path:%d client:%d", __FUNCTION__, path, tuner_client);
            tuner_status_map[path].tuner_client = tuner_client;
        }
    }
    else {
        tuner_client = (U16BIT)Am_tuner_getTunerClientIdByType(getTunerType(path));
        if (INVALID_TUNER_ID == tuner_client)
        {
            ALOGE("%s: tuner_client is invalid", __FUNCTION__);
            return FALSE;
        }
        WRAPPER_TUNER_STATUS tuner_status;
        tuner_status.tuner_client = tuner_client;
        ALOGD("%s: insert path:%d client:%d into map", __FUNCTION__, path, tuner_client);
        MAP_INSERT_ITEM(tuner_status_map, path, tuner_status);
    }

    BOOLEAN ret = getFrontendIds(path);

    return ret;
}
BOOLEAN Wrapper_TuneIsOpened(U8BIT path)
{
    U16BIT tuner_client = findTunerClient(path);

    return (INVALID_TUNER_ID != tuner_client);
}

EW_STB_TUNE_SIGNAL_TYPE Wrapper_TuneGetSignalType(U8BIT path)
{
    E_TTYPE signal_type = getSignalType(path);
    ALOGD("%s: path:%d signal:%d", __FUNCTION__, path, signal_type);

    EW_STB_TUNE_SIGNAL_TYPE s_type = WRAPPER_TUNE_SIGNAL_NONE;
    switch (signal_type) {
        case E_TERR_TYPE_ANALOG:
            {
                s_type = WRAPPER_TUNE_SIGNAL_ANALOG;
                break;
            }
        case E_TERR_TYPE_DVBC:
            {
                s_type = WRAPPER_TUNE_SIGNAL_QAM;
                break;
            }
        case E_TERR_TYPE_DVBS:
            {
                s_type = WRAPPER_TUNE_SIGNAL_QPSK;
                break;
            }
        case E_TERR_TYPE_DVBT:
            {
                s_type = WRAPPER_TUNE_SIGNAL_COFDM;
                break;
            }
        case E_TERR_TYPE_ISDBT:
            {
                s_type = WRAPPER_TUNE_SIGNAL_ISDBT;
                break;
            }
        case E_TERR_TYPE_VSB:
            {
               s_type = WRAPPER_TUNE_SIGNAL_VSB;
               break;
            }
        case E_TERR_TYPE_QAMB:
            {
               s_type = WRAPPER_TUNE_SIGNAL_QAMB;
               break;
            }
        default:
            {
                s_type = WRAPPER_TUNE_SIGNAL_NONE;
                break;
            }
    }

    return s_type;
}
E_TTYPE Wrapper_TuneGetActualSignalType(U8BIT path)
{
    U16BIT tuner_client = findTunerClient(path);
    if (tuner_client == INVALID_TUNER_ID) {
        ALOGE("%s: path %d is invalid", __FUNCTION__, path);
        return E_TERR_TYPE_UNKNOWN;
    }

    jobject frontendInfo = Am_tuner_getFrontendInfo(tuner_client);
    if (NULL == frontendInfo) {
        ALOGE("%s: FrontendInfo is null", __FUNCTION__);
        return E_TERR_TYPE_UNKNOWN;
    }

    bool attached = false;
    JNIEnv *env = Am_tuner_getJNIEnv(&attached);
    if (NULL == env) {
        ALOGE("%s: env is null", __FUNCTION__);
        return E_TERR_TYPE_UNKNOWN;
    }

    //1.Test jobject class
    jclass fe_info_class = env->FindClass("android/media/tv/tuner/frontend/FrontendInfo");
    if (JNI_TRUE != env->IsInstanceOf(frontendInfo, fe_info_class)) {
        ALOGE("%s: not FrontendInfo object", __FUNCTION__);
        if (attached) {
            Am_tuner_detachJNIEnv();
        }
        return E_TERR_TYPE_UNKNOWN;
    }
    //2.show Frontend info
    jfieldID fType = env->GetFieldID(fe_info_class, "mType", "I");
    U16BIT type = env->GetIntField(frontendInfo, fType);

    ALOGD("%s: frontend type: %d", __FUNCTION__, type);
    env->DeleteWeakGlobalRef(frontendInfo);

    if (attached) {
        Am_tuner_detachJNIEnv();
    }


    return (E_TTYPE)type;
}
void Wrapper_TuneSetSignalType(U8BIT path, EW_STB_TUNE_SIGNAL_TYPE type)
{
    E_TTYPE signal_type = E_TERR_TYPE_UNKNOWN;
    if (WRAPPER_TUNE_SIGNAL_QPSK == type) {
        signal_type = E_TERR_TYPE_DVBS;
    }else if (WRAPPER_TUNE_SIGNAL_COFDM == type) {
        signal_type = E_TERR_TYPE_DVBT;
    }else if (WRAPPER_TUNE_SIGNAL_QAM == type) {
        signal_type = E_TERR_TYPE_DVBC;
    }else if (WRAPPER_TUNE_SIGNAL_ISDBT == type) {
        signal_type = E_TERR_TYPE_ISDBT;
    }
    else if (WRAPPER_TUNE_SIGNAL_VSB == type) {
        signal_type = E_TERR_TYPE_VSB;
    }
    else if (WRAPPER_TUNE_SIGNAL_QAMB == type) {
        signal_type = E_TERR_TYPE_QAMB;
    }

    ALOGD("start:%s path:%d type:%d signal_type:%d", __FUNCTION__, path, type, signal_type);

    if (KEY_CONTAINED_IN_MAP(tuner_status_map, path)) {
        ALOGD("%s: path:%d curr_signal_type:%d signal_type:%d",
              __FUNCTION__, path, tuner_status_map[path].signal_type, signal_type);
        if (tuner_status_map[path].signal_type != signal_type) {
            tuner_status_map[path].signal_type = signal_type;
            tuner_status_map[path].tuning_params_changed = TRUE;
            tuner_status_map[path].tune_lock = FALSE;
            U16BIT tuner_client = findTunerClient(path);
            if (tuner_client != INVALID_TUNER_ID) {
                Am_tuner_cancelTuning(tuner_client);
                Am_tuner_clearOnTuneEventListener(tuner_client);
                Am_tuner_closeFrontend(tuner_client);
            }

            if (E_TERR_TYPE_DVBS == tuner_status_map[path].signal_type) {
                closeLnb(path);
            }
       }
    }
    else {
        WRAPPER_TUNER_STATUS tuner_status;
        tuner_status.signal_type = signal_type;
        ALOGD("%s: insert path:%d signal_type:%d into map", __FUNCTION__, path, signal_type);
        MAP_INSERT_ITEM(tuner_status_map, path, tuner_status);
    }

    return;
}
EW_STB_TUNE_TMODE Wrapper_TuneGetActualTerrMode(U8BIT path)
{
    EW_STB_TUNE_TMODE tmode = WRAPPER_TUNE_MODE_COFDM_UNDEFINED;
    if (KEY_CONTAINED_IN_MAP(tuner_status_map, path)) {
        tmode = tuner_status_map[path].tmode;
    }

    return tmode;
}
EW_STB_TUNE_TBWIDTH Wrapper_TuneGetActualTerrBwidth(U8BIT path)
{
    EW_STB_TUNE_TBWIDTH tbwidth = WRAPPER_TUNE_TBWIDTH_8MHZ;
    if (KEY_CONTAINED_IN_MAP(tuner_status_map, path)) {
        tbwidth = tuner_status_map[path].tbwidth;
    }

    return tbwidth;
}

S64BIT Wrapper_TuneGetMinTunerFreqKHz(U8BIT path)
{
    S64BIT min_freq;

    min_freq = getCurrentFrontendParameter(path, FRONTEND_PARAM_MIN_FREQ);
    min_freq /= 1000;
    ALOGI("%s: Min Tuner Freq: %lld KHz", __FUNCTION__, min_freq);
    return min_freq;
}

S64BIT Wrapper_TuneGetMaxTunerFreqKHz(U8BIT path)
{
    S64BIT max_freq;
    max_freq = getCurrentFrontendParameter(path, FRONTEND_PARAM_MAX_FREQ);
    max_freq /= 1000;
    ALOGI("%s: Max Tuner Freq: %lld KHz", __FUNCTION__, max_freq);
    return max_freq;
}

EW_STB_TUNE_TCONST Wrapper_TuneGetActualTerrConstellation(U8BIT path)
{
    U16BIT tuner_client = findTunerClient(path);
    if (INVALID_TUNER_ID == tuner_client) {
        ALOGE("%s: path %d is invalid", __FUNCTION__, path);
        return WRAPPER_TUNE_TCONST_UNDEFINED;
    }

    ALOGD("%s: tuner_client: %d", __FUNCTION__, tuner_client);

    EW_STB_TUNE_TCONST terr_const = WRAPPER_TUNE_TCONST_UNDEFINED;
    if (WRAPPER_TUNER_STATE_LOCKED == Wrapper_TuneGetLockStatus(path)) {
        Frontend_Status stfrontendStatus = getFrontendStatus(tuner_client, FRONTEND_STATUS_TYPE_MODULATION);
        switch (stfrontendStatus.modulation) {
            case DVBT_CONSTELLATION_QPSK:
               terr_const = WRAPPER_TUNE_TCONST_QPSK;
               break;
            case DVBT_CONSTELLATION_16QAM:
                terr_const = WRAPPER_TUNE_TCONST_QAM16;
                break;
            case DVBT_CONSTELLATION_64QAM:
                terr_const = WRAPPER_TUNE_TCONST_QAM64;
                break;

            // WRAPPER_TUNE_TCONST_QAM128;

            case DVBT_CONSTELLATION_256QAM:
                terr_const = WRAPPER_TUNE_TCONST_QAM256;
                break;
            case DVBT_CONSTELLATION_UNDEFINED:
            default:
                terr_const = WRAPPER_TUNE_TCONST_UNDEFINED;
                break;
        }
    }
    else {
        ALOGD("%s: unlock", __FUNCTION__);
    }

    ALOGD("%s: path:%d terr_const:%d", __FUNCTION__, path, terr_const);

    return terr_const;
}
EW_STB_TUNE_HIERARCHY Wrapper_TuneGetActualTerrHierarchy(U8BIT path)
{
    U16BIT tuner_client = findTunerClient(path);
    if (INVALID_TUNER_ID == tuner_client) {
        ALOGE("%s: path %d is invalid", __FUNCTION__, path);
        return WRAPPER_TUNE_HIERARCHY_UNDEFINED;
    }

    ALOGD("%s: tuner_client: %d", __FUNCTION__, tuner_client);

    EW_STB_TUNE_HIERARCHY hierarchy = WRAPPER_TUNE_HIERARCHY_UNDEFINED;
    Frontend_Status stfrontendStatus = getFrontendStatus(tuner_client, FRONTEND_STATUS_TYPE_HIERARCHY);
    switch (stfrontendStatus.hierarchy) {
        case DVBT_HIERARCHY_NON_NATIVE:
           hierarchy = WRAPPER_TUNE_HIERARCHY_NONE;
           break;
        case DVBT_HIERARCHY_1_NATIVE:
           hierarchy = WRAPPER_TUNE_HIERARCHY_1;
           break;
        case DVBT_HIERARCHY_2_NATIVE:
           hierarchy = WRAPPER_TUNE_HIERARCHY_2;
           break;
        case DVBT_HIERARCHY_4_NATIVE:
           hierarchy = WRAPPER_TUNE_HIERARCHY_4;
           break;
/* TODO: how to INDEPTH
        case DVBT_HIERARCHY_NON_INDEPTH:
           hierarchy = WRAPPER_TUNE_HIERARCHY_8;
           break;
        case DVBT_HIERARCHY_1_INDEPTH:
           hierarchy = WRAPPER_TUNE_HIERARCHY_16;
           break;
        case DVBT_HIERARCHY_2_INDEPTH:
           hierarchy = WRAPPER_TUNE_HIERARCHY_32;
           break;
        case DVBT_HIERARCHY_4_INDEPTH:
           hierarchy = WRAPPER_TUNE_HIERARCHY_64;
           break;
*/
        case DVBT_HIERARCHY_UNDEFINED:
        case DVBT_HIERARCHY_AUTO:
        default:
           hierarchy = WRAPPER_TUNE_HIERARCHY_NONE;
           break;
    }

    ALOGD("%s: path:%d hierarchy:%d", __FUNCTION__, path, hierarchy);

    return hierarchy;
}

void Wrapper_TuneSetPLP(U8BIT path, U8BIT plp)
{
    if (!KEY_CONTAINED_IN_MAP(tuner_status_map, path)) {
        ALOGE("%s: path %d is invalid", __FUNCTION__, path);
        return;
    }

    ALOGD("%s: path:%d curr_tuner_plp:%d plp:%d",
          __FUNCTION__, path, tuner_status_map[path].tuner_plp, plp);

    if (tuner_status_map[path].tuner_plp != plp) {
        tuner_status_map[path].tuner_plp = plp;
        tuner_status_map[path].tuning_params_changed = TRUE;
    }
}

U8BIT Wrapper_TuneGetPLP(U8BIT path)
{
    U16BIT tuner_client = findTunerClient(path);
    if (INVALID_TUNER_ID == tuner_client) {
        ALOGE("%s: path %d is invalid", __FUNCTION__, path);
        return 0;
    }

    Frontend_Status stfrontendStatus = getFrontendStatus(tuner_client, FRONTEND_STATUS_TYPE_PLP_ID);
    U8BIT plp_id = (U8BIT)stfrontendStatus.plp_id;

    ALOGD("%s: path:%d plp: %d", __FUNCTION__, path, plp_id);
    return plp_id;
}

U16BIT Wrapper_TuneGetMPLPIDList(U8BIT path, U8BIT *plp_list, U16BIT listlen)
{
    if (!KEY_CONTAINED_IN_MAP(tuner_status_map, path)) {
        ALOGE("%s: path %d is invalid", __FUNCTION__, path);
        return 0;
    }
    U16BIT length = (U16BIT)tuner_status_map[path].t2_plp_list.size();
    ALOGD("%s: length = %d", __FUNCTION__,length);
    if (length <= listlen && length > 0)
    {
        U8BIT *list = tuner_status_map[path].t2_plp_list.data();
        memcpy(plp_list, list, length);
    }
    ALOGD("%s: path:%d plp len: %d", __FUNCTION__, path, length);
    return length;
}


U32BIT Wrapper_TuneGetActualSymbolRate(U8BIT path)
{
    if (!KEY_CONTAINED_IN_MAP(tuner_status_map, path)) {
        ALOGE("%s: path %d is invalid", __FUNCTION__, path);
        return 0;
    }

    U32BIT srate = tuner_status_map[path].tuner_srate;

    ALOGD("%s: path:%d tuner_srate:%d", __FUNCTION__, path, srate);
    return srate;
}
void Wrapper_TuneGetSupportedSystemType(U8BIT path, U8BIT *support_sys)
{
    if (support_sys != NULL) {
        support_sys[WRAPPER_TUNE_SYSTEM_TYPE_DVBT] = TRUE;
        support_sys[WRAPPER_TUNE_SYSTEM_TYPE_DVBT2] = TRUE;
        support_sys[WRAPPER_TUNE_SYSTEM_TYPE_DVBS] = TRUE;
        support_sys[WRAPPER_TUNE_SYSTEM_TYPE_DVBS2] = TRUE;
        support_sys[WRAPPER_TUNE_SYSTEM_TYPE_DVBC] = TRUE;
    }
}

U32BIT Wrapper_TuneGetMinTunerSymbolRate(U8BIT path)
{
    return getCurrentFrontendParameter(path, FRONTEND_PARAM_MIN_SRATE);
}

U32BIT Wrapper_TuneGetMaxTunerSymbolRate(U8BIT path)
{
    U32BIT symbol_rate;

    symbol_rate = getCurrentFrontendParameter(path, FRONTEND_PARAM_MAX_SRATE);
    if (symbol_rate == 0)
    {
        symbol_rate = 0x0FFFFFFF;
    }
    return symbol_rate;
}

void Wrapper_TuneInitialise(U8BIT paths)
{
    ALOGD("%s: clear tuner_status_map", __FUNCTION__);
    tuner_status_map.clear();
}
void Wrapper_TuneSetActualTsInputIdx(U8BIT path, S32BIT frontend_fd)
{
    ALOGD("%s: not support", __FUNCTION__);
}
void Wrapper_TuneSetActualSupportedSystemType(U8BIT path, S32BIT frontend_fd)
{
    ALOGD("%s: not support", __FUNCTION__);
}
void Wrapper_TuneSetSystemType(U8BIT path, EW_STB_TUNE_SYSTEM_TYPE type)
{
    if (KEY_CONTAINED_IN_MAP(tuner_status_map, path)) {
        ALOGD("%s: path:%d curr_sys_type:%d type:%d",
              __FUNCTION__, path, tuner_status_map[path].sys_type, type);
        if (tuner_status_map[path].sys_type != type) {
            tuner_status_map[path].sys_type = type;
            tuner_status_map[path].tuning_params_changed = TRUE;
        }
    }
    else {
        WRAPPER_TUNER_STATUS tuner_status;
        tuner_status.sys_type = type;
        ALOGD("%s: insert path:%d sys_type:%d into map", __FUNCTION__, path, type);
        MAP_INSERT_ITEM(tuner_status_map, path, tuner_status);
    }
}
EW_STB_TUNE_SYSTEM_TYPE Wrapper_TuneGetSystemType(U8BIT path)
{
    EW_STB_TUNE_SYSTEM_TYPE sys_type = WRAPPER_TUNE_SYSTEM_TYPE_UNKNOWN;

    if (KEY_CONTAINED_IN_MAP(tuner_status_map, path)) {
        ALOGD("%s: path:%d is contained in map", __FUNCTION__, path);
        sys_type = tuner_status_map[path].sys_type;
    }

    ALOGD("%s: path:%d sys_type:%d", __FUNCTION__, path, sys_type);
    return sys_type;
}
void Wrapper_TuneAutoRelock(U8BIT path, BOOLEAN state)
{
    if (KEY_CONTAINED_IN_MAP(tuner_status_map, path)) {
        ALOGD("%s: path:%d curr_auto_relock:%d state:%d",
              __FUNCTION__, path, tuner_status_map[path].auto_relock, state);

        if (tuner_status_map[path].auto_relock != state) {
            tuner_status_map[path].auto_relock = state;
            tuner_status_map[path].tuning_params_changed = TRUE;
        }
    }
    else {
        WRAPPER_TUNER_STATUS tuner_status;
        tuner_status.auto_relock = state;
        ALOGD("%s: insert path:%d auto_relock:%d into map", __FUNCTION__, path, state);
        MAP_INSERT_ITEM(tuner_status_map, path, tuner_status);
    }
}
EW_STB_TUNE_TCODERATE Wrapper_TuneGetActualTerrLpCodeRate(U8BIT path)
{
    ALOGD("%s: not support", __FUNCTION__);
    return WRAPPER_TUNE_TCODERATE_UNDEFINED;
}
EW_STB_TUNE_TCODERATE Wrapper_TuneGetActualTerrHpCodeRate(U8BIT path)
{
    ALOGD("%s: not support", __FUNCTION__);
    return WRAPPER_TUNE_TCODERATE_UNDEFINED;
}
EW_STB_TUNE_TGUARDINT Wrapper_TuneGetActualTerrGuardInt(U8BIT path)
{
    ALOGD("%s: not support", __FUNCTION__);
    return WRAPPER_TUNE_TGUARDINT_UNDEFINED;
}
U16BIT Wrapper_TuneGetActualTerrCellId(U8BIT path)
{
    ALOGD("%s: not support", __FUNCTION__);
    return 0xFFFF;
}
void Wrapper_TuneUpdateFeUsage(U8BIT path, BOOLEAN use)
{
    ALOGD("start:%s", __FUNCTION__);
    if (KEY_CONTAINED_IN_MAP(tuner_status_map, path)) {
        ALOGD("%s: path:%d is contained in map", __FUNCTION__, path);
    }
    else {
        WRAPPER_TUNER_STATUS tuner_status;
        ALOGD("%s: insert path:%d into map", __FUNCTION__, path);
        MAP_INSERT_ITEM(tuner_status_map, path, tuner_status);
    }

    U8BIT usage = tuner_status_map[path].frontend_usage;
    if (use) {
        usage++;
    }
    else if (usage > 0) {
        usage--;
    }
    else {
        ALOGE("%s: usage is 0", __FUNCTION__);
    }
    tuner_status_map[path].frontend_usage = usage;
    ALOGD("%s: frontend_usage:%d use:%d", __FUNCTION__, tuner_status_map[path].frontend_usage, use);
}
BOOLEAN Wrapper_TuneIsTvPlatform()
{
    return TRUE;
}

void Wrapper_TuneRestartTuner(U8BIT path)
{
    ALOGD("%s: not support", __FUNCTION__);
}
void Wrapper_TuneSetSearchMode(U8BIT path, BOOLEAN mode)
{
    U16BIT tuner_client = INVALID_TUNER_ID;
    if (KEY_CONTAINED_IN_MAP(tuner_status_map, path)) {
        ALOGD("%s: path:%d is contained in map", __FUNCTION__, path);
        tuner_client = findTunerClient(path);
        if (INVALID_TUNER_ID == tuner_client) {
            tuner_client = (U16BIT)Am_tuner_getTunerClientIdByType(getTunerType(path));
            if (INVALID_TUNER_ID == tuner_client)
            {
                ALOGE("%s: tuner_client is invalid", __FUNCTION__);
                return ;
            }

            ALOGD("%s: path:%d client:%d", __FUNCTION__, path, tuner_client);
            tuner_status_map[path].tuner_client = tuner_client;
        }
    }
    else {
        tuner_client = (U16BIT)Am_tuner_getTunerClientIdByType(getTunerType(path));
        if (INVALID_TUNER_ID == tuner_client)
        {
            ALOGE("%s: tuner_client is invalid", __FUNCTION__);
            return ;
        }
        WRAPPER_TUNER_STATUS tuner_status;
        tuner_status.tuner_client = tuner_client;
        ALOGD("%s: insert path:%d client:%d into map", __FUNCTION__, path, tuner_client);
        MAP_INSERT_ITEM(tuner_status_map, path, tuner_status);
    }

    if (Wrapper_TuneIsTvPlatform()) {
        ALOGD("%s: curr_search_mode:%d mode:%d",
              __FUNCTION__, tuner_status_map[path].tuner_search_mode, mode);

        if (tuner_status_map[path].tuner_search_mode != mode) {
            tuner_status_map[path].tuner_search_mode = mode;
            tuner_status_map[path].tuning_params_changed = TRUE;
        }
    }
}
BOOLEAN Wrapper_TuneIsSearchMode(U8BIT path)
{
    if (!KEY_CONTAINED_IN_MAP(tuner_status_map, path)) {
        ALOGE("%s: path %d is invalid", __FUNCTION__, path);
        return FALSE;
    }

    BOOLEAN search_mode = FALSE;
    if (Wrapper_TuneIsTvPlatform()) {
        search_mode = tuner_status_map[path].tuner_search_mode;
    }

    ALOGD("%s: path:%d search_mode:%d", __FUNCTION__, path, search_mode);

    return search_mode;
}
void Wrapper_TuneAllStart()
{
    ALOGD("%s: Do nothing", __FUNCTION__);
}

void Wrapper_TuneAllStop()
{
    for (auto iter = tuner_status_map.begin(); iter != tuner_status_map.end(); ++iter) {
        U16BIT tuner_client = iter->second.tuner_client;
        if (tuner_client == INVALID_TUNER_ID) {
            ALOGE("%s path:%d is invalid", __FUNCTION__, iter->first);
            continue;
        }

        ALOGD("%s: tuner_client: %d", __FUNCTION__, tuner_client);
        Am_tuner_closeFrontend(tuner_client);

        iter->second.tuner_client = INVALID_TUNER_ID;
        iter->second.current_tuning = FALSE;
        iter->second.tune_lock = FALSE;
    }
}

void Wrapper_TuneSetFrontendFd(U8BIT path, U32BIT fe_fd)
{
    ALOGD("%s: not support", __FUNCTION__);
}
//dvb-c
EW_STB_TUNE_CMODE Wrapper_TuneGetActualCableMode(U8BIT path)
{
    U16BIT tuner_client = findTunerClient(path);
    if (INVALID_TUNER_ID == tuner_client) {
        ALOGE("%s: path %d is invalid", __FUNCTION__, path);
        return WRAPPER_TUNE_MODE_QAM_UNDEFINED;
    }

    ALOGD("%s: tuner_client: %d", __FUNCTION__, tuner_client);

    EW_STB_TUNE_CMODE cmode = WRAPPER_TUNE_MODE_QAM_UNDEFINED;
    Frontend_Status stfrontendStatus = getFrontendStatus(tuner_client, FRONTEND_STATUS_TYPE_MODULATION);
    switch (stfrontendStatus.modulation) {
           // WRAPPER_TUNE_MODE_QAM_4;
           //  WRAPPER_TUNE_MODE_QAM_8;
        case DVBC_MODULATION_MOD_16QAM:
           cmode = WRAPPER_TUNE_MODE_QAM_16;
           break;
        case DVBC_MODULATION_MOD_32QAM:
           cmode = WRAPPER_TUNE_MODE_QAM_32;
           break;
        case DVBC_MODULATION_MOD_64QAM:
           cmode = WRAPPER_TUNE_MODE_QAM_64;
           break;
        case DVBC_MODULATION_MOD_128QAM:
           cmode = WRAPPER_TUNE_MODE_QAM_128;
           break;
        case DVBC_MODULATION_MOD_256QAM:
           cmode = WRAPPER_TUNE_MODE_QAM_256;
           break;
        default:
           cmode = WRAPPER_TUNE_MODE_QAM_UNDEFINED;
           break;
    }

    ALOGD("%s: path:%d modulation:%u cmode:%u", __FUNCTION__,
          path, stfrontendStatus.modulation, cmode);

    return cmode;
}

//dvb-s
EW_STB_TUNE_MODULATION Wrapper_TuneGetModulation(U8BIT path)
{
    if (!KEY_CONTAINED_IN_MAP(tuner_status_map, path)) {
        ALOGE("%s: path %d is invalid", __FUNCTION__, path);
        return WRAPPER_TUNE_MOD_AUTO;
    }

    ALOGD("%s: signal_type:%d tune_modulation:%d", __FUNCTION__,
          tuner_status_map[path].signal_type, tuner_status_map[path].tune_modulation);

    if  (E_TERR_TYPE_DVBS == tuner_status_map[path].signal_type) {
        return tuner_status_map[path].tune_modulation;
    }

    return WRAPPER_TUNE_MOD_AUTO;
}
void Wrapper_TuneSetModulation(U8BIT path, EW_STB_TUNE_MODULATION modulation)
{
    if (!KEY_CONTAINED_IN_MAP(tuner_status_map, path)) {
        ALOGE("%s: path %d is invalid", __FUNCTION__, path);
        return;
    }

    ALOGD("%s: curr_modulation:%d modulation:%d",
          __FUNCTION__, tuner_status_map[path].tune_modulation, modulation);

    if (tuner_status_map[path].tune_modulation != modulation) {
        tuner_status_map[path].tune_modulation = modulation;
        tuner_status_map[path].tuning_params_changed = TRUE;
    }
}
void Wrapper_TuneSetLOFrequency(U8BIT path, S32BIT lo_freq)
{
    if (!KEY_CONTAINED_IN_MAP(tuner_status_map, path)) {
        ALOGE("%s: path %d is invalid", __FUNCTION__, path);
        return;
    }

    ALOGD("%s: curr_lo_freq %d lo_freq %d",
          __FUNCTION__, tuner_status_map[path].tuner_lo_freq, lo_freq);

    if (tuner_status_map[path].tuner_lo_freq != lo_freq) {
        tuner_status_map[path].tuner_lo_freq = lo_freq;
        tuner_status_map[path].tuning_params_changed = TRUE;
    }
}
EW_STB_TUNE_LNB_VOLTAGE Wrapper_TuneGetLNBVoltage(U8BIT path)
{
    if (!KEY_CONTAINED_IN_MAP(tuner_status_map, path)) {
        ALOGE("%s: path %d is invalid", __FUNCTION__, path);
        return WRAPPER_LNB_VOLTAGE_OFF;
    }

    ALOGD("%s: signal_type:%d tune_voltage:%d", __FUNCTION__,
          tuner_status_map[path].signal_type, tuner_status_map[path].tune_voltage);

    if (E_TERR_TYPE_DVBS == tuner_status_map[path].signal_type) {
        return tuner_status_map[path].tune_voltage;
    }

    return WRAPPER_LNB_VOLTAGE_OFF;
}
void Wrapper_TuneSetLNBVoltage(U8BIT path, EW_STB_TUNE_LNB_VOLTAGE voltage, BOOLEAN retune)
{
    if (!KEY_CONTAINED_IN_MAP(tuner_status_map, path)) {
        ALOGE("%s: path %d is invalid", __FUNCTION__, path);
        return;
    }

    if (!openLnb(path)) {
        ALOGE("%s: open lnb failed", __FUNCTION__);
        return;
    }

    ALOGD("%s: signal_type:%d curr_voltage:%d voltage:%d retune:%d",
          __FUNCTION__, tuner_status_map[path].signal_type, tuner_status_map[path].tune_voltage, voltage, retune);

    if (E_TERR_TYPE_DVBS == tuner_status_map[path].signal_type) {
        if (tuner_status_map[path].tune_voltage != voltage) {
            tuner_status_map[path].tune_voltage = voltage;
            if (retune) {
                tuner_status_map[path].tuning_params_changed = TRUE;
            }
        }
        setLnbVoltage(tuner_status_map[path].tuner_lnb, voltage);
    }
}

BOOLEAN Wrapper_TuneGet22kState(U8BIT path)
{
    if (!KEY_CONTAINED_IN_MAP(tuner_status_map, path)) {
        ALOGE("%s: path %d is invalid", __FUNCTION__, path);
        return FALSE;
    }

    ALOGD("%s: signal_type:%d 22khz:%d",
          __FUNCTION__, tuner_status_map[path].signal_type, tuner_status_map[path].use_22khz);

    if (E_TERR_TYPE_DVBS == tuner_status_map[path].signal_type) {
        return  tuner_status_map[path].use_22khz;
    }

    return FALSE;
}
void Wrapper_TuneSet22kState(U8BIT path, BOOLEAN state, BOOLEAN retune)
{
    if (!KEY_CONTAINED_IN_MAP(tuner_status_map, path)) {
        ALOGE("%s: path %d is invalid", __FUNCTION__, path);
        return;
    }

    ALOGD("%s: signal_type:%d curr_22khz:%d state:%d retune:%d",
          __FUNCTION__, tuner_status_map[path].signal_type, tuner_status_map[path].use_22khz, state, retune);

    if (E_TERR_TYPE_DVBS == tuner_status_map[path].signal_type) {
        if (!openLnb(path)) {
            ALOGE("%s: open lnb failed", __FUNCTION__);
            return;
        }

        if (tuner_status_map[path].use_22khz != state) {
            tuner_status_map[path].use_22khz = state;
            if (retune) {
                tuner_status_map[path].tuning_params_changed = TRUE;
            }
        }
        setLnbTone(tuner_status_map[path].tuner_lnb, state);
    }
}

void Wrapper_TuneSendDISEQCMessage(U8BIT path, U8BIT *data, U8BIT size)
{
    if (!openLnb(path)) {
        ALOGE("%s: open lnb failed", __FUNCTION__);
        return;
    }

    if (size > 0) {
        std::vector<char> message;
        for (U8BIT i = 0; i < size; i++) {
            ALOGD("%s: [0x%02x]", __FUNCTION__, data[i]);
            message.push_back(static_cast<char>(data[i]));
        }
        Am_lnb_sendDiseqcMessage(tuner_status_map[path].tuner_lnb, message);
    }
}
void Wrapper_TuneSendBurstMessage(U8BIT path, U8BIT data)
{
    if (!openLnb(path)) {
        ALOGE("%s: open lnb failed", __FUNCTION__);
        return;
    }

    ALOGD("%s: [0x%02x]", __FUNCTION__, data);

    DVBS_LNB_POSITION pos = DVBS_LNB_POSITION_UNDEFINED;
    if (data == 0x00 || data == 0xFF) {
        if (data == 0x00) {
            pos = DVBS_LNB_POSITION_POSITION_A;
        }
        else {
            pos = DVBS_LNB_POSITION_POSITION_B;
        }
        Am_lnb_setSatellitePosition(tuner_status_map[path].tuner_lnb, pos);
    }
}

void Wrapper_TuneReceiveDISEQCReply(U8BIT path, U8BIT *data, U8BIT size, U32BIT timeout)
{
    ALOGD("%s: not support", __FUNCTION__);
}

BOOLEAN Wrapper_Tune_BlindScan(U8BIT path, E_TTYPE sys_type, Wrapper_Tune_BlindCallback_t cb, void *user_data,
                                      unsigned int start_freq, unsigned int stop_freq, EW_STB_TUNE_BlindUnicable_t unicable)
{
    U16BIT client_id = (U16BIT)Am_tuner_getTunerClientIdByType(getTunerType(path));

    if (!KEY_CONTAINED_IN_MAP(tuner_status_map, path)) {
        ALOGE("%s: path:%d isn't contained in map, client_id:%d", __FUNCTION__, path, client_id);
        return FALSE;
    }

    tuner_status_map[path].tune_lock = FALSE;
    tuner_status_map[path].tuner_client = client_id;
    tuner_status_map[path].blindscan_tp_freq.clear();
    tuner_status_map[path].blindscan_tp_srate.clear();

    if (!openLnb(path)) {
        ALOGE("%s: open lnb failed", __FUNCTION__);
        return FALSE;
    }

    bool attached = false;
    JNIEnv *env = Am_tuner_getJNIEnv(&attached);
    if (NULL == env) {
        ALOGE("%s: env is null", __FUNCTION__);
        return FALSE;
    }

    ALOGD("%s: signal_type:%d freq range[%u,%u]", __FUNCTION__, tuner_status_map[path].signal_type, start_freq, stop_freq);

    if (tuner_status_map[path].signal_type == E_TERR_TYPE_DVBC) {
        // TODO
    }
    else if(tuner_status_map[path].signal_type == E_TERR_TYPE_DVBS) {
        tuner_status_map[path].blindscan_event_cb = cb;
        tuner_status_map[path].blindscan_cb_user_data = user_data;

        Dvbs_Frontend_Settings dvbsFrontendSettings;
        memset(&dvbsFrontendSettings, 0, sizeof(Dvbs_Frontend_Settings));

        dvbsFrontendSettings.frequency = start_freq * 1000 * 1000;      //MHz to Hz
        dvbsFrontendSettings.end_frequency = stop_freq * 1000 * 1000;   //MHz to Hz

        jobject dvbsSettingObject = dvb_utils_getDvbsFrontendSettingsObject(env, dvbsFrontendSettings);
        if (NULL == dvbsSettingObject) {
            ALOGE("%s: not get dvbs frontend settings object", __FUNCTION__);
            if (attached) {
                Am_tuner_detachJNIEnv();
            }
            return FALSE;
        }
        Am_tuner_scan(client_id, dvbsSettingObject, SCAN_TYPE_BLIND, (long)blindscanCallback);
    }

    if (attached) {
        Am_tuner_detachJNIEnv();
    }

    tuner_status_map[path].current_tuning = TRUE;
    tuner_status_map[path].blindscan_mode = TRUE;
    tuner_status_map[path].tuning_params_changed = FALSE;

    return TRUE;
}
BOOLEAN Wrapper_Tune_BlindExit(U8BIT path)
{
    U16BIT tuner_client = findTunerClient(path);
    if (tuner_client == INVALID_TUNER_ID) {
        ALOGE("%s path:%d is invalid", __FUNCTION__, path);
        return FALSE;
    }

    ALOGD("%s path:%d client_id:%d DVBS Blind Scan", __FUNCTION__, path, tuner_client);
    Am_tuner_cancelScanning(tuner_client);
    if (E_TERR_TYPE_DVBS == tuner_status_map[path].signal_type) {
        closeLnb(path);
    }

    tuner_status_map[path].blindscan_mode = FALSE;
    tuner_status_map[path].tuner_client = INVALID_TUNER_ID;
    tuner_status_map[path].current_tuning = FALSE;
    tuner_status_map[path].tune_lock = FALSE;

    return TRUE;
}
void Wrapper_Tune_BlindGetTPCount(U8BIT path, U16BIT *count)
{
    if (!KEY_CONTAINED_IN_MAP(tuner_status_map, path)) {
        ALOGE("%s: path:%d isn't contained in map", __FUNCTION__, path);
        return;
    }

    if (tuner_status_map[path].blindscan_tp_freq.size() != tuner_status_map[path].blindscan_tp_srate.size()) {
        ALOGE("%s freq size=%d srate size=%d",
              __FUNCTION__, tuner_status_map[path].blindscan_tp_freq.size(), tuner_status_map[path].blindscan_tp_srate.size());
        *count = 0;
    }
    else {
        *count = (U16BIT)(tuner_status_map[path].blindscan_tp_freq.size());
    }
    ALOGD("%s: TP count: %d", __FUNCTION__, *count);
}
BOOLEAN Wrapper_Tune_BlindGetTPInfo(U8BIT path, U32BIT** freq, U32BIT** srate, U16BIT *count)
{
    if (!KEY_CONTAINED_IN_MAP(tuner_status_map, path)) {
        ALOGE("%s: path:%d isn't contained in map", __FUNCTION__, path);
        return FALSE;
    }

    if (tuner_status_map[path].blindscan_tp_freq.size() != tuner_status_map[path].blindscan_tp_srate.size()) {
        ALOGE("%s freq size=%d srate size=%d",
              __FUNCTION__, tuner_status_map[path].blindscan_tp_freq.size(), tuner_status_map[path].blindscan_tp_srate.size());
        *count = 0;
        return FALSE;
    }
    else {
        *count =  (U16BIT)(tuner_status_map[path].blindscan_tp_freq.size());
        *freq = tuner_status_map[path].blindscan_tp_freq.data();
        *srate = tuner_status_map[path].blindscan_tp_srate.data();
    }

    ALOGD("%s: Blind scan result: %d", __FUNCTION__, *count);
    for (U16BIT i = 0; i < *count; i++) {
        ALOGD("%s: ===== TP[%d]: Freq %d Srate %d =====", __FUNCTION__, i, (*freq)[i], (*srate)[i]);
    }

    return TRUE;
}

