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
 * @brief   Set Top Box - Hardware Layer, PVR play and record functions
 * @file    stbpvrpr.c
 * @date    October 2018
 */

/*#define PLAY_DEBUG*/
/*#define RECORD_DEBUG*/

//---includes for this file----------------------------------------------------
// compiler library header files
#include <fcntl.h>
#include <errno.h>

// third party header files
#include "am_mw/am_rec.h"
#include "am_adp/am_tfile.h"
#include "am_adp/am_av.h"

// Ocean Blue header files
#include "techtype.h"
#include "dbgfuncs.h"

#include "stbhwdef.h"
#include "stbhwos.h"
#include "stbhwmem.h"
#include "stbhwdsk.h"
#include "stbpvrpr.h"

//---constant definitions for this file----------------------------------------
#define INVALID_RES_ID           255


#ifdef PLAY_DEBUG
   #define PLAY_DBG(x,...)       STB_SPDebugWrite("%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )
#else
   #define PLAY_DBG(x,...)
#endif

#ifdef RECORD_DEBUG
   #define REC_DBG(x,...)        STB_SPDebugWrite("%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )
#else
   #define REC_DBG(x,...)
#endif

//---local typedef structs for this file---------------------------------------
typedef enum
{
   PLAY_STOPPED,
   PLAY_STARTING,
   PLAY_STARTED
} E_PLAY_STATE;

typedef struct
{
   U8BIT rec_index;

   U8BIT tuner;
   U8BIT rec_demux;

   AM_REC_Handle_t rec_handle;
   AM_TFile_t tfile;
   AM_AV_TimeshiftMediaInfo_t media_info;

   BOOLEAN has_video;
   BOOLEAN has_audio;

   E_STB_PVR_START_MODE rec_mode;
   U32BIT timeshift_duration;

   U16BIT disk_id;
   U8BIT basename[16];

   U8BIT play_demux;

   E_STB_PVR_START_MODE play_mode;
   S16BIT play_speed;

   E_PLAY_STATE play_state;
} S_TIMESHIFT_STATUS;

/* The following enums are taken from vendor/amlogic/dvb/am_adp/am_av/aml/aml.c
 * As the status is provided to user code the enums should really be public :-(
 * The names have been changed in case AMLogic do provide them in a public header file
 * at some point in the future */
enum
{
   AV_TIMESHIFT_STATUS_STOP,
   AV_TIMESHIFT_STATUS_PLAY,
   AV_TIMESHIFT_STATUS_PAUSE,
   AV_TIMESHIFT_STATUS_FFFB,
   AV_TIMESHIFT_STATUS_EXIT,
   AV_TIMESHIFT_STATUS_INITOK,
   AV_TIMESHIFT_STATUS_SEARCHOK,
};

//---local (static) variable declarations for this file------------------------
//   (internal variables declared static to make them local)
static U8BIT num_recorders = 0;
static U8BIT num_players = 0;

static S_TIMESHIFT_STATUS s_timeshift_status;


//---local function prototypes for this file-----------------------------------
//   (internal functions declared static to make them local)
static void RecEventHandler(long dev_no, int event_type, void *param, void *data);
static void PlayEventHandler(long dev_no, int event_type, void *param, void *data);


//---global function definitions-----------------------------------------------

/**
 * @brief   Initialisation for playback
 * @param   num_audio_decoders number of audio decoders available
 * @param   num_video_decoders number of video decoders available
 * @return  Number of players, 0 if unsuccessful or unsupported
 */
U8BIT STB_PVRInitPlayback(U8BIT num_audio_decoders, U8BIT num_video_decoders)
{
   FUNCTION_START(STB_PVRInitPlayback);

   USE_UNWANTED_PARAM(num_audio_decoders);

   if (num_video_decoders != 0)
   {
      PLAY_DBG("");

      /* Only one player needed for timeshift */
      num_players = 1;

      s_timeshift_status.play_demux = INVALID_RES_ID;
      s_timeshift_status.play_mode = START_RUNNING;
      s_timeshift_status.play_speed = 100;
      s_timeshift_status.play_state = PLAY_STOPPED;
   }

   FUNCTION_FINISH(STB_PVRInitPlayback);

   return(num_players);
}

/**
 * @brief   Initialisation for recording
 * @param   num_tuners number of tuners available for recording
 * @return  Number of recorders, 0 if unsuccessful or unsupported
 */
U8BIT STB_PVRInitRecording(U8BIT num_tuners)
{
   FUNCTION_START(STB_PVRInitRecording);
   USE_UNWANTED_PARAM(num_tuners);

   if (NUM_RECORDERS != 0)
   {
      REC_DBG("");

      /* Only one recorder needed for timeshift */
      num_recorders = 1;

      s_timeshift_status.rec_index = 0;
      s_timeshift_status.tuner = INVALID_RES_ID;
      s_timeshift_status.rec_demux = INVALID_RES_ID;

      /* Default to starting paused because only timeshift is supported */
      s_timeshift_status.rec_mode = START_PAUSED;
   }

   FUNCTION_FINISH(STB_PVRInitRecording);

   return(num_recorders);
}

/**
 * @brief   Set startup mode for playback
 * @param   audio_decoder audio decoder being used for playback
 * @param   video_decoder video decoder being used for playback
 * @param   mode playback startup mode
 */
void STB_PVRSetPlayStartMode(U8BIT audio_decoder, U8BIT video_decoder, E_STB_PVR_START_MODE mode)
{
   FUNCTION_START(STB_PVRSetPlayStartMode);

   USE_UNWANTED_PARAM(audio_decoder);

   if (video_decoder < num_players)
   {
      s_timeshift_status.play_mode = mode;
   }

   FUNCTION_FINISH(STB_PVRSetPlayStartMode);
}

/**
 * @brief   Informs the platform whether there's video in the file to be played.
 *          Should be called before playback is started.
 * @param   audio_decoder audio decoder being used for playback
 * @param   video_decoder video decoder being used for playback
 * @param   has_video TRUE if the recording contains video, FALSE otherwise
 */
void STB_PVRPlayHasVideo(U8BIT audio_decoder, U8BIT video_decoder, BOOLEAN has_video)
{
   FUNCTION_START(STB_PVRPlayHasVideo);

   USE_UNWANTED_PARAM(audio_decoder);
   USE_UNWANTED_PARAM(video_decoder);
   USE_UNWANTED_PARAM(has_video);

   FUNCTION_FINISH(STB_PVRPlayHasVideo);
}

