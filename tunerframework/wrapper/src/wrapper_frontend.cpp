#include "wrapper_frontend.h"
#include <map>
#include <vector>
#include <jni.h>
#include "JNI_tuner.h"
#include "filter_utils.h"
#include "dvb_frontend_setting_utils.h"
#include "type_change_utils.h"
#include "frontend_utils.h"

#define LOG_TAG "wrapper_frontend"

using namespace android;
using namespace std;

using ::android::sp;
using ::android::RefBase;

#define MAP_INSERT_ITEM(__MAP__, __KEY__, __VALUE__) __MAP__.insert(std::make_pair(__KEY__, __VALUE__))
#define WRPPER_HW_EV_CLASS_TUNER             3
#define WRPPER_HW_EV_TYPE_LOCKED             2
#define WRPPER_HW_EV_TYPE_NOTLOCKED          3
void (*Lock_SendEvent) (BOOLEAN repeat, U16BIT event_class, U16BIT event_type, void *data, U32BIT data_size);

typedef map<U8BIT, int> TUNER_MAP;

static int gTunerClient = INVALID_TUNER_ID;

static std::vector<int> gFrontendList;
//static jobject gWeakRefPmtFilter = NULL;
static jobject gWeakRefPatFilter = NULL;
static TUNER_MAP tuner_map;

static auto testFailLeave = [](bool attached){if (attached) Am_tuner_detachJNIEnv();};
static BOOLEAN isTvPlatform = FALSE;

// tuner parameter
static EW_STB_TUNE_TBWIDTH TBWidth = WRAPPER_TUNE_TBWIDTH_8MHZ;
static E_TTYPE signal_type = E_TERR_TYPE_DVBT;
static EW_STB_TUNE_SYSTEM_TYPE sys_type = WRAPPER_TUNE_SYSTEM_TYPE_UNKNOWN;
static EW_STB_TUNE_TMODE Tmode = WRAPPER_TUNE_MODE_COFDM_UNDEFINED;
static EE_TUNER_STATE tuner_state  = E_TUNER_IDLE;
static EW_STB_TUNE_CMODE cable_mode = WRAPPER_TUNE_MODE_QAM_UNDEFINED;
static EW_STB_TUNE_MODULATION tune_modulation = WRAPPER_TUNE_MOD_AUTO;
static EW_STB_TUNE_LNB_VOLTAGE tune_voltage = WRAPPER_LNB_VOLTAGE_OFF;

static U8BIT curr_path = 0;
static U8BIT tuner_plp = 0;
static U8BIT frontend_usage = 0;
static U16BIT tuner_lo_freq;
static BOOLEAN tuner_search_mode = FALSE;
//static BOOLEAN  tuner_lock = FALSE;
static BOOLEAN use_22khz = FALSE;
static BOOLEAN tuning_params_changed = FALSE;
static BOOLEAN curr_starttune = FALSE;


static U32BIT tuner_srate = 0;
static BOOLEAN auto_relock= FALSE;

static Wrapper_Tnue_BlindCallback_t blindscan_event_cb = NULL;
static void* blindscan_cb_user_data = NULL;
static std::vector<U32BIT> blindscan_tp_freq;
static std::vector<U32BIT> blindscan_tp_srate;
static BOOLEAN blindscan_mode = FALSE;

static jobject tuner_lnb;
DVBT_BANDWIDTH Wrapper_TuneGetTerrBwidth(EW_STB_TUNE_TBWIDTH tbwidth);
DVBT_TRANSMISSION_MODE Wrapper_TuneGetTransmissionMode(EW_STB_TUNE_TMODE tmode);

void blindscanCallback(int scanCallbackMessageType, jobjectArray scanCallbackMessage);
void LnbCallback(jobject lnb, int eventType, jbyteArray diseqcMessage);

BOOLEAN tuner_getFrontendIds(U8BIT path)
{
    ALOGD("start:%s", __FUNCTION__);
    bool ret = FALSE;

    jobject list = Am_tuner_getFrontendIds(gTunerClient);
    if (NULL == list) {
        ALOGD("%s : test fail, list is null", __FUNCTION__);
        return ret;
    }

    bool attached = false;
    JNIEnv *env = Am_tuner_getJNIEnv(&attached);
    if (NULL == env) {
        ALOGD("%s : test fail, env is null", __FUNCTION__);
        return ret;
    }
    //1.Test jobject class
    jclass listClazz = env->FindClass("java/util/List");
    if (JNI_TRUE != env->IsInstanceOf(list, listClazz)) {
        ALOGD("%s : test fail, not list object", __FUNCTION__);
        testFailLeave(attached);
        return ret;
    }
    //2.show Frontend Id
    jmethodID listSize = env->GetMethodID(listClazz, "size", "()I");
    jmethodID listGet = env->GetMethodID(listClazz, "get", "(I)Ljava/lang/Object;");

    int frontendSize = env->CallIntMethod(list, listSize);
    ALOGD("%s : frontend size : %d", __FUNCTION__, frontendSize);
    for (int i=0; i < frontendSize; i++) {
        jobject id = env->CallObjectMethod(list, listGet, i);
        jclass integerClazz = env->FindClass("java/lang/Integer");
        if (JNI_TRUE != env->IsInstanceOf(id, integerClazz)) {
            ALOGD("%s : test fail, id is not integer", __FUNCTION__);
            testFailLeave(attached);
            return ret;
        }
        jmethodID intValue = env->GetMethodID(integerClazz, "intValue", "()I");
        int frontendId = env->CallIntMethod(id, intValue);
        ALOGD(":%s, frontendId : %d", __FUNCTION__, frontendId);
        gFrontendList.push_back(frontendId);
    }
    if (attached) {
        Am_tuner_detachJNIEnv();
    }
    int ListSize = gFrontendList.size();
    if (ListSize > 0)
    {
        ret = TRUE;
        ALOGD("%s : frontendSize : %d", __FUNCTION__, ListSize);
    }
    env->DeleteWeakGlobalRef(list);
    ALOGD("end:%s", __FUNCTION__);
    return ret;
}

Frontend_Status tuner_getFrontendStatus(int tuner_client, FRONTEND_STATUS_TYPE DataType)
{
    ALOGD("start:%s", __FUNCTION__);
    Frontend_Status stfrontendStatus;
    memset(&stfrontendStatus, 0, sizeof(Frontend_Status));
    if (INVALID_TUNER_ID == tuner_client) {
        ALOGD("%s : tuner_client is invalid", __FUNCTION__);
        return stfrontendStatus;
    }

    bool attached = false;
    JNIEnv *env = Am_tuner_getJNIEnv(&attached);
    if (NULL == env) {
        ALOGD("%s : test fail, env is null", __FUNCTION__);
        return stfrontendStatus;
    }
    FRONTEND_STATUS_TYPE aStatusTypes = DataType;

    jintArray jaStatusTypes = TypeChangeUtils::getJNIArray(env, (int*)&aStatusTypes, 1);
    jobject frontendStatusObject = Am_tuner_getFrontendStatus(tuner_client, jaStatusTypes);
    frontend_utils_parseFrontendStatus(env, frontendStatusObject, &stfrontendStatus);

    if (attached) {
        Am_tuner_detachJNIEnv();
    }
    env->DeleteWeakGlobalRef(frontendStatusObject);
    ALOGD("end:%s", __FUNCTION__);
    return stfrontendStatus;
}
bool GetTunerLockStatus(int tuner_client)
{
    ALOGD("start:%s", __FUNCTION__);
    bool ret = FALSE;
    if (INVALID_TUNER_ID == tuner_client)
    {
        ALOGD("%s : tuner_client is invalid", __FUNCTION__);
        return FALSE;
    }
    Frontend_Status stfrontendStatus;
    memset(&stfrontendStatus, 0, sizeof(Frontend_Status));

    bool attached = false;
    JNIEnv *env = Am_tuner_getJNIEnv(&attached);
    if (NULL == env) {
        ALOGD("%s : test fail, env is null", __FUNCTION__);
        return FALSE;
    }
    int aStatusTypes = FRONTEND_STATUS_TYPE_DEMOD_LOCK;

    jintArray jaStatusTypes = TypeChangeUtils::getJNIArray(env, &aStatusTypes, 1);
    jobject frontendStatusObject = Am_tuner_getFrontendStatus(tuner_client, jaStatusTypes);
    frontend_utils_parseFrontendStatus(env, frontendStatusObject, &stfrontendStatus);

    if (attached) {
        Am_tuner_detachJNIEnv();
    }
    if (1 == stfrontendStatus.is_demod_locked)
    {
        ret = TRUE;
    }else
    {
        ret = FALSE;
    }
    env->DeleteWeakGlobalRef(frontendStatusObject);
    ALOGD("end:%s ret:%d", __FUNCTION__, ret);
    return ret;

}
void PatFilterCallback(jobject filter, jobjectArray filterEventArray, int filterStatus)
{
    ALOGD("start:%s", __FUNCTION__);

    bool attached = false;
    JNIEnv *env = Am_tuner_getJNIEnv(&attached);
    if (NULL == env) {
        ALOGD("%s : test fail, env is null", __FUNCTION__);
        return;
    }
    //show filter status
    ALOGD("filterStatus : %d", filterStatus);

    //handle PAT Filter Event
    if (NULL != filterEventArray) {
        int eventSize = env->GetArrayLength(filterEventArray);
        for (int index = 0; index < eventSize; index++) {
            //1.check section event
            jobject filterEvent = env->GetObjectArrayElement(filterEventArray, index);
            Section_Event stSectionEvent;
            memset(&stSectionEvent, 0, sizeof(Section_Event));
            //get data length
            filter_utils_getSectionEvent(env, filterEvent, &stSectionEvent);

            ALOGD("tableId :%d, version :%d, section num :%d, data length :%d", stSectionEvent.tableId, stSectionEvent.version,
                stSectionEvent.sectionNum, stSectionEvent.dataLength);

            //3.read section data
            char *buffer = new char[stSectionEvent.dataLength];
            int readSize = Am_filter_read(filter, buffer, 0, stSectionEvent.dataLength);
            ALOGD("read Pat data size :%d ", readSize);
            if (readSize > stSectionEvent.dataLength) {
                ALOGD("%s : test fail, read data too long than real data size", __FUNCTION__);
            } else {
                for (int i = 0; i < readSize; i++) {
                    ALOGD("0X%x ", buffer[i]);
                }
            }
            delete[] buffer;
        }
    }
    if (attached) {
        Am_tuner_detachJNIEnv();
    }
    ALOGD("end:%s", __FUNCTION__);
}

