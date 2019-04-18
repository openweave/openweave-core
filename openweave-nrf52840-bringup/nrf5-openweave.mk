#
#   Copyright (c) 2019 Google LLC.
#   All rights reserved.
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#

#
#   @file
#         Component makefile for incorporating OpenWeave into an nRF5
#         application.
#

#
#   This makefile is designed to work in conjunction with the nrf5-app.mk
#   makefile.  nRF5 applications should include this file in their top
#   level Makefile after including nrf5-app.mk.  E.g.:
#
#       include nrf5-app.mk
#       include nrf5-openweave.mk
#
#       PROJECT_ROOT := $(realpath .)
#       
#       APP := openweave-nrf52840-bringup
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

# Location of OpenWeave source tree
WEAVE_ROOT ?= $(realpath ..)

# Archtecture for which OpenWeave will be built.
WEAVE_HOST_ARCH := armv7-unknown-linux-gnu

# Directory into which the OpenWeave build system will place its output. 
WEAVE_OUTPUT_DIR = $(OUTPUT_DIR)/openweave

# Directory containing nRF5-specific Weave project configuration files.
WEAVE_PROJECT_CONFIG_DIR = $(WEAVE_ROOT)/build/config/nrf5

# Architcture on which OpenWeave is being built.
WEAVE_BUILD_ARCH = $(shell $(WEAVE_ROOT)/third_party/nlbuild-autotools/repo/third_party/autoconf/config.guess | sed -e 's/[[:digit:].]*$$//g')


# ==================================================
# Compilation flags specific to building OpenWeave
# ==================================================

WEAVE_CPPFLAGS = $(STD_CFLAGS) $(CFLAGS) $(OPT_FLAGS) $(DEFINE_FLAGS) $(INC_FLAGS) 
WEAVE_CXXFLAGS = $(STD_CXXFLAGS) $(CXXFLAGS) 


# ==================================================
# OpenWeave configuration options
# ==================================================

WEAVE_CONFIGURE_OPTIONS = \
    AR="$(AR)" AS="$(AS)" CC="$(CC)" CXX="$(CXX)" LD="$(LD)" OBJCOPY="$(OBJCOPY)" RANLIB="$(RANLIB)" \
    INSTALL="$(INSTALL) $(INSTALLFLAGS)" \
    CPPFLAGS="$(WEAVE_CPPFLAGS)" \
    CXXFLAGS="$(WEAVE_CXXFLAGS)" \
    --prefix=$(WEAVE_OUTPUT_DIR) \
    --exec-prefix=$(WEAVE_OUTPUT_DIR) \
    --host=$(WEAVE_HOST_ARCH) \
    --build=$(WEAVE_BUILD_ARCH) \
    --with-device-layer=nrf5 \
    --with-network-layer=all \
    --with-target-network=lwip \
    --with-lwip=internal \
    --with-lwip-target=nrf5 \
    --with-inet-endpoint="tcp udp" \
    --with-openssl=no \
    --with-logging-style=external \
    --with-weave-project-includes=$(WEAVE_PROJECT_CONFIG_DIR) \
    --with-weave-system-project-includes=$(WEAVE_PROJECT_CONFIG_DIR) \
    --with-weave-inet-project-includes=$(WEAVE_PROJECT_CONFIG_DIR) \
    --with-weave-ble-project-includes=$(WEAVE_PROJECT_CONFIG_DIR) \
    --with-weave-warm-project-includes=$(WEAVE_PROJECT_CONFIG_DIR) \
    --disable-ipv4 \
    --disable-tests \
    --disable-tools \
    --disable-docs \
    --disable-java \
    --disable-device-manager

# Enable debug and disable optimization if DEBUG set.
ifeq ($(DEBUG),1)
WEAVE_CONFIGURE_OPTIONS += --enable-debug --enable-optimization=no
else
WEAVE_CONFIGURE_OPTIONS += --enable-optimization=yes
endif


# ==================================================
# Adjustments to application build settings to 
#   incorporate OpenWeave
# ==================================================

