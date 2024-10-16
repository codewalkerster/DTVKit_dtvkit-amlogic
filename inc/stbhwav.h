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
 * @brief   Header file - Function prototypes for A/V control
 * @file    stbhwav.h
 * @date    06/02/2001
 */

// pre-processor mechanism so multiple inclusions don't cause compilation error

#ifndef _STBHWAV_H
#define _STBHWAV_H

#include "techtype.h"
#include "stbhwc.h"
#include "osdtype.h"
#include <pthread.h>


//---Constant and macro definitions for public use-----------------------------

//---Enumerations for public use-----------------------------------------------

typedef enum e_stb_av_audio_mode
{
    AV_AUDIO_STEREO = 0,
    AV_AUDIO_LEFT = 1,
    AV_AUDIO_RIGHT = 2,
    AV_AUDIO_MONO = 3,
    AV_AUDIO_MULTICHANNEL = 4
} E_STB_AV_AUDIO_MODE;

typedef enum e_stb_av_decode_source
{
    AV_DEMUX = 0,
    AV_MEMORY = 1
} E_STB_AV_DECODE_SOURCE;

typedef enum e_stb_av_video_codec
{
    AV_VIDEO_CODEC_AUTO = 0,
    AV_VIDEO_CODEC_MPEG1 = 1,
    AV_VIDEO_CODEC_MPEG2 = 2,
    AV_VIDEO_CODEC_H264 = 3,
    AV_VIDEO_CODEC_H265 = 4,
    AV_VIDEO_CODEC_VP9 = 5,
    AV_VIDEO_CODEC_AVS = 6,
    AV_VIDEO_CODEC_AVS2 = 7,
    AV_VIDEO_CODEC_MPEG4 = 8,
    AV_VIDEO_CODEC_DVES_AVC = 9,
    AV_VIDEO_CODEC_DVES_HEVC = 10,
    AV_VIDEO_CODEC_AVS3 = 11,
    AV_VIDEO_CODEC_H266 = 12
} E_STB_AV_VIDEO_CODEC;

typedef enum e_stb_av_audio_codec
{
    AV_AUDIO_CODEC_AUTO = 0,
    AV_AUDIO_CODEC_MP2 = 1,
    AV_AUDIO_CODEC_MP3 = 2,
    AV_AUDIO_CODEC_AC3 = 3,
    AV_AUDIO_CODEC_EAC3 = 4,
    AV_AUDIO_CODEC_AAC = 5,
    AV_AUDIO_CODEC_HEAAC = 6,
    AV_AUDIO_CODEC_AAC_ADTS = 7,
    AV_AUDIO_CODEC_HEAACV2 = 8,
    AV_AUDIO_CODEC_AC4 = 9
} E_STB_AV_AUDIO_CODEC;

typedef enum
{
    AV_SHOW = 0,
    AV_HIDDEN
} SET_AV_BLANK;

typedef enum
{
    AUTO = 0,
    OVERRIDE_BY_BLACK
} AV_MUTE_OPTION;

typedef enum avout_control_bits
{
    AVOUT_VOL = 0,
    AVOUT_SYSTEM,
    AVOUT_CIP_LIC,
    AVOUT_CIP_RET,
    AVOUT_CIP_PIN,
    AVOUT_CIP_OTHER1,
    AVOUT_CIP_OTHER2,
    AVOUT_CIP_OTHER3
} E_AV_OUT_CONTROL_FLAG;

typedef E_ASPECT_RATIO E_STB_AV_ASPECT_RATIO;

typedef enum e_stb_av_video_format
{
    VIDEO_FORMAT_UNDEFINED = 255,
    VIDEO_FORMAT_AUTO = 0,
    VIDEO_FORMAT_ORIGINAL,
    VIDEO_FORMAT_PALBGH,
    VIDEO_FORMAT_PALDKL,
    VIDEO_FORMAT_PALI,
    VIDEO_FORMAT_PALM,
    VIDEO_FORMAT_PALN,
    VIDEO_FORMAT_NTSC,
    VIDEO_FORMAT_SECAMBGH,
    VIDEO_FORMAT_SECAMDKL,
    VIDEO_FORMAT_576IHD,
    VIDEO_FORMAT_576PHD,
    VIDEO_FORMAT_720HD,     /* HD Format, 1280x720  progressive, 50 fps */
    VIDEO_FORMAT_720P50HD = VIDEO_FORMAT_720HD,
    VIDEO_FORMAT_720P60HD,
    VIDEO_FORMAT_1080IHD,   /* HD Format, 1920x1080 interlaced, 25 fps */
    VIDEO_FORMAT_1080P25HD, /* HD Format, 1920x1080 progressive, 25 fps */
    VIDEO_FORMAT_1080P30HD, /* HD Format, 1920x1080 progressive, 30 fps */
    VIDEO_FORMAT_1080I50HD, /* HD Format, 1920x1080 interlaced,  50 fps */
    VIDEO_FORMAT_1080P50HD, /* HD Format, 1920x1080 progressive, 50 fps */
    VIDEO_FORMAT_1080P60HD, /* HD Format, 1920x1080 progressive, 60 fps */
    VIDEO_FORMAT_2160P24UHD,
    VIDEO_FORMAT_2160P25UHD,
    VIDEO_FORMAT_2160P30UHD,
    VIDEO_FORMAT_2160P50UHD,
    VIDEO_FORMAT_2160P60UHD,
} E_STB_AV_VIDEO_FORMAT;

