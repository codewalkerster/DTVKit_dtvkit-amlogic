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

#define WRAPPER_PLAYER_BUFFER_SIZE_VIDEO_DEFAULT 1024 * 1024 * 4L
#define WRAPPER_PLAYER_BUFFER_SIZE_AUDIO_DEFAULT 1024 * 1024 * 2L

typedef enum {
    WP_VIDEO_STREAM_TYPE_UNDEFINED,
    WP_VIDEO_STREAM_TYPE_RESERVED,   // ITU-T | ISO/IEC Reserved
    WP_VIDEO_STREAM_TYPE_MPEG1,      // ISO/IEC 11172
    WP_VIDEO_STREAM_TYPE_MPEG2,      // ITU-T Rec.H.262 and ISO/IEC 13818-2
    WP_VIDEO_STREAM_TYPE_MPEG4P2,    // ISO/IEC 14496-2 (MPEG-4 H.263 based video)
    WP_VIDEO_STREAM_TYPE_AVC,        // ITU-T Rec.H.264 and ISO/IEC 14496-10
    WP_VIDEO_STREAM_TYPE_HEVC,       // ITU-T Rec. H.265 and ISO/IEC 23008-2
    WP_VIDEO_STREAM_TYPE_VC1,        // Microsoft VC.1
    WP_VIDEO_STREAM_TYPE_VP8,        // Google VP8
    WP_VIDEO_STREAM_TYPE_VP9,        // Google VP9
    WP_VIDEO_STREAM_TYPE_AV1,        // AOMedia Video 1
    WP_VIDEO_STREAM_TYPE_AVS,        // Chinese Standard
    WP_VIDEO_STREAM_TYPE_AVS2,       // New Chinese Standard
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

S8BIT Wrapper_Player_Initialise(void);
S8BIT Wrapper_Player_Create(jni_asplayer_init_params params, jni_asplayer_handle *handle);
S8BIT Wrapper_Player_Destroy(jni_asplayer_handle handle);
S8BIT Wrapper_Player_SetParams(jni_asplayer_handle handle, jni_asplayer_parameter type, void *parameter);
S8BIT Wrapper_Player_SetVideoParams(jni_asplayer_handle handle, jni_asplayer_video_params *params);
S8BIT Wrapper_Player_SetAudioParams(jni_asplayer_handle handle, jni_asplayer_audio_params *params);
S8BIT Wrapper_Player_GetVideoInfo(jni_asplayer_handle handle, jni_asplayer_video_info *pInfo);
S8BIT Wrapper_Player_SetAudioStereoMode(jni_asplayer_handle handle, jni_asplayer_audio_stereo_mode Mode);
S8BIT Wrapper_Player_GetAudioStereoMode(jni_asplayer_handle handle, jni_asplayer_audio_stereo_mode *pMode);
S8BIT Wrapper_Player_SetSurface(jni_asplayer_handle handle);
S8BIT Wrapper_Player_RegisterEventCallBack(jni_asplayer_handle handle, event_callback cb, void* userData);
S8BIT Wrapper_Player_ShowVideo(jni_asplayer_handle handle);
S8BIT Wrapper_Player_HideVideo(jni_asplayer_handle handle);
S8BIT Wrapper_Player_StartVideoDecoding(jni_asplayer_handle handle);
S8BIT Wrapper_Player_StopVideoDecoding(jni_asplayer_handle handle);
S8BIT Wrapper_Player_StartAudioDecoding(jni_asplayer_handle handle);
S8BIT Wrapper_Player_StopAudioDecoding(jni_asplayer_handle handle);
S8BIT Wrapper_Player_SetADParams(jni_asplayer_handle handle, jni_asplayer_audio_params *pParams);
S8BIT Wrapper_Player_EnableADMix(jni_asplayer_handle handle);
S8BIT Wrapper_Player_DisableADMix(jni_asplayer_handle handle);
S8BIT Wrapper_Player_SetADMixLevel(jni_asplayer_handle handle, S32BIT mix_level);
S8BIT Wrapper_Player_GetADMixLevel(jni_asplayer_handle handle, S32BIT *mix_level);
S8BIT Wrapper_Player_SetAudioMute(jni_asplayer_handle handle, BOOLEAN audio_mute);
S8BIT Wrapper_Player_SetPIPMode(jni_asplayer_handle handle, jni_asplayer_pip_mode mode);
S8BIT Wrapper_Player_SetWorkMode(jni_asplayer_handle handle, jni_asplayer_work_mode mode);
S8BIT Wrapper_Player_ResetWorkMode(jni_asplayer_handle handle);

int Wrapper_Player_GetAVFilterId(BOOLEAN isAudio, int pid, int vidoeStreamType, int audioStreamType);
int Wrapper_Player_GetAvSyncHwId();

#ifdef __cplusplus
}
#endif

#endif