void RegisterCallback(SendEvent callback)
{
     Lock_SendEvent  = callback;
}

void scanCallback(int event) {
    ALOGD("sh1164-->:%s : enter tuner event : %d", __FUNCTION__, event);
    int tuner_client = INVALID_TUNER_ID;
    TUNER_MAP::iterator it = tuner_map.find( curr_path );
    if (it != tuner_map.end())
    {
        tuner_client = it->second;
    }else
    {
        ALOGD("tuner_client is invalid: %d ", tuner_client);
        //return;
    }
    //3.test get tuner status

    if (0 == event)
    {
        Lock_SendEvent(FALSE, WRPPER_HW_EV_CLASS_TUNER, WRPPER_HW_EV_TYPE_LOCKED, &curr_path, sizeof(U8BIT));
        ALOGD("scanCallback: tuner lock");
    }
    else
    {
        Lock_SendEvent(FALSE, WRPPER_HW_EV_CLASS_TUNER, WRPPER_HW_EV_TYPE_NOTLOCKED, &curr_path, sizeof(U8BIT));
        ALOGD("scanCallback: tuner unlock");
    }

    ALOGD("%s : leave tuner event : %d,curr_path=%d", __FUNCTION__, event, curr_path);
}
void blindscanCallback(int scanCallbackMessageType, jobjectArray scanCallbackMessage) {
    ALOGD("start:%s, scanCallbackMessageType :%d ", __FUNCTION__, scanCallbackMessageType);
    Scan_Callback_Message scanMessage;
    memset(&scanMessage, 0, sizeof(Scan_Callback_Message));
    if (NULL != scanCallbackMessage) {
        bool attached = false;
        JNIEnv *env = Am_tuner_getJNIEnv(&attached);
        if (NULL == env) {
            ALOGD("%s : test fail, env is null", __FUNCTION__);
            return;
        }
        frontend_utils_parseScanCallbackMessage(env, scanCallbackMessageType, scanCallbackMessage, &scanMessage);
        ALOGD("Scan_Callback_Message scanMessageType : %d", scanMessage.scanMessageType);
        for (int value : scanMessage.integer_value) {
            ALOGD("Scan_Callback_Message message : %d", value);
        }
    }

    switch (scanMessage.scanMessageType)
    {
        case SCAN_MESSAGE_PROGRESS_PERCENT:
            if (scanMessage.integer_value.size() == 1)
            {
                U8BIT progress = scanMessage.integer_value[0];
                if (blindscan_event_cb != NULL)
                {
                    EW_STB_TUNE_BlindEvent_t evt;
                    evt.status = WRAPPER_AM_FEND_BLIND_UPDATEPROCESS;
                    evt.process = progress;
                    blindscan_event_cb(curr_path, &evt, blindscan_cb_user_data);
                }
            }
            break;
        case SCAN_MESSAGE_FREQUENCY:
            if (scanMessage.integer_value.size() == 1)
            {
                U32BIT freq = scanMessage.integer_value[0];
                blindscan_tp_freq.push_back(freq);

            }
            break;
        case SCAN_MESSAGE_SYMBOL_RATE:
            if (scanMessage.integer_value.size() == 1)
            {
                U32BIT srate = scanMessage.integer_value[0];
                blindscan_tp_srate.push_back(srate);
            }
            break;

        case SCAN_MESSAGE_END:
            break;
        default:
            ALOGD("scanCallback: default");
            break;
    }
    ALOGD("end:%s", __FUNCTION__);
}

void LnbCallback(jobject lnb, int eventType, jbyteArray diseqcMessage) {
    ALOGD("start:%s, lnb : %p, eventType :%d", __FUNCTION__, lnb, eventType);

    bool attached = false;
    JNIEnv *env = Am_tuner_getJNIEnv(&attached);
    if (NULL == env) {
        ALOGD("%s : test fail, env is null", __FUNCTION__);
        return;
    }
    if (NULL != diseqcMessage) {
        std::vector<char> message;
        TypeChangeUtils::getCharVector(env, diseqcMessage, &message);
        for (int i = 0; i < message.size(); i++) {
            ALOGD("0X%x ", message[i]);
        }
    }
    ALOGD("end:%s", __FUNCTION__);
}

