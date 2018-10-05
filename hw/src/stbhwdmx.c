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
 * @brief   Set Top Box - Hardware Layer, STB Demux Functions
 * @file    stbhwdmx.c
 * @date    October 2018
 */

//#define DEMUX_DEBUG

/*---includes for this file---------------------------------------------------*/
/* compiler library header files */

/* third party header files */

/* STB header files */
#include "techtype.h"
#include "dbgfuncs.h"
#include "stbhwdmx.h"


/*---constant definitions for this file--------------------------------------*/
#define DMX_ERR(x,...)        STB_SPDebugWrite("%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )

#ifdef DEMUX_DEBUG
#define DMX_DBG(x,...)        STB_SPDebugWrite("%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )
#else
#define DMX_DBG(x,...)
#endif

/* Local ENUM/TYPE Definitions */

/*---local (static) variable declarations for this file----------------------*/

/*---local function prototypes for this file---------------------------------*/

/*---global function definitions---------------------------------------------*/


/**
 * @brief   Initialises the demux / programmable transport interface
 * @param   paths Number of demux paths to be initialised
 * @param   inc_pes_collection Not used
 */
void STB_DMXInitialise(U8BIT paths, BOOLEAN inc_pes_collection)
{
   FUNCTION_START(STB_DMXInitialise);
   USE_UNWANTED_PARAM(paths);
   USE_UNWANTED_PARAM(inc_pes_collection);
   FUNCTION_FINISH(STB_DMXInitialise);
}

/**
 * @brief   Returns the capability flags of the given demux
 * @param   path - demux
 * @return  Capability flags
 */
U16BIT STB_DMXGetCapabilities(U8BIT path)
{
   FUNCTION_START(STB_DMXGetCapabilities);
   USE_UNWANTED_PARAM(path);
   FUNCTION_FINISH(STB_DMXGetCapabilities);
   return(0);
}

/**
 * @brief   Changes the packet IDs for the PCR Video, Audio, Text and Data
 * @param   path The demux path to be configured
 * @param   pcr_pid The PID to use for the Program Clock Reference
 * @param   video_pid The PID to use for the Video PES
 * @param   audio_pid The PID to use for the Audio PES
 * @param   text_pid The PID to use for the Teletext data
 * @param   data_pid The PID to use for the data
 */
void STB_DMXChangeDecodePIDs(U8BIT path, U16BIT pcr_pid, U16BIT video_pid, U16BIT audio_pid,
   U16BIT text_pid, U16BIT data_pid, U16BIT ad_pid)
{
   FUNCTION_START(STB_DMXChangeDecodePIDs);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(pcr_pid);
   USE_UNWANTED_PARAM(video_pid);
   USE_UNWANTED_PARAM(audio_pid);
   USE_UNWANTED_PARAM(text_pid);
   USE_UNWANTED_PARAM(data_pid);
   USE_UNWANTED_PARAM(ad_pid);
   FUNCTION_FINISH(STB_DMXChangeDecodePIDs);
}

/**
 * @brief   Changes just the teletext PID
 * @param   path The demux path to configure
 * @param   text_pid The PID to use for the teletext data
 */
void STB_DMXChangeTextPID(U8BIT path, U16BIT text_pid)
{
   FUNCTION_START(STB_DMXChangeTextPID);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(text_pid);
   FUNCTION_FINISH(STB_DMXChangeTextPID);
}

/**
 * @brief   Get a New PID Filter & Setup Associated Buffer and Callback
 *          Function Address.
 * @param   path Required Decode Path Number.
 * @param   pid Required PID to Demux.
 * @param   func_ptr User's Interrupt Procedure Function Address.
 * @return  New PID filter identifier or invalid id.
 */
U16BIT  STB_DMXGrabPIDFilter(U8BIT path, U16BIT pid, FILTER_CALLBACK func_ptr)
{
   FUNCTION_START(STB_DMXGrabPIDFilter);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(pid);
   USE_UNWANTED_PARAM(func_ptr);
   FUNCTION_FINISH(STB_DMXGrabPIDFilter);

   return(0);
}

