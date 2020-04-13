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

// Ocean Blue header files
#include "techtype.h"
#include "dbgfuncs.h"

#include "stbhwdef.h"
#include "stbhwos.h"
#include "stbhwmem.h"
#include "stbhwdsk.h"
#include "stbpvrpr.h"

/* third party header files */
#define  AV_AUDIO_STEREO        AV_AUDIO_STEREO_TSP
#define  AV_AUDIO_RIGHT         AV_AUDIO_RIGHT_TSP
#define  AV_AUDIO_LEFT          AV_AUDIO_LEFT_TSP
#define  AV_AUDIO_MONO          AV_AUDIO_MONO_TSP
#define  AV_AUDIO_MULTICHANNEL  AV_AUDIO_MULTICHANNEL_TSP
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
#define  AV_AUDIO_CODEC_AC3    AV_AUDIO_CODEC_AC3_TSP

#include "dvr_wrapper.h"

#ifdef SUPPORT_CAS
#include "am_cas.h"
#endif

#undef  AV_AUDIO_RIGHT
#undef  AV_AUDIO_LEFT
#undef  AV_AUDIO_MONO
#undef  AV_AUDIO_MULTICHANNEL
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
#undef  AV_AUDIO_CODEC_AC3



//---constant definitions for this file----------------------------------------
#define INVALID_RES_ID           255
#define DVR_MODE_PROP    "vendor.tv.dtv.dvr.mode"
//#define PRE_SET_AUDIO
#define INVALID_PLAYER_HDLE -1


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

typedef enum
{
   REC_STOPPED,
   REC_STARTING,
   REC_STARTED
} E_REC_STATE;

typedef struct
{
   U8BIT rec_index;

   U8BIT tuner;
   U8BIT rec_demux;

   DVR_WrapperRecord_t recorder;
   DVR_WrapperPidsInfo_t pids_info;

   BOOLEAN has_video;
   BOOLEAN has_audio;

   E_STB_PVR_START_MODE rec_mode;
   U32BIT timeshift_duration;

   U16BIT disk_id;
   U8BIT basename[16];

#ifdef SUPPORT_CAS
   S_CAS_STATUS cas_status;
   void *secure_buf;
   SecMemHandle secmem_handle;
#endif

   E_REC_STATE rec_state;
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
   DVR_VideoFormat_t video_fmt;
   U16BIT audio_pid;
   DVR_AudioFormat_t audio_fmt;
   U16BIT ad_pid;
   DVR_AudioFormat_t ad_fmt;
   U16BIT pcr_pid;

   DVR_WrapperPlayback_t player;
   am_tsplayer_handle tsplayer_handle;

#ifdef SUPPORT_CAS
   S_CAS_STATUS cas_status;
   void *secure_buf;
   SecMemHandle secmem_handle;
#endif

   DVR_PlaybackFlag_t flags;

   U16BIT disk_id;
   U8BIT basename[16];

   BOOLEAN is_timeshift;

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
   AV_TIMESHIFT_STATUS_STARTOK,
};

//---local (static) variable declarations for this file------------------------
//   (internal variables declared static to make them local)
static U8BIT num_recorders = 0;
static U8BIT num_players = 0;

static S_REC_STATUS *s_rec_status = NULL;
static S_RECPLAY_STATUS *s_recplay_status = NULL;

//---local function prototypes for this file-----------------------------------
//   (internal functions declared static to make them local)
static DVR_Result_t RecEventHandler(DVR_RecordEvent_t event, void *params, void *userdata);
static DVR_Result_t PlayEventHandler(DVR_PlaybackEvent_t event, void *params, void *userdata);
static void tsplayer_callback(void *user_data, am_tsplayer_event *event);
static U8BIT getPlayIndex(U8BIT audio_decoder, U8BIT video_decoder);
static U8BIT getRecIndex(U16BIT disk_id, U8BIT *name);
static U8BIT getDvrMode();
static void setDvrMode(U8BIT dvr_id, U8BIT mode);
static U32BIT getPVRConfigInt(const char *config, U32BIT def);
static U16BIT getDiskIdByRecIndex(U8BIT index);
static BOOLEAN updatePlayback(U8BIT play_index);


//---global function definitions-----------------------------------------------
#define DVR_STREAM_TYPE_TO_TYPE(_t) (((_t) >> 24) & 0xF)
#define DVR_STREAM_TYPE_TO_FMT(_t)  ((_t) & 0xFFFFFF)

static DVR_VideoFormat_t toDvrVideoFormat(E_STB_AV_VIDEO_CODEC codec)
{
   DVR_VideoFormat_t fmt = DVR_VIDEO_FORMAT_MPEG2;

   switch (codec)
   {
      case AV_VIDEO_CODEC_MPEG1:
         fmt = DVR_VIDEO_FORMAT_MPEG1;
      break;
      case AV_VIDEO_CODEC_MPEG2:
         fmt = DVR_VIDEO_FORMAT_MPEG2;
      break;
      case AV_VIDEO_CODEC_H264:
         fmt = DVR_VIDEO_FORMAT_H264;
      break;
      case AV_VIDEO_CODEC_H265:
         fmt = DVR_VIDEO_FORMAT_HEVC;
      break;
      default:
      break;
   }
   return fmt;
}