//bin.luo   PR1
void Wrapper_TuneStartTuner(U8BIT path, U32BIT freq, U32BIT srate, EW_STB_TUNE_FEC fec, S8BIT freq_off, EW_STB_TUNE_TMODE tmode, EW_STB_TUNE_TBWIDTH tbwidth, EW_STB_TUNE_CMODE cmode, EW_STB_TUNE_ANALOG_VIDEO_TYPE anlg_vtype)
{
    ALOGD("start:%s   path:%d  freq:%d  cmode:%d srate:%d", __FUNCTION__,path, freq, cmode,srate);
    static BOOLEAN curr_tuner = FALSE;
    static E_TTYPE curr_type = E_TERR_TYPE_UNKNOWN;
    if (curr_tuner == FALSE)
    {
        curr_type = signal_type;
    }else
    {
        if (curr_type != signal_type)
        {
            ALOGD("start:%s   type different curr_type:%d signal_type:%d ", __FUNCTION__, curr_type, signal_type);
            Am_tuner_cancelScanning(gTunerClient);
            Am_tuner_closeFrontend(gTunerClient);
        }
    }
    int ClientId = Am_tuner_getTunerClientIdByType(TUNER_TYPE_DEFAULT);
    ALOGD("start:%s  ClientId:%d  gTunerClient:%d", __FUNCTION__, ClientId, gTunerClient);
    //update tuner map
    TUNER_MAP::iterator it = tuner_map.find( path );
    if (it != tuner_map.end())
    {
        if (tuner_lnb != NULL)
       {
           Am_lnb_close(tuner_lnb);
           ALOGD("start:%s   tuner_lnb is close", __FUNCTION__);
       }
        tuner_map.erase(it);
        MAP_INSERT_ITEM( tuner_map, path, ClientId );
        ALOGI("%s Already Find ClientId: %d  update from map.", __FUNCTION__, ClientId);

        tuner_lnb = Am_tuner_openLnb(ClientId, (long)LnbCallback);
        if (tuner_lnb == NULL)
        {
            ALOGD("start:%s tuner_lnb is Null", __FUNCTION__);
        }
    }
    else
    {
        MAP_INSERT_ITEM( tuner_map, path, ClientId );
        ALOGI("%s instert new ClientId : %d.", __FUNCTION__, ClientId);
    }
    gTunerClient = ClientId;

    ALOGD("gTunerClient: %d ", gTunerClient);
    bool attached = false;
    JNIEnv *env = Am_tuner_getJNIEnv(&attached);
    if (NULL == env)
    {
        ALOGD("%s : test fail, env is null", __FUNCTION__);
        return;
    }
    long callbackContext = (long)scanCallback;
    Am_tuner_setOnTuneEventListener(gTunerClient, callbackContext);
    Tmode = tmode;
    TBWidth = tbwidth;
    tuner_srate = srate;
    cable_mode = cmode;
    ALOGD("%s : signal_type:%d", __FUNCTION__,signal_type);
    if (signal_type == E_TERR_TYPE_DVBT)
    {
        Dvbt_Frontend_Settings dvbtFrontendSettings;
        memset(&dvbtFrontendSettings, 0, sizeof(Dvbt_Frontend_Settings));
        dvbtFrontendSettings.frequency = freq;
        dvbtFrontendSettings.transmissionMode = Wrapper_TuneGetTransmissionMode(tmode);
        dvbtFrontendSettings.bandwidth = Wrapper_TuneGetTerrBwidth(tbwidth);
        if (sys_type == WRAPPER_TUNE_SYSTEM_TYPE_DVBT)
        {
            dvbtFrontendSettings.standard = DVBT_STANDARD_T;
        }
        else if (sys_type == WRAPPER_TUNE_SYSTEM_TYPE_DVBT2)
        {
            dvbtFrontendSettings.standard = DVBT_STANDARD_T2;
        }
        //dvbtFrontendSettings.standard = E_TERR_TYPE_DVBC;
        jobject dvbtSettingObject = dvb_utils_getDvbtFrontendSettingsObject(env, dvbtFrontendSettings);
        if (NULL == dvbtSettingObject)
        {
            ALOGD("%s : test fail, dvbt frontend setting not create", __FUNCTION__);
            testFailLeave(attached);
            return;
        }
        Am_tuner_tune(gTunerClient, dvbtSettingObject);
        curr_starttune = TRUE;
    }else if (signal_type == E_TERR_TYPE_DVBC)
    {
        //1.Cancel dvbt tuning
        //Am_tuner_closeFrontend(gTunerClient);
        //2.prepare dvbc frontendSettings
        Dvbc_Frontend_Settings dvbcFrontendSettings;
        memset(&dvbcFrontendSettings, 0, sizeof(Dvbc_Frontend_Settings));
        dvbcFrontendSettings.frequency = freq;
        dvbcFrontendSettings.modulation = DVBC_MODULATION_AUTO;
        dvbcFrontendSettings.symbolRate = srate;

        jobject dvbcSettingObject = dvb_utils_getDvbcFrontendSettingsObject(env, dvbcFrontendSettings);
        if (NULL == dvbcSettingObject) {
            ALOGD("%s : test fail, dvbt frontend setting not create", __FUNCTION__);
            testFailLeave(attached);
            return;
        }
        Am_tuner_tune(gTunerClient, dvbcSettingObject);
        curr_starttune = TRUE;
    }else if(signal_type == E_TERR_TYPE_DVBS)
    {
        Dvbs_Frontend_Settings dvbsFrontendSettings;
        memset(&dvbsFrontendSettings, 0, sizeof(Dvbs_Frontend_Settings));
        dvbsFrontendSettings.frequency = freq*1000;
        dvbsFrontendSettings.symbol_rate = srate;
        dvbsFrontendSettings.modulation = DVBS_MODULATION_AUTO;
        dvbsFrontendSettings.scan_type= DVBS_SCAN_TYPE_DISEQC;
        dvbsFrontendSettings.pilot = DVBS_PILOT_AUTO;
        dvbsFrontendSettings.code_rate.fec = DVBS_FEC_AUTO;
        dvbsFrontendSettings.code_rate.isLinear = FALSE;
        dvbsFrontendSettings.code_rate.isShortFrames = true;
        dvbsFrontendSettings.code_rate.bitsPer1000Symbol = 0;
        ALOGD("%s :frequency=%d ", __FUNCTION__, dvbsFrontendSettings.frequency);
        jobject dvbsSettingObject = dvb_utils_getDvbsFrontendSettingsObject(env, dvbsFrontendSettings);
        if (NULL == dvbsSettingObject) {
            ALOGD("%s : test fail, dvbs frontend setting not create", __FUNCTION__);
            testFailLeave(attached);
            return;
        }
        Am_tuner_tune(gTunerClient, dvbsSettingObject);
        curr_starttune = TRUE;
    }
    curr_tuner = TRUE;
    tuning_params_changed = FALSE;
    //tuner_getFrontendIds(path);
    ReleaseEnv(attached);
    ALOGD("end:%s ", __FUNCTION__);

}
void Wrapper_TuneStopTuner(U8BIT path)
{
    ALOGD("start:%s", __FUNCTION__);
    int client_id = INVALID_TUNER_ID;
    TUNER_MAP::iterator it = tuner_map.find( path );
    if (it != tuner_map.end())
    {
        client_id = it->second;
        tuner_map.erase(it);
    }else
    {
        ALOGD("client_id is invalid: %d", client_id);
        return;
    }

    jint result;
    if (blindscan_mode)
    {
        result = Am_tuner_cancelScanning(client_id);
        blindscan_mode = FALSE;
    }
    else
    {
        result = Am_tuner_cancelTuning(client_id);
        Am_tuner_clearOnTuneEventListener(client_id);
    }

    if (result)
    {
        ALOGD("%s : StopTuner %d fail", __FUNCTION__, client_id);
    }else
    {
        ALOGD("%s : StopTuner %d success", __FUNCTION__, client_id);
    }

    curr_starttune = FALSE;
    gTunerClient = INVALID_TUNER_ID;

    ALOGD("end:%s", __FUNCTION__);
}
U8BIT Wrapper_TuneGetSignalStrength(U8BIT path)
{
    ALOGD("start:%s", __FUNCTION__);
    U8BIT s_strength = 0;
    int tuner_client = INVALID_TUNER_ID;
    TUNER_MAP::iterator it = tuner_map.find( path );
    if (it != tuner_map.end()&& curr_starttune == TRUE)
    {
        tuner_client = it->second;
    }else
    {
        ALOGD("tuner_client is invalid: %d", tuner_client);
        return 0;
    }
    ALOGD("tuner_client : %d", tuner_client);
    if (WRAPPER_TUNER_STATE_LOCKED == Wrapper_TuneGetLockStatus(path))
    {
        Frontend_Status stfrontendStatus = tuner_getFrontendStatus(tuner_client, FRONTEND_STATUS_TYPE_SIGNAL_STRENGTH);
        s_strength = stfrontendStatus.signal_strength;
    }else
    {
        ALOGD("end:%s  unlock", __FUNCTION__);
    }


    ALOGD("end:%s  s_strength:%d", __FUNCTION__,s_strength);
    return s_strength;
}

U32BIT Wrapper_TuneGetDataIntegrity(U8BIT path)
{
    ALOGD("start:%s", __FUNCTION__);
    U8BIT ber = 0;
    int tuner_client = INVALID_TUNER_ID;
    TUNER_MAP::iterator it = tuner_map.find( path );
    if (it != tuner_map.end()&& curr_starttune == TRUE)
    {
        tuner_client = it->second;
    }else
    {
        ALOGD("tuner_client is invalid: %d", tuner_client);
        return 0;
    }
    if (WRAPPER_TUNER_STATE_LOCKED == Wrapper_TuneGetLockStatus(path))
    {
        Frontend_Status stfrontendStatus = tuner_getFrontendStatus(tuner_client, FRONTEND_STATUS_TYPE_BER);
        ber = stfrontendStatus.ber;
    }else
    {
        ALOGD("end:%s  unlock", __FUNCTION__);
    }

    ALOGD("end:%s", __FUNCTION__);
    return ber;
}

U8BIT Wrapper_TuneGetSignalQuality(U8BIT path)
{
    ALOGD("start:%s", __FUNCTION__);
    U8BIT s_quality = 0;
    int tuner_client = INVALID_TUNER_ID;
    TUNER_MAP::iterator it = tuner_map.find( path );
    if (it != tuner_map.end()&& curr_starttune == TRUE)
    {
        tuner_client = it->second;
    }else
    {
        ALOGD("tuner_client is invalid: %d", tuner_client);
        return 0;
    }
    if (WRAPPER_TUNER_STATE_LOCKED == Wrapper_TuneGetLockStatus(path))
    {
        Frontend_Status stfrontendStatus = tuner_getFrontendStatus(tuner_client, FRONTEND_STATUS_TYPE_SIGNAL_QUALITY);

        s_quality = stfrontendStatus.signal_quality;
    }else
    {
        ALOGD("end:%s  unlock", __FUNCTION__);
    }

    ALOGD("end:%s s_quality:%d", __FUNCTION__,s_quality);
    return s_quality;
}

U32BIT Wrapper_TuneGetActualTerrFrequency(U8BIT path)
{
    ALOGD("start:%s", __FUNCTION__);
    U32BIT terr_fre = 0;
    int tuner_client = INVALID_TUNER_ID;
    TUNER_MAP::iterator it = tuner_map.find( path );
    if (it != tuner_map.end()&& curr_starttune == TRUE)
    {
        tuner_client = it->second;
    }else
    {
        ALOGD("tuner_client is invalid: %d", tuner_client);
        return 0;
    }
    if (WRAPPER_TUNER_STATE_LOCKED == Wrapper_TuneGetLockStatus(path))
    {
        Frontend_Status stfrontendStatus = tuner_getFrontendStatus(tuner_client, FRONTEND_STATUS_TYPE_FEC);

        terr_fre = stfrontendStatus.inner_fec;
    }else
    {
        ALOGD("end:%s  unlock", __FUNCTION__);
    }


    ALOGD("end:%s", __FUNCTION__);
    return terr_fre;
}
S8BIT Wrapper_TuneGetActualTerrFreqOffset(U8BIT path)
{
    ALOGD("start:%s", __FUNCTION__);
    S8BIT fre_offset = 0;
    int tuner_client = INVALID_TUNER_ID;
    TUNER_MAP::iterator it = tuner_map.find( path );
    if (it != tuner_map.end()&& curr_starttune == TRUE)
    {
        tuner_client = it->second;
    }else
    {
        ALOGD("tuner_client is invalid: %d", tuner_client);
        return 0;
    }
    if (WRAPPER_TUNER_STATE_LOCKED == Wrapper_TuneGetLockStatus(path))
    {
        Frontend_Status stfrontendStatus = tuner_getFrontendStatus(tuner_client, FRONTEND_STATUS_TYPE_FREQ_OFFSET);

        fre_offset = stfrontendStatus.freq_offset;
    }else
    {
        ALOGD("end:%s  unlock", __FUNCTION__);
    }

    ALOGD("end:%s", __FUNCTION__);
    return fre_offset;
}
EW_TUNER_EVENT Wrapper_TuneGetLockStatus(U8BIT path)
{
    ALOGD("start:%s", __FUNCTION__);
    EW_TUNER_EVENT lock_st = WRAPPER_TUNER_STATE_UNKNOWN;
    bool event = false;
    int tuner_client = INVALID_TUNER_ID;
    TUNER_MAP::iterator it = tuner_map.find( path );
    if (it != tuner_map.end() && curr_starttune == TRUE)
    {
        tuner_client = it->second;
    }else
    {
        ALOGD("tuner_client is invalid: %d", tuner_client);
        return lock_st;
    }
    Frontend_Status stfrontendStatus = tuner_getFrontendStatus(tuner_client, FRONTEND_STATUS_TYPE_DEMOD_LOCK);

    event = stfrontendStatus.is_demod_locked;
    if (true == event)
    {
        lock_st = WRAPPER_TUNER_STATE_LOCKED;
    }
    else
    {
        lock_st = WRAPPER_TUNER_STATE_TIMEOUT;
    }

    ALOGD("end:%s event:%d", __FUNCTION__, event);
    return lock_st;
}

