/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef JNI_ASPLAYER_H
#define JNI_ASPLAYER_H

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t         bool_t;
/*Call back event type*/
typedef enum {
    JNI_ASPLAYER_EVENT_TYPE_PTS = 0,        // Pts in for some stream
    JNI_ASPLAYER_EVENT_TYPE_USERDATA_AFD,   // User data (afd)
    JNI_ASPLAYER_EVENT_TYPE_USERDATA_CC,    // User data (cc)
    JNI_ASPLAYER_EVENT_TYPE_VIDEO_CHANGED,  // Video format changed
    JNI_ASPLAYER_EVENT_TYPE_AUDIO_CHANGED,  // Audio format changed
    JNI_ASPLAYER_EVENT_TYPE_DATA_LOSS,      // Demod data loss
    JNI_ASPLAYER_EVENT_TYPE_DATA_RESUME,    // Demod data resume
    JNI_ASPLAYER_EVENT_TYPE_DECODE_FIRST_FRAME_VIDEO, //The video decoder outputs the first frame
    JNI_ASPLAYER_EVENT_TYPE_DECODE_FIRST_FRAME_AUDIO, //The audio decoder outputs the first frame
    JNI_ASPLAYER_EVENT_TYPE_RENDER_FIRST_FRAME_VIDEO, //The video decoder render the first frame
    JNI_ASPLAYER_EVENT_TYPE_RENDER_FIRST_FRAME_AUDIO, //The audio decoder render the first frame
    JNI_ASPLAYER_EVENT_TYPE_AV_SYNC_DONE     //Av sync done
} jni_asplayer_event_type;


typedef enum {
    JNI_ASPLAYER_KEY_AUDIO_PRESENTATION_ID = 0,
    JNI_ASPLAYER_KEY_VIDEO_SECLEVEL,
    JNI_ASPLAYER_KEY_SET_AUDIO_PATCH_MANAGE_MODE,
    JNI_ASPLAYER_KEY_AUDIO_SECLEVEL,
    JNI_ASPLAYER_KEY_SET_SPDIF_STATUS,
    JNI_ASPLAYER_KEY_SET_VIDEO_RECOVERY_MODE,
    JNI_ASPLAYER_KEY_SET_OSD,
    JNI_ASPLAYER_KEY_SET_LOGGER_LEVEL,
    JNI_ASPLAYER_KEY_SET_WMA_DESCR,
    JNI_ASPLAYER_KEY_SET_ES_AUDIO_EXTRA_PARAM,
    JNI_ASPLAYER_KEY_SET_STREAM_EOF,
    JNI_ASPLAYER_KEY_BOOTPLAY_MODE,
    JNI_ASPLAYER_KEY_ENABLE_VFRAME_COUNTER,
    JNI_ASPLAYER_KEY_SET_AUDIO_LANG,
} jni_asplayer_parameter;

typedef enum
{
    JNI_ASPLAYER_AUDIO_PATCH_MANAGE_AUTO = -1,
    JNI_ASPLAYER_AUDIO_PATCH_MANAGE_FORCE_DISABLE,
    JNI_ASPLAYER_AUDIO_PATCH_MANAGE_FORCE_ENABLE,
} jni_asplayer_audio_patch_manage_mode;

typedef enum {
    JNI_ASPLAYER_KEY_VIDEO_STATE = 0,
} jni_asplayer_state_type;

typedef enum {
    JNI_ASPLAYER_KEY_SPDIF_MODE_NONE  = 0,
    JNI_ASPLAYER_KEY_SPDIF_MODE_NEVER = 1,
    JNI_ASPLAYER_KEY_SPDIF_MODE_ONCE  = 2,
} jni_asplayer_spdif_mode;

typedef enum {
    JNI_ASPLAYER_AV_INFO  = 0,              // Get audio and video information
    JNI_ASPLAYER_AUDIO_INFO = 1,            // Get audio information only
    JNI_ASPLAYER_VIDEO_INFO  = 2,           // Get video information only
    JNI_ASPLAYER_VFRAME_COUNTER_INFO  = 3,  // Get video frame counter info only
} jni_asplayer_av_info_state;

typedef enum {
    JNI_ASPLAYER_EXTENDED_BOOTPLAY_MODE = 0,        //BootPlay uses videotunnel and software audio decoder lib
} jni_asplayer_extended_setup;

typedef struct {
    uint8_t *data;      // Call to provide buffer pointer
    size_t data_len;    // The length of the buffer
    size_t actual_len;  // Copy the length of the actual json
    jni_asplayer_av_info_state av_flag;  // Information acquisition flags for audio and video
} jni_asplayer_state_t;


