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
 * If you or your organization is not a member of DTVKit then you have access
 * to this source code outside of the terms of the licence agreement
 * and you are expected to delete this and any associated files immediately.
 * Further information on DTVKit, membership and terms can be found at www.dtvkit.org
 *******************************************************************************/

#include <string.h>
#include <unistd.h>
#include <algorithm>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <vector>
#include <sstream>
using namespace std;

// Ocean Blue header files
#include "techtype.h"
//#include "dbgfuncs.h"

extern "C" {
#include "stbpvrpr.h"
#include "stbhwos.h"
#include "stbhwdsk.h"
#include "stbhwcfg.h"
//#include "stbhwdef.h"
//#include "stbhwmem.h"
//#include "stbhwdmx.h"
//#include "stb_utils.h"
#include "afd_ctrl.h"
}

#include "wrapper_pvr.h"
#include "wrapper_player.h"
#include "JNIASPlayer.h"
#include "JDvrLib.h"

#ifdef SUPPORT_CAS
#include "ca_glue.h"
#endif

#define PVR_DEBUG
#define DEBUG_FUNCTIONS

#ifdef PVR_DEBUG
#define PVR_DBG(x,...)      STB_SPDebugWrite("%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )
#else
#define PVR_DBG(x,...)
#endif
#define PVR_ERR(x,...)      STB_SPDebugWrite("%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )
#define PVR_INFO(x,...)     STB_SPDebugWrite("%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )

#ifdef DEBUG_FUNCTIONS
#define LOG_ENTER PVR_DBG("enter")
#define LOG_LEAVE PVR_DBG("leave")
#define LOG_LEAVE_EARLY PVR_DBG("leave early")
#else
#define LOG_ENTER
#define LOG_LEAVE
#define LOG_LEAVE_EARLY
#endif

#define LOG_NOT_IMPLEMENTED PVR_DBG("*NOT IMPLEMENTED*")

struct S_REC_STATUS
{
   BOOLEAN in_use;
   BOOLEAN is_timeshift;
   E_STB_PVR_START_MODE start_mode;
   U32BIT limit_seconds;   // in seconds
   U32BIT limit_size;      // in MB
   am_dvr_file_handle dvr_file_handle;
   am_dvr_recorder_handle dvr_recorder_handle;

   U8BIT state;   // Recorder state
   condition_variable state_cond;    // the condition is signaled when state has a change
   mutex state_mutex;  // the mutex associated with above condition

   am_dvr_recording_progress progress;
   U8BIT rec_index;
   U16BIT disk_id;
   vector<S_PVR_PID_INFO> pids_array;

   S_REC_STATUS() : in_use(FALSE), is_timeshift(FALSE), start_mode(START_RUNNING)
      , limit_seconds(0), limit_size(0), dvr_file_handle(NULL), dvr_recorder_handle(NULL)
      , state(0), state_cond{}, state_mutex{}, rec_index(INVALID_RES_ID), disk_id(INVALID_RES_ID)
   {
      pids_array.clear();
   }
};

struct S_RECPLAY_STATUS
{
   BOOLEAN in_use;
   BOOLEAN is_timeshift;
   E_STB_PVR_START_MODE start_mode;
   am_dvr_file_handle dvr_file_handle;
   am_dvr_player_handle dvr_player_handle;
   jni_asplayer_handle asplayer_handle;

   U8BIT state;   // Player state
   //condition_variable state_cond;    // the condition is signaled when state has a change
   //mutex state_mutex;  // the mutex associated with above condition
   am_dvr_playback_progress progress;
   S16BIT speed;
   U8BIT audio_decoder;
   U8BIT video_decoder;

   U16BIT video_pid;
   U16BIT audio_pid;

   S_RECPLAY_STATUS() : in_use(FALSE), is_timeshift(FALSE), start_mode(START_RUNNING)
      , dvr_file_handle(NULL), dvr_player_handle(NULL), asplayer_handle(0), state(0)
      //, state_cond{}, state_mutex{}
      , speed(0), audio_decoder(INVALID_RES_ID), video_decoder(INVALID_RES_ID)
      , video_pid(0), audio_pid(0)
   {
   }
};

#define MAX_RECORDERS 6
static S_REC_STATUS s_rec_status[MAX_RECORDERS];

#define MAX_PLAYERS 6
static S_RECPLAY_STATUS s_recplay_status[MAX_PLAYERS];

static U8BIT num_recorders = 0;
static U8BIT num_players = 0;

static void on_recorder_evt_cb(am_dvr_recorder_handle handle, am_dvr_recorder_event event, void *event_data);
static void on_player_evt_cb(am_dvr_player_handle handle, am_dvr_player_event event, void *event_data);
static am_dvr_stream_type type_map1(E_PVR_PID_TYPE type);
static int video_codec_map1(E_STB_AV_VIDEO_CODEC format);
static int audio_codec_map1(E_STB_AV_AUDIO_CODEC format);
static U32BIT getPVRConfigInt(const char *config, U32BIT def);
static U8BIT to_index(U8BIT video_decoder, U8BIT audio_decoder);

typedef vector<S_PVR_PID_INFO> PID_VECTOR;
static void get_outstanding_pids(PID_VECTOR& curr, PID_VECTOR& given, PID_VECTOR& to_add, PID_VECTOR& to_remove);
static ostream& operator<<(ostream& os, const S_PVR_PID_INFO& info);

/**
 * @brief   Initialisation for playback
 * @param   num_audio_decoders number of audio decoders available
 * @param   num_video_decoders number of video decoders available
 * @return  Number of players, 0 if unsuccessful or unsupported
 */
U8BIT STB_PVRInitPlayback(U8BIT num_audio_decoders, U8BIT num_video_decoders)
{
   LOG_ENTER;

   num_players = min(num_video_decoders,(U8BIT)MAX_PLAYERS);
   const U8BIT ret = num_players;

   LOG_LEAVE;
   return ret;
}

/**
 * @brief   Initialisation for recording
 * @param   num_tuners number of tuners available for recording
 * @return  Number of recorders, 0 if unsuccessful or unsupported
 */
U8BIT STB_PVRInitRecording(U8BIT num_tuners)
{
   LOG_ENTER;

   num_recorders = 2;
   const U8BIT ret = num_recorders;

   LOG_LEAVE;
   return ret;
}

int STB_PVRRecord_Encrypt(void *crypto_inf, void *cb_param)
{
    return 0;
}

int STB_PVRPlay_Decrypt(void *crypto_inf, void *cb_param)
{
    return 0;
}

/**
 * @brief   Set startup mode for playback
 * @param   audio_decoder audio decoder being used for playback
 * @param   video_decoder video decoder being used for playback
 * @param   mode playback startup mode
 */
