//! C/C++
#include <stdint.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <poll.h>
#include <map>
#include <vector>
#include <pthread.h>

//!self
#include "wrapper_player.h"
#include "wrapper_dmx.h"
#include "wrapper_frontend.h"
#include "filter_utils.h"

//!JNI
#include <jni.h>
#include "JNIASPlayer.h"
#include "JNI_tuner.h"

#define LOG_TAG "wrapper_player"

using namespace android;

static int gPlayerClient = WRAPPER_PLAYER_INVALID_ID;
static jobject PlayerTuner;
static auto playerFailLeave = [](bool attached){if (attached) Am_tuner_detachJNIEnv();};
static jobject gPlayerWeakRefVideoFilter = NULL;
static jobject gPlayerWeakRefAudioFilter = NULL;

static void player_VideoFilterCallback(jobject filter, jobjectArray filterEventArray, int filterStatus);
static void player_AudioFilterCallback(jobject filter, jobjectArray filterEventArray, int filterStatus);


/******************************************************************/

S8BIT Wrapper_Player_Initialise(void)
{
    bool attached = false;
    JNIEnv *env = Am_tuner_getJNIEnv(&attached);
    if (NULL == env) {
        ALOGD("%s : get fail, env is null", __FUNCTION__);
        return -1 ;
    }
    else
    {
        ALOGD("%s : get env success!", __FUNCTION__);
    }

    //Set JNI evn, need create by ASPlayer self?
    gPlayerClient = Am_tuner_getTunerClientId();
    if (INVALID_TUNER_ID == gPlayerClient) {
        ALOGD("%s : get fail", __FUNCTION__);
        return -1 ;
    }
    PlayerTuner = Am_tuner_getTunerObjectByClientId(gPlayerClient);
    if (NULL == PlayerTuner) {
        ALOGD("%s : get fail, get Tuner object is null", __FUNCTION__);
        playerFailLeave(attached);
        return -1 ;
    }
    return 0;
}

S8BIT Wrapper_Player_Create(jni_asplayer_init_params params, jni_asplayer_handle *handle)
{
    S8BIT ret = -1;
    jni_asplayer_handle player_handle;
    if (JniASPlayer_create(params, (void *)PlayerTuner, &player_handle) == JNI_ASPLAYER_OK)
    {
        ret = JNI_ASPLAYER_OK;
        ALOGD("%s : player_handle = %u", __FUNCTION__, player_handle);
        JniASPlayer_prepare(player_handle);
        * handle = player_handle;
    }
    else
    {
        ALOGD("%s : Player set fail, handle = %u", __FUNCTION__, player_handle);
    }
    return ret;
}

S8BIT Wrapper_Player_Destroy(jni_asplayer_handle handle)
{
    S8BIT ret = -1;
    if (JniASPlayer_release(handle) == JNI_ASPLAYER_OK)
    {
        ret = JNI_ASPLAYER_OK;
        ALOGD("%s : handle = %u", __FUNCTION__, handle);
    }
    else
    {
        ALOGD("%s : Player set fail, handle = %u", __FUNCTION__, handle);
    }
    return ret;
}

S8BIT Wrapper_Player_SetParams(jni_asplayer_handle handle, jni_asplayer_parameter type, void *parameter)
{
    S8BIT ret = -1;
    if (1)//(JniASPlayer_setParams(handle, type, parameter) == JNI_ASPLAYER_OK)
    {
        ret = JNI_ASPLAYER_OK;
        ALOGD("%s : func not impl, handle = %u", __FUNCTION__, handle);
    }
    else
    {
        ALOGD("%s : Player set fail, handle = %u", __FUNCTION__, handle);
    }
    return ret;
}

S8BIT Wrapper_Player_SetVideoParams(jni_asplayer_handle handle, jni_asplayer_video_params *params)
{
    S8BIT ret = -1;
    if (JniASPlayer_setVideoParams(handle, params) == JNI_ASPLAYER_OK)
    {
        ret = JNI_ASPLAYER_OK;
        ALOGD("%s : handle = %u", __FUNCTION__, handle);
    }
    else
    {
        ALOGD("%s : Player set fail, handle = %u", __FUNCTION__, handle);
    }
    return ret;
}

S8BIT Wrapper_Player_SetAudioParams(jni_asplayer_handle handle, jni_asplayer_audio_params *params)
{
    S8BIT ret = -1;
    if (JniASPlayer_setAudioParams(handle, params) == JNI_ASPLAYER_OK)
    {
        ret = JNI_ASPLAYER_OK;
        ALOGD("%s : handle = %u", __FUNCTION__, handle);
    }
    else
    {
        ALOGD("%s : Player set fail, handle = %u", __FUNCTION__, handle);
    }
    return ret;
}