/*Callback event mask*/
#define JNI_ASPLAYER_EVENT_TYPE_PTS_MASK            (1 << 0)
#define JNI_ASPLAYER_EVENT_TYPE_USERDATA_AFD_MASK   (1 << 1)
#define JNI_ASPLAYER_EVENT_TYPE_USERDATA_CC_MASK    (1 << 2)
#define JNI_ASPLAYER_EVENT_TYPE_DATA_LOSS_MASK      (1 << 3)
#define JNI_ASPLAYER_EVENT_TYPE_DATA_RESUME_MASK    (1 << 4)

/*Secure level which should be consistent with definition of dmx.h*/
#define JNI_ASPLAYER_DMX_FILTER_SEC_LEVEL1   (1 << 10)
#define JNI_ASPLAYER_DMX_FILTER_SEC_LEVEL2   (2 << 10)
#define JNI_ASPLAYER_DMX_FITLER_SEC_LEVEL3   (3 << 10)
#define JNI_ASPLAYER_DMX_FILTER_SEC_LEVEL4   (4 << 10)
#define JNI_ASPLAYER_DMX_FILTER_SEC_LEVEL5   (5 << 10)
#define JNI_ASPLAYER_DMX_FITLER_SEC_LEVEL6   (6 << 10)
#define JNI_ASPLAYER_DMX_FITLER_SEC_LEVEL7   (7 << 10)

/*JniASPlayer extended setup mask*/
#define JNI_ASPLAYER_EXTENDED_BOOTPLAY_MODE_MASK (1 << JNI_ASPLAYER_EXTENDED_BOOTPLAY_MODE)

/*Function return type*/
typedef enum {
    JNI_ASPLAYER_OK  = 0,                      // OK
    JNI_ASPLAYER_ERROR_INVALID_PARAMS = -1,    // Parameters invalid
    JNI_ASPLAYER_ERROR_INVALID_OPERATION = -2, // Operation invalid
    JNI_ASPLAYER_ERROR_INVALID_OBJECT = -3,    // Object invalid
    JNI_ASPLAYER_ERROR_RETRY = -4,             // Retry
    JNI_ASPLAYER_ERROR_BUSY = -5,              // Device busy
    JNI_ASPLAYER_ERROR_END_OF_STREAM = -6,     // End of stream
    JNI_ASPLAYER_ERROR_IO            = -7,     // Io error
    JNI_ASPLAYER_ERROR_WOULD_BLOCK   = -8,     // Blocking error
    JNI_ASPLAYER_ERROR_MAX = -254
} jni_asplayer_result;

/** Playback mode */
typedef enum
{
    JNI_ASPLAYER_PLAYBACK_MODE_PASSTHROUGH   = 0,    // Passthrough mode
    JNI_ASPLAYER_PLAYBACK_MODE_ES_MODE       = 1,    // ES mode
} jni_asplayer_playback_mode;

/*Data input source type*/
typedef enum
{
    JNI_ASPLAYER_TS_DEMOD = 0,                          // TS Data input from demod
    JNI_ASPLAYER_TS_MEMORY = 1,                         // TS Data input from memory
    JNI_ASPLAYER_ES_MEMORY = 2,                         // ES Data input from memory
} jni_asplayer_input_source_type;

/*Ts stream type*/
typedef enum {
    JNI_ASPLAYER_TS_STREAM_VIDEO = 0,                   // Video
    JNI_ASPLAYER_TS_STREAM_AUDIO = 1,                   // Audio
    JNI_ASPLAYER_TS_STREAM_AD = 2,                      // Audio description
    JNI_ASPLAYER_TS_STREAM_SUB = 3,                     // Subtitle
} jni_asplayer_stream_type;

/*Avsync mode*/
typedef enum {
    JNI_ASPLAYER_TS_SYNC_VMASTER = 0,                   // Video Master
    JNI_ASPLAYER_TS_SYNC_AMASTER = 1,                   // Audio Master
    JNI_ASPLAYER_TS_SYNC_PCRMASTER = 2,                 // PCR Master
    JNI_ASPLAYER_TS_SYNC_NOSYNC = 3                     // Free run
} jni_asplayer_avsync_mode;

/*Player working mode*/
typedef enum {
    JNI_ASPLAYER_WORK_MODE_NORMAL = 0,             // Normal mode
    JNI_ASPLAYER_WORK_MODE_CACHING_ONLY = 1,       // Only caching data, do not decode. Used in FCC
    JNI_ASPLAYER_WORK_MODE_DECODE_ONLY = 2         // Decode data but do not output
} jni_asplayer_work_mode;

/*Player PIP mode*/
typedef enum {
    JNI_ASPLAYER_PIP_MODE_NORMAL = 0,           // Normal mode
    JNI_ASPLAYER_PIP_MODE_PIP = 1               // PIP mode
} jni_asplayer_pip_mode;

