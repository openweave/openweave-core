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

#include <internal/WeaveDeviceInternal.h>
#include <ConfigurationManager.h>
#include <Weave/Core/WeaveKeyIds.h>
#include <Weave/Core/WeaveVendorIdentifiers.hpp>
#include <Weave/Profiles/security/WeaveApplicationKeys.h>
#include <Weave/Profiles/vendor/nestlabs/device-description/NestProductIdentifiers.hpp>

#include "esp_wifi.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <new>

using namespace ::nl;
using namespace ::nl::Weave;
using namespace ::nl::Weave::Profiles::Security::AppKeys;
using namespace ::nl::Weave::Profiles::DeviceDescription;
using namespace ::nl::Weave::Device::Internal;

using ::nl::Weave::kWeaveVendor_NestLabs;

namespace nl {
namespace Weave {
namespace Device {

namespace {

enum
{
    kNestWeaveProduct_Connect = 0x0016
};

enum
{
    kMaxGroupKeys = WEAVE_CONFIG_MAX_APPLICATION_EPOCH_KEYS +       // Maximum number of Epoch keys
                    WEAVE_CONFIG_MAX_APPLICATION_GROUPS +           // Maximum number of Application Group Master keys
                    1 +                                             // Maximum number of Root keys (1 for Service root key)
                    1                                               // Fabric secret
};


class GroupKeyStore : public ::nl::Weave::Profiles::Security::AppKeys::GroupKeyStoreBase
{
public:
    virtual WEAVE_ERROR RetrieveGroupKey(uint32_t keyId, WeaveGroupKey & key);
    virtual WEAVE_ERROR StoreGroupKey(const WeaveGroupKey & key);
    virtual WEAVE_ERROR DeleteGroupKey(uint32_t keyId);
    virtual WEAVE_ERROR DeleteGroupKeysOfAType(uint32_t keyType);
    virtual WEAVE_ERROR EnumerateGroupKeys(uint32_t keyType, uint32_t * keyIds, uint8_t keyIdsArraySize, uint8_t & keyCount);
    virtual WEAVE_ERROR Clear(void);
    virtual WEAVE_ERROR RetrieveLastUsedEpochKeyId(void);
    virtual WEAVE_ERROR StoreLastUsedEpochKeyId(void);

private:

    // NOTE: These members are for internal use by the following friend classes.

    friend class ::nl::Weave::Device::ConfigurationManager;

    WEAVE_ERROR Init();

private:
    uint32_t mKeyIndex[kMaxGroupKeys];
    uint8_t mNumKeys;

    WEAVE_ERROR AddKeyToIndex(uint32_t keyId, bool & indexUpdated);
    WEAVE_ERROR WriteKeyIndex(nvs_handle handle);
    WEAVE_ERROR DeleteKeyOrKeys(uint32_t targetKeyId, uint32_t targetKeyType);

