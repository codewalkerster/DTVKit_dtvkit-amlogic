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
 * @file    stbhwav_tsplayer.c
 * @date    March 2020
 */

/*---includes for this file--------------------------------------------------*/
/* compiler library header files */
#include <stdio.h>
#include <string.h>

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

/* third party header files */
#define  AV_AUDIO_STEREO        AV_AUDIO_STEREO_TSP
#define  AV_AUDIO_RIGHT         AV_AUDIO_RIGHT_TSP
#define  AV_AUDIO_LEFT          AV_AUDIO_LEFT_TSP
#define  AV_VIDEO_CODEC_AUTO    AV_VIDEO_CODEC_AUTO_TSP
#define  AV_VIDEO_CODEC_H264    AV_VIDEO_CODEC_H264_TSP
#define  AV_VIDEO_CODEC_H265    AV_VIDEO_CODEC_H265_TSP
#define  AV_VIDEO_CODEC_MPEG1   AV_VIDEO_CODEC_MPEG1_TSP
#define  AV_VIDEO_CODEC_MPEG2   AV_VIDEO_CODEC_MPEG2_TSP
#define  AV_VIDEO_CODEC_VP9     AV_VIDEO_CODEC_VP9_TSP
#define  AV_AUDIO_CODEC_AUTO   AV_AUDIO_CODEC_AUTO_TSP
#define  AV_AUDIO_CODEC_MP2    AV_AUDIO_CODEC_MP2_TSP
#define  AV_AUDIO_CODEC_MP3    AV_AUDIO_CODEC_MP3_TSP
#define  AV_AUDIO_CODEC_AC3    AV_AUDIO_CODEC_AC3_TSP
#define  AV_AUDIO_CODEC_EAC3   AV_AUDIO_CODEC_EAC3_TSP
#define  AV_AUDIO_CODEC_DTS    AV_AUDIO_CODEC_DTS_TSP
#define  AV_AUDIO_CODEC_AAC    AV_AUDIO_CODEC_AAC_TSP
#define  AV_AUDIO_CODEC_LATM    AV_AUDIO_CODEC_LATM_TSP
#define  AV_AUDIO_CODEC_AC4    AV_AUDIO_CODEC_AC4_TSP

#ifdef SUPPORT_CAS
#include "am_cas.h"
#endif

#include "AmTsPlayer.h"

#undef  AV_AUDIO_RIGHT
#undef  AV_AUDIO_LEFT
#undef  AV_VIDEO_CODEC_AUTO
#undef  AV_VIDEO_CODEC_H264
#undef  AV_VIDEO_CODEC_H265
#undef  AV_VIDEO_CODEC_MPEG1
#undef  AV_VIDEO_CODEC_MPEG2
#undef  AV_VIDEO_CODEC_VP9
#undef  AV_AUDIO_CODEC_AUTO
#undef  AV_AUDIO_CODEC_MP2
#undef  AV_AUDIO_CODEC_MP3
#undef  AV_AUDIO_CODEC_AC3
#undef  AV_AUDIO_CODEC_EAC3
#undef  AV_AUDIO_CODEC_DTS
#undef  AV_AUDIO_CODEC_AAC
#undef  AV_AUDIO_CODEC_LATM
#undef  AV_AUDIO_CODEC_AC4



/*---macro definitions for this file-----------------------------------------*/
#define AV_DEBUG
#define VIDEO_DEBUG
#define AUDIO_DEBUG


#ifdef AV_DEBUG
   #define AV_DBG(x,...)   STB_SPDebugWrite("%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )
#else
   #define AV_DBG(x,...)
#endif

#ifdef VIDEO_DEBUG
   #define VID_DBG(x,...)  STB_SPDebugWrite("%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )
#else
   #define VID_DBG(x,...)
#endif

#ifdef AUDIO_DEBUG
   #define AUD_DBG(x,...)  STB_SPDebugWrite("%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )
#else
   #define AUD_DBG(x,...)
#endif

#define ERR_DBG(x,...)     STB_SPDebugWrite("%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )

#define RET_DBG(_fun_, _ret_)

#define INVALID_PID 0x1fff

#define INVALID_RES_ID    255

#define MIN_AV_SPEED    -600
#define MAX_AV_SPEED     600
#define MAX_PLAYER_NUM     32

#define INVALID_PLAYER_HANDLE 0
#define IS_INVALID_PLAYER_HANDLE(_path_)    ((av_paths_status[_path_].player_handle) == INVALID_PLAYER_HANDLE)

#define IS_CACHED(_m_) ((_m_) & DECODING_MODE_CACHE_ONLY)
#define IS_AD_ENABLE(_m_) ((_m_) & DECODING_AD_ENABLE)
#define IS_AUDIO_DISABLE(_m_) ((_m_) & DECODING_AUDIO_DISABLE)

/*---constant definitions for this file--------------------------------------*/

/*---local typedef structs for this file-------------------------------------*/
typedef enum
{
   DECODER_A_STOP_V_STOP,
   DECODER_A_START_V_STOP,
   DECODER_A_STOP_V_START,
   DECODER_A_START_V_START
} E_AV_DECODER_STATE;

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
   am_tsplayer_handle player_handle;
   am_tsplayer_video_codec video_format;
   am_tsplayer_audio_codec audio_format;
   am_tsplayer_audio_codec ad_format;
   am_tsplayer_audio_stereo_mode audio_mode;
   E_AV_DECODER_STATE av_decoder_state;
   BOOLEAN injecting;

   BOOLEAN iframe_shown;
   U8BIT* iframe_data;
   U32BIT iframe_data_size;
   E_STB_AV_VIDEO_CODEC iframe_codec;

   void (*callback)(S_STB_AV_VIDEO_INFO *, void *, int);
   void *user_data;

   U8BIT volume;
   U8BIT mute;
   U8BIT* sample_data;
   U32BIT sample_data_size;
   U32BIT loop_count;
   BOOLEAN audio_descriptor_active;
   U16BIT video_pid;
   U16BIT audio_pid;
   U16BIT pcr_pid;
   U16BIT ad_pid;
   U8BIT video_decoder;
   U8BIT audio_decoder;
   S_DISPLAY_INFO display_info;

   U32BIT param;

   E_STB_DECODING_MODE decoding_mode;

#ifdef SUPPORT_CAS
   E_STB_DRM_TYPE drm_mode;
   SecMemHandle secmem_handle;
#endif
} AV_PATH_STATUS;

typedef struct
{
   U8BIT  decoder;
   BOOLEAN decoder_id_valid;
   U32BIT decoder_id;
   BOOLEAN sync_id_valid;
   U32BIT sync_id;
} S_VIDEO_DECODER_PRIV_DATA;

#if 0
typedef struct
{
   AM_VOUT_Format_t am_format;
   E_STB_AV_VIDEO_FORMAT format;
} S_VIDEO_MODE;
#endif

/*---local (static) variable declarations for this file----------------------*/
static AV_PATH_STATUS *av_paths_status = NULL;
static void** video_surface = NULL;

static BOOLEAN av_start_flag = FALSE;
static U8BIT num_paths = 0;


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

static S_VIDEO_MODE video_modes[] =
{
   {HW_AM_VOUT_FORMAT_576I, VIDEO_FORMAT_576IHD},
   {HW_AM_VOUT_FORMAT_576P, VIDEO_FORMAT_576PHD},
   {HW_AM_VOUT_FORMAT_720P, VIDEO_FORMAT_720P50HD},
   {HW_AM_VOUT_FORMAT_1080I, VIDEO_FORMAT_1080IHD},
   {HW_AM_VOUT_FORMAT_1080P, VIDEO_FORMAT_1080P50HD}
};

/*---local function prototypes for this file---------------------------------*/
static void AVEventHandler(void *user_data, am_tsplayer_event *event);
am_tsplayer_result AV_CreateTsPlayer(U8BIT path, am_tsplayer_input_source_type source_type, int32_t dmx_dev_id, int32_t event_mask);
am_tsplayer_result AV_ReleaseTsPlayer(U8BIT path);
am_tsplayer_result AV_GetPlayerHandleByPath(U8BIT video, U8BIT audio, am_tsplayer_handle * play_hdle, BOOLEAN recreat_hdl);
am_tsplayer_result AV_StartAudioDecode(am_tsplayer_handle player_hdle, U16BIT a_pid, am_tsplayer_audio_codec format, am_tsplayer_audio_stereo_mode audio_mode, U8BIT vol, BOOLEAN mute);
am_tsplayer_result AV_SetAudioDecode(am_tsplayer_handle player_hdle, am_tsplayer_audio_stereo_mode audio_mode, U8BIT vol, BOOLEAN mute);
am_tsplayer_result AV_StartVideoDecode(am_tsplayer_handle player_hdle, U16BIT v_pid, U16BIT pcr_pid, am_tsplayer_video_codec format, am_tsplayer_avsync_mode mode);

static int AV_SetAudioVolume(am_tsplayer_handle player_handle, U8BIT vol, BOOLEAN mute);

static E_STB_AV_VIDEO_CODEC toVideoCodec(am_tsplayer_video_codec codec);
static E_STB_AV_AUDIO_CODEC toAudioCodec(am_tsplayer_audio_codec codec);
U8BIT STB_AVGetPath(U8BIT video_decoder, U8BIT audio_decoder);


/*---global function definitions----------------------------------------------*/

/**
 * @brief   Initialises the AV components
 * @param   audio_paths The number of audio paths
 * @param   video_paths The number of video paths
 */