/*Audio stereo output mode*/
typedef enum {
    JNI_ASPLAYER_AV_AUDIO_STEREO = 0,                   // Stereo mode
    JNI_ASPLAYER_AV_AUDIO_LEFT = 1,                     // Output left channel
    JNI_ASPLAYER_AV_AUDIO_RIGHT = 2,                    // Output right channel
    JNI_ASPLAYER_AV_AUDIO_SWAP = 3,                     // Swap left and right channels
    JNI_ASPLAYER_AV_AUDIO_LRMIX = 4                     // Mix left and right channels
} jni_asplayer_audio_stereo_mode;

/*Video decoder trick mode*/
typedef enum {
    JNI_ASPLAYER_AV_VIDEO_TRICK_MODE_NONE = 0,          // Disable trick mode
    JNI_ASPLAYER_AV_VIDEO_TRICK_MODE_SMOOTH = 1,        // Smooth trick mode
    JNI_ASPLAYER_AV_VIDEO_TRICK_MODE_BY_SEEK = 2,       // Trick mode by seek
    JNI_ASPLAYER_AV_VIDEO_TRICK_MODE_IONLY = 3          // Decode and out I frame only
} jni_asplayer_video_trick_mode;

typedef enum {
    JNI_ASPLAYER_TRANSITION_MODE_BEFORE_BLACK = 0,              // black screen
    JNI_ASPLAYER_TRANSITION_MODE_BEFORE_LAST_IMAGE = 1          // keep last frame
} jni_asplayer_transition_mode_before;

/*JniASPlayer handle*/
typedef size_t jni_asplayer_handle;

/*JniASPlayer init parameters*/
typedef struct {
    jni_asplayer_playback_mode playback_mode; // playback mode
    jni_asplayer_input_source_type source;  // Input source type
    int64_t event_mask;                    // Mask the event type needed by caller
} jni_asplayer_init_params;

/*JniASPlayer input buffer type*/
typedef struct {
    void *buf_data;                        // Input buffer addr
    int32_t offset;
    int32_t buf_size;                      // Input buffer size
} jni_asplayer_input_buffer;

/*JniASPlayer input buffer type*/
typedef struct {
    void *buf_data;                        // Input buffer addr
    int32_t buf_size;                      // Input buffer size
    uint64_t pts;                          //Frame pts,used only for frame mode
    int32_t isvideo;
} jni_asplayer_input_frame_buffer;

/*JniASPlayer video init parameters*/
typedef struct {
    const char* mimeType;                  // Video mimeType
    int32_t width;                         // Video width
    int32_t height;                        // Video height
    int32_t pid;                           // Video pid in TS
    int32_t filterId;                      // video track filter id in Tuner
    int32_t avSyncHwId;                    // AvSyncHwId
    bool scrambled;                        // scrambled or not
    jobject mediaFormat;                   // Video MediaFormat
} jni_asplayer_video_params;

/*JniASPlayer audio init parameters*/
typedef struct {
    const char* mimeType;                   // Audio mimeType
    int32_t sampleRate;                     // Audio sampleRate
    int32_t channelCount;                   // Audio channel count
    int32_t pid;                            // Audio pid in ts
    int32_t filterId;                       // Audio track filter id in Tuner
    int32_t avSyncHwId;                     // AvSyncHwId
    int32_t seclevel;                       // Audio security level
    bool scrambled;                         // scrambled or not
    jobject mediaFormat;                    // Audio MediaFormat
} jni_asplayer_audio_params;

/*Video basic information*/
typedef struct {
    uint32_t width;                        // Video frame width
    uint32_t height;                       // Video frame height
    uint32_t framerate;                    // Video frame rate
    uint32_t aspectRatio;                  // Video aspect ratio
} jni_asplayer_video_info;

/*Video qos information*/
typedef struct {
    uint32_t num;
    uint32_t type;
    uint32_t size;
    uint32_t pts;
    uint32_t max_qp;
    uint32_t avg_qp;
    uint32_t min_qp;
    uint32_t max_skip;
    uint32_t avg_skip;
    uint32_t min_skip;
    uint32_t max_mv;
    uint32_t min_mv;
    uint32_t avg_mv;
    uint32_t decode_buffer;
} jni_asplayer_video_qos;

/*Audio basic information*/
typedef struct {
    uint32_t sample_rate;                  // Audio sample rate
    uint32_t channels;                     // Audio channels
    uint32_t channel_mask;                 // Audio channel mask
    uint32_t bitrate;                      // Audio bitrate
} jni_asplayer_audio_info;

typedef struct {
    uint32_t frame_width;
    uint32_t frame_height;
    uint32_t frame_rate;
    uint32_t frame_aspectratio;
} jni_asplayer_video_format_t;