/**
 * @brief   Sets the time the next notification event should be sent during playback.
 *          This is required for CI+, but may also be used for other purposes.
 * @param   audio_decoder audio decoder being used for playback
 * @param   video_decoder video decoder being used for playback
 * @param   notify_time time in seconds the next notification event is to be sent
 */
void STB_PVRSetPlaybackNotifyTime(U8BIT audio_decoder, U8BIT video_decoder, U32BIT notify_time)
{
   FUNCTION_START(STB_PVRSetPlaybackNotifyTime);

   USE_UNWANTED_PARAM(audio_decoder);
   USE_UNWANTED_PARAM(video_decoder);
   USE_UNWANTED_PARAM(notify_time);

   FUNCTION_FINISH(STB_PVRSetPlaybackNotifyTime);
}

/**
 * @brief   Starts playback
 * @param   disk_id disk containing the recording to be played
 * @param   audio_decoder audio decoder to be used for playback
 * @param   video_decoder video decoder to be used for playback
 * @param   demux demux to be used for playback
 * @param   basename basename of the recording to be played
 * @return  TRUE if playback is started, FALSE otherwise
 */
BOOLEAN STB_PVRPlayStart(U16BIT disk_id, U8BIT audio_decoder, U8BIT video_decoder, U8BIT demux,
   U8BIT *basename)
{
   BOOLEAN play_started;
   AM_ErrorCode_t am_error;
   AM_AV_TimeshiftPara_t ts_params;

   FUNCTION_START(STB_PVRPlayStart);

   USE_UNWANTED_PARAM(audio_decoder);

   play_started = FALSE;

   if (video_decoder < num_players)
   {
      /* Only timeshift is supported, so check the recording being played is timeshift */
      if ((s_timeshift_status.rec_handle != NULL) &&
         (s_timeshift_status.disk_id == disk_id) &&
         (strcmp((char *)s_timeshift_status.basename, (char *)basename) == 0))
      {
         /* This is the timeshift recording */
         s_timeshift_status.play_demux = demux;

         memset(&ts_params, 0, sizeof(AM_AV_TimeshiftPara_t));

         ts_params.dmx_id = demux;
         ts_params.mode = AM_AV_TIMESHIFT_MODE_TIMESHIFTING;
         ts_params.tfile = s_timeshift_status.tfile;

         if (s_timeshift_status.play_mode == START_PAUSED)
         {
            ts_params.start_paused = AM_TRUE;
            s_timeshift_status.play_speed = 0;
         }
         else
         {
            ts_params.start_paused = AM_FALSE;
            s_timeshift_status.play_speed = 100;
         }

         ts_params.media_info.duration = s_timeshift_status.timeshift_duration;

         /* The media info to be played back is the same as is being recorded */
         memcpy(&ts_params.media_info, &s_timeshift_status.media_info,
            sizeof(AM_AV_TimeshiftMediaInfo_t));

         s_timeshift_status.play_state = PLAY_STARTING;

         am_error = AM_AV_StartTimeshift(video_decoder, &ts_params);
         if (am_error == AM_SUCCESS)
         {
            PLAY_DBG("Starting timeshift playback, speed=%u%%", s_timeshift_status.play_speed);

            AM_EVT_Subscribe(video_decoder, AM_AV_EVT_PLAYER_STATE_CHANGED, PlayEventHandler,
               &s_timeshift_status);
            AM_EVT_Subscribe(video_decoder, AM_AV_EVT_PLAYER_SPEED_CHANGED, PlayEventHandler,
               &s_timeshift_status);
            AM_EVT_Subscribe(video_decoder, AM_AV_EVT_PLAYER_TIME_CHANGED, PlayEventHandler,
               &s_timeshift_status);
            AM_EVT_Subscribe(video_decoder, AM_AV_EVT_PLAYER_UPDATE_INFO, PlayEventHandler,
               &s_timeshift_status);
#if 0
            am_error = AM_AV_PlayTimeshift(video_decoder);
            if ((am_error == AM_SUCCESS) && (s_timeshift_status.play_speed == 0))
            {
               am_error = AM_AV_PauseTimeshift(video_decoder);
            }

            if (am_error != AM_SUCCESS)
            {
               PLAY_DBG("Start pause/play failed, error %d", am_error);
            }
#endif
            play_started = TRUE;
         }
         else
         {
            PLAY_DBG("Failed to start timeshift, error %d", am_error);
         }
      }
      else
      {
         PLAY_DBG("Only timeshift playback is supported!");
      }
   }
   else
   {
      PLAY_DBG("Can't start playback with video %u (audio %u)", video_decoder, audio_decoder);
   }

   FUNCTION_FINISH(STB_PVRPlayStart);

   return(play_started);
}

/**
 * @brief   Returns status of playback with the given decoders
 * @param   audio_decoder audio decoder being used for playback
 * @param   video_decoder video decoder being used for playback
 * @return  TRUE if playback is in progress with the given decoders
 */
BOOLEAN STB_PVRIsPlayStarted(U8BIT audio_decoder, U8BIT video_decoder)
{
   BOOLEAN retval;

   FUNCTION_START(STB_PVRIsPlayStarted);
   USE_UNWANTED_PARAM(audio_decoder);

   retval = FALSE;

   if (video_decoder < num_players)
   {
      if (s_timeshift_status.play_state == PLAY_STARTED)
      {
         retval = TRUE;
      }
   }

   FUNCTION_FINISH(STB_PVRIsPlayStarted);

   return(retval);
}

/**
 * @brief   Sets the playback position after playback has started (i.e. jump to bookmark)
 * @param   audio_decoder audio decoder being used for playback
 * @param   video_decoder video decoder being used for playback
 * @param   position_in_seconds position to jump to in the recording in seconds from the beginning
 * @return  TRUE if position is set successfully, FALSE otherwise
 */
