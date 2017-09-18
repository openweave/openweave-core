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
#      This file is the GNU automake template for the Nest Weave
#      Device Manager library.
#

include $(abs_top_nlbuild_autotools_dir)/automake/pre.am

EXTRA_DIST                                = \
    WeaveDeviceManager.h                    \
    AndroidBleApplicationDelegate.h         \
    AndroidBlePlatformDelegate.h            \
    $(NULL)

lib_LTLIBRARIES                           = libWeaveDeviceManager.la

libWeaveDeviceManager_la_CPPFLAGS         = \
    -I$(top_srcdir)/src/include             \
    $(LWIP_CPPFLAGS)                        \
    $(SOCKETS_CPPFLAGS)                     \
    $(NULL)

libWeaveDeviceManager_la_CXXFLAGS         = \
    $(PTHREAD_CFLAGS)                       \
    $(NULL)

libWeaveDeviceManager_la_LDFLAGS          = \
    $(AM_LDFLAGS)                           \
    -version-info $(LIBWEAVE_VERSION_INFO)  \
    $(NULL)

libWeaveDeviceManager_la_SOURCES          = \
    WeaveDeviceManager.cpp                  \
    $(NULL)

if WEAVE_BUILD_DEVICE_MANAGER_WITH_DUMMY_PERSISTED_STORE
libWeaveDeviceManager_la_SOURCES         += \
    WeavePlatformPersistedStorage.cpp       \
    $(NULL)
endif # WEAVE_BUILD_DEVICE_MANAGER_WITH_DUMMY_PERSISTED_STORE

if CONFIG_NETWORK_LAYER_BLE
if WEAVE_ENABLE_WOBLE_TEST
libWeaveDeviceManager_la_CPPFLAGS        += \
    -I$(top_srcdir)/src/ble                 \
    $(NULL)
libWeaveDeviceManager_la_SOURCES         += \
    WoBleTest.cpp                           \
    $(NULL)
endif # WEAVE_ENABLE_WOBLE_TEST
endif # CONFIG_NETWORK_LAYER_BLE

libWeaveDeviceManager_la_LIBADD           = \
    -L$(top_builddir)/src/lib -lWeave       \
    $(LWIP_LDFLAGS) $(LWIP_LIBS)            \
    $(SOCKETS_LDFLAGS) $(SOCKETS_LIBS)      \
    $(PTHREAD_LIBS)                         \
    $(NULL)

libWeaveDeviceManager_la_DEPENDENCIES     = \
    $(top_builddir)/src/lib/libWeave.a      \
    $(NULL)

if WEAVE_WITH_ANDROID
if WEAVE_LOGGING_STYLE_ANDROID

#
# The Weave SDK supports either C Standard I/O, Android, or external
# (platform-defined) logging. If we're building with Android, include
# the Android NDK logging library in the Weave Device Manager library.
#

libWeaveDeviceManager_la_LIBADD          += \
    -llog                                   \
    $(NULL)

endif # WEAVE_WITH_ANDROID
endif # WEAVE_LOGGING_STYLE_ANDROID

if WEAVE_WITH_JAVA

# For some unknown reason, including JNI_CPPFLAGS breaks Android.

if !WEAVE_WITH_ANDROID

libWeaveDeviceManager_la_CPPFLAGS        += \
	$(JNI_CPPFLAGS)                     \
	$(NULL)

endif # !WEAVE_WITH_ANDROID

libWeaveDeviceManager_la_SOURCES         += \
    WeaveDeviceManager-JNI.cpp              \
    PairingCodeUtils-JNI.cpp                \
    $(NULL)

if CONFIG_NETWORK_LAYER_BLE

libWeaveDeviceManager_la_SOURCES         += \
    AndroidBleApplicationDelegate.cpp       \
    AndroidBlePlatformDelegate.cpp          \
    $(NULL)

endif # CONFIG_NETWORK_LAYER_BLE

# TODO: is this needed???
libWeaveDeviceManager_la_LDFLAGS          += \
    -u _Init                                \
    -u _Java_nl_Weave_DeviceManager_WeaveDeviceManager_newDeviceManager \
    -no-undefined                           \
    $(NULL)

endif # WEAVE_WITH_JAVA

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
