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
 *          Defines the public interface for the Device Layer ConfigurationManager object.
 */

#ifndef CONFIGURATION_MANAGER_H
#define CONFIGURATION_MANAGER_H

#include <Weave/Support/platform/PersistedStorage.h>
#include <Weave/Profiles/device-description/DeviceDescription.h>
#include <Weave/Profiles/network-provisioning/NetworkProvisioning.h>

namespace nl {
namespace Weave {
namespace DeviceLayer {

class PlatformManagerImpl;
class ConfigurationManagerImpl;
class TraitManager;
namespace Internal {
class DeviceControlServer;
class NetworkProvisioningServer;
}

/**
 * Provides access to runtime and build-time configuration information for a Weave device.
 */
class ConfigurationManager
{
    using WeaveDeviceDescriptor = ::nl::Weave::Profiles::DeviceDescription::WeaveDeviceDescriptor;

public:

    // ===== Members that define the public interface of the ConfigurationManager

    enum
    {
        kMaxPairingCodeLength = 15,
        kMaxSerialNumberLength = WeaveDeviceDescriptor::kMaxSerialNumberLength,
        kMaxFirmwareRevisionLength = WeaveDeviceDescriptor::kMaxSoftwareVersionLength,
    };

    WEAVE_ERROR GetVendorId(uint16_t & vendorId);
    WEAVE_ERROR GetProductId(uint16_t & productId);
    WEAVE_ERROR GetProductRevision(uint16_t & productRev);
    WEAVE_ERROR GetSerialNumber(char * buf, size_t bufSize, size_t & serialNumLen);
    WEAVE_ERROR GetPrimaryWiFiMACAddress(uint8_t * buf);
    WEAVE_ERROR GetPrimary802154MACAddress(uint8_t * buf);
    WEAVE_ERROR GetManufacturingDate(uint16_t & year, uint8_t & month, uint8_t & dayOfMonth);
    WEAVE_ERROR GetFirmwareRevision(char * buf, size_t bufSize, size_t & outLen);
    WEAVE_ERROR GetFirmwareBuildTime(uint16_t & year, uint8_t & month, uint8_t & dayOfMonth,
            uint8_t & hour, uint8_t & minute, uint8_t & second);
    WEAVE_ERROR GetDeviceId(uint64_t & deviceId);
    WEAVE_ERROR GetDeviceCertificate(uint8_t * buf, size_t bufSize, size_t & certLen);
    WEAVE_ERROR GetDevicePrivateKey(uint8_t * buf, size_t bufSize, size_t & keyLen);
    WEAVE_ERROR GetPairingCode(char * buf, size_t bufSize, size_t & pairingCodeLen);
    WEAVE_ERROR GetServiceId(uint64_t & serviceId);
    WEAVE_ERROR GetFabricId(uint64_t & fabricId);
    WEAVE_ERROR GetServiceConfig(uint8_t * buf, size_t bufSize, size_t & serviceConfigLen);
    WEAVE_ERROR GetPairedAccountId(char * buf, size_t bufSize, size_t & accountIdLen);

    WEAVE_ERROR StoreDeviceId(uint64_t deviceId);
    WEAVE_ERROR StoreSerialNumber(const char * serialNum);
    WEAVE_ERROR StorePrimaryWiFiMACAddress(const uint8_t * buf);
    WEAVE_ERROR StorePrimary802154MACAddress(const uint8_t * buf);
    WEAVE_ERROR StoreManufacturingDate(const char * mfgDate);
    WEAVE_ERROR StoreFabricId(uint64_t fabricId);
    WEAVE_ERROR StoreDeviceCertificate(const uint8_t * cert, size_t certLen);
    WEAVE_ERROR StoreDevicePrivateKey(const uint8_t * key, size_t keyLen);
    WEAVE_ERROR StorePairingCode(const char * pairingCode);
    WEAVE_ERROR StoreServiceProvisioningData(uint64_t serviceId, const uint8_t * serviceConfig, size_t serviceConfigLen, const char * accountId, size_t accountIdLen);
    WEAVE_ERROR ClearServiceProvisioningData();
    WEAVE_ERROR StoreServiceConfig(const uint8_t * serviceConfig, size_t serviceConfigLen);
    WEAVE_ERROR StorePairedAccountId(const char * accountId, size_t accountIdLen);

    WEAVE_ERROR GetDeviceDescriptor(WeaveDeviceDescriptor & deviceDesc);
    WEAVE_ERROR GetDeviceDescriptorTLV(uint8_t * buf, size_t bufSize, size_t & encodedLen);
    WEAVE_ERROR GetQRCodeString(char * buf, size_t bufSize);

