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
 * @brief   Set Top Box - Hardware Layer, AV Control and decoding
 * @file    stbhwav.c
 * @date    October 2018
 */

/*---includes for this file--------------------------------------------------*/
/* compiler library header files */

/* third party header files */

/* STB header files */
#include "techtype.h"
#include "dbgfuncs.h"
#include "stbhwav.h"

/*---macro definitions for this file-----------------------------------------*/
//#define AV_DEBUG
//#define VIDEO_DEBUG
//#define AUDIO_DEBUG

#ifdef AV_DEBUG
   #define AV_DBG(x,...)   STB_SPDebugWrite("%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )
#else
   #define AV_DBG(x,...)
#endif

#ifdef VIDEO_DEBUG
   #define VID_DBG(x,...)  STB_SPDebugWrite("%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )
#else
   #define VID_DBG(x,...)
#endif

#ifdef AUDIO_DEBUG
   #define AUD_DBG(x,...)  STB_SPDebugWrite("%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )
#else
   #define AUD_DBG(x,...)
#endif

#define ERR_DBG(x,...)     STB_SPDebugWrite("%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )


/*---constant definitions for this file--------------------------------------*/

/*---local typedef structs for this file-------------------------------------*/

/*---local (static) variable declarations for this file----------------------*/

/*---local function prototypes for this file---------------------------------*/

/*---global function definitions----------------------------------------------*/

/**
 * @brief   Initialises the AV components
 * @param   audio_paths The number of audio paths
 * @param   video_paths The number of video paths
 */
void STB_AVInitialise(U8BIT audio_paths, U8BIT video_paths)
{
   FUNCTION_START(STB_AVInitialise);
   USE_UNWANTED_PARAM(audio_paths);
   USE_UNWANTED_PARAM(video_paths);
   FUNCTION_FINISH(STB_AVInitialise);
}

/**
 * @brief   Sets the aspect ratio of the connected television
 * @param   path The video path to be set
 * @param   ratio The aspect ratio of the tv
 * @param   format The signal format of the tv
 */
void STB_AVSetTVType(U8BIT path, E_STB_AV_ASPECT_RATIO ratio, E_STB_AV_VIDEO_FORMAT format)
{
   FUNCTION_START(STB_AVSetTVType);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(ratio);
   USE_UNWANTED_PARAM(format);
   FUNCTION_FINISH(STB_AVSetTVType);
}

/**
 * @brief   Set the aspect ratio setting of the tv
 * @param   path the video path
 * @param   ratio The requested aspect ratio setting of the tv
 * @return  Valid aspect ratio setting, only different from the parameter if that was invalid
 */
E_STB_AV_ASPECT_RATIO STB_TVSetAspectRatio(E_STB_AV_ASPECT_RATIO ratio)
{
   FUNCTION_START(STB_TVSetAspectRatio);
   USE_UNWANTED_PARAM(ratio);
   FUNCTION_FINISH(STB_TVSetAspectRatio);

   return ASPECT_UNDEFINED;
}

/**
 * @brief   Register callback for updated video information
 * @param   path video decoder path
 * @param   callback - the callback to call when video information is updated
 * @param   vtc_user_data - user data to pass to the callback
 */
void STB_AVSetVideoCallback(U8BIT path, void (*callback)(S_STB_AV_VIDEO_INFO *, void *),
   void *user_data)
{
   FUNCTION_START(STB_AVSetVideoCallback);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(user_data);
   FUNCTION_FINISH(STB_AVSetVideoCallback);
}

/**
 * @brief   Apply video transformation
 * @param   input - input video rectangle
 * @param   output - output video rectangle
 */
void STB_AVApplyVideoTransformation(U8BIT path, S_RECTANGLE* src, S_RECTANGLE* dest)
{
   FUNCTION_START(STB_AVApplyVideoTransformation);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(src);
   USE_UNWANTED_PARAM(dest);
   FUNCTION_FINISH(STB_AVApplyVideoTransformation);
}

/**
 * @brief   Blanks or unblanks the video display
 * @param   path the video path to be configured
 * @param   blank TRUE to blank, FALSE to unblank
 */
void STB_AVBlankVideo(U8BIT path, BOOLEAN blank)
{
   FUNCTION_START(STB_AVBlankVideo);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(blank);
   FUNCTION_FINISH(STB_AVBlankVideo);
}

/**
 * @brief   Routes a specified AV source to a specified AV output
 * @param   output The output to be connected
 * @param   source The source signal to be connected
 * @param   param parameter associated with the source
 */
