#include <WeavePlatform-ESP32-Internal.h>

#include <Weave/Core/WeaveKeyIds.h>
#include <Weave/Profiles/security/WeaveApplicationKeys.h>

#include "nvs_flash.h"
#include "nvs.h"

#include <new>

namespace WeavePlatform {

namespace Internal {

static const char gNVSNamespace_Weave[]             = "weave";
static const char gNVSNamespace_WeaveCounters[]     = "weave-counters";
static const char gNVSNamespace_WeaveGroupKeys[]    = "weave-grp-keys";

static const char gNVSKeyName_DeviceId[]            = "device-id";
static const char gNVSKeyName_SerialNum[]           = "serial-num";
static const char gNVSKeyName_ManufacturingDate[]   = "mfg-date";
static const char gNVSKeyName_PairingCode[]         = "pairing-code";
static const char gNVSKeyName_FabricId[]            = "fabric-id";
static const char gNVSKeyName_DeviceCert[]          = "device-cert";
static const char gNVSKeyName_DevicePrivateKey[]    = "device-key";
static const char gNVSKeyName_ServiceConfig[]       = "service-config";
static const char gNVSKeyName_PairedAccountId[]     = "account-id";
static const char gNVSKeyName_ServiceId[]           = "service-id";
static const char gNVSKeyName_FabricSecret[]        = "fabric-secret";
static const char gNVSKeyName_ServiceRootKey[]      = "srk";
static const char gNVSKeyName_EpochKeyPrefix[]      = "ek-";
static const char gNVSKeyName_AppMasterKeyIndex[]   = "amk-index";
static const char gNVSKeyName_AppMasterKeyPrefix[]  = "amk-";
static const char gNVSKeyName_LastUsedEpochKeyId[]  = "last-ek-id";


using namespace ::nl::Weave::Profiles::Security::AppKeys;
using namespace ::nl::Weave::Profiles::DeviceDescription;

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

static GroupKeyStore gGroupKeyStore;


static WEAVE_ERROR GetNVS(const char * ns, const char * name, uint8_t * buf, size_t bufSize, size_t & outLen);
static WEAVE_ERROR GetNVS(const char * ns, const char * name, char * buf, size_t bufSize, size_t & outLen);
static WEAVE_ERROR GetNVS(const char * ns, const char * name, uint32_t & val);
static WEAVE_ERROR GetNVS(const char * ns, const char * name, uint64_t & val);
static WEAVE_ERROR StoreNVS(const char * ns, const char * name, const uint8_t * data, size_t dataLen);
static WEAVE_ERROR StoreNVS(const char * ns, const char * name, const char * data);
static WEAVE_ERROR StoreNVS(const char * ns, const char * name, uint32_t val);
static WEAVE_ERROR StoreNVS(const char * ns, const char * name, uint64_t val);
static WEAVE_ERROR ClearNVSKey(const char * ns, const char * name);
static WEAVE_ERROR ClearNVSNamespace(const char * ns);
static WEAVE_ERROR GetNVSBlobLength(const char * ns, const char * name, size_t & outLen);


} // namespace Internal

using namespace Internal;

WEAVE_ERROR ConfigurationManager::Init()
{
    WEAVE_ERROR err;
    nvs_handle handle;
    bool needClose = false;

    // Force initialization of weave NVS namespace if it doesn't already exist.
    err = nvs_open(gNVSNamespace_Weave, NVS_READONLY, &handle);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        err = nvs_open(gNVSNamespace_Weave, NVS_READWRITE, &handle);
        SuccessOrExit(err);
        needClose = true;

        err = nvs_commit(handle);
        SuccessOrExit(err);
    }
    SuccessOrExit(err);
    needClose = true;

    // Force initialization of the global GroupKeyStore object.
    new ((void *)&gGroupKeyStore) GroupKeyStore();

exit:
    if (needClose)
    {
        nvs_close(handle);
    }
    return err;
}

