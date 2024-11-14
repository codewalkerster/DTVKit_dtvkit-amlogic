/*******************************************************************************
 * Copyright (c) 2018 The DTVKit Open Software Foundation Ltd (www.dtvkit.org)
 *
 * This file is part of a DTVKit Software Component
 * You are permitted to copy, modify or distribute this file subject to the terms
 * of the DTVKit 1.0 Licence which can be found in licence.txt or at www.dtvkit.org
 *
 * THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
 * EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
 *
 * If you or your organisation is not a member of DTVKit then you have access
 * to this source code outside of the terms of the licence agreement
 * and you are expected to delete this and any associated files immediately.
 * Further information on DTVKit, membership and terms can be found at www.dtvkit.org
 *******************************************************************************/
/**
 * @brief   Set Top Box - Hardware Layer, AV Control and decoding
 * @file    stbhwav_amlmp.c
 * @date    September 2020
 */

/*---includes for this file--------------------------------------------------*/
/* compiler library header files */
#include <stdio.h>
#include <string.h>
#include <cutils/properties.h>
#include "dtv_log.h"
#define TAG  "STBHWAV_AMLMP"
#define loff_t off_t

/* STB header files */
#include "techtype.h"

#include "stbhwdef.h"
//#define DEBUG_FUNCTIONS
#include "dbgfuncs.h"
#include "stbhwos.h"
#include "stbhwmem.h"
#include "stbhwav.h"
#include "stbhwosd.h"
#include "stbpvrpr.h"
#include "internal.h"
#include "stbhwdef.h"
#include "stb_utils.h"
#include "stbhwdemux_usb.h"
#include "afd_ctrl.h"

/* third party header files */
/*#define  AV_AUDIO_STEREO        AV_AUDIO_STEREO_TSP*/
/*#define  AV_AUDIO_RIGHT         AV_AUDIO_RIGHT_TSP*/
/*#define  AV_AUDIO_LEFT          AV_AUDIO_LEFT_TSP*/
/*#define  AV_AUDIO_SWAP          AV_AUDIO_SWAP_TSP*/
/*#define  AV_AUDIO_LRMIX         AV_AUDIO_LRMIX_TSP*/
/*#define  AV_VIDEO_CODEC_AUTO    AV_VIDEO_CODEC_AUTO_TSP*/
/*#define  AV_VIDEO_CODEC_H264    AV_VIDEO_CODEC_H264_TSP*/
/*#define  AV_VIDEO_CODEC_H265    AV_VIDEO_CODEC_H265_TSP*/
/*#define  AV_VIDEO_CODEC_MPEG1   AV_VIDEO_CODEC_MPEG1_TSP*/
/*#define  AV_VIDEO_CODEC_MPEG2   AV_VIDEO_CODEC_MPEG2_TSP*/
/*#define  AV_VIDEO_CODEC_VP9     AV_VIDEO_CODEC_VP9_TSP*/
/*#define  AV_AUDIO_CODEC_AUTO   AV_AUDIO_CODEC_AUTO_TSP*/
/*#define  AV_AUDIO_CODEC_MP2    AV_AUDIO_CODEC_MP2_TSP*/
/*#define  AV_AUDIO_CODEC_MP3    AV_AUDIO_CODEC_MP3_TSP*/
/*#define  AV_AUDIO_CODEC_AC3    AV_AUDIO_CODEC_AC3_TSP*/
/*#define  AV_AUDIO_CODEC_EAC3   AV_AUDIO_CODEC_EAC3_TSP*/
/*#define  AV_AUDIO_CODEC_DTS    AV_AUDIO_CODEC_DTS_TSP*/
/*#define  AV_AUDIO_CODEC_AAC    AV_AUDIO_CODEC_AAC_TSP*/
/*#define  AV_AUDIO_CODEC_LATM   AV_AUDIO_CODEC_LATM_TSP*/

#ifdef SUPPORT_CAS
#include "ca_glue.h"
#endif

#include <Aml_MP/Aml_MP.h>
/*#include <AmTsPlayer.h>*/
#include "systemcontrol.h"
#include <cutils/properties.h>


/*#undef  AV_AUDIO_RIGHT*/
/*#undef  AV_AUDIO_LEFT*/
/*#undef  AV_VIDEO_CODEC_AUTO*/
/*#undef  AV_VIDEO_CODEC_H264*/
/*#undef  AV_VIDEO_CODEC_H265*/
/*#undef  AV_VIDEO_CODEC_MPEG1*/
/*#undef  AV_VIDEO_CODEC_MPEG2*/
/*#undef  AV_VIDEO_CODEC_VP9*/
/*#undef  AV_AUDIO_CODEC_AUTO*/
/*#undef  AV_AUDIO_CODEC_MP2*/
/*#undef  AV_AUDIO_CODEC_MP3*/
/*#undef  AV_AUDIO_CODEC_AC3*/
/*#undef  AV_AUDIO_CODEC_EAC3*/
/*#undef  AV_AUDIO_CODEC_DTS*/
/*#undef  AV_AUDIO_CODEC_AAC*/
/*#undef  AV_AUDIO_CODEC_LATM*/

/*---macro definitions for this file-----------------------------------------*/
#define AV_DEBUG
#define VIDEO_DEBUG
#define AUDIO_DEBUG


#ifdef AV_DEBUG
   #define AV_DBG(x,...) DTV_LOG(ANDROID_LOG_INFO, TAG, "%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )
#else
   #define AV_DBG(x,...)
#endif

#ifdef VIDEO_DEBUG
   #define VID_DBG(x,...) DTV_LOG(ANDROID_LOG_INFO, TAG, "%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )
#else
   #define VID_DBG(x,...)
#endif

#ifdef AUDIO_DEBUG
   #define AUD_DBG(x,...) DTV_LOG(ANDROID_LOG_INFO, TAG, "%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )
#else
   #define AUD_DBG(x,...)
#endif

#define ERR_DBG(x,...) DTV_LOG(ANDROID_LOG_INFO, TAG, "%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )

#define RET_DBG(_fun_, _ret_)

#define INVALID_PID 0x1fff

#ifdef  INVALID_RES_ID
#undef  INVALID_RES_ID
#endif
#define INVALID_RES_ID   255

#define MIN_AV_SPEED    -600
#define MAX_AV_SPEED     600
#define MAX_PLAYER_NUM     32

#define INVALID_PLAYER_HANDLE -1
#define IS_INVALID_PLAYER_HANDLE(_path_)    ((av_paths_status[_path_].player_handle) == AML_MP_INVALID_HANDLE)

#define IS_CACHED(_m_) ((_m_) & DECODING_MODE_CACHE_ONLY)
#define IS_AD_ENABLE(_m_) ((_m_) & DECODING_AD_ENABLE)
#define IS_AUDIO_DISABLE(_m_) ((_m_) & DECODING_AUDIO_DISABLE)

/*---constant definitions for this file--------------------------------------*/

/*---local typedef structs for this file-------------------------------------*/
typedef enum
{
    DECODER_STATE_STOPPED,
    DECODER_STATE_STARTED,
} E_DECODER_STATE;
#define DECODER_STATE_MASK 1

typedef enum
{
    AUDIO_DECODER,
    VIDEO_DECODER,
    AD_DECODER,
} E_DECODER_INDEX;

typedef struct
{
   E_STB_AV_ASPECT_RATIO screen_aspect_ratio;
   U16BIT screen_width;
   U16BIT screen_height;
} S_DISPLAY_INFO;

typedef struct
{
    uint8_t         :6;
    uint8_t  af_flag:1;
    uint8_t         :1;
    uint8_t  af     :4;
    uint8_t         :4;
    uint16_t reserved;
    uint32_t pts;
} USERDATA_AFD_t;

typedef struct
{
   U8BIT decoder;
   E_STB_AV_DECODE_SOURCE source;
   U8BIT demux;
   pthread_rwlock_t lock;
   AML_MP_PLAYER        player_handle;
   Aml_MP_CodecID    video_format;
   Aml_MP_CodecID    audio_format;
   Aml_MP_CodecID    ad_format;
   Aml_MP_AudioBalance  audio_mode;
   BOOLEAN              injecting;

   BOOLEAN              iframe_shown;
   U8BIT*               iframe_data;
   U32BIT               iframe_data_size;
   E_STB_AV_VIDEO_CODEC iframe_codec;

   void (*callback)(S_STB_AV_VIDEO_INFO *, void *, int);
   void *user_data;

   float                volume;
   U8BIT                mute;
   U8BIT*               sample_data;
   U32BIT               sample_data_size;
   U32BIT               loop_count;
   BOOLEAN              audio_descriptor_active;
   U16BIT               video_pid;
   U16BIT               audio_pid;
   U16BIT               pcr_pid;
   U16BIT               ad_pid;
   U8BIT                video_decoder;
   U8BIT                audio_decoder;
   S_DISPLAY_INFO       display_info;

   U32BIT               param;

   E_STB_DECODING_MODE decoding_mode;

   U32BIT               video_out_control;
   U32BIT               audio_out_control;
#ifdef SUPPORT_CAS
   E_STB_DRM_TYPE       drm_mode;
   AML_MP_SECMEM        secmem_handle;
#endif
    U16BIT              audio_presentation_id;
} AV_PATH_STATUS;

typedef struct
{
   U8BIT  decoder;
   BOOLEAN decoder_id_valid;
   U32BIT decoder_id;
   BOOLEAN sync_id_valid;
   U32BIT sync_id;
} S_VIDEO_DECODER_PRIV_DATA;

#ifdef RDK_COMPILE
typedef struct
{
   U16BIT  x;
   U16BIT  y;
   U16BIT  w;
   U16BIT  h;
} S_WINDOW_RECT;

// This array can be regarded as a "path" - "video window rectangle" map
static S_WINDOW_RECT window_rect_array[32];
#endif

/*---local (static) variable declarations for this file----------------------*/
static AV_PATH_STATUS *av_paths_status = NULL;
static void** video_surface = NULL;
static BOOLEAN av_start_flag = FALSE;
static U8BIT num_paths = 0;
static BOOLEAN video_blank_lock = FALSE;
static BOOLEAN audio_mute_lock = FALSE;
static int mlistener_dtv = 0;

struct SysClientWrapper_t * pSysClientWrapper;

typedef enum
{
   HW_AM_VOUT_FORMAT_UNKNOWN,              /**< 未知的模式*/
   HW_AM_VOUT_FORMAT_576CVBS,              /**< PAL制CVBS输出*/
   HW_AM_VOUT_FORMAT_480CVBS,              /**< NTSC制CVBS输出*/
   HW_AM_VOUT_FORMAT_576I,                 /**< 576I*/
   HW_AM_VOUT_FORMAT_576P,                 /**< 576P*/
   HW_AM_VOUT_FORMAT_480I,                 /**< 480I*/
   HW_AM_VOUT_FORMAT_480P,                 /**< 480P*/
   HW_AM_VOUT_FORMAT_720P,                 /**< 720P*/
   HW_AM_VOUT_FORMAT_1080I,                /**< 1080I*/
   HW_AM_VOUT_FORMAT_1080P,                /**< 1080P*/
} HW_AM_VOUT_Format_t;

typedef struct
{
   HW_AM_VOUT_Format_t am_format;
   E_STB_AV_VIDEO_FORMAT format;
} S_VIDEO_MODE;

 typedef enum {
    AMP_TSPLAYER_KEY_SPDIF_MODE_NONE  = 0,
    AMP_TSPLAYER_KEY_SPDIF_MODE_NEVER = 1,
    AMP_TSPLAYER_KEY_SPDIF_MODE_ONCE  = 2,
} amp_spdif_mode;

static S_VIDEO_MODE video_modes[] =
{
   {HW_AM_VOUT_FORMAT_576I, VIDEO_FORMAT_576IHD},
   {HW_AM_VOUT_FORMAT_576P, VIDEO_FORMAT_576PHD},
   {HW_AM_VOUT_FORMAT_720P, VIDEO_FORMAT_720P50HD},
   {HW_AM_VOUT_FORMAT_1080I, VIDEO_FORMAT_1080IHD},
   {HW_AM_VOUT_FORMAT_1080P, VIDEO_FORMAT_1080P50HD}
};

typedef enum
{
    VIDEO_SIGNAL_STABLE = 0,
    VIDEO_SIGNAL_LOSS
} VIDEO_SIGNAL_TYPE;

static VIDEO_SIGNAL_TYPE FLAGS = 0;
static U8BIT dtv_path = 0;
static int test_screen = 0;

/*---local function prototypes for this file---------------------------------*/
static void AVEventHandler(void *user_data, Aml_MP_PlayerEventType eventType, int64_t param);
static int AV_CreateTsPlayer_l(U8BIT path, Aml_MP_InputSourceType source_type, int32_t dmx_dev_id, int32_t event_mask);
static int AV_ReleaseTsPlayer_l(U8BIT path);
static int AV_GetPlayerHandleByPath_l(U8BIT video, U8BIT audio, AML_MP_PLAYER* player_handle, BOOLEAN recreat_handle); //return AML_MP_PLAYER or am_tsplayer_handle
static int AV_GetPathByPlayerHandle(AML_MP_PLAYER *player_handle);
static int AV_StartAudioDecode_l(AML_MP_PLAYER player_handle, U16BIT a_pid, Aml_MP_CodecID format, Aml_MP_AudioBalance audio_mode, U8BIT vol, BOOLEAN mute, int audioPresentationId);
static int AV_StartVideoDecode_l(AML_MP_PLAYER player_handle, U16BIT v_pid, U16BIT pcr_pid, Aml_MP_CodecID format, Aml_MP_AVSyncSource mode);
static E_DECODER_STATE AV_GetDecoderState_l(AML_MP_PLAYER player_handle, E_DECODER_INDEX index);
static Aml_MP_StreamType toStreamType(E_DECODER_INDEX index);
static E_DECODER_STATE toDecoderState(AML_MP_DecodingState decodingState);
static Aml_MP_StreamType toMPStreamType(E_HBBTV_PTS_SOURCE_TYPE streamType);


//for PVR
static int AV_SetAudioDecode_l(AML_MP_PLAYER player_handle, Aml_MP_AudioBalance audio_mode, U8BIT vol, BOOLEAN mute);
static E_STB_AV_VIDEO_CODEC toVideoCodec(Aml_MP_CodecID codec);
static E_STB_AV_AUDIO_CODEC toAudioCodec(Aml_MP_CodecID codec);
BOOLEAN STB_AVAcquirePath(U8BIT video_decoder, U8BIT audio_decoder);
BOOLEAN STB_AVReleasePath(U8BIT video_decoder, U8BIT audio_decoder);
U8BIT STB_AVGetPath(U8BIT video_decoder, U8BIT audio_decoder);

//for PVR end

static int AV_SetAudioVolumeAndMute_l(AML_MP_PLAYER player_handle, U8BIT vol, BOOLEAN mute);
static int AV_SetAudioVolume_l(AML_MP_PLAYER player_handle, U8BIT vol);
static int AV_SetAudioMute_l(AML_MP_PLAYER player_handle, BOOLEAN mute);
static BOOLEAN AV_UpdateAudioOutControl_l(U8BIT path, E_AV_OUT_CONTROL_FLAG flag, BOOLEAN mute);
static int AV_SetVideoColor(U8BIT av_path, BOOLEAN is_black, BOOLEAN mode);

/**
 * @brief   add auto detection function
 */
static void AV_EventCallback(int color);
static void AV_GetInstance(void);

/*---global function definitions----------------------------------------------*/

/**
 * @brief   Initialises the AV components
 * @param   audio_paths The number of audio paths
 * @param   video_paths The number of video paths
 */
void STB_AVInitialise(U8BIT audio_paths, U8BIT video_paths)
{
   U16BIT av_path;
   int ret;

   FUNCTION_START(STB_AVInitialise);

   if (av_paths_status == NULL)
   {
      /* add auto detection function */
      if (!mlistener_dtv)
      {
          AV_GetInstance();
          mlistener_dtv = 1;
      }

      num_paths = video_paths;
      AV_DBG("video paths=%u demux = %d", num_paths, aml_hw_cfg.demux + 5);

      av_paths_status = (AV_PATH_STATUS*) STB_MEMGetSysRAM(sizeof(AV_PATH_STATUS) * num_paths);
      video_surface = (void**) STB_MEMGetSysRAM(sizeof(void*) * num_paths);
      /* AV paths */
      if (av_paths_status != NULL)
      {
         memset(av_paths_status, 0, num_paths * sizeof(AV_PATH_STATUS));
         for (av_path = 0; av_path < num_paths; av_path++)
         {
            av_paths_status[av_path].decoder            = av_path;
            av_paths_status[av_path].source             = AV_DEMUX;
            av_paths_status[av_path].injecting          = FALSE;
            av_paths_status[av_path].iframe_shown       = FALSE;
            av_paths_status[av_path].iframe_codec       = AV_VIDEO_CODEC_AUTO;
            av_paths_status[av_path].iframe_data        = NULL;
            av_paths_status[av_path].audio_descriptor_active = FALSE;
            av_paths_status[av_path].audio_pid          = INVALID_PID;
            av_paths_status[av_path].video_pid          = INVALID_PID;
            av_paths_status[av_path].ad_pid             = INVALID_PID;
            av_paths_status[av_path].pcr_pid            = INVALID_PID;
            av_paths_status[av_path].player_handle      = AML_MP_INVALID_HANDLE;
            av_paths_status[av_path].volume             = 100;
            av_paths_status[av_path].mute               = FALSE;
            av_paths_status[av_path].audio_mode         = AML_MP_AUDIO_BALANCE_STEREO;
            av_paths_status[av_path].video_decoder      = INVALID_RES_ID;
            av_paths_status[av_path].audio_decoder      = INVALID_RES_ID;
            av_paths_status[av_path].video_out_control     = 0;
            av_paths_status[av_path].audio_out_control     = 0;

            av_paths_status[av_path].display_info.screen_width = 1920;
            av_paths_status[av_path].display_info.screen_height = 1080;
            av_paths_status[av_path].display_info.screen_aspect_ratio = ASPECT_RATIO_16_9;
#ifdef SUPPORT_CAS
            av_paths_status[av_path].drm_mode           = DRM_NONE;
            av_paths_status[av_path].secmem_handle      = NULL;
#endif
            av_paths_status[av_path].audio_presentation_id = INVALID_PID;
            pthread_rwlock_init(&av_paths_status[av_path].lock, NULL);
         }
         S_DISPLAY_INFO display_info;

         display_info.screen_width          = 1920;
         display_info.screen_height         = 1080;
         display_info.screen_aspect_ratio   = ASPECT_RATIO_16_9;
         STB_OSDResize(FALSE, display_info.screen_width, display_info.screen_height, 0, 0);
         STB_OSSendEvent(FALSE, HW_EV_CLASS_HDMI, HW_EV_TYPE_HDMI_CONNECT, NULL, 0);

         const char* version = NULL;
         Aml_MP_GetVersion(&version);
         AV_DBG("Aml_MP version:%s", version);
      }
      else
      {
         ERR_DBG("Failed to allocate memory for 'av_status'");
      }
   }
   else
   {
      ERR_DBG("Already initialised");
   }
   FUNCTION_FINISH(STB_AVInitialise);
}

/**
 * @brief   Sets the aspect ratio of the connected television
 * @param   path The video path to be set
 * @param   ratio The aspect ratio of the tv
 * @param   format The signal format of the tv
 */
void STB_AVSetTVType(U8BIT path, E_STB_AV_ASPECT_RATIO ratio, E_STB_AV_VIDEO_FORMAT format)
{
   FUNCTION_START(STB_AVSetTVType);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(ratio);
   USE_UNWANTED_PARAM(format);
   FUNCTION_FINISH(STB_AVSetTVType);
}

/**
 * @brief   Set the aspect ratio setting of the tv
 * @param   path the video path
 * @param   ratio The requested aspect ratio setting of the tv
 * @return  Valid aspect ratio setting, only different from the parameter if that was invalid
 */
E_STB_AV_ASPECT_RATIO STB_TVSetAspectRatio(E_STB_AV_ASPECT_RATIO ratio)
{
   FUNCTION_START(STB_TVSetAspectRatio);
   USE_UNWANTED_PARAM(ratio);
   FUNCTION_FINISH(STB_TVSetAspectRatio);

   return ASPECT_RATIO_16_9;;
}

static void invokeCallback(AV_PATH_STATUS *status, S_STB_AV_VIDEO_INFO *info)
{
   if (status != NULL && status->callback != NULL)
   {
      if (info != NULL)
      {
         if (status->display_info.screen_width != 0
            && status->display_info.screen_height != 0)
         {
            info->flags |= VIDEO_INFO_SCREEN_RESOLUTION | VIDEO_INFO_DISPLAY_ASPECT_RATIO;

            info->screen_width = status->display_info.screen_width;
            info->screen_height = status->display_info.screen_height;
            info->display_aspect_ratio = status->display_info.screen_aspect_ratio;
         }
      }

      status->callback(info, status->user_data, status->video_decoder);
   }
}

/**
 * @brief   Register callback for updated video information
 * @param   path av path
 * @param   callback - the callback to call when video information is updated
 * @param   vtc_user_data - user data to pass to the callback
 */
void STB_AVSetVideoCallback(U8BIT path, void (*callback)(S_STB_AV_VIDEO_INFO *, void *, int), void *user_data)
{
   S_STB_AV_VIDEO_INFO info;

   FUNCTION_START(STB_AVSetVideoCallback);

   U8BIT av_path = path;
   //av_path = STB_AVGetPath(path, INVALID_RES_ID);
   //if (av_path == INVALID_RES_ID) {
      //VID_DBG("av_path(%u) res invalid", av_path);
      //return;
   //}
   VID_DBG("av_path(%u) res set  user_data  [%p]", av_path, user_data);
    if (av_path < num_paths) {
        av_paths_status[av_path].callback = callback;
        av_paths_status[av_path].user_data = user_data;
    }
/*
   if ((callback != NULL) && (av_paths_status[av_path].display_info.screen_width != 0) && (av_paths_status[av_path].display_info.screen_height != 0))
   {
      info.flags = (E_STB_AV_VIDEO_INFO_TYPE)(VIDEO_INFO_SCREEN_RESOLUTION | VIDEO_INFO_DISPLAY_ASPECT_RATIO);
      info.screen_width = av_paths_status[av_path].display_info.screen_width;
      info.screen_height = av_paths_status[av_path].display_info.screen_height;
      info.display_aspect_ratio = av_paths_status[av_path].display_info.screen_aspect_ratio;

      av_paths_status[path].callback(&info, av_paths_status[av_path].user_data, av_paths_status[av_path].video_decoder);
   }
*/
   FUNCTION_FINISH(STB_AVSetVideoCallback);
}

/**
 * @brief   acquire av path used video and audio codec
 * @param   audio_decoder audio decoder being used for play
 * @param   video_decoder video decoder being used for play
 * @param   mode playback startup mode
 */
BOOLEAN STB_AVAcquirePath(U8BIT video_decoder, U8BIT audio_decoder)
{
   FUNCTION_START(STB_AVAcquirePath);
   int i;
   BOOLEAN acquired = FALSE;

    for (i = 0; i < num_paths; i++)
    {
        if (av_paths_status[i].video_decoder == INVALID_RES_ID
            && av_paths_status[i].audio_decoder == INVALID_RES_ID)
        {
            av_paths_status[i].video_decoder = video_decoder;
            av_paths_status[i].audio_decoder = audio_decoder;
            acquired = TRUE;
            break;
        }
    }

   FUNCTION_FINISH(STB_AVAcquirePath);
   return acquired;
}

void STB_AVSetVideoBlankLock(BOOLEAN enable)
{
   video_blank_lock = enable;
}

void STB_AVSetAudioMuteLock(BOOLEAN enable)
{
   audio_mute_lock = enable;
}

BOOLEAN STB_AVReleasePath(U8BIT video_decoder, U8BIT audio_decoder)
{
   int i;
   BOOLEAN released = FALSE;

   FUNCTION_START(STB_AVReleasePath);

   for (i = 0; i < num_paths; i++)
   {
      if (av_paths_status[i].video_decoder == video_decoder
         || av_paths_status[i].audio_decoder == audio_decoder)
      {
         av_paths_status[i].video_decoder = INVALID_RES_ID;
         av_paths_status[i].audio_decoder = INVALID_RES_ID;
         av_paths_status[i].decoding_mode = 0;
         av_paths_status[i].volume = 100;
         av_paths_status[i].mute = FALSE;

         video_surface[i] = NULL;

         released = TRUE;
         break;
      }
   }

   FUNCTION_FINISH(STB_AVReleasePath);

   return released;
}

/**
 * @brief   get av path used video and audio codec
 * @param   audio_decoder audio decoder being used for play
 * @param   video_decoder video decoder being used for play
 * @param   mode playback startup mode
 */