void STB_AVSetAVOutputSource(E_STB_AV_OUTPUTS output, E_STB_AV_SOURCES source, U32BIT param)
{
   FUNCTION_START(STB_AVSetAVOutputSource);
   USE_UNWANTED_PARAM(output);
   USE_UNWANTED_PARAM(source);
   USE_UNWANTED_PARAM(param);
   FUNCTION_FINISH(STB_AVSetAVOutputSource);
}

/**
 * @brief   Turns on/off all AV outputs (e.g. for standby mode)
 */
void STB_AVSetAVOutput(BOOLEAN av_on)
{
   FUNCTION_START(STB_AVSetAVOutput);
   USE_UNWANTED_PARAM(av_on);
   FUNCTION_FINISH(STB_AVSetAVOutput);
}

/**
 * @brief   Sets the output channel of the RF Modulator
 * @param   chan the UHF channel number
 */
void STB_AVSetUhfModulatorChannel(U8BIT chan)
{
   FUNCTION_START(STB_AVSetUhfModulatorChannel);
   USE_UNWANTED_PARAM(chan);
   FUNCTION_FINISH(STB_AVSetUhfModulatorChannel);
}

/**
 * @brief   Gets the current RF modulator channel
 * @return  The RF frequency as a UHF channel number
 */
U8BIT STB_AVGetUhfModulatorChannel(void)
{
   FUNCTION_START(STB_AVGetUhfModulatorChannel);
   FUNCTION_FINISH(STB_AVGetUhfModulatorChannel);
   return 0;
}

/**
 * @brief   Sets the volume of the audio output
 * @param   path the audio path to be configured
 * @param   vol the audio volume (0-100%)
 */
void STB_AVSetAudioVolume(U8BIT path, U8BIT vol)
{
   FUNCTION_START(STB_AVSetAudioVolume);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(vol);
   FUNCTION_FINISH(STB_AVSetAudioVolume);
}

/**
 * @brief   Gets the current volume of the audio output
 * @param   path The audio path to query
 * @return  audio volume (0-100%)
 */
U8BIT STB_AVGetAudioVolume(U8BIT path)
{
   FUNCTION_START(STB_AVGetAudioVolume);
   USE_UNWANTED_PARAM(path);
   FUNCTION_FINISH(STB_AVGetAudioVolume);

   return 0;
}

/**
 * @brief   Mutes or unmutes the audio output
 * @param   path The audio path to be configured
 * @param   mute TRUE to mute, FALSE to unmute
 */
void STB_AVSetAudioMute(U8BIT path, BOOLEAN mute)
{
   FUNCTION_START(STB_AVSetAudioMute);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(mute);
   FUNCTION_FINISH(STB_AVSetAudioMute);
}

/**
 * @brief   Gets the current muting status of the audio output
 * @param   path the audio path to be queried
 * @return  TRUE audio is muted, FALSE otherwise
 */
BOOLEAN STB_AVGetAudioMute(U8BIT path)
{
   FUNCTION_START(STB_AVGetAudioMute);
   USE_UNWANTED_PARAM(path);
   FUNCTION_FINISH(STB_AVGetAudioMute);

   return(FALSE);
}

/**
 * @brief   Configures the audio channel mode (stereo/left/right)
 * @param   path the audio path to be configured
 * @param   mode the audio mode to use
 */
void STB_AVChangeAudioMode(U8BIT path, E_STB_AV_AUDIO_MODE mode)
{
   FUNCTION_START(STB_AVChangeAudioMode);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(mode);
   FUNCTION_FINISH(STB_AVChangeAudioMode);
}

/**
 * @brief   Starts the Audio decoder
 * @param   path the audio decoder path to be started
 */
void STB_AVStartAudioDecoding(U8BIT path)
{
   FUNCTION_START(STB_AVStartAudioDecoding);
   USE_UNWANTED_PARAM(path);
   FUNCTION_FINISH(STB_AVStartAudioDecoding);
}

/**
 * @brief   Starts the video decoder
 * @param   path the video decode path to be started
 */
void STB_AVStartVideoDecoding(U8BIT path)
{
   FUNCTION_START(STB_AVStartVideoDecoding);
   USE_UNWANTED_PARAM(path);
   FUNCTION_FINISH(STB_AVStartVideoDecoding);
}

/**
 * @brief   Pause video decoding
 * @param   path Required Decode Path Number.
 */
