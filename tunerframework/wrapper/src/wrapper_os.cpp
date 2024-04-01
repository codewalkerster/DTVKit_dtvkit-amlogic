//! C/C++
#include "wrapper_os.h"

//wrapper os api
//#define WRAPPER_DBG(x,...)          STB_SPDebugWrite("%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )

#define ASSERT(condition)   assert(condition);

static char debug_msg_buff[512];

void* wrapper_OSCreateQueue(U16BIT msg_size, U16BIT num_msgs)
{
   S_QUEUE *queue;
   int rc;

   queue = (S_QUEUE *)wrapper_MEMGetSysRAM(sizeof(S_QUEUE));
   if (queue != NULL)
   {
      rc = pthread_mutex_init(&queue->mutex, NULL);
      if (rc == 0)
      {
         rc = pthread_cond_init(&queue->cond, NULL);
         if (rc == 0)
         {
            queue->elem_size = msg_size;
            queue->queue_size = num_msgs;
            queue->elem_count = 0;
            queue->array = wrapper_MEMGetSysRAM(queue->elem_size * queue->queue_size);
            if (queue->array != NULL)
            {
               queue->read_ptr = queue->array;
               queue->write_ptr = queue->array;
               queue->end_ptr = (char *)queue->array + queue->elem_size * queue->queue_size;
            }
            else
            {
               pthread_mutex_destroy(&queue->mutex);
               pthread_cond_destroy(&queue->cond);
               wrapper_MEMFreeSysRAM(queue);
               queue = NULL;
            }
         }
         else
         {
            pthread_mutex_destroy(&queue->mutex);
            wrapper_MEMFreeSysRAM(queue);
            queue = NULL;
         }
      }
      else
      {
         wrapper_MEMFreeSysRAM(queue);
         queue = NULL;
      }
   }

   return((void *)queue);
}
BOOLEAN wrapper_OSReadQueue(void *queue_handle, void *data, U16BIT msg_size, U16BIT timeout)
{
   S_QUEUE *queue;
   struct timespec abstime;
   BOOLEAN success;

   success = FALSE;

   if ((queue_handle != NULL) && (data != NULL))
   {
      queue = (S_QUEUE*)queue_handle;

      if (queue->elem_size == msg_size)
      {
         pthread_mutex_lock(&queue->mutex);
         //FVP_OS_ERR("Read:Msg count %d!",queue->elem_count);
         success = TRUE;
         if (timeout == TIMEOUT_NOW)
         {
            /* No timeout */
            if (queue->elem_count == 0)
            {
               success = FALSE;
            }
         }
         else if (timeout == TIMEOUT_NEVER)
         {
            while ((queue->elem_count == 0) && success)
            {
               success = WaitNoTimeout(queue);
            }
         }
         else
         {
            CalcAbstime(timeout, &abstime);

            /* success = not timed-out yet */
            while ((queue->elem_count == 0) && success)
            {
               success = WaitTimeout(queue, &abstime);
            }
         }

         if (success)
         {
            /* There is something in the queue */
            memcpy(data, queue->read_ptr, queue->elem_size);
            queue->read_ptr = (char *)queue->read_ptr + queue->elem_size;
            if (queue->read_ptr == queue->end_ptr)
            {
               queue->read_ptr = queue->array;
            }
            --queue->elem_count;
            pthread_cond_broadcast(&queue->cond);
         }

         pthread_mutex_unlock(&queue->mutex);
      }
   }

   return success;
}

