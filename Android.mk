LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

include $(LOCAL_PATH)/Config.mk

ifeq ($(TARGET_ARCH),"arm")
    DTVKIT_USE_STDINT = 0
else
    DTVKIT_USE_STDINT = 1
endif

DTVKIT_OPTIMISATION_OPTION?=-O2
ifeq ($(DTVKIT_BUILD_MODE),release)
LOCAL_CFLAGS += $(DTVKIT_OPTIMISATION_OPTION)
else
LOCAL_CFLAGS += -g
endif

DTVKIT_WITH_CAS ?= 0
ifeq ($(DTVKIT_WITH_CAS), 1)
    LOCAL_CFLAGS += -DSUPPORT_CAS
endif

DTVKIT_WITH_TSPLAYER ?= 0
ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 29&& echo OK),OK)
    DTVKIT_WITH_TSPLAYER = 1
endif

LOCAL_MODULE := libdtvkit_platform
LOCAL_MODULE_TAGS := optional

ifneq ($(DTVKIT_USE_STDINT),1)
    LOCAL_CFLAGS += -DNO_STDINT_H
endif
ifeq ($(DTVKIT_COLOUR_DEPTH),8)
    LOCAL_CFLAGS += -DOSD_8_BIT
else
    ifeq ($(DTVKIT_COLOUR_DEPTH),16)
        LOCAL_CFLAGS += -DOSD_16_BIT
    else
        DTVKIT_COLOUR_DEPTH=32
        LOCAL_CFLAGS += -DOSD_32_BIT
        ifeq ($(DTVKIT_OSD_ST_MODE), 1)
            DTVKIT_COLOUR_DEPTH=31
            LOCAL_CFLAGS += -DOSD_ST_MODE
        endif
    endif
endif
LOCAL_CFLAGS += -DCOLOUR_DEPTH=$(DTVKIT_COLOUR_DEPTH)

LOCAL_CFLAGS += -D_FILE_OFFSET_BITS=64

ifeq ($(DTVKIT_INCLUDE_TEST_KEYS),1)
LOCAL_CFLAGS += -DINCLUDE_TEST_KEYS
endif

ifeq ($(PRODUCT_SUPPORT_SWDEMUX),true)
LOCAL_CFLAGS += -DMEDIACODEC_PLAYER
endif


ifeq (,$(wildcard $(LOCAL_PATH)/../../dvb))
    DVB_PATH := vendor/amlogic/common/prebuilt/dvb
else
    DVB_PATH := vendor/amlogic/common/external/dvb
endif

ifeq (,$(wildcard $(LOCAL_PATH)/../../dvb))
LOCAL_C_INCLUDES := \
    $(DVB_PATH)/ndk/include \
	$(DVB_PATH)/ndk/include/linux
else
LOCAL_C_INCLUDES := \
    $(DVB_PATH)/android/ndk/include/linux \
    $(DVB_PATH)/android/ndk/include
endif

MEDIAHAL_INCLUDE:=vendor/amlogic/common/mediahal_sdk/include
ifneq (,$(wildcard media_hal))
  MEDIAHAL_INCLUDE:=media_hal/AmTsplayer/include
endif

ifeq ($(DTVKIT_WITH_TSPLAYER), 1)
    LOCAL_C_INCLUDES += $(MEDIAHAL_INCLUDE)
    LOCAL_C_INCLUDES += vendor/amlogic/common/libdvr/include
endif

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../DVBCore/inc \
    $(LOCAL_PATH)/../DVBCore/platform/inc \
    $(LOCAL_PATH)/../CI-Plus/include \
    $(LOCAL_PATH)/../MHEG5/include \
    $(LOCAL_PATH)/../android-rpcservice/modules/binderservice/inc \
    $(LOCAL_PATH)/hw/inc \
    $(LOCAL_PATH)/os/inc \
    $(DVB_PATH)/include \
    $(DVB_PATH)/include/am_adp \
    $(DVB_PATH)/include/am_mw \
    external/sqlite/dist \
    bionic/libc/kernel/uapi \
    bionic/libc/kernel/android/uapi \
    bionic/libc/stdio \
    bionic/libc/include \
    bionic/libc/../libm/include \


ifeq ($(PRODUCT_SUPPORT_SWDEMUX),true)
    SWDMX_PATH := vendor/amlogic/common/external/libswdemux
    LOCAL_C_INCLUDES += $(SWDMX_PATH)/
endif


ifeq ($(TARGET_ARCH),"arm")
    ANDROID_HEADERS+=" -I${BIONIC_LIB}/arch-arm/include"
    ANDROID_HEADERS+=" -I${BIONIC_LIB}/kernel/uapi/asm-arm"
else
    ANDROID_HEADERS+=" -I${BIONIC_LIB}/arch-arm64/include"
    ANDROID_HEADERS+=" -I${BIONIC_LIB}/kernel/uapi/asm-arm64"
endif

LOCAL_SRC_FILES := hw/src/stbhwplatform.c \
    hw/src/stbhwini.c \
    hw/src/stbhwmem.c \
    hw/src/stbhwtun.c \
    hw/src/stbhwdmx.c \
    hw/src/stbhwdsk.c \
    hw/src/stbhwfp.c  \
    hw/src/stbhwsp.c  \
    hw/src/stbhwcrypt.c \
    hw/src/stbhwupg.c \
    hw/src/stbhwci.c \
    hw/src/stbhwosd.c \
    hw/src/stbhwnet.c \
    hw/src/stbhwvbi.c \
    hw/src/stbhwcfg.c \
    os/src/stbos_event.c      \
    os/src/stbos_mutex.c      \
    os/src/stbos_queue.c      \
    os/src/stbos_rtc.c        \
    os/src/stbos_semaphore.c  \
    os/src/stbos_task.c

ifeq ($(DTVKIT_WITH_TSPLAYER), 1)
    LOCAL_SRC_FILES += hw/src/stbhwav_tsplayer.c
    LOCAL_SRC_FILES += hw/src/stbpvrpr_tsplayer.c
else
    LOCAL_SRC_FILES += hw/src/stbhwav.c
    LOCAL_SRC_FILES += hw/src/stbpvrpr.c
endif

LOCAL_CFLAGS+=-DANDROID $(DTVKIT_OPTIMISATION_OPTION)
LOCAL_PRELINK_MODULE := false
LOCAL_ARM_MODE := arm
LOCAL_STATIC_LIBRARIES+=libexpat libcutils
ifeq ($(PRODUCT_SUPPORT_SWDEMUX),true)
    LOCAL_SHARED_LIBRARIES+=liblog libswdemux
else
    LOCAL_SHARED_LIBRARIES+=liblog
endif

LOCAL_CFLAGS += -DSUPPORT_DTVKIT_IN_VENDOR


include $(BUILD_STATIC_LIBRARY)