    static WEAVE_ERROR FormKeyName(uint32_t keyId, char * buf, size_t bufSize);
};

GroupKeyStore gGroupKeyStore;

const char kNVSNamespace_WeaveFactory[]      = "weave-factory";
const char kNVSNamespace_WeaveConfig[]       = "weave-config";
const char kNVSNamespace_WeaveCounters[]     = "weave-counters";

// weave-factory keys
const char kNVSKeyName_SerialNum[]           = "serial-num";
const char kNVSKeyName_ManufacturingDate[]   = "mfg-date";
const char kNVSKeyName_PairingCode[]         = "pairing-code";
const char kNVSKeyName_DeviceId[]            = "device-id";
const char kNVSKeyName_DeviceCert[]          = "device-cert";
const char kNVSKeyName_DevicePrivateKey[]    = "device-key";

// weave-config keys
const char kNVSKeyName_FabricId[]            = "fabric-id";
const char kNVSKeyName_ServiceConfig[]       = "service-config";
const char kNVSKeyName_PairedAccountId[]     = "account-id";
const char kNVSKeyName_ServiceId[]           = "service-id";
const char kNVSKeyName_FabricSecret[]        = "fabric-secret";
const char kNVSKeyName_GroupKeyIndex[]       = "group-key-index";
const char kNVSKeyName_GroupKeyPrefix[]      = "gk-";
const char kNVSKeyName_LastUsedEpochKeyId[]  = "last-ek-id";
const char kNVSKeyName_FailSafeArmed[]       = "fail-safe-armed";
const char kNVSKeyName_WiFiStationSecType[]  = "sta-sec-type";

const size_t kMaxGroupKeyNameLength = max(sizeof(kNVSKeyName_FabricSecret), sizeof(kNVSKeyName_GroupKeyPrefix) + 8);

WEAVE_ERROR GetNVS(const char * ns, const char * name, uint8_t * buf, size_t bufSize, size_t & outLen);
WEAVE_ERROR GetNVS(const char * ns, const char * name, char * buf, size_t bufSize, size_t & outLen);
WEAVE_ERROR GetNVS(const char * ns, const char * name, uint32_t & val);
WEAVE_ERROR GetNVS(const char * ns, const char * name, uint64_t & val);
WEAVE_ERROR StoreNVS(const char * ns, const char * name, const uint8_t * data, size_t dataLen);
WEAVE_ERROR StoreNVS(const char * ns, const char * name, const char * data);
WEAVE_ERROR StoreNVS(const char * ns, const char * name, uint32_t val);
WEAVE_ERROR StoreNVS(const char * ns, const char * name, uint64_t val);
WEAVE_ERROR ClearNVSKey(const char * ns, const char * name);
WEAVE_ERROR ClearNVSNamespace(const char * ns);
WEAVE_ERROR EnsureNamespace(const char * ns);

} // unnamed namespace


// ==================== Configuration Manager Public Methods ====================

WEAVE_ERROR ConfigurationManager::GetVendorId(uint16_t& vendorId)
{
    vendorId = (uint16_t)WEAVE_DEVICE_CONFIG_DEVICE_VENDOR_ID;
    return WEAVE_NO_ERROR;
}

WEAVE_ERROR ConfigurationManager::GetProductId(uint16_t& productId)
{
    productId = (uint16_t)WEAVE_DEVICE_CONFIG_DEVICE_PRODUCT_ID;
    return WEAVE_NO_ERROR;
}

WEAVE_ERROR ConfigurationManager::GetProductRevision(uint16_t& productRev)
{
    productRev = (uint16_t)WEAVE_DEVICE_CONFIG_DEVICE_PRODUCT_REVISION;
    return WEAVE_NO_ERROR;
}

WEAVE_ERROR ConfigurationManager::GetSerialNumber(char * buf, size_t bufSize, size_t & serialNumLen)
{
    return GetNVS(kNVSNamespace_WeaveFactory, kNVSKeyName_SerialNum, buf, bufSize, serialNumLen);
}

WEAVE_ERROR ConfigurationManager::GetManufacturingDate(uint16_t& year, uint8_t& month, uint8_t& dayOfMonth)
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

WEAVE_ERROR ConfigurationManager::GetFirmwareRevision(char * buf, size_t bufSize, size_t & outLen)
{
#ifdef WEAVE_DEVICE_CONFIG_DEVICE_FIRMWARE_REVISION
    if (WEAVE_DEVICE_CONFIG_DEVICE_FIRMWARE_REVISION[0] != 0)
    {
        outLen = min(bufSize, sizeof(WEAVE_DEVICE_CONFIG_DEVICE_FIRMWARE_REVISION) - 1);
        memcpy(buf, WEAVE_DEVICE_CONFIG_DEVICE_FIRMWARE_REVISION, outLen);
        return WEAVE_NO_ERROR;
    }
    else
#endif // WEAVE_DEVICE_CONFIG_DEVICE_FIRMWARE_REVISION
    {
        outLen = 0;
        return WEAVE_DEVICE_ERROR_CONFIG_NOT_FOUND;
    }
}

WEAVE_ERROR ConfigurationManager::GetFirmwareBuildTime(uint16_t & year, uint8_t & month, uint8_t & dayOfMonth,
        uint8_t & hour, uint8_t & minute, uint8_t & second)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    const char * buildDateStr = __DATE__; // e.g. Feb 12 1996
    const char * buildTimeStr = __TIME__; // e.g. 23:59:01
    char monthStr[4];
    char * p;

    static const char months[] = "JanFebMarAprMayJunJulAugSepOctNovDec";

    memcpy(monthStr, buildDateStr, 3);
    monthStr[3] = 0;