void STB_AVInitialise(U8BIT audio_paths, U8BIT video_paths)
{
   U16BIT av_path;
   am_tsplayer_result ret;
   uint32_t pl_ver_m, pl_ver_l;
   FUNCTION_START(STB_AVInitialise);

   if (av_paths_status == NULL)
   {
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
            av_paths_status[av_path].decoder = av_path;
            av_paths_status[av_path].source = AV_DEMUX;
            av_paths_status[av_path].injecting = FALSE;
            av_paths_status[av_path].iframe_shown = FALSE;
            av_paths_status[av_path].iframe_codec = AV_VIDEO_CODEC_AUTO;
            av_paths_status[av_path].iframe_data = NULL;
            av_paths_status[av_path].audio_descriptor_active = FALSE;
            av_paths_status[av_path].av_decoder_state = DECODER_A_STOP_V_STOP;
            av_paths_status[av_path].audio_pid = INVALID_PID;
            av_paths_status[av_path].video_pid = INVALID_PID;
            av_paths_status[av_path].ad_pid = INVALID_PID;
            av_paths_status[av_path].pcr_pid = INVALID_PID;
            av_paths_status[av_path].player_handle = INVALID_PLAYER_HANDLE;
            av_paths_status[av_path].volume = 100;
            av_paths_status[av_path].mute = FALSE;
            av_paths_status[av_path].audio_mode = AV_AUDIO_STEREO_TSP;
            av_paths_status[av_path].video_decoder = INVALID_RES_ID;
            av_paths_status[av_path].audio_decoder = INVALID_RES_ID;

            av_paths_status[av_path].display_info.screen_width = 1920;
            av_paths_status[av_path].display_info.screen_height = 1080;
            av_paths_status[av_path].display_info.screen_aspect_ratio = ASPECT_RATIO_16_9;
#ifdef SUPPORT_CAS
            av_paths_status[av_path].drm_mode = DRM_NONE;
            av_paths_status[av_path].secmem_handle = (SecMemHandle)NULL;
#endif
            pthread_rwlock_init(&av_paths_status[av_path].lock, NULL);
         }
         S_DISPLAY_INFO display_info;

         display_info.screen_width = 1920;
         display_info.screen_height = 1080;
         display_info.screen_aspect_ratio = ASPECT_RATIO_16_9;
         STB_OSDResize(FALSE, display_info.screen_width, display_info.screen_height, 0, 0);
         STB_OSSendEvent(FALSE, HW_EV_CLASS_HDMI, HW_EV_TYPE_HDMI_CONNECT, NULL, 0);

         ret = AmTsPlayer_getVersion(&pl_ver_m, &pl_ver_l);
         if (ret == AM_TSPLAYER_OK)
             AV_DBG("TsPlayer version:%d.%d", pl_ver_m,pl_ver_l);
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
   av_paths_status[av_path].callback = callback;
   av_paths_status[av_path].user_data = user_data;

   /*
   if ((callback != NULL) 
    && (av_paths_status[av_path].display_info.screen_width != 0)
    && (av_paths_status[av_path].display_info.screen_height != 0))
   {
      info.flags = VIDEO_INFO_SCREEN_RESOLUTION | VIDEO_INFO_DISPLAY_ASPECT_RATIO;
      info.screen_width = av_paths_status[av_path].display_info.screen_width;
      info.screen_height = av_paths_status[av_path].display_info.screen_height;
      info.display_aspect_ratio = av_paths_status[av_path].display_info.screen_aspect_ratio;
      VID_DBG("av_path(%u) res sucess", av_path);
      av_paths_status[av_path].callback(&info,
                                      av_paths_status[av_path].user_data,
                                      av_paths_status[av_path].video_decoder);
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
   int i;
   BOOLEAN acquired = FALSE;

   FUNCTION_START(STB_AVAcquirePath);

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
 * @param   path the video path to be configured
 * @param   blank TRUE to blank, FALSE to unblank
 */
void STB_AVBlankVideo(U8BIT path, BOOLEAN blank)
{
   am_tsplayer_result ret;
   am_tsplayer_handle player_handle;
   FUNCTION_START(STB_AVBlankVideo);
   U8BIT av_path = INVALID_RES_ID;

   ret = AV_GetPlayerHandleByPath(path, INVALID_RES_ID, &player_handle, FALSE);
   if (ret != AM_TSPLAYER_OK)
   {
       VID_DBG("Cannot get player handle video_decoder[%d]", path);
       return;
   }

   if (blank == TRUE)
   {
       ret = AmTsPlayer_hideVideo(player_handle);
       if (ret != AM_TSPLAYER_OK)
       {
           AUD_DBG("Hide video failed, err:%d", ret);
       }
   }
   else
   {
       ret = AmTsPlayer_showVideo(player_handle);
       if (ret != AM_TSPLAYER_OK)
       {
           AUD_DBG("Show video failed, err:%d", ret);
       }
   }

   VID_DBG("blank=%u ret=%d", blank, ret);
   FUNCTION_FINISH(STB_AVBlankVideo);
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
   am_tsplayer_result ret;
   am_tsplayer_handle player_handle;
   U8BIT av_path = STB_AVGetPath(INVALID_RES_ID, path);

   FUNCTION_START(STB_AVSetAudioVolume);

   AUD_DBG("set volume: %d:[-:%d] vol[%d]", av_path, path, vol);

   if (av_path != INVALID_RES_ID)
   {
      av_paths_status[av_path].volume = vol;
      av_paths_status[av_path].mute = (vol == 0) ? TRUE : FALSE;
   }

   ret = AV_GetPlayerHandleByPath(INVALID_RES_ID, path, &player_handle, FALSE);
   if (ret != AM_TSPLAYER_OK)
   {
       AUD_DBG("Cannot get player handle audio path[%d]", path);
       return;
   }

   ret = AV_SetAudioVolume(player_handle, vol, av_paths_status[av_path].mute);

   FUNCTION_FINISH(STB_AVSetAudioVolume);
}

/**
 * @brief   Gets the current volume of the audio output
 * @param   path The audio path to query
 * @return  audio volume (0-100%)
 */
U8BIT STB_AVGetAudioVolume(U8BIT path)
{
   am_tsplayer_result ret;
   am_tsplayer_handle player_handle;
   FUNCTION_START(STB_AVGetAudioVolume);

   ret = AV_GetPlayerHandleByPath(INVALID_RES_ID, path, &player_handle, FALSE);
   if (ret != AM_TSPLAYER_OK)
   {
       AUD_DBG("Cannot get player handle audio:[%d]", path);
       return av_paths_status[path].volume;
   }
   U32BIT vol = 0;

   ret = AmTsPlayer_getAudioVolume(player_handle, &vol);
   if (ret == AM_TSPLAYER_OK)
   {
       AUD_DBG("Get audio volume, vol:%d", vol);
   }
   else
   {
       AUD_DBG("Get audio volume failed, err:%d", ret);
   }

   U8BIT av_path = STB_AVGetPath(INVALID_RES_ID, path);
   if (av_path != INVALID_RES_ID)
   {
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
void STB_AVSetAudioMute(U8BIT path, BOOLEAN mute)
{
   am_tsplayer_result ret;
   am_tsplayer_handle player_handle;
   U8BIT av_path = STB_AVGetPath(INVALID_RES_ID, path);

   FUNCTION_START(STB_AVSetAudioMute);

   AUD_DBG("set mute: %d:[-:%d] mute[%d]", av_path, path, mute);

   if (av_path != INVALID_RES_ID)
   {
      av_paths_status[av_path].mute = mute;
   }

   ret = AV_GetPlayerHandleByPath(INVALID_RES_ID, path, &player_handle, FALSE);
   if (ret != AM_TSPLAYER_OK)
   {
       AUD_DBG("Cannot get player handle audio[%d]", path);
       return;
   }

   ret = AmTsPlayer_setAudioMute(player_handle, mute, mute);
   if (ret != AM_TSPLAYER_OK)
   {
       AUD_DBG("Set audio mute failed, err:%d", ret);
   }
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
   am_tsplayer_result ret;
   bool_t analog_mute, digital_mute;
   am_tsplayer_handle player_handle;
   FUNCTION_START(STB_AVGetAudioMute);

   ret = AV_GetPlayerHandleByPath(INVALID_RES_ID, path, &player_handle, FALSE);
   if (ret != AM_TSPLAYER_OK)
   {
       AUD_DBG("Cannot get player handle[%d]", path);
       return FALSE;
   }

   ret = AmTsPlayer_getAudioMute(player_handle, &analog_mute, &digital_mute);
   if (ret == AM_TSPLAYER_OK)
   {
      AUD_DBG("Get audio mute, mute:%d", digital_mute);
      if (digital_mute)
          retval = TRUE;
      else
          retval = FALSE;
   }
   else
   {
      AUD_DBG("Get audio mute failed, err:%d", ret);
   }

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
   am_tsplayer_result ret;
   am_tsplayer_handle player_handle;
   am_tsplayer_audio_stereo_mode audio_mode;
   U8BIT av_path = STB_AVGetPath(INVALID_RES_ID, path);

   FUNCTION_START(STB_AVChangeAudioMode);

   AUD_DBG("set stereo mode %d[-:%d] mode[%d]", av_path, path, mode);

   switch (mode)
   {
      case AV_AUDIO_STEREO:
         audio_mode = AV_AUDIO_STEREO_TSP;
         break;
      case AV_AUDIO_RIGHT:
         audio_mode = AV_AUDIO_RIGHT_TSP;
         break;
      case AV_AUDIO_LEFT:
         audio_mode = AV_AUDIO_LEFT_TSP;
         break;
      case AV_AUDIO_MONO:
         audio_mode = AV_AUDIO_SWAP;
         break;
      case AV_AUDIO_MULTICHANNEL:
         audio_mode = AV_AUDIO_LRMIX;
         break;
      default:
         AUD_DBG("Not support audio mode:%d", mode);
         return;
   }

   if (av_path != INVALID_RES_ID)
   {
      av_paths_status[av_path ].audio_mode = audio_mode;
   }

   ret = AV_GetPlayerHandleByPath(INVALID_RES_ID, path, &player_handle, FALSE);
   if (ret != AM_TSPLAYER_OK)
   {
       AUD_DBG("Cannot get player handle audio[%d]", path);
       return;
   }

   ret = AmTsPlayer_setAudioStereoMode(player_handle, audio_mode);
   if (ret != AM_TSPLAYER_OK)
   {
      AUD_DBG("Set aduio stereo mode[%d] failed, err:%d", audio_mode, ret);
      return;
   }
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
   am_tsplayer_result ret;
   am_tsplayer_handle player_handle;
   am_tsplayer_audio_codec audio_format;

   FUNCTION_START(STB_AVStartAudioDecoding);

   U8BIT av_path = STB_AVGetPath(INVALID_RES_ID, path);
   if (av_path == INVALID_RES_ID) {
      AUD_DBG("audio decoder get path error audio=%u av_path=%u", path, av_path);
      return;
   }
   if (STB_PVRIsPlayStopped(av_paths_status[av_path].audio_decoder, av_paths_status[av_path].video_decoder))
   {
      AUD_DBG("audio decoder path=%u av_path=%u", path, av_path);

      DMXGetDecodePIDs(av_paths_status[av_path].demux, &pcr_pid, &video_pid, &audio_pid, &ad_pid, &preselection_id);
      audio_format = av_paths_status[av_path].audio_format;

      if (audio_pid != 0 && audio_pid != INVALID_PID)
      {
         ret = AV_GetPlayerHandleByPath(av_paths_status[av_path].video_decoder, path, &player_handle, TRUE);
         if (ret != AM_TSPLAYER_OK)
         {
             AUD_DBG("Cannot get player handle[%d]", path);
             return;
         }
         switch (av_paths_status[av_path].av_decoder_state)
         {
         case DECODER_A_START_V_STOP:
         case DECODER_A_START_V_START:
            AUD_DBG("A NOW:A_START: start audio PID=%u FMT:%d", audio_pid, audio_format);
            if (audio_pid != av_paths_status[av_path].audio_pid)
            {
                AUD_DBG("changing audio PID %u->%d", av_paths_status[av_path].audio_pid, audio_pid);
                AmTsPlayer_stopAudioDecoding(player_handle);
                ret = AV_StartAudioDecode(player_handle,
                                          audio_pid,
                                          audio_format,
                                          av_paths_status[av_path].audio_mode,
                                          av_paths_status[av_path].volume,
                                          av_paths_status[av_path].mute);
                if (ret == AM_TSPLAYER_OK)
                    av_paths_status[av_path].audio_pid = audio_pid;
            }
            break;

         case DECODER_A_STOP_V_START:
            /*starting audio when video is already started*/
            AUD_DBG("A NOW:A_STOP_V_START: start audio PID=%u FMT:%d", audio_pid, audio_format);
            ret = AV_StartAudioDecode(player_handle,
                                      audio_pid,
                                      audio_format,
                                      av_paths_status[av_path].audio_mode,
                                      av_paths_status[av_path].volume,
                                      av_paths_status[av_path].mute);
            if (ret == AM_TSPLAYER_OK)
            {
                av_paths_status[av_path].audio_pid = audio_pid;
                av_paths_status[av_path].av_decoder_state = DECODER_A_START_V_START;
                STB_OSSendEvent(FALSE, HW_EV_CLASS_DECODE, HW_EV_TYPE_AUDIO_STARTED, &path, sizeof(U8BIT));
            }
            break;

         case DECODER_A_STOP_V_STOP:
            AUD_DBG("A NOW:A_STOP_V_STOP: start audio PID=%u FMT:%d", audio_pid, audio_format);
            ret = AV_StartAudioDecode(player_handle,
                                      audio_pid,
                                      audio_format,
                                      av_paths_status[av_path].audio_mode,
                                      av_paths_status[av_path].volume,
                                      av_paths_status[av_path].mute);
            if (ret == AM_TSPLAYER_OK)
            {
                av_paths_status[av_path].audio_pid = audio_pid;
                av_paths_status[av_path].av_decoder_state = DECODER_A_START_V_STOP;
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
      am_tsplayer_video_codec video_format;
      am_tsplayer_audio_codec ad_format;

      DMXGetDecodePIDs(av_paths_status[av_path].demux, &pcr_pid, &video_pid, &audio_pid, &ad_pid, &preselection_id);
      audio_format = av_paths_status[av_path].audio_format;
      ad_format = av_paths_status[av_path].ad_format;
      video_format = av_paths_status[av_path].video_format;

      AUD_DBG("av-pvr: path=%u state=%u, apid:%d vpid:%d",
                path,
                av_paths_status[av_path].av_decoder_state,
                audio_pid,
                video_pid);

      if (audio_pid != 0 && audio_pid != INVALID_PID)
      {
         ret = AV_GetPlayerHandleByPath(av_paths_status[av_path].video_decoder,
                                        av_paths_status[av_path].audio_decoder,
                                        &player_handle,
                                        TRUE);
         if (ret != AM_TSPLAYER_OK)
         {
            AUD_DBG("Cannot get player handle[%d]av_path[%u]", path, av_path);
            return;
         }
         switch (av_paths_status[av_path].av_decoder_state)
         {
         case DECODER_A_START_V_STOP:
         case DECODER_A_START_V_START:
            AUD_DBG("av-pvr: Audio decoder already started");
            if (audio_pid != av_paths_status[av_path].audio_pid)
            {
                AUD_DBG("av-pvr: changing audio PID %u->%d", av_paths_status[av_path].audio_pid, audio_pid);
                if (PVRChangeDecodePIDs(av_paths_status[av_path].audio_decoder,
                                          av_paths_status[av_path].video_decoder,
                                          pcr_pid,
                                          video_pid,
                                          audio_pid,
                                          ad_pid,
                                          toVideoCodec(video_format),
                                          toAudioCodec(audio_format),
                                          toAudioCodec(ad_format)))
                {
                    AV_SetAudioDecode(player_handle,
                                        av_paths_status[av_path].audio_mode,
                                        av_paths_status[av_path].volume,
                                        av_paths_status[av_path].mute);

                    av_paths_status[av_path].audio_pid = audio_pid;
                }
            }
            break;

         case DECODER_A_STOP_V_START:
            AUD_DBG("av-pvr: video already started, audio PID=%u", audio_pid);
            if (audio_pid != av_paths_status[av_path].audio_pid)
            {
                if (PVRChangeDecodePIDs(av_paths_status[av_path].audio_decoder,
                                          av_paths_status[av_path].video_decoder,
                                          pcr_pid, 
                                          video_pid,
                                          audio_pid,
                                          ad_pid,
                                          toVideoCodec(video_format),
                                          toAudioCodec(audio_format),
                                          toAudioCodec(ad_format)))
                {
                    AV_SetAudioDecode(player_handle,
                                        av_paths_status[av_path].audio_mode,
                                        av_paths_status[av_path].volume,
                                        av_paths_status[av_path].mute);
                    av_paths_status[av_path].audio_pid = audio_pid;
                    av_paths_status[av_path].av_decoder_state = DECODER_A_START_V_START;
                    STB_OSSendEvent(FALSE, HW_EV_CLASS_DECODE, HW_EV_TYPE_AUDIO_STARTED, &path, sizeof(U8BIT));
                }
            }
            break;

         case DECODER_A_STOP_V_STOP:
            AUD_DBG("av-pvr: av all stopped, audio PID=%u", audio_pid);
            /*pvr radio will get here*/
            if (audio_pid != av_paths_status[av_path].audio_pid)
            {
                if (PVRChangeDecodePIDs(av_paths_status[av_path].audio_decoder,
                                          av_paths_status[av_path].video_decoder,
                                          pcr_pid,
                                          video_pid,
                                          audio_pid,
                                          ad_pid,
                                          toVideoCodec(video_format),
                                          toAudioCodec(audio_format),
                                          toAudioCodec(ad_format)))
                {
                    AV_SetAudioDecode(player_handle,
                                        av_paths_status[av_path].audio_mode,
                                        av_paths_status[av_path].volume,
                                        av_paths_status[av_path].mute);
                    av_paths_status[av_path].audio_pid = audio_pid;
                    av_paths_status[av_path].av_decoder_state = DECODER_A_START_V_STOP;
                    STB_OSSendEvent(FALSE, HW_EV_CLASS_DECODE, HW_EV_TYPE_AUDIO_STARTED, &path, sizeof(U8BIT));
                }
            }
            break;
         default:
            break;
         }
      }

   }

   FUNCTION_FINISH(STB_AVStartAudioDecoding);
}

#ifdef SUPPORT_CAS
/*
 *
 Mode on the given video path
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
   U16BIT video_pid, audio_pid, pcr_pid, ad_pid;
   U8BIT preselection_id;
   am_tsplayer_result ret;
   am_tsplayer_handle player_handle;
   am_tsplayer_video_codec video_format;
   am_tsplayer_video_params video_param;
   FUNCTION_START(STB_AVStartVideoDecoding);

   U8BIT av_path = STB_AVGetPath(path, INVALID_RES_ID);
   if (av_path == INVALID_RES_ID) {
      AUD_DBG("video decoder get path error video=%u av_path=%u", path, av_path);
      return;
   }

   if (STB_PVRIsPlayStopped(av_paths_status[av_path].audio_decoder, av_paths_status[av_path].video_decoder))
   {
      VID_DBG("video path=%u av_path=%u", path, av_path);
      DMXGetDecodePIDs(av_paths_status[av_path].demux, &pcr_pid, &video_pid, &audio_pid, &ad_pid, &preselection_id);
      video_format = av_paths_status[av_path].video_format;

      if (video_pid != 0)
      {
         ret = AV_GetPlayerHandleByPath(av_paths_status[av_path].video_decoder, 
                                        av_paths_status[av_path].audio_decoder, 
                                        &player_handle, 
                                        TRUE);
         if (ret != AM_TSPLAYER_OK)
         {
             VID_DBG("Cannot get TsPlayer. video path:%d", path);
             return;
         }
         if (video_surface[av_path] != NULL) {
            VID_DBG("set tsplayer surface %d:[%d:%d] [%p], player[0x%zx]",
               av_path,
               av_paths_status[av_path].video_decoder,
               av_paths_status[av_path].audio_decoder,
               video_surface[av_path],
               player_handle);
            AmTsPlayer_setSurface(player_handle,video_surface[av_path]);
         } else {
            VID_DBG("Cannot set surface to TsPlayer, surface is NULL. video path:(%d:%d)",
               av_path, av_paths_status[av_path].video_decoder);
         }
         switch (av_paths_status[av_path].av_decoder_state)
         {
         case DECODER_A_STOP_V_START:
         case DECODER_A_START_V_START:
            VID_DBG("V NOW:V_START, Video decoder already started");
            if (video_pid != av_paths_status[av_path].video_pid)
            {
                VID_DBG("#### video PID changed %u->%u, decoding restarted, FMT:%d ####", av_paths_status[av_path].video_pid, video_pid, video_format);
                AmTsPlayer_stopVideoDecoding(player_handle);
                ret = AV_StartVideoDecode(player_handle, video_pid, pcr_pid, video_format, TS_SYNC_PCRMASTER);
                if (ret == AM_TSPLAYER_OK)
                {
                    av_paths_status[av_path].video_pid = video_pid;
                    av_paths_status[av_path].pcr_pid = pcr_pid;
                }
            }
            break;
         case DECODER_A_START_V_STOP:
            VID_DBG("V NOW:A_START_V_STOP: start video PID=%u PCR=%u FMT=%d", video_pid, pcr_pid, video_format);
            ret = AV_StartVideoDecode(player_handle, video_pid, pcr_pid, video_format, TS_SYNC_PCRMASTER);
            if (ret == AM_TSPLAYER_OK)
            {
                av_paths_status[av_path].video_pid = video_pid;
                av_paths_status[av_path].pcr_pid = pcr_pid;
                av_paths_status[av_path].av_decoder_state = DECODER_A_START_V_START;
            }
            break;
         case DECODER_A_STOP_V_STOP:
            VID_DBG("V NOW:A_STOP_V_STOP: start video PID=%u PCR=%u FMT=%d", video_pid, pcr_pid, video_format);
            ret = AV_StartVideoDecode(player_handle, video_pid, pcr_pid, video_format, TS_SYNC_PCRMASTER);
            if (ret == AM_TSPLAYER_OK)
            {
                av_paths_status[av_path].audio_pid = INVALID_PID;
                av_paths_status[av_path].video_pid = video_pid;
                av_paths_status[av_path].pcr_pid = pcr_pid;
                av_paths_status[av_path].av_decoder_state = DECODER_A_STOP_V_START;
            }
            break;
         default:
            break;
         }
      }
   }
   else
   {
      am_tsplayer_audio_codec audio_format;
      am_tsplayer_audio_codec ad_format;

      DMXGetDecodePIDs(av_paths_status[av_path].demux, &pcr_pid, &video_pid, &audio_pid, &ad_pid, &preselection_id);
      audio_format = av_paths_status[av_path].audio_format;
      ad_format = av_paths_status[av_path].ad_format;
      video_format = av_paths_status[av_path].video_format;

      VID_DBG("av-pvr: path=%u state=%u, apid:%d vpid:%d", path, av_paths_status[av_path].av_decoder_state, audio_pid, video_pid);

      if (audio_pid == 0)
      {
         audio_pid = INVALID_PID;
      }

      if (video_pid != 0)
      {
         switch (av_paths_status[av_path].av_decoder_state)
         {
         case DECODER_A_STOP_V_START:
         case DECODER_A_START_V_START:
            /*Just in case we get two calls to audio start without a stop
              There's an API to switch, so we'll use it*/
            VID_DBG("av-pvr: Video decoder already started");
            if (video_pid != av_paths_status[av_path].video_pid)
            {
               VID_DBG("av-pvr: video PID changed %u->%u, notify to pvr", av_paths_status[av_path].video_pid, video_pid);
               if (PVRChangeDecodePIDs(av_paths_status[av_path].audio_decoder,
                                         av_paths_status[av_path].video_decoder,
                                         pcr_pid,
                                         video_pid,
                                         audio_pid,
                                         ad_pid,
                                         toVideoCodec(video_format),
                                         toAudioCodec(audio_format),
                                         toAudioCodec(ad_format)))
               {
                  av_paths_status[av_path].video_pid = video_pid;
               }
            }
            break;
         case DECODER_A_START_V_STOP:
            VID_DBG("av-pvr: audio already started, video PID=%u", video_pid);

            if (video_pid != av_paths_status[av_path].video_pid)
            {
               VID_DBG("av-pvr: video PID changed %u->%u, notify to pvr", av_paths_status[av_path].video_pid, video_pid);
               if (PVRChangeDecodePIDs(av_paths_status[av_path].video_decoder,
                                         av_paths_status[av_path].audio_decoder,
                                         pcr_pid,
                                         video_pid,
                                         audio_pid,
                                         ad_pid,
                                         toVideoCodec(video_format),
                                         toAudioCodec(audio_format),
                                         toAudioCodec(ad_format)))
               {
                  av_paths_status[av_path].video_pid = video_pid;
                  av_paths_status[av_path].av_decoder_state = DECODER_A_START_V_START;
               }
            }
            break;
         case DECODER_A_STOP_V_STOP:
            VID_DBG("av-pvr: video PID=%u, PCR=%u", video_pid, pcr_pid);
            if (PVRChangeDecodePIDs(av_paths_status[av_path].audio_decoder,
                                      av_paths_status[av_path].video_decoder,
                                      pcr_pid,
                                      video_pid,
                                      audio_pid,
                                      ad_pid,
                                      toVideoCodec(video_format),
                                      toAudioCodec(audio_format),
                                      toAudioCodec(ad_format)))
            {
               av_paths_status[av_path].video_pid = video_pid;
               av_paths_status[av_path].av_decoder_state = DECODER_A_STOP_V_START;
            }
            break;
         default:
            break;
         }
      }
   }

   FUNCTION_FINISH(STB_AVStartVideoDecoding);
}

/**
 * @brief   Pause video decoding
 * @param   path Required Decode Path Number.
 */
void  STB_AVPauseVideoDecoding(U8BIT path)
{
   FUNCTION_START(STB_AVPauseVideoDecoding);
#if 0
   if (av_paths_status[path].injecting)
   {
      AM_AV_PauseInject(path);
   }
   else
   {
      switch (av_paths_status[path].av_decoder_state)
      {
         case DECODER_A_START_V_START:
         {
            /*restart the decoder in audio only, but leave last frame onscreen*/
            AM_AV_SetTSSource(path, av_paths_status[path].demux + AM_AV_TS_SRC_DMX0);
            AM_AV_StartTSWithPCR(path, INVALID_PID, av_paths_status[path].audio_pid, INVALID_PID, -1, av_paths_status[path].audio_format);
            break;
         }
         case DECODER_A_STOP_V_START:
         {
            /*Stop decoding, but leave last frame onscreen*/
            AM_AV_StopTS(path);
            break;
         }
         default:
            VID_DBG("can't pause: already stopped");
            break;
      }
   }
#endif
   FUNCTION_FINISH(STB_AVPauseVideoDecoding);
}

/**
 * @brief   Resume video decoding
 * @param   path Required Decode Path Number.
 */
void  STB_AVResumeVideoDecoding(U8BIT path)
{
   FUNCTION_START(STB_AVResumeVideoDecoding);
#if 0
   VID_DBG("STB_AVResumeVideoDecoding----");


   if (av_paths_status[path].injecting)
   {
      AM_AV_ResumeInject(path);
   }
   else
   {
      switch (av_paths_status[path].av_decoder_state)
      {
         case DECODER_A_START_V_START:
         {
            AM_AV_SetTSSource(path, av_paths_status[path].demux + AM_AV_TS_SRC_DMX0);
            AM_AV_StartTSWithPCR(path, av_paths_status[path].video_pid,
                                 av_paths_status[path].audio_pid,
                                 av_paths_status[path].pcr_pid,
                                 av_paths_status[path].video_format,
                                 av_paths_status[path].audio_format);
            break;
         }
         case DECODER_A_STOP_V_START:
         {
            AM_AV_SetTSSource(path, av_paths_status[path].demux + AM_AV_TS_SRC_DMX0);
            AM_AV_StartTSWithPCR(path, av_paths_status[path].video_pid,
                                 av_paths_status[path].audio_pid,
                                 av_paths_status[path].pcr_pid,
                                 av_paths_status[path].video_format,
                                 av_paths_status[path].audio_format);
            break;
         }
         default:
            VID_DBG("can't resume: already stopped");
            break;
      }
   }
#endif
   FUNCTION_FINISH(STB_AVResumeVideoDecoding);
}

/**
 * @brief   Stops the video decoder
 * @param   path the video decoder path to be stopped
 */
void STB_AVStopVideoDecoding(U8BIT path)
{
   am_tsplayer_result ret;
   S_STB_AV_VIDEO_INFO info;
   FUNCTION_START(STB_AVStopVideoDecoding);
   U8BIT av_path = STB_AVGetPath(path, INVALID_RES_ID);

   VID_DBG("video codec path=%u av_path = %u", path, av_path);
   if (av_path == INVALID_RES_ID) {
     VID_DBG("get av_path error video codec path=%u av_path = %u", path, av_path);
     return;
   }
   info.flags = 0;
#if 0
   if (av_paths_status[path].injecting)
   {
      //AV_StopInjection(path);
   }
   else
#endif
   {
      switch (av_paths_status[av_path].av_decoder_state)
      {
         case DECODER_A_STOP_V_START:
         {
            VID_DBG("V NOW:A_STOP_V_START: Stop Video decoding");
            if (!IS_INVALID_PLAYER_HANDLE(av_path)) {
               ret = AmTsPlayer_stopVideoDecoding(av_paths_status[av_path].player_handle);
               if (ret == AM_TSPLAYER_OK) {
                   AV_ReleaseTsPlayer(av_path);
               }else {
                   VID_DBG("AmTsPlayer_stopVideoDecoding failed, err:%d", ret);
               }
            }
            {
                av_paths_status[av_path].audio_pid = INVALID_PID;
                av_paths_status[av_path].video_pid = INVALID_PID;
                av_paths_status[av_path].ad_pid = INVALID_PID;
                av_paths_status[av_path].pcr_pid = INVALID_PID;
                av_paths_status[av_path].av_decoder_state = DECODER_A_STOP_V_STOP;
                STB_OSSendEvent(FALSE, HW_EV_CLASS_DECODE, HW_EV_TYPE_VIDEO_STOPPED, &path, sizeof(U8BIT));
            }
            info.status = DECODER_STATUS_NONE;
            info.flags = VIDEO_INFO_DECODER_STATUS;
            break;
         }

         case DECODER_A_START_V_START:
         {
            VID_DBG("V NOW:A_START_V_START, Stop Video decoding");
            if (!IS_INVALID_PLAYER_HANDLE(av_path)) {
               ret = AmTsPlayer_stopVideoDecoding(av_paths_status[av_path].player_handle);
               if (ret == AM_TSPLAYER_OK)
               {
               }
            }
            {
                av_paths_status[av_path].av_decoder_state = DECODER_A_START_V_STOP;
                av_paths_status[av_path].video_pid = INVALID_PID;
                av_paths_status[av_path].pcr_pid = INVALID_PID;
                STB_OSSendEvent(FALSE, HW_EV_CLASS_DECODE, HW_EV_TYPE_VIDEO_STOPPED, &path, sizeof(U8BIT));
            }
            info.status = DECODER_STATUS_NONE;
            info.flags = VIDEO_INFO_DECODER_STATUS;
            break;
         }
         case DECODER_A_STOP_V_STOP:
         case DECODER_A_START_V_STOP:
            VID_DBG("V NOW:V_STOP, Stop video decode already");
            break;
      }
   }
   if ((info.flags != 0) && (av_paths_status[av_path].callback != NULL))
   {
      /*reset the afd*/
      info.flags |= VIDEO_INFO_AFD;
      info.afd = 0;

      invokeCallback(&av_paths_status[av_path], &info);
      /*
      av_paths_status[av_path].callback(&info,
                                      av_paths_status[av_path].user_data,
                                      av_paths_status[av_path].video_decoder);
      */
   }

   FUNCTION_FINISH(STB_AVStopVideoDecoding);
}

/**
 * @brief   Stops the audio decoder
 * @param   path the audio decoder path to be stopped
 */
void STB_AVStopAudioDecoding(U8BIT path)
{
   am_tsplayer_result ret;

   FUNCTION_START(STB_AVStopAudioDecoding);
   U8BIT av_path = STB_AVGetPath(INVALID_RES_ID, path);

   AUD_DBG("audio codec path=%u av_path = %u", path, av_path);
   if (av_path == INVALID_RES_ID) {
     VID_DBG("get av_path error audio codec path=%u av_path = %u", path, av_path);
     return;
   }

#if 0
   if (av_paths_status[path].injecting)
   {
      AV_StopInjection(path);
   }
   else
#endif
   {
      switch (av_paths_status[av_path].av_decoder_state)
      {
         case DECODER_A_STOP_V_START:
         case DECODER_A_STOP_V_STOP:
            AUD_DBG("A NOW:A_STOP, Stop audio decode already");
            break;
         case DECODER_A_START_V_STOP:
            AUD_DBG("A NOW:A_START_V_STOP, Stop Audio decoding");
            if (!IS_INVALID_PLAYER_HANDLE(av_path)) {
               ret = AmTsPlayer_stopAudioDecoding(av_paths_status[av_path].player_handle);
               if (ret == AM_TSPLAYER_OK) {
                   AV_ReleaseTsPlayer(av_path);
               }else {
                   AUD_DBG("AmTsPlayer_stopAudioDecoding failed, err:%d", ret);
               }
            }
            {
                av_paths_status[av_path].audio_pid = INVALID_PID;
                av_paths_status[av_path].av_decoder_state = DECODER_A_STOP_V_STOP;
                STB_OSSendEvent(FALSE, HW_EV_CLASS_DECODE, HW_EV_TYPE_AUDIO_STOPPED, &path, sizeof(U8BIT));
            }
            break;
         case DECODER_A_START_V_START:
            AUD_DBG("A NOW:A_START_V_START, Stop Audio decoding");
            if (!IS_INVALID_PLAYER_HANDLE(av_path)) {
               ret = AmTsPlayer_stopAudioDecoding(av_paths_status[av_path].player_handle);
               if (ret == AM_TSPLAYER_OK)
               {
               }
            }
            {
                av_paths_status[av_path].audio_pid = INVALID_PID;
                av_paths_status[av_path].av_decoder_state = DECODER_A_STOP_V_START;
                STB_OSSendEvent(FALSE, HW_EV_CLASS_DECODE, HW_EV_TYPE_AUDIO_STOPPED, &path, sizeof(U8BIT));
            }
            break;
      }
   }

   FUNCTION_FINISH(STB_AVStopAudioDecoding);
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
   uint64_t video_pts;
   am_tsplayer_result ret;
   am_tsplayer_handle player_handle;
   FUNCTION_START(STB_AVGetSTC);

   U8BIT av_path = STB_AVGetPath(path, INVALID_RES_ID);

   VID_DBG("video codec path=%u av_path = %u", path, av_path);
   if (av_path == INVALID_RES_ID) {
     VID_DBG("get av_path error video codec path=%u av_path = %u", path, av_path);
     return;
   }

   pthread_rwlock_rdlock(&av_paths_status[av_path].lock);

   ret = AV_GetPlayerHandleByPath(av_paths_status[av_path].video_decoder,
                                  av_paths_status[av_path].audio_decoder,
                                  &player_handle,
                                  FALSE);
   if (ret != AM_TSPLAYER_OK)
   {
       AUD_DBG("Cannot get player handle video path:[%u] av_path:[%d]", path, av_path);
       pthread_rwlock_unlock(&av_paths_status[av_path].lock);
       return;
   }
   STB_SPDebugWrite(" %s %d", __FUNCTION__, __LINE__);
   ret = AmTsPlayer_getPts(player_handle, TS_STREAM_VIDEO, &video_pts);
   STB_SPDebugWrite(" %s %d", __FUNCTION__, __LINE__);
   if (ret == AM_TSPLAYER_OK)
   {
       memset(stc, 0, 5);
       stc[0] = (U8BIT)((video_pts >> 32) & 0xff);
       stc[1] = (U8BIT)((video_pts >> 24) & 0xff);
       stc[2] = (U8BIT)((video_pts >> 16) & 0xff);
       stc[3] = (U8BIT)((video_pts >> 8) & 0xff);
       stc[4] = (U8BIT)(video_pts & 0xff);
       AUD_DBG("######### %x%x%x%x%x [%u] ########", stc[0],stc[1],stc[2],stc[3],stc[4], video_pts);
   }
   pthread_rwlock_unlock(&av_paths_status[av_path].lock);
   FUNCTION_FINISH(STB_AVGetSTC);
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

   if (video_surface[av_path] != surface)
   {
      video_surface[av_path] = surface;

      {
         am_tsplayer_result ret;
         am_tsplayer_handle player_handle;

         ret = AV_GetPlayerHandleByPath(av_paths_status[av_path].video_decoder,
                                  av_paths_status[av_path].audio_decoder,
                                  &player_handle,
                                  FALSE);
         if (ret == AM_TSPLAYER_OK)
         {
            ret = AmTsPlayer_setSurface(player_handle, surface ? surface : (void*)-1);
            AV_DBG("set tsplayer surface %d:[%d:%d]:[%p] = %d, player[0x%zx]",
               av_path,
               av_paths_status[av_path].video_decoder,
               av_paths_status[av_path].audio_decoder,
               surface,
               ret,
               player_handle);
         }
         else
         {
            AV_DBG("failed to get player handle, %d:[%d:%d]",
               av_path,
               av_paths_status[av_path].video_decoder,
               av_paths_status[av_path].audio_decoder);
         }
      }

      video_surface[av_path] = surface;
   }


   FUNCTION_FINISH(STB_AVSetSurface);

   return success;
}

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
        av_paths_status[av_path].video_format = AV_VIDEO_CODEC_AUTO_TSP;
        VID_DBG("AUTO");
        break;
     case AV_VIDEO_CODEC_H264:
        av_paths_status[av_path].video_format = AV_VIDEO_CODEC_H264_TSP;
        VID_DBG("H264");
        break;
     case AV_VIDEO_CODEC_H265:
        av_paths_status[av_path].video_format = AV_VIDEO_CODEC_H265_TSP;
        VID_DBG("H265");
        break;
     case AV_VIDEO_CODEC_MPEG1:
        av_paths_status[av_path].video_format = AV_VIDEO_CODEC_MPEG1_TSP;
        VID_DBG("MPEG1");
        break;
     case AV_VIDEO_CODEC_MPEG2:
        av_paths_status[av_path].video_format = AV_VIDEO_CODEC_MPEG2_TSP;
        VID_DBG("MPEG2");
        break;
     case AV_VIDEO_CODEC_VP9:
        av_paths_status[av_path].video_format = AV_VIDEO_CODEC_VP9_TSP;
        VID_DBG("VP9");
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
      av_paths_status[av_path].audio_format = AV_AUDIO_CODEC_AC3_TSP;
      AUD_DBG("AC3");
      break;
   case AV_AUDIO_CODEC_EAC3:
      av_paths_status[av_path].audio_format = AV_AUDIO_CODEC_EAC3_TSP;
      AUD_DBG("E-AC3");
      break;
   case AV_AUDIO_CODEC_AC4:
      av_paths_status[av_path].audio_format = AV_AUDIO_CODEC_AC4_TSP;
      AUD_DBG("AC4");
      break;
   case AV_AUDIO_CODEC_AAC:
      av_paths_status[av_path].audio_format = AV_AUDIO_CODEC_AAC_TSP;
      AUD_DBG("AAC");
      break;
   case AV_AUDIO_CODEC_HEAAC:
   case AV_AUDIO_CODEC_HEAACV2:
      av_paths_status[av_path].audio_format = AV_AUDIO_CODEC_LATM_TSP;
      AUD_DBG("LATM");
      break;
      //case AV_AUDIO_CODEC_AUTO:
   case AV_AUDIO_CODEC_MP2:
      av_paths_status[av_path].audio_format = AV_AUDIO_CODEC_MP2_TSP;
      AUD_DBG("MPEG");
      break;
   case AV_AUDIO_CODEC_MP3:
      av_paths_status[av_path].audio_format = AV_AUDIO_CODEC_MP3_TSP;
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

   av_paths_status[av_path].sample_data = STB_MEMGetSysRAM(size);
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
      av_paths_status[av_path].iframe_data = STB_MEMGetSysRAM(size);
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
   am_tsplayer_result result;
   am_tsplayer_video_codec codec;

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
   am_tsplayer_result err;
   am_tsplayer_handle player_handle;
   am_tsplayer_audio_params ad_param;
   U16BIT video_pid, audio_pid, pcr_pid, ad_pid;
   U8BIT preselection_id;
   FUNCTION_START(STB_AVStartADDecoding);
   U8BIT av_path = STB_AVGetPath(INVALID_RES_ID, path);

   AUD_DBG("video codec path=%u av_path = %u", path, av_path);
   if (av_path == INVALID_RES_ID) {
     AUD_DBG("get av_path error video codec path=%u av_path = %u", path, av_path);
     return FALSE;
   }

   err = AV_GetPlayerHandleByPath(av_paths_status[av_path].video_decoder,
                                  av_paths_status[av_path].audio_decoder,
                                  &player_handle,
                                  TRUE);
   if (err != AM_TSPLAYER_OK)
   {
       AUD_DBG("Cannot get player handle av_path[%d]", av_path);
       return FALSE;
   }

   if (STB_PVRIsPlayStopped(av_paths_status[av_path].audio_decoder, av_paths_status[av_path].video_decoder))
   {
      DMXGetDecodePIDs(av_paths_status[av_path].demux,
                         &pcr_pid,
                         &video_pid,
                         &audio_pid,
                         &ad_pid,
                         &preselection_id);

      ad_param.pid = ad_pid;
      ad_param.codectype = av_paths_status[av_path].ad_format;
      err = AmTsPlayer_setADParams(player_handle, &ad_param);
      if (err != AM_TSPLAYER_OK) {
          ret = FALSE;
          AUD_DBG("Set AD Param err:%d, pid[%d] fmt[%d]", err, ad_pid, av_paths_status[av_path].ad_format);
      }else {
          AUD_DBG("Start AD decoding ok, pid[%d] fmt[%d]", ad_pid, av_paths_status[av_path].ad_format);
          av_paths_status[av_path].ad_pid = ad_pid;
      }

      err = AmTsPlayer_enableADMix(player_handle);
      if (err != AM_TSPLAYER_OK) {
          AUD_DBG("Enable AD err:%d", err);
          return FALSE;
      }
   }
   else
   {
      am_tsplayer_video_codec video_format;
      am_tsplayer_audio_codec audio_format;
      am_tsplayer_audio_codec ad_format;

      DMXGetDecodePIDs(av_paths_status[av_path].demux, &pcr_pid, &video_pid, &audio_pid, &ad_pid, &preselection_id);
      audio_format = av_paths_status[av_path].audio_format;
      ad_format = av_paths_status[av_path].ad_format;
      video_format = av_paths_status[av_path].video_format;

      AUD_DBG("av-pvr: path=%u state=%u, adpid:%d vpid:%d",
                av_path,
                av_paths_status[av_path].av_decoder_state,
                audio_pid,
                video_pid);

      if (ad_pid != 0 && ad_pid != INVALID_PID)
      {
         if (PVRChangeDecodePIDs(av_paths_status[av_path].audio_decoder,
                                   av_paths_status[av_path].video_decoder,
                                   pcr_pid,
                                   video_pid,
                                   audio_pid,
                                   ad_pid,
                                   toVideoCodec(video_format),
                                   toAudioCodec(audio_format),
                                   toAudioCodec(ad_format)))
         {
            AUD_DBG("Start AD decoding ok");
         }
         else
         {
            ret = FALSE;
            AUD_DBG("Start AD decoding fail");
         }
      }
   }
   FUNCTION_FINISH(STB_AVStartADDecoding);
   return ret;
}

/**
 * @brief   Stops decoding audio description on the given audio path.
 * @param   path audio decoder path to be stopped
 */
void STB_AVStopADDecoding(U8BIT path)
{
   am_tsplayer_result ret;
   am_tsplayer_audio_params ad_param;
   FUNCTION_START(STB_AVStopADDecoding);
   U8BIT av_path = STB_AVGetPath(INVALID_RES_ID, path);

   AUD_DBG("audiocodec path=%u av_path = %u", path, av_path);
   if (av_path == INVALID_RES_ID) {
     AUD_DBG("get av_path error audio codec path=%u av_path = %u", path, av_path);
     return;
   }

   if (IS_INVALID_PLAYER_HANDLE(av_path))
   {
      AUD_DBG("Invalid player handle");
      return;
   }

   if (STB_PVRIsPlayStopped(av_paths_status[av_path].audio_decoder, av_paths_status[av_path].video_decoder))
   {
      ret = AmTsPlayer_disableADMix(av_paths_status[av_path].player_handle);
      if (ret != AM_TSPLAYER_OK) {
          AUD_DBG("Stop AD decoding err:%d", ret);
      }else {
          AUD_DBG("Stop AD decoding ok");
          av_paths_status[av_path].ad_pid = INVALID_PID;
      }
      if (av_paths_status[av_path].av_decoder_state == DECODER_A_STOP_V_STOP)
      {
          AUD_DBG("NOW:A_STOP_V_STOP, ReleaseTsPlayer");
          AV_ReleaseTsPlayer(av_path);
      }
   }
   else
   {
      am_tsplayer_video_codec video_format;
      am_tsplayer_audio_codec audio_format;
      am_tsplayer_audio_codec ad_format;
      U16BIT video_pid, audio_pid, pcr_pid, ad_pid;
      U8BIT preselection_id;

      DMXGetDecodePIDs(av_paths_status[av_path].demux, &pcr_pid, &video_pid, &audio_pid, &ad_pid, &preselection_id);
      audio_format = av_paths_status[av_path].audio_format;
      video_format = av_paths_status[av_path].video_format;
      ad_format = AV_AUDIO_CODEC_AUTO_TSP;
      ad_pid = INVALID_PID;

      AUD_DBG("av-pvr: path=%u state=%u, adpid:%d vpid:%d", path, av_paths_status[av_path].av_decoder_state, audio_pid, video_pid);

      {
         if (PVRChangeDecodePIDs(av_paths_status[av_path].audio_decoder, 
                                   av_paths_status[av_path].video_decoder,
                                   pcr_pid,
                                   video_pid,
                                   audio_pid,
                                   ad_pid,
                                   toVideoCodec(video_format),
                                   toAudioCodec(audio_format),
                                   toAudioCodec(ad_format)))
         {
            AUD_DBG("Stop AD decoding ok");
         }
         else
         {
            ret = FALSE;
            AUD_DBG("Stop AD decoding fail");
         }
      }
   }
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
      av_paths_status[av_path].ad_format = AV_AUDIO_CODEC_AC3_TSP;
      break;
   case AV_AUDIO_CODEC_EAC3:
      AV_DBG("AD Codec[%d]: EAC3", codec);
      av_paths_status[av_path].ad_format = AV_AUDIO_CODEC_EAC3_TSP;
      break;
   case AV_AUDIO_CODEC_AC4:
      AV_DBG("AD Codec[%d]: AC4", codec);
      av_paths_status[av_path].ad_format = AV_AUDIO_CODEC_AC4_TSP;
      break;
   case AV_AUDIO_CODEC_AAC:
      AV_DBG("AD Codec[%d]: AAC", codec);
      av_paths_status[av_path].ad_format = AV_AUDIO_CODEC_AAC_TSP;
      break;
   case AV_AUDIO_CODEC_HEAAC:
   case AV_AUDIO_CODEC_HEAACV2:
      AV_DBG("AD Codec[%d]: HEAAC/HEAACV2", codec);
      av_paths_status[av_path].ad_format = AV_AUDIO_CODEC_LATM_TSP;
      break;
   case AV_AUDIO_CODEC_MP2 :
      AV_DBG("AD Codec[%d]: MP2", codec);
      av_paths_status[av_path].ad_format = AV_AUDIO_CODEC_MP2_TSP;
      break;
   case AV_AUDIO_CODEC_MP3 :
      AV_DBG("AD Codec[%d]: MP3", codec);
      av_paths_status[av_path].ad_format = AV_AUDIO_CODEC_MP3_TSP;
      break;
   case AV_AUDIO_CODEC_AUTO :
   default:
      AV_DBG("AD Codec[%d]: AUTO/OTHER", codec);
      av_paths_status[av_path].ad_format = AV_AUDIO_CODEC_MP2_TSP;
      success = FALSE;
      break;
   }

   FUNCTION_FINISH(STB_AVSetADCodec);

   return(success);
}

