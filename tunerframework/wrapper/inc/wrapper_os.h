#ifndef _TF_OS_H
#define _TF_OS_H

#include "techtype.h"

#ifdef __cplusplus
extern "C" {
#endif
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <poll.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>




#define TIMEOUT_NEVER   0xffff
#define TIMEOUT_NOW     0



typedef struct queue
{
   void *array;
   void *read_ptr;
   void *write_ptr;
   void *end_ptr;

   U16BIT elem_size;
   U16BIT queue_size;
   U16BIT elem_count;

   pthread_mutex_t mutex;
   pthread_cond_t cond;
} S_QUEUE;


void* wrapper_OSCreateQueue(U16BIT msg_size, U16BIT num_msgs);
BOOLEAN wrapper_OSReadQueue(void *queue_handle, void *data, U16BIT msg_size, U16BIT timeout);
BOOLEAN wrapper_OSWriteQueue(void *queue_handle, void *data, U16BIT msg_size, U16BIT timeout);
BOOLEAN wrapper_OSDestroyQueue(void *queue_handle);
void* wrapper_OSCreateTask(void (*function)(void *), void *param, U8BIT *name);
void* wrapper_MEMGetSysRAM(U32BIT bytes);
void wrapper_MEMFreeSysRAM(void *block_ptr);
static void CalcAbstime(U32BIT timeout, struct timespec *ts);
static BOOLEAN WaitNoTimeout(S_QUEUE *queue);
static BOOLEAN WaitTimeout(S_QUEUE *queue, struct timespec *abstime);
void STB_SPDebugWrite(const char *format, ... );


#ifdef __cplusplus
}
#endif

#endif