BOOLEAN STB_PVRPlaySetPosition(U8BIT audio_decoder, U8BIT video_decoder, U32BIT position_in_seconds)
{
   BOOLEAN retval;
   AM_ErrorCode_t am_error;

   FUNCTION_START(STB_PVRPlaySetPosition);
   USE_UNWANTED_PARAM(audio_decoder);

   retval = FALSE;

   if (video_decoder < num_players)
   {
      if (s_timeshift_status.play_state == PLAY_STARTED)
      {
         if (s_timeshift_status.play_speed == 0)
         {
            am_error = AM_AV_SeekTimeshift(video_decoder, position_in_seconds * 1000, AM_FALSE);
            if (am_error == AM_SUCCESS)
            {
               PLAY_DBG("%lu secs", position_in_seconds);
               retval = TRUE;
            }
            else
            {
               PLAY_DBG("Failed to set play position, error 0x%x", am_error);
            }
         }
         else
         {
            am_error = AM_AV_SeekTimeshift(video_decoder, position_in_seconds * 1000, AM_TRUE);
            if (am_error == AM_SUCCESS)
            {
               PLAY_DBG("%lu secs", position_in_seconds);
               retval = TRUE;
            }
            else
            {
               PLAY_DBG("Failed to set play position, error 0x%x", am_error);
            }
         }
      }
      else
      {
         PLAY_DBG("Timeshift playback isn't started");
      }
   }

   FUNCTION_FINISH(STB_PVRPlaySetPosition);

   return(retval);
}

/**
 * @brief   Stops playback
 * @param   audio_decoder audio decoder being used for playback
 * @param   video_decoder video decoder being used for playback
 */
void STB_PVRPlayStop(U8BIT audio_decoder, U8BIT video_decoder)
{
   AM_ErrorCode_t am_error;

   FUNCTION_START(STB_PVRPlayStop);
   USE_UNWANTED_PARAM(audio_decoder);

   if (video_decoder < num_players)
   {
      if (s_timeshift_status.play_state != PLAY_STOPPED)
      {
         am_error = AM_AV_StopTimeshift(video_decoder);
         if (am_error == AM_SUCCESS)
         {
            PLAY_DBG("Timeshift playback stopped");
         }
         else
         {
            PLAY_DBG("Failed to stop timeshift playback, error %d", am_error);
         }

         s_timeshift_status.play_state = PLAY_STOPPED;

         AM_EVT_Unsubscribe(video_decoder, AM_AV_EVT_PLAYER_STATE_CHANGED, PlayEventHandler,
            &s_timeshift_status);
         AM_EVT_Unsubscribe(video_decoder, AM_AV_EVT_PLAYER_SPEED_CHANGED, PlayEventHandler,
            &s_timeshift_status);
         AM_EVT_Unsubscribe(video_decoder, AM_AV_EVT_PLAYER_TIME_CHANGED, PlayEventHandler,
            &s_timeshift_status);
         AM_EVT_Unsubscribe(video_decoder, AM_AV_EVT_PLAYER_UPDATE_INFO, PlayEventHandler,
            &s_timeshift_status);

         STB_OSSendEvent(FALSE, HW_EV_CLASS_PVR, HW_EV_TYPE_PVR_PLAY_STOP, NULL, 0);
      }
      else
      {
         PLAY_DBG("Timeshift playback isn't started");
      }
   }

   FUNCTION_FINISH(STB_PVRPlayStop);
}

/**
 * @brief   Returns whether audio and video playback has been started
 * @param   audio_decoder audio decoder being used for playback
 * @param   video_decoder video decoder being used for playback
 * @param   video returned as TRUE if video is being decoded
 * @param   audio returned as TRUE if audio is being decoded
 */
void STB_PVRPlayEnabled(U8BIT audio_decoder, U8BIT video_decoder, BOOLEAN *video, BOOLEAN *audio)
{
   FUNCTION_START(STB_PVRPlayEnabled);
   USE_UNWANTED_PARAM(audio_decoder);

   if ((video_decoder < num_players) && (s_timeshift_status.play_state != PLAY_STOPPED))
   {
      *video = s_timeshift_status.has_video;
      *audio = s_timeshift_status.has_audio;
   }
   else
   {
      *video = FALSE;
      *audio = FALSE;
   }

   FUNCTION_FINISH(STB_PVRPlayEnabled);
}

/**
 * @brief   Acquires an index to be used to reference a recording
 * @param   tuner tuner to be used for the recording
 * @param   demux demux to be used for the recording
 * @return  recording index, 255 if none available
 */
U8BIT STB_PVRAcquireRecorderIndex(U8BIT tuner, U8BIT demux)
{
   U8BIT rec_index;

   FUNCTION_START(STB_PVRAcquireRecorderIndex);

   /* Check whether the timeshift recorder is available */
   if (s_timeshift_status.tuner == INVALID_RES_ID)
   {
      s_timeshift_status.tuner = tuner;
      s_timeshift_status.rec_demux = demux;
      rec_index = s_timeshift_status.rec_index;
   }
   else
   {
      rec_index = INVALID_RES_ID;
   }

   REC_DBG("Acquired recorder %u", rec_index);

   FUNCTION_FINISH(STB_PVRAcquireRecorderIndex);

   return(rec_index);
}

/**
 * @brief   Releases a recording index when no longer needed
 * @param   rec_index recoding index
 */
void STB_PVRReleaseRecorderIndex(U8BIT rec_index)
{
   FUNCTION_START(STB_PVRReleaseRecorderIndex);

   REC_DBG("Releasing recorder %u", rec_index);

   if (rec_index < num_recorders)
   {
      s_timeshift_status.tuner = INVALID_RES_ID;
      s_timeshift_status.rec_demux = INVALID_RES_ID;
   }

   FUNCTION_FINISH(STB_PVRReleaseRecorderIndex);
}

/**
 * @brief   Called to apply the given descrambler key to the PID data being recorded.
 *          This function may be called before the recording has actually started.
 * @param   rec_index recording index
 * @param   desc_type descrambler type
 * @param   parity key parity
 * @param   key key data
 * @param   iv provides an initialisation vector data, if required for the descrambler type
 * @return  TRUE if successful, FALSE otherwise
 */
BOOLEAN STB_PVRApplyDescramblerKey(U8BIT rec_index, E_STB_DMX_DESC_TYPE desc_type,
   E_STB_DMX_DESC_KEY_PARITY parity, U8BIT *key, U8BIT *iv)
{
   FUNCTION_START(STB_PVRApplyDescramblerKey);
   USE_UNWANTED_PARAM(rec_index);
   USE_UNWANTED_PARAM(desc_type);
   USE_UNWANTED_PARAM(parity);
   USE_UNWANTED_PARAM(key);
   USE_UNWANTED_PARAM(iv);
   FUNCTION_FINISH(STB_PVRApplyDescramblerKey);

   return(FALSE);
}

