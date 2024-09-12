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
 * @brief   Header file - Function prototypes for operating system
 * @file    stbhwos.h
 * @date    12/02/2002
 */

// pre-processor mechanism so multiple inclusions don't cause compilation error
#ifndef _STBHWOS_H

#define _STBHWOS_H

#include "techtype.h"

//---Constant and macro definitions for public use-----------------------------

// hardware event class and types
#define HW_EV_CLASS_HANDSET             0
#define HW_EV_CLASS_KEYPAD              1
#define HW_EV_CLASS_LNB                 2
#define HW_EV_CLASS_TUNER               3
#define HW_EV_CLASS_DECODE              4
#define HW_EV_CLASS_SCART               5
#define HW_EV_CLASS_DISK                6
#define HW_EV_CLASS_DVD                 7
#define HW_EV_CLASS_PVR                 8
#define HW_EV_CLASS_USB                 9
#define HW_EV_CLASS_HDMI               10
#define HW_EV_CLASS_CEC                11

#ifdef SUPPORT_CAS
#define HW_EV_CLASS_PLAY               12
#endif

#define HW_EV_CLASS_TELETEXT           13
#define HW_EV_CLASS_CAS           14
#define HW_EV_CLASS_SUBTITLE           15


#define HW_EV_CLASS_PRIVATE           255    /* Class to allow events to be created that are unique to a platform */

#define HW_EV_TYPE_FALSE                  0
#define HW_EV_TYPE_TRUE                   1
#define HW_EV_TYPE_LOCKED                 2
#define HW_EV_TYPE_NOTLOCKED              3

#define HW_EV_TYPE_AUDIO_STARTED          4
#define HW_EV_TYPE_VIDEO_STARTED          5
#define HW_EV_TYPE_AUDIO_STOPPED          6
#define HW_EV_TYPE_VIDEO_STOPPED          7

#define HW_EV_TYPE_4_3                    8
#define HW_EV_TYPE_16_9                   9

#define HW_EV_TYPE_SIGNAL_DATA_BAD        10
#define HW_EV_TYPE_SIGNAL_DATA_OK         11

#define HW_EV_TYPE_FORMAT_COMPLETE        12
#define HW_EV_TYPE_FORMAT_FAILED          13

#define HW_EV_TYPE_REPAIR_COMPLETE        14
#define HW_EV_TYPE_REPAIR_FAILED          15

#define HW_EV_TYPE_DVD_DISK_INSERTED      16
#define HW_EV_TYPE_DVD_DISK_REMOVED       17

#define HW_EV_TYPE_PVR_REC_START          18
#define HW_EV_TYPE_PVR_REC_STOP           19
#define HW_EV_TYPE_PVR_PLAY_START         20
#define HW_EV_TYPE_PVR_PLAY_STOP          21
#define HW_EV_TYPE_PVR_PLAY_BOF           22    /* Playback has reached the beginning of the file */
#define HW_EV_TYPE_PVR_PLAY_EOF           23    /* Playback has reached the end of the file */
#define HW_EV_TYPE_PVR_PLAY_NOTIFY_TIME   24    /* Playback has reached the end of the file */

#define HW_EV_TYPE_SAMPLE_STOPPED         25

#define HW_EV_TYPE_DISK_CONNECTED         26
#define HW_EV_TYPE_DISK_REMOVED           27
#define HW_EV_TYPE_DISK_FULL              28

#define HW_EV_TYPE_HDMI_CONNECT           29
#define HW_EV_TYPE_HDMI_DISCONNECT        30

#define HW_EV_TYPE_CEC_PLAY               31
#define HW_EV_TYPE_CEC_STANDBY            32

#define HW_EV_TYPE_AD_STARTED             34
#define HW_EV_TYPE_AD_STOPPED             35

#define HW_EV_TYPE_VIDEO_UNDERFLOW        36
#define HW_EV_TYPE_AUDIO_UNDERFLOW        37

#ifdef SUPPORT_CAS
#define HW_EV_TYPE_PLAY_STATE_CHANGED     38
#endif
#define HW_EV_TYPE_SIGNAL_RECOVER         39
#define HW_EV_TYPE_VIDEO_SCAMBLED         40
#define HW_EV_TYPE_AUDIO_SCAMBLED         41
#define HW_EV_TYPE_DECODE_NO_DATA         42
#define HW_EV_TYPE_DECODE_DATA_RESUME     43
#define HW_EV_TYPE_DECODE_VIDEO_FIRST_FRAME     44
#define HW_EV_TYPE_DECODE_INPUT_DATA_LOSS       45
#define HW_EV_TYPE_DECODE_INPUT_DATA_RESUME     46
#define HW_EV_TYPE_DECODE_AUDIO_HEAAC     47

#define HW_EV_TYPE_TELETEXT_NORMAL        50
#define HW_EV_TYPE_TELETEXT_SEPARATE      51

#define HW_EV_TYPE_PVR_REC_STORE          52

#define HW_EV_TYPE_CAS_MSG           	53

#define HW_EV_TYPE_VIDEO_DECODER_PRIV_DATA 54