/**
 * @brief   Releases a previously allocated PID filter
 * @param   path the demux path of the filter
 * @param   pfilt_id the handle of the filter
 */
void STB_DMXReleasePIDFilter(U8BIT path, U16BIT pfilt_id)
{
   FUNCTION_START(STB_DMXReleasePIDFilter);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(pfilt_id);
   FUNCTION_FINISH(STB_DMXReleasePIDFilter);
}

/**
 * @brief   Allocated a new section filter on the specified PID filter
 * @param   path the demux path to use
 * @param   pfilt_id the PID filter to assign the section filter to
 * @return  The section filter handle
 */
U16BIT STB_DMXGrabSectFilter(U8BIT path, U16BIT pfilt_id)
{
   FUNCTION_START(STB_DMXGrabSectFilter);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(pfilt_id);
   FUNCTION_FINISH(STB_DMXGrabSectFilter);

   return(0);
}

/**
 * @brief   Releases a previously allocated section filter
 * @param   path the demux path of the filter
 * @param   sfilt_id the handle of the section filter
 */
void STB_DMXReleaseSectFilter(U8BIT path, U16BIT sfilt_id)
{
   FUNCTION_START(STB_DMXReleaseSectFilter);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(sfilt_id);
   FUNCTION_FINISH(STB_DMXReleaseSectFilter);
}

/**
 * @brief   Configures a match and mask for a specified section filter
 * @param   path the demux path of the section filter
 * @param   sfilt_id the handle of the section filter
 * @param   match_ptr pointer to the match bytes
 * @param   mask_ptr pointer to the mask bytes
 * @param   not_equal_byte_index the byte position for a not equal compare
 * @param   crc TRUE to use CRC checking FALSE to ignore
 */
void STB_DMXSetupSectFilter(U8BIT path, U16BIT sfilt_id, U8BIT *match_ptr, U8BIT *mask_ptr,
   U8BIT not_equal_byte_index, BOOLEAN crc)
{
   FUNCTION_START(STB_DMXSetupSectFilter);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(sfilt_id);
   USE_UNWANTED_PARAM(match_ptr);
   USE_UNWANTED_PARAM(mask_ptr);
   USE_UNWANTED_PARAM(not_equal_byte_index);
   USE_UNWANTED_PARAM(crc);
   FUNCTION_FINISH(STB_DMXSetupSectFilter);
}

/**
 * @brief   Start Specified PID Filter Collecting Data.
 * @param   path Required Decode Path Number.
 * @param   pfilter_id Required PID Filter Identifier.
 */
void  STB_DMXStartPIDFilter(U8BIT path, U16BIT pfilt_id)
{
   FUNCTION_START(STB_DMXStartPIDFilter);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(pfilt_id);
   FUNCTION_FINISH(STB_DMXStartPIDFilter);
}

/**
 * @brief   Stop Specified PID Filter Collecting Data.
 * @param   path Required Decode Path Number.
 * @param   pfilter_id Required PID Filter Identifier.
 */
void  STB_DMXStopPIDFilter(U8BIT path, U16BIT pfilt_id)
{
   FUNCTION_START(STB_DMXStopPIDFilter);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(pfilt_id);
   FUNCTION_FINISH(STB_DMXStopPIDFilter);
}

/**
 * @brief   Copies a filtered section to caller's buffer
 * @param   path the demux path to use
 * @param   buffer the caller's buffer
 * @param   size the size of the caller's buffer
 * @param   pfilt_id the handle of the PID filter to read from
 * @return  TRUE copied ok
 * @return  FALSE no data to copy
 */
BOOLEAN STB_DMXCopyPIDFilterSect(U8BIT path, U8BIT *buffer, U16BIT size, U16BIT pfilt_id)
{
   FUNCTION_START(STB_DMXCopyPIDFilterSect);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(buffer);
   USE_UNWANTED_PARAM(size);
   USE_UNWANTED_PARAM(pfilt_id);
   FUNCTION_FINISH(STB_DMXCopyPIDFilterSect);

   return FALSE;
}