/**
 * @brief   Starts recording
 * @param   disk_id disk on which the recording is to be saved
 * @param   rec_index recording index to be used for the recording
 * @param   basename base filename to be used for the recording
 * @param   num_pids number of PIDs to be recorded
 * @param   pid_array PIDs to be recorded
 * @return  TRUE if recording is started, FALSE otherwise
 */
BOOLEAN STB_PVRRecordStart(U16BIT disk_id, U8BIT rec_index, U8BIT *basename,
   U16BIT num_pids, S_PVR_PID_INFO *pid_array)
{
   BOOLEAN retval;
   U16BIT i;
   AM_ErrorCode_t am_error;
   AM_REC_CreatePara_t create_params;
   AM_REC_RecPara_t rec_params;
   int tfile_flags;

   FUNCTION_START(STB_PVRRecordStart);

   retval = FALSE;

   if (rec_index < num_recorders)
   {
      if (s_timeshift_status.rec_mode == START_PAUSED)
      {
         memset(&create_params, 0, sizeof(AM_REC_CreatePara_t));

         create_params.fend_dev = s_timeshift_status.tuner;
         create_params.dvr_dev = s_timeshift_status.rec_demux;
         create_params.async_fifo_id = 0;

         STB_DSKFullPathname(disk_id, NULL, (U8BIT *)create_params.store_dir,
            sizeof(create_params.store_dir));

         REC_DBG("Starting recording in directory \"%s\"", create_params.store_dir);

         am_error = AM_REC_Create(&create_params, &s_timeshift_status.rec_handle);
         if (am_error == AM_SUCCESS)
         {
            s_timeshift_status.disk_id = disk_id;
            strncpy((char *)s_timeshift_status.basename, (char *)basename, sizeof(s_timeshift_status.basename));

            AM_REC_SetTFile(s_timeshift_status.rec_handle, NULL, REC_TFILE_FLAG_AUTO_CREATE);

            AM_EVT_Subscribe((long)s_timeshift_status.rec_handle, AM_REC_EVT_RECORD_START,
               RecEventHandler, &s_timeshift_status);
            AM_EVT_Subscribe((long)s_timeshift_status.rec_handle, AM_REC_EVT_RECORD_END,
               RecEventHandler, &s_timeshift_status);

            s_timeshift_status.has_video = FALSE;
            s_timeshift_status.has_audio = FALSE;

            memset(&rec_params, 0, sizeof(AM_REC_RecPara_t));
            memset(&s_timeshift_status.media_info, 0, sizeof(AM_AV_TimeshiftMediaInfo_t));

            /* Setup the initial set of PIDs that are to be recorded */
            REC_DBG("Recording PIDs:");
            for (i = 0; i < num_pids; i++)
            {
               if (pid_array[i].type == PVR_PID_TYPE_VIDEO)
               {
                  s_timeshift_status.has_video = TRUE;
                  s_timeshift_status.media_info.vid_pid = pid_array[i].pid;

                  switch (pid_array[i].u.video_codec)
                  {
                     case AV_VIDEO_CODEC_MPEG1:
                     case AV_VIDEO_CODEC_MPEG2:
                        s_timeshift_status.media_info.vid_fmt = VFORMAT_MPEG12;
                        break;
                     case AV_VIDEO_CODEC_H264:
                        s_timeshift_status.media_info.vid_fmt = VFORMAT_H264;
                        break;
                     default:
                        break;
                  }
                  REC_DBG("  VIDEO %u", pid_array[i].pid);
               }
               else if (pid_array[i].type == PVR_PID_TYPE_AUDIO)
               {
                  s_timeshift_status.has_audio = TRUE;
                  s_timeshift_status.media_info.audios[s_timeshift_status.media_info.aud_cnt].pid = pid_array[i].pid;

                  switch(pid_array[i].u.audio_codec)
                  {
                     case AV_AUDIO_CODEC_AC3:
                        s_timeshift_status.media_info.audios[s_timeshift_status.media_info.aud_cnt].fmt = AFORMAT_AC3;
                        break;
                     case AV_AUDIO_CODEC_EAC3:
                        s_timeshift_status.media_info.audios[s_timeshift_status.media_info.aud_cnt].fmt = AFORMAT_EAC3;
                        break;
                     case AV_AUDIO_CODEC_AAC:
                     case AV_AUDIO_CODEC_HEAAC:
                        s_timeshift_status.media_info.audios[s_timeshift_status.media_info.aud_cnt].fmt = AFORMAT_AAC;
                        break;
                     case AV_AUDIO_CODEC_MP2:
                     case AV_AUDIO_CODEC_MP3:
                        s_timeshift_status.media_info.audios[s_timeshift_status.media_info.aud_cnt].fmt = AFORMAT_MPEG;
                        break;
                     default:
                        break;
                  }

                  s_timeshift_status.media_info.aud_cnt++;
                  REC_DBG("  AUDIO %u", pid_array[i].pid);
               }
               else if (pid_array[i].type == PVR_PID_TYPE_SUBTITLES)
               {
                  s_timeshift_status.media_info.subtitles[s_timeshift_status.media_info.sub_cnt].pid = pid_array[i].pid;
                  s_timeshift_status.media_info.sub_cnt++;
                  REC_DBG("  SUBTITLES %u", pid_array[i].pid);
               }
               else if (pid_array[i].type == PVR_PID_TYPE_TELETEXT)
               {
                  s_timeshift_status.media_info.teletexts[s_timeshift_status.media_info.ttx_cnt].pid = pid_array[i].pid;
                  s_timeshift_status.media_info.ttx_cnt++;
                  REC_DBG("  TELETEXT %u", pid_array[i].pid);
               }
               else
               {
                  REC_DBG("  Not recording %u, type %u", pid_array[i].pid, pid_array[i].type);
               }
            }

            memcpy(&rec_params.media_info, &s_timeshift_status.media_info,
               sizeof(AM_AV_TimeshiftMediaInfo_t));

            strncpy(rec_params.prefix_name, "TimeShifting", AM_REC_NAME_MAX);
            strncpy(rec_params.suffix_name, "ts", AM_REC_SUFFIX_MAX);
            rec_params.is_timeshift = true;
            rec_params.total_time = s_timeshift_status.timeshift_duration;

            REC_DBG("Starting timeshift recording %p for %lu secs", s_timeshift_status.rec_handle,
               rec_params.total_time);

            am_error = AM_REC_StartRecord(s_timeshift_status.rec_handle, &rec_params);
            if (am_error == AM_SUCCESS)
            {
               am_error = AM_REC_GetTFile(s_timeshift_status.rec_handle,
                  &s_timeshift_status.tfile, &tfile_flags);
               if (am_error == AM_SUCCESS)
               {
                  AM_EVT_Subscribe((long)s_timeshift_status.tfile, AM_TFILE_EVT_START_TIME_CHANGED,
                     RecEventHandler, &s_timeshift_status);
                  AM_EVT_Subscribe((long)s_timeshift_status.tfile, AM_TFILE_EVT_END_TIME_CHANGED,
                     RecEventHandler, &s_timeshift_status);

                  am_error = AM_TFile_TimeStart(s_timeshift_status.tfile);
                  if (am_error != AM_SUCCESS)
                  {
                     REC_DBG("AM_TFile_TimeStart failed, error %d", am_error);
                  }
               }
               else
               {
                  REC_DBG("Failed to get recording tfile, error %d", am_error);
               }

               retval = TRUE;
            }
            else
            {
               REC_DBG("Failed to start recording, error %d", am_error);

               AM_EVT_Unsubscribe((long)s_timeshift_status.rec_handle, AM_REC_EVT_RECORD_START,
                  RecEventHandler, &s_timeshift_status);
               AM_EVT_Unsubscribe((long)s_timeshift_status.rec_handle, AM_REC_EVT_RECORD_END,
                  RecEventHandler, &s_timeshift_status);

               AM_REC_Destroy(s_timeshift_status.rec_handle);
               s_timeshift_status.rec_handle = NULL;
            }
         }
         else
         {
            REC_DBG("Failed to create recording, error %d", am_error);
         }
      }
      else
      {
         REC_DBG("Only recording for timeshift is supported!");
      }
   }
   else
   {
      REC_DBG("Invalid recorder %u", rec_index);
   }

   FUNCTION_FINISH(STB_PVRRecordStart);

   return(retval);
}