#define HW_EV_TYPE_RESOURCE_BUSY          55

#define HW_EV_TYPE_VIDEO_ERROR_FRAME_COUNT         56

#define HW_EV_TYPE_VIDEO_UNSUPPORT         57

#define HW_EV_TYPE_VIDEO_CHANGED           58

#define HW_EV_TYPE_PVR_TIMESHIFT_FR_REACHED_BEGIN   59
#define HW_EV_TYPE_PVR_TIMESHIFT_FF_REACHED_END     60
#define HW_EV_TYPE_PVR_START_DECODE                 61

#define EV_TYPE_CAS_HW                                            62
#define EV_TYPE_CAS_LONG_HW                                       63
#define EV_TYPE_CAS_PVR_META_DATA                            64

// timeouts for queues, semaphores etc
#define TIMEOUT_NOW     0
#define TIMEOUT_NEVER   0xffff

//---Enumerations for public use-----------------------------------------------

//---Global type defs for public use-------------------------------------------

//---Global Function prototypes for public use---------------------------------

/**
 * @brief   Allows setting of initial boot time.
 */
void STB_OSInitialise(void);

/**
 * @brief   Register the function that will be called when STB_OSSendEvent is used.
 * @param   func callback function
 */
void STB_OSRegisterCallback(void (*func)(BOOLEAN, U16BIT, U16BIT, void *, U32BIT));

/**
 * @brief   Send an event by calling the registered callback function
 * @param   repeat TRUE if the event is a repeat of the last event
 * @param   event_class event class
 * @param   event_type event identifier.
 * @param   data pointer to the data associated with the event
 * @param   data_size size of the data pointed by data_pointer
 */
void STB_OSSendEvent(BOOLEAN repeat, U16BIT event_class, U16BIT event_type, void *data, U32BIT data_size);

/**
 * @brief   Set the local time in seconds since midnight 1-1-1970
 * @param   num_seconds time in seconds
 */
void STB_OSSetClockRTC(U32BIT num_seconds);

/**
 * @brief   Returns the current time in seconds. This is calculated by using
 *          the set UTC time and adding the difference between the system boot
 *          time when it was set (sync_time) and the system boot time now.
 * @return  The current time in seconds since midnight 1-1-1970.
 */
U32BIT STB_OSGetClockRTC(void);

/**
 * @brief   Set the time in seconds since midnight 1-1-1970 in GMT
 * @param   num_seconds time in seconds
 */
void STB_OSSetClockGMT(U32BIT num_seconds);

/**
 * @brief   Returns the system time in seconds
 * @return  system time in seconds
 */
U32BIT STB_OSGetClockGMT(void);

/**
 * @brief   Get Current Computer Clock Time.
 * @return  Time in Milliseconds.
 */
U32BIT STB_OSGetClockMilliseconds(void);

/**
 * @brief   Get Difference between Given Time and Current Time.
 * @param   timestamp Given Clock Value to Compare Against.
 * @return  Time Difference in MilliSeconds.
 */
U32BIT STB_OSGetClockDiff(U32BIT timestamp);

/**
 * @brief   Set the daylight change time timestamp in seconds
 * @param   num_seconds time in seconds
 */
void STB_OSSetClockOffsetChange(U32BIT num_seconds);

/**
 * @brief   Set the time zone timestamp in seconds
 * @param   num_seconds time in seconds
 */
void STB_OSSetClockTimeZoneDiff(S32BIT num_seconds);

/**
 * @brief   Set the next time zone timestamp in seconds
 * @param   num_seconds time in seconds
 */
void STB_OSSetClockTimeZoneNext(S32BIT num_seconds);

/**
 * @brief   Create a New Task to the calling process. Upon success, the
 *          created task runs on its own stack, as part of the calling
 *          process context.  Task termination must be achieved by calling
 *          STB_OSDestroyTask()
 * @param   function task entry point
 * @param   param user defined parameter passed when task is started
 * @param   stack stack size
 * @param   priority task priority, min 0, max 15
 * @param   name task name, length is restricted to 16 characters, including the terminating null byte ('\0')
 * @return  handle of task
 */
void* STB_OSCreateTask(void *(*function)(void *), void *param, U32BIT stack, U8BIT priority, U8BIT *name);

/**
 * @brief   Suspend the calling task
 */
void STB_OSTaskSuspend(void);

/**
 * @brief   Delay Task for Specifed Time Period.
 * @param   timeout delay in milliSeconds.
 */
void STB_OSTaskDelay(U16BIT timeout);

/**
 * @brief   Put Calling Task to Sleep. This is the equivalent of a Task Reschedule which is useful
 *          when a task wants to release the CPU on an application preemption point.
 */
void STB_OSTaskSleep(void);

/**
 * @brief   Wake up a task that was put to sleep when it called STB_OSTaskSleep().
 */
void STB_OSTaskWakeUp(void);

/**
 * @brief   Delete Task must be called upon termination of each task as it
 *          frees all OS specific resources allocated for the specific task.
 */
void STB_OSDestroyTask(void *task);

/**
 * @brief   Set a New Priority Level for Specified Task.
 * @param   task task whose priority is to be changed
 * @param   priority new priority
 * @return  Old task priority
 */