void  STB_AVPauseVideoDecoding(U8BIT path)
{
   FUNCTION_START(STB_AVPauseVideoDecoding);
   USE_UNWANTED_PARAM(path);
   FUNCTION_FINISH(STB_AVPauseVideoDecoding);
}

/**
 * @brief   Resume video decoding
 * @param   path Required Decode Path Number.
 */
void  STB_AVResumeVideoDecoding(U8BIT path)
{
   FUNCTION_START(STB_AVResumeVideoDecoding);
   USE_UNWANTED_PARAM(path);
   FUNCTION_FINISH(STB_AVResumeVideoDecoding);
}

/**
 * @brief   Stops the video decoder
 * @param   path the video decoder path to be stopped
 */
void STB_AVStopVideoDecoding(U8BIT path)
{
   FUNCTION_START(STB_AVStopVideoDecoding);
   USE_UNWANTED_PARAM(path);
   FUNCTION_FINISH(STB_AVStopVideoDecoding);
}

/**
 * @brief   Stops the audio decoder
 * @param   path the audio decoder path to be stopped
 */
void STB_AVStopAudioDecoding(U8BIT path)
{
   FUNCTION_START(STB_AVStopAudioDecoding);
   USE_UNWANTED_PARAM(path);
   FUNCTION_FINISH(STB_AVStopAudioDecoding);
}

/**
 * @brief   Returns the current 33-bit System Time Clock from the PCR PES.
 *          On some systems, this information may need to be obtained from the associated demux,
 *          which will be contained in the 'param' value when STB_AVSetVideoSource is called.
 * @param   path video path
 * @param   stc an array in which the STC will be returned, ordered such that
 *                stc[0] contains the MS bit (33) of the STC value and stc[4]
 *                contains the LS bits (0-7).
 */
void STB_AVGetSTC(U8BIT path, U8BIT stc[5])
{
   FUNCTION_START(STB_AVGetSTC);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(stc);
   FUNCTION_FINISH(STB_AVGetSTC);
}

/**
 * @brief   Sets the source of the video decoder
 * @param   path video path to configure
 * @param   source the source device to use
 * @param   param tuner or demux number
 */
void STB_AVSetVideoSource(U8BIT path, E_STB_AV_DECODE_SOURCE source, U32BIT param)
{
   FUNCTION_START(STB_AVSetVideoSource);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(source);
   USE_UNWANTED_PARAM(param);
   FUNCTION_FINISH(STB_AVSetVideoSource);
}

/**
 * @brief   Sets the source of the audio decoder
 * @param   path audio path to configure
 * @param   source the source device to use
 * @param   param tuner or demux number
 */
void STB_AVSetAudioSource(U8BIT path, E_STB_AV_DECODE_SOURCE source, U32BIT param)
{
   FUNCTION_START(STB_AVSetAudioSource);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(source);
   USE_UNWANTED_PARAM(param);
   FUNCTION_FINISH(STB_AVSetAudioSource);
}

/**
 * @brief   Sets the video codec to be used when decoding video with the given video decoder path
 * @param   path video path
 * @param   codec codec to be used
 * @return  TRUE if the codec is supported and is set correctly, FALSE otherwise
 */
BOOLEAN STB_AVSetVideoCodec(U8BIT path, E_STB_AV_VIDEO_CODEC codec)
{
   FUNCTION_START(STB_AVSetVideoCodec);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(codec);
   FUNCTION_FINISH(STB_AVSetAudioSource);

   return FALSE;
}

/**
 * @brief   Sets the audio codec to be used when decoding audio with the given audio decoder path
 * @param   path audio path
 * @param   codec codec to be used
 * @return  TRUE if the codec is supported and is set correctly, FALSE otherwise
 */
BOOLEAN STB_AVSetAudioCodec(U8BIT path, E_STB_AV_AUDIO_CODEC codec)
{
   FUNCTION_START(STB_AVSetVideoCodec);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(codec);
   FUNCTION_FINISH(STB_AVSetAudioSource);

   return FALSE;
}

/**
 * @brief   Loads an audio sample for subsequent playback
 * @param   path the decoder path to use for playback
 * @param   data the audio sample data to be loaded
 * @param   size the size of the audio sample in bytes
 * @return  E_HW_STATUS code
 */
E_HW_STATUS STB_AVLoadAudioSample(U8BIT path, U8BIT *data, U32BIT size)
{
   FUNCTION_START(STB_AVLoadAudioSample);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(data);
   USE_UNWANTED_PARAM(size);
   FUNCTION_FINISH(STB_AVLoadAudioSample);

   return HW_GEN_ERROR;
}

