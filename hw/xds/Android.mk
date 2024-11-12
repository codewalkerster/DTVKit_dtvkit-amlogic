#PRODUCT_SUPPORT_CCDATABASE?=true

ifeq ($(PRODUCT_SUPPORT_CCDATABASE), true)
    ENABLE_XDS := true
endif

ifeq ($(ENABLE_XDS), true)

LOCAL_PATH := $(call my-dir)

XDS_COMM_LIBS := \
    libcutils \
    libutils \
    liblog

include $(CLEAR_VARS)
LOCAL_MODULE := libxds_static
LOCAL_SRC_FILES := \
    src/xds.c \
    src/xds_608.c \
    src/xds_708.c
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/src
LOCAL_SHARED_LIBRARIES := libccdataserver_client
LOCAL_SHARED_LIBRARIES += $(XDS_COMM_LIBS)
LOCAL_MULTILIB := 32
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS := optional
LOCAL_VENDOR_MODULE := true
LOCAL_LICENSE_KINDS := legacy_notice
LOCAL_LICENSE_CONDITIONS := notice
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libxds
LOCAL_WHOLE_STATIC_LIBRARIES := libxds_static
LOCAL_SHARED_LIBRARIES := libccdataserver_client
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/src
LOCAL_SHARED_LIBRARIES += $(XDS_COMM_LIBS)
LOCAL_MULTILIB := 32
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS := optional
LOCAL_VENDOR_MODULE := true
LOCAL_LICENSE_KINDS := legacy_notice
LOCAL_LICENSE_CONDITIONS := notice
include $(BUILD_SHARED_LIBRARY)

include $(LOCAL_PATH)/test/Android.mk

endif
