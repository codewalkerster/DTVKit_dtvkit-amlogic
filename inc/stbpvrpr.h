/*******************************************************************************
 * Copyright © 2014 The DTVKit Open Software Foundation Ltd (www.dtvkit.org)
 * Copyright © 2004 Ocean Blue Software Ltd
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
 * @file    stbpvrpr.h
 * @brief   Header file - macros and function prototypes for public use
 * @date    07/02/2003
 *
 */

// pre-processor mechanism so multiple inclusions don't cause compilation error

#ifndef _STBPVRPR_H
#define _STBPVRPR_H

#include "techtype.h"
#include "stbhwav.h"
#include "stbhwdmx.h"

//---Constant and macro definitions for public use-----------------------------

//---Enumerations for public use-----------------------------------------------
typedef enum
{
    PVR_PID_TYPE_VIDEO,
    PVR_PID_TYPE_AUDIO,
    PVR_PID_TYPE_PCR,
    PVR_PID_TYPE_SUBTITLES,
    PVR_PID_TYPE_SECTION,
    PVR_PID_TYPE_TELETEXT
} E_PVR_PID_TYPE;

/**
 * @brief   DVR playback vendor info
 * @Please  make sure the struct was same as DVR_PlaybackVendor_t
 * @which   in vendor/amlogic/reference/libdvr/include/dvr_playback.h
 */
typedef enum {
    PVR_PLAYBACK_VENDOR_DEF,            /**< default*/
    PVR_PLAYBACK_VENDOR_AML,            /**< aml*/
    PVR_PLAYBACK_VENDOR_AMAZON          /**< amazon*/
} E_PVR_PLAYBACKVENDOR_TYPE;


//---Global type defs for public use-------------------------------------------

typedef enum e_stb_pvr_start_mode
{
    START_RUNNING = 0,
    START_PAUSED = 1,
    START_AVSYNC = 2,
    START_EXPORT_PES = 3,
    START_IMPORT_TS = 4,
    START_IMPORT_PES = 5
} E_STB_PVR_START_MODE;

typedef enum e_stb_pvr_play_mode
{
    PLAY_NO_TRICK = 0,
    PLAY_TRICK_FORWARDS = 1,
    PLAY_TRICK_REVERSE = 2,
    PLAY_TRICK_FRAME = 3
} E_STB_PVR_PLAY_MODE;

// from e_jdvr_playback_status
typedef enum e_stb_pvr_playback_status
{
    CONTROLLER_STATUS_TO_START = 0x200,
    CONTROLLER_STATUS_TO_PAUSE = 0x201,
    CONTROLLER_STATUS_TO_EXIT = 0x202,
    CONTROLLER_STATUS_TO_SET_SPEED = 0x203,
    CONTROLLER_STATUS_TO_SEEK = 0x204,
    DTVKIT_STATUS_PMT_ACQUIRED = 0x210,
    CAS_STATUS_READY = 0x211,
    RECORDER_STATUS_FILE_MODIFIED = 0x212
} E_STB_PVR_PLAYBACK_STATUS;

typedef struct
{
    BOOLEAN is_smp;
    BOOLEAN is_timeshift;
    UINTPTR cb_param;
    int (*crypto_cb)(void *crypto_inf, void *cb_param);
} S_CAS_STATUS;

typedef struct s_pvr_pid_info
{
    E_PVR_PID_TYPE type;
    U16BIT pid;
    union
    {
        E_STB_AV_VIDEO_CODEC video_codec;
        E_STB_AV_AUDIO_CODEC audio_codec;
    } u;
} S_PVR_PID_INFO;

typedef struct s_notify_time_info
{
    U8BIT audio_codec;
    U32BIT time;
} S_NOTIFY_TIME_INFO;


//---Global Function prototypes for public use---------------------------------
/**
 * @brief   Initialisation for playback
 * @param   num_audio_decoders number of audio decoders available
 * @param   num_video_decoders number of video decoders available
 * @return  Number of players, 0 if unsuccessful or unsupported
 */
U8BIT STB_PVRInitPlayback(U8BIT num_audio_decoders, U8BIT num_video_decoders);

/**
 * @brief   Initialisation for recording
 * @param   num_tuners number of tuners available for recording
 * @return  Number of recorders, 0 if unsuccessful or unsupported
 */
U8BIT STB_PVRInitRecording(U8BIT num_tuners);