/**
 * @brief   Plays back a previously loaded audio sample
 * @param   path the audio path to use for playback
 * @param   loop_count the number of times to play the sample, 0=forever
 * @return  E_HW_STATUS code
 */
E_HW_STATUS STB_AVPlayAudioSample(U8BIT path, U32BIT loop_count)
{
   FUNCTION_START(STB_AVPlayAudioSample);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(loop_count);
   FUNCTION_FINISH(STB_AVPlayAudioSample);

   return HW_GEN_ERROR;
}

/**
 * @brief   Pauses playback of an audio sample
 * @param   path Audio path on which to pause
 * @return  E_HW_STATUS code
 */
E_HW_STATUS STB_AVPauseAudioSample(U8BIT path)
{
   FUNCTION_START(STB_AVPauseAudioSample);
   USE_UNWANTED_PARAM(path);
   FUNCTION_FINISH(STB_AVPauseAudioSample);

   return HW_GEN_ERROR;
}

/**
 * @brief   Resumes playback of an audio sample
 * @param   path Audio path on which to resume
 * @return  E_HW_STATUS code
 */
E_HW_STATUS STB_AVResumeAudioSample(U8BIT path)
{
   FUNCTION_START(STB_AVResumeAudioSample);
   USE_UNWANTED_PARAM(path);
   FUNCTION_FINISH(STB_AVResumeAudioSample);

   return HW_GEN_ERROR;
}

/**
 * @brief   Stops playback of an audio sample
 * @param   path Audio path on which to stop
 */
void STB_AVStopAudioSample(U8BIT path)
{
   FUNCTION_START(STB_AVStopAudioSample);
   USE_UNWANTED_PARAM(path);
   FUNCTION_FINISH(STB_AVStopAudioSample);
}

/**
 * @brief   Sets the codec to be used when decoding the next i-frame from memory
 * @param   path video path
 * @param   codec codec to be used
 * @return  TRUE if the codec is supported and is set correctly, FALSE otherwise
 */
BOOLEAN STB_AVSetIFrameCodec(U8BIT path, E_STB_AV_VIDEO_CODEC codec)
{
   FUNCTION_START(STB_AVSetIFrameCodec);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(codec);
   FUNCTION_FINISH(STB_AVSetIFrameCodec);
   return(FALSE);
}

/**
 * @brief   Loads a video I Frame for subsequent decode and display
 * @param   path the video decode path to be used
 * @param   data the I frame data to be loaded
 * @param   size the size of the data in bytes
 */
void STB_AVLoadIFrame(U8BIT path, U8BIT *data, U32BIT size)
{
   FUNCTION_START(STB_AVLoadIFrame);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(data);
   USE_UNWANTED_PARAM(size);
   FUNCTION_FINISH(STB_AVLoadIFrame);
}

/**
 * @brief   Decode and display previously loaded I frame data
 * @param   path the video path to use
 */
void STB_AVShowIFrame(U8BIT path)
{
   FUNCTION_START(STB_AVShowIFrame);
   USE_UNWANTED_PARAM(path);
   FUNCTION_FINISH(STB_AVShowIFrame);
}

/**
 * @brief   Hides a previously shown I frame
 * @param   path the video path containing the I frame
 */
void STB_AVHideIFrame(U8BIT path)
{
   FUNCTION_START(STB_AVHideIFrame);
   USE_UNWANTED_PARAM(path);
   FUNCTION_FINISH(STB_AVHideIFrame);
}

/**
 * @brief   Returns minimum video play speed as a percentage.
 * @param   video_decoder video decoder path
 * @return  Minimum play speed.
 */
S16BIT STB_AVGetMinPlaySpeed(U8BIT path)
{
   FUNCTION_START(STB_AVGetMinPlaySpeed);
   USE_UNWANTED_PARAM(path);
   FUNCTION_FINISH(STB_AVGetMinPlaySpeed);

   return(0);
}

/**
 * @brief   Returns maximum video play speed as a percentage.
 * @param   video_decoder video decoder path
 * @return  Maximum play speed.
 */
S16BIT STB_AVGetMaxPlaySpeed(U8BIT path)
{
   FUNCTION_START(STB_AVGetMinPlaySpeed);
   USE_UNWANTED_PARAM(path);
   FUNCTION_FINISH(STB_AVGetMinPlaySpeed);

   return(0);
}

