#
#    Copyright (c) 2019 Google LLC.
#    All rights reserved.
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
#
#        http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
#

#
#   @file
#         Component makefile for incorporating OpenThread into an EFR32
#         application.
#

#
#   This makefile is intended to work in conjunction with the efr32-app.mk
#   makefile to build the OpenWeave example applications on Silicon Labs platforms. 
#   EFR32 applications should include this file in their top level Makefile
#   along with the other makefiles in this directory.  E.g.:
#
#       PROJECT_ROOT = $(realpath .)
#
#       BUILD_SUPPORT_DIR = $(PROJECT_ROOT)/third_party/openweave-core/build/efr32
#       
#       include $(BUILD_SUPPORT_DIR)/efr32-app.mk
#       include $(BUILD_SUPPORT_DIR)/efr32-openweave.mk
#       include $(BUILD_SUPPORT_DIR)/efr32-openthread.mk
#       include $(BUILD_SUPPORT_DIR)/efr32-freertos.mk
#
#       PROJECT_ROOT := $(realpath .)
#       
#       APP := openweave-efr32-bringup
#       
#       SRCS = \
#           $(PROJECT_ROOT)/main.cpp \
#           ...
#
#       $(call GenerateBuildRules)
#       


# ==================================================
# General settings
# ==================================================

# OpenThread source root directory
OPENTHREAD_ROOT ?= $(PROJECT_ROOT)/third_party/openthread

# Target for which OpenThread will be built.
OPENTHREAD_TARGET = $(EFR32FAMILY)

# Archtecture for which OpenThread will be built.
OPENTHREAD_HOST_ARCH = arm-none-eabi

# Directory into which the OpenThread build system will place its output. 
OPENTHREAD_OUTPUT_DIR = $(OUTPUT_DIR)/openthread

# Directory containing OpenThread libraries.
OPENTHREAD_LIB_DIR = $(OPENTHREAD_OUTPUT_DIR)/$(OPENTHREAD_TARGET)/lib

# Prerequisite target for anything that depends on the output of the OpenThread
# build process.
OPENTHREAD_PREREQUISITE = install-thread

# Name of OpenThread's platform config file.  By default, this is set to
# the EFR32-specific file found in OpenThread's examples directory.  
# Applications can override this to force inclusion of their own configuration
# file. However, in most cases, the application-specified file should include
# the EFR32 file to ensure that OpenThread is configured properly for the
# for EFR32 platforms.
OPENTHREAD_PROJECT_CONFIG = openthread-core-efr32-config.h

# Additional header files needed by the EFR32 port of OpenThread
# but not installed automatically by OpenThread's build system.
OPENTHREAD_PLATFORM_HEADERS = \
    $(OPENTHREAD_ROOT)/examples/platforms/openthread-system.h

# ==================================================
# Compilation flags / settings specific to building
#   OpenThread itself.
# ==================================================

OPENTHREAD_CPPFLAGS = $(STD_CFLAGS) $(DEBUG_FLAGS) $(OPT_FLAGS) -Wno-expansion-to-defined \
                      $(OPENTHREAD_DEFINE_FLAGS) $(OPENTHREAD_INC_FLAGS)
                      
OPENTHREAD_CXXFLAGS = $(STD_CXXFLAGS) -Wno-expansion-to-defined -fno-exceptions -fno-exceptions -fno-rtti 
OPENTHREAD_DEFINE_FLAGS = $(foreach def,$(OPENTHREAD_DEFINES),-D$(def))
OPENTHREAD_INC_FLAGS = $(foreach dir,$(OPENTHREAD_INC_DIRS),-I$(dir))

OPENTHREAD_LDFLAGS              = \
    -specs=nosys.specs            \
    -Wl,--gc-sections

OPENTHREAD_DEFINES = \
    EFR32 \
    OPENTHREAD_PROJECT_CORE_CONFIG_FILE='\"$(OPENTHREAD_PROJECT_CONFIG)\"' \
    MBEDTLS_CONFIG_FILE='\"mbedtls-config.h\"' \
    MBEDTLS_USER_CONFIG_FILE='\"efr32-weave-mbedtls-config.h\"' \
    RADIO_CONFIG_DMP_SUPPORT=1 \
    $(MCU)