S8BIT Wrapper_Player_GetVideoInfo(jni_asplayer_handle handle, jni_asplayer_video_info *pInfo)
{
    S8BIT ret = -1;
    if (JniASPlayer_getVideoInfo(handle, pInfo) == JNI_ASPLAYER_OK)
    {
        ret = JNI_ASPLAYER_OK;
        ALOGD("%s : handle = %u", __FUNCTION__, handle);
    }
    else
    {
        ALOGD("%s : Player get fail, handle = %u", __FUNCTION__, handle);
    }
    return ret;
}

S8BIT Wrapper_Player_SetAudioStereoMode(jni_asplayer_handle handle, jni_asplayer_audio_stereo_mode Mode)
{
    S8BIT ret = -1;
    if (1)//JniASPlayer_setAudioStereoMode(handle, Mode) == JNI_ASPLAYER_OK)
    {
        ret = JNI_ASPLAYER_OK;
        ALOGD("%s : func not impl, handle = %u", __FUNCTION__, handle);
    }
    else
    {
        ALOGD("%s : Player get fail, handle = %u", __FUNCTION__, handle);
    }
    return ret;
}

S8BIT Wrapper_Player_GetAudioStereoMode(jni_asplayer_handle handle, jni_asplayer_audio_stereo_mode *pMode)
{
    S8BIT ret = -1;
    if (0)//JniASPlayer_getAudioStereoMode(handle, pMode) == JNI_ASPLAYER_OK)
    {
        ret = JNI_ASPLAYER_OK;
        ALOGD("%s : handle = %u", __FUNCTION__, handle);
    }
    else
    {
        ALOGD("%s : Player get fail, handle = %u", __FUNCTION__, handle);
    }
    return ret;
}

S8BIT Wrapper_Player_SetSurface(jni_asplayer_handle handle)
{
    S8BIT ret = -1;
    jobject surface = Am_tuner_getSurface();
    if (JniASPlayer_setSurface(handle, (void *)surface) == JNI_ASPLAYER_OK)
    {
        ret = JNI_ASPLAYER_OK;
        ALOGD("%s : handle = %u", __FUNCTION__, handle);
    }
    else
    {
        ALOGD("%s : Player set fail, handle = %u", __FUNCTION__, handle);
    }
    return ret;
}

S8BIT Wrapper_Player_RegisterEventCallBack(jni_asplayer_handle handle, event_callback cb, void* userData)
{
    S8BIT ret = -1;
    if (JniASPlayer_registerCb(handle, cb, userData) == JNI_ASPLAYER_OK)
    {
        ret = JNI_ASPLAYER_OK;
        ALOGD("%s : handle = %u", __FUNCTION__, handle);
    }
    else
    {
        ALOGD("%s : Player set fail, handle = %u", __FUNCTION__, handle);
    }
    return ret;
}

S8BIT Wrapper_Player_ShowVideo(jni_asplayer_handle handle)
{
    return 0;
}

S8BIT Wrapper_Player_HideVideo(jni_asplayer_handle handle)
{
    return 0;
}

S8BIT Wrapper_Player_StartVideoDecoding(jni_asplayer_handle handle)
{
    S8BIT ret = -1;
    if (JniASPlayer_startVideoDecoding(handle) == JNI_ASPLAYER_OK)
    {
        ret = JNI_ASPLAYER_OK;
        ALOGD("%s : handle = %u", __FUNCTION__, handle);
    }
    else
    {
        ALOGD("%s : Player set fail, handle = %u", __FUNCTION__, handle);
    }
    return ret;
}


S8BIT Wrapper_Player_StopVideoDecoding(jni_asplayer_handle handle)
{
    S8BIT ret = -1;
    if (JniASPlayer_stopVideoDecoding(handle) == JNI_ASPLAYER_OK)
    {
        ret = JNI_ASPLAYER_OK;
        ALOGD("%s : handle = %u", __FUNCTION__, handle);
    }
    else
    {
        ALOGD("%s : Player set fail, handle = %u", __FUNCTION__, handle);
    }
    return ret;
}

S8BIT Wrapper_Player_StartAudioDecoding(jni_asplayer_handle handle)
{
    S8BIT ret = -1;
    if (JniASPlayer_startAudioDecoding(handle) == JNI_ASPLAYER_OK)
    {
        ret = JNI_ASPLAYER_OK;
        ALOGD("%s : handle = %u", __FUNCTION__, handle);
    }
    else
    {
        ALOGD("%s : Player set fail, handle = %u", __FUNCTION__, handle);
    }
    return ret;
}

S8BIT Wrapper_Player_StopAudioDecoding(jni_asplayer_handle handle)
{
    S8BIT ret = -1;
    if (JniASPlayer_stopAudioDecoding(handle) == JNI_ASPLAYER_OK)
    {
        ret = JNI_ASPLAYER_OK;
        ALOGD("%s : handle = %u", __FUNCTION__, handle);
    }
    else
    {
        ALOGD("%s : Player set fail, handle = %u", __FUNCTION__, handle);
    }
    return ret;
}