BOOLEAN Wrapper_TuneOpen(U8BIT path)
{
    ALOGD("start:%s path:%d", __FUNCTION__,path);
    if (INVALID_TUNER_ID == gTunerClient)
    {
        gTunerClient = Am_tuner_getTunerClientIdByType(TUNER_TYPE_DEFAULT);
    }
    ALOGD("gTunerClient:%d", gTunerClient);
    if (INVALID_TUNER_ID == gTunerClient)
    {
        ALOGD("%s : gTunerClient is invalid", __FUNCTION__);
        return FALSE;
    }
    curr_path = path;
    MAP_INSERT_ITEM( tuner_map, path, gTunerClient );
    BOOLEAN ret =  tuner_getFrontendIds(path);
    ALOGD("%s :tuner_getFrontendIds is %d", __FUNCTION__, ret);
    ALOGD("end:%s curr_path:%d", __FUNCTION__, curr_path);
    return ret;
}
BOOLEAN Wrapper_TuneIsOpened(U8BIT path)
{
    BOOLEAN ret = FALSE;

    ALOGD("start:%s path:%d", __FUNCTION__,path);
    int tuner_client = INVALID_TUNER_ID;
    TUNER_MAP::iterator it = tuner_map.find( path );
    if (it != tuner_map.end())
    {
        tuner_client = it->second;
        ret = TRUE;
    }else
    {
        ALOGD("tuner_client is invalid: %d", tuner_client);
        ret = FALSE;
    }
    ALOGD("end:%s path:%d result:%d", __FUNCTION__,path, ret);
    return ret;
}

int Wrapper_GetTunerId(int path)
{
    int tuner_client = INVALID_TUNER_ID;
    TUNER_MAP::iterator it = tuner_map.find( path );
    if (it != tuner_map.end())
    {
        tuner_client = it->second;
    }else
    {
        ALOGD("tuner_client is invalid: %d", tuner_client);
        return INVALID_TUNER_ID;
    }
    return tuner_client;
}
jclass getValueClass(JNIEnv *env, jobject valueObject, const char *name)
{
   if ((NULL == env) || (NULL == valueObject)) {
        ALOGE("%s:input parameter error", __FUNCTION__);
        return NULL;
    }

    if (env->IsSameObject(valueObject, nullptr)) {
        ALOGE("%s:value Object is nullptr", __FUNCTION__);
        return NULL;
    }
    jclass valueClazz = env->FindClass(name);
    if (JNI_TRUE != env->IsInstanceOf(valueObject, valueClazz)) {
        ALOGD("value object not check value class");
        return NULL;
    }
    return valueClazz;
}

//bin.luo  PR2
U16BIT Wrapper_TuneGetSignalType(U8BIT path)
{
    ALOGD("start:%s path:%d", __FUNCTION__,path);
    U16BIT s_type = WRAPPER_TUNE_SIGNAL_NONE;
    switch (signal_type)
    {
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
        default:
            {
                s_type = WRAPPER_TUNE_SIGNAL_NONE;
                break;
            }
    }
    ALOGD("end:%s path:%d", __FUNCTION__,path);
    return s_type;
}
U16BIT Wrapper_TuneGetActualSignalType(U8BIT path)
{
    ALOGD("start:%s path:%d", __FUNCTION__,path);

    int tuner_client = INVALID_TUNER_ID;
    TUNER_MAP::iterator it = tuner_map.find( path );
    if (it != tuner_map.end())
    {
        tuner_client = it->second;
    }else
    {
        ALOGD("tuner_client is invalid: %d", tuner_client);
        return 0;
    }

    jobject frontendInfo = Am_tuner_getFrontendInfo(tuner_client);
    if (NULL == frontendInfo) {
        ALOGD("%s : test fail, FrontendInfo is null", __FUNCTION__);
        return 0;
    }

    bool attached = false;
    JNIEnv *env = Am_tuner_getJNIEnv(&attached);
    if (NULL == env) {
        ALOGD("%s : test fail, env is null", __FUNCTION__);
        return 0;
    }
    //1.Test jobject class
    jclass clazz = env->FindClass("android/media/tv/tuner/frontend/FrontendInfo");
    if (JNI_TRUE != env->IsInstanceOf(frontendInfo, clazz)) {
        ALOGD("%s : test fail, not FrontendInfo object", __FUNCTION__);
        testFailLeave(attached);
        return 0;
    }
    //2.show Frontend info
    jfieldID fId = env->GetFieldID(clazz, "mId", "I"); //
    jfieldID fType = env->GetFieldID(clazz, "mType", "I");

    int id = env->GetIntField(frontendInfo, fId);
    int type = env->GetIntField(frontendInfo, fType);

    ALOGD("%s : frontend id : %d, type : %d", __FUNCTION__, id, type);

    if (attached) {
        Am_tuner_detachJNIEnv();
    }
    env->DeleteWeakGlobalRef(frontendInfo);
    ALOGD("end:%s path:%d", __FUNCTION__,path);  //JNI signal_type=TYPE_UNDEFINED, TYPE_ANALOG, TYPE_ATSC, TYPE_ATSC3, TYPE_DVBC, TYPE_DVBS,TYPE_DVBT, TYPE_ISDBS, TYPE_ISDBS3, TYPE_ISDBT, TYPE_DTMB
    return type;
}
void Wrapper_TuneSetSignalType(U8BIT path, EW_STB_TUNE_SIGNAL_TYPE type)   //E_STB_TUNE_SIGNAL_TYPEÓëE_TTYPEÊýÖµ²»·û£¬»¹Ðè¸ãÇå³þdvbtFrontendSettings.standardÊýÖµ·¶Î§
{
    ALOGD("start:%s path:%d type:%d", __FUNCTION__,path,type);
    if (WRAPPER_TUNE_SIGNAL_QPSK == type)
    {
        signal_type = E_TERR_TYPE_DVBS;
    }else if (WRAPPER_TUNE_SIGNAL_COFDM == type)
    {
        signal_type = E_TERR_TYPE_DVBT;
    }else if (WRAPPER_TUNE_SIGNAL_QAM == type)
    {
        signal_type = E_TERR_TYPE_DVBC;
    }
    ALOGD("end:%s path:%d", __FUNCTION__,path);
    return;
}
EW_STB_TUNE_TMODE Wrapper_TuneGetActualTerrMode(U8BIT path)
{
    ALOGD("start:%s path:%d", __FUNCTION__,path);

    ALOGD("end:%s path:%d", __FUNCTION__,path);
    return Tmode;
}
EW_STB_TUNE_TBWIDTH Wrapper_TuneGetActualTerrBwidth(U8BIT path)
{
    ALOGD("start:%s path:%d", __FUNCTION__,path);

    ALOGD("end:%s path:%d", __FUNCTION__,path);
    return TBWidth;
}

U32BIT Wrapper_TuneGetMinTunerFreqKHz(U8BIT path)
{
    ALOGD("start:%s  path:%d", __FUNCTION__, path);
    U32BIT fre_min = 0;
    int tuner_client = INVALID_TUNER_ID;
    TUNER_MAP::iterator it = tuner_map.find( path );
    if (it != tuner_map.end())
    {
        tuner_client = it->second;
    }else
    {
        ALOGD("tuner_client is invalid: %d", tuner_client);
        return fre_min;
    }

    bool attached = false;
    JNIEnv *env = Am_tuner_getJNIEnv(&attached);
    if (NULL == env) {
        ALOGD("%s : test fail, env is null", __FUNCTION__);
        return fre_min;
    }
    int frontendSize = gFrontendList.size();
    ALOGD("%s : frontendSize : %d", __FUNCTION__, frontendSize);
    for (int frontendId : gFrontendList)
    {
        ALOGD("%s : frontendId : %d", __FUNCTION__, frontendId);
        jobject frontendInfo = Am_tuner_getFrontendInfoById(tuner_client, frontendId);
        if (NULL == frontendInfo) {
            ALOGD("%s : test fail, FrontendInfo is null", __FUNCTION__);
            testFailLeave(attached);
            return fre_min;
        }
        //1.Test jobject class
        jclass clazz = env->FindClass("android/media/tv/tuner/frontend/FrontendInfo");
        if (JNI_TRUE != env->IsInstanceOf(frontendInfo, clazz)) {
            ALOGD("%s : test fail, not FrontendInfo object", __FUNCTION__);
            testFailLeave(attached);
            return fre_min;
        }

        //2.show Frontend info
        jfieldID fType = env->GetFieldID(clazz, "mType", "I");
        int type = env->GetIntField(frontendInfo, fType);
        if (E_TERR_TYPE_DVBT == type)
        {
            jmethodID  fFrequencyRange = env->GetMethodID(clazz, "getFrequencyRange", "()Landroid/util/Range;");
            jobject FrequencyRange = env->CallObjectMethod(frontendInfo, fFrequencyRange);

            jclass range_clazz = env->FindClass("android/util/Range");
            if (JNI_TRUE != env->IsInstanceOf(FrequencyRange, range_clazz))
            {
                ALOGD("%s : test fail, not FrequencyRange object", __FUNCTION__);
                testFailLeave(attached);
                return fre_min;
            }

            jmethodID getlower = env->GetMethodID(range_clazz,"getLower","()Ljava/lang/Comparable;");
            jobject get_min = env->CallObjectMethod(FrequencyRange,getlower);

            jclass minClazz = getValueClass(env, get_min, INTEGER_CLASS);
            if (NULL == minClazz)
            {
                   // return false;
            }
            jmethodID intminId = env->GetMethodID(minClazz, "intValue", "()I");
            fre_min = env->CallIntMethod(get_min, intminId);
            ALOGD("%s : type : %d min:%d", __FUNCTION__, type, fre_min);
            env->DeleteLocalRef(FrequencyRange);
        }

        env->DeleteWeakGlobalRef(frontendInfo);
    }

    if (attached) {
        Am_tuner_detachJNIEnv();
    }

    ALOGD("end:%s ", __FUNCTION__);
    return fre_min;
}