void STB_PVRSetPlayStartMode(U8BIT audio_decoder, U8BIT video_decoder, E_STB_PVR_START_MODE mode)
{
   //LOG_ENTER;

   auto itBegin = s_recplay_status;
   auto itEnd = s_recplay_status + MAX_PLAYERS;
   auto pred = [](S_RECPLAY_STATUS& s){return s.in_use == FALSE;};
   auto it = find_if(itBegin,itEnd,pred);
   const int play_index = distance(itBegin,it);

   if ( play_index >= num_players )
   {
      PVR_ERR("Player index %d is invalid",play_index);
      return;
   }

   S_RECPLAY_STATUS* prps = &s_recplay_status[play_index];
   prps->in_use = TRUE;
   prps->start_mode = mode;
   prps->audio_decoder = audio_decoder;
   prps->video_decoder = video_decoder;
   fill_n((uint8_t*)&prps->progress,sizeof(am_dvr_playback_progress),0);
   PVR_INFO("play_index:%d, start_mode:%d", play_index, (int)prps->start_mode);

   //LOG_LEAVE;
}

/**
 * @brief   Sets the startup mode for a recording. This function should be called
 *          before the recording is started and is used to when pausing live TV
 *          in which case the additional param defines the length of the pause
 *          buffer to be used, in seconds.
 * @param   rec_index recording index to be used for the recording
 * @param   mode startup mode
 * @param   param additional parameter linked to the mode. When pausing live TV,
 *          this is the length of the pause buffer, in seconds.
 *          format:
 *             [0] - duration, in seconds
 *             [1] - size, in megabytes
 */
void STB_PVRSetRecordStartMode(U8BIT rec_index, E_STB_PVR_START_MODE mode, U32BIT *param)
{
   LOG_ENTER;

   if ( rec_index >= num_recorders )
   {
      PVR_ERR("Invalid recorder index %u is given",(U32BIT)rec_index);
      return;
   }

   S_REC_STATUS* prs = &s_rec_status[rec_index];
   prs->start_mode = mode;
   prs->limit_seconds = param[0];
   prs->limit_size = param[1];
   prs->is_timeshift = ((prs->limit_seconds > 0 || prs->limit_size > 0) ? TRUE : FALSE);
   PVR_INFO("start_mode:%d limit_seconds:%u, limit_size:%u, is_timeshift:%d",
         (int)prs->start_mode,prs->limit_seconds,prs->limit_size,(int)prs->is_timeshift);

   LOG_LEAVE;
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
   //LOG_ENTER;
   LOG_NOT_IMPLEMENTED;
   //LOG_LEAVE;
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
   //LOG_ENTER;
   LOG_NOT_IMPLEMENTED;
   //LOG_LEAVE;
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
   LOG_ENTER;

   const int play_index = to_index(video_decoder,audio_decoder);
   if ( play_index >= num_players )
   {
      PVR_ERR("Player index %d is invalid",play_index);
      return FALSE;
   }
   S_RECPLAY_STATUS* prps = &s_recplay_status[play_index];

   PVR_DBG("Calling Wrapper_Player_Initialise(WP_TUNER_TYPE_DVR_PLAY)");
   U8BIT av_path = STB_AVGetPath(video_decoder, audio_decoder);
   Wrapper_Player_Initialise(av_path, WP_TUNER_TYPE_DVR_PLAY);

   jni_asplayer_handle asplayer_handle;
   jni_asplayer_init_params asplayer_params;
   asplayer_params.source = JNI_ASPLAYER_TS_MEMORY;
   asplayer_params.playback_mode = JNI_ASPLAYER_PLAYBACK_MODE_PASSTHROUGH;
   asplayer_params.event_mask = 1;
   PVR_DBG("Calling Wrapper_Player_Create");
   int ret = Wrapper_Player_Create(asplayer_params, &asplayer_handle, av_path);
   if (ret != 0)
   {
      PVR_ERR("Failed to create Wrapper Player");
      return FALSE;
   }
   PVR_INFO("ASPlayer handle: %p",asplayer_handle);
   prps->asplayer_handle = asplayer_handle;

   //create afd context
   uint32_t decoder_id = Wrapper_Player_GetInstanceNo(asplayer_handle);
   afd_create_context(av_path, decoder_id);

   STB_AVSetPlayerHandle(audio_decoder,video_decoder,asplayer_handle);
   STB_AVNotifyEventHandler(audio_decoder, video_decoder, NULL, 0);

   U8BIT path_prefix[256] = {0};
   BOOLEAN ret1 = STB_DSKFullPathname(disk_id,basename,(PU8BIT)path_prefix,sizeof(path_prefix));
   if (ret1 == FALSE)
   {
      PVR_ERR("Failed to get path_prefix based on input disk_id %d and basename %s",disk_id,basename);
      LOG_LEAVE_EARLY;
      return FALSE;
   }
   PVR_INFO("path_prefix:%s",path_prefix);

   S8BIT ret2 = 0;
   ret2 = Wrapper_PVR_File_create3((PU8BIT)path_prefix,&prps->dvr_file_handle);
   if (ret2 == -1)
   {
      PVR_ERR("Failed to create recording file %s",path_prefix);
      LOG_LEAVE_EARLY;
      return FALSE;
   }
   PVR_INFO("file handle for playback: %p",prps->dvr_file_handle);

   wrapper_player_init_params params;
   params.asplayer_handle = asplayer_handle;
   params.jdvrfile_handle = prps->dvr_file_handle;
   params.callback = on_player_evt_cb;
   params.path = video_decoder;

   ret2 = Wrapper_PVR_Player_create(&params,&prps->dvr_player_handle);
   if (ret2 == -1)
   {
      PVR_ERR("Failed to create player");
      LOG_LEAVE_EARLY;
      return FALSE;
   }
   PVR_INFO("JDvrPlayer handle: %p",prps->dvr_player_handle);

   int32_t video_pid = 0;
   int32_t audio_pid = 0;
   Wrapper_PVR_File_getVideoPID(prps->dvr_file_handle,&video_pid);
   Wrapper_PVR_File_getAudioPID(prps->dvr_file_handle,&audio_pid);
   prps->video_pid = (U16BIT)video_pid;
   prps->audio_pid = (U16BIT)audio_pid;

   if (prps->start_mode == START_RUNNING)
   {
      ret2 = Wrapper_PVR_Player_play(prps->dvr_player_handle);
      if (ret2 == -1)
      {
         PVR_ERR("Failed to play recording");
         LOG_LEAVE_EARLY;
         return FALSE;
      }
   }
   else if (prps->start_mode == START_PAUSED)
   {
      ret2 = Wrapper_PVR_Player_pause(prps->dvr_player_handle);
      if (ret2 == -1)
      {
         PVR_ERR("Failed to pause playback");
         LOG_LEAVE_EARLY;
         return FALSE;
      }
   }

   LOG_LEAVE;
   return TRUE;
}

/**
 * @brief   Returns status of playback with the given decoders
 * @param   audio_decoder audio decoder being used for playback
 * @param   video_decoder video decoder being used for playback
 * @return  TRUE if playback is in progress with the given decoders
 */