S8BIT Wrapper_Player_SetADParams(jni_asplayer_handle handle, jni_asplayer_audio_params *pParams)
{
    S8BIT ret = -1;
    if (JniASPlayer_setADParams(handle, pParams) == JNI_ASPLAYER_OK)
    {
        ret = JNI_ASPLAYER_OK;
        ALOGD("%s : handle = %u", __FUNCTION__, handle);
    }
    else
    {
        ALOGD("%s : set AD params fail, handle = %u", __FUNCTION__, handle);
    }
    return ret;
}

S8BIT Wrapper_Player_EnableADMix(jni_asplayer_handle handle)
{
    S8BIT ret = -1;
    if (JniASPlayer_enableADMix(handle) == JNI_ASPLAYER_OK)
    {
        ret = JNI_ASPLAYER_OK;
        ALOGD("%s : handle = %u", __FUNCTION__, handle);
    }
    else
    {
        ALOGD("%s :enable AD Mix failed, handle = %u", __FUNCTION__, handle);
    }
    return ret;
}

S8BIT Wrapper_Player_DisableADMix(jni_asplayer_handle handle)
{
    S8BIT ret = -1;
    if (JniASPlayer_disableADMix(handle) == JNI_ASPLAYER_OK)
    {
        ret = JNI_ASPLAYER_OK;
        ALOGD("%s : handle = %u", __FUNCTION__, handle);
    }
    else
    {
        ALOGD("%s : disable AD Mix failed, handle = %u", __FUNCTION__, handle);
    }
    return ret;
}

S8BIT Wrapper_Player_SetADMixLevel(jni_asplayer_handle handle, S32BIT mix_level)
{
    S8BIT ret = -1;
    if (JniASPlayer_setADMixLevel(handle, mix_level) == JNI_ASPLAYER_OK)
    {
        ret = JNI_ASPLAYER_OK;
        ALOGD("%s : mix_level = %d, handle = %u", __FUNCTION__, mix_level, handle);
    }
    else
    {
        ALOGD("%s :set AD mix level failed, handle = %u", __FUNCTION__, handle);
    }
    return ret;
}

S8BIT Wrapper_Player_GetADMixLevel(jni_asplayer_handle handle, S32BIT *mix_level)
{
    S8BIT ret = -1;
    if (JniASPlayer_getADMixLevel(handle, mix_level) == JNI_ASPLAYER_OK)
    {
        ret = JNI_ASPLAYER_OK;
        ALOGD("%s : mix_level = %d, handle = %u", __FUNCTION__, *mix_level, handle);
    }
    else
    {
        ALOGD("%s :set AD mix level failed, handle = %u", __FUNCTION__, handle);
    }
    return ret;
}

S8BIT Wrapper_Player_SetAudioMute(jni_asplayer_handle handle, BOOLEAN audio_mute)
{
    S8BIT ret = -1;
    if (JniASPlayer_setAudioMute(handle, audio_mute, audio_mute) == JNI_ASPLAYER_OK)
    {
        ret = JNI_ASPLAYER_OK;
        ALOGD("%s : audio_mute = %d, handle = %u", __FUNCTION__, audio_mute, handle);
    }
    else
    {
        ALOGD("%s :set audio mute failed, handle = %u", __FUNCTION__, handle);
    }
    return ret;
}

S8BIT Wrapper_Player_SetPIPMode(jni_asplayer_handle handle, jni_asplayer_pip_mode mode)
{
    S8BIT ret = -1;
    if (JniASPlayer_setPIPMode(handle, mode) == JNI_ASPLAYER_OK)
    {
        ret = JNI_ASPLAYER_OK;
        ALOGD("%s : mode = %d, handle = %u", __FUNCTION__, mode, handle);
    }
    else
    {
        ALOGD("%s :set PIP mode failed, handle = %u", __FUNCTION__, handle);
    }
    return ret;
}

S8BIT Wrapper_Player_SetWorkMode(jni_asplayer_handle handle, jni_asplayer_work_mode mode)
{
    S8BIT ret = -1;
    if (JniASPlayer_setWorkMode(handle, mode) == JNI_ASPLAYER_OK)
    {
        ret = JNI_ASPLAYER_OK;
        ALOGD("%s : mode = %d, handle = %u", __FUNCTION__, mode, handle);
    }
    else
    {
        ALOGD("%s :set work mode failed, handle = %u", __FUNCTION__, handle);
    }
    return ret;
}