/**
 * @brief   Set startup mode for playback
 * @param   audio_decoder audio decoder being used for playback
 * @param   video_decoder video decoder being used for playback
 * @param   mode playback startup mode
 */
void STB_PVRSetPlayStartMode(U8BIT audio_decoder, U8BIT video_decoder, E_STB_PVR_START_MODE mode);

/**
 * @brief   Informs the platform whether there's video in the file to be played.
 *          Should be called before playback is started.
 * @param   audio_decoder audio decoder being used for playback
 * @param   video_decoder video decoder being used for playback
 * @param   has_video TRUE if the recording contains video, FALSE otherwise
 */
void STB_PVRPlayHasVideo(U8BIT audio_decoder, U8BIT video_decoder, BOOLEAN has_video);

/**
 * @brief   Sets the time the next notification event should be sent during playback.
 *          This is required for CI+, but may also be used for other purposes.
 * @param   audio_decoder audio decoder being used for playback
 * @param   video_decoder video decoder being used for playback
 * @param   notify_time time in seconds the next notification event is to be sent
 */
void STB_PVRSetPlaybackNotifyTime(U8BIT audio_decoder, U8BIT video_decoder, U32BIT notify_time);

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
                         U8BIT *basename);

/**
 * @brief   Changes the main audio PID being decoded during playback. This can
 *          be used to switch between main audio and broadcaster mix AD.
 * @param   audio_decoder - audio decoder for playback
 * @param   video_decoder - video decoder for playback
 * @param   pid - new audio PID to decode
 * @param   codec - new audio codec
 * @return  TRUE if the PID is changed successfully, FALSE otherwise
 */
BOOLEAN STB_PVRPlayChangeAudio(U8BIT audio_decoder, U8BIT video_decoder, U16BIT pid, U8BIT codec);

/**
 * @brief   Returns status of playback with the given decoders
 * @param   audio_decoder audio decoder being used for playback
 * @param   video_decoder video decoder being used for playback
 * @return  TRUE if playback is in progress with the given decoders
 */
BOOLEAN STB_PVRIsPlayStarted(U8BIT audio_decoder, U8BIT video_decoder);

/**
 *  * @brief   Returns whether a PVR playback is starting.
 *   * @param   audio_decoder audio decoder being used for playback
 *    * @param   video_decoder video decoder being used for playback
 *     * @return  TRUE if playback is starting with the given decoders
 *      */
BOOLEAN STB_PVRIsPlayStarting(U8BIT audio_decoder, U8BIT video_decoder);

/**
 * @brief   Returns status of playback with the given decoders
 * @param   audio_decoder audio decoder being used for playback
 * @param   video_decoder video decoder being used for playback
 * @return  TRUE if playback is not in progress with the given decoders
 */
BOOLEAN STB_PVRIsPlayStopped(U8BIT audio_decoder, U8BIT video_decoder);

/**
 * @brief   Sets the playback position after playback has started (i.e. jump to bookmark)
 * @param   audio_decoder audio decoder being used for playback
 * @param   video_decoder video decoder being used for playback
 * @param   position_in_seconds position to jump to in the recording in seconds from the beginning
 * @return  TRUE if position is set successfully, FALSE otherwise
 */
BOOLEAN STB_PVRPlaySetPosition(U8BIT audio_decoder, U8BIT video_decoder, U32BIT position_in_seconds);

/**
 * @brief   Stops playback
 * @param   audio_decoder audio decoder being used for playback
 * @param   video_decoder video decoder being used for playback
 */
void STB_PVRPlayStop(U8BIT audio_decoder, U8BIT video_decoder);

/**
 * @brief   Returns whether audio and video playback has been started
 * @param   audio_decoder audio decoder being used for playback
 * @param   video_decoder video decoder being used for playback
 * @param   video returned as TRUE if video is being decoded
 * @param   audio returned as TRUE if audio is being decoded
 */
void STB_PVRPlayEnabled(U8BIT audio_decoder, U8BIT video_decoder, BOOLEAN *video, BOOLEAN *audio);

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
                                  U16BIT rec_date, U8BIT rec_hour, U8BIT rec_min);

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
 *             [1] - size, in MB
 */
void STB_PVRSetRecordStartMode(U8BIT rec_index, E_STB_PVR_START_MODE mode, U32BIT *param);

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
                           U16BIT num_pids, S_PVR_PID_INFO *pid_array);

