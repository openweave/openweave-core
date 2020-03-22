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
#      Java Device Manager library.
#

include $(abs_top_nlbuild_autotools_dir)/automake/pre.am

if WEAVE_WITH_JAVA

EXTRA_DIST                                         = \
    AndroidBleApplicationDelegate.h                  \
    AndroidBlePlatformDelegate.h                     \
    $(srcdir)/src                                    \
    $(srcdir)/stub_src                               \
    $(NULL)

lib_LTLIBRARIES                                    = libWeaveDeviceManager.la

libWeaveDeviceManager_la_CPPFLAGS              = \
    -I$(top_srcdir)/src/device-manager               \
    -I$(top_srcdir)/src/include                      \
	$(JNI_CPPFLAGS)                                  \
	$(LWIP_CPPFLAGS)                                 \
	$(SOCKETS_CPPFLAGS)                              \
	$(NULL)

libWeaveDeviceManager_la_CXXFLAGS              = \
    $(PTHREAD_CFLAGS)                                \
    $(NULL)

libWeaveDeviceManager_la_LDFLAGS               = \
    $(AM_LDFLAGS)                                    \
    -avoid-version                                   \
    -module                                          \
    -export-dynamic                                  \
    $(NULL)

libWeaveDeviceManager_la_SOURCES               = \
    WeaveDeviceManager-JNI.cpp                       \
    PairingCodeUtils-JNI.cpp                         \
    $(NULL)

if CONFIG_NETWORK_LAYER_BLE

libWeaveDeviceManager_la_SOURCES              += \
    AndroidBleApplicationDelegate.cpp                \
    AndroidBlePlatformDelegate.cpp                   \
    $(NULL)

endif # CONFIG_NETWORK_LAYER_BLE

libWeaveDeviceManager_la_LIBADD                = \
    -L$(top_builddir)/src/lib -lWeave                \
    $(LWIP_LDFLAGS) $(LWIP_LIBS)                     \
    $(SOCKETS_LDFLAGS) $(SOCKETS_LIBS)               \
    $(PTHREAD_LIBS)                                  \
    $(NULL)

if WEAVE_WITH_ANDROID
libWeaveDeviceManager_la_LIBADD                   += \
    -llog                                            \
    $(NULL)
endif # WEAVE_WITH_ANDROID

libWeaveDeviceManager_la_DEPENDENCIES              = \
    $(top_builddir)/src/lib/libWeave.a               \
    $(NULL)


JARS                                               = \
    WeaveDeviceManager.jar                           \
    $(NULL)

WeaveDeviceManager_jar_JAVA_SRCS                   =        \
    nl/Weave/DeviceManager/WiFiSecurityType.java            \
    nl/Weave/DeviceManager/WeaveDeviceManagerException.java \
    nl/Weave/DeviceManager/WiFiMode.java                    \
    nl/Weave/DeviceManager/TestMain.java                    \
    nl/Weave/DeviceManager/WiFiRole.java                    \
    nl/Weave/DeviceManager/RendezvousMode.java              \
    nl/Weave/DeviceManager/GetNetworkFlags.java             \
    nl/Weave/DeviceManager/NetworkInfo.java                 \
    nl/Weave/DeviceManager/NetworkType.java                 \
    nl/Weave/DeviceManager/WeaveDeviceManager.java          \
    nl/Weave/DeviceManager/WeaveDeviceException.java        \
    nl/Weave/DeviceManager/DeviceFeatures.java              \
    nl/Weave/DeviceManager/NevisPairingCodeDecoding.java    \
    nl/Weave/DeviceManager/WeaveDeviceManager.java          \
    nl/Weave/DataManagement/WdmClient.java                  \
    nl/Weave/DataManagement/WdmClientImpl.java              \
    nl/Weave/DataManagement/GenericTraitUpdatableDataSink.java \
    nl/Weave/DataManagement/GenericTraitUpdatableDataSinkImpl.java \
    nl/Weave/DataManagement/WdmClientFactory.java           \
    nl/Weave/DataManagement/ResourceIdentifier.java         \
    nl/Weave/DataManagement/WdmClientFlushUpdateException.java          \
    nl/Weave/DataManagement/WdmClientFlushUpdateDeviceException.java    \
    $(NULL)

WeaveDeviceManager_jar_JFLAGS                      = -source 6 -target 6

WeaveDeviceManager_jar_JAVA_MAIN_CLASS             = nl.Weave.DeviceManager.TestMain

if WEAVE_WITH_ANDROID
WeaveDeviceManager_jar_JAVA_CLASSPATHS             = $(ANDROID_HOME)/platforms/android-19/android.jar
else
WeaveDeviceManager_jar_JAVA_EXTRA_SRC_DIRS         = $(srcdir)/stub_src
endif # WEAVE_WITH_ANDROID

include $(abs_top_srcdir)/build/make/java.mk

all-local: all-JARS
install-data-local: install-data-JARS
uninstall-local: uninstall-JARS
mostlyclean-local: mostlyclean-local-JARS

endif # WEAVE_WITH_JAVA

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