    p = strstr(months, monthStr);
    VerifyOrExit(p != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    month = ((p - months) / 3) + 1;

    dayOfMonth = strtoul(buildDateStr + 4, &p, 10);
    VerifyOrExit(p == buildDateStr + 6, err = WEAVE_ERROR_INVALID_ARGUMENT);

    year = strtoul(buildDateStr + 7, &p, 10);
    VerifyOrExit(p == buildDateStr + 11, err = WEAVE_ERROR_INVALID_ARGUMENT);

    hour = strtoul(buildTimeStr, &p, 10);
    VerifyOrExit(p == buildTimeStr + 2, err = WEAVE_ERROR_INVALID_ARGUMENT);

    minute = strtoul(buildTimeStr + 3, &p, 10);
    VerifyOrExit(p == buildTimeStr + 5, err = WEAVE_ERROR_INVALID_ARGUMENT);

    second = strtoul(buildTimeStr + 6, &p, 10);
    VerifyOrExit(p == buildTimeStr + 8, err = WEAVE_ERROR_INVALID_ARGUMENT);

exit:
    return err;
}

WEAVE_ERROR ConfigurationManager::GetDeviceCertificate(uint8_t * buf, size_t bufSize, size_t & certLen)
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

WEAVE_ERROR ConfigurationManager::GetDeviceCertificateLength(size_t & certLen)
{
    WEAVE_ERROR err = GetDeviceCertificate((uint8_t *)NULL, 0, certLen);
    if (err == WEAVE_ERROR_BUFFER_TOO_SMALL)
    {
        err = WEAVE_NO_ERROR;
    }
    return err;
}

WEAVE_ERROR ConfigurationManager::GetDevicePrivateKey(uint8_t * buf, size_t bufSize, size_t & keyLen)
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

WEAVE_ERROR ConfigurationManager::GetDevicePrivateKeyLength(size_t & keyLen)
{
    WEAVE_ERROR err = GetDevicePrivateKey((uint8_t *)NULL, 0, keyLen);
    if (err == WEAVE_ERROR_BUFFER_TOO_SMALL)
    {
        err = WEAVE_NO_ERROR;
    }
    return err;
}

WEAVE_ERROR ConfigurationManager::GetServiceConfig(uint8_t * buf, size_t bufSize, size_t & serviceConfigLen)
{
    return GetNVS(kNVSNamespace_WeaveConfig, kNVSKeyName_ServiceConfig, buf, bufSize, serviceConfigLen);
}

WEAVE_ERROR ConfigurationManager::GetServiceConfigLength(size_t & serviceConfigLen)
{
    WEAVE_ERROR err = GetServiceConfig((uint8_t *)NULL, 0, serviceConfigLen);
    if (err == WEAVE_ERROR_BUFFER_TOO_SMALL)
    {
        err = WEAVE_NO_ERROR;
    }
    return err;
}

WEAVE_ERROR ConfigurationManager::GetServiceId(uint64_t & serviceId)
{
    return GetNVS(kNVSNamespace_WeaveConfig, kNVSKeyName_ServiceId, serviceId);
}

WEAVE_ERROR ConfigurationManager::GetPairedAccountId(char * buf, size_t bufSize, size_t accountIdLen)
{
    return GetNVS(kNVSNamespace_WeaveConfig, kNVSKeyName_PairedAccountId, buf, bufSize, accountIdLen);
}

WEAVE_ERROR ConfigurationManager::StoreDeviceId(uint64_t deviceId)
{
    return (deviceId != kNodeIdNotSpecified)
           ? StoreNVS(kNVSNamespace_WeaveFactory, kNVSKeyName_DeviceId, deviceId)
           : ClearNVSKey(kNVSNamespace_WeaveFactory, kNVSKeyName_DeviceId);
}

WEAVE_ERROR ConfigurationManager::StoreSerialNumber(const char * serialNum)
{
    return StoreNVS(kNVSNamespace_WeaveFactory, kNVSKeyName_SerialNum, serialNum);
}

WEAVE_ERROR ConfigurationManager::StoreManufacturingDate(const char * mfgDate)
{
    return StoreNVS(kNVSNamespace_WeaveFactory, kNVSKeyName_ManufacturingDate, mfgDate);
}

WEAVE_ERROR ConfigurationManager::StoreFabricId(uint64_t fabricId)
{
    return (fabricId != kFabricIdNotSpecified)
           ? StoreNVS(kNVSNamespace_WeaveConfig, kNVSKeyName_FabricId, fabricId)
           : ClearNVSKey(kNVSNamespace_WeaveConfig, kNVSKeyName_FabricId);
}

WEAVE_ERROR ConfigurationManager::StoreDeviceCertificate(const uint8_t * cert, size_t certLen)
{
    return StoreNVS(kNVSNamespace_WeaveFactory, kNVSKeyName_DeviceCert, cert, certLen);
}

WEAVE_ERROR ConfigurationManager::StoreDevicePrivateKey(const uint8_t * key, size_t keyLen)
{
    return StoreNVS(kNVSNamespace_WeaveFactory, kNVSKeyName_DevicePrivateKey, key, keyLen);
}

WEAVE_ERROR ConfigurationManager::StorePairingCode(const char * pairingCode)
{
    return StoreNVS(kNVSNamespace_WeaveFactory, kNVSKeyName_PairingCode, pairingCode);
}

WEAVE_ERROR ConfigurationManager::StoreServiceProvisioningData(uint64_t serviceId,
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

WEAVE_ERROR ConfigurationManager::ClearServiceProvisioningData()
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

WEAVE_ERROR ConfigurationManager::StoreServiceConfig(const uint8_t * serviceConfig, size_t serviceConfigLen)
{
    return StoreNVS(kNVSNamespace_WeaveConfig, kNVSKeyName_ServiceConfig, serviceConfig, serviceConfigLen);
}

WEAVE_ERROR ConfigurationManager::StoreAccountId(const char * accountId, size_t accountIdLen)
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

WEAVE_ERROR ConfigurationManager::GetPersistedCounter(const char * key, uint32_t & value)
{
    WEAVE_ERROR err = GetNVS(kNVSNamespace_WeaveCounters, key, value);
    if (err == WEAVE_DEVICE_ERROR_CONFIG_NOT_FOUND)
    {
        err = WEAVE_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND;
    }
    return err;
}

WEAVE_ERROR ConfigurationManager::StorePersistedCounter(const char * key, uint32_t value)
{
    return StoreNVS(kNVSNamespace_WeaveCounters, key, value);
}

WEAVE_ERROR ConfigurationManager::GetDeviceDescriptor(WeaveDeviceDescriptor & deviceDesc)
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

WEAVE_ERROR ConfigurationManager::GetDeviceDescriptorTLV(uint8_t * buf, size_t bufSize, size_t & encodedLen)
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

WEAVE_ERROR ConfigurationManager::GetQRCodeString(char * buf, size_t bufSize)
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


WEAVE_ERROR ConfigurationManager::GetWiFiAPSSID(char * buf, size_t bufSize)
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

bool ConfigurationManager::IsMemberOfFabric()
{
    return FabricState.FabricId != ::nl::Weave::kFabricIdNotSpecified;
}

void ConfigurationManager::InitiateFactoryReset()
{
    PlatformMgr.ScheduleWork(DoFactoryReset);
}

// ==================== Configuration Manager Private Methods ====================

WEAVE_ERROR ConfigurationManager::Init()
{
    WEAVE_ERROR err;
    uint32_t failSafeArmed;

    mFlags = 0;

    // Force initialization of weave NVS namespaces if they doesn't already exist.
    err = EnsureNamespace(kNVSNamespace_WeaveFactory);
    SuccessOrExit(err);
    err = EnsureNamespace(kNVSNamespace_WeaveConfig);
    SuccessOrExit(err);
    err = EnsureNamespace(kNVSNamespace_WeaveCounters);
    SuccessOrExit(err);

    // Initialize the global GroupKeyStore object.
    new ((void *)&gGroupKeyStore) GroupKeyStore();
    err = gGroupKeyStore.Init();
    SuccessOrExit(err);

    // If the fail-safe was armed when the device last shutdown, initiate a factory reset.
    if (GetNVS(kNVSNamespace_WeaveConfig, kNVSKeyName_FailSafeArmed, failSafeArmed) == WEAVE_NO_ERROR &&
        failSafeArmed != 0)
    {
        WeaveLogProgress(DeviceLayer, "Detected fail-safe armed on reboot; initiating factory reset");
        InitiateFactoryReset();
    }
    err = WEAVE_NO_ERROR;

exit:
    return err;
}

WEAVE_ERROR ConfigurationManager::ConfigureWeaveStack()
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
        uint8_t nodeIdBytes[sizeof(uint64_t)];
        size_t nodeIdLen = sizeof(nodeIdBytes);
        err = nvs_get_blob(handle, kNVSKeyName_DeviceId, nodeIdBytes, &nodeIdLen);
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
            VerifyOrExit(nodeIdLen == sizeof(nodeIdBytes), err = ESP_ERR_NVS_INVALID_LENGTH);
            FabricState.LocalNodeId = Encoding::BigEndian::Get64(nodeIdBytes);
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
    FabricState.GroupKeyStore = GetGroupKeyStore();

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

::nl::Weave::Profiles::Security::AppKeys::GroupKeyStoreBase * ConfigurationManager::GetGroupKeyStore()
{
    return &gGroupKeyStore;
}

bool ConfigurationManager::CanFactoryReset()
{
    // TODO: query the application to determine if factory reset is allowed.
    return true;
}

WEAVE_ERROR ConfigurationManager::SetFailSafeArmed()
{
    return StoreNVS(kNVSNamespace_WeaveConfig, kNVSKeyName_FailSafeArmed, (uint32_t)1);
}

WEAVE_ERROR ConfigurationManager::ClearFailSafeArmed()
{
    return ClearNVSKey(kNVSNamespace_WeaveConfig, kNVSKeyName_FailSafeArmed);
}

WEAVE_ERROR ConfigurationManager::GetWiFiStationSecurityType(Profiles::NetworkProvisioning::WiFiSecurityType & secType)
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

WEAVE_ERROR ConfigurationManager::UpdateWiFiStationSecurityType(Profiles::NetworkProvisioning::WiFiSecurityType secType)
{
    WEAVE_ERROR err;
    Profiles::NetworkProvisioning::WiFiSecurityType curSecType;

    if (secType != Profiles::NetworkProvisioning::kWiFiSecurityType_NotSpecified)
    {
        err = GetWiFiStationSecurityType(curSecType);
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


#if WEAVE_PROGRESS_LOGGING

void ConfigurationManager::LogDeviceConfig()
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

void ConfigurationManager::DoFactoryReset(intptr_t arg)
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

namespace {

// ==================== Group Key Store Implementation ====================

WEAVE_ERROR GroupKeyStore::RetrieveGroupKey(uint32_t keyId, WeaveGroupKey & key)
{
    WEAVE_ERROR err;
    size_t keyLen;
    char keyName[kMaxGroupKeyNameLength + 1];

    err = FormKeyName(keyId, keyName, sizeof(keyName));
    SuccessOrExit(err);

    err = GetNVS(kNVSNamespace_WeaveConfig, keyName, key.Key, sizeof(key.Key), keyLen);
    if (err == WEAVE_DEVICE_ERROR_CONFIG_NOT_FOUND)
    {
        err = WEAVE_ERROR_KEY_NOT_FOUND;
    }
    SuccessOrExit(err);

    if (keyId != WeaveKeyId::kFabricSecret)
    {
    	memcpy(&key.StartTime, key.Key + kWeaveAppGroupKeySize, sizeof(uint32_t));
    	keyLen -= sizeof(uint32_t);
    }

    key.KeyId = keyId;
    key.KeyLen = keyLen;

exit:
    return err;
}

WEAVE_ERROR GroupKeyStore::StoreGroupKey(const WeaveGroupKey & key)
{
    WEAVE_ERROR err;
    nvs_handle handle;
    char keyName[kMaxGroupKeyNameLength + 1];
    uint8_t keyData[WeaveGroupKey::MaxKeySize];
    bool needClose = false;
    bool indexUpdated = false;

    err = FormKeyName(key.KeyId, keyName, sizeof(keyName));
    SuccessOrExit(err);

    err = AddKeyToIndex(key.KeyId, indexUpdated);
    SuccessOrExit(err);

    err = nvs_open(kNVSNamespace_WeaveConfig, NVS_READWRITE, &handle);
    SuccessOrExit(err);
    needClose = true;

    memcpy(keyData, key.Key, WeaveGroupKey::MaxKeySize);
    if (key.KeyId != WeaveKeyId::kFabricSecret)
    {
        memcpy(keyData + kWeaveAppGroupKeySize, (const void *)&key.StartTime, sizeof(uint32_t));
    }

    if (LOG_LOCAL_LEVEL >= ESP_LOG_INFO)
    {
        if (WeaveKeyId::IsAppEpochKey(key.KeyId))
        {
            WeaveLogProgress(DeviceLayer, "GroupKeyStore: storing epoch key %s/%s (key len %" PRId8 ", start time %" PRIu32 ")",
                    kNVSNamespace_WeaveConfig, keyName, key.KeyLen, key.StartTime);
        }
        else if (WeaveKeyId::IsAppGroupMasterKey(key.KeyId))
        {
            WeaveLogProgress(DeviceLayer, "GroupKeyStore: storing app master key %s/%s (key len %" PRId8 ", global id 0x%" PRIX32 ")",
                    kNVSNamespace_WeaveConfig, keyName, key.KeyLen, key.GlobalId);
        }
        else
        {
            const char * keyType = (WeaveKeyId::IsAppRootKey(key.KeyId)) ? "root": "general";
            WeaveLogProgress(DeviceLayer, "GroupKeyStore: storing %s key %s/%s (key len %" PRId8 ")", keyType, kNVSNamespace_WeaveConfig, keyName, key.KeyLen);
        }
    }

    err = nvs_set_blob(handle, keyName, keyData, WeaveGroupKey::MaxKeySize);
    SuccessOrExit(err);

    if (indexUpdated)
    {
        err = WriteKeyIndex(handle);
        SuccessOrExit(err);
    }

    // Commit the value to the persistent store.
    err = nvs_commit(handle);
    SuccessOrExit(err);

exit:
	if (needClose)
	{
		nvs_close(handle);
	}
	if (err != WEAVE_NO_ERROR && indexUpdated)
	{
	    mNumKeys--;
	}
	ClearSecretData(keyData, sizeof(keyData));
    return err;
}

WEAVE_ERROR GroupKeyStore::DeleteGroupKey(uint32_t keyId)
{
    return DeleteKeyOrKeys(keyId, WeaveKeyId::kType_None);
}

WEAVE_ERROR GroupKeyStore::DeleteGroupKeysOfAType(uint32_t keyType)
{
    return DeleteKeyOrKeys(WeaveKeyId::kNone, keyType);
}

WEAVE_ERROR GroupKeyStore::EnumerateGroupKeys(uint32_t keyType, uint32_t * keyIds,
        uint8_t keyIdsArraySize, uint8_t & keyCount)
{
    keyCount = 0;

    for (uint8_t i = 0; i < mNumKeys && keyCount < keyIdsArraySize; i++)
    {
        if (keyType == WeaveKeyId::kType_None || WeaveKeyId::GetType(mKeyIndex[i]) == keyType)
        {
            keyIds[keyCount++] = mKeyIndex[i];
        }
    }

    return WEAVE_NO_ERROR;
}

WEAVE_ERROR GroupKeyStore::Clear(void)
{
    return DeleteKeyOrKeys(WeaveKeyId::kNone, WeaveKeyId::kType_None);
}

WEAVE_ERROR GroupKeyStore::RetrieveLastUsedEpochKeyId(void)
{
    WEAVE_ERROR err;

    err = GetNVS(kNVSNamespace_WeaveConfig, kNVSKeyName_LastUsedEpochKeyId, LastUsedEpochKeyId);
    if (err == WEAVE_DEVICE_ERROR_CONFIG_NOT_FOUND)
    {
        LastUsedEpochKeyId = WeaveKeyId::kNone;
        err = WEAVE_NO_ERROR;
    }
    return err;
}

WEAVE_ERROR GroupKeyStore::StoreLastUsedEpochKeyId(void)
{
    return StoreNVS(kNVSNamespace_WeaveConfig, kNVSKeyName_LastUsedEpochKeyId, LastUsedEpochKeyId);
}

WEAVE_ERROR GroupKeyStore::Init()
{
    WEAVE_ERROR err;
    size_t indexSizeBytes;

    err = GetNVS(kNVSNamespace_WeaveConfig, kNVSKeyName_GroupKeyIndex, (uint8_t *)mKeyIndex, sizeof(mKeyIndex), indexSizeBytes);
    if (err == WEAVE_DEVICE_ERROR_CONFIG_NOT_FOUND)
    {
        err = WEAVE_NO_ERROR;
        indexSizeBytes = 0;
    }
    SuccessOrExit(err);

    mNumKeys = indexSizeBytes / sizeof(uint32_t);

exit:
    return err;
}

WEAVE_ERROR GroupKeyStore::AddKeyToIndex(uint32_t keyId, bool & indexUpdated)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    indexUpdated = false;

    for (uint8_t i = 0; i < mNumKeys; i++)
    {
        if (mKeyIndex[i] == keyId)
        {
            ExitNow(err = WEAVE_NO_ERROR);
        }
    }

    VerifyOrExit(mNumKeys < kMaxGroupKeys, err = WEAVE_ERROR_TOO_MANY_KEYS);

    mKeyIndex[mNumKeys++] = keyId;
    indexUpdated = true;

exit:
    return err;
}

WEAVE_ERROR GroupKeyStore::WriteKeyIndex(nvs_handle handle)
{
    WeaveLogProgress(DeviceLayer, "GroupKeyStore: writing key index %s/%s (num keys %" PRIu8 ")", kNVSNamespace_WeaveConfig, kNVSKeyName_GroupKeyIndex, mNumKeys);
    return nvs_set_blob(handle, kNVSKeyName_GroupKeyIndex, mKeyIndex, mNumKeys * sizeof(uint32_t));
}

WEAVE_ERROR GroupKeyStore::DeleteKeyOrKeys(uint32_t targetKeyId, uint32_t targetKeyType)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    nvs_handle handle;
    char keyName[kMaxGroupKeyNameLength + 1];
    bool needClose = false;

    for (uint8_t i = 0; i < mNumKeys; )
    {
        uint32_t curKeyId = mKeyIndex[i];

        if ((targetKeyId == WeaveKeyId::kNone && targetKeyType == WeaveKeyId::kType_None) ||
            curKeyId == targetKeyId ||
            WeaveKeyId::GetType(curKeyId) == targetKeyType)
        {
            if (!needClose)
            {
                err = nvs_open(kNVSNamespace_WeaveConfig, NVS_READWRITE, &handle);
                SuccessOrExit(err);
                needClose = true;
            }

            err = FormKeyName(curKeyId, keyName, sizeof(keyName));
            SuccessOrExit(err);

            err = nvs_erase_key(handle, keyName);
            if (err == ESP_OK && LOG_LOCAL_LEVEL >= ESP_LOG_INFO)
            {
                const char * keyType;
                if (WeaveKeyId::IsAppRootKey(curKeyId))
                {
                    keyType = "root";
                }
                else if (WeaveKeyId::IsAppGroupMasterKey(curKeyId))
                {
                    keyType = "app master";
                }
                else if (WeaveKeyId::IsAppEpochKey(curKeyId))
                {
                    keyType = "epoch";
                }
                else
                {
                    keyType = "general";
                }
                WeaveLogProgress(DeviceLayer, "GroupKeyStore: erasing %s key %s/%s", keyType, kNVSNamespace_WeaveConfig, keyName);
            }
            else if (err == ESP_ERR_NVS_NOT_FOUND)
            {
                err = WEAVE_NO_ERROR;
            }
            SuccessOrExit(err);

            mNumKeys--;

            memmove(&mKeyIndex[i], &mKeyIndex[i+1], (mNumKeys - i) * sizeof(uint32_t));
        }
        else
        {
            i++;
        }
    }

    if (needClose)
    {
        err = WriteKeyIndex(handle);
        SuccessOrExit(err);

        // Commit to the persistent store.
        err = nvs_commit(handle);
        SuccessOrExit(err);
    }

exit:
    if (needClose)
    {
        nvs_close(handle);
    }
    return err;
}

WEAVE_ERROR GroupKeyStore::FormKeyName(uint32_t keyId, char * buf, size_t bufSize)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(bufSize >= kMaxGroupKeyNameLength, err = WEAVE_ERROR_BUFFER_TOO_SMALL);

    if (keyId == WeaveKeyId::kFabricSecret)
    {
        strcpy(buf, kNVSKeyName_FabricSecret);
    }
    else
    {
        snprintf(buf, bufSize, "%s%08" PRIX32, kNVSKeyName_GroupKeyPrefix, keyId);
    }

exit:
    return err;
}


// ==================== Utility Functions for accessing ESP NVS ====================

WEAVE_ERROR GetNVS(const char * ns, const char * name, uint8_t * buf, size_t bufSize, size_t & outLen)
{
    WEAVE_ERROR err;
    nvs_handle handle;
    bool needClose = false;

    err = nvs_open(ns, NVS_READONLY, &handle);
    SuccessOrExit(err);
    needClose = true;

    outLen = bufSize;
    err = nvs_get_blob(handle, name, buf, &outLen);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        outLen = 0;
        err = WEAVE_DEVICE_ERROR_CONFIG_NOT_FOUND;
    }
    else if (err == ESP_ERR_NVS_INVALID_LENGTH)
    {
        err = WEAVE_ERROR_BUFFER_TOO_SMALL;
    }
    SuccessOrExit(err);

exit:
    if (needClose)
    {
        nvs_close(handle);
    }

    return err;
}

WEAVE_ERROR GetNVS(const char * ns, const char * name, char * buf, size_t bufSize, size_t & outLen)
{
    WEAVE_ERROR err;
    nvs_handle handle;
    bool needClose = false;

    err = nvs_open(ns, NVS_READONLY, &handle);
    SuccessOrExit(err);
    needClose = true;

    outLen = bufSize;
    err = nvs_get_str(handle, name, buf, &outLen);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        outLen = 0;
        err = WEAVE_DEVICE_ERROR_CONFIG_NOT_FOUND;
    }
    else if (err == ESP_ERR_NVS_INVALID_LENGTH)
    {
        err = WEAVE_ERROR_BUFFER_TOO_SMALL;
    }
    SuccessOrExit(err);

