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

#ifndef CONFIGURATION_MANAGER_H
#define CONFIGURATION_MANAGER_H

#include <Weave/Profiles/device-description/DeviceDescription.h>

namespace nl {
namespace Weave {
namespace Device {

class TraitManager;
namespace Internal
{
class DeviceControlServer;
}

class ConfigurationManager
{

public:
    enum
    {
        kMaxPairingCodeLength = 15,
        kMaxSerialNumberLength = ::nl::Weave::Profiles::DeviceDescription::WeaveDeviceDescriptor::kMaxSerialNumberLength,
        kMaxFirmwareRevisionLength = ::nl::Weave::Profiles::DeviceDescription::WeaveDeviceDescriptor::kMaxSoftwareVersionLength,
    };

    WEAVE_ERROR GetVendorId(uint16_t & vendorId);
    WEAVE_ERROR GetProductId(uint16_t & productId);
    WEAVE_ERROR GetProductRevision(uint16_t & productRev);
    WEAVE_ERROR GetSerialNumber(char * buf, size_t bufSize, size_t & serialNumLen);
    WEAVE_ERROR GetManufacturingDate(uint16_t & year, uint8_t & month, uint8_t & dayOfMonth);
    WEAVE_ERROR GetFirmwareRevision(char * buf, size_t bufSize, size_t & outLen);
    WEAVE_ERROR GetFirmwareBuildTime(uint16_t & year, uint8_t & month, uint8_t & dayOfMonth,
            uint8_t & hour, uint8_t & minute, uint8_t & second);
    WEAVE_ERROR GetDeviceCertificate(uint8_t * buf, size_t bufSize, size_t & certLen);
    WEAVE_ERROR GetDeviceCertificateLength(size_t & certLen);
    WEAVE_ERROR GetDevicePrivateKey(uint8_t * buf, size_t bufSize, size_t & keyLen);
    WEAVE_ERROR GetDevicePrivateKeyLength(size_t & keyLen);
    WEAVE_ERROR GetServiceId(uint64_t & serviceId);
    WEAVE_ERROR GetServiceConfig(uint8_t * buf, size_t bufSize, size_t & serviceConfigLen);
    WEAVE_ERROR GetServiceConfigLength(size_t & serviceConfigLen);
    WEAVE_ERROR GetPairedAccountId(char * buf, size_t bufSize, size_t accountIdLen);

    WEAVE_ERROR StoreDeviceId(uint64_t deviceId);
    WEAVE_ERROR StoreSerialNumber(const char * serialNum);
    WEAVE_ERROR StoreManufacturingDate(const char * mfgDate);
    WEAVE_ERROR StoreFabricId(uint64_t fabricId);
    WEAVE_ERROR StoreDeviceCertificate(const uint8_t * cert, size_t certLen);
    WEAVE_ERROR StoreDevicePrivateKey(const uint8_t * key, size_t keyLen);
    WEAVE_ERROR StorePairingCode(const char * pairingCode);
    WEAVE_ERROR StoreServiceProvisioningData(uint64_t serviceId, const uint8_t * serviceConfig, size_t serviceConfigLen, const char * accountId, size_t accountIdLen);
    WEAVE_ERROR ClearServiceProvisioningData();
    WEAVE_ERROR StoreServiceConfig(const uint8_t * serviceConfig, size_t serviceConfigLen);
    WEAVE_ERROR StoreAccountId(const char * accountId, size_t accountIdLen);

    WEAVE_ERROR GetPersistedCounter(const char * key, uint32_t & value);
    WEAVE_ERROR StorePersistedCounter(const char * key, uint32_t value);

    WEAVE_ERROR GetDeviceDescriptor(::nl::Weave::Profiles::DeviceDescription::WeaveDeviceDescriptor & deviceDesc);
    WEAVE_ERROR GetDeviceDescriptorTLV(uint8_t * buf, size_t bufSize, size_t & encodedLen);
    WEAVE_ERROR GetQRCodeString(char * buf, size_t bufSize);

    WEAVE_ERROR GetWiFiAPSSID(char * buf, size_t bufSize);

    bool IsServiceProvisioned();
    bool IsPairedToAccount();
    bool IsMemberOfFabric();

    void InitiateFactoryReset();

private:

    // NOTE: These members are for internal use by the following friends.

    friend class ::nl::Weave::Device::PlatformManager;
    friend class ::nl::Weave::Device::Internal::DeviceControlServer;
    friend class ::nl::Weave::Device::TraitManager;

    WEAVE_ERROR Init();
    WEAVE_ERROR ConfigureWeaveStack();
    ::nl::Weave::Profiles::Security::AppKeys::GroupKeyStoreBase * GetGroupKeyStore();
    bool CanFactoryReset();
    WEAVE_ERROR SetFailSafeArmed();
    WEAVE_ERROR ClearFailSafeArmed();

private:

    // NOTE: These members are private to the class and should not be used by friends.

    enum
    {
        kFlag_IsServiceProvisioned      = 0x01,
        kFlag_IsPairedToAccount         = 0x02,
    };

    uint8_t mFlags;
    char mPairingCode[kMaxPairingCodeLength + 1];

    static void DoFactoryReset(intptr_t arg);
};

inline bool ConfigurationManager::IsServiceProvisioned()
{
    return ::nl::GetFlag(mFlags, kFlag_IsServiceProvisioned);
}

inline bool ConfigurationManager::IsPairedToAccount()
{
    return ::nl::GetFlag(mFlags, kFlag_IsPairedToAccount);
}

} // namespace Device
} // namespace Weave
} // namespace nl

#endif // CONFIGURATION_MANAGER_H