typedef enum e_stb_av_outputs
{
    AV_OUTPUT_TV_SCART = 0,
    AV_OUTPUT_VCR_SCART = 1,
    AV_OUTPUT_AUX_SCART = 2,
    AV_OUTPUT_HDMI = 3
} E_STB_AV_OUTPUTS;

typedef enum e_stb_av_sources
{
    AV_SOURCE_ENCODER_SVIDEO = 0,
    AV_SOURCE_ENCODER_RGB = 1,
    AV_SOURCE_ENCODER_COMPOSITE = 2,
    AV_SOURCE_VCR_SVIDEO = 3,
    AV_SOURCE_VCR_RGB = 4,
    AV_SOURCE_VCR_COMPOSITE = 5,
    AV_SOURCE_AUX_SVIDEO = 6,
    AV_SOURCE_AUX_RGB = 7,
    AV_SOURCE_AUX_COMPOSITE = 8,
    AV_SOURCE_DVD_SVIDEO = 9,
    AV_SOURCE_DVD_RGB = 10,
    AV_SOURCE_DVD_COMPOSITE = 11,
    AV_SOURCE_ANALOG_TUNER = 12,
    AV_SOURCE_ANALOG_TUNER_WITH_OSD = 13,
    AV_SOURCE_HDMI = 14,
    AV_SOURCE_CVBS = 15,
    AV_SOURCE_TUNER = 16,
    AV_SOURCE_NONE = 255
} E_STB_AV_SOURCES;

typedef enum e_stb_av_srm_reply
{
    SRM_OK = 0,
    SRM_BUSY = 1,
    SRM_NOT_REQUIRED = 2
} E_STB_AV_SRM_REPLY;

typedef enum
{
    VIDEO_INFO_VIDEO_RESOLUTION = 0x01,
    VIDEO_INFO_SCREEN_RESOLUTION = 0x02,
    VIDEO_INFO_VIDEO_ASPECT_RATIO = 0x04,
    VIDEO_INFO_DISPLAY_ASPECT_RATIO = 0x08,
    VIDEO_INFO_ASPECT_MODE = 0x10,
    VIDEO_INFO_AFD = 0x20,
    VIDEO_INFO_DECODER_STATUS = 0x40
} E_STB_AV_VIDEO_INFO_TYPE;

typedef enum
{
    DECODER_STATUS_NONE,
    DECODER_STATUS_VIDEO,
    DECODER_STATUS_IFRAME,
    DECODER_STATUS_DECODE_FIRST_FRAME_VIDEO
} E_STB_AV_DECODER_STATUS;

typedef enum
{
    DIGITAL_AUDIO_PCM,
    DIGITAL_AUDIO_COMPRESSED,
    DIGITAL_AUDIO_AUTO
} E_STB_DIGITAL_AUDIO_TYPE;

typedef enum e_stb_av_audio_route
{
    SPEAKER_TV_N_AD_HEADPHONE_TV_N_AD,
    SPEAKER_TV_ONLY_HEADPHONE_AD_ONLY,
    SPEAKER_TV_N_AD_HEADPHONE_AD_ONLY,
    SPEAKER_TV_ONLY_HEADPHONE_TV_N_AD,
    AUTO_ROUTE_UNKNOWN,
} E_STB_AV_AUDIO_ROUTE;

typedef enum
{
    DECODING_MODE_NORMAL = 0x00,
    DECODING_MODE_CACHE_ONLY = 0x01,
    DECODING_AD_ENABLE = 0x02,
    DECODING_AUDIO_DISABLE = 0x04,
} E_STB_DECODING_MODE;