# Add OpenWeave-specific paths to the standard include directories.
STD_INC_DIRS += \
    $(WEAVE_OUTPUT_DIR)/include \
    $(WEAVE_ROOT)/src/adaptations/device-layer/trait-support \
    $(WEAVE_ROOT)/third_party/lwip/repo/lwip/src/include \
    $(WEAVE_ROOT)/src/lwip \
	$(WEAVE_ROOT)/src/lwip/nrf5 \
	$(WEAVE_ROOT)/src/lwip/freertos

# Add OpenWeave-specific flags to standard link flags.
STD_LDFLAGS += -L$(WEAVE_OUTPUT_DIR)/lib

# Add OpenWeave libraries to standard libraries list. 
STD_LIBS += \
    -lDeviceLayer \
	-lWeave \
	-lWarm \
	-lInetLayer \
	-lmincrypt \
	-lnlfaultinjection \
	-lSystemLayer \
	-luECC \
	-llwip

# Add install-weave target as a compilation prerequisite to all
# application source files to ensure that OpenWeave gets built and
# installed prior to compiling the application.
COMPILE_PREREQUISITES += install-weave


# ==================================================
# Late-bound build rules for OpenWeave
# ==================================================

# Add OpenWeaveBuildRules to the list of late-bound build rules that
# will be evaluated when GenerateBuildRules is called. 
LATE_BOUND_RULES += OpenWeaveBuildRules

define OpenWeaveBuildRules

.PHONY : config-weave check-config-weave build-weave install-weave clean-weave

check-config-weave : | $(WEAVE_OUTPUT_DIR)
	@echo $(WEAVE_ROOT)/configure $(WEAVE_CONFIGURE_OPTIONS) > $(WEAVE_OUTPUT_DIR)/config.args.tmp; \
	(test -r $(WEAVE_OUTPUT_DIR)/config.args && cmp -s $(WEAVE_OUTPUT_DIR)/config.args.tmp $(WEAVE_OUTPUT_DIR)/config.args) || \
	    mv $(WEAVE_OUTPUT_DIR)/config.args.tmp $(WEAVE_OUTPUT_DIR)/config.args; \
	rm -f $(WEAVE_OUTPUT_DIR)/config.args.tmp;

$(WEAVE_OUTPUT_DIR)/config.args : check-config-weave
	@: # Null action required to work around make's crazy timestamp caching behavior.

$(WEAVE_OUTPUT_DIR)/config.status : $(WEAVE_OUTPUT_DIR)/config.args
	@echo "$(HDR_PREFIX)CONFIGURE OPENWEAVE..."
	$(NO_ECHO)(cd $(WEAVE_OUTPUT_DIR) && $(WEAVE_ROOT)/configure $(WEAVE_CONFIGURE_OPTIONS))

config-weave : $(WEAVE_OUTPUT_DIR)/config.status

build-weave : config-weave
	@echo "$(HDR_PREFIX)BUILD OPENWEAVE..."
	$(NO_ECHO)MAKEFLAGS= make -C $(WEAVE_OUTPUT_DIR) --no-print-directory all V=$(VERBOSE)

install-weave : | build-weave
	@echo "$(HDR_PREFIX)INSTALL OPENWEAVE..."
	$(NO_ECHO)MAKEFLAGS= make -C $(WEAVE_OUTPUT_DIR) --no-print-directory install V=$(VERBOSE)

clean-weave:
	@echo "$(HDR_PREFIX)RM $(WEAVE_OUTPUT_DIR)"
	$(NO_ECHO)rm -rf $(WEAVE_OUTPUT_DIR)

$(WEAVE_OUTPUT_DIR) :
	@echo "$(HDR_PREFIX)MKDIR $$@"
	$(NO_ECHO)mkdir -p "$$@"

endef


# ==================================================
# OpenWeave-specific help definitions
# ==================================================

define TargetHelp +=


  config-weave          Run the OpenWeave configure script.
  
  build-weave           Build the OpenWeave libraries.
  
  install-weave         Install OpenWeave libraries and headers in 
                        build output directory for use by application.
  
  clean-weave           Clean all build outputs produced by the OpenWeave
                        build process.
endef
