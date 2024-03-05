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
 * @file    stbhwfp.h
 * @brief   Header file - Function prototypes for front panel control
 * @date    06/02/2001
 */

// pre-processor mechanism so multiple inclusions don't cause compilation error

#ifndef _STBHWFP_H
#define _STBHWFP_H

#include "techtype.h"

//---Constant and macro definitions for public use-----------------------------

//---Enumerations for public use-----------------------------------------------
typedef enum
{
    WAKEUP_UNKNOWN = 0,
    WAKEUP_TIMER,
    WAKEUP_USER
} E_FRONTPANEL_WAKEUP_TYPE;

typedef enum
{
    LED_OFF = 0,
    LED_ON,
    LED_BLINKING
} E_LED_STATE;

//---Global type defs for public use-------------------------------------------

//---Global Function prototypes for public use---------------------------------
/**
 * @brief   Initialise the Front Panel and Remote Control components
 */
void STB_FPInitialise(void);

/**
 * @brief   Globally enables the front panel keys
 * @param   enable set to TRUE to enable keys, FALSE otherwise
 */
void STB_FPKeyEnable(BOOLEAN enable);

/**
 * @brief   Globally enables the remote control keys
 * @param   enable set to TRUE to enable keys, FALSE otherwise
 */
void STB_FPRemoteEnable(BOOLEAN enable);

/**
 * @brief   Sets the handset code/id of the remote control
 * @param   hset_code The 16 bit handset code
 */
void STB_FPSetHandsetCode(U16BIT hset_code);

/**
 * @brief   Show a static or scrolling text string on the front panel
 * @param   string text string to be displayed
 * @param   scrollstep characters to move each step (0=static)
 * @param   scrollrate delay between steps (in units of 10mS)
 */
void STB_FPDisplayString(const char *string, U8BIT scrollstep, U16BIT scrollrate);

/**
 * @brief   Set the current date on the front panel
 * @param   mjd modified Julian day
 * @param   year year
 * @param   month month
 * @param   day day
 * @param   hours hours
 * @param   minutes minutes
 * @param   seconds seconds
 */
void STB_FPSetClock(U16BIT mjd, U16BIT year, U8BIT month, U8BIT day, U8BIT hours, U8BIT minutes, U8BIT seconds);

/**
 * @brief   Instructs the front panel to show/hide the time
 * @param   BOOLEAN show if TRUE, show the time, else hide it.
 */
void STB_FPShowClock(BOOLEAN show);

/**
 * @brief   Sets the local time when the front panel should come out of standby. The date is passed
 *          as mjd and as year, month and day and the number of minutes until the wake up is also
 *          passed for an easier implementation. It is implementation specific which parameters to
 *          use.
 * @param   mjd modified Julian day of the wake up time
 * @param   year wake up year
 * @param   month wake up month
 * @param   day wake up day
 * @param   hours wake up hour
 * @param   minutes wake up minute
 * @param   time_in_mins Number of minutes until wakeup
 */
void STB_FPSetWakeUpTime(U16BIT mjd, U16BIT year, U8BIT month, U8BIT day, U8BIT hours, U8BIT minutes,
                         U32BIT time_in_mins);

/**
 * @brief   Controls the  box low power standby status
 * @param   standby if TRUE instructs the front panel to put the box in low power
 *          standby, if FALSE instructs the front panel to wake the box up from
 *          low power stand-by
 */
void STB_FPSetStandby(BOOLEAN standby);

/**
 * @brief   Returns the wake up type
 * @return  WAKEUP_UNKNOWN when the front panel didn't actively power up the board, i.e. the power
 *          has been unplugged and plugged back;
 *          WAKEUP_TIMER the front panel is turning the board up after a timer has expired (see
 *          STB_FPSetWakeUpTime)
 *          WAKEUP_USER is returned when the front panel is turning the board up because the user
 *          pressed the stand-by button.
 */
E_FRONTPANEL_WAKEUP_TYPE STB_FPGetWakeUpType(void);

/**
 * @brief   Turns front panel LEDs on or off
 * @param   led_id The LED number to control
 * @param   led_state TRUE to turn on, FALSE to turn off
 */
void STB_FPSetLedState(U8BIT led_id, E_LED_STATE led_state, U16BIT period, U8BIT duty_cycle);

void STB_FPWaitForUserInput(void);
U8BIT STB_FPGetDisplaySize(void);
void STB_FPAnimation(U8BIT anim_type, U8BIT frame_rate);
void STB_FPRefreshWatchdog(void);
void STB_FPSetWdogPeriod(U16BIT watchdog_period);
void STB_FPSetAutoStandby(BOOLEAN auto_standby);
void STB_FPSetAutoStandbyTime(U8BIT minute);

#endif //  _STBHWFP_H