U8BIT STB_OSTaskPriority(void *task, U8BIT priority);

/**
 * @brief   Lock the calling task. Prevents task scheduler from preempting
 *          calling task and should always be used as a pair with
 *          STB_OSTaskUnlock() to create a critical region of code execution
 *          that cannot be interrupted by another task.
 */
void STB_OSTaskLock(void);

/**
 * @brief   Unlock the calling task. Allows task scheduler to preempting
 *          calling task and should always be used as a pair with
 *          STB_OSTaskLock() to create a critical region of code execution
 *          that cannot be interrupted by another task.
 */
void STB_OSTaskUnlock(void);

/**
 * @brief   Returns the handle of the current task
 * @return  Handle of current task
 */
void* STB_OSGetCurrentTask(void);

/**
 * @brief   Create Queue of given number of messages and size of message
 * @param   msg_size  Queue Message Packet Size
 * @param   num_msgs  Queue Message Depth in Packets
 * @return  Queue Handle - Number for success, NULL upon failure.
 */
void* STB_OSCreateQueue(U16BIT msg_size, U16BIT msg_max);

/**
 * @brief   Read a message from a queue
 * @param   queue Queue Handle
 * @param   data User's Read Message Buffer Start Address.
 * @param   msg_size Message Packet Size in Bytes.
 * @param   timeout timeout in milliseconds
 * @return  TRUE for success, FALSE upon failure.
 */
BOOLEAN STB_OSReadQueue(void *queue, void *msg, U16BIT msg_size, U16BIT timeout);

/**
 * @brief   Write a message to the queue
 * @param   queue Queue Handle
 * @param   data message to be queued
 * @param   msg_size size of message in bytes
 * @param   timeout timeout in milliseconds
 * @return  TRUE for success, FALSE upon failure.
 */
BOOLEAN STB_OSWriteQueue(void *queue, void *msg, U16BIT msg_size, U16BIT timeout);

/**
 * @brief   Destroy Queue
 * @param   queue Unique Queue Handle Identifier Variable Address.
 * @return  TRUE for success, FALSE upon failure.
 */
BOOLEAN STB_OSDestroyQueue(void *queue);

/**
 * @brief   Create a Semaphore
 * @return  Seamphore Handle Address upon success, or NULL upon failure.
 */
void* STB_OSCreateSemaphore(void);

/**
 * @brief   Create a counting semaphore.
 * @param   value initial value for semaphore.
 * @return  Seamphore handle upon success, or NULL upon failure.
 */
void* STB_OSCreateCountSemaphore(U32BIT value);

/**
 * @brief   Initialise a counting semaphore
 * @param   semaphore Semaphore handle.
 * @param   value New value for semaphore.
 * @warning This is a very dangerous function, and should be used
 *           very carefully.
 */
void STB_OSInitCountSemaphore(void *semaphore, U32BIT value);

/**
 * @brief   Delete a Semaphore
 * @param   semaphore Semaphore handle.
 * @return  TRUE for success, FALSE upon failure.
 */
void STB_OSDeleteSemaphore(void *semaphore);

/**
 * @brief   Wait on Semaphore indefinite or Until Released.
 * @param   semaphore Semaphore handle.
 * @return  TRUE for success, FALSE upon failure.
 */
void STB_OSSemaphoreWait(void *semaphore);

/**
 * @brief   Signal a Semaphore to Release it by decrementing its counter.
 * @param   semaphore Semaphore handle.
 */
void STB_OSSemaphoreSignal(void *semaphore);

/**
 * @brief   Wait on Semaphore for Set Time Period in an Attempt to Acquire.
 * @param   semaphore Semaphore handle.
 * @param   timeout Time Period to Wait in milliseconds.
 * @return  TRUE for success, FALSE upon failure.
 */
BOOLEAN STB_OSSemaphoreWaitTimeout(void *semaphore, U16BIT timeout);

/**
 * @brief   Create a mutex.
 * @return  Newly created mutex, or NULL.
 */
void* STB_OSCreateMutex(void);

/**
 * @brief   Lock a mutex (a.k.a. 'enter', 'wait' or 'get').
 * @param   mutex_var The mutex to lock.
 */
void STB_OSMutexLock(void *mutex);


/**
 * @brief   Lock a mutex (a.k.a. 'enter', 'wait' or 'get').
 * @param   mutex_var The mutex to lock.
 * @param   timeout wait time,unit is ms.
 */

S32BIT STB_OSMutexLockTimeout(void *mutex_var,U32BIT timeout);

/**
 * @brief   Unlock a mutex (a.k.a. 'leave', 'signal' or 'release')
 * @param   mutex_var The mutex to unlock.
 */
void STB_OSMutexUnlock(void *mutex);

/**
 * @brief   Delete a mutex.
 * @param   mutex_var The mutex to delete.
 */
void STB_OSDeleteMutex(void *mutex);

/**
 * @brief   Reset the board
 */
void STB_OSResetCPU(void);

#endif //  _STBHWOS_H

//*****************************************************************************
// End of file
//*****************************************************************************

