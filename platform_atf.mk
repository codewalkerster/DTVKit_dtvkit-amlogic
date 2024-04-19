LOCAL_HEADER_LIBRARIES := jni_headers

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -gt 33 && echo OK),OK)
    LOCAL_C_INCLUDES += \
    vendor/amlogic/common/ASPlayer/libs/JNI-ASPlayer-library/src/main/jni/include \
    vendor/amlogic/common/prebuilt/libmediadrm/jcas/include \
    vendor/amlogic/reference/apps/JDvrLib/jni/include
endif

LOCAL_C_INCLUDES += \
$(LOCAL_PATH)/hw/src \
$(LOCAL_PATH)/tunerframework/wrapper/inc \
vendor/amlogic/common/libdsm \

LOCAL_SRC_FILES += hw/src/afc/stbhwtun_afc.c \
hw/src/afc/stbhwdmx_afc.c \
hw/src/afc/stbhwav_asplayer.c \
hw/src/afc/stbpvrpr_jdvrlib.cpp \
hw/src/afc/ca_glue.c

LOCAL_SHARED_LIBRARIES+=libdtvkit_tuner_jni
LOCAL_SHARED_LIBRARIES+=libdtvkit_tuner_jni_wrapper

LOCAL_CFLAGS += -DUSE_AFD_DEVICE

LOCAL_MODULE := libdtvkit_platform_ATF
