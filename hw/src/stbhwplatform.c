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
 * @brief   Set Top Box - Hardware Layer, Platform functions
 * @file    stbhwplatform.c
 * @date    October 2018
 */

/*#define  PLATFORM_DEBUG*/

/*---includes for this file--------------------------------------------------*/
/* compiler library header files */

/* third party header files */

/* STB header files */
#include "techtype.h"
#include "dbgfuncs.h"
#include "stbhwdef.h"

/*---macro definitions for this file-----------------------------------------*/
#ifdef  PLATFORM_DEBUG
#define  PLAT_DBG(x,...)         STB_SPDebugWrite("%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )
#else
#define  PLAT_DBG(x,...)
#endif

/*---constant definitions for this file--------------------------------------*/

/*---local typedef structs for this file-------------------------------------*/

/*---local (static) variable declarations for this file----------------------*/

/*---local function prototypes for this file---------------------------------*/

/*---local function definitions----------------------------------------------*/

/*---global function definitions---------------------------------------------*/

/**
 * @brief   Queries the number of demux paths available
 * @return  The number of demux paths
 */
U8BIT  STB_HWGetDemuxPaths(void)
{
   FUNCTION_START(STB_HWGetDemuxPaths);
   FUNCTION_FINISH(STB_HWGetDemuxPaths);

   return NUM_DEMUX_PATHS;
}

/**
 * @brief   Returns the platform hardware identifier code
 * @return  The platform id
 */
U16BIT  STB_HWGetHwId(void)
{
   FUNCTION_START(STB_HWGetHwId);
   FUNCTION_FINISH(STB_HWGetHwId);
   return 0;
}

/**
 * @brief   Returns the platform customer identifier code
 * @return  The platform id
 */
U16BIT  STB_HWGetCustomerId(void)
{
   FUNCTION_START(STB_HWGetCustomerId);
   FUNCTION_FINISH(STB_HWGetCustomerId);
   return 0;
}

/**
 * @brief   Return the OUI for this device
 */
U8BIT* STB_HWGetOUI(void)
{
   FUNCTION_START(STB_HWGetOUI);
   FUNCTION_FINISH(STB_HWGetOUI);
   return(NULL);
}

/**
 * @brief   Returns the number of front end (Tuner) paths on the platform
 * @return  The number of tuner paths
 */
U8BIT  STB_HWGetTunerPaths(void)
{
   FUNCTION_START(STB_HWGetTunerPaths);
   FUNCTION_FINISH(STB_HWGetTunerPaths);
   return NUM_TUNER_PATHS;
}

/**
 * @brief   Returns the number of recordings that can take place at the same time
 * @return  The number of recordings
 */
U8BIT STB_HWGetNumRecorders(void)
{
   FUNCTION_START(STB_HWGetNumRecorders);
   FUNCTION_FINISH(STB_HWGetNumRecorders);
   return NUM_RECORDERS;
}

/**
 * @brief   Returns the number of audio decoding paths on the platform
 * @return  The number of audio decode paths
 */
U8BIT STB_HWGetAudioDecodePaths(void)
{
   FUNCTION_START(STB_HWGetAudioDecodePaths);
   FUNCTION_FINISH(STB_HWGetAudioDecodePaths);

   return NUM_AUDIO_DECODE_PATHS;
}

/**
 * @brief   Returns the number of video decoding paths on the platform
 * @return  The number of video decode paths
 */
U8BIT  STB_HWGetVideoDecodePaths(void)
{
   FUNCTION_START(STB_HWGetVideoDecodePaths);
   FUNCTION_FINISH(STB_HWGetVideoDecodePaths);

   return NUM_VIDEO_DECODE_PATHS;
}

/**
 * @brief   Returns the serial number of the Set Top Box
 * @return  The serial number
 */
U32BIT STB_HWGetBoxSerialNumber(void)
{
   FUNCTION_START(STB_HWGetBoxSerialNumber);
   FUNCTION_FINISH(STB_HWGetBoxSerialNumber);
   return 0;
}

/**
 * @brief   Reset the board
 */
void STB_OSResetCPU(void)
{
   FUNCTION_START(STB_OSResetCPU);
   FUNCTION_FINISH(STB_OSResetCPU);
}

/**
 * @brief   Returns the serial number of the Set Top Box
 * @return  The serial number
 */
U8BIT STB_HWGetNumCISlots(void)
{
   FUNCTION_START(STB_HWGetNumCISlots);
   FUNCTION_FINISH(STB_HWGetNumCISlots);
   return(NUM_CI_SLOTS);
}

/**
 * @brief   Gives the name (optionally including path) of the SQL database file
 * @param   pathname - array into which the full path will be written
 * @param   max_pathname_len - size of the pathname array
 * @return  TRUE if the pathname is returned successfully.
 */
BOOLEAN STB_GetSqlFileName(U8BIT *pathname, U16BIT max_pathname_len)
{
   FUNCTION_START(STB_GetSqlFileName);
   USE_UNWANTED_PARAM(pathname);
   USE_UNWANTED_PARAM(max_pathname_len);
   FUNCTION_FINISH(STB_GetSqlFileName);

   return FALSE;
}