OPENTHREAD_INC_DIRS = \
    $(PROJECT_ROOT) \
    $(FREERTOS_ROOT)/Source/include \
    $(FREERTOS_ROOT)/Source/portable/GCC/$(FREERTOS_TARGET) \
    $(FREERTOSCONFIG_DIR) \
    $(OPENWEAVE_ROOT)/src/adaptations/device-layer/include/Weave/DeviceLayer/EFR32 \
    $(OPENTHREAD_ROOT)/examples/platforms \
    $(OPENTHREAD_ROOT)/examples/platforms/$(OPENTHREAD_TARGET) \
    $(OPENTHREAD_ROOT)/examples/platforms/$(OPENTHREAD_TARGET)/crypto \
    $(OPENTHREAD_ROOT)/third_party/mbedtls \
    $(OPENTHREAD_ROOT)/third_party/mbedtls/repo/include \
    $(OPENTHREAD_ROOT)/third_party/mbedtls/repo/include/mbedtls \
    $(EFR32_SDK_ROOT)/platform/CMSIS/Include \
    $(EFR32_SDK_ROOT)/platform/emlib/inc \
    $(EFR32_SDK_ROOT)/platform/radio/rail_lib/protocol/ble \
    $(EFR32_SDK_ROOT)/platform/radio/rail_lib/common \
    $(EFR32_SDK_ROOT)/util/third_party/mbedtls/configs \
    $(EFR32_SDK_ROOT)/util/third_party/mbedtls/sl_crypto/include \
    $(EFR32_SDK_ROOT)/protocol/bluetooth/ble_stack/inc/soc \
    $(EFR32_SDK_ROOT)/protocol/bluetooth/ble_stack/inc/common \
    $(EFR32_SDK_ROOT)/app/bluetooth/common/util \
    $(HAL_CONF_DIR)


ifeq ($(EFR32FAMILY), efr32mg12)
OPENTHREAD_INC_DIRS += \
    $(EFR32_SDK_ROOT)/platform/Device/SiliconLabs/EFR32MG12P/Include \
    $(EFR32_SDK_ROOT)/platform/radio/rail_lib/chip/efr32/efr32xg1x
else
ifeq ($(EFR32FAMILY), efr32mg21)
OPENTHREAD_INC_DIRS += \
    $(EFR32_SDK_ROOT)/platform/Device/SiliconLabs/EFR32MG21/Include \
    $(EFR32_SDK_ROOT)/platform/radio/rail_lib/chip/efr32/efr32xg2x \
    $(EFR32_SDK_ROOT)/platform \
    $(EFR32_SDK_ROOT)/platform/base \
    $(EFR32_SDK_ROOT)/platform/base/hal/micro/cortexm3/compiler
endif
endif

# Location of hal-config.h for the selected family and board.
HAL_CONF_DIR     = $(OPENTHREAD_ROOT)/examples/platforms/$(OPENTHREAD_TARGET)/$(BOARD_LC)

# ==================================================
# OpenThread configuration options
# ==================================================

OPENTHREAD_CONFIGURE_OPTIONS = \
    CPP="$(CPP)" CC="$(CCACHE) $(CC)" CXX="$(CCACHE) $(CXX)" \
    CCAS="$(CCACHE) $(CC)" AS="${AS}" AR="$(AR)" RANLIB="$(RANLIB)" \
    NM="$(NM)" STRIP="$(STRIP)" OBJDUMP="$(OBJCOPY)" OBJCOPY="$(OBJCOPY)" \
    SIZE="$(SIZE)" RANLIB="$(RANLIB)" INSTALL="$(INSTALL) $(INSTALLFLAGS) -m644" \
    CPPFLAGS="$(OPENTHREAD_CPPFLAGS)" \
    CXXFLAGS="$(OPENTHREAD_CXXFLAGS)" \
    LDFLAGS="" \
    --prefix=$(OPENTHREAD_OUTPUT_DIR) \
    --exec-prefix=$(OPENTHREAD_OUTPUT_DIR)/$(OPENTHREAD_TARGET)/ \
    --host=$(OPENTHREAD_HOST_ARCH) \
    --srcdir="$(OPENTHREAD_ROOT)" \
    --enable-cli \
    --enable-ftd \
    --enable-mtd \
    --enable-linker-map \
    --enable-executable=no \
    --with-examples=$(OPENTHREAD_TARGET) \
    --disable-tools \
    --disable-tests \
    --disable-docs
    
# Enable / disable optimization.
ifeq ($(OPT),1)
OPENTHREAD_CONFIGURE_OPTIONS += --enable-optimization=yes
else
OPENTHREAD_CONFIGURE_OPTIONS += --enable-optimization=no
endif


# ==================================================
# Adjustments to standard build settings to 
#   incorporate OpenThread into the application.
# ==================================================

STD_INC_DIRS += \
    $(OPENTHREAD_OUTPUT_DIR)/include \
    $(OPENTHREAD_ROOT)/third_party/mbedtls/repo/include

# Add the location of OpenThread libraries to application link action.
# Also add the location of the cc310 library, in case it is needed.
STD_LDFLAGS += \
    -L$(OPENTHREAD_LIB_DIR)
    
# Add OpenThread libraries to standard libraries list. 
STD_LIBS += \
    -lopenthread-ftd \
    -lopenthread-cli-ftd \
    -lopenthread-mtd \
    -lopenthread-cli-mtd \
    -lopenthread-platform-utils \
    -lopenthread-$(EFR32FAMILY) \
    -lmbedcrypto \
    -lsilabs-$(EFR32FAMILY)-sdk