/**
 * @brief   Flushes (emDMXes) the buffer of a speficied PID filter
 * @param   path the demux path of the filter
 * @param   pfilt_id the handle of the PID filter
 */
void STB_DMXFlushPIDFilterBuffer(U8BIT path, U16BIT pfilt_id)
{
   FUNCTION_START(STB_DMXFlushPIDFilterBuffer);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(pfilt_id);
   FUNCTION_FINISH(STB_DMXFlushPIDFilterBuffer);
}

/**
 * @brief   Skips (discards) a section in the PID filter buffer
 * @param   path the demux path of the filter
 * @param   pfilt_id the PID filter handle
 */
void STB_DMXSkipPIDFilterSect(U8BIT path, U16BIT pfilt_id)
{
   FUNCTION_START(STB_DMXSkipPIDFilterSect);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(pfilt_id);
   FUNCTION_FINISH(STB_DMXSkipPIDFilterSect);
}

/**
 * @brief   Returns the maximum number of section filters available on this hw
 * @return  The number of filters
 */
U8BIT STB_DMXGetMaxSectionFilters(void)
{
   FUNCTION_START(STB_DMXGetMaxSectionFilters);
   FUNCTION_FINISH(STB_DMXGetMaxSectionFilters);

   return 0;
}

/**
 * @brief   Configures the source of the demux
 * @param   path the demux path to configure
 * @param   source the source to use
 * @param   param source specific parameters (e.g. tuner number)
 */
void STB_DMXSetDemuxSource(U8BIT path, E_STB_DMX_DEMUX_SOURCE source, U8BIT param)
{
   FUNCTION_START(STB_DMXSetDemuxSource);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(source);
   USE_UNWANTED_PARAM(param);
   FUNCTION_FINISH(STB_DMXSetDemuxSource);
}

/**
 * @brief   Gets the current source of a given demux
 * @param   path the demux path to query
 * @param   source the source of the demux
 * @param   param the source specific parameter (e.g. tuner number)
 */
void STB_DMXGetDemuxSource(U8BIT path, E_STB_DMX_DEMUX_SOURCE *source, U8BIT *param)
{
   FUNCTION_START(STB_DMXGetDemuxSource);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(source);
   USE_UNWANTED_PARAM(param);
   FUNCTION_FINISH(STB_DMXGetDemuxSource);
}

/**
 * @brief   Reads Teletext PES data from the demux
 * @param   path the demux path to read
 * @param   buffer pointer to PES data
 * @param   num_bytes the number of bytes of data
 */
void STB_DMXReadTextPES(U8BIT path, U8BIT **buffer, U32BIT *num_bytes)
{
   FUNCTION_START(STB_DMXReadTextPES);
   USE_UNWANTED_PARAM(path);

   *num_bytes = 0;
   *buffer = NULL;

   FUNCTION_FINISH(STB_DMXReadTextPES);
}

/**
 * @brief   Writes data to the demux from memory
 * @param   path the demux path to be written
 * @param   data the data to be written
 * @param   size the number of bytes to be written
 */
void STB_DMXWriteDemux(U8BIT path, U8BIT *data, U32BIT size)
{
   FUNCTION_START(STB_DMXWriteDemux);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(data);
   USE_UNWANTED_PARAM(size);
   FUNCTION_FINISH(STB_DMXWriteDemux);
}

/**
 * @brief   Acquires a descrambler for the specified track on this path
 * @param   path the demux path for which the descrambler is acquired
 * @param   track enum representing audio, video or subtitles PES
 * @return  TRUE on success, FALSE otherwise
 */
BOOLEAN STB_DMXGetDescramblerKey(U8BIT path, E_STB_DMX_DESC_TRACK track)
{
   FUNCTION_START(STB_DMXGetDescramblerKey);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(track);
   FUNCTION_FINISH(STB_DMXGetDescramblerKey);
   return(FALSE);
}

/**
 * @brief   Frees the descrambler for the specified track on this path
 * @param   path the demux path for which the descrambler is freed
 * @param   track enum representing audio, video or subtitles PES
 * @return  TRUE on success, FALSE otherwise
 */