typedef struct {
    uint32_t sample_rate;
    uint32_t channels;
    uint32_t channel_mask;
} jni_asplayer_audio_format_t;

typedef struct {
    int32_t first_lang;
    int32_t second_lang;
} jni_asplayer_audio_lang;


typedef struct {
    jni_asplayer_stream_type stream_type;
    uint64_t pts;
    uint64_t renderTime;
} jni_asplayer_pts_t;

typedef struct {
    uint8_t  *data;
    size_t   len;
} mpeg_user_data_t;

typedef struct {
    jni_asplayer_stream_type stream_type;
    bool_t  scramling;
} scamling_t;

typedef struct {
    uint32_t video_overflow_num;                        // Video overflow num
    uint32_t video_underflow_num;                       // Video underflow num
    uint32_t audio_overflow_num;                        // Audio overflow num
    uint32_t audio_underflow_num;                       // Audio underflow num
} av_flow_t;

/*JniASPlayer call back event*/
typedef struct {
    jni_asplayer_event_type type;           // Call back event type
    union {
        /*If type is VIDEO_CHANGED send new video basic info*/
        jni_asplayer_video_format_t video_format;
        /*If type is AUDIO_CHANGED send new video basic info*/
        jni_asplayer_audio_format_t audio_format;
        /*Audio/Video/Subtitle pts after pes parser*/
        jni_asplayer_pts_t pts;
        /*User data send cc /afd /dvb subtitle to caller*/
        mpeg_user_data_t mpeg_user_data;
        /*Scrambling status changed send scrambling info to caller*/
        scamling_t scramling;
        /*Callback audio/video input buffer ptr*/
        void* bufptr;
        /*If Audio/Video overflow/underflow count the num*/
        av_flow_t av_flow_cnt;
    } event;
}jni_asplayer_event;

/*Event callback function ptr*/
typedef void (*event_callback) (void *user_data, jni_asplayer_event *event);

/**
 * @brief:        Initialize JNI environment.
 *                Must called from Java thread.
 *                Java thread -> c/cpp(JNI) -> registerJNI()
 * @param: env    JNIEnv    JNIEnv from JNI method call
 * @return:       The JniASPlayer result.
 */
jni_asplayer_result JniASPlayer_registerJNI(JNIEnv *env);

/**
 * @brief:        Create JniASPlayer instance.
 *                Set input mode demux_id and event mask to JniASPlayer.
 * @param:        Params    Init params with input mode demux_id and event mask.
 * @param:        *pHandle  JniASPlayer handle.
 * @return:       The JniASPlayer result.
*/
jni_asplayer_result  JniASPlayer_create(jni_asplayer_init_params Params, void *tuner, jni_asplayer_handle *pHandle);

/**
 * @brief:        Get ASPlayer instance.
 * @param:        handle    JniASPlayer handle.
 * @param:        *pASPlayer ASPlayer instance.
 * @return:       The JniASPlayer result.
*/
jni_asplayer_result  JniASPlayer_getJavaASPlayer(jni_asplayer_handle handle, jobject *pASPlayer);

/**
 * @brief:        Prepare JniASPlayer instance.
 * @param:        Handle  JniASPlayer handle.
 * @return:       The JniASPlayer result.
*/
jni_asplayer_result  JniASPlayer_prepare(jni_asplayer_handle Handle);

/**
 *@brief:        Get JniASPlayer interface version information.
 *@param:        *versionM    JniASPlayer interface version.
 *@param:        *VersionL    JniASPlayer interface version.
 *@return:       The JniASPlayer result.
 */
jni_asplayer_result  JniASPlayer_getVersion(uint32_t *versionM,
                                          uint32_t *VersionL);

/**
 *@brief:        Get the instance number of specified JniASPlayer.
 *@param:        Handle    JniASPlayer handle.
 *@param:        *Numb     JniASPlayer instance number.
 *@return:       The JniASPlayer result.
 */
jni_asplayer_result  JniASPlayer_getInstansNo(jni_asplayer_handle Handle, uint32_t *Numb);

/**
 *@brief:        Get the sync instance number of specified JniASPlayer .
 *@param:        Handle    JniASPlayer handle.
 *@param:        *Numb     JniASPlayer instance number.
 *@return:       The JniASPlayer result.
 */
jni_asplayer_result  JniASPlayer_getSyncInstansNo(jni_asplayer_handle Handle, int32_t *Numb);

/**
 *@brief:        Register event callback to specified JniASPlayer
 *@param:        Handle    JniASPlayer handle.
 *@param:        pfunc     Event callback function ptr.
 *@param:        *param    Extra data ptr.
 *@return:       The JniASPlayer result.
 */
jni_asplayer_result  JniASPlayer_registerCb(jni_asplayer_handle Handle, event_callback pfunc, void *param);

