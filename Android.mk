LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

ifeq ($(TARGET_ARCH),"arm")
    DTVKIT_USE_STDINT = 0
else
    DTVKIT_USE_STDINT = 1
endif

DTVKIT_OPTIMISATION_OPTION?=-O2

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

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../DVBCore/inc \
    $(LOCAL_PATH)/../DVBCore/platform/inc \
    $(LOCAL_PATH)/../CI-Plus/include \
    $(LOCAL_PATH)/hw/inc \
    $(LOCAL_PATH)os/inc \
    vendor/amlogic/common/external/dvb/android/ndk/include \
    vendor/amlogic/common/external/dvb/include \
    vendor/amlogic/common/external/dvb/android/ndk/include \
    bionic/libc/kernel/uapi \
    bionic/libc/kernel/android/uapi \
    bionic/libc/stdio \
    bionic/libc/include \
    bionic/libc/../libm/include \
    hardware/amlogic/media/amcodec/include\

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
    hw/src/stbhwav.c  \
    hw/src/stbhwdsk.c \
    hw/src/stbhwfp.c  \
    hw/src/stbhwsp.c  \
    hw/src/stbhwcrypt.c \
    hw/src/stbhwupg.c \
    hw/src/stbhwci.c \
    hw/src/stbhwosd.c \
    hw/src/stbpvrpr.c \
    hw/src/stbhwnet.c \
    hw/src/stbhwvbi.c \
    os/src/stbos_event.c      \
    os/src/stbos_mutex.c      \
    os/src/stbos_queue.c      \
    os/src/stbos_rtc.c        \
    os/src/stbos_semaphore.c  \
    os/src/stbos_task.c

LOCAL_CFLAGS+=-DANDROID $(DTVKIT_OPTIMISATION_OPTION)
LOCAL_PRELINK_MODULE := false
LOCAL_ARM_MODE := arm
LOCAL_STATIC_LIBRARIES+=libcutils

LOCAL_VENDOR_MODULE := true

include $(BUILD_STATIC_LIBRARY)
