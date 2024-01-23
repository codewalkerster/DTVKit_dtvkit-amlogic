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
#include "wrapper_os.h"
#include "filter_utils.h"

//!JNI
#include <jni.h>
#include "JNIASPlayer.h"
#include "JNI_tuner.h"

#define LOG_TAG "wrapper_player"

using namespace android;

static auto playerFailLeave = [](bool attached){if (attached) Am_tuner_detachJNIEnv();};
static U8BIT num_paths = 0;

typedef struct
{
    U8BIT player_no;
    jni_asplayer_handle player_handle;
    WRAPPER_TUNER_TYPE tunerType;
    int playerClient;
    jobject playerTuner;
    jobject playerWeakRefVideoFilter;
    jobject playerWeakRefAudioFilter;
    jobject playerWeakRefADFilter;
}WRAPPER_PLAYER_AV_STATUS;

static WRAPPER_PLAYER_AV_STATUS *wp_player_av_status = NULL;

static int player_GetAVFilterId(bool isAudio, int pid, int videoStreamType, int audioStreamType, jni_asplayer_handle handle);
static int player_GetAVSyncHwId(jni_asplayer_handle handle);
static int player_GetADFilterId(int ad_pid, int audioStreamType, jni_asplayer_handle handle);
static int player_GetADSyncHwId(jni_asplayer_handle handle);
static void player_VideoFilterCallback(jobject filter, jobjectArray filterEventArray, int filterStatus);
static void player_AudioFilterCallback(jobject filter, jobjectArray filterEventArray, int filterStatus);


/******************************************************************/
S8BIT Wrapper_Player_AVInit(U8BIT player_paths)
{
    U16BIT av_path;

    if (wp_player_av_status == NULL)
    {
        wp_player_av_status = (WRAPPER_PLAYER_AV_STATUS *)wrapper_MEMGetSysRAM((sizeof(WRAPPER_PLAYER_AV_STATUS) * player_paths));
        memset(wp_player_av_status, 0, player_paths * sizeof(WRAPPER_PLAYER_AV_STATUS));
        num_paths = player_paths;

        if (wp_player_av_status != NULL)
        {
            for (av_path = 0; av_path < player_paths; av_path++)
            {
                wp_player_av_status[av_path].player_no = WRAPPER_PLAYER_INVALID_RES_ID;
                wp_player_av_status[av_path].player_handle = WRAPPER_PLAYER_INVALID_HANDLE;
                wp_player_av_status[av_path].tunerType = WP_TUNER_TYPE_MAX;
                wp_player_av_status[av_path].playerClient = WRAPPER_PLAYER_INVALID_ID;
                wp_player_av_status[av_path].playerTuner = NULL;
                wp_player_av_status[av_path].playerWeakRefVideoFilter = NULL;
                wp_player_av_status[av_path].playerWeakRefAudioFilter = NULL;
                wp_player_av_status[av_path].playerWeakRefADFilter = NULL;
            }
        }

        ALOGI("%s : init success.", __FUNCTION__);
    }
    return 0;
}

S8BIT Wrapper_Player_Initialise(U8BIT av_path, WRAPPER_TUNER_TYPE tunerType)
{
    bool attached = false;
    JNIEnv *env = Am_tuner_getJNIEnv(&attached);
    if (NULL == env) {
        ALOGI("%s : get fail, env is null", __FUNCTION__);
        return -1 ;
    }
    else
    {
        ALOGI("%s : tuner type: %d, get env success!", __FUNCTION__, tunerType);
    }

    //Set JNI evn, need create by ASPlayer self?
    wp_player_av_status[av_path].tunerType = tunerType;

    if (tunerType >= WP_TUNER_TYPE_DVR_RECORD && tunerType <= WP_TUNER_TYPE_DVR_PLAY)
    {
        wp_player_av_status[av_path].playerClient = Am_tuner_getTunerClientIdByType((int)tunerType);
        if (INVALID_TUNER_ID == wp_player_av_status[av_path].playerClient) {
            ALOGI("%s : get fail", __FUNCTION__);
            return -1 ;
        }
        wp_player_av_status[av_path].playerTuner = Am_tuner_getDvrTunerByType((int)tunerType);
    }
    else
    {
        switch (wp_player_av_status[av_path].player_no)
        {
            case 0:
            {
                tunerType = WP_TUNER_TYPE_LIVE_0;
                break ;
            }
            case 1:
            {
                tunerType = WP_TUNER_TYPE_LIVE_1;
                break ;
            }
            case 2 :
            {
                tunerType = WP_TUNER_TYPE_LIVE_2;
                break ;
            }
            default:
            {
                tunerType = WP_TUNER_TYPE_LIVE_0;
                break;
            }
        }
        wp_player_av_status[av_path].playerClient = Am_tuner_getTunerClientIdByType((int)tunerType);

        if (INVALID_TUNER_ID == wp_player_av_status[av_path].playerClient) {
            ALOGI("%s : get fail", __FUNCTION__);
            return -1 ;
        }
        wp_player_av_status[av_path].playerTuner = Am_tuner_getTunerObjectByClientId(wp_player_av_status[av_path].playerClient);
    }
    ALOGI("%s : playerClient = %d, tunerType: %d, av_path: %d", __FUNCTION__, wp_player_av_status[av_path].playerClient, tunerType, av_path);

    if (NULL == wp_player_av_status[av_path].playerTuner) {
        ALOGI("%s : get fail, get Tuner object is null", __FUNCTION__);
        playerFailLeave(attached);
        return -1 ;
    }

    if (attached) {
        Am_tuner_detachJNIEnv();
    }
    return 0;
}

