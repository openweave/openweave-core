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

#define WEAVE_DEVICE_TARGET ESP32

namespace nl {
namespace Weave {
namespace DeviceLayer {

class TraitManager;
template<class Target> class ConfigurationManagerImpl;
namespace Internal
{
class DeviceControlServer;
class NetworkProvisioningServer;
}

struct WEAVE_DEVICE_TARGET {};

namespace Interface {

/**
 * Provides access to runtime and build-time configuration information for a Weave device.
 */
template<class Target>
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

    WEAVE_ERROR GetDeviceDescriptor(::nl::Weave::Profiles::DeviceDescription::WeaveDeviceDescriptor & deviceDesc);
    WEAVE_ERROR GetDeviceDescriptorTLV(uint8_t * buf, size_t bufSize, size_t & encodedLen);
    WEAVE_ERROR GetQRCodeString(char * buf, size_t bufSize);

    WEAVE_ERROR GetWiFiAPSSID(char * buf, size_t bufSize);

    bool IsServiceProvisioned();
    bool IsPairedToAccount();
    bool IsMemberOfFabric();

    void InitiateFactoryReset();

private:

    // Members for internal use by the following friends.

    friend class ::nl::Weave::DeviceLayer::PlatformManager;
    friend class ::nl::Weave::DeviceLayer::TraitManager;
    friend class ::nl::Weave::DeviceLayer::Internal::DeviceControlServer;
    friend class ::nl::Weave::DeviceLayer::Internal::NetworkProvisioningServer;
    friend WEAVE_ERROR ::nl::Weave::Platform::PersistedStorage::Read(::nl::Weave::Platform::PersistedStorage::Key key, uint32_t & value);
    friend WEAVE_ERROR ::nl::Weave::Platform::PersistedStorage::Write(::nl::Weave::Platform::PersistedStorage::Key key, uint32_t value);

    WEAVE_ERROR Init();
    WEAVE_ERROR ConfigureWeaveStack();
    ::nl::Weave::Profiles::Security::AppKeys::GroupKeyStoreBase * GetGroupKeyStore(); // TODO: maybe remove this???
    bool CanFactoryReset();
    WEAVE_ERROR GetFailSafeArmed(bool & val);
    WEAVE_ERROR SetFailSafeArmed(bool val);
    WEAVE_ERROR ReadPersistedStorageValue(::nl::Weave::Platform::PersistedStorage::Key key, uint32_t & value);
    WEAVE_ERROR WritePersistedStorageValue(::nl::Weave::Platform::PersistedStorage::Key key, uint32_t value);

private:

    // Private members reserved for use by this class only.

    using ImplClass = ConfigurationManagerImpl<Target>;
    ImplClass * Impl() { return static_cast<ImplClass *>(this); }
    const ImplClass * Impl() const { return static_cast<const ImplClass *>(this); }

protected:

    // Access to construction/destruction is limited to subclasses.
    ConfigurationManager() = default;
    ~ConfigurationManager() = default;
};

/**
 * Id of the vendor that produced the device.
 */
template<class Target>
WEAVE_ERROR ConfigurationManager<Target>::GetVendorId(uint16_t & vendorId)
{
    return Impl()->_GetVendorId(vendorId);
}

/**
 * Device product id assigned by the vendor.
 */
template<class Target>
WEAVE_ERROR ConfigurationManager<Target>::GetProductId(uint16_t & productId)
{
    return Impl()->_GetProductId(productId);
}

/**
 * Product revision number assigned by the vendor.
 */
template<class Target>
WEAVE_ERROR ConfigurationManager<Target>::GetProductRevision(uint16_t & productRev)
{
    return Impl()->_GetProductRevision(productRev);
}

/**
 *
 */
template<class Target>
WEAVE_ERROR ConfigurationManager<Target>::GetSerialNumber(char * buf, size_t bufSize, size_t & serialNumLen)
{
    return Impl()->_GetSerialNumber(buf, bufSize, serialNumLen);
}

template<class Target>
WEAVE_ERROR ConfigurationManager<Target>::GetPrimaryWiFiMACAddress(uint8_t * buf)
{
    return Impl()->_GetPrimaryWiFiMACAddress(buf);
}

template<class Target>
WEAVE_ERROR ConfigurationManager<Target>::GetPrimary802154MACAddress(uint8_t * buf)
{
    return Impl()->_GetPrimary802154MACAddress(buf);
}

template<class Target>
WEAVE_ERROR ConfigurationManager<Target>::GetManufacturingDate(uint16_t & year, uint8_t & month, uint8_t & dayOfMonth)
{
    return Impl()->_GetManufacturingDate(year, month, dayOfMonth);
}

template<class Target>
WEAVE_ERROR ConfigurationManager<Target>::GetFirmwareRevision(char * buf, size_t bufSize, size_t & outLen)
{
    return Impl()->_GetFirmwareRevision(buf, bufSize, outLen);
}

template<class Target>
WEAVE_ERROR ConfigurationManager<Target>::GetFirmwareBuildTime(uint16_t & year, uint8_t & month, uint8_t & dayOfMonth,
        uint8_t & hour, uint8_t & minute, uint8_t & second)
{
    return Impl()->_GetFirmwareBuildTime(year, month, dayOfMonth, hour, minute, second);
}

template<class Target>
WEAVE_ERROR ConfigurationManager<Target>::GetDeviceId(uint64_t & deviceId)
{
    return Impl()->_GetDeviceId(deviceId);
}

template<class Target>
WEAVE_ERROR ConfigurationManager<Target>::GetDeviceCertificate(uint8_t * buf, size_t bufSize, size_t & certLen)
{
    return Impl()->_GetDeviceCertificate(buf, bufSize, certLen);
}

template<class Target>
WEAVE_ERROR ConfigurationManager<Target>::GetDevicePrivateKey(uint8_t * buf, size_t bufSize, size_t & keyLen)
{
    return Impl()->_GetDevicePrivateKey(buf, bufSize, keyLen);
}

template<class Target>
WEAVE_ERROR ConfigurationManager<Target>::GetPairingCode(char * buf, size_t bufSize, size_t & pairingCodeLen)
{
    return Impl()->_GetPairingCode(buf, bufSize, pairingCodeLen);
}

template<class Target>
WEAVE_ERROR ConfigurationManager<Target>::GetServiceId(uint64_t & serviceId)
{
    return Impl()->_GetServiceId(serviceId);
}

template<class Target>
WEAVE_ERROR ConfigurationManager<Target>::GetFabricId(uint64_t & fabricId)
{
    return Impl()->_GetFabricId(fabricId);
}

template<class Target>
WEAVE_ERROR ConfigurationManager<Target>::GetServiceConfig(uint8_t * buf, size_t bufSize, size_t & serviceConfigLen)
{
    return Impl()->_GetServiceConfig(buf, bufSize, serviceConfigLen);
}

template<class Target>
WEAVE_ERROR ConfigurationManager<Target>::GetPairedAccountId(char * buf, size_t bufSize, size_t & accountIdLen)
{
    return Impl()->_GetPairedAccountId(buf, bufSize, accountIdLen);
}

template<class Target>
WEAVE_ERROR ConfigurationManager<Target>::StoreDeviceId(uint64_t deviceId)
{
    return Impl()->_StoreDeviceId(deviceId);
}

template<class Target>
WEAVE_ERROR ConfigurationManager<Target>::StoreSerialNumber(const char * serialNum)
{
    return Impl()->_StoreSerialNumber(serialNum);
}

template<class Target>
WEAVE_ERROR ConfigurationManager<Target>::StorePrimaryWiFiMACAddress(const uint8_t * buf)
{
    return Impl()->_StorePrimaryWiFiMACAddress(buf);
}

template<class Target>
WEAVE_ERROR ConfigurationManager<Target>::StorePrimary802154MACAddress(const uint8_t * buf)
{
    return Impl()->_StorePrimary802154MACAddress(buf);
}

template<class Target>
WEAVE_ERROR ConfigurationManager<Target>::StoreManufacturingDate(const char * mfgDate)
{
    return Impl()->_StoreManufacturingDate(mfgDate);
}

template<class Target>
WEAVE_ERROR ConfigurationManager<Target>::StoreFabricId(uint64_t fabricId)
{
    return Impl()->_StoreFabricId(fabricId);
}

template<class Target>
WEAVE_ERROR ConfigurationManager<Target>::StoreDeviceCertificate(const uint8_t * cert, size_t certLen)
{
    return Impl()->_StoreDeviceCertificate(cert, certLen);
}

template<class Target>
WEAVE_ERROR ConfigurationManager<Target>::StoreDevicePrivateKey(const uint8_t * key, size_t keyLen)
{
    return Impl()->_StoreDevicePrivateKey(key, keyLen);
}

template<class Target>
WEAVE_ERROR ConfigurationManager<Target>::StorePairingCode(const char * pairingCode)
{
    return Impl()->_StorePairingCode(pairingCode);
}

template<class Target>
WEAVE_ERROR ConfigurationManager<Target>::StoreServiceProvisioningData(uint64_t serviceId, const uint8_t * serviceConfig, size_t serviceConfigLen, const char * accountId, size_t accountIdLen)
{
    return Impl()->_StoreServiceProvisioningData(serviceId, serviceConfig, serviceConfigLen, accountId, accountIdLen);
}

template<class Target>
WEAVE_ERROR ConfigurationManager<Target>::ClearServiceProvisioningData()
{
    return Impl()->_ClearServiceProvisioningData();
}

template<class Target>
WEAVE_ERROR ConfigurationManager<Target>::StoreServiceConfig(const uint8_t * serviceConfig, size_t serviceConfigLen)
{
    return Impl()->_StoreServiceConfig(serviceConfig, serviceConfigLen);
}

template<class Target>
WEAVE_ERROR ConfigurationManager<Target>::StorePairedAccountId(const char * accountId, size_t accountIdLen)
{
    return Impl()->_StorePairedAccountId(accountId, accountIdLen);
}

template<class Target>
WEAVE_ERROR ConfigurationManager<Target>::ReadPersistedStorageValue(::nl::Weave::Platform::PersistedStorage::Key key, uint32_t & value)
{
    return Impl()->_ReadPersistedStorageValue(key, value);
}

template<class Target>
WEAVE_ERROR ConfigurationManager<Target>::WritePersistedStorageValue(::nl::Weave::Platform::PersistedStorage::Key key, uint32_t value)
{
    return Impl()->_WritePersistedStorageValue(key, value);
}

template<class Target>
WEAVE_ERROR ConfigurationManager<Target>::GetDeviceDescriptor(::nl::Weave::Profiles::DeviceDescription::WeaveDeviceDescriptor & deviceDesc)
{
    return Impl()->_GetDeviceDescriptor(deviceDesc);
}

template<class Target>
WEAVE_ERROR ConfigurationManager<Target>::GetDeviceDescriptorTLV(uint8_t * buf, size_t bufSize, size_t & encodedLen)
{
    return Impl()->_GetDeviceDescriptorTLV(buf, bufSize, encodedLen);
}

template<class Target>
WEAVE_ERROR ConfigurationManager<Target>::GetQRCodeString(char * buf, size_t bufSize)
{
    return Impl()->_GetQRCodeString(buf, bufSize);
}

template<class Target>
WEAVE_ERROR ConfigurationManager<Target>::GetWiFiAPSSID(char * buf, size_t bufSize)
{
    return Impl()->_GetWiFiAPSSID(buf, bufSize);
}

template<class Target>
bool ConfigurationManager<Target>::IsServiceProvisioned()
{
    return Impl()->_IsServiceProvisioned();
}

template<class Target>
bool ConfigurationManager<Target>::IsPairedToAccount()
{
    return Impl()->_IsPairedToAccount();
}

template<class Target>
bool ConfigurationManager<Target>::IsMemberOfFabric()
{
    return Impl()->_IsMemberOfFabric();
}

template<class Target>
void ConfigurationManager<Target>::InitiateFactoryReset()
{
    return Impl()->_InitiateFactoryReset();
}

template<class Target>
WEAVE_ERROR ConfigurationManager<Target>::Init()
{
    return Impl()->_Init();
}

template<class Target>
WEAVE_ERROR ConfigurationManager<Target>::ConfigureWeaveStack()
{
    return Impl()->_ConfigureWeaveStack();
}

template<class Target>
::nl::Weave::Profiles::Security::AppKeys::GroupKeyStoreBase * ConfigurationManager<Target>::GetGroupKeyStore()
{
     return Impl()->_GetGroupKeyStore();
}

template<class Target>
bool ConfigurationManager<Target>::CanFactoryReset()
{
    return Impl()->_CanFactoryReset();
}

template<class Target>
WEAVE_ERROR ConfigurationManager<Target>::GetFailSafeArmed(bool & val)
{
    return Impl()->_GetFailSafeArmed(val);
}

template<class Target>
WEAVE_ERROR ConfigurationManager<Target>::SetFailSafeArmed(bool val)
{
    return Impl()->_SetFailSafeArmed(val);
}

} // namespace Interface


/**
 * A type alias for the class that defines the interface to the ConfigurationManager object.
 *
 * API users are encouraged to use this rather than the more cumbersome templated type name.
 */
using ConfigurationManager = Interface::ConfigurationManager<WEAVE_DEVICE_TARGET>;

} // namespace DeviceLayer
} // namespace Weave
} // namespace nl

/* Include a header file containing the implementation of the ConfigurationManager
 * object for the selected Device Layer target.
 */
#define WEAVE_DEVICE_TARGET_INCLUDE <Weave/DeviceLayer/WEAVE_DEVICE_TARGET/ConfigurationManagerImpl.h>
#include WEAVE_DEVICE_TARGET_INCLUDE
#undef WEAVE_DEVICE_TARGET_INCLUDE

namespace nl {
namespace Weave {
namespace DeviceLayer {

/**
 * Returns a reference to the public interface of the singleton ConfigurationManager object.
 *
 * API users should use this to access features of the ConfigurationManager object that are common
 * to all target platforms.
 */
inline ConfigurationManager & ConfigurationMgr()
{
    return ConfigurationManagerImpl<WEAVE_DEVICE_TARGET>::Instance();
}

} // namespace DeviceLayer
} // namespace Weave
} // namespace nl

#endif // CONFIGURATION_MANAGER_H