    outLen -= 1; // Don't count trailing nul.

exit:
    if (needClose)
    {
        nvs_close(handle);
    }

    return err;
}

WEAVE_ERROR GetNVS(const char * ns, const char * name, uint32_t & val)
{
    WEAVE_ERROR err;
    nvs_handle handle;
    bool needClose = false;

    err = nvs_open(ns, NVS_READONLY, &handle);
    SuccessOrExit(err);
    needClose = true;

    err = nvs_get_u32(handle, name, &val);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        err = WEAVE_DEVICE_ERROR_CONFIG_NOT_FOUND;
    }
    SuccessOrExit(err);

exit:
    if (needClose)
    {
        nvs_close(handle);
    }
    return err;
}

WEAVE_ERROR GetNVS(const char * ns, const char * name, uint64_t & val)
{
    WEAVE_ERROR err;
    nvs_handle handle;
    bool needClose = false;

    err = nvs_open(ns, NVS_READONLY, &handle);
    SuccessOrExit(err);
    needClose = true;

    err = nvs_get_u64(handle, name, &val);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        err = WEAVE_DEVICE_ERROR_CONFIG_NOT_FOUND;
    }
    SuccessOrExit(err);

exit:
    if (needClose)
    {
        nvs_close(handle);
    }
    return err;
}