typedef enum
{
    DRM_NONE,
    DRM_SECURE_INPUT_BUFFER,
    DRM_NORMAL_INPUT_BUFFER
} E_STB_DRM_TYPE;


/*Hbbtv PTS/STC Timeline type*/
typedef enum {
    HBBTV_PTS_TIME_UNKNOWN = 0,
    HBBTV_PTS_TIME_VIDEO   = 1,     //Video
    HBBTV_PTS_TIME_AUDIO   = 2,     //Audio
    HBBTV_PTS_TIME_PCR     = 3,     //PCR 9
    HBBTV_PTS_TIME_STC     = 4,     //System time clock 10
} E_HBBTV_PTS_SOURCE_TYPE;


//---Global type defs for public use-------------------------------------------

typedef struct
{
    BOOLEAN macrovision_set;
    U8BIT macrovision;

    BOOLEAN aps_set;
    U8BIT aps;

    BOOLEAN cgms_a_set;
    U8BIT cgms_a;

    BOOLEAN ict_set;
    U8BIT ict;

    BOOLEAN hdcp_set;
    BOOLEAN hdcp;

    BOOLEAN scms_set;
    U8BIT scms;

    BOOLEAN dot_set;
    U8BIT dot;
} S_STB_AV_COPY_PROTECTION;

typedef struct
{
    E_STB_AV_VIDEO_INFO_TYPE flags;
    U16BIT video_width;
    U16BIT video_height;
    U16BIT screen_width;
    U16BIT screen_height;
    E_STB_AV_ASPECT_RATIO video_aspect_ratio;
    E_STB_AV_ASPECT_RATIO display_aspect_ratio;
    U8BIT afd;
    E_STB_AV_DECODER_STATUS status;
} S_STB_AV_VIDEO_INFO;

/**
 * @brief Callback function used to notify new closed caption data.
 *
 * @param path Decode path from where this closed caption data comes from.
 * @param context Context pointer that was passed to STB_AVRegisterCcCallback().
 * @param data Pointer to the buffer holding the closed caption data.
 * @param data_len Size of the buffer in bytes.
 *
 * @return None.
 */
typedef void (*CC_DATA_CALLBACK)(U8BIT path, void* context, U8BIT* data, U16BIT data_len);


//---Global Function prototypes for public use---------------------------------

/**
 * @brief   Initialises the AV components
 * @param   audio_paths number of audio decoders
 * @param   video_paths number of video decoders
 */
void STB_AVInitialise(U8BIT audio_paths, U8BIT video_paths);

U32BIT STB_AVGetBlankFlag(U8BIT path);

void STB_AVSetVideoBlankLock(BOOLEAN enable);

void STB_AVSetAudioMuteLock(BOOLEAN enable);

/**
 * @brief   Clear Full Screen
 */
void STB_ClearFullScreen(void);

/**
 * @brief   no sig need full screen display
 */
BOOLEAN STB_SetFullScreen(void);

/**
 * @brief   Blanks or unblanks the video display
 * @param   window VT id
 * @param   blank TRUE to blank, FALSE to unblank
 * @param   force_black  TRUE to force black, else with user setting
 */
void STB_AVSetWindowColor(U8BIT window, BOOLEAN blank, BOOLEAN force_black, BOOLEAN force_all, U8BIT path, BOOLEAN mode);

/**
 * @brief   Blanks or unblanks the video display
 * @param   path video path
 * @param   blank TRUE to blank, FALSE to unblank
 */
BOOLEAN STB_AVBlankVideo(U8BIT path, E_AV_OUT_CONTROL_FLAG flag, BOOLEAN av_blank);

/**
 * @brief   clearlastframe or unclearlastframe the video display
 * @param   path video path
 * @param   clearlastframe TRUE to blank, FALSE to unblank
*/
void STB_AVClearLastFrame(U8BIT path,BOOLEAN clearlastframe);

/**
 * @brief   Gets the Transition Color config
 * @return  TRUE transition color is black, FALSE otherwise
 */
BOOLEAN STB_AVGetIsBlackTransitionColor(void);

 /**
 * @brief   Get Static Frame Enable or not
 */
BOOLEAN STB_AVGetStaticFrameEnable();

/**
 * @brief   Sets the source of the input to the given video decoder path
 * @param   path video path to configure
 * @param   source the source device to use
 * @param   param parameter set with the source, typically the tuner or demux number
 */
void STB_AVSetVideoSource(U8BIT path, E_STB_AV_DECODE_SOURCE source, U32BIT param);

