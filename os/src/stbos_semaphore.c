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


#include <stdlib.h>
#include <semaphore.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/time.h>
#include <errno.h>


/* STB Header Files */
#include "techtype.h"
#include "dbgfuncs.h"
#include "stbhwos.h"
#include "stbhwmem.h"

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
   sem_t *sem;
   int err;

   FUNCTION_START(STB_OSCreateSemaphore);

   sem = (sem_t *)STB_MEMGetSysRAM(sizeof(sem_t));
   if (sem != NULL)
   {
      /* Initialise the semaphore with a count of 1 and allow it to be shared with
       * other threads but not other processes */
      err = sem_init(sem, 0, 1);
      if (err != 0)
      {
         STB_MEMFreeSysRAM(sem);
         sem = NULL;

         SEM_DBG("Failed to init semaphore, error %d", errno);
      }
   }

   FUNCTION_FINISH(STB_OSCreateSemaphore);

   return((void *)sem);
}

/**
 * @brief   Create a counting semaphore.
 * @param   value - initial value for semaphore.
 * @return  Seamphore handle upon success, or NULL upon failure.
 */
void* STB_OSCreateCountSemaphore(U32BIT value)
{
   sem_t *sem;
   int err;

   FUNCTION_START(STB_OSCreateCountSemaphore);

   sem = (sem_t *)STB_MEMGetSysRAM(sizeof(sem_t));
   if (sem != NULL)
   {
      /* Initialise the semaphore with a count of 1 and allow it to be shared with
       * other threads but not other processes */
      err = sem_init(sem, 0, value);
      if (err != 0)
      {
         STB_MEMFreeSysRAM(sem);
         sem = NULL;

         SEM_DBG("Failed to init semaphore, error %d", errno);
      }
   }

   FUNCTION_FINISH(STB_OSCreateCountSemaphore);

   return((void *)sem);
}

/**
 * @brief   Delete a Semaphore
 * @param   semaphore - Semaphore handle.
 * @return  TRUE for success, FALSE upon failure.
 */
void STB_OSDeleteSemaphore(void *semaphore)
{
   int err;

   FUNCTION_START(STB_OSDeleteSemaphore);

   if (semaphore != NULL)
   {
      err = sem_destroy((sem_t *)semaphore);

      if (err != 0)
      {
         SEM_DBG("Failed to destroy semaphore 0x%x, error %d", semaphore, errno);
      }

      STB_MEMFreeSysRAM(semaphore);
   }

   FUNCTION_FINISH(STB_OSDeleteSemaphore);
}

/**
 * @brief   Signal a Semaphore to Release it by decrementing its counter.
 * @param   semaphore Semaphore handle.
 */
void STB_OSSemaphoreSignal(void *semaphore)
{
   int err;

   FUNCTION_START(STB_OSSemaphoreSignal);

   if (semaphore != NULL)
   {
      err = sem_post((sem_t *)semaphore);
      if (err != 0)
      {
         SEM_DBG("Failed to unlock semaphore 0x%p, error %d", semaphore, errno);
      }
   }
   else
   {
      SEM_DBG("NULL semaphore");
   }

   FUNCTION_FINISH(STB_OSSemaphoreSignal);
}

/**
 * @brief   Wait on Semaphore Indefinity or Until Released.
 * @param   semaphore Semaphore handle.
 * @return  TRUE for success, FALSE upon failure.
 */
void STB_OSSemaphoreWait(void *semaphore)
{
   int err;

   FUNCTION_START(STB_OSSemaphoreWait);

   if (semaphore != NULL)
   {
      err = sem_wait((sem_t *)semaphore);

      if (err != 0)
      {
         SEM_DBG("Failed to lock semaphore 0x%p, error %d", semaphore, errno);
      }
   }
   else
   {
      SEM_DBG("NULL semaphore");
   }

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
   struct timeval tv;
   struct timespec abs_timeout;
   U32BIT usec;
   int err;
   BOOLEAN result;

   FUNCTION_START(STB_OSSemaphoreWaitTimeout);

   if (semaphore != NULL)
   {
      result = TRUE;

      gettimeofday(&tv, NULL);
      usec = tv.tv_usec + (1000 * timeout);
      while (usec >= 1000000)
      {
         usec -= 1000000;
         tv.tv_sec++;
      }
      abs_timeout.tv_sec = tv.tv_sec;
      abs_timeout.tv_nsec = 1000 * usec;

      do
      {
         err = sem_timedwait((sem_t *)semaphore, &abs_timeout);
      } while (err == -1 && errno == EINTR);   /* Restart when interrupted by handler */

      if (err != 0)
      {
         result = FALSE;
         if (errno != ETIMEDOUT)
         {
            SEM_DBG("Failed to lock semaphore 0x%p, error %d", semaphore, errno);
         }
      }
   }
   else
   {
      SEM_DBG("NULL semaphore");
      result = FALSE;
   }

   FUNCTION_FINISH(STB_OSSemaphoreWaitTimeout);

   return result;
}