WEAVE_ERROR StoreNVS(const char * ns, const char * name, const uint8_t * data, size_t dataLen)
{
    WEAVE_ERROR err;
    nvs_handle handle;
    bool needClose = false;

    if (data != NULL)
    {
        err = nvs_open(ns, NVS_READWRITE, &handle);
        SuccessOrExit(err);
        needClose = true;

        err = nvs_set_blob(handle, name, data, dataLen);
        SuccessOrExit(err);

        // Commit the value to the persistent store.
        err = nvs_commit(handle);
        SuccessOrExit(err);

        WeaveLogProgress(DeviceLayer, "StoreNVS: %s/%s = (blob length %" PRId32 ")", ns, name, dataLen);
    }

    else
    {
        err = ClearNVSKey(ns, name);
        SuccessOrExit(err);
    }

exit:
    if (needClose)
    {
        nvs_close(handle);
    }

    return err;
}

WEAVE_ERROR StoreNVS(const char * ns, const char * name, const char * data)
{
    WEAVE_ERROR err;
    nvs_handle handle;
    bool needClose = false;

    if (data != NULL)
    {
        err = nvs_open(ns, NVS_READWRITE, &handle);
        SuccessOrExit(err);
        needClose = true;

        err = nvs_set_str(handle, name, data);
        SuccessOrExit(err);

        // Commit the value to the persistent store.
        err = nvs_commit(handle);
        SuccessOrExit(err);

        WeaveLogProgress(DeviceLayer, "StoreNVS: %s/%s = \"%s\"", ns, name, data);
    }

    else
    {
        err = ClearNVSKey(ns, name);
        SuccessOrExit(err);
    }

exit:
    if (needClose)
    {
        nvs_close(handle);
    }

    return err;
}

