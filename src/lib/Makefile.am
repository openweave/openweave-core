#
#    Copyright (c) 2014-2018 Nest Labs, Inc.
#    Copyright (c) 2019-2020 Google, LLC.
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
#      This file is the GNU automake template for the Nest Weave
#      library.
#

include $(abs_top_nlbuild_autotools_dir)/automake/pre.am

# Pull in the sources that comprise the Weave library.

include ../system/SystemLayer.am
include ../inet/InetLayer.am
include ../ble/BleLayer.am
include core/WeaveCore.am
include support/WeaveSupport.am
include profiles/WeaveProfiles.am
include ../warm/Warm.am
include ../device-manager/DeviceManager.am

lib_LIBRARIES                        = libWeave.a

libWeave_a_CPPFLAGS                  =                        \
    -I$(top_srcdir)/src/include                               \
    -I$(top_srcdir)/third_party/openssl-jpake/openssl/include \
    $(nl_SystemLayer_CPPFLAGS)                                \
    $(nl_InetLayer_CPPFLAGS)                                  \
    $(LWIP_CPPFLAGS)                                          \
    $(SOCKETS_CPPFLAGS)                                       \
    $(NULL)

if WEAVE_CONFIG_CUSTOM_BUILTIN_SCHEMA_INCLUDE
libWeave_a_CPPFLAGS                 += \
    -I$(WEAVE_CONFIG_CUSTOM_BUILTIN_SCHEMA_INCLUDE)              \
    $(NULL)
else
libWeave_a_CPPFLAGS                 += \
    -I$(top_srcdir)/src/test-apps/schema                      \
    $(NULL)
endif # WEAVE_CONFIG_CUSTOM_BUILTIN_SCHEMA_INCLUDE

if WEAVE_ENABLE_WOBLE_TEST
libWeave_a_CPPFLAGS                 += \
    -I$(top_srcdir)/src/device-manager \
    $(NULL)
endif # WEAVE_ENABLE_WOBLE_TEST

libWeave_a_SOURCES                   = $(nl_SystemLayer_sources)
libWeave_a_SOURCES                  += $(nl_InetLayer_sources)
libWeave_a_SOURCES                  += $(nl_DeviceManager_sources)

if CONFIG_NETWORK_LAYER_BLE
libWeave_a_SOURCES                  += $(nl_BleLayer_sources)
endif # CONFIG_NETWORK_LAYER_BLE

libWeave_a_SOURCES                  += $(nl_WeaveCore_sources)
libWeave_a_SOURCES                  += $(nl_WeaveSupport_sources)
libWeave_a_SOURCES                  += $(nl_WeaveProfiles_sources)

if WEAVE_BUILD_WARM
libWeave_a_SOURCES                  += $(nl_Warm_sources)
endif # WEAVE_BUILD_WARM

# If and ONLY if Weave has been configured to use the interal OpenSSL, then 
# include the OpenSSL objects in with libWeave.a.  Effectively, the internal
# OpenSSL is a private, Weave-only implementation which we don't want interfering
# with the host OpenSSL.
#
# The symbols are already built with default hidden visibility, so
# complete the hiding by absorbing the package objects into the Weave
# archive such that the integrator will not need to resolve any
# OpenSSL symbols within Weave itself.
#
# NOTE: This is NOT required for shared library support (Weave as a
#       lib_LTLIBRARY) because the static OpenSSL archive is linked in the
#       shared Weave DLL/DSO.
#
#       This will not stop the bleeding of the internal OpenSSL
#       headers through Weave public headers, potentially causing header
#       conflicts with the host system OpenSSL headers.
#
# We also need to play tricks with automake via 'Dash' because,
# otherwise, automake thinks that -type and -name are linker options.
#
Dash                                 = -
#if WEAVE_WITH_OPENSSL_INTERNAL
libWeave_a_LIBADD                    = $(shell find $(top_builddir)/third_party/openssl $(Dash)type f $(Dash)name "*.o" $(Dash)path "*/crypto/*.o")
#endif # WEAVE_WITH_OPENSSL_INTERNAL

# Include micro-ecc objects in with libWeave.a
libWeave_a_LIBADD                   += $(shell find $(top_builddir)/third_party/micro-ecc $(Dash)type f $(Dash)name "*.o")

# Include mincrypt objects in with libWeave.a
libWeave_a_LIBADD                   += $(shell find $(top_builddir)/third_party/android/platform-system/core-mincrypt $(Dash)type f $(Dash)name "*.o")

# Include openssl-jpake objects in with libWeave.a
libWeave_a_LIBADD                   += $(shell find $(top_builddir)/third_party/openssl-jpake $(Dash)type f $(Dash)name "*.o")

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