BOOLEAN STB_DMXFreeDescramblerKey(U8BIT path, E_STB_DMX_DESC_TRACK track)
{
   FUNCTION_START(STB_DMXFreeDescramblerKey);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(track);
   FUNCTION_FINISH(STB_DMXFreeDescramblerKey);
   return(FALSE);
}

/**
 * @brief   Set the descrambler key data for the specified track on this path
 * @param   path the demux path for which the descrambler key data is set
 * @param   track enum representing audio, video or subtitles PES
 * @param   parity even or odd
 * @param   data pointer to the key data, its length depends on the descrambler
 *          type (see STB_DMXSetDescramblerType)
 * @return  TRUE on success, FALSE otherwise
 */
BOOLEAN STB_DMXSetDescramblerKeyData(U8BIT path, E_STB_DMX_DESC_TRACK track,
   E_STB_DMX_DESC_KEY_PARITY parity, U8BIT *data)
{
   FUNCTION_START(STB_DMXSetDescramblerKeyData);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(track);
   USE_UNWANTED_PARAM(parity);
   USE_UNWANTED_PARAM(data);
   FUNCTION_FINISH(STB_DMXSetDescramblerKeyData);
   return(FALSE);
}

/**
 * @brief   Get the descrambler key usage for the specified track on this path as set by
 *          STB_DMXSetKeyUsage
 * @param   path the demux path that the descrambler key usage refers to
 * @param   track enum representing audio, video or subtitles PES
 * @param   key_usage whether the descrambler has been set to operate at PES level, transport
 *          level or all.
 * @return  TRUE on success, FALSE otherwise
 */
BOOLEAN STB_DMXGetKeyUsage(U8BIT path, E_STB_DMX_DESC_TRACK track, E_STB_DMX_KEY_USAGE *key_usage)
{
   FUNCTION_START(STB_DMXGetKeyUsage);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(track);
   USE_UNWANTED_PARAM(key_usage);
   FUNCTION_FINISH(STB_DMXGetKeyUsage);
   return(FALSE);
}

/**
 * @brief   Set the descrambler key usage for the specified track on this path
 * @param   path the demux path that the descrambler key usage refers to
 * @param   track enum representing audio, video or subtitles PES
 * @param   key_usage whether the descrambler operates at PES level, transport
 *          level or all.
 * @return  TRUE on success, FALSE otherwise
 */
BOOLEAN STB_DMXSetKeyUsage(U8BIT path, E_STB_DMX_DESC_TRACK track, E_STB_DMX_KEY_USAGE key_usage)
{
   FUNCTION_START(STB_DMXSetKeyUsage);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(track);
   USE_UNWANTED_PARAM(key_usage);
   FUNCTION_FINISH(STB_DMXSetKeyUsage);
   return(FALSE);
}

/**
 * @brief   Get the descrambler type for the specified track on this path, as set by
 *          STB_DMXSetDescramblerType
 * @param   path the demux path that the descrambler type refers to
 * @param   type descrambler type (DES, AES, etc...)
 * @return  TRUE on success, FALSE otherwise
 */
BOOLEAN STB_DMXGetDescramblerType(U8BIT path, E_STB_DMX_DESC_TRACK track, E_STB_DMX_DESC_TYPE *type)
{
   FUNCTION_START(STB_DMXGetDescramblerType);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(track);
   USE_UNWANTED_PARAM(type);
   FUNCTION_FINISH(STB_DMXGetDescramblerType);
   return(FALSE);
}

/**
 * @brief   Set the descrambler type for the specified track on this path
 * @param   path the demux path that the descrambler type refers to
 * @param   type descrambler type (DES, AES, etc...)
 * @return  TRUE on success, FALSE otherwise
 */
BOOLEAN STB_DMXSetDescramblerType(U8BIT path, E_STB_DMX_DESC_TRACK track, E_STB_DMX_DESC_TYPE type)
{
   FUNCTION_START(STB_DMXSetDescramblerType);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(track);
   USE_UNWANTED_PARAM(type);
   FUNCTION_FINISH(STB_DMXSetDescramblerType);
   return(FALSE);
}

/*---local function definitions----------------------------------------------*/