BOOLEAN STB_PVRIsPlayStarted(U8BIT audio_decoder, U8BIT video_decoder)
{
   //LOG_ENTER;
   const int play_index = to_index(video_decoder,audio_decoder);
   if ( play_index >= num_players )
   {
      PVR_ERR("Player index %d is invalid",play_index);
      return FALSE;
   }
   S_RECPLAY_STATUS* prps = &s_recplay_status[play_index];

   BOOLEAN ret = ((prps->state >= 2 && prps->state <= 5) ? TRUE : FALSE);

   //PVR_DBG(" returns %s",(ret == TRUE ? "TRUE" : "FALSE"));

   //LOG_LEAVE;
   return ret;
}

/**
 * @brief   Returns status of playback with the given decoders
 * @param   audio_decoder audio decoder being used for playback
 * @param   video_decoder video decoder being used for playback
 * @return  TRUE if playback is not in progress with the given decoders
 */
BOOLEAN STB_PVRIsPlayStopped(U8BIT audio_decoder, U8BIT video_decoder)
{
   //LOG_ENTER;
   const int play_index = to_index(video_decoder,audio_decoder);
   if ( play_index >= num_players )
   {
      PVR_ERR("Player index %d is invalid",play_index);
      // In such condition, it is better to assume that play has been stopped
      return TRUE;
   }
   S_RECPLAY_STATUS* prps = &s_recplay_status[play_index];

   BOOLEAN ret = ((prps->state < 2 || prps->state > 5) ? TRUE : FALSE);

   //LOG_LEAVE;
   return ret;
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
   LOG_ENTER;

   const int play_index = to_index(video_decoder,audio_decoder);
   if ( play_index >= num_players )
   {
      PVR_ERR("Player index %d is invalid",play_index);
      return FALSE;
   }
   S_RECPLAY_STATUS* prps = &s_recplay_status[play_index];

   S8BIT ret = Wrapper_PVR_Player_seek(prps->dvr_player_handle, position_in_seconds);
   if (ret == -1)
   {
      PVR_ERR("Failed to seek");
   }

   LOG_LEAVE;
   return (ret == 0) ? TRUE : FALSE;
}

/**
 * @brief   Stops playback
 * @param   audio_decoder audio decoder being used for playback
 * @param   video_decoder video decoder being used for playback
 */
void STB_PVRPlayStop(U8BIT audio_decoder, U8BIT video_decoder)
{
   LOG_ENTER;

   const int play_index = to_index(video_decoder,audio_decoder);
   if ( play_index >= num_players )
   {
      PVR_ERR("Player index %d is invalid",play_index);
      return;
   }
   S_RECPLAY_STATUS* prps = &s_recplay_status[play_index];

   S8BIT ret = Wrapper_PVR_Player_stop(prps->dvr_player_handle);
   if (ret == -1)
   {
      PVR_ERR("Failed to stop playback");
   }
   fill_n((uint8_t*)&prps->progress,sizeof(am_dvr_playback_progress),0);
   prps->in_use = FALSE;

   ret = Wrapper_Player_Destroy(prps->asplayer_handle);
   if (ret < 0)
   {
       PVR_INFO("Destroy player failed, err:%d", ret);
   }
   else
   {
       PVR_INFO("Destroy player, player_handle[%d]:0x%u", play_index, prps->asplayer_handle);
       STB_AVSetPlayerHandle(audio_decoder,video_decoder, WRAPPER_PLAYER_INVALID_HANDLE);
   }
   //release afd context
   afd_release_context(play_index);

   LOG_LEAVE;
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
   //LOG_ENTER;
   LOG_NOT_IMPLEMENTED;
   //LOG_LEAVE;
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
   //LOG_ENTER;
   LOG_NOT_IMPLEMENTED;
   //LOG_LEAVE;
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
   LOG_ENTER;

   U8BIT rec_index;
   for (rec_index=0; rec_index<MAX_RECORDERS; rec_index++)
   {
      if (s_rec_status[rec_index].in_use == FALSE)
      {
         break;
      }
   }
   if (rec_index >= MAX_RECORDERS)
   {
      rec_index = 255;
      PVR_ERR("cannot get a recorder index and returns 255");
   }
   s_rec_status[rec_index].in_use = TRUE;
   s_rec_status[rec_index].rec_index = rec_index;
   PVR_INFO("returns rec_index: %d",(int)rec_index);

   LOG_LEAVE;
   return rec_index;
}

/**
 * @brief   Releases a recording index when no longer needed
 * @param   rec_index recoding index
 */
void STB_PVRReleaseRecorderIndex(U8BIT rec_index)
{
   LOG_ENTER;

   if (rec_index >= MAX_RECORDERS)
   {
      PVR_ERR("rec_index %d is invalid",(int)rec_index);
   }
   else
   {
      memset((U8BIT*)&s_rec_status[rec_index],0,sizeof(S_REC_STATUS));
   }

   LOG_LEAVE;
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
   E_STB_DMX_DESC_KEY_PARITY parity, U8BIT *key, U8BIT *iv, U16BIT num_pids, S_PVR_PID_INFO *pid_array)
{
   //LOG_ENTER;
   LOG_NOT_IMPLEMENTED;
   //LOG_LEAVE;

   return TRUE;
}

BOOLEAN STB_PVRApplyEncryptionKey(U8BIT rec_index, E_STB_DMX_DESC_TYPE desc_type,
                                   E_STB_DMX_DESC_KEY_PARITY parity, U8BIT *key, U8BIT *iv, U16BIT num_pids, S_PVR_PID_INFO *pid_array)
{
   //LOG_ENTER;
   LOG_NOT_IMPLEMENTED;
   //LOG_LEAVE;

   return TRUE;
}

U32BIT STB_PVRGetRecordingSegmentSizeKB()
{
   return getPVRConfigInt("vendor.tv.dtv.pvr.segment_size_kb", 100 * 1024/*100MB*/);
}

