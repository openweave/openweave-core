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
#      Component makefile for building Nest Weave within the ESP32 ESP-IDF environment.
#

# Weave source directory
WEAVE_ROOT					?= $(COMPONENT_PATH)/weave

# Directories into which the Weave build system will place its output. 
OUTPUT_DIR					:= $(COMPONENT_BUILD_DIR)/output
OUTPUT_LIB_DIR				:= $(OUTPUT_DIR)/lib

# Directory containing esp32-specific Weave project configuration files.
PROJECT_CONFIG_DIR          := $(COMPONENT_PATH)/project-config

# Archtecture for which Weave will be built.
HOST_ARCH                   := xtensa-unknown-linux-gnu

# Architcture on which Weave is being built.
BUILD_ARCH                  := $(shell $(WEAVE_ROOT)/third_party/nlbuild-autotools/repo/autoconf/config.guess | sed -e 's/[[:digit:].]*$$//g')

# Directory containing the esp32-specific LwIP component sources.
LWIP_COMPONENT_DIR      	:= $(PROJECT_PATH)/components/lwip

# Include directories to be searched when building Weave.
INCLUDES                    := $(BUILD_DIR_BASE)/include \
                               $(LWIP_COMPONENT_DIR)/include/lwip \
                               $(LWIP_COMPONENT_DIR)/include/lwip/port \
                               $(IDF_PATH)/components/freertos/include \
                               $(IDF_PATH)/components/freertos/include/freertos \
                               $(IDF_PATH)/components/newlib/include \
                               $(IDF_PATH)/components/esp32/include \
                               $(IDF_PATH)/components/soc/include \
                               $(IDF_PATH)/components/soc/esp32/include \
                               $(IDF_PATH)/components/heap/include \
                               $(IDF_PATH)/components/driver/include

# Compiler flags for building Weave
CFLAGS                      += $(foreach inc,$(INCLUDES),-I$(inc))
CPPFLAGS                    += $(foreach inc,$(INCLUDES),-I$(inc))
CXXFLAGS                    += $(foreach inc,$(INCLUDES),-I$(inc))

INSTALL                     := /usr/bin/install
INSTALLFLAGS                := --compare -v

# Weave source configuration options
CONFIGURE_OPTIONS       	:= AR="$(AR)" CC="$(CC)" CXX="$(CXX)" LD="$(LD)" OBJCOPY="$(OBJCOPY)" \
                               INSTALL="$(INSTALL) $(INSTALLFLAGS)" \
                               CFLAGS="$(CFLAGS)" \
                               CPPFLAGS="$(CPPFLAGS)" \
                               CXXFLAGS="$(CXXFLAGS)" \
                               --prefix=$(OUTPUT_DIR) \
                               --exec-prefix=$(OUTPUT_DIR) \
                               --host=$(HOST_ARCH) \
                               --build=$(BUILD_ARCH) \
                               --with-network-layer=inet \
                               --with-target-network=lwip \
                               --with-lwip=$(LWIP_COMPONENT_DIR) \
                               --with-inet-endpoint="tcp udp tun dns" \
                               --with-openssl=no \
                               --disable-tests \
                               --disable-tools \
                               --disable-docs \
                               --disable-java \
                               --disable-device-manager \
                               --with-logging-style=external \
                               --with-weave-project-includes=$(PROJECT_CONFIG_DIR) \
                               --with-weave-system-project-includes=$(PROJECT_CONFIG_DIR) \
                               --with-weave-inet-project-includes=$(PROJECT_CONFIG_DIR) \
                               --with-weave-warm-project-includes=$(PROJECT_CONFIG_DIR)

# Enable debug and disable optimization if ESP-IDF Optimization Level is set to Debug.
ifeq ($(CONFIG_OPTIMIZATION_LEVEL_DEBUG),y)
CONFIGURE_OPTIONS           += --enable-debug --enable-optimization=no
else
CONFIGURE_OPTIONS           +=  --enable-optimization=yes
endif

# Header directories to be included when building other components that use Weave. 
COMPONENT_ADD_INCLUDEDIRS 	 = project-config \
							   ../../build/weave/output/include

# Linker flags to be included when building other components that use Weave. 
COMPONENT_ADD_LDFLAGS        = -L$(OUTPUT_LIB_DIR) \
					           -lWeave \
					           -lInetLayer \
					           -lmincrypt \
					           -lnlfaultinjection \
					           -lSystemLayer \
					           -luECC \
					           -lWarm

# Tell the ESP-IDF build system that the Weave component defines its own build
# and clean targets.
COMPONENT_OWNBUILDTARGET 	 = 1
COMPONENT_OWNCLEANTARGET 	 = 1


# ===== Rules =====

.PHONY : check-config-args-updated
check-config-args-updated : | $(OUTPUT_DIR)
	echo $(WEAVE_ROOT)/configure $(CONFIGURE_OPTIONS) > $(OUTPUT_DIR)/config.args.tmp; \
	(test -r $(OUTPUT_DIR)/config.args && cmp -s $(OUTPUT_DIR)/config.args.tmp $(OUTPUT_DIR)/config.args) || \
	    mv $(OUTPUT_DIR)/config.args.tmp $(OUTPUT_DIR)/config.args; \
	rm -f $(OUTPUT_DIR)/config.args.tmp;

$(OUTPUT_DIR)/config.args : check-config-args-updated
	@: # Null action required to work around make's crazy timestamp caching behavior.

$(OUTPUT_DIR)/config.status : $(OUTPUT_DIR)/config.args
	echo "CONFIGURE WEAVE..."
	(cd $(OUTPUT_DIR) && $(WEAVE_ROOT)/configure $(CONFIGURE_OPTIONS))

configure-weave : $(OUTPUT_DIR)/config.status

$(OUTPUT_DIR) :
	echo "MKDIR $@"
	@mkdir -p "$@"

build-weave : configure-weave
	echo "BUILD WEAVE..."
	MAKEFLAGS= make -C $(OUTPUT_DIR) --no-print-directory all

install-weave : | build-weave
	echo "INSTALL WEAVE..."
	MAKEFLAGS= make -C $(OUTPUT_DIR) --no-print-directory install

build : build-weave install-weave

clean:
	echo "RM $(OUTPUT_DIR)"
	rm -rf $(OUTPUT_DIR)
