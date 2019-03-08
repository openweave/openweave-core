#
#    Copyright (c) 2018-2019 Nest Labs, Inc.
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
#    Description:
#      Helper makefile for building OpenWeave for Android from within a Gradle-based
#      Android project.
#
#      NB: This version of the helper makefile only works with Android NDK versions
#      r19 and higher.
#


# ================================================================================
# Utility Definitions
# ================================================================================

HOST_OS                         = $(shell uname -s)
HOST_HW                         = $(shell uname -m)
HOST_PROCS                      = $(shell getconf _NPROCESSORS_ONLN)


# ================================================================================
# Input Configuration
#
# When invoked from within a gradle project, these values are passed in via the
# environment by the BuildWeaveTask.
# ================================================================================

# ANDROID_HOME: Installation location of the Android SDK
ANDROID_HOME                   ?= /opt/android-sdk-linux/

# ANDROID_NDK_HOME: Installation location of the Android NDK
ANDROID_NDK_HOME               ?= $(ANDROID_HOME)/ndk-bundle

# ANDROID_API: Target Android API version 
ANDROID_API                    ?= 19

# ANDROID_ABI: Target Android ABI 
ANDROID_ABI                    ?= armeabi-v7a

# ANDROID_STL: Target C++ STL 
ANDROID_STL                    ?= system

# ANDROID_HOST_TAG: Name of subdirectory within NDK in which executables for
# this build host reside.
# (See https://developer.android.com/ndk/guides/other_build_systems).
ifeq ($(HOST_OS),Linux)
  ANDROID_HOST_TAG             ?= linux-x86_64
endif
ifeq ($(HOST_OS),Darwin)
  ANDROID_HOST_TAG             ?= darwin-x86_64
endif

# WEAVE_SOURCE_ROOT: Root directory for OpenWeave source tree.
WEAVE_SOURCE_ROOT              ?= /home/jay/projects/weave

# WEAVE_BUILD_DIR: Directory in which temporary build output is placed.
WEAVE_BUILD_DIR                ?= /tmp/weave-build/$(ANDROID_ABI)

# WEAVE_RESULTS_DIR: Directory in which final build artifacts are placed.
WEAVE_RESULTS_DIR              ?= /tmp/weave-output/$(ANDROID_ABI)

# ENABLE_WEAVE_DEBUG: Enable/disable code optimization and generation of symbol information. 
ENABLE_WEAVE_DEBUG             ?= 1

# ENABLE_WEAVE_LOGGING: Enable/disable OpenWeave logging code.
ENABLE_WEAVE_LOGGING           ?= 1

# BUILD_JOBS: Number of simultaneous processes to use during a build.
BUILD_JOBS                     ?= $(HOST_PROCS)



# ================================================================================
# Output Configuration
# ================================================================================

WEAVE_SHARED_LIB				= libWeaveSecuritySupport.so
WEAVE_SHARED_LIB_BUILD_DIR		= $(WEAVE_BUILD_DIR)/src/wrappers/jni/security-support/.libs


# ================================================================================
# Tools and Executables
# ================================================================================

TOOLCHAIN_BIN_DIR               = $(ANDROID_NDK_HOME)/toolchains/llvm/prebuilt/${ANDROID_HOST_TAG}/bin

