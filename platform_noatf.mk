LOCAL_C_INCLUDES += $(TOP)/$(LOCAL_PATH)/../../../aml_mp_sdk/include
LOCAL_CFLAGS += -DDTVKIT_WITH_AML_MP_SDK
ifeq ($(SUPPORT_CAS), true)
    LOCAL_CFLAGS += -DSUPPORT_CAS
    LOCAL_SRC_FILES += hw/src/ca_glue_amlmp.c
endif

LOCAL_SRC_FILES += hw/src/stbhwtun.c
LOCAL_SRC_FILES += hw/src/stbhwtun_ex.c
LOCAL_SRC_FILES += hw/src/stbhwdmx.c
LOCAL_SRC_FILES += hw/src/linuxdvbdmx_wrapper.c
LOCAL_SRC_FILES += hw/src/stbhwresm.c
LOCAL_SRC_FILES += hw/src/stbhwav_amlmp.c
LOCAL_SRC_FILES += hw/src/stbpvrpr_amlmp.c

LOCAL_MODULE := libdtvkit_platform
