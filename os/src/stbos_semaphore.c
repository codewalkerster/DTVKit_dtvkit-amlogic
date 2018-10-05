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
 * @brief   Set Top Box - System Interface for Semaphore Mechanism.
 * @file    stbos_semaphore.c
 * @date    October 2018
 */


/* STB Header Files */
#include "techtype.h"
#include "dbgfuncs.h"
#include "stbhwos.h"

/* Local MACRO Definitions */

/*#define  SEM_DEBUG*/

#ifdef  SEM_DEBUG
#define  SEM_DBG(x,...)       STB_SPDebugWrite("%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )
#else
#define  SEM_DBG(x,...)
#endif

/* Local CONSTANT  Definitions */

/* Local ENUM/TYPE Definitions */

/* Local VARIABLE  Declarations */

/* Local PROTOTYPE Declarations */

/**
 * @brief   Create a Semaphore
 * @return  Seamphore Handle Address upon success, or NULL upon failure.
 */
void* STB_OSCreateSemaphore(void)
{
   FUNCTION_START(STB_OSCreateSemaphore);

   FUNCTION_FINISH(STB_OSCreateSemaphore);

   return(NULL);
}

/**
 * @brief   Create a counting semaphore.
 * @param   value - initial value for semaphore.
 * @return  Seamphore handle upon success, or NULL upon failure.
 */
void* STB_OSCreateCountSemaphore(U32BIT value)
{
   FUNCTION_START(STB_OSCreateCountSemaphore);
   FUNCTION_FINISH(STB_OSCreateCountSemaphore);

   return(NULL);
}

/**
 * @brief   Initialise a counting semaphore
 * @param   semaphore - Semaphore handle.
 * @param   value - New value for semaphore.
 * @warning This is a very dangerous function, and should be used
 *           very carefully.
 */
void STB_OSInitCountSemaphore(void *semaphore, U32BIT value)
{
   USE_UNWANTED_PARAM(semaphore);
   USE_UNWANTED_PARAM(value);
}

/**
 * @brief   Delete a Semaphore
 * @param   semaphore - Semaphore handle.
 * @return  TRUE for success, FALSE upon failure.
 */
void STB_OSDeleteSemaphore(void *semaphore)
{
   FUNCTION_START(STB_OSDeleteSemaphore);
   FUNCTION_FINISH(STB_OSDeleteSemaphore);
}

/**
 * @brief   Signal a Semaphore to Release it by decrementing its counter.
 * @param   semaphore Semaphore handle.
 */
void STB_OSSemaphoreSignal(void *semaphore)
{
   FUNCTION_START(STB_OSSemaphoreSignal);
   FUNCTION_FINISH(STB_OSSemaphoreSignal);
}

/**
 * @brief   Wait on Semaphore Indefinity or Until Released.
 * @param   semaphore Semaphore handle.
 * @return  TRUE for success, FALSE upon failure.
 */
void STB_OSSemaphoreWait(void *semaphore)
{
   FUNCTION_START(STB_OSSemaphoreWait);
   FUNCTION_FINISH(STB_OSSemaphoreWait);
}

/**
 * @brief   Wait on Semaphore for Set Time Period in an Attempt to Acquire.
 * @param   semaphore Semaphore handle.
 * @param   timeout Time Period to Wait in milliseconds.
 * @return  TRUE for success, FALSE upon failure.
 */
BOOLEAN STB_OSSemaphoreWaitTimeout(void *semaphore, U16BIT timeout)
{
   BOOLEAN result;

   FUNCTION_START(STB_OSSemaphoreWaitTimeout);

   result = FALSE;

   FUNCTION_FINISH(STB_OSSemaphoreWaitTimeout);

   return result;
}