U8BIT STB_AVGetPath(U8BIT video_decoder, U8BIT audio_decoder)
{
   FUNCTION_START(STB_AVGetPath);

    U8BIT i;
    U8BIT path = INVALID_RES_ID;

    for (i = 0; i < num_paths && path == INVALID_RES_ID; i++)
    {
       if ((video_decoder != INVALID_RES_ID && av_paths_status[i].video_decoder == video_decoder)
          || (audio_decoder != INVALID_RES_ID && av_paths_status[i].audio_decoder == audio_decoder))
       {
          path = i;
       }
    }

   FUNCTION_FINISH(STB_AVGetPath);
   return path;
}

/**
 * @brief   Apply video transformation
 * @param   input - input video rectangle
 * @param   output - output video rectangle
 */
void STB_AVApplyVideoTransformation(U8BIT path, S_RECTANGLE* src, S_RECTANGLE* dest)
{
   FUNCTION_START(STB_AVApplyVideoTransformation);

   USE_UNWANTED_PARAM(src);
   USE_UNWANTED_PARAM(path);

   if ((src != NULL) && (dest != NULL))
   {
      /*used as a Quad, not the literal meaning*/
      S_QVALUE_EX crop = {.path = path, .values = {src->top, src->left, src->top, src->left}};
      S_QVALUE_EX rect = {.path = path, .values = {dest->left, dest->top, dest->width, dest->height}};

      VID_DBG("video(%d): (%u, %u), (%u x %u) out: (%u, %u), (%u x %u)",
         path,
         src->left, src->top, src->width, src->height,
         dest->left, dest->top, dest->width, dest->height);


      STB_OSSendEvent(FALSE, HW_EV_CLASS_PRIVATE, HW_EV_TYPE_VIDEO_CROPPING_CHANGED, &crop, sizeof(S_QVALUE_EX));
      STB_OSSendEvent(FALSE, HW_EV_CLASS_PRIVATE, HW_EV_TYPE_VIDEO_RECTANGLE_CHANGED, &rect, sizeof(S_QVALUE_EX));
   }

   FUNCTION_FINISH(STB_AVApplyVideoTransformation);
}

/**
 * @brief   Blanks or unblanks the video display
 * @param   window VT id
 * @param   blank TRUE to blank, FALSE to unblank
 * @param   force_black  TRUE to force black, else with user setting
*/
void STB_AVSetWindowColor(U8BIT window, BOOLEAN blank, BOOLEAN force_black, BOOLEAN force_all, U8BIT path, BOOLEAN mode)
{
    //this function is only supported for CVTE/CTV bluescreen feature
    BOOLEAN pip_enabled = STB_Is_PIP_Enabled();
    BOOLEAN fcc_enabled = STB_Is_FCC_Enabled();

    if (video_blank_lock || pip_enabled || fcc_enabled)
    {
        VID_DBG("Video blank locked or pip/fcc enable, can not change");
        return;
    }

    VID_DBG("window:%d, force_all:%d blank:%d force_black:%d",
        window, force_all, blank, force_black);

#ifndef RDK_COMPILE
    if (window > 0)
    {
        U8BIT win = window;
        U8BIT win_max = window;

        if (force_all)
        {
            win = 1;
            win_max = 2;
        }

        for (; win <= win_max; win++)
        {
            if (blank == TRUE)
            {
                SC_setVideoColor(win, force_black? VIDEO_LAYER_COLOR_BLACK : SC_getScreenColorSetting());
            }
            else
            {
                SC_setVideoColor(win, VIDEO_LAYER_COLOR_MAX);
            }
        }
    }
#endif
}

/**
 * @brief   control video display
 * @param   mute: av hidden or show
 * @param   mute_option: auto or force set color
 * @param   av_out_flag
*/
void STB_AVMuteControl(U8BIT window, U8BIT path , SET_AV_BLANK mute, AV_MUTE_OPTION mute_option, E_AV_OUT_CONTROL_FLAG av_out_flag)
{
    BOOLEAN get_player_handle = FALSE;
    int system_color = 0;

#ifndef RDK_COMPILE
    system_color = SC_getScreenColorSetting();
#endif

    VID_DBG("(av_hidden=1): %d, system_color(blue=1): %d, av_out_flag=%d, video_pid = 0x%x",
        mute, system_color, av_out_flag, av_paths_status[path].video_pid);

    if (mute == AV_HIDDEN)
    {
        if ((system_color == VIDEO_LAYER_COLOR_BLACK) && (av_paths_status[path].video_pid != INVALID_PID))
        {
            get_player_handle = STB_AVBlankVideo(path, av_out_flag, TRUE);
            VID_DBG("mute hidden (handle = %d)", get_player_handle);
            if (get_player_handle == FALSE)
            {
                AV_SetVideoColor(path, TRUE, TRUE);
                STB_AVSetWindowColor(window, TRUE, TRUE, FALSE, path, TRUE);
                VID_DBG("mute_option(set black color)");
            }
        }
        else
        {
            VID_DBG("mute_option(auto=0): %d", mute_option);
            AV_SetVideoColor(path, TRUE, TRUE);
            if (mute_option == AUTO)
            {
                STB_AVSetWindowColor(window, TRUE, FALSE, FALSE, path, TRUE);
            }
            else if(mute_option == OVERRIDE_BY_BLACK)
            {
                STB_AVSetWindowColor(window, TRUE, TRUE, FALSE, path, TRUE);
            }
        }
    }
    else if (mute == AV_SHOW)
    {
        if ((system_color == VIDEO_LAYER_COLOR_BLACK) && (av_paths_status[path].video_pid != INVALID_PID))
        {
            get_player_handle = STB_AVBlankVideo(path, av_out_flag, FALSE);
            VID_DBG("black unmute (handle = %d)", get_player_handle);
            if (get_player_handle == FALSE)
            {
                VID_DBG("black unmute ");
                STB_AVSetWindowColor(window, FALSE, FALSE, FALSE, path, TRUE);
            }
        }
        else
        {
            VID_DBG("blue unmute ");
            STB_AVSetWindowColor(window, FALSE, FALSE, FALSE, path, TRUE);
        }
    }
}


/**
 * @brief   Get Static Frame Enable or not
 */
BOOLEAN STB_AVGetStaticFrameEnable()
{
    BOOLEAN ret = FALSE;
    BOOLEAN pip_enabled = STB_Is_PIP_Enabled();
    BOOLEAN fcc_enabled = STB_Is_FCC_Enabled();

    FUNCTION_START(STB_AVGetStaticFrameEnable);

    if (pip_enabled || fcc_enabled)
    {
        VID_DBG("Disable static frame function when pip/fcc enable");
        return ret;
    }

#ifndef RDK_COMPILE
    ret = SC_getStaticFrameEnable();
#endif

    FUNCTION_FINISH(STB_AVGetStaticFrameEnable);

    return ret;
}

/**
 * @brief   Gets the Transition Color config
 * @return  TRUE transition color is black, FALSE otherwise
 */
BOOLEAN STB_AVGetIsBlackTransitionColor(void)
{
    static char buf[PROPERTY_VALUE_MAX] = {0};

    int len = property_get("vendor.isblack.transition.color", buf, "true");

    if (len <= 0)
    {
        VID_DBG("Failed to get transition color property, using default value.");
        return true;
    }

    VID_DBG("transition_color[%s]", buf);

    if (!strncmp(buf, "false", 5))
    {
        return false;
    }
    return true;
}

/**
 * @brief   Clear Full Screen
 */
void STB_ClearFullScreen(void)
{
    FUNCTION_START(STB_ClearFullScreen);
#ifndef RDK_COMPILE
    SC_setDisplayMode(test_screen,1);
#endif
    FUNCTION_FINISH(STB_ClearFullScreen);
}

/**
 * @brief   no sig need full screen display
 */
BOOLEAN STB_SetFullScreen(void)
{
    int ret = 0;
    FUNCTION_START(STB_SetFullScreen);
#ifndef RDK_COMPILE
    test_screen = SC_getDisplayMode();

    /*5:VPP_DISPLAY_MODE_FULL*/
    if (5 != test_screen)
    {
        /*no sig need full screen,1:save database*/
        SC_setDisplayMode(5,1);
        ret = 1;
    }
#endif
    FUNCTION_FINISH(STB_SetFullScreen);
    return ret;
}

/**
 * @brief auto detect
 */
static void AV_GetInstance()
{
    FUNCTION_START(AV_GetInstance);
#ifndef RDK_COMPILE
    pSysClientWrapper = SC_getInstance();
    SC_setSysClientCallback(AV_EventCallback);
#endif
    FUNCTION_FINISH(AV_GetInstance);
}
#define INVALID_A_V_PID 0x1FFF
/**
 * @brief set color frame
 */
static void AV_EventCallback(int color)
{
    FUNCTION_START(AV_EventCallback);
    VID_DBG("FLAGS=%d",FLAGS);
    if ((FLAGS == VIDEO_SIGNAL_LOSS) || (av_paths_status[0].video_pid == INVALID_A_V_PID && av_paths_status[0].audio_pid != INVALID_A_V_PID))
    {
        VID_DBG("color=%d",color);
#ifndef RDK_COMPILE
        if (color)
        {
            SC_setVideoColor(1, 1);//blue
        }
        else
        {
            SC_setVideoColor(1, 0);//black
        }
#endif
    }
    FUNCTION_FINISH(AV_EventCallback);
}

/**
 * @brief   Blanks or unblanks the video display
 * @param   path the video path to be configured
 * @param   blank TRUE to blank, FALSE to unblank
 */
BOOLEAN STB_AVBlankVideo(U8BIT path, E_AV_OUT_CONTROL_FLAG flag, BOOLEAN av_blank)
{
    int ret;
    AML_MP_PLAYER player_handle;
    U8BIT av_path = INVALID_RES_ID;
    BOOLEAN blank = FALSE;

    FUNCTION_START(STB_AVBlankVideo);

    if (video_blank_lock || path >= num_paths)
    {
        VID_DBG("Video blank locked or path is invalid, can not change(%d)", path);
        return FALSE;
    }
    VID_DBG("path[%u], blank[%d], av_out_flag[%x], video_out_control[%x]", path, av_blank, flag, av_paths_status[path].video_out_control);

    if (av_blank) {
        av_paths_status[path].video_out_control |= (1<<flag);
    } else {
        av_paths_status[path].video_out_control &= (~(1<<flag));
    }

    if (av_paths_status[path].video_out_control) {
        blank = TRUE;
    } else {
        blank = FALSE;
    }

    pthread_rwlock_t* _l = STB_AVGetLockByPath(path);
    if (_l == NULL) {
        VID_DBG("Can't get lock, video decoder[%d]", path);
        return FALSE;
    }

    pthread_rwlock_rdlock(_l);
    ret = AV_GetPlayerHandleByPath_l(path, INVALID_RES_ID, &player_handle, FALSE);
    if (ret < 0) {
        VID_DBG("Cannot get player video decoder[%d]", path);
        pthread_rwlock_unlock(_l);
        return FALSE;
    }

    if (blank == TRUE) {
        ret = Aml_MP_Player_HideVideo(player_handle);
        if (ret < 0)
            AUD_DBG("Hide video failed, err:%d", ret);
        else
            AUD_DBG("Hide video");
    } else {
        ret = Aml_MP_Player_ShowVideo(player_handle);
        if (ret < 0)
            AUD_DBG("Show video failed, err:%d", ret);
        else
            AUD_DBG("Show video");
    }
    pthread_rwlock_unlock(_l);

    FUNCTION_FINISH(STB_AVBlankVideo);
    return TRUE;
}

U32BIT STB_AVGetBlankFlag(U8BIT path)
{
   return av_paths_status[path].video_out_control;
}

/**
 * @brief   clearlastframe or unclearlastframe the video display
 * @param   path video path
 * @param   clearlastframe TRUE to blank, FALSE to unblank
*/
void STB_AVClearLastFrame(U8BIT path,BOOLEAN clearlastframe)
{
    int ret;
    AML_MP_PLAYER player_handle;

    FUNCTION_START(STB_AVClearLastFrame);

    VID_DBG("path[%u], clearlastframe[%d]", path, clearlastframe);

    pthread_rwlock_t* _l = STB_AVGetLockByPath(path);
    if (_l == NULL) {
        VID_DBG("Can't get lock, video decoder[%d]", path);
        return;
    }

    pthread_rwlock_rdlock(_l);
    ret = AV_GetPlayerHandleByPath_l(path, INVALID_RES_ID, &player_handle, FALSE);
    if (ret < 0) {
        VID_DBG("Cannot get player handle video[%d]", path);
        pthread_rwlock_unlock(_l);
        return ;
    }

    ret = Aml_MP_Player_SetParameter(player_handle, AML_MP_PLAYER_PARAMETER_BLACK_OUT, &clearlastframe);
    if (ret < 0)
    {
        AUD_DBG("SetParameter clearlastframe failed, err:%d", ret);
    }
    pthread_rwlock_unlock(_l);

    FUNCTION_FINISH(STB_AVClearLastFrame);
}

/**
 * @brief   Routes a specified AV source to a specified AV output
 * @param   output The output to be connected
 * @param   source The source signal to be connected
 * @param   param parameter associated with the source
 */
void STB_AVSetAVOutputSource(E_STB_AV_OUTPUTS output, E_STB_AV_SOURCES source, U32BIT param)
{
   FUNCTION_START(STB_AVSetAVOutputSource);
   USE_UNWANTED_PARAM(output);
   USE_UNWANTED_PARAM(source);
   USE_UNWANTED_PARAM(param);
   FUNCTION_FINISH(STB_AVSetAVOutputSource);
}

/**
 * @brief   Turns on/off all AV outputs (e.g. for standby mode)
 */
void STB_AVSetAVOutput(BOOLEAN av_on)
{
   FUNCTION_START(STB_AVSetAVOutput);
   USE_UNWANTED_PARAM(av_on);
   FUNCTION_FINISH(STB_AVSetAVOutput);
}

/**
 * @brief   Sets the output channel of the RF Modulator
 * @param   chan the UHF channel number
 */
void STB_AVSetUhfModulatorChannel(U8BIT chan)
{
   FUNCTION_START(STB_AVSetUhfModulatorChannel);
   USE_UNWANTED_PARAM(chan);
   FUNCTION_FINISH(STB_AVSetUhfModulatorChannel);
}

/**
 * @brief   Gets the current RF modulator channel
 * @return  The RF frequency as a UHF channel number
 */
U8BIT STB_AVGetUhfModulatorChannel(void)
{
   FUNCTION_START(STB_AVGetUhfModulatorChannel);
   FUNCTION_FINISH(STB_AVGetUhfModulatorChannel);
   return 0;
}

/**
 * @brief   Sets the volume of the audio output
 * @param   path the audio path to be configured
 * @param   vol the audio volume (0-100%)(the value of vol is from 0 to 100)
 */
void STB_AVSetAudioVolume(U8BIT path, U8BIT vol)
{
   FUNCTION_START(STB_AVSetAudioVolume);

   int ret;
   AML_MP_PLAYER player_handle;
   U8BIT av_path = STB_AVGetPath(INVALID_RES_ID, path);

   AUD_DBG("set volume: %d:[-:%d] vol[%d]", av_path, path, vol);

   if (av_path != INVALID_RES_ID)
   {
      av_paths_status[av_path].volume = vol;
      av_paths_status[av_path].mute = (vol == 0) ? TRUE : FALSE;
   }

    BOOLEAN mute = AV_UpdateAudioOutControl_l(av_path, AVOUT_VOL, (vol == 0) ? TRUE : FALSE);

    pthread_rwlock_t* _l = STB_AVGetLockByPath(path);
    if (_l == NULL) {
        AUD_DBG("Can't get lock, audio decoder[%d]", path);
        return;
    }

    pthread_rwlock_rdlock(_l);
    ret = AV_GetPlayerHandleByPath_l(INVALID_RES_ID, path, &player_handle, FALSE);
    if (ret < 0) {
        AUD_DBG("Cannot get player audio path[%d](vol)", path);
        pthread_rwlock_unlock(_l);
        return;
    }

    AV_SetAudioMute_l(player_handle, mute);
    AV_SetAudioVolume_l(player_handle, vol);
    pthread_rwlock_unlock(_l);

   FUNCTION_FINISH(STB_AVSetAudioVolume);
}

/**
 * @brief   Gets the current volume of the audio output
 * @param   path The audio path to query
 * @return  audio volume (0-100%)
 */
U8BIT STB_AVGetAudioVolume(U8BIT path)
{
   int ret;
   float vol = 0;
   AML_MP_PLAYER player_handle;
   FUNCTION_START(STB_AVGetAudioVolume);

    pthread_rwlock_t* _l = STB_AVGetLockByPath(path);
    if (_l == NULL) {
        VID_DBG("Can't get lock, video decoder[%d]", path);
        return (path < num_paths) ? av_paths_status[path].volume : 0;
    }

    pthread_rwlock_rdlock(_l);
    ret = AV_GetPlayerHandleByPath_l(INVALID_RES_ID, path, &player_handle, FALSE);
    if (ret < 0) {
        AUD_DBG("Cannot get player handle audio[%d]", path);
        pthread_rwlock_unlock(_l);
        return (path < num_paths) ? av_paths_status[path].volume : 0;
    }

    ret = Aml_MP_Player_GetVolume(player_handle, &vol);
    if (ret < 0) {
        AUD_DBG("Get audio volume, vol:%f", vol);
    } else {
        AUD_DBG("Get audio volume failed, err:%d", ret);
    }
    pthread_rwlock_unlock(_l);

   U8BIT av_path = STB_AVGetPath(INVALID_RES_ID, path);

   if (av_path != INVALID_RES_ID) {
     av_paths_status[av_path].volume = vol;
   }

   FUNCTION_FINISH(STB_AVGetAudioVolume);
   return vol;
}

/**
 * @brief   Mutes or unmutes the audio output
 * @param   path The audio path to be configured
 * @param   mute TRUE to mute, FALSE to unmute
 */
void STB_AVSetAudioMute(U8BIT path, E_AV_OUT_CONTROL_FLAG flag, BOOLEAN audio_mute)
{
   int ret;
   AML_MP_PLAYER player_handle;
   BOOLEAN mute = FALSE;

   FUNCTION_START(STB_AVSetAudioMute);

   U8BIT av_path = STB_AVGetPath(INVALID_RES_ID, path);

   AUD_DBG("set mute: %d:[-:%d] mute[%d] flag[%x] path_flag[%x]", av_path, path, audio_mute, flag, av_paths_status[path].audio_out_control);

   if (audio_mute_lock)
   {
      AUD_DBG("Audio mute locked, can not change");
      return;
   }

    mute = AV_UpdateAudioOutControl_l(av_path, flag, audio_mute);

    pthread_rwlock_t* _l = STB_AVGetLockByPath(path);
    if (_l == NULL) {
        AUD_DBG("Can't get lock, audio decoder[%d]", path);
        return;
    }

    pthread_rwlock_rdlock(_l);
    ret = AV_GetPlayerHandleByPath_l(INVALID_RES_ID, path, &player_handle, FALSE);
    if (ret < 0) {
        AUD_DBG("Cannot get player handle audio[%d]", path);
        pthread_rwlock_unlock(_l);
        return;
    }

    AV_SetAudioMute_l(player_handle, mute);
    pthread_rwlock_unlock(_l);

    FUNCTION_FINISH(STB_AVSetAudioMute);
}

/**
 * @brief   Gets the current muting status of the audio output
 * @param   path the audio path to be queried
 * @return  TRUE audio is muted, FALSE otherwise
 */
BOOLEAN STB_AVGetAudioMute(U8BIT path)
{
   BOOLEAN retval = FALSE;
   int ret;
   char analog_mute, digital_mute;
   char mute = 0;
   AML_MP_PLAYER player_handle;
   FUNCTION_START(STB_AVGetAudioMute);

    pthread_rwlock_t* _l = STB_AVGetLockByPath(path);
    if (_l == NULL) {
        AUD_DBG("Can't get lock, audio decoder[%d]", path);
        return FALSE;
    }

    pthread_rwlock_rdlock(_l);
    ret = AV_GetPlayerHandleByPath_l(INVALID_RES_ID, path, &player_handle, FALSE);
    if (ret < 0) {
        AUD_DBG("Cannot get player handle[%d]", path);
        pthread_rwlock_unlock(_l);
        return FALSE;
    }

    ret = Aml_MP_Player_GetParameter(player_handle, AML_MP_PLAYER_PARAMETER_AUDIO_MUTE, &mute);
    if (ret == 0) {
        AUD_DBG("Get audio mute, mute:%d", mute);
        if (mute) {
            retval = TRUE;
        } else {
            retval = FALSE;
        }
    } else {
        AUD_DBG("Get audio mute failed, err:%d", ret);
    }
    pthread_rwlock_unlock(_l);

   FUNCTION_FINISH(STB_AVGetAudioMute);
   return(retval);
}

/**
 * @brief   Configures the audio channel mode (stereo/left/right)
 * @param   path the audio path to be configured
 * @param   mode the audio mode to use
 */
void STB_AVChangeAudioMode(U8BIT path, E_STB_AV_AUDIO_MODE mode)
{
   int ret;
   AML_MP_PLAYER player_handle;
   Aml_MP_AudioBalance audio_mode;
   FUNCTION_START(STB_AVChangeAudioMode);
   U8BIT av_path = STB_AVGetPath(INVALID_RES_ID, path);
   AUD_DBG("set stereo mode %d[-:%d] mode[%d]", av_path, path, mode);

   switch (mode)
   {
      case AV_AUDIO_STEREO:
         audio_mode = AML_MP_AUDIO_BALANCE_STEREO;
         break;
      case AV_AUDIO_RIGHT:
         audio_mode = AML_MP_AUDIO_BALANCE_RIGHT;
         break;
      case AV_AUDIO_LEFT:
         audio_mode = AML_MP_AUDIO_BALANCE_LEFT;
         break;
      case AV_AUDIO_MONO:
         audio_mode = AML_MP_AUDIO_BALANCE_LRMIX;
         break;
      case AV_AUDIO_MULTICHANNEL:
         audio_mode = AML_MP_AUDIO_BALANCE_STEREO;
         break;
      default:
         AUD_DBG("Not support audio mode:%d", mode);
         return;
   }
    if (av_path != INVALID_RES_ID)
    {
        av_paths_status[av_path ].audio_mode = audio_mode;
    }

    pthread_rwlock_t* _l = STB_AVGetLockByPath(path);
    if (_l == NULL) {
        AUD_DBG("Can't get lock, audio decoder[%d]", path);
        return;
    }

    pthread_rwlock_rdlock(_l);
    ret = AV_GetPlayerHandleByPath_l(INVALID_RES_ID, path, &player_handle, FALSE);
    if (ret != 0) {
        AUD_DBG("Cannot get player handle audio[%d]", path);
        pthread_rwlock_unlock(_l);
        return;
    }

    ret = Aml_MP_Player_SetParameter(player_handle, AML_MP_PLAYER_PARAMETER_AUDIO_BALANCE, &audio_mode);
    if (ret < 0) {
        AUD_DBG("%d Set audio stereo mode[%d] failed, err:%d", __LINE__, audio_mode, ret);
    } else {
        AUD_DBG("Set audio stereo mode[%d]", audio_mode);
    }
    pthread_rwlock_unlock(_l);

   FUNCTION_FINISH(STB_AVChangeAudioMode);
}

/**
 * @brief   Starts the Audio decoder
 * @param   path the audio decoder path to be started
 */