/**
 * @brief   Sets the volume of the audio description output
 * @param   path audio path to be configured
 * @param   vol audio volume (0-100%)
 */
void STB_AVSetADVolume(U8BIT path, U8BIT vol)
{
   am_tsplayer_result ret;
   am_tsplayer_handle player_handle;
   FUNCTION_START(STB_AVSetADVolume);
   U8BIT av_path = STB_AVGetPath(INVALID_RES_ID, path);

   AUD_DBG("audio codec path=%u av_path = %u", path, av_path);
   if (av_path == INVALID_RES_ID) {
     AUD_DBG("get av_path error audio codec path=%u av_path = %u", path, av_path);
     return;
   }

   ret = AV_GetPlayerHandleByPath(av_paths_status[av_path].video_decoder,
                                  av_paths_status[av_path].audio_decoder,
                                  &player_handle,
                                  FALSE);
   if (ret != AM_TSPLAYER_OK)
   {
       AUD_DBG("Cannot get player handle[%d]", av_path);
       return;
   }

   ret = AmTsPlayer_setAudioVolume(player_handle, vol);
   AUD_DBG("SetVolume ad path[%d] vol[%d] err:%d av_path:%u", path,  vol, ret, av_path);

   FUNCTION_FINISH(STB_AVSetADVolume);
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
   am_tsplayer_result ret;
   am_tsplayer_handle handle;
   U8BIT frame_rate = 0;

   FUNCTION_START(STB_AVGetVideoFrameRate);

   VID_DBG("vpath:%u", path);

   ret = AV_GetPlayerHandleByPath(path, INVALID_RES_ID, &handle, FALSE);
   if (ret == AM_TSPLAYER_OK)
   {
      am_tsplayer_video_info info;

      ret = AmTsPlayer_getVideoInfo(handle, &info);
      if (ret == AM_TSPLAYER_OK)
      {
         frame_rate = (U8BIT)info.framerate;
      }
   }

   FUNCTION_FINISH(STB_AVGetVideoFrameRate);
   return frame_rate;
}