U32BIT STB_PVRGetTimeshiftRecordingSegmentSizeKB()
{
   return getPVRConfigInt("vendor.tv.dtv.pvr.timeshift_segment_size_kb", 100 * 1024/*100MB*/);
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
   LOG_ENTER;

   S_REC_STATUS* prs = &s_rec_status[rec_index];
   PVR_INFO("disk_id:%d, rec_index:%d, basename:%s, num_pids:%d",
         disk_id,rec_index,basename,num_pids);

   if (-1 == Wrapper_PVR_Initialise(prs->is_timeshift ? 1 : 0)) {
      LOG_LEAVE_EARLY;
      return FALSE;
   }

   U8BIT path_prefix[256] = {0};
   BOOLEAN ret1 = STB_DSKFullPathname(disk_id,basename,(PU8BIT)path_prefix,sizeof(path_prefix));
   if (ret1 == FALSE)
   {
      PVR_ERR("Failed to get path_prefix based on input disk_id %d and basename %s",disk_id,basename);
      LOG_LEAVE_EARLY;
      return FALSE;
   }
   PVR_INFO("path_prefix:%s",path_prefix);

   S8BIT ret2 = 0;
   if (prs->is_timeshift == FALSE)
   {
      ret2 = Wrapper_PVR_File_create1((PU8BIT)path_prefix,0,&prs->dvr_file_handle);
   }
   else
   {
      ret2 = Wrapper_PVR_File_create2((PU8BIT)path_prefix,prs->limit_size*1024*1024,prs->limit_seconds,TRUE,&prs->dvr_file_handle);
   }
   if (ret2 == -1)
   {
      PVR_ERR("Failed to create recording file %s",path_prefix);
      LOG_LEAVE_EARLY;
      return FALSE;
   }
   PVR_INFO("file handle for recording: %p",prs->dvr_file_handle);

   wrapper_recorder_init_params params;
   params.jdvrfile_handle = prs->dvr_file_handle;
   params.segment_size = 30*1024*1024;
   params.callback = on_recorder_evt_cb;
   params.is_timeshift = prs->is_timeshift;

   ret2 = Wrapper_PVR_Recorder_create(&params,&prs->dvr_recorder_handle);
   if (ret2 == -1)
   {
      PVR_ERR("Failed to create recorder");
      LOG_LEAVE_EARLY;
      return FALSE;
   }
   const auto& handle = prs->dvr_recorder_handle;
   PVR_INFO("recorder handle: %p",prs->dvr_recorder_handle);

   {
      unique_lock<mutex> lock(prs->state_mutex);
      if (prs->state == 0)
      {
         cv_status ret3 = prs->state_cond.wait_for(lock,chrono::seconds(3));
         if (ret3 == cv_status::timeout)
         {
            PVR_ERR("wait unsuccessfully, state:%d",prs->state);
         }
         else
         {
            PVR_DBG("wait successfully, state:%d",prs->state);
         }
      }
   }

   auto& curr = prs->pids_array;
   PID_VECTOR given(pid_array,pid_array+num_pids);
   PID_VECTOR to_add;
   PID_VECTOR to_remove;

   get_outstanding_pids(curr,given,to_add,to_remove);

   for_each(to_remove.begin(),to_remove.end(),[&handle](auto info){
      Wrapper_PVR_Recorder_removeStream(handle,info.pid);
   });

   for_each(to_add.begin(),to_add.end(),[&handle](auto info){
      int format = 0;
      if (info.type == PVR_PID_TYPE_VIDEO)
      {
         format = video_codec_map1(info.u.video_codec);
      }
      else if (info.type == PVR_PID_TYPE_AUDIO)
      {
         format = audio_codec_map1(info.u.audio_codec);
      }
      Wrapper_PVR_Recorder_addStream(handle,info.pid,type_map1(info.type),format);
   });
   swap(curr,given);

   ret2 = Wrapper_PVR_Recorder_start(handle);
   if (ret2 == -1)
   {
      PVR_ERR("Failed to start recorder");
      LOG_LEAVE_EARLY;
      return FALSE;
   }
   prs->disk_id = disk_id;

   LOG_LEAVE;
   return TRUE;
}

/**
* @brief   Pauses a recording currently taking place
* @param   rec_index recording index
* @return  TRUE if the recording is successfully paused, FALSE otherwise
*/
BOOLEAN STB_PVRRecordPause(U8BIT rec_index)
{
   LOG_ENTER;

   S_REC_STATUS* prs = &s_rec_status[rec_index];
   PVR_INFO("rec_index:%d", rec_index);
   const am_dvr_recorder_handle handle = prs->dvr_recorder_handle;

   S8BIT ret = Wrapper_PVR_Recorder_pause(handle);

   LOG_LEAVE;
   return (ret == 0) ? TRUE : FALSE;
}

/**
 * @brief   Resumes a paused recording
 * @param   rec_index recording index
 * @return  TRUE if the recording is successfully resumed, FALSE otherwise
 */
BOOLEAN STB_PVRRecordResume(U8BIT rec_index)
{
   LOG_ENTER;

   S_REC_STATUS* prs = &s_rec_status[rec_index];
   PVR_INFO("rec_index:%d", rec_index);
   const am_dvr_recorder_handle handle = prs->dvr_recorder_handle;

   U8BIT ret = Wrapper_PVR_Recorder_start(handle);

   LOG_LEAVE;
   return (ret == 0) ? TRUE : FALSE;
}

/**
 * @brief   Stops a recording
 * @param   rec_index recording index
 */
void STB_PVRRecordStop(U8BIT rec_index)
{
   LOG_ENTER;

   S_REC_STATUS* prs = &s_rec_status[rec_index];
   PVR_INFO("rec_index:%d", rec_index);
   const am_dvr_recorder_handle handle = prs->dvr_recorder_handle;

   // Don't call stop() if JDvrRecorder is in STOPPING or INITIAL state.
   if (prs->state > 1 && prs->state < 5) {
      U8BIT ret = Wrapper_PVR_Recorder_stop(handle);
      prs->disk_id = INVALID_RES_ID;
   }

   LOG_LEAVE;
}

/**
 * @brief   Changes the record descramble mode while recording
 * @param   rec_index current recording index  to be updated
 * @param   mode 1:descramble or 0:free
 * @return  TRUE if the mode have been successfully changed, FALSE otherwise
 */
BOOLEAN STB_PVRRecordChangeDesMode(U8BIT rec_index, int mode)
{
   //LOG_ENTER;
   LOG_NOT_IMPLEMENTED;
   //LOG_LEAVE;
   return TRUE;
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
   LOG_ENTER;
   S_REC_STATUS* prs = &s_rec_status[rec_index];
   const auto& handle = prs->dvr_recorder_handle;

   auto& curr = prs->pids_array;
   PID_VECTOR given(pids_array,pids_array+num_pids);
   PID_VECTOR to_add;
   PID_VECTOR to_remove;

   get_outstanding_pids(curr,given,to_add,to_remove);

   for_each(to_remove.begin(),to_remove.end(),[&handle](auto info){
      Wrapper_PVR_Recorder_removeStream(handle,info.pid);
   });

   for_each(to_add.begin(),to_add.end(),[&handle](auto info){
      int format = 0;
      if (info.type == PVR_PID_TYPE_VIDEO)
      {
         format = video_codec_map1(info.u.video_codec);
      }
      else if (info.type == PVR_PID_TYPE_AUDIO)
      {
         format = audio_codec_map1(info.u.audio_codec);
      }
      Wrapper_PVR_Recorder_addStream(handle,info.pid,type_map1(info.type),format);
   });
   swap(curr,given);

   LOG_LEAVE;
   return TRUE;
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
   //LOG_ENTER;
   LOG_NOT_IMPLEMENTED;
   //LOG_LEAVE;
}
#endif

/**
 * @brief   Returns whether recording has been started
 * @param   rec_index recording index being queried
 * @return  TRUE if recording has been started
 */