S8BIT Wrapper_Player_ResetWorkMode(jni_asplayer_handle handle)
{
    S8BIT ret = -1;
    if (JniASPlayer_resetWorkMode(handle) == JNI_ASPLAYER_OK)
    {
        ret = JNI_ASPLAYER_OK;
        ALOGD("%s :  handle = %u", __FUNCTION__, handle);
    }
    else
    {
        ALOGD("%s :reset work mode failed, handle = %u", __FUNCTION__, handle);
    }
    return ret;
}


int Wrapper_Player_GetAVFilterId(BOOLEAN isAudio, int pid, int vidoeStreamType, int audioStreamType)
{
    ALOGD("start:%s, isAudio : %d, pid :%d, vidoeStreamType : %d, audioStreamType : %d ", __FUNCTION__, isAudio, pid, vidoeStreamType, audioStreamType);

    bool attached = false;
    JNIEnv *env = Am_tuner_getJNIEnv(&attached);
    if (NULL == env) {
        ALOGD("%s : test fail, env is null", __FUNCTION__);
        return INVALID_VALUE;
    }
    jobject avFilter;
    if (true == isAudio) {
        Am_filter_callback vidoefilterCallback = player_VideoFilterCallback;
        avFilter = Am_tuner_openFilter(gPlayerClient, MAIN_TYPE_TS, SUBTYPE_AUDIO, WRAPPER_PLAYER_BUFFER_SIZE_VIDEO_DEFAULT, (long)vidoefilterCallback);
    } else {
        Am_filter_callback audioCallback = player_AudioFilterCallback;
        avFilter = Am_tuner_openFilter(gPlayerClient, MAIN_TYPE_TS, SUBTYPE_VIDEO, WRAPPER_PLAYER_BUFFER_SIZE_AUDIO_DEFAULT, (long)audioCallback);
    }

    if (NULL == avFilter) {
        ALOGD("%s : test fail, gPlayerWeakRefAudioFilter is null", __FUNCTION__);
        playerFailLeave(attached);
        return INVALID_VALUE;
    }

    TS_Filter_Configuration tsFilterConfiguration;
    memset(&tsFilterConfiguration, 0, sizeof(TS_Filter_Configuration));
    tsFilterConfiguration.pid = pid;
    tsFilterConfiguration.type = MAIN_TYPE_TS;
    tsFilterConfiguration.setting.av_setting.is_passthrough = true;
    if (true == isAudio) {
        tsFilterConfiguration.setting.av_setting.is_audio = true;
        tsFilterConfiguration.setting.av_setting.audio_strem_type = AUDIO_STREAM_TYPE_MPEG2;
    } else {
        tsFilterConfiguration.setting.av_setting.is_audio = false;
        tsFilterConfiguration.setting.av_setting.video_strem_type = true;
    }
    jobject tsFilterConfigurationObject = filter_utils_getAVTsFilterConfiguration(env, tsFilterConfiguration);
    if (NULL == tsFilterConfigurationObject) {
        ALOGD("%s : test fail, get AVTsFilterConfigurationObject object error", __FUNCTION__);
        env->DeleteWeakGlobalRef(avFilter);
        playerFailLeave(attached);
        return INVALID_VALUE;
    }
    int result = Am_filter_configure(avFilter, tsFilterConfigurationObject);
    ALOGD("%s : filter configure result : %d", __FUNCTION__, result);
    //3.start filter
    result = Am_filter_start(avFilter);
    ALOGD("%s : filter start result : %d", __FUNCTION__, result);
    //4.get filter Id
    int filterId = Am_filter_getId(avFilter);
    ALOGD("%s : filter Id : %d", __FUNCTION__, filterId);
    if (true == isAudio) {
        gPlayerWeakRefAudioFilter = avFilter;
    } else {
        gPlayerWeakRefVideoFilter = avFilter;
    }

    if (attached) {
        Am_tuner_detachJNIEnv();
    }
    ALOGD("end:%s", __FUNCTION__);
    return filterId;
}

int Wrapper_Player_GetAvSyncHwId()
{
    ALOGD("start:%s", __FUNCTION__);

    bool attached = false;
    JNIEnv *env = Am_tuner_getJNIEnv(&attached);
    if (NULL == env) {
        ALOGD("%s : test fail, env is null", __FUNCTION__);
        return INVALID_VALUE;
    }
    int avSyncHwId = Am_tuner_getAvSyncHwId(gPlayerClient, gPlayerWeakRefVideoFilter != NULL ? gPlayerWeakRefVideoFilter : gPlayerWeakRefAudioFilter);
    ALOGD("end:%s, AV SyncHwId : %d", __FUNCTION__, avSyncHwId);
    return avSyncHwId;
}

static void player_VideoFilterCallback(jobject filter, jobjectArray filterEventArray, int filterStatus)
{
    ALOGD("start:%s", __FUNCTION__);
}

static void player_AudioFilterCallback(jobject filter, jobjectArray filterEventArray, int filterStatus)
{
    ALOGD("start:%s", __FUNCTION__);
}