S8BIT Wrapper_Player_Create(jni_asplayer_init_params params, jni_asplayer_handle *handle, U8BIT av_path)
{
    S8BIT ret = -1;
//    jni_asplayer_handle player_handle;
    if (JniASPlayer_create(params, (void *)wp_player_av_status[av_path].playerTuner, &wp_player_av_status[av_path].player_handle) == JNI_ASPLAYER_OK)
    {
        ret = JNI_ASPLAYER_OK;
        ALOGI("%s : player_handle = %u, av_path: %d", __FUNCTION__, wp_player_av_status[av_path].player_handle, av_path);
        JniASPlayer_prepare(wp_player_av_status[av_path].player_handle);
        * handle = wp_player_av_status[av_path].player_handle;
    }
    else
    {
        ALOGI("%s : Player set fail, handle = %u", __FUNCTION__, wp_player_av_status[av_path].player_handle);
    }
    return ret;
}

S8BIT Wrapper_Player_Destroy(jni_asplayer_handle handle)
{
    S8BIT ret = -1;

    U8BIT av_path = Wrapper_Player_GetPlayerPathByHandle(handle);
    wp_player_av_status[av_path].player_no = WRAPPER_PLAYER_INVALID_RES_ID;
    wp_player_av_status[av_path].player_handle = WRAPPER_PLAYER_INVALID_HANDLE;
    wp_player_av_status[av_path].tunerType = WP_TUNER_TYPE_MAX;
    wp_player_av_status[av_path].playerClient = WRAPPER_PLAYER_INVALID_ID;
    wp_player_av_status[av_path].playerTuner = NULL;
    wp_player_av_status[av_path].playerWeakRefVideoFilter = NULL;
    wp_player_av_status[av_path].playerWeakRefAudioFilter = NULL;
    wp_player_av_status[av_path].playerWeakRefADFilter = NULL;

    if (JniASPlayer_release(handle) == JNI_ASPLAYER_OK)
    {
        ret = JNI_ASPLAYER_OK;
        ALOGI("%s : handle = %u", __FUNCTION__, handle);
    }
    else
    {
        ALOGI("%s : Player set fail, handle = %u", __FUNCTION__, handle);
    }
    return ret;
}

BOOLEAN Wrapper_Player_SetPlayerNo(U8BIT player_no)
{
   U8BIT i;
   BOOLEAN acquired = FALSE;

   for (i = 0; i < num_paths; i++)
   {
       if (wp_player_av_status[i].player_no == WRAPPER_PLAYER_INVALID_RES_ID)
       {
           wp_player_av_status[i].player_no = player_no;
           acquired = TRUE;
           break;
       }
   }

   return acquired;
}

U8BIT Wrapper_Player_GetPlayerPathByNo(U8BIT player_no)
{
    U8BIT i;
    U8BIT av_path = WRAPPER_PLAYER_INVALID_RES_ID;

    for (i = 0; i < num_paths; i++)
    {
       if (player_no != WRAPPER_PLAYER_INVALID_RES_ID && wp_player_av_status[i].player_no == player_no)
       {
          av_path = i;
       }
    }

   return av_path;
}

S8BIT Wrapper_Player_SetParams(jni_asplayer_handle handle, jni_asplayer_parameter type, void *parameter)
{
    S8BIT ret = -1;
    if (1)//(JniASPlayer_setParams(handle, type, parameter) == JNI_ASPLAYER_OK)
    {
        ret = JNI_ASPLAYER_OK;
        ALOGI("%s : func not impl, handle = %u", __FUNCTION__, handle);
    }
    else
    {
        ALOGI("%s : Player set fail, handle = %u", __FUNCTION__, handle);
    }
    return ret;
}