void STB_AVStartAudioDecoding(U8BIT path)
{
   U16BIT video_pid, audio_pid, pcr_pid, ad_pid;
   U8BIT preselection_id;
   BOOLEAN audio_mute = FALSE;
   int ret;
   AML_MP_PLAYER player_handle;
   Aml_MP_CodecID audio_format;

   FUNCTION_START(STB_AVStartAudioDecoding);

   U8BIT av_path = STB_AVGetPath(INVALID_RES_ID, path);
   if (av_path == INVALID_RES_ID) {
      AUD_DBG("audio decoder get path error audio=%u av_path=%u", path, av_path);
      return;
   }

    pthread_rwlock_t* _l = STB_AVGetLockByPath(path);
    if (_l == NULL) {
        AUD_DBG("Can't get lock, audio decoder[%d]", path);
        return;
    }

    pthread_rwlock_wrlock(_l);
    ret = AV_GetPlayerHandleByPath_l(av_paths_status[av_path].video_decoder, av_paths_status[av_path].audio_decoder, &player_handle, TRUE);
    if (ret < 0) {
        AUD_DBG("Cannot get player handle[%d], av_path[%d]", path, av_path);
        pthread_rwlock_unlock(_l);
        return;
    }

   if (STB_PVRIsPlayStopped(av_paths_status[av_path].audio_decoder, av_paths_status[av_path].video_decoder))
   {
      AUD_DBG("audio decoder path=%u av_path=%u", path, av_path);

      DMXGetDecodePIDs(av_paths_status[av_path].demux, &pcr_pid, &video_pid, &audio_pid, &ad_pid, &preselection_id);
      audio_format = av_paths_status[av_path].audio_format;

      if (av_paths_status[av_path].audio_out_control) {
          AUD_DBG("-->mute audio");
          audio_mute = TRUE;
          ret = Aml_MP_Player_SetParameter(player_handle, AML_MP_PLAYER_PARAMETER_AUDIO_MUTE, &audio_mute);
          if (ret < 0) {
                AUD_DBG("mute audio failed, err:%d", ret);
          }
      } else {
          AUD_DBG("-->umute audio");
      }

      if (audio_pid != 0 && audio_pid != INVALID_PID)
      {
         switch (AV_GetDecoderState_l(player_handle, AUDIO_DECODER))
         {
         case DECODER_STATE_STARTED:
            AUD_DBG("A NOW:A_START: start audio PID=%u(%u) FMT:%d", audio_pid, preselection_id, audio_format);
            if (audio_pid != av_paths_status[av_path].audio_pid ||
                preselection_id != av_paths_status[av_path].audio_presentation_id)
            {
                AUD_DBG("changing audio PID %u->%d(%u->%d)", av_paths_status[av_path].audio_pid, audio_pid, av_paths_status[av_path].audio_presentation_id, preselection_id);
                Aml_MP_Player_StopAudioDecoding(player_handle);
                ret = AV_StartAudioDecode_l(player_handle,audio_pid,audio_format,av_paths_status[av_path].audio_mode,av_paths_status[av_path].volume,av_paths_status[av_path].mute, preselection_id);
                if (ret == 0) {
                    av_paths_status[av_path].audio_pid = audio_pid;
                    av_paths_status[av_path].audio_presentation_id = preselection_id;
                }
            }
            break;

         case DECODER_STATE_STOPPED:
            AUD_DBG("A NOW:A_STOP: start audio PID=%u FMT:%d", audio_pid, audio_format);
            ret = AV_StartAudioDecode_l(player_handle,audio_pid,audio_format,av_paths_status[av_path].audio_mode,av_paths_status[av_path].volume,av_paths_status[av_path].mute, preselection_id);
            if (ret == 0)
            {
                av_paths_status[av_path].audio_pid = audio_pid;
                av_paths_status[av_path].audio_presentation_id = preselection_id;
                STB_OSSendEvent(FALSE, HW_EV_CLASS_DECODE, HW_EV_TYPE_AUDIO_STARTED, &path, sizeof(U8BIT));
            }
            break;
         default:
            break;
         }
      }
   }
   else
   {
      Aml_MP_CodecID video_format;
      Aml_MP_CodecID ad_format;

      DMXGetDecodePIDs(av_paths_status[av_path].demux, &pcr_pid, &video_pid, &audio_pid, &ad_pid, &preselection_id);
      audio_format = av_paths_status[av_path].audio_format;
      ad_format = av_paths_status[av_path].ad_format;
      video_format = av_paths_status[av_path].video_format;

      AUD_DBG("av-pvr: path=%u, apid:%d(%d), adpid:%d, vpid:%d", path, audio_pid, preselection_id, ad_pid, video_pid);

      if (audio_pid != 0 && audio_pid != INVALID_PID)
      {
         switch (AV_GetDecoderState_l(player_handle, AUDIO_DECODER))
         {
         case DECODER_STATE_STARTED:
            AUD_DBG("av-pvr: Audio decoder already started");
            if (audio_pid != av_paths_status[av_path].audio_pid ||
                preselection_id != av_paths_status[av_path].audio_presentation_id)
            {
                AUD_DBG("av-pvr: changing audio PID %u->%d(%u->%d)",
                    av_paths_status[av_path].audio_pid, audio_pid, av_paths_status[av_path].audio_presentation_id, preselection_id);
                if (PVRChangeDecodePIDs(av_paths_status[av_path].audio_decoder, av_paths_status[av_path].video_decoder, pcr_pid, video_pid, audio_pid, ad_pid,
                       toVideoCodec(video_format), toAudioCodec(audio_format), toAudioCodec(ad_format), preselection_id))
                {
                    AV_SetAudioDecode_l(player_handle, av_paths_status[av_path].audio_mode, av_paths_status[av_path].volume, av_paths_status[av_path].mute);
                    av_paths_status[av_path].audio_pid = audio_pid;
                    av_paths_status[av_path].audio_presentation_id = preselection_id;
                }
            }
            break;

         case DECODER_STATE_STOPPED:
            AUD_DBG("av-pvr: audio stopped, audio PID=%u(%d)", audio_pid, preselection_id);
            /*pvr radio will get here*/
            if (audio_pid != av_paths_status[av_path].audio_pid ||
                preselection_id != av_paths_status[av_path].audio_presentation_id)
            {
                if (PVRChangeDecodePIDs(av_paths_status[av_path].audio_decoder, av_paths_status[av_path].video_decoder, pcr_pid, video_pid, audio_pid, ad_pid,
                       toVideoCodec(video_format), toAudioCodec(audio_format), toAudioCodec(ad_format), preselection_id))
                {
                    AV_SetAudioDecode_l(player_handle, av_paths_status[av_path].audio_mode, av_paths_status[av_path].volume, av_paths_status[av_path].mute);
                    av_paths_status[av_path].audio_pid = audio_pid;
                    av_paths_status[av_path].audio_presentation_id = preselection_id;
                    STB_OSSendEvent(FALSE, HW_EV_CLASS_DECODE, HW_EV_TYPE_AUDIO_STARTED, &path, sizeof(U8BIT));
                }
            }
            break;
         default:
            break;
         }
      }

    }
    pthread_rwlock_unlock(_l);

    FUNCTION_FINISH(STB_AVStartAudioDecoding);
}

#ifdef SUPPORT_CAS
/*
 * Drm Mode on the given video path
 * @param   path video decoder path
 * @param   mode Drm Mode
 */
void STB_AVSetDrmMode(U8BIT path, E_STB_DRM_TYPE mode)
{
    VID_DBG("path=%u", path);
    if (path < num_paths) {
        U8BIT av_path = STB_AVGetPath(path, INVALID_RES_ID);

        if (av_path == INVALID_RES_ID) {
            AUD_DBG("STB_AVSetDrmMode error video=%u av_path=%u", path, av_path);
            return;
        }
        av_paths_status[av_path].drm_mode = mode;
    } else {
        VID_DBG("path=%u is error,not set drm mode", path);
  }
}
#endif

/**
 * @brief   Starts the video decoder
 * @param   path the video decode path to be started
 */
void STB_AVStartVideoDecoding(U8BIT path)
{
    U16BIT video_pid = INVALID_PID, audio_pid = INVALID_PID, pcr_pid = INVALID_PID, ad_pid = INVALID_PID;
    U8BIT preselection_id = INVALID_RES_ID;
    int ret;
    AML_MP_PLAYER player_handle;
    Aml_MP_CodecID video_format;
    Aml_MP_VideoParams video_param;
    FUNCTION_START(STB_AVStartVideoDecoding);

    U8BIT av_path = STB_AVGetPath(path, INVALID_RES_ID);
    if (av_path == INVALID_RES_ID) {
        AUD_DBG("video decoder get path error video=%u av_path=%u", path, av_path);
        return;
    }

    pthread_rwlock_t* _l = STB_AVGetLockByPath(path);
    if (_l == NULL) {
        VID_DBG("Can't get lock, video decoder[%d]", path);
        return;
    }

    pthread_rwlock_wrlock(_l);
    ret = AV_GetPlayerHandleByPath_l(av_paths_status[av_path].video_decoder, av_paths_status[av_path].audio_decoder, &player_handle, TRUE);
    if (ret < 0) {
        AUD_DBG("Cannot get player handle[%d], av_path[%d]", path, av_path);
        pthread_rwlock_unlock(_l);
        return;
    }

    if (STB_PVRIsPlayStopped(av_paths_status[av_path].audio_decoder, av_paths_status[av_path].video_decoder))
    {
        VID_DBG("video path=%u av_path=%u", path, av_path);
        DMXGetDecodePIDs(av_paths_status[av_path].demux, &pcr_pid, &video_pid, &audio_pid, &ad_pid, &preselection_id);
        video_format = av_paths_status[av_path].video_format;

        if (video_pid != 0)
        {
            if (video_surface[av_path] != NULL) {
                VID_DBG("set AML MP surface %d:[%d:%d] [%p], player[0x%p]",
                        av_path,
                        av_paths_status[av_path].video_decoder,
                        av_paths_status[av_path].audio_decoder,
                        video_surface[av_path],
                        player_handle);
                void* tmp = video_surface[av_path];
                ret = Aml_MP_Player_SetParameter(player_handle, AML_MP_PLAYER_PARAMETER_SURFACE_HANDLE, tmp);
                if (ret < 0) {
                    VID_DBG("Failed to set surface handle, err:%d", ret);
                }
            } else {
                VID_DBG("Cannot set surface to AML MP, surface is NULL. video path: (%d, %d)",
                        av_path, av_paths_status[av_path].video_decoder);
            }

            if (av_paths_status[path].video_out_control) {
                VID_DBG("-->hide video");
                ret = Aml_MP_Player_HideVideo(player_handle);
                if (ret < 0) {
                    VID_DBG("Hide video failed, err:%d", ret);
                }
            } else {
                VID_DBG("-->Show video");
            }

            switch (AV_GetDecoderState_l(player_handle, VIDEO_DECODER))
            {
            case DECODER_STATE_STARTED:
                VID_DBG("V NOW:V_START, Video decoder already started");
                if (video_pid != av_paths_status[av_path].video_pid)
                {
                    VID_DBG("#### video PID changed %u->%u, decoding restarted, FMT:%d ####", av_paths_status[av_path].video_pid, video_pid, video_format);
                    Aml_MP_Player_StopVideoDecoding(player_handle);
                    ret = AV_StartVideoDecode_l(player_handle, video_pid, pcr_pid, video_format, AML_MP_AVSYNC_SOURCE_PCR);
                    if (ret == 0)
                    {
                        av_paths_status[av_path].video_pid = video_pid;
                        av_paths_status[av_path].pcr_pid = pcr_pid;
                    }
                }
                break;
            case DECODER_STATE_STOPPED:
                VID_DBG("V NOW:V_STOP: start video PID=%u PCR=%u FMT=%d", video_pid, pcr_pid, video_format);
                ret = AV_StartVideoDecode_l(player_handle, video_pid, pcr_pid, video_format, AML_MP_AVSYNC_SOURCE_PCR);
                if (ret == 0)
                {
                    av_paths_status[av_path].video_pid = video_pid;
                    av_paths_status[av_path].pcr_pid = pcr_pid;
                }
                break;
            default:
                break;
            }
        }
    }
    else
    {
        Aml_MP_CodecID audio_format;
        Aml_MP_CodecID ad_format;

        DMXGetDecodePIDs(av_paths_status[av_path].demux, &pcr_pid, &video_pid, &audio_pid, &ad_pid, &preselection_id);
        audio_format = av_paths_status[av_path].audio_format;
        ad_format = av_paths_status[av_path].ad_format;
        video_format = av_paths_status[av_path].video_format;

        VID_DBG("av-pvr: path=%u, apid:%d vpid:%d", path, audio_pid, video_pid);

        if (audio_pid == 0)
        {
            audio_pid = INVALID_PID;
        }

        if (video_pid != 0)
        {
            switch (AV_GetDecoderState_l(player_handle, VIDEO_DECODER))
            {
            case DECODER_STATE_STARTED:
                /*Just in case we get two calls to audio start without a stop
                  There's an API to switch, so we'll use it*/
                VID_DBG("av-pvr: Video decoder already started");
                if (video_pid != av_paths_status[av_path].video_pid)
                {
                    VID_DBG("av-pvr: video PID changed %u->%u, notify to pvr", av_paths_status[av_path].video_pid, video_pid);
                    if (PVRChangeDecodePIDs(av_paths_status[av_path].audio_decoder,
                                            av_paths_status[av_path].video_decoder, pcr_pid, video_pid, audio_pid, ad_pid,
                                            toVideoCodec(video_format), toAudioCodec(audio_format), toAudioCodec(ad_format), preselection_id))
                    {
                        av_paths_status[av_path].video_pid = video_pid;
                    }
                }
                break;
            case DECODER_STATE_STOPPED:
                VID_DBG("av-pvr: video stopped, video PID=%u", video_pid);
                if (video_pid != av_paths_status[av_path].video_pid)
                {
                    VID_DBG("av-pvr: video PID changed %u->%u, notify to pvr", av_paths_status[av_path].video_pid, video_pid);
                    if (PVRChangeDecodePIDs(av_paths_status[av_path].audio_decoder,
                                            av_paths_status[av_path].video_decoder, pcr_pid, video_pid, audio_pid, ad_pid,
                                            toVideoCodec(video_format), toAudioCodec(audio_format), toAudioCodec(ad_format), preselection_id))
                    {
                        av_paths_status[av_path].video_pid = video_pid;
                    }
                }
                break;
            default:
                break;
            }
        }
    }
    pthread_rwlock_unlock(_l);

    FUNCTION_FINISH(STB_AVStartVideoDecoding);
}

/**
 * @brief   Pause video decoding
 * @param   path Required Decode Path Number.
 */
void  STB_AVPauseVideoDecoding(U8BIT path)
{
   FUNCTION_START(STB_AVPauseVideoDecoding);
   FUNCTION_FINISH(STB_AVPauseVideoDecoding);
}

/**
 * @brief   Resume video decoding
 * @param   path Required Decode Path Number.
 */
void  STB_AVResumeVideoDecoding(U8BIT path)
{
   FUNCTION_START(STB_AVResumeVideoDecoding);
   FUNCTION_FINISH(STB_AVResumeVideoDecoding);
}

/**
 * @brief   Stops the video decoder
 * @param   path the video decoder path to be stopped
 */
void STB_AVStopVideoDecoding(U8BIT path)
{
   int ret;
   S_STB_AV_VIDEO_INFO info;
   AML_MP_PLAYER player_handle;
   FUNCTION_START(STB_AVStopVideoDecoding);

   U8BIT av_path = STB_AVGetPath(path, INVALID_RES_ID);

   VID_DBG("video codec path=%u av_path = %u", path, av_path);
   if (av_path == INVALID_RES_ID) {
     VID_DBG("get av_path error video codec path=%u av_path = %u", path, av_path);
     return;
   }
   info.flags = (E_STB_AV_VIDEO_INFO_TYPE)0;

    pthread_rwlock_t* _l = STB_AVGetLockByPath(path);
    if (_l == NULL) {
        VID_DBG("Can't get lock, video decoder[%d]", path);
        return;
    }

    pthread_rwlock_wrlock(_l);
    ret = AV_GetPlayerHandleByPath_l(av_paths_status[av_path].video_decoder, av_paths_status[av_path].audio_decoder, &player_handle, FALSE);
    if (ret < 0) {
        AUD_DBG("Cannot get player handle[%d], av_path[%d]", path, av_path);
        pthread_rwlock_unlock(_l);
        return;
    }

   if (STB_PVRIsPlayStopped(av_paths_status[av_path].audio_decoder, av_paths_status[av_path].video_decoder))
   {
      switch (AV_GetDecoderState_l(player_handle, VIDEO_DECODER))
      {
         case DECODER_STATE_STARTED:
         {
            VID_DBG("V NOW:V_START: Stop Video decoding");
            if (!IS_INVALID_PLAYER_HANDLE(av_path)) {
               ret = Aml_MP_Player_StopVideoDecoding(av_paths_status[av_path].player_handle);
               if (ret == 0) {
                    if (AV_GetDecoderState_l(player_handle, AUDIO_DECODER) == DECODER_STATE_STOPPED &&
                        AV_GetDecoderState_l(player_handle, AD_DECODER) == DECODER_STATE_STOPPED)
                    {
                       AV_ReleaseTsPlayer_l(av_path);
                   } else {
                       VID_DBG("A NOW: A_START");
                   }
               }else {
                   VID_DBG("AmlMpPlayer_stopVideoDecoding failed, err:%d", ret);
               }
            }
            {
                av_paths_status[av_path].video_pid = INVALID_PID;
                av_paths_status[av_path].pcr_pid = INVALID_PID;
                STB_OSSendEvent(FALSE, HW_EV_CLASS_DECODE, HW_EV_TYPE_VIDEO_STOPPED, &path, sizeof(U8BIT));
            }
            info.status = DECODER_STATUS_NONE;
            info.flags = VIDEO_INFO_DECODER_STATUS;
            break;
         }

         case DECODER_STATE_STOPPED:
            VID_DBG("V NOW:V_STOP, Stop video decode already");
            break;
      }
   }
   else
   {
      U16BIT video_pid = INVALID_PID, audio_pid = INVALID_PID, pcr_pid = INVALID_PID, ad_pid = INVALID_PID;
      U8BIT preselection_id = INVALID_RES_ID;
      Aml_MP_CodecID video_format;
      Aml_MP_CodecID audio_format;
      Aml_MP_CodecID ad_format;

      DMXGetDecodePIDs(av_paths_status[av_path].demux, &pcr_pid, &video_pid, &audio_pid, &ad_pid, &preselection_id);
      audio_format = av_paths_status[av_path].audio_format;
      ad_format = av_paths_status[av_path].ad_format;
      video_format = av_paths_status[av_path].video_format;

      AUD_DBG("av-pvr: V NOW:V_START: Stop Video decoding. path=%u, apid:%d[%d], preselection_id:%d[%d], adpid:%d[%d], vpid:%d[%d]", path, \
          audio_pid, av_paths_status[av_path].audio_pid, \
          preselection_id, av_paths_status[av_path].audio_presentation_id, \
          ad_pid, av_paths_status[av_path].ad_pid, \
          video_pid, av_paths_status[av_path].video_pid);

      if (audio_pid == 0)
      {
         audio_pid = INVALID_PID;
      }

      if (video_pid != 0)
      {
         switch (AV_GetDecoderState_l(player_handle, VIDEO_DECODER))
         {
         case DECODER_STATE_STARTED:
            /*Just in case we get two calls to audio start without a stop
              There's an API to switch, so we'll use it*/
            VID_DBG("av-pvr: Video decoder started");
            video_pid = INVALID_PID;
            if (video_pid != av_paths_status[av_path].video_pid)
            {
               VID_DBG("av-pvr: video PID changed %u->%u, notify to pvr", av_paths_status[av_path].video_pid, video_pid);
               if (PVRChangeDecodePIDs(av_paths_status[av_path].audio_decoder,
                                         av_paths_status[av_path].video_decoder, pcr_pid, video_pid, audio_pid, ad_pid,
                    toVideoCodec(video_format), toAudioCodec(audio_format), toAudioCodec(ad_format), preselection_id))
               {
                  av_paths_status[av_path].video_pid = video_pid;
                  if (AV_GetDecoderState_l(player_handle, AUDIO_DECODER) == DECODER_STATE_STOPPED) {
                     //release afd context
                     afd_release_context(av_path);
                  } else {
                     VID_DBG("av-pvr: A NOW: A_START");
                  }
               }
               {
                   av_paths_status[av_path].video_pid = INVALID_PID;
                   av_paths_status[av_path].pcr_pid = INVALID_PID;
                   STB_OSSendEvent(FALSE, HW_EV_CLASS_DECODE, HW_EV_TYPE_VIDEO_STOPPED, &path, sizeof(U8BIT));
               }
               info.status = DECODER_STATUS_NONE;
               info.flags = VIDEO_INFO_DECODER_STATUS;
            }
            break;
         case DECODER_STATE_STOPPED:
            VID_DBG("av-pvr: video stoppped already");
            break;
         default:
            break;
         }
      }
    }
    pthread_rwlock_unlock(_l);

   if ((info.flags != 0) && (av_paths_status[av_path].callback != NULL))
   {
      /*reset the afd*/
      info.flags |= VIDEO_INFO_AFD;
      info.afd = 0;

        invokeCallback(&av_paths_status[av_path], &info);
        //av_paths_status[av_path].callback(&info, av_paths_status[av_path].user_data, av_paths_status[av_path].video_decoder);
   }

   FUNCTION_FINISH(STB_AVStopVideoDecoding);
}

/**
 * @brief   Stops the audio decoder
 * @param   path the audio decoder path to be stopped
 */
void STB_AVStopAudioDecoding(U8BIT path)
{
    int ret;
    AML_MP_PLAYER player_handle;

   FUNCTION_START(STB_AVStopAudioDecoding);
   U8BIT av_path = STB_AVGetPath(INVALID_RES_ID, path);

   AUD_DBG("audio codec path=%u av_path = %u", path, av_path);
   if (av_path == INVALID_RES_ID) {
     VID_DBG("get av_path error audio codec path=%u av_path = %u", path, av_path);
     return;
   }

    pthread_rwlock_t* _l = STB_AVGetLockByPath(path);
    if (_l == NULL) {
        AUD_DBG("Can't get lock, audio decoder[%d]", path);
        return;
    }

    pthread_rwlock_wrlock(_l);
    ret = AV_GetPlayerHandleByPath_l(av_paths_status[av_path].video_decoder, av_paths_status[av_path].audio_decoder, &player_handle, FALSE);
    if (ret < 0) {
        AUD_DBG("Cannot get player handle[%d], av_path[%d]", path, av_path);
        pthread_rwlock_unlock(_l);
        return;
    }

    if (STB_PVRIsPlayStopped(av_paths_status[av_path].audio_decoder, av_paths_status[av_path].video_decoder))
    {
        switch (AV_GetDecoderState_l(player_handle, AUDIO_DECODER)) {
            case DECODER_STATE_STOPPED:
            {
                AUD_DBG("A NOW:A_STOP, Stop audio decode already");
                if (!IS_INVALID_PLAYER_HANDLE(av_path)) {
                    if (AV_GetDecoderState_l(player_handle, VIDEO_DECODER) == DECODER_STATE_STOPPED &&
                        AV_GetDecoderState_l(player_handle, AD_DECODER) == DECODER_STATE_STOPPED)
                    {
                        AV_ReleaseTsPlayer_l(av_path);
                    }
                }
                break;
            }
            case DECODER_STATE_STARTED:
            {
                AUD_DBG("A NOW:A_START, Stop Audio decoding");
                if (!IS_INVALID_PLAYER_HANDLE(av_path)) {
                    ret = Aml_MP_Player_StopAudioDecoding(av_paths_status[av_path].player_handle);
                    if (ret == 0) {
                        if (AV_GetDecoderState_l(player_handle, VIDEO_DECODER) == DECODER_STATE_STOPPED &&
                            AV_GetDecoderState_l(player_handle, AD_DECODER) == DECODER_STATE_STOPPED)
                        {
                            AV_ReleaseTsPlayer_l(av_path);
                        } else {
                            AUD_DBG("V NOW:V_START");
                        }
                    }else {
                        AUD_DBG("Aml_MP_Player_StopAudioDecoding failed, err:%d", ret);
                    }
                }

                av_paths_status[av_path].audio_pid = INVALID_PID;
                av_paths_status[av_path].audio_presentation_id = INVALID_PID;
                STB_OSSendEvent(FALSE, HW_EV_CLASS_DECODE, HW_EV_TYPE_AUDIO_STOPPED, &path, sizeof(U8BIT));
                break;
            }
        }
    } else {
        U16BIT video_pid = INVALID_PID, audio_pid = INVALID_PID, pcr_pid = INVALID_PID, ad_pid = INVALID_PID;
        U8BIT preselection_id = INVALID_RES_ID;
        Aml_MP_CodecID video_format;
        Aml_MP_CodecID audio_format;
        Aml_MP_CodecID ad_format;

        DMXGetDecodePIDs(av_paths_status[av_path].demux, &pcr_pid, &video_pid, &audio_pid, &ad_pid, &preselection_id);
        audio_format = av_paths_status[av_path].audio_format;
        ad_format = av_paths_status[av_path].ad_format;
        video_format = av_paths_status[av_path].video_format;

        AUD_DBG("av-pvr: Stop Audio decoding. path=%u, apid:%d[%d], preselection_id:%d[%d], adpid:%d[%d], vpid:%d[%d]", path, \
            audio_pid, av_paths_status[av_path].audio_pid, \
            preselection_id, av_paths_status[av_path].audio_presentation_id, \
            ad_pid, av_paths_status[av_path].ad_pid, \
            video_pid, av_paths_status[av_path].video_pid);

        if (audio_pid == 0)
        {
           audio_pid = INVALID_PID;
        }

        if (audio_pid != 0)
        {
           switch (AV_GetDecoderState_l(player_handle, AUDIO_DECODER))
           {
           case DECODER_STATE_STARTED:
              /*Just in case we get two calls to audio start without a stop
                There's an API to switch, so we'll use it*/
              VID_DBG("av-pvr: audio decoder started");
              audio_pid = INVALID_PID;
              if (audio_pid != av_paths_status[av_path].audio_pid \
                || preselection_id != av_paths_status[av_path].audio_presentation_id)
              {
                 VID_DBG("av-pvr: audio PID changed %u->%u, notify to pvr", av_paths_status[av_path].audio_pid, audio_pid);
                 if (PVRChangeDecodePIDs(av_paths_status[av_path].audio_decoder,
                                           av_paths_status[av_path].video_decoder, pcr_pid, video_pid, audio_pid, ad_pid,
                      toVideoCodec(video_format), toAudioCodec(audio_format), toAudioCodec(ad_format), preselection_id))
                 {
                    av_paths_status[av_path].audio_pid = audio_pid;
                    VID_DBG("av-pvr: A NOW: A_STOP");
                 }
                 {
                     av_paths_status[av_path].audio_pid = INVALID_PID;
                     av_paths_status[av_path].pcr_pid = INVALID_PID;
                     STB_OSSendEvent(FALSE, HW_EV_CLASS_DECODE, HW_EV_TYPE_AUDIO_STOPPED, &path, sizeof(U8BIT));
                 }
              }
              break;
           case DECODER_STATE_STOPPED:
              VID_DBG("av-pvr: audio stopped already");
              audio_pid = INVALID_PID;
              if (audio_pid != av_paths_status[av_path].audio_pid \
                || preselection_id != av_paths_status[av_path].audio_presentation_id)
              {
                 VID_DBG("av-pvr: audio stopped. audio PID changed %u->%u, notify to pvr", av_paths_status[av_path].audio_pid, audio_pid);
                 if (PVRChangeDecodePIDs(av_paths_status[av_path].audio_decoder,
                                           av_paths_status[av_path].video_decoder, pcr_pid, video_pid, audio_pid, ad_pid,
                      toVideoCodec(video_format), toAudioCodec(audio_format), toAudioCodec(ad_format), preselection_id))
                 {
                    av_paths_status[av_path].audio_pid = audio_pid;
                    VID_DBG("av-pvr: A NOW: A_STOP");
                 }
                 {
                     av_paths_status[av_path].audio_pid = INVALID_PID;
                     av_paths_status[av_path].pcr_pid = INVALID_PID;
                 }
              }
              break;
           default:
              break;
           }
        }
    }
    pthread_rwlock_unlock(_l);

    FUNCTION_FINISH(STB_AVStopAudioDecoding);
}

