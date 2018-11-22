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
 * @file    stbhwav.c
 * @date    October 2018
 */

/*---includes for this file--------------------------------------------------*/
/* compiler library header files */
#include <stdio.h>
#include <string.h>

/* third party header files */
#include "am_adp/am_av.h"
#include "am_adp/am_vout.h"
#include "am_adp/am_aout.h"

/* STB header files */
#include "techtype.h"

//#define DEBUG_FUNCTIONS
#include "dbgfuncs.h"
#include "stbhwos.h"
#include "stbhwmem.h"
#include "stbhwav.h"
#include "internal.h"

/*---macro definitions for this file-----------------------------------------*/
//#define AV_DEBUG
//#define VIDEO_DEBUG
//#define AUDIO_DEBUG


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


#define INVALID_PID 0x1fff
#define AOUT_DEV 0
#define VOUT_DEV 0

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
   E_STB_AV_DECODE_SOURCE source;
   U8BIT demux_path;
   E_STB_AV_VIDEO_CODEC video_codec;
   E_STB_AV_AUDIO_CODEC audio_codec;
   E_STB_AV_AUDIO_CODEC ad_codec;
   AM_AV_VFormat_t video_format;
   AM_AV_AFormat_t audio_format;
   AM_AV_AFormat_t ad_format;
   E_AV_DECODER_STATE av_decoder_state;
   U16BIT video_width;
   U16BIT video_height;
   U16BIT screen_width;
   U16BIT screen_height;
   E_STB_AV_ASPECT_RATIO video_aspect_ratio;
   E_STB_AV_ASPECT_RATIO screen_aspect_ratio;
   U8BIT video_afd;
   BOOLEAN iframe_shown;
   U8BIT* iframe_data;
   U32BIT iframe_data_size;
   U16BIT iframe_input_x;
   U16BIT iframe_input_y;
   U16BIT iframe_input_width;
   U16BIT iframe_input_height;
   U16BIT iframe_output_x;
   U16BIT iframe_output_y;
   U16BIT iframe_output_width;
   U16BIT iframe_output_height;
   E_STB_AV_VIDEO_CODEC iframe_codec;
   E_STB_AV_DECODER_STATUS decoder_status;
   void (*callback)(S_STB_AV_VIDEO_INFO *, void *);
   void *user_data;
   U8BIT volume;
   U8BIT* sample_data;
   U32BIT sample_data_size;
   U32BIT loop_count;
   BOOLEAN audio_descriptor_active;
   U16BIT video_pid;
   U16BIT audio_pid;
   U16BIT pcr_pid;
   U16BIT ad_pid;
} AV_PATH_STATUS;

typedef struct
{
   AM_VOUT_Format_t am_format;
   E_STB_AV_VIDEO_FORMAT format;
} S_VIDEO_MODE;

/*---local (static) variable declarations for this file----------------------*/
static AV_PATH_STATUS *av_paths_status = NULL;
static U8BIT num_paths = 0;

static S_VIDEO_MODE video_modes[] =
{
   {AM_VOUT_FORMAT_576I, VIDEO_FORMAT_576IHD},
   {AM_VOUT_FORMAT_576P, VIDEO_FORMAT_576PHD},
   {AM_VOUT_FORMAT_720P, VIDEO_FORMAT_720P50HD},
   {AM_VOUT_FORMAT_1080I, VIDEO_FORMAT_1080IHD},
   {AM_VOUT_FORMAT_1080P, VIDEO_FORMAT_1080P50HD}
};

/*---local function prototypes for this file---------------------------------*/

void formatChangedCallback(long dev_no, int event_type, void *param, void *data);
void stateUpdateCallback(long dev_no, int event_type, void *param, void *data);

/*---global function definitions----------------------------------------------*/

/**
 * @brief   Initialises the AV components
 * @param   audio_paths The number of audio paths
 * @param   video_paths The number of video paths
 */
