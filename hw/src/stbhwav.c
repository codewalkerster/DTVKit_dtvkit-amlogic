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
#include "am_adp/am_misc.h"
#include "am_adp/am_userdata.h"

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

/*---macro definitions for this file-----------------------------------------*/
//#define AV_DEBUG
#define VIDEO_DEBUG
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

#define VIDEO_PTS_FILE  "/sys/class/tsync/pts_video"

#define MIN_AV_SPEED    -600
#define MAX_AV_SPEED     600


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
   U8BIT decoder;
   E_STB_AV_DECODE_SOURCE source;
   U8BIT demux;

   AM_AV_VFormat_t video_format;
   AM_AV_AFormat_t audio_format;
   AM_AV_AFormat_t ad_format;
   E_AV_DECODER_STATE av_decoder_state;
   BOOLEAN injecting;

   BOOLEAN iframe_shown;
   U8BIT* iframe_data;
   U32BIT iframe_data_size;
   E_STB_AV_VIDEO_CODEC iframe_codec;

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
static S_DISPLAY_INFO display_info;
static S_VIDEO_MODE video_modes[] =
{
   {AM_VOUT_FORMAT_576I, VIDEO_FORMAT_576IHD},
   {AM_VOUT_FORMAT_576P, VIDEO_FORMAT_576PHD},
   {AM_VOUT_FORMAT_720P, VIDEO_FORMAT_720P50HD},
   {AM_VOUT_FORMAT_1080I, VIDEO_FORMAT_1080IHD},
   {AM_VOUT_FORMAT_1080P, VIDEO_FORMAT_1080P50HD}
};

/*---local function prototypes for this file---------------------------------*/