/**
 * @brief   Starts the Audio decoder
 * @param   path the audio decoder path to be started
 */
void STB_AVSwitchAudioTrack(U8BIT path)
{
    U16BIT video_pid, audio_pid, pcr_pid, ad_pid;
    U8BIT preselection_id;
    BOOLEAN audio_mute = FALSE;
    int ret;
    AML_MP_PLAYER player_handle;
    Aml_MP_CodecID audio_format;
    FUNCTION_START(STB_AVSwitchAudioTrack);

    FUNCTION_START(STB_AVStopAudioDecoding);
    U8BIT av_path = STB_AVGetPath(INVALID_RES_ID, path);

    AUD_DBG("audio codec path=%u av_path = %u", path, av_path);
    if (av_path == INVALID_RES_ID) {
        VID_DBG("get av_path error audio codec path=%u av_path = %u", path, av_path);
        return;
    }

    pthread_rwlock_t* _l = STB_AVGetLockByPath(path);
    if (_l == NULL) {
        AUD_DBG("Can't get lock, audio decoder[%d]", path);
        return;
    }

    pthread_rwlock_wrlock(_l);
    ret = AV_GetPlayerHandleByPath_l(av_paths_status[av_path].video_decoder, av_paths_status[av_path].audio_decoder, &player_handle, FALSE);
    if (ret < 0) {
        AUD_DBG("Cannot get player handle[%d], av_path[%d]", path, av_path);
        pthread_rwlock_unlock(_l);
        return;
    }

    switch (AV_GetDecoderState_l(player_handle, AUDIO_DECODER))
    {
        case DECODER_STATE_STOPPED:
        {
            AUD_DBG("A NOW:A_STOP, Stop audio decode already");
            if (!IS_INVALID_PLAYER_HANDLE(av_path)) {
                if (AV_GetDecoderState_l(player_handle, VIDEO_DECODER) == DECODER_STATE_STOPPED &&
                    AV_GetDecoderState_l(player_handle, AD_DECODER) == DECODER_STATE_STOPPED)
                {
                    AV_ReleaseTsPlayer_l(av_path);
                }
            }
            break;
        }
        case DECODER_STATE_STARTED:
        {
            AUD_DBG("A NOW:A_START, Stop Audio decoding");
            if (!IS_INVALID_PLAYER_HANDLE(av_path)) {
                ret = Aml_MP_Player_StopAudioDecoding(av_paths_status[av_path].player_handle);
                if (ret == 0) {
                    if (AV_GetDecoderState_l(player_handle, VIDEO_DECODER) == DECODER_STATE_STOPPED &&
                        AV_GetDecoderState_l(player_handle, AD_DECODER) == DECODER_STATE_STOPPED)
                    {
                        AV_ReleaseTsPlayer_l(av_path);
                    } else {
                        AUD_DBG("V NOW:V_START");
                    }
                }else {
                    AUD_DBG("Aml_MP_Player_StopAudioDecoding failed, err:%d", ret);
                }
            }
            av_paths_status[av_path].audio_pid = INVALID_PID;
            av_paths_status[av_path].audio_presentation_id = INVALID_PID;
            STB_OSSendEvent(FALSE, HW_EV_CLASS_DECODE, HW_EV_TYPE_AUDIO_STOPPED, &path, sizeof(U8BIT));
            break;
        }
    }

    if (STB_PVRIsPlayStopped(av_paths_status[av_path].audio_decoder, av_paths_status[av_path].video_decoder))
    {
        AUD_DBG("audio decoder path=%u av_path=%u", path, av_path);

        DMXGetDecodePIDs(av_paths_status[av_path].demux, &pcr_pid, &video_pid, &audio_pid, &ad_pid, &preselection_id);
        audio_format = av_paths_status[av_path].audio_format;

        if (av_paths_status[av_path].audio_out_control) {
            AUD_DBG("-->mute audio");
            audio_mute = TRUE;
            ret = Aml_MP_Player_SetParameter(player_handle, AML_MP_PLAYER_PARAMETER_AUDIO_MUTE, &audio_mute);
            if (ret < 0) {
                AUD_DBG("mute audio failed, err:%d", ret);
            }
        } else {
            AUD_DBG("-->umute audio");
        }

        if (audio_pid != 0 && audio_pid != INVALID_PID)
        {
            switch (AV_GetDecoderState_l(player_handle, AUDIO_DECODER))
            {
                case DECODER_STATE_STARTED:
                    AUD_DBG("A NOW:A_START: start audio PID=%u(%u) FMT:%d", audio_pid, preselection_id, audio_format);
                    if (audio_pid != av_paths_status[av_path].audio_pid || preselection_id != av_paths_status[av_path].audio_presentation_id)
                    {
                        AUD_DBG("changing audio PID %u->%d(%u->%d)", av_paths_status[av_path].audio_pid, audio_pid, av_paths_status[av_path].audio_presentation_id, preselection_id);
                        Aml_MP_Player_StopAudioDecoding(player_handle);
                        ret = AV_StartAudioDecode_l(player_handle,audio_pid,audio_format,av_paths_status[av_path].audio_mode,av_paths_status[av_path].volume,av_paths_status[av_path].mute, preselection_id);
                        if (ret == 0) {
                            av_paths_status[av_path].audio_pid = audio_pid;
                            av_paths_status[av_path].audio_presentation_id = preselection_id;
                        }
                    }
                    break;

                case DECODER_STATE_STOPPED:
                    AUD_DBG("A NOW:A_STOP: start audio PID=%u FMT:%d", audio_pid, audio_format);
                    ret = AV_StartAudioDecode_l(player_handle,audio_pid,audio_format,av_paths_status[av_path].audio_mode,av_paths_status[av_path].volume,av_paths_status[av_path].mute, preselection_id);
                    if (ret == 0)
                    {
                        av_paths_status[av_path].audio_pid = audio_pid;
                        av_paths_status[av_path].audio_presentation_id = preselection_id;
                        STB_OSSendEvent(FALSE, HW_EV_CLASS_DECODE, HW_EV_TYPE_AUDIO_STARTED, &path, sizeof(U8BIT));
                    }
                    break;

                default:
                    break;
            }
        }
    }
    else
    {
        Aml_MP_CodecID video_format;
        Aml_MP_CodecID ad_format;

        DMXGetDecodePIDs(av_paths_status[av_path].demux, &pcr_pid, &video_pid, &audio_pid, &ad_pid, &preselection_id);
        audio_format = av_paths_status[av_path].audio_format;
        ad_format = av_paths_status[av_path].ad_format;
        video_format = av_paths_status[av_path].video_format;

        AUD_DBG("av-pvr: path=%u, apid:%d(%d), adpid:%d, vpid:%d", path, audio_pid, preselection_id, ad_pid, video_pid);

        if (audio_pid != 0 && audio_pid != INVALID_PID)
        {
            switch (AV_GetDecoderState_l(player_handle, AUDIO_DECODER))
            {
                case DECODER_STATE_STARTED:
                    AUD_DBG("av-pvr: Audio decoder already started");
                    if (audio_pid != av_paths_status[av_path].audio_pid || preselection_id != av_paths_status[av_path].audio_presentation_id)
                    {
                        AUD_DBG("av-pvr: changing audio PID %u->%d(%u->%d)",
                        av_paths_status[av_path].audio_pid, audio_pid, av_paths_status[av_path].audio_presentation_id, preselection_id);
                        if (PVRChangeDecodePIDs(av_paths_status[av_path].audio_decoder, av_paths_status[av_path].video_decoder, pcr_pid, video_pid, audio_pid, ad_pid,
                            toVideoCodec(video_format), toAudioCodec(audio_format), toAudioCodec(ad_format), preselection_id))
                        {
                            AV_SetAudioDecode_l(player_handle, av_paths_status[av_path].audio_mode, av_paths_status[av_path].volume, av_paths_status[av_path].mute);
                            av_paths_status[av_path].audio_pid = audio_pid;
                            av_paths_status[av_path].audio_presentation_id = preselection_id;
                        }
                    }
                    break;

                case DECODER_STATE_STOPPED:
                    AUD_DBG("av-pvr: audio stopped, audio PID=%u(%d)", audio_pid, preselection_id);
                    /*pvr radio will get here*/
                    if (audio_pid != av_paths_status[av_path].audio_pid || preselection_id != av_paths_status[av_path].audio_presentation_id)
                    {
                        if (PVRChangeDecodePIDs(av_paths_status[av_path].audio_decoder, av_paths_status[av_path].video_decoder, pcr_pid, video_pid, audio_pid, ad_pid,
                            toVideoCodec(video_format), toAudioCodec(audio_format), toAudioCodec(ad_format), preselection_id))
                        {
                            AV_SetAudioDecode_l(player_handle, av_paths_status[av_path].audio_mode, av_paths_status[av_path].volume, av_paths_status[av_path].mute);
                            av_paths_status[av_path].audio_pid = audio_pid;
                            av_paths_status[av_path].audio_presentation_id = preselection_id;
                            STB_OSSendEvent(FALSE, HW_EV_CLASS_DECODE, HW_EV_TYPE_AUDIO_STARTED, &path, sizeof(U8BIT));
                        }
                    }
                    break;

                default:
                    break;
            }
        }
    }
    pthread_rwlock_unlock(_l);

    FUNCTION_FINISH(STB_AVSwitchAudioTrack);
}

/**
 * @brief   Returns the current 33-bit System Time Clock from the PCR PES.
 *          On some systems, this information may need to be obtained from the associated demux,
 *          which will be contained in the 'param' value when STB_AVSetVideoSource is called.
 * @param   path video path
 * @param   stc an array in which the STC will be returned, ordered such that
 *                stc[0] contains the MS bit (33) of the STC value and stc[4]
 *                contains the LS bits (0-7).
 */
void STB_AVGetSTC(U8BIT path, U8BIT stc[5])
{
   int64_t video_pts = 0;
   int ret;
   AML_MP_PLAYER player_handle;
   FUNCTION_START(STB_AVGetSTC);
   U8BIT av_path = STB_AVGetPath(path, INVALID_RES_ID);

   VID_DBG("video codec path=%u av_path = %u", path, av_path);
   if (av_path == INVALID_RES_ID) {
      VID_DBG("get av_path error video codec path=%u av_path = %u", path, av_path);
      return;
   }

    pthread_rwlock_t* _l = STB_AVGetLockByPath(path);
    if (_l == NULL) {
        VID_DBG("Can't get lock, video decoder[%d]", path);
        return;
    }

    pthread_rwlock_rdlock(_l);
    ret = AV_GetPlayerHandleByPath_l(av_paths_status[av_path].video_decoder,
                                    av_paths_status[av_path].audio_decoder, &player_handle, FALSE);
   if (ret < 0)
   {
       AUD_DBG("Cannot get player handle video path:[%u] av_path:[%d]", path, av_path);
       pthread_rwlock_unlock(_l);
       return;
   }
   STB_SPDebugWrite(" %s %d", __FUNCTION__, __LINE__);
   ret = Aml_MP_Player_GetCurrentPts(player_handle, AML_MP_STREAM_TYPE_VIDEO, &video_pts);
   STB_SPDebugWrite(" %s %d", __FUNCTION__, __LINE__);
   if (ret == 0)
   {
       memset(stc, 0, 5);
       stc[0] = (U8BIT)((video_pts >> 32) & 0xff);
       stc[1] = (U8BIT)((video_pts >> 24) & 0xff);
       stc[2] = (U8BIT)((video_pts >> 16) & 0xff);
       stc[3] = (U8BIT)((video_pts >> 8) & 0xff);
       stc[4] = (U8BIT)(video_pts & 0xff);
       AUD_DBG("######### %x%x%x%x%x [%lld] ########", stc[0],stc[1],stc[2],stc[3],stc[4], video_pts);
   }
   pthread_rwlock_unlock(_l);
   FUNCTION_FINISH(STB_AVGetSTC);
}

void STB_AVGetSTCByStreamTypePCR(U8BIT path, U8BIT stc[5])
{
   int64_t video_pts = 0;
   int ret;
   AML_MP_PLAYER player_handle;
   FUNCTION_START(STB_AVGetSTCByStreamTypePCR);
   U8BIT av_path = STB_AVGetPath(path, INVALID_RES_ID);

   VID_DBG("video codec path=%u av_path = %u", path, av_path);
   if (av_path == INVALID_RES_ID) {
      VID_DBG("get av_path error video codec path=%u av_path = %u", path, av_path);
      return;
   }

    pthread_rwlock_t* _l = STB_AVGetLockByPath(path);
    if (_l == NULL) {
        VID_DBG("Can't get lock, video decoder[%d]", path);
        return;
    }

    pthread_rwlock_rdlock(_l);
    ret = AV_GetPlayerHandleByPath_l(av_paths_status[av_path].video_decoder,
                                    av_paths_status[av_path].audio_decoder, &player_handle, FALSE);
    if (ret < 0)
    {
        AUD_DBG("Cannot get player handle video path:[%u] av_path:[%d]", path, av_path);
        pthread_rwlock_unlock(_l);
        return;
    }
    STB_SPDebugWrite(" %s %d", __FUNCTION__, __LINE__);
    ret = Aml_MP_Player_GetCurrentPts(player_handle, AML_MP_STREAM_TYPE_STC, &video_pts);
    AUD_DBG("the ret value = %d",ret);
    STB_SPDebugWrite(" %s %d", __FUNCTION__, __LINE__);
    if (ret == 0)
    {
        memset(stc, 0, 5);
        stc[0] = (U8BIT)((video_pts >> 32) & 0xff);
        stc[1] = (U8BIT)((video_pts >> 24) & 0xff);
        stc[2] = (U8BIT)((video_pts >> 16) & 0xff);
        stc[3] = (U8BIT)((video_pts >> 8) & 0xff);
        stc[4] = (U8BIT)(video_pts & 0xff);
        AUD_DBG("######### %x%x%x%x%x [%lld] ########", stc[0],stc[1],stc[2],stc[3],stc[4], video_pts);
    }
    pthread_rwlock_unlock(_l);

    FUNCTION_FINISH(STB_AVGetSTCByStreamTypePCR);
}

/**
 * @brief   Returns the current 33-bit System Time Clock from the PCR PES.
 *          On some systems, this information may need to be obtained from the associated demux,
 *          which will be contained in the 'param' value when STB_AVSetVideoSource is called.
 * @param   path video path
 * @param   stc an array in which the STC will be returned, ordered such that
 *                stc[0] contains the MS bit (33) of the STC value and stc[4]
 *                contains the LS bits (0-7).
 * @param   StreamType enum Aml_MP_StreamType,point to which pts do you want
 */
void STB_AVGetSTCByStreamType(U8BIT path, U8BIT stc[5], E_HBBTV_PTS_SOURCE_TYPE StreamType)
{
   int64_t video_pts = 0;
   int ret;
   AML_MP_PLAYER player_handle;
   FUNCTION_START(STB_AVGetSTCByStreamType);
   U8BIT av_path = STB_AVGetPath(path, INVALID_RES_ID);

   VID_DBG("video codec path=%u av_path = %u", path, av_path);
   if (av_path == INVALID_RES_ID) {
      VID_DBG("get av_path error video codec path=%u av_path = %u", path, av_path);
      return;
   }

    pthread_rwlock_t* _l = STB_AVGetLockByPath(path);
    if (_l == NULL) {
        VID_DBG("Can't get lock, video decoder[%d]", path);
        return;
    }

    pthread_rwlock_rdlock(_l);
    ret = AV_GetPlayerHandleByPath_l(av_paths_status[av_path].video_decoder,
                                    av_paths_status[av_path].audio_decoder, &player_handle, FALSE);
    if (ret < 0)
    {
        AUD_DBG("Cannot get player handle video path:[%u] av_path:[%d]", path, av_path);
        pthread_rwlock_unlock(_l);
        return;
    }
    STB_SPDebugWrite(" %s %d", __FUNCTION__, __LINE__);
    ret = Aml_MP_Player_GetCurrentPts(player_handle, toMPStreamType(StreamType), &video_pts);
    AUD_DBG("StreamType = %d,MP_StreamType = %d", StreamType, toMPStreamType(StreamType));
    STB_SPDebugWrite(" %s %d", __FUNCTION__, __LINE__);
    if (ret == 0)
    {
        memset(stc, 0, 5);
        stc[0] = (U8BIT)((video_pts >> 32) & 0xff);
        stc[1] = (U8BIT)((video_pts >> 24) & 0xff);
        stc[2] = (U8BIT)((video_pts >> 16) & 0xff);
        stc[3] = (U8BIT)((video_pts >> 8) & 0xff);
        stc[4] = (U8BIT)(video_pts & 0xff);
        AUD_DBG("######### %x%x%x%x%x [%lld] ########", stc[0],stc[1],stc[2],stc[3],stc[4], video_pts);
    }
    pthread_rwlock_unlock(_l);

    FUNCTION_FINISH(STB_AVGetSTCByStreamType);
}

/**
 * @brief   Returns the current PTS from 33-bit System Time Clock.
 * @param   stc an array and we can calculate pts from it.
 *                stc[0] contains the MS bit (33) of the STC value and stc[4]
 *                contains the LS bits (0-7).
 */
U64BIT STB_AVGetPTSBySTC(U8BIT stc[5])
{
    U64BIT pts = 0;
    if (stc)
    {
        pts = (((uint64_t)stc[0] << 32) + ((uint64_t)stc[1] << 24) + ((uint64_t)stc[2] << 16) + ((uint64_t)stc[3] << 8) + (uint64_t)stc[4]);
        AUD_DBG("######### %x%x%x%x%x [%llu] ########", stc[0],stc[1],stc[2],stc[3],stc[4], pts);
    }
    return pts;
}

/**
 * @brief   Sets the source of the video decoder
 * @param   path video path to configure
 * @param   source the source device to use
 * @param   param tuner or demux number
 */
void STB_AVSetVideoSource(U8BIT path, E_STB_AV_DECODE_SOURCE source, U32BIT param)
{
   FUNCTION_START(STB_AVSetVideoSource);

   VID_DBG("path=%u, source=%u, param=%u", path, source, param);
   U8BIT av_path = STB_AVGetPath(path, INVALID_RES_ID);

   VID_DBG("video codec path=%u av_path = %u", path, av_path);
   if (av_path == INVALID_RES_ID) {
      VID_DBG("get av_path error video codec path=%u av_path = %u", path, av_path);
      return;
   }

   if (path < num_paths)
   {
      if (source == AV_DEMUX)
      {
         av_paths_status[av_path].demux = param & 0xff;
      }
   }

   FUNCTION_FINISH(STB_AVSetVideoSource);
}

/**
 * @brief   Sets the source of the audio decoder
 * @param   path audio path to configure
 * @param   source the source device to use
 * @param   param tuner or demux number
 */
void STB_AVSetAudioSource(U8BIT path, E_STB_AV_DECODE_SOURCE source, U32BIT param)
{
   FUNCTION_START(STB_AVSetAudioSource);

   AUD_DBG("path=%u, source=%u, param=%u", path, source, param);
   U8BIT av_path = STB_AVGetPath(INVALID_RES_ID, path);

   AUD_DBG("audio codec path=%u av_path = %u", path, av_path);
   if (av_path == INVALID_RES_ID) {
      AUD_DBG("get av_path error audio codec path=%u av_path = %u", path, av_path);
      return;
   }

   if (path < num_paths)
   {
      if (source == AV_DEMUX)
      {
         av_paths_status[av_path].demux = param & 0xff;
         av_paths_status[av_path].param = ((param & 0xff0000) >> 16);
      }
   }

   FUNCTION_FINISH(STB_AVSetAudioSource);
}


/**
 * @brief   Sets the video surface  with the given video decoder path
 * @param   path video path
 * @param   video surface point
 * @return  TRUE if the codec is supported and is set correctly, FALSE otherwise
 */
BOOLEAN STB_AVSetSurface(U8BIT path, void *surface)
{
   BOOLEAN success = TRUE;
   U8BIT av_path = STB_AVGetPath(path, INVALID_RES_ID);

   VID_DBG("set surface %d:[%d:-] [%p]", av_path, path, surface);
   if (av_path == INVALID_RES_ID) {
      VID_DBG("get av_path error video codec path=%u av_path = %u", path, av_path);
      return FALSE;
   }
   FUNCTION_START(STB_AVSetSurface);
   if (video_surface[av_path] != surface) {
      video_surface[av_path] = surface;
      {
        int ret;
        AML_MP_PLAYER player_handle;
        pthread_rwlock_t* _l = STB_AVGetLockByPath(path);
        if (_l == NULL) {
            VID_DBG("Can't get lock, video decoder[%d]", path);
            return FALSE;
        }

        pthread_rwlock_rdlock(_l);
        ret = AV_GetPlayerHandleByPath_l(av_paths_status[av_path].video_decoder, av_paths_status[av_path].audio_decoder, &player_handle, FALSE);
        if (ret == 0) {
            ret = Aml_MP_Player_SetParameter(player_handle, AML_MP_PLAYER_PARAMETER_SURFACE_HANDLE, surface ? surface : (void*)-1);
            if (ret < 0)
            {
                AV_DBG("Failed to set parameter, err:%d", ret);
                success = FALSE;
            }
            else
            {
                AV_DBG("set AML MP surface %d:[%d:%d]:[%p] = %d, player[0x%p]",
                   av_path,
                   av_paths_status[av_path].video_decoder,
                   av_paths_status[av_path].audio_decoder,
                   surface,
                   ret,
                   player_handle);
            }
         }
         else
         {
            AV_DBG("failed to get player handle, %d:[%d:%d]",
               av_path,
               av_paths_status[av_path].video_decoder,
               av_paths_status[av_path].audio_decoder);
        }
        pthread_rwlock_unlock(_l);
      }

      video_surface[av_path] = surface;
   }
   FUNCTION_FINISH(STB_AVSetSurface);

   return success;
}

/**
 * @brief   Whether the black screen when the device signal disappears
 * @param   path video path
 * @param   is_black TRUE is Black screen when the signal disappears, FALSE is still frame
 * @return  TRUE if the codec is supported and is set correctly, FALSE otherwise
 */