void STB_AVInitialise(U8BIT audio_paths, U8BIT video_paths)
{
   U16BIT av_path;
   AM_AOUT_OpenPara_t aout_param;
   AM_VOUT_OpenPara_t vout_param;
   AM_AV_OpenPara_t av_param;
   AM_ErrorCode_t retval;

   FUNCTION_START(STB_AVInitialise);

   if (av_paths_status == NULL)
   {
      num_paths = video_paths;

      AV_DBG("video paths=%u", num_paths);

      av_paths_status = (AV_PATH_STATUS*) STB_MEMGetSysRAM(sizeof(AV_PATH_STATUS) * num_paths);
      /* AV paths */
      if (av_paths_status != NULL)
      {
         for (av_path = 0; av_path < num_paths; av_path++)
         {
            //memset(path_status, 0, sizeof(AV_PATH_STATUS));
            av_paths_status[av_path].source = AV_DEMUX;
            av_paths_status[av_path].video_codec = AV_AUDIO_CODEC_AUTO;

            av_paths_status[av_path].video_aspect_ratio = ASPECT_UNDEFINED;
            av_paths_status[av_path].screen_aspect_ratio = ASPECT_RATIO_16_9;
            av_paths_status[av_path].video_afd = 0xFF;
            av_paths_status[av_path].decoder_status = DECODER_STATUS_NONE;

            //av_paths_status[av_path].iframe_shown = FALSE;
            av_paths_status[av_path].iframe_codec = AV_VIDEO_CODEC_AUTO;

            // av_paths_status[av_path].av_mutex = STB_OSCreateMutex();

            av_paths_status[av_path].audio_descriptor_active = FALSE;
            av_paths_status[av_path].av_decoder_state = DECODER_A_STOP_V_STOP;
            av_paths_status[av_path].audio_pid = INVALID_PID;
            av_paths_status[av_path].video_pid = INVALID_PID;
            av_paths_status[av_path].ad_pid = INVALID_PID;
            av_paths_status[av_path].pcr_pid = INVALID_PID;

            av_param.vout_dev_no = 0;
            retval = AM_AV_Open(av_path, &av_param);
            if (retval != AM_SUCCESS)
            {
               AV_DBG("AM_AV_Open failed, err %d", retval);
            }
         }

         memset(&aout_param, 0, sizeof(aout_param));
         memset(&vout_param, 0, sizeof(vout_param));
         AM_AOUT_Open(AOUT_DEV, &aout_param);
         AM_VOUT_Open(VOUT_DEV, &vout_param);
         AM_EVT_Init();
         AM_EVT_Subscribe(av_path, AM_AV_EVT_VIDEO_AVAILABLE, stateUpdateCallback, (void*)(unsigned int)av_path);

         STB_OSSendEvent(FALSE, HW_EV_CLASS_HDMI, HW_EV_TYPE_HDMI_CONNECT, NULL, 0);
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

/**
 * @brief   Register callback for updated video information
 * @param   path video decoder path
 * @param   callback - the callback to call when video information is updated
 * @param   vtc_user_data - user data to pass to the callback
 */
void STB_AVSetVideoCallback(U8BIT path, void (*callback)(S_STB_AV_VIDEO_INFO *, void *),
   void *user_data)
{
   FUNCTION_START(STB_AVSetVideoCallback);
   if (av_paths_status[path].callback)
   {
      AM_EVT_Unsubscribe(path, AM_AV_EVT_VIDEO_ASPECT_RATIO_CHANGED, formatChangedCallback, (void*)(unsigned int)path);
      AM_EVT_Unsubscribe(path, AM_AV_EVT_VIDEO_DISPLAY_MODE_CHANGED, formatChangedCallback, (void*)(unsigned int)path);

   }
   av_paths_status[path].callback = callback;
   av_paths_status[path].user_data = user_data;

   AM_EVT_Subscribe(path, AM_AV_EVT_VIDEO_ASPECT_RATIO_CHANGED, formatChangedCallback, (void*)(unsigned int)path);
   AM_EVT_Subscribe(path, AM_AV_EVT_VIDEO_DISPLAY_MODE_CHANGED, formatChangedCallback, (void*)(unsigned int)path);


   FUNCTION_FINISH(STB_AVSetVideoCallback);
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
   if(dest != NULL)
   {
      printf("transformation: left:%d top:%d width:%d height:%d\n", dest->left, dest->top, dest->width, dest->height);
      AM_AV_SetVideoWindow(path, dest->left, dest->top, dest->width, dest->height);
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
   FUNCTION_START(STB_AVBlankVideo);

   AV_DBG("blank=%u", blank);

   if( blank == TRUE)
   {
      AM_AV_DisableVideo(path);
   }
   else
   {
      AM_AV_EnableVideo(path);
   }

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
 * @param   vol the audio volume (0-100%)
 */
void STB_AVSetAudioVolume(U8BIT path, U8BIT vol)
{
   FUNCTION_START(STB_AVSetAudioVolume);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(vol);
   FUNCTION_FINISH(STB_AVSetAudioVolume);
}

/**
 * @brief   Gets the current volume of the audio output
 * @param   path The audio path to query
 * @return  audio volume (0-100%)
 */
U8BIT STB_AVGetAudioVolume(U8BIT path)
{
   FUNCTION_START(STB_AVGetAudioVolume);
   USE_UNWANTED_PARAM(path);
   FUNCTION_FINISH(STB_AVGetAudioVolume);

   return 0;
}

/**
 * @brief   Mutes or unmutes the audio output
 * @param   path The audio path to be configured
 * @param   mute TRUE to mute, FALSE to unmute
 */
void STB_AVSetAudioMute(U8BIT path, BOOLEAN mute)
{
   FUNCTION_START(STB_AVSetAudioMute);
   if(mute==TRUE)
   {
      AM_AOUT_SetMute(AOUT_DEV, 1);
   }
   else
   {
      AM_AOUT_SetMute(AOUT_DEV, 0);
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
   AM_Bool_t mute;
   BOOLEAN retval;
   FUNCTION_START(STB_AVGetAudioMute);
   AM_AOUT_GetMute(AOUT_DEV, &mute);
   if(mute)
   {
      retval = TRUE;
   }
   else
   {
      retval = FALSE;
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
   FUNCTION_START(STB_AVChangeAudioMode);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(mode);
   FUNCTION_FINISH(STB_AVChangeAudioMode);
}

/**
 * @brief   Starts the Audio decoder
 * @param   path the audio decoder path to be started
 */
void STB_AVStartAudioDecoding(U8BIT path)
{
   U16BIT video_pid, audio_pid, pcr_pid, ad_pid;
   AM_AV_VFormat_t video_format;
   AM_AV_AFormat_t audio_format;
   FUNCTION_START(STB_AVStartAudioDecoding);

   AUD_DBG("path=%u", path);

   DMXGetDecodePIDs(av_paths_status->demux_path, &pcr_pid, &video_pid, &audio_pid, &ad_pid);
   audio_format = av_paths_status[path].audio_format;
   video_format = av_paths_status[path].video_format;

   if(video_pid == 0 )
   {
      video_pid = INVALID_PID;
      pcr_pid = INVALID_PID;
      video_format = -1;
   }

   if(audio_pid != 0 && audio_pid != INVALID_PID)
   {
      switch (av_paths_status[path].av_decoder_state)
      {
      case DECODER_A_START_V_STOP:
      case DECODER_A_START_V_START:
         /*Just in case we get two calls to audio start without a stop
           There's an API to switch, so we'll use it*/
         AUD_DBG("Audio decoder already started");
         if(audio_pid != av_paths_status[path].audio_pid)
         {
            AUD_DBG("changing audio PID %u->%d", av_paths_status[path].audio_pid, audio_pid);

            AM_AV_SwitchTSAudio(path,audio_pid,audio_format);
            av_paths_status[path].audio_pid = audio_pid;
         }
         /*state*/
         break;

      case DECODER_A_STOP_V_START:
         /*starting audio when video is already started*/
         AUD_DBG("video already started, audio PID=%u", audio_pid);
         av_paths_status[path].audio_pid = audio_pid;
         AM_AV_SwitchTSAudio(path,audio_pid,audio_format);
         av_paths_status[path].av_decoder_state = DECODER_A_START_V_START;
         STB_OSSendEvent(FALSE, HW_EV_CLASS_DECODE, HW_EV_TYPE_AUDIO_STARTED, &path, sizeof(U8BIT));
         break;

      case DECODER_A_STOP_V_STOP:
         AUD_DBG("video not started, audio PID=%u, PCR PID=%u", audio_pid, pcr_pid);

         av_paths_status[path].audio_pid = audio_pid;
         av_paths_status[path].video_pid = video_pid;
         av_paths_status[path].pcr_pid = pcr_pid;
         //AM_AV_DisableVideo(path);
         AM_AV_StartTSWithPCR(path, video_pid, audio_pid, pcr_pid, video_format, audio_format);
         av_paths_status[path].av_decoder_state = DECODER_A_START_V_STOP;
         STB_OSSendEvent(FALSE, HW_EV_CLASS_DECODE, HW_EV_TYPE_AUDIO_STARTED, &path, sizeof(U8BIT));
         break;

      default:
         break;
      }
   }
   FUNCTION_FINISH(STB_AVStartAudioDecoding);
}

/**
 * @brief   Starts the video decoder
 * @param   path the video decode path to be started
 */
void STB_AVStartVideoDecoding(U8BIT path)
{
   U16BIT video_pid, audio_pid, pcr_pid, ad_pid;
   AM_AV_VFormat_t video_format;
   AM_AV_AFormat_t audio_format;
   FUNCTION_START(STB_AVStartVideoDecoding);

   VID_DBG("path=%u", path);

   DMXGetDecodePIDs(av_paths_status->demux_path, &pcr_pid, &video_pid, &audio_pid, &ad_pid);
   audio_format = av_paths_status[path].audio_format;
   video_format = av_paths_status[path].video_format;

   if(video_pid == 0 )
   {
      video_pid = INVALID_PID;
      pcr_pid = INVALID_PID;
      video_format = -1;
   }

   AM_AV_SetVideoAspectRatio(path, AM_AV_VIDEO_ASPECT_AUTO);
   AM_AV_SetVideoAspectMatchMode(path,AM_AV_VIDEO_ASPECT_MATCH_COMBINED);
   if(video_pid != 0 && video_pid != INVALID_PID)
   {
      switch (av_paths_status[path].av_decoder_state)
      {
      case DECODER_A_STOP_V_START:
      case DECODER_A_START_V_START:
         /*Just in case we get two calls to audio start without a stop
           There's an API to switch, so we'll use it*/
         VID_DBG("Video decoder already started");
         if (video_pid != av_paths_status[path].video_pid)
         {
            VID_DBG("PID changed but already running");
         }
         /*state*/
         break;
      case DECODER_A_START_V_STOP:
         /*starting video when audio is already started*/
         av_paths_status[path].audio_pid = audio_pid;

         VID_DBG("audio already started, video PID=%u", video_pid);

         if (video_pid != av_paths_status[path].video_pid)
         {
            VID_DBG("video PID changed %u->%u, decoding restarted", av_paths_status[path].video_pid, video_pid);
            AM_AV_StopTS(path);
            AM_AV_StartTSWithPCR(path, video_pid, audio_pid, pcr_pid, video_format, audio_format);
         }
         else
         {
            AM_AV_EnableVideo(path);
         }
         av_paths_status[path].av_decoder_state = DECODER_A_START_V_START;
         av_paths_status[path].video_pid = video_pid;
         av_paths_status[path].pcr_pid = pcr_pid;
         STB_OSSendEvent(FALSE, HW_EV_CLASS_DECODE, HW_EV_TYPE_VIDEO_STARTED, &path, sizeof(U8BIT));
         break;
      case DECODER_A_STOP_V_STOP:
         VID_DBG("audio not started, video PID=%u", video_pid);
         av_paths_status[path].audio_pid = INVALID_PID;
         av_paths_status[path].video_pid = video_pid;
         av_paths_status[path].video_pid = pcr_pid;
#if 0
         /*start with invalid audio pid if the audio decoder shouldn't be started yet*/
         AM_AV_StartTSWithPCR(path, video_pid, INVALID_PID, pcr_pid, video_format, audio_format);
#else
         /*There seems to be a problem with using invalid pids, so just start up the audio decoder early*/
         AM_AV_StartTSWithPCR(path, video_pid, audio_pid, pcr_pid, video_format, audio_format);
#endif
         av_paths_status[path].av_decoder_state = DECODER_A_STOP_V_START;
         STB_OSSendEvent(FALSE, HW_EV_CLASS_DECODE, HW_EV_TYPE_VIDEO_STARTED, &path, sizeof(U8BIT));
         break;
      default:
         break;
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
   USE_UNWANTED_PARAM(path);
   FUNCTION_FINISH(STB_AVPauseVideoDecoding);
}

/**
 * @brief   Resume video decoding
 * @param   path Required Decode Path Number.
 */
void  STB_AVResumeVideoDecoding(U8BIT path)
{
   FUNCTION_START(STB_AVResumeVideoDecoding);
   USE_UNWANTED_PARAM(path);
   FUNCTION_FINISH(STB_AVResumeVideoDecoding);
}

/**
 * @brief   Stops the video decoder
 * @param   path the video decoder path to be stopped
 */
void STB_AVStopVideoDecoding(U8BIT path)
{
   FUNCTION_START(STB_AVStopVideoDecoding);
   VID_DBG("path=%u", path);

   switch (av_paths_status[path].av_decoder_state)
   {
      case DECODER_A_STOP_V_START:
         VID_DBG("audio not running, stop decoding");
         AM_AV_StopTS(path);
         av_paths_status[path].audio_pid = INVALID_PID;
         av_paths_status[path].video_pid = INVALID_PID;
         av_paths_status[path].ad_pid = INVALID_PID;
         av_paths_status[path].pcr_pid = INVALID_PID;
         av_paths_status[path].av_decoder_state = DECODER_A_STOP_V_STOP;
         STB_OSSendEvent(FALSE, HW_EV_CLASS_DECODE, HW_EV_TYPE_VIDEO_STOPPED, &path, sizeof(U8BIT));
         break;
      case DECODER_A_START_V_START:
         VID_DBG("audio still running, just hide the video for now");
         //AM_AV_DisableVideo(path);
         av_paths_status[path].av_decoder_state = DECODER_A_START_V_STOP;
         av_paths_status[path].video_pid = INVALID_PID;
         av_paths_status[path].pcr_pid = INVALID_PID;
         STB_OSSendEvent(FALSE, HW_EV_CLASS_DECODE, HW_EV_TYPE_VIDEO_STOPPED, &path, sizeof(U8BIT));
         break;
      case DECODER_A_STOP_V_STOP:
      case DECODER_A_START_V_STOP:
         VID_DBG("already stopped");
         break;

   }
   FUNCTION_FINISH(STB_AVStopVideoDecoding);
}

/**
 * @brief   Stops the audio decoder
 * @param   path the audio decoder path to be stopped
 */
void STB_AVStopAudioDecoding(U8BIT path)
{
   FUNCTION_START(STB_AVStopAudioDecoding);
   AUD_DBG("path=%u", path);

   switch (av_paths_status[path].av_decoder_state)
   {
     case DECODER_A_STOP_V_START:
     case DECODER_A_STOP_V_STOP:
        AUD_DBG("already stopped");
        break;
      case DECODER_A_START_V_STOP:
         AUD_DBG("video not running, stop decoding");
         AM_AV_StopTS(path);
         STB_OSSendEvent(FALSE, HW_EV_CLASS_DECODE, HW_EV_TYPE_AUDIO_STOPPED, &path, sizeof(U8BIT));
         av_paths_status[path].audio_pid = INVALID_PID;
         av_paths_status[path].av_decoder_state = DECODER_A_STOP_V_STOP;
         break;
      case DECODER_A_START_V_START:
         AUD_DBG("video still running, switch to invalid audio pid");
         AM_AV_SwitchTSAudio(path,INVALID_PID,av_paths_status[path].audio_format);
         av_paths_status[path].audio_pid = INVALID_PID;
         av_paths_status[path].av_decoder_state = DECODER_A_STOP_V_START;
         STB_OSSendEvent(FALSE, HW_EV_CLASS_DECODE, HW_EV_TYPE_AUDIO_STOPPED, &path, sizeof(U8BIT));
         break;
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
   FUNCTION_START(STB_AVGetSTC);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(stc);
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
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(source);
   USE_UNWANTED_PARAM(param);
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
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(source);
   USE_UNWANTED_PARAM(param);
   FUNCTION_FINISH(STB_AVSetAudioSource);
}

/**
 * @brief   Sets the video codec to be used when decoding video with the given video decoder path
 * @param   path video path
 * @param   codec codec to be used
 * @return  TRUE if the codec is supported and is set correctly, FALSE otherwise
 */
BOOLEAN STB_AVSetVideoCodec(U8BIT path, E_STB_AV_VIDEO_CODEC codec)
{
   FUNCTION_START(STB_AVSetVideoCodec);
   BOOLEAN success = FALSE;

   switch(codec)
   {
   case AV_VIDEO_CODEC_H264:
      VID_DBG("H264");
      av_paths_status[path].video_format = VFORMAT_H264;
      success = TRUE;
      break;
   case AV_VIDEO_CODEC_MPEG1:
   case AV_VIDEO_CODEC_MPEG2:
      VID_DBG("MP2");
      //case AV_VIDEO_CODEC_AUTO:
      av_paths_status[path].video_format = VFORMAT_MPEG12;
      success = TRUE;
      break;
   default:
      VID_DBG("Unrecognised video codec %u", codec);
      break;
   }

   FUNCTION_FINISH(STB_AVSetAudioSource);

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
   FUNCTION_START(STB_AVSetVideoCodec);
   BOOLEAN success = FALSE;

   switch(codec)
   {
   case AV_AUDIO_CODEC_AC3:
      av_paths_status[path].audio_format = AFORMAT_AC3;
      success = TRUE;
      AUD_DBG("AC3");
      break;
   case AV_AUDIO_CODEC_AAC:
   case AV_AUDIO_CODEC_HEAAC:
      av_paths_status[path].audio_format = AFORMAT_AAC;
      success = TRUE;
      AUD_DBG("AAC");
      break;
      //case AV_AUDIO_CODEC_AUTO:
   case AV_AUDIO_CODEC_MP2:
   case AV_AUDIO_CODEC_MP3:
      av_paths_status[path].audio_format = AFORMAT_MPEG;
      success = TRUE;
      AUD_DBG("MPEG");
      break;
   default:
      AUD_DBG("Unrecognised audio codec %u", codec);
      break;
   }

   FUNCTION_FINISH(STB_AVSetAudioSource);

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
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(data);
   USE_UNWANTED_PARAM(size);
   FUNCTION_FINISH(STB_AVLoadAudioSample);

   return HW_GEN_ERROR;
}

/**
 * @brief   Plays back a previously loaded audio sample
 * @param   path the audio path to use for playback
 * @param   loop_count the number of times to play the sample, 0=forever
 * @return  E_HW_STATUS code
 */
E_HW_STATUS STB_AVPlayAudioSample(U8BIT path, U32BIT loop_count)
{
   FUNCTION_START(STB_AVPlayAudioSample);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(loop_count);
   FUNCTION_FINISH(STB_AVPlayAudioSample);

   return HW_GEN_ERROR;
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
   USE_UNWANTED_PARAM(path);
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
   FUNCTION_START(STB_AVSetIFrameCodec);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(codec);
   FUNCTION_FINISH(STB_AVSetIFrameCodec);
   return(FALSE);
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
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(data);
   USE_UNWANTED_PARAM(size);
   FUNCTION_FINISH(STB_AVLoadIFrame);
}

/**
 * @brief   Decode and display previously loaded I frame data
 * @param   path the video path to use
 */
void STB_AVShowIFrame(U8BIT path)
{
   FUNCTION_START(STB_AVShowIFrame);
   USE_UNWANTED_PARAM(path);
   FUNCTION_FINISH(STB_AVShowIFrame);
}

/**
 * @brief   Hides a previously shown I frame
 * @param   path the video path containing the I frame
 */
void STB_AVHideIFrame(U8BIT path)
{
   FUNCTION_START(STB_AVHideIFrame);
   USE_UNWANTED_PARAM(path);
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

   return(0);
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

   return(0);
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
   FUNCTION_START(STB_AVGetNextPlaySpeed);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(speed);
   USE_UNWANTED_PARAM(inc);
   USE_UNWANTED_PARAM(include_slow_speeds);
   FUNCTION_FINISH(STB_AVGetNextPlaySpeed);

   return(0);
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
void STB_AVStartADDecoding(U8BIT path)
{
   U16BIT video_pid, audio_pid, pcr_pid, ad_pid;
   FUNCTION_START(STB_AVStartADDecoding);

   DMXGetDecodePIDs(av_paths_status[path].demux_path, &pcr_pid, &video_pid, &audio_pid, &ad_pid);
   AM_AV_SetAudioAd(path,1,ad_pid,av_paths_status[path].ad_format);
   FUNCTION_FINISH(STB_AVStartADDecoding);
}

/**
 * @brief   Stops decoding audio description on the given audio path.
 * @param   path audio decoder path to be stopped
 */
void STB_AVStopADDecoding(U8BIT path)
{
   FUNCTION_START(STB_AVStopADDecoding);

   AM_AV_SetAudioAd(path,0,INVALID_PID,av_paths_status[path].ad_format);
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
   BOOLEAN success = FALSE;
   FUNCTION_START(STB_AVSetADCodec);
   switch(codec)
   {
   case AV_AUDIO_CODEC_AC3:
      av_paths_status[path].ad_format = AFORMAT_AC3;
      success=TRUE;
      break;
   case AV_AUDIO_CODEC_AAC:
      av_paths_status[path].ad_format = AFORMAT_AAC;
      success=TRUE;
      break;
   case AV_AUDIO_CODEC_AUTO :
   case AV_AUDIO_CODEC_MP2 :
   case AV_AUDIO_CODEC_MP3 :
      av_paths_status[path].ad_format = AFORMAT_MPEG;
      success=TRUE;
      break;
   default:
      av_paths_status[path].ad_format = AFORMAT_MPEG;
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
   FUNCTION_START(STB_AVSetADVolume);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(vol);
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

   FUNCTION_FINISH(STB_AVGetHDMISupportedModes);

   return(num_modes);
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
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(width);
   USE_UNWANTED_PARAM(height);
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
   FUNCTION_START(STB_AVGetVideoFrameRate);
   USE_UNWANTED_PARAM(path);
   FUNCTION_FINISH(STB_AVGetVideoFrameRate);
   return 0;
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

/*---local function definitions----------------------------------------------*/

void formatChangedCallback(long dev_no, int event_type, void *param, void *data)
{
   U32BIT path;
   S_STB_AV_VIDEO_INFO info;
   E_STB_AV_VIDEO_INFO_TYPE flags=0;
   AM_AV_VideoAspectRatio_t aspect_ratio;

   path=(U32BIT)data;
   AV_DBG("CALLBACK FIRED!!!");
   AV_DBG("previous AR %d",av_paths_status[path].video_aspect_ratio);
   switch(event_type)
   {
   case AM_AV_EVT_VIDEO_ASPECT_RATIO_CHANGED:
      flags = VIDEO_INFO_VIDEO_ASPECT_RATIO;
      AV_DBG("AR %p",param);
      aspect_ratio = (AM_AV_VideoAspectRatio_t)param;
      av_paths_status[path].video_aspect_ratio = aspect_ratio;
      break;
   case AM_AV_EVT_VIDEO_DISPLAY_MODE_CHANGED:
      flags = VIDEO_INFO_VIDEO_ASPECT_RATIO;
      break;
   case AM_AV_EVT_VIDEO_WINDOW_CHANGED:
      flags = VIDEO_INFO_VIDEO_ASPECT_RATIO;
      break;
   default:
      break;
   }
   AV_DBG("new AR %d",av_paths_status[path].video_aspect_ratio);
   info.flags = flags;
   info.video_width = av_paths_status[path].video_width;
   info.video_height = av_paths_status[path].video_height;
   info.screen_width = av_paths_status[path].screen_width;
   info.screen_height = av_paths_status[path].screen_height;
   info.video_aspect_ratio = av_paths_status[path].video_aspect_ratio;
   info.display_aspect_ratio = av_paths_status[path].screen_aspect_ratio;
   info.afd = av_paths_status[path].video_afd;
   info.status = av_paths_status[path].decoder_status;

   if (av_paths_status[path].callback)
   {
      av_paths_status[path].callback(&info, av_paths_status[path].user_data);
   }
}

void stateUpdateCallback(long dev_no, int event_type, void *param, void *data)
{
   U32BIT path;
   path=(U32BIT)data;
   AV_DBG("CALLBACK FIRED!!!");
   switch(event_type)
   {
   case AM_AV_EVT_VIDEO_AVAILABLE:
      STB_OSSendEvent(FALSE, HW_EV_CLASS_DECODE, HW_EV_TYPE_VIDEO_STARTED, &path, sizeof(U8BIT));
      STB_OSSendEvent(FALSE, HW_EV_CLASS_DECODE, HW_EV_TYPE_AUDIO_STARTED, &path, sizeof(U8BIT));
      break;
   default:
      break;
   }
}

/***
 * End of file
 */
