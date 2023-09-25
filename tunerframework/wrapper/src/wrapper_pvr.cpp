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

static jobject sTunerForNormalRecording = NULL;
static jobject sTunerForTimeshiftRecording = NULL;
//static jobject sTunerForPlayback = NULL;

static jfieldID GetFieldIDOrDie(JNIEnv* env, jclass clazz, const char* field_name, const char* field_signature) {
    jfieldID res = env->GetFieldID(clazz, field_name, field_signature);
    if (res == NULL)
    {
        ALOGE("Unable to find static field %s with signature %s", field_name, field_signature);
    }
    return res;
}

// mode:0  prepare tuner for normal recording
// mode:1  prepare tuner for timeshift recording
// mode:2  prepare tuner for playback
S8BIT Wrapper_PVR_Initialise(int32_t mode)
{
    LOG_ENTER;

    if (mode == 0)
    {
       sTunerForNormalRecording = Am_tuner_getDvrTunerByType(TUNER_TYPE_DVR_RECORD);
       if (sTunerForNormalRecording == NULL)
       {
          ALOGE("failed to get Tuner for normal recording");
          return -1;
       }
       ALOGD("TunerForNormalRecording:%p",sTunerForNormalRecording);
    }
    else if (mode == 1)
    {
       sTunerForTimeshiftRecording = Am_tuner_getDvrTunerByType(TUNER_TYPE_DVR_TIMESHIFT_RECORD);
       if (sTunerForTimeshiftRecording == NULL)
       {
          ALOGE("failed to get Tuner for timeshift recording");
          return -1;
       }
       ALOGD("TunerForTimeshiftRecording:%p",sTunerForTimeshiftRecording);
    }
    else if(mode == 2)
    {
       //sTunerForPlayback = Am_tuner_getDvrTunerByType(TUNER_TYPE_DVR_PLAY);
       //if (sTunerForPlayback == NULL)
       //{
       //   ALOGE("failed to get Tuner for PVR playback");
       //   return -1;
       //}
       //ALOGD("TunerForPlayback:%p",sTunerForPlayback);
       ALOGE("Tuner object for playback is not expected to be obtained here");
    }
    else
    {
       ALOGE("Invalid mode %d is given",mode);
       return -1;
    }

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

S8BIT Wrapper_PVR_File_duration2(const PU8BIT path_prefix, int64_t* pduration)
{
    return AmDvr_File_duration2(path_prefix,pduration);
}

S8BIT Wrapper_PVR_File_size(am_dvr_file_handle handle, int64_t* psize)
{
    LOG_ENTER;

    S8BIT ret = AmDvr_File_size(handle,psize);

    LOG_LEAVE;
    return ret;
}

S8BIT Wrapper_PVR_File_size2(const PU8BIT path_prefix, int64_t* psize)
{
    return AmDvr_File_size2(path_prefix,psize);
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

S8BIT Wrapper_PVR_File_getVideoPID(am_dvr_file_handle handle, int32_t* pPID)
{
    //LOG_ENTER;

    AmDvr_File_getVideoPID(handle, pPID);

    //LOG_LEAVE;
    return 0;
}

S8BIT Wrapper_PVR_File_getAudioPID(am_dvr_file_handle handle, int32_t* pPID)
{
    //LOG_ENTER;

    AmDvr_File_getAudioPID(handle, pPID);

    //LOG_LEAVE;
    return 0;
}

S8BIT Wrapper_PVR_Recorder_create(wrapper_recorder_init_params* params, am_dvr_recorder_handle* phandle)
{
    LOG_ENTER;

    am_dvr_recorder_init_params recorder_params;

    recorder_params.tuner = (params->is_timeshift ? sTunerForTimeshiftRecording : sTunerForNormalRecording);
    recorder_params.jdvrfile_handle = (am_dvr_file_handle)params->jdvrfile_handle;
    recorder_params.settings = NULL;
    recorder_params.callback = params->callback;

    S8BIT ret = AmDvr_Recorder_create(&recorder_params,phandle);

    LOG_LEAVE;
    return ret;
}

S8BIT Wrapper_PVR_Recorder_addStream(am_dvr_recorder_handle handle, S32BIT pid, am_dvr_stream_type type, S32BIT format)
{
    S8BIT ret = AmDvr_Recorder_addStream(handle,pid,type,format);
    ALOGD("%s, pid:%d type:%d format:%d ret:%d",__FUNCTION__,pid,type,format,(int)ret);
    return ret;
}

S8BIT Wrapper_PVR_Recorder_removeStream(am_dvr_recorder_handle handle, S32BIT pid)
{
    S8BIT ret = AmDvr_Recorder_removeStream(handle,pid);
    ALOGD("%s, pid:%d ret:%d",__FUNCTION__,pid,(int)ret);
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

    if (params->asplayer_handle == 0)
    {
       ALOGE("Input ASPlayer handle is invalid");
       return -1;
    }
    ALOGD("ASPlayer handle: %p", params->asplayer_handle);

    jobject jasplayer;
    JniASPlayer_getJavaASPlayer(params->asplayer_handle,&jasplayer);
    if (jasplayer == 0)
    {
       ALOGE("ASPlayer jobject is invalid");
       return -1;
    }
    ALOGD("ASPlayer jobject: %p", jasplayer);

    am_dvr_player_init_params dvr_player_params;
    dvr_player_params.asplayer = jasplayer;
    dvr_player_params.jdvrfile_handle = (am_dvr_file_handle)params->jdvrfile_handle;
    dvr_player_params.settings = NULL;
    dvr_player_params.callback = params->callback;

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

S8BIT Wrapper_PVR_Player_pause(am_dvr_player_handle handle)
{
    LOG_ENTER;

    S8BIT ret = AmDvr_Player_pause(handle);

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

S8BIT Wrapper_PVR_Player_setSpeed(am_dvr_player_handle handle, double speed)
{
    LOG_ENTER;

    S8BIT ret = AmDvr_Player_setSpeed(handle, speed);

    LOG_LEAVE;
    return ret;
}

S8BIT Wrapper_PVR_Player_seek(am_dvr_player_handle handle, int32_t seconds)
{
    LOG_ENTER;

    S8BIT ret = AmDvr_Player_seek(handle, seconds);

    LOG_LEAVE;
    return ret;
}