BOOLEAN STB_AVSetStillFrame(U8BIT path, BOOLEAN is_black)
{
    BOOLEAN success = TRUE;
    U8BIT av_path = STB_AVGetPath(path, INVALID_RES_ID);

    FUNCTION_START(STB_AVSetStillFrame);

    if (av_path == INVALID_RES_ID) {
        VID_DBG("get av_path error video codec path=%u av_path = %u", path, av_path);
        return FALSE;
    }

    int ret;
    AML_MP_PLAYER player_handle;

    pthread_rwlock_t* _l = STB_AVGetLockByPath(path);
    if (_l == NULL) {
        VID_DBG("Can't get lock, video decoder[%d]", path);
        return FALSE;
    }

    pthread_rwlock_rdlock(_l);
    ret = AV_GetPlayerHandleByPath_l(av_paths_status[av_path].video_decoder, av_paths_status[av_path].audio_decoder, &player_handle, FALSE);

    if (ret == 0) {
        ret = Aml_MP_Player_SetParameter(player_handle, AML_MP_PLAYER_PARAMETER_BLACK_OUT, &is_black);
        if (ret < 0)
        {
            AV_DBG("Failed to set parameter, err:%d", ret);
            success = FALSE;
        }
        else
        {
            AV_DBG("Set Still Frame %d:[%d:%d]:[%d] = %d, player[0x%p]",
                   av_path,
                   av_paths_status[av_path].video_decoder,
                   av_paths_status[av_path].audio_decoder,
                   is_black,
                   ret,
                   player_handle);
        }
    }
    else
    {
        AV_DBG("failed to get player handle, %d:[%d:%d]",
               av_path,
               av_paths_status[av_path].video_decoder,
               av_paths_status[av_path].audio_decoder);
        ret = AV_SetVideoColor(av_path, is_black, TRUE);
        if (ret <0)
            success = FALSE;
    }

    pthread_rwlock_unlock(_l);

    FUNCTION_FINISH(STB_AVSetStillFrame);

    return success;
}

#ifdef RDK_COMPILE
/**
 * @brief   Set video window for specific video decoder
 * @param   path video path
 * @param   x The coordinate of the left side of the window
 * @param   y The coordinate of the top of the window
 * @param   w Window width
 * @param   h Window height
 * @return  TRUE if set video window correctly, FALSE otherwise
 */
BOOLEAN STB_AVStoreVideoWindow(U8BIT path, U16BIT x, U16BIT y, U16BIT w, U16BIT h)
{
   BOOLEAN success = TRUE;
   FUNCTION_START(STB_AVStoreVideoWindow);

   if (INVALID_RES_ID == path)
   {
       path = 0;
   }

   window_rect_array[path].x=x;
   window_rect_array[path].y=y;
   window_rect_array[path].w=w;
   window_rect_array[path].h=h;
   VID_DBG("store rect:(%d,%d,%d,%d) for path:%d",x,y,w,h,path);
   FUNCTION_FINISH(STB_AVStoreVideoWindow);

   return success;
}
#endif

/**
 * @brief   Gets the video surface  with the given video decoder path
 * @param   path video path
 * @return  surface poniter
 */
void * STB_AVGetSurface(U8BIT path)
{
    U8BIT av_path = STB_AVGetPath(path, INVALID_RES_ID);

    VID_DBG("[%d:%d]=%p", av_path, path, video_surface[av_path]);
    if (av_path == INVALID_RES_ID) {
      VID_DBG("get av_path error video codec path=%u av_path = %u", path, av_path);
      return NULL;
    }
   return video_surface[av_path];
}

/**
 * \brief Set video window axis
 *
 * \param path video path
 * \param x
 * \param y
 * \param width
 * \param height
 *
 * @return TRUE if video window set correctly
 */
BOOLEAN STB_AVSetVideoWindow(U8BIT path, int x, int y, int width, int height)
{
    int ret;
    AML_MP_PLAYER player_handle;

    FUNCTION_START(STB_AVSetVideoWindow);

    U8BIT av_path = STB_AVGetPath(path, INVALID_RES_ID);
    if (av_path == INVALID_RES_ID) {
        VID_DBG("get av path error video codec path=%u, av_path=%u", path, av_path);
        return FALSE;
    }

    VID_DBG("[%d,%d,%d,%d]", x, y, width, height);

    pthread_rwlock_t* _l = STB_AVGetLockByPath(path);
    if (_l == NULL) {
        VID_DBG("Can't get lock, video decoder[%d]", path);
        return FALSE;
    }

    pthread_rwlock_rdlock(_l);
    ret = AV_GetPlayerHandleByPath_l(path, INVALID_RES_ID, &player_handle, FALSE);
    if (ret < 0) {
        VID_DBG("Cannot get player handle video[%d]", path);
        pthread_rwlock_unlock(_l);
        return FALSE;
    }

    ret = Aml_MP_Player_SetVideoWindow(player_handle, x, y, width, height);

    if (ret < 0) {
        VID_DBG("SetVideoWindow failed, err:%d", ret);
    }
    pthread_rwlock_unlock(_l);

    FUNCTION_FINISH(STB_AVSetVideoWindow);
    return TRUE;
}

/**
 * @brief   Sets the video codec to be used when decoding video with the given video decoder path
 * @param   path video path
 * @param   codec codec to be used
 * @return  TRUE if the codec is supported and is set correctly, FALSE otherwise
 */
BOOLEAN STB_AVSetVideoCodec(U8BIT path, E_STB_AV_VIDEO_CODEC codec)
{
    BOOLEAN success;

    FUNCTION_START(STB_AVSetVideoCodec);
    U8BIT av_path = STB_AVGetPath(path, INVALID_RES_ID);

    VID_DBG("video codec path=%u av_path = %u", path, av_path);
    if (av_path == INVALID_RES_ID) {
        VID_DBG("get av_path error video codec path=%u av_path = %u", path, av_path);
        return FALSE;
    }

    success = TRUE;

    switch (codec)
    {
        case AV_VIDEO_CODEC_AUTO:
            av_paths_status[av_path].video_format = AML_MP_CODEC_UNKNOWN;
            VID_DBG("AUTO");
            break;
        case AV_VIDEO_CODEC_H264:
            av_paths_status[av_path].video_format = AML_MP_VIDEO_CODEC_H264;
            VID_DBG("H264");
            break;
        case AV_VIDEO_CODEC_H265:
            av_paths_status[av_path].video_format = AML_MP_VIDEO_CODEC_HEVC;
            VID_DBG("H265");
            break;
        case AV_VIDEO_CODEC_MPEG1:
            av_paths_status[av_path].video_format = AML_MP_VIDEO_CODEC_MPEG12;
            VID_DBG("MPEG1");
            break;
        case AV_VIDEO_CODEC_MPEG2:
            av_paths_status[av_path].video_format = AML_MP_VIDEO_CODEC_MPEG12;
            VID_DBG("MPEG2");
            break;
        case AV_VIDEO_CODEC_VP9:
            av_paths_status[av_path].video_format = AML_MP_VIDEO_CODEC_VP9;
            VID_DBG("VP9");
            break;
        case AV_VIDEO_CODEC_AVS:
            av_paths_status[av_path].video_format = AML_MP_VIDEO_CODEC_AVS;
            VID_DBG("AVS");
            break;
        case AV_VIDEO_CODEC_AVS2:
            av_paths_status[av_path].video_format = AML_MP_VIDEO_CODEC_AVS2;
            VID_DBG("AVS2");
            break;
        case AV_VIDEO_CODEC_MPEG4:
            av_paths_status[av_path].video_format = AML_MP_VIDEO_CODEC_MPEG4;
            VID_DBG("MPEG4");
            break;
        case AV_VIDEO_CODEC_DVES_AVC:
            av_paths_status[av_path].video_format = AML_MP_VIDEO_CODEC_DVES_AVC;
            VID_DBG("DVES AVC");
            break;
        case AV_VIDEO_CODEC_DVES_HEVC:
            av_paths_status[av_path].video_format = AML_MP_VIDEO_CODEC_DVES_HEVC;
            VID_DBG("DVES HEVC");
            break;
        default:
            VID_DBG("Unrecognised video codec %u", codec);
            success = FALSE;
        break;
    }

    FUNCTION_FINISH(STB_AVSetVideoCodec);

    return success;
}

/**
 * @brief   Sets the audio codec to be used when decoding audio with the given audio decoder path
 * @param   path audio path
 * @param   codec codec to be used
 * @return  TRUE if the codec is supported and is set correctly, FALSE otherwise
 */
BOOLEAN STB_AVSetAudioCodec(U8BIT path, E_STB_AV_AUDIO_CODEC codec)
{
   BOOLEAN success;

   FUNCTION_START(STB_AVSetAudioCodec);
   U8BIT av_path = STB_AVGetPath(INVALID_RES_ID, path);

   AUD_DBG("audio codec path=%u av_path = %u", path, av_path);
   if (av_path == INVALID_RES_ID) {
      AUD_DBG("get av_path error audio codec path=%u av_path = %u", path, av_path);
      return FALSE;
   }

   success = TRUE;

   switch (codec)
   {
   case AV_AUDIO_CODEC_AC3:
      av_paths_status[av_path].audio_format = AML_MP_AUDIO_CODEC_AC3;
      AUD_DBG("AC3");
      break;
   case AV_AUDIO_CODEC_EAC3:
      av_paths_status[av_path].audio_format = AML_MP_AUDIO_CODEC_EAC3;
      AUD_DBG("E-AC3");
      break;
    case AV_AUDIO_CODEC_AC4:
        av_paths_status[av_path].audio_format = AML_MP_AUDIO_CODEC_AC4;
        AUD_DBG("AC4");
        break;
   case AV_AUDIO_CODEC_AAC:
      av_paths_status[av_path].audio_format = AML_MP_AUDIO_CODEC_AAC;
      AUD_DBG("AAC");
      break;
   case AV_AUDIO_CODEC_HEAAC:
   case AV_AUDIO_CODEC_HEAACV2:
      av_paths_status[av_path].audio_format = AML_MP_AUDIO_CODEC_LATM;
      AUD_DBG("LATM");
      break;
      //case AV_AUDIO_CODEC_AUTO:
   case AV_AUDIO_CODEC_MP2:
      av_paths_status[av_path].audio_format = AML_MP_AUDIO_CODEC_MP2;
      AUD_DBG("MPEG");
      break;
   case AV_AUDIO_CODEC_MP3:
      av_paths_status[av_path].audio_format = AML_MP_AUDIO_CODEC_MP3;
      AUD_DBG("MPEG");
      break;
   default:
      AUD_DBG("Unrecognised audio codec %u", codec);
      success = FALSE;
      break;
   }

   FUNCTION_FINISH(STB_AVSetAudioCodec);

   return success;
}

/**
 * @brief   Loads an audio sample for subsequent playback
 * @param   path the decoder path to use for playback
 * @param   data the audio sample data to be loaded
 * @param   size the size of the audio sample in bytes
 * @return  E_HW_STATUS code
 */
E_HW_STATUS STB_AVLoadAudioSample(U8BIT path, U8BIT *data, U32BIT size)
{
   FUNCTION_START(STB_AVLoadAudioSample);
   U8BIT av_path = STB_AVGetPath(INVALID_RES_ID, path);

   AUD_DBG("audio codec path=%u av_path = %u", path, av_path);
   if (av_path == INVALID_RES_ID) {
      AUD_DBG("get av_path error audio codec path=%u av_path = %u", path, av_path);
      return HW_OK;
   }

   if (av_paths_status[av_path].sample_data != NULL)
   {
      STB_MEMFreeSysRAM(av_paths_status[av_path].sample_data);
      av_paths_status[av_path].sample_data = NULL;
      av_paths_status[av_path].sample_data_size = 0;
   }

   av_paths_status[av_path].sample_data = (U8BIT*)STB_MEMGetSysRAM(size);
   if (av_paths_status[av_path].sample_data != NULL)
   {
      memcpy(av_paths_status[av_path].sample_data,data,size);
      av_paths_status[av_path].sample_data_size = size;
   }
   FUNCTION_FINISH(STB_AVLoadAudioSample);

   return HW_OK;
}

/**
 * @brief   Plays back a previously loaded audio sample
 * @param   path the audio path to use for playback
 * @param   loop_count the number of times to play the sample, 0=forever
 * @return  E_HW_STATUS code
 */
E_HW_STATUS STB_AVPlayAudioSample(U8BIT path, U32BIT loop_count)
{
   //AM_ErrorCode_t retval;
   E_HW_STATUS success = HW_GEN_ERROR;

   FUNCTION_START(STB_AVPlayAudioSample);
 #if 0
   if (av_paths_status[path].sample_data_size > 0)
   {
      if ( AM_AV_SetTSSource(path, AM_AV_TS_SRC_HIU) == AM_SUCCESS)
      {
         retval = AM_AV_StartAudioESData(path, AFORMAT_MPEG, av_paths_status[path].sample_data,
                          av_paths_status[path].sample_data_size,loop_count);
         if (retval == AM_SUCCESS)
         {
           success = HW_OK;
         }
         else
         {
            ERR_DBG("AM_AV_StartAudioESData failed, error: %d",retval-AM_AV_ERROR_BASE);
         }
      }
   }
 #endif
   FUNCTION_FINISH(STB_AVPlayAudioSample);

   return success;
}

/**
 * @brief   Pauses playback of an audio sample
 * @param   path Audio path on which to pause
 * @return  E_HW_STATUS code
 */
E_HW_STATUS STB_AVPauseAudioSample(U8BIT path)
{
   FUNCTION_START(STB_AVPauseAudioSample);
   USE_UNWANTED_PARAM(path);
   FUNCTION_FINISH(STB_AVPauseAudioSample);

   return HW_GEN_ERROR;
}

/**
 * @brief   Resumes playback of an audio sample
 * @param   path Audio path on which to resume
 * @return  E_HW_STATUS code
 */
E_HW_STATUS STB_AVResumeAudioSample(U8BIT path)
{
   FUNCTION_START(STB_AVResumeAudioSample);
   USE_UNWANTED_PARAM(path);
   FUNCTION_FINISH(STB_AVResumeAudioSample);

   return HW_GEN_ERROR;
}

/**
 * @brief   Stops playback of an audio sample
 * @param   path Audio path on which to stop
 */
void STB_AVStopAudioSample(U8BIT path)
{
   FUNCTION_START(STB_AVStopAudioSample);
#if 0
   AM_AV_StopAudioES(path);
   AM_AV_SetTSSource(path, AM_AV_TS_SRC_DMX0);
#endif
   FUNCTION_FINISH(STB_AVStopAudioSample);
}

/**
 * @brief   Sets the codec to be used when decoding the next i-frame from memory
 * @param   path video path
 * @param   codec codec to be used
 * @return  TRUE if the codec is supported and is set correctly, FALSE otherwise
 */
BOOLEAN STB_AVSetIFrameCodec(U8BIT path, E_STB_AV_VIDEO_CODEC codec)
{
   BOOLEAN supported;
   FUNCTION_START(STB_AVSetIFrameCodec);
   U8BIT av_path = STB_AVGetPath(path, INVALID_RES_ID);

   VID_DBG("video codec path=%u av_path = %u", path, av_path);
   if (av_path == INVALID_RES_ID) {
      VID_DBG("get av_path error video codec path=%u av_path = %u", path, av_path);
      return FALSE;
   }

   switch (codec)
   {
      case AV_VIDEO_CODEC_MPEG1:
      case AV_VIDEO_CODEC_MPEG2:
      case AV_VIDEO_CODEC_H264:
         supported = TRUE;
         av_paths_status[av_path].iframe_codec = codec;
         break;
      default:
         supported = FALSE;
   }

   FUNCTION_FINISH(STB_AVSetIFrameCodec);
   return(supported);
}

/**
 * @brief   Loads a video I Frame for subsequent decode and display
 * @param   path the video decode path to be used
 * @param   data the I frame data to be loaded
 * @param   size the size of the data in bytes
 */
void STB_AVLoadIFrame(U8BIT path, U8BIT *data, U32BIT size)
{
   FUNCTION_START(STB_AVLoadIFrame);
   U8BIT av_path = STB_AVGetPath(path, INVALID_RES_ID);

   VID_DBG("video codec path=%u av_path = %u", path, av_path);
   if (av_path == INVALID_RES_ID) {
      VID_DBG("get av_path error video codec path=%u av_path = %u", path, av_path);
      return;
   }

   if (av_paths_status[av_path].iframe_data != NULL)
   {
      STB_MEMFreeSysRAM(av_paths_status[av_path].iframe_data);
      av_paths_status[av_path].iframe_data = NULL;
   }

   if (size != 0)
   {
      av_paths_status[av_path].iframe_data = (U8BIT*)STB_MEMGetSysRAM(size);
      if (av_paths_status[av_path].iframe_data != NULL)
      {
         VID_DBG("buffering %lu byte iframe",size);
         av_paths_status[av_path].iframe_data_size = size;
         memcpy(av_paths_status[av_path].iframe_data,data,size);
      }
      else
      {
         VID_DBG("failed to allocate memory for iframe");
      }
   }

   FUNCTION_FINISH(STB_AVLoadIFrame);
}

/**
 * @brief   Decode and display previously loaded I frame data
 * @param   path the video path to use
 */
void STB_AVShowIFrame(U8BIT path)
{
   int result;
   Aml_MP_CodecID codec;

   FUNCTION_START(STB_AVShowIFrame);
#if 0
   if (av_paths_status[path].iframe_data != NULL)
   {
      switch (av_paths_status[path].iframe_codec)
      {
         case AV_VIDEO_CODEC_MPEG1:
            codec = AV_VIDEO_CODEC_MPEG1_TSP;
            break;
         case AV_VIDEO_CODEC_MPEG2:
            codec = AV_VIDEO_CODEC_MPEG2_TSP;
            break;
         case AV_VIDEO_CODEC_H264:
            codec = AV_VIDEO_CODEC_H264_TSP;
            break;
         default:
            codec = AV_VIDEO_CODEC_AUTO_TSP;
            break;
      }

      VID_DBG("path=%u", path);

      if (codec <= AV_VIDEO_CODEC_VP9)
      {
         /* Subscribe to the event that indicates when the end of the data has been seen,
          * which should also be when the iframe has finished decoding and can be displayed */
         AM_EVT_Subscribe(path, AM_AV_EVT_VIDEO_ES_END, AVEventHandler, &av_paths_status[path]);

         result = AM_AV_StartVideoESData(path, codec, av_paths_status[path].iframe_data,
                                          av_paths_status[path].iframe_data_size);
         if (result == AM_SUCCESS)
         {
            VID_DBG("showing iframe");
         }
         else
         {
            VID_DBG(" iframe play failed, error %d", result-AM_AV_ERROR_BASE);
         }
      }
   }
#endif
   FUNCTION_FINISH(STB_AVShowIFrame);
}

/**
 * @brief   Hides a previously shown I frame
 * @param   path the video path containing the I frame
 */
void STB_AVHideIFrame(U8BIT path)
{
   FUNCTION_START(STB_AVHideIFrame);

   VID_DBG("path=%u", path);
#if 0
   if (av_paths_status[path].iframe_shown)
   {
      AM_AV_DisableVideo(path);
      AM_AV_StopVideoES(path);
      av_paths_status[path].iframe_shown = FALSE;
   }

   if (av_paths_status[path].iframe_data != NULL)
   {
      AM_EVT_Unsubscribe(path, AM_AV_EVT_VIDEO_ES_END, AVEventHandler, &av_paths_status[path]);
   }
#endif
   FUNCTION_FINISH(STB_AVHideIFrame);
}

/**
 * @brief   Returns minimum video play speed as a percentage.
 * @param   video_decoder video decoder path
 * @return  Minimum play speed.
 */
S16BIT STB_AVGetMinPlaySpeed(U8BIT path)
{
   FUNCTION_START(STB_AVGetMinPlaySpeed);
   USE_UNWANTED_PARAM(path);
   FUNCTION_FINISH(STB_AVGetMinPlaySpeed);

   return(MIN_AV_SPEED);
}

/**
 * @brief   Returns maximum video play speed as a percentage.
 * @param   video_decoder video decoder path
 * @return  Maximum play speed.
 */
S16BIT STB_AVGetMaxPlaySpeed(U8BIT path)
{
   FUNCTION_START(STB_AVGetMinPlaySpeed);
   USE_UNWANTED_PARAM(path);
   FUNCTION_FINISH(STB_AVGetMinPlaySpeed);

   return(MAX_AV_SPEED);
}

/**
 * @brief   Returns the next valid speed that is +/- inc above or below the
 *          given speed. Slow motion speeds (>-100% and < 100%) can be included.
 * @param   path Decode path
 * @param   speed Percentage speed above/below which the new speed is calculated
 * @param   inc number of speeds above that specified to return
 * @param   include_slow_speeds selects whether speeds >-100% and <100% are included
 * @return  Speed as a percentage
 */
S16BIT STB_AVGetNextPlaySpeed(U8BIT path, S16BIT speed, S16BIT inc, BOOLEAN include_slow_speeds)
{
   S16BIT new_speed;

   FUNCTION_START(STB_AVGetNextPlaySpeed);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(include_slow_speeds);

   new_speed = speed;

   if ((speed >= MIN_AV_SPEED) && (speed <= MAX_AV_SPEED))
   {
      if (inc > 0)
      {
         switch (speed)
         {
            case -600:
               new_speed = -400;
               break;
            case -400:
               new_speed = -100;
               break;
            case -100:
            case 0:
               new_speed = 100;
               break;
            case 100:
               new_speed = 400;
               break;
            case 400:
               new_speed = 600;
               break;
            default:
               new_speed = speed;
               break;
         }
      }
      else if (inc < 0)
      {
         switch (speed)
         {
            case 600:
               new_speed = 400;
               break;
            case 400:
               new_speed = 100;
               break;
            case 100:
            case 0:
               new_speed = -100;
               break;
            case -100:
               new_speed = -400;
               break;
            case -400:
               new_speed = -600;
               break;
            default:
               new_speed = speed;
               break;
         }
      }
   }

   FUNCTION_FINISH(STB_AVGetNextPlaySpeed);

   return(new_speed);
}

/**
 * @brief     Sets the source of the audio decoder
 * @param     path - audio path to configure
 * @param     source - the source device to use
 * @param     param - tuner or demux number
 */
void STB_AVSetADSource(U8BIT path, E_STB_AV_DECODE_SOURCE source, U32BIT param)
{
   FUNCTION_START(STB_AVSetADSource);
   AV_DBG("path=%u, source=%u, param=%u", path, source, param);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(source);
   USE_UNWANTED_PARAM(param);
   FUNCTION_FINISH(STB_AVSetADSource);
}

/**
 * @brief   Configures the audio description channel mode (stereo/left/right) in the case where
 *          dual-mono audio is used, such that only the audio from one channel is heard.
 * @param   path audio path to be configured
 * @param   mode audio mode to use
 */
void STB_AVChangeADMode(U8BIT path, E_STB_AV_AUDIO_MODE mode)
{
   FUNCTION_START(STB_AVChangeADMode);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(mode);
   FUNCTION_FINISH(STB_AVChangeADMode);
}

/**
 * @brief   Starts decoding audio description on the given audio path
 * @param   path audio decoder path to be started
 */