AR                              = $(TOOLCHAIN_BIN_DIR)/$(ABI_BINTOOLS_PREFIX)ar
AS                              = $(TOOLCHAIN_BIN_DIR)/$(ABI_BINTOOLS_PREFIX)as
CPP                             =
# NOTE: One might be inclined to use the generic clang executable provided in the NDK,
# and then specify the target architecture using clang's -target option.  However libtool,
# which is used by OpenWeave build system, is hostile to this option.  Thus we invoke the
# target-specific clang executable as a work-around.
CC                              = $(TOOLCHAIN_BIN_DIR)/$(ABI_TARGET_TUPLE)$(ANDROID_API)-clang
CXX                             = $(TOOLCHAIN_BIN_DIR)/$(ABI_TARGET_TUPLE)$(ANDROID_API)-clang++
LD                              = $(TOOLCHAIN_BIN_DIR)/$(ABI_BINTOOLS_PREFIX)ld
STRIP                           = $(TOOLCHAIN_BIN_DIR)/$(ABI_BINTOOLS_PREFIX)strip
NM                              = $(TOOLCHAIN_BIN_DIR)/$(ABI_BINTOOLS_PREFIX)nm
RANLIB                          = $(TOOLCHAIN_BIN_DIR)/$(ABI_BINTOOLS_PREFIX)ranlib
OBJCOPY                         = $(TOOLCHAIN_BIN_DIR)/$(ABI_BINTOOLS_PREFIX)objcopy
ECHO                            = @echo
MAKE                            = make
MKDIR_P                         = mkdir -p
LN_S                            = ln -s
RM_F                            = rm -f
INSTALL                         = /usr/bin/install
CONFIG_GUESS                    = $(WEAVE_SOURCE_ROOT)/third_party/nlbuild-autotools/repo/third_party/autoconf/config.guess


# ================================================================================
# Common C/C++ Defines
# ================================================================================

DEFINES                        += -D__ANDROID_API__=$(ANDROID_API) \
                                  -DWEAVE_PROGRESS_LOGGING=$(ENABLE_WEAVE_LOGGING) \
                                  -DWEAVE_ERROR_LOGGING=$(ENABLE_WEAVE_LOGGING) \
                                  -DWEAVE_DETAIL_LOGGING=$(ENABLE_WEAVE_LOGGING)

ifeq ($(ENABLE_WEAVE_DEBUG),1)
DEFINES                         = -DDEBUG=$(ENABLE_WEAVE_DEBUG) -UNDEBUG
else
DEFINES                         = -DNDEBUG=1 -UDEBUG
endif


# ================================================================================
# Common C/C++ Includes
# ================================================================================

INCLUDES                       += -isystem $(ANDROID_NDK_HOME)/sysroot/usr/include \
                                  $(ANDROID_STL_INCLUDES)

# NB: Regardless of where JAVA_HOME points, always use the JNI headers from the Android NDK,
# and only include the top-most directory (include), not the system directory (include/linux). 
# Because the NDK mixes the JNI headers in with the linux headers, listing the system
# directory in the -I flags will result in strange compilation errors.  And unlike the standard
# Java jni.h, the jni.h that comes with the Android NDK does not depend on any system-specific
# JNI header files (e.g. jni_md.h).  Thus only the top-level include directory is needed.
JNI_INCLUDE_DIRS                = $(ABI_SYSROOT)/usr/include


# ================================================================================
# Compilation/Build Flags
# ================================================================================
 
CPPFLAGS                        = $(ABI_CPPFLAGS) $(DEFINES) $(INCLUDES)
CFLAGS                          = $(ABI_CFLAGS) $(CPPFLAGS) -ffunction-sections -funwind-tables
CXXFLAGS                        = $(ABI_CXXFLAGS) $(CPPFLAGS) -fno-rtti
LDFLAGS                         = $(ABI_LDFLAGS) -Wl,--gc-sections
INSTALLFLAGS                    = -p
JOBSFLAG                        = -j$(BUILD_JOBS)


# ================================================================================
# OpenWeave Configure Options
# ================================================================================

CONFIGURE_OPTIONS               = --prefix=/ \
                                  --enable-java \
                                  --enable-shared \
                                  --disable-cocoa \
                                  --disable-docs \
                                  --disable-python \
                                  --disable-tests \
                                  --disable-tools \
                                  --build=$(shell $(CONFIG_GUESS)) \
                                  --host=$(ABI_CONFIG_TUPLE) \
                                  --with-weave-project-includes=$(WEAVE_SOURCE_ROOT)/build/config/android \