S8BIT Wrapper_Player_SetVideoParams(jni_asplayer_handle handle, jni_asplayer_video_params *video_params, WRAPPER_PLAYER_VIDEO_STREAM_TYPE format)
{
    S8BIT ret = -1;

    if (video_params != NULL)
    {
        video_params->filterId = player_GetAVFilterId(false, video_params->pid, format, WP_AUDIO_STREAM_TYPE_UNDEFINED, handle);
        video_params->avSyncHwId = player_GetAVSyncHwId(handle);
        if (JniASPlayer_setVideoParams(handle, video_params) == JNI_ASPLAYER_OK)
        {
            ret = JNI_ASPLAYER_OK;
            ALOGI("%s : video pid= %d, mime= %s, handle = %u", __FUNCTION__, video_params->pid, video_params->mimeType, handle);
        }
        else
        {
            ALOGI("%s : Player set fail, handle = %u", __FUNCTION__, handle);
        }
    }
    else
    {
        ALOGI("%s : Player param error, handle = %u", __FUNCTION__, handle);
    }
    return ret;
}

S8BIT Wrapper_Player_SetAudioParams(jni_asplayer_handle handle, jni_asplayer_audio_params *audio_params, WRAPPER_PLAYER_AUDIO_STREAM_TYPE format)
{
    S8BIT ret = -1;
    if (audio_params != NULL)
    {
        audio_params->filterId = player_GetAVFilterId(true, audio_params->pid, WP_VIDEO_STREAM_TYPE_UNDEFINED, format, handle);
        audio_params->avSyncHwId = player_GetAVSyncHwId(handle);

        if (JniASPlayer_setAudioParams(handle, audio_params) == JNI_ASPLAYER_OK)
        {
            ret = JNI_ASPLAYER_OK;
            ALOGI("%s : audio pid= %d, mime= %s, handle = %u", __FUNCTION__, audio_params->pid, audio_params->mimeType, handle);
        }
        else
        {
            ALOGI("%s : Player set fail, handle = %u", __FUNCTION__, handle);
        }
    }
    else
    {
        ALOGI("%s : Player param error, handle = %u", __FUNCTION__, handle);
    }
    return ret;
}

S8BIT Wrapper_Player_GetVideoInfo(jni_asplayer_handle handle, jni_asplayer_video_info *pInfo)
{
    S8BIT ret = -1;
    if (JniASPlayer_getVideoInfo(handle, pInfo) == JNI_ASPLAYER_OK)
    {
        ret = JNI_ASPLAYER_OK;
        ALOGI("%s : handle = %u", __FUNCTION__, handle);
    }
    else
    {
        ALOGI("%s : Player get fail, handle = %u", __FUNCTION__, handle);
    }
    return ret;
}

S8BIT Wrapper_Player_SetAudioDualMonoMode(jni_asplayer_handle handle, jni_asplayer_audio_dual_mono_mode Mode)
{
    S8BIT ret = -1;
    if (JniASPlayer_setAudioDualMonoMode(handle, Mode) == JNI_ASPLAYER_OK)
    {
        ret = JNI_ASPLAYER_OK;
        ALOGI("%s : mode= %d, handle = %u", __FUNCTION__, Mode, handle);
    }
    else
    {
        ALOGI("%s : Player get fail, handle = %u", __FUNCTION__, handle);
    }
    return ret;
}

S8BIT Wrapper_Player_GetAudioDualMonoMode(jni_asplayer_handle handle, jni_asplayer_audio_dual_mono_mode *pMode)
{
    S8BIT ret = -1;
    if (JniASPlayer_getAudioDualMonoMode(handle, pMode) == JNI_ASPLAYER_OK)
    {
        ret = JNI_ASPLAYER_OK;
        ALOGI("%s : handle = %u", __FUNCTION__, handle);
    }
    else
    {
        ALOGI("%s : Player get fail, handle = %u", __FUNCTION__, handle);
    }
    return ret;
}

S8BIT Wrapper_Player_SetSurface(jni_asplayer_handle handle)
{
    S8BIT ret = -1;

    U8BIT av_path = Wrapper_Player_GetPlayerPathByHandle(handle);
    jobject surface = Am_tuner_getSurfaceByTunerClient(wp_player_av_status[av_path].playerClient);
    if (JniASPlayer_setSurface(handle, (void *)surface) == JNI_ASPLAYER_OK)
    {
        Am_tuner_DeleteSurfaceRef(surface);
        ret = JNI_ASPLAYER_OK;
        ALOGI("%s : handle = %u", __FUNCTION__, handle);
    }
    else
    {
        ALOGI("%s : Player set fail, handle = %u", __FUNCTION__, handle);
    }
    return ret;
}

