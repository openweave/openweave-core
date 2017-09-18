#
#    Copyright (c) 2014-2017 Nest Labs, Inc.
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
#      This file is the GNU automake template for the Nest Weave SDK
#      source code.
#

include $(abs_top_nlbuild_autotools_dir)/automake/pre.am

EXTRA_DIST                        = \
    tools/simnet                    \
    $(NULL)

DEVICE_MANAGER_SUBDIRS            = \
    device-manager                  \
    device-manager/python           \
    device-manager/cocoa            \
    device-manager/java             \
    $(NULL)

MAYBE_DEVICE_MANAGER_SUBDIRS      = \
    $(NULL)

if WEAVE_BUILD_DEVICE_MANAGER
MAYBE_DEVICE_MANAGER_SUBDIRS     += \
    $(DEVICE_MANAGER_SUBDIRS)       \
    $(NULL)
endif # WEAVE_BUILD_DEVICE_MANAGER

WARM_SUBDIRS                      = \
    warm                            \
    $(NULL)

MAYBE_WARM_SUBDIRS                = \
    $(NULL)

if WEAVE_BUILD_WARM
MAYBE_WARM_SUBDIRS               += \
    $(WARM_SUBDIRS)                 \
    $(NULL)
endif # WEAVE_BUILD_WARM

BLE_SUBDIRS                       = \
    ble                             \
    $(NULL)

BLUEZ_SUBDIRS                     = \
    platform/ble/bluez              \
    $(NULL)

MAYBE_BLE_SUBDIRS                 = \
    $(NULL)

if CONFIG_NETWORK_LAYER_BLE
MAYBE_BLE_SUBDIRS                += \
    $(BLE_SUBDIRS)                  \
    $(NULL)
endif # CONFIG_NETWORK_LAYER_BLE

if CONFIG_BLE_PLATFORM_BLUEZ
MAYBE_BLE_SUBDIRS                += \
    $(BLUEZ_SUBDIRS)                \
    $(NULL)
endif # CONFIG_BLE_PLATFORM_BLUEZ

EXAMPLES_SUBDIR                   = \
    examples                        \
    $(NULL)

# Always package (e.g. for 'make dist') these subdirectories.

DIST_SUBDIRS                      = \
    lib/support                     \
    include                         \
    $(BLE_SUBDIRS)                  \
    $(BLUEZ_SUBDIRS)                \
    lwip                            \
    system                          \
    inet                            \
    lib                             \
    lib/support/verhoeff            \
    $(DEVICE_MANAGER_SUBDIRS)       \
    wrappers/jni                    \
    ra-daemon                       \
    test-apps                       \
    test-apps/wrapper-tests/jni     \
    tools/weave                     \
    tools/misc                      \
    wrappers/jni                    \
    $(EXAMPLES_SUBDIR)              \
    $(WARM_SUBDIRS)                 \
    $(NULL)

# Always build (e.g. for 'make all') these subdirectories.

SUBDIRS                           = \
    lib/support                     \
    include                         \
    $(MAYBE_WARM_SUBDIRS)           \
    $(MAYBE_BLE_SUBDIRS)            \
    lwip                            \
    system                          \
    inet                            \
    lib                             \
    lib/support/verhoeff            \
    $(MAYBE_DEVICE_MANAGER_SUBDIRS) \
    wrappers/jni                    \
    test-apps/wrapper-tests/jni     \
    ra-daemon                       \
    test-apps                       \
    tools/weave                     \
    tools/misc                      \
    $(EXAMPLES_SUBDIR)              \
    $(NULL)

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
