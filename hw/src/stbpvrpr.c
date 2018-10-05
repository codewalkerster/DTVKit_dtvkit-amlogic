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

// third party header files

// Ocean Blue header files
#include "techtype.h"
#include "dbgfuncs.h"
#include "stbpvrpr.h"

//---constant definitions for this file----------------------------------------
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

//---local (static) variable declarations for this file------------------------
//   (internal variables declared static to make them local)

//---local function prototypes for this file-----------------------------------
//   (internal functions declared static to make them local)


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
   USE_UNWANTED_PARAM(num_video_decoders);

   FUNCTION_FINISH(STB_PVRInitPlayback);

   return(0);
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
   FUNCTION_FINISH(STB_PVRInitRecording);

   return(0);
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
   USE_UNWANTED_PARAM(video_decoder);
   USE_UNWANTED_PARAM(mode);

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
   FUNCTION_START(STB_PVRPlayStart);
   USE_UNWANTED_PARAM(disk_id);
   USE_UNWANTED_PARAM(audio_decoder);
   USE_UNWANTED_PARAM(video_decoder);
   USE_UNWANTED_PARAM(demux);
   USE_UNWANTED_PARAM(basename);
   FUNCTION_FINISH(STB_PVRPlayStart);

   return(FALSE);
}

/**
 * @brief   Returns status of playback with the given decoders
 * @param   audio_decoder audio decoder being used for playback
 * @param   video_decoder video decoder being used for playback
 * @return  TRUE if playback is in progress with the given decoders
 */
BOOLEAN STB_PVRIsPlayStarted(U8BIT audio_decoder, U8BIT video_decoder)
{
   FUNCTION_START(STB_PVRIsPlayStarted);
   USE_UNWANTED_PARAM(audio_decoder);
   USE_UNWANTED_PARAM(video_decoder);
   FUNCTION_FINISH(STB_PVRIsPlayStarted);

   return(FALSE);
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
   FUNCTION_START(STB_PVRPlaySetPosition);
   USE_UNWANTED_PARAM(audio_decoder);
   USE_UNWANTED_PARAM(video_decoder);
   USE_UNWANTED_PARAM(position_in_seconds);
   FUNCTION_FINISH(STB_PVRPlaySetPosition);

   return(FALSE);
}

/**
 * @brief   Stops playback
 * @param   audio_decoder audio decoder being used for playback
 * @param   video_decoder video decoder being used for playback
 */
void STB_PVRPlayStop(U8BIT audio_decoder, U8BIT video_decoder)
{
   FUNCTION_START(STB_PVRPlayStop);
   USE_UNWANTED_PARAM(audio_decoder);
   USE_UNWANTED_PARAM(video_decoder);
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
   USE_UNWANTED_PARAM(video_decoder);

   *video = FALSE;
   *audio = FALSE;

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
   FUNCTION_START(STB_PVRAcquireRecorderIndex);
   USE_UNWANTED_PARAM(tuner);
   USE_UNWANTED_PARAM(demux);
   FUNCTION_FINISH(STB_PVRAcquireRecorderIndex);

   return(255);
}

/**
 * @brief   Releases a recording index when no longer needed
 * @param   rec_index recoding index
 */
void STB_PVRReleaseRecorderIndex(U8BIT rec_index)
{
   FUNCTION_START(STB_PVRReleaseRecorderIndex);
   USE_UNWANTED_PARAM(rec_index);
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
   FUNCTION_START(STB_PVRRecordStart);
   USE_UNWANTED_PARAM(disk_id);
   USE_UNWANTED_PARAM(rec_index);
   USE_UNWANTED_PARAM(basename);
   USE_UNWANTED_PARAM(num_pids);
   USE_UNWANTED_PARAM(pid_array);
   FUNCTION_FINISH(STB_PVRRecordStart);

   return(FALSE);
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

   return(FALSE);
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

   return(FALSE);
}

/**
 * @brief   Stops a recording
 * @param   rec_index recording index
 */
void STB_PVRRecordStop(U8BIT rec_index)
{
   FUNCTION_START(STB_PVRRecordStop);
   USE_UNWANTED_PARAM(rec_index);
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
   USE_UNWANTED_PARAM(rec_index);
   USE_UNWANTED_PARAM(mode);
   USE_UNWANTED_PARAM(param);
   FUNCTION_FINISH(STB_PVRSetRecordStartMode);
}

/**
 * @brief   Returns whether recording has been started
 * @param   rec_index recording index being queried
 * @return  TRUE if recording has been started
 */
BOOLEAN STB_PVRIsRecordStarted(U8BIT rec_index)
{
   FUNCTION_START(STB_PVRIsRecordStarted);
   USE_UNWANTED_PARAM(rec_index);
   FUNCTION_FINISH(STB_PVRIsRecordStarted);

   return(FALSE);
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

   USE_UNWANTED_PARAM(rec_index);
   *video = FALSE;
   *audio = FALSE;

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
   FUNCTION_START(STB_PVRSetPlaySpeed);
   USE_UNWANTED_PARAM(audio_decoder);
   USE_UNWANTED_PARAM(video_decoder);
   USE_UNWANTED_PARAM(speed);
   FUNCTION_FINISH(STB_PVRSetPlaySpeed);

   return(FALSE);
}

/**
 * @brief   Returns the current playback speed
 * @param   audio_decoder audio decoder being used for playback
 * @param   video_decoder video decoder being used for playback
 * @return  current playback speed as a percentage
 */
S16BIT STB_PVRGetPlaySpeed(U8BIT audio_decoder, U8BIT video_decoder)
{
   FUNCTION_START(STB_PVRGetPlaySpeed);
   USE_UNWANTED_PARAM(audio_decoder);
   USE_UNWANTED_PARAM(video_decoder);
   FUNCTION_FINISH(STB_PVRGetPlaySpeed);

   return(0);
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
   FUNCTION_FINISH(STB_PVRCanBeUsedForRecording);

   return(FALSE);
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
   FUNCTION_START(STB_PVRGetRecordingInfo);

   USE_UNWANTED_PARAM(disk_id);
   USE_UNWANTED_PARAM(basename);

   *rec_size_kb = 0;

   FUNCTION_FINISH(STB_PVRGetRecordingInfo);

   return(FALSE);
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
   FUNCTION_START(STB_PVRGetElapsedTime);

   USE_UNWANTED_PARAM(audio_decoder);
   USE_UNWANTED_PARAM(video_decoder);

   *elapsed_hours = 0;
   *elapsed_mins = 0;
   *elapsed_secs = 0;

   FUNCTION_FINISH(STB_PVRGetElapsedTime);

   return(FALSE);
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
