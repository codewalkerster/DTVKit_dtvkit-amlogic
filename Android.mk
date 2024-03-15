LOCAL_PATH:= $(call my-dir)



SUPPORT_PLATFORM_STATIC_LIB = true
SUPPORT_PLATFORM_SHARED_LIB = true

ifeq ($(SUPPORT_PLATFORM_STATIC_LIB), true)
	include $(LOCAL_PATH)/platform.mk
    include $(BUILD_STATIC_LIBRARY)
endif

ifeq ($(SUPPORT_PLATFORM_SHARED_LIB), true)
	include $(LOCAL_PATH)/platform.mk

	LOCAL_SHARED_LIBRARIES += \
            libutils

	LOCAL_SHARED_LIBRARIES += \
			libaml_mp_sdk.vendor \
			libmediahal_resman

	include $(BUILD_SHARED_LIBRARY)
endif

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 29 && echo OK),OK)
    include $(LOCAL_PATH)/atf.mk
    include $(LOCAL_PATH)/tunerframework/wrapper/Android.mk
endif