static DVR_AudioFormat_t toDvrAudioFormat(E_STB_AV_AUDIO_CODEC codec)
{
   DVR_AudioFormat_t fmt = DVR_AUDIO_FORMAT_MPEG;

   switch (codec)
   {
      case AV_AUDIO_CODEC_MP2:
      case AV_AUDIO_CODEC_MP3:
         fmt = DVR_AUDIO_FORMAT_MPEG;
      break;
      case AV_AUDIO_CODEC_EAC3:
         fmt = DVR_AUDIO_FORMAT_EAC3;
      break;
      case AV_AUDIO_CODEC_AC3:
         fmt = DVR_AUDIO_FORMAT_AC3;
      break;
      case AV_AUDIO_CODEC_AAC:
      case AV_AUDIO_CODEC_HEAAC:
         fmt = DVR_AUDIO_FORMAT_AAC;
      break;
      default:
      break;
   }
   return fmt;
}
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
            s_recplay_status[index].is_timeshift = FALSE;
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
            s_rec_status[index].rec_state = REC_STOPPED;
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
   U8BIT play_index;
   U8BIT rec_index;
   BOOLEAN is_timeshift = FALSE;
   int i;

   FUNCTION_START(STB_PVRPlayStart);

   USE_UNWANTED_PARAM(audio_decoder);

   play_started = FALSE;

   play_index = getPlayIndex(audio_decoder, video_decoder);

   if (play_index != INVALID_RES_ID)
   {
      rec_index = getRecIndex(disk_id, basename);
      if (rec_index != INVALID_RES_ID)
      {
         if (s_rec_status[rec_index].rec_mode == START_PAUSED)
         {
            is_timeshift = TRUE;
         }
      }

      s_recplay_status[play_index].play_demux = demux;
      s_recplay_status[play_index].disk_id = disk_id;
      strncpy((char*)s_recplay_status[play_index].basename,
         (char*)basename, sizeof(s_recplay_status[play_index].basename));
      s_recplay_status[play_index].is_timeshift = is_timeshift;

      s_recplay_status[play_index].has_audio = FALSE;
      s_recplay_status[play_index].audio_pid = 0;
      s_recplay_status[play_index].has_video = FALSE;
      s_recplay_status[play_index].video_pid = 0;

      if (is_timeshift)
      {
         s_recplay_status[play_index].play_speed = 0;

         {
            DVR_WrapperPidsInfo_t *p_pids_info = &s_rec_status[rec_index].pids_info;

            for (i = 0; i < p_pids_info->nb_pids; i++) {
               switch (DVR_STREAM_TYPE_TO_TYPE(p_pids_info->pids[i].type))
               {
                  case DVR_STREAM_TYPE_VIDEO:
                  s_recplay_status[play_index].has_video = s_rec_status[rec_index].has_video;
                  s_recplay_status[play_index].video_pid = p_pids_info->pids[i].pid;
                  s_recplay_status[play_index].video_fmt = DVR_STREAM_TYPE_TO_FMT(p_pids_info->pids[i].type);
                  break;
#ifdef PRE_SET_AUDIO
                  /*audio track will be resolved from upper layer*/
                  case DVR_STREAM_TYPE_AUDIO:
                  s_recplay_status[play_index].has_audio = s_rec_status[rec_index].has_audio;
                  s_recplay_status[play_index].audio_pid = p_pids_info->pids[i].pid;
                  s_recplay_status[play_index].audio_fmt = DVR_STREAM_TYPE_TO_FMT(p_pids_info->pids[i].type);
                  break;
#endif
                  default:
                  break;
               }
            }
         }
      }
      else
      {
         s_recplay_status[play_index].play_speed = 100;

         {
            uint32_t segment_nb;
            uint64_t *p_segment_ids;
            DVR_RecordSegmentInfo_t seg_info;
            int error;
            char location[512];


            STB_DSKFullPathname(s_recplay_status[play_index].disk_id,
                s_recplay_status[play_index].basename,
                location,
                sizeof(location));

            error = dvr_segment_get_list(location, &segment_nb, &p_segment_ids);
            if (!error && segment_nb) {
               error = dvr_segment_get_info(location, p_segment_ids[0], &seg_info);
               free(p_segment_ids);
            }

            if (!error) {
               for (i = 0; i < seg_info.nb_pids; i++) {
                  switch (DVR_STREAM_TYPE_TO_TYPE(seg_info.pids[i].type))
                  {
                     case DVR_STREAM_TYPE_VIDEO:
                     s_recplay_status[play_index].has_video = TRUE;
                     s_recplay_status[play_index].video_pid = seg_info.pids[i].pid;
                     s_recplay_status[play_index].video_fmt = DVR_STREAM_TYPE_TO_FMT(seg_info.pids[i].type);
                     break;
#ifdef PRE_SET_AUDIO
                     case DVR_STREAM_TYPE_AUDIO:
                     s_recplay_status[play_index].has_audio = TRUE;
                     s_recplay_status[play_index].audio_pid = seg_info.pids[i].pid;
                     s_recplay_status[play_index].audio_fmt = DVR_STREAM_TYPE_TO_FMT(seg_info.pids[i].type);
                     break;
#endif
                     default:
                     break;
                  }
               }
            }
         }
      }

      {
         PLAY_DBG("ready to start play...");
         play_started = updatePlayback(play_index);
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

      int error;
      U8BIT play_index;

   FUNCTION_START(STB_PVRPlaySetPosition);

   retval = FALSE;

   PLAY_DBG("set play position: %d(s)", position_in_seconds);

   play_index = getPlayIndex(audio_decoder, video_decoder);
   if (play_index != INVALID_RES_ID)
   {
      if (s_recplay_status[play_index].play_state == PLAY_STARTED)
      {
         if (s_recplay_status[play_index].play_speed == 0)
         {
            error = dvr_wrapper_seek_playback(s_recplay_status[play_index].player, position_in_seconds * 1000);
            if (!error)
            {
               PLAY_DBG("%lu secs", position_in_seconds);
               retval = TRUE;
            }
            else
            {
               PLAY_DBG("Failed to set play position, error 0x%x", error);
            }
         }
         else
         {
            error = dvr_wrapper_seek_playback(s_recplay_status[play_index].player, position_in_seconds * 1000);
            if (!error)
            {
               PLAY_DBG("%lu secs", position_in_seconds);
               retval = TRUE;
            }
            else
            {
               PLAY_DBG("Failed to set play position, error 0x%x", error);
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

   int error;
   U8BIT play_index;

   FUNCTION_START(STB_PVRPlayStop);

   play_index = getPlayIndex(audio_decoder, video_decoder);
   if (play_index != INVALID_RES_ID)
   {
      if (s_recplay_status[play_index].play_state != PLAY_STOPPED)
      {
         error = dvr_wrapper_stop_playback(s_recplay_status[play_index].player);
         if (!error)
         {
            PLAY_DBG("Timeshift playback stopped");
         }
         else
         {
            PLAY_DBG("Failed to stop timeshift playback, error %d", error);
         }

#ifdef SUPPORT_CAS
         if (s_recplay_status[play_index].cas_status.is_smp)
         {
             PLAY_DBG("destroy secmem handle:%#x, secure_buf:%#x",
                s_recplay_status[play_index].secmem_handle,
                s_recplay_status[play_index].secure_buf);
             
             if (s_recplay_status[play_index].secmem_handle)
             {
                 AM_CA_DestroySecmem(s_recplay_status[play_index].secmem_handle);
                 s_recplay_status[play_index].secmem_handle = (SecMemHandle)NULL;
                 s_recplay_status[play_index].secure_buf = NULL;
             }
         }
#endif
         error = dvr_wrapper_close_playback(s_recplay_status[play_index].player);

         {
            /*release TsPlayer*/
            AmTsPlayer_release(s_recplay_status[play_index].tsplayer_handle);
            s_recplay_status[play_index].tsplayer_handle = INVALID_PLAYER_HDLE;
         }

         s_recplay_status[play_index].play_state = PLAY_STOPPED;
         s_recplay_status[play_index].video_decoder = INVALID_RES_ID;
         s_recplay_status[play_index].audio_decoder = INVALID_RES_ID;
         s_recplay_status[play_index].player = NULL;

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

#ifdef SUPPORT_CAS
/**
 * @brief   Sets the cas status for a pvr play. This function should be called
 *          before the timeshift is started and is used to when pausing live TV.
 * @param   audio_decoder audio decoder being used for playback
 * @param   video_decoder video decoder being used for playback
 * @param   cas_status cas status
 */
void STB_PVRPlaySetCASStatus(U8BIT audio_decoder, U8BIT video_decoder, S_CAS_STATUS *cas_status)
{
   U8BIT play_index;

   FUNCTION_START(STB_PVRPlaySetCASStatus);

   REC_DBG("dec_cb[%#x], is_smp[%u], cb_param[%#x]",
		cas_status->crypto_cb,
		cas_status->is_smp,
		cas_status->cb_param);

   play_index = getPlayIndex(audio_decoder, video_decoder);
   if (play_index != INVALID_RES_ID)
   {
      memcpy(&s_recplay_status[play_index].cas_status, cas_status, sizeof(S_CAS_STATUS));
   }

   FUNCTION_FINISH(STB_PVRPlaySetCASStatus);
}
#endif

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
   int error;
   DVR_WrapperRecordOpenParams_t rec_open_params;
   DVR_WrapperRecordStartParams_t rec_start_params;
   BOOLEAN is_timeshift;
   U8BIT dvr_mode;
   int cnt;

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

      memset(&rec_open_params, 0, sizeof(DVR_WrapperRecordOpenParams_t));

      rec_open_params.dmx_dev_id = s_rec_status[rec_index].rec_demux;
      rec_open_params.segment_size = 100 * 1024 * 1024;/*100MB*/
      rec_open_params.max_size = 0;
      rec_open_params.max_time = 0;
      rec_open_params.event_fn = RecEventHandler;
      rec_open_params.event_userdata = &s_rec_status[rec_index];
      rec_open_params.flags = 0;
      if (is_timeshift)
          rec_open_params.flags |= DVR_RECORD_FLAG_ACCURATE;

      dvr_mode = getDvrMode();
      setDvrMode(rec_open_params.dmx_dev_id, dvr_mode);

      STB_DSKFullPathname(disk_id, NULL, (U8BIT *)rec_open_params.location,
         sizeof(rec_open_params.location));
      strncpy((char*)s_rec_status[rec_index].basename, (char*)basename, sizeof(s_rec_status[rec_index].basename));
      snprintf(rec_open_params.location, DVR_MAX_LOCATION_SIZE,
            "%s/%s", rec_open_params.location, s_rec_status[rec_index].basename);

      REC_DBG("Starting recording in directory \"%s\"", rec_open_params.location);

      rec_open_params.is_timeshift = (is_timeshift) ? DVR_TRUE : DVR_FALSE;

#ifdef SUPPORT_CAS
	 s_rec_status[rec_index].timeshift_duration = 120;  //TODO: will remove

      PLAY_DBG("is_smp:%d", s_rec_status[rec_index].cas_status.is_smp);
	 if (s_rec_status[rec_index].cas_status.is_smp)
	 {
	    rec_open_params.crypto_data = (void *)s_rec_status[rec_index].cas_status.cb_param;
	    rec_open_params.crypto_fn = s_rec_status[rec_index].cas_status.crypto_cb;
	 }
#endif

      error = dvr_wrapper_open_record(&s_rec_status[rec_index].recorder, &rec_open_params);
      if (!error)
      {
         s_rec_status[rec_index].disk_id = disk_id;
         strncpy((char *)s_rec_status[rec_index].basename, (char *)basename, sizeof(s_rec_status[rec_index].basename));

         s_rec_status[rec_index].has_video = FALSE;
         s_rec_status[rec_index].has_audio = FALSE;

         memset(&s_rec_status[rec_index].pids_info, 0, sizeof(DVR_WrapperRecordStartParams_t));

         /* Setup the initial set of PIDs that are to be recorded */
         REC_DBG("Recording PIDs:");
         cnt = s_rec_status[rec_index].pids_info.nb_pids = 0;

         for (i = 0; i < num_pids && cnt < DVR_MAX_RECORD_PIDS_COUNT; i++)
         {
            if (pid_array[i].type == PVR_PID_TYPE_VIDEO)
            {
               s_rec_status[rec_index].has_video = TRUE;
               s_rec_status[rec_index].pids_info.pids[cnt].type =
                  (DVR_STREAM_TYPE_VIDEO << 24) | toDvrVideoFormat(pid_array[i].u.video_codec);
               s_rec_status[rec_index].pids_info.pids[cnt].pid = pid_array[i].pid;
               cnt++;
               REC_DBG("  VIDEO %u", pid_array[i].pid);
            }
            else if (pid_array[i].type == PVR_PID_TYPE_AUDIO)
            {
               s_rec_status[rec_index].has_audio = TRUE;
               s_rec_status[rec_index].pids_info.pids[cnt].type =
                  (DVR_STREAM_TYPE_AUDIO << 24) | toDvrAudioFormat(pid_array[i].u.audio_codec);
               s_rec_status[rec_index].pids_info.pids[cnt].pid = pid_array[i].pid;
               cnt++;
               REC_DBG("  AUDIO %u", pid_array[i].pid);
            }
            else if (pid_array[i].type == PVR_PID_TYPE_SUBTITLES)
            {
               s_rec_status[rec_index].pids_info.pids[cnt].type = DVR_STREAM_TYPE_SUBTITLE << 24;
               s_rec_status[rec_index].pids_info.pids[cnt].pid = pid_array[i].pid;
               cnt++;
               REC_DBG("  SUBTITLES %u", pid_array[i].pid);
            }
            else if (pid_array[i].type == PVR_PID_TYPE_TELETEXT)
            {
               s_rec_status[rec_index].pids_info.pids[cnt].type = DVR_STREAM_TYPE_TELETEXT << 24;
               s_rec_status[rec_index].pids_info.pids[cnt].pid = pid_array[i].pid;
               cnt++;
               REC_DBG("  TELETEXT %u", pid_array[i].pid);
            }
            else if (pid_array[i].type == PVR_PID_TYPE_SECTION)
            {
               s_rec_status[rec_index].pids_info.pids[cnt].type = DVR_STREAM_TYPE_OTHER << 24;
               s_rec_status[rec_index].pids_info.pids[cnt].pid = pid_array[i].pid;
               cnt++;
               REC_DBG("  SECTION %u", pid_array[i].pid);
            }
            else
            {
               REC_DBG("  Not recording %u, type %u", pid_array[i].pid, pid_array[i].type);
            }
         }
         s_rec_status[rec_index].pids_info.nb_pids = cnt;

#ifdef SUPPORT_CAS
        do
        {
            void *buf = NULL;
            SecMemHandle secmem_handle;
            uint32_t secmem_size = 0;

            if (!s_rec_status[rec_index].cas_status.is_smp)
                break;
            
            secmem_handle = AM_CA_CreateSecmem(SERVICE_PVR_RECORDING, &buf, &secmem_size);
            if (!secmem_handle)
            {
                REC_DBG("Create secmem session failed.");
                break;
            }
            s_rec_status[rec_index].secmem_handle = secmem_handle;
            s_rec_status[rec_index].secure_buf = buf;
            REC_DBG("secmem handle: %#x, secure_buf:%#x, size:%#x",
                    secmem_handle, buf, secmem_size);

            dvr_wrapper_set_record_secure_buffer(
                s_rec_status[rec_index].recorder,
                s_rec_status[rec_index].secure_buf,
                secmem_size);
        } while (0);
#endif

         REC_DBG("Starting %s recording %p for %d secs/%llu B, [%s.ts]",
            (is_timeshift)? "timeshift" : "normal",
            s_rec_status[rec_index].recorder,
            rec_open_params.max_time,
            rec_open_params.max_size,
            rec_open_params.location);

         memset(&rec_start_params, 0, sizeof(rec_start_params));
         rec_start_params.pids_info.nb_pids = s_rec_status[rec_index].pids_info.nb_pids;
         memcpy(&rec_start_params.pids_info.pids, s_rec_status[rec_index].pids_info.pids,
            sizeof(rec_start_params.pids_info.pids));
         error = dvr_wrapper_start_record(s_rec_status[rec_index].recorder, &rec_start_params);
         if (!error)
         {
            retval = TRUE;
            s_rec_status[rec_index].rec_state = REC_STARTING;
         }
         else
         {
#ifdef SUPPORT_CAS
             if (s_rec_status[rec_index].cas_status.is_smp)
             {
                 REC_DBG("rease secmem handle:%#x, secure_buf:%#x",
                    s_rec_status[rec_index].secmem_handle,
                    s_rec_status[rec_index].secure_buf);

                 if (s_rec_status[rec_index].secmem_handle)
                 {
                     AM_CA_DestroySecmem(s_rec_status[rec_index].secmem_handle);
                     s_rec_status[rec_index].secmem_handle = (SecMemHandle)NULL;
                     s_rec_status[rec_index].secure_buf = NULL;
                 }
             }
#endif
            REC_DBG("Failed to start recording, error %d", error);

            dvr_wrapper_close_record(s_rec_status[rec_index].recorder);
            s_rec_status[rec_index].recorder = NULL;
         }
      }
      else
      {
         REC_DBG("Failed to open recording, error %d", error);
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
   int error;

   FUNCTION_START(STB_PVRRecordStop);

   if (rec_index < num_recorders)
   {
      REC_DBG("Stopping recording %u, handle %p", rec_index, s_rec_status[rec_index].recorder);

      if (s_rec_status[rec_index].recorder != NULL)
      {
         error = dvr_wrapper_stop_record(s_rec_status[rec_index].recorder);
         if (error)
         {
            REC_DBG("Failed to stop recording %u, error %d", s_rec_status[rec_index].recorder, error);
         }
#ifdef SUPPORT_CAS
         if (s_rec_status[rec_index].cas_status.is_smp)
         {
             REC_DBG("rease secmem session:%#x, secure_buf:%#x",
                s_rec_status[rec_index].secmem_handle,
                s_rec_status[rec_index].secure_buf);
             if (s_rec_status[rec_index].secmem_handle)
             {
                 AM_CA_DestroySecmem(s_rec_status[rec_index].secmem_handle);
                 s_rec_status[rec_index].secmem_handle = (SecMemHandle)NULL;
                 s_rec_status[rec_index].secure_buf = NULL;
             }
         }
#endif

         dvr_wrapper_close_record(s_rec_status[rec_index].recorder);
         s_rec_status[rec_index].recorder = NULL;
         s_rec_status[rec_index].rec_state = REC_STOPPED;
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

#ifdef SUPPORT_CAS
/**
 * @brief   Sets the cas status for a recording. This function should be called
 *          before the recording is started and is used to when pausing live TV.
 * @param   rec_index recording index to be used for the recording
 * @param   cas_status cas status
 */
void STB_PVRRecordSetCASStatus(U8BIT rec_index, S_CAS_STATUS *cas_status)
{
   FUNCTION_START(STB_PVRRecordSetCASStatus);

   REC_DBG("index %u, enc_cb[%#x], is_smp[%u], cb_param[%#x]",
		rec_index, cas_status->crypto_cb,
		cas_status->is_smp, cas_status->cb_param);

   if (rec_index < num_recorders)
   {
      memcpy(&s_rec_status[rec_index].cas_status, cas_status, sizeof(S_CAS_STATUS));
   }

   FUNCTION_FINISH(STB_PVRRecordSetCASStatus);
}
#endif

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
      s_rec_status[rec_index].rec_state = REC_STOPPED;
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
      if (s_rec_status[rec_index].recorder != NULL)
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

   if ((rec_index < num_recorders) && (s_rec_status[rec_index].recorder != NULL))
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

static BOOLEAN check_speed_ok(S16BIT speed)
{
    return TRUE;
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
   int error;
   U8BIT play_index = INVALID_RES_ID;

   FUNCTION_START(STB_PVRSetPlaySpeed);
   USE_UNWANTED_PARAM(audio_decoder);

   retval = FALSE;

   play_index = getPlayIndex(audio_decoder, video_decoder);

   if (play_index != INVALID_RES_ID && s_recplay_status[play_index].play_state == PLAY_STARTED)
   {
      if (speed != s_recplay_status[play_index].play_speed)
      {
         if (speed == 0 && s_recplay_status[play_index].play_speed == 100)
         {
            error = dvr_wrapper_pause_playback(s_recplay_status[play_index].player);
         }
         else if (speed == 100 && s_recplay_status[play_index].play_speed == 0)
         {
            error = dvr_wrapper_resume_playback(s_recplay_status[play_index].player);
         }
         else if (check_speed_ok(speed))
         {
            if (s_recplay_status[play_index].play_speed == 0)
               error = dvr_wrapper_resume_playback(s_recplay_status[play_index].player);
            error = dvr_wrapper_set_playback_speed(s_recplay_status[play_index].player, speed);
         }
         else
         {
            PLAY_DBG("Unsupported play speed %d", speed);
            error = -1;
         }

         if (!error)
         {
            PLAY_DBG("Set play speed to %d%% -> %d%%",
               s_recplay_status[play_index].play_speed, speed);
            s_recplay_status[play_index].play_speed = speed;
            retval = TRUE;
         }
         else
         {
            PLAY_DBG("Failed to set play speed to %d (%d), error 0x%x", speed, speed, error);
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
   BOOLEAN ret = FALSE;
   U32BIT rec_size_kb;

   FUNCTION_START(STB_PVRIsValidRecording);

   REC_DBG("disk 0x%04x, name %s", disk_id, basename);

   ret = STB_PVRGetRecordingSize(disk_id, basename, &rec_size_kb);
   if (ret != TRUE)
   {
       if (rec_size_kb == 0)
           ret = FALSE;
   }

   FUNCTION_FINISH(STB_PVRIsValidRecording);

   return(ret);
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
   int error;
   uint32_t n_ids;
   uint64_t *p_ids;
   char file_path[DVR_MAX_LOCATION_SIZE];

   FUNCTION_START(STB_PVRDeleteRecording);

   STB_DSKFullPathname(disk_id, basename, file_path, sizeof(file_path));

   error = dvr_segment_get_list(file_path, &n_ids, &p_ids);
   if (!error) {
      int i;
      for (i = 0; i < n_ids; i++) {
         error = dvr_segment_delete(file_path, p_ids[i]);
         REC_DBG("delete recording: %s:%d %d.", file_path, p_ids[i], error);
      }
      free(p_ids);
   }
   if (!error)
   {
   }

   FUNCTION_FINISH(STB_PVRDeleteRecording);
   return (!error) ? TRUE : FALSE;
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

   int error;
   uint32_t n_ids;
   uint64_t *p_ids;
   DVR_RecordSegmentInfo_t info;
   uint64_t size_b;

   char file_path[DVR_MAX_LOCATION_SIZE];

   FUNCTION_START(STB_PVRGetRecordingInfo);

   retval = FALSE;
   *rec_size_kb = 0;

   STB_DSKFullPathname(disk_id, basename, file_path, sizeof(file_path));

   size_b = 0;
   memset(&info, 0, sizeof(info));
   error = dvr_segment_get_list(file_path, &n_ids, &p_ids);
   if (!error) {
      int i;
      for (i = 0; i < n_ids; i++) {
         error = dvr_segment_get_info(file_path, p_ids[i], &info);
         if (!error) {
            size_b += info.size;
         } else {
            REC_DBG("recording: %s:%d getinfo fail.", file_path, p_ids[i]);
            break;
         }
      }
      free(p_ids);
   }

   if (!error)
   {
      retval = TRUE;
      *rec_size_kb = size_b / 1024;
   }
   else
   {
      REC_DBG("Failed to get size on recording \"%s\", error %d", file_path, error);
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
   int error;

   U32BIT seconds;
   U8BIT play_index;

   FUNCTION_START(STB_PVRGetElapsedTime);

   retval = FALSE;

   play_index = getPlayIndex(audio_decoder, video_decoder);
   if (play_index != INVALID_RES_ID)
   {
      DVR_WrapperPlaybackStatus_t status;

      error = dvr_wrapper_get_playback_status(s_recplay_status[play_index].player, &status);
      if (!error)
      {
         seconds = status.info_cur.time / 1000;

         *elapsed_hours = seconds / 3600;
         *elapsed_mins = seconds / 60 - (*elapsed_hours * 60);
         *elapsed_secs = seconds - (*elapsed_hours * 3600) - (*elapsed_mins * 60);

         PLAY_DBG("%02u:%02u:%02u", *elapsed_hours, *elapsed_mins,
            *elapsed_secs);

         retval = TRUE;
      }
      else
      {
         PLAY_DBG("Failed to get timeshift playback info, error 0x%x", error);
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

      if (s_recplay_status[play_index].has_video)
         *video_pid = s_recplay_status[play_index].video_pid;
      else
         *video_pid = 0;

      if (s_recplay_status[play_index].has_audio)
         *audio_pid = s_recplay_status[play_index].audio_pid;
      else
         *audio_pid = 0;

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
BOOLEAN PVRChangeDecodePIDs(U8BIT audio_decoder, U8BIT video_decoder,
   U16BIT pcr_pid, U16BIT video_pid, U16BIT audio_pid, U16BIT ad_pid,
   U32BIT video_fmt, U32BIT audio_fmt, U32BIT ad_fmt)
{
   U16BIT *pids;
   U8BIT play_index;
   int video_changed = 0, audio_changed = 0, ad_changed = 0;
   BOOLEAN done = FALSE;

   FUNCTION_START(PVRChangeDecodePIDs);

   PLAY_DBG("%u: pcr=%u, video=%u, audio=%u, ad=%u", play_index, pcr_pid, video_pid, audio_pid, ad_pid);

   play_index = getPlayIndex(audio_decoder, video_decoder);
   if (play_index != INVALID_RES_ID)
   {
      if (s_recplay_status[play_index].audio_pid != audio_pid)
      {
         s_recplay_status[play_index].audio_pid = audio_pid;
         s_recplay_status[play_index].audio_fmt = toDvrAudioFormat(audio_fmt);

         if (s_recplay_status[play_index].audio_pid > 0
             && s_recplay_status[play_index].audio_pid < 0x1fff)
             s_recplay_status[play_index].has_audio = TRUE;
         else
             s_recplay_status[play_index].has_audio = FALSE;

         PLAY_DBG("audio pid changed.");
         audio_changed = 1;
      }
      if (s_recplay_status[play_index].ad_pid != ad_pid)
      {
         s_recplay_status[play_index].ad_pid = ad_pid;
         s_recplay_status[play_index].ad_fmt = toDvrAudioFormat(audio_fmt);

         PLAY_DBG("ad pid changed.");
         ad_changed = 1;
      }

      if (s_recplay_status[play_index].video_pid != video_pid)
      {
         s_recplay_status[play_index].video_pid = video_pid;
         s_recplay_status[play_index].video_fmt = toDvrVideoFormat(video_fmt);

         if (s_recplay_status[play_index].video_pid > 0
             && s_recplay_status[play_index].video_pid < 0x1fff)
             s_recplay_status[play_index].has_video = TRUE;
         else
             s_recplay_status[play_index].has_video = FALSE;

         PLAY_DBG("video pid changed.");
         video_changed = 1;
      }
      s_recplay_status[play_index].pcr_pid = pcr_pid;

      if (video_changed || audio_changed || ad_changed) {
         done = updatePlayback(play_index);
      }
   }

   FUNCTION_FINISH(PVRChangeDecodePIDs);
   return done;
}

/**
 * @brief   PVR will not start if less than this minimum free space
 * @return  minimum free space in KB
 */
U32BIT STB_PVRGetMinDiskSpace()
{
   return getPVRConfigInt("tv.dtv.pvr.disk_free_min_to_start_kb", 0);
}

/**
 * @brief   PVR will stop if less than this minimum free space(default 10MB)
 * @return  minimum free space in KB
 */
U32BIT STB_PVRGetMinDiskSpaceLeft()
{
   return getPVRConfigInt("tv.dtv.pvr.disk_free_min_to_stop_kb", 10*1024);
}

void STB_PVRCheckDiskSpace(void)
{
   U8BIT index;
   for (index = 0; index < num_recorders; index++)
   {
      if (STB_PVRIsRecordStarted(index))
      {
         U16BIT disk_id = getDiskIdByRecIndex(index);
         if (disk_id != INVALID_RES_ID && STB_DSKIsMounted(disk_id))
         {
            STB_DSKCheckSpace(disk_id);
         }
      }
   }
}

BOOLEAN STB_PVRGetPlayerHandle(U8BIT audio_decoder, U8BIT video_decoder, void **p_handle)
{
   U8BIT play_index;
   BOOLEAN ret;

   ret = FALSE;

   play_index = getPlayIndex(audio_decoder, video_decoder);
   if (play_index != INVALID_RES_ID)
   {
      if (p_handle && s_recplay_status[play_index].tsplayer_handle != INVALID_PLAYER_HDLE)
      {
         *p_handle = (void *)s_recplay_status[play_index].tsplayer_handle;
         ret = TRUE;
      }
   }
   return ret;
}

//---local function definitions------------------------------------------------

static BOOLEAN updatePlayback(U8BIT play_index)
{
   BOOLEAN done;
   DVR_PlaybackPids_t play_pids;
   int error;

   if (!s_recplay_status[play_index].has_video
      && !s_recplay_status[play_index].has_audio)
   {
      PLAY_DBG("Failed to start pvr playback, no a/v setting");
      return FALSE;
   }

   memset(&play_pids, 0, sizeof(play_pids));

   play_pids.video.type = DVR_STREAM_TYPE_VIDEO;
   play_pids.audio.type = DVR_STREAM_TYPE_AUDIO;
   play_pids.audio.type = DVR_STREAM_TYPE_AD;

   if (s_recplay_status[play_index].has_video) {
      play_pids.video.pid = s_recplay_status[play_index].video_pid;
      play_pids.video.format = s_recplay_status[play_index].video_fmt;
   } else {
      play_pids.video.pid = 0x1fff;
   }
   if (s_recplay_status[play_index].has_audio) {
      play_pids.audio.pid = s_recplay_status[play_index].audio_pid;
      play_pids.audio.format = s_recplay_status[play_index].audio_fmt;
   } else {
      play_pids.audio.pid = 0x1fff;
   }

   play_pids.ad.pid = s_recplay_status[play_index].ad_pid;
   play_pids.ad.format = s_recplay_status[play_index].ad_fmt;

   done = FALSE;

   if (s_recplay_status[play_index].play_state == PLAY_STOPPED)
   {
      DVR_WrapperPlaybackOpenParams_t play_params;
      /*start*/
      memset(&play_params, 0, sizeof(play_params));

      /*open TsPlayer*/
      {
         uint32_t versionM, versionL;
         am_tsplayer_init_params init_param =
         {
            .source = TS_MEMORY,
            .dmx_dev_id = s_recplay_status[play_index].play_demux,
            .event_mask = 0,
                 /*AM_TSPLAYER_EVENT_TYPE_PTS_MASK
               | AM_TSPLAYER_EVENT_TYPE_DTV_SUBTITLE_MASK
               | AM_TSPLAYER_EVENT_TYPE_USERDATA_AFD_MASK
               | AM_TSPLAYER_EVENT_TYPE_VIDEO_CHANGED_MASK
               | AM_TSPLAYER_EVENT_TYPE_AUDIO_CHANGED_MASK
               | AM_TSPLAYER_EVENT_TYPE_DATA_LOSS_MASK
               | AM_TSPLAYER_EVENT_TYPE_DATA_RESUME_MASK
               | AM_TSPLAYER_EVENT_TYPE_SCRAMBLING_MASK
               | AM_TSPLAYER_EVENT_TYPE_FIRST_FRAME_MASK,*/
         };
#ifdef SUPPORT_CAS
        if (s_recplay_status[play_index].cas_status.is_smp)
        {
            init_param.drmmode = TS_INPUT_BUFFER_TYPE_SECURE;
            PLAY_DBG("hanyh: open drmmode:%d", init_param.drmmode);
        }
#endif
         am_tsplayer_result result =
            AmTsPlayer_create(init_param, &s_recplay_status[play_index].tsplayer_handle);
         PLAY_DBG("open TsPlayer %s, result(%d)", (result)? "FAIL" : "OK", result);

         result = AmTsPlayer_getVersion(&versionM, &versionL);
         PLAY_DBG("TsPlayer verison(%d.%d) %s, result(%d)",
            versionM, versionL,
            (result)? "FAIL" : "OK",
            result);

         result = AmTsPlayer_registerCb(s_recplay_status[play_index].tsplayer_handle,
            tsplayer_callback,
            &s_recplay_status[play_index]);

         result = AmTsPlayer_setWorkMode(s_recplay_status[play_index].tsplayer_handle, TS_PLAYER_MODE_NORMAL);
         PLAY_DBG(" TsPlayer set Workmode NORMAL %s, result(%d)", (result)? "FAIL" : "OK", result);
         //result = AmTsPlayer_setSyncMode(s_recplay_status[play_index].tsplayer_handle, TS_SYNC_NOSYNC );
         //PLAY_DBG(" TsPlayer set Syncmode FREERUN %s, result(%d)", (result)? "FAIL" : "OK", result);
         result = AmTsPlayer_setSyncMode(s_recplay_status[play_index].tsplayer_handle, TS_SYNC_PCRMASTER );
         PLAY_DBG(" TsPlayer set Syncmode PCRMASTER %s, result(%d)", (result)? "FAIL" : "OK", result);

         play_params.playback_handle =
            (Playback_DeviceHandle_t)s_recplay_status[play_index].tsplayer_handle;
      }

      play_params.dmx_dev_id = s_recplay_status[play_index].play_demux;
      play_params.event_fn = PlayEventHandler;
      play_params.event_userdata = &s_recplay_status[play_index];
      play_params.block_size = 188 * 1024;
#ifdef SUPPORT_CAS
      PLAY_DBG("is_smp:%d", s_recplay_status[play_index].cas_status.is_smp);
      if (s_recplay_status[play_index].cas_status.is_smp)
      {
          play_params.block_size = 256*1024;
          play_params.crypto_fn = s_recplay_status[play_index].cas_status.crypto_cb;
          play_params.crypto_data = NULL;
          PLAY_DBG("dec_func:%#x", play_params.crypto_fn);
      }
#endif
      STB_DSKFullPathname(s_recplay_status[play_index].disk_id,
         s_recplay_status[play_index].basename,
         play_params.location,
         sizeof(play_params.location));
      play_params.is_timeshift = (s_recplay_status[play_index].is_timeshift)? DVR_TRUE : DVR_FALSE;

      error = dvr_wrapper_open_playback(&s_recplay_status[play_index].player, &play_params);
      if (!error)
      {
         DVR_PlaybackFlag_t play_flag =
            (s_recplay_status[play_index].play_speed == 0)? DVR_PLAYBACK_STARTED_PAUSEDLIVE : 0;

#ifdef SUPPORT_CAS
        do
        {
            void *buf = NULL;
            SecMemHandle secmem_handle;
            uint32_t secmem_size = 0;

            if (!s_recplay_status[play_index].cas_status.is_smp)
                break;

            secmem_handle = AM_CA_CreateSecmem(SERVICE_PVR_PLAY, &buf, &secmem_size);
            if (!secmem_handle)
            {
                PLAY_DEBUG("Create replay secmem session failed.");
                break;
            }

            s_recplay_status[play_index].secmem_handle = secmem_handle;
            s_recplay_status[play_index].secure_buf = buf;
            PLAY_DBG("secmem session: %#x, secure_buf:%#x, size:%#x",
                    secmem_handle, buf, secmem_size);

            dvr_wrapper_set_playback_secure_buffer(
                s_recplay_status[play_index].player,
                s_recplay_status[play_index].secure_buf,
                secmem_size);
        } while (0);
#endif
         PLAY_DBG("Starting pvr playback, speed=%u%%", s_recplay_status[play_index].play_speed);

         error = dvr_wrapper_start_playback(s_recplay_status[play_index].player, play_flag, &play_pids);
         if (error)
         {
            PLAY_DBG("Start pause/play failed, error %d", error);
         }

         done = TRUE;

         s_recplay_status[play_index].play_state = PLAY_STARTING;

         if (s_recplay_status[play_index].has_audio)
            STB_OSSendEvent(FALSE, HW_EV_CLASS_DECODE, HW_EV_TYPE_AUDIO_STARTED,
               &s_recplay_status[play_index].audio_decoder, sizeof(U8BIT));
      }
      else
      {
         PLAY_DBG("Failed to start pvr playback, error %d", error);
      }
   }
   else
   {
      /*update*/
      error = dvr_wrapper_update_playback(s_recplay_status[play_index].player, &play_pids);
      if (!error)
      {
         done = TRUE;
      }
      else
      {
         PLAY_DBG("update pvr playback failed, error %d", error);
      }
   }

   return done;
}

static U32BIT getPVRConfigInt(const char *config, U32BIT def)
{
    return property_get_int32(config, def);
}
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

static U8BIT getRecIndex(U16BIT disk_id, U8BIT *basename)
{
   U8BIT i;
   U8BIT rec_index = INVALID_RES_ID;

   for (i = 0; i < num_recorders && rec_index == INVALID_RES_ID; i++)
   {
      if ((s_rec_status[i].recorder != NULL) &&
         (s_rec_status[i].disk_id == disk_id) &&
         (strcmp((char *)s_rec_status[i].basename, (char *)basename) == 0))
      {
         rec_index = i;
      }
   }

   return rec_index;
}

static U16BIT getDiskIdByRecIndex(U8BIT index)
{
   return s_rec_status[index].disk_id;
}

static DVR_Result_t RecEventHandler(DVR_RecordEvent_t event, void *params, void *userdata)
{
   S_REC_STATUS *rec_status;

   if (userdata != NULL)
   {
      rec_status = (S_REC_STATUS *)userdata;
      DVR_WrapperRecordStatus_t *status = (DVR_WrapperRecordStatus_t *)params;

      switch (event)
      {
         case DVR_RECORD_EVENT_STATUS:
         {
            switch (status->state)
            {
               case DVR_RECORD_STATE_STARTED:
                  if (rec_status->rec_state == REC_STARTING) {
                     REC_DBG("Recording started, handle %p", rec_status->recorder);
                     rec_status->rec_state = REC_STARTED;
                     STB_OSSendEvent(FALSE, HW_EV_CLASS_PVR, HW_EV_TYPE_PVR_REC_START,
                        &rec_status->rec_index, sizeof(U8BIT));
                  }
               break;
               case DVR_RECORD_STATE_STOPPED:
                  REC_DBG("Recording stopped, handle %p", rec_status->recorder);
                  STB_OSSendEvent(FALSE, HW_EV_CLASS_PVR, HW_EV_TYPE_PVR_REC_STOP,
                     &rec_status->rec_index, sizeof(U8BIT));
               break;
               default:
               break;
            }
            break;
         }
         default:
         {
            REC_DBG("Unhandled recording event %d", event);
            break;
         }
      }
   }
   return DVR_SUCCESS;
}

static DVR_Result_t PlayEventHandler(DVR_PlaybackEvent_t event, void *params, void *userdata)
{
   S_RECPLAY_STATUS *play_status;

   if (userdata != NULL)
   {
      play_status = (S_RECPLAY_STATUS *)userdata;

      switch (event)
      {
         case DVR_PLAYBACK_EVENT_TRANSITION_OK:
         {
            /**< Update the current player information*/
            DVR_WrapperPlaybackStatus_t *status = (DVR_WrapperPlaybackStatus_t *)params;
            {
               PLAY_DBG("Info update: current=%d, full=%d, state=%d",
                  status->info_cur.time,
                  status->info_full.time,
                  status->state);

               if ((play_status->play_state == PLAY_STARTING) &&
                  ((status->state == DVR_PLAYBACK_STATE_START) ||
                   (status->state == DVR_PLAYBACK_STATE_PAUSE) ||
                   (status->state == DVR_PLAYBACK_STATE_FF) ||
                   (status->state == DVR_PLAYBACK_STATE_FB)))
               {
                  /* Playback has started successfully */
                  PLAY_DBG("Timeshift playback has started");
                  play_status->play_state = PLAY_STARTED;
                  STB_OSSendEvent(FALSE, HW_EV_CLASS_PVR, HW_EV_TYPE_PVR_PLAY_START, NULL, 0);
               }

            }
            break;
         }
         case DVR_PLAYBACK_EVENT_REACHED_END:
         {
            /**< File player's EOF*/
            PLAY_DBG("EOF");
            STB_OSSendEvent(FALSE, HW_EV_CLASS_PVR, HW_EV_TYPE_PVR_PLAY_EOF, NULL, 0);
            break;
         }
         default:
         {
            PLAY_DBG("Unhandled event %d", event);
            break;
         }
      }
   }
   return DVR_SUCCESS;
}

static void tsplayer_callback(void *user_data, am_tsplayer_event *event)
{
   S_RECPLAY_STATUS *play_status;

   if (user_data != NULL) {
      play_status = (S_RECPLAY_STATUS *)user_data;

      switch (event->type) {
         default:
         STB_AVNotifyEventHandler(play_status->audio_decoder, play_status->video_decoder, (void*)event);
         break;
      }
   }
}