WEAVE_ERROR ConfigurationManager::ConfigureWeaveStack()
{
    WEAVE_ERROR err;
    nvs_handle handle;
    bool needClose = false;
    size_t pairingCodeLen;

    err = nvs_open(gNVSNamespace_Weave, NVS_READONLY, &handle);
    SuccessOrExit(err);
    needClose = true;

    // Read the device id from NVS.
    err = nvs_get_u64(handle, gNVSKeyName_DeviceId, &FabricState.LocalNodeId);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        // TODO: make this a DEBUG-only feature
        ESP_LOGI(TAG, "Device id not found in nvs; using default");
        FabricState.LocalNodeId = gTestDeviceId;
        err = WEAVE_NO_ERROR;
    }
    SuccessOrExit(err);

    // Read the fabric id from NVS.  If not present, then the device is not currently a
    // member of a Weave fabric.
    err = nvs_get_u64(handle, gNVSKeyName_FabricId, &FabricState.FabricId);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        FabricState.FabricId = kFabricIdNotSpecified;
        err = WEAVE_NO_ERROR;
    }
    SuccessOrExit(err);

    // Read the pairing code from NVS.
    pairingCodeLen = sizeof(mPairingCode);
    err = nvs_get_str(handle, gNVSKeyName_PairingCode, mPairingCode, &pairingCodeLen);
    if (err == ESP_ERR_NVS_NOT_FOUND || pairingCodeLen == 0)
    {
        // TODO: make this a DEBUG-only feature
        ESP_LOGI(TAG, "Pairing code not found in nvs; using default");
        strcpy(mPairingCode, gTestPairingCode);
        ExitNow(err = WEAVE_NO_ERROR);
    }
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

WEAVE_ERROR ConfigurationManager::GetVendorId(uint16_t& vendorId)
{
    // TODO: get from build config
    vendorId = kWeaveVendor_NestLabs;
    return WEAVE_NO_ERROR;
}

WEAVE_ERROR ConfigurationManager::GetProductId(uint16_t& productId)
{
    // TODO: get from build config
    productId = 4242;
    return WEAVE_NO_ERROR;
}

WEAVE_ERROR ConfigurationManager::GetProductRevision(uint16_t& productRev)
{
    // TODO: get from build config
    productRev = 1;
    return WEAVE_NO_ERROR;
}

WEAVE_ERROR ConfigurationManager::GetSerialNumber(char * buf, size_t bufSize, size_t & serialNumLen)
{
    return GetNVS(gNVSNamespace_Weave, gNVSKeyName_SerialNum, buf, bufSize, serialNumLen);
}

WEAVE_ERROR ConfigurationManager::GetManufacturingDate(uint16_t& year, uint8_t& month, uint8_t& dayOfMonth)
{
    WEAVE_ERROR err;
    char dateStr[11]; // sized for big-endian date: YYYY-MM-DD
    size_t dateLen;
    char *parseEnd;

    err = GetNVS(gNVSNamespace_Weave, gNVSKeyName_ManufacturingDate, dateStr, sizeof(dateStr), dateLen);
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
    // TODO: get from build config
    outLen = 0;
    return WEAVE_PLATFORM_ERROR_CONFIG_NOT_FOUND;
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

    err = GetNVS(gNVSNamespace_Weave, gNVSKeyName_DeviceCert, buf, bufSize, certLen);

    // TODO: make this a debug-only feature
    if (err == WEAVE_PLATFORM_ERROR_CONFIG_NOT_FOUND)
    {
        certLen = gTestDeviceCertLength;
        VerifyOrExit(gTestDeviceCertLength <= bufSize, err = WEAVE_ERROR_BUFFER_TOO_SMALL);
        ESP_LOGI(TAG, "Device certificate not found in nvs; using default");
        memcpy(buf, gTestDeviceCert, gTestDeviceCertLength);
        err = WEAVE_NO_ERROR;
    }

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

    err = GetNVS(gNVSNamespace_Weave, gNVSKeyName_DevicePrivateKey, buf, bufSize, keyLen);

    // TODO: make this a debug-only feature
    if (err == WEAVE_PLATFORM_ERROR_CONFIG_NOT_FOUND)
    {
        keyLen = gTestDevicePrivateKeyLength;
        VerifyOrExit(gTestDevicePrivateKeyLength <= bufSize, err = WEAVE_ERROR_BUFFER_TOO_SMALL);
        ESP_LOGI(TAG, "Device private key not found in nvs; using default");
        memcpy(buf, gTestDevicePrivateKey, gTestDevicePrivateKeyLength);
        err = WEAVE_NO_ERROR;
    }

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
    return GetNVS(gNVSNamespace_Weave, gNVSKeyName_ServiceConfig, buf, bufSize, serviceConfigLen);
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
    return GetNVS(gNVSNamespace_Weave, gNVSKeyName_ServiceId, serviceId);
}

