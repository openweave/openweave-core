#
#    Copyright (c) 2018 Nest Labs, Inc.
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
#    Description:
#      Component makefile for building OpenWeave within the ESP32 ESP-IDF environment.
#

# OpenWeave source root directory
WEAVE_ROOT					?= $(realpath ..)

# Archtecture for which Weave will be built.
WEAVE_HOST_ARCH             := armv7-unknown-linux-gnu

# Directory into which the Weave build system will place its output. 
WEAVE_OUTPUT_DIRECTORY		:= $(realpath .)/$(OUTPUT_DIRECTORY)/openweave

# Directory containing esp32-specific Weave project configuration files.
WEAVE_PROJECT_CONFIG_DIR    := $(WEAVE_ROOT)/build/config/nrf5

# Architcture on which Weave is being built.
WEAVE_BUILD_ARCH            := $(shell $(WEAVE_ROOT)/third_party/nlbuild-autotools/repo/third_party/autoconf/config.guess | sed -e 's/[[:digit:].]*$$//g')

# Include directories to be searched when building OpenWeave itself.
WEAVE_BUILD_INCLUDES        += $(WEAVE_ROOT)/src/adaptations/device-layer/trait-support \
                               $(INC_FOLDERS)

# Compiler flags for building Weave
WEAVE_CPPFLAGS              += $(CFLAGS) $(addprefix -I, $(WEAVE_BUILD_INCLUDES))
WEAVE_CXXFLAGS              += $(CXXFLAGS)

# Misc tools and associated values
INSTALL                     := /usr/bin/install
INSTALLFLAGS                := --compare -v

# TODO: FIX THIS!!!
WEAVE_CC = ccache $(GNU_INSTALL_ROOT)/arm-none-eabi-gcc
WEAVE_CXX = ccache $(GNU_INSTALL_ROOT)/arm-none-eabi-g++
AR = $(GNU_INSTALL_ROOT)/arm-none-eabi-ar

# OpenWeave configuration options
CONFIGURE_OPTIONS       	:= AR="$(AR)" CC="$(WEAVE_CC)" CXX="$(WEAVE_CXX)" LD="$(LD)" OBJCOPY="$(OBJCOPY)" \
                               INSTALL="$(INSTALL) $(INSTALLFLAGS)" \
                               CPPFLAGS="$(WEAVE_CPPFLAGS)" \
                               CXXFLAGS="$(WEAVE_CXXFLAGS)" \
                               --prefix=$(WEAVE_OUTPUT_DIRECTORY) \
                               --exec-prefix=$(WEAVE_OUTPUT_DIRECTORY) \
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

# Enable debug and disable optimization if ESP-IDF Optimization Level is set to Debug.
ifeq ($(DEBUG),1)
CONFIGURE_OPTIONS           += --enable-debug --enable-optimization=no
else
CONFIGURE_OPTIONS           +=  --enable-optimization=yes
endif


WEAVE_INCLUDES              := $(WEAVE_OUTPUT_DIRECTORY)/include \
                               $(WEAVE_ROOT)/third_party/lwip/repo/lwip/src/include \
                               $(WEAVE_ROOT)/src/lwip \
							   $(WEAVE_ROOT)/src/lwip/nrf5 \
							   $(WEAVE_ROOT)/src/lwip/freertos

# Libraries to be included when building components that use Weave. 
WEAVE_LIBS                  := -L$(WEAVE_OUTPUT_DIRECTORY)/lib \
							   -Wl,--start-group \
					           -lDeviceLayer \
					           -lWeave \
					           -lWarm \
					           -lInetLayer \
					           -lmincrypt \
					           -lnlfaultinjection \
					           -lSystemLayer \
					           -luECC \
					           -llwip \
					           -Wl,--end-group


# ===== Rules =====

$(WEAVE_OUTPUT_DIRECTORY) :
	echo "MKDIR $@"
	@mkdir -p "$@"

.PHONY : check-config-args-updated
check-config-args-updated : | $(WEAVE_OUTPUT_DIRECTORY)
	echo $(WEAVE_ROOT)/configure $(CONFIGURE_OPTIONS) > $(WEAVE_OUTPUT_DIRECTORY)/config.args.tmp; \
	(test -r $(WEAVE_OUTPUT_DIRECTORY)/config.args && cmp -s $(WEAVE_OUTPUT_DIRECTORY)/config.args.tmp $(WEAVE_OUTPUT_DIRECTORY)/config.args) || \
	    mv $(WEAVE_OUTPUT_DIRECTORY)/config.args.tmp $(WEAVE_OUTPUT_DIRECTORY)/config.args; \
	rm -f $(WEAVE_OUTPUT_DIRECTORY)/config.args.tmp;

$(WEAVE_OUTPUT_DIRECTORY)/config.args : check-config-args-updated
	@: # Null action required to work around make's crazy timestamp caching behavior.

$(WEAVE_OUTPUT_DIRECTORY)/config.status : $(WEAVE_OUTPUT_DIRECTORY)/config.args
	echo "CONFIGURE OPENWEAVE..."
	(cd $(WEAVE_OUTPUT_DIRECTORY) && $(WEAVE_ROOT)/configure $(CONFIGURE_OPTIONS))

.PHONY : configure-weave
configure-weave : $(WEAVE_OUTPUT_DIRECTORY)/config.status

.PHONY : build-weave
build-weave : configure-weave
	echo "BUILD OPENWEAVE..."
	MAKEFLAGS= make -C $(WEAVE_OUTPUT_DIRECTORY) --no-print-directory all V=1

.PHONY : install-weave
install-weave : | build-weave
	echo "INSTALL OPENWEAVE..."
	MAKEFLAGS= make -C $(WEAVE_OUTPUT_DIRECTORY) --no-print-directory install

clean-weave:
	echo "RM $(WEAVE_OUTPUT_DIRECTORY)"
	rm -rf $(WEAVE_OUTPUT_DIRECTORY)