/**
 * @brief   Returns the scan type of the video being decoded
 * @param   path video path
 * @return  1: progressive, 0: interlaced, 255: invalid
 */
U8BIT STB_AVGetVideoScanType(U8BIT path)
{
   am_tsplayer_result ret;
   am_tsplayer_handle handle;
   U8BIT scan_type = 0;

   FUNCTION_START(STB_AVGetVideoScanType);

   VID_DBG("vpath:%u", path);

   ret = AV_GetPlayerHandleByPath(path, INVALID_RES_ID, &handle, FALSE);
   if (ret == AM_TSPLAYER_OK)
   {
      am_tsplayer_vdec_stat stat;

      ret = AmTsPlayer_getVideoStat(handle, &stat);
      if (ret == AM_TSPLAYER_OK)
      {
         scan_type =
            ((stat.vf_type & 0x01) == 0x01
            || (stat.vf_type & 0x03) == 0x03
            || (stat.vf_type & 0x08) == 0x08)
            ? 0 : 1;
      }
   }

   FUNCTION_FINISH(STB_AVGetVideoScanType);
   return scan_type;

}


/**
 * @brief Apply the specified copy protection. This function is used for CI+
 * @param copy_protection  - settings to be used for each output
 */
void STB_AVSetCopyProtection(S_STB_AV_COPY_PROTECTION *copy_protection)
{
   FUNCTION_START(STB_AVSetCopyProtection);
   USE_UNWANTED_PARAM(copy_protection);
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
   AV_DBG("[decoding mode]: %d:[%d:%d] = (%d -> %d)",
      av_path,
      video_decoder,
      audio_decoder,
      av_paths_status[av_path].decoding_mode,
      mode);

   if (av_paths_status[av_path].decoding_mode != mode)
   {
      #define IS_CACHED(_m_) ((_m_) & DECODING_MODE_CACHE_ONLY)
      #define IS_AD_ENABLE(_m_) ((_m_) & DECODING_AD_ENABLE)
      #define IS_AUDIO_DISABLE(_m_) ((_m_) & DECODING_AUDIO_DISABLE)

      if (IS_CACHED(av_paths_status[av_path].decoding_mode) != IS_CACHED(mode))
      {
         am_tsplayer_result ret;
         am_tsplayer_handle player_handle;

         ret = AV_GetPlayerHandleByPath(av_paths_status[av_path].video_decoder,
                                  av_paths_status[av_path].audio_decoder,
                                  &player_handle,
                                  FALSE);
         if (ret == AM_TSPLAYER_OK)
         {
            am_tsplayer_work_mode work_mode =
               IS_CACHED(mode) ? TS_PLAYER_MODE_CACHING_ONLY : TS_PLAYER_MODE_NORMAL;
            ret = AmTsPlayer_setWorkMode(player_handle, work_mode);
            AV_DBG("set tsplayer work mode: %d:[%d:%d] [%d] = %d, player[0x%zx]",
               av_path,
               av_paths_status[av_path].video_decoder,
               av_paths_status[av_path].audio_decoder,
               work_mode,
               ret,
               player_handle);
         }
         else
         {
            AV_DBG("failed to get player handle, %d[%d:%d]",
               av_path,
               av_paths_status[av_path].video_decoder,
               av_paths_status[av_path].audio_decoder);
         }
      }

      if (IS_AUDIO_DISABLE(av_paths_status[av_path].decoding_mode) != IS_AUDIO_DISABLE(mode))
      {
         am_tsplayer_result ret;
         am_tsplayer_handle player_handle;

         av_paths_status[av_path].mute = IS_AUDIO_DISABLE(mode) ? TRUE : FALSE;

         ret = AV_GetPlayerHandleByPath(av_paths_status[av_path].video_decoder,
                                    av_paths_status[av_path].audio_decoder,
                                    &player_handle,
                                    FALSE);
         if (ret == AM_TSPLAYER_OK)
         {
            ret = AV_SetAudioVolume(player_handle,
               av_paths_status[av_path].volume,
               av_paths_status[av_path].mute);
         }
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
   U16BIT video_pid, audio_pid, pcr_pid, ad_pid;

   FUNCTION_START(STB_AVSyncDecodingFromPVR);
   U8BIT av_path = STB_AVGetPath(video_decoder, audio_decoder);

   VID_DBG("av_path = [%u:%u] = %u", video_decoder, audio_decoder, av_path);
   if (av_path == INVALID_RES_ID) {
     VID_DBG("get av_path error video codec av_path = %u", av_path);
     return;
   }

   if (!STB_PVRIsPlayStopped(audio_decoder, video_decoder))
   {

      PVRGetDecodePIDs(audio_decoder, video_decoder, &pcr_pid, &video_pid, &audio_pid, &ad_pid);

      if (video_pid == 0)
      {
         video_pid = INVALID_PID;
         pcr_pid = INVALID_PID;
         av_paths_status[av_path].av_decoder_state = DECODER_A_STOP_V_STOP;
      }
      if (audio_pid == 0)
      {
         audio_pid = INVALID_PID;
      }

      if (audio_pid != 0 && audio_pid != INVALID_PID)
      {
         switch (av_paths_status[av_path].av_decoder_state)
         {
         case DECODER_A_STOP_V_STOP:
            av_paths_status[av_path].av_decoder_state = DECODER_A_START_V_STOP;
            break;
         case DECODER_A_STOP_V_START:
            av_paths_status[av_path].av_decoder_state = DECODER_A_START_V_START;
            break;
         default:
            break;
         }
      }

      if (video_pid != 0 && video_pid != INVALID_PID)
      {
         switch (av_paths_status[av_path].av_decoder_state)
         {
         case DECODER_A_START_V_STOP:
            av_paths_status[av_path].av_decoder_state = DECODER_A_START_V_START;
            break;
         case DECODER_A_STOP_V_STOP:
            av_paths_status[av_path].av_decoder_state = DECODER_A_STOP_V_START;
            break;
         default:
            break;
         }
      }

      /*refresh the current pvr pid status*/
      av_paths_status[av_path].video_pid = video_pid;
      av_paths_status[av_path].audio_pid = audio_pid;
      av_paths_status[av_path].pcr_pid = pcr_pid;
      av_paths_status[av_path].ad_pid = ad_pid;

      AV_DBG("av-pvr: state=%u, apid:%d vpid:%d", av_paths_status[av_path].av_decoder_state, audio_pid, video_pid);
   }

   FUNCTION_FINISH(STB_AVSyncDecodingFromPVR);
}

void STB_AVNotifyEventHandler(U8BIT audio_path, U8BIT video_path, void *event, int64_t param)
{
   am_tsplayer_event * evt = NULL;
   FUNCTION_START(STB_AVNotifyEventHandler);
   U8BIT av_path = STB_AVGetPath(video_path, audio_path);
   if (av_path == INVALID_RES_ID) {
     VID_DBG("get av_path error video codec path=%u audio codec:%u av_path = %u", audio_path, video_path, av_path);
     return;
   }

   if (event)
   {
      evt = (am_tsplayer_event *)event;
      AVEventHandler(&av_paths_status[av_path], evt);
   }

   FUNCTION_FINISH(STB_AVNotifyEventHandler);
}

pthread_rwlock_t * STB_AVGetLockByPath(U8BIT path)
{
    FUNCTION_START(STB_AVGetLockByPath);

    if (path < num_paths)
        return &(av_paths_status[path].lock);
    return NULL;

    FUNCTION_FINISH(STB_AVGetLockByPath);
}

/*---local function definitions----------------------------------------------*/

static void AVEventHandler(void *user_data, am_tsplayer_event *event)
{
   AV_PATH_STATUS *status;
   S_STB_AV_VIDEO_INFO info;

   status = (AV_PATH_STATUS *)user_data;
   info.flags = 0;
   if (event)
   {
      switch (event->type)
      {
          case AM_TSPLAYER_EVENT_TYPE_PTS:
          {
              AV_DBG("[evt][%d] AM_TSPLAYER_EVENT_TYPE_PTS: stream_type:%d, pts[%d]\n",
              status->decoder,
              event->event.pts.stream_type,
              event->event.pts.pts);
              break;
          }
          case AM_TSPLAYER_EVENT_TYPE_DTV_SUBTITLE:
          {
              uint8_t* pbuf = event->event.mpeg_user_data.data;
              uint32_t size = event->event.mpeg_user_data.len;
              AV_DBG("[evt][%d] AM_TSPLAYER_EVENT_TYPE_DTV_SUBTITLE: %x-%x-%x-%x ,size %d\n",
              status->decoder,
              pbuf[0], pbuf[1], pbuf[2], pbuf[3], size);
              break;
          }
          case AM_TSPLAYER_EVENT_TYPE_USERDATA_CC:
          {
              uint8_t* pbuf = event->event.mpeg_user_data.data;
              uint32_t size = event->event.mpeg_user_data.len;
              AV_DBG("[evt][%d] AM_TSPLAYER_EVENT_TYPE_USERDATA_CC: %x-%x-%x-%x ,size %d\n",
              status->decoder,
              pbuf[0], pbuf[1], pbuf[2], pbuf[3], size);
			  break;
          }
	      case AM_TSPLAYER_EVENT_TYPE_USERDATA_AFD:
          {
              uint8_t* pbuf = event->event.mpeg_user_data.data;
              uint32_t size = event->event.mpeg_user_data.len;
              AV_DBG("[evt][%d] AM_TSPLAYER_EVENT_TYPE_USERDATA_AFD: %x-%x-%x-%x ,size %d\n",
              status->decoder,
              pbuf[0], pbuf[1], pbuf[2], pbuf[3], size);
              USERDATA_AFD_t afd = *((USERDATA_AFD_t *)pbuf);
              afd.reserved = afd.pts = 0;
              info.flags = VIDEO_INFO_AFD;
              info.afd = afd.af & 0x7;
              VID_DBG("[evt] video afd changed: flg[0x%x] fmt[0x%x]\n", afd.af_flag, afd.af);
              break;
          }
          case AM_TSPLAYER_EVENT_TYPE_VIDEO_CHANGED:
          {
              AV_DBG("[evt][%d] AM_TSPLAYER_EVENT_TYPE_VIDEO_CHANGED: [width:height] [%d x %d] @%d aspectratio[%d]\n",
              status->decoder,
              event->event.video_format.frame_width,
              event->event.video_format.frame_height,
              event->event.video_format.frame_rate,
              event->event.video_format.frame_aspectratio);
              if ((event->event.video_format.frame_width != 0) && (event->event.video_format.frame_height != 0))
              {
                  info.flags |= VIDEO_INFO_VIDEO_RESOLUTION;
                  info.video_width = event->event.video_format.frame_width;
                  info.video_height = event->event.video_format.frame_height;
                  AV_DBG("Video res changed, %u x %u", info.video_width, info.video_height);
                  invokeCallback(status, &info);
                  /*
                  if ((status != NULL) && (status->callback != NULL))
                      status->callback(&info, status->user_data, status->video_decoder);
                  */
              }
              info.flags = 0;
              switch (event->event.video_format.frame_aspectratio)
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
                    VID_DBG("Unhandled video aspect ratio");
                    break;
              }
              info.flags |= VIDEO_INFO_DECODER_STATUS;
              info.status = DECODER_STATUS_VIDEO;
              break;
          }
          case AM_TSPLAYER_EVENT_TYPE_AUDIO_CHANGED:
          {
              AV_DBG("[evt][%d] AM_TSPLAYER_EVENT_TYPE_AUDIO_CHANGED: sample_rate:%d, channels:%d\n",
              status->decoder,
              event->event.audio_format.sample_rate,
              event->event.audio_format.channels);
              break;
          }
          case AM_TSPLAYER_EVENT_TYPE_DATA_LOSS:
          {
              AV_DBG("[evt][%d] AM_TSPLAYER_EVENT_TYPE_DATA_LOSS\n", status->decoder);
              STB_OSSendEvent(FALSE, HW_EV_CLASS_DECODE, HW_EV_TYPE_DECODE_NO_DATA, &status->decoder, sizeof(U8BIT));
              break;
          }
          case AM_TSPLAYER_EVENT_TYPE_DATA_RESUME:
          {
              AV_DBG("[evt][%d] AM_TSPLAYER_EVENT_TYPE_DATA_RESUME\n", status->decoder);
              break;
          }
          case AM_TSPLAYER_EVENT_TYPE_SCRAMBLING:
          {
              AV_DBG("[evt][%d] AM_TSPLAYER_EVENT_TYPE_SCRAMBLING: stream_type:%d is_scramling[%d]\n",
              status->decoder,
              event->event.scramling.stream_type,
              event->event.scramling.scramling);
              if (event->event.scramling.stream_type == TS_STREAM_VIDEO)
              {
                  AV_DBG("Video Scambled");
                  STB_OSSendEvent(FALSE, HW_EV_CLASS_DECODE, HW_EV_TYPE_VIDEO_SCAMBLED, &status->decoder, sizeof(U8BIT));
              }
              else if (event->event.scramling.stream_type == TS_STREAM_AUDIO || event->event.scramling.stream_type == TS_STREAM_AD)
              {
                  AV_DBG("Audio Scambled");
                  STB_OSSendEvent(FALSE, HW_EV_CLASS_DECODE, HW_EV_TYPE_AUDIO_SCAMBLED, &status->decoder, sizeof(U8BIT));
              }
              break;
          }
          case AM_TSPLAYER_EVENT_TYPE_FIRST_FRAME:
          {
              AV_DBG("[evt][%d] AM_TSPLAYER_EVENT_TYPE_FIRST_FRAME: ## VIDEO_AVAILABLE ##\n", status->decoder);
              STB_OSSendEvent(FALSE, HW_EV_CLASS_DECODE, HW_EV_TYPE_VIDEO_STARTED, &status->decoder, sizeof(U8BIT));
              {
                 am_tsplayer_result ret;
                 U32BIT sync_id;

                 ret = AmTsPlayer_getSyncInstansNo(status->player_handle, &sync_id);
                 if (ret != AM_TSPLAYER_OK)
                    sync_id = -1;

                 S_VIDEO_DECODER_PRIV_DATA priv =
                 {
                    .decoder_id_valid = FALSE,
                    .sync_id = sync_id,
                    .sync_id_valid = TRUE,
                 };
                 STB_OSSendEvent(FALSE, HW_EV_CLASS_DECODE, HW_EV_TYPE_VIDEO_DECODER_PRIV_DATA, &priv, sizeof(priv));
              }
              break;
          }
          default:
              break;
      }
      if ((info.flags != 0) && (status != NULL) && (status->callback != NULL))
      {
         invokeCallback(status, &info);
         //status->callback(&info, status->user_data, status->video_decoder);
      }
   }
}

#if 0
BOOLEAN AV_StartInjection(U8BIT path)
{
   AM_ErrorCode_t retval;
   AM_AV_InjectPara_t para;
   U16BIT video_pid, audio_pid, pcr_pid, ad_pid;
   BOOLEAN success = FALSE;

   FUNCTION_START(AV_StartInjection);

   DMXGetDecodePIDs(av_paths_status[path].demux, &pcr_pid, &video_pid, &audio_pid, &ad_pid);

   para.vid_fmt = av_paths_status[path].video_format;
   para.aud_fmt = av_paths_status[path].audio_format;
   para.pkg_fmt = PFORMAT_TS;
   para.vid_id  = video_pid;
   para.aud_id  = audio_pid;
   av_paths_status[path].video_pid = video_pid;
   av_paths_status[path].audio_pid = audio_pid;

   AM_AV_EnableVideo(path);
   retval = AM_AV_SetTSSource(path, AM_AV_TS_SRC_HIU);
   if (retval == AM_SUCCESS)
   {
      retval = AM_AV_StartInject(path, &para);
      if (retval == AM_SUCCESS)
      {
         av_paths_status[path].injecting = TRUE;
         success = TRUE;
      }
      else
      {
         ERR_DBG("AM_AV_StartInject failed, err %d", retval-AM_AV_ERROR_BASE);
      }
   }
   else
   {
      ERR_DBG("AM_AV_SetTSSource failed, err %d", retval-AM_AV_ERROR_BASE);
   }

   FUNCTION_FINISH(AV_StartInjection);
   return success;
}

BOOLEAN AV_StopInjection(U8BIT path)
{
   AM_ErrorCode_t retval;
   BOOLEAN success = FALSE;

   FUNCTION_START(AV_StopInjection);

   if (av_paths_status[path].injecting)
   {
      retval = AM_AV_StopInject(path);
      if (retval != AM_SUCCESS)
      {
         ERR_DBG("AM_AV_StopInject failed, err %d", retval-AM_AV_ERROR_BASE);
      }
      else
      {
         retval = AM_AV_SetTSSource(path, aml_hw_cfg.demux + AM_AV_TS_SRC_DMX0);
         if (retval != AM_SUCCESS)
         {
            ERR_DBG("AM_AV_SetTSSource failed, err %d", retval-AM_AV_ERROR_BASE);
         }
         else
         {
            av_paths_status[path].injecting = FALSE;
            success = TRUE;
            av_paths_status[path].video_pid = INVALID_PID;
            av_paths_status[path].audio_pid = INVALID_PID;
         }
         STB_OSSendEvent(FALSE, HW_EV_CLASS_DECODE, HW_EV_TYPE_VIDEO_STOPPED, &path, sizeof(U8BIT));
      }
   }
   else
   {
       AV_DBG("AM_AV_StopInject while not injecting\n");
   }

   FUNCTION_FINISH(AV_StopInjection);

   return success;
}
#endif
void AV_InjectData(U8BIT path,U8BIT *data, U32BIT size)
{
#if 0
   U8BIT *buffer = data;
   U32BIT left = size;
   U32BIT sent;
   AM_ErrorCode_t retval = AM_SUCCESS;

   if (!av_paths_status[path].injecting)
   {
      AV_StartInjection(path);
   }
   while (left > 0 && retval == AM_SUCCESS)
   {
      sent = left;
      retval = AM_AV_InjectData(path, AM_AV_INJECT_MULTIPLEX , buffer, &sent, -1);
      buffer += sent;
      left -= sent;
   }
   if (retval != AM_SUCCESS)
   {
      ERR_DBG("AM_AV_InjectData failed, err %d", retval-AM_AV_ERROR_BASE);
   }
#endif
}

am_tsplayer_result AV_CreateTsPlayer(U8BIT path,
                       am_tsplayer_input_source_type source_type, int32_t dmx_dev_id, int32_t event_mask)
{
    U32BIT decoder_id;
    am_tsplayer_result ret;
    am_tsplayer_init_params parm;
    am_tsplayer_handle player_handle;

    memset(&parm, 0, sizeof(am_tsplayer_init_params));
    parm.source = source_type;
    parm.dmx_dev_id = dmx_dev_id;
    parm.event_mask = event_mask;
#ifdef SUPPORT_CAS
    /*todo, incorrect convertion!!*/
    parm.drmmode = av_paths_status[path].drm_mode;

    if (parm.drmmode != DRM_NONE)
    {
        AM_CA_PreParam_t param;
        param.dmx_dev = dmx_dev_id;
        STB_CAPVRPlayStart(&param);
        CasSession section_handle;
        STB_CAPVRGetPlaySection(&section_handle);
        AV_DBG("section_handle get playback [%p].", section_handle);
        av_paths_status[path].secmem_handle =
                AM_CA_CreateSecmem(section_handle, SERVICE_LIVE_PLAY, NULL, NULL);
        if (!av_paths_status[path].secmem_handle) {
            AV_DBG("Create live secmem failed.");
        }
    }
#endif
    pthread_rwlock_wrlock(&av_paths_status[path].lock);
    ret = AmTsPlayer_create(parm, &player_handle);
    if (ret == AM_TSPLAYER_OK)
    {
        av_paths_status[path].player_handle = player_handle;
        ret = AmTsPlayer_getInstansNo(player_handle, &decoder_id);
        if (ret != AM_TSPLAYER_OK)
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

        ret = AmTsPlayer_registerCb(player_handle, AVEventHandler, &av_paths_status[path]);
        AV_DBG("Create Ts player success. player_hdle[%d]:0x%zx instance_no:%d dxm_id:%d", path, player_handle, decoder_id, dmx_dev_id);
    }
    else
    {
        av_paths_status[path].player_handle = INVALID_PLAYER_HANDLE;
        AV_DBG("Create Ts player failed, err:%d", ret);
    }
    pthread_rwlock_unlock(&av_paths_status[path].lock);
    return ret;
}

am_tsplayer_result AV_ReleaseTsPlayer(U8BIT path)
{
    am_tsplayer_result ret;
    pthread_rwlock_wrlock(&av_paths_status[path].lock);
    AV_DBG("Will Release Ts player");
    if (IS_INVALID_PLAYER_HANDLE(path))
    {
        AV_DBG("Release Ts player alreadly.");
    }
    else
    {
        ret = AmTsPlayer_registerCb(av_paths_status[path].player_handle, NULL, NULL);
        ret = AmTsPlayer_release(av_paths_status[path].player_handle);
        if (ret != AM_TSPLAYER_OK)
        {
            AV_DBG("Release Ts player failed, err:%d", ret);
        }
        else
        {
            AV_DBG("Release Ts player, player_hdle[%d]:0x%zx", path, av_paths_status[path].player_handle);
            av_paths_status[path].player_handle = INVALID_PLAYER_HANDLE;
        }
#ifdef SUPPORT_CAS
        if (av_paths_status[path].secmem_handle)
        {
            CasSession section_handle;
            STB_CAPVRGetPlaySection(&section_handle);
            AM_CA_DestroySecmem(section_handle, av_paths_status[path].secmem_handle);
            av_paths_status[path].secmem_handle = (SecMemHandle)NULL;
        }
#endif
    }
    pthread_rwlock_unlock(&av_paths_status[path].lock);

    return ret;
}

am_tsplayer_result AV_GetPlayerHandleByPath(U8BIT video_decoder, U8BIT audio_decoder, am_tsplayer_handle * play_hdle, BOOLEAN recreat_hdl)
{
    am_tsplayer_result ret = AM_TSPLAYER_ERROR_MAX;

    if (STB_PVRIsPlayStopped(audio_decoder, video_decoder))
    {
       //get path
       U8BIT av_path = INVALID_RES_ID;
       av_path = STB_AVGetPath(video_decoder, audio_decoder);

       if (av_path != INVALID_RES_ID && !IS_INVALID_PLAYER_HANDLE(av_path)) {
           ret = AM_TSPLAYER_OK;
       }
       else if (recreat_hdl)
       {
           ret = AV_CreateTsPlayer(av_path, TS_DEMOD, av_paths_status[av_path].demux, 0);
           ret = AM_TSPLAYER_OK;

           if (IS_CACHED(av_paths_status[av_path].decoding_mode))
           {
              am_tsplayer_result result =
                 AmTsPlayer_setWorkMode(av_paths_status[av_path].player_handle,
                    TS_PLAYER_MODE_CACHING_ONLY);
              AV_DBG("set tsplayer work mode: %d:[%d:%d] caching_only = %d, player[0x%zx]",
                  av_path,
                  av_paths_status[av_path].video_decoder,
                  av_paths_status[av_path].audio_decoder,
                  result,
                  av_paths_status[av_path].player_handle);
           }
       }
       *play_hdle = av_paths_status[av_path].player_handle;
    }
    else
    {
       if (STB_PVRGetPlayerHandle(video_decoder, audio_decoder, (void **)play_hdle) == TRUE)
          ret = AM_TSPLAYER_OK;
    }

    return ret;
}

am_tsplayer_result AV_StartAudioDecode(am_tsplayer_handle player_hdle, U16BIT a_pid,
                                     am_tsplayer_audio_codec format, am_tsplayer_audio_stereo_mode audio_mode, U8BIT vol, BOOLEAN mute)
{
    am_tsplayer_result ret;
    am_tsplayer_audio_params audio_param;

    audio_param.pid = a_pid;
    audio_param.codectype = format;
    ret = AmTsPlayer_setAudioParams(player_hdle, &audio_param);
    if (ret != AM_TSPLAYER_OK)
    {
        AUD_DBG("Set audio params failed, pid:%d fmt:%d err:%d", a_pid, format, ret);
        return ret;
    }
    ret = AmTsPlayer_setAudioStereoMode(player_hdle, audio_mode);
    if (ret != AM_TSPLAYER_OK)
    {
        AUD_DBG("Set aduio stereo mode[%d] failed, err:%d", audio_mode, ret);
        return ret;
    }
    ret = AV_SetAudioVolume(player_hdle, vol, mute);
    if (ret != AM_TSPLAYER_OK)
    {
        AUD_DBG("Set audio volume[%d] mute[%d] failed, err:%d", vol, mute, ret);
        return ret;
    }
    ret = AmTsPlayer_startAudioDecoding(player_hdle);
    if (ret != AM_TSPLAYER_OK)
    {
        AUD_DBG("Start audio decode failed, pid:%d fmt:%d err:%d, player[0x%zx]", a_pid, format, ret, player_hdle);
        return ret;
    }

    AUD_DBG("Start audio decode, pid:%d fmt:%d, volume[%d], audio_mode[%d], mute[%d], player[0x%zx]", a_pid, format, vol, audio_mode, mute, player_hdle);
    return ret;
}

am_tsplayer_result AV_SetAudioDecode(am_tsplayer_handle player_hdle, am_tsplayer_audio_stereo_mode audio_mode, U8BIT vol, BOOLEAN mute)
{
    am_tsplayer_result ret;

    ret = AmTsPlayer_setAudioStereoMode(player_hdle, audio_mode);
    if (ret != AM_TSPLAYER_OK)
    {
        AUD_DBG("Set aduio stereo mode[%d] failed, err:%d", audio_mode, ret);
        return ret;
    }
    ret = AV_SetAudioVolume(player_hdle, vol, mute);
    if (ret != AM_TSPLAYER_OK)
    {
        AUD_DBG("Set audio volume[%d] mute[%d] failed, err:%d", vol, mute, ret);
        return ret;
    }
    AUD_DBG("Set audio decode, volume[%d], audio_mode[%d], player[0x%zx]", vol, audio_mode, player_hdle);
    return ret;
}

am_tsplayer_result AV_StartVideoDecode(am_tsplayer_handle player_hdle,
                       U16BIT v_pid, U16BIT pcr_pid, am_tsplayer_video_codec format, am_tsplayer_avsync_mode mode)
{
    am_tsplayer_result ret;
    am_tsplayer_video_params video_param;

    ret = AmTsPlayer_setSyncMode(player_hdle, mode);
    if (ret != AM_TSPLAYER_OK)
    {
        VID_DBG("Set sync mode failed, sync_mode:%d err:%d", mode, ret);
        return ret;
    }
    ret = AmTsPlayer_setPcrPid(player_hdle, pcr_pid);
    if (ret != AM_TSPLAYER_OK)
    {
        VID_DBG("Set pcr pid failed, pcr_pid:%d err:%d", pcr_pid, ret);
        return ret;
    }
    video_param.pid = v_pid;
    video_param.codectype = format;
    ret = AmTsPlayer_setVideoParams(player_hdle, &video_param);
    if (ret != AM_TSPLAYER_OK)
    {
        VID_DBG("Set video params failed, v_pid:%d fmt:%d err:%d", v_pid, format, ret);
        return ret;
    }
    ret = AmTsPlayer_startVideoDecoding(player_hdle);
    if (ret != AM_TSPLAYER_OK)
    {
        VID_DBG("Start video decode failed, v_pid:%d pcr_pid:%d fmt:%d sync:%d err:%d, player[0x%zx]", v_pid, pcr_pid, format, mode, ret, player_hdle);
        return ret;
    }

    VID_DBG("Start video decode, v_pid:%d pcr_pid:%d fmt:%d sync:%d, player[0x%zx]", v_pid, pcr_pid, format, mode, player_hdle);
    return ret;
}

static int AV_SetAudioVolume(am_tsplayer_handle player_handle, U8BIT vol, BOOLEAN mute)
{
   am_tsplayer_result ret;

   if (mute)
   {
      ret = AmTsPlayer_setAudioVolume(player_handle, vol);
      if (ret != AM_TSPLAYER_OK)
      {
         AUD_DBG("Set audio volume[%d] failed, err:%d", vol, ret);
         return ret;
      }
      ret = AmTsPlayer_setAudioMute(player_handle, mute, mute);
      if (ret != AM_TSPLAYER_OK)
      {
         AUD_DBG("Set audio mute[%d] failed, err:%d", mute, ret);
      }
   }
   else
   {
      ret = AmTsPlayer_setAudioMute(player_handle, mute, mute);
      if (ret != AM_TSPLAYER_OK)
      {
         AUD_DBG("Set audio mute[%d] failed, err:%d", mute, ret);
      }
      ret = AmTsPlayer_setAudioVolume(player_handle, vol);
      if (ret != AM_TSPLAYER_OK)
      {
         AUD_DBG("Set audio volume[%d] failed, err:%d", vol, ret);
         return ret;
      }
   }

   return ret;
}

static E_STB_AV_VIDEO_CODEC toVideoCodec(am_tsplayer_video_codec codec)
{
   switch (codec)
   {
      case AV_VIDEO_CODEC_MPEG1_TSP: return AV_VIDEO_CODEC_MPEG1;
      case AV_VIDEO_CODEC_MPEG2_TSP: return AV_VIDEO_CODEC_MPEG2;
      case AV_VIDEO_CODEC_H264_TSP: return AV_VIDEO_CODEC_H264;
      case AV_VIDEO_CODEC_H265_TSP: return AV_VIDEO_CODEC_H265;
      case AV_VIDEO_CODEC_VP9_TSP: return AV_VIDEO_CODEC_VP9;
      default:                      return AV_VIDEO_CODEC_AUTO;
   }
   return AV_VIDEO_CODEC_AUTO;
}

static E_STB_AV_AUDIO_CODEC toAudioCodec(am_tsplayer_audio_codec codec)
{
   switch (codec)
   {
      case AV_AUDIO_CODEC_MP2_TSP: return AV_AUDIO_CODEC_MP2;
      case AV_AUDIO_CODEC_MP3_TSP: return AV_AUDIO_CODEC_MP3;
      case AV_AUDIO_CODEC_AC3_TSP: return AV_AUDIO_CODEC_AC3;
      case AV_AUDIO_CODEC_EAC3_TSP: return AV_AUDIO_CODEC_EAC3;
      case AV_AUDIO_CODEC_AC4_TSP: return AV_AUDIO_CODEC_AC4;
      case AV_AUDIO_CODEC_AAC_TSP: return AV_AUDIO_CODEC_AAC;
      case AV_AUDIO_CODEC_LATM_TSP: return AV_AUDIO_CODEC_HEAAC;
      default:                     return AV_AUDIO_CODEC_AUTO;
   }
   return AV_AUDIO_CODEC_MP2;
}