    WEAVE_ERROR GetWiFiAPSSID(char * buf, size_t bufSize);

    bool IsServiceProvisioned();
    bool IsPairedToAccount();
    bool IsMemberOfFabric();

    void InitiateFactoryReset();

private:

    // ===== Members for internal use by the following friends.

    friend class ::nl::Weave::DeviceLayer::PlatformManagerImpl;
    friend class ::nl::Weave::DeviceLayer::TraitManager;
    friend class ::nl::Weave::DeviceLayer::Internal::DeviceControlServer;
    friend WEAVE_ERROR ::nl::Weave::Platform::PersistedStorage::Read(::nl::Weave::Platform::PersistedStorage::Key key, uint32_t & value);
    friend WEAVE_ERROR ::nl::Weave::Platform::PersistedStorage::Write(::nl::Weave::Platform::PersistedStorage::Key key, uint32_t value);

    using ImplClass = ConfigurationManagerImpl;

    WEAVE_ERROR Init();
    WEAVE_ERROR ConfigureWeaveStack();
    ::nl::Weave::Profiles::Security::AppKeys::GroupKeyStoreBase * GetGroupKeyStore(); // TODO: maybe remove this???
    bool CanFactoryReset();
    WEAVE_ERROR GetFailSafeArmed(bool & val);
    WEAVE_ERROR SetFailSafeArmed(bool val);
    WEAVE_ERROR ReadPersistedStorageValue(::nl::Weave::Platform::PersistedStorage::Key key, uint32_t & value);
    WEAVE_ERROR WritePersistedStorageValue(::nl::Weave::Platform::PersistedStorage::Key key, uint32_t value);

protected:

    // Access to construction/destruction is limited to subclasses.
    ConfigurationManager() = default;
    ~ConfigurationManager() = default;
};

/**
 * Returns a reference to the public interface of the ConfigurationManager singleton object.
 *
 * Weave application should use this to access features of the ConfigurationManager object
 * that are common to all platforms.
 */
extern ConfigurationManager & ConfigurationMgr(void);

/**
 * Returns the platform-specific implementation of the ConfigurationManager singleton object.
 *
 * Weave applications can use this to gain access to features of the ConfigurationManager
 * that are specific to the selected platform.
 */
extern ConfigurationManagerImpl & ConfigurationMgrImpl(void);

} // namespace DeviceLayer
} // namespace Weave
} // namespace nl

/* Include a header file containing the implementation of the ConfigurationManager
 * object for the selected platform.
 */
#ifdef EXTERNAL_CONFIGURATIONMANAGERIMPL_HEADER
#include EXTERNAL_CONFIGURATIONMANAGERIMPL_HEADER
#else
#define CONFIGURATIONMANAGERIMPL_HEADER <Weave/DeviceLayer/WEAVE_DEVICE_LAYER_TARGET/ConfigurationManagerImpl.h>
#include CONFIGURATIONMANAGERIMPL_HEADER
#endif