/**
 * @brief   Pauses a recording currently taking place
 * @param   rec_index recording index
 * @return  TRUE if the recording is successfully paused, FALSE otherwise
 */
BOOLEAN STB_PVRRecordPause(U8BIT rec_index);

/**
 * @brief   Resumes a paused recording
 * @param   rec_index recording index
 * @return  TRUE if the recording is successfully resumed, FALSE otherwise
 */
BOOLEAN STB_PVRRecordResume(U8BIT rec_index);

/**
 * @brief   Changes the record descramble mode while recording
 * @param   rec_index current recording index  to be updated
 * @param   mode 1:descramble or 0:free
 * @return  TRUE if the mode have been successfully changed, FALSE otherwise
 */
BOOLEAN STB_PVRRecordChangeDesMode(U8BIT rec_index, int mode);
/**
 * @brief   Changes the PIDs while recording
 * @param   rec_index current recording index  to be updated
 * @param   num_pids number of PIDs in PID array
 * @param   pid_array new PID list to be recorded
 * @return  TRUE if the PIDs have been successfully changed, FALSE otherwise
 */
BOOLEAN STB_PVRRecordChangePids(U8BIT rec_index, U16BIT num_pids, S_PVR_PID_INFO *pids_array);

/**
 * @brief   Stops a recording
 * @param   rec_index recording index
 */
void STB_PVRRecordStop(U8BIT rec_index);

/**
 * @brief   Returns whether recording has been started
 * @param   rec_index recording index being queried
 * @return  TRUE if recording has been started
 */
BOOLEAN STB_PVRIsRecordStarted(U8BIT rec_index);

/**
 * @brief   Returns status of audio/video recording
 * @param   rec_index recording index being used for recording
 * @param   video pointer to a boolean value that indicates whether the video data is being recorded
 * @param   audio pointer to a boolean value that indicates whether the audio data is being recorded
 */
void STB_PVRRecordEnabled(U8BIT rec_index, BOOLEAN *video, BOOLEAN *audio);

/**
 * @brief   Enables or disables encryption and sets the encryption key to be used
 * @param   rec_index recording index to be set
 * @param   state whether encryption is enabled of disabled
 * @param   key encryption key, ignored if state is FALSE
 * @param   iv initialisation vector, ignored if state is FALSE
 * @param   key_len length of encryption key, ignored if state is FALSE
 */
void STB_PVRSetRecordEncryptionKey(U8BIT rec_index, BOOLEAN state, U8BIT *key, U8BIT *iv, U32BIT key_len);

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
                                     U8BIT *key, U8BIT *iv, U32BIT key_len);

/**
 * @brief   Sets trick mode during playback
 * @param   audio_decoder audio decoder being used for playback
 * @param   video_decoder video decoder being used for playback
 * @param   mode trick mode to be used
 * @param   speed playback speed to be used as a percentage (100% = normal playback)
 */
void STB_PVRPlayTrickMode(U8BIT audio_decoder, U8BIT video_decoder, E_STB_PVR_PLAY_MODE mode, S16BIT speed);

/**
 * @brief   Save current frame of playback
 * @param   audio_decoder audio decoder being used for playback
 * @param   video_decoder video decoder being used for playback
 */
void STB_PVRSaveFrame(U8BIT audio_decoder, U8BIT video_decoder);

/**
 * @brief   Returns the current playback speed
 * @param   audio_decoder audio decoder being used for playback
 * @param   video_decoder video decoder being used for playback
 * @return  current playback speed as a percentage
 */
S16BIT STB_PVRGetPlaySpeed(U8BIT audio_decoder, U8BIT video_decoder);

/**
 * @brief   Set the play speed for the specified decoder
 * @param   audio_decoder audio decoder being used for playback
 * @param   video_decoder video decoder being used for playback
 * @param   speed Play speed as a percentage (i.e 100% = normal playback)
 * @return  TRUE if successful
 */
BOOLEAN STB_PVRSetPlaySpeed(U8BIT audio_decoder, U8BIT video_decoder, S16BIT speed);

/**
 * @brief   Checks whether any of the files already exist that would be created
 *          by a recording with the given base filename.
 * @param   disk_id disk to be checked
 * @param   basename base filename to be used for a recording
 * @return  TRUE if none of the files exist, FALSE otherwise
 */