/**
 * @brief   Returns the next valid speed that is +/- inc above or below the
 *          given speed. Slow motion speeds (>-100% and < 100%) can be included.
 * @param   path Decode path
 * @param   speed Percentage speed above/below which the new speed is calculated
 * @param   inc number of speeds above that specified to return
 * @param   include_slow_speeds selects whether speeds >-100% and <100% are included
 * @return  Speed as a percentage
 */
S16BIT STB_AVGetNextPlaySpeed(U8BIT path, S16BIT speed, S16BIT inc, BOOLEAN include_slow_speeds)
{
   FUNCTION_START(STB_AVGetNextPlaySpeed);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(speed);
   USE_UNWANTED_PARAM(inc);
   USE_UNWANTED_PARAM(include_slow_speeds);
   FUNCTION_FINISH(STB_AVGetNextPlaySpeed);

   return(0);
}

/**
 * @brief     Sets the source of the audio decoder
 * @param     path - audio path to configure
 * @param     source - the source device to use
 * @param     param - tuner or demux number
 */
void STB_AVSetADSource(U8BIT path, E_STB_AV_DECODE_SOURCE source, U32BIT param)
{
   FUNCTION_START(STB_AVSetADSource);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(source);
   USE_UNWANTED_PARAM(param);
   FUNCTION_FINISH(STB_AVSetADSource);
}

/**
 * @brief   Configures the audio description channel mode (stereo/left/right) in the case where
 *          dual-mono audio is used, such that only the audio from one channel is heard.
 * @param   path audio path to be configured
 * @param   mode audio mode to use
 */
void STB_AVChangeADMode(U8BIT path, E_STB_AV_AUDIO_MODE mode)
{
   FUNCTION_START(STB_AVChangeADMode);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(mode);
   FUNCTION_FINISH(STB_AVChangeADMode);
}

/**
 * @brief   Starts decoding audio description on the given audio path
 * @param   path audio decoder path to be started
 */
void STB_AVStartADDecoding(U8BIT path)
{
   FUNCTION_START(STB_AVStartADDecoding);
   USE_UNWANTED_PARAM(path);
   FUNCTION_FINISH(STB_AVStartADDecoding);
}

/**
 * @brief   Stops decoding audio description on the given audio path.
 * @param   path audio decoder path to be stopped
 */
void STB_AVStopADDecoding(U8BIT path)
{
   FUNCTION_START(STB_AVStopADDecoding);
   USE_UNWANTED_PARAM(path);
   FUNCTION_FINISH(STB_AVStopADDecoding);
}

/**
 * @brief   Sets the codec to be used for audio description when decoding audio
 *          with the given audio decoder path
 * @param   path audio path
 * @param   codec codec to be used
 * @return  TRUE if the codec is supported and is set correctly, FALSE otherwise
 */
BOOLEAN STB_AVSetADCodec(U8BIT path, E_STB_AV_AUDIO_CODEC codec)
{
   FUNCTION_START(STB_AVSetADCodec);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(codec);
   FUNCTION_FINISH(STB_AVSetADCodec);

   return(FALSE);
}

/**
 * @brief   Sets the volume of the audio description output
 * @param   path audio path to be configured
 * @param   vol audio volume (0-100%)
 */
void STB_AVSetADVolume(U8BIT path, U8BIT vol)
{
   FUNCTION_START(STB_AVSetADVolume);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(vol);
   FUNCTION_FINISH(STB_AVSetADVolume);
}

/**
 * @brief   Sets the standby state of the HDMI output
 * @param   standby TRUE to put the HDMI in standby, FALSE to come out of standby
 */
void STB_AVSetHDMIStandby(BOOLEAN standby)
{
   FUNCTION_START(STB_AVSetHDMIStandby);
   USE_UNWANTED_PARAM(standby);
   FUNCTION_FINISH(STB_AVSetHDMIStandby);
}

/**
 * @brief   Returns the resolutions supported by the HDMI
 * @param   modes array of supported modes
 * @return  number of supported modes
 */
U8BIT STB_AVGetHDMISupportedModes(E_STB_AV_VIDEO_FORMAT **modes)
{
   FUNCTION_START(STB_AVGetHDMISupportedModes);
   USE_UNWANTED_PARAM(modes);
   FUNCTION_FINISH(STB_AVGetHDMISupportedModes);
   return(0);
}


/**
 * @brief   Enables AV output to HDMI
 */
void STB_AVEnableHDMIDecoding(void)
{
   FUNCTION_START(STB_AVEnableHDMIDecoding);
   FUNCTION_FINISH(STB_AVEnableHDMIDecoding);
}

/**
 * @brief   Disables AV output to HDMI
 */
