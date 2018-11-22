#**************************************************************************
#  Makefile for platform hardware layer
#**************************************************************************

INCLUDES = ./inc \
           $(DTVKIT_DVBCORE_ROOT)/inc \
           $(DTVKIT_DVBCORE_ROOT)/platform/inc \
           $(DTVKIT_CIPLUS_ROOT)/include

SRCDIR = src
SRCS = \
	stbhwplatform.c \
	stbhwini.c \
	stbhwmem.c \
	stbhwtun.c \
	stbhwdmx.c \
	stbhwav.c  \
	stbhwdsk.c \
	stbhwfp.c  \
	stbhwsp.c  \
	stbhwcrypt.c \
	stbhwupg.c \
	stbhwci.c \
	stbhwosd.c \
	stbpvrpr.c \
	stbhwnet.c \
	stbhwvbi.c

DEFINES += _FILE_OFFSET_BITS=64

ifeq ($(DTVKIT_INCLUDE_HBBTV),1)
SRCS += stbhwmp.c
endif

COMP=hw

include ../common.mak

clean: common_clean

module_clean:

