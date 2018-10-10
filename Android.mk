LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

$(info $(shell (. $(LOCAL_PATH)/android_setenv.sh; make -C $(LOCAL_PATH))))

LOCAL_MODULE := libdtvkit_platform
LOCAL_SRC_FILES := build/bin/libdtvkit_platform.a
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_SUFFIX := .a

include $(BUILD_PREBUILT)