BOOLEAN STB_PVRIsValidRecording(U16BIT disk_id, U8BIT *basename);

/**
 * @brief   Checks whether any of the files already exist that would be created
 *          by a recording with the given base filename.
 * @param   disk_id disk to be checked
 * @param   basename base filename to be used for a recording
 * @return  TRUE if none of the files exist, FALSE otherwise
 */
BOOLEAN STB_PVRCanBeUsedForRecording(U16BIT disk_id, U8BIT *basename);

/**
 * @brief   Deletes any files associated with the given base filename that were created
 *          as a result of the recording being performed.
 * @param   disk_id disk containing the recording to be deleted
 * @param   basename base filename used for the recording
 * @return  TRUE if successful, FALSE otherwise
 */
BOOLEAN STB_PVRDeleteRecording(U16BIT disk_id, U8BIT *basename);

/**
 * @brief   Returns the size in kilobytes of the recording defined by the given base filename.
 * @param   disk_id disk containing the recording to be queried
 * @param   basename base filename of recording to get info about
 * @param   rec_size_kb returned size of recording in kilobytes
 * @return  TRUE if the information is successfully gathered
 */
BOOLEAN STB_PVRGetRecordingSize(U16BIT disk_id, U8BIT *basename, U32BIT *rec_size_kb);

/**
 * @brief   Returns the elapsed playback time in hours, mins & secs
 * @param   audio_decoder audio decoder being used for playback
 * @param   video_decoder video decoder being used for playback
 * @param   elapsed_hours current number of hours into the playback
 * @param   elapsed_mins current number of minutes into the playback
 * @param   elapsed_secs current number of seconds into the playback
 * @param   elapsed_ms current number of ms into the playback
 * @return  TRUE if the info has been successfully gathered
 */
BOOLEAN STB_PVRGetElapsedTime(U8BIT audio_decoder, U8BIT video_decoder, U16BIT *elapsed_hours,
                              U8BIT *elapsed_mins, U8BIT *elapsed_secs, U16BIT *elapsed_ms);

/**
 * @brief   Acquires an index to be used to reference a recording
 * @param   tuner tuner to be used for the recording
 * @param   demux demux to be used for the recording
 * @return  recording index, 255 if none available
 */
U8BIT STB_PVRAcquireRecorderIndex(U8BIT tuner, U8BIT demux);

/**
 * @brief   Releases a recording index when no longer needed
 * @param   rec_index recoding index
 */
void STB_PVRReleaseRecorderIndex(U8BIT rec_index);

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
                                   E_STB_DMX_DESC_KEY_PARITY parity, U8BIT *key, U8BIT *iv, U16BIT num_pids, S_PVR_PID_INFO *pid_array);

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
                         U16BIT *pcr_pid, U16BIT *video_pid, U16BIT *audio_pid, U16BIT *ad_pid);

/**
 * @brief   Changes the packet IDs for the PCR Video, Audio, Text and Data
 * @param   audio_decoder decoder id of the pvr audio
 * @param   video_decoder decoder id of the pvr video
 * @param   pcr_pid The PID to use for the Program Clock Reference
 * @param   video_pid The PID to use for the Video PES
 * @param   audio_pid The PID to use for the Audio PES
 * @param   ad_pid The PID to use for the AD PES
 * @return  TRUE if successful, FALSE otherwise
 */
BOOLEAN PVRChangeDecodePIDs(U8BIT audio_decoder, U8BIT video_decoder,
                            U16BIT pcr_pid, U16BIT video_pid, U16BIT audio_pid, U16BIT ad_pid,
                            U32BIT video_fmt, U32BIT audio_fmt, U32BIT ad_fmt, U16BIT audio_presentation_id);

/**
 * @brief   PVR will not start if less than this minimum free space
 * @return  minimum free space in KB
 */
U32BIT STB_PVRGetMinDiskSpace();

/**
 * @brief   PVR will stop if less than this minimum free space(default:10MB)
 * @return  minimum free space in KB
 */
U32BIT STB_PVRGetMinDiskSpaceLeft();

void STB_PVRCheckDiskSpace(void);

BOOLEAN STB_PVRGetPlayerHandle(U8BIT audio_decoder, U8BIT video_decoder, void **p_handle);

/**
 * @brief   Returns the length in time of the recording
 * @param   rec_index recording index to be set
 * @param   secs returned length of recording in seconds
 * @param   secs_truncated returned truncated length of recording in seconds
 * @return  TRUE if the information is successfully gathered
 */
