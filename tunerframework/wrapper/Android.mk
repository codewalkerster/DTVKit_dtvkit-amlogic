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

ifeq ($(PRODUCT_SUPPORT_TUNER_FRAMEWORK), true)
$(warning "build jni => $(PRODUCT_SUPPORT_TUNER_FRAMEWORK)")
    LOCAL_C_INCLUDES += vendor/amlogic/common/ASPlayer/libs/JNI-ASPlayer-library/src/main/jni/include
    LOCAL_C_INCLUDES += vendor/amlogic/reference/apps/JDvrLib/jni/include
    LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../../android-inputsource/logicdtvkit/src/logictuner/tuner_jni/include

    LOCAL_SHARED_LIBRARIES += libjdvrlib-jni
    LOCAL_SHARED_LIBRARIES += libdtvkit_tuner_jni
    LOCAL_SHARED_LIBRARIES += libjniasplayer-jni
else
$(warning "prebuild")
    LOCAL_C_INCLUDES += $(LOCAL_PATH)/../JNI_asplayer/include
    LOCAL_C_INCLUDES += $(LOCAL_PATH)/../JNI_dvr/include
    LOCAL_C_INCLUDES += $(LOCAL_PATH)/../JNI_tuner/include

    LOCAL_LDFLAGS +=$(LOCAL_PATH)/../JNI_asplayer/lib/libjniasplayer-jni.so
    LOCAL_LDFLAGS +=$(LOCAL_PATH)/../JNI_dvr/lib/libjdvrlib-jni.so
    LOCAL_LDFLAGS +=$(LOCAL_PATH)/../JNI_tuner/lib/libdtvkit_tuner_jni.so
endif

LOCAL_HEADER_LIBRARIES := jni_headers

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS := optional
LOCAL_VENDOR_MODULE := true
LOCAL_LICENSE_KINDS := legacy_notice
LOCAL_LICENSE_CONDITIONS := notice

include $(BUILD_SHARED_LIBRARY)
