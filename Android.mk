LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

include $(LOCAL_PATH)/Config.mk

ifeq ($(DTVKIT_INCLUDE_TEMI),1)
LOCAL_CFLAGS += -DTEMI_TIMELINES
endif

#LOCAL_SANITIZE := address
ifeq ($(DTVKIT_AMLOGIC_SANITIZE), true)
    LOCAL_SANITIZE := address
    $(warning "DTVKIT_AMLOGIC_SANITIZE Opened")
else
    $(warning "DTVKIT_AMLOGIC_SANITIZE Closed")
endif


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
ifeq ($(DTVKIT_CI_PHYS_TYPE), usb)
    LOCAL_LDFLAGS := $(LOCAL_PATH)/../releaseDTVKit/libsmit_usbcam.a
endif

SUPPORT_CAS := true
ifeq ($(SUPPORT_CAS), true)
    LOCAL_CFLAGS += -DSUPPORT_CAS
endif

DTVKIT_WITH_TSPLAYER ?= 0
ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 28&& echo OK),OK)
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

MEDIAHAL_INCLUDE:=vendor/amlogic/common/mediahal_sdk/include

ifneq (,$(wildcard media_hal))
  MEDIAHAL_INCLUDE:=media_hal/AmTsplayer/include
endif

#LOCAL_CFLAGS += -DCONFIG_AMLOGIC_DVB_COMPAT
LOCAL_CFLAGS += -DANDROID_PLATFORM_SDK_VERSION=$(PLATFORM_SDK_VERSION)

ifeq ($(DTVKIT_WITH_TSPLAYER), 1)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 30&& echo OK),OK)
    ifeq ($(TARGET_BUILD_KERNEL_4_9), true)
        $(info "Build dtvkit-amlogic for AndroidR kernel 4.9")
        #LOCAL_C_INCLUDES := vendor/amlogic/common/kernel/common/include/uapi/linux/dvb/
    else
        $(info "Build dtvkit-amlogic for AndroidR kernel > 4.9")
        #LOCAL_C_INCLUDES := common/include/uapi/linux/dvb/
    endif
else
    $(info "Build dtvkit-amlogic for AndroidP/Q ")
    #LOCAL_C_INCLUDES := common/include/uapi/linux/dvb/
endif

    LOCAL_C_INCLUDES += \
        $(MEDIAHAL_INCLUDE)

    LOCAL_CFLAGS += -DUSE_TSPLAYER
endif

ifeq ($(SUPPORT_CAS), true)
    LOCAL_C_INCLUDES += $(LOCAL_PATH)/../cas_hal/libamcas/include \
    $(LOCAL_PATH)/../DVBCore/midware/CA/inc \
    $(LOCAL_PATH)/../DVBCore/midware/stb/inc
endif

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../DVBCore/inc \
    $(LOCAL_PATH)/../DVBCore/platform/inc \
    $(LOCAL_PATH)/../CI-Plus/include \
    $(LOCAL_PATH)/../MHEG5/include \
    $(LOCAL_PATH)/../android-rpcservice/modules/binderservice/inc \
    $(LOCAL_PATH)/../DVBCore/CERT/inc \
    $(LOCAL_PATH)/../DVBCore/dvb/inc \
    $(LOCAL_PATH)/hw/inc \
    $(LOCAL_PATH)/os/inc \
    external/sqlite/dist \
    $(LOCAL_PATH)/../../../frameworks/services/systemcontrol \
    $(LOCAL_PATH)/../../../frameworks/services/systemcontrol/PQ/include \
    system/core/libutils/include \
    bionic/libc/kernel/uapi \
    bionic/libc/kernel/android/uapi \
    bionic/libc/stdio \
    bionic/libc/include \
    bionic/libc/../libm/include \

LOCAL_CFLAGS += \
    -Wno-unused-function \
    -Wno-unused-parameter \
    -Wno-unused-variable \
    -Wno-pointer-sign \
    -Werror=implicit-function-declaration \
    -Wno-typedef-redefinition \
    -Wno-unknown-attributes \
    -Werror=int-to-pointer-cast \
    -Werror=pointer-to-int-cast \
    -Werror=incompatible-pointer-types \
    -Werror


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

