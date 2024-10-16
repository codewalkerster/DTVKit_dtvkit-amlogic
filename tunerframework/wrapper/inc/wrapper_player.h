#ifndef _TF_PLAYER_H
#define _TF_PLAYER_H
#include "techtype.h"
#ifdef __cplusplus
extern "C" {
#endif

//!JNI
#include "JNIASPlayer.h"
//typedef U8BIT bool;
//#define true 1
//#define false 0

#define WRAPPER_PLAYER_INVALID_HANDLE   (0)
#define WRAPPER_PLAYER_INVALID_ID   (0xFFFF)
#define WRAPPER_PLAYER_INVALID_RES_ID   (255)

#define WRAPPER_PLAYER_BUFFER_SIZE_VIDEO_DEFAULT 1024 * 1024 * 4L
#define WRAPPER_PLAYER_BUFFER_SIZE_AUDIO_DEFAULT 1024 * 1024 * 2L
#define WRAPPER_PLAYER_BUFFER_SIZE_PCR_DEFAULT 1024 * 1024

typedef enum {
    WP_VIDEO_STREAM_TYPE_UNDEFINED,
    WP_VIDEO_STREAM_TYPE_RESERVED,   // ITU-T | ISO/IEC Reserved
    WP_VIDEO_STREAM_TYPE_MPEG1,      // ISO/IEC 11172
    WP_VIDEO_STREAM_TYPE_MPEG2,      // ITU-T Rec.H.262 and ISO/IEC 13818-2
    WP_VIDEO_STREAM_TYPE_MPEG4P2,    // ISO/IEC 14496-2 (MPEG-4 H.263 based video)
    WP_VIDEO_STREAM_TYPE_AVC,        // ITU-T Rec.H.264 and ISO/IEC 14496-10
    WP_VIDEO_STREAM_TYPE_HEVC,       // ITU-T Rec. H.265 and ISO/IEC 23008-2
    WP_VIDEO_STREAM_TYPE_VVC,        // H.266
    WP_VIDEO_STREAM_TYPE_VC1,        // Microsoft VC.1
    WP_VIDEO_STREAM_TYPE_VP8,        // Google VP8
    WP_VIDEO_STREAM_TYPE_VP9,        // Google VP9
    WP_VIDEO_STREAM_TYPE_AV1,        // AOMedia Video 1
    WP_VIDEO_STREAM_TYPE_AVS,        // Chinese Standard
    WP_VIDEO_STREAM_TYPE_AVS2,       // New Chinese Standard
    WP_VIDEO_STREAM_TYPE_AVS3,       // New Chinese Standard
    WP_VIDEO_STREAM_TYPE_DVES_AVC,   // Dolby Vision
    WP_VIDEO_STREAM_TYPE_DVES_HEVC   // Dolby Vision
}WRAPPER_PLAYER_VIDEO_STREAM_TYPE;

typedef enum {
    WP_AUDIO_STREAM_TYPE_UNDEFINED,
    WP_AUDIO_STREAM_TYPE_PCM,        // Uncompressed Audio
    WP_AUDIO_STREAM_TYPE_MP3,        // MPEG Audio Layer III versions
    WP_AUDIO_STREAM_TYPE_MPEG1,      // ISO/IEC 11172 Audio
    WP_AUDIO_STREAM_TYPE_MPEG2,      // ISO/IEC 13818-3
    WP_AUDIO_STREAM_TYPE_MPEGH,      // ISO/IEC 23008-3 (MPEG-H Part 3)
    WP_AUDIO_STREAM_TYPE_AAC,        //ISO/IEC 14496-3
    WP_AUDIO_STREAM_TYPE_AC3,        //Dolby Digital
    WP_AUDIO_STREAM_TYPE_EAC3,       // Dolby Digital Plus
    WP_AUDIO_STREAM_TYPE_AC4,        //Dolby AC-4
    WP_AUDIO_STREAM_TYPE_DTS,        //Basic DTS
    WP_AUDIO_STREAM_TYPE_DTS_HD,     //High Resolution DTS
    WP_AUDIO_STREAM_TYPE_WMA,        //Windows Media Audio
    WP_AUDIO_STREAM_TYPE_OPUS,       // Opus Interactive Audio Codec
    WP_AUDIO_STREAM_TYPE_VORBIS,     // VORBIS Interactive Audio Codec
    WP_AUDIO_STREAM_TYPE_DRA,        // SJ/T 11368-2006
    WP_AUDIO_STREAM_TYPE_AAC_ADTS,   // AAC with ADTS (Audio Data Transport Format).
    WP_AUDIO_STREAM_TYPE_AAC_LATM,   // AAC with ADTS with LATM (Low-overhead MPEG-4 Audio Transport Multiplex).
    WP_AUDIO_STREAM_TYPE_AAC_HE_ADTS,// High-Efficiency AAC (HE-AAC) with ADTS (Audio Data Transport Format).
    WP_AUDIO_STREAM_TYPE_AAC_HE_LATM,// High-Efficiency AAC (HE-AAC) with LATM (Low-overhead MPEG-4 Audio Transport Multiplex).
}WRAPPER_PLAYER_AUDIO_STREAM_TYPE;

typedef enum {
    WP_TUNER_TYPE_LIVE_0               = 0,
    WP_TUNER_TYPE_LIVE_1               = 1,
    WP_TUNER_TYPE_DVR_RECORD           = 2,
    WP_TUNER_TYPE_DVR_TIMESHIFT_RECORD = 3,
    WP_TUNER_TYPE_DVR_PLAY             = 4,
    WP_TUNER_TYPE_SCAN                 = 5,
    WP_TUNER_TYPE_LIVE_2               = 6,
    WP_TUNER_TYPE_BACKGROUND           = 7,
    WP_TUNER_TYPE_MAX                  = 255
}WRAPPER_TUNER_TYPE;

S8BIT Wrapper_Player_AVInit(U8BIT player_paths);
S8BIT Wrapper_Player_Initialise(U8BIT av_path, WRAPPER_TUNER_TYPE tunerType);
S8BIT Wrapper_Player_Create(jni_asplayer_init_params params, jni_asplayer_handle *handle, U8BIT av_path);
S8BIT Wrapper_Player_Destroy(jni_asplayer_handle handle);
S8BIT Wrapper_Player_SetPlayerNo(U8BIT av_path, U8BIT player_no);
WRAPPER_TUNER_TYPE Wrapper_Player_GetPlayerTunerType(U8BIT av_path);
U8BIT Wrapper_Player_GetPlayerPathByNo(U8BIT player_no);
S8BIT Wrapper_Player_SetParams(jni_asplayer_handle handle, jni_asplayer_parameter type, void *parameter);
S8BIT Wrapper_Player_GetParams(jni_asplayer_handle handle, jni_asplayer_parameter type, void *parameter);
S8BIT Wrapper_Player_SetVideoParams(jni_asplayer_handle handle, jni_asplayer_video_params *video_params, WRAPPER_PLAYER_VIDEO_STREAM_TYPE format);
S8BIT Wrapper_Player_SetAudioParams(jni_asplayer_handle handle, jni_asplayer_audio_params *audio_params, WRAPPER_PLAYER_AUDIO_STREAM_TYPE format);
S8BIT Wrapper_Player_SetPcrPid(jni_asplayer_handle handle, U16BIT pcr_pid);
S8BIT Wrapper_Player_SetAudioLanguage(U32BIT pri_language_code, U32BIT sec_language_code);
S8BIT Wrapper_Player_GetVideoInfo(jni_asplayer_handle handle, jni_asplayer_video_info *pInfo);
S8BIT Wrapper_Player_SetAudioDualMonoMode(jni_asplayer_handle handle, jni_asplayer_audio_dual_mono_mode mode);
S8BIT Wrapper_Player_GetAudioDualMonoMode(jni_asplayer_handle handle, jni_asplayer_audio_dual_mono_mode *pMode);
S8BIT Wrapper_Player_SetSurface(jni_asplayer_handle handle, BOOLEAN is_pip);
S8BIT Wrapper_Player_RegisterEventCallBack(jni_asplayer_handle handle, event_callback cb, void* userData);
S8BIT Wrapper_Player_StartVideoDecoding(jni_asplayer_handle handle);
S8BIT Wrapper_Player_StopVideoDecoding(jni_asplayer_handle handle);
S8BIT Wrapper_Player_StartAudioDecoding(jni_asplayer_handle handle);
S8BIT Wrapper_Player_StopAudioDecoding(jni_asplayer_handle handle);
S8BIT Wrapper_Player_SwitchAudioTrack(jni_asplayer_handle handle, jni_asplayer_audio_params *audio_params, WRAPPER_PLAYER_AUDIO_STREAM_TYPE format);
S8BIT Wrapper_Player_SetADParams(jni_asplayer_handle handle, jni_asplayer_audio_params *ad_params, WRAPPER_PLAYER_AUDIO_STREAM_TYPE format);
S8BIT Wrapper_Player_EnableADMix(jni_asplayer_handle handle);
S8BIT Wrapper_Player_DisableADMix(jni_asplayer_handle handle);
S8BIT Wrapper_Player_SetADMixLevel(jni_asplayer_handle handle, S32BIT mix_level);
S8BIT Wrapper_Player_GetADMixLevel(jni_asplayer_handle handle, S32BIT *mix_level);
S8BIT Wrapper_Player_SetAudioMute(jni_asplayer_handle handle, BOOLEAN audio_mute);
S8BIT Wrapper_Player_SetVideoMute(jni_asplayer_handle handle, jni_asplayer_video_mute video_mute);
S8BIT Wrapper_Player_SetStillFrame(jni_asplayer_handle handle, jni_asplayer_transition_mode_before mode);
S8BIT Wrapper_Player_SetVideoColor(jni_asplayer_handle handle, jni_asplayer_screen_color_mode mode, jni_asplayer_screen_color color);
S8BIT Wrapper_Player_SetPIPMode(jni_asplayer_handle handle, jni_asplayer_pip_mode mode);
S8BIT Wrapper_Player_SetWorkMode(jni_asplayer_handle handle, jni_asplayer_work_mode mode);
S8BIT Wrapper_Player_ResetWorkMode(void);
U8BIT Wrapper_Player_GetPlayerPathByHandle(jni_asplayer_handle handle);
jni_asplayer_handle Wrapper_Player_GetPlayerHandleByPath(U16BIT av_path);

int Wrapper_Player_GetInstanceNo(jni_asplayer_handle handle);
int Wrapper_Player_GetSyncInstanceNo(jni_asplayer_handle handle);


#ifdef __cplusplus
}
#endif

#endif