/**
 * @brief   Pauses a recording currently taking place
 * @param   rec_index recording index
 * @return  TRUE if the recording is successfully paused, FALSE otherwise
 */
BOOLEAN STB_PVRRecordPause(U8BIT rec_index)
{
   FUNCTION_START(STB_PVRRecordPause);
   USE_UNWANTED_PARAM(rec_index);
   FUNCTION_FINISH(STB_PVRRecordPause);

   return(TRUE);
}

/**
 * @brief   Resumes a paused recording
 * @param   rec_index recording index
 * @return  TRUE if the recording is successfully resumed, FALSE otherwise
 */
BOOLEAN STB_PVRRecordResume(U8BIT rec_index)
{
   FUNCTION_START(STB_PVRRecordResume);
   USE_UNWANTED_PARAM(rec_index);
   FUNCTION_FINISH(STB_PVRRecordResume);

   return(TRUE);
}

/**
 * @brief   Stops a recording
 * @param   rec_index recording index
 */
void STB_PVRRecordStop(U8BIT rec_index)
{
   AM_ErrorCode_t am_error;

   FUNCTION_START(STB_PVRRecordStop);

   if (rec_index < num_recorders)
   {
      REC_DBG("Stopping recording %u, handle %p", rec_index, s_timeshift_status.rec_handle);

      if (s_timeshift_status.rec_handle != NULL)
      {
         am_error = AM_REC_StopRecord(s_timeshift_status.rec_handle);
         if (am_error != AM_SUCCESS)
         {
            REC_DBG("Failed to stop recording %u, error %d", s_timeshift_status.rec_handle, am_error);
         }

         AM_EVT_Unsubscribe((long)s_timeshift_status.rec_handle, AM_REC_EVT_RECORD_START,
            RecEventHandler, &s_timeshift_status);
         AM_EVT_Unsubscribe((long)s_timeshift_status.rec_handle, AM_REC_EVT_RECORD_END,
            RecEventHandler, &s_timeshift_status);
         AM_EVT_Unsubscribe((long)s_timeshift_status.tfile, AM_TFILE_EVT_START_TIME_CHANGED,
            RecEventHandler, &s_timeshift_status);
         AM_EVT_Unsubscribe((long)s_timeshift_status.tfile, AM_TFILE_EVT_END_TIME_CHANGED,
            RecEventHandler, &s_timeshift_status);

         AM_REC_Destroy(s_timeshift_status.rec_handle);
         s_timeshift_status.rec_handle = NULL;
         s_timeshift_status.tfile = NULL;
      }
   }

   FUNCTION_FINISH(STB_PVRRecordStop);
}

/**
 * @brief   Changes the PIDs while recording
 * @param   rec_index current recording index  to be updated
 * @param   num_pids number of PIDs in PID array
 * @param   pid_array new PID list to be recorded
 * @return  TRUE if the PIDs have been successfully changed, FALSE otherwise
 */
BOOLEAN STB_PVRRecordChangePids(U8BIT rec_index, U16BIT num_pids, S_PVR_PID_INFO *pids_array)
{
   FUNCTION_START(STB_PVRRecordChangePids);

   REC_DBG("Recording %u", rec_index);

   USE_UNWANTED_PARAM(rec_index);
   USE_UNWANTED_PARAM(num_pids);
   USE_UNWANTED_PARAM(pids_array);
   FUNCTION_FINISH(STB_PVRRecordChangePids);

   return(FALSE);
}

/**
 * @brief   Sets the startup mode for a recording. This function should be called
 *          before the recording is started and is used to when pausing live TV
 *          in which case the additional param defines the length of the pause
 *          buffer to be used, in seconds.
 * @param   rec_index recording index to be used for the recording
 * @param   mode startup mode
 * @param   param additional parameter linked to the mode. When pausing live TV,
                    this is the length of the pause buffer, in seconds.
 */
void STB_PVRSetRecordStartMode(U8BIT rec_index, E_STB_PVR_START_MODE mode, U32BIT param)
{
   FUNCTION_START(STB_PVRSetRecordStartMode);

   REC_DBG("index %u, mode %u, param %lu", rec_index, mode, param);

   if (rec_index < num_recorders)
   {
      s_timeshift_status.rec_mode = mode;
      s_timeshift_status.timeshift_duration = param;
   }

   FUNCTION_FINISH(STB_PVRSetRecordStartMode);
}

