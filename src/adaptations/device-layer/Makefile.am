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
#      This file is the GNU automake template for the OpenWeave Device Layer
#      library.

include $(abs_top_nlbuild_autotools_dir)/automake/pre.am

#
# Local headers to build against and distribute but not to install
# since they are not part of the Weave SDK public API.
#
noinst_HEADERS                        = \
    include/Weave/DeviceLayer/ConfigurationManager.h \
    include/Weave/DeviceLayer/ConnectivityManager.h \
    include/Weave/DeviceLayer/internal/BLEManager.h \
    include/Weave/DeviceLayer/internal/DeviceControlServer.h \
    include/Weave/DeviceLayer/internal/DeviceDescriptionServer.h \
    include/Weave/DeviceLayer/internal/DeviceIdentityTraitDataSource.h \
    include/Weave/DeviceLayer/internal/EchoServer.h \
    include/Weave/DeviceLayer/internal/ESPUtils.h \
    include/Weave/DeviceLayer/internal/FabricProvisioningServer.h \
    include/Weave/DeviceLayer/internal/NetworkInfo.h \
    include/Weave/DeviceLayer/internal/NetworkProvisioningServer.h \
    include/Weave/DeviceLayer/internal/ServiceDirectoryManager.h \
    include/Weave/DeviceLayer/internal/ServiceProvisioningServer.h \
    include/Weave/DeviceLayer/internal/ServiceTunnelAgent.h \
    include/Weave/DeviceLayer/internal/WeaveDeviceConfig-ESP32.h \
    include/Weave/DeviceLayer/internal/WeaveDeviceLayerInternal.h \
    include/Weave/DeviceLayer/PlatformManager.h \
    include/Weave/DeviceLayer/TimeSyncManager.h \
    include/Weave/DeviceLayer/TraitManager.h \
    include/Weave/DeviceLayer/WeaveDeviceConfig.h \
    include/Weave/DeviceLayer/WeaveDeviceError.h \
    include/Weave/DeviceLayer/WeaveDeviceEvent.h \
    include/Weave/DeviceLayer/WeaveDeviceLayer.h \
    trait-support/weave/trait/description/DeviceIdentityTrait.h \
    $(NULL)

EXTRA_DIST                            = \
    GenTestDeviceIds.py                 \
    $(NULL)

if CONFIG_DEVICE_LAYER

lib_LIBRARIES                   		= libDeviceLayer.a

libDeviceLayer_a_CPPFLAGS        = \
    -I$(top_srcdir)/src/include    \
    $(LWIP_CPPFLAGS)               \
    $(SOCKETS_CPPFLAGS)            \
    $(NULL)

libDeviceLayer_a_SOURCES         = \
    AESBlockCipher.cpp                  \
    BLEManager.cpp                      \
    CASEAuth.cpp                        \
    ConfigurationManager.cpp            \
    ConnectivityManager.cpp             \
    DeviceControlServer.cpp             \
    DeviceDescriptionServer.cpp         \
    DeviceIdentityTraitDataSource.cpp   \
    EchoServer.cpp                      \
    Entropy.cpp                         \
    ESPUtils.cpp                        \
    FabricProvisioningServer.cpp        \
    Globals.cpp                         \
    Logging.cpp                         \
    NetworkInfo.cpp                     \
    NetworkProvisioningServer.cpp       \
    PlatformManager.cpp                 \
    ServiceDirectoryManager.cpp         \
    ServiceProvisioningServer.cpp       \
    ServiceTunnelAgent.cpp              \
    TestDeviceIds.cpp                   \
    Time.cpp                            \
    TimeSyncManager.cpp                 \
    TraitManager.cpp                    \
    Warm.cpp                            \
    trait-support/weave/trait/description/DeviceIdentityTrait.cpp \
    $(NULL)

endif # CONFIG_DEVICE_LAYER

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
