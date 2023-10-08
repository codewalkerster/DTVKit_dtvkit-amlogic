#**************************************************************************
#  Makefile for platform hardware layer
#**************************************************************************
include $(CURDIR)/../../android-rpcservice/FuncCfg.mk

include ../Config.mk

ifdef DTVKIT_INCLUDE_TEMI
    ifeq ($(DTVKIT_INCLUDE_TEMI),1)
        DEFINES += TEMI_TIMELINES
    endif
endif

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
    stbswcfg.c \
    stbhwresm.c \
    linuxdvbdmx_wrapper.c \
    stbhwav_amlmp.c \
    stbpvrpr_amlmp.c \
    ca_glue_amlmp.c \
    stbhwci.c \
    stbhwdemux_usb.c \
    stbhwtun_ex.c \
    fsm_base.c

ifneq ($(PRODUCT_SUPPORT_EMUTUNNER), false)
SRCS +=  emu_tuner.c \
		emu_dmx.c \
		emu_config.c

LOCAL_CFLAGS += -DEMUTUNNER_ENABLE
endif

#SRCS = $(notdir $(wildcard ./src/*.c))
$(warning "SRCS",  $(SRCS))



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