/**
 * @brief   Returns whether recording has been started
 * @param   rec_index recording index being queried
 * @return  TRUE if recording has been started
 */
BOOLEAN STB_PVRIsRecordStarted(U8BIT rec_index)
{
   BOOLEAN retval;

   FUNCTION_START(STB_PVRIsRecordStarted);

   retval = FALSE;

   if (rec_index < num_recorders)
   {
      if (s_timeshift_status.rec_handle != NULL)
      {
         retval = TRUE;
      }

      REC_DBG("%s", (retval ? "yes" : "no"));
   }

   FUNCTION_FINISH(STB_PVRIsRecordStarted);

   return(retval);
}

/**
 * @brief   Returns status of audio/video recording
 * @param   rec_index recording index being used for recording
 * @param   video returned as TRUE if video data is being recorded
 * @param   audio returned as TRUE if audio data is being recorded
 */
void STB_PVRRecordEnabled(U8BIT rec_index, BOOLEAN *video, BOOLEAN *audio)
{
   FUNCTION_START(STB_PVRRecordEnabled);

   if ((rec_index < num_recorders) && (s_timeshift_status.rec_handle != NULL))
   {
      *video = s_timeshift_status.has_video;
      *audio = s_timeshift_status.has_audio;
   }
   else
   {
      *video = FALSE;
      *audio = FALSE;
   }

   FUNCTION_FINISH(STB_PVRRecordEnabled);
}

/**
 * @brief   Sets trick mode during playback
 * @param   audio_decoder audio decoder being used for playback
 * @param   video_decoder video decoder being used for playback
 * @param   mode trick mode to be used
 * @param   speed playback speed to be used as a percentage (100% = normal playback)
 */
void STB_PVRPlayTrickMode(U8BIT audio_decoder, U8BIT video_decoder, E_STB_PVR_PLAY_MODE mode, S16BIT speed)
{
   FUNCTION_START(STB_PVRPlayTrickMode);
   USE_UNWANTED_PARAM(audio_decoder);
   USE_UNWANTED_PARAM(video_decoder);
   USE_UNWANTED_PARAM(mode);
   USE_UNWANTED_PARAM(speed);
   FUNCTION_FINISH(STB_PVRPlayTrickMode);
}

/**
 * @brief   Set the play speed for the specified decoder
 * @param   audio_decoder audio decoder being used for playback
 * @param   video_decoder video decoder being used for playback
 * @param   speed Play speed as a percentage (i.e 100% = normal playback)
 * @return  TRUE if successful
 */
BOOLEAN STB_PVRSetPlaySpeed(U8BIT audio_decoder, U8BIT video_decoder, S16BIT speed)
{
   BOOLEAN retval;
   AM_ErrorCode_t am_error;
   int am_speed;

   FUNCTION_START(STB_PVRSetPlaySpeed);
   USE_UNWANTED_PARAM(audio_decoder);

   retval = FALSE;

   if ((video_decoder < num_players) && (s_timeshift_status.play_state == PLAY_STARTED))
   {
      if (speed != s_timeshift_status.play_speed)
      {
         if (speed == 0)
         {
            am_error = AM_AV_PauseTimeshift(video_decoder);
         }
         else if (speed == 100)
         {
            am_error = AM_AV_ResumeTimeshift(video_decoder);
         }
         else if (speed > 100)
         {
            am_speed = speed / 100;

            if (am_speed != 0)
            {
               am_error = AM_AV_FastForwardTimeshift(video_decoder, am_speed);
            }
            else
            {
               PLAY_DBG("Unsupported play speed %d", speed);
               am_error = AM_AV_ERR_NOT_SUPPORTED;
            }
         }
         else if (speed <= -100)
         {
            am_speed = speed / -100;

            if (am_speed != 0)
            {
               am_error = AM_AV_FastBackwardTimeshift(video_decoder, am_speed);
            }
            else
            {
               PLAY_DBG("Unsupported play speed %d", speed);
               am_error = AM_AV_ERR_NOT_SUPPORTED;
            }
         }
         else
         {
            PLAY_DBG("Unsupported play speed %d", speed);
            am_error = AM_AV_ERR_NOT_SUPPORTED;
         }

         if (am_error == AM_SUCCESS)
         {
            PLAY_DBG("Set play speed to %d%%", speed);
            s_timeshift_status.play_speed = speed;
            retval = TRUE;
         }
         else
         {
            PLAY_DBG("Failed to set play speed to %d (%d), error 0x%x", speed, am_speed, am_error);
         }
      }
   }

   FUNCTION_FINISH(STB_PVRSetPlaySpeed);

   return(retval);
}

/**
 * @brief   Returns the current playback speed
 * @param   audio_decoder audio decoder being used for playback
 * @param   video_decoder video decoder being used for playback
 * @return  current playback speed as a percentage
 */
S16BIT STB_PVRGetPlaySpeed(U8BIT audio_decoder, U8BIT video_decoder)
{
   S16BIT speed;

   FUNCTION_START(STB_PVRGetPlaySpeed);
   USE_UNWANTED_PARAM(audio_decoder);

   if (video_decoder < num_players)
   {
      speed = s_timeshift_status.play_speed;
   }
   else
   {
      speed = 0;
   }

   FUNCTION_FINISH(STB_PVRGetPlaySpeed);

   return(speed);
}

/**
 * @brief   Unused function
 * @param   audio_decoder audio decoder being used for playback
 * @param   video_decoder video decoder being used for playback
 */
void STB_PVRSaveFrame(U8BIT audio_decoder, U8BIT video_decoder)
{
   FUNCTION_START(STB_PVRSaveFrame);
   USE_UNWANTED_PARAM(audio_decoder);
   USE_UNWANTED_PARAM(video_decoder);
   FUNCTION_FINISH(STB_PVRSaveFrame);
}

/**
 * @brief   Checks whether any of the files already exist that would be created
 *          by a recording with the given base filename.
 * @param   disk_id disk to be checked
 * @param   basename base filename to be used for a recording
 * @return  TRUE if none of the files exist, FALSE otherwise
 */
