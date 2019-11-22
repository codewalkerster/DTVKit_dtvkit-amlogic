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
 * @brief   Set Top Box - Hardware Layer, Top level initialisation
 * @file    stbhwini.c
 * @date    October 2018
 */

//#define ENABLE_DEBUG

/*---includes for this file---------------------------------------------------*/
/* compiler library header files */

/* third party header files */

/* STB header files */
#include "techtype.h"
#include "dbgfuncs.h"

#include "stbhwdef.h"
#include "stbhwini.h"
#include "stbhwmem.h"
#include "stbhwdmx.h"
#include "stbhwtun.h"
#include "stbhwosd.h"
#include "stbhwav.h"
#include "stbhwdsk.h"
#include "stbhwnet.h"

/*---constant definitions for this file---------------------------------------*/
#ifdef ENABLE_DEBUG
#define DBG(x,...)         STB_SPDebugWrite("%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )
#else
#define DBG(x,...)
#endif

/*---local typedef structs for this file--------------------------------------*/

/*---local (static) variable declarations for this file-----------------------*/

/*---global variable definitions----------------------------------------------*/


/*---local function prototypes for this file----------------------------------*/

/*---external function prototypes for this file-------------------------------*/

/*---global function definitions---------------------------------------------*/

/**
 * @brief   Initialise the platform layer by calling the platform layer initialisation functions
 *          that are appropriate for this platform.
 * @param   hw_subt The subtitling mode mask, indicates which types of subtitles are required
 */
void STB_HWInitialise(E_HW_SUBT_CONTROL_MASK hw_subt)
{
   FUNCTION_START(STB_HWInitialise);

   STB_CfgInitialise();

   STB_MEMInitialiseRAM();

   if (hw_subt == HW_SUBT_NONE)
   {
      STB_DMXInitialise(NUM_DEMUX_PATHS, FALSE);
   }
   else
   {
      STB_DMXInitialise(NUM_DEMUX_PATHS, TRUE);
   }

   STB_OSDInitialise(0);
   STB_AVInitialise(NUM_AUDIO_DECODE_PATHS, NUM_VIDEO_DECODE_PATHS);
   STB_MEMInitialiseNVM();
   //STB_NWInitialise();

   /* Number of tuners is dynamically determined, so 0 is passed in */
   STB_TuneInitialise(0);

   STB_DSKInitialise();

   FUNCTION_FINISH(STB_HWInitialise);
}

/**
 * @brief   Used to transition the system between the various standby states.
 *          It's assumed that the system can move to any state from any of the other states.
 * @param   state state the system is requesting to move to
 * @return  TRUE if the system has or will move to the requested state, FALSE otherwise
 */
BOOLEAN STB_HWSetStandbyState(E_HW_STANDBY_STATE state)
{
   FUNCTION_START(STB_HWSetStandbyState);
   USE_UNWANTED_PARAM(state);
   FUNCTION_FINISH(STB_HWSetStandbyState);

   return FALSE;
}

/**
 * @brief   Returns the current standby state of the system.
 *          STANDBY_LOWPOWER should never be returned by this function because in this state
 *          DVBCore is assumed not to be active
 * @return  current standby state
 */
E_HW_STANDBY_STATE STB_HWGetStandbyState(void)
{
   FUNCTION_START(STB_HWGetStandbyState);
   FUNCTION_FINISH(STB_HWGetStandbyState);
   return(HW_STANDBY_ON);
}

/**
 * @brief   Used to set the wake up time for when the system is going to be put into the low power
 *          standby state. Not all the supplied parameters are expected to be used, just those
 *          that are needed by the system to ensure the system wakes up at the required time.
 * @param   mjd wake up date in Modified Julian Date format
 * @param   year the year in full (e.g. 2016) the system is to wake up
 * @param   month month number (January=1, December=12) the system is to wake up
 * @param   day day number (1 to 28, 29, 30 or 31) the system is to wake up
 * @param   hours time in hours (0 to 23) the system is to wake up
 * @param   minutes time in minutes (0 to 59) the system is to wake up
 * @param   time_in_mins the time in minutes from the current date/time to the wake up time
 */
void STB_HWSetWakeUpTime(U16BIT mjd, U16BIT year, U8BIT month, U8BIT day, U8BIT hours,
   U8BIT minutes, U32BIT time_in_mins)
{
   FUNCTION_START(STB_HWSetWakeUpTime);
   USE_UNWANTED_PARAM(mjd);
   USE_UNWANTED_PARAM(year);
   USE_UNWANTED_PARAM(month);
   USE_UNWANTED_PARAM(day);
   USE_UNWANTED_PARAM(hours);
   USE_UNWANTED_PARAM(minutes);
   USE_UNWANTED_PARAM(time_in_mins);
   FUNCTION_FINISH(STB_HWSetWakeUpTime);
}

/**
 * @brief   Returns what caused the system to be powered on. This is used to work out what to
 *          do after being powered on (e.g. if due to user interaction then the system needs to
 *          enter the HW_STANDBY_ON state, whereas if it's due to the wake up timer then the system
 *          needs to work out what it needs to be do next, which may be to start a recording for
 *          a PVR type system, in which case it will enter the HW_STANDBY_ACTIVE state).
 * @return  the cause of the power on
 */
E_HW_WAKEUP_TYPE STB_HWGetWakeUpType(void)
{
   FUNCTION_START(STB_HWGetWakeUpType);
   FUNCTION_FINISH(STB_HWGetWakeUpType);
   return(HW_WAKEUP_UNKNOWN);
}

/*---local function definitions-----------------------------------------------*/