S8BIT Wrapper_Player_RegisterEventCallBack(jni_asplayer_handle handle, event_callback cb, void* userData)
{
    S8BIT ret = -1;
    if (JniASPlayer_registerCb(handle, cb, userData) == JNI_ASPLAYER_OK)
    {
        ret = JNI_ASPLAYER_OK;
        ALOGI("%s : handle = %u", __FUNCTION__, handle);
    }
    else
    {
        ALOGI("%s : Player set fail, handle = %u", __FUNCTION__, handle);
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
        ALOGI("%s : handle = %u", __FUNCTION__, handle);
    }
    else
    {
        ALOGI("%s : Player set fail, handle = %u", __FUNCTION__, handle);
    }
    return ret;
}


S8BIT Wrapper_Player_StopVideoDecoding(jni_asplayer_handle handle)
{
    S8BIT ret = -1;
    if (JniASPlayer_stopVideoDecoding(handle) == JNI_ASPLAYER_OK)
    {
        ret = JNI_ASPLAYER_OK;
        U8BIT av_path = Wrapper_Player_GetPlayerPathByHandle(handle);
        if (av_path != WRAPPER_PLAYER_INVALID_RES_ID)
        {
            if (wp_player_av_status[av_path].playerWeakRefVideoFilter != NULL)
            {
                Am_filter_close(wp_player_av_status[av_path].playerWeakRefVideoFilter);
                wp_player_av_status[av_path].playerWeakRefVideoFilter = NULL;
            }
        }
        ALOGI("%s : av_path = %d, handle = %u", __FUNCTION__, av_path, handle);
    }
    else
    {
        ALOGI("%s : Player set fail, handle = %u", __FUNCTION__, handle);
    }
    return ret;
}

S8BIT Wrapper_Player_StartAudioDecoding(jni_asplayer_handle handle)
{
    S8BIT ret = -1;
    if (JniASPlayer_startAudioDecoding(handle) == JNI_ASPLAYER_OK)
    {
        ret = JNI_ASPLAYER_OK;
        ALOGI("%s : handle = %u", __FUNCTION__, handle);
    }
    else
    {
        ALOGI("%s : Player set fail, handle = %u", __FUNCTION__, handle);
    }
    return ret;
}

S8BIT Wrapper_Player_StopAudioDecoding(jni_asplayer_handle handle)
{
    S8BIT ret = -1;
    if (JniASPlayer_stopAudioDecoding(handle) == JNI_ASPLAYER_OK)
    {
        ret = JNI_ASPLAYER_OK;
        U8BIT av_path = Wrapper_Player_GetPlayerPathByHandle(handle);
        if (av_path != WRAPPER_PLAYER_INVALID_RES_ID)
        {
            if (wp_player_av_status[av_path].playerWeakRefAudioFilter != NULL)
            {
                Am_filter_close(wp_player_av_status[av_path].playerWeakRefAudioFilter);
                wp_player_av_status[av_path].playerWeakRefAudioFilter = NULL;
            }
        }
        ALOGI("%s : av_path = %d, handle = %u", __FUNCTION__, av_path, handle);
    }
    else
    {
        ALOGI("%s : Player set fail, handle = %u", __FUNCTION__, handle);
    }
    return ret;
}

S8BIT Wrapper_Player_SwitchAudioTrack(jni_asplayer_handle handle, jni_asplayer_audio_params *audio_params, WRAPPER_PLAYER_AUDIO_STREAM_TYPE format)
{
    S8BIT ret = -1;
    if (audio_params != NULL)
    {
        audio_params->filterId = player_GetAVFilterId(true, audio_params->pid, WP_VIDEO_STREAM_TYPE_UNDEFINED, format, handle);
        audio_params->avSyncHwId = player_GetAVSyncHwId(handle);
        if (JniASPlayer_switchAudioTrack(handle, audio_params) == JNI_ASPLAYER_OK)
        {
            ret = JNI_ASPLAYER_OK;
            ALOGI("%s : audio pid= %d, handle = %u", __FUNCTION__, audio_params->pid, handle);
        }
        else
        {
            ALOGI("%s :switch audio track failed, handle = %u", __FUNCTION__, handle);
        }
    }
    else
    {
        ALOGI("%s :switch audio track failed, handle = %u", __FUNCTION__, handle);
    }
    return ret;
}