U32BIT Wrapper_TuneGetMaxTunerFreqKHz(U8BIT path)
{
    ALOGD("start:%s  path:%d", __FUNCTION__, path);
    U32BIT fre_max = 0;
    int tuner_client = INVALID_TUNER_ID;
    TUNER_MAP::iterator it = tuner_map.find( path );
    if (it != tuner_map.end())
    {
        tuner_client = it->second;
    }else
    {
        ALOGD("tuner_client is invalid: %d", tuner_client);
        return fre_max;
    }

    bool attached = false;
    JNIEnv *env = Am_tuner_getJNIEnv(&attached);
    if (NULL == env) {
        ALOGD("%s : test fail, env is null", __FUNCTION__);
        return fre_max;
    }
    int frontendSize = gFrontendList.size();
    ALOGD("%s : frontendSize : %d", __FUNCTION__, frontendSize);
    for (int frontendId : gFrontendList)
    {
        ALOGD("%s : frontendId : %d", __FUNCTION__, frontendId);
        jobject frontendInfo = Am_tuner_getFrontendInfoById(tuner_client, frontendId);
        if (NULL == frontendInfo) {
            ALOGD("%s : test fail, FrontendInfo is null", __FUNCTION__);
            testFailLeave(attached);
            return fre_max;
        }
        //1.Test jobject class
        jclass clazz = env->FindClass("android/media/tv/tuner/frontend/FrontendInfo");
        if (JNI_TRUE != env->IsInstanceOf(frontendInfo, clazz)) {
            ALOGD("%s : test fail, not FrontendInfo object", __FUNCTION__);
            testFailLeave(attached);
            return fre_max;
        }

        //2.show Frontend info
        jfieldID fType = env->GetFieldID(clazz, "mType", "I");
        int type = env->GetIntField(frontendInfo, fType);
        if (E_TERR_TYPE_DVBT == type)
        {
            jmethodID  fFrequencyRange = env->GetMethodID(clazz, "getFrequencyRange", "()Landroid/util/Range;");
            jobject FrequencyRange = env->CallObjectMethod(frontendInfo, fFrequencyRange);

            jclass range_clazz = env->FindClass("android/util/Range");
            if (JNI_TRUE != env->IsInstanceOf(FrequencyRange, range_clazz))
            {
                ALOGD("%s : test fail, not FrequencyRange object", __FUNCTION__);
                testFailLeave(attached);
                return fre_max;
            }

            jmethodID getupper = env->GetMethodID(range_clazz,"getUpper","()Ljava/lang/Comparable;");
            jobject get_max = env->CallObjectMethod(FrequencyRange,getupper);

            jclass maxClazz = getValueClass(env, get_max, INTEGER_CLASS);
            if (NULL == maxClazz)
            {
                return fre_max;
            }
            jmethodID intmaxId = env->GetMethodID(maxClazz, "intValue", "()I");
            fre_max = env->CallIntMethod(get_max, intmaxId);
            ALOGD("%s : type : %d min:%d", __FUNCTION__, type, fre_max);
            env->DeleteLocalRef(FrequencyRange);
        }

        env->DeleteWeakGlobalRef(frontendInfo);
    }

    if (attached) {
        Am_tuner_detachJNIEnv();
    }
    ALOGD("end:%s ", __FUNCTION__);
    return fre_max;
}


EW_STB_TUNE_TCONST Wrapper_TuneGetActualTerrConstellation(U8BIT path)
{
    ALOGD("start:%s path:%d", __FUNCTION__,path);
    EW_STB_TUNE_TCONST terr_const = WRAPPER_TUNE_TCONST_UNDEFINED;
    int tuner_client = INVALID_TUNER_ID;
    TUNER_MAP::iterator it = tuner_map.find( path );
    if (it != tuner_map.end())
    {
        tuner_client = it->second;
    }else
    {
        ALOGD("tuner_client is invalid: %d", tuner_client);
        return terr_const;
    }
    if (WRAPPER_TUNER_STATE_LOCKED == Wrapper_TuneGetLockStatus(path))
    {
        Frontend_Status stfrontendStatus = tuner_getFrontendStatus(tuner_client, FRONTEND_STATUS_TYPE_MODULATION);
        switch (stfrontendStatus.modulation)
        {
            case 0:
               terr_const = WRAPPER_TUNE_TCONST_QPSK;
               break;
            case 1:
                terr_const = WRAPPER_TUNE_TCONST_QAM16;
                break;
            case 3:
                terr_const = WRAPPER_TUNE_TCONST_QAM64;
                break;
            case 4:
                terr_const = WRAPPER_TUNE_TCONST_QAM128;
                break;
            case 5:
                terr_const = WRAPPER_TUNE_TCONST_QAM256;
                break;
            default:
                terr_const = WRAPPER_TUNE_TCONST_UNDEFINED;
                break;
        }
    }
    else
    {
        ALOGD("end:%s  unlock", __FUNCTION__);
    }
    ALOGD("end:%s path:%d terr_const:%d", __FUNCTION__,path, terr_const);
    return terr_const;
}
EW_STB_TUNE_HIERARCHY Wrapper_TuneGetActualTerrHierarchy(U8BIT path)
{
    ALOGD("start:%s path:%d", __FUNCTION__,path);
    int tuner_client = INVALID_TUNER_ID;
    EW_STB_TUNE_HIERARCHY hierarchy = WRAPPER_TUNE_HIERARCHY_UNDEFINED;
    TUNER_MAP::iterator it = tuner_map.find( path );
    if (it != tuner_map.end())
    {
        tuner_client = it->second;
    }else
    {
        ALOGD("tuner_client is invalid: %d", tuner_client);
        return hierarchy;
    }
    Frontend_Status stfrontendStatus = tuner_getFrontendStatus(tuner_client, FRONTEND_STATUS_TYPE_HIERARCHY);
    int ret = stfrontendStatus.hierarchy;
    switch (ret)
    {
        case 0:
           hierarchy = WRAPPER_TUNE_HIERARCHY_NONE;
           break;
        case 1:
           hierarchy = WRAPPER_TUNE_HIERARCHY_1;
           break;
        case 2:
           hierarchy = WRAPPER_TUNE_HIERARCHY_2;
           break;
        case 4:
           hierarchy = WRAPPER_TUNE_HIERARCHY_4;
           break;
        case 8:
           hierarchy = WRAPPER_TUNE_HIERARCHY_8;
           break;
        case 16:
           hierarchy = WRAPPER_TUNE_HIERARCHY_16;
           break;
        case 32:
           hierarchy = WRAPPER_TUNE_HIERARCHY_32;
           break;
        case 64:
           hierarchy = WRAPPER_TUNE_HIERARCHY_64;
           break;
        case 128:
           hierarchy = WRAPPER_TUNE_HIERARCHY_128;
           break;
        default:
           hierarchy = WRAPPER_TUNE_HIERARCHY_UNDEFINED;
           break;
    }
    ALOGD("end:%s path:%d ret:%d", __FUNCTION__, path, ret);
    return hierarchy;
}

/*
S32BIT STB_TuneGetMPLPIDList(U8BIT path, U8BIT *plp_list, U16BIT listlen)   // Tuner Framework Ã»ÓÐÌá¹©¸ÃÖµ
{
    ALOGD("start:%s path:%d", __FUNCTION__,path);
    S32BIT ret_val = 0;
    int tuner_client = INVALID_TUNER_ID;
    TUNER_MAP::iterator it = tuner_map.find( path );
    if (it != tuner_map.end())
    {
        tuner_client = it->second;
    }else
    {
        ALOGD("tuner_client is invalid: %d", tuner_client);
        return terr_const;
    }
    if (TUNER_STATE_LOCKED == STB_TuneGetLockStatus(path))
    {
        Frontend_Status stfrontendStatus = tuner_getFrontendStatus(tuner_client, FRONTEND_STATUS_TYPE_BER);
        memcpy(plp_list,&stfrontendStatus.plp_id, sizeof(int));
    }else
    {
        ALOGD("end:%s  unlock", __FUNCTION__);
    }
    ALOGD("end:%s path:%d terr_const:%d", __FUNCTION__,path, terr_const);
    return terr_const;
}
*/
void Wrapper_TuneSetPLP(U8BIT path, U8BIT plp)
{
    ALOGD("start:%s path:%d", __FUNCTION__,path);
    tuner_plp = plp;
    ALOGD("end:%s path:%d plp_id=%d", __FUNCTION__, path, plp);
}
U8BIT Wrapper_TuneGetPLP(U8BIT path)
{
    ALOGD("start:%s path:%d", __FUNCTION__,path);
    U8BIT plp_id = 0;
    int tuner_client = INVALID_TUNER_ID;
    TUNER_MAP::iterator it = tuner_map.find( path );
    if (it != tuner_map.end())
    {
        tuner_client = it->second;
    }else
    {
        ALOGD("tuner_client is invalid: %d", tuner_client);
        return plp_id;
    }
    Frontend_Status stfrontendStatus = tuner_getFrontendStatus(tuner_client, FRONTEND_STATUS_TYPE_BER);
    plp_id = stfrontendStatus.plp_id;

    ALOGD("end:%s path:%d plp_id=%d", __FUNCTION__, path, plp_id);
    return plp_id;
}