/**
 *@brief:        Get event callback to specified JniASPlayer
 *@param:        Handle      JniASPlayer handle.
 *@param:        *pfunc      ptr of Event callback function ptr.
 *@param:        *ppParam    Set the callback, with a pointer to the parameter.
 *@return:       The JniASPlayer result.
 */
jni_asplayer_result  JniASPlayer_getCb(jni_asplayer_handle Handle, event_callback *pfunc, void* *ppParam);

/**
 *@brief:        Release specified JniASPlayer instance.
 *@param:        handle     JniASPlayer handle.
 *@return:       The JniASPlayer result.
 */
jni_asplayer_result  JniASPlayer_release(jni_asplayer_handle handle);

/**
 *@brief:        Flush specified JniASPlayer instance.
 *@param:        Handle         JniASPlayer handle.
 *@return:       The JniASPlayer result.
 */
jni_asplayer_result  JniASPlayer_flush(jni_asplayer_handle handle);

/**
 *@brief:        Flush DvrPlayback of specified JniASPlayer instance.
 *@param:        Handle         JniASPlayer handle.
 *@return:       The JniASPlayer result.
 */
jni_asplayer_result  JniASPlayer_flushDvr(jni_asplayer_handle handle);

/**
 *@brief:        Write Frame data to specified JniASPlayer instance.
 *               It will only work when TS input's source type is TS_MEMORY.
 *@param:        Handle       JniASPlayer handle.
 *@param:        *buf         Input buffer struct (1.Buffer type:secure/no
 *                            2.secure buffer ptr 3.buffer len).
 *@param:        timeout_ms   Time out limit.
 *@return:       The JniASPlayer result.
 */
jni_asplayer_result  JniASPlayer_writeFrameData(jni_asplayer_handle Handle,
                                              jni_asplayer_input_frame_buffer *buf,
                                              uint64_t timeout_ms);

/**
 *@brief:        Write data to specified JniASPlayer instance.
 *               It will only work when TS input's source type is TS_MEMORY.
 *@param:        Handle         JniASPlayer handle.
 *@param:        *buf           Input buffer struct (1.Buffer type:secure/no
 *                              2.secure buffer ptr 3.buffer len).
 *@param:        timeout_ms     Time out limit .
 *@return:       The JniASPlayer result.
 */
jni_asplayer_result  JniASPlayer_writeData(jni_asplayer_handle Handle, jni_asplayer_input_buffer *buf, uint64_t timeout_ms);

/**
 *@brief:        Set work mode to specified JniASPlayer instance.
 *@param:        Handle     JniASPlayer handle.
 *@param:        mode       The enum of work mode.
 *@return:       The JniASPlayer result.
 */
jni_asplayer_result  JniASPlayer_setWorkMode (jni_asplayer_handle Handle, jni_asplayer_work_mode mode);

/**
 *@brief:        Reset work mode to specified JniASPlayer instance.
 *@param:        Handle     JniASPlayer handle.
 *@return:       The JniASPlayer result.
 */
jni_asplayer_result  JniASPlayer_resetWorkMode (jni_asplayer_handle Handle);

/**
 *@brief:        Set PIP mode to specified JniASPlayer instance.
 *@param:        Handle     JniASPlayer handle.
 *@param:        mode       The enum of PIP mode.
 *@return:       The JniASPlayer result.
 */
jni_asplayer_result  JniASPlayer_setPIPMode (jni_asplayer_handle Handle, jni_asplayer_pip_mode mode);

/*AV sync*/
/**
 *@brief:        Get the playing time of specified JniASPlayer instance.
 *@param:        Handle     JniASPlayer handle.
 *@param:        *time      Playing time.
 *@return:       The JniASPlayer result.
 */
jni_asplayer_result  JniASPlayer_getCurrentTime(jni_asplayer_handle Handle, int64_t *time);

/**
 *@brief:        Get the pts of specified JniASPlayer instance.
 *@param:        Handle     JniASPlayer handle.
 *@param:        StrType    stream type.
 *@param:        *pts       pts.
 *@return:       The JniASPlayer result.
 */
jni_asplayer_result  JniASPlayer_getPts(jni_asplayer_handle Handle, jni_asplayer_stream_type StrType, uint64_t *pts);

/**
 *@brief:        Set the tsync mode for specified JniASPlayer instance.
 *@param:        Handle     JniASPlayer handle.
 *@param:        mode       The enum of avsync mode.
 *@return:       The JniASPlayer result.
 */
jni_asplayer_result  JniASPlayer_setSyncMode(jni_asplayer_handle Handle, jni_asplayer_avsync_mode mode);

/**
 *@brief:        Get the tsync mode for specified JniASPlayer instance.
 *@param:        Handle    JniASPlayer handle.
 *@param:        *mode     The avsync mode of specified JniASPlayer instance.
 *@return:       The JniASPlayer result.
 */