/**
 * @brief   Sets the video surface  with the given video decoder path
 * @param   path video path
 * @param   video surface point
 * @return  TRUE if the codec is supported and is set correctly, FALSE otherwise
 */
BOOLEAN STB_AVSetSurface(U8BIT path, void *surface);

/**
 * @brief   Whether the black screen when the device signal disappears
 * @param   path video path
 * @param   is_black TRUE is Black screen when the signal disappears, FALSE is still frame
 * @return  TRUE if the codec is supported and is set correctly, FALSE otherwise
 */
BOOLEAN STB_AVSetStillFrame(U8BIT path, BOOLEAN is_black);

/**
 * @brief   control video display
 * @param   mute: av hidden or show
 * @param   mute_option: auto or force set color
 * @param   av_out_flag
*/
void STB_AVMuteControl(U8BIT window, U8BIT path , SET_AV_BLANK mute, AV_MUTE_OPTION mute_option, E_AV_OUT_CONTROL_FLAG av_out_flag);

/**
 * @brief   Gets the video surface  with the given video decoder path
 * @param   path video path
 * @return  surface pointer
 */
void * STB_AVGetSurface(U8BIT path);

/**
 * \brief Set video window with the given video decoder path
 *
 * \param path
 * \param x
 * \param y
 * \param width
 * \param height
 *
 * \return TRUE if set correctly, FALSE otherwise
 */
BOOLEAN STB_AVSetVideoWindow(U8BIT path, int x, int y, int width, int height);
/**
 * @brief   acquire av path used video and audio codec
 * @param   audio_decoder audio decoder being used for play
 * @param   video_decoder video decoder being used for play
 * @param   mode playback startup mode
 */
BOOLEAN STB_AVAcquirePath(U8BIT video_decoder, U8BIT audio_decoder);
BOOLEAN STB_AVReleasePath(U8BIT video_decoder, U8BIT audio_decoder);

/**
 * @brief   get av path used video and audio codec
 * @param   audio_decoder audio decoder being used for play
 * @param   video_decoder video decoder being used for play
 * @param   mode playback startup mode
 */
U8BIT STB_AVGetPath(U8BIT video_decoder, U8BIT audio_decoder);

/**
 * @brief   Sets the video codec to be used when decoding video with the given video decoder path
 * @param   path video path
 * @param   codec codec to be used
 * @return  TRUE if the codec is supported and is set correctly, FALSE otherwise
 */
BOOLEAN STB_AVSetVideoCodec(U8BIT path, E_STB_AV_VIDEO_CODEC codec);

/**
 * @brief   Returns the video codec previously set for the given video path.
 *          This function is currently unused within DVBCore.
 * @param   path video path
 * @return  codec previously set
 */
E_STB_AV_VIDEO_CODEC STB_AVGetVideoCodec(U8BIT path);

#ifdef SUPPORT_CAS
/**
 * @brief   Set Drm Mode on the given video path
 * @param   path video decoder path
 * @param   mode Drm Mode
 */
void STB_AVSetDrmMode(U8BIT path, E_STB_DRM_TYPE mode);
#endif

/**
 * @brief   Starts video decoding on the given video path
 * @param   path video decoder path to be started
 */
void STB_AVStartVideoDecoding(U8BIT path);

/**
 * @brief   Pauses video decoding on the given video path.
 *          The video should not be blanked.
 * @param   path video decoder path to be paused
 */
void STB_AVPauseVideoDecoding(U8BIT path);

/**
 * @brief   Resumes video decoding on the given video path that has previously been paused
 *          The video should not be unblanked.
 * @param   path video decoder path to be resumed
 */
void STB_AVResumeVideoDecoding(U8BIT path);

/**
 * @brief   Stops video decoding on the given video path.
 *          The video is not expected to be blanked.
 * @param   path video decoder path to be stopped
 */
void STB_AVStopVideoDecoding(U8BIT decoder);

/**
 * @brief   Sets the source of the input for the main audio on the given audio decoder path
 * @param   path audio path to configure
 * @param   source the source device to use
 * @param   param parameter set with the source, typically the tuner or demux number
 */
void STB_AVSetAudioSource(U8BIT path, E_STB_AV_DECODE_SOURCE source, U32BIT param);

/**
 * @brief   Sets the audio codec to be used when decoding audio with the given audio decoder path
 * @param   path audio path
 * @param   codec codec to be used
 * @return  TRUE if the codec is supported and is set correctly, FALSE otherwise
 */
BOOLEAN STB_AVSetAudioCodec(U8BIT path, E_STB_AV_AUDIO_CODEC codec);

