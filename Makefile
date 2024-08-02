
export MODULE_NAME := dtvkit_platform

export MODULE_ROOT:=$(shell pwd)

ifeq ($(HAS_CCDATABASE),1)
  export ENABLE_XDS = 1
endif

ALL_PREREQUISITES = os hw

ALL_OBJS = $(BIN_BUILD_PATH)/os/*.o $(BIN_BUILD_PATH)/hw/*.o

MODULE_LIB = $(BIN_BUILD_PATH)/lib$(MODULE_NAME).a
ifeq ($(ENABLE_XDS),1)
  XDS_LIB = $(BIN_BUILD_PATH)/libxds.a
endif

DTVKIT_INSTALL_DIR ?= $(MODULE_ROOT)/install

INSTALL_LIB_PATH=$(DTVKIT_INSTALL_DIR)/lib
INSTALL_INC_PATH=$(DTVKIT_INSTALL_DIR)/include/$(MODULE_NAME)
INSTALL_DOC_PATH=$(DTVKIT_INSTALL_DIR)/doc/$(MODULE_NAME)
INSTALL_AUX_PATH=$(DTVKIT_INSTALL_DIR)/share/$(MODULE_NAME)

# default target must be before inclusion common.mak to avoid using its default rule
all : linkall

include common.mak

linkall : $(MODULE_LIB) $(XDS_LIB)
$(MODULE_LIB): $(ALL_PREREQUISITES:%=$(BIN_BUILD_PATH)/lib%.a)
	$(QUIET)$(DTVKIT_AR) rsc $@ $(ALL_OBJS)

$(BIN_BUILD_PATH)/libos.a:
	$(QUIET)$(MAKE) -C os

$(BIN_BUILD_PATH)/libhw.a:
	$(QUIET)$(MAKE) -C hw

$(BIN_BUILD_PATH)/libxds.a:
	$(QUIET)$(MAKE) -C hw/xds/dtvkit

clean: common_clean
	@rm -rf $(BIN_BUILD_PATH)/lib$(MODULE_NAME).a
ifeq ($(ENABLE_XDS),1)
	@rm -rf $(BIN_BUILD_PATH)/libxds.a
endif
	# Remove the folder only if it's not empty
	-@rmdir $(DTVKIT_OUTPUT_DIR)

module_clean: $(ADDITIONAL_CLEAN)
	@$(MAKE) -C os clean
	@$(MAKE) -C hw clean
ifeq ($(ENABLE_XDS),1)
	@$(MAKE) -C hw/xds/dtvkit clean
endif

define install
	install -C -d $(1) && \
	install -C -m u=rw,go=r,a-s -t $(1) $(2)
endef

define rmdir_if_empty
	if [ -d $(1) ]; then \
		find $(1) -depth -type d -empty -delete; \
	fi
endef

install: linkall
	@echo Installing $(MODULE_NAME)
	@echo Installing $(INSTALL_LIB_PATH)
	@echo Installing $(MODULE_LIB)
	@$(call install,$(INSTALL_LIB_PATH),$(MODULE_LIB))
	@$(call install,$(INSTALL_INC_PATH)/hw/inc,hw/inc/*.h)
ifeq ($(ENABLE_XDS),1)
	@echo Installing $(XDS_LIB)
	@$(call install,$(INSTALL_LIB_PATH),$(XDS_LIB))
endif

uninstall:
	@echo Uninstalling $(MODULE_NAME)
	@rm -f $(INSTALL_LIB_PATH)/$(notdir $(MODULE_LIB))
	@rm -rf $(INSTALL_INC_PATH)
ifeq ($(ENABLE_XDS),1)
	@echo Uninstalling $(XDS_NAME)
	@rm -f $(INSTALL_LIB_PATH)/$(notdir $(XDS_LIB))
endif
	@$(call rmdir_if_empty,$(DTVKIT_INSTALL_DIR))
