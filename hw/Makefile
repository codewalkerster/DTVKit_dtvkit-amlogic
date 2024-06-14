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

INCLUDES =  ./inc \
            ../inc

SRCDIR = src:hal:atv
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
    stbhwutils.c \
    fsm_base.c \
    stbpathcfg.c \
    afd_ctrl.c \
    aml_frontend_api.c \
    atv_vdin_tvafe.c \
    atv_vlfend.c \
    atv_vlfend_test.c \
    linux_v4l2.c \
    cJSON.c

ifneq ($(PRODUCT_SUPPORT_EMUTUNNER), false)
SRCS += emu_tuner.c \
        emu_dmx.c \
        emu_config.c

DEFINES += EMUTUNNER_ENABLE
endif

ifeq ($(PRODUCT_SUPPORT_HDI), 1)
SRCS += aml_subtitle_hdi.cpp
else
SRCS += aml_subtitle.cpp
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