BOOLEAN STB_PVRIsRecordStarted(U8BIT rec_index)
{
   //LOG_ENTER;
   S_REC_STATUS* prs = &s_rec_status[rec_index];
   const am_dvr_recorder_handle handle = prs->dvr_recorder_handle;

   BOOLEAN ret = ((prs->state >= 2 && prs->state <= 4) ? TRUE : FALSE);
   //PVR_DBG("rec_index:%d, state:%d, ret:%d",rec_index,prs->state,(int)ret);

   //LOG_LEAVE;
   return TRUE;
}

/**
 * @brief   Returns status of audio/video recording
 * @param   rec_index recording index being used for recording
 * @param   video returned as TRUE if video data is being recorded
 * @param   audio returned as TRUE if audio data is being recorded
 */
void STB_PVRRecordEnabled(U8BIT rec_index, BOOLEAN *video, BOOLEAN *audio)
{
   //LOG_ENTER;
   LOG_NOT_IMPLEMENTED;
   //LOG_LEAVE;
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
   //LOG_ENTER;
   LOG_NOT_IMPLEMENTED;
   //LOG_LEAVE;
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
   LOG_ENTER;

   const int play_index = to_index(video_decoder,audio_decoder);
   if ( play_index >= num_players )
   {
      PVR_ERR("Player index %d is invalid",play_index);
      return FALSE;
   }
   S_RECPLAY_STATUS* prps = &s_recplay_status[play_index];

   PVR_DBG("input speed: %hd",speed);
   double speed2 = ((double)speed)/100.0;
   prps->speed = speed;
   int ret = Wrapper_PVR_Player_setSpeed(prps->dvr_player_handle,speed2);
   if (ret == -1)
   {
      PVR_ERR("Failed to setSpeed %.2f",speed2);
   }

   LOG_LEAVE;
   return (ret == 0) ? TRUE : FALSE;
}

/**
 * @brief   Returns the current playback speed
 * @param   audio_decoder audio decoder being used for playback
 * @param   video_decoder video decoder being used for playback
 * @return  current playback speed as a percentage
 */
S16BIT STB_PVRGetPlaySpeed(U8BIT audio_decoder, U8BIT video_decoder)
{
   //LOG_ENTER;

   const int play_index = to_index(video_decoder,audio_decoder);
   if ( play_index >= num_players )
   {
      PVR_ERR("Player index %d is invalid",play_index);
      return FALSE;
   }
   S_RECPLAY_STATUS* prps = &s_recplay_status[play_index];

   const S16BIT speed = prps->speed;
   //PVR_DBG("returns %hd",speed);

   //LOG_LEAVE;
   return speed;
}

/**
 * @brief   Unused function
 * @param   audio_decoder audio decoder being used for playback
 * @param   video_decoder video decoder being used for playback
 */
void STB_PVRSaveFrame(U8BIT audio_decoder, U8BIT video_decoder)
{
   //LOG_ENTER;
   LOG_NOT_IMPLEMENTED;
   //LOG_LEAVE;
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
   //LOG_ENTER;

   U8BIT path_prefix[256] = {0};
   BOOLEAN ret = STB_DSKFullPathname(disk_id,basename,(PU8BIT)path_prefix,sizeof(path_prefix));
   if (ret == FALSE)
   {
      PVR_ERR("Failed to get path prefix based on input disk_id %d and basename %s",disk_id,basename);
      LOG_LEAVE_EARLY;
      return FALSE;
   }

   int64_t recording_duration;
   ret = Wrapper_PVR_File_duration2(path_prefix,&recording_duration);
   if (ret == -1)
   {
      LOG_LEAVE_EARLY;
      return FALSE;
   }

   ret = ((recording_duration>0) ? TRUE : FALSE);
   PVR_DBG(" checking recording %s, result %s",path_prefix,(ret==TRUE ? "TRUE" : "FALSE"));

   //LOG_LEAVE;
   return ret;
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
   //LOG_ENTER;
   LOG_NOT_IMPLEMENTED;
   //LOG_LEAVE;

   return TRUE;
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
   LOG_ENTER;

   U8BIT path_prefix[256] = {0};
   BOOLEAN ret1 = STB_DSKFullPathname(disk_id,basename,(PU8BIT)path_prefix,sizeof(path_prefix));
   if (ret1 == FALSE)
   {
      PVR_ERR("Failed to get path prefix based on input disk_id %d and basename %s",disk_id,basename);
      LOG_LEAVE_EARLY;
      return FALSE;
   }
   PVR_INFO("recording to delete: %s",path_prefix);

   int ret = Wrapper_PVR_deleteRecord(path_prefix);
   if (ret == -1)
   {
      PVR_ERR("Failed to delete recording: %s",path_prefix);
   }

   LOG_LEAVE;
   return (ret == 0) ? TRUE : FALSE;
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
   //LOG_ENTER;

   U8BIT path_prefix[256] = {0};
   BOOLEAN ret1 = STB_DSKFullPathname(disk_id,basename,(PU8BIT)path_prefix,sizeof(path_prefix));
   if (ret1 == FALSE)
   {
      PVR_ERR("Failed to get path_prefix based on input disk_id %d and basename %s",disk_id,basename);
      LOG_LEAVE_EARLY;
      return FALSE;
   }

   S8BIT ret2 = 0;
   int64_t recording_size;
   ret2 = Wrapper_PVR_File_size2(path_prefix,&recording_size);
   if (ret2 == -1)
   {
      LOG_LEAVE_EARLY;
      return FALSE;
   }

   *rec_size_kb = recording_size/1024;
   //PVR_DBG(" returns %d kb",*rec_size_kb);

   //LOG_LEAVE;
   return TRUE;
}

/**
 * @brief   Returns the length in ms and the size in KB of the recording
 * @param   disk_id disk containing the recording to be queried
 * @param   basename base filename of recording to get info about
 * @param   secs returned length of recording in seconds
 * @param   rec_size_kb returned size of recording in kilobytes
 * @return  TRUE if the information is successfully gathered
 */
BOOLEAN STB_PVRGetRecordingLength(U16BIT disk_id, U8BIT *basename, U32BIT *rec_length_ms, U32BIT *rec_size_kb)
{
   //LOG_ENTER;

   U8BIT path_prefix[256] = {0};
   BOOLEAN ret1 = STB_DSKFullPathname(disk_id,basename,(PU8BIT)path_prefix,sizeof(path_prefix));
   if (ret1 == FALSE)
   {
      PVR_ERR("Failed to get path_prefix based on input disk_id %d and basename %s",disk_id,basename);
      LOG_LEAVE_EARLY;
      return FALSE;
   }

   int64_t recording_size;
   S8BIT ret2 = Wrapper_PVR_File_size2(path_prefix,&recording_size);
   if (ret2 == -1)
   {
      LOG_LEAVE_EARLY;
      return FALSE;
   }
   *rec_size_kb = recording_size/1024;

   int64_t recording_duration;
   ret2 = Wrapper_PVR_File_duration2(path_prefix,&recording_duration);
   if (ret2 == -1)
   {
      LOG_LEAVE_EARLY;
      return FALSE;
   }
   *rec_length_ms = recording_duration;

   PVR_DBG(" returns %d ms, %d kb",*rec_length_ms,*rec_size_kb);

   //LOG_LEAVE;
   return TRUE;
}