WEAVE_ERROR ConfigurationManager::GetPairedAccountId(char * buf, size_t bufSize, size_t accountIdLen)
{
    return GetNVS(gNVSNamespace_Weave, gNVSKeyName_PairedAccountId, buf, bufSize, accountIdLen);
}

WEAVE_ERROR ConfigurationManager::StoreDeviceId(uint64_t deviceId)
{
    return (deviceId != kNodeIdNotSpecified)
           ? StoreNVS(gNVSNamespace_Weave, gNVSKeyName_DeviceId, deviceId)
           : ClearNVSKey(gNVSNamespace_Weave, gNVSKeyName_DeviceId);
}

WEAVE_ERROR ConfigurationManager::StoreSerialNumber(const char * serialNum)
{
    return StoreNVS(gNVSNamespace_Weave, gNVSKeyName_SerialNum, serialNum);
}

WEAVE_ERROR ConfigurationManager::StoreManufacturingDate(const char * mfgDate)
{
    return StoreNVS(gNVSNamespace_Weave, gNVSKeyName_ManufacturingDate, mfgDate);
}

WEAVE_ERROR ConfigurationManager::StoreFabricId(uint64_t fabricId)
{
    return (fabricId != kFabricIdNotSpecified)
           ? StoreNVS(gNVSNamespace_Weave, gNVSKeyName_FabricId, fabricId)
           : ClearNVSKey(gNVSNamespace_Weave, gNVSKeyName_FabricId);
}

WEAVE_ERROR ConfigurationManager::StoreDeviceCertificate(const uint8_t * cert, size_t certLen)
{
    return StoreNVS(gNVSNamespace_Weave, gNVSKeyName_DeviceCert, cert, certLen);
}

WEAVE_ERROR ConfigurationManager::StoreDevicePrivateKey(const uint8_t * key, size_t keyLen)
{
    return StoreNVS(gNVSNamespace_Weave, gNVSKeyName_DevicePrivateKey, key, keyLen);
}

WEAVE_ERROR ConfigurationManager::StorePairingCode(const char * pairingCode)
{
    return StoreNVS(gNVSNamespace_Weave, gNVSKeyName_PairingCode, pairingCode);
}

WEAVE_ERROR ConfigurationManager::StoreServiceProvisioningData(uint64_t serviceId,
        const uint8_t * serviceConfig, size_t serviceConfigLen,
        const char * accountId, size_t accountIdLen)
{
    WEAVE_ERROR err;
    nvs_handle handle;
    bool needClose = false;
    char *accountIdCopy = NULL;

    err = nvs_open(gNVSNamespace_Weave, NVS_READWRITE, &handle);
    SuccessOrExit(err);
    needClose = true;

    err = nvs_set_u64(handle, gNVSKeyName_ServiceId, serviceId);
    SuccessOrExit(err);

    err = nvs_set_blob(handle, gNVSKeyName_ServiceConfig, serviceConfig, serviceConfigLen);
    SuccessOrExit(err);

    accountIdCopy = strndup(accountId, accountIdLen);
    VerifyOrExit(accountIdCopy != NULL, err = WEAVE_ERROR_NO_MEMORY);
    err = nvs_set_str(handle, gNVSKeyName_PairedAccountId, accountIdCopy);
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

    err = nvs_open(gNVSNamespace_Weave, NVS_READWRITE, &handle);
    SuccessOrExit(err);
    needClose = true;

    err = nvs_erase_key(handle, gNVSKeyName_ServiceId);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        err = WEAVE_NO_ERROR;
    }
    SuccessOrExit(err);

    err = nvs_erase_key(handle, gNVSKeyName_ServiceConfig);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        err = WEAVE_NO_ERROR;
    }
    SuccessOrExit(err);

    err = nvs_erase_key(handle, gNVSKeyName_PairedAccountId);
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
    return StoreNVS(gNVSNamespace_Weave, gNVSKeyName_ServiceConfig, serviceConfig, serviceConfigLen);
}