ifeq ($(ENABLE_WEAVE_DEBUG),1)
CONFIGURE_OPTIONS              += --enable-debug --enable-optimization=no
else
CONFIGURE_OPTIONS              += --enable-optimization=yes
endif


# ================================================================================
# Android ABI-specific Configurations
# ================================================================================

ABI_TARGET_TUPLE_armeabi        = arm-linux-androideabi
ABI_BINTOOLS_PREFIX_armeabi     = arm-linux-androideabi-
ABI_CONFIG_TUPLE_armeabi        = arm-unknown-linux-android
ABI_SYSROOT_armeabi             = $(ANDROID_NDK_HOME)/platforms/android-$(ANDROID_API)/arch-arm
ABI_CPPFLAGS_armeabi            = -march=armv5te -mtune=xscale -msoft-float -isystem $(ANDROID_NDK_HOME)/sysroot/usr/include/arm-linux-androideabi
ABI_CFLAGS_armeabi              = -fstack-protector
ABI_CXXFLAGS_armeabi            = -I$(ANDROID_NDK_HOME)/platforms/android-$(ANDROID_API)/arch-arm/usr/include
ABI_LDFLAGS_armeabi             = -march=armv5te -mtune=xscale -msoft-float

ABI_TARGET_TUPLE_armeabi-v7a    = armv7a-linux-androideabi
ABI_BINTOOLS_PREFIX_armeabi-v7a = arm-linux-androideabi-
ABI_CONFIG_TUPLE_armeabi-v7a    = armv7-unknown-linux-android
ABI_SYSROOT_armeabi-v7a         = $(ANDROID_NDK_HOME)/platforms/android-$(ANDROID_API)/arch-arm
ABI_CPPFLAGS_armeabi-v7a        = -march=armv7-a -mfpu=vfpv3-d16 -mfloat-abi=softfp -isystem $(ANDROID_NDK_HOME)/sysroot/usr/include/arm-linux-androideabi
ABI_CFLAGS_armeabi-v7a          = -fstack-protector
ABI_CXXFLAGS_armeabi-v7a        = -I$(ANDROID_NDK_HOME)/platforms/android-$(ANDROID_API)/arch-arm/usr/include
ABI_LDFLAGS_armeabi-v7a         = -march=armv7-a -Wl,--fix-cortex-a8

ABI_TARGET_TUPLE_arm64-v8a      = aarch64-linux-android
ABI_BINTOOLS_PREFIX_arm64-v8a   = aarch64-linux-android-
ABI_CONFIG_TUPLE_arm64-v8a      = arm64-unknown-linux-android
ABI_SYSROOT_arm64-v8a           = $(ANDROID_NDK_HOME)/platforms/android-$(ANDROID_API)/arch-arm64
ABI_CPPFLAGS_arm64-v8a          = -march=armv8-a -isystem $(ANDROID_NDK_HOME)/sysroot/usr/include/aarch64-linux-android
ABI_CFLAGS_arm64-v8a            = -fstack-protector
ABI_CXXFLAGS_arm64-v8a          = -I$(ANDROID_NDK_HOME)/platforms/android-$(ANDROID_API)/arch-arm/usr/include
ABI_LDFLAGS_arm64-v8a           = -march=armv8-a

ABI_TARGET_TUPLE_x86            = i686-linux-android
ABI_BINTOOLS_PREFIX_x86         = i686-linux-android-
ABI_CONFIG_TUPLE_x86            = x86-unknown-linux-android
ABI_SYSROOT_x86                 = $(ANDROID_NDK_HOME)/platforms/android-$(ANDROID_API)/arch-x86
ABI_CPPFLAGS_x86                = -march=i686 -mtune=intel -mssse3 -mfpmath=sse -m32 -isystem $(ANDROID_NDK_HOME)/sysroot/usr/include/i686-linux-android
ABI_CFLAGS_x86                  =
ABI_CXXFLAGS_x86                = -I$(ANDROID_NDK_HOME)/platforms/android-$(ANDROID_API)/arch-x86/usr/include
ABI_LDFLAGS_x86                 = -march=i686 -mtune=intel -mssse3 -mfpmath=sse -m32