S8BIT Wrapper_Player_SetADParams(jni_asplayer_handle handle, jni_asplayer_audio_params *ad_params, WRAPPER_PLAYER_AUDIO_STREAM_TYPE format)
{
    S8BIT ret = -1;

    if (ad_params != NULL)
    {
        ad_params->filterId = player_GetADFilterId(ad_params->pid, format, handle);
        ad_params->avSyncHwId = player_GetADSyncHwId(handle);
        if (JniASPlayer_setADParams(handle, ad_params) == JNI_ASPLAYER_OK)
        {
            ret = JNI_ASPLAYER_OK;
            ALOGI("%s : AD pid= %d, mime= %s, handle = %u", __FUNCTION__, ad_params->pid, ad_params->mimeType, handle);
        }
        else
        {
            ALOGI("%s : set AD params fail, handle = %u", __FUNCTION__, handle);
        }
    }
    else
    {
        ALOGI("%s : Player param error, handle = %u", __FUNCTION__, handle);
    }
    return ret;
}

S8BIT Wrapper_Player_EnableADMix(jni_asplayer_handle handle)
{
    S8BIT ret = -1;
    if (JniASPlayer_enableADMix(handle) == JNI_ASPLAYER_OK)
    {
        ret = JNI_ASPLAYER_OK;
        ALOGI("%s : handle = %u", __FUNCTION__, handle);
    }
    else
    {
        ALOGI("%s :enable AD Mix failed, handle = %u", __FUNCTION__, handle);
    }
    return ret;
}

S8BIT Wrapper_Player_DisableADMix(jni_asplayer_handle handle)
{
    S8BIT ret = -1;
    if (JniASPlayer_disableADMix(handle) == JNI_ASPLAYER_OK)
    {
        ret = JNI_ASPLAYER_OK;
        U8BIT av_path = Wrapper_Player_GetPlayerPathByHandle(handle);
        if (av_path != WRAPPER_PLAYER_INVALID_RES_ID)
        {
            if (wp_player_av_status[av_path].playerWeakRefADFilter != NULL)
            {
                Am_filter_close(wp_player_av_status[av_path].playerWeakRefADFilter);
                wp_player_av_status[av_path].playerWeakRefADFilter = NULL;
            }
        }
        ALOGI("%s : av_path = %d, handle = %u", __FUNCTION__, av_path, handle);
    }
    else
    {
        ALOGI("%s : disable AD Mix failed, handle = %u", __FUNCTION__, handle);
    }
    return ret;
}

S8BIT Wrapper_Player_SetADMixLevel(jni_asplayer_handle handle, S32BIT mix_level)
{
    S8BIT ret = -1;
    if (JniASPlayer_setADMixLevel(handle, mix_level) == JNI_ASPLAYER_OK)
    {
        ret = JNI_ASPLAYER_OK;
        ALOGI("%s : mix_level = %d, handle = %u", __FUNCTION__, mix_level, handle);
    }
    else
    {
        ALOGI("%s :set AD mix level failed, handle = %u", __FUNCTION__, handle);
    }
    return ret;
}

S8BIT Wrapper_Player_GetADMixLevel(jni_asplayer_handle handle, S32BIT *mix_level)
{
    S8BIT ret = -1;
    if (JniASPlayer_getADMixLevel(handle, mix_level) == JNI_ASPLAYER_OK)
    {
        ret = JNI_ASPLAYER_OK;
        ALOGI("%s : mix_level = %d, handle = %u", __FUNCTION__, *mix_level, handle);
    }
    else
    {
        ALOGI("%s :set AD mix level failed, handle = %u", __FUNCTION__, handle);
    }
    return ret;
}

S8BIT Wrapper_Player_SetAudioMute(jni_asplayer_handle handle, BOOLEAN audio_mute)
{
    S8BIT ret = -1;
    if (JniASPlayer_setAudioMute(handle, audio_mute, audio_mute) == JNI_ASPLAYER_OK)
    {
        ret = JNI_ASPLAYER_OK;
        ALOGI("%s : audio_mute = %d, handle = %u", __FUNCTION__, audio_mute, handle);
    }
    else
    {
        ALOGI("%s :set audio mute failed, handle = %u", __FUNCTION__, handle);
    }
    return ret;
}

S8BIT Wrapper_Player_SetVideoMute(jni_asplayer_handle handle, jni_asplayer_video_mute video_mute)
{
    S8BIT ret = -1;
    ALOGI("%s : start", __FUNCTION__);

    if (JniASPlayer_setVideoMute(handle, video_mute) == JNI_ASPLAYER_OK)
    {
        ret = JNI_ASPLAYER_OK;
        ALOGI("%s : video_mute = %d, handle = %u", __FUNCTION__, video_mute, handle);
    }
    else
    {
        ALOGI("%s :set video mute failed, handle = %u", __FUNCTION__, handle);
    }
    return ret;
}

