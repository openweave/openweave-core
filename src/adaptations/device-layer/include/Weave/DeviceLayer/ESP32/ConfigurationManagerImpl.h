/*
 *
 *    Copyright (c) 2018 Nest Labs, Inc.
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

/**
 *    @file
 *          Provides an implementation of the ConfigurationManager object
 *          for the ESP32 platform.
 */

#ifndef CONFIGURATION_MANAGER_IMPL_H
#define CONFIGURATION_MANAGER_IMPL_H

#include <Weave/DeviceLayer/internal/GenericConfigurationManagerImpl.h>
#include <Weave/DeviceLayer/ESP32/ESP32Config.h>

namespace nl {
namespace Weave {
namespace DeviceLayer {

namespace Internal {
class NetworkProvisioningServerImpl;
}

// Instruct the compiler to instantiate the GenericConfigurationManagerImpl<> template
// only when explicitly instructed to do so.
extern template class Internal::GenericConfigurationManagerImpl<ConfigurationManagerImpl>;


/**
 * Concrete implementation of the ConfigurationManager interface for the ESP32.
 */
class ConfigurationManagerImpl
    : public ConfigurationManager,
      public Internal::GenericConfigurationManagerImpl<ConfigurationManagerImpl>,
      private Internal::ESP32Config
{
    // Allow the ConfigurationManager interface class to delegate method calls to
    // the implementation methods provided by this class.
    friend class ConfigurationManager;

    // Allow the GenericConfigurationManagerImpl base class to access helper methods and types
    // defined on this class.
    friend class Internal::GenericConfigurationManagerImpl<ConfigurationManagerImpl>;

public:

    // ===== Implementation-specific members that may be accessed directly by the application.

    static ConfigurationManagerImpl & Instance();

private:

    // ===== Methods that implement the ConfigurationManager public interface.

    WEAVE_ERROR _Init();
    WEAVE_ERROR _GetPrimaryWiFiMACAddress(uint8_t * buf);
    WEAVE_ERROR _GetDeviceDescriptor(::nl::Weave::Profiles::DeviceDescription::WeaveDeviceDescriptor & deviceDesc);
    ::nl::Weave::Profiles::Security::AppKeys::GroupKeyStoreBase * _GetGroupKeyStore();
    bool _CanFactoryReset();
    void _InitiateFactoryReset();
    WEAVE_ERROR _ReadPersistedStorageValue(::nl::Weave::Platform::PersistedStorage::Key key, uint32_t & value);
    WEAVE_ERROR _WritePersistedStorageValue(::nl::Weave::Platform::PersistedStorage::Key key, uint32_t value);

    // NOTE: Other public interface methods are implemented by GenericConfigurationManagerImpl<>.

private:

    // ===== Members for internal use by the following friends.

    friend class Internal::NetworkProvisioningServerImpl;
    friend ConfigurationManager & ConfigurationMgr();

    static ConfigurationManagerImpl sInstance;

    WEAVE_ERROR GetWiFiStationSecurityType(::nl::Weave::Profiles::NetworkProvisioning::WiFiSecurityType & secType);
    WEAVE_ERROR UpdateWiFiStationSecurityType(::nl::Weave::Profiles::NetworkProvisioning::WiFiSecurityType secType);

private:

    // ===== Private members reserved for use by this class only.

    static void DoFactoryReset(intptr_t arg);
};

/**
 * Returns a reference to the singleton object that implements the ConnectionManager interface.
 *
 * API users can use this to gain access to features of the ConnectionManager that are specific
 * to the ESP32 implementation.
 */
inline ConfigurationManagerImpl & ConfigurationManagerImpl::Instance()
{
    return sInstance;
}

inline ConfigurationManager & ConfigurationMgr()
{
    return ConfigurationManagerImpl::sInstance;
}

} // namespace DeviceLayer
} // namespace Weave
} // namespace nl

#endif // CONFIGURATION_MANAGER_IMPL_H
