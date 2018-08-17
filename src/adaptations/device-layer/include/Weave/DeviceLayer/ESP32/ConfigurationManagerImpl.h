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
 *          Provides the ESP32 implementation of the Device Layer ConfigurationManager object.
 */

#ifndef CONFIGURATION_MANAGER_IMPL_H
#define CONFIGURATION_MANAGER_IMPL_H

#include <Weave/DeviceLayer/internal/GenericConfigurationManagerImpl.h>

namespace nl {
namespace Weave {
namespace DeviceLayer {

extern template class Internal::GenericIdentityConfigurationImpl<ConfigurationManagerImpl<ESP32>, Internal::NVSKeys>;

/**
 * Concrete implementation of the ConfigurationManager interface for the ESP32.
 */
template<>
class ConfigurationManagerImpl<ESP32>
    : public ConfigurationManager,
      public Internal::GenericProductConfigurationImpl,
      public Internal::GenericIdentityConfigurationImpl<ConfigurationManagerImpl<ESP32>, Internal::NVSKeys>
{
    // Allow the ConfigurationManager interface class to delegate method calls to
    // the implementation methods defined below.

    friend class ConfigurationManager;

    // Allow the generic implementation bases classes to access helper methods on this class.

    friend class Internal::GenericIdentityConfigurationImpl<ConfigurationManagerImpl<ESP32>, Internal::NVSKeys>;

public:

    // Public implementation-specific members that may be accessed directly by the application.

    static ConfigurationManagerImpl<ESP32> & Instance();

private:

    // Implementations for public members of the ConfigurationManager interface.

//    WEAVE_ERROR _GetSerialNumber(char * buf, size_t bufSize, size_t & serialNumLen);
//    WEAVE_ERROR _GetManufacturingDate(uint16_t & year, uint8_t & month, uint8_t & dayOfMonth);
    WEAVE_ERROR _GetDeviceCertificate(uint8_t * buf, size_t bufSize, size_t & certLen);
    WEAVE_ERROR _GetDeviceCertificateLength(size_t & certLen);
    WEAVE_ERROR _GetDevicePrivateKey(uint8_t * buf, size_t bufSize, size_t & keyLen);
    WEAVE_ERROR _GetDevicePrivateKeyLength(size_t & keyLen);
    WEAVE_ERROR _GetServiceId(uint64_t & serviceId);
    WEAVE_ERROR _GetServiceConfig(uint8_t * buf, size_t bufSize, size_t & serviceConfigLen);
    WEAVE_ERROR _GetServiceConfigLength(size_t & serviceConfigLen);
    WEAVE_ERROR _GetPairedAccountId(char * buf, size_t bufSize, size_t & accountIdLen);
//    WEAVE_ERROR _StoreDeviceId(uint64_t deviceId);
//    WEAVE_ERROR _StoreSerialNumber(const char * serialNum);
//    WEAVE_ERROR _StoreManufacturingDate(const char * mfgDate);
    WEAVE_ERROR _StoreFabricId(uint64_t fabricId);
    WEAVE_ERROR _StoreDeviceCertificate(const uint8_t * cert, size_t certLen);
    WEAVE_ERROR _StoreDevicePrivateKey(const uint8_t * key, size_t keyLen);
    WEAVE_ERROR _StorePairingCode(const char * pairingCode);
    WEAVE_ERROR _StoreServiceProvisioningData(uint64_t serviceId, const uint8_t * serviceConfig,
            size_t serviceConfigLen, const char * accountId, size_t accountIdLen);
    WEAVE_ERROR _ClearServiceProvisioningData();
    WEAVE_ERROR _StoreServiceConfig(const uint8_t * serviceConfig, size_t serviceConfigLen);
    WEAVE_ERROR _StoreAccountId(const char * accountId, size_t accountIdLen);
    WEAVE_ERROR _GetDeviceDescriptor(::nl::Weave::Profiles::DeviceDescription::WeaveDeviceDescriptor & deviceDesc);
    WEAVE_ERROR _GetDeviceDescriptorTLV(uint8_t * buf, size_t bufSize, size_t & encodedLen);
    WEAVE_ERROR _GetQRCodeString(char * buf, size_t bufSize);
    WEAVE_ERROR _GetWiFiAPSSID(char * buf, size_t bufSize);
    bool _IsServiceProvisioned();
    bool _IsPairedToAccount();
    bool _IsMemberOfFabric();
    void _InitiateFactoryReset();

    // Implementations for "internal use" members of the ConfigurationManager interface.

    WEAVE_ERROR _Init();
    WEAVE_ERROR _ConfigureWeaveStack();
    ::nl::Weave::Profiles::Security::AppKeys::GroupKeyStoreBase * _GetGroupKeyStore();
    bool _CanFactoryReset();
    WEAVE_ERROR _SetFailSafeArmed();
    WEAVE_ERROR _ClearFailSafeArmed();
    WEAVE_ERROR _GetWiFiStationSecurityType(::nl::Weave::Profiles::NetworkProvisioning::WiFiSecurityType & secType);
    WEAVE_ERROR _UpdateWiFiStationSecurityType(::nl::Weave::Profiles::NetworkProvisioning::WiFiSecurityType secType);
    WEAVE_ERROR _ReadPersistedStorageValue(::nl::Weave::Platform::PersistedStorage::Key key, uint32_t & value);
    WEAVE_ERROR _WritePersistedStorageValue(::nl::Weave::Platform::PersistedStorage::Key key, uint32_t value);

    // Methods called by the generic implementation base classes (e.g. GenericIdentityConfigurationImpl) to
    // read and write dynamic configuration values to persistent storage.

    WEAVE_ERROR ReadConfigValue(Internal::NVSKey key, char * buf, size_t bufSize, size_t & outLen);
    WEAVE_ERROR ReadConfigValue(Internal::NVSKey key, uint32_t & val);
    WEAVE_ERROR ReadConfigValue(Internal::NVSKey key, uint64_t & val);
    WEAVE_ERROR ReadConfigValueBin(Internal::NVSKey key, uint8_t * buf, size_t bufSize, size_t & outLen);
    WEAVE_ERROR WriteConfigValue(Internal::NVSKey key, const char * str);
    WEAVE_ERROR WriteConfigValue(Internal::NVSKey key, const char * str, size_t strLen);
    WEAVE_ERROR WriteConfigValue(Internal::NVSKey key, uint32_t val);
    WEAVE_ERROR WriteConfigValue(Internal::NVSKey key, uint64_t val);
    WEAVE_ERROR WriteConfigValueBin(Internal::NVSKey key, const uint8_t * data, size_t dataLen);
    WEAVE_ERROR ClearConfigValue(Internal::NVSKey key);

    // Private members reserved for use by this class only.

    enum
    {
        kFlag_IsServiceProvisioned      = 0x01,
        kFlag_IsPairedToAccount         = 0x02,
    };

    uint8_t mFlags;
    char mPairingCode[kMaxPairingCodeLength + 1];

    static ConfigurationManagerImpl sInstance;

    void LogDeviceConfig();

    static void DoFactoryReset(intptr_t arg);
};

/**
 * Returns a reference to the singleton object that implements the ConnectionManager interface.
 *
 * API users can use this to gain access to features of the ConnectionManager that are specific
 * to the ESP32 implementation.
 */
inline ConfigurationManagerImpl<ESP32> & ConfigurationManagerImpl<ESP32>::Instance()
{
    return sInstance;
}

inline bool ConfigurationManagerImpl<ESP32>::_IsServiceProvisioned()
{
    return ::nl::GetFlag(mFlags, kFlag_IsServiceProvisioned);
}

inline bool ConfigurationManagerImpl<ESP32>::_IsPairedToAccount()
{
    return ::nl::GetFlag(mFlags, kFlag_IsPairedToAccount);
}

inline WEAVE_ERROR ConfigurationManagerImpl<ESP32>::ReadConfigValue(Internal::NVSKey key, uint8_t * buf, size_t bufSize, size_t & outLen)
{
    return Internal::GetNVS(key, buf, bufSize, outLen);
}

inline WEAVE_ERROR ConfigurationManagerImpl<ESP32>::ReadConfigValue(Internal::NVSKey key, char * buf, size_t bufSize, size_t & outLen)
{
    return Internal::GetNVS(key, buf, bufSize, outLen);
}

inline WEAVE_ERROR ConfigurationManagerImpl<ESP32>::ReadConfigValue(Internal::NVSKey key, uint32_t & val)
{
    return Internal::GetNVS(key, val);
}

inline WEAVE_ERROR ConfigurationManagerImpl<ESP32>::ReadConfigValue(Internal::NVSKey key, uint64_t & val)
{
    return Internal::GetNVS(key, val);
}

inline WEAVE_ERROR ConfigurationManagerImpl<ESP32>::WriteConfigValue(Internal::NVSKey key, const uint8_t * data, size_t dataLen)
{
    return Internal::StoreNVS(key, data, dataLen);
}

inline WEAVE_ERROR ConfigurationManagerImpl<ESP32>::WriteConfigValue(Internal::NVSKey key, const char * data)
{
    return Internal::StoreNVS(key, data);
}

inline WEAVE_ERROR ConfigurationManagerImpl<ESP32>::WriteConfigValue(Internal::NVSKey key, uint32_t val)
{
    return Internal::StoreNVS(key, val);
}

inline WEAVE_ERROR ConfigurationManagerImpl<ESP32>::WriteConfigValue(Internal::NVSKey key, uint64_t val)
{
    return Internal::StoreNVS(key, val);
}

} // namespace DeviceLayer
} // namespace Weave
} // namespace nl

#endif // CONFIGURATION_MANAGER_IMPL_H