S8BIT Wrapper_Player_SetVideoBlackOut(jni_asplayer_handle handle, jni_asplayer_transition_mode_before mode)
{
    S8BIT ret = -1;
    if (JniASPlayer_setTransitionModeBefore(handle, mode) == JNI_ASPLAYER_OK)
    {
        ret = JNI_ASPLAYER_OK;
        ALOGI("%s : mode = %d, handle = %u", __FUNCTION__, mode, handle);
    }
    else
    {
        ALOGI("%s :set video failed, handle = %u", __FUNCTION__, handle);
    }
    return ret;
}

S8BIT Wrapper_Player_SetVideoColor(jni_asplayer_handle handle, jni_asplayer_screen_color_mode mode, jni_asplayer_screen_color color)
{
    S8BIT ret = -1;
    if (JniASPlayer_setScreenColor(handle, mode, color) == JNI_ASPLAYER_OK)
    {
        ret = JNI_ASPLAYER_OK;
        ALOGI("%s : color = %d, handle = %u", __FUNCTION__, color, handle);
    }
    else
    {
        ALOGI("%s :set video failed, handle = %u", __FUNCTION__, handle);
    }
    return ret;
}

S8BIT Wrapper_Player_SetPIPMode(jni_asplayer_handle handle, jni_asplayer_pip_mode mode)
{
    S8BIT ret = -1;
    if (JniASPlayer_setPIPMode(handle, mode) == JNI_ASPLAYER_OK)
    {
        ret = JNI_ASPLAYER_OK;
        ALOGI("%s : mode = %d, handle = %u", __FUNCTION__, mode, handle);
    }
    else
    {
        ALOGI("%s :set PIP mode failed, handle = %u", __FUNCTION__, handle);
    }
    return ret;
}

S8BIT Wrapper_Player_SetWorkMode(jni_asplayer_handle handle, jni_asplayer_work_mode mode)
{
    S8BIT ret = -1;
    if (JniASPlayer_setWorkMode(handle, mode) == JNI_ASPLAYER_OK)
    {
        ret = JNI_ASPLAYER_OK;
        ALOGI("%s : mode = %d, handle = %u", __FUNCTION__, mode, handle);
    }
    else
    {
        ALOGI("%s :set work mode failed, handle = %u", __FUNCTION__, handle);
    }
    return ret;
}

S8BIT Wrapper_Player_ResetWorkMode(void)
{
    S8BIT ret = -1;
    U8BIT i;

    for (i = 0; i < num_paths; i++)
    {
        if (wp_player_av_status[i].tunerType == WP_TUNER_TYPE_LIVE_0 ||
            wp_player_av_status[i].tunerType == WP_TUNER_TYPE_LIVE_1 ||
            wp_player_av_status[i].tunerType == WP_TUNER_TYPE_LIVE_2)
        {
            if (JniASPlayer_resetWorkMode(wp_player_av_status[i].player_handle) == JNI_ASPLAYER_OK)
            {
                ret = JNI_ASPLAYER_OK;
                ALOGI("%s :  handle = %u, i = %d", __FUNCTION__, wp_player_av_status[i].player_handle, i);
            }
            else
            {
                ALOGI("%s :reset work mode failed, handle = %u, i = %d", __FUNCTION__, wp_player_av_status[i].player_handle, i);
            }
        }
    }

    return ret;
}

U8BIT Wrapper_Player_GetPlayerPathByHandle(jni_asplayer_handle handle)
{
    U8BIT i;
    U8BIT av_path = WRAPPER_PLAYER_INVALID_RES_ID;

    if (handle != WRAPPER_PLAYER_INVALID_HANDLE)
    {
        for (i = 0; i < num_paths; i++)
        {
            if (handle == wp_player_av_status[i].player_handle)
            {
                av_path = i;
            }
        }
    }
    ALOGI("%s : handle = %u, player_handle = %u, av_path: %d", __FUNCTION__, handle, wp_player_av_status[av_path].player_handle, av_path);

    return av_path;
}

jni_asplayer_handle Wrapper_Player_GetPlayerHandleByPath(U16BIT av_path)
{
    ALOGI("%s : player_handle = %u, av_path: %d", __FUNCTION__, wp_player_av_status[av_path].player_handle, av_path);
    return wp_player_av_status[av_path].player_handle;
}

