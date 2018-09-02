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

if CONFIG_DEVICE_LAYER

lib_LIBRARIES                           = libDeviceLayer.a

libDeviceLayer_a_CPPFLAGS               = \
    -I$(top_srcdir)/src/include           \
    $(LWIP_CPPFLAGS)                      \
    $(SOCKETS_CPPFLAGS)                   \
    $(NULL)

libDeviceLayer_a_SOURCES                = \
    BLEManager.cpp                        \
    CASEAuth.cpp                          \
    DeviceControlServer.cpp               \
    DeviceDescriptionServer.cpp           \
    DeviceIdentityTraitDataSource.cpp     \
    EchoServer.cpp                        \
    FabricProvisioningServer.cpp          \
    Globals.cpp                           \
    NetworkInfo.cpp                       \
    PersistedStorage.cpp                  \
    ServiceDirectoryManager.cpp           \
    ServiceProvisioningServer.cpp         \
    ServiceTunnelAgent.cpp                \
    SystemEventSupport.cpp                \
    SystemTimerSupport.cpp                \
    TestDeviceIds.cpp                     \
    TimeSyncManager.cpp                   \
    TraitManager.cpp                      \
    Warm.cpp                              \
    trait-support/weave/trait/description/DeviceIdentityTrait.cpp \
    $(NULL)

if WEAVE_DEVICE_LAYER_TARGET_ESP32

libDeviceLayer_a_SOURCES               +=       \
    ESP32/AESBlockCipher.cpp                    \
    ESP32/ConfigurationManagerImpl.cpp          \
    ESP32/ConnectivityManagerImpl.cpp           \
    ESP32/Entropy.cpp                           \
    ESP32/ESP32Config.cpp                       \
    ESP32/ESP32Utils.cpp                        \
    ESP32/GroupKeyStoreImpl.cpp                 \
    ESP32/Logging.cpp                           \
    ESP32/LwIPCoreLock.cpp                      \
    ESP32/NetworkProvisioningServerImpl.cpp     \
    ESP32/PlatformManagerImpl.cpp               \
    ESP32/SystemTimeSupport.cpp                 \
    $(NULL)

endif # WEAVE_DEVICE_LAYER_TARGET_ESP32

EXTRA_DIST                              = \
    $(NULL)

endif # CONFIG_DEVICE_LAYER

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