ifeq ($(DTVKIT_CI_PHYS_TYPE), usb)
    LOCAL_LDFLAGS := $(LOCAL_PATH)/../releaseDTVKit/libsmit_usbcam.a
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
    hw/src/stbhwresm.c \
    hw/src/stbhwdemux_usb.c \
    hw/src/linuxdvbdmx_wrapper.c \
    hw/src/systemcontrol.cpp \
    hw/src/stbhwtun_ex.c \
    hw/src/fsm_base.c \
    os/src/stbos_timer.c \
    os/src/stbos_event.c      \
    os/src/stbos_mutex.c      \
    os/src/stbos_queue.c      \
    os/src/stbos_rtc.c        \
    os/src/stbos_semaphore.c  \
    os/src/stbos_task.c       \
    os/src/stbos_utils.c      \
    os/src/dtv_log.c

ifneq ($(PRODUCT_SUPPORT_EMUTUNNER), false)
LOCAL_SRC_FILES += hw/src/emu_tuner.c \
                   hw/src/emu_dmx.c \
                   hw/src/emu_config.c

LOCAL_CFLAGS += -DEMUTUNNER_ENABLE
endif

ifeq ($(SUPPORT_CAS), true)
    LOCAL_CFLAGS += -DSUPPORT_CAS
    LOCAL_SRC_FILES += hw/src/ca_glue_amlmp.c
    LOCAL_C_INCLUDES += $(TOP)/$(LOCAL_PATH)/../../../aml_mp_sdk/include
    #LOCAL_C_INCLUDES += $(TOP)/vendor/amlogic/common/aml_mp_sdk/include
    LOCAL_CFLAGS += -DDTVKIT_WITH_AML_MP_SDK
endif

ifeq ($(DTVKIT_WITH_TSPLAYER), 1)
    ifneq ($(DTVKIT_WITH_AML_MP_SDK), true)
        LOCAL_SRC_FILES += hw/src/stbhwav_tsplayer.c
        LOCAL_SRC_FILES += hw/src/stbpvrpr_tsplayer.c
    else
        LOCAL_SRC_FILES += hw/src/stbhwav_amlmp.c
        LOCAL_SRC_FILES += hw/src/stbpvrpr_amlmp.c
        LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../../aml_mp_sdk/include
        #LOCAL_C_INCLUDES += vendor/amlogic/common/aml_mp_sdk/include
        LOCAL_CFLAGS += -DDTVKIT_WITH_AML_MP_SDK
    endif
else
    LOCAL_SRC_FILES += hw/src/stbhwav.c
    LOCAL_SRC_FILES += hw/src/stbpvrpr.c
endif

LOCAL_CFLAGS+=-DANDROID $(DTVKIT_OPTIMISATION_OPTION)
LOCAL_PRELINK_MODULE := false
LOCAL_ARM_MODE := arm
SUPPORT_DTVKIT_IN_VENDOR := true
LOCAL_STATIC_LIBRARIES+=libexpat libcutils
LOCAL_SHARED_LIBRARIES+=libmediahal_resman
ifeq ($(PRODUCT_SUPPORT_SWDEMUX),true)
    LOCAL_SHARED_LIBRARIES+=liblog libswdemux
else
    LOCAL_SHARED_LIBRARIES+=liblog
endif

ifeq ($(SUPPORT_DTVKIT_IN_VENDOR), true)
    LOCAL_VENDOR_MODULE := true
    LOCAL_CFLAGS += -DDTVKIT_IN_VENDOR_PARTITION
endif

LOCAL_SHARED_LIBRARIES+=libsystemcontrolservice
LOCAL_SHARED_LIBRARIES+=vendor.amlogic.hardware.systemcontrol@1.0
LOCAL_SHARED_LIBRARIES+=vendor.amlogic.hardware.systemcontrol@1.1

include $(BUILD_STATIC_LIBRARY)
