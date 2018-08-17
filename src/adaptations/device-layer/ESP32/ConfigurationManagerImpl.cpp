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
 *          Provides the implementation of the Device Layer ConfigurationManager object
 *          for the ESP32.
 */

#include <Weave/DeviceLayer/internal/WeaveDeviceLayerInternal.h>
#include <Weave/DeviceLayer/ConfigurationManager.h>
#include <Weave/Core/WeaveKeyIds.h>
#include <Weave/Core/WeaveVendorIdentifiers.hpp>
#include <Weave/Profiles/security/WeaveApplicationKeys.h>
#include <Weave/Profiles/vendor/nestlabs/device-description/NestProductIdentifiers.hpp>
#include <Weave/DeviceLayer/ESP32/GroupKeyStoreImpl.h>
#include <Weave/DeviceLayer/ESP32/NVSSupport.h>

#include "esp_wifi.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <new>

using namespace ::nl;
using namespace ::nl::Weave;
using namespace ::nl::Weave::Profiles::Security::AppKeys;
using namespace ::nl::Weave::Profiles::DeviceDescription;
using namespace ::nl::Weave::DeviceLayer::Internal;

using ::nl::Weave::kWeaveVendor_NestLabs;

namespace nl {
namespace Weave {
namespace DeviceLayer {

namespace {

enum
{
    kNestWeaveProduct_Connect = 0x0016
};

// Singleton instance of Weave Group Key Store for the ESP32
//
// NOTE: This is declared as a private global variable, rather than a static
// member of ConfigurationManagerImpl, to reduce the number of headers that
// must be included by the application when using the ConfigurationManager API.
//
GroupKeyStoreImpl gGroupKeyStore;

} // unnamed namespace


// Singleton instance of the ConfigurationManager object for the ESP32.
ConfigurationManagerImpl<ESP32> ConfigurationManagerImpl<ESP32>::sInstance;


// ==================== Implementations for ConfigurationManager Public Interface Methods ====================

#if 0

WEAVE_ERROR ConfigurationManagerImpl<ESP32>::_GetSerialNumber(char * buf, size_t bufSize, size_t & serialNumLen)
{
    return GetNVS(kNVSNamespace_WeaveFactory, kNVSKeyName_SerialNum, buf, bufSize, serialNumLen);
}

WEAVE_ERROR ConfigurationManagerImpl<ESP32>::_GetManufacturingDate(uint16_t& year, uint8_t& month, uint8_t& dayOfMonth)
{
    WEAVE_ERROR err;
    enum {
        kDateStringLength = 10 // YYYY-MM-DD
    };
    char dateStr[kDateStringLength + 1];
    size_t dateLen;
    char *parseEnd;

    err = GetNVS(kNVSNamespace_WeaveFactory, kNVSKeyName_ManufacturingDate, dateStr, sizeof(dateStr), dateLen);
    SuccessOrExit(err);

    VerifyOrExit(dateLen == kDateStringLength, err = WEAVE_ERROR_INVALID_ARGUMENT);

    year = strtoul(dateStr, &parseEnd, 10);
    VerifyOrExit(parseEnd == dateStr + 4, err = WEAVE_ERROR_INVALID_ARGUMENT);

    month = strtoul(dateStr + 5, &parseEnd, 10);
    VerifyOrExit(parseEnd == dateStr + 7, err = WEAVE_ERROR_INVALID_ARGUMENT);

    dayOfMonth = strtoul(dateStr + 8, &parseEnd, 10);
    VerifyOrExit(parseEnd == dateStr + 10, err = WEAVE_ERROR_INVALID_ARGUMENT);

exit:
    if (err != WEAVE_NO_ERROR && err != WEAVE_DEVICE_ERROR_CONFIG_NOT_FOUND)
    {
        WeaveLogError(DeviceLayer, "Invalid manufacturing date: %s", dateStr);
    }
    return err;
}

#endif

WEAVE_ERROR ConfigurationManagerImpl<ESP32>::_GetDeviceCertificate(uint8_t * buf, size_t bufSize, size_t & certLen)
{
    WEAVE_ERROR err;

    err = GetNVS(kNVSNamespace_WeaveFactory, kNVSKeyName_DeviceCert, buf, bufSize, certLen);

#if WEAVE_DEVICE_CONFIG_ENABLE_TEST_DEVICE_IDENTITY

    if (err == WEAVE_DEVICE_ERROR_CONFIG_NOT_FOUND)
    {
        certLen = TestDeviceCertLength;
        VerifyOrExit(TestDeviceCertLength <= bufSize, err = WEAVE_ERROR_BUFFER_TOO_SMALL);
        WeaveLogProgress(DeviceLayer, "Device certificate not found in nvs; using default");
        memcpy(buf, TestDeviceCert, TestDeviceCertLength);
        err = WEAVE_NO_ERROR;
    }

#endif // WEAVE_DEVICE_CONFIG_ENABLE_TEST_DEVICE_IDENTITY

    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR ConfigurationManagerImpl<ESP32>::_GetDeviceCertificateLength(size_t & certLen)
{
    WEAVE_ERROR err = GetDeviceCertificate((uint8_t *)NULL, 0, certLen);
    if (err == WEAVE_ERROR_BUFFER_TOO_SMALL)
    {
        err = WEAVE_NO_ERROR;
    }
    return err;
}

WEAVE_ERROR ConfigurationManagerImpl<ESP32>::_GetDevicePrivateKey(uint8_t * buf, size_t bufSize, size_t & keyLen)
{
    WEAVE_ERROR err;

    err = GetNVS(kNVSNamespace_WeaveFactory, kNVSKeyName_DevicePrivateKey, buf, bufSize, keyLen);

#if WEAVE_DEVICE_CONFIG_ENABLE_TEST_DEVICE_IDENTITY

    if (err == WEAVE_DEVICE_ERROR_CONFIG_NOT_FOUND)
    {
        keyLen = TestDevicePrivateKeyLength;
        VerifyOrExit(TestDevicePrivateKeyLength <= bufSize, err = WEAVE_ERROR_BUFFER_TOO_SMALL);
        WeaveLogProgress(DeviceLayer, "Device private key not found in nvs; using default");
        memcpy(buf, TestDevicePrivateKey, TestDevicePrivateKeyLength);
        err = WEAVE_NO_ERROR;
    }

#endif // WEAVE_DEVICE_CONFIG_ENABLE_TEST_DEVICE_IDENTITY

    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR ConfigurationManagerImpl<ESP32>::_GetDevicePrivateKeyLength(size_t & keyLen)
{
    WEAVE_ERROR err = GetDevicePrivateKey((uint8_t *)NULL, 0, keyLen);
    if (err == WEAVE_ERROR_BUFFER_TOO_SMALL)
    {
        err = WEAVE_NO_ERROR;
    }
    return err;
}

WEAVE_ERROR ConfigurationManagerImpl<ESP32>::_GetServiceConfig(uint8_t * buf, size_t bufSize, size_t & serviceConfigLen)
{
    return GetNVS(kNVSNamespace_WeaveConfig, kNVSKeyName_ServiceConfig, buf, bufSize, serviceConfigLen);
}

WEAVE_ERROR ConfigurationManagerImpl<ESP32>::_GetServiceConfigLength(size_t & serviceConfigLen)
{
    WEAVE_ERROR err = GetServiceConfig((uint8_t *)NULL, 0, serviceConfigLen);
    if (err == WEAVE_ERROR_BUFFER_TOO_SMALL)
    {
        err = WEAVE_NO_ERROR;
    }
    return err;
}

WEAVE_ERROR ConfigurationManagerImpl<ESP32>::_GetServiceId(uint64_t & serviceId)
{
    return GetNVS(kNVSNamespace_WeaveConfig, kNVSKeyName_ServiceId, serviceId);
}

WEAVE_ERROR ConfigurationManagerImpl<ESP32>::_GetPairedAccountId(char * buf, size_t bufSize, size_t & accountIdLen)
{
    return GetNVS(kNVSNamespace_WeaveConfig, kNVSKeyName_PairedAccountId, buf, bufSize, accountIdLen);
}

WEAVE_ERROR ConfigurationManagerImpl<ESP32>::_GetDeviceId(uint64_t & deviceId)
{
    WEAVE_ERROR err;
    uint8_t deviceIdBytes[sizeof(uint64_t)];
    size_t deviceIdLen = sizeof(deviceIdBytes);

    // Read the device id from NVS.  For the convenience of manufacturing, on the ESP32, the value
    // is stored as an 8-byte blob in big-endian format, rather than a u64 value.
    err = ReadNVSValue(NVSKeys::DeviceId, deviceIdBytes, sizeof(deviceIdBytes), deviceIdLen);
    if (err == WEAVE_NO_ERROR)
    {
        if (deviceIdLen == sizeof(deviceIdBytes))
        {
            deviceId = Encoding::BigEndian::Get64(deviceIdBytes);
        }
        else
        {
            err = ESP_ERR_NVS_INVALID_LENGTH;
        }
    }

    return err;
}

WEAVE_ERROR ConfigurationManagerImpl<ESP32>::_StoreDeviceId(uint64_t deviceId)
{
    WEAVE_ERROR err;

    if (deviceId != kNodeIdNotSpecified)
    {
        uint8_t deviceIdBytes[sizeof(uint64_t)];

        Encoding::BigEndian::Put64(deviceIdBytes, deviceId);
        err = WriteNVSValue(NVSKeys::DeviceId, deviceIdBytes, sizeof(deviceIdBytes));
    }

    else
    {
        err = ClearNVSValue(NVSKeys::DeviceId);
    }

    return err;
}

#if 0

WEAVE_ERROR ConfigurationManagerImpl<ESP32>::_StoreSerialNumber(const char * serialNum)
{
    return StoreNVS(kNVSNamespace_WeaveFactory, kNVSKeyName_SerialNum, serialNum);
}

WEAVE_ERROR ConfigurationManagerImpl<ESP32>::_StoreManufacturingDate(const char * mfgDate)
{
    return StoreNVS(kNVSNamespace_WeaveFactory, kNVSKeyName_ManufacturingDate, mfgDate);
}

#endif

WEAVE_ERROR ConfigurationManagerImpl<ESP32>::_StoreFabricId(uint64_t fabricId)
{
    return (fabricId != kFabricIdNotSpecified)
           ? StoreNVS(kNVSNamespace_WeaveConfig, kNVSKeyName_FabricId, fabricId)
           : ClearNVSKey(kNVSNamespace_WeaveConfig, kNVSKeyName_FabricId);
}

WEAVE_ERROR ConfigurationManagerImpl<ESP32>::_StoreDeviceCertificate(const uint8_t * cert, size_t certLen)
{
    return StoreNVS(kNVSNamespace_WeaveFactory, kNVSKeyName_DeviceCert, cert, certLen);
}

WEAVE_ERROR ConfigurationManagerImpl<ESP32>::_StoreDevicePrivateKey(const uint8_t * key, size_t keyLen)
{
    return StoreNVS(kNVSNamespace_WeaveFactory, kNVSKeyName_DevicePrivateKey, key, keyLen);
}

WEAVE_ERROR ConfigurationManagerImpl<ESP32>::_StorePairingCode(const char * pairingCode)
{
    return StoreNVS(kNVSNamespace_WeaveFactory, kNVSKeyName_PairingCode, pairingCode);
}

WEAVE_ERROR ConfigurationManagerImpl<ESP32>::_StoreServiceProvisioningData(uint64_t serviceId,
        const uint8_t * serviceConfig, size_t serviceConfigLen,
        const char * accountId, size_t accountIdLen)
{
    WEAVE_ERROR err;
    nvs_handle handle;
    bool needClose = false;
    char *accountIdCopy = NULL;

    err = nvs_open(kNVSNamespace_WeaveConfig, NVS_READWRITE, &handle);
    SuccessOrExit(err);
    needClose = true;

    err = nvs_set_u64(handle, kNVSKeyName_ServiceId, serviceId);
    SuccessOrExit(err);

    err = nvs_set_blob(handle, kNVSKeyName_ServiceConfig, serviceConfig, serviceConfigLen);
    SuccessOrExit(err);

    if (accountId != NULL && accountIdLen != 0)
    {
        accountIdCopy = strndup(accountId, accountIdLen);
        VerifyOrExit(accountIdCopy != NULL, err = WEAVE_ERROR_NO_MEMORY);
        err = nvs_set_str(handle, kNVSKeyName_PairedAccountId, accountIdCopy);
        free(accountIdCopy);
        SuccessOrExit(err);
    }
    else
    {
        err = nvs_erase_key(handle, kNVSKeyName_PairedAccountId);
        if (err == ESP_ERR_NVS_NOT_FOUND)
        {
            err = WEAVE_NO_ERROR;
        }
        SuccessOrExit(err);
    }

    // Commit the value to the persistent store.
    err = nvs_commit(handle);
    SuccessOrExit(err);

    SetFlag(mFlags, kFlag_IsServiceProvisioned);
    SetFlag(mFlags, kFlag_IsPairedToAccount, (accountId != NULL && accountIdLen != 0));

exit:
    if (needClose)
    {
        nvs_close(handle);
    }
    return err;
}

WEAVE_ERROR ConfigurationManagerImpl<ESP32>::_ClearServiceProvisioningData()
{
    WEAVE_ERROR err;
    nvs_handle handle;
    bool needClose = false;

    err = nvs_open(kNVSNamespace_WeaveConfig, NVS_READWRITE, &handle);
    SuccessOrExit(err);
    needClose = true;

    err = nvs_erase_key(handle, kNVSKeyName_ServiceId);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        err = WEAVE_NO_ERROR;
    }
    SuccessOrExit(err);

    err = nvs_erase_key(handle, kNVSKeyName_ServiceConfig);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        err = WEAVE_NO_ERROR;
    }
    SuccessOrExit(err);

    err = nvs_erase_key(handle, kNVSKeyName_PairedAccountId);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        err = WEAVE_NO_ERROR;
    }
    SuccessOrExit(err);

