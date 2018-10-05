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

/* third party header files */

/* STB Header Files */
#include "techtype.h"
#include "dbgfuncs.h"

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
   FUNCTION_FINISH(STB_CIRouteTS);

   return(FALSE);
}

/*---local function definitions----------------------------------------------*/