static void AVEventHandler(long dev_no, int event_type, void *param, void *data);
static AM_ErrorCode_t AVGetVOutDisplay(U32BIT *v_display);


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
   AM_VOUT_Format_t am_format;

   FUNCTION_START(STB_AVInitialise);

   if (av_paths_status == NULL)
   {
      num_paths = video_paths;

      AV_DBG("video paths=%u demux = %d", num_paths, aml_hw_cfg.demux + 5);

      av_paths_status = (AV_PATH_STATUS*) STB_MEMGetSysRAM(sizeof(AV_PATH_STATUS) * num_paths);
      /* AV paths */
      if (av_paths_status != NULL)
      {
         memset(av_paths_status, 0, num_paths * sizeof(AV_PATH_STATUS));

         AM_EVT_Init();

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

            av_param.vout_dev_no = 0;
            av_param.afd_enable = 1;

            retval = AM_AV_Open(av_path, &av_param);
            if (retval != AM_SUCCESS)
            {
               ERR_DBG("AM_AV_Open failed, err %d", retval);
            }
            else
            {
               //need add offset 4,(ts0 ts1 ts2 hiu hiu1 dmx0 dmx1 dmx2)change demux id to enum value
               AM_AV_SetTSSource(av_path, aml_hw_cfg.demux + AM_AV_TS_SRC_DMX0);

               /* Prevent AMLogic AV code from applying any video scaling */
               //AM_FileEcho("/sys/class/video/screen_mode", "5");

               AM_EVT_Subscribe(av_path, AM_AV_EVT_VIDEO_AVAILABLE, AVEventHandler,
                  &av_paths_status[av_path]);
               AM_EVT_Subscribe(av_path, AM_AV_EVT_VIDEO_ASPECT_RATIO_CHANGED, AVEventHandler,
                  &av_paths_status[av_path]);
               AM_EVT_Subscribe(av_path, AM_AV_EVT_VIDEO_RESOLUTION_CHANGED, AVEventHandler,
                  &av_paths_status[av_path]);
               AM_EVT_Subscribe(av_path, AM_AV_EVT_VIDEO_WINDOW_CHANGED, AVEventHandler,
                  &av_paths_status[av_path]);
               AM_EVT_Subscribe(av_path, AM_AV_EVT_VIDEO_AFD_CHANGED, AVEventHandler,
                  &av_paths_status[av_path]);
            }
         }

         memset(&aout_param, 0, sizeof(aout_param));
         memset(&vout_param, 0, sizeof(vout_param));

         AM_AOUT_Open(AOUT_DEV, &aout_param);
         AM_VOUT_Open(VOUT_DEV, &vout_param);

         display_info.screen_width = 1920;
         display_info.screen_height = 1080;
         display_info.screen_aspect_ratio = ASPECT_RATIO_16_9;
         STB_OSDResize(FALSE, display_info.screen_width, display_info.screen_height, 0, 0);

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
void STB_AVSetVideoCallback(U8BIT path, void (*callback)(S_STB_AV_VIDEO_INFO *, void *), void *user_data)
{
   S_STB_AV_VIDEO_INFO info;

   FUNCTION_START(STB_AVSetVideoCallback);

   av_paths_status[path].callback = callback;
   av_paths_status[path].user_data = user_data;

   if ((callback != NULL) && (display_info.screen_width != 0) && (display_info.screen_height != 0))
   {
      info.flags = VIDEO_INFO_SCREEN_RESOLUTION | VIDEO_INFO_DISPLAY_ASPECT_RATIO;
      info.screen_width = display_info.screen_width;
      info.screen_height = display_info.screen_height;
      info.display_aspect_ratio = display_info.screen_aspect_ratio;

      av_paths_status[path].callback(&info, av_paths_status[path].user_data);
   }

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

   if ((src != NULL) && (dest != NULL))
   {
      /*used as a Quad, not the literal meaning*/
      S_RECTANGLE crop = {src->top, src->left, src->top, src->left};

      VID_DBG("video: (%u, %u), (%u x %u) out: (%u, %u), (%u x %u)",
         src->left, src->top, src->width, src->height,
         dest->left, dest->top, dest->width, dest->height);

      STB_OSSendEvent(FALSE, HW_EV_CLASS_PRIVATE, HW_EV_TYPE_VIDEO_CROPPING_CHANGED, &crop, sizeof(S_RECTANGLE));
      STB_OSSendEvent(FALSE, HW_EV_CLASS_PRIVATE, HW_EV_TYPE_VIDEO_RECTANGLE_CHANGED, dest, sizeof(S_RECTANGLE));
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

   VID_DBG("blank=%u", blank);

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

   av_paths_status[path].volume=vol;

   AM_AOUT_SetVolume(AOUT_DEV, vol);

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
   FUNCTION_FINISH(STB_AVGetAudioVolume);

   return av_paths_status[path].volume;
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

   if (!STB_PVRIsPlayStarted(path, path))
   {
      AUD_DBG("path=%u", path);

      DMXGetDecodePIDs(av_paths_status[path].demux, &pcr_pid, &video_pid, &audio_pid, &ad_pid);
      audio_format = av_paths_status[path].audio_format;
      video_format = av_paths_status[path].video_format;

      if(video_pid == 0 )
      {
         video_pid = INVALID_PID;
         pcr_pid = INVALID_PID;
         video_format = -1;
         av_paths_status[path].av_decoder_state = DECODER_A_STOP_V_STOP;
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
            AM_AV_SwitchTSAudio(path,audio_pid,audio_format); //The audio pid and fmt have been set when video decoding
            av_paths_status[path].av_decoder_state = DECODER_A_START_V_START;
            STB_OSSendEvent(FALSE, HW_EV_CLASS_DECODE, HW_EV_TYPE_AUDIO_STARTED, &path, sizeof(U8BIT));
            break;

         case DECODER_A_STOP_V_STOP:
            AUD_DBG("video not started, audio PID=%u, PCR PID=%u", audio_pid, pcr_pid);
            av_paths_status[path].audio_pid = audio_pid;
            av_paths_status[path].video_pid = video_pid;
            av_paths_status[path].pcr_pid = pcr_pid;
            //AM_AV_DisableVideo(path);
            AM_AV_SetTSSource(path, av_paths_status[path].demux + AM_AV_TS_SRC_DMX0);
            AM_AV_StartTSWithPCR(path, video_pid, audio_pid, pcr_pid, video_format, audio_format);
            av_paths_status[path].av_decoder_state = DECODER_A_START_V_STOP;
            STB_OSSendEvent(FALSE, HW_EV_CLASS_DECODE, HW_EV_TYPE_AUDIO_STARTED, &path, sizeof(U8BIT));
            break;

         default:
            break;
         }
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

   if (!STB_PVRIsPlayStarted(path, path))
   {
      VID_DBG("path=%u", path);

      DMXGetDecodePIDs(av_paths_status[path].demux, &pcr_pid, &video_pid, &audio_pid, &ad_pid);

      audio_format = av_paths_status[path].audio_format;
      video_format = av_paths_status[path].video_format;

      if (audio_pid == 0)
      {
         audio_pid = INVALID_PID;
      }

      if (video_pid != 0)
      {
         //AM_AV_SetVideoAspectRatio(path, AM_AV_VIDEO_ASPECT_AUTO);
         //AM_AV_SetVideoAspectMatchMode(path,AM_AV_VIDEO_ASPECT_MATCH_IGNORE);

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
               AM_AV_SetTSSource(path, av_paths_status[path].demux + AM_AV_TS_SRC_DMX0);
               AM_AV_StartTSWithPCR(path, video_pid, audio_pid, pcr_pid, video_format, audio_format);
            }
            else
            {
               AM_AV_EnableVideo(path);
            }
            av_paths_status[path].av_decoder_state = DECODER_A_START_V_START;
            av_paths_status[path].video_pid = video_pid;
            av_paths_status[path].pcr_pid = pcr_pid;
            break;
         case DECODER_A_STOP_V_STOP:
            VID_DBG("audio not started, video PID=%u, PCR=%u", video_pid,pcr_pid);
            av_paths_status[path].audio_pid = INVALID_PID;
            av_paths_status[path].video_pid = video_pid;
            av_paths_status[path].pcr_pid = pcr_pid;

            /*There seems to be a problem with using invalid pids, so just start up the audio decoder early*/
            AM_AV_SetTSSource(path, av_paths_status[path].demux + AM_AV_TS_SRC_DMX0);
            AM_AV_StartTSWithPCR(path, video_pid, audio_pid, pcr_pid, video_format, audio_format);

            av_paths_status[path].av_decoder_state = DECODER_A_STOP_V_START;
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

   FUNCTION_FINISH(STB_AVPauseVideoDecoding);
}

/**
 * @brief   Resume video decoding
 * @param   path Required Decode Path Number.
 */
void  STB_AVResumeVideoDecoding(U8BIT path)
{
   FUNCTION_START(STB_AVResumeVideoDecoding);

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

   FUNCTION_FINISH(STB_AVResumeVideoDecoding);
}

/**
 * @brief   Stops the video decoder
 * @param   path the video decoder path to be stopped
 */
void STB_AVStopVideoDecoding(U8BIT path)
{
   S_STB_AV_VIDEO_INFO info;
   AM_ErrorCode_t error;

   FUNCTION_START(STB_AVStopVideoDecoding);

   VID_DBG("path=%u", path);

   info.flags = 0;
   if (av_paths_status[path].injecting)
   {
      AV_StopInjection(path);
   }
   else
   {
      switch (av_paths_status[path].av_decoder_state)
      {
         case DECODER_A_STOP_V_START:
         {
            VID_DBG("audio not running, stop decoding");
            error = AM_AV_StopTS(path);
            VID_DBG("Stopped decoding, result %d", error);

            info.flags = VIDEO_INFO_DECODER_STATUS;
            info.status = DECODER_STATUS_NONE;

            av_paths_status[path].audio_pid = INVALID_PID;
            av_paths_status[path].video_pid = INVALID_PID;
            av_paths_status[path].ad_pid = INVALID_PID;
            av_paths_status[path].pcr_pid = INVALID_PID;
            av_paths_status[path].av_decoder_state = DECODER_A_STOP_V_STOP;

            STB_OSSendEvent(FALSE, HW_EV_CLASS_DECODE, HW_EV_TYPE_VIDEO_STOPPED, &path, sizeof(U8BIT));
            break;
         }

         case DECODER_A_START_V_START:
         {
            VID_DBG("audio still running, just hide the video for now");

            info.flags = VIDEO_INFO_DECODER_STATUS;
            info.status = DECODER_STATUS_NONE;

            //AM_AV_DisableVideo(path);
            AM_AV_SetTSSource(path, av_paths_status[path].demux + AM_AV_TS_SRC_DMX0);
            AM_AV_StartTSWithPCR(path, INVALID_PID, av_paths_status[path].audio_pid, INVALID_PID, -1, av_paths_status[path].audio_format);
            av_paths_status[path].av_decoder_state = DECODER_A_START_V_STOP;
            av_paths_status[path].video_pid = INVALID_PID;
            av_paths_status[path].pcr_pid = INVALID_PID;
            STB_OSSendEvent(FALSE, HW_EV_CLASS_DECODE, HW_EV_TYPE_VIDEO_STOPPED, &path, sizeof(U8BIT));
            break;
         }
         case DECODER_A_STOP_V_STOP:
         case DECODER_A_START_V_STOP:
            VID_DBG("already stopped");
            break;
      }
   }
   if ((info.flags != 0) && (av_paths_status[path].callback != NULL))
   {
      av_paths_status[path].callback(&info, av_paths_status[path].user_data);
   }

   FUNCTION_FINISH(STB_AVStopVideoDecoding);
}

/**
 * @brief   Stops the audio decoder
 * @param   path the audio decoder path to be stopped
 */
void STB_AVStopAudioDecoding(U8BIT path)
{
   AM_ErrorCode_t error;

   FUNCTION_START(STB_AVStopAudioDecoding);
   AUD_DBG("path=%u", path);

   if (av_paths_status[path].injecting)
   {
      AV_StopInjection(path);
   }
   else
   {
      switch (av_paths_status[path].av_decoder_state)
      {
         case DECODER_A_STOP_V_START:
         case DECODER_A_STOP_V_STOP:
            AUD_DBG("already stopped");
            break;
         case DECODER_A_START_V_STOP:
            AUD_DBG("video not running, stop decoding");
            error = AM_AV_StopTS(path);
            AUD_DBG("Stopped decoding, result %d", error);
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

   FUNCTION_START(STB_AVGetSTC);

   memset(stc, 0, 5);

   if (path < num_paths)
   {
      if (AM_AV_GetVideoPts(path, &video_pts) == AM_SUCCESS)
      {
         stc[0] = (U8BIT)((video_pts >> 32) & 0xff);
         stc[1] = (U8BIT)((video_pts >> 24) & 0xff);
         stc[2] = (U8BIT)((video_pts >> 16) & 0xff);
         stc[3] = (U8BIT)((video_pts >> 8) & 0xff);
         stc[4] = (U8BIT)(video_pts & 0xff);
      }
   }

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

   if (path < num_paths)
   {
      if (source == AV_DEMUX)
      {
         av_paths_status[path].demux = param & 0xff;
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

   if (path < num_paths)
   {
      if (source == AV_DEMUX)
      {
         av_paths_status[path].demux = param & 0xff;
      }
   }

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
   BOOLEAN success;

   FUNCTION_START(STB_AVSetVideoCodec);

   success = TRUE;

   switch(codec)
   {
   case AV_VIDEO_CODEC_H264:
      av_paths_status[path].video_format = VFORMAT_H264;
      VID_DBG("H264");
      break;
   case AV_VIDEO_CODEC_H265:
      av_paths_status[path].video_format = VFORMAT_HEVC;
      VID_DBG("H265/HEVC");
      break;
   case AV_VIDEO_CODEC_MPEG1:
   case AV_VIDEO_CODEC_MPEG2:
      av_paths_status[path].video_format = VFORMAT_MPEG12;
      VID_DBG("MP2");
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

   success = TRUE;

   switch(codec)
   {
   case AV_AUDIO_CODEC_AC3:
      av_paths_status[path].audio_format = AFORMAT_AC3;
      AUD_DBG("AC3");
      break;
   case AV_AUDIO_CODEC_EAC3:
      av_paths_status[path].audio_format = AFORMAT_EAC3;
      AUD_DBG("E-AC3");
      break;
   case AV_AUDIO_CODEC_AAC:
   case AV_AUDIO_CODEC_HEAAC:
      av_paths_status[path].audio_format = AFORMAT_AAC;
      AUD_DBG("AAC");
      break;
      //case AV_AUDIO_CODEC_AUTO:
   case AV_AUDIO_CODEC_MP2:
   case AV_AUDIO_CODEC_MP3:
      av_paths_status[path].audio_format = AFORMAT_MPEG;
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

   if (av_paths_status[path].sample_data != NULL)
   {
      STB_MEMFreeSysRAM(av_paths_status[path].sample_data);
      av_paths_status[path].sample_data = NULL;
      av_paths_status[path].sample_data_size = 0;
   }

   av_paths_status[path].sample_data = STB_MEMGetSysRAM(size);
   if (av_paths_status[path].sample_data != NULL)
   {
      memcpy(av_paths_status[path].sample_data,data,size);
      av_paths_status[path].sample_data_size = size;
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
   AM_ErrorCode_t retval;
   E_HW_STATUS success = HW_GEN_ERROR;

   FUNCTION_START(STB_AVPlayAudioSample);
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

   AM_AV_StopAudioES(path);
   AM_AV_SetTSSource(path, AM_AV_TS_SRC_DMX0);
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

   switch(codec)
   {
      case AV_VIDEO_CODEC_MPEG1:
      case AV_VIDEO_CODEC_MPEG2:
      case AV_VIDEO_CODEC_H264:
         supported = TRUE;
         av_paths_status[path].iframe_codec = codec;
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

   if (av_paths_status[path].iframe_data != NULL)
   {
      STB_MEMFreeSysRAM(av_paths_status[path].iframe_data);
      av_paths_status[path].iframe_data = NULL;
   }

   if (size != 0)
   {
      av_paths_status[path].iframe_data = STB_MEMGetSysRAM(size);
      if (av_paths_status[path].iframe_data != NULL)
      {
         VID_DBG("buffering %lu byte iframe",size);
         av_paths_status[path].iframe_data_size = size;
         memcpy(av_paths_status[path].iframe_data,data,size);
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
   AM_ErrorCode_t result;
   AM_AV_VFormat_t codec;

   FUNCTION_START(STB_AVShowIFrame);

   if (av_paths_status[path].iframe_data != NULL)
   {
      switch(av_paths_status[path].iframe_codec)
      {
         case AV_VIDEO_CODEC_MPEG2:
         case AV_VIDEO_CODEC_MPEG1:
            codec = VFORMAT_MPEG12;
            break;
         case AV_VIDEO_CODEC_H264:
            codec = VFORMAT_H264;
            break;
         default:
            codec = VFORMAT_MAX;
            break;
      }

      VID_DBG("path=%u", path);

      if (codec < VFORMAT_MAX)
      {
         /* Subscribe to the event that indicates when the end of the data has been seen,
          * which should also be when the iframe has finished decoding and can be displayed */
         AM_EVT_Subscribe(path, AM_AV_EVT_VIDEO_ES_END, AVEventHandler, &av_paths_status[path]);

         result = AM_AV_StartVideoESData(path, codec, av_paths_status[path].iframe_data,
                                          av_paths_status[path].iframe_data_size);
         if(result == AM_SUCCESS)
         {
            VID_DBG("showing iframe");
         }
         else
         {
            VID_DBG(" iframe play failed, error %d", result-AM_AV_ERROR_BASE);
         }
      }
   }

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
void STB_AVStartADDecoding(U8BIT path)
{
   U16BIT video_pid, audio_pid, pcr_pid, ad_pid;

   FUNCTION_START(STB_AVStartADDecoding);

   DMXGetDecodePIDs(av_paths_status[path].demux, &pcr_pid, &video_pid, &audio_pid, &ad_pid);
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
   BOOLEAN success;

   FUNCTION_START(STB_AVSetADCodec);

   success = TRUE;

   switch(codec)
   {
   case AV_AUDIO_CODEC_AC3:
      av_paths_status[path].ad_format = AFORMAT_AC3;
      break;
   case AV_AUDIO_CODEC_EAC3:
      av_paths_status[path].ad_format = AFORMAT_EAC3;
      break;
   case AV_AUDIO_CODEC_AAC:
   case AV_AUDIO_CODEC_HEAAC:
   case AV_AUDIO_CODEC_HEAACV2:
      av_paths_status[path].ad_format = AFORMAT_AAC;
      break;
   case AV_AUDIO_CODEC_AUTO :
   case AV_AUDIO_CODEC_MP2 :
   case AV_AUDIO_CODEC_MP3 :
      av_paths_status[path].ad_format = AFORMAT_MPEG;
      break;
   default:
      av_paths_status[path].ad_format = AFORMAT_MPEG;
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
   FUNCTION_START(STB_AVSetADVolume);
   AM_AOUT_SetVolume(AOUT_DEV, vol);
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

   if (width != NULL)
   {
      *width = display_info.screen_width;
   }
   if (height != NULL)
   {
      *height = display_info.screen_height;
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

static void AVEventHandler(long dev_no, int event_type, void *param, void *data)
{
   AV_PATH_STATUS *status;
   S_STB_AV_VIDEO_INFO info;
   AM_AV_VideoStatus_t *video_status;
   AM_USERDATA_AFD_t *afd;

   status = (AV_PATH_STATUS *)data;

   info.flags = 0;

   switch(event_type)
   {
      case AM_AV_EVT_VIDEO_ASPECT_RATIO_CHANGED:
      {
         switch ((AM_AV_VideoAspectRatio_t)param)
         {
            case AM_AV_VIDEO_ASPECT_4_3:
               info.flags |= VIDEO_INFO_VIDEO_ASPECT_RATIO;
               info.video_aspect_ratio = ASPECT_RATIO_4_3;
               VID_DBG("Video aspect ratio 4:3");
               break;
            case AM_AV_VIDEO_ASPECT_16_9:
               info.flags |= VIDEO_INFO_VIDEO_ASPECT_RATIO;
               info.video_aspect_ratio = ASPECT_RATIO_16_9;
               VID_DBG("Video aspect ratio 16:9");
               break;
            default:
               VID_DBG("Unhandled video aspect ratio %d", (int)param);
               break;
         }
         break;
      }

      case AM_AV_EVT_VIDEO_RESOLUTION_CHANGED:
      {
         if (param != NULL)
         {
            video_status = (AM_AV_VideoStatus_t *)param;
            if ((video_status->src_w != 0) && (video_status->src_h != 0))
            {
               info.flags |= VIDEO_INFO_VIDEO_RESOLUTION;
               info.video_width = video_status->src_w;
               info.video_height = video_status->src_h;
               VID_DBG("Video res changed, %u x %u", info.video_width, info.video_height);
            }
         }
         break;
      }

      case AM_AV_EVT_VIDEO_AVAILABLE:
      {
         info.flags = VIDEO_INFO_DECODER_STATUS;
         info.status = DECODER_STATUS_VIDEO;
         VID_DBG("Video decoding started");
         STB_OSSendEvent(FALSE, HW_EV_CLASS_DECODE, HW_EV_TYPE_VIDEO_STARTED, &status->decoder, sizeof(U8BIT));
         break;
      }

      case AM_AV_EVT_VIDEO_WINDOW_CHANGED:
      {
#ifdef VIDEO_DEBUG
         if (param != NULL)
         {
            AM_AV_VideoWindow_t *window = (AM_AV_VideoWindow_t *)param;
            VID_DBG("Video window changed: (%u, %u), (%u x %u)", window->x, window->y, window->w, window->h);
         }
#endif
         break;
      }

      case AM_AV_EVT_VIDEO_ES_END:
      {
         VID_DBG("AM_AV_EVT_VIDEO_ES_END: iframe displayed");
         AM_AV_EnableVideo(status->decoder);

         status->iframe_shown = TRUE;

         info.flags = VIDEO_INFO_DECODER_STATUS;
         info.status = DECODER_STATUS_IFRAME;
         break;
      }

      case AM_AV_EVT_VIDEO_AFD_CHANGED:
      {
         afd = param;
         info.flags = VIDEO_INFO_AFD;
         info.afd = afd->af & 0x7;
         VID_DBG("[evt] video afd changed: flg[0x%x] fmt[0x%x]\n", afd->af_flag, afd->af);
         break;
      }

      default:
      {
         AV_DBG("Unhandled event type %d", event_type - AM_AV_EVT_BASE);
         break;
      }
   }

   if ((info.flags != 0) && (status->callback != NULL))
   {
      status->callback(&info, status->user_data);
   }
}

static AM_ErrorCode_t AVGetVOutDisplay(U32BIT *v_display)
{
   AM_ErrorCode_t ret;
   U8BIT buf[32] = {0};
   U8BIT display[32] = {0};

   *v_display = 1080;

   ret = AM_FileRead("/sys/class/display/mode", buf, sizeof(buf));
   if (!ret) {
      sscanf(buf, "%[^a-z]", display);
      *v_display = atoi(display);
      VID_DBG("GetVOutDisplay buf:%s display:%s %d", buf, display, *v_display);
      STB_SPDebugWrite("AVGetVOutDisplay buf:%s display:%s %d", buf, display, *v_display);
   }
   return ret;
}

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

void AV_InjectData(U8BIT path,U8BIT *data, U32BIT size)
{
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
}
