# Option to include platform source required for DTVKit HbbTV v1.5
DTVKIT_INCLUDE_HBBTV:=0

# Set to 1 to include support for TEMI timeline support
DTVKIT_INCLUDE_TEMI=1

# To include hard coded CI Plus test keys and certificates, enable
DTVKIT_INCLUDE_TEST_KEYS:=1

DTVKIT_WITH_AML_MP_SDK := false
ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 28 && echo OK),OK)
DTVKIT_WITH_AML_MP_SDK := true
endif

ifneq ($(PRODUCT_SUPPORT_CIPLUS),false)
LOCAL_CFLAGS += -DCOMMON_INTERFACE
endif
