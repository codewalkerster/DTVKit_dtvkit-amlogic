#**************************************************************************
#  Makefile for platform OS layer
#**************************************************************************
include ../Config.mk
INCLUDES += $(DTVKIT_DVBCORE_ROOT)/inc \
            $(DTVKIT_DVBCORE_ROOT)/platform/inc

SRCDIR = src
SRCS = stbos_event.c      \
       stbos_mutex.c      \
       stbos_queue.c      \
       stbos_rtc.c        \
       stbos_semaphore.c  \
       stbos_task.c       \
       stbos_utils.c

COMP=os

include ../common.mak

clean: common_clean

module_clean:

