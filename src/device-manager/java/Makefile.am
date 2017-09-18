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

EXTRA_DIST                                         = \
    $(srcdir)/src                                    \
    $(srcdir)/stub_src                               \
    $(NULL)

if WEAVE_WITH_JAVA

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
