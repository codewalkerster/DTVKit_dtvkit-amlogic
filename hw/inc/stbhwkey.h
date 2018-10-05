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
 * @brief   Handset id, codes and front panel events
 * @file    stbhwkey.h
 * @date    October 2018
 */

#ifndef _STBHWKEY_H
#define _STBHWKEY_H

/*---Constant and macro definitions for public use-----------------------------*/

#define HANDSET_ID             0x0000

/* Handset keycodes...*/
#define HS_EVENT_STANDBY       0x00810001
#define HS_EVENT_MUTE          0x00810002
#define HS_EVENT_NUMERIC_1     0x00810003
#define HS_EVENT_NUMERIC_2     0x00810004
#define HS_EVENT_NUMERIC_3     0x00810005
#define HS_EVENT_NUMERIC_4     0x00810006
#define HS_EVENT_NUMERIC_5     0x00810007
#define HS_EVENT_NUMERIC_6     0x00810008
#define HS_EVENT_NUMERIC_7     0x00810009
#define HS_EVENT_NUMERIC_8     0x0081000a
#define HS_EVENT_NUMERIC_9     0x0081000b
#define HS_EVENT_NUMERIC_0     0x0081000c
#define HS_EVENT_EPG           0x0081000d
#define HS_EVENT_FAV           0x0081000e
#define HS_EVENT_MENU          0x0081000f
#define HS_EVENT_ESC           0x00810010
#define HS_EVENT_UP            0x00810011
#define HS_EVENT_LEFT          0x00810012
#define HS_EVENT_OK            0x00810013
#define HS_EVENT_RIGHT         0x00810014
#define HS_EVENT_DOWN          0x00810015
#define HS_EVENT_SERVICES      0x00810016
#define HS_EVENT_RED           0x00810017
#define HS_EVENT_GREEN         0x00810018
#define HS_EVENT_YELLOW        0x00810019
#define HS_EVENT_BLUE          0x0081001a
#define HS_EVENT_SUBT          0x0081001b
#define HS_EVENT_CHAN_UP       0x0081001c
#define HS_EVENT_CHAN_DOWN     0x0081001d
#define HS_EVENT_VOL_UP        0x0081001e
#define HS_EVENT_VOL_DOWN      0x0081001f
#define HS_EVENT_TEXT          0x00810020
#define HS_EVENT_LANG          0x00810021
#define HS_EVENT_INFO          0x00810022
#define HS_EVENT_RECALL        0x00810023
#define HS_EVENT_AD            0x00810024

#define HS_EVENT_WIDE          0x00810025
#define HS_EVENT_TIMER         0x00810026
#define HS_EVENT_AV_SOURCE     0x00810027

#define HS_EVENT_PLAY          0x00810028
#define HS_EVENT_FF            0x00810029
#define HS_EVENT_FR            0x0081002a

#define HS_EVENT_GOTO          0x0081002b
#define HS_EVENT_RPT           0x0081002c
#define HS_EVENT_PROG_TIMER    0x0081002d
#define HS_EVENT_MODE          0x0081002e
#define HS_EVENT_RPT_AB        0x0081002f
#define HS_EVENT_BOOKMARK      0x00810030

#define HS_EVENT_PAUSE         0x00810031
#define HS_EVENT_STOP          0x00810032

#define HS_EVENT_REC           0x00810040

#ifdef PVR_BUILD

#define HS_EVENT_STEPF         HS_EVENT_PAUSE
#define HS_EVENT_SM            0x00810041
#define HS_EVENT_SLOWF         HS_EVENT_SM

#define HS_EVENT_USB             0x00810042
#define HS_EVENT_DEFINE_BOOKMARK 0x00810043
#define HS_EVENT_NEXT_BOOKMARK   0x00810044

#endif // PVR_BUILD

#ifdef TEXT_TO_SPEECH
/* Buttons used in Text to Speech Systems */
#define HS_EVENT_TF            0x00810074
#define HS_EVENT_SHHH          0x00810068  /* Shh button */
#define HS_EVENT_WHEREAMI      0x00810077
#endif

/* Front Panel key codes...*/
#define KP_EVENT_STANDBY       0x00820008
#define KP_EVENT_MENU          0x0082016c
#define KP_EVENT_VOL_DOWN      0x00820165
#define KP_EVENT_VOL_UP        0x00820166
#define KP_EVENT_CHAN_UP       0x00820009
#define KP_EVENT_CHAN_DOWN     0x0082000a
#define KP_EVENT_OK            0x00820169
#define KP_EVENT_AV_SOURCE     0x0082016a


/* Front panel values */
#define PANEL_SCROLL_DELAY    90


#endif /*_STBHWKEY_H*/