BOOLEAN STB_AVStartADDecoding(U8BIT path)
{
   BOOLEAN ret = TRUE;
   int err;
   AML_MP_PLAYER player_handle;
   Aml_MP_AudioParams ad_param;
   U16BIT video_pid, audio_pid, pcr_pid, ad_pid;
   U8BIT preselection_id;
   FUNCTION_START(STB_AVStartADDecoding);
   U8BIT av_path = STB_AVGetPath(INVALID_RES_ID, path);

   AUD_DBG("video codec path=%u av_path = %u", path, av_path);
   if (av_path == INVALID_RES_ID) {
      AUD_DBG("get av_path error video codec path=%u av_path = %u", path, av_path);
      return FALSE;
   }

    pthread_rwlock_t* _l = STB_AVGetLockByPath(path);
    if (_l == NULL) {
        AUD_DBG("Can't get lock, audio decoder[%d]", path);
        return FALSE;
    }

    pthread_rwlock_wrlock(_l);
    err = AV_GetPlayerHandleByPath_l(av_paths_status[av_path].video_decoder,
                                    av_paths_status[av_path].audio_decoder, &player_handle, TRUE);
    if (err < 0) {
        AUD_DBG("Cannot get player handle av_path[%d]", av_path);
        pthread_rwlock_unlock(_l);
        return FALSE;
    }

   if (STB_PVRIsPlayStopped(av_paths_status[av_path].audio_decoder, av_paths_status[av_path].video_decoder))
   {
      DMXGetDecodePIDs(av_paths_status[av_path].demux, &pcr_pid, &video_pid, &audio_pid, &ad_pid, &preselection_id);
      if (ad_pid != 0 && ad_pid != INVALID_PID && ad_pid != av_paths_status[av_path].ad_pid) {
          memset(&ad_param, 0, sizeof(ad_param));
          ad_param.pid = ad_pid;
          ad_param.audioCodec = av_paths_status[path].ad_format;
          err = Aml_MP_Player_SetADParams(player_handle, &ad_param);
          if (err < 0) {
              ret = FALSE;
              AUD_DBG("Set AD Param err:%d, pid[%d] fmt[%d]", err, ad_pid, av_paths_status[av_path].ad_format);
          }

          err = Aml_MP_Player_StartADDecoding(player_handle);
          if (err < 0) {
              AUD_DBG("Enable AD err:%d", err);
              ret = FALSE;;
          } else {
              AUD_DBG("Start AD decoding ok, pid[%d] fmt[%d]", ad_pid, av_paths_status[av_path].ad_format);
              av_paths_status[av_path].ad_pid = ad_pid;
          }
      } else {
          AUD_DBG("Don't need call AD start actually, ad_pid[%d], current ad_pid[%d]", ad_pid, av_paths_status[av_path].ad_pid);
      }
   }
   else
   {
      Aml_MP_CodecID video_format;
      Aml_MP_CodecID audio_format;
      Aml_MP_CodecID ad_format;

      DMXGetDecodePIDs(av_paths_status[av_path].demux, &pcr_pid, &video_pid, &audio_pid, &ad_pid, &preselection_id);
      audio_format = av_paths_status[av_path].audio_format;
      ad_format = av_paths_status[av_path].ad_format;
      video_format = av_paths_status[av_path].video_format;

      AUD_DBG("av-pvr: path=%u, adpid:%d vpid:%d", av_path, ad_pid, video_pid);

      if (ad_pid != 0 && ad_pid != INVALID_PID)
      {
        ret = PVRChangeDecodePIDs(av_paths_status[av_path].audio_decoder,
                                    av_paths_status[av_path].video_decoder, pcr_pid, video_pid, audio_pid, ad_pid,
                                    toVideoCodec(video_format), toAudioCodec(audio_format), toAudioCodec(ad_format), preselection_id);

        if (ret) {
            AUD_DBG("Start AD decoding ok");
            av_paths_status[av_path].ad_pid = ad_pid;
        } else {
            AUD_DBG("Start AD decoding fail");
        }
      }
   }
   pthread_rwlock_unlock(_l);

   FUNCTION_FINISH(STB_AVStartADDecoding);
   return ret;
}

/**
 * @brief   Stops decoding audio description on the given audio path.
 * @param   path audio decoder path to be stopped
 */
void STB_AVStopADDecoding(U8BIT path)
{
    int ret;
    Aml_MP_AudioParams ad_param;
    FUNCTION_START(STB_AVStopADDecoding);
    U8BIT av_path = STB_AVGetPath(INVALID_RES_ID, path);
    AML_MP_PLAYER player_handle;

   AUD_DBG("audiocodec path=%u av_path = %u", path, av_path);
   if (av_path == INVALID_RES_ID) {
      AUD_DBG("get av_path error audio codec path=%u av_path = %u", path, av_path);
      return;
   }

    pthread_rwlock_t* _l = STB_AVGetLockByPath(path);
    if (_l == NULL) {
        AUD_DBG("Can't get lock, audio decoder[%d]", path);
        return;
    }

    pthread_rwlock_wrlock(_l);
    ret = AV_GetPlayerHandleByPath_l(av_paths_status[av_path].video_decoder, av_paths_status[av_path].audio_decoder, &player_handle, FALSE);
    if (ret < 0) {
        AUD_DBG("Cannot get player handle[%d], av_path[%d]", path, av_path);
        pthread_rwlock_unlock(_l);
        return;
    }

   if (STB_PVRIsPlayStopped(av_paths_status[av_path].audio_decoder, av_paths_status[av_path].video_decoder))
   {
      if (IS_INVALID_PLAYER_HANDLE(av_path))
      {
         AUD_DBG("Invalid player handle");
         pthread_rwlock_unlock(_l);
         return;
      }

      ret = Aml_MP_Player_StopADDecoding(av_paths_status[av_path].player_handle);
      if (ret < 0) {
          AUD_DBG("Stop AD decoding err:%d", ret);
      }else {
          AUD_DBG("Stop AD decoding ok");
          av_paths_status[av_path].ad_pid = INVALID_PID;
      }
      if (AV_GetDecoderState_l(player_handle, AUDIO_DECODER) == DECODER_STATE_STOPPED &&
            AV_GetDecoderState_l(player_handle, VIDEO_DECODER) == DECODER_STATE_STOPPED)
      {
          AUD_DBG("NOW:A_STOP_V_STOP, ReleaseTsPlayer");
          AV_ReleaseTsPlayer_l(av_path);
      }
   }
   else
   {
      Aml_MP_CodecID video_format;
      Aml_MP_CodecID audio_format;
      Aml_MP_CodecID ad_format;
      U16BIT video_pid, audio_pid, pcr_pid, ad_pid;
      U8BIT preselection_id;

      DMXGetDecodePIDs(av_paths_status[av_path].demux, &pcr_pid, &video_pid, &audio_pid, &ad_pid, &preselection_id);
      audio_format = av_paths_status[av_path].audio_format;
      video_format = av_paths_status[av_path].video_format;
      ad_format = AML_MP_CODEC_UNKNOWN;
      ad_pid = INVALID_PID;

      AUD_DBG("av-pvr: path=%u, adpid:%d vpid:%d", path, audio_pid, video_pid);

      {
        ret = PVRChangeDecodePIDs(av_paths_status[av_path].audio_decoder,
                                    av_paths_status[av_path].video_decoder, pcr_pid, video_pid, audio_pid, ad_pid,
                                    toVideoCodec(video_format), toAudioCodec(audio_format), toAudioCodec(ad_format), preselection_id);

        if (ret) {
            AUD_DBG("Stop AD decoding ok");
            av_paths_status[av_path].ad_pid = INVALID_PID;
        } else {
            AUD_DBG("Stop AD decoding fail");
        }
      }
   }
   pthread_rwlock_unlock(_l);
   FUNCTION_FINISH(STB_AVStopADDecoding);
}

/**
 * @brief   Sets the codec to be used for audio description when decoding audio
 *          with the given audio decoder path
 * @param   path audio path
 * @param   codec codec to be used
 * @return  TRUE if the codec is supported and is set correctly, FALSE otherwise
 */
BOOLEAN STB_AVSetADCodec(U8BIT path, E_STB_AV_AUDIO_CODEC codec)
{
   BOOLEAN success;

   FUNCTION_START(STB_AVSetADCodec);
   U8BIT av_path = STB_AVGetPath(INVALID_RES_ID, path);

   AUD_DBG("audiocodec path=%u av_path = %u", path, av_path);
   if (av_path == INVALID_RES_ID) {
      AUD_DBG("get av_path error audio codec path=%u av_path = %u", path, av_path);
      return FALSE;
   }

   success = TRUE;

   switch (codec)
   {
   case AV_AUDIO_CODEC_AC3:
      AV_DBG("AD Codec[%d]: AC3", codec);
      av_paths_status[av_path].ad_format = AML_MP_AUDIO_CODEC_AC3;
      break;
   case AV_AUDIO_CODEC_EAC3:
      AV_DBG("AD Codec[%d]: EAC3", codec);
      av_paths_status[av_path].ad_format = AML_MP_AUDIO_CODEC_EAC3;
      break;
    case AV_AUDIO_CODEC_AC4:
        AV_DBG("AD Codec[%d]: AC4", codec);
        av_paths_status[av_path].ad_format = AML_MP_AUDIO_CODEC_AC4;
        break;
   case AV_AUDIO_CODEC_AAC:
      AV_DBG("AD Codec[%d]: AAC", codec);
      av_paths_status[av_path].ad_format = AML_MP_AUDIO_CODEC_AAC;
      break;
   case AV_AUDIO_CODEC_HEAAC:
   case AV_AUDIO_CODEC_HEAACV2:
      AV_DBG("AD Codec[%d]: HEAAC/HEAACV2", codec);
      av_paths_status[av_path].ad_format = AML_MP_AUDIO_CODEC_LATM;
      break;
   case AV_AUDIO_CODEC_MP2 :
      AV_DBG("AD Codec[%d]: MP2", codec);
      av_paths_status[av_path].ad_format = AML_MP_AUDIO_CODEC_MP2;
      break;
   case AV_AUDIO_CODEC_MP3 :
      AV_DBG("AD Codec[%d]: MP3", codec);
      av_paths_status[av_path].ad_format = AML_MP_AUDIO_CODEC_MP3;
      break;
   case AV_AUDIO_CODEC_AUTO :
   default:
      AV_DBG("AD Codec[%d]: AUTO/OTHER", codec);
      av_paths_status[av_path].ad_format = AML_MP_AUDIO_CODEC_MP2;
      success = FALSE;
      break;
   }

   FUNCTION_FINISH(STB_AVSetADCodec);

   return(success);
}

/**
 * @brief   Sets the volume of the audio description output
 * @param   path audio path to be configured
 * @param   vol ad volume (0-100%)
 */
void STB_AVSetADVolume(U8BIT path, U8BIT vol)
{
   int ret;
   AML_MP_PLAYER player_handle;
   FUNCTION_START(STB_AVSetADVolume);

   U8BIT av_path = STB_AVGetPath(INVALID_RES_ID, path);
   AUD_DBG("audio codec path=%u av_path = %u", path, av_path);

   if (av_path == INVALID_RES_ID) {
      AUD_DBG("get av_path error audio codec path=%u av_path = %u", path, av_path);
      return;
   }

    pthread_rwlock_t* _l = STB_AVGetLockByPath(path);
    if (_l == NULL) {
        AUD_DBG("Can't get lock, audio decoder[%d]", path);
        return;
    }

    pthread_rwlock_rdlock(_l);
    ret = AV_GetPlayerHandleByPath_l(INVALID_RES_ID, path, &player_handle, FALSE);
    if (ret < 0) {
        AUD_DBG("Cannot get player handle[%d]", av_path);
        pthread_rwlock_unlock(_l);
        return;
    }

    ret = Aml_MP_Player_SetADVolume(player_handle, vol);
    AUD_DBG("SetADVolume ad path[%d] vol[%d] err:%d, av_path: %d", path,  vol, ret, av_path);
    pthread_rwlock_unlock(_l);

   FUNCTION_FINISH(STB_AVSetADVolume);
}

/**
 * @brief   Gets the current volume of the audio description output
 * @param   path The audio path to query
 * @return  ad volume (0-100%)
 */
U8BIT STB_AVGetADVolume(U8BIT path)
{
   int ret;
   AML_MP_PLAYER player_handle;
   FUNCTION_START(STB_AVGetADVolume);

    pthread_rwlock_t* _l = STB_AVGetLockByPath(path);
    if (_l == NULL) {
        AUD_DBG("Can't get lock, audio decoder[%d]", path);
        return 0;
    }

    pthread_rwlock_rdlock(_l);
    ret = AV_GetPlayerHandleByPath_l(INVALID_RES_ID, path, &player_handle, FALSE);
    if (ret < 0) {
        AUD_DBG("Cannot get player handle audio[%d]", path);
        pthread_rwlock_unlock(_l);
        return 0;
   }

    float vol = 0;
    ret = Aml_MP_Player_GetADVolume(player_handle, &vol);
    AUD_DBG("GetADVolume vol:%f, err:%d", vol, ret);
    pthread_rwlock_unlock(_l);

   FUNCTION_FINISH(STB_AVGetADVolume);
   return vol;
}

/**
 * @brief   Sets the mix level of the audio description output
 * @param   path audio path to be configured
 * @param   vol ad mixer level (0-100%)
 */
void STB_AVSetADMixLevel(U8BIT path, U8BIT vol)
{
   FUNCTION_START(STB_AVSetADMixLevel);

   int ret;
   AML_MP_PLAYER player_handle;
   U8BIT av_path = STB_AVGetPath(INVALID_RES_ID, path);
   Aml_MP_ADVolume ad_volume;

   AUD_DBG("audio codec path=%u av_path = %u, vol = %d", path, av_path, vol);
   if (av_path == INVALID_RES_ID) {
       AUD_DBG("get av_path error audio codec path=%u av_path = %u", path, av_path);
       return;
   }

    if (vol > 100) {
        vol = 100;
    }
    ad_volume.masterVolume = (int)(100 - vol);
    ad_volume.slaveVolume = (int)vol;

    pthread_rwlock_t* _l = STB_AVGetLockByPath(path);
    if (_l == NULL) {
        AUD_DBG("Can't get lock, audio decoder[%d]", path);
        return;
    }

    pthread_rwlock_rdlock(_l);
    ret = AV_GetPlayerHandleByPath_l(av_paths_status[av_path].video_decoder, av_paths_status[av_path].audio_decoder, &player_handle, FALSE);
    if (ret < 0) {
        AUD_DBG("Cannot get player handle[%d]", av_path);
        pthread_rwlock_unlock(_l);
        return;
    }

    ret = Aml_MP_Player_SetParameter(player_handle, AML_MP_PLAYER_PARAMETER_AD_MIX_LEVEL, (void*)&ad_volume);
    AUD_DBG("SetADMixLevel ad path[%d] vol[%d] err:%d, av_path: %d", path,  vol, ret, av_path);
    pthread_rwlock_unlock(_l);

   FUNCTION_FINISH(STB_AVSetADMixLevel);
}

/**
 * @brief   Gets the mix level of the audio description output
 * @param   path The audio path to query
 * @return  ad mixer level (0-100%)
 */
U8BIT STB_AVGetADMixLevel(U8BIT path)
{
    int ret;
    AML_MP_PLAYER player_handle;
    FUNCTION_START(STB_AVGetADMixLevel);

    U8BIT av_path = STB_AVGetPath(INVALID_RES_ID, path);
    Aml_MP_ADVolume ad_mix_level;
    ad_mix_level.masterVolume = 0;
    ad_mix_level.slaveVolume = 0;

    pthread_rwlock_t* _l = STB_AVGetLockByPath(path);
    if (_l == NULL) {
        AUD_DBG("Can't get lock, audio decoder[%d]", path);
        return 0;
    }

    pthread_rwlock_rdlock(_l);
    ret = AV_GetPlayerHandleByPath_l(INVALID_RES_ID, path, &player_handle, FALSE);
    if (ret < 0) {
        AUD_DBG("Cannot get player handle audio[%d]", path);
        pthread_rwlock_unlock(_l);
        return 0;
    }

    ret = Aml_MP_Player_GetParameter(player_handle, AML_MP_PLAYER_PARAMETER_AD_MIX_LEVEL, &ad_mix_level);
    AUD_DBG("GetADMixLevel level:%d, err:%d", ad_mix_level.slaveVolume, ret);
    pthread_rwlock_unlock(_l);

    FUNCTION_FINISH(STB_AVGetADMixLevel);
    return ad_mix_level.slaveVolume;
}

/**
 * @brief   Sets the standby state of the HDMI output
 * @param   standby TRUE to put the HDMI in standby, FALSE to come out of standby
 */
void STB_AVSetHDMIStandby(BOOLEAN standby)
{
   FUNCTION_START(STB_AVSetHDMIStandby);
   USE_UNWANTED_PARAM(standby);
   FUNCTION_FINISH(STB_AVSetHDMIStandby);
}

/**
 * @brief   Returns the resolutions supported by the HDMI
 * @param   modes array of supported modes
 * @return  number of supported modes
 */
U8BIT STB_AVGetHDMISupportedModes(E_STB_AV_VIDEO_FORMAT **modes)
{
#if 0
   AM_VOUT_Format_t am_format;
   U8BIT num_modes;
   AM_ErrorCode_t err;
   U16BIT i;

   FUNCTION_START(STB_AVGetHDMISupportedModes);

   num_modes = 0;

   if ((err = AM_VOUT_GetFormat(VOUT_DEV, &am_format)) == AM_SUCCESS)
   {
      for (i = 0; i < sizeof(video_modes) / sizeof(video_modes[0]); i++)
      {
         if (video_modes[i].am_format == am_format)
         {
            *modes = &(video_modes[i].format);
            num_modes = 1;
            AV_DBG("mode %u", **modes);
            break;
         }
      }
   }
   else
   {
      ERR_DBG("AM_VOUT_GetFormat failed, err %d", err);
   }
#else
   U16BIT i;
   U8BIT num_modes = 0;
   HW_AM_VOUT_Format_t am_format = HW_AM_VOUT_FORMAT_720P;

   for (i = 0; i < sizeof(video_modes) / sizeof(video_modes[0]); i++)
   {
      if (video_modes[i].am_format == am_format)
      {
         *modes = &(video_modes[i].format);
         num_modes = 1;
         AV_DBG("mode %u", **modes);
         break;
      }
   }
#endif
   FUNCTION_FINISH(STB_AVGetHDMISupportedModes);

   return (num_modes);
}


/**
 * @brief   Enables AV output to HDMI
 */
void STB_AVEnableHDMIDecoding(void)
{
   //FUNCTION_START(STB_AVEnableHDMIDecoding);
   //FUNCTION_FINISH(STB_AVEnableHDMIDecoding);
}

/**
 * @brief   Disables AV output to HDMI
 */
void STB_AVDisableHDMIDecoding(void)
{
   // FUNCTION_START(STB_AVDisableHDMIDecoding);
   // FUNCTION_FINISH(STB_AVDisableHDMIDecoding);
}

/**
 * @brief   Returns whether HDCP has authenticated
 * @return  TRUE if authenticated, FALSE otherwise
 */
BOOLEAN STB_AVIsHDCPAuthenticated(void)
{
   FUNCTION_START(STB_AVIsHDCPAuthenticated);
   FUNCTION_FINISH(STB_AVIsHDCPAuthenticated);
   return (TRUE);
}

/**
 * @brief   Sets the audio delay on the given path
 * @param   path decoder path
 * @param   millisecond audio delay to be applied
 */
void STB_AVSetAudioDelay(U8BIT path, U16BIT millisecond)
{
   FUNCTION_START(STB_AVSetAudioDelay);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(millisecond);
   FUNCTION_FINISH(STB_AVSetAudioDelay);
}


/**
 * @brief   Sets the SPDIF output mode, PCM or compressed audio
 * @param   path decoder path
 * @param   audio_type PCM or compressed
 */
void STB_AVSetSpdifMode(U8BIT path, E_STB_DIGITAL_AUDIO_TYPE audio_type)
{
   FUNCTION_START(STB_AVSetSpdifMode);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(audio_type);
   FUNCTION_FINISH(STB_AVSetSpdifMode);
}

/**
 * @brief   Returns the current size of the screen in pixels
 * @param   path output path
 * @param   width returned width
 * @param   height returned height
 */
void STB_AVGetScreenSize(U8BIT path, U16BIT *width, U16BIT *height)
{
   FUNCTION_START(STB_AVGetScreenSize);
   U8BIT av_path = STB_AVGetPath(path, INVALID_RES_ID);

   VID_DBG("video codec path=%u av_path = %u", path, av_path);
   if (av_path == INVALID_RES_ID) {
      VID_DBG("get av_path error video codec path=%u av_path = %u", path, av_path);
      return;
   }

   if (width != NULL)
   {
      *width = av_paths_status[av_path].display_info.screen_width;
   }
   if (height != NULL)
   {
      *height = av_paths_status[av_path].display_info.screen_height;
   }

   FUNCTION_FINISH(STB_AVGetScreenSize);
}

/**
 * @brief   Sets the HDMI audio output mode, PCM or compressed
 * @param   path decoder path
 * @param   audio_type PCM or compressed
 */
void STB_AVSetHDMIAudioMode(U8BIT path, E_STB_DIGITAL_AUDIO_TYPE audio_type)
{
   FUNCTION_START(STB_AVSetHDMIAudioMode);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(audio_type);
   FUNCTION_FINISH(STB_AVSetHDMIAudioMode);
}

/**
 * @brief   Returns the native resolution, i.e. the resolution that is
 *          set with a call to STB_AVSetTVType(path, ratio, VIDEO_FORMAT_AUTO);
 * @param   width pointer to the variable where to store the width
 * @param   height pointer to the variable where to store the height
 */
void STB_AVGetHDMINativeResolution(U16BIT *width, U16BIT *height)
{
   FUNCTION_START(STB_AVGetHDMINativeResolution);
   USE_UNWANTED_PARAM(width);
   USE_UNWANTED_PARAM(height);
   FUNCTION_FINISH(STB_AVGetHDMINativeResolution);
}

/**
 * @brief   Apply System Renewability Message (SRM) to HDCP function
 * @param   path output path
 * @param   data SRM data
 * @param   len length of SRM data in bytes
 * @return  SRM_OK, SRM_BUSY or SRM_NOT_REQUIRED
 */
E_STB_AV_SRM_REPLY STB_AVApplySRM(U8BIT path, U8BIT *data, U32BIT len)
{
   FUNCTION_START(STB_AVApplySRM);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(data);
   USE_UNWANTED_PARAM(len);
   FUNCTION_FINISH(STB_AVApplySRM);

   return SRM_NOT_REQUIRED;
}

/**
 * @brief   Returns the frame rate of the video being decoded
 * @param   path video path
 * @return  video frame rate in frame per seconds
 */
U8BIT STB_AVGetVideoFrameRate(U8BIT path)
{
   int ret;
   AML_MP_PLAYER player_handle;
   U8BIT frame_rate = 0;
   FUNCTION_START(STB_AVGetVideoFrameRate);
   // VID_DBG("vpath:%u", path);

    pthread_rwlock_t* _l = STB_AVGetLockByPath(path);
    if (_l == NULL) {
        AUD_DBG("Can't get lock, video decoder[%d]", path);
        return 0;
    }

    pthread_rwlock_rdlock(_l);
    ret = AV_GetPlayerHandleByPath_l(path, INVALID_RES_ID, &player_handle, FALSE);
    if (ret == 0) {
        Aml_MP_VideoInfo info = {0};
        ret = Aml_MP_Player_GetParameter(player_handle, AML_MP_PLAYER_PARAMETER_VIDEO_INFO, &info);
        if (ret == 0) {
            frame_rate = (U8BIT)info.frameRate;
        }
    }
    pthread_rwlock_unlock(_l);

    FUNCTION_FINISH(STB_AVGetVideoFrameRate);
    return frame_rate;
}

/**
 * @brief   Returns the width of the video being decoded
 * @param   path video path
 * @return  video width in frame per seconds
 */
U32BIT STB_AVGetVideoWidth(U8BIT path)
{
    int ret;
    AML_MP_PLAYER player_handle;
    U32BIT width = 0;
    FUNCTION_START(STB_AVGetVideoWidth);
    VID_DBG("vpath:%u", path);

    pthread_rwlock_t* _l = STB_AVGetLockByPath(path);
    if (_l == NULL) {
        AUD_DBG("Can't get lock, video decoder[%d]", path);
        return 0;
    }

    pthread_rwlock_rdlock(_l);
    ret = AV_GetPlayerHandleByPath_l(path, INVALID_RES_ID, &player_handle, FALSE);
    if (ret == 0) {
        Aml_MP_VideoInfo info = {0};
        ret = Aml_MP_Player_GetParameter(player_handle, AML_MP_PLAYER_PARAMETER_VIDEO_INFO, &info);
        if (ret == 0) {
            width = info.width;
        }
    }
    pthread_rwlock_unlock(_l);

    FUNCTION_FINISH(STB_AVGetVideoWidth);
    return width;
}

/**
 * @brief   Returns the height of the video being decoded
 * @param   path video path
 * @return  video height in frame per seconds
 */
U32BIT STB_AVGetVideoHeight(U8BIT path)
{
    int ret;
    AML_MP_PLAYER player_handle;
    U32BIT height = 0;
    FUNCTION_START(STB_AVGetVideoHeight);
    VID_DBG("vpath:%u", path);

    pthread_rwlock_t* _l = STB_AVGetLockByPath(path);
    if (_l == NULL) {
        AUD_DBG("Can't get lock, video decoder[%d]", path);
        return 0;
    }

    pthread_rwlock_rdlock(_l);
    ret = AV_GetPlayerHandleByPath_l(path, INVALID_RES_ID, &player_handle, FALSE);
    if (ret == 0) {
        Aml_MP_VideoInfo info = {0};
        ret = Aml_MP_Player_GetParameter(player_handle, AML_MP_PLAYER_PARAMETER_VIDEO_INFO, &info);
        if (ret == 0) {
            height = info.height;
        }
    }
    pthread_rwlock_unlock(_l);

    FUNCTION_FINISH(STB_AVGetVideoHeight);
    return height;
}

/**
 * @brief   Returns the scan type of the video being decoded
 * @param   path video path
 * @return  1: progressive, 0: interlaced, 255: invalid
 */