jni_asplayer_result  JniASPlayer_getSyncMode(jni_asplayer_handle Handle, jni_asplayer_avsync_mode *mode);

/**
 *@brief:        Set pcr pid to specified JniASPlayer instance.
 *@param:        Handle     JniASPlayer handle.
 *@param:        pid        The pid of pcr.
 *@return:       The JniASPlayer result.
 */
jni_asplayer_result  JniASPlayer_setPcrPid(jni_asplayer_handle Handle, uint32_t pid);


/*Player control interface*/
/**
 *@brief:        Start Fast play for specified JniASPlayer instance.
 *@param:        Handle     JniASPlayer handle.
 *@param:        scale      Fast play speed.
 *@return:       The JniASPlayer result.
 */
jni_asplayer_result  JniASPlayer_startFast(jni_asplayer_handle Handle, float scale);

/**
 *@brief:        Stop Fast play for specified JniASPlayer instance.
 *@param:        Handle       JniASPlayer handle.
 *@return:       The JniASPlayer result.
 */
jni_asplayer_result  JniASPlayer_stopFast(jni_asplayer_handle Handle);

/**
 *@brief:        Set trick mode for specified JniASPlayer instance.
 *@param:        Handle        JniASPlayer handle.
 *@param:        trickmode     The enum of trick mode type
 *@return:       The JniASPlayer result.
 */
jni_asplayer_result  JniASPlayer_setTrickMode(jni_asplayer_handle Handle, jni_asplayer_video_trick_mode trickmode);

/**
 *@brief:        Set Surface ptr to specified JniASPlayer instance.
 *@param:        Handle       JniASPlayer handle.
 *@param:        *pSurface    Surface ptr
 *@return:       The JniASPlayer result.
 */
jni_asplayer_result  JniASPlayer_setSurface(jni_asplayer_handle Handle, void* pSurface);

/**
 *@brief:        Set video params need by demuxer and video decoder
 *               for specified JniASPlayer instance.
 *@param:        Handle      JniASPlayer handle.
 *@param:        *pParams    Params need by demuxer and video decoder.
 *@return:       The JniASPlayer result.
 */
jni_asplayer_result  JniASPlayer_setVideoParams(jni_asplayer_handle Handle, jni_asplayer_video_params *pParams);

/**
 *@brief:        Set if need keep last frame for video display
 *               for specified JniASPlayer instance.
 *@param:        Handle     JniASPlayer handle.
 *@param:        mode       transition mode before.
 *@return:       The JniASPlayer result.
 */
jni_asplayer_result  JniASPlayer_setTransitionModeBefore(jni_asplayer_handle Handle, jni_asplayer_transition_mode_before mode);

/**
 *@brief:        Get video basic info of specified JniASPlayer instance.
 *@param:        Handle      JniASPlayer handle.
 *@param:        *pInfo      The ptr of video basic info struct .
 *@return:       The JniASPlayer result.
 */
jni_asplayer_result  JniASPlayer_getVideoInfo(jni_asplayer_handle Handle, jni_asplayer_video_info *pInfo);

/**
 *@brief:        Start video decoding for specified JniASPlayer instance .
 *@param:        Handle      JniASPlayer handle.
 *@return:       The JniASPlayer result.
 */
jni_asplayer_result  JniASPlayer_startVideoDecoding(jni_asplayer_handle Handle);

/**
 *@brief:        Pause video decoding for specified JniASPlayer instance .
 *@param:        Handle       JniASPlayer handle.
 *@return:       The JniASPlayer result.
 */
jni_asplayer_result  JniASPlayer_pauseVideoDecoding(jni_asplayer_handle Handle);

/**
 *@brief:        Resume video decoding for specified JniASPlayer instance .
 *@param:        Handle      JniASPlayer handle.
 *@return:       The JniASPlayer result.
 */
jni_asplayer_result  JniASPlayer_resumeVideoDecoding(jni_asplayer_handle Handle);

/**
 *@brief:        Stop video decoding for specified JniASPlayer instance .
 *@param:        Handle     JniASPlayer handle.
 *@return:       The JniASPlayer result.
 */
jni_asplayer_result  JniASPlayer_stopVideoDecoding(jni_asplayer_handle Handle);


/*Audio interface*/
/**
 *@brief:        Set audio volume to specified JniASPlayer instance .
 *@param:        Handle     JniASPlayer handle.
 *@param:        volume     Volume value.
 *@return:       The JniASPlayer result.
 */
jni_asplayer_result  JniASPlayer_setAudioVolume(jni_asplayer_handle Handle, int32_t volume);

/**
 *@brief:        Get audio volume value from specified JniASPlayer instance .
 *@param:        Handle      JniASPlayer handle.
 *@param:        *volume     Volume value.
 *@return:       The JniASPlayer result.
 */
