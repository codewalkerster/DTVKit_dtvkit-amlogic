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


#include <sys/time.h>
#include <time.h>
#include <stdio.h>

/* STB Header Files */
#include "techtype.h"
#include "dbgfuncs.h"
#include <cutils/properties.h>
#include "stb_utils.h"

#ifndef RDK_COMPILE
#include <sys/system_properties.h>
#endif


/*!- Local MACRO Definitions */
#define RTC_TICKS_PER_SEC     1000

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

/* UTC Time in Seconds */
static U32BIT utc_seconds = 0;

/* System Time from when the Time was Last Set */
static U32BIT sync_time = 0;


/* Local PROTOTYPE Declarations */
static U32BIT SysBootTime(void);
static U32BIT SysBootTimeSeconds(void);
static U32BIT STB_OSGetSystemTime(void);
static time_t STB_OSGetSystemUnixTimeStamp(void);

/**
 * @brief   Allows setting of initial boot time.
 */
void STB_OSInitialise(void)
{
   FUNCTION_START(STB_OSInitialise);

   /* Call SysBootTime at the very beginning to initialise the static variable inside of it */
   SysBootTime();

   FUNCTION_FINISH(STB_OSInitialise);
}

/**
 * @brief   Set the time in seconds since midnight 1-1-1970
 * @param   num_seconds time in seconds
 */
void STB_OSSetClockRTC(U32BIT num_seconds)
{
   FUNCTION_START(STB_OSSetClockRTC);

   char prop_time[64] = {0};
   int64_t local_time,temp_time;

   utc_seconds = num_seconds;

   /* Save the system time at the point the clock has been set */
   sync_time = SysBootTimeSeconds();

   local_time = (int64_t)(STB_OSGetSystemUnixTimeStamp());
   //minus current timezone and app will translate it stream time by adding current timezone
   temp_time = (int64_t)num_seconds - local_time;

   sprintf(prop_time, "%ld000", (LONG)temp_time);//prop need ms
#ifdef DTVKIT_IN_VENDOR_PARTITION
   property_set("vendor.sys.tv.stream.localtime", prop_time);
#else
      STB_DVRProp_Set("vendor.sys.tv.stream.localtime", prop_time);
#endif

   RTC_DBG("Time set to %u secs at %u msecs", num_seconds, sync_time);

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

   /* Actual time is given by the value saved in
    * utc_seconds with the difference between the time
    * when it was set (sync_time) and the time now.   */
   time_now = utc_seconds + (SysBootTimeSeconds() - sync_time);
   RTC_DBG("time_now:%u = utc_seconds:%u + (SysBootTimeSeconds:%u-sync_time:%u)",time_now,utc_seconds, SysBootTimeSeconds(), sync_time);

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
   U32BIT diff;

   FUNCTION_START(STB_OSGetClockDiff);

   /* Calculate difference between current time & given value */
   diff = SysBootTime() - timestamp;

   FUNCTION_FINISH(STB_OSGetClockDiff);

   return diff;
}

/**
 * @brief    Get Number of Clock Ticks per Second.
 * @return   Number of Ticks.
 */
U32BIT STB_OSGetClockPerSec(void)
{
   FUNCTION_START(STB_OSGetClockPerSec);
   FUNCTION_FINISH(STB_OSGetClockPerSec);

   return(RTC_TICKS_PER_SEC);
}

/**
 * @brief   Get Current Computer Clock Time.
 * @return  Time in Milliseconds.
 */
U32BIT STB_OSGetClockMilliseconds(void)
{
   U32BIT millisecs;

   FUNCTION_START(STB_OSGetClockMilliseconds);

   millisecs = SysBootTime();

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
   char prop_time[64] = {0};
   int64_t system_time,temp_time;

   system_time = (int64_t)(STB_OSGetSystemUnixTimeStamp()/*STB_OSGetSystemTime()*/);
   temp_time = (int64_t)num_seconds - system_time;

   sprintf(prop_time, "%ld000", (LONG)temp_time);//prop need ms
   //use time that contains timezone instead in STB_OSSetClockRTC
#ifdef DTVKIT_IN_VENDOR_PARTITION
   property_set("vendor.sys.tv.stream.realtime", prop_time);
#else
      STB_DVRProp_Set("vendor.sys.tv.stream.realtime", prop_time);
#endif
   RTC_DBG("prop_time[%d] = ts_time[%d] - system_time[%d]\n", (U32BIT)temp_time, num_seconds, (U32BIT)system_time);

   FUNCTION_FINISH(STB_OSSetClockGMT);
}

/**
 * @brief   Returns the system time in seconds
 * @return  system time in seconds
 */
U32BIT STB_OSGetClockGMT(void)
{
   struct timeval tv;

   FUNCTION_START(STB_OSGetClockGMT);

   gettimeofday(&tv, NULL);

   FUNCTION_FINISH(STB_OSGetClockGMT);

   return tv.tv_sec;
}