static int player_GetAVFilterId(bool isAudio, int pid, int videoStreamType, int audioStreamType, jni_asplayer_handle handle)
{
    ALOGI("start:%s, isAudio : %d, pid :%d, videoStreamType : %d, audioStreamType : %d ", __FUNCTION__, isAudio, pid, videoStreamType, audioStreamType);

    bool attached = false;
    JNIEnv *env = Am_tuner_getJNIEnv(&attached);
    if (NULL == env) {
        ALOGI("%s : test fail, env is null", __FUNCTION__);
        return INVALID_VALUE;
    }

    jobject avFilter;
    U8BIT av_path = Wrapper_Player_GetPlayerPathByHandle(handle);
    if (true == isAudio) {
        if (wp_player_av_status[av_path].playerWeakRefAudioFilter != NULL)
        {
            Am_filter_close(wp_player_av_status[av_path].playerWeakRefAudioFilter);
            wp_player_av_status[av_path].playerWeakRefAudioFilter = NULL;
        }
        Am_filter_callback audioFilterCallback = player_AudioFilterCallback;
        avFilter = Am_tuner_openFilter(wp_player_av_status[av_path].playerClient, MAIN_TYPE_TS, SUBTYPE_AUDIO, WRAPPER_PLAYER_BUFFER_SIZE_AUDIO_DEFAULT, (long)audioFilterCallback, 0);
    } else {
        if (wp_player_av_status[av_path].playerWeakRefVideoFilter != NULL)
        {
            Am_filter_close(wp_player_av_status[av_path].playerWeakRefVideoFilter);
            wp_player_av_status[av_path].playerWeakRefVideoFilter = NULL;
        }
        Am_filter_callback videoFilterCallback = player_VideoFilterCallback;
        avFilter = Am_tuner_openFilter(wp_player_av_status[av_path].playerClient, MAIN_TYPE_TS, SUBTYPE_VIDEO, WRAPPER_PLAYER_BUFFER_SIZE_VIDEO_DEFAULT, (long)videoFilterCallback, 0);
    }

    if (NULL == avFilter) {
        ALOGI("%s : test fail, gPlayerWeakRefAudioFilter is null", __FUNCTION__);
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
        tsFilterConfiguration.setting.av_setting.audio_strem_type = audioStreamType;
    } else {
        tsFilterConfiguration.setting.av_setting.is_audio = false;
        tsFilterConfiguration.setting.av_setting.video_strem_type = videoStreamType;
    }
    jobject tsFilterConfigurationObject = filter_utils_getAVTsFilterConfiguration(env, tsFilterConfiguration);
    if (NULL == tsFilterConfigurationObject) {
        ALOGI("%s : test fail, get AVTsFilterConfigurationObject object error", __FUNCTION__);
        env->DeleteWeakGlobalRef(avFilter);
        playerFailLeave(attached);
        return INVALID_VALUE;
    }
    int result = Am_filter_configure(avFilter, tsFilterConfigurationObject);
    ALOGI("%s : filter configure result : %d", __FUNCTION__, result);
    //3.start filter
    result = Am_filter_start(avFilter);
    ALOGI("%s : filter start result : %d", __FUNCTION__, result);
    //4.get filter Id
    int filterId = Am_filter_getId(avFilter);
    ALOGI("%s : filter Id : %d", __FUNCTION__, filterId);
    if (true == isAudio) {
        wp_player_av_status[av_path].playerWeakRefAudioFilter = avFilter;
    } else {
        wp_player_av_status[av_path].playerWeakRefVideoFilter = avFilter;
    }

    if (attached) {
        Am_tuner_detachJNIEnv();
    }
    ALOGI("end:%s", __FUNCTION__);
    return filterId;
}

static int player_GetAVSyncHwId(jni_asplayer_handle handle)
{
    ALOGI("start:%s", __FUNCTION__);

    bool attached = false;
    JNIEnv *env = Am_tuner_getJNIEnv(&attached);
    if (NULL == env) {
        ALOGI("%s : test fail, env is null", __FUNCTION__);
        return INVALID_VALUE;
    }
    U16BIT av_path = Wrapper_Player_GetPlayerPathByHandle(handle);
    int avSyncHwId = Am_tuner_getAvSyncHwId(wp_player_av_status[av_path].playerClient,
        wp_player_av_status[av_path].playerWeakRefVideoFilter != NULL ? wp_player_av_status[av_path].playerWeakRefVideoFilter : wp_player_av_status[av_path].playerWeakRefAudioFilter);

    if (attached) {
        Am_tuner_detachJNIEnv();
    }
    ALOGI("end:%s, playerClient: %d, AV SyncHwId : %d", __FUNCTION__, wp_player_av_status[av_path].playerClient, avSyncHwId);
    return avSyncHwId;
}