jni_asplayer_result  JniASPlayer_getAudioVolume(jni_asplayer_handle Handle, int32_t *volume);

/*Audio interface*/
/**
 *@brief:        Set audio stereo mode to specified JniASPlayer instance .
 *@param:        Handle     JniASPlayer handle.
 *@param:        Mode       Stereo mode.
 *@return:       The JniASPlayer result.
 */
jni_asplayer_result  JniASPlayer_setAudioStereoMode(jni_asplayer_handle Handle, jni_asplayer_audio_stereo_mode Mode);

/**
 *@brief:        Get audio stereo mode to specified JniASPlayer instance .
 *@param:        Handle    JniASPlayer handle.
 *@param:        *pMode    Stereo mode.
 *@return:       The JniASPlayer result.
 */
jni_asplayer_result  JniASPlayer_getAudioStereoMode(jni_asplayer_handle Handle, jni_asplayer_audio_stereo_mode *pMode);

/**
 *@brief:        Set audio output mute to specified JniASPlayer instance .
 *@param:        Handle         JniASPlayer handle.
 *@param:        analog_mute    If analog mute or unmute .
 *@param:        digital_mute   If digital mute or unmute .
 *@return:       The JniASPlayer result.
 */
jni_asplayer_result  JniASPlayer_setAudioMute(jni_asplayer_handle Handle, bool_t analog_mute, bool_t digital_mute);

/**
 *@brief:        Get audio output mute status from specified
                 JniASPlayer instance .
 *@param:        Handle            JniASPlayer handle.
 *@param:        *analog_unmute    If analog mute or unmute .
 *@param:        *digital_unmute   If digital mute or unmute .
 *@return:       The JniASPlayer result.
 */
jni_asplayer_result  JniASPlayer_getAudioMute(jni_asplayer_handle Handle, bool_t *analog_unmute, bool_t *digital_unmute);

/**
 *@brief:        Set audio params need by demuxer and audio decoder
 *               to specified JniASPlayer instance.
 *@param:        Handle     JniASPlayer handle.
 *@param:        *pParams   Params need by demuxer and audio decoder.
 *@return:       The JniASPlayer result.
 */
jni_asplayer_result  JniASPlayer_setAudioParams(jni_asplayer_handle Handle, jni_asplayer_audio_params *pParams);

/**
 *@brief:        Switch audio track for specified JniASPlayer instance.
 *@param:        Handle     JniASPlayer handle.
 *@param:        *pParams   Params need by demuxer and audio decoder.
 *@return:       The JniASPlayer result.
 */
jni_asplayer_result  JniASPlayer_switchAudioTrack(jni_asplayer_handle Handle, jni_asplayer_audio_params *pParams);

/**
 *@brief:        Get audio basic info of specified JniASPlayer instance.
 *@param:        Handle      JniASPlayer handle.
 *@param:        *pInfo      The ptr of audio basic info struct .
 *@return:       The JniASPlayer result.
 */
jni_asplayer_result  JniASPlayer_getAudioInfo(jni_asplayer_handle Handle,  jni_asplayer_audio_info *pInfo);

/**
 *@brief:        Start audio decoding for specified JniASPlayer instance .
 *@param:        Handle     JniASPlayer handle.
 *@return:       The JniASPlayer result.
 */
jni_asplayer_result  JniASPlayer_startAudioDecoding(jni_asplayer_handle Handle);

/**
 *@brief:        Pause audio decoding for specified JniASPlayer instance .
 *@param:        Handle     JniASPlayer handle.
 *@return:       The JniASPlayer result.
 */
jni_asplayer_result  JniASPlayer_pauseAudioDecoding(jni_asplayer_handle Handle);

/**
 *@brief:        Resume audio decoding for specified JniASPlayer instance .
 *@param:        Handle     JniASPlayer handle.
 *@return:       The JniASPlayer result.
 */
jni_asplayer_result  JniASPlayer_resumeAudioDecoding(jni_asplayer_handle Handle);

/**
 *@brief:        Stop audio decoding for specified JniASPlayer instance .
 *@param:        Handle     JniASPlayer handle.
 *@return:       The JniASPlayer result.
 */
jni_asplayer_result  JniASPlayer_stopAudioDecoding(jni_asplayer_handle Handle);

/*Audio description interface*/
/**
 *@brief:        Set audio description params need by demuxer
 *               and audio decoder to specified JniASPlayer instance.
 *@param:        Handle     JniASPlayer handle.
 *@param:        *pParams   Params need by demuxer and audio decoder.
 *@return:       The JniASPlayer result.
 */
jni_asplayer_result  JniASPlayer_setADParams(jni_asplayer_handle Handle, jni_asplayer_audio_params *pParams);