/**
 * @brief   Returns the elapsed playback time in hours, mins & secs
 * @param   audio_decoder audio decoder being used for playback
 * @param   video_decoder video decoder being used for playback
 * @param   elapsed_hours current number of hours into the playback
 * @param   elapsed_mins current number of minutes into the playback
 * @param   elapsed_secs current number of seconds into the playback
 * @param   elapsed_ms current number of seconds into the playback
 * @return  TRUE if the info has been successfully gathered
 */
BOOLEAN STB_PVRGetElapsedTime(U8BIT audio_decoder, U8BIT video_decoder, U16BIT *elapsed_hours,
   U8BIT *elapsed_mins, U8BIT *elapsed_secs, U16BIT *elapsed_ms)
{
   //LOG_ENTER;

   const int play_index = to_index(video_decoder,audio_decoder);
   if ( play_index >= num_players )
   {
      PVR_ERR("Player index %d is invalid",play_index);
      return FALSE;
   }
   S_RECPLAY_STATUS* prps = &s_recplay_status[play_index];

   const am_dvr_playback_progress progress = prps->progress;
   const U64BIT total = progress.currTime - progress.startTime;
   *elapsed_hours = total/1000/3600;
   *elapsed_mins = total/1000%3600/60;
   *elapsed_secs = total/1000%60;
   *elapsed_ms = total%1000;
   PVR_DBG("returns %02hu:%02hu:%02hu.%03hu",*elapsed_hours,*elapsed_mins,*elapsed_secs,*elapsed_ms);

   //LOG_LEAVE;
   return TRUE;
}

/**
 * @brief   Returns the length in time of the recording
 * @param   rec_index recording index to be set
 * @param   secs returned length of recording in seconds
 * @param   secs_truncated returned truncated length of recording in seconds
 * @return  TRUE if the information is successfully gathered
 */
BOOLEAN STB_PVRGetRecordingLengthTruncated(U8BIT rec_index, U32BIT *msecs, U32BIT *msecs_truncated)
{
   //LOG_ENTER;

   if ( rec_index >= num_recorders )
   {
      PVR_ERR("Invalid recorder index %u is given",(U32BIT)rec_index);
      return FALSE;
   }

   S_REC_STATUS* prs = &s_rec_status[rec_index];

   const am_dvr_recording_progress progress = prs->progress;
   *msecs = (U32BIT)progress.endTime;
   *msecs_truncated = (U32BIT)progress.startTime;

   //LOG_LEAVE;
   return TRUE;
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
   //LOG_ENTER;
   LOG_NOT_IMPLEMENTED;
   //LOG_LEAVE;
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
   //LOG_ENTER;
   LOG_NOT_IMPLEMENTED;
   //LOG_LEAVE;
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
   //LOG_ENTER;
   LOG_NOT_IMPLEMENTED;
   //LOG_LEAVE;
   return FALSE;
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
   //LOG_ENTER;
   LOG_NOT_IMPLEMENTED;
   //LOG_LEAVE;
}

/**
 * @brief get default disk by prop setting for android, which has high priority to the setting from apps
*/
U16BIT STB_PVRGetDefaultDiskForced(void)
{
   return 0;
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
   //LOG_ENTER;

   const int play_index = to_index(video_decoder,audio_decoder);
   if ( play_index >= num_players )
   {
      PVR_ERR("Player index %d is invalid",play_index);
      return FALSE;
   }
   S_RECPLAY_STATUS* prps = &s_recplay_status[play_index];

   *pcr_pid = prps->video_pid;
   *video_pid = prps->video_pid;
   *audio_pid = prps->audio_pid;
   *ad_pid = 0;
   PVR_DBG("video_pid:%hu, audio_pid:%hu, ad_pid:%hu, pcr_pid:%hu",
         *video_pid,*audio_pid,*ad_pid,*pcr_pid);

   //LOG_LEAVE;
   return TRUE;
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
   U32BIT video_fmt, U32BIT audio_fmt, U32BIT ad_fmt, U16BIT audio_presentation_id)
{
   //LOG_ENTER;
   LOG_NOT_IMPLEMENTED;
   //LOG_LEAVE;
   return TRUE;
}

/**
 * @brief   PVR will not start if less than this minimum free space
 * @return  minimum free space in KB
 */
U32BIT STB_PVRGetMinDiskSpace()
{
   return getPVRConfigInt("vendor.tv.dtv.pvr.disk_free_min_to_start_kb", 0);
}


/**
 * @brief   PVR will stop if less than this minimum free space(default 10MB)
 * @return  minimum free space in KB
 */
U32BIT STB_PVRGetMinDiskSpaceLeft()
{
   return getPVRConfigInt("vendor.tv.dtv.pvr.disk_free_min_to_stop_kb", 10*1024);
}

void STB_PVRCheckDiskSpace(void)
{
   U8BIT index;
   for (index = 0; index < num_recorders; index++)
   {
      if (STB_PVRIsRecordStarted(index))
      {
         U16BIT disk_id = s_rec_status[index].disk_id;
         //PVR_DBG("check disk_id [%u] mount[%d]index[%d]", disk_id, STB_DSKIsMounted(disk_id), index);
         if (disk_id != INVALID_RES_ID && STB_DSKIsMounted(disk_id))
         {
            STB_DSKCheckSpace(disk_id);
         }
      }
   }
}

BOOLEAN STB_PVRGetPlayerHandle(U8BIT audio_decoder, U8BIT video_decoder, void **p_handle)
{
   if (p_handle == NULL) {
      PVR_ERR("Invalid handle pointer");
      return FALSE;
   }
   const int play_index = to_index(video_decoder,audio_decoder);
   if ( play_index >= num_players )
   {
      PVR_ERR("Player index %d is invalid",play_index);
      return FALSE;
   }
   S_RECPLAY_STATUS* prps = &s_recplay_status[play_index];

   *p_handle = (void*)prps->asplayer_handle;
   PVR_DBG("returns asplayer handle: %p",*p_handle);
   return TRUE;
}

/**
 * @brief   Store libdvr specific information i.e. force_sysclock in
            s_rec_status of porting layer
 * @param   rec_index recorder index
 * @param   val value of force_sysclock.
            0: determine index time source based on actual situation
            1: force to use system clock as PVR index time source
 * @return  TRUE if store successfully, FALSE if invalid rec_index is given.
 */
BOOLEAN STB_PVRStoreLibdvrExtParam1InPortingLayer(U8BIT rec_index, U8BIT val)
{
   LOG_NOT_IMPLEMENTED;
   return FALSE;
}