static int player_GetADFilterId(int ad_pid, int audioStreamType, jni_asplayer_handle handle)
{
    ALOGI("start:%s, ad_pid :%d, audioStreamType : %d", __FUNCTION__, ad_pid, audioStreamType);

    bool attached = false;
    JNIEnv *env = Am_tuner_getJNIEnv(&attached);
    if (NULL == env) {
        ALOGI("%s : test fail, env is null", __FUNCTION__);
        return INVALID_VALUE;
    }

    jobject avFilter;
    U8BIT av_path = Wrapper_Player_GetPlayerPathByHandle(handle);
    if (wp_player_av_status[av_path].playerWeakRefADFilter != NULL)
    {
        Am_filter_close(wp_player_av_status[av_path].playerWeakRefADFilter);
        wp_player_av_status[av_path].playerWeakRefADFilter = NULL;
    }
    Am_filter_callback audioFilterCallback = player_AudioFilterCallback;
    avFilter = Am_tuner_openFilter(wp_player_av_status[av_path].playerClient, MAIN_TYPE_TS, SUBTYPE_AUDIO, WRAPPER_PLAYER_BUFFER_SIZE_AUDIO_DEFAULT, (long)audioFilterCallback, 0);

    if (NULL == avFilter) {
        ALOGI("%s : test fail, gPlayerWeakRefAudioFilter is null", __FUNCTION__);
        playerFailLeave(attached);
        return INVALID_VALUE;
    }

    TS_Filter_Configuration tsFilterConfiguration;
    memset(&tsFilterConfiguration, 0, sizeof(TS_Filter_Configuration));
    tsFilterConfiguration.pid = ad_pid;
    tsFilterConfiguration.type = MAIN_TYPE_TS;
    tsFilterConfiguration.setting.av_setting.is_passthrough = true;
    tsFilterConfiguration.setting.av_setting.is_audio = true;
    tsFilterConfiguration.setting.av_setting.audio_strem_type = audioStreamType;
    jobject tsFilterConfigurationObject = filter_utils_getAVTsFilterConfiguration(env, tsFilterConfiguration);
    if (NULL == tsFilterConfigurationObject) {
        ALOGI("%s : test fail, get AVTsFilterConfigurationObject object error", __FUNCTION__);
        env->DeleteWeakGlobalRef(avFilter);
        playerFailLeave(attached);
        return INVALID_VALUE;
    }
    int result = Am_filter_configure(avFilter, tsFilterConfigurationObject);
    ALOGI("%s : filter configure result : %d", __FUNCTION__, result);
    //3.start filter
    result = Am_filter_start(avFilter);
    ALOGI("%s : filter start result : %d", __FUNCTION__, result);
    //4.get filter Id
    int filterId = Am_filter_getId(avFilter);
    ALOGI("%s : filter Id : %d", __FUNCTION__, filterId);
    wp_player_av_status[av_path].playerWeakRefADFilter = avFilter;

    if (attached) {
        Am_tuner_detachJNIEnv();
    }
    ALOGI("end:%s", __FUNCTION__);
    return filterId;
}

static int player_GetADSyncHwId(jni_asplayer_handle handle)
{
    ALOGI("start:%s", __FUNCTION__);

    bool attached = false;
    JNIEnv *env = Am_tuner_getJNIEnv(&attached);
    if (NULL == env) {
        ALOGI("%s : test fail, env is null", __FUNCTION__);
        return INVALID_VALUE;
    }
    U16BIT av_path = Wrapper_Player_GetPlayerPathByHandle(handle);
    int avSyncHwId = Am_tuner_getAvSyncHwId(wp_player_av_status[av_path].playerClient, wp_player_av_status[av_path].playerWeakRefADFilter);

    if (attached) {
        Am_tuner_detachJNIEnv();
    }
    ALOGI("end:%s, playerClient: %d, AD SyncHwId : %d", __FUNCTION__, wp_player_av_status[av_path].playerClient, avSyncHwId);
    return avSyncHwId;
}

int Wrapper_Player_GetInstanceNo(jni_asplayer_handle handle)
{
    int ret = 0;
    if (JniASPlayer_getInstanceNo(handle, &ret) == JNI_ASPLAYER_OK)
    {
        ALOGI("%s :  handle = %u, instanceNo = %d", __FUNCTION__, handle, ret);
    }
    else
    {
        ALOGI("%s :get instanceNo failed, handle = %u", __FUNCTION__, handle);
    }
    return ret;
}

int Wrapper_Player_GetSyncInstanceNo(jni_asplayer_handle handle)
{
    int ret = -1;
    if (JniASPlayer_getSyncInstanceNo(handle, &ret) == JNI_ASPLAYER_OK)
    {
        ALOGI("%s :  handle = %u, syncId = %d", __FUNCTION__, handle, ret);
    }
    else
    {
        ALOGI("%s :get syncInstanceNo failed, handle = %u", __FUNCTION__, handle);
    }
    return ret;
}

static void player_VideoFilterCallback(jobject filter, jobjectArray filterEventArray, int filterStatus)
{
    ALOGI("start:%s", __FUNCTION__);
}

static void player_AudioFilterCallback(jobject filter, jobjectArray filterEventArray, int filterStatus)
{
    ALOGI("start:%s", __FUNCTION__);
}