BOOLEAN STB_PVRIsValidRecording(U16BIT disk_id, U8BIT *basename)
{
   FUNCTION_START(STB_PVRIsValidRecording);
   USE_UNWANTED_PARAM(disk_id);
   USE_UNWANTED_PARAM(basename);

   REC_DBG("disk 0x%04x, name %s", disk_id, basename);

   FUNCTION_FINISH(STB_PVRIsValidRecording);

   return(FALSE);
}

/**
 * @brief   Checks whether any of the files already exist that would be created
 *          by a recording with the given base filename.
 * @param   disk_id disk to be checked
 * @param   basename base filename to be used for a recording
 * @return  TRUE if none of the files exist, FALSE otherwise
 */
BOOLEAN STB_PVRCanBeUsedForRecording(U16BIT disk_id, U8BIT *basename)
{
   FUNCTION_START(STB_PVRCanBeUsedForRecording);
   USE_UNWANTED_PARAM(disk_id);
   USE_UNWANTED_PARAM(basename);
   REC_DBG("disk 0x%04x, name %s", disk_id, basename);
   FUNCTION_FINISH(STB_PVRCanBeUsedForRecording);

   return(TRUE);
}

/**
 * @brief   Deletes any files associated with the given base filename that were created
 *          as a result of the recording being performed.
 * @param   disk_id disk containing the recording to be deleted
 * @param   basename base filename used for the recording
 * @return  TRUE if successful, FALSE otherwise
 */
BOOLEAN STB_PVRDeleteRecording(U16BIT disk_id, U8BIT *basename)
{
   FUNCTION_START(STB_PVRDeleteRecording);
   USE_UNWANTED_PARAM(disk_id);
   USE_UNWANTED_PARAM(basename);
   FUNCTION_FINISH(STB_PVRDeleteRecording);

   return(FALSE);
}

/**
 * @brief   Returns the size in kilobytes of the recording defined by the given base filename.
 * @param   disk_id disk containing the recording to be queried
 * @param   basename base filename of recording to get info about
 * @param   rec_size_kb returned size of recording in kilobytes
 * @return  TRUE if the information is successfully gathered
 */
BOOLEAN STB_PVRGetRecordingSize(U16BIT disk_id, U8BIT *basename, U32BIT *rec_size_kb)
{
   BOOLEAN retval;
   AM_ErrorCode_t am_error;
   AM_REC_RecInfo_t rec_info;

   FUNCTION_START(STB_PVRGetRecordingInfo);

   retval = FALSE;

   if ((s_timeshift_status.rec_handle != NULL) && (s_timeshift_status.disk_id == disk_id) &&
      (strcmp((char *)basename, (char *)s_timeshift_status.basename) == 0))
   {
      am_error = AM_REC_GetRecordInfo(s_timeshift_status.rec_handle, &rec_info);
      if (am_error == AM_SUCCESS)
      {
         *rec_size_kb = rec_info.file_size / 1024;
         retval = TRUE;
      }
      else
      {
         REC_DBG("Failed to get info on recording %p, error %d", s_timeshift_status.rec_handle,
            am_error);
      }
   }
   else
   {
      *rec_size_kb = 0;
   }

   FUNCTION_FINISH(STB_PVRGetRecordingInfo);

   return(retval);
}

/**
 * @brief   Returns the elapsed playback time in hours, mins & secs
 * @param   audio_decoder audio decoder being used for playback
 * @param   video_decoder video decoder being used for playback
 * @param   elapsed_hours current number of hours into the playback
 * @param   elapsed_mins current number of minutes into the playback
 * @param   elapsed_secs current number of seconds into the playback
 * @return  TRUE if the info has been successfully gathered
 */
BOOLEAN STB_PVRGetElapsedTime(U8BIT audio_decoder, U8BIT video_decoder, U8BIT *elapsed_hours,
   U8BIT *elapsed_mins, U8BIT *elapsed_secs)
{
   BOOLEAN retval;
   AM_ErrorCode_t am_error;
   AM_AV_TimeshiftInfo_t info;
   U32BIT seconds;

   FUNCTION_START(STB_PVRGetElapsedTime);

   USE_UNWANTED_PARAM(audio_decoder);

   retval = FALSE;

   if (video_decoder < num_players)
   {
      am_error = AM_AV_GetTimeshiftInfo(video_decoder, &info);
      if (am_error == AM_SUCCESS)
      {
         seconds = info.current_time / 1000;

         *elapsed_hours = seconds / 3600;
         *elapsed_mins = seconds / 60 - (*elapsed_hours * 60);
         *elapsed_secs = seconds - (*elapsed_hours * 3600) - (*elapsed_mins * 60);

         PLAY_DBG("%02u:%02u:%02u", *elapsed_hours, *elapsed_mins,
            *elapsed_secs);

         retval = TRUE;
      }
      else
      {
         PLAY_DBG("Failed to get timeshift playback info, error 0x%x", am_error);
      }
   }

   FUNCTION_FINISH(STB_PVRGetElapsedTime);

   return(retval);
}

/**
 * @brief   Enables or disables encryption and sets the encryption key to be used
 * @param   rec_index recording index to be set
 * @param   state whether encryption is enabled of disabled
 * @param   key encryption key, ignored if state is FALSE
 * @param   iv initialisation vector, ignored if state is FALSE
 * @param   key_len length of encryption key, ignored if state is FALSE
 */
void STB_PVRSetRecordEncryptionKey(U8BIT rec_index, BOOLEAN state, U8BIT *key, U8BIT *iv, U32BIT key_len)
{
   FUNCTION_START(STB_PVRSetRecordEncryptionKey);

   USE_UNWANTED_PARAM(rec_index);
   USE_UNWANTED_PARAM(state);
   USE_UNWANTED_PARAM(key);
   USE_UNWANTED_PARAM(iv);
   USE_UNWANTED_PARAM(key_len);

   FUNCTION_FINISH(STB_PVRSetRecordEncryptionKey);
}

/**
 * @brief   Enables and sets the key that will be used to decrypt an encrypted
 *          recording during playback
 * @param   audio_decoder audio decoder used for playback
 * @param   video_decoder video decoder used for playback
 * @param   state whether decryption is enabled of disabled
 * @param   key decryption key, ignored if state is FALSE
 * @param   iv initialisation vector, ignored if state is FALSE
 * @param   key_len length of decryption key, ignored if state is FALSE
 */
