#ifndef _TF_PVR_H
#define _TF_PVR_H

#include "JDvrLib.h"
#include "JNIASPlayer.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    am_dvr_file_handle jdvrfile_handle;
    on_recorder_event_callback callback;
    uint8_t is_timeshift;
    int   recorder_buffer_size;   // in bytes, 0 for using default value
    int   filter_buffer_size;     // in bytes, 0 for using default value
    int   segment_size;           // in bytes, 0 for using default value
    bool  encrypting;
    int   enc_mode;       // 1: "r2r" mode, 2: "tse" mode
    int   block_size;     // for "r2r" mode only
} wrapper_recorder_init_params;

typedef struct {
    jni_asplayer_handle asplayer_handle;
    am_dvr_file_handle jdvrfile_handle;
    on_player_event_callback callback;
    uint8_t path;
} wrapper_player_init_params;

int8_t Wrapper_PVR_Initialise(int32_t mode);
int8_t Wrapper_PVR_File_create1(const uint8_t* path_prefix, uint8_t trunc, am_dvr_file_handle* phandle);
int8_t Wrapper_PVR_File_create2(const uint8_t* path_prefix, int32_t limit_size, int32_t limit_seconds, uint8_t trunc, am_dvr_file_handle* phandle);
int8_t Wrapper_PVR_File_create3(const uint8_t* path_prefix, am_dvr_file_handle* phandle);
int8_t Wrapper_PVR_File_duration(am_dvr_file_handle handle, int64_t* pduration);
int8_t Wrapper_PVR_File_duration2(const uint8_t* path_prefix, int64_t* pduration);
int8_t Wrapper_PVR_File_size(am_dvr_file_handle handle, int64_t* psize);
int8_t Wrapper_PVR_File_size2(const uint8_t* path_prefix, int64_t* psize);
int8_t Wrapper_PVR_File_destroy(am_dvr_file_handle handle);
int8_t Wrapper_PVR_File_getVideoPID(am_dvr_file_handle handle, int32_t* pPID);
int8_t Wrapper_PVR_File_getAudioPID(am_dvr_file_handle handle, int32_t* pPID);
int8_t Wrapper_PVR_File_getVideoFormat(am_dvr_file_handle handle, int* pformat);
int8_t Wrapper_PVR_File_getAudioFormat(am_dvr_file_handle handle, int* pformat);
int8_t Wrapper_PVR_File_isEncrypted(am_dvr_file_handle handle, uint8_t* pencrypted);

int8_t Wrapper_PVR_deleteRecord(const uint8_t* path_prefix);

int8_t Wrapper_PVR_Recorder_create(wrapper_recorder_init_params* params, am_dvr_recorder_handle* phandle);
int8_t Wrapper_PVR_Recorder_addStream(am_dvr_recorder_handle handle, int32_t pid, am_dvr_stream_type type, int32_t format);
int8_t Wrapper_PVR_Recorder_removeStream(am_dvr_recorder_handle handle, int32_t pid);
int8_t Wrapper_PVR_Recorder_start(am_dvr_recorder_handle handle);
int8_t Wrapper_PVR_Recorder_stop(am_dvr_recorder_handle handle);
int8_t Wrapper_PVR_Recorder_pause(am_dvr_recorder_handle handle);
int8_t Wrapper_PVR_Recorder_sendMessage(am_dvr_recorder_handle handle, uint16_t msg, uint64_t param1, uint64_t param2);

int8_t Wrapper_PVR_Player_create(wrapper_player_init_params* params, am_dvr_player_handle* phandle);
int8_t Wrapper_PVR_Player_play(am_dvr_player_handle handle);
int8_t Wrapper_PVR_Player_pause(am_dvr_player_handle handle);
int8_t Wrapper_PVR_Player_stop(am_dvr_player_handle handle);
int8_t Wrapper_PVR_Player_setSpeed(am_dvr_player_handle handle, double speed);
int8_t Wrapper_PVR_Player_seek(am_dvr_player_handle handle, int32_t seconds);
int8_t Wrapper_PVR_Player_sendMessage(am_dvr_recorder_handle handle, uint16_t msg, uint64_t param1, uint64_t param2);

#ifdef __cplusplus
}
#endif

#endif