U32BIT Wrapper_TuneGetActualSymbolRate(U8BIT path)
{
    ALOGD("start:%s path:%d", __FUNCTION__,path);

    ALOGD("end:%s path:%d tuner_srate: %d", __FUNCTION__, path, tuner_srate);
    return tuner_srate;
}
void Wrapper_TuneGetSupportedSystemType(U8BIT path, U8BIT *support_sys) //old API Ö»ÊµÏÖÁË¸³Öµ²Ù×÷
{
    ALOGD("start:%s path:%d", __FUNCTION__,path);
    support_sys[WRAPPER_TUNE_SYSTEM_TYPE_DVBT] = TRUE;
    ALOGD("end:%s path:%d tuner_srate: %d", __FUNCTION__, path, tuner_srate);
    return;
}
U32BIT Wrapper_TuneGetMaxTunerSymbolRate(U8BIT path)
{
    ALOGD("start:%s", __FUNCTION__);
    U32BIT rate_max = 0;
    int tuner_client = INVALID_TUNER_ID;
    TUNER_MAP::iterator it = tuner_map.find( path );
    if (it != tuner_map.end())
    {
        tuner_client = it->second;
    }else
    {
        ALOGD("tuner_client is invalid: %d", tuner_client);
        return rate_max;
    }

    bool attached = false;
    JNIEnv *env = Am_tuner_getJNIEnv(&attached);
    if (NULL == env) {
        ALOGD("%s : test fail, env is null", __FUNCTION__);
        return rate_max;
    }
    int frontendSize = gFrontendList.size();
    ALOGD("%s : frontendSize : %d", __FUNCTION__, frontendSize);
    for (int frontendId : gFrontendList)
    {
        ALOGD("%s : frontendId : %d", __FUNCTION__, frontendId);
        jobject frontendInfo = Am_tuner_getFrontendInfoById(tuner_client, frontendId);
        if (NULL == frontendInfo) {
            ALOGD("%s : test fail, FrontendInfo is null", __FUNCTION__);
            testFailLeave(attached);
            return rate_max;
        }
        //1.Test jobject class
        jclass clazz = env->FindClass("android/media/tv/tuner/frontend/FrontendInfo");
        if (JNI_TRUE != env->IsInstanceOf(frontendInfo, clazz)) {
            ALOGD("%s : test fail, not FrontendInfo object", __FUNCTION__);
            testFailLeave(attached);
            return rate_max;
        }

        //2.show Frontend info
        jfieldID fType = env->GetFieldID(clazz, "mType", "I");
        int type = env->GetIntField(frontendInfo, fType);
        if (E_TERR_TYPE_DVBT == type)
        {
            jmethodID  fSymbolRateRange = env->GetMethodID(clazz, "getSymbolRateRange", "()Landroid/util/Range;");
            jobject SymbolRateRange = env->CallObjectMethod(frontendInfo, fSymbolRateRange);

            jclass rate_clazz = env->FindClass("android/util/Range");
            if (JNI_TRUE != env->IsInstanceOf(SymbolRateRange, rate_clazz))
            {
                ALOGD("%s : test fail, not SymbolRateRange object", __FUNCTION__);
                testFailLeave(attached);
                return rate_max;
            }

            jmethodID getupper = env->GetMethodID(rate_clazz,"getUpper","()Ljava/lang/Comparable;");
            jobject get_max = env->CallObjectMethod(SymbolRateRange,getupper);

            jclass maxClazz = getValueClass(env, get_max, INTEGER_CLASS);
            if (NULL == maxClazz)
            {
                ALOGD("%s : test fail, maxClazz class is NULL", __FUNCTION__);
                return rate_max;
            }
            jmethodID intminId = env->GetMethodID(maxClazz, "intValue", "()I");
            rate_max = env->CallIntMethod(get_max, intminId);
            ALOGD("%s : type : %d max:%d", __FUNCTION__, type, rate_max);
            env->DeleteLocalRef(SymbolRateRange);
        }

        env->DeleteWeakGlobalRef(frontendInfo);
    }

    if (attached) {
        Am_tuner_detachJNIEnv();
    }

    ALOGD("end:%s ", __FUNCTION__);
    return rate_max;
}
DVBT_BANDWIDTH Wrapper_TuneGetTerrBwidth(EW_STB_TUNE_TBWIDTH tbwidth)
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

DVBT_TRANSMISSION_MODE Wrapper_TuneGetTransmissionMode(EW_STB_TUNE_TMODE tmode)
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

