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
#include <pthread.h>

/* third party header files */

/* DVBCore header files */
#include "techtype.h"
#include "dbgfuncs.h"
#include "stbhwmem.h"

/*!- Select-Deselect Local Debug Text Output */
/*#define  MUTEX_DEBUG*/

#ifdef  MUTEX_DEBUG
#define  MUTEX_DBG(x,...)        STB_SPDebugWrite("%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )
#else
#define  MUTEX_DBG(x,...)
#endif


/*---constant definitions for this file--------------------------------------*/

/*---local typedef structs for this file-------------------------------------*/
struct mutex_s
{
   pthread_mutex_t lock;
   pthread_t thread_id;
   U32BIT lock_count;
};

typedef struct mutex_s mutex_t;

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
   mutex_t *mutex_handle;
   pthread_mutexattr_t mutex_attr_ptr;

   FUNCTION_START(STB_OSCreateMutex);

   mutex_handle = (mutex_t *)STB_MEMGetSysRAM(sizeof(mutex_t));
   if (mutex_handle != NULL)
   {
      pthread_mutexattr_init(&mutex_attr_ptr);
      if (pthread_mutex_init(&mutex_handle->lock, &mutex_attr_ptr) == 0)
      {
         mutex_handle->thread_id = 0;
         mutex_handle->lock_count = 0;
      }
   }

   MUTEX_DBG("Created mutex 0x%p", mutex_handle);

   FUNCTION_FINISH(STB_OSCreateMutex);

   return((void *)mutex_handle);
}

/**
 * @brief   Lock a mutex (a.k.a. 'enter', 'wait' or 'get').
 * @param   mutex_var The mutex to lock.
 */
void STB_OSMutexLock(void *mutex_var)
{
   mutex_t *mutex_handle;

   FUNCTION_START(STB_OSMutexLock);

   if (mutex_var != NULL)
   {
      mutex_handle = (mutex_t *)mutex_var;

      if (pthread_equal(mutex_handle->thread_id, pthread_self()) != 0)
      {
         /* This thread already owns the mutex so just increment the lock count */
         mutex_handle->lock_count++;
      }
      else
      {
         /* The mutex is either already owned by another thread or isn't owned at all
            so just attempt to acquire it. */
         pthread_mutex_lock((pthread_mutex_t *) &mutex_handle->lock);

         mutex_handle->thread_id = pthread_self();
         mutex_handle->lock_count = 1;
      }
   }
   else
   {
      MUTEX_DBG("NULL mutex");
   }

   FUNCTION_FINISH(STB_OSMutexLock);
}

/**
 * @brief   Unlock a mutex (a.k.a. 'leave', 'signal' or 'release')
 * @param   mutex_var The mutex to unlock.
 */
void STB_OSMutexUnlock(void *mutex_var)
{
   mutex_t *mutex_handle;

   FUNCTION_START(STB_OSMutexUnlock);

   if (mutex_var != NULL)
   {
      mutex_handle = (mutex_t *)mutex_var;

      /* if the same thread as original lock, decrement counter */
      if (pthread_equal(mutex_handle->thread_id, pthread_self()) != 0)
      {
         mutex_handle->lock_count--;

         if (mutex_handle->lock_count == 0)
         {
            /* Mutex has now been released by this thread */
            mutex_handle->thread_id = 0;
            pthread_mutex_unlock((pthread_mutex_t *) &mutex_handle->lock);
         }
      }
      else
      {
         MUTEX_DBG("Thread doesn't own mutex %p", mutex_var);
      }
   }
   else
   {
      MUTEX_DBG("NULL mutex");
   }

   FUNCTION_FINISH(STB_OSMutexUnlock);
}

/**
 * @brief   Delete a mutex.
 * @param   mutex_var The mutex to delete.
 */
void STB_OSDeleteMutex(void *mutex_var)
{
   mutex_t *mutex_handle;

   FUNCTION_START(STB_OSDeleteMutex);

   if (mutex_var != NULL)
   {
      mutex_handle = (mutex_t *)mutex_var;

      pthread_mutex_destroy((pthread_mutex_t *) &mutex_handle->lock);

      STB_MEMFreeSysRAM(mutex_var);
   }
   else
   {
      MUTEX_DBG("NULL mutex");
   }

   FUNCTION_FINISH(STB_OSDeleteMutex);
}

/*---local function definitions----------------------------------------------*/