U8BIT STB_AVGetVideoScanType(U8BIT path)
{
    int ret;
    AML_MP_PLAYER player_handle;
    U8BIT scan_type = 0;

    FUNCTION_START(STB_AVGetVideoScanType);

    // VID_DBG("vpath:%u", path);

    pthread_rwlock_t* _l = STB_AVGetLockByPath(path);
    if (_l == NULL) {
        AUD_DBG("Can't get lock, video decoder[%d]", path);
        return 0;
    }

    pthread_rwlock_rdlock(_l);
    ret = AV_GetPlayerHandleByPath_l(path, INVALID_RES_ID, &player_handle, FALSE);
    if (ret == 0) {
        Aml_MP_VdecStat info = {0};

        ret = Aml_MP_Player_GetParameter(player_handle, AML_MP_PLAYER_PARAMETER_VIDEO_DECODE_STAT, &info);
        if (ret == 0) {
            if ((info.vf_type & 0x01) == 0x01 ||
                    (info.vf_type & 0x03) == 0x03 ||
                    (info.vf_type & 0x08) == 0x08)
            {
                scan_type = 0;
            } else {
                scan_type = 1;
            }
        }
    }
    pthread_rwlock_unlock(_l);

    FUNCTION_FINISH(STB_AVGetVideoScanType);
    return scan_type;
}

/**
 * @brief Apply the specified copy protection. This function is used for CI+
 * @param copy_protection  - settings to be used for each output
 */
void STB_AVSetCopyProtection(S_STB_AV_COPY_PROTECTION *copy_protection)
{

   //copy freely (use case 2-9):      CP=1, L=0
   //copy no more (use case 10-17): CP=0, L=1
   //copy once (use case 18-25):  CP=0, L=0
   //copy never (use case 26-33):     CP=0, L=1
   //copy freely (use case 34-41):    CP=1, L=0


   FUNCTION_START(STB_AVSetCopyProtection);
   int ret;
   AML_MP_PLAYER player_handle;
   amp_spdif_mode type = AMP_TSPLAYER_KEY_SPDIF_MODE_NONE;
   U8BIT path = 0;

   U8BIT av_path = STB_AVGetPath(INVALID_RES_ID, path);

   AUD_DBG("audio codec path=%u av_path = %u", path, av_path);
   if (av_path == INVALID_RES_ID) {
      AUD_DBG("get av_path error audio codec path=%u av_path = %u", path, av_path);
      return;
   }

    pthread_rwlock_t* _l = STB_AVGetLockByPath(path);
    if (_l == NULL) {
        AUD_DBG("Can't get lock, audio decoder[%d]", path);
        return;
    }

    pthread_rwlock_rdlock(_l);
    ret = AV_GetPlayerHandleByPath_l(av_paths_status[av_path].video_decoder,
                                    av_paths_status[av_path].audio_decoder, &player_handle, FALSE);
   if (ret < 0)
   {
       AUD_DBG("Cannot get player handle[%d]", av_path);
       pthread_rwlock_unlock(_l);
       return;
   }

   AUD_DBG("set spdif protection: scms:[0x%x ] scms_set:[0x%x ] ", copy_protection->scms, copy_protection->scms_set);

   if (copy_protection->scms_set== TRUE)
   {
        if (copy_protection->scms == 2)
        {
            type = AMP_TSPLAYER_KEY_SPDIF_MODE_NONE;
        }
        else if (copy_protection->scms == 0)
        {
            type = AMP_TSPLAYER_KEY_SPDIF_MODE_ONCE;
        }
        else if (copy_protection->scms == 1)
        {
            type = AMP_TSPLAYER_KEY_SPDIF_MODE_NEVER;
        }

        ret = Aml_MP_Player_SetParameter(player_handle, AML_MP_PLAYER_PARAMETER_SPDIF_PROTECTION, (void*)&type);

        if (ret < 0)
        {
           AUD_DBG("Set audio spdf protection failed, err:%d", ret);
        }
    }
    pthread_rwlock_unlock(_l);

   FUNCTION_FINISH(STB_AVSetCopyProtection);
}

/**
 * @brief   Sets the decoding mode
 * @param   path decoder path
 * @param   decoding mode
 */
void STB_AVSetDecodingMode(U8BIT audio_decoder, U8BIT video_decoder, E_STB_DECODING_MODE mode)
{
    U8BIT av_path;
    FUNCTION_START(STB_AVSetDecodingMode);
    av_path = STB_AVGetPath(video_decoder, audio_decoder);

    if (av_path == INVALID_RES_ID)
    {
        VID_DBG("get av_path error, %d(%d:%d)", av_path, video_decoder, audio_decoder);
        return;
    }
    AV_DBG("[decoding mode]: %d(a:%d v:%d) = (%d -> %d)",
           av_path,
           audio_decoder,
           video_decoder,
           av_paths_status[av_path].decoding_mode,
           mode);

    if (av_paths_status[av_path].decoding_mode != mode)
    {

        if (IS_CACHED(av_paths_status[av_path].decoding_mode) != IS_CACHED(mode))
        {
            int ret;
            AML_MP_PLAYER player_handle;

            pthread_rwlock_t* _l = STB_AVGetLockByPath(video_decoder);
            if (_l == NULL) {
                AUD_DBG("Can't get lock, video decoder[%d], audio decoder[%d]", video_decoder, audio_decoder);
                return;
            }

            pthread_rwlock_rdlock(_l);
            ret = AV_GetPlayerHandleByPath_l(av_paths_status[av_path].video_decoder,
                                             av_paths_status[av_path].audio_decoder,
                                             &player_handle,
                                             FALSE);
            if (ret == 0) {
                Aml_MP_PlayerWorkMode work_mode =
                    IS_CACHED(mode) ? AML_MP_PLAYER_MODE_CACHING_ONLY : AML_MP_PLAYER_MODE_NORMAL;
                ret = Aml_MP_Player_SetParameter(player_handle, AML_MP_PLAYER_PARAMETER_WORK_MODE, (void *)(&work_mode));
                AV_DBG("set AML MP work mode: %d:[%d:%d] [%d] = %d, player[0x%p]",
                       av_path,
                       av_paths_status[av_path].video_decoder,
                       av_paths_status[av_path].audio_decoder,
                       work_mode,
                       ret,
                       player_handle);
            } else {
                AV_DBG("failed to get player handle, %d[%d:%d]",
                       av_path,
                       av_paths_status[av_path].video_decoder,
                       av_paths_status[av_path].audio_decoder);
            }
            pthread_rwlock_unlock(_l);
        }

        if (IS_AUDIO_DISABLE(av_paths_status[av_path].decoding_mode) != IS_AUDIO_DISABLE(mode)
                || IS_CACHED(mode))
        {
            int ret;
            AML_MP_PLAYER player_handle;

            av_paths_status[av_path].mute = (IS_AUDIO_DISABLE(mode) || IS_CACHED(mode)) ? TRUE : FALSE;

            pthread_rwlock_t* _l = STB_AVGetLockByPath(video_decoder);
            if (_l == NULL) {
                AUD_DBG("Can't get lock, video decoder[%d], audio decoder[%d]", video_decoder, audio_decoder);
                return;
            }

            pthread_rwlock_rdlock(_l);
            ret = AV_GetPlayerHandleByPath_l(av_paths_status[av_path].video_decoder,
                                             av_paths_status[av_path].audio_decoder,
                                             &player_handle,
                                             FALSE);
            if (ret >= 0)
            {
                ret = AV_SetAudioVolumeAndMute_l(player_handle,
                                                 av_paths_status[av_path].volume,
                                                 av_paths_status[av_path].mute);
                if (ret < 0)
                {
                    AV_DBG("failed to set audio volume, %d[%d:%d]",
                           av_path,
                           av_paths_status[av_path].volume,
                           av_paths_status[av_path].mute);
                }
            }
            pthread_rwlock_unlock(_l);
        }

        av_paths_status[av_path].decoding_mode = mode;
    }
    FUNCTION_FINISH(STB_AVSetDecodingMode);
}

/**
 * @brief   Sync decoding info from pvr
 * @param   path the audio decoder path to be started
 */
void STB_AVSyncDecodingFromPVR(U8BIT audio_decoder, U8BIT video_decoder)
{
   U16BIT video_pid = INVALID_PID, audio_pid = INVALID_PID, pcr_pid = INVALID_PID, ad_pid = INVALID_PID;
   U8BIT av_path;

   FUNCTION_START(STB_AVSyncDecodingFromPVR);
   av_path = STB_AVGetPath(video_decoder, audio_decoder);

   VID_DBG("av_path = [%u:%u] = %u", video_decoder, audio_decoder, av_path);
   if (av_path == INVALID_RES_ID) {
      VID_DBG("get av_path error, %d(%d:%d)", av_path, video_decoder, audio_decoder);
      return;
   }

   if (!STB_PVRIsPlayStopped(audio_decoder, video_decoder))
   {
      PVRGetDecodePIDs(audio_decoder, video_decoder, &pcr_pid, &video_pid, &audio_pid, &ad_pid);

      if (video_pid == 0)
      {
         video_pid = INVALID_PID;
         pcr_pid = INVALID_PID;
      }
      if (audio_pid == 0)
      {
         audio_pid = INVALID_PID;
      }

      /*refresh the current pvr pid status*/
      av_paths_status[av_path].video_pid = video_pid;
      av_paths_status[av_path].audio_pid = audio_pid;
      av_paths_status[av_path].pcr_pid = pcr_pid;
      av_paths_status[av_path].ad_pid = ad_pid;

      AV_DBG("av-pvr:, apid:%d vpid:%d", audio_pid, video_pid);
   }

   FUNCTION_FINISH(STB_AVSyncDecodingFromPVR);
}

void STB_AVNotifyEventHandler(U8BIT audio_path, U8BIT video_path, void *event, int64_t param)
{
   Aml_MP_PlayerEventType evt = AML_MP_EVENT_UNKNOWN;
   FUNCTION_START(STB_AVNotifyEventHandler);
   U8BIT av_path = STB_AVGetPath(video_path, audio_path);
   if (av_path == INVALID_RES_ID) {
      VID_DBG("get av_path error video codec path=%u audio codec:%u av_path = %u", audio_path, video_path, av_path);
      return;
   }

   if (event)
   {
      evt = *(Aml_MP_PlayerEventType*)event;
      AVEventHandler(&av_paths_status[av_path], evt, param);
   }

   FUNCTION_FINISH(STB_AVNotifyEventHandler);
}

void STB_AVRegisterCcCallback(U8BIT path, void* context, CC_DATA_CALLBACK callback)
{
   FUNCTION_START(STB_AVRegisterCcCallback);
   FUNCTION_FINISH(STB_AVRegisterCcCallback);
}

pthread_rwlock_t *STB_AVGetLockByPath(U8BIT path)
{
   FUNCTION_START(STB_AVGetLockByPath);

   if (path < num_paths)
      return &(av_paths_status[path].lock);
   return NULL;

   FUNCTION_FINISH(STB_AVGetLockByPath);
}

S16BIT STB_AVGetAC4ActivePresentationsID(U8BIT path)
{
    int ret;
    AML_MP_PLAYER handle;
    S16BIT presentations_id = -1;
    FUNCTION_START(STB_AVGetAC4ActivePresentationsID);

    pthread_rwlock_t* _l = STB_AVGetLockByPath(path);
    if (_l == NULL) {
        AUD_DBG("Can't get lock, path[%d]", path);
        return 0;
    }

    pthread_rwlock_rdlock(_l);
    ret = AV_GetPlayerHandleByPath_l(path, INVALID_RES_ID, &handle, FALSE);
    if (ret < 0)
    {
        AUD_DBG("Cannot get player handle audio[%d]", path);
        pthread_rwlock_unlock(_l);
        return 0;
    }

    ret = Aml_MP_Player_GetParameter(handle, AML_MP_PLAYER_PARAMETER_AUDIO_PRESENTATION_ID, &presentations_id);
    if (ret < 0)
    {
        AUD_DBG("Cannot get presentation ID for path[%d]", path);
    }

    pthread_rwlock_unlock(_l);
    //AUD_DBG("presentations_id:%d, err:%d", presentations_id, ret);
    FUNCTION_FINISH(STB_AVGetAC4ActivePresentationsID);
    return presentations_id;
}


BOOLEAN STB_AVSetAudioLanguage(U8BIT path, U32BIT pri_language_code, U32BIT sec_language_code)
{
    int ret;
    AML_MP_PLAYER player_handle;
    Aml_MP_AudioLanguage audioLang;

    FUNCTION_START(STB_AVSetAudioLanguage);

    VID_DBG("STB_AVSetAudioLanguage, pri_language_code:0x%x, sec_language_code:0x%x", pri_language_code, sec_language_code);
    U8BIT av_path = STB_AVGetPath(path, INVALID_RES_ID);
    if (av_path == INVALID_RES_ID) {
        VID_DBG("get av path error video codec path=%u, av_path=%u", path, av_path);
        return FALSE;
    }

    audioLang.firstLanguage = pri_language_code;
    audioLang.secondLanguage = sec_language_code;

    pthread_rwlock_t* _l = STB_AVGetLockByPath(path);
    if (_l == NULL) {
        AUD_DBG("Can't get lock, path[%d]", path);
        return FALSE;
    }

    pthread_rwlock_rdlock(_l);
    ret = AV_GetPlayerHandleByPath_l(path, INVALID_RES_ID, &player_handle, FALSE);
    if (ret < 0) {
        VID_DBG("Cannot get player handle video[%d]", path);
        pthread_rwlock_unlock(_l);
        return FALSE;
    }

    ret = Aml_MP_Player_SetParameter(player_handle, AML_MP_PLAYER_PARAMETER_AUDIO_LANGUAGE, &audioLang);
    pthread_rwlock_unlock(_l);

    if (ret < 0) {
        VID_DBG("STB_AVSetAudioLanguage failed, err:%d", ret);
    }

    FUNCTION_FINISH(STB_AVSetAudioLanguage);
    return TRUE;
}

BOOLEAN STB_AVResetWorkMode(void)
{
    FUNCTION_START(STB_AVResetWorkMode);
    //for tuner framework
    FUNCTION_FINISH(STB_AVResetWorkMode);
    return TRUE;
}

BOOLEAN STB_AVSetPlayIndex(U8BIT path, U8BIT index)
{
    FUNCTION_START(STB_AVSetPlayIndex);
    //for tuner framework
    USE_UNWANTED_PARAM(path);
    USE_UNWANTED_PARAM(index);
    FUNCTION_FINISH(STB_AVSetPlayIndex);
    return TRUE;
}

/*---local function definitions----------------------------------------------*/
//Dtvkit will check int and pointer convert, need convert to intptr_t or uintptr_t first
static void AVEventHandler(void *user_data, Aml_MP_PlayerEventType eventType, int64_t param)
{
    AV_PATH_STATUS *status;
    S_STB_AV_VIDEO_INFO info;

    status = (AV_PATH_STATUS *)user_data;
    info.flags = 0;

    switch (eventType)
    {
    case AML_MP_PLAYER_EVENT_USERDATA_CC:
    {
        Aml_MP_PlayerEventMpegUserData* userData = (Aml_MP_PlayerEventMpegUserData*)(intptr_t)param;
        uint8_t* pbuf = userData->data;
        uint32_t size = userData->len;
        AV_DBG("[evt][%d] AML_MP_PLAYER_EVENT_USERDATA_CC: %x-%x-%x-%x ,size %d\n",
               status->decoder,
               pbuf[0], pbuf[1], pbuf[2], pbuf[3], size);
        break;
    }
    case AML_MP_PLAYER_EVENT_USERDATA_AFD:
    {
        Aml_MP_PlayerEventMpegUserData* userData = (Aml_MP_PlayerEventMpegUserData*)(intptr_t)param;
        if (userData != NULL && userData->data != NULL) {
            uint8_t* pbuf = userData->data;
            uint32_t size = userData->len;
            AV_DBG("[evt][%d] AML_MP_PLAYER_EVENT_USERDATA_AFD: %x-%x-%x-%x ,size %d\n",
                   status->decoder,
                   pbuf[0], pbuf[1], pbuf[2], pbuf[3], size);
            USERDATA_AFD_t *afd = (USERDATA_AFD_t *)pbuf;
            afd->reserved = afd->pts = 0;
            info.flags = VIDEO_INFO_AFD;
            info.afd = afd->af & 0x7;
            VID_DBG("[evt][%d] video afd changed: flg[0x%x] fmt[0x%x]\n", status->decoder, afd->af_flag, afd->af);
        }
        break;
    }
    case AML_MP_PLAYER_EVENT_VIDEO_CHANGED:
    {
        Aml_MP_PlayerEventVideoFormat *videoFormat = (Aml_MP_PlayerEventVideoFormat*)(intptr_t)param;
        AV_DBG("[evt][%d] AML_MP_PLAYER_EVENT_VIDEO_CHANGED: [width:height] [%d x %d] @%d aspectratio[%d]\n",
               status->decoder,
               videoFormat->frame_width,
               videoFormat->frame_height,
               videoFormat->frame_rate,
               videoFormat->frame_aspectratio);
        if ((videoFormat->frame_width != 0) && (videoFormat->frame_height != 0))
        {
            info.flags |= VIDEO_INFO_VIDEO_RESOLUTION;
            info.video_width = videoFormat->frame_width;
            info.video_height = videoFormat->frame_height;
            AV_DBG("Video res changed, %u x %u", info.video_width, info.video_height);
            invokeCallback(status, &info);
            /*
                          if ((status != NULL) && (status->callback != NULL))
                              status->callback(&info, status->user_data, status->video_decoder);
            */
        }
        info.flags = 0;
        switch (videoFormat->frame_aspectratio)
        {
        case 0:
            info.flags |= VIDEO_INFO_VIDEO_ASPECT_RATIO;
            info.video_aspect_ratio = ASPECT_RATIO_4_3;
            VID_DBG("Video aspect ratio 4:3");
            break;
        case 1:
            info.flags |= VIDEO_INFO_VIDEO_ASPECT_RATIO;
            info.video_aspect_ratio = ASPECT_RATIO_16_9;
            VID_DBG("Video aspect ratio 16:9");
            break;
        default:
            VID_DBG(" Unhandled video aspect ratio default is 16:9");
            info.flags |= VIDEO_INFO_VIDEO_ASPECT_RATIO;
            info.video_aspect_ratio = ASPECT_RATIO_16_9;
            break;
        }
        info.flags |= VIDEO_INFO_DECODER_STATUS;
        info.status = DECODER_STATUS_VIDEO;
        STB_OSSendEvent(FALSE, HW_EV_CLASS_DECODE, HW_EV_TYPE_VIDEO_CHANGED, &status->decoder, sizeof(U8BIT));
        break;
    }
    case AML_MP_PLAYER_EVENT_AUDIO_CHANGED:
    {
        Aml_MP_PlayerEventAudioFormat *audioFormat = (Aml_MP_PlayerEventAudioFormat*)(intptr_t)param;
        if (audioFormat != NULL)
        {
            AV_DBG("[evt][%d] AML_MP_PLAYER_EVENT_AUDIO_CHANGED: sample_rate:%d, channels:%d\n",
                   status->decoder,
                   audioFormat->sample_rate,
                   audioFormat->channels);
        }
        break;
    }
    case AML_MP_PLAYER_EVENT_VIDEO_DECODE_FIRST_FRAME:
    {
        AV_DBG("[evt][%d] AML_MP_PLAYER_EVENT_VIDEO_DECODE_FIRST_FRAME!\n", status->decoder);
        STB_OSSendEvent(FALSE, HW_EV_CLASS_DECODE, HW_EV_TYPE_DECODE_VIDEO_FIRST_FRAME, &status->decoder, sizeof(U8BIT));
        info.flags |= VIDEO_INFO_DECODER_STATUS;
        info.status = DECODER_STATUS_DECODE_FIRST_FRAME_VIDEO;
        FLAGS = VIDEO_SIGNAL_STABLE;
        break;
    }
    case AML_MP_PLAYER_EVENT_AUDIO_DECODE_FIRST_FRAME:
    {
        AV_DBG("[evt][%d] AML_MP_PLAYER_EVENT_AUDIO_DECODE_FIRST_FRAME!\n", status->decoder);
        //STB_OSSendEvent(FALSE, HW_EV_CLASS_DECODE, HW_EV_TYPE_DECODE_AUDIO_FIRST_FRAME, &status->decoder, sizeof(U8BIT));
        AV_DBG("[evt][%d] AML_MP_PLAYER_EVENT_AUDIO_DECODE_FIRST_FRAME: ## AUDIO_AVAILABLE ##\n", status->decoder);
        STB_OSSendEvent(FALSE, HW_EV_CLASS_DECODE, HW_EV_TYPE_AUDIO_STARTED, &status->decoder, sizeof(U8BIT));
        break;
    }
    case AML_MP_PLAYER_EVENT_DATA_LOSS:
    {
        AV_DBG("[evt][%d] AML_MP_PLAYER_EVENT_DATA_LOSS!\n", status->decoder);
        STB_OSSendEvent(FALSE, HW_EV_CLASS_DECODE, HW_EV_TYPE_DECODE_INPUT_DATA_LOSS, &status->decoder, sizeof(U8BIT));
        FLAGS = VIDEO_SIGNAL_LOSS;
        break;
    }
    case AML_MP_PLAYER_EVENT_DATA_RESUME:
    {
        AV_DBG("[evt][%d] AML_MP_PLAYER_EVENT_DATA_RESUME\n", status->decoder);
        STB_OSSendEvent(FALSE, HW_EV_CLASS_DECODE, HW_EV_TYPE_DECODE_INPUT_DATA_RESUME, &status->decoder, sizeof(U8BIT));
        FLAGS = VIDEO_SIGNAL_STABLE;
        break;
    }
    case AML_MP_PLAYER_EVENT_DECODER_DATA_LOSS:
    {
        AV_DBG("[evt][%d] AML_MP_PLAYER_EVENT_DECODER_DATA_LOSS!\n", status->decoder);
        STB_OSSendEvent(FALSE, HW_EV_CLASS_DECODE, HW_EV_TYPE_DECODE_NO_DATA, &status->decoder, sizeof(U8BIT));
        FLAGS = VIDEO_SIGNAL_LOSS;
        break;
    }
    case AML_MP_PLAYER_EVENT_DECODER_DATA_RESUME:
    {
        AV_DBG("[evt][%d] AML_MP_PLAYER_EVENT_DECODER_DATA_RESUME\n", status->decoder);
        STB_OSSendEvent(FALSE, HW_EV_CLASS_DECODE, HW_EV_TYPE_DECODE_DATA_RESUME, &status->decoder, sizeof(U8BIT));
        FLAGS = VIDEO_SIGNAL_STABLE;
        break;
    }
    case AML_MP_PLAYER_EVENT_SCRAMBLING:
    {
        Aml_MP_PlayerEventScrambling *scrambling = (Aml_MP_PlayerEventScrambling*)(intptr_t)param;
        AV_DBG("[evt][%d] AML_MP_PLAYER_EVENT_SCRAMBLING: stream_type:%d is_scramling[%d]\n",
               status->decoder,
               scrambling->type,
               scrambling->scramling);
        if (scrambling->type == AML_MP_STREAM_TYPE_VIDEO)
        {
            AV_DBG("Video Scambled");
            STB_OSSendEvent(FALSE, HW_EV_CLASS_DECODE, HW_EV_TYPE_VIDEO_SCAMBLED, &status->decoder, sizeof(U8BIT));
        }
        else if (scrambling->type == AML_MP_STREAM_TYPE_AUDIO || scrambling->type == AML_MP_STREAM_TYPE_AD)
        {
            AV_DBG("Audio Scambled");
            STB_OSSendEvent(FALSE, HW_EV_CLASS_DECODE, HW_EV_TYPE_AUDIO_SCAMBLED, &status->decoder, sizeof(U8BIT));
        }
        break;
    }
    case AML_MP_PLAYER_EVENT_FIRST_FRAME:
    {
        AV_DBG("[evt][%d] AML_MP_PLAYER_EVENT_FIRST_FRAME: ## VIDEO_AVAILABLE ##\n", status->decoder);
        STB_OSSendEvent(FALSE, HW_EV_CLASS_DECODE, HW_EV_TYPE_VIDEO_STARTED, &status->decoder, sizeof(U8BIT));
        {
            int ret;
            U32BIT sync_id = 0;

            ret = Aml_MP_Player_GetParameter(status->player_handle, AML_MP_PLAYER_PARAMETER_SYNC_ID, (void *)&sync_id);
            if (ret != 0)
                sync_id = -1;

            S_VIDEO_DECODER_PRIV_DATA priv =
            {
                .decoder = status->decoder,
                .decoder_id_valid = FALSE,
                .sync_id = sync_id,
                .sync_id_valid = TRUE,
            };
            STB_OSSendEvent(FALSE, HW_EV_CLASS_DECODE, HW_EV_TYPE_VIDEO_DECODER_PRIV_DATA, &priv, sizeof(priv));
        }
        FLAGS = VIDEO_SIGNAL_STABLE;
        break;
    }
    case AML_MP_PLAYER_EVENT_VIDEO_ERROR_FRAME_COUNT:
    {
        AV_DBG("[evt][%d] AML_MP_PLAYER_EVENT_VIDEO_ERROR_FRAME_COUNT\n", status->decoder);
        STB_OSSendEvent(FALSE, HW_EV_CLASS_DECODE, HW_EV_TYPE_VIDEO_ERROR_FRAME_COUNT, NULL, 0);
        break;
    }

    case AML_MP_PLAYER_EVENT_VIDEO_UNSUPPORT:
    {
        AV_DBG("[evt][%d] AML_MP_PLAYER_EVENT_VIDEO_UNSUPPORT\n", status->decoder);
        STB_OSSendEvent(FALSE, HW_EV_CLASS_DECODE, HW_EV_TYPE_VIDEO_UNSUPPORT, &status->decoder, sizeof(U8BIT));
        break;
    }

    default:
        break;
    }

    if ((info.flags != 0) && (status != NULL) && (status->callback != NULL))
    {
        invokeCallback(status, &info);
        // status->callback(&info, status->user_data, status->video_decoder);
    }
}

