LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libdtvkit_tuner_jni_wrapper

LOCAL_CFLAGS = $(L_CFLAGS)

LOCAL_SRC_FILES := src/wrapper_dmx.cpp \
                   src/wrapper_frontend.cpp \
                   src/wrapper_pvr.cpp \
                   src/wrapper_player.cpp \
                   src/wrapper_os.cpp

LOCAL_C_INCLUDES += frameworks/base/core/jni/include \
                                           $(LOCAL_PATH)/inc
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../hw/inc/

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libutils \
    liblog \
    libbase

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -gt 33 && echo OK),OK)
$(warning "build jni => $(PRODUCT_SUPPORT_TUNER_FRAMEWORK)")
    LOCAL_C_INCLUDES += vendor/amlogic/common/ASPlayer/libs/JNI-ASPlayer-library/src/main/jni/include
    LOCAL_C_INCLUDES += vendor/amlogic/common/prebuilt/libmediadrm/jcas/include
    LOCAL_C_INCLUDES += vendor/amlogic/reference/apps/JDvrLib/jni/include
    LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../../android-inputsource/logicdtvkit/src/logictuner/tuner_jni/include

    LOCAL_SHARED_LIBRARIES += libjdvrlib-jni
    LOCAL_SHARED_LIBRARIES += libdtvkit_tuner_jni
    LOCAL_SHARED_LIBRARIES += libjniasplayer-jni
    LOCAL_SHARED_LIBRARIES += libjcas_jni
endif

LOCAL_HEADER_LIBRARIES := jni_headers
LOCAL_MULTILIB := 32
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS := optional
LOCAL_VENDOR_MODULE := true
LOCAL_LICENSE_KINDS := legacy_notice
LOCAL_LICENSE_CONDITIONS := notice

include $(BUILD_SHARED_LIBRARY)