void STB_AVDisableHDMIDecoding(void)
{
   FUNCTION_START(STB_AVDisableHDMIDecoding);
   FUNCTION_FINISH(STB_AVDisableHDMIDecoding);
}

/**
 * @brief   Returns whether HDCP has authenticated
 * @return  TRUE if authenticated, FALSE otherwise
 */
BOOLEAN STB_AVIsHDCPAuthenticated(void)
{
   FUNCTION_START(STB_AVIsHDCPAuthenticated);
   FUNCTION_FINISH(STB_AVIsHDCPAuthenticated);
   return (FALSE);
}

/**
 * @brief   Sets the audio delay on the given path
 * @param   path decoder path
 * @param   millisecond audio delay to be applied
 */
void STB_AVSetAudioDelay(U8BIT path, U16BIT millisecond)
{
   FUNCTION_START(STB_AVSetAudioDelay);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(millisecond);
   FUNCTION_FINISH(STB_AVSetAudioDelay);
}


/**
 * @brief   Sets the SPDIF output mode, PCM or compressed audio
 * @param   path decoder path
 * @param   audio_type PCM or compressed
 */
void STB_AVSetSpdifMode(U8BIT path, E_STB_DIGITAL_AUDIO_TYPE audio_type)
{
   FUNCTION_START(STB_AVSetSpdifMode);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(audio_type);
   FUNCTION_FINISH(STB_AVSetSpdifMode);
}

/**
 * @brief   Returns the current size of the screen in pixels
 * @param   path output path
 * @param   width returned width
 * @param   height returned height
 */
void STB_AVGetScreenSize(U8BIT path, U16BIT *width, U16BIT *height)
{
   FUNCTION_START(STB_AVGetScreenSize);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(width);
   USE_UNWANTED_PARAM(height);
   FUNCTION_FINISH(STB_AVGetScreenSize);
}

/**
 * @brief   Sets the HDMI audio output mode, PCM or compressed
 * @param   path decoder path
 * @param   audio_type PCM or compressed
 */
void STB_AVSetHDMIAudioMode(U8BIT path, E_STB_DIGITAL_AUDIO_TYPE audio_type)
{
   FUNCTION_START(STB_AVSetHDMIAudioMode);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(audio_type);
   FUNCTION_FINISH(STB_AVSetHDMIAudioMode);
}

/**
 * @brief   Returns the native resolution, i.e. the resolution that is
 *          set with a call to STB_AVSetTVType(path, ratio, VIDEO_FORMAT_AUTO);
 * @param   width pointer to the variable where to store the width
 * @param   height pointer to the variable where to store the height
 */
void STB_AVGetHDMINativeResolution(U16BIT *width, U16BIT *height)
{
   FUNCTION_START(STB_AVGetHDMINativeResolution);
   USE_UNWANTED_PARAM(width);
   USE_UNWANTED_PARAM(height);
   FUNCTION_FINISH(STB_AVGetHDMINativeResolution);
}

/**
 * @brief   Apply System Renewability Message (SRM) to HDCP function
 * @param   path output path
 * @param   data SRM data
 * @param   len length of SRM data in bytes
 * @return  SRM_OK, SRM_BUSY or SRM_NOT_REQUIRED
 */
E_STB_AV_SRM_REPLY STB_AVApplySRM(U8BIT path, U8BIT *data, U32BIT len)
{
   FUNCTION_START(STB_AVApplySRM);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(data);
   USE_UNWANTED_PARAM(len);
   FUNCTION_FINISH(STB_AVApplySRM);

   return SRM_NOT_REQUIRED;
}

/**
 * @brief   Returns the frame rate of the video being decoded
 * @param   path video path
 * @return  video frame rate in frame per seconds
 */
U8BIT STB_AVGetVideoFrameRate(U8BIT path)
{
   FUNCTION_START(STB_AVGetVideoFrameRate);
   USE_UNWANTED_PARAM(path);
   FUNCTION_FINISH(STB_AVGetVideoFrameRate);
   return 0;
}



/**
 * @brief Apply the specified copy protection. This function is used for CI+
 * @param copy_protection  - settings to be used for each output
 */
void STB_AVSetCopyProtection(S_STB_AV_COPY_PROTECTION *copy_protection)
{
   FUNCTION_START(STB_AVSetCopyProtection);
   USE_UNWANTED_PARAM(copy_protection);
   FUNCTION_FINISH(STB_AVSetCopyProtection);
}

/*---local function definitions----------------------------------------------*/


/***
 * End of file
 */
