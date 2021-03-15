# Option to include platform source required for DTVKit HbbTV v1.5
DTVKIT_INCLUDE_HBBTV:=0

# To include hard coded CI Plus test keys and certificates, enable
#DTVKIT_INCLUDE_TEST_KEYS:=1

DTVKIT_WITH_AML_MP_SDK := false
ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 29 && echo OK),OK)
DTVKIT_WITH_AML_MP_SDK := true
endif