ABI_TARGET_TUPLE_x86_64         = x86_64-linux-android
ABI_BINTOOLS_PREFIX_x86_64      = x86_64-linux-android-
ABI_CONFIG_TUPLE_x86_64         = x86_64-unknown-linux-android
ABI_SYSROOT_x86_64              = $(ANDROID_NDK_HOME)/platforms/android-$(ANDROID_API)/arch-x86_64
ABI_CPPFLAGS_x86_64             = -march=x86-64 -msse4.2 -mpopcnt -m64 -mtune=intel -isystem $(ANDROID_NDK_HOME)/sysroot/usr/include/x86_64-linux-android
ABI_CFLAGS_x86_64               =
ABI_CXXFLAGS_x86_64             = -I$(ANDROID_NDK_HOME)/platforms/android-$(ANDROID_API)/arch-x86/usr/include
ABI_LDFLAGS_x86_64              = -march=x86-64 -msse4.2 -mpopcnt -m64 -mtune=intel

ABI_TARGET_TUPLE                = $(ABI_TARGET_TUPLE_$(ANDROID_ABI))
ABI_BINTOOLS_PREFIX             = $(ABI_BINTOOLS_PREFIX_$(ANDROID_ABI))
ABI_CONFIG_TUPLE                = $(ABI_CONFIG_TUPLE_$(ANDROID_ABI))
ABI_SYSROOT                     = $(ABI_SYSROOT_$(ANDROID_ABI))
ABI_CPPFLAGS                    = $(ABI_CPPFLAGS_$(ANDROID_ABI))
ABI_CFLAGS                      = $(ABI_CFLAGS_$(ANDROID_ABI))
ABI_CXXFLAGS                    = $(ABI_CXXFLAGS_$(ANDROID_ABI))
ABI_LDFLAGS                     = $(ABI_LDFLAGS_$(ANDROID_ABI))


# ================================================================================
# Android C++ Runtime (STL) Selection
# ================================================================================

ANDROID_STLROOT                 = $(ANDROID_NDK_HOME)/sources/cxx-stl

ANDROID_STL_INCLUDE_DIRS_libc++ = $(ANDROID_STLROOT)/llvm-libc++/include \
                                  $(ANDROID_STLROOT)/llvm-libc++abi/include \
                                  $(ANDROID_NDK_HOME)/sources/android/support/include
ANDROID_STL_INCLUDE_DIRS_gnustl = $(ANDROID_STLROOT)/gnu-libstdc++/4.9/include
ANDROID_STL_INCLUDE_DIRS_system = $(ANDROID_STLROOT)/system/include

ANDROID_STL_INCLUDES            = $(foreach dir,$(ANDROID_STL_INCLUDE_DIRS_$(ANDROID_STL)),-isystem $(dir))


# ================================================================================
# Build Rules
# ================================================================================

.DEFAULT_GOAL := all

.PHONY : all display-env build configure install

all: install

