#include <internal/WeavePlatformInternal.h>
#include <ConfigurationManager.h>
#include <Weave/Core/WeaveKeyIds.h>
#include <Weave/Profiles/security/WeaveApplicationKeys.h>
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <new>

using namespace ::nl;
using namespace ::nl::Weave;
using namespace ::nl::Weave::Profiles::Security::AppKeys;
using namespace ::nl::Weave::Profiles::DeviceDescription;
using namespace ::WeavePlatform::Internal;

namespace WeavePlatform {

namespace {

class GroupKeyStore : public ::nl::Weave::Profiles::Security::AppKeys::GroupKeyStoreBase
{
public:
    virtual WEAVE_ERROR RetrieveGroupKey(uint32_t keyId, WeaveGroupKey & key);
    virtual WEAVE_ERROR StoreGroupKey(const WeaveGroupKey & key);
    virtual WEAVE_ERROR DeleteGroupKey(uint32_t keyId);
    virtual WEAVE_ERROR DeleteGroupKeysOfAType(uint32_t keyType);
    virtual WEAVE_ERROR EnumerateGroupKeys(uint32_t keyType, uint32_t * keyIds, uint8_t keyIdsArraySize, uint8_t & keyCount);
    virtual WEAVE_ERROR Clear(void);
    virtual WEAVE_ERROR GetCurrentUTCTime(uint32_t& utcTime);

protected:
    virtual WEAVE_ERROR RetrieveLastUsedEpochKeyId(void);
    virtual WEAVE_ERROR StoreLastUsedEpochKeyId(void);
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
const char kNVSKeyName_ServiceRootKey[]      = "srk";
const char kNVSKeyName_EpochKeyPrefix[]      = "ek-";
const char kNVSKeyName_AppMasterKeyIndex[]   = "amk-index";
const char kNVSKeyName_AppMasterKeyPrefix[]  = "amk-";
const char kNVSKeyName_LastUsedEpochKeyId[]  = "last-ek-id";
const char kNVSKeyName_FailSafeArmed[]       = "fail-safe-armed";

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
WEAVE_ERROR GetNVSBlobLength(const char * ns, const char * name, size_t & outLen);
WEAVE_ERROR EnsureNamespace(const char * ns);
WEAVE_ERROR EraseNamespace(const char * ns);

} // unnamed namespace


// ==================== Configuration Manager Public Methods ====================

WEAVE_ERROR ConfigurationManager::GetVendorId(uint16_t& vendorId)
{
    vendorId = (uint16_t)WEAVE_PLATFORM_CONFIG_DEVICE_VENDOR_ID;
    return WEAVE_NO_ERROR;
}

WEAVE_ERROR ConfigurationManager::GetProductId(uint16_t& productId)
{
    productId = (uint16_t)WEAVE_PLATFORM_CONFIG_DEVICE_PRODUCT_ID;
    return WEAVE_NO_ERROR;
}

WEAVE_ERROR ConfigurationManager::GetProductRevision(uint16_t& productRev)
{
    productRev = (uint16_t)WEAVE_PLATFORM_CONFIG_DEVICE_PRODUCT_REVISION;
    return WEAVE_NO_ERROR;
}

WEAVE_ERROR ConfigurationManager::GetSerialNumber(char * buf, size_t bufSize, size_t & serialNumLen)
{
    return GetNVS(kNVSNamespace_WeaveFactory, kNVSKeyName_SerialNum, buf, bufSize, serialNumLen);
}

WEAVE_ERROR ConfigurationManager::GetManufacturingDate(uint16_t& year, uint8_t& month, uint8_t& dayOfMonth)
{
    WEAVE_ERROR err;
    char dateStr[11]; // sized for big-endian date: YYYY-MM-DD
    size_t dateLen;
    char *parseEnd;

    err = GetNVS(kNVSNamespace_WeaveFactory, kNVSKeyName_ManufacturingDate, dateStr, sizeof(dateStr), dateLen);
    SuccessOrExit(err);

    VerifyOrExit(dateLen == sizeof(dateStr), err = WEAVE_ERROR_INVALID_ARGUMENT);

    year = strtoul(dateStr, &parseEnd, 10);
    VerifyOrExit(parseEnd == dateStr + 4, err = WEAVE_ERROR_INVALID_ARGUMENT);

    month = strtoul(dateStr + 5, &parseEnd, 10);
    VerifyOrExit(parseEnd == dateStr + 7, err = WEAVE_ERROR_INVALID_ARGUMENT);

    dayOfMonth = strtoul(dateStr + 8, &parseEnd, 10);
    VerifyOrExit(parseEnd == dateStr + 10, err = WEAVE_ERROR_INVALID_ARGUMENT);

exit:
    return err;
}

WEAVE_ERROR ConfigurationManager::GetFirmwareRevision(char * buf, size_t bufSize, size_t & outLen)
{
#ifdef WEAVE_PLATFORM_CONFIG_DEVICE_FIRMWARE_REVISION
    if (WEAVE_PLATFORM_CONFIG_DEVICE_FIRMWARE_REVISION[0] != 0)
    {
        outLen = min(bufSize, sizeof(WEAVE_PLATFORM_CONFIG_DEVICE_FIRMWARE_REVISION) - 1);
        memcpy(buf, WEAVE_PLATFORM_CONFIG_DEVICE_FIRMWARE_REVISION, outLen);
        return WEAVE_NO_ERROR;
    }
    else
#endif // WEAVE_PLATFORM_CONFIG_DEVICE_FIRMWARE_REVISION
    {
        outLen = 0;
        return WEAVE_PLATFORM_ERROR_CONFIG_NOT_FOUND;
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

#if WEAVE_PLATFORM_CONFIG_ENABLE_TEST_DEVICE_IDENTITY

    if (err == WEAVE_PLATFORM_ERROR_CONFIG_NOT_FOUND)
    {
        certLen = TestDeviceCertLength;
        VerifyOrExit(TestDeviceCertLength <= bufSize, err = WEAVE_ERROR_BUFFER_TOO_SMALL);
        ESP_LOGI(TAG, "Device certificate not found in nvs; using default");
        memcpy(buf, TestDeviceCert, TestDeviceCertLength);
        err = WEAVE_NO_ERROR;
    }

#endif // WEAVE_PLATFORM_CONFIG_ENABLE_TEST_DEVICE_IDENTITY

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

#if WEAVE_PLATFORM_CONFIG_ENABLE_TEST_DEVICE_IDENTITY

    if (err == WEAVE_PLATFORM_ERROR_CONFIG_NOT_FOUND)
    {
        keyLen = TestDevicePrivateKeyLength;
        VerifyOrExit(TestDevicePrivateKeyLength <= bufSize, err = WEAVE_ERROR_BUFFER_TOO_SMALL);
        ESP_LOGI(TAG, "Device private key not found in nvs; using default");
        memcpy(buf, TestDevicePrivateKey, TestDevicePrivateKeyLength);
        err = WEAVE_NO_ERROR;
    }

#endif // WEAVE_PLATFORM_CONFIG_ENABLE_TEST_DEVICE_IDENTITY

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
           ? StoreNVS(kNVSNamespace_WeaveConfig, kNVSKeyName_DeviceId, deviceId)
           : ClearNVSKey(kNVSNamespace_WeaveConfig, kNVSKeyName_DeviceId);
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

    accountIdCopy = strndup(accountId, accountIdLen);
    VerifyOrExit(accountIdCopy != NULL, err = WEAVE_ERROR_NO_MEMORY);
    err = nvs_set_str(handle, kNVSKeyName_PairedAccountId, accountIdCopy);
    free(accountIdCopy);
    SuccessOrExit(err);

    // Commit the value to the persistent store.
    err = nvs_commit(handle);
    SuccessOrExit(err);

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

WEAVE_ERROR ConfigurationManager::GetPersistedCounter(const char * key, uint32_t & value)
{
    WEAVE_ERROR err = GetNVS(kNVSNamespace_WeaveCounters, key, value);
    if (err == WEAVE_PLATFORM_ERROR_CONFIG_NOT_FOUND)
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
    if (err == WEAVE_PLATFORM_ERROR_CONFIG_NOT_FOUND)
    {
        err = WEAVE_NO_ERROR;
    }
    SuccessOrExit(err);

    err = esp_wifi_get_mac(ESP_IF_WIFI_STA, deviceDesc.PrimaryWiFiMACAddress);
    SuccessOrExit(err);

    err = GetWiFiAPSSID(deviceDesc.RendezvousWiFiESSID, sizeof(deviceDesc.RendezvousWiFiESSID));
    SuccessOrExit(err);

    err = GetSerialNumber(deviceDesc.SerialNumber, sizeof(deviceDesc.SerialNumber), outLen);
    if (err == WEAVE_PLATFORM_ERROR_CONFIG_NOT_FOUND)
    {
        err = WEAVE_NO_ERROR;
    }
    SuccessOrExit(err);

    err = GetFirmwareRevision(deviceDesc.SoftwareVersion, sizeof(deviceDesc.SoftwareVersion), outLen);
    if (err == WEAVE_PLATFORM_ERROR_CONFIG_NOT_FOUND)
    {
        err = WEAVE_NO_ERROR;
    }
    SuccessOrExit(err);

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

WEAVE_ERROR ConfigurationManager::GetWiFiAPSSID(char * buf, size_t bufSize)
{
    WEAVE_ERROR err;
    uint8_t mac[6];

    err = esp_wifi_get_mac(ESP_IF_WIFI_STA, mac);
    SuccessOrExit(err);

    snprintf(buf, bufSize, "%s%02X%02X", WEAVE_PLATFORM_CONFIG_WIFI_AP_SSID_PREFIX, mac[4], mac[5]);
    buf[bufSize - 1] = 0;

exit:
    return err;
}

bool ConfigurationManager::IsServiceProvisioned()
{
    WEAVE_ERROR err;
    uint64_t serviceId;

    err = GetServiceId(serviceId);
    return (err == WEAVE_NO_ERROR && serviceId != 0);
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

    // Force initialization of weave NVS namespaces if they doesn't already exist.
    err = EnsureNamespace(kNVSNamespace_WeaveFactory);
    SuccessOrExit(err);
    err = EnsureNamespace(kNVSNamespace_WeaveConfig);
    SuccessOrExit(err);
    err = EnsureNamespace(kNVSNamespace_WeaveCounters);
    SuccessOrExit(err);

    // Initialize the global GroupKeyStore object.
    new ((void *)&gGroupKeyStore) GroupKeyStore();

    // If the fail-safe was armed when the device last shutdown, initiate a factory reset.
    if (GetNVS(kNVSNamespace_WeaveConfig, kNVSKeyName_FailSafeArmed, failSafeArmed) == WEAVE_NO_ERROR &&
        failSafeArmed != 0)
    {
        ESP_LOGI(TAG, "Detected fail-safe armed on reboot; initiating factory reset");
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

    err = nvs_open(kNVSNamespace_WeaveFactory, NVS_READONLY, &handle);
    SuccessOrExit(err);
    needClose = true;

    // Read the device id from NVS.
    err = nvs_get_u64(handle, kNVSKeyName_DeviceId, &FabricState.LocalNodeId);
#if WEAVE_PLATFORM_CONFIG_ENABLE_TEST_DEVICE_IDENTITY
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        ESP_LOGI(TAG, "Device id not found in nvs; using hard-coded default: %" PRIX64, TestDeviceId);
        FabricState.LocalNodeId = TestDeviceId;
        err = WEAVE_NO_ERROR;
    }
#endif // WEAVE_PLATFORM_CONFIG_ENABLE_TEST_DEVICE_IDENTITY
    SuccessOrExit(err);

    // Read the fabric id from NVS.  If not present, then the device is not currently a
    // member of a Weave fabric.
    err = nvs_get_u64(handle, kNVSKeyName_FabricId, &FabricState.FabricId);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        FabricState.FabricId = kFabricIdNotSpecified;
        err = WEAVE_NO_ERROR;
    }
    SuccessOrExit(err);

    // Read the pairing code from NVS.
    pairingCodeLen = sizeof(mPairingCode);
    err = nvs_get_str(handle, kNVSKeyName_PairingCode, mPairingCode, &pairingCodeLen);
#ifdef CONFIG_USE_TEST_PAIRING_CODE
    if (CONFIG_USE_TEST_PAIRING_CODE[0] != 0 && err == ESP_ERR_NVS_NOT_FOUND)
    {
        ESP_LOGI(TAG, "Pairing code not found in nvs; using hard-coded default: %s", CONFIG_USE_TEST_PAIRING_CODE);
        memcpy(mPairingCode, CONFIG_USE_TEST_PAIRING_CODE, min(sizeof(mPairingCode) - 1, sizeof(CONFIG_USE_TEST_PAIRING_CODE)));
        mPairingCode[sizeof(mPairingCode) - 1] = 0;
        err = WEAVE_NO_ERROR;
    }
#endif // CONFIG_USE_TEST_PAIRING_CODE
    SuccessOrExit(err);

    FabricState.PairingCode = mPairingCode;

    // Configure the FabricState object with a reference to the GroupKeyStore object.
    FabricState.GroupKeyStore = &gGroupKeyStore;

exit:
    if (needClose)
    {
        nvs_close(handle);
    }
    return err;
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

void ConfigurationManager::DoFactoryReset(intptr_t arg)
{
    WEAVE_ERROR err;

    ESP_LOGI(TAG, "Performing factory reset");

    // Erase all values in the weave-config NVS namespace.
    err = EraseNamespace(kNVSNamespace_WeaveConfig);
    if (err != WEAVE_NO_ERROR)
    {
        ESP_LOGE(TAG, "EraseNamespace(WeaveConfig) failed: %s", nl::ErrorStr(err));
    }

    // Restore WiFi persistent settings to default values.
    err = esp_wifi_restore();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_wifi_restore() failed: %s", nl::ErrorStr(err));
    }

    // Restart the system.
    ESP_LOGI(TAG, "System restarting");
    esp_restart();
}

namespace {

// ==================== Group Key Store Implementation ====================

WEAVE_ERROR GroupKeyStore::RetrieveGroupKey(uint32_t keyId, WeaveGroupKey & key)
{
    WEAVE_ERROR err;
    size_t keyLen;

    // TODO: add support for other group key types

    VerifyOrExit(keyId == WeaveKeyId::kFabricSecret, err = WEAVE_ERROR_KEY_NOT_FOUND);

    err = GetNVS(kNVSNamespace_WeaveConfig, kNVSKeyName_FabricSecret, key.Key, sizeof(key.Key), keyLen);
    if (err == WEAVE_PLATFORM_ERROR_CONFIG_NOT_FOUND)
    {
        err = WEAVE_ERROR_KEY_NOT_FOUND;
    }
    SuccessOrExit(err);

    key.KeyId = keyId;
    key.KeyLen = keyLen;

exit:
    return err;
}

WEAVE_ERROR GroupKeyStore::StoreGroupKey(const WeaveGroupKey & key)
{
    WEAVE_ERROR err;

    // TODO: add support for other group key types

    VerifyOrExit(key.KeyId == WeaveKeyId::kFabricSecret, err = WEAVE_ERROR_INVALID_KEY_ID);

    err = StoreNVS(kNVSNamespace_WeaveConfig, kNVSKeyName_FabricSecret, key.Key, key.KeyLen);
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR GroupKeyStore::DeleteGroupKey(uint32_t keyId)
{
    WEAVE_ERROR err;

    // TODO: add support for other group key types

    VerifyOrExit(keyId == WeaveKeyId::kFabricSecret, err = WEAVE_ERROR_KEY_NOT_FOUND);

    err = ClearNVSKey(kNVSNamespace_WeaveConfig, kNVSKeyName_FabricSecret);
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR GroupKeyStore::DeleteGroupKeysOfAType(uint32_t keyType)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // TODO: add support for other group key types

    if (WeaveKeyId::IsGeneralKey(keyType))
    {
        err = ClearNVSKey(kNVSNamespace_WeaveConfig, kNVSKeyName_FabricSecret);
        SuccessOrExit(err);
    }

exit:
    return err;
}

WEAVE_ERROR GroupKeyStore::EnumerateGroupKeys(uint32_t keyType, uint32_t * keyIds,
        uint8_t keyIdsArraySize, uint8_t & keyCount)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    size_t keyLen;

    // Verify the supported key type is specified.
    VerifyOrExit(WeaveKeyId::IsGeneralKey(keyType) ||
                 WeaveKeyId::IsAppRootKey(keyType) ||
                 WeaveKeyId::IsAppEpochKey(keyType) ||
                 WeaveKeyId::IsAppGroupMasterKey(keyType), err = WEAVE_ERROR_INVALID_KEY_ID);

    keyCount = 0;

    if (WeaveKeyId::IsGeneralKey(keyType))
    {
        err = GetNVSBlobLength(kNVSNamespace_WeaveConfig, kNVSKeyName_FabricSecret, keyLen);
        SuccessOrExit(err);

        if (keyLen != 0)
        {
            VerifyOrExit(keyCount < keyIdsArraySize, err = WEAVE_ERROR_BUFFER_TOO_SMALL);
            keyIds[keyCount++] = WeaveKeyId::kFabricSecret;
        }
    }

    // TODO: add support for other group key types

exit:
    return err;
}

WEAVE_ERROR GroupKeyStore::Clear(void)
{
    return ClearNVSNamespace(kNVSNamespace_WeaveConfig);
}

WEAVE_ERROR GroupKeyStore::GetCurrentUTCTime(uint32_t & utcTime)
{
    // TODO: support real time when available.
    return WEAVE_ERROR_UNSUPPORTED_CLOCK;
}

WEAVE_ERROR GroupKeyStore::RetrieveLastUsedEpochKeyId(void)
{
    WEAVE_ERROR err;

    err = GetNVS(kNVSNamespace_WeaveConfig, kNVSKeyName_LastUsedEpochKeyId, LastUsedEpochKeyId);
    if (err == WEAVE_PLATFORM_ERROR_CONFIG_NOT_FOUND)
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
        err = WEAVE_PLATFORM_ERROR_CONFIG_NOT_FOUND;
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
        err = WEAVE_PLATFORM_ERROR_CONFIG_NOT_FOUND;
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
        err = WEAVE_PLATFORM_ERROR_CONFIG_NOT_FOUND;
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
        err = WEAVE_PLATFORM_ERROR_CONFIG_NOT_FOUND;
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

        ESP_LOGI(TAG, "StoreNVS: %s/%s = (blob length %" PRId32 ")", ns, name, dataLen);
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

        ESP_LOGI(TAG, "StoreNVS: %s/%s = \"%s\"", ns, name, data);
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

    ESP_LOGI(TAG, "StoreNVS: %s/%s = %" PRIu32, ns, name, val);

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

    ESP_LOGI(TAG, "StoreNVS: %s/%s = %" PRIu64, ns, name, val);

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

    ESP_LOGI(TAG, "ClearNVSKey: %s/%s", ns, name);

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

WEAVE_ERROR GetNVSBlobLength(const char * ns, const char * name, size_t & outLen)
{
    WEAVE_ERROR err;
    nvs_handle handle;
    bool needClose = false;

    err = nvs_open(ns, NVS_READONLY, &handle);
    SuccessOrExit(err);
    needClose = true;

    outLen = 0;
    err = nvs_get_blob(handle, name, NULL, &outLen);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        outLen = 0;
        ExitNow(err = WEAVE_NO_ERROR);
    }
    if (err == ESP_ERR_NVS_INVALID_LENGTH)
    {
        ExitNow(err = WEAVE_NO_ERROR);
    }
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

WEAVE_ERROR EraseNamespace(const char * ns)
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

} // unnamed namespace
} // namespace WeavePlatform

namespace nl {
namespace Weave {
namespace Platform {
namespace PersistedStorage {

using namespace ::WeavePlatform;

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

