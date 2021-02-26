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
 * @brief   Set Top Box - Hardware Layer, Resource functions
 * @file    stbhwresm.c
 * @date    October 2018
 */

#define RESM_DEBUG

/*---includes for this file--------------------------------------------------*/
/* compiler library header files */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

/* STB header files */
#include "dbgfuncs.h"

#include "stbhwresm.h"

/*---Macro Definitions for this file-----------------------------------------*/
#ifdef RESM_DEBUG
   #define RESM_DBG(x,...)        STB_SPDebugWrite("%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )
#else
   #define RESM_DBG(x,...)
#endif

#define RESM_ERR(x,...)        STB_SPDebugWrite("%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )

/*---constant definitions for this file--------------------------------------*/
static int resm_fd = -1;
static BOOLEAN resm_support = FALSE;

/*---local typedef structs for this file-------------------------------------*/
BOOLEAN STB_Resman_Support()
{
    return resman_support();
}

BOOLEAN STB_Resman_Request(S32BIT app_type, S32BIT res_type, S32BIT timeout)
{
    RESM_DBG("dtvkit start request resource %d.", res_type);

    if (resm_fd == -1)
        resm_fd = resman_init("dtvkit", app_type);

    if (!resman_acquire_wait(resm_fd, res_type, timeout)) {
        RESM_ERR("res_auquire(handle = %d, res = %d) failed!!!\n",
                resm_fd, res_type);
        return FALSE;
    }

    RESM_DBG("dtvkit request resource (%d) done.", res_type);

    return TRUE;
}

void STB_Resman_FreeRes(S32BIT res_type)
{
    if (resm_fd == -1)
        return;

    if (resman_release(resm_fd, res_type)) {
        RESM_ERR("Resman_FreeRes(handle = %d, res = %d) failed!!!\n",
                resm_fd, res_type);
        return;
    }
}

void STB_Resman_Release()
{
    if (resm_fd != -1) {
        resman_close(resm_fd);
        resm_fd = -1;
    }
}
