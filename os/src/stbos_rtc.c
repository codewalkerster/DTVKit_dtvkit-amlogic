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
 * @brief   Set Top Box - System Interface for Real Time Clock facility.
 * @file    stbos_rtc.c
 * @date    October 2018
 */


/* STB Header Files */
#include "techtype.h"
#include "dbgfuncs.h"


/*!- Local MACRO Definitions */

/*!- Select-Deselect Local Debug Text Output */
/*#define  RTC_DEBUG*/

#ifdef  RTC_DEBUG
#define  RTC_DBG(x,...)       STB_SPDebugWrite("%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )
#else
#define  RTC_DBG(x,...)
#endif


/* Local CONSTANT  Definitions */

/* Local ENUM/TYPE Definitions */

/* Local VARIABLE  Declarations */

/* Local PROTOTYPE Declarations */


/**
 * @brief   Allows setting of initial boot time.
 */
void STB_OSInitialise(void)
{
   FUNCTION_START(STB_OSInitialise);
   FUNCTION_FINISH(STB_OSInitialise);
}

/**
 * @brief   Set the time in seconds since midnight 1-1-1970
 * @param   num_seconds time in seconds
 */
void STB_OSSetClockRTC(U32BIT num_seconds)
{
   FUNCTION_START(STB_OSSetClockRTC);
   FUNCTION_FINISH(STB_OSSetClockRTC);
}

/**
 * @brief   Returns the current time in seconds. This is calculated by using
 *          the set UTC time and adding the difference between the system boot
 *          time when it was set (sync_time) and the system boot time now.
 * @return  The current time in seconds since midnight 1-1-1970.
 */
U32BIT STB_OSGetClockRTC(void)
{
   U32BIT time_now;

   FUNCTION_START(STB_OSGetClockRTC);

   time_now = 0;

   FUNCTION_FINISH(STB_OSGetClockRTC);

   return time_now;
}

/**
 * @brief   Get Difference between Given Time and Current Time.
 * @param   timestamp Given Clock Value to Compare Against.
 * @return  Time Difference in MilliSeconds.
 */
U32BIT STB_OSGetClockDiff(U32BIT timestamp)
{
   FUNCTION_START(STB_OSGetClockDiff);
   FUNCTION_FINISH(STB_OSGetClockDiff);

   return 0;
}

/**
 * @brief    Get Number of Clock Ticks per Second.
 * @return   Number of Ticks.
 */
U32BIT STB_OSGetClockPerSec(void)
{
   FUNCTION_START(STB_OSGetClockPerSec);
   FUNCTION_FINISH(STB_OSGetClockPerSec);

   return(0);
}

/**
 * @brief   Get Current Computer Clock Time.
 * @return  Time in Milliseconds.
 */
U32BIT STB_OSGetClockMilliseconds(void)
{
   U32BIT millisecs;

   FUNCTION_START(STB_OSGetClockMilliseconds);

   millisecs = 0;

   FUNCTION_FINISH(STB_OSGetClockMilliseconds);

   return millisecs;
}

/**
 * @brief   Set the time in seconds since midnight 1-1-1970 in GMT
 * @param   num_seconds time in seconds
 */
void STB_OSSetClockGMT(U32BIT num_seconds)
{
   FUNCTION_START(STB_OSSetClockGMT);
   USE_UNWANTED_PARAM(num_seconds);
   FUNCTION_FINISH(STB_OSSetClockGMT);
}

/**
 * @brief   Returns the system time in seconds
 * @return  system time in seconds
 */
U32BIT STB_OSGetClockGMT(void)
{
   FUNCTION_START(STB_OSGetClockGMT);
   FUNCTION_FINISH(STB_OSGetClockGMT);

   return(0);
}

