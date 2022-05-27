#**************************************************************************
#  Makefile for platform hardware layer
#**************************************************************************
include ../Config.mk
INCLUDES = ./inc \
          $(DTVKIT_DVBCORE_ROOT)/inc \
          $(DTVKIT_DVBCORE_ROOT)/platform/inc \
          $(DTVKIT_DVBCORE_ROOT)/midware/CA/inc \
          $(DTVKIT_DVBCORE_ROOT)/midware/stb/inc \
          $(DTVKIT_DVBCORE_ROOT)/CERT/inc \
          $(DTVKIT_DVBCORE_ROOT)/dvb/inc \
          $(DTVKIT_CIPLUS_ROOT)/include \
          $(DTVKIT_MHEG5_ROOT)/include

ifneq ($(RDK_COMPILE),1)
INCLUDES += \
           $(DTVKIT_RPCSERVICE_ROOT)/modules/binderservice/inc
endif



SRCDIR = src
SRCS = stbhwplatform.c \
	stbhwini.c \
	stbhwmem.c \
	stbhwtun.c \
	stbhwdmx.c \
	stbhwdsk.c \
	stbhwfp.c  \
	stbhwsp.c  \
	stbhwcrypt.c \
	stbhwupg.c \
	stbhwosd.c \
	stbhwnet.c \
     stbhwvbi.c \
     stbhwcfg.c \
     stbhwresm.c \
     linuxdvbdmx_wrapper.c \
     stbhwav_amlmp.c \
     stbpvrpr_amlmp.c \
     ca_glue_amlmp.c \
     emu_tuner.c \
     emu_dmx.c \
     emu_config.c \
     stbhwci.c



DEFINES += _FILE_OFFSET_BITS=64

ifeq ($(DTVKIT_INCLUDE_HBBTV),1)
SRCS += stbhwmp.c
endif

ifeq ($(DTVKIT_INCLUDE_TEST_KEYS),1)
DEFINES += INCLUDE_TEST_KEYS
endif

COMP=hw

include ../common.mak

clean: common_clean

module_clean:

