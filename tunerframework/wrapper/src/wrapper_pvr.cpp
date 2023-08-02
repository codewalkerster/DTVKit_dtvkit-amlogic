#include "wrapper_pvr.h"

#include <jni.h>
#include "JNIASPlayer.h"
#include "JNI_tuner.h"
#include "JDvrLibJNI.h"

#define LOG_TAG "wrapper_pvr"

#define DEBUG_FUNCTIONS
#ifdef DEBUG_FUNCTIONS
#define LOG_ENTER ALOGD("%s:%d, enter",__FUNCTION__,__LINE__)
#define LOG_LEAVE ALOGD("%s:%d, leave",__FUNCTION__,__LINE__)
#else
#define LOG_ENTER
#define LOG_LEAVE
#endif

using namespace android;

struct dvr_recorder_settings_t {
    jfieldID statusMaskField;
    jfieldID lowThresholdField;
    jfieldID highThresholdField;
    jfieldID packetSizeField;
    jfieldID dataFormatField;
    jfieldID recorderBufferSizeField;
    jfieldID filterBufferSizeField;
    jfieldID segmentSizeField;
};

struct dvr_player_settings_t {
};

static int sPVRTunerClientId = 0xffff;
static jobject sTunerForPVR = NULL;

static jclass gDvrRecorderSettingsCls;
static dvr_recorder_settings_t gDvrRecorderSettingsCtx;

static jclass gDvrPlayerSettingsCls;
static dvr_player_settings_t gDvrPlayerSettingsCtx;

static jfieldID GetFieldIDOrDie(JNIEnv* env, jclass clazz, const char* field_name, const char* field_signature) {
    jfieldID res = env->GetFieldID(clazz, field_name, field_signature);
    if (res == NULL)
    {
        ALOGE("Unable to find static field %s with signature %s", field_name, field_signature);
    }
    return res;
}

S8BIT Wrapper_PVR_Initialise(void)
{
    LOG_ENTER;

    bool needsDetach = false;
    JNIEnv *env = Am_tuner_getJNIEnv(&needsDetach);
    if (NULL == env)
    {
        ALOGE("failed to get JNIEnv*. needsDetach:%d",(int)needsDetach);
        return -1;
    }
    //sPVRTunerClientId = Am_tuner_getTunerClientId();
    //if (sPVRTunerClientId == 0xffff)
    //{
    //    ALOGE("failed to call Am_tuner_getTunerClientId");
    //    return -1;
    //}
    //sTunerForPVR = Am_tuner_getTunerObjectByClientId(sPVRTunerClientId);
    //if (sTunerForPVR == NULL)
    //{
    //    ALOGE("failed to call Am_tuner_getTunerObjectByClientId");
    //    if (needsDetach)
    //    {
    //        Am_tuner_detachJNIEnv();
    //    }
    //    return -1;
    //}
    sTunerForPVR = Am_tuner_getRecordTuner();
    if (sTunerForPVR == NULL)
    {
        ALOGE("failed to call Am_tuner_getRecordTuner");
        if (needsDetach)
        {
            Am_tuner_detachJNIEnv();
        }
        return -1;
    }
    //ALOGD("TunerForPVR:0x%x",sTunerForPVR);

    jclass dvrRecorderSettingsCls = env->FindClass("com/droidlogic/jdvrlib/JDvrRecorderSettings");
    gDvrRecorderSettingsCls = static_cast<jclass>(env->NewGlobalRef(dvrRecorderSettingsCls));
    env->DeleteLocalRef(dvrRecorderSettingsCls);
    gDvrRecorderSettingsCtx.statusMaskField = GetFieldIDOrDie(env, gDvrRecorderSettingsCls, "mStatusMask", "I");
    gDvrRecorderSettingsCtx.lowThresholdField = GetFieldIDOrDie(env, gDvrRecorderSettingsCls, "mLowThreshold", "J");
    gDvrRecorderSettingsCtx.highThresholdField = GetFieldIDOrDie(env, gDvrRecorderSettingsCls, "mHighThreshold", "J");
    gDvrRecorderSettingsCtx.packetSizeField = GetFieldIDOrDie(env, gDvrRecorderSettingsCls, "mPacketSize", "J");
    gDvrRecorderSettingsCtx.dataFormatField = GetFieldIDOrDie(env, gDvrRecorderSettingsCls, "mDataFormat", "I");
    gDvrRecorderSettingsCtx.recorderBufferSizeField = GetFieldIDOrDie(env, gDvrRecorderSettingsCls, "mRecorderBufferSize", "I");
    gDvrRecorderSettingsCtx.filterBufferSizeField = GetFieldIDOrDie(env, gDvrRecorderSettingsCls, "mFilterBufferSize", "I");
    gDvrRecorderSettingsCtx.segmentSizeField = GetFieldIDOrDie(env, gDvrRecorderSettingsCls, "mSegmentSize", "I");

    jclass dvrPlayerSettingsCls = env->FindClass("com/droidlogic/jdvrlib/JDvrPlayerSettings");
    gDvrPlayerSettingsCls = static_cast<jclass>(env->NewGlobalRef(dvrPlayerSettingsCls));
    env->DeleteLocalRef(dvrPlayerSettingsCls);

    LOG_LEAVE;
    return 0;
}