display-env:
	@echo "----------------------------------------------------------------------"
	@echo "Makefile             = $(realpath $(lastword $(MAKEFILE_LIST)))"
	@echo "JAVA_HOME            = $(JAVA_HOME) (source: $(origin JAVA_HOME))"
	@echo "ANDROID_HOME         = $(ANDROID_HOME) (source: $(origin ANDROID_HOME))"
	@echo "ANDROID_NDK_HOME     = $(ANDROID_NDK_HOME) (source: $(origin ANDROID_NDK_HOME))"
	@echo "ANDROID_API          = $(ANDROID_API) (source: $(origin ANDROID_API))"
	@echo "ANDROID_ABI          = $(ANDROID_ABI) (source: $(origin ANDROID_ABI))"
	@echo "ANDROID_STL          = $(ANDROID_STL) (source: $(origin ANDROID_STL))"
	@echo "ANDROID_HOST_TAG     = $(ANDROID_HOST_TAG) (source: $(origin ANDROID_HOST_TAG))"
	@echo "WEAVE_SOURCE_ROOT    = $(WEAVE_SOURCE_ROOT) (source: $(origin WEAVE_SOURCE_ROOT))"
	@echo "WEAVE_BUILD_DIR      = $(WEAVE_BUILD_DIR) (source: $(origin WEAVE_BUILD_DIR))"
	@echo "WEAVE_RESULTS_DIR    = $(WEAVE_RESULTS_DIR) (source: $(origin WEAVE_RESULTS_DIR))"
	@echo "ENABLE_WEAVE_DEBUG   = $(ENABLE_WEAVE_DEBUG) (source: $(origin ENABLE_WEAVE_DEBUG))"
	@echo "ENABLE_WEAVE_LOGGING = $(ENABLE_WEAVE_LOGGING) (source: $(origin ENABLE_WEAVE_LOGGING))"
	@echo "BUILD_JOBS           = $(BUILD_JOBS) (source: $(origin BUILD_JOBS))"
	@echo "HOST_OS              = $(HOST_OS)"
	@echo "HOST_HW              = $(HOST_HW)"
	@echo "HOST_PROCS           = $(HOST_PROCS)"
	@echo "----------------------------------------------------------------------"

install: display-env $(WEAVE_RESULTS_DIR)/$(WEAVE_SHARED_LIB)

$(WEAVE_RESULTS_DIR)/$(WEAVE_SHARED_LIB): $(WEAVE_SHARED_LIB_BUILD_DIR)/$(WEAVE_SHARED_LIB) | $(WEAVE_RESULTS_DIR)
	$(ECHO) "  INSTALL"
	$(INSTALL) $^ $@

$(WEAVE_BUILD_DIR)/src/wrappers/jni/security-support/.libs/libWeaveSecuritySupport.so: build

build: display-env configure
	$(ECHO) "  BUILD"
	$(MAKE) $(JOBSFLAG) -C $(WEAVE_BUILD_DIR) --no-print-directory all V=1

configure: display-env $(WEAVE_BUILD_DIR)/config.status

$(WEAVE_BUILD_DIR)/config.status: | $(WEAVE_BUILD_DIR)
	$(ECHO) "  CONFIG"
	cd $(WEAVE_BUILD_DIR) && \
	$(WEAVE_SOURCE_ROOT)/configure \
        CPP="$(CPP)" \
        CC="$(CC)" \
        CXX="$(CXX)" \
        OBJC="$(OBJC)" \
        OBJCXX="$(OBJCXX)" \
        AR="$(AR)" \
        RANLIB="$(RANLIB)" \
        NM="$(NM)" \
        STRIP="$(STRIP)" \
        INSTALL="$(INSTALL) $(INSTALLFLAGS)" \
        CPPFLAGS="$(CPPFLAGS)" \
        CFLAGS="$(CFLAGS)" \
        CXXFLAGS="$(CXXFLAGS)" \
        OBJCFLAGS="$(OBJCFLAGS)" \
        OBJCXXFLAGS="$(OBJCXXFLAGS)" \
        LDFLAGS="$(LDFLAGS)" \
        JAVA_HOME="$(JAVA_HOME)" \
        JNI_INCLUDE_DIRS="$(JNI_INCLUDE_DIRS)" \
        $(CONFIGURE_OPTIONS)
	
$(WEAVE_BUILD_DIR) $(WEAVE_RESULTS_DIR):
	$(ECHO) "  MKDIR    $@"
	@$(MKDIR_P) "$@"