    // Commit to the persistent store.
    err = nvs_commit(handle);
    SuccessOrExit(err);

    // If necessary, post an event alerting other subsystems to the change in
    // the account pairing state.
    if (IsPairedToAccount())
    {
        WeaveDeviceEvent event;
        event.Type = WeaveDeviceEvent::kEventType_AccountPairingChange;
        event.AccountPairingChange.IsPairedToAccount = false;
        PlatformMgr.PostEvent(&event);
    }

    // If necessary, post an event alerting other subsystems to the change in
    // the service provisioning state.
    if (IsServiceProvisioned())
    {
        WeaveDeviceEvent event;
        event.Type = WeaveDeviceEvent::kEventType_ServiceProvisioningChange;
        event.ServiceProvisioningChange.IsServiceProvisioned = false;
        event.ServiceProvisioningChange.ServiceConfigUpdated = false;
        PlatformMgr.PostEvent(&event);
    }

    ClearFlag(mFlags, kFlag_IsServiceProvisioned);
    ClearFlag(mFlags, kFlag_IsPairedToAccount);

exit:
    if (needClose)
    {
        nvs_close(handle);
    }
    return err;
}

WEAVE_ERROR ConfigurationManagerImpl<ESP32>::_StoreServiceConfig(const uint8_t * serviceConfig, size_t serviceConfigLen)
{
    return StoreNVS(kNVSNamespace_WeaveConfig, kNVSKeyName_ServiceConfig, serviceConfig, serviceConfigLen);
}

WEAVE_ERROR ConfigurationManagerImpl<ESP32>::_StoreAccountId(const char * accountId, size_t accountIdLen)
{
    WEAVE_ERROR err;

    if (accountId != NULL && accountIdLen != 0)
    {
        char * accountIdCopy = strndup(accountId, accountIdLen);
        VerifyOrExit(accountIdCopy != NULL, err = WEAVE_ERROR_NO_MEMORY);
        err = StoreNVS(kNVSNamespace_WeaveConfig, kNVSKeyName_PairedAccountId, accountIdCopy);
        free(accountIdCopy);
        SuccessOrExit(err);
        SetFlag(mFlags, kFlag_IsPairedToAccount);
    }
    else
    {
        err = ClearNVSKey(kNVSNamespace_WeaveConfig, kNVSKeyName_PairedAccountId);
        SuccessOrExit(err);
        ClearFlag(mFlags, kFlag_IsPairedToAccount);
    }

exit:
    return err;
}

WEAVE_ERROR ConfigurationManagerImpl<ESP32>::_GetDeviceDescriptor(WeaveDeviceDescriptor & deviceDesc)
{
    WEAVE_ERROR err;
    size_t outLen;

    deviceDesc.Clear();

    deviceDesc.DeviceId = FabricState.LocalNodeId;

    deviceDesc.FabricId = FabricState.FabricId;

    err = GetVendorId(deviceDesc.VendorId);
    SuccessOrExit(err);

    err = GetProductId(deviceDesc.ProductId);
    SuccessOrExit(err);

    err = GetProductRevision(deviceDesc.ProductRevision);
    SuccessOrExit(err);

    err = GetManufacturingDate(deviceDesc.ManufacturingDate.Year, deviceDesc.ManufacturingDate.Month, deviceDesc.ManufacturingDate.Day);
    if (err == WEAVE_DEVICE_ERROR_CONFIG_NOT_FOUND)
    {
        err = WEAVE_NO_ERROR;
    }
    SuccessOrExit(err);

    err = esp_wifi_get_mac(ESP_IF_WIFI_STA, deviceDesc.PrimaryWiFiMACAddress);
    SuccessOrExit(err);

    err = GetWiFiAPSSID(deviceDesc.RendezvousWiFiESSID, sizeof(deviceDesc.RendezvousWiFiESSID));
    SuccessOrExit(err);

    err = GetSerialNumber(deviceDesc.SerialNumber, sizeof(deviceDesc.SerialNumber), outLen);
    if (err == WEAVE_DEVICE_ERROR_CONFIG_NOT_FOUND)
    {
        err = WEAVE_NO_ERROR;
    }
    SuccessOrExit(err);

    err = GetFirmwareRevision(deviceDesc.SoftwareVersion, sizeof(deviceDesc.SoftwareVersion), outLen);
    if (err == WEAVE_DEVICE_ERROR_CONFIG_NOT_FOUND)
    {
        err = WEAVE_NO_ERROR;
    }
    SuccessOrExit(err);

    // If we're pretending to be a Nest Connect, fake the presence of a 805.15.4 radio by encoding
    // the Weave device id in the Primary 802.15.4 MAC address field.  This is necessary to fool
    // the Nest mobile app into believing we are indeed a Connect.
    if (deviceDesc.VendorId == kWeaveVendor_NestLabs && deviceDesc.ProductId == kNestWeaveProduct_Connect)
    {
        ::nl::Weave::Encoding::BigEndian::Put64(deviceDesc.Primary802154MACAddress, deviceDesc.DeviceId);
        deviceDesc.DeviceId = ::nl::Weave::kNodeIdNotSpecified;
    }

exit:
    return err;
}

WEAVE_ERROR ConfigurationManagerImpl<ESP32>::_GetDeviceDescriptorTLV(uint8_t * buf, size_t bufSize, size_t & encodedLen)
{
    WEAVE_ERROR err;
    WeaveDeviceDescriptor deviceDesc;

    err = GetDeviceDescriptor(deviceDesc);
    SuccessOrExit(err);

    err = WeaveDeviceDescriptor::EncodeTLV(deviceDesc, buf, (uint32_t)bufSize, encodedLen);
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR ConfigurationManagerImpl<ESP32>::_GetQRCodeString(char * buf, size_t bufSize)
{
    WEAVE_ERROR err;
    WeaveDeviceDescriptor deviceDesc;
    uint32_t encodedLen;

    err = GetDeviceDescriptor(deviceDesc);
    SuccessOrExit(err);

    strncpy(deviceDesc.PairingCode, FabricState.PairingCode, WeaveDeviceDescriptor::kMaxPairingCodeLength);
    deviceDesc.PairingCode[WeaveDeviceDescriptor::kMaxPairingCodeLength] = 0;

    err = WeaveDeviceDescriptor::EncodeText(deviceDesc, buf, (uint32_t)bufSize, encodedLen);
    SuccessOrExit(err);

exit:
    return err;
}


WEAVE_ERROR ConfigurationManagerImpl<ESP32>::_GetWiFiAPSSID(char * buf, size_t bufSize)
{
    WEAVE_ERROR err;
    uint8_t mac[6];

    err = esp_wifi_get_mac(ESP_IF_WIFI_STA, mac);
    SuccessOrExit(err);

    snprintf(buf, bufSize, "%s%02X%02X", WEAVE_DEVICE_CONFIG_WIFI_AP_SSID_PREFIX, mac[4], mac[5]);
    buf[bufSize - 1] = 0;

exit:
    return err;
}

bool ConfigurationManagerImpl<ESP32>::_IsMemberOfFabric()
{
    return FabricState.FabricId != ::nl::Weave::kFabricIdNotSpecified;
}

void ConfigurationManagerImpl<ESP32>::_InitiateFactoryReset()
{
    PlatformMgr.ScheduleWork(DoFactoryReset);
}

// ==================== Configuration Manager "Internal Use" Methods ====================

WEAVE_ERROR ConfigurationManagerImpl<ESP32>::_Init()
{
    WEAVE_ERROR err;
    uint32_t failSafeArmed;

    mFlags = 0;

    // Force initialization of weave NVS namespaces if they doesn't already exist.
    err = EnsureNVSNamespace(kNVSNamespace_WeaveFactory);
    SuccessOrExit(err);
    err = EnsureNVSNamespace(kNVSNamespace_WeaveConfig);
    SuccessOrExit(err);
    err = EnsureNVSNamespace(kNVSNamespace_WeaveCounters);
    SuccessOrExit(err);

    // Initialize the global GroupKeyStore object.
    err = gGroupKeyStore.Init();
    SuccessOrExit(err);

    // If the fail-safe was armed when the device last shutdown, initiate a factory reset.
    if (ReadNVSValue(NVSKeys::FailSafeArmed, failSafeArmed) == WEAVE_NO_ERROR &&
        failSafeArmed != 0)
    {
        WeaveLogProgress(DeviceLayer, "Detected fail-safe armed on reboot; initiating factory reset");
        InitiateFactoryReset();
    }
    err = WEAVE_NO_ERROR;

exit:
    return err;
}

WEAVE_ERROR ConfigurationManagerImpl<ESP32>::_ConfigureWeaveStack()
{
    WEAVE_ERROR err;
    nvs_handle handle;
    bool needClose = false;
    size_t pairingCodeLen;

    // Open the weave-factory namespace.
    err = nvs_open(kNVSNamespace_WeaveFactory, NVS_READONLY, &handle);
    SuccessOrExit(err);
    needClose = true;

    // Read the device id from NVS.  For the convenience of manufacturing, the value
    // is expected to be stored as an 8-byte blob in big-endian format, rather than a
    // u64 value.
    {
        uint8_t deviceIdBytes[sizeof(uint64_t)];
        size_t deviceIdLen = sizeof(deviceIdBytes);
        err = nvs_get_blob(handle, NVSKeys::DeviceId.Name, deviceIdBytes, &deviceIdLen);
#if WEAVE_DEVICE_CONFIG_ENABLE_TEST_DEVICE_IDENTITY
        if (err == ESP_ERR_NVS_NOT_FOUND)
        {
            WeaveLogProgress(DeviceLayer, "Device id not found in nvs; using hard-coded default: %" PRIX64, TestDeviceId);
            FabricState.LocalNodeId = TestDeviceId;
            err = WEAVE_NO_ERROR;
        }
        else
#endif // WEAVE_DEVICE_CONFIG_ENABLE_TEST_DEVICE_IDENTITY
        {
            SuccessOrExit(err);
            VerifyOrExit(deviceIdLen == sizeof(deviceIdBytes), err = ESP_ERR_NVS_INVALID_LENGTH);
            FabricState.LocalNodeId = Encoding::BigEndian::Get64(deviceIdBytes);
        }
    }

    // Read the pairing code from NVS.
    pairingCodeLen = sizeof(mPairingCode);
    err = nvs_get_str(handle, kNVSKeyName_PairingCode, mPairingCode, &pairingCodeLen);
#ifdef CONFIG_USE_TEST_PAIRING_CODE
    if (CONFIG_USE_TEST_PAIRING_CODE[0] != 0 && err == ESP_ERR_NVS_NOT_FOUND)
    {
        WeaveLogProgress(DeviceLayer, "Pairing code not found in nvs; using hard-coded default: %s", CONFIG_USE_TEST_PAIRING_CODE);
        memcpy(mPairingCode, CONFIG_USE_TEST_PAIRING_CODE, min(sizeof(mPairingCode) - 1, sizeof(CONFIG_USE_TEST_PAIRING_CODE)));
        mPairingCode[sizeof(mPairingCode) - 1] = 0;
        err = WEAVE_NO_ERROR;
    }
#endif // CONFIG_USE_TEST_PAIRING_CODE
    SuccessOrExit(err);

    FabricState.PairingCode = mPairingCode;

    nvs_close(handle);
    needClose = false;

    // Open the weave-config namespace.
    err = nvs_open(kNVSNamespace_WeaveConfig, NVS_READONLY, &handle);
    SuccessOrExit(err);
    needClose = true;

    // Read the fabric id from NVS.  If not present, then the device is not currently a
    // member of a Weave fabric.
    err = nvs_get_u64(handle, kNVSKeyName_FabricId, &FabricState.FabricId);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        FabricState.FabricId = kFabricIdNotSpecified;
        err = WEAVE_NO_ERROR;
    }
    SuccessOrExit(err);

    // Determine whether the device is currently service provisioned.
    {
        size_t l;
        bool isServiceProvisioned = (nvs_get_blob(handle, kNVSKeyName_ServiceConfig, NULL, &l) != ESP_ERR_NVS_NOT_FOUND);
        SetFlag(mFlags, kFlag_IsServiceProvisioned, isServiceProvisioned);
    }

    // Determine whether the device is currently paired to an account.
    {
        size_t l;
        bool isPairedToAccount = (nvs_get_str(handle, kNVSKeyName_PairedAccountId, NULL, &l) != ESP_ERR_NVS_NOT_FOUND);
        SetFlag(mFlags, kFlag_IsPairedToAccount, isPairedToAccount);
    }

    // Configure the FabricState object with a reference to the GroupKeyStore object.
    // TODO: remove GetGroupKeyStore() from "internal" interface.
    FabricState.GroupKeyStore = _GetGroupKeyStore();

#if WEAVE_PROGRESS_LOGGING
    LogDeviceConfig();
#endif

exit:
    if (needClose)
    {
        nvs_close(handle);
    }
    return err;
}

::nl::Weave::Profiles::Security::AppKeys::GroupKeyStoreBase * ConfigurationManagerImpl<ESP32>::_GetGroupKeyStore()
{
    return &gGroupKeyStore;
}

bool ConfigurationManagerImpl<ESP32>::_CanFactoryReset()
{
    // TODO: query the application to determine if factory reset is allowed.
    return true;
}

WEAVE_ERROR ConfigurationManagerImpl<ESP32>::_SetFailSafeArmed()
{
    return StoreNVS(kNVSNamespace_WeaveConfig, kNVSKeyName_FailSafeArmed, (uint32_t)1);
}

WEAVE_ERROR ConfigurationManagerImpl<ESP32>::_ClearFailSafeArmed()
{
    return ClearNVSKey(kNVSNamespace_WeaveConfig, kNVSKeyName_FailSafeArmed);
}

WEAVE_ERROR ConfigurationManagerImpl<ESP32>::_GetWiFiStationSecurityType(Profiles::NetworkProvisioning::WiFiSecurityType & secType)
{
    WEAVE_ERROR err;
    uint32_t secTypeInt;

    err = GetNVS(kNVSNamespace_WeaveConfig, kNVSKeyName_WiFiStationSecType, secTypeInt);
    if (err == WEAVE_NO_ERROR)
    {
        secType = (Profiles::NetworkProvisioning::WiFiSecurityType)secTypeInt;
    }
    return err;
}

WEAVE_ERROR ConfigurationManagerImpl<ESP32>::_UpdateWiFiStationSecurityType(Profiles::NetworkProvisioning::WiFiSecurityType secType)
{
    WEAVE_ERROR err;
    Profiles::NetworkProvisioning::WiFiSecurityType curSecType;

    if (secType != Profiles::NetworkProvisioning::kWiFiSecurityType_NotSpecified)
    {
        err = _GetWiFiStationSecurityType(curSecType);
        if (err == WEAVE_DEVICE_ERROR_CONFIG_NOT_FOUND || (err == WEAVE_NO_ERROR && secType != curSecType))
        {
            uint32_t secTypeInt = secType;
            err = StoreNVS(kNVSNamespace_WeaveConfig, kNVSKeyName_WiFiStationSecType, secTypeInt);
        }
        SuccessOrExit(err);
    }
    else
    {
        err = ClearNVSKey(kNVSNamespace_WeaveConfig, kNVSKeyName_WiFiStationSecType);
        SuccessOrExit(err);
    }

exit:
    return err;
}

WEAVE_ERROR ConfigurationManagerImpl<ESP32>::_ReadPersistedStorageValue(::nl::Weave::Platform::PersistedStorage::Key key, uint32_t & value)
{
    WEAVE_ERROR err = GetNVS(kNVSNamespace_WeaveCounters, key, value);
    if (err == WEAVE_DEVICE_ERROR_CONFIG_NOT_FOUND)
    {
        err = WEAVE_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND;
    }
    return err;
}

WEAVE_ERROR ConfigurationManagerImpl<ESP32>::_WritePersistedStorageValue(::nl::Weave::Platform::PersistedStorage::Key key, uint32_t value)
{
    return StoreNVS(kNVSNamespace_WeaveCounters, key, value);
}


// ==================== Configuration Manager Private Methods ====================

#if WEAVE_PROGRESS_LOGGING

void ConfigurationManagerImpl<ESP32>::LogDeviceConfig()
{
    WEAVE_ERROR err;

    WeaveLogProgress(DeviceLayer, "Device Configuration:");

    WeaveLogProgress(DeviceLayer, "  Device Id: %016" PRIX64, FabricState.LocalNodeId);

    {
        char serialNum[kMaxSerialNumberLength];
        size_t serialNumLen;
        err = GetSerialNumber(serialNum, sizeof(serialNum), serialNumLen);
        WeaveLogProgress(DeviceLayer, "  Serial Number: %s", (err == WEAVE_NO_ERROR) ? serialNum : "(not set)");
    }

    {
        uint16_t vendorId;
        if (GetVendorId(vendorId) != WEAVE_NO_ERROR)
        {
            vendorId = 0;
        }
        WeaveLogProgress(DeviceLayer, "  Vendor Id: %" PRId16 " (0x%" PRIX16 ")%s",
                vendorId, vendorId, (vendorId == kWeaveVendor_NestLabs) ? " (Nest)" : "");
    }

    {
        uint16_t productId;
        if (GetProductId(productId) != WEAVE_NO_ERROR)
        {
            productId = 0;
        }
        WeaveLogProgress(DeviceLayer, "  Product Id: %" PRId16 " (0x%" PRIX16 ")", productId, productId);
    }

    if (FabricState.FabricId != kFabricIdNotSpecified)
    {
        WeaveLogProgress(DeviceLayer, "  Fabric Id: %" PRIX64, FabricState.FabricId);
    }
    else
    {
        WeaveLogProgress(DeviceLayer, "  Fabric Id: (none)");
    }

    WeaveLogProgress(DeviceLayer, "  Pairing Code: %s", mPairingCode);
}

#endif // WEAVE_PROGRESS_LOGGING

void ConfigurationManagerImpl<ESP32>::DoFactoryReset(intptr_t arg)
{
    WEAVE_ERROR err;

    WeaveLogProgress(DeviceLayer, "Performing factory reset");

    // Erase all values in the weave-config NVS namespace.
    err = ClearNVSNamespace(kNVSNamespace_WeaveConfig);
    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogError(DeviceLayer, "ClearNVSNamespace(WeaveConfig) failed: %s", nl::ErrorStr(err));
    }

    // Restore WiFi persistent settings to default values.
    err = esp_wifi_restore();
    if (err != ESP_OK)
    {
        WeaveLogError(DeviceLayer, "esp_wifi_restore() failed: %s", nl::ErrorStr(err));
    }

    // Restart the system.
    WeaveLogProgress(DeviceLayer, "System restarting");
    esp_restart();
}

} // namespace DeviceLayer
} // namespace Weave
} // namespace nl


#include "GenericConfigurationManagerImpl.cpp"

namespace nl {
namespace Weave {
namespace DeviceLayer {
namespace Internal {
template class GenericIdentityConfigurationImpl<ConfigurationManagerImpl<ESP32>, ESP32NVSKeys>;
} // namespace Internal
} // namespace DeviceLayer
} // namespace Weave
} // namespace nl