S8BIT Wrapper_PVR_File_create1(const PU8BIT path_prefix, BOOLEAN trunc, am_dvr_file_handle* phandle)
{
    LOG_ENTER;

    S8BIT ret = AmDvr_File_create1((const char*)path_prefix,(bool)trunc,phandle);

    LOG_LEAVE;
    return ret;
}

S8BIT Wrapper_PVR_File_create2(const PU8BIT path_prefix, S32BIT limit_size, S32BIT limit_seconds, BOOLEAN trunc, am_dvr_file_handle* phandle)
{
    LOG_ENTER;

    S8BIT ret = AmDvr_File_create2((const char*)path_prefix,limit_size,limit_seconds,(bool)trunc,phandle);

    LOG_LEAVE;
    return ret;
}

S8BIT Wrapper_PVR_File_create3(const PU8BIT path_prefix, am_dvr_file_handle* phandle)
{
    LOG_ENTER;

    S8BIT ret = AmDvr_File_create3((const char*)path_prefix,phandle);

    LOG_LEAVE;
    return ret;
}

S8BIT Wrapper_PVR_File_duration(am_dvr_file_handle handle, int64_t* pduration)
{
    LOG_ENTER;

    S8BIT ret = AmDvr_File_duration(handle,pduration);

    LOG_LEAVE;
    return ret;
}

S8BIT Wrapper_PVR_File_size(am_dvr_file_handle handle, int64_t* psize)
{
    LOG_ENTER;

    S8BIT ret = AmDvr_File_size(handle,psize);

    LOG_LEAVE;
    return ret;
}

S8BIT Wrapper_PVR_File_destroy(am_dvr_file_handle handle)
{
    LOG_ENTER;

    AmDvr_File_destroy(handle);

    LOG_LEAVE;
    return 0;
}

S8BIT Wrapper_PVR_deleteRecord(const PU8BIT path_prefix)
{
    LOG_ENTER;

    AmDvr_deleteRecord((char*)path_prefix);

    LOG_LEAVE;
    return 0;
}