static void on_recorder_evt_cb(am_dvr_recorder_handle handle, am_dvr_recorder_event event, void *event_data)
{
   auto itBegin = s_rec_status;
   auto itEnd = s_rec_status + MAX_RECORDERS;
   auto pred = [handle](S_REC_STATUS& rs){return rs.dvr_recorder_handle == handle;};
   auto it = find_if(itBegin,itEnd,pred);
   if (it == itEnd)
   {
      PVR_ERR("Input recorder handle %p is invalid",handle);
      return;
   }
   U8BIT rec_index = distance(itBegin,it);

   if (event == AM_DVR_RECORDER_EVENT_PROGRESS) {
      am_dvr_recording_progress* evt = (am_dvr_recording_progress*) event_data;
      if (evt != NULL) {
         it->progress = *evt;
         it->state = (U8BIT)evt->state;
         PVR_DBG("AM_DVR_RECORDER_EVENT_PROGRESS: "
               "sessionNumber:%d, state:%d, "
               "duration:%lld, startTime:%lld, endTime:%lld, "
               "numberOfSegments:%d, firstSegmentId:%d, lastSegmentId:%d, size:%lld",
               evt->sessionNumber,evt->state,
               evt->duration,evt->startTime,evt->endTime,
               evt->numberOfSegments,evt->firstSegmentId,evt->lastSegmentId,evt->size);
         STB_OSSendEvent(FALSE, HW_EV_CLASS_PVR, HW_EV_TYPE_PVR_REC_STORE, &it->rec_index, sizeof(U8BIT));
      }
   } else if (event == AM_DVR_RECORDER_EVENT_INITIAL_STATE) {
      PVR_DBG("AM_DVR_RECORDER_EVENT_INITIAL_STATE");
      {
         lock_guard<mutex> lock(it->state_mutex);
         it->state = 1;
      }
      it->state_cond.notify_all();
   } else if (event == AM_DVR_RECORDER_EVENT_STARTING_STATE) {
      PVR_DBG("AM_DVR_RECORDER_EVENT_STARTING_STATE");
      it->state = 2;
   } else if (event == AM_DVR_RECORDER_EVENT_STARTED_STATE) {
      PVR_DBG("AM_DVR_RECORDER_EVENT_STARTED_STATE");
      if (it->state == 2) {
         STB_OSSendEvent(FALSE, HW_EV_CLASS_PVR, HW_EV_TYPE_PVR_REC_START,&(it->rec_index),1);
         PVR_DBG("signal PVR_REC_START");
      }
      it->state = 3;
   } else if (event == AM_DVR_RECORDER_EVENT_PAUSED_STATE) {
      PVR_DBG("AM_DVR_RECORDER_EVENT_PAUSED_STATE");
      it->state = 4;
   } else if (event == AM_DVR_RECORDER_EVENT_STOPPING_STATE) {
      PVR_DBG("AM_DVR_RECORDER_EVENT_STOPPING_STATE");
      STB_OSSendEvent(FALSE, HW_EV_CLASS_PVR, HW_EV_TYPE_PVR_REC_STOP,&(it->rec_index),1);
      PVR_DBG("signal PVR_REC_STOP");
      it->state = 5;
   } else if (event == AM_DVR_RECORDER_EVENT_NO_DATA_ERROR) {
      PVR_DBG("AM_DVR_RECORDER_EVENT_NO_DATA_ERROR");
   } else if (event == AM_DVR_RECORDER_EVENT_IO_ERROR) {
      PVR_DBG("AM_DVR_RECORDER_EVENT_IO_ERROR");
   } else if (event == AM_DVR_RECORDER_EVENT_DISK_FULL_ERROR) {
      PVR_DBG("AM_DVR_RECORDER_EVENT_DISK_FULL_ERROR");
   } else {
      PVR_DBG("unknown event: %d",event);
   }
}

static void on_player_evt_cb(am_dvr_player_handle handle, am_dvr_player_event event, void *event_data)
{
   auto itBegin = s_recplay_status;
   auto itEnd = s_recplay_status + MAX_PLAYERS;
   auto pred = [handle](S_RECPLAY_STATUS& rps){return rps.dvr_player_handle == handle;};
   auto it = find_if(itBegin,itEnd,pred);
   if (it == itEnd)
   {
      PVR_ERR("Input player handle %p is invalid",handle);
      return;
   }

   if (event == AM_DVR_PLAYER_EVENT_PROGRESS) {
      am_dvr_playback_progress* evt = (am_dvr_playback_progress*) event_data;
      if (evt != NULL) {
         it->progress = *evt;
         it->state = (U8BIT)evt->state;
         //it->speed = (S16BIT)(100*evt->speed);
         PVR_DBG("AM_DVR_PLAYER_EVENT_PROGRESS: "
               "sessionNumber:%d, state:%d, speed:%.2f, "
               "currTime:%lld, startTime:%lld, endTime:%lld, duration:%lld, "
               "currSegmentId:%d, firstSegmentId:%d, lastSegmentId:%d, numberOfSegments:%d",
               evt->sessionNumber,evt->state,evt->speed,
               evt->currTime,evt->startTime,evt->endTime,evt->duration,
               evt->currSegmentId,evt->firstSegmentId,evt->lastSegmentId,evt->numberOfSegments);
      }
   } else if (event == AM_DVR_PLAYER_EVENT_EOS) {
      PVR_DBG("AM_DVR_PLAYER_EVENT_EOS");
   } else if (event == AM_DVR_PLAYER_EVENT_EDGE_LEAVING) {
      PVR_DBG("AM_DVR_PLAYER_EVENT_EDGE_LEAVING");
   } else if (event == AM_DVR_PLAYER_EVENT_INITIAL_STATE) {
      PVR_DBG("AM_DVR_PLAYER_EVENT_INITIAL_STATE");
      it->state = 1;
   } else if (event == AM_DVR_PLAYER_EVENT_STARTING_STATE) {
      PVR_DBG("AM_DVR_PLAYER_EVENT_STARTING_STATE");
      it->state = 2;
   } else if (event == AM_DVR_PLAYER_EVENT_SMOOTH_PLAYING_STATE) {
      PVR_DBG("AM_DVR_PLAYER_EVENT_SMOOTH_PLAYING_STATE");
      if (it->state == 2) {
         STB_OSSendEvent(FALSE, HW_EV_CLASS_PVR, HW_EV_TYPE_PVR_PLAY_START,&(it->audio_decoder),1);
         PVR_DBG("signal PVR_PLAY_START");
      }
      it->state = 3;
   } else if (event == AM_DVR_PLAYER_EVENT_SKIPPING_PLAYING_STATE) {
      PVR_DBG("AM_DVR_PLAYER_EVENT_SKIPPING_PLAYING_STATE");
      if (it->state == 2) {
         STB_OSSendEvent(FALSE, HW_EV_CLASS_PVR, HW_EV_TYPE_PVR_PLAY_START,&(it->audio_decoder),1);
         PVR_DBG("signal PVR_PLAY_START");
      }
      it->state = 4;
   } else if (event == AM_DVR_PLAYER_EVENT_PAUSED_STATE) {
      PVR_DBG("AM_DVR_PLAYER_EVENT_PAUSED_STATE");
      if (it->state == 2) {
         STB_OSSendEvent(FALSE, HW_EV_CLASS_PVR, HW_EV_TYPE_PVR_PLAY_START,&(it->audio_decoder),1);
         PVR_DBG("signal PVR_PLAY_START");
      }
      it->state = 5;
   } else if (event == AM_DVR_PLAYER_EVENT_STOPPING_STATE) {
      PVR_DBG("AM_DVR_PLAYER_EVENT_STOPPING_STATE");
      STB_OSSendEvent(FALSE, HW_EV_CLASS_PVR, HW_EV_TYPE_PVR_PLAY_STOP,&(it->audio_decoder),1);
      PVR_DBG("signal PVR_PLAY_STOP");
      it->state = 6;
   } else {
      PVR_DBG("unknown event: %d",event);
   }
}

