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
 * @brief   Set Top Box - Hardware Layer, VBI functions.
 * @file    stbhwvbi.c
 * @date    October 2018
 */

/*#define VBI_DEBUG*/

/*---includes for this file--------------------------------------------------*/
/* compiler library header files */
#include <stdio.h>

/* third party header files */

/* STB Header Files */
#include "techtype.h"
#include "dbgfuncs.h"
#include "dtv_log.h"
#define TAG  "STBHWVBI"

/*---macro definitions for this file-----------------------------------------*/

#ifdef VBI_DEBUG
#define VBI_DBG(x,...) DTV_LOG(ANDROID_LOG_INFO, TAG, "%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )
#else
#define VBI_DBG(x,...)
#endif

/*---constant definitions for this file--------------------------------------*/

/*---local typedef structs for this file-------------------------------------*/

/*---local (static) variable declarations for this file----------------------*/

/*---local function prototypes for this file---------------------------------*/

/*---global function definitions----------------------------------------------*/

/**
 * @brief   Initialises the hardware interface for VBI support
 */
void STB_HWInitialiseVBI(void)
{
   FUNCTION_START(STB_HWInitialiseVBI);
   FUNCTION_FINISH(STB_HWInitialiseVBI);
}

/**
 * @brief   This function is called by the DVB stack to pass data to be inserted into the VBI
 * @param   pes_data_field data to be inserted
 * @param   num_bytes number of bytes of data to be inserted
 */
void STB_HWVBIInsert(U8BIT *pes_data_field, U32BIT num_bytes)
{
   FUNCTION_START(STB_HWVBIInsert);

   USE_UNWANTED_PARAM(pes_data_field);
   USE_UNWANTED_PARAM(num_bytes);
   
   FUNCTION_FINISH(STB_HWVBIInsert);
}

/*---local function definitions----------------------------------------------*/
