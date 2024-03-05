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
 * @file    stbhwini.h
 * @brief   Header file - Function prototype for init function
 * @date    20/06/2002
 */

// pre-processor mechanism so multiple inclusions don't cause compilation error

#ifndef _STBHWINI_H

#define _STBHWINI_H

//---Constant and macro definitions for public use-----------------------------


//---Enumerations for public use-----------------------------------------------
typedef enum
{
    HW_DVB_SYSTEM   = 0,
    HW_ISDB_SYSTEM
} E_HW_DTVSYSTEM_TYPE;

typedef enum e_hw_subt_control
{
    HW_SUBT_NONE = 0,    // no effects of subtitle selection on hardware layer
    HW_SUBT_EBU = 1,     // enables teletext subtitles facilities
    HW_SUBT_DVB = 2      // enables DVB subtitle facilities
} E_HW_SUBT_CONTROL_MASK;

typedef enum
{
    HW_STANDBY_ON,          /* System is fully on */
    HW_STANDBY_ACTIVE,      /* System is in standby but only the outputs are turned off */
    HW_STANDBY_PASSIVE,     /* Low power standby state with DVBCore still active but not doing anything */
    HW_STANDBY_LOWPOWER     /* Lowest power standby state; DVBCore does not expect to be active */
} E_HW_STANDBY_STATE;

typedef enum
{
    HW_WAKEUP_UNKNOWN = 0,  /* Cause of the system waking up isn't known, so could be power cycle */
    HW_WAKEUP_TIMER,        /* The system was woken up by the wake up timer */
    HW_WAKEUP_USER          /* The system was woken up by user interaction (e.g. front panel or RCU) */
} E_HW_WAKEUP_TYPE;

//---Global type defs for public use-------------------------------------------

//---Global Function prototypes for public use---------------------------------
/**
 * @brief   Initialise the platform layer by calling the platform layer initialisation functions
 *          that are appropriate for this platform.
 * @param   hw_subt The subtitling mode mask, indicates which types of subtitles are required
 */
void STB_HWInitialise(E_HW_SUBT_CONTROL_MASK hw_subt);

/**
 * @brief   Terminate the platform layer by calling the platform layer termination functions
 *          that are appropriate for this platform. This should release all resources
 *          allocated by STB_HWInitialise()
 */
void STB_HWTerminate(void);

/**
 * @brief   Used to transition the system between the various standby states.
 *          It's assumed that the system can move to any state from any of the other states.
 * @param   state state the system is requesting to move to
 * @return  TRUE if the system has or will move to the requested state, FALSE otherwise
 */
BOOLEAN STB_HWSetStandbyState(E_HW_STANDBY_STATE state);

/**
 * @brief   Returns the current standby state of the system.
 *          STANDBY_LOWPOWER should never be returned by this function because in this state
 *          DVBCore is assumed not to be active
 * @return  current standby state
 */
E_HW_STANDBY_STATE STB_HWGetStandbyState(void);

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
                         U8BIT minutes, U32BIT time_in_mins);

/**
 * @brief   Returns what caused the system to be powered on. This is used to work out what to
 *          do after being powered on (e.g. if due to user interaction then the system needs to
 *          enter the HW_STANDBY_ON state, whereas if it's due to the wake up timer then the system
 *          needs to work out what it needs to be do next, which may be to start a recording for
 *          a PVR type system, in which case it will enter the HW_STANDBY_ACTIVE state).
 * @return  the cause of the power on
 */
E_HW_WAKEUP_TYPE STB_HWGetWakeUpType(void);

/**
 * @brief   Perform any operations required before entering standby
 * @deprecated This function is no longer used by DVBCore; STB_HWSetStandbyState is used instead.
 */
void STB_HWEnterStandby(void);

/**
 * @brief   Perform any operations required when coming out of standby
 * @deprecated This function is no longer used by DVBCore; STB_HWSetStandbyState is used instead.
 */
void STB_HWLeaveStandby(void);

/**
 * @brief   Set DTV system type
 * @param   type: E_DTV_SYSTEM_TYPE
 */
void STB_HWSetDtvSystem(E_HW_DTVSYSTEM_TYPE type);


/**
 * @brief   Get DTV system type
 * @return E_DTV_SYSTEM_TYPE
 */
E_HW_DTVSYSTEM_TYPE STB_HWGetDtvSystem();


// B:   Register Callback API, call by up level
typedef void (*OSD_OverlayClear_Func)();
typedef void (*OSD_OverlayDraw_Func)(int src_width, int src_height, int dst_x, int dst_y, int dst_width, int dst_height, const unsigned char *data);
typedef void (*OSD_OverlayDrawFinished_Func)();

void STB_HWRegCB_OverlayClear_Func(OSD_OverlayClear_Func func);
void STB_HWRegCB_OverlayDraw_Func(OSD_OverlayDraw_Func func);
void STB_HWRegCB_OverlayDrawFinished_Func(OSD_OverlayDrawFinished_Func func);

typedef BOOLEAN (*DP_GetSearchMode_Func)(U8BIT path);
typedef U8BIT (*DP_GetPathDemux_Func)(U8BIT path);

void STB_HWRegCB_GetSearchMode_Func(DP_GetSearchMode_Func func);
void STB_HWRegCB_GetPathDemux_Func(DP_GetPathDemux_Func func);


typedef BOOLEAN (*DP_IsDecodingPath_Func)(U8BIT path);
typedef BOOLEAN (*DP_IsRecordingPath_Func)(U8BIT path);

void STB_HWRegCB_IsDecodingPath_Func(DP_IsDecodingPath_Func func);
void STB_HWRegCB_IsRecordingPath_Func(DP_IsRecordingPath_Func func);



// E:   Register Callback API, call by up level

#endif //  _STBHWINI_H
