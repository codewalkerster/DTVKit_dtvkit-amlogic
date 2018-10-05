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
 * @brief   Set Top Box - Operating System Interface for Mutexes.
 * @file    stbos_mutex.c
 * @date    October 2018
 */


/*---includes for this file--------------------------------------------------*/

/* compiler library header files */

/* third party header files */

/* DVBCore header files */
#include "techtype.h"
#include "dbgfuncs.h"

/*!- Select-Deselect Local Debug Text Output */
/*#define  MUTEX_DEBUG*/

#ifdef  MUTEX_DEBUG
#define  MUTEX_DBG(x,...)        STB_SPDebugWrite("%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )
#else
#define  MUTEX_DBG(x,...)
#endif


/*---constant definitions for this file--------------------------------------*/

/*---local typedef structs for this file-------------------------------------*/

/*---local (static) variable declarations for this file----------------------*/

/*   (internal variables declared static to make them local) */

/*---local function prototypes for this file---------------------------------*/
/*   (internal functions declared static to make them local) */

/*---global function definitions---------------------------------------------*/

/**
 * @brief   Create a mutex.
 * @return  Newly created mutex, or NULL.
 */
void* STB_OSCreateMutex(void)
{
   FUNCTION_START(STB_OSCreateMutex);
   FUNCTION_FINISH(STB_OSCreateMutex);

   return NULL;
}

/**
 * @brief   Lock a mutex (a.k.a. 'enter', 'wait' or 'get').
 * @param   mutex_var The mutex to lock.
 */
void STB_OSMutexLock(void *mutex_var)
{
   FUNCTION_START(STB_OSMutexLock);
   FUNCTION_FINISH(STB_OSMutexLock);
}

/**
 * @brief   Unlock a mutex (a.k.a. 'leave', 'signal' or 'release')
 * @param   mutex_var The mutex to unlock.
 */
void STB_OSMutexUnlock(void *mutex_var)
{
   FUNCTION_START(STB_OSMutexUnlock);
   FUNCTION_FINISH(STB_OSMutexUnlock);
}

/**
 * @brief   Delete a mutex.
 * @param   mutex_var The mutex to delete.
 */
void STB_OSDeleteMutex(void *mutex_var)
{
   FUNCTION_START(STB_OSDeleteMutex);
   FUNCTION_FINISH(STB_OSDeleteMutex);
}

/*---local function definitions----------------------------------------------*/

