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
 * @brief   Set Top Box - Hardware Layer, CI functions.
 * @file    stbhwci.c
 * @date    October 2018
 */

/*#define CI_DEBUG*/
/*#define CI_ERROR*/

/*---includes for this file--------------------------------------------------*/
/* compiler library header files */
#include <stdio.h>
#include <stdarg.h>

/* third party header files */

/* STB Header Files */
#include "techtype.h"
#include "dbgfuncs.h"
#include "stbhwci.h"
#include "stbhwnvm.h"
#include "stbcios.h" /*for STB_CIDebugPrintf()*/

/*---macro definitions for this file-----------------------------------------*/

#ifdef CI_DEBUG
#define CI_DBG(x,...)      STB_SPDebugWrite("%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )
#else
#define CI_DBG(x,...)
#endif

#ifdef CI_ERROR
#define CI_ERR(x,...)      STB_SPDebugWrite("%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )
#else
#define CI_ERR(x,...)
#endif

/*---constant definitions for this file--------------------------------------*/

/*---local typedef structs for this file-------------------------------------*/

/*---local (static) variable declarations for this file----------------------*/

/*---local function prototypes for this file---------------------------------*/

/*---global function definitions----------------------------------------------*/

/**
 * @brief   Return number of CI slots
 * @note    When supporting USB CAMs, this function returns
 *          the same value as STB_CIUsbCamTotal().
 * @return  Number of CI slots on the receiver
 */
U8BIT STB_CIGetSlotCount(void)
{
   return 1;
}

/**
 * @brief   Puts CI control into standby mode (power off)
 */
void STB_CIStandbyOn(void)
{
   FUNCTION_START(STB_CIStandbyOn);
   FUNCTION_FINISH(STB_CIStandbyOn);
}

/**
 * @brief   Brings CI out of standby mode (power on)
 */
void STB_CIStandbyOff(void)
{
   FUNCTION_START(STB_CIStandbyOff);
   FUNCTION_FINISH(STB_CIStandbyOff);
}

/**
 * @brief   Sets up routing between a tuner and slot
 * @param   tuner - tuner to be routed
 * @param   slot_id - slot to apply routing to
 * @param   pass_through - TRUE if the TS should be routed through the slot, FALSE otherwise
 * @return  TRUE if successful, FALSE otherwise
 */
BOOLEAN STB_CIRouteTS(U8BIT tuner, U8BIT slot_id, BOOLEAN pass_through)
{
   FUNCTION_START(STB_CIRouteTS);

   USE_UNWANTED_PARAM(tuner);
   USE_UNWANTED_PARAM(slot_id);
   USE_UNWANTED_PARAM(pass_through);

   STB_CIDebugPrintf("STB_CIRouteTS(%u, %u, %u)", tuner, slot_id, pass_through);

   FUNCTION_FINISH(STB_CIRouteTS);

   return(FALSE);
}

/**
 * @brief   Return CI+ host key
 * @param   type type of host key
 * @param   key pointer to the key data
 * @param   length number of bytes in key data
 * @note    The pointer must remain valid while the CI+ stack is running
 * @param   slot_id Zero-based CI slot identifier (0, 1, ...)
 */
void STB_CIGetHostKey(E_STB_CI_KEY_TYPE type, U8BIT **key, U16BIT *length)
{
   *length = 0;
   *key = NULL;
}

/**
 * @brief   Read data from secure non-volatile area
 * @param   buffer pointer to data buffer to read into
 * @param   len number of bytes to read
 * @return  TRUE if read operation was successful, FALSE otherwise
 */
BOOLEAN STB_CIReadSecureNVM(U8BIT *buffer, U32BIT len)
{
   return FALSE;
}

/**
 * @brief   Write data into secure non-volatile area
 * @param   buffer pointer to data buffer to write
 * @param   len number of bytes to write
 * @return  TRUE if read operation was successful, FALSE otherwise
 */
BOOLEAN STB_CIWriteSecureNVM(U8BIT *buffer, U32BIT len)
{
   return FALSE;
}

/**
 * @brief   Write debug string to output
 * @param   format string & format
 */
void STB_CIDebugPrintf(const char *format, ... )
{
   static char debug_msg_buff[512];
   va_list vparams;

   FUNCTION_START(STB_SPDebugNoCnWrite);

   ASSERT(format != NULL);

   va_start(vparams, format);
   vsnprintf(debug_msg_buff, sizeof(debug_msg_buff), format, vparams);
   va_end(vparams);

   printf("%s", debug_msg_buff);
   fflush(stdout);

   FUNCTION_FINISH(STB_SPDebugNoCnWrite);
}

/*---local function definitions----------------------------------------------*/