U32BIT Wrapper_TuneGetMinTunerSymbolRate(U8BIT path)
{
    ALOGD("start:%s", __FUNCTION__);
    U32BIT rate_min = 0;
    int tuner_client = INVALID_TUNER_ID;
    TUNER_MAP::iterator it = tuner_map.find( path );
    if (it != tuner_map.end())
    {
        tuner_client = it->second;
    }else
    {
        ALOGD("tuner_client is invalid: %d", tuner_client);
        return rate_min;
    }

    bool attached = false;
    JNIEnv *env = Am_tuner_getJNIEnv(&attached);
    if (NULL == env) {
        ALOGD("%s : test fail, env is null", __FUNCTION__);
        return rate_min;
    }
    int frontendSize = gFrontendList.size();
    ALOGD("%s : frontendSize : %d", __FUNCTION__, frontendSize);
    for (int frontendId : gFrontendList)
    {
        ALOGD("%s : frontendId : %d", __FUNCTION__, frontendId);
        jobject frontendInfo = Am_tuner_getFrontendInfoById(tuner_client, frontendId);
        if (NULL == frontendInfo) {
            ALOGD("%s : test fail, FrontendInfo is null", __FUNCTION__);
            testFailLeave(attached);
            return rate_min;
        }
        //1.Test jobject class
        jclass clazz = env->FindClass("android/media/tv/tuner/frontend/FrontendInfo");
        if (JNI_TRUE != env->IsInstanceOf(frontendInfo, clazz)) {
            ALOGD("%s : test fail, not FrontendInfo object", __FUNCTION__);
            testFailLeave(attached);
            return rate_min;
        }

        //2.show Frontend info
        jfieldID fType = env->GetFieldID(clazz, "mType", "I");
        int type = env->GetIntField(frontendInfo, fType);
        if (E_TERR_TYPE_DVBT == type)
        {
            jmethodID  fSymbolRateRange = env->GetMethodID(clazz, "getSymbolRateRange", "()Landroid/util/Range;");
            ALOGD("%s : SymbolRateRange CallObjectMethod", __FUNCTION__);
            jobject SymbolRateRange = env->CallObjectMethod(frontendInfo, fSymbolRateRange);
            jclass rate_clazz = env->FindClass("android/util/Range");
            if (JNI_TRUE != env->IsInstanceOf(SymbolRateRange, rate_clazz))
            {
                ALOGD("%s : test fail, not FrequencyRange object", __FUNCTION__);
                testFailLeave(attached);
                return rate_min;
            }

            jmethodID getlower = env->GetMethodID(rate_clazz,"getLower","()Ljava/lang/Comparable;");
            ALOGD("%s : get_min CallObjectMethod", __FUNCTION__);
            jobject get_min = env->CallObjectMethod(SymbolRateRange,getlower);

            jclass minClazz = getValueClass(env, get_min, INTEGER_CLASS);
            if (NULL == minClazz)
            {
                ALOGD("%s : test fail, minClazz class is NULL", __FUNCTION__);
                return rate_min;
            }
            jmethodID intminId = env->GetMethodID(minClazz, "intValue", "()I");
            rate_min = env->CallIntMethod(get_min, intminId);
            ALOGD("%s : type : %d min:%d", __FUNCTION__, type, rate_min);
            env->DeleteLocalRef(SymbolRateRange);
        }

        env->DeleteWeakGlobalRef(frontendInfo);
    }

    if (attached) {
        Am_tuner_detachJNIEnv();
    }

    ALOGD("end:%s ", __FUNCTION__);
    return rate_min;
}
//bin.luo P3
void Wrapper_TuneInitialise(U8BIT paths)
{
    ALOGD("start:%s", __FUNCTION__);

    ALOGD("end:%s ", __FUNCTION__);
}
void Wrapper_TuneSetActualTsInputIdx(U8BIT path, S32BIT frontend_fd)  //Tuner Framework ²»Ö§³ÖÌá¹©get·½·¨£¬»ñÈ¡¸Ã²ÎÊý
{
    ALOGD("start:%s", __FUNCTION__);
    ALOGD("%s:not support", __FUNCTION__);
    ALOGD("end:%s ", __FUNCTION__);
}
void Wrapper_TuneSetActualSupportedSystemType(U8BIT path, S32BIT frontend_fd)   //Tuner Framework ²»Ö§³ÖÌá¹©get·½·¨£¬»ñÈ¡¸Ã²ÎÊý
{
    ALOGD("start:%s", __FUNCTION__);
    ALOGD("%s:not support", __FUNCTION__);
    ALOGD("end:%s ", __FUNCTION__);
}
void Wrapper_TuneSetSystemType(U8BIT path, EW_STB_TUNE_SYSTEM_TYPE type)
{
    ALOGD("start:%s", __FUNCTION__);
    sys_type = type;
    ALOGD("end:%s sys_type:%d", __FUNCTION__, sys_type);
}
EW_STB_TUNE_SYSTEM_TYPE Wrapper_TuneGetSystemType(U8BIT path)
{
    ALOGD("start:%s", __FUNCTION__);

    ALOGD("end:%s sys_type:%d", __FUNCTION__, sys_type);
    return sys_type;
}
void Wrapper_TuneAutoRelock(U8BIT path, BOOLEAN state)
{
    ALOGD("start:%s", __FUNCTION__);
    auto_relock = state;
    ALOGD("end:%s auto_relock:%d", __FUNCTION__, auto_relock);
}
EW_STB_TUNE_TCODERATE Wrapper_TuneGetActualTerrLpCodeRate(U8BIT path)  //Tuner Framework ²»Ö§³ÖÌá¹©get·½·¨£¬»ñÈ¡¸Ã²ÎÊý
{
    ALOGD("start:%s", __FUNCTION__);
    ALOGD("%s:not support", __FUNCTION__);
    ALOGD("end:%s", __FUNCTION__);
    return WRAPPER_TUNE_TCODERATE_UNDEFINED;
}
EW_STB_TUNE_TCODERATE Wrapper_TuneGetActualTerrHpCodeRate(U8BIT path)  //Tuner Framework ²»Ö§³ÖÌá¹©get·½·¨£¬»ñÈ¡¸Ã²ÎÊý
{
    ALOGD("start:%s", __FUNCTION__);
    ALOGD("%s:not support", __FUNCTION__);
    ALOGD("end:%s", __FUNCTION__);
    return WRAPPER_TUNE_TCODERATE_UNDEFINED;
}
EW_STB_TUNE_TGUARDINT Wrapper_TuneGetActualTerrGuardInt(U8BIT path)
{
    ALOGD("start:%s", __FUNCTION__);
    ALOGD("end:%s", __FUNCTION__);
    return WRAPPER_TUNE_TGUARDINT_UNDEFINED;
}
U16BIT Wrapper_TuneGetActualTerrCellId(U8BIT path)  //Tuner Framework ²»Ö§³ÖÌá¹©get·½·¨£¬»ñÈ¡¸Ã²ÎÊý
{
    ALOGD("start:%s", __FUNCTION__);
    ALOGD("%s:not support", __FUNCTION__);
    ALOGD("end:%s", __FUNCTION__);
    return 0;
}
void Wrapper_TuneUpdateFeUsage(U8BIT path, BOOLEAN use)
{
    ALOGD("start:%s", __FUNCTION__);
    if (use)
    {
        frontend_usage++;
    }else
    {
        frontend_usage--;
    }
    ALOGD("end:%s", __FUNCTION__);
}
BOOLEAN Wrapper_TuneIsTvPlatform()
{
    return isTvPlatform;
}
//bin.luo P4
void Wrapper_TuneRestartTuner(U8BIT path)  //ÀÏ½Ó¿ÚÃ»ÓÐ£¬ÔÝ²»ÊµÏÖ
{
    ALOGD("start:%s", __FUNCTION__);
    ALOGD("%s:not support", __FUNCTION__);
    ALOGD("end:%s", __FUNCTION__);
}
void Wrapper_TuneSetSearchMode(U8BIT path, BOOLEAN mode)
{
    ALOGD("start:%s  path:%d", __FUNCTION__, path);
    if (Wrapper_TuneIsTvPlatform())
    {
        if (tuner_search_mode != mode)
        {
            if (mode && tuner_state == E_TUNER_EXITED)
            {
                tuner_state = E_TUNER_IDLE;
            }
            else if (!mode && tuner_state == E_TUNER_IDLE)
            {
                /* DTVKit can only be set to the exit state when it exits. */
                /* tuner_status[path].state = TUNER_EXITED; */
            }

            tuner_search_mode = mode;

        }
    }
    ALOGD("end:%s", __FUNCTION__);
}
BOOLEAN Wrapper_TuneIsSearchMode(U8BIT path)
{
    ALOGD("start:%s", __FUNCTION__);

    BOOLEAN search_mode = FALSE;
    if (Wrapper_TuneIsTvPlatform())
    {
        search_mode = tuner_search_mode;
    }
    ALOGD("end:%s", __FUNCTION__);
    return search_mode;
}
void Wrapper_TuneAllStart()
{
    ALOGD("start:%s", __FUNCTION__);
    if (Wrapper_TuneIsTvPlatform() && tuner_state == E_TUNER_EXITED)
    {
        tuner_state = E_TUNER_IDLE;
        tuner_search_mode = FALSE;
    }
    ALOGD("end:%s", __FUNCTION__);
}

void Wrapper_TuneAllStop()
{
    ALOGD("start:%s", __FUNCTION__);
    ALOGD("end:%s", __FUNCTION__);
}

void Wrapper_TuneSetFrontendFd(U8BIT path, U32BIT fe_fd)
{
    ALOGD("start:%s", __FUNCTION__);
    ALOGD("end:%s", __FUNCTION__);
}
//dvb-c
EW_STB_TUNE_CMODE Wrapper_TuneGetActualCableMode(U8BIT path)
{
    ALOGD("start:%s  path:%d", __FUNCTION__, path);
    EW_STB_TUNE_CMODE CableMode = cable_mode;
    ALOGD("end:%s CableMode:%d", __FUNCTION__, CableMode);
    return CableMode;
}

