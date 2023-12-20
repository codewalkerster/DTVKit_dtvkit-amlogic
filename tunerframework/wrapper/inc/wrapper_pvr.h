#ifndef _TF_PVR_H
#define _TF_PVR_H

#include "techtype.h"
#include "JDvrLib.h"
#include "JNIASPlayer.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    am_dvr_file_handle jdvrfile_handle;
    U32BIT segment_size;
    on_recorder_event_callback callback;
    BOOLEAN is_timeshift;
} wrapper_recorder_init_params;

typedef struct {
    jni_asplayer_handle asplayer_handle;
    am_dvr_file_handle jdvrfile_handle;
    on_player_event_callback callback;
    U8BIT path;
} wrapper_player_init_params;

S8BIT Wrapper_PVR_Initialise(int32_t mode);
S8BIT Wrapper_PVR_File_create1(const PU8BIT path_prefix, BOOLEAN trunc, am_dvr_file_handle* phandle);
S8BIT Wrapper_PVR_File_create2(const PU8BIT path_prefix, S32BIT limit_size, S32BIT limit_seconds, BOOLEAN trunc, am_dvr_file_handle* phandle);
S8BIT Wrapper_PVR_File_create3(const PU8BIT path_prefix, am_dvr_file_handle* phandle);
S8BIT Wrapper_PVR_File_duration(am_dvr_file_handle handle, int64_t* pduration);
S8BIT Wrapper_PVR_File_duration2(const PU8BIT path_prefix, int64_t* pduration);
S8BIT Wrapper_PVR_File_size(am_dvr_file_handle handle, int64_t* psize);
S8BIT Wrapper_PVR_File_size2(const PU8BIT path_prefix, int64_t* psize);
S8BIT Wrapper_PVR_File_destroy(am_dvr_file_handle handle);
S8BIT Wrapper_PVR_File_getVideoPID(am_dvr_file_handle handle, int32_t* pPID);
S8BIT Wrapper_PVR_File_getAudioPID(am_dvr_file_handle handle, int32_t* pPID);
S8BIT Wrapper_PVR_File_getVideoFormat(am_dvr_file_handle handle, int* pformat);
S8BIT Wrapper_PVR_File_getAudioFormat(am_dvr_file_handle handle, int* pformat);

S8BIT Wrapper_PVR_deleteRecord(const PU8BIT path_prefix);

S8BIT Wrapper_PVR_Recorder_create(wrapper_recorder_init_params* params, am_dvr_recorder_handle* phandle);
S8BIT Wrapper_PVR_Recorder_addStream(am_dvr_recorder_handle handle, S32BIT pid, am_dvr_stream_type type, S32BIT format);
S8BIT Wrapper_PVR_Recorder_removeStream(am_dvr_recorder_handle handle, S32BIT pid);
S8BIT Wrapper_PVR_Recorder_start(am_dvr_recorder_handle handle);
S8BIT Wrapper_PVR_Recorder_stop(am_dvr_recorder_handle handle);
S8BIT Wrapper_PVR_Recorder_pause(am_dvr_recorder_handle handle);

S8BIT Wrapper_PVR_Player_create(wrapper_player_init_params* params, am_dvr_player_handle* phandle);
S8BIT Wrapper_PVR_Player_play(am_dvr_player_handle handle);
S8BIT Wrapper_PVR_Player_pause(am_dvr_player_handle handle);
S8BIT Wrapper_PVR_Player_stop(am_dvr_player_handle handle);
S8BIT Wrapper_PVR_Player_setSpeed(am_dvr_player_handle handle, double speed);
S8BIT Wrapper_PVR_Player_seek(am_dvr_player_handle handle, int32_t seconds);

#ifdef __cplusplus
}
#endif

#endif

