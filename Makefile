
export MODULE_NAME := dtvkit_platform

export MODULE_ROOT:=$(shell pwd)

ALL_PREREQUISITES = os hw

ALL_OBJS = $(BIN_BUILD_PATH)/os/*.o $(BIN_BUILD_PATH)/hw/*.o

MODULE_LIB = $(BIN_BUILD_PATH)/lib$(MODULE_NAME).a

# default target must be before inclusion common.mak to avoid using its default rule
all : linkall

include common.mak

linkall : $(MODULE_LIB)
$(MODULE_LIB): $(ALL_PREREQUISITES:%=$(BIN_BUILD_PATH)/lib%.a)
	$(QUIET)$(DTVKIT_AR) rsc $@ $(ALL_OBJS)

$(BIN_BUILD_PATH)/libos.a:
	$(QUIET)$(MAKE) -C os

$(BIN_BUILD_PATH)/libhw.a:
	$(QUIET)$(MAKE) -C hw

clean: common_clean
	@rm -rf $(BIN_BUILD_PATH)/lib$(MODULE_NAME).a
	# Remove the folder only if it's not empty
	-@rmdir $(DTVKIT_OUTPUT_DIR)

module_clean: $(ADDITIONAL_CLEAN)
	@$(MAKE) -C os clean
	@$(MAKE) -C hw clean

