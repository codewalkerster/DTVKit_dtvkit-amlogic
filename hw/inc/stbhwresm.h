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
 * @brief   Header file - Function prototypes for tuner control
 * @file    stbhwresm.h
 * @date    06/02/2001
 */

// pre-processor mechanism so multiple inclusions don't cause compilation error

#ifndef _STBHWRESM_H

#define _STBHWRESM_H

//---#includes for this file---------------------------------------------------
#include "techtype.h"

/* third party header files */
#include <stdbool.h>
#include <resourcemanage.h>

//---Constant and macro definitions for public use-----------------------------
BOOLEAN STB_Resman_Support();
BOOLEAN STB_Resman_Request(S32BIT app_type, S32BIT res_type, S32BIT timeout);
void STB_Resman_FreeRes(S32BIT res_type);
void STB_Resman_Release();

#endif //  _STBHWRESM_H