/**
 * @brief   Returns the audio codec previously set for the given audio path.
 *          This function is currently unused within DVBCore.
 * @param   path audio path
 * @return  codec previously set
 */
E_STB_AV_AUDIO_CODEC STB_AVGetAudioCodec(U8BIT path);

/**
 * @brief   Configures the main audio channel mode (stereo/left/right) in the case where
 *          dual-mono audio is used, such that only the audio from one channel is heard.
 * @param   path audio path to be configured
 * @param   mode audio mode to use
 */
void STB_AVChangeAudioMode(U8BIT path, E_STB_AV_AUDIO_MODE mode);

/**
 * @brief   Starts audio decoding on the given audio path
 * @param   path audio decoder path to be started
 */
void STB_AVStartAudioDecoding(U8BIT decoder);

/**
 * @brief   Stops audio decoding on the given audio path.
 * @param   path audio decoder path to be stopped
 */
void STB_AVStopAudioDecoding(U8BIT decoder);

/**
 * @brief   Switch audio track on the given audio path.
 * @param   path audio decoder path to be stopped
 */
void STB_AVSwitchAudioTrack(U8BIT path);

/**
 * @brief   Sets the volume of the main audio output
 * @param   path audio path to be configured
 * @param   vol audio volume (0-100%)
 */
void STB_AVSetAudioVolume(U8BIT path, U8BIT vol);

/**
 * @brief   Returns the current volume of the main audio output
 * @param   path audio path
 * @return  audio volume (0-100%)
 */
U8BIT STB_AVGetAudioVolume(U8BIT path);

/**
 * @brief   Mute or unmute the audio output on the given audio decoder path
 * @param   path audio path to be configured
 * @param   mute TRUE to mute, FALSE to unmute
 */
void STB_AVSetAudioMute(U8BIT path, E_AV_OUT_CONTROL_FLAG flag, BOOLEAN audio_mute);

/**
 * @brief   Returns the current mute setting of the audio output on the given path
 * @param   path audio path
 * @return  TRUE if audio is muted, FALSE otherwise
 */
BOOLEAN STB_AVGetAudioMute(U8BIT path);

/**
 * @brief   Sets the source of the input for the audio description audio on the
 *          given audio decoder path
 * @param   path audio path to configure
 * @param   source the source device to use
 * @param   param parameter set with the source, typically the tuner or demux number
 */
void STB_AVSetADSource(U8BIT path, E_STB_AV_DECODE_SOURCE source, U32BIT param);

/**
 * @brief   Sets the codec to be used for audio description when decoding audio
 *          with the given audio decoder path
 * @param   path audio path
 * @param   codec codec to be used
 * @return  TRUE if the codec is supported and is set correctly, FALSE otherwise
 */
BOOLEAN STB_AVSetADCodec(U8BIT path, E_STB_AV_AUDIO_CODEC codec);

/**
 * @brief   Returns the codec previously set for audio description on the given audio path.
 *          This function is currently unused within DVBCore.
 * @param   path audio path
 * @return  codec previously set
 */
E_STB_AV_AUDIO_CODEC STB_AVGetADCodec(U8BIT path);

/**
 * @brief   Configures the audio description channel mode (stereo/left/right) in the case where
 *          dual-mono audio is used, such that only the audio from one channel is heard.
 * @param   path audio path to be configured
 * @param   mode audio mode to use
 */
void STB_AVChangeADMode(U8BIT path, E_STB_AV_AUDIO_MODE mode);

/**
 * @brief   Starts decoding audio description on the given audio path
 * @param   path audio decoder path to be started
 */
BOOLEAN STB_AVStartADDecoding(U8BIT decoder);

/**
 * @brief   Stops decoding audio description on the given audio path.
 * @param   path audio decoder path to be stopped
 */
void STB_AVStopADDecoding(U8BIT decoder);

/**
 * @brief   Sets the volume of the audio description output
 * @param   path audio path to be configured
 * @param   vol ad volume (0-100%)
 */
void STB_AVSetADVolume(U8BIT path, U8BIT vol);

/**
 * @brief   Returns the current volume of the audio description output
 * @param   path audio path
 * @return  ad volume (0-100%)
 */
U8BIT STB_AVGetADVolume(U8BIT path);

/**
 * @brief   Sets the mix level of the audio description output
 * @param   path audio path to be configured
 * @param   vol ad mixer level (0-100%)
 */
void STB_AVSetADMixLevel(U8BIT path, U8BIT vol);

/**
 * @brief   Returns the current mixer level of the audio description output
 * @param   path audio path
 * @return  ad mixer level (0-100%)
 */