//dvb-s pr4
EW_STB_TUNE_MODULATION Wrapper_TuneGetModulation(U8BIT path)
{
    ALOGD("start:%s  path:%d", __FUNCTION__, path);
    if  (E_TERR_TYPE_DVBS == signal_type)
    {
        ALOGD("end:%s signal_type; %d tune_modulation:%d", __FUNCTION__, signal_type, tune_modulation);
        return tune_modulation;
    }
    else
    {
        ALOGD("end:%s signal_type; %d tune_modulation:%d", __FUNCTION__, signal_type, tune_modulation);
        return WRAPPER_TUNE_MOD_AUTO;
    }
}
void Wrapper_TuneSetModulation(U8BIT path, EW_STB_TUNE_MODULATION modulation)
{
    ALOGD("start:%s modulation:%d", __FUNCTION__, modulation);
    tune_modulation  = modulation;
    ALOGD("end:%s", __FUNCTION__);
}
void Wrapper_TuneSetLOFrequency(U8BIT tuner, U16BIT lo_freq)
{
    ALOGD("start:%s", __FUNCTION__);
    tuner_lo_freq = lo_freq;
    ALOGD("end:%s lo_freq:%d", __FUNCTION__, lo_freq);
}
EW_STB_TUNE_LNB_VOLTAGE Wrapper_TuneGetLNBVoltage(U8BIT path)
{
    ALOGD("start:%s  path:%d", __FUNCTION__, path);
    if (E_TERR_TYPE_DVBS == signal_type)
    {
        return tune_voltage;
    }
    else
    {
        return WRAPPER_LNB_VOLTAGE_OFF;
    }
}
void Wrapper_TuneSetLNBVoltage(U8BIT path, EW_STB_TUNE_LNB_VOLTAGE voltage, BOOLEAN retune)
{
    ALOGD("start:%s  path:%d", __FUNCTION__, path);
    int Client_id = INVALID_TUNER_ID;
    TUNER_MAP::iterator it = tuner_map.find( path );
    if (it != tuner_map.end())
    {
        Client_id = it->second;
    }else
    {
        ALOGD("end:%s tuner_client is invalid", __FUNCTION__);
        return;
    }
    tuner_lnb = Am_tuner_openLnb(Client_id, (long)LnbCallback);
    if (tuner_lnb == NULL)
    {
        ALOGD("end:%s tuner_lnb is NULL", __FUNCTION__);
        return;
    }
    if (E_TERR_TYPE_DVBS == signal_type)
    {
        if (tune_voltage != voltage)
        {
            tune_voltage = voltage;
            if (retune)
            {
                tuning_params_changed = TRUE;
            }
        }
        Wrapper_TuneSetVoltageInterface(path, voltage);
    }

    ALOGD("end:%s", __FUNCTION__);
}
void Wrapper_TuneSetVoltageInterface(U8BIT path, EW_STB_TUNE_LNB_VOLTAGE voltage)  //ioctl setting µ½Çý¶¯²ã µçÑ¹Öµ
{
    ALOGD("start:%s  path:%d", __FUNCTION__, path);
    DVBS_LNB_VOLTAGE voltage_type = DVBS_LNB_VOLTAGE_NONE;
    if (WRAPPER_LNB_VOLTAGE_14V == voltage)
    {
        voltage_type = DVBS_LNB_VOLTAGE_14V;
    }else if (WRAPPER_LNB_VOLTAGE_18V == voltage)
    {
        voltage_type = DVBS_LNB_VOLTAGE_18V;
    }else
    {
        voltage_type = DVBS_LNB_VOLTAGE_NONE;
    }
    Am_lnb_setVoltage(tuner_lnb, voltage_type);
    ALOGD("end:%s", __FUNCTION__);
}
BOOLEAN Wrapper_TuneGet22kState(U8BIT path)
{
    ALOGD("start:%s  path:%d", __FUNCTION__, path);
    if (E_TERR_TYPE_DVBS == signal_type)
    {
        return use_22khz;
    }
    else
    {
        return FALSE;
    }
}
void Wrapper_TuneSet22kState(U8BIT path, BOOLEAN state, BOOLEAN retune) //控制22khz 开关
{
    ALOGD("start:%s  path:%d", __FUNCTION__, path);
    if (E_TERR_TYPE_DVBS == signal_type)
    {
        if (use_22khz != state)
        {
            use_22khz = state;
            if (retune)
            {
                tuning_params_changed = TRUE;
            }
        }
        Wrapper_TuneSetTone(path, use_22khz);
    }
    ALOGD("end:%s", __FUNCTION__);
}
BOOLEAN Wrapper_TuneSetTone(U8BIT path, BOOLEAN use_22khz)
{
    ALOGD("start:%s", __FUNCTION__);
    int client_id = INVALID_TUNER_ID;
    TUNER_MAP::iterator it = tuner_map.find( path );
    if (it != tuner_map.end())
    {
        client_id = it->second;
    }else
    {
        ALOGD("tuner_client is invalid");
        return FALSE;
    }
    tuner_lnb = Am_tuner_openLnb(client_id, (long)LnbCallback);
    if (tuner_lnb == NULL)
    {
        ALOGD("end:%s tuner_lnb is NULL", __FUNCTION__);
        return FALSE;
    }

    if (use_22khz == TRUE)
    {
        Am_lnb_setTone(tuner_lnb, DVBS_LNB_TONE_CONTINUOUS);
        ALOGD("end:%s  TRUE", __FUNCTION__);
        return TRUE;
    }else
    {
        Am_lnb_setTone(tuner_lnb, DVBS_LNB_TONE_NONE);
        ALOGD("end:%s  FALSE", __FUNCTION__);
        return FALSE;
    }
    ALOGD("end:%s", __FUNCTION__);
    return TRUE;
}
void Wrapper_TuneSendDISEQCMessage(U8BIT path, U8BIT *data, U8BIT size) //ÏòÓ²¼þ²ã·¢ËÍDisEqc message
{
    ALOGD("start:%s", __FUNCTION__);
    int client_id = INVALID_TUNER_ID;
    TUNER_MAP::iterator it = tuner_map.find( path );
    if (it != tuner_map.end())
    {
         client_id = it->second;
    }else
    {
        ALOGD("tuner_client is invalid");
         return;
    }

    if (size > 0)
    {
         std::vector<char> message;
         for (U8BIT i = 0; i < size; i++)
         {
             ALOGD("%s [0x%02x]", __FUNCTION__, data[i]);
             message.push_back(static_cast<char>(data[i]));
         }
         Am_lnb_sendDiseqcMessage(tuner_lnb, message);
    }

    ALOGD("end:%s", __FUNCTION__);
}
void Wrapper_TuneSendBurstMessage(U8BIT path, U8BIT data)  //ÏòÓ²¼þ²ã·¢ËÍBurst message
{
    ALOGD("start:%s", __FUNCTION__);
    int Client_id = INVALID_TUNER_ID;
    TUNER_MAP::iterator it = tuner_map.find( path );
    if (it != tuner_map.end())
    {
        Client_id = it->second;
    }else
    {
        ALOGD("tuner_client is invalid");
        return;
    }

    ALOGD("%s [0x%02x]", __FUNCTION__, data);

    DVBS_LNB_POSITION pos = DVBS_LNB_POSITION_UNDEFINED;
    if (data == 0x00 || data == 0xFF)
    {
        if (data == 0x00)
        {
            pos = DVBS_LNB_POSITION_POSITION_A;
        }
        else
        {
            pos = DVBS_LNB_POSITION_POSITION_B;
        }
        Am_lnb_setSatellitePosition(tuner_lnb, pos);
    }
    ALOGD("end:%s", __FUNCTION__);
}

void Wrapper_TuneReceiveDISEQCReply(U8BIT path, U8BIT *data, U8BIT size, U32BIT timeout) //´Óµ×²ã¶ÁDisEqc reply
{
    ALOGD("start:%s", __FUNCTION__);
    ALOGD("end:%s", __FUNCTION__);
}
/*
void Wrapper_TuneSetPulseLimitEast(U8BIT path, U16BIT count)
{
    ALOGD("start:%s", __FUNCTION__);
    ALOGD("end:%s", __FUNCTION__);
}
void Wrapper_TuneSetPulseLimitWest(U8BIT path, U16BIT count)
{
    ALOGD("start:%s", __FUNCTION__);
    ALOGD("end:%s", __FUNCTION__);
}
*/
//Pr5
BOOLEAN Wrapper_Tune_BlindScan(U8BIT path, E_TTYPE sys_type, Wrapper_Tnue_BlindCallback_t cb, void *user_data,
                                      unsigned int start_freq, unsigned int stop_freq, EW_STB_TUNE_BlindUnicable_t unicable)
{
    ALOGD("start:%s path:%d", __FUNCTION__,path);

    blindscan_tp_freq.clear();
    blindscan_tp_srate.clear();

    int client_id = Am_tuner_getTunerClientIdByType(TUNER_TYPE_DEFAULT);
    ALOGD("start:%s client_id:%d  gTunerClient:%d", __FUNCTION__, client_id, gTunerClient);
    if (curr_path != path)
    {
        MAP_INSERT_ITEM(tuner_map, path, client_id);
        curr_path = path;
    }
    else
    {
        TUNER_MAP::iterator it = tuner_map.find(path);
        if (it != tuner_map.end())
        {
            if (client_id != it->second)
            {
                Am_lnb_close(tuner_lnb);
                tuner_lnb = Am_tuner_openLnb(client_id, (long)LnbCallback);
                if (tuner_lnb == NULL)
                {
                    ALOGD("start:%s   tuner_lnb is Null", __FUNCTION__);
                }
                it = tuner_map.erase(it);
                MAP_INSERT_ITEM(tuner_map, path, client_id);
            }
        }
        else
        {
            MAP_INSERT_ITEM(tuner_map, path, client_id);
        }
    }

    gTunerClient = client_id;

    ALOGD("gTunerClient: %d ", gTunerClient);
    bool attached = false;
    JNIEnv *env = Am_tuner_getJNIEnv(&attached);
    if (NULL == env)
    {
        ALOGD("%s : test fail, env is null", __FUNCTION__);
        return FALSE;
    }

    long callbackContext = (long)blindscanCallback;
    ALOGD("%s : signal_type:%d", __FUNCTION__,signal_type);
    if (signal_type == E_TERR_TYPE_DVBC)
    {
        // TODO
    }
    else if(signal_type == E_TERR_TYPE_DVBS)
    {
        blindscan_event_cb = cb;
        blindscan_cb_user_data = user_data;

        Dvbs_Frontend_Settings dvbsFrontendSettings;
        memset(&dvbsFrontendSettings, 0, sizeof(Dvbs_Frontend_Settings));
        jobject dvbsSettingObject = dvb_utils_getDvbsFrontendSettingsObject(env, dvbsFrontendSettings);
        if (NULL == dvbsSettingObject) {
            ALOGD("%s : test fail, dvbs frontend setting not create", __FUNCTION__);
            testFailLeave(attached);
            return FALSE;
        }
        Am_tuner_scan(gTunerClient, dvbsSettingObject, SCAN_TYPE_BLIND, callbackContext);
        curr_starttune = TRUE;
        blindscan_mode = TRUE;
    }

    tuning_params_changed = FALSE;
    ReleaseEnv(attached);
    ALOGD("end:%s ", __FUNCTION__);

    return TRUE;
}
BOOLEAN Wrapper_Tune_BlindExit(U8BIT path)
{
    return TRUE;
}
void Wrapper_Tune_BlindGetTPCount(U8BIT path, U16BIT *count)
{
    ALOGD("start:%s", __FUNCTION__);
    if (blindscan_tp_freq.size() != blindscan_tp_srate.size())
    {
        ALOGE("%s freq size=%d srate size=%d", __FUNCTION__, blindscan_tp_freq.size(), blindscan_tp_srate.size());
        *count = 0;
    }
    else
    {
        *count = (U16BIT)(blindscan_tp_freq.size());
    }
    ALOGD("end:%s", __FUNCTION__);
}
BOOLEAN Wrapper_Tune_BlindGetTPInfo(U8BIT path, U32BIT** freq, U32BIT** srate, U16BIT *count)
{
    ALOGD("start:%s", __FUNCTION__);
    if (blindscan_tp_freq.size() != blindscan_tp_srate.size())
    {
        ALOGE("%s freq size=%d srate size=%d", __FUNCTION__, blindscan_tp_freq.size(), blindscan_tp_srate.size());
        *count = 0;
        return FALSE;
    }
    else
    {
        *count = (U16BIT)(blindscan_tp_freq.size());
        *freq = blindscan_tp_freq.data();
        *srate = blindscan_tp_srate.data();
    }

    ALOGD("end:%s", __FUNCTION__);
    return TRUE;
}