static am_dvr_stream_type type_map1(E_PVR_PID_TYPE type)
{
   am_dvr_stream_type ret;
   switch (type)
   {
      case PVR_PID_TYPE_VIDEO:
         ret = AM_DVR_STREAM_TYPE_VIDEO;
         break;
      case PVR_PID_TYPE_AUDIO:
         ret = AM_DVR_STREAM_TYPE_AUDIO;
         break;
      case PVR_PID_TYPE_SUBTITLES:
         ret = AM_DVR_STREAM_TYPE_SUBTITLE;
         break;
      case PVR_PID_TYPE_SECTION:
         ret = AM_DVR_STREAM_TYPE_OTHER;
         break;
      case PVR_PID_TYPE_TELETEXT:
         ret = AM_DVR_STREAM_TYPE_TELETEXT;
         break;
      default:
         ret = AM_DVR_STREAM_TYPE_OTHER;
   }
   //PVR_DBG("type mapping: %d => %d",type,ret);
   return ret;
}

static int video_codec_map1(E_STB_AV_VIDEO_CODEC format)
{
   switch (format)
   {
      case AV_VIDEO_CODEC_AUTO:
         return 0;
      case AV_VIDEO_CODEC_MPEG1:
         return 2;
      case AV_VIDEO_CODEC_MPEG2:
         return 3;
      case AV_VIDEO_CODEC_H264:
         return 5;
      case AV_VIDEO_CODEC_H265:
         return 6;
      case AV_VIDEO_CODEC_VP9:
         return 9;
      case AV_VIDEO_CODEC_AVS:
         return 11;
      case AV_VIDEO_CODEC_AVS2:
         return 12;
      case AV_VIDEO_CODEC_MPEG4:
         return 4;
      default:
         return 0;
   }
}

static int audio_codec_map1(E_STB_AV_AUDIO_CODEC format)
{
   switch (format)
   {
      case AV_AUDIO_CODEC_AUTO:
         return 0;
      case AV_AUDIO_CODEC_MP2:
         return 4;
      case AV_AUDIO_CODEC_MP3:
         return 2;
      case AV_AUDIO_CODEC_AC3:
         return 7;
      case AV_AUDIO_CODEC_EAC3:
         return 8;
      case AV_AUDIO_CODEC_AAC:
         return 6;
      case AV_AUDIO_CODEC_HEAAC:
         return 0;
      case AV_AUDIO_CODEC_AAC_ADTS:
         return 16;
      case AV_AUDIO_CODEC_HEAACV2:
         return 0;
      case AV_AUDIO_CODEC_AC4:
         return 9;
      default:
         return 0;
   }
}

static U32BIT getPVRConfigInt(const char *config, U32BIT def)
{
    char buf[16]={0};

    if (!STB_Get_Prop(config,buf,sizeof(buf))) {
        return def;
    }

    const long int i = strtol(buf,NULL,0);
    if ((i == LONG_MIN || i == LONG_MAX) && errno == ERANGE) {
        return def;
    }

    return (U32BIT)i;
}

static U8BIT to_index(U8BIT video_decoder, U8BIT audio_decoder)
{
   auto itBegin = s_recplay_status;
   auto itEnd = s_recplay_status + MAX_PLAYERS;
   auto pred = [video_decoder,audio_decoder](S_RECPLAY_STATUS& s){
      bool cond1 = (video_decoder != INVALID_RES_ID && video_decoder == s.video_decoder);
      bool cond2 = (audio_decoder != INVALID_RES_ID && audio_decoder == s.audio_decoder);
      return cond1 || cond2;
   };
   auto it = find_if(itBegin,itEnd,pred);
   return (it != itEnd) ? (U8BIT)distance(itBegin,it) : 255;
}

static ostream& operator<<(ostream& os, const S_PVR_PID_INFO& info)
{
   os << "(" << (int)info.pid << "," << (int)info.type << ")";
   return os;
}

static void get_outstanding_pids(PID_VECTOR& curr, PID_VECTOR& given, PID_VECTOR& to_add, PID_VECTOR& to_remove)
{
   auto pred1 = [](const S_PVR_PID_INFO i1,const S_PVR_PID_INFO i2){return i1.pid < i2.pid;};
   auto pred2 = [](const S_PVR_PID_INFO i1,const S_PVR_PID_INFO i2){return i1.pid == i2.pid;};
   auto pred3 = [](const S_PVR_PID_INFO info){return info.type==PVR_PID_TYPE_PCR;};
   stringstream log_buf;

   sort(curr.begin(),curr.end(),pred1);
   sort(given.begin(),given.end(),pred1);

   log_buf << "curr (pid,type): ";
   copy(curr.begin(),curr.end(),ostream_iterator<S_PVR_PID_INFO>(log_buf,","));
   PVR_INFO("STB_PVR %s",log_buf.str().c_str());
   log_buf.str(""); log_buf.clear();

   log_buf << "given (pid,type): ";
   copy(given.begin(),given.end(),ostream_iterator<S_PVR_PID_INFO>(log_buf,","));
   PVR_INFO("STB_PVR %s",log_buf.str().c_str());
   log_buf.str(""); log_buf.clear();

   auto newEndIt = remove_if(given.begin(),given.end(),pred3);
   given.erase(newEndIt,given.end());

   to_add.clear();
   to_remove.clear();
   set_difference(curr.begin(),curr.end(),given.begin(),given.end(),back_inserter(to_remove),pred1);
   set_difference(given.begin(),given.end(),curr.begin(),curr.end(),back_inserter(to_add),pred1);

   log_buf << "to_add (pid,type): ";
   copy(to_add.begin(),to_add.end(),ostream_iterator<S_PVR_PID_INFO>(log_buf,","));
   PVR_INFO("STB_PVR %s",log_buf.str().c_str());
   log_buf.str(""); log_buf.clear();

   log_buf << "to_remove (pid,type): ";
   copy(to_remove.begin(),to_remove.end(),ostream_iterator<S_PVR_PID_INFO>(log_buf,","));
   PVR_INFO("STB_PVR %s",log_buf.str().c_str());
   log_buf.str(""); log_buf.clear();
}