WEAVE_ERROR StoreNVS(const char * ns, const char * name, uint32_t val)
{
    WEAVE_ERROR err;
    nvs_handle handle;
    bool needClose = false;

    err = nvs_open(ns, NVS_READWRITE, &handle);
    SuccessOrExit(err);
    needClose = true;

    err = nvs_set_u32(handle, name, val);
    SuccessOrExit(err);

    // Commit the value to the persistent store.
    err = nvs_commit(handle);
    SuccessOrExit(err);

    WeaveLogProgress(DeviceLayer, "StoreNVS: %s/%s = %" PRIu32 " (0x%" PRIX32 ")", ns, name, val, val);

exit:
    if (needClose)
    {
        nvs_close(handle);
    }

    return err;
}

WEAVE_ERROR StoreNVS(const char * ns, const char * name, uint64_t val)
{
    WEAVE_ERROR err;
    nvs_handle handle;
    bool needClose = false;

    err = nvs_open(ns, NVS_READWRITE, &handle);
    SuccessOrExit(err);
    needClose = true;

    err = nvs_set_u64(handle, name, val);
    SuccessOrExit(err);

    // Commit the value to the persistent store.
    err = nvs_commit(handle);
    SuccessOrExit(err);

    WeaveLogProgress(DeviceLayer, "StoreNVS: %s/%s = %" PRIu64 " (0x%" PRIX64 ")", ns, name, val, val);

exit:
    if (needClose)
    {
        nvs_close(handle);
    }

    return err;
}