void STB_PVRSetPlaybackDecryptionKey(U8BIT audio_decoder, U8BIT video_decoder, BOOLEAN state,
   U8BIT *key, U8BIT *iv, U32BIT key_len)
{
   FUNCTION_START(STB_PVRSetPlaybackDecryptionKey);

   USE_UNWANTED_PARAM(audio_decoder);
   USE_UNWANTED_PARAM(video_decoder);
   USE_UNWANTED_PARAM(state);
   USE_UNWANTED_PARAM(iv);
   USE_UNWANTED_PARAM(key);
   USE_UNWANTED_PARAM(key_len);

   FUNCTION_FINISH(STB_PVRSetPlaybackDecryptionKey);
}

/**
 * @brief   Changes the main audio PID being decoded during playback. This can
 *          be used to switch between main audio and broadcaster mix AD.
 * @param   audio_decoder - audio decoder for playback
 * @param   video_decoder - video decoder for playback
 * @param   pid - new audio PID to decode
 * @param   codec - new audio codec
 * @return  TRUE if the PID is changed successfully, FALSE otherwise
 */
BOOLEAN STB_PVRPlayChangeAudio(U8BIT audio_decoder, U8BIT video_decoder, U16BIT pid, U8BIT codec)
{
   FUNCTION_START(STB_PVRPlayChangeAudio);

   USE_UNWANTED_PARAM(audio_decoder);
   USE_UNWANTED_PARAM(video_decoder);
   USE_UNWANTED_PARAM(pid);
   USE_UNWANTED_PARAM(codec);

   FUNCTION_FINISH(STB_PVRPlayChangeAudio);

   return(FALSE);
}

/**
 * @brief   Set the retention limit for the playback. This function is used for CI+
 * @param   audio_decoder audio decoder being used for playback
 * @param   video_decoder video decoder being used for playback
 * @param   retention_limit Retention limit in minutes
 * @param   rec_data data when the recording was taken
 * @param   rec_hour hour when the recording was taken
 * @param   rec_min minute when the recording was taken
 */
void STB_PVRPlaySetRetentionLimit(U8BIT audio_decoder, U8BIT video_decoder, U32BIT retention_limit,
   U16BIT rec_date, U8BIT rec_hour, U8BIT rec_min)
{
   FUNCTION_START(STB_PVRPlaySetRetentionLimit);

   USE_UNWANTED_PARAM(audio_decoder);
   USE_UNWANTED_PARAM(video_decoder);
   USE_UNWANTED_PARAM(retention_limit);
   USE_UNWANTED_PARAM(rec_date);
   USE_UNWANTED_PARAM(rec_hour);
   USE_UNWANTED_PARAM(rec_min);

   FUNCTION_FINISH(STB_PVRPlaySetRetentionLimit);
}

//---local function definitions------------------------------------------------

static void RecEventHandler(long dev_no, int event_type, void *param, void *data)
{
   S_TIMESHIFT_STATUS *ts_status;

   if (data != NULL)
   {
      ts_status = (S_TIMESHIFT_STATUS *)data;

      switch (event_type)
      {
         case AM_REC_EVT_RECORD_START:
         {
            REC_DBG("Timeshift recording started, handle %p", ts_status->rec_handle);
            STB_OSSendEvent(FALSE, HW_EV_CLASS_PVR, HW_EV_TYPE_PVR_REC_START,
               &ts_status->rec_index, sizeof(U8BIT));
            break;
         }

         case AM_REC_EVT_RECORD_END:
         {
            REC_DBG("Timeshift recording stopped");
            STB_OSSendEvent(FALSE, HW_EV_CLASS_PVR, HW_EV_TYPE_PVR_REC_STOP,
               &ts_status->rec_index, sizeof(U8BIT));
            break;
         }

         case AM_TFILE_EVT_START_TIME_CHANGED:
         {
//            REC_DBG("TFile start changed: %ld", (long)param);
            break;
         }

         case AM_TFILE_EVT_END_TIME_CHANGED:
         {
//            REC_DBG("TFile end changed: %ld", (long)param);
            break;
         }

         default:
         {
            REC_DBG("Unhandled recording event %d", event_type);
            break;
         }
      }
   }
}

static void PlayEventHandler(long dev_no, int event_type, void *param, void *data)
{
   static int last_status = AV_TIMESHIFT_STATUS_EXIT;

   S_TIMESHIFT_STATUS *ts_status;

   if (data != NULL)
   {
      ts_status = (S_TIMESHIFT_STATUS *)data;

      switch (event_type)
      {
         case AM_AV_EVT_PLAYER_STATE_CHANGED:
         {
            /**< File player's state changed, the parameter is the new state(AM_AV_MPState_t)*/
            PLAY_DBG("State changed: %d", (AM_AV_MPState_t)param);
            break;
         }
         case AM_AV_EVT_PLAYER_SPEED_CHANGED:
         {
            /**< File player's playing speed changed, the parameter is the new speed(0:normal，<0:backward，>0:fast forward)*/
            PLAY_DBG("Speed changed: %ld", (long)param);
            break;
         }
         case AM_AV_EVT_PLAYER_TIME_CHANGED:
         {
            /**< File player's current time changed，the parameter is the current time*/
            PLAY_DBG("Time changed: %ld", (long)param);
            break;
         }
         case AM_AV_EVT_PLAYER_UPDATE_INFO:
         {
            /**< Update the current player information*/
            AM_AV_TimeshiftInfo_t *info = (AM_AV_TimeshiftInfo_t *)param;

            if (info->status != last_status)
            {
               PLAY_DBG("Info update: current=%d, full=%d, status=%d", info->current_time,
                  info->full_time, info->status);

               if ((ts_status->play_state == PLAY_STARTING) &&
                  ((info->status == AV_TIMESHIFT_STATUS_PLAY) ||
                   (info->status == AV_TIMESHIFT_STATUS_PAUSE) ||
                   (info->status == AV_TIMESHIFT_STATUS_FFFB)))
               {
                  /* Playback has started successfully */
                  PLAY_DBG("Timeshift playback has started");
                  ts_status->play_state = PLAY_STARTED;
                  STB_OSSendEvent(FALSE, HW_EV_CLASS_PVR, HW_EV_TYPE_PVR_PLAY_START, NULL, 0);
               }

               last_status = info->status;
            }
            break;
         }
         default:
         {
            PLAY_DBG("Unhandled event %d", event_type);
            break;
         }
      }
   }
}

