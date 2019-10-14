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

#define PLAY_DEBUG
#define RECORD_DEBUG

//---includes for this file----------------------------------------------------
// compiler library header files
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>

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
#define DVR_MODE_PROP    "tv.dtv.dvr.mode"



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
} S_REC_STATUS;

typedef struct {
   U8BIT play_index;

   U8BIT play_demux;

   E_STB_PVR_START_MODE play_mode;
   S16BIT play_speed;

   E_PLAY_STATE play_state;

   U8BIT video_decoder;
   U8BIT audio_decoder;

   BOOLEAN has_video;
   BOOLEAN has_audio;

   U16BIT video_pid;
   U16BIT audio_pid;
   U16BIT pcr_pid;
   U16BIT ad_pid;

} S_RECPLAY_STATUS;

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

static S_REC_STATUS *s_rec_status = NULL;
static S_RECPLAY_STATUS *s_recplay_status = NULL;

//---local function prototypes for this file-----------------------------------
//   (internal functions declared static to make them local)
static void RecEventHandler(long dev_no, int event_type, void *param, void *data);
static void PlayEventHandler(long dev_no, int event_type, void *param, void *data);
static U8BIT getPlayIndex(U8BIT audio_decoder, U8BIT video_decoder);
static U8BIT getRecIndex(U8BIT disk_id, U8BIT *name);
static U8BIT getDvrMode();
static void setDvrMode(U8BIT dvr_id, U8BIT mode);


static void *des_open();
static int des_close(void *cryptor);
static void des_crypt(void *cryptor, uint8_t *dst, uint8_t *src, int len, int decrypt);

static uint8_t des_key[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77};
static AM_Crypt_Ops_t des_ops = {
    .open = des_open,
    .close = des_close,
    .crypt = des_crypt,
};


//---global function definitions-----------------------------------------------

/**
 * @brief   Initialisation for playback
 * @param   num_audio_decoders number of audio decoders available
 * @param   num_video_decoders number of video decoders available
 * @return  Number of players, 0 if unsuccessful or unsupported
 */