/**
 * @brief   Set the daylight change time timestamp in seconds
 * @param   num_seconds time in seconds
 */
void STB_OSSetClockOffsetChange(U32BIT num_seconds)
{
   char prop_time[64] = {0};
   int64_t system_time,temp_time;

   FUNCTION_START(STB_OSSetClockOffsetChange);

   sprintf(prop_time, "%ld000", (LONG)num_seconds);//prop need ms

#ifdef DTVKIT_IN_VENDOR_PARTITION
   property_set("vendor.sys.tv.stream.offsetchange", prop_time);
#else
      STB_DVRProp_Set("vendor.sys.tv.stream.offsetchange", prop_time);
#endif

   FUNCTION_FINISH(STB_OSSetClockOffsetChange);
}

/**
 * @brief   Set the time zone timestamp in seconds
 * @param   num_seconds time in seconds
 */
void STB_OSSetClockTimeZoneDiff(S32BIT num_seconds)
{
   FUNCTION_START(STB_OSSetClockTimeZoneDiff);

   char prop_time[64] = {0};

   sprintf(prop_time, "%ld000", (LONG)num_seconds);//prop need ms
#ifdef DTVKIT_IN_VENDOR_PARTITION
   property_set("vendor.sys.tv.stream.timeozone", prop_time);
#else
      STB_DVRProp_Set("vendor.sys.tv.stream.timeozone", prop_time);
#endif

   FUNCTION_FINISH(STB_OSSetClockTimeZoneDiff);
}

/**
 * @brief   Set the next time zone timestamp in seconds
 * @param   num_seconds time in seconds
 */
void STB_OSSetClockTimeZoneNext(S32BIT num_seconds)
{
   char prop_time[64] = {0};

   FUNCTION_START(STB_OSSetClockTimeZoneNext);

   sprintf(prop_time, "%ld000", (LONG)num_seconds);//prop need ms
#ifdef DTVKIT_IN_VENDOR_PARTITION
       property_set("vendor.sys.tv.stream.timeozone.next", prop_time);
#else
       STB_DVRProp_Set("vendor.sys.tv.stream.timeozone.next", prop_time);
#endif

   FUNCTION_FINISH(STB_OSSetClockTimeZoneNext);
}


/****************************************************************************
 *  Local Procedures
 ****************************************************************************/

/*!**************************************************************************
 * @brief    Get the time in milliseconds since the system was booted.
 * @return   Time in milliseconds since system booted
 * @warning  None.
 * @bug      None.
 ****************************************************************************/
static U32BIT SysBootTime(void)
{
    struct timespec tsp;
    U64BIT boot_time_in_msec=0;

    /* Notice CLOCK_MONOTONIC is not affected by discontinuous jumps in the system time */
    clock_gettime(CLOCK_MONOTONIC,&tsp);
    /* Overflow is unlikely to happen here for reasons below:
     * 1) Time out of CLOCK_MONOTONIC starts from 0 and reflects actual elapsed time from boot;
     * 2) Time out of CLOCK_MONOTONIC is not affected by discontinuous jumps in the system time;
     */
    boot_time_in_msec=tsp.tv_sec*1000+tsp.tv_nsec/1000000;
    //RTC_DBG("timespec=(%u,%ld), ret=%u", tsp.tv_sec,tsp.tv_nsec,(U32BIT)boot_time_in_msec);
    return (U32BIT)boot_time_in_msec;
}

static U32BIT SysBootTimeSeconds(void)
{
    struct timespec tsp;

    /* Notice CLOCK_MONOTONIC is not affected by discontinuous jumps in the system time */
    clock_gettime(CLOCK_MONOTONIC,&tsp);
    /* Overflow is unlikely to happen here for reasons below:
     * 1) Time out of CLOCK_MONOTONIC starts from 0 and reflects actual elapsed time from boot;
     * 2) Time out of CLOCK_MONOTONIC is not affected by discontinuous jumps in the system time;
     */
    //RTC_DBG("timespec=(%u,%ld), ret=%u", tsp.tv_sec,tsp.tv_nsec,(U32BIT)boot_time_in_msec);
    return (U32BIT)tsp.tv_sec;
}

static U32BIT STB_OSGetSystemTime(void)
{
    time_t t;
    struct tm * lt;
    time (&t);//获取Unix时间戳。
    lt = localtime (&t);//转为时间结构。
    RTC_DBG( "systime:%d/%d/%d %d:%d:%d dst:%d\n",lt->tm_year+1900, lt->tm_mon, lt->tm_mday, lt->tm_hour, lt->tm_min, lt->tm_sec, tm_gmt->tm_isdst);
    return t;
}

static time_t STB_OSGetSystemUnixTimeStamp(void)
{
    time_t unixTimeStamp;
    time (&unixTimeStamp);
    RTC_DBG("systime unix=%ld", unixTimeStamp);
    return unixTimeStamp;
}
