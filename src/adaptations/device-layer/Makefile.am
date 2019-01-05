#
#    Copyright (c) 2014-2018 Nest Labs, Inc.
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

#
#    Description:
#      This file is the GNU automake template for the OpenWeave Device Layer
#      library.

include $(abs_top_nlbuild_autotools_dir)/automake/pre.am

EXTRA_DIST                              = \
    ADAPTATION-PATTERNS.md                \
    DESIGN-NOTES.md                       \
    GenTestDeviceIds.py                   \
    $(NULL)

#
# Local headers to build against and distribute but not to install
# since they are not part of the Weave SDK public API.
#
noinst_HEADERS                          = \
    include/Weave/DeviceLayer/ConfigurationManager.h \
    include/Weave/DeviceLayer/ConnectivityManager.h \
    include/Weave/DeviceLayer/ESP32/WeaveDevicePlatformConfig.h \
    include/Weave/DeviceLayer/ESP32/ESP32Utils.h \
    include/Weave/DeviceLayer/ESP32/GroupKeyStoreImpl.h \
    include/Weave/DeviceLayer/ESP32/ESP32Config.h \
    include/Weave/DeviceLayer/ESP32/ConnectivityManagerImpl.h \
    include/Weave/DeviceLayer/ESP32/BLEManagerImpl.h \
    include/Weave/DeviceLayer/ESP32/ConfigurationManagerImpl.h \
    include/Weave/DeviceLayer/ESP32/PlatformManagerImpl.h \
    include/Weave/DeviceLayer/ESP32/WeaveDevicePlatformEvent.h \
    include/Weave/DeviceLayer/ESP32/NetworkProvisioningServerImpl.h \
    include/Weave/DeviceLayer/nRF5/ConfigurationManagerImpl.h \
    include/Weave/DeviceLayer/nRF5/PlatformManagerImpl.h \
    include/Weave/DeviceLayer/nRF5/NRF5Config.h \
    include/Weave/DeviceLayer/nRF5/WeaveDevicePlatformConfig.h \
    include/Weave/DeviceLayer/nRF5/WeaveDevicePlatformEvent.h \
    include/Weave/DeviceLayer/FreeRTOS/GenericPlatformManagerImpl_FreeRTOS.h \
    include/Weave/DeviceLayer/FreeRTOS/GenericPlatformManagerImpl_FreeRTOS.ipp \
    include/Weave/DeviceLayer/GeneralUtils.h \
    include/Weave/DeviceLayer/PlatformManager.h \
    include/Weave/DeviceLayer/TimeSyncManager.h \
    include/Weave/DeviceLayer/TraitManager.h \
    include/Weave/DeviceLayer/WeaveDeviceConfig.h \
    include/Weave/DeviceLayer/WeaveDeviceError.h \
    include/Weave/DeviceLayer/WeaveDeviceEvent.h \
    include/Weave/DeviceLayer/WeaveDeviceLayer.h \
    include/Weave/DeviceLayer/internal/BLEManager.h \
    include/Weave/DeviceLayer/internal/DeviceControlServer.h \
    include/Weave/DeviceLayer/internal/DeviceDescriptionServer.h \
    include/Weave/DeviceLayer/internal/DeviceIdentityTraitDataSource.h \
    include/Weave/DeviceLayer/internal/EchoServer.h \
    include/Weave/DeviceLayer/internal/FabricProvisioningServer.h \
    include/Weave/DeviceLayer/internal/GenericConfigurationManagerImpl.h \
    include/Weave/DeviceLayer/internal/GenericConfigurationManagerImpl.ipp \
    include/Weave/DeviceLayer/internal/GenericNetworkProvisioningServerImpl.h \
    include/Weave/DeviceLayer/internal/GenericNetworkProvisioningServerImpl.ipp \
    include/Weave/DeviceLayer/internal/GenericPlatformManagerImpl.h \
    include/Weave/DeviceLayer/internal/GenericPlatformManagerImpl.ipp \
    include/Weave/DeviceLayer/internal/NetworkInfo.h \
    include/Weave/DeviceLayer/internal/NetworkProvisioningServer.h \
    include/Weave/DeviceLayer/internal/ServiceDirectoryManager.h \
    include/Weave/DeviceLayer/internal/ServiceProvisioningServer.h \
    include/Weave/DeviceLayer/internal/ServiceTunnelAgent.h \
    include/Weave/DeviceLayer/internal/WeaveDeviceLayerInternal.h \
    trait-support/weave/trait/description/DeviceIdentityTrait.h \
    $(NULL)

if CONFIG_DEVICE_LAYER

lib_LIBRARIES                           = libDeviceLayer.a

libDeviceLayer_a_CPPFLAGS               = \
    -I$(top_srcdir)/src/include           \
    $(LWIP_CPPFLAGS)                      \
    $(SOCKETS_CPPFLAGS)                   \
    $(NULL)

libDeviceLayer_a_SOURCES                = \
    CASEAuth.cpp                          \
    DeviceControlServer.cpp               \
    DeviceDescriptionServer.cpp           \
    DeviceIdentityTraitDataSource.cpp     \
    EchoServer.cpp                        \
    FabricProvisioningServer.cpp          \
    GeneralUtils.cpp                      \
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
    trait-support/weave/trait/description/DeviceIdentityTrait.cpp \
    $(NULL)

if WEAVE_DEVICE_LAYER_TARGET_ESP32

libDeviceLayer_a_SOURCES               +=       \
    ESP32/AESBlockCipher.cpp                    \
    ESP32/BLEManagerImpl.cpp                    \
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
    LwIP/WarmSupport.cpp                        \
    $(NULL)

endif # WEAVE_DEVICE_LAYER_TARGET_ESP32

if WEAVE_DEVICE_LAYER_TARGET_NRF5

libDeviceLayer_a_SOURCES               +=       \
    nRF5/AESBlockCipher.cpp                     \
    nRF5/BLEManagerImpl.cpp                     \
    nRF5/ConfigurationManagerImpl.cpp           \
    nRF5/PlatformManagerImpl.cpp                \
    nRF5/ConnectivityManagerImpl.cpp            \
    nRF5/NetworkProvisioningServerImpl.cpp      \
    nRF5/GroupKeyStoreImpl.cpp                  \
    nRF5/NRF5Config.cpp                         \
    nRF5/Entropy.cpp                            \
    nRF5/Logging.cpp                            \
    FreeRTOS/SystemTimeSupport.cpp              \
    $(NULL)

endif # WEAVE_DEVICE_LAYER_TARGET_NRF5

endif # CONFIG_DEVICE_LAYER

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