WEAVE_ERROR ConfigurationManager::GetPersistedCounter(const char * key, uint32_t & value)
{
    WEAVE_ERROR err = GetNVS(gNVSNamespace_WeaveCounters, key, value);
    if (err == WEAVE_PLATFORM_ERROR_CONFIG_NOT_FOUND)
    {
        err = WEAVE_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND;
    }
    return err;
}

WEAVE_ERROR ConfigurationManager::StorePersistedCounter(const char * key, uint32_t value)
{
    return StoreNVS(gNVSNamespace_WeaveCounters, key, value);
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

    // TODO: return PrimaryWiFiMACAddress

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

bool ConfigurationManager::IsServiceProvisioned()
{
    WEAVE_ERROR err;
    uint64_t serviceId;

    err = GetServiceId(serviceId);
    return (err == WEAVE_NO_ERROR && serviceId != 0);
}


namespace Internal {

WEAVE_ERROR GroupKeyStore::RetrieveGroupKey(uint32_t keyId, WeaveGroupKey & key)
{
    WEAVE_ERROR err;
    size_t keyLen;

    // TODO: add support for other group key types

    VerifyOrExit(keyId == WeaveKeyId::kFabricSecret, err = WEAVE_ERROR_KEY_NOT_FOUND);

    err = GetNVS(gNVSNamespace_WeaveGroupKeys, gNVSKeyName_FabricSecret, key.Key, sizeof(key.Key), keyLen);
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

    err = StoreNVS(gNVSNamespace_WeaveGroupKeys, gNVSKeyName_FabricSecret, key.Key, key.KeyLen);
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR GroupKeyStore::DeleteGroupKey(uint32_t keyId)
{
    WEAVE_ERROR err;

    // TODO: add support for other group key types

    VerifyOrExit(keyId == WeaveKeyId::kFabricSecret, err = WEAVE_ERROR_KEY_NOT_FOUND);

    err = ClearNVSKey(gNVSNamespace_WeaveGroupKeys, gNVSKeyName_FabricSecret);
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
        err = ClearNVSKey(gNVSNamespace_WeaveGroupKeys, gNVSKeyName_FabricSecret);
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
        err = GetNVSBlobLength(gNVSNamespace_WeaveGroupKeys, gNVSKeyName_FabricSecret, keyLen);
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
    return ClearNVSNamespace(gNVSNamespace_WeaveGroupKeys);
}

WEAVE_ERROR GroupKeyStore::GetCurrentUTCTime(uint32_t & utcTime)
{
    // TODO: support real time when available.
    return WEAVE_ERROR_UNSUPPORTED_CLOCK;
}

WEAVE_ERROR GroupKeyStore::RetrieveLastUsedEpochKeyId(void)
{
    WEAVE_ERROR err;

    err = GetNVS(gNVSNamespace_WeaveGroupKeys, gNVSKeyName_LastUsedEpochKeyId, LastUsedEpochKeyId);
    if (err == WEAVE_PLATFORM_ERROR_CONFIG_NOT_FOUND)
    {
        LastUsedEpochKeyId = WeaveKeyId::kNone;
        err = WEAVE_NO_ERROR;
    }
    return err;
}

WEAVE_ERROR GroupKeyStore::StoreLastUsedEpochKeyId(void)
{
    return StoreNVS(gNVSNamespace_WeaveGroupKeys, gNVSKeyName_LastUsedEpochKeyId, LastUsedEpochKeyId);
}

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

} // namespace Internal
} // namespace WeavePlatform


namespace nl {
namespace Weave {
namespace Platform {
namespace PersistedStorage {

using namespace ::WeavePlatform;

WEAVE_ERROR Read(Key key, uint32_t & value)
{
    return ConfigMgr.GetPersistedCounter(key, value);
}

WEAVE_ERROR Write(Key key, uint32_t value)
{
    return ConfigMgr.StorePersistedCounter(key, value);
}

} // PersistedStorage
} // Platform
} // Weave
} // nl