U8BIT STB_AVGetADMixLevel(U8BIT path);

/**
 * @brief   Returns the current 33-bit System Time Clock from the PCR PES.
 *          On some systems, this information may need to be obtained from the associated demux,
 *          which will be contained in the 'param' value when STB_AVSetVideoSource is called.
 * @param   path video path
 * @param   stc an array in which the STC will be returned, ordered such that
 *                stc[0] contains the MS bit (33) of the STC value and stc[4]
 *                contains the LS bits (0-7).
 *
 */
void STB_AVGetSTCByStreamTypePCR(U8BIT path, U8BIT stc[5]);


/**
 * @brief   Returns the current 33-bit System Time Clock from the PCR PES.
 *          On some systems, this information may need to be obtained from the associated demux,
 *          which will be contained in the 'param' value when STB_AVSetVideoSource is called.
 * @param   path video path
 * @param   stc an array in which the STC will be returned, ordered such that
 *                stc[0] contains the MS bit (33) of the STC value and stc[4]
 *                contains the LS bits (0-7).
 */
void STB_AVGetSTC(U8BIT path, U8BIT stc[5]);


/**
 * @brief   Returns the current 33-bit System Time Clock from the PCR PES.
 *          On some systems, this information may need to be obtained from the associated demux,
 *          which will be contained in the 'param' value when STB_AVSetVideoSource is called.
 * @param   path video path
 * @param   stc an array in which the STC will be returned, ordered such that
 *                stc[0] contains the MS bit (33) of the STC value and stc[4]
 *                contains the LS bits (0-7).
 * @param   StreamType enum E_HBBTV_PTS_SOURCE_TYPE,point to which pts do you want
 */
void STB_AVGetSTCByStreamType(U8BIT path, U8BIT stc[5], E_HBBTV_PTS_SOURCE_TYPE StreamType);

/**
 * @brief   Returns the current PTS from 33-bit System Time Clock.
 * @param   stc an array and we can calculate pts from it.
 *                stc[0] contains the MS bit (33) of the STC value and stc[4]
 *                contains the LS bits (0-7).
 */
U64BIT STB_AVGetPTSBySTC(U8BIT stc[5]);

/**
 * @brief   Sets the aspect ratio and signal format for the connected television
 * @param   path The video path to be set
 * @param   ratio The aspect ratio of the tv
 * @param   format The signal format of the tv
 */
void STB_AVSetTVType(U8BIT path, E_STB_AV_ASPECT_RATIO ratio, E_STB_AV_VIDEO_FORMAT format);

/**
 * @brief   Returns the current size of the screen in pixels
 * @param   path 0
 * @param   width returned width
 * @param   height returned height
 */
void STB_AVGetScreenSize(U8BIT path, U16BIT *width, U16BIT *height);

/**
 * @brief   Routes a specified AV source to a specified AV output
 * @param   output The output to be connected
 * @param   source The source signal to be connected
 * @param   param parameter associated with the source
 */
void STB_AVSetAVOutputSource(E_STB_AV_OUTPUTS output, E_STB_AV_SOURCES source, U32BIT param);

/**
 * @brief   Turns on/off all AV outputs (e.g. for standby mode)
 * @param   av_on TRUE to turn on, FALSE to turn off
 */
void STB_AVSetAVOutput(BOOLEAN av_on);

/**
 * @brief   Sets the standby state of the HDMI output
 * @param   standby TRUE to put the HDMI in standby, FALSE to come out of standby
 */
void STB_AVSetHDMIStandby(BOOLEAN standby);

/**
 * @brief   Returns the resolutions supported by the HDMI
 * @param   modes array of supported modes
 * @return  number of supported modes
 */
U8BIT STB_AVGetHDMISupportedModes(E_STB_AV_VIDEO_FORMAT **modes);

/**
 * @brief   Returns the native resolution, i.e. the resolution that is
 *          set with a call to STB_AVSetTVType(path, ratio, VIDEO_FORMAT_AUTO);
 * @param   width pointer to the variable where to store the width
 * @param   height pointer to the variable where to store the height
 */
void STB_AVGetHDMINativeResolution(U16BIT *width, U16BIT *height);

/**
 * @brief   Enables AV output to HDMI
 */
void STB_AVEnableHDMIDecoding(void);

/**
 * @brief   Disables AV output to HDMI
 */
void STB_AVDisableHDMIDecoding(void);

/**
 * @brief   Returns whether HDCP has authenticated
 * @return  TRUE if authenticated, FALSE otherwise
 */
