LOCAL_PATH:= $(call my-dir)

SUPPORT_PLATFORM_STATIC_LIB = true
SUPPORT_PLATFORM_SHARED_LIB = true



ifeq ($(SUPPORT_PLATFORM_STATIC_LIB), true)
    $(warning "build noatf platform static lib")

	include $(LOCAL_PATH)/platform_base.mk
	include $(LOCAL_PATH)/platform_noatf.mk

    include $(BUILD_STATIC_LIBRARY)
endif

ifeq ($(SUPPORT_PLATFORM_SHARED_LIB), true)
    $(warning "build noatf platform share lib")

	include $(LOCAL_PATH)/platform_base.mk
	include $(LOCAL_PATH)/platform_noatf.mk

	LOCAL_SHARED_LIBRARIES += \
            libutils

	LOCAL_SHARED_LIBRARIES += \
			libaml_mp_sdk.vendor \
			libmediahal_resman

	include $(BUILD_SHARED_LIBRARY)
endif

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -gt 33 && echo OK),OK)
    ifeq ($(SUPPORT_PLATFORM_STATIC_LIB), true)
        $(warning "build atf platform static lib")

	    include $(LOCAL_PATH)/platform_base.mk
	    include $(LOCAL_PATH)/platform_atf.mk

        include $(BUILD_STATIC_LIBRARY)
    endif

	ifeq ($(SUPPORT_PLATFORM_SHARED_LIB), true)
        $(warning "build noatf platform share lib")

	    include $(LOCAL_PATH)/platform_base.mk
	    include $(LOCAL_PATH)/platform_atf.mk

	    LOCAL_SHARED_LIBRARIES += \
            libutils

	    LOCAL_SHARED_LIBRARIES += \
			libdsm

	    include $(BUILD_SHARED_LIBRARY)
    endif

    include $(LOCAL_PATH)/tunerframework/wrapper/Android.mk
endif