/**
 *@brief:        Set audio description volume
 *@param:        handle        JniASPlayer handle.
 *@param:        volumeDb      AD volume in dB.
 *@return:       The JniASPlayer result.
 */
jni_asplayer_result  JniASPlayer_setADVolumeDB(jni_asplayer_handle handle, float volumeDB);

/**
 *@brief:        Get audio description volume
 *@param:        handle        JniASPlayer handle.
 *@param:        *volumeDB     AD volume in dB.
 *@return:       The JniASPlayer result.
 */
jni_asplayer_result  JniASPlayer_getADVolumeDB(jni_asplayer_handle handle, float *volumeDB);

/**
 *@brief:        Enable audio description mix with master audio
 *@param:        handle     JniASPlayer handle.
 *@return:       The JniASPlayer result.
 */
jni_asplayer_result  JniASPlayer_enableADMix(jni_asplayer_handle handle);

/**
 *@brief:        Disable audio description mix with master audio
 *@param:        handle     JniASPlayer handle.
 *@return:       The JniASPlayer result.
 */
jni_asplayer_result  JniASPlayer_disableADMix(jni_asplayer_handle handle);

/**
 *@brief:        Set audio description mix level (ad vol)
 *@param:        handle        JniASPlayer handle.
 *@param:        mixLevel      audio description mix level.
 *@return:       The JniASPlayer result.
 */
jni_asplayer_result  JniASPlayer_setADMixLevel(jni_asplayer_handle handle, int32_t mixLevel);

/**
 *@brief:        Get audio description mix level (ad vol)
 *@param:        handle        JniASPlayer handle.
 *@param:        *mixLevel     audio description mix level.
 *@return:       The JniASPlayer result.
 */
jni_asplayer_result  JniASPlayer_getADMixLevel(jni_asplayer_handle Handle, int32_t *mixLevel);

/**
 *@brief:        Get audio description basic info of specified
 *               JniASPlayer instance.
 *@param:        Handle    JniASPlayer handle.
 *@param:        *pInfo    The ptr of audio basic info struct .
 *@return:       The JniASPlayer result.
 */
jni_asplayer_result  JniASPlayer_getADInfo(jni_asplayer_handle Handle, jni_asplayer_audio_info *pInfo);

/*Subtitle interface*/
/**
 *@brief:        Set subtitle pid for specified JniASPlayer instance .
 *@param:        Handle    JniASPlayer handle.
 *@param:        pid       The pid of subtitle.
 *@return:       The JniASPlayer result.
 */
jni_asplayer_result  JniASPlayer_setSubPid(jni_asplayer_handle Handle, uint32_t pid);

/**
 *@brief:        get Params for specified JniASPlayer instance .
 *@param:        Handle    JniASPlayer handle.
 *@param:        type      JniASPlayer parameter type.
 *@param:        *arg      The qualified pointer returned
                           by the function.
 *@return:       The JniASPlayer result.
 */
jni_asplayer_result  JniASPlayer_getParams(jni_asplayer_handle Handle, jni_asplayer_parameter type, void* arg);

/**
 *@brief:        set Params for specified JniASPlayer instance .
 *@param:        Handle     JniASPlayer handle.
 *@return:       The JniASPlayer result.
 */
jni_asplayer_result  JniASPlayer_setParams(jni_asplayer_handle Handle, jni_asplayer_parameter type, void* arg);

/**
 *@brief:        get State for specified JniASPlayer instance .
 *@param:        Handle    JniASPlayer handle.
 *@return:       The JniASPlayer result.
 */
jni_asplayer_result JniASPlayer_getState(jni_asplayer_handle Handle,jni_asplayer_state_t* state);

/**
 *@brief:        Start subtitle for specified JniASPlayer instance .
 *@param:        Handle    JniASPlayer handle.
 *@return:       The JniASPlayer result.
 */
jni_asplayer_result  JniASPlayer_startSub(jni_asplayer_handle Handle);

/**
 *@brief:        Stop subtitle for specified JniASPlayer instance .
 *@param:        Handle    JniASPlayer handle.
 *@return:       The JniASPlayer result.
 */
jni_asplayer_result  JniASPlayer_stopSub(jni_asplayer_handle Handle);

/**
 *@brief:        Get the first pts of specified JniASPlayer instance.
 *@param:        Handle    JniASPlayer handle.
 *@param:        StrType   stream type.
 *@param:        *pts      output pts.
 *@return:       The JniASPlayer result.
 */
jni_asplayer_result  JniASPlayer_getFirstPts(jni_asplayer_handle Handle, jni_asplayer_stream_type StrType, uint64_t *pts);


#ifdef __cplusplus
};
#endif


#endif //JNI_ASPLAYER_H