BOOLEAN wrapper_OSWriteQueue(void *queue_handle, void *data, U16BIT msg_size, U16BIT timeout)
{
    S_QUEUE *queue;
    struct timespec abstime;
    BOOLEAN success;


    success = FALSE;

    if ((queue_handle != NULL) && (data != NULL))
    {
        queue = (S_QUEUE*)queue_handle;

        if (queue->elem_size == msg_size)
        {
            pthread_mutex_lock(&queue->mutex);
            success = TRUE;

            if (queue->elem_count == queue->queue_size)
            {
                success = FALSE;
            }

            if (timeout == TIMEOUT_NOW)
            {
                /* No timeout */
            }
            else if (timeout == TIMEOUT_NEVER)
            {
                while ((queue->elem_count == queue->queue_size) && success)
                {
                    success = WaitNoTimeout(queue);
                }
            }
            else
            {
                CalcAbstime(timeout, &abstime);
                /* success = not timed-out yet */
                while ((queue->elem_count == queue->queue_size) && success)
                {
                    success = WaitTimeout(queue, &abstime);
                }
            }

            if (success)
            {
                /* There is space in the queue */
                memcpy(queue->write_ptr, data, queue->elem_size);
                queue->write_ptr = (char *)queue->write_ptr + queue->elem_size;
                if (queue->write_ptr == queue->end_ptr)
                {
                    queue->write_ptr = queue->array;
                }
                ++queue->elem_count;
                pthread_cond_broadcast(&queue->cond);
            }

            pthread_mutex_unlock(&queue->mutex);
        }
    }


    return success;
}

BOOLEAN wrapper_OSDestroyQueue(void *queue_handle)
{
   S_QUEUE *queue = (S_QUEUE*)queue_handle;
   BOOLEAN success;


   if (queue != NULL)
   {
      pthread_mutex_destroy(&queue->mutex);
      pthread_cond_destroy(&queue->cond);
      wrapper_MEMFreeSysRAM(queue->array);
      wrapper_MEMFreeSysRAM(queue);
      success = TRUE;
   }
   else
   {
      success = FALSE;
   }


   return success;
}

void* wrapper_OSCreateTask(void (*function)(void *), void *param, U8BIT *name)
{
   pthread_t thread;
   pthread_attr_t attr;

   /* Create a set of default creation attributes */
   pthread_attr_init(&attr);

   /* Ensure thread is detached so resources are freed on exit */
   pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

   /* Create the task */
   pthread_create(&thread, &attr, (void *(*)(void *))function, param);

   /* Destroy the creation attributes */
   pthread_attr_destroy(&attr);

   pthread_setname_np(thread, (const char *)name);


   return((void *)thread);
}

void* wrapper_MEMGetSysRAM(U32BIT bytes)
{
   void *retval;

   //FUNCTION_START(FVP_MEMGetSysRAM);

   retval = NULL;

   if (bytes > 0)
   {
      retval = malloc((size_t)bytes);
      if (retval == NULL)
      {
         //WRAPPER_DBG("Get Memory Failed!");
      }
   }

   //FUNCTION_FINISH(FVP_MEMGetSysRAM);

   return(retval);
}

void wrapper_MEMFreeSysRAM(void *block_ptr)
{
   //FUNCTION_START(FVP_MEMFreeSysRAM);

   if (block_ptr != NULL)
   {
      free(block_ptr);
   }

   //FUNCTION_FINISH(FVP_MEMFreeSysRAM);
}

static void CalcAbstime(U32BIT timeout, struct timespec *ts)
{
   U32BIT nsec;
   U32BIT msec;
   U32BIT sec;
   struct timeval tv;

   gettimeofday(&tv, NULL);

   ts->tv_sec = tv.tv_sec;
   ts->tv_nsec = tv.tv_usec * 1000;

   sec = timeout / 1000;                  /* Seconds in timeout */
   msec = timeout - sec * 1000;           /* Milliseconds only (less than a second) */
   nsec = ts->tv_nsec + msec * 1000000L;  /* Total nanoseconds - between 0 and 2 seconds */
   if (nsec > 1000000000L)
   {
      /* nsec is no more than 2 seconds, make it less than one */
      nsec -= 1000000000L;
      ++sec;
   }

   ts->tv_sec += sec;
   ts->tv_nsec = nsec;
}

static BOOLEAN WaitNoTimeout(S_QUEUE *queue)
{
   BOOLEAN signalled;
   int rc;

   rc = pthread_cond_wait(&queue->cond, &queue->mutex);
   if (rc == 0)
   {
      signalled = TRUE;
   }
   else
   {
      signalled = FALSE;
   }

   return signalled;
}

static BOOLEAN WaitTimeout(S_QUEUE *queue, struct timespec *abstime)
{
   BOOLEAN signalled;
   int rc;

   rc = pthread_cond_timedwait(&queue->cond, &queue->mutex, abstime);
   if (rc == 0)
   {
      signalled = TRUE;
   }
   else
   {
      signalled = FALSE;
   }

   return signalled;
}

