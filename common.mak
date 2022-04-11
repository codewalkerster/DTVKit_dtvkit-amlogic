# Makefiles including common.mak must ensure the following variables have the appropriate value
# - APP_ROOT: absolute path of the DVBCore component
# Makefiles including common.mak can set the following variables
# - MODULE_PREREQUISITES: prerequisites needed by the calling Makefile (which has to define the relative rules)
# - COMP: component name used to create bin/$(COMP)/ for object files and bin/. Missing when included from the top level Makefile

ifeq ($(DTVKIT_CC),)
$(warning DTVKIT_CC not set, using $(CC))
DTVKIT_CC=$(CC)
endif

ifeq ($(DTVKIT_AR),)
$(warning DTVKIT_AR not set, using $(AR))
DTVKIT_AR=$(AR)
endif



DTVKIT_OPTIMISATION_OPTION?=-O2

# CFLAGS is reset here
ifeq ($(DTVKIT_BUILD_MODE),release)
CFLAGS = $(DTVKIT_OPTIMISATION_OPTION)
else
CFLAGS = -g
endif
CFLAGS += -fPIC
CFLAGS += -DSUPPORT_CAS
CFLAGS += $(DTVKIT_ADDITIONAL_COMPILER_OPTIONS)

#ifeq ($(DTVKIT_CC),/opt/gcc-linaro-6.3.1-2017.02-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-gcc)
DTVKIT_ROOT=${MODULE_ROOT}/../
DTVKIT_DVBCORE_ROOT=${DTVKIT_ROOT}/DVBCore
DTVKIT_CIPLUS_ROOT=${DTVKIT_ROOT}/CI-Plus
DTVKIT_MHEG5_ROOT=${DTVKIT_ROOT}/MHEG5
CFLAGS += -D_FILE_OFFSET_BITS=64
CFLAGS += -DINCLUDE_TEST_KEYS
CFLAGS += -DCONFIG_AMLOGIC_DVB_COMPAT
CFLAGS += -DUSE_TSPLAYER
CFLAGS += -DRDK_COMPILE

CFLAGS += -I../../rdklib/liblog/include
CFLAGS += -I../../rdklib/include
CFLAGS += -I../../rdklib/libdvr_release/include/libdvr
CFLAGS += -I../../rdklib/expat/include
CFLAGS += -I../../rdklib/aml_mp_sdk/include
CFLAGS += -I../../rdklib/aml-cas-hal/include/libamcas
CFLAGS += -I../../rdklib/mediahal_sdk/include
#endif

DEFINES += $(RDK_DEFINES)
INCLUDES += $(RDK_INCLUDES)

ifeq ($(DTVKIT_DVBCORE_ROOT),)
$(error Please set DTVKIT_DVBCORE_ROOT to point to the DVBCore source tree)
endif


#QUIET ?= @

DFLAG = -MMD

ifneq ($(DTVKIT_USE_STDINT),1)
DEFINES += NO_STDINT_H
endif
ifeq ($(DTVKIT_COLOUR_DEPTH),8)
DEFINES += OSD_8_BIT
else
ifeq ($(DTVKIT_COLOUR_DEPTH),16)
DEFINES += OSD_16_BIT
else
DTVKIT_COLOUR_DEPTH=32
DEFINES += OSD_32_BIT
ifeq ($(DTVKIT_OSD_ST_MODE), 1)
DTVKIT_COLOUR_DEPTH=31
DEFINES += OSD_ST_MODE
endif
endif
endif
DEFINES += COLOUR_DEPTH=$(DTVKIT_COLOUR_DEPTH)

DTVKIT_OUTPUT_DIR?=$(MODULE_ROOT)/build

BIN_BUILD_PATH=$(DTVKIT_OUTPUT_DIR)/bin
SRC_BUILD_PATH=$(DTVKIT_OUTPUT_DIR)/src
INC_BUILD_PATH=$(DTVKIT_OUTPUT_DIR)/inc

# Include path common for all the modules -
# and ensure that it is always first
INCLUDES := $(DTVKIT_APP_ROOT)/inc $(INCLUDES)

CFLAGS += $(patsubst %,-W%,$(WARNINGS))
CFLAGS += $(patsubst %,-D%,$(DEFINES))
CFLAGS += $(patsubst %,-I%,$(INCLUDES))

ifneq ($(COMP),)
# directory to put the *.o *.d *.P files for this component
OBJDIR = $(BIN_BUILD_PATH)/$(COMP)
# Library name for DVBCore component.
TRGT_LIB = $(BIN_BUILD_PATH)/lib$(COMP).a
# What to do when cleaning this module (do nothing if COMP is not defined)
COMMON_CLEAN_ACTION=rm -rf $(BIN_BUILD_PATH)/$(COMP) $(TRGT_LIB)
endif

SRCDIR = src
DEPF = $(OBJDIR)/$(*F)
CSRCS = $(filter %.c,$(SRCS))
DSRCS = $(filter-out %.c,$(SRCS))
OBJS  = ${CSRCS:%.c=$(OBJDIR)/%.o}
OBJS += ${DSRCS:%=$(OBJDIR)/%.o}

# default version numbers
DTVKIT_UI_MAJOR_VERSION ?= 15
DTVKIT_UI_MINOR_VERSION ?= 3
DTVKIT_UI_RELEASE_VERSION ?= 0

export TOOLS_PATH = $(DTVKIT_APP_ROOT)/tools

# define the commands for compiling ...
define compile
	@echo Compiling $(FULL_SRC_PATH)$<
	$(QUIET)$(DTVKIT_CC) $(DFLAG) $(CFLAGS) -o $@ -c $(FULL_SRC_PATH)$<
	@cat $(DEPF).d | sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' -e '/^$$/ d' > $(DEPF).t
	@cat $(DEPF).t | sed -e 's/$$/ :/' >> $(DEPF).d;
	@rm $(DEPF).t
endef

#
# Default target
#
default : $(MODULE_PREREQUISITES) $(OBJDIR) $(TRGT_LIB)

#
# Rule to create target library
#
$(TRGT_LIB): $(OBJS)
	@echo Building $@
	$(QUIET)$(DTVKIT_AR) rsc $@ $(filter %.o,$^)
	@echo "PREVCSRCS=$(CSRCS:%.c=%)" > $(OBJDIR)/srcs.mk

#
# The compile rule
#
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(compile)

#
# Rule to make required directories
#
$(OBJDIR) $(BIN_BUILD_PATH) $(INC_BUILD_PATH) $(SRC_BUILD_PATH):
	@echo Creating $@
	@mkdir -p $@

common_clean: module_clean
	$(COMMON_CLEAN_ACTION)
	-rmdir $(SRC_BUILD_PATH) $(INC_BUILD_PATH) $(BIN_BUILD_PATH) $(OBJDIR)

-include $(OBJS:%.o=%.d)