namespace nl {
namespace Weave {
namespace DeviceLayer {

/**
 * Id of the vendor that produced the device.
 */
inline WEAVE_ERROR ConfigurationManager::GetVendorId(uint16_t & vendorId)
{
    return static_cast<ImplClass*>(this)->_GetVendorId(vendorId);
}

/**
 * Device product id assigned by the vendor.
 */
inline WEAVE_ERROR ConfigurationManager::GetProductId(uint16_t & productId)
{
    return static_cast<ImplClass*>(this)->_GetProductId(productId);
}

/**
 * Product revision number assigned by the vendor.
 */
inline WEAVE_ERROR ConfigurationManager::GetProductRevision(uint16_t & productRev)
{
    return static_cast<ImplClass*>(this)->_GetProductRevision(productRev);
}

inline WEAVE_ERROR ConfigurationManager::GetSerialNumber(char * buf, size_t bufSize, size_t & serialNumLen)
{
    return static_cast<ImplClass*>(this)->_GetSerialNumber(buf, bufSize, serialNumLen);
}

inline WEAVE_ERROR ConfigurationManager::GetPrimaryWiFiMACAddress(uint8_t * buf)
{
    return static_cast<ImplClass*>(this)->_GetPrimaryWiFiMACAddress(buf);
}

inline WEAVE_ERROR ConfigurationManager::GetPrimary802154MACAddress(uint8_t * buf)
{
    return static_cast<ImplClass*>(this)->_GetPrimary802154MACAddress(buf);
}

inline WEAVE_ERROR ConfigurationManager::GetManufacturingDate(uint16_t & year, uint8_t & month, uint8_t & dayOfMonth)
{
    return static_cast<ImplClass*>(this)->_GetManufacturingDate(year, month, dayOfMonth);
}

inline WEAVE_ERROR ConfigurationManager::GetFirmwareRevision(char * buf, size_t bufSize, size_t & outLen)
{
    return static_cast<ImplClass*>(this)->_GetFirmwareRevision(buf, bufSize, outLen);
}

inline WEAVE_ERROR ConfigurationManager::GetFirmwareBuildTime(uint16_t & year, uint8_t & month, uint8_t & dayOfMonth,
        uint8_t & hour, uint8_t & minute, uint8_t & second)
{
    return static_cast<ImplClass*>(this)->_GetFirmwareBuildTime(year, month, dayOfMonth, hour, minute, second);
}

inline WEAVE_ERROR ConfigurationManager::GetDeviceId(uint64_t & deviceId)
{
    return static_cast<ImplClass*>(this)->_GetDeviceId(deviceId);
}

inline WEAVE_ERROR ConfigurationManager::GetDeviceCertificate(uint8_t * buf, size_t bufSize, size_t & certLen)
{
    return static_cast<ImplClass*>(this)->_GetDeviceCertificate(buf, bufSize, certLen);
}

inline WEAVE_ERROR ConfigurationManager::GetDevicePrivateKey(uint8_t * buf, size_t bufSize, size_t & keyLen)
{
    return static_cast<ImplClass*>(this)->_GetDevicePrivateKey(buf, bufSize, keyLen);
}

inline WEAVE_ERROR ConfigurationManager::GetPairingCode(char * buf, size_t bufSize, size_t & pairingCodeLen)
{
    return static_cast<ImplClass*>(this)->_GetPairingCode(buf, bufSize, pairingCodeLen);
}

inline WEAVE_ERROR ConfigurationManager::GetServiceId(uint64_t & serviceId)
{
    return static_cast<ImplClass*>(this)->_GetServiceId(serviceId);
}

inline WEAVE_ERROR ConfigurationManager::GetFabricId(uint64_t & fabricId)
{
    return static_cast<ImplClass*>(this)->_GetFabricId(fabricId);
}

inline WEAVE_ERROR ConfigurationManager::GetServiceConfig(uint8_t * buf, size_t bufSize, size_t & serviceConfigLen)
{
    return static_cast<ImplClass*>(this)->_GetServiceConfig(buf, bufSize, serviceConfigLen);
}

inline WEAVE_ERROR ConfigurationManager::GetPairedAccountId(char * buf, size_t bufSize, size_t & accountIdLen)
{
    return static_cast<ImplClass*>(this)->_GetPairedAccountId(buf, bufSize, accountIdLen);
}

inline WEAVE_ERROR ConfigurationManager::StoreDeviceId(uint64_t deviceId)
{
    return static_cast<ImplClass*>(this)->_StoreDeviceId(deviceId);
}

inline WEAVE_ERROR ConfigurationManager::StoreSerialNumber(const char * serialNum)
{
    return static_cast<ImplClass*>(this)->_StoreSerialNumber(serialNum);
}

inline WEAVE_ERROR ConfigurationManager::StorePrimaryWiFiMACAddress(const uint8_t * buf)
{
    return static_cast<ImplClass*>(this)->_StorePrimaryWiFiMACAddress(buf);
}

inline WEAVE_ERROR ConfigurationManager::StorePrimary802154MACAddress(const uint8_t * buf)
{
    return static_cast<ImplClass*>(this)->_StorePrimary802154MACAddress(buf);
}

inline WEAVE_ERROR ConfigurationManager::StoreManufacturingDate(const char * mfgDate)
{
    return static_cast<ImplClass*>(this)->_StoreManufacturingDate(mfgDate);
}

inline WEAVE_ERROR ConfigurationManager::StoreFabricId(uint64_t fabricId)
{
    return static_cast<ImplClass*>(this)->_StoreFabricId(fabricId);
}

inline WEAVE_ERROR ConfigurationManager::StoreDeviceCertificate(const uint8_t * cert, size_t certLen)
{
    return static_cast<ImplClass*>(this)->_StoreDeviceCertificate(cert, certLen);
}

inline WEAVE_ERROR ConfigurationManager::StoreDevicePrivateKey(const uint8_t * key, size_t keyLen)
{
    return static_cast<ImplClass*>(this)->_StoreDevicePrivateKey(key, keyLen);
}

inline WEAVE_ERROR ConfigurationManager::StorePairingCode(const char * pairingCode)
{
    return static_cast<ImplClass*>(this)->_StorePairingCode(pairingCode);
}

inline WEAVE_ERROR ConfigurationManager::StoreServiceProvisioningData(uint64_t serviceId, const uint8_t * serviceConfig, size_t serviceConfigLen, const char * accountId, size_t accountIdLen)
{
    return static_cast<ImplClass*>(this)->_StoreServiceProvisioningData(serviceId, serviceConfig, serviceConfigLen, accountId, accountIdLen);
}

inline WEAVE_ERROR ConfigurationManager::ClearServiceProvisioningData()
{
    return static_cast<ImplClass*>(this)->_ClearServiceProvisioningData();
}

inline WEAVE_ERROR ConfigurationManager::StoreServiceConfig(const uint8_t * serviceConfig, size_t serviceConfigLen)
{
    return static_cast<ImplClass*>(this)->_StoreServiceConfig(serviceConfig, serviceConfigLen);
}

inline WEAVE_ERROR ConfigurationManager::StorePairedAccountId(const char * accountId, size_t accountIdLen)
{
    return static_cast<ImplClass*>(this)->_StorePairedAccountId(accountId, accountIdLen);
}

inline WEAVE_ERROR ConfigurationManager::ReadPersistedStorageValue(::nl::Weave::Platform::PersistedStorage::Key key, uint32_t & value)
{
    return static_cast<ImplClass*>(this)->_ReadPersistedStorageValue(key, value);
}

inline WEAVE_ERROR ConfigurationManager::WritePersistedStorageValue(::nl::Weave::Platform::PersistedStorage::Key key, uint32_t value)
{
    return static_cast<ImplClass*>(this)->_WritePersistedStorageValue(key, value);
}

inline WEAVE_ERROR ConfigurationManager::GetDeviceDescriptor(WeaveDeviceDescriptor & deviceDesc)
{
    return static_cast<ImplClass*>(this)->_GetDeviceDescriptor(deviceDesc);
}

inline WEAVE_ERROR ConfigurationManager::GetDeviceDescriptorTLV(uint8_t * buf, size_t bufSize, size_t & encodedLen)
{
    return static_cast<ImplClass*>(this)->_GetDeviceDescriptorTLV(buf, bufSize, encodedLen);
}

inline WEAVE_ERROR ConfigurationManager::GetQRCodeString(char * buf, size_t bufSize)
{
    return static_cast<ImplClass*>(this)->_GetQRCodeString(buf, bufSize);
}

inline WEAVE_ERROR ConfigurationManager::GetWiFiAPSSID(char * buf, size_t bufSize)
{
    return static_cast<ImplClass*>(this)->_GetWiFiAPSSID(buf, bufSize);
}

inline bool ConfigurationManager::IsServiceProvisioned()
{
    return static_cast<ImplClass*>(this)->_IsServiceProvisioned();
}

inline bool ConfigurationManager::IsPairedToAccount()
{
    return static_cast<ImplClass*>(this)->_IsPairedToAccount();
}

inline bool ConfigurationManager::IsMemberOfFabric()
{
    return static_cast<ImplClass*>(this)->_IsMemberOfFabric();
}

inline void ConfigurationManager::InitiateFactoryReset()
{
    static_cast<ImplClass*>(this)->_InitiateFactoryReset();
}

inline WEAVE_ERROR ConfigurationManager::Init()
{
    return static_cast<ImplClass*>(this)->_Init();
}

inline WEAVE_ERROR ConfigurationManager::ConfigureWeaveStack()
{
    return static_cast<ImplClass*>(this)->_ConfigureWeaveStack();
}

inline ::nl::Weave::Profiles::Security::AppKeys::GroupKeyStoreBase * ConfigurationManager::GetGroupKeyStore()
{
     return static_cast<ImplClass*>(this)->_GetGroupKeyStore();
}

inline bool ConfigurationManager::CanFactoryReset()
{
    return static_cast<ImplClass*>(this)->_CanFactoryReset();
}

inline WEAVE_ERROR ConfigurationManager::GetFailSafeArmed(bool & val)
{
    return static_cast<ImplClass*>(this)->_GetFailSafeArmed(val);
}

inline WEAVE_ERROR ConfigurationManager::SetFailSafeArmed(bool val)
{
    return static_cast<ImplClass*>(this)->_SetFailSafeArmed(val);
}

} // namespace DeviceLayer
} // namespace Weave
} // namespace nl

#endif // CONFIGURATION_MANAGER_H
