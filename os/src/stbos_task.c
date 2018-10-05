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

/* Local PROTOTYPE Declarations */

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
void* STB_OSCreateTask(void (*function)(void *), void *param, U32BIT stack, U8BIT priority, U8BIT *name)
{
   FUNCTION_START(STB_OSCreateTask);

   USE_UNWANTED_PARAM(name);

   FUNCTION_FINISH(STB_OSCreateTask);

   return(NULL);
}

/**
 * @brief   Delete Task must be called upon termination of each task as it
 *          frees all OS specific resources allocated for the specific task.
 * @param   task task handle to be destroyed
 */
void STB_OSDestroyTask(void *task)
{
   FUNCTION_START(STB_OSDestroyTask);
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
   FUNCTION_START(STB_OSTaskDelay);
   FUNCTION_FINISH(STB_OSTaskDelay);
}

/**
 * @brief   Put Calling Task to Sleep. This is the equivalent of a Task Reschedule which is useful
 *          when a task wants to release the CPU on an application preemption point.
 */
void  STB_OSTaskSleep(void)
{
   FUNCTION_START(STB_OSTaskSleep);
   FUNCTION_FINISH(STB_OSTaskSleep);
}

/**
 * @brief   Wake up a task that was put to sleep when it called STB_OSTaskSleep().
 */
void STB_OSTaskWakeUp(void)
{
   FUNCTION_START(STB_OSTaskWakeUp);
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
   FUNCTION_START(STB_OSTaskPriority);

   FUNCTION_FINISH(STB_OSTaskPriority);

   return(0);
}

/**
 * @brief   Suspend the calling task
 */
void  STB_OSTaskSuspend(void)
{
   FUNCTION_START(STB_OSTaskSuspend);
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

   return NULL;
}

