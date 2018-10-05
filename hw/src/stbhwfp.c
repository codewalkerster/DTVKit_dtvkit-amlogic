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
 * @brief   Set Top Box - Hardware Layer, Front Panel & Handset
 * @file    stbhwfp.c
 * @date    October 2018
 */

/*#define  PANEL_DEBUG*/

/*---includes for this file--------------------------------------------------*/
/* compiler library header files */

/* third party header files */

/* STB header files */
#include "techtype.h"
#include "dbgfuncs.h"
#include "stbhwfp.h"

/*---macro definitions for this file-----------------------------------------*/
#ifdef  PANEL_DEBUG
#define  PANEL_DBG(x,...)     STB_SPDebugWrite("%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )
#else
#define  PANEL_DBG(x,...)
#endif

/*---constant definitions for this file--------------------------------------*/

/*---local typedef structs for this file-------------------------------------*/

/*---local (static) variable declarations for this file----------------------*/

/*---local function prototypes for this file---------------------------------*/

/*---global function definitions---------------------------------------------*/

/**
 * @brief   Initialise the Front Panel and Remote Control components
 */
void STB_FPInitialise(void)
{
   FUNCTION_START(STB_FPInitialise);
   FUNCTION_FINISH(STB_FPInitialise);
}

/**
 * @brief   Globally enables the front panel keys
 * @param   enable set to TRUE to enable keys, FALSE otherwise
 */
void STB_FPKeyEnable(BOOLEAN enable)
{
   FUNCTION_START(STB_FPKeyEnable);
   USE_UNWANTED_PARAM(enable);
   FUNCTION_FINISH(STB_FPKeyEnable);
}

/**
 * @brief   Globally enables the remote control keys
 * @param   enable set to TRUE to enable keys, FALSE otherwise
 */
void STB_FPRemoteEnable(BOOLEAN enable)
{
   FUNCTION_START(STB_FPRemoteEnable);
   USE_UNWANTED_PARAM(enable);
   FUNCTION_FINISH(STB_FPRemoteEnable);
}

/**
 * @brief   Waits (blocks the calling thread) until a user input occurs
 */
void STB_FPWaitForUserInput(void)
{
   FUNCTION_START(STB_FPWaitForUserInput);
   FUNCTION_FINISH(STB_FPWaitForUserInput);
}

/**
 * @brief   Returns the number of available digits on the front panel display
 * @return  The number of digits
 */
U8BIT STB_FPGetDisplaySize(void)
{
   FUNCTION_START(STB_FPGetDisplaySize);
   FUNCTION_FINISH(STB_FPGetDisplaySize);
   return(0);
}

/**
 * @brief   Turns front panel LEDs on or off
 * @param   led_id The LED number to control
 * @param   led_state TRUE to turn on, FALSE to turn off
 */
void STB_FPSetLedState(U8BIT led_id, E_LED_STATE led_state, U16BIT period, U8BIT duty_cycle)
{
   FUNCTION_START(STB_FPSetLedState);

   USE_UNWANTED_PARAM(led_id);
   USE_UNWANTED_PARAM(led_state);
   USE_UNWANTED_PARAM(period);
   USE_UNWANTED_PARAM(duty_cycle);

   FUNCTION_FINISH(STB_FPSetLedState);
}

/**
 * @brief   Sets the handset code/id of the remote control
 * @param   hset_code The 16 bit handset code
 */
void STB_FPSetHandsetCode(U16BIT hset_code)
{
   FUNCTION_START(STB_FPSetHandsetCode);

   USE_UNWANTED_PARAM(hset_code);

   FUNCTION_FINISH(STB_FPSetHandsetCode);
}

/**
 * @brief   Display an animation sequence on the front panel
 * @param   anim_type The animation style to display
 * @param   frame_rate Delay between frames (in units of 10mS)
 */
void STB_FPAnimation(U8BIT anim_type, U8BIT frame_rate)
{
   FUNCTION_START(STB_FPAnimation);

   USE_UNWANTED_PARAM(anim_type);
   USE_UNWANTED_PARAM(frame_rate);

   FUNCTION_FINISH(STB_FPAnimation);
}

/**
 * @brief   Show a static or scrolling text string on the front panel
 * @param   string_addr address of text string to be displayed
 * @param   scrollstep characters to move each step (0=static)
 * @param   scrollrate delay between steps (in units of 10mS)
 */
void STB_FPDisplayString(const char *string_addr, U8BIT scrollstep, U16BIT scrollrate)
{
   FUNCTION_START(STB_FPDisplayString);
   USE_UNWANTED_PARAM(string_addr);
   USE_UNWANTED_PARAM(scrollstep);
   USE_UNWANTED_PARAM(scrollrate);
   FUNCTION_FINISH(STB_FPDisplayString);
}

/**
 * @brief   Set the current date and time on the front panel. The date is passed as mjd and as
 *          year, month and day for an easier implementation.
 * @param   mjd modified Julian day
 * @param   year year
 * @param   month month
 * @param   day day
 * @param   hours hours
 * @param   minutes minutes
 * @param   seconds seconds
 */
void STB_FPSetClock(U16BIT mjd, U16BIT year, U8BIT month, U8BIT day, U8BIT hours, U8BIT minutes, U8BIT seconds)
{
   FUNCTION_START(STB_FPSetClock);
   USE_UNWANTED_PARAM(mjd);
   USE_UNWANTED_PARAM(year);
   USE_UNWANTED_PARAM(month);
   USE_UNWANTED_PARAM(day);
   USE_UNWANTED_PARAM(hours);
   USE_UNWANTED_PARAM(minutes);
   USE_UNWANTED_PARAM(seconds);
   FUNCTION_FINISH(STB_FPSetClock);
}

/**
 * @brief   Instructs the front panel to show/hide the time
 * @param   BOOLEAN show if TRUE, show the time, else hide it.
 */
void STB_FPShowClock(BOOLEAN show)
{
   FUNCTION_START(STB_FPShowClock);
   USE_UNWANTED_PARAM(show);
   FUNCTION_FINISH(STB_FPShowClock);
}

/*---local function definitions----------------------------------------------*/