# Add the appropriate OpenThread target as a prerequisite to all application
# compilation targets to ensure that OpenThread gets built and its header
# files installed prior to compiling any dependent source files.
STD_COMPILE_PREREQUISITES += $(OPENTHREAD_PREREQUISITE)

# Add the OpenThread libraries as prerequisites for linking the application.
STD_LINK_PREREQUISITES += \
    $(OPENTHREAD_LIB_DIR)/libopenthread-ftd.a \
    $(OPENTHREAD_LIB_DIR)/libopenthread-cli-ftd.a \
    $(OPENTHREAD_LIB_DIR)/libopenthread-cli-mtd.a \
    $(OPENTHREAD_LIB_DIR)/libopenthread-platform-utils.a \
    $(OPENTHREAD_LIB_DIR)/libopenthread-$(EFR32FAMILY).a \
    $(OPENTHREAD_LIB_DIR)/libmbedcrypto.a \
    $(OPENTHREAD_LIB_DIR)/libsilabs-$(EFR32FAMILY)-sdk.a

# ==================================================
# Late-bound build rules for OpenThread
# ==================================================

# Add OpenThreadBuildRules to the list of late-bound build rules that
# will be evaluated when GenerateBuildRules is called. 
LATE_BOUND_RULES += OpenThreadBuildRules

# Rules for configuring, building and installing OpenThread from source.
define OpenThreadBuildRules

.PHONY : bootstrap-thread config-thread .check-config-thread build-thread install-thread clean-thread

bootstrap-thread $(OPENTHREAD_ROOT)/configure : $(OPENTHREAD_ROOT)/configure.ac
	@echo "$(HDR_PREFIX)BOOTSTRAPPING OPENTHREAD..."
	$(NO_ECHO)(cd $(OPENTHREAD_ROOT); ./bootstrap)

.check-config-thread : | $(OPENTHREAD_OUTPUT_DIR)
	@echo $(OPENTHREAD_ROOT)/configure $(OPENTHREAD_CONFIGURE_OPTIONS) > $(OPENTHREAD_OUTPUT_DIR)/config.args.tmp; \
	(test -r $(OPENTHREAD_OUTPUT_DIR)/config.args && cmp -s $(OPENTHREAD_OUTPUT_DIR)/config.args.tmp $(OPENTHREAD_OUTPUT_DIR)/config.args) || \
	    mv $(OPENTHREAD_OUTPUT_DIR)/config.args.tmp $(OPENTHREAD_OUTPUT_DIR)/config.args; \
	rm -f $(OPENTHREAD_OUTPUT_DIR)/config.args.tmp;

$(OPENTHREAD_OUTPUT_DIR)/config.args : .check-config-thread
	@: # Null action required to work around make's crazy timestamp caching behavior.

$(OPENTHREAD_OUTPUT_DIR)/config.status : $(OPENTHREAD_ROOT)/configure $(OPENTHREAD_OUTPUT_DIR)/config.args
	@echo "$(HDR_PREFIX)CONFIGURE OPENTHREAD..."
	$(NO_ECHO)(cd $(OPENTHREAD_OUTPUT_DIR) && $(OPENTHREAD_ROOT)/configure $(OPENTHREAD_CONFIGURE_OPTIONS))

config-thread : $(OPENTHREAD_OUTPUT_DIR)/config.status

build-thread : config-thread
	@echo "$(HDR_PREFIX)BUILD OPENTHREAD..."
	MAKEFLAGS= make -C $(OPENTHREAD_OUTPUT_DIR) LDFLAGS="$(OPENTHREAD_LDFLAGS)" --no-print-directory all V=$(VERBOSE)

install-thread : | build-thread
	@echo "$(HDR_PREFIX)INSTALL OPENTHREAD..."
	$(NO_ECHO)MAKEFLAGS= make -C $(OPENTHREAD_OUTPUT_DIR) --no-print-directory install V=$(VERBOSE)
	$(NO_ECHO)$(INSTALL) $(INSTALLFLAGS) $(OPENTHREAD_PLATFORM_HEADERS) $(OPENTHREAD_OUTPUT_DIR)/include/openthread/platform

clean-thread:
	@echo "$(HDR_PREFIX)RM $(OPENTHREAD_OUTPUT_DIR)"
	$(NO_ECHO)rm -rf $(OPENTHREAD_OUTPUT_DIR)

$(OPENTHREAD_OUTPUT_DIR) :
	@echo "$(HDR_PREFIX)MKDIR $$@"
	$(NO_ECHO)mkdir -p "$$@"

endef

# ==================================================
# OpenThread-specific help definitions
# ==================================================

define TargetHelp +=


  bootstrap-thread      Run the OpenThread bootstrap script.
  
  config-thread         Run the OpenThread configure script.
  
  build-thread          Build the OpenThread libraries.
  
  install-thread        Install OpenThread libraries and headers in 
                        build output directory for use by application.
  
  clean-thread          Clean all build outputs produced by the OpenThread
                        build process.
endef

define OptionHelp +=

endef
