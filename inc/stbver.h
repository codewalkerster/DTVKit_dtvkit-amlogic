/*******************************************************************************
 * Copyright © 2014 The DTVKit Open Software Foundation Ltd (www.dtvkit.org)
 * Copyright © 2008 Ocean Blue Software Ltd
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
 * @file    stbver.h
 * @brief   Header file - Function prototypes for version numbers
 * @date    11/12/2008
 */


#ifndef _STBVER_H
#define _STBVER_H

#include "techtype.h"

typedef struct
{
    U8BIT product;
    U8BIT major;
    U8BIT minor;
    U8BIT release;
    U32BIT revision;
} APP_SW_VER_STRUCT;

/* NOTE: This definition must comply with that in version.h of the boot code */
typedef struct
{
    U8BIT major;
    U8BIT minor;
} T_BOOT_SW_VER_NUMBER;

/**
 * @brief   Get Software Version Number for Application
 */
void STB_OSGetAppSwVersionNum(APP_SW_VER_STRUCT *vptr);

/**
 * @brief   Get Software Version Number for Boot Loader
 */
void STB_OSGetBootSwVersionNum(T_BOOT_SW_VER_NUMBER *vptr);

#endif /*_STBVER_H*/
