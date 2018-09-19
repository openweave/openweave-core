#
#    Copyright (c) 2015-2018 Nest Labs, Inc.
#    Copyright (c) 2018 Google LLC
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
#      This file is the GNU automake template for the Nest Labs Weave
#      third-party software directory.
#

include $(abs_top_nlbuild_autotools_dir)/automake/pre.am

EXTRA_DIST                                   = \
    android/platform-libcore                   \
    android/platform-system/README.third_party \
    bluez/bluez.url                            \
    bluez/bluez.version                        \
    cstyle                                     \
    nlbuild-autotools                          \
    $(NULL)

# Always package (e.g. for 'make dist') these subdirectories.

DIST_SUBDIRS                                 = \
    lwip                                       \
    android/platform-system/core-mincrypt      \
    bluez/repo                                 \
    micro-ecc                                  \
    openssl                                    \
    openssl-jpake                              \
    nltest/repo                                \
    nlio/repo                                  \
    nlassert/repo                              \
    nlfaultinjection/repo                      \
    $(NULL)

# Always build (e.g. for 'make all') these subdirectories.

SUBDIRS                                      = \
    android/platform-system/core-mincrypt      \
    micro-ecc                                  \
    openssl                                    \
    openssl-jpake                              \
    $(NULL)

# Only build (e.g. for 'make all') these subdirectories when requested.

if WEAVE_WITH_NLUNITTEST_INTERNAL
SUBDIRS                                    += \
    nltest/repo                               \
    $(NULL)
endif # WEAVE_WITH_NLUNITTEST_INTERNAL

if WEAVE_WITH_NLIO_INTERNAL
SUBDIRS                                    += \
    nlio/repo                                 \
    $(NULL)
endif # WEAVE_WITH_NLIO_INTERNAL

if WEAVE_WITH_NLASSERT_INTERNAL
SUBDIRS                                    += \
    nlassert/repo                             \
    $(NULL)
endif # WEAVE_WITH_NLASSERT_INTERNAL

if WEAVE_WITH_NLFAULTINJECTION_INTERNAL
SUBDIRS                                    += \
    nlfaultinjection/repo                     \
    $(NULL)
endif # WEAVE_WITH_NLFAULTINJECTION_INTERNAL

if WEAVE_WITH_BLUEZ_INTERNAL
SUBDIRS                                    += \
    bluez/repo                                \
    $(NULL)
endif # WEAVE_WITH_BLUEZ_INTERNAL
include $(abs_top_nlbuild_autotools_dir)/automake/post.am