BOOLEAN AV_StartInjection(U8BIT path)
{
   if (path >= num_paths)
   {
      AV_DBG("Invalid path: %d", path);
      return FALSE;
   }
   av_paths_status[path].injecting = TRUE;

   return TRUE;
}

BOOLEAN AV_StopInjection(U8BIT path)
{
   if (path >= num_paths)
   {
      AV_DBG("Invalid path: %d", path);
      return FALSE;
   }
   av_paths_status[path].injecting = FALSE;

   return TRUE;
}

void AV_InjectData(U8BIT path, U8BIT *data, U32BIT size)
{
    U8BIT *buffer = data;
    U32BIT left = size;
    U32BIT sent;
    int ret;
    AML_MP_PLAYER player_handle;
    pthread_rwlock_t* _l = STB_AVGetLockByPath(path);

    if (_l == NULL)
    {
        VID_DBG("Can't get lock, path[%d]", path);
        return;
    }

    pthread_rwlock_wrlock(_l);
    if (av_paths_status[path].injecting == FALSE)
    {
        VID_DBG("Can't inject for path[%d]", path);
        pthread_rwlock_unlock(_l);
        return;
    }

    ret = AV_GetPlayerHandleByPath_l(path, INVALID_RES_ID, &player_handle, FALSE);
    if (ret < 0 && IS_INVALID_PLAYER_HANDLE(path) && av_paths_status[path].video_pid == INVALID_PID && av_paths_status[path].audio_pid == INVALID_PID)
    {
        VID_DBG("Cannot get player handle video[%d]", path);
        VID_DBG("Create one DMX instance...");
        ret = AV_CreateTsPlayer_l(path, AML_MP_INPUT_SOURCE_TS_MEMORY, av_paths_status[path].demux, 0);
        if (ret < 0)
        {
            VID_DBG("Cannot get player handle AGAIN video[%d]", path);
            pthread_rwlock_unlock(_l);
            return;
        }
        player_handle = av_paths_status[path].player_handle;
    }

    // Ensure that `buffer` and `left` are properly managed
    while (left > 0 && ret >= 0)
    {
        sent = left;
        ret = Aml_MP_Player_WriteData(player_handle, buffer, sent);
        if (ret < 0)
        {
            VID_DBG("Aml_MP_Player_WriteData failed, err %d", ret);
            break; // Exit loop if write fails
        }

        buffer += sent;
        left -= sent;
    }

    pthread_rwlock_unlock(_l);

    if (ret != (int) size)
    {
        VID_DBG("Aml_MP_Player_WriteData failed, err %d", ret);
    }
}

int AV_CreateTsPlayer_l(U8BIT path,
                       Aml_MP_InputSourceType source_type, int32_t dmx_dev_id, int32_t event_mask)
{
    U32BIT decoder_id = 0;
    uint32_t numb = 0;
    int ret;
    Aml_MP_PlayerCreateParams parm;
    AML_MP_PLAYER player_handle = AML_MP_INVALID_HANDLE;

    if (path >= num_paths)
    {
        AV_DBG("Invalid path: %d", path);
        return AML_MP_ERROR;
    }

    memset(&parm, 0, sizeof(parm));
    parm.channelId = path;
    parm.demuxId = (Aml_MP_DemuxId)dmx_dev_id;
    parm.sourceType = source_type;
#ifdef SUPPORT_CAS
    if (av_paths_status[path].drm_mode == DRM_NONE)
    {
        parm.drmMode = AML_MP_INPUT_STREAM_NORMAL;
    }
    else
    {
        parm.drmMode = AML_MP_INPUT_STREAM_ENCRYPTED;
    }
    AV_DBG("parm.drmMode = %d", parm.drmMode);
#endif
    ret = Aml_MP_Player_Create(&parm, &player_handle);
    if (ret == 0)
    {
        av_paths_status[path].player_handle = player_handle;
        ret = Aml_MP_Player_GetParameter(player_handle, AML_MP_PLAYER_PARAMETER_INSTANCE_ID, &decoder_id);
        if (ret != 0)
            decoder_id = -1;

        {
            S_VIDEO_DECODER_PRIV_DATA priv =
            {
                .decoder = av_paths_status[path].decoder,
                .decoder_id = decoder_id,
                .decoder_id_valid = TRUE,
                .sync_id_valid = FALSE,
            };
            STB_OSSendEvent(FALSE, HW_EV_CLASS_DECODE, HW_EV_TYPE_VIDEO_DECODER_PRIV_DATA, &priv, sizeof(priv));
        }
        //create afd context
        afd_create_context(path, decoder_id);

        bool useTif = true;
        Aml_MP_Player_SetParameter(player_handle, AML_MP_PLAYER_PARAMETER_USE_TIF, &useTif);
        ret = Aml_MP_Player_RegisterEventCallBack(player_handle, AVEventHandler, &av_paths_status[path]);
        AV_DBG("Create Aml MP player success. path=%d player_handle[%p]:0x%zx dxm_id:%d", path, player_handle, numb, dmx_dev_id);

#ifdef RDK_COMPILE
        if (video_surface[path] != NULL) {
            VID_DBG("set AML MP surface(tunnelid): %d",*((int*)video_surface[path]));
            Aml_MP_Player_SetParameter(player_handle, AML_MP_PLAYER_PARAMETER_VIDEO_TUNNEL_ID, video_surface[path]);
        } else {
            VID_DBG("Cannot set surface to AML MP, surface(tunnelid) ptr is NULL.");
        }
        //preset widow parameter
        int32_t x = window_rect_array[path].x;
        int32_t y = window_rect_array[path].y;
        int32_t w = window_rect_array[path].w;
        int32_t h = window_rect_array[path].h;
        AV_DBG("SetVideoWindow hdl:%d (%d,%d,%d,%d)", player_handle,x,y,w,h);
        Aml_MP_Player_SetVideoWindow(player_handle,x,y,w,h);

#endif
    }
    else
    {
        av_paths_status[path].player_handle = AML_MP_INVALID_HANDLE;
        AV_DBG("Create Aml MP player failed, err:%d", ret);
    }
    return ret;
}


int AV_ReleaseTsPlayer_l(U8BIT path)
{
    int ret = 0;

    if (path >= num_paths)
    {
        AV_DBG("Invalid path: %d", path);
        return AML_MP_ERROR;
    }

    AV_DBG("Will Release Ts player");

    //release afd context
    afd_release_context(path);

    if (IS_INVALID_PLAYER_HANDLE(path))
    {
        AV_DBG("Release Ts player already.");
    }
    else
    {
        ret = Aml_MP_Player_RegisterEventCallBack(av_paths_status[path].player_handle, NULL, NULL);
        if (ret < 0)
        {
            AV_DBG("Failed to deregister event callback, err:%d", ret);
        }

        ret = Aml_MP_Player_Destroy(av_paths_status[path].player_handle);
        if (ret < 0)
        {
            AV_DBG("Destroy Aml MP player failed, err:%d", ret);
        }
        else
        {
            AV_DBG("Destroy Aml MP player, player_handle[%d]:0x%p", path, av_paths_status[path].player_handle);
            av_paths_status[path].player_handle = AML_MP_INVALID_HANDLE;
        }
    }

    return ret;
}

int AV_GetPlayerHandleByPath_l(U8BIT video_decoder, U8BIT audio_decoder, AML_MP_PLAYER * player_handle, BOOLEAN recreat_handle)
{
    int ret = -1;
    Aml_MP_InputSourceType mode = 0;

    if (STB_PVRIsPlayStopped(audio_decoder, video_decoder))
    {
        U8BIT av_path;
        av_path = STB_AVGetPath(video_decoder, audio_decoder);

        if (av_path == INVALID_RES_ID)
        {
            VID_DBG("get av_path error, %d(%d:%d)", av_path, video_decoder, audio_decoder);
            return -1;
        }

        if (!IS_INVALID_PLAYER_HANDLE(av_path))
        {
            ret = 0;
        }
        else if (recreat_handle && av_path != INVALID_RES_ID)
        {
            if (STB_CIUsbInsertedState(INSERTED_STATE))
                mode = AML_MP_INPUT_SOURCE_USBCAM;
            else
                mode = AML_MP_INPUT_SOURCE_TS_DEMOD;
            if (av_paths_status[av_path].injecting)
                mode = AML_MP_INPUT_SOURCE_TS_MEMORY;

            ret = AV_CreateTsPlayer_l(av_path, mode, av_paths_status[av_path].demux, 0);

            if (IS_CACHED(av_paths_status[av_path].decoding_mode))
            {
                Aml_MP_PlayerWorkMode work_mode = AML_MP_PLAYER_MODE_CACHING_ONLY;
                int result = Aml_MP_Player_SetParameter(av_paths_status[av_path].player_handle, AML_MP_PLAYER_PARAMETER_WORK_MODE, (void *)(&work_mode));
                AV_DBG("set AML MP work mode: %d:[%d:%d] caching_only = %d, player[0x%p]",
                       av_path,
                       av_paths_status[av_path].video_decoder,
                       av_paths_status[av_path].audio_decoder,
                       result,
                       av_paths_status[av_path].player_handle);
            }
        }
        *player_handle = av_paths_status[av_path].player_handle;
    }
    else
    {
        if (STB_PVRGetPlayerHandle(video_decoder, audio_decoder, (void**)player_handle) == TRUE) {
            ret = 0;
        }
        AV_DBG("pvr path, handle:%p, ret: %d", player_handle, ret);
    }

    return ret;
}

int AV_GetPathByPlayerHandle(AML_MP_PLAYER *player_handle)
{
   int i;
   if (!player_handle)
      return INVALID_RES_ID;
   for (i = 0; i < num_paths; i++)
   {
      if (av_paths_status[i].player_handle == player_handle)
         return i;
   }
   return INVALID_RES_ID;
}

int AV_StartAudioDecode_l(AML_MP_PLAYER player_handle, U16BIT a_pid,
                        Aml_MP_CodecID format, Aml_MP_AudioBalance audio_mode, U8BIT vol, BOOLEAN mute, int audioPresentationId)
{
   int ret;
   Aml_MP_AudioParams audio_param;

   memset(&audio_param, 0, sizeof(audio_param));

   audio_param.pid = a_pid;
   audio_param.audioCodec = format;
   ret = Aml_MP_Player_SetAudioParams(player_handle, &audio_param);
   if (ret < 0)
   {
      AUD_DBG("Set audio params failed, pid:%d fmt:%d err:%d", a_pid, format, ret);
      return ret;
   }

   if (audioPresentationId >= 0)
   {
      ret = Aml_MP_Player_SetParameter(player_handle, AML_MP_PLAYER_PARAMETER_AUDIO_PRESENTATION_ID, &audioPresentationId);
      if (ret < 0)
      {
         AUD_DBG("Set audio presentation id[%d] failed, err:%d", audioPresentationId, ret);
         return ret;
      }
   }

   ret = Aml_MP_Player_SetParameter(player_handle, AML_MP_PLAYER_PARAMETER_AUDIO_BALANCE, &audio_mode);
   if (ret < 0)
   {
      AUD_DBG("Set audio stereo mode[%d] failed, err:%d", audio_mode, ret);
      return ret;
   }

   ret = AV_SetAudioVolumeAndMute_l(player_handle, vol, mute);
   if (ret < 0)
   {
      return ret;
   }

   ret = Aml_MP_Player_StartAudioDecoding(player_handle);
   if (ret < 0)
   {
      AUD_DBG("Start audio decode failed, pid:%d fmt:%d err:%d, player[0x%p]", a_pid, format, ret, player_handle);
      return ret;
   }

   AUD_DBG("Start audio decode, pid:%d fmt:%d, volume[%d], audio_mode[%d], mute[%d], player[0x%p]", a_pid, format, vol, audio_mode, mute, player_handle);
   return ret;
}

int AV_SetAudioDecode_l(AML_MP_PLAYER player_handle, Aml_MP_AudioBalance audio_mode, U8BIT vol, BOOLEAN mute)
{
   int ret;

   ret = Aml_MP_Player_SetParameter(player_handle, AML_MP_PLAYER_PARAMETER_AUDIO_BALANCE, &audio_mode);
   if (ret < 0)
   {
      AUD_DBG("Set audio stereo mode[%d] failed, err:%d", audio_mode, ret);
      return ret;
   }

   ret = AV_SetAudioVolumeAndMute_l(player_handle, vol, mute);
   if (ret < 0)
   {
      return ret;
   }
   AUD_DBG("Set audio decode, volume[%d], audio_mode[%d], player[0x%p]", vol, audio_mode, player_handle);
   return ret;
}

int AV_StartVideoDecode_l(AML_MP_PLAYER player_handle,
                       U16BIT v_pid, U16BIT pcr_pid, Aml_MP_CodecID format, Aml_MP_AVSyncSource mode)
{
   int ret;
   Aml_MP_VideoParams video_param;

   memset(&video_param, 0, sizeof(video_param));
   ret = Aml_MP_Player_SetAVSyncSource(player_handle, mode);
   if (ret < 0)
   {
      VID_DBG("Set sync mode failed, sync_mode:%d err:%d", mode, ret);
      return ret;
   }
   if (pcr_pid)
   {
      ret = Aml_MP_Player_SetPcrPid(player_handle, pcr_pid);
      if (ret < 0)
      {
         VID_DBG("Set pcr pid failed, pcr_pid:%d err:%d", pcr_pid, ret);
         return ret;
      }
   }
   video_param.pid = v_pid;
   video_param.videoCodec = format;
   ret = Aml_MP_Player_SetVideoParams(player_handle, &video_param);
   if (ret < 0)
   {
      VID_DBG("Set video params failed, v_pid:%d fmt:%d err:%d", v_pid, format, ret);
      return ret;
   }
   ret = Aml_MP_Player_StartVideoDecoding(player_handle);
   if (ret < 0)
   {
      VID_DBG("Start video decode failed, v_pid:%d pcr_pid:%d fmt:%d sync:%d err:%d, player[0x%p]", v_pid, pcr_pid, format, mode, ret, player_handle);
      return ret;
   }

   VID_DBG("Start video decode, v_pid:%d pcr_pid:%d fmt:%d sync:%d, player[0x%p]", v_pid, pcr_pid, format, mode, player_handle);
   return ret;
}

E_DECODER_STATE AV_GetDecoderState_l(AML_MP_PLAYER player_handle, E_DECODER_INDEX index)
{
    AML_MP_DecodingState decodingState = AML_MP_DECODING_STATE_STOPPED;
    Aml_MP_StreamType streamType;
    int ret;

    if (player_handle == NULL) {
        return DECODER_STATE_STOPPED;
    }

    streamType = toStreamType(index);

    if (AML_MP_STREAM_TYPE_UNKNOWN == streamType) {
        return DECODER_STATE_STOPPED;
    }
    ret = Aml_MP_Player_GetDecodingState(player_handle, streamType, &decodingState);

    E_DECODER_STATE state = toDecoderState(decodingState);
    return state;
}

static int AV_SetAudioVolumeAndMute_l(AML_MP_PLAYER player_handle, U8BIT vol, BOOLEAN mute)
{
   int ret = 0;
   int av_path;
   BOOLEAN audioOutControl;

   av_path = AV_GetPathByPlayerHandle(player_handle);

   AUD_DBG("set aud path [%d] vol[%d] mute[%d]", av_path, vol, mute);
   if (av_path == INVALID_RES_ID)
   {
      AUD_DBG("av path invalid, fatal error");
      return -1;
   }

    if (mute) {
        ret |= AV_SetAudioVolume_l(player_handle, vol);
        audioOutControl = AV_UpdateAudioOutControl_l(av_path, AVOUT_VOL, mute);
        AV_SetAudioMute_l(player_handle, audioOutControl);
    } else {
        audioOutControl = AV_UpdateAudioOutControl_l(av_path, AVOUT_VOL, mute);
        AV_SetAudioMute_l(player_handle, audioOutControl);
        ret |= AV_SetAudioVolume_l(player_handle, vol);
    }

   return ret;
}

static int AV_SetAudioVolume_l(AML_MP_PLAYER player_handle, U8BIT vol)
{
   int ret;

   if (audio_mute_lock)
   {
      AUD_DBG("set audio volume failed, locked");
      return 0;
   }

   AUD_DBG("set aud vol[%d]", vol);

   ret = Aml_MP_Player_SetVolume(player_handle, vol);
   if (ret < 0)
   {
      AUD_DBG("Set audio volume[%d] failed, err:%d", vol, ret);
      return ret;
   }

   return ret;
}

static int AV_SetAudioMute_l(AML_MP_PLAYER player_handle, BOOLEAN mute)
{
   int ret;

   if (audio_mute_lock)
   {
      AUD_DBG("set audio mute failed, locked");
      return 0;
   }

   AUD_DBG("set aud mute[%d]", mute);

   ret = Aml_MP_Player_SetParameter(player_handle, AML_MP_PLAYER_PARAMETER_AUDIO_MUTE, &mute);
   if (ret < 0)
   {
      AUD_DBG("Set audio mute[%d] failed, err:%d", mute, ret);
   }

   return ret;
}

static BOOLEAN AV_UpdateAudioOutControl_l(U8BIT av_path, E_AV_OUT_CONTROL_FLAG flag, BOOLEAN mute) {
    BOOLEAN audio_mute = FALSE;

    if (av_path == INVALID_RES_ID) {
        /*force save audio mute flag before av path create*/
        av_path = 0;
    }

    if (mute) {
        av_paths_status[av_path].audio_out_control |= (1<<flag);
    } else {
        av_paths_status[av_path].audio_out_control &= (~(1<<flag));
    }

    if (av_paths_status[av_path].audio_out_control) {
        audio_mute = TRUE;
    } else {
        audio_mute = FALSE;
    }

    if (flag == AVOUT_VOL) {
        av_paths_status[av_path].mute = mute;
    }

    return audio_mute;
}

static int AV_SetVideoColor(U8BIT av_path, BOOLEAN is_black, BOOLEAN mode)
{
    int ret;
    Aml_MP_PlayerCreateParams parm;
    AML_MP_PLAYER player_handle = AML_MP_INVALID_HANDLE;

    if (av_path >= num_paths)
    {
        AV_DBG("Invalid path: %d", av_path);
        return AML_MP_ERROR;
    }

    memset(&parm, 0, sizeof(parm));
    parm.channelId = av_path;
    parm.demuxId = (Aml_MP_DemuxId)av_paths_status[av_path].demux;
    parm.sourceType = AML_MP_INPUT_SOURCE_ES_MEMORY;
    parm.drmMode = AML_MP_INPUT_STREAM_NORMAL;
    ret = Aml_MP_Player_Create(&parm, &player_handle);
    if (ret == 0)
    {
        AV_DBG("create player success, handle:%p", player_handle);
    }
    else
    {
        AV_DBG("Set color failed, err:%d", ret);
        return ret;
    }

    ret = Aml_MP_Player_SetParameter(player_handle, AML_MP_PLAYER_PARAMETER_SURFACE_HANDLE, video_surface[av_path]);
    if (ret < 0)
    {
        AV_DBG("Set color failed, err:%d", ret);
    }

    ret = AV_StartVideoDecode_l(player_handle, 0x1fff, 0x1fff, AML_MP_VIDEO_CODEC_MPEG12, AML_MP_AVSYNC_SOURCE_PCR);
    if (ret < 0)
    {
        AV_DBG("Set color failed, err:%d", ret);
    }

    ret = Aml_MP_Player_SetParameter(player_handle, AML_MP_PLAYER_PARAMETER_BLACK_OUT, &is_black);
    if (ret < 0)
    {
        AV_DBG("Set color failed, err:%d", ret);
    }

    ret = Aml_MP_Player_StopVideoDecoding(player_handle);
    if (ret < 0)
    {
        AV_DBG("Set color failed, err:%d", ret);
    }

    ret = Aml_MP_Player_Destroy(player_handle);
    if (ret < 0)
    {
        AV_DBG("Set color failed, err:%d", ret);
        return ret;
    }
    return ret;
}

static Aml_MP_StreamType toStreamType(E_DECODER_INDEX index) {
    switch (index) {
        case AUDIO_DECODER: return AML_MP_STREAM_TYPE_AUDIO;
        case VIDEO_DECODER: return AML_MP_STREAM_TYPE_VIDEO;
        case AD_DECODER:    return AML_MP_STREAM_TYPE_AD;
        default:            return AML_MP_STREAM_TYPE_UNKNOWN;
    }
    return AML_MP_STREAM_TYPE_UNKNOWN;
}

static E_DECODER_STATE toDecoderState(AML_MP_DecodingState decodingState) {
    switch (decodingState) {
        case AML_MP_DECODING_STATE_STOPPED:
            return DECODER_STATE_STOPPED;
        case AML_MP_DECODING_STATE_START_PENDING:
        case AML_MP_DECODING_STATE_STARTED:
        case AML_MP_DECODING_STATE_PAUSED:
            return DECODER_STATE_STARTED;
    }
    return DECODER_STATE_STOPPED;
}

static E_STB_AV_VIDEO_CODEC toVideoCodec(Aml_MP_CodecID codec)
{
   switch (codec)
   {
      case AML_MP_VIDEO_CODEC_MPEG12: return AV_VIDEO_CODEC_MPEG2;
      case AML_MP_VIDEO_CODEC_H264: return AV_VIDEO_CODEC_H264;
      case AML_MP_VIDEO_CODEC_HEVC: return AV_VIDEO_CODEC_H265;
      case AML_MP_VIDEO_CODEC_VP9: return AV_VIDEO_CODEC_VP9;
      case AML_MP_VIDEO_CODEC_DVES_AVC: return AV_VIDEO_CODEC_DVES_AVC;
      case AML_MP_VIDEO_CODEC_DVES_HEVC: return AV_VIDEO_CODEC_DVES_HEVC;
      default:                      return AV_VIDEO_CODEC_AUTO;
   }
   return AV_VIDEO_CODEC_AUTO;
}

static E_STB_AV_AUDIO_CODEC toAudioCodec(Aml_MP_CodecID codec)
{
   switch (codec)
   {
      case AML_MP_AUDIO_CODEC_MP2: return AV_AUDIO_CODEC_MP2;
      case AML_MP_AUDIO_CODEC_MP3: return AV_AUDIO_CODEC_MP3;
      case AML_MP_AUDIO_CODEC_AC3: return AV_AUDIO_CODEC_AC3;
      case AML_MP_AUDIO_CODEC_EAC3: return AV_AUDIO_CODEC_EAC3;
      case AML_MP_AUDIO_CODEC_AC4: return AV_AUDIO_CODEC_AC4;
      case AML_MP_AUDIO_CODEC_AAC: return AV_AUDIO_CODEC_AAC;
      case AML_MP_AUDIO_CODEC_LATM: return AV_AUDIO_CODEC_HEAAC;
      default:                     return AV_AUDIO_CODEC_AUTO;
   }
   return AV_AUDIO_CODEC_MP2;
}

static Aml_MP_StreamType toMPStreamType(E_HBBTV_PTS_SOURCE_TYPE streamType)
{
    switch (streamType)
    {
        case HBBTV_PTS_TIME_VIDEO: return AML_MP_STREAM_TYPE_VIDEO;
        case HBBTV_PTS_TIME_AUDIO: return AML_MP_STREAM_TYPE_AUDIO;
        case HBBTV_PTS_TIME_PCR:   return AML_MP_STREAM_TYPE_PCR;
        case HBBTV_PTS_TIME_STC:   return AML_MP_STREAM_TYPE_STC;
        default:                   break;
    }

    return (AML_MP_STREAM_TYPE_UNKNOWN);
}