BOOLEAN STB_AVIsHDCPAuthenticated(void);

/**
 * @brief   Sets the codec to be used when decoding the next i-frame from memory
 * @param   path video path
 * @param   codec codec to be used
 * @return  TRUE if the codec is supported and is set correctly, FALSE otherwise
 */
BOOLEAN STB_AVSetIFrameCodec(U8BIT path, E_STB_AV_VIDEO_CODEC codec);

/**
 * @brief   Provides the video data for an i-frame for subsequent decode and display
 * @param   path video decode path to be used
 * @param   data i-frame data to be saved for decoding
 * @param   size size of the data in bytes
 */
void STB_AVLoadIFrame(U8BIT path, U8BIT *data, U32BIT size);

/**
 * @brief   Decodes and displays the previously loaded i-frame data
 * @param   path video path to use
 */
void STB_AVShowIFrame(U8BIT path);

/**
 * @brief   Hides the i-frame currently being displayed
 * @param   path video path to use
 */
void STB_AVHideIFrame(U8BIT path);

/**
 * @brief   Plays back a previously loaded audio sample
 * @param   path the audio path to use for playback
 * @param   loop_count the number of times to play the sample, 0=forever
 * @return  HW_OK on success, error code otherwise
 */
E_HW_STATUS STB_AVPlayAudioSample(U8BIT path, U32BIT loop_count);

/**
 * @brief   Loads an audio sample for subsequent playback
 * @param   path the decoder path to use for playback
 * @param   data the audio sample data to be loaded
 * @param   size the size of the audio sample in bytes
 * @return  HW_OK on success, error code otherwise
 */
E_HW_STATUS STB_AVLoadAudioSample(U8BIT path, U8BIT *data, U32BIT size);

/**
 * @brief   Pauses playback of an audio sample
 * @param   path Audio path on which to pause
 * @return  HW_OK on success, error code otherwise
 */
E_HW_STATUS STB_AVPauseAudioSample(U8BIT path);

/**
 * @brief   Resumes playback of an audio sample
 * @param   path Audio path on which to resume
 * @return  HW_OK on success, error code otherwise
 */
E_HW_STATUS STB_AVResumeAudioSample(U8BIT path);

/**
 * @brief   Stops playback of an audio sample
 * @param   path Audio path on which to stop
 */
void STB_AVStopAudioSample(U8BIT path);

/**
 * @brief   Sets the SPDIF output mode, PCM or compressed audio
 * @param   path decoder path
 * @param   spdif_type PCM or compressed
 */
void STB_AVSetSpdifMode(U8BIT path, E_STB_DIGITAL_AUDIO_TYPE audio_type);

/**
 * @brief   Sets the HDMI audio output mode, PCM or compressed
 * @param   path decoder path
 * @param   audio_type PCM or compressed
 */
void STB_AVSetHDMIAudioMode(U8BIT path, E_STB_DIGITAL_AUDIO_TYPE audio_type);

/**
 * @brief   Sets the audio delay on the given path
 * @param   path decoder path
 * @param   millisecond audio delay to be applied
 */
void STB_AVSetAudioDelay(U8BIT path, U16BIT millisecond);

/**
 * @brief   Register callback for updated video information
 * @param   path decoder path
 * @param   callback the callback to call when video information is updated
 * @param   user_data user data to pass to the callback
 */
void STB_AVSetVideoCallback(U8BIT path, void (*callback)(S_STB_AV_VIDEO_INFO *, void *, int), void *user_data);

/**
 * @brief   Apply video transformation
 * @param   path decoder path
 * @param   input input video rectangle
 * @param   output output video rectangle
 */
void STB_AVApplyVideoTransformation(U8BIT path, S_RECTANGLE *input, S_RECTANGLE *output);

/**
 * @brief   Returns minimum video play speed as a percentage.
 * @param   video_decoder video decoder path
 * @return  Minimum play speed.
 */
S16BIT STB_AVGetMinPlaySpeed(U8BIT video_decoder);

/**
 * @brief   Returns maximum video play speed as a percentage.
 * @param   video_decoder video decoder path
 * @return  Maximum play speed.
 */
S16BIT STB_AVGetMaxPlaySpeed(U8BIT video_decoder);

/**
 * @brief   Returns the next valid speed that is +/- inc above or below the
 *          given speed. Slow motion speeds (>-100% and < 100%) can be included.
 * @param   path Decode path
 * @param   speed Percentage speed above/below which the new speed is calculated
 * @param   inc number of speeds above that specified to return
 * @param   include_slow_speeds selects whether speeds >-100% and <100% are included
 * @return  Speed as a percentage
 */
