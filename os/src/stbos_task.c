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
 * @brief   Set Top Box - Linux Op.System Interface for Task Mechanism.
 * @file    stbos_task.c
 * @date    October 2018
 */

// compiler library header files
#define _GNU_SOURCE
#include <pthread.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/prctl.h>

/* STB Header Files */
#include "techtype.h"
#include "dbgfuncs.h"
#include "stbhwos.h"

/* Local MACRO Definitions */
/*#define TASK_DEBUG*/

#ifdef  TASK_DEBUG
#define  TASK_DBG(x,...)         STB_SPDebugWrite("%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )
#else
#define  TASK_DBG(x,...)
#endif

/* Local ENUM/TYPE Definitions */

/* Local VARIABLE  Declarations */
static pthread_mutex_t sleep_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  sleep_cond = PTHREAD_COND_INITIALIZER;

/* Local PROTOTYPE Declarations */
static int MapToOSPriority(U8BIT priority);


/**
 * @brief   Create a New Task to the calling process. Upon success, the
 *          created task runs on its own stack, as part of the calling
 *          process context.  Task termination must be achieved by calling
 *          STB_OSDestroyTask()
 * @param   function task entry point
 * @param   param user defined parameter passed when task is started
 * @param   stack stack size
 * @param   priority task priority, min 0, max 15
 * @param   name task name
 * @return  handle of task
 */
void* STB_OSCreateTask(void *(*function)(void *), void *param, U32BIT stack, U8BIT priority, U8BIT *name)
{
   pthread_attr_t attr;
   struct sched_param parm;
   pthread_t handle;
   int err;

   FUNCTION_START(STB_OSCreateTask);

   // Create a set of default creation attributes
   pthread_attr_init(&attr);

   if (stack > 0)
   {
      TASK_DBG("not set stack size");
      //pthread_attr_setstacksize(&attr, stack);
   }

   // Ensure thread is detached so resources are freed on exit
   err = pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
   if (err == 0)
   {
      parm.sched_priority = MapToOSPriority(priority);
      pthread_attr_setschedparam(&attr, &parm);
   }

   // Create the task
   err = pthread_create(&handle, &attr, function, param);
   if (err != 0)
   {
      TASK_DBG("Failed to create task %s, err=%d (%s)", name, err, strerror(err));
      handle = 0;
   }
   pthread_setname_np(handle, name);

   // Destroy the creation attributes
   pthread_attr_destroy(&attr);

   FUNCTION_FINISH(STB_OSCreateTask);

   return((void *)handle);
}

/**
 * @brief   Delete Task must be called upon termination of each task as it
 *          frees all OS specific resources allocated for the specific task.
 * @param   task task handle to be destroyed
 */
void STB_OSDestroyTask(void *task)
{
   void *status;

   FUNCTION_START(STB_OSDestroyTask);

   if (task != NULL)
   {
      pthread_join((pthread_t)task, &status);
   }

   FUNCTION_FINISH(STB_OSDestroyTask);
}

/**
 * @brief   Lock the calling task. Prevents task scheduler from preempting
 *          calling task and should always be used as a pair with
 *          STB_OSTaskUnlock() to create a critical region of code execution
 *          that cannot be interrupted by another task.
 */
void STB_OSTaskLock(void)
{
   FUNCTION_START(STB_OSTaskLock);
   FUNCTION_FINISH(STB_OSTaskLock);
}

/**
 * @brief   Unlock the calling task. Allows task scheduler to preempting
 *          calling task and should always be used as a pair with
 *          STB_OSTaskLock() to create a critical region of code execution
 *          that cannot be interrupted by another task.
 */
void STB_OSTaskUnlock(void)
{
   FUNCTION_START(STB_OSTaskUnlock);
   FUNCTION_FINISH(STB_OSTaskUnlock);
}

/**
 * @brief   Delay Task for Specifed Time Period.
 * @param   timeout delay in milliSeconds.
 */
void STB_OSTaskDelay(U16BIT timeout)
{
   struct timespec t;

   FUNCTION_START(STB_OSTaskDelay);

   t.tv_sec = timeout / 1000;
   t.tv_nsec = ((U32BIT)timeout % 1000) * 1000000L;

   while ((nanosleep(&t, &t) < 0) && (errno == EINTR))
      ;

   FUNCTION_FINISH(STB_OSTaskDelay);
}

/**
 * @brief   Put Calling Task to Sleep. This is the equivalent of a Task Reschedule which is useful
 *          when a task wants to release the CPU on an application preemption point.
 */
void  STB_OSTaskSleep(void)
{
   FUNCTION_START(STB_OSTaskSleep);

   /* Lock the mutex */
   pthread_mutex_lock(&sleep_mutex);

   /* Wait on the cond variable */
   pthread_cond_wait(&sleep_cond, &sleep_mutex);
   pthread_mutex_unlock(&sleep_mutex);

   FUNCTION_FINISH(STB_OSTaskSleep);
}

/**
 * @brief   Wake up a task that was put to sleep when it called STB_OSTaskSleep().
 */
void STB_OSTaskWakeUp(void)
{
   FUNCTION_START(STB_OSTaskWakeUp);

   pthread_mutex_lock(&sleep_mutex);
   pthread_cond_broadcast(&sleep_cond);
   pthread_mutex_unlock(&sleep_mutex);

   FUNCTION_FINISH(STB_OSTaskWakeUp);
}


/**
 * @brief   Set a New Priority Level for Specified Task.
 * @param   task task whose priority is to be changed
 * @param   priority new priority
 * @return  Old task priority
 */
U8BIT STB_OSTaskPriority(void *task, U8BIT priority)
{
   struct sched_param parm;

   FUNCTION_START(STB_OSTaskPriority);

   if (task != NULL)
   {
      memset(&parm, 0, sizeof(parm));

      parm.sched_priority = MapToOSPriority(priority);
      pthread_setschedparam(*((pthread_t *)task), SCHED_FIFO, &parm);
   }

   FUNCTION_FINISH(STB_OSTaskPriority);

   return(0);
}

/**
 * @brief   Suspend the calling task
 */
void  STB_OSTaskSuspend(void)
{
   FUNCTION_START(STB_OSTaskSuspend);

   sched_yield();

   FUNCTION_FINISH(STB_OSTaskSuspend);
}

/**
 * @brief   Returns the handle of the current task
 * @return  Handle of current task
 */
void* STB_OSGetCurrentTask(void)
{
   FUNCTION_START(STB_OSGetCurrentTask);
   FUNCTION_FINISH(STB_OSGetCurrentTask);

   return((void *)pthread_self());
}


static int MapToOSPriority(U8BIT priority)
{
   int min_priority, max_priority;
   int priority_range;
   int os_priority;

   FUNCTION_START(MapToOSPriority);

   min_priority = sched_get_priority_min(SCHED_FIFO);
   max_priority = sched_get_priority_max(SCHED_FIFO);
   priority_range = (max_priority - min_priority) + 1;

   os_priority = min_priority + (priority * priority_range) / 16;

   FUNCTION_FINISH(MapToOSPriority);

   return(os_priority);
}