S8BIT Wrapper_PVR_Recorder_create(wrapper_recorder_init_params* params, am_dvr_recorder_handle* phandle)
{
    LOG_ENTER;

    bool needsDetach = false;
    JNIEnv *env = Am_tuner_getJNIEnv(&needsDetach);
    if (NULL == env)
    {
        ALOGE("failed to get JNIEnv*. needsDetach:%d",(int)needsDetach);
        return -1;
    }

    am_dvr_recorder_init_params recorder_params;

    recorder_params.tuner = sTunerForPVR;
    recorder_params.jdvrfile_handle = (am_dvr_file_handle)params->jdvrfile_handle;
    recorder_params.settings = env->AllocObject(gDvrRecorderSettingsCls);
    env->SetIntField(recorder_params.settings,gDvrRecorderSettingsCtx.statusMaskField,0);
    env->SetLongField(recorder_params.settings,gDvrRecorderSettingsCtx.lowThresholdField,250);
    env->SetLongField(recorder_params.settings,gDvrRecorderSettingsCtx.highThresholdField,750);
    env->SetLongField(recorder_params.settings,gDvrRecorderSettingsCtx.packetSizeField,188);
    env->SetIntField(recorder_params.settings,gDvrRecorderSettingsCtx.dataFormatField,0);
    env->SetIntField(recorder_params.settings,gDvrRecorderSettingsCtx.recorderBufferSizeField,188*32768);
    env->SetIntField(recorder_params.settings,gDvrRecorderSettingsCtx.filterBufferSizeField,188*4096);
    env->SetIntField(recorder_params.settings,gDvrRecorderSettingsCtx.segmentSizeField,
            params->segment_size>0 ? params->segment_size : 100*1024*1024);
    recorder_params.callback = params->callback;

    S8BIT ret = AmDvr_Recorder_create(&recorder_params,phandle);

    LOG_LEAVE;
    return ret;
}

S8BIT Wrapper_PVR_Recorder_addStream(am_dvr_recorder_handle handle, S32BIT pid, am_dvr_stream_type type, S32BIT format)
{
    LOG_ENTER;

    S8BIT ret = AmDvr_Recorder_addStream(handle,pid,type,format);

    LOG_LEAVE;
    return ret;
}

S8BIT Wrapper_PVR_Recorder_removeStream(am_dvr_recorder_handle handle, S32BIT pid)
{
    LOG_ENTER;

    S8BIT ret = AmDvr_Recorder_removeStream(handle,pid);

    LOG_LEAVE;
    return ret;
}

S8BIT Wrapper_PVR_Recorder_start(am_dvr_recorder_handle handle)
{
    LOG_ENTER;

    S8BIT ret = AmDvr_Recorder_start(handle);

    LOG_LEAVE;
    return ret;
}

S8BIT Wrapper_PVR_Recorder_stop(am_dvr_recorder_handle handle)
{
    LOG_ENTER;

    S8BIT ret = AmDvr_Recorder_stop(handle);

    LOG_LEAVE;
    return ret;
}

S8BIT Wrapper_PVR_Recorder_pause(am_dvr_recorder_handle handle)
{
    LOG_ENTER;

    S8BIT ret = AmDvr_Recorder_pause(handle);

    LOG_LEAVE;
    return ret;
}

S8BIT Wrapper_PVR_Player_create(wrapper_player_init_params* params, am_dvr_player_handle* phandle)
{
    LOG_ENTER;

    bool needsDetach = false;
    JNIEnv *env = Am_tuner_getJNIEnv(&needsDetach);
    if (NULL == env)
    {
        ALOGE("failed to get JNIEnv*. needsDetach:%d",(int)needsDetach);
        return -1;
    }

    am_dvr_player_init_params dvr_player_params;

    dvr_player_params.tuner = sTunerForPVR;
    dvr_player_params.jdvrfile_handle = (am_dvr_file_handle)params->jdvrfile_handle;
    dvr_player_params.settings = env->AllocObject(gDvrPlayerSettingsCls);
    dvr_player_params.callback = params->callback;
    dvr_player_params.surface = Am_tuner_getSurface();

    S8BIT ret = AmDvr_Player_create(&dvr_player_params,phandle);
    if (ret == -1)
    {
        ALOGE("Fail to create JDvrPlayer");
        return -1;
    }
    ALOGD("JDvrPlayer handle: %p", *phandle);

    LOG_LEAVE;
    return 0;
}

S8BIT Wrapper_PVR_Player_play(am_dvr_player_handle handle)
{
    LOG_ENTER;

    S8BIT ret = AmDvr_Player_play(handle);

    LOG_LEAVE;
    return ret;
}

S8BIT Wrapper_PVR_Player_stop(am_dvr_player_handle handle)
{
    LOG_ENTER;

    S8BIT ret = AmDvr_Player_stop(handle);

    LOG_LEAVE;
    return ret;
}

