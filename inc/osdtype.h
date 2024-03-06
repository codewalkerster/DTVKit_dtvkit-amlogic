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
 * @brief   Header file - Function prototypes for A/V control
 * @file    osdtype.h
 * @date    06/02/2014
 */

// pre-processor mechanism so multiple inclusions don't cause compilation error
#ifndef _OSDTYPE_H
#define _OSDTYPE_H

#include "techtype.h"

//---Constant and macro definitions for public use-----------------------------

#define SD_HEIGHT        576
#define SD_WIDTH         720
#define HD_HEIGHT        720
#define HD_WIDTH        1280
#define FULL_HD_HEIGHT  1080
#define FULL_HD_WIDTH   1920

//---Enumerations for public use-----------------------------------------------

typedef enum
{
    ASPECT_RATIO_4_3,
    ASPECT_RATIO_16_9,
    ASPECT_UNDEFINED = 255
} E_ASPECT_RATIO;

typedef enum e_blit_op
{
    STB_BLIT_COPY,
    STB_BLIT_A_BLEND
} E_BLIT_OP;

//---Global type defs for public use-------------------------------------------

typedef struct
{
    S32BIT left;
    S32BIT top;
    U32BIT width;
    U32BIT height;
} S_RECTANGLE;

typedef struct
{
    S32BIT s1;
    S32BIT s2;
    U32BIT u1;
    U32BIT u2;
} S_QVALUE;

typedef struct
{
    U32BIT path;
    S_QVALUE values;
} S_QVALUE_EX;


#endif /*_OSDTYPE_H*/
