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
 * @brief   Set Top Box - Operating system Interface for Queues
 * @file    stbos_queue.c
 * @date    October 2018
 */


/* STB Header Files */
#include "techtype.h"
#include "dbgfuncs.h"
#include "stbhwos.h"

/* Local MACRO Definitions */

/*!- Select-Deselect Local Debug Text Output */
/*#define   QUEUE_DEBUG*/

#ifdef  QUEUE_DEBUG
#define  QUEUE_DBG(x,...)        STB_SPDebugWrite("%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )
#else
#define  QUEUE_DBG(x,...)
#endif

/* Local CONSTANT  Definitions */

/* Local ENUM/TYPE Definitions */

/* Local VARIABLE  Declarations */

/* Local PROTOTYPE Declarations */


/**
 * @brief   Create Queue of given number of messages and size of message
 * @param   msg_size  - Queue Message Packet Size
 * @param   num_msgs  - Queue Message Depth in Packets
 * @return  Queue Handle - Number for success, NULL upon failure.
 */
void* STB_OSCreateQueue(U16BIT msg_size, U16BIT num_msgs)
{
   FUNCTION_START(STB_OSCreateQueue);
   FUNCTION_FINISH(STB_OSCreateQueue);

   return NULL;
}

/**
 * @brief   Destroy Queue
 * @param   queue_handle - Unique Queue Handle Identifier Variable Address.
 * @return  TRUE for success, FALSE upon failure.
 */
BOOLEAN STB_OSDestroyQueue(void *queue_handle)
{
   BOOLEAN success;

   FUNCTION_START(STB_OSDestroyQueue);

   success = FALSE;

   FUNCTION_FINISH(STB_OSDestroyQueue);

   return success;
}

/**
 * @brief   Read a message from a queue
 * @param   queue - Queue Handle
 * @param   data - User's Read Message Buffer Start Address.
 * @param   msg_size - Message Packet Size in Bytes.
 * @param   timeout - timeout in milliseconds
 * @return  TRUE for success, FALSE upon failure.
 */
BOOLEAN STB_OSReadQueue(void *queue_handle, void *data, U16BIT msg_size, U16BIT timeout)
{
   BOOLEAN success;

   FUNCTION_START(STB_OSReadQueue);

   ASSERT(queue != NULL);
   ASSERT(data != NULL);

   success = FALSE;

   USE_UNWANTED_PARAM(msg_size);

   FUNCTION_FINISH(STB_OSReadQueue);

   return success;
}

/**
 * @brief   Write a message to the queue
 * @param   queue - Queue Handle
 * @param   data - message to be queued
 * @param   msg_size - size of message in bytes
 * @param   timeout - timeout in milliseconds
 * @return  TRUE for success, FALSE upon failure.
 */
BOOLEAN STB_OSWriteQueue(void *queue_handle, void *data, U16BIT msg_size, U16BIT timeout)
{
   BOOLEAN success;

   FUNCTION_START(STB_OSWriteQueue);

   ASSERT(queue != NULL);
   ASSERT(data != NULL);

   USE_UNWANTED_PARAM(msg_size);

   success = FALSE;

   FUNCTION_FINISH(STB_OSWriteQueue);

   return success;
}