BOOLEAN STB_PVRGetRecordingLengthTruncated(U8BIT rec_index, U32BIT *secs, U32BIT *secs_truncated);

/**
 * @brief   Store libdvr specific information i.e. force_sysclock in
            s_rec_status of porting layer
 * @param   rec_index recorder index
 * @param   val value of force_sysclock.
            0: determine index time source based on actual situation
            1: force to use system clock as PVR index time source
 * @return  TRUE if store successfully, FALSE if invalid rec_index is given.
 */
BOOLEAN STB_PVRStoreLibdvrExtParam1InPortingLayer(U8BIT rec_index, U8BIT val);

/**
 * @brief   Returns the length in ms and the size in KB of the recording
 * @param   disk_id disk containing the recording to be queried
 * @param   basename base filename of recording to get info about
 * @param   secs returned length of recording in seconds
 * @param   rec_size_kb returned size of recording in kilobytes
 * @return  TRUE if the information is successfully gathered
 */
BOOLEAN STB_PVRGetRecordingLength(U16BIT disk_id, U8BIT *basename, U32BIT *rec_length_ms, U32BIT *rec_size_kb);

int STB_PVRRecord_Encrypt(void *crypto_inf, void *cb_param);

int STB_PVRPlay_Decrypt(void *crypto_inf, void *cb_param);

BOOLEAN STB_PVRStartAVDecoding(U8BIT index);
BOOLEAN STB_PVRIsPlayInitialled(U8BIT audio_decoder, U8BIT video_decoder);
U8BIT STB_PVRGetPlayPath(U8BIT audio_decoder, U8BIT video_decoder);

#ifdef SUPPORT_CAS

/**
 * @brief   Sets the cas status for a pvr play. This function should be called
 *          before the timeshift is started and is used to when pausing live TV.
 * @param   audio_decoder audio decoder being used for playback
 * @param   video_decoder video decoder being used for playback
 * @param   cas_status cas status
 */
void STB_PVRPlaySetCASStatus(U8BIT audio_decoder, U8BIT video_decoder, S_CAS_STATUS *cas_status);

/**
 * @brief   Sets the cas status for a recording. This function should be called
 *          before the recording is started and is used to when pausing live TV.
 * @param   rec_index recording index to be used for the recording
 * @param   cas_status cas status
 */
void STB_PVRRecordSetCASStatus(U8BIT rec_index, S_CAS_STATUS *cas_status);

#endif

/**
 * @brief   Increase and return the current valid serial according to the serialized filename schema under this disk path
 * @param   disk_id disk on which the serial belongs to
 * @return  the next valid serial
 */
U32BIT STB_DSKIncreaseCurrentFileNameSerial(U16BIT disk_id);

#ifdef RDK_COMPILE
/**
 * @brief   Store PVR video rectangle for future reference.
 *          At the moment Player.setRectangle is called, TsPlayer instance for PVR playback
 *          has not been created, so the rectangle need to be stored somewhere for future
 *          Player.play reference.
 * @param   video_decoder   decoder index
 * @param   x   rectangle left
 * @param   y   rectangle top
 * @param   w   rectangle width
 * @param   h   rectangle height
 * @return  TRUE if operation succeeds, FALSE if invalid video_decoder is given.
 */
BOOLEAN STB_PVRStoreVideoWindow(U8BIT video_decoder, U16BIT x, U16BIT y, U16BIT w, U16BIT h);
#endif

/**
 * @brief   The STB_PVRSendMessage function in STB_PVR API allows DTVKit
 *          modules to send a specified message to STB_PVR.
 * @param   path    The data structure handle associated with PVR recording
 *                  and playback process.
 * @param   msg     The message to be sent.
 *           =0x110 CAS metadata for PVR.
 *                  param1: the char* pointer for metadata string which is
 *                    0 terminated. The caller is responsible for maintaining
 *                    the lifecycle of the memory.
 *                  param2: the length of the metadata string.
 * @param   param1  Message-specific information. Data associated with the
 *                  message.
 * @param   param2  Additional message-specific information.
 * @return  The result of message processing depends on the specific message
 *          sent.
 */
S32BIT STB_PVRSendMessage(U8BIT path, U16BIT msg, U64BIT param1, U64BIT param2);

#endif //  _STBPVRPR_H