S16BIT STB_AVGetNextPlaySpeed(U8BIT video_decoder, S16BIT speed, S16BIT inc,
                              BOOLEAN include_slow_speeds);

/**
 * @brief Apply the specified copy protection. This function is used for CI+
 * @param copy_protection
 */
void STB_AVSetCopyProtection(S_STB_AV_COPY_PROTECTION *copy_protection);

/**
 * @brief   Sets the output channel of the RF Modulator
 * @param   chan the UHF channel number
 */
void STB_AVSetUhfModulatorChannel(U8BIT chan);

/**
 * @brief   Gets the current RF modulator channel
 * @return  The RF frequency as a UHF channel number
 */
U8BIT STB_AVGetUhfModulatorChannel(void);

/**
 * @brief   Returns the frame rate of the video being decoded
 * @param   path video path
 * @return  video frame rate in frame per seconds
 */
U8BIT STB_AVGetVideoFrameRate(U8BIT path);

/**
 * @brief   Returns the width of the video being decoded
 * @param   path video path
 * @return  video width in frame per seconds
 */
U32BIT STB_AVGetVideoWidth(U8BIT path);

/**
 * @brief   Returns the height of the video being decoded
 * @param   path video path
 * @return  video height in frame per seconds
 */
U32BIT STB_AVGetVideoHeight(U8BIT path);

/**
 * @brief   Returns the scan type of the video being decoded
 * @param   path video path
 * @return  1: progressive, 0: interlaced, 255: invalid
 */
U8BIT STB_AVGetVideoScanType(U8BIT path);

/**
 * @brief   Apply System Renewability Message (SRM) to HDCP function
 * @param   path output path
 * @param   data SRM data
 * @param   len length of SRM data in bytes
 * @return  SRM_OK, SRM_BUSY or SRM_NOT_REQUIRED
 */
E_STB_AV_SRM_REPLY STB_AVApplySRM(U8BIT path, U8BIT *data, U32BIT len);

/**
 * @brief   Sets the decoding mode
 * @param   path decoder path
 * @param   mode decoding mode
 */
void STB_AVSetDecodingMode(U8BIT audio_decoder, U8BIT video_decoder, E_STB_DECODING_MODE mode);

void STB_AVCECOneTouchPlay(void);
E_STB_AV_VIDEO_FORMAT STB_AVGetHDMINativeFormat(void);
void STB_AVPlayMp3(U8BIT path, U8BIT *buffer, U32BIT buffer_size);
void STB_AVStopMp3(U8BIT path);

/**
 * @brief   Sync decoding info from pvr
 * @param   the audio decoder and/or video decoder used by pvr
 */
void STB_AVSyncDecodingFromPVR(U8BIT audio_decoder, U8BIT video_decoder);

void STB_AVNotifyEventHandler(U8BIT audio_path, U8BIT video_path, void *event, int64_t param);

pthread_rwlock_t * STB_AVGetLockByPath(U8BIT path);

S16BIT STB_AVGetAC4ActivePresentationsID(U8BIT path);

BOOLEAN STB_AVSetAudioLanguage(U8BIT path, U32BIT pri_language_code, U32BIT sec_language_code);
BOOLEAN STB_AVSetPlayerHandle(U8BIT audio_decoder, U8BIT video_decoder, size_t player_handle);
BOOLEAN STB_AVGetPlayerHandle(U8BIT audio_decoder, U8BIT video_decoder, size_t *player_handle);
BOOLEAN STB_AVResetWorkMode(void);
BOOLEAN STB_AVSetPlayIndex(U8BIT path, U8BIT index);

/**
 * @brief Registers CC callback function.
 *
 * @param path Decode path to get the closed caption data from.
 * @param context Context pointer to be forwarded to the callback calls.
 * @param callback Pointer to closed caption data ready callback function. NULL stops closed
 *                 caption data reporting.
 *
 * @return None.
 */
void STB_AVRegisterCcCallback(U8BIT path, void* context, CC_DATA_CALLBACK callback);


#ifdef RDK_COMPILE
/**
 * @brief   Set video window for specific video decoder
 * @param   path video path
 * @param   x The coordinate of the left side of the window
 * @param   y The coordinate of the top of the window
 * @param   w Window width
 * @param   h Window height
 * @return  TRUE if set video window correctly, FALSE otherwise
 */
BOOLEAN STB_AVStoreVideoWindow(U8BIT path, U16BIT x, U16BIT y, U16BIT w, U16BIT h);
#endif

#endif //  _STBHWAV_H