U8BIT STB_PVRInitPlayback(U8BIT num_audio_decoders, U8BIT num_video_decoders)
{
   U8BIT index;

   FUNCTION_START(STB_PVRInitPlayback);

   USE_UNWANTED_PARAM(num_audio_decoders);

   if (num_video_decoders != 0)
   {
      num_players = num_video_decoders;

      PLAY_DBG("video decoders=%d audio decoders=%d", num_video_decoders, num_audio_decoders);

      s_recplay_status = (S_RECPLAY_STATUS*) STB_MEMGetSysRAM(sizeof(S_RECPLAY_STATUS) * num_players);
      if (s_recplay_status != NULL)
      {
         memset(s_recplay_status, 0, num_players * sizeof(S_RECPLAY_STATUS));

         for (index = 0; index < num_players; index++)
         {
            s_recplay_status[index].play_index = index;
            s_recplay_status[index].play_demux = INVALID_RES_ID;
            s_recplay_status[index].play_mode = START_RUNNING;
            s_recplay_status[index].play_speed = 100;
            s_recplay_status[index].play_state = PLAY_STOPPED;
            s_recplay_status[index].video_decoder = INVALID_RES_ID;
            s_recplay_status[index].audio_decoder = INVALID_RES_ID;
            s_recplay_status[index].video_pid = 0;
            s_recplay_status[index].audio_pid = 0;
            s_recplay_status[index].pcr_pid = 0;
            s_recplay_status[index].ad_pid = 0;
         }
      }
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
   U8BIT index;

   FUNCTION_START(STB_PVRInitRecording);
   USE_UNWANTED_PARAM(num_tuners);

   if (NUM_RECORDERS != 0)
   {
      num_recorders = NUM_RECORDERS;

      REC_DBG("recoders=%d", num_recorders);

      s_rec_status = (S_REC_STATUS*) STB_MEMGetSysRAM(sizeof(S_REC_STATUS) * num_recorders);
      if (s_rec_status != NULL)
      {
         memset(s_rec_status, 0, num_recorders * sizeof(S_REC_STATUS));

         for (index = 0; index < num_recorders; index++)
         {
            s_rec_status[index].rec_index = index;
            s_rec_status[index].tuner = INVALID_RES_ID;
            s_rec_status[index].rec_demux = INVALID_RES_ID;
            s_rec_status[index].rec_mode = START_RUNNING;
         }
      }
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

   if (video_decoder < num_players)
   {
      s_recplay_status[video_decoder].play_mode = mode;
      s_recplay_status[video_decoder].video_decoder = video_decoder;
      s_recplay_status[video_decoder].audio_decoder = audio_decoder;

      s_recplay_status[video_decoder].play_demux = INVALID_RES_ID;
      s_recplay_status[video_decoder].play_speed = 100;
      s_recplay_status[video_decoder].play_state = PLAY_STOPPED;
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
   AM_ErrorCode_t am_error = AM_SUCCESS;
   AM_AV_TimeshiftPara_t ts_params;
   U8BIT play_index;
   U8BIT rec_index;
   BOOLEAN is_timeshift = FALSE;

   FUNCTION_START(STB_PVRPlayStart);

   USE_UNWANTED_PARAM(audio_decoder);

   play_started = FALSE;

   play_index = getPlayIndex(audio_decoder, video_decoder);

   if (play_index != INVALID_RES_ID)
   {
      rec_index = getRecIndex(disk_id, basename);
      if (rec_index != INVALID_RES_ID)
      {
         if (s_recplay_status[rec_index].play_mode == START_PAUSED)
         {
            is_timeshift = TRUE;
         }
      }

      s_recplay_status[play_index].play_demux = demux;

      memset(&ts_params, 0, sizeof(AM_AV_TimeshiftPara_t));

      ts_params.dmx_id = demux;

      if (is_timeshift)
      {
         ts_params.mode = AM_AV_TIMESHIFT_MODE_TIMESHIFTING;
         ts_params.tfile = s_rec_status[rec_index].tfile;
         ts_params.start_paused = AM_TRUE;
         s_recplay_status[play_index].play_speed = 0;
         s_recplay_status[play_index].has_video = s_rec_status[rec_index].has_video;
         s_recplay_status[play_index].has_audio = s_rec_status[rec_index].has_audio;
         s_recplay_status[play_index].video_pid = s_rec_status[rec_index].media_info.vid_pid;
         s_recplay_status[play_index].audio_pid = s_rec_status[rec_index].media_info.audios[0].pid;

         ts_params.media_info.duration = s_rec_status[rec_index].timeshift_duration;

         /* The media info to be played back is the same as is being recorded */
         memcpy(&ts_params.media_info, &s_rec_status[rec_index].media_info,
            sizeof(AM_AV_TimeshiftMediaInfo_t));
      }
      else
      {
         char file_path[256];
         ts_params.mode = AM_AV_TIMESHIFT_MODE_PLAYBACK;
         STB_DSKFullPathname(disk_id, basename, file_path, sizeof(file_path));
         snprintf(ts_params.file_path, sizeof(ts_params.file_path), "%s.ts", file_path);
         ts_params.start_paused = AM_FALSE;
         s_recplay_status[play_index].play_speed = 100;

         am_error = AM_REC_GetMediaInfoFromFile(ts_params.file_path, &ts_params.media_info);
         if (am_error == AM_SUCCESS)
         {
            if (ts_params.media_info.vid_pid >= 0 && ts_params.media_info.vid_pid < 0x1fff)
            {
               s_recplay_status[play_index].has_video = TRUE;
            }
            else
            {
               s_recplay_status[play_index].has_video = FALSE;
            }
            if (ts_params.media_info.aud_cnt > 0)
            {
               s_recplay_status[play_index].has_audio = TRUE;
            }
            else
            {
               s_recplay_status[play_index].has_audio = FALSE;
            }
         }
         else
         {
            PLAY_DBG("Get Mediainfo from \"%s\" error: %x", ts_params.file_path, am_error);
         }
      }

      s_recplay_status[play_index].play_state = PLAY_STARTING;

      if (am_error == AM_SUCCESS)
      {
         U8BIT dvr_mode = getDvrMode();
         if ((aml_hw_cfg.pvr.encrypt & ((is_timeshift)? 0x10 : 0x01))
             && (dvr_mode == 0))
            AM_AV_SetCryptOps(video_decoder, &des_ops);
         else
            AM_AV_SetCryptOps(video_decoder, NULL);


         am_error = AM_AV_StartTimeshift(video_decoder, &ts_params);
         if (am_error == AM_SUCCESS)
         {
            PLAY_DBG("Starting timeshift playback, speed=%u%%", s_recplay_status[play_index].play_speed);

            AM_EVT_Subscribe(video_decoder, AM_AV_EVT_PLAYER_STATE_CHANGED, PlayEventHandler,
               &s_recplay_status[play_index]);
            AM_EVT_Subscribe(video_decoder, AM_AV_EVT_PLAYER_SPEED_CHANGED, PlayEventHandler,
               &s_recplay_status[play_index]);
            AM_EVT_Subscribe(video_decoder, AM_AV_EVT_PLAYER_TIME_CHANGED, PlayEventHandler,
               &s_recplay_status[play_index]);
            AM_EVT_Subscribe(video_decoder, AM_AV_EVT_PLAYER_UPDATE_INFO, PlayEventHandler,
               &s_recplay_status[play_index]);
#if 0
            am_error = AM_AV_PlayTimeshift(video_decoder);
            if ((am_error == AM_SUCCESS) && (s_recplay_status[play_index].play_speed == 0))
            {
               am_error = AM_AV_PauseTimeshift(video_decoder);
            }

            if (am_error != AM_SUCCESS)
            {
               PLAY_DBG("Start pause/play failed, error %d", am_error);
            }
#endif
            play_started = TRUE;
            STB_OSSendEvent(FALSE, HW_EV_CLASS_DECODE, HW_EV_TYPE_AUDIO_STARTED, &audio_decoder, sizeof(U8BIT));
         }
         else
         {
            PLAY_DBG("Failed to start timeshift, error %d", am_error);
         }
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
   U8BIT play_index;

   FUNCTION_START(STB_PVRIsPlayStarted);

   retval = FALSE;

   play_index = getPlayIndex(audio_decoder, video_decoder);
   if (play_index != INVALID_RES_ID)
   {
      if (s_recplay_status[play_index].play_state == PLAY_STARTED)
      {
         retval = TRUE;
      }
   }

   FUNCTION_FINISH(STB_PVRIsPlayStarted);

   return(retval);
}

/**
 * @brief   Returns status of playback with the given decoders
 * @param   audio_decoder audio decoder being used for playback
 * @param   video_decoder video decoder being used for playback
 * @return  TRUE if playback is not in progress with the given decoders
 */
BOOLEAN STB_PVRIsPlayStopped(U8BIT audio_decoder, U8BIT video_decoder)
{
   BOOLEAN retval;
   U8BIT play_index;

   FUNCTION_START(STB_PVRIsPlayStopped);

   retval = TRUE;

   play_index = getPlayIndex(audio_decoder, video_decoder);
   if (play_index != INVALID_RES_ID)
   {
      if (s_recplay_status[play_index].play_state > PLAY_STOPPED)
      {
         retval = FALSE;
      }
   }

   FUNCTION_FINISH(STB_PVRIsPlayStopped);

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
   U8BIT play_index;

   FUNCTION_START(STB_PVRPlaySetPosition);

   retval = FALSE;

   play_index = getPlayIndex(audio_decoder, video_decoder);
   if (play_index != INVALID_RES_ID)
   {
      if (s_recplay_status[play_index].play_state == PLAY_STARTED)
      {
         if (s_recplay_status[play_index].play_speed == 0)
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
   U8BIT play_index;

   FUNCTION_START(STB_PVRPlayStop);

   play_index = getPlayIndex(audio_decoder, video_decoder);
   if (play_index != INVALID_RES_ID)
   {
      if (s_recplay_status[play_index].play_state != PLAY_STOPPED)
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

         s_recplay_status[play_index].play_state = PLAY_STOPPED;
         s_recplay_status[play_index].video_decoder = INVALID_RES_ID;
         s_recplay_status[play_index].audio_decoder = INVALID_RES_ID;

         AM_EVT_Unsubscribe(video_decoder, AM_AV_EVT_PLAYER_STATE_CHANGED, PlayEventHandler,
            &s_recplay_status[play_index]);
         AM_EVT_Unsubscribe(video_decoder, AM_AV_EVT_PLAYER_SPEED_CHANGED, PlayEventHandler,
            &s_recplay_status[play_index]);
         AM_EVT_Unsubscribe(video_decoder, AM_AV_EVT_PLAYER_TIME_CHANGED, PlayEventHandler,
            &s_recplay_status[play_index]);
         AM_EVT_Unsubscribe(video_decoder, AM_AV_EVT_PLAYER_UPDATE_INFO, PlayEventHandler,
            &s_recplay_status[play_index]);

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
   U8BIT play_index;

   FUNCTION_START(STB_PVRPlayEnabled);

   play_index = getPlayIndex(audio_decoder, video_decoder);
   if ((play_index != INVALID_RES_ID) && (s_recplay_status[play_index].play_state != PLAY_STOPPED))
   {
      *video = s_recplay_status[play_index].has_video;
      *audio = s_recplay_status[play_index].has_audio;
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
   U8BIT rec_index = INVALID_RES_ID;
   int i;

   FUNCTION_START(STB_PVRAcquireRecorderIndex);

   for (i = 0; (i < num_recorders) && (rec_index == INVALID_RES_ID); i++)
   {
      if (s_rec_status[i].tuner == INVALID_RES_ID)
      {
         s_rec_status[i].tuner = tuner;
         s_rec_status[i].rec_demux = demux;
         rec_index = s_rec_status[i].rec_index;
      }
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
      s_rec_status[rec_index].tuner = INVALID_RES_ID;
      s_rec_status[rec_index].rec_demux = INVALID_RES_ID;
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
   BOOLEAN is_timeshift;
   U8BIT dvr_mode;

   FUNCTION_START(STB_PVRRecordStart);

   retval = FALSE;

   if (rec_index < num_recorders)
   {
      if (s_rec_status[rec_index].rec_mode == START_PAUSED)
      {
         is_timeshift = TRUE;
      }
      else
      {
         is_timeshift = FALSE;
      }

      memset(&create_params, 0, sizeof(AM_REC_CreatePara_t));

      create_params.fend_dev = s_rec_status[rec_index].tuner;
      create_params.dvr_dev = s_rec_status[rec_index].rec_demux;
      create_params.async_fifo_id = rec_index;

      dvr_mode = getDvrMode();
      setDvrMode(create_params.dvr_dev, dvr_mode);
      STB_DSKFullPathname(disk_id, NULL, (U8BIT *)create_params.store_dir,
         sizeof(create_params.store_dir));

      REC_DBG("Starting recording in directory \"%s\"", create_params.store_dir);

      am_error = AM_REC_Create(&create_params, &s_rec_status[rec_index].rec_handle);
      if (am_error == AM_SUCCESS)
      {
         s_rec_status[rec_index].disk_id = disk_id;
         strncpy((char *)s_rec_status[rec_index].basename, (char *)basename, sizeof(s_rec_status[rec_index].basename));

         AM_REC_SetTFile(s_rec_status[rec_index].rec_handle, NULL, REC_TFILE_FLAG_AUTO_CREATE);

         {
            AM_EVT_Subscribe((long)s_rec_status[rec_index].rec_handle, AM_REC_EVT_RECORD_START,
               RecEventHandler, &s_rec_status[rec_index]);
            AM_EVT_Subscribe((long)s_rec_status[rec_index].rec_handle, AM_REC_EVT_RECORD_END,
               RecEventHandler, &s_rec_status[rec_index]);
         }

         s_rec_status[rec_index].has_video = FALSE;
         s_rec_status[rec_index].has_audio = FALSE;

         memset(&rec_params, 0, sizeof(AM_REC_RecPara_t));
         memset(&s_rec_status[rec_index].media_info, 0, sizeof(AM_AV_TimeshiftMediaInfo_t));

         /* Setup the initial set of PIDs that are to be recorded */
         REC_DBG("Recording PIDs:");
         for (i = 0; i < num_pids; i++)
         {
            if (pid_array[i].type == PVR_PID_TYPE_VIDEO)
            {
               s_rec_status[rec_index].has_video = TRUE;
               s_rec_status[rec_index].media_info.vid_pid = pid_array[i].pid;

               switch (pid_array[i].u.video_codec)
               {
                  case AV_VIDEO_CODEC_MPEG1:
                  case AV_VIDEO_CODEC_MPEG2:
                     s_rec_status[rec_index].media_info.vid_fmt = VFORMAT_MPEG12;
                     break;
                  case AV_VIDEO_CODEC_H264:
                     s_rec_status[rec_index].media_info.vid_fmt = VFORMAT_H264;
                     break;
                  case AV_VIDEO_CODEC_H265:
                     s_rec_status[rec_index].media_info.vid_fmt = VFORMAT_HEVC;
                     break;
                  default:
                     break;
               }
               REC_DBG("  VIDEO %u", pid_array[i].pid);
            }
            else if (pid_array[i].type == PVR_PID_TYPE_AUDIO)
            {
               s_rec_status[rec_index].has_audio = TRUE;
               s_rec_status[rec_index].media_info.audios[s_rec_status[rec_index].media_info.aud_cnt].pid = pid_array[i].pid;

               switch(pid_array[i].u.audio_codec)
               {
                  case AV_AUDIO_CODEC_AC3:
                     s_rec_status[rec_index].media_info.audios[s_rec_status[rec_index].media_info.aud_cnt].fmt = AFORMAT_AC3;
                     break;
                  case AV_AUDIO_CODEC_EAC3:
                     s_rec_status[rec_index].media_info.audios[s_rec_status[rec_index].media_info.aud_cnt].fmt = AFORMAT_EAC3;
                     break;
                  case AV_AUDIO_CODEC_AAC:
                  case AV_AUDIO_CODEC_HEAAC:
                     s_rec_status[rec_index].media_info.audios[s_rec_status[rec_index].media_info.aud_cnt].fmt = AFORMAT_AAC;
                     break;
                  case AV_AUDIO_CODEC_MP2:
                  case AV_AUDIO_CODEC_MP3:
                     s_rec_status[rec_index].media_info.audios[s_rec_status[rec_index].media_info.aud_cnt].fmt = AFORMAT_MPEG;
                     break;
                  default:
                     break;
               }

               s_rec_status[rec_index].media_info.aud_cnt++;
               REC_DBG("  AUDIO %u", pid_array[i].pid);
            }
            else if (pid_array[i].type == PVR_PID_TYPE_SUBTITLES)
            {
               s_rec_status[rec_index].media_info.subtitles[s_rec_status[rec_index].media_info.sub_cnt].pid = pid_array[i].pid;
               s_rec_status[rec_index].media_info.sub_cnt++;
               REC_DBG("  SUBTITLES %u", pid_array[i].pid);
            }
            else if (pid_array[i].type == PVR_PID_TYPE_TELETEXT)
            {
               s_rec_status[rec_index].media_info.teletexts[s_rec_status[rec_index].media_info.ttx_cnt].pid = pid_array[i].pid;
               s_rec_status[rec_index].media_info.ttx_cnt++;
               REC_DBG("  TELETEXT %u", pid_array[i].pid);
            }
            else
            {
               REC_DBG("  Not recording %u, type %u", pid_array[i].pid, pid_array[i].type);
            }
         }

         memcpy(&rec_params.media_info, &s_rec_status[rec_index].media_info,
            sizeof(AM_AV_TimeshiftMediaInfo_t));

         if (is_timeshift)
         {
            strncpy(rec_params.prefix_name, "TimeShifting", AM_REC_NAME_MAX);
            rec_params.is_timeshift = true;
         }
         else
         {
            strncpy(rec_params.prefix_name, s_rec_status[rec_index].basename, AM_REC_NAME_MAX);
            rec_params.is_timeshift = false;
         }

         strncpy(rec_params.suffix_name, "ts", AM_REC_SUFFIX_MAX);
         rec_params.total_time = s_rec_status[rec_index].timeshift_duration;

         if (is_timeshift)
         {
            REC_DBG("Starting timeshift recording %p for %lu secs, [%s.ts]", s_rec_status[rec_index].rec_handle,
               rec_params.total_time, rec_params.prefix_name);
         }
         else
         {
            REC_DBG("Starting normal recording %p for %lu secs, [%s.ts]", s_rec_status[rec_index].rec_handle,
               rec_params.total_time, rec_params.prefix_name);
         }

         if ((aml_hw_cfg.pvr.encrypt & ((is_timeshift)? 0x10 : 0x01))
             && (dvr_mode == 0))
             rec_params.crypt_ops = &des_ops;

         am_error = AM_REC_StartRecord(s_rec_status[rec_index].rec_handle, &rec_params);
         if (am_error == AM_SUCCESS)
         {
            if (is_timeshift)
            {
               am_error = AM_REC_GetTFile(s_rec_status[rec_index].rec_handle,
                  &s_rec_status[rec_index].tfile, &tfile_flags);
               if (am_error == AM_SUCCESS)
               {
                  AM_EVT_Subscribe((long)s_rec_status[rec_index].tfile, AM_TFILE_EVT_START_TIME_CHANGED,
                     RecEventHandler, &s_rec_status[rec_index]);
                  AM_EVT_Subscribe((long)s_rec_status[rec_index].tfile, AM_TFILE_EVT_END_TIME_CHANGED,
                     RecEventHandler, &s_rec_status[rec_index]);

                  am_error = AM_TFile_TimeStart(s_rec_status[rec_index].tfile);
                  if (am_error != AM_SUCCESS)
                  {
                     REC_DBG("AM_TFile_TimeStart failed, error %d", am_error);
                  }
               }
               else
               {
                  REC_DBG("Failed to get recording tfile, error %d", am_error);
               }
            }

            retval = TRUE;
         }
         else
         {
            REC_DBG("Failed to start recording, error %d", am_error);

            {
               AM_EVT_Unsubscribe((long)s_rec_status[rec_index].rec_handle, AM_REC_EVT_RECORD_START,
                  RecEventHandler, &s_rec_status[rec_index]);
               AM_EVT_Unsubscribe((long)s_rec_status[rec_index].rec_handle, AM_REC_EVT_RECORD_END,
                  RecEventHandler, &s_rec_status[rec_index]);
            }

            AM_REC_Destroy(s_rec_status[rec_index].rec_handle);
            s_rec_status[rec_index].rec_handle = NULL;
         }
      }
      else
      {
         REC_DBG("Failed to create recording, error %d", am_error);
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
      REC_DBG("Stopping recording %u, handle %p", rec_index, s_rec_status[rec_index].rec_handle);

      if (s_rec_status[rec_index].rec_handle != NULL)
      {
         am_error = AM_REC_StopRecord(s_rec_status[rec_index].rec_handle);
         if (am_error != AM_SUCCESS)
         {
            REC_DBG("Failed to stop recording %u, error %d", s_rec_status[rec_index].rec_handle, am_error);
         }

         AM_EVT_Unsubscribe((long)s_rec_status[rec_index].rec_handle, AM_REC_EVT_RECORD_START,
            RecEventHandler, &s_rec_status[rec_index]);
         AM_EVT_Unsubscribe((long)s_rec_status[rec_index].rec_handle, AM_REC_EVT_RECORD_END,
            RecEventHandler, &s_rec_status[rec_index]);
         AM_EVT_Unsubscribe((long)s_rec_status[rec_index].tfile, AM_TFILE_EVT_START_TIME_CHANGED,
            RecEventHandler, &s_rec_status[rec_index]);
         AM_EVT_Unsubscribe((long)s_rec_status[rec_index].tfile, AM_TFILE_EVT_END_TIME_CHANGED,
            RecEventHandler, &s_rec_status[rec_index]);

         AM_REC_Destroy(s_rec_status[rec_index].rec_handle);
         s_rec_status[rec_index].rec_handle = NULL;
         s_rec_status[rec_index].tfile = NULL;
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
      s_rec_status[rec_index].rec_mode = mode;
      s_rec_status[rec_index].timeshift_duration = param;
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
      if (s_rec_status[rec_index].rec_handle != NULL)
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

   if ((rec_index < num_recorders) && (s_rec_status[rec_index].rec_handle != NULL))
   {
      *video = s_rec_status[rec_index].has_video;
      *audio = s_rec_status[rec_index].has_audio;
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
   U8BIT play_index = INVALID_RES_ID;
   int i;

   FUNCTION_START(STB_PVRSetPlaySpeed);
   USE_UNWANTED_PARAM(audio_decoder);

   retval = FALSE;

   play_index = getPlayIndex(audio_decoder, video_decoder);

   if (play_index != INVALID_RES_ID && s_recplay_status[play_index].play_state == PLAY_STARTED)
   {
      if (speed != s_recplay_status[play_index].play_speed)
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
            s_recplay_status[play_index].play_speed = speed;
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
   U8BIT play_index;

   FUNCTION_START(STB_PVRGetPlaySpeed);

   play_index = getPlayIndex(audio_decoder, video_decoder);

   if (play_index != INVALID_RES_ID)
   {
      speed = s_recplay_status[play_index].play_speed;
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

   return(TRUE);
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
   AM_TFile_t file_handle;
   AM_ErrorCode_t am_error;
   char file_path[256];

   FUNCTION_START(STB_PVRDeleteRecording);

   STB_DSKFullPathname(disk_id, basename, file_path, sizeof(file_path));
   snprintf(file_path, sizeof(file_path), "%s.ts", file_path);

   am_error = AM_TFile_Open(&file_handle, file_path, AM_FALSE, 0, 0);
   if (am_error == AM_SUCCESS)
   {
      file_handle->delete_on_close = 1;
      am_error = AM_TFile_Close(file_handle);
   }

   FUNCTION_FINISH(STB_PVRDeleteRecording);

   return (am_error == AM_SUCCESS) ? TRUE : FALSE;
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
   U8BIT rec_index;
   AM_TFile_t tfile;
   char file_path[256];

   FUNCTION_START(STB_PVRGetRecordingInfo);

   retval = FALSE;
   *rec_size_kb = 0;

/*
   rec_index = getRecIndex(disk_id, basename);
   if (rec_index != INVALID_RES_ID)
   {
      am_error = AM_REC_GetRecordInfo(s_rec_status[rec_index].rec_handle, &rec_info);
      if (am_error == AM_SUCCESS)
      {
         *rec_size_kb = rec_info.file_size / 1024;
         retval = TRUE;
      }
      else
      {
         REC_DBG("Failed to get info on recording %p, error %d", s_rec_status[rec_index].rec_handle,
            am_error);
      }
   }
*/
   STB_DSKFullPathname(disk_id, basename, file_path, sizeof(file_path));
   snprintf(file_path, sizeof(file_path), "%s.ts", file_path);
   am_error = AM_TFile_Open(&tfile, file_path, AM_FALSE, 0, 0);
   if (am_error == AM_SUCCESS)
   {
      *rec_size_kb = tfile->size / 1024;
      retval = TRUE;
      AM_TFile_Close(tfile);
   }
   else
   {
      REC_DBG("Failed to get size on recording \"%s\", error %d", file_path, am_error);
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
   U8BIT play_index;

   FUNCTION_START(STB_PVRGetElapsedTime);

   retval = FALSE;

   play_index = getPlayIndex(audio_decoder, video_decoder);
   if (play_index != INVALID_RES_ID)
   {
      am_error = AM_AV_GetTimeshiftInfo(s_recplay_status[play_index].video_decoder, &info);
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

/**
 * @brief get default disk by prop setting for android, which has high priority to the setting from apps
*/
U16BIT STB_PVRGetDefaultDiskForced(void)
{
   U8BIT forced_default_path[256] = { 0 };
   U8BIT forced_default_path_prop[] = "tv.dtv.pvr.path";
   U16BIT forced_default_disk_id = INVALID_DISK_ID;
   U16BIT num_disks;
   U16BIT index;
   U16BIT disk_id;
   U8BIT disk_path[256];

   AM_PropRead(forced_default_path_prop, forced_default_path, sizeof(forced_default_path));
   if (strlen(forced_default_path))
   {
      num_disks = STB_DSKGetNumDisks();
      for (index = 0; index < num_disks; index++)
      {
         disk_id = STB_DSKGetDiskIdByIndex(index);
         if (disk_id != INVALID_DISK_ID)
         {
            if (STB_DSKFullPathname(disk_id, (U8BIT *)"", disk_path, sizeof(disk_path)))
            {
               if (strcmp((char *)disk_path, forced_default_path) == 0 )
               {
                  forced_default_disk_id = disk_id;
                  REC_DBG("default disk forced to [%d][%s]", forced_default_disk_id, forced_default_path);
                  break;
               }
            }
         }
      }
   }
   return forced_default_disk_id;
}


/**
 * @brief   Internal function that returns the decode PIDs for the given pvr
 * @param   audio_decoder decoder id of the audio
 * @param   video_decoder decoder id of the video
 * @param   pcr_pid pointer for returned PCR PID value
 * @param   video_pid pointer for returned video PID value
 * @param   audio_pid pointer for returned audio PID value
 * @param   ad_pid pointer for returned AD PID value
 * @return  TRUE if pvr is valid and PIDs are returned, FALSE otherwise
 */
BOOLEAN PVRGetDecodePIDs(U8BIT audio_decoder, U8BIT video_decoder,
   U16BIT *pcr_pid, U16BIT *video_pid, U16BIT *audio_pid, U16BIT *ad_pid)
{
   BOOLEAN retval;
   U8BIT play_index;

   FUNCTION_START(PVRGetDecodePIDs);

   play_index = getPlayIndex(audio_decoder, video_decoder);
   if (play_index != INVALID_RES_ID)
   {
      *pcr_pid = s_recplay_status[play_index].pcr_pid;
      *video_pid = s_recplay_status[play_index].video_pid;
      *audio_pid = s_recplay_status[play_index].audio_pid;
      *ad_pid = s_recplay_status[play_index].ad_pid;
   }
   else
   {
      retval = FALSE;
   }

   FUNCTION_FINISH(PVRGetDecodePIDs);

   return(retval);
}

/**
 * @brief   Changes the packet IDs for the PCR Video, Audio, Text and Data
 * @param   audio_decoder decoder id of the pvr audio
 * @param   video_decoder decoder id of the pvr video
 * @param   pcr_pid The PID to use for the Program Clock Reference
 * @param   video_pid The PID to use for the Video PES
 * @param   audio_pid The PID to use for the Audio PES
 * @param   ad_pid The PID to use for the AD PES
 */
void PVRChangeDecodePIDs(U8BIT audio_decoder, U8BIT video_decoder,
   U16BIT pcr_pid, U16BIT video_pid, U16BIT audio_pid, U16BIT ad_pid)
{
   U16BIT *pids;
   U8BIT play_index;

   FUNCTION_START(PVRChangeDecodePIDs);

   PLAY_DBG("%u: pcr=%u, video=%u, audio=%u, ad=%u", play_index, pcr_pid, video_pid, audio_pid, ad_pid);

   play_index = getPlayIndex(audio_decoder, video_decoder);
   if (play_index != INVALID_RES_ID)
   {
      if (s_recplay_status[play_index].audio_pid != audio_pid)
      {
         s_recplay_status[play_index].audio_pid = audio_pid;
         PLAY_DBG("audio pid changed.");
      }
      if (s_recplay_status[play_index].video_pid != video_pid)
      {
         s_recplay_status[play_index].video_pid = video_pid;
         s_recplay_status[play_index].pcr_pid = pcr_pid;
         PLAY_DBG("video pid changed.");
         //should do something.
      }
   }

   FUNCTION_FINISH(PVRChangeDecodePIDs);
}


//---local function definitions------------------------------------------------
/**
 * @brief get dvr mode. This function is used for dvr
 * @return U8BIT mode
 */
static U8BIT getDvrMode()
{
   U8BIT mode = 0;

   BOOLEAN dvr_ts_enable = property_get_int32(DVR_MODE_PROP, 0);
   if (dvr_ts_enable)
   {
       mode = 1;
   }
   else
   {
       mode = 0;
   }
   return mode;
}
/**
 * @brief set dvr mode. This function is used for dvr
 * @param U8BIT dvr device num
 * @param U8BIT mode
 */
static void setDvrMode(U8BIT dvr_id, U8BIT mode)
{
   U8BIT dvr_mode[128];
   BOOLEAN dvr_ts_enable = (mode == 1)? TRUE : FALSE;
   sprintf(dvr_mode, "/sys/class/stb/dvr%d_mode", dvr_id);
   if (dvr_ts_enable)
   {
       STB_SPDebugWrite("setDvrMode: ts");
       AM_FileEcho(dvr_mode, "ts");
   }
   else
   {
       STB_SPDebugWrite("setDvrMode: pid");
       AM_FileEcho(dvr_mode, "pid");
   }
}


static U8BIT getPlayIndex(U8BIT audio_decoder, U8BIT video_decoder)
{
   U8BIT i;
   U8BIT play_index = INVALID_RES_ID;

   for (i = 0; i < num_players && play_index == INVALID_RES_ID; i++)
   {
      if (s_recplay_status[i].video_decoder == video_decoder
         || s_recplay_status[i].audio_decoder == audio_decoder)
      {
         play_index = i;
      }
   }
   return play_index;
}

static U8BIT getRecIndex(U8BIT disk_id, U8BIT *basename)
{
   U8BIT i;
   U8BIT rec_index = INVALID_RES_ID;

   for (i = 0; i < num_recorders && rec_index == INVALID_RES_ID; i++)
   {
      if ((s_rec_status[i].rec_handle != NULL) &&
         (s_rec_status[i].disk_id == disk_id) &&
         (strcmp((char *)s_rec_status[i].basename, (char *)basename) == 0))
      {
         rec_index = i;
      }
   }

   return rec_index;
}

static void RecEventHandler(long dev_no, int event_type, void *param, void *data)
{
   S_REC_STATUS *rec_status;

   if (data != NULL)
   {
      rec_status = (S_REC_STATUS *)data;

      switch (event_type)
      {
         case AM_REC_EVT_RECORD_START:
         {
            REC_DBG("Recording started, handle %p", rec_status->rec_handle);
            STB_OSSendEvent(FALSE, HW_EV_CLASS_PVR, HW_EV_TYPE_PVR_REC_START,
               &rec_status->rec_index, sizeof(U8BIT));
            break;
         }

         case AM_REC_EVT_RECORD_END:
         {
            REC_DBG("Recording stopped");
            STB_OSSendEvent(FALSE, HW_EV_CLASS_PVR, HW_EV_TYPE_PVR_REC_STOP,
               &rec_status->rec_index, sizeof(U8BIT));
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

   S_RECPLAY_STATUS *play_status;

   if (data != NULL)
   {
      play_status = (S_RECPLAY_STATUS *)data;

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

               if ((play_status->play_state == PLAY_STARTING) &&
                  ((info->status == AV_TIMESHIFT_STATUS_PLAY) ||
                   (info->status == AV_TIMESHIFT_STATUS_PAUSE) ||
                   (info->status == AV_TIMESHIFT_STATUS_FFFB)))
               {
                  /* Playback has started successfully */
                  PLAY_DBG("Timeshift playback has started");
                  play_status->play_state = PLAY_STARTED;
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

static void *des_open()
{
    char buf[4096];
    char *p1, *p2;

    AM_FileRead("/proc/cpuinfo", buf, sizeof(buf));
    if ((p1 = strstr(buf, "Serial"))) {
        if ((p2 = strstr(p1, ": ")))
            sscanf(p2, ": %02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx",
                &des_key[0], &des_key[1], &des_key[2], &des_key[3],
                &des_key[4], &des_key[5], &des_key[6], &des_key[7]);
    }

    printf("des key: %02x%02x%02x%02x%02x%02x%02x%02x\n",
        des_key[0], des_key[1], des_key[2], des_key[3],
        des_key[4], des_key[5], des_key[6], des_key[7]);

    return AM_CRYPT_des_open(des_key, 64);
}

static int des_close(void *cryptor)
{
    return AM_CRYPT_des_close(cryptor);
}

static void des_crypt(void *cryptor, uint8_t *dst, uint8_t *src, int len, int decrypt)
{
    AM_CRYPT_des_crypt(cryptor, dst, src, len, NULL, decrypt);
}