WEAVE_ERROR ClearNVSKey(const char * ns, const char * name)
{
    WEAVE_ERROR err;
    nvs_handle handle;
    bool needClose = false;

    err = nvs_open(ns, NVS_READWRITE, &handle);
    SuccessOrExit(err);
    needClose = true;

    err = nvs_erase_key(handle, name);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        ExitNow(err = WEAVE_NO_ERROR);
    }
    SuccessOrExit(err);

    // Commit the value to the persistent store.
    err = nvs_commit(handle);
    SuccessOrExit(err);

    WeaveLogProgress(DeviceLayer, "ClearNVSKey: %s/%s", ns, name);

exit:
    if (needClose)
    {
        nvs_close(handle);
    }

    return err;
}

WEAVE_ERROR ClearNVSNamespace(const char * ns)
{
    WEAVE_ERROR err;
    nvs_handle handle;
    bool needClose = false;

    err = nvs_open(ns, NVS_READWRITE, &handle);
    SuccessOrExit(err);
    needClose = true;

    err = nvs_erase_all(handle);
    SuccessOrExit(err);

    err = nvs_commit(handle);
    SuccessOrExit(err);

exit:
    if (needClose)
    {
        nvs_close(handle);
    }
    return err;
}

WEAVE_ERROR EnsureNamespace(const char * ns)
{
    WEAVE_ERROR err;
    nvs_handle handle;
    bool needClose = false;

    err = nvs_open(ns, NVS_READONLY, &handle);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        err = nvs_open(ns, NVS_READWRITE, &handle);
        SuccessOrExit(err);
        needClose = true;

        err = nvs_commit(handle);
        SuccessOrExit(err);
    }
    SuccessOrExit(err);
    needClose = true;

exit:
    if (needClose)
    {
        nvs_close(handle);
    }
    return err;
}

} // unnamed namespace
} // namespace Device
} // namespace Weave
} // namespace nl

namespace nl {
namespace Weave {
namespace Platform {
namespace PersistedStorage {

using namespace ::nl::Weave::Device;

WEAVE_ERROR Read(Key key, uint32_t & value)
{
    return ConfigurationMgr.GetPersistedCounter(key, value);
}

WEAVE_ERROR Write(Key key, uint32_t value)
{
    return ConfigurationMgr.StorePersistedCounter(key, value);
}

} // PersistedStorage
} // Platform
} // Weave
} // nl

