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
 *          Contains non-inline method definitions for the
 *          GenericConfigurationManagerImpl<> template.
 */

#ifndef GENERIC_CONFIGURATION_MANAGER_IMPL_IPP
#define GENERIC_CONFIGURATION_MANAGER_IMPL_IPP

#include <Weave/DeviceLayer/internal/WeaveDeviceLayerInternal.h>
#include <Weave/DeviceLayer/internal/GenericConfigurationManagerImpl.h>


namespace nl {
namespace Weave {
namespace DeviceLayer {
namespace Internal {

template<class ImplClass>
WEAVE_ERROR GenericConfigurationManagerImpl<ImplClass>::_Init()
{
    mFlags = 0;

    // Cache flags indicating whether the device is currently service provisioned, is a member of a fabric,
    // and/or is paired to an account.
    SetFlag(mFlags, kFlag_IsServiceProvisioned, Impl()->ConfigValueExists(ImplClass::kConfigKey_ServiceConfig));
    SetFlag(mFlags, kFlag_IsMemberOfFabric, Impl()->ConfigValueExists(ImplClass::kConfigKey_FabricId));
    SetFlag(mFlags, kFlag_IsPairedToAccount, Impl()->ConfigValueExists(ImplClass::kConfigKey_PairedAccountId));

    return WEAVE_NO_ERROR;
}

template<class ImplClass>
WEAVE_ERROR GenericConfigurationManagerImpl<ImplClass>::_ConfigureWeaveStack()
{
    WEAVE_ERROR err;
    size_t pairingCodeLen;

    static char sPairingCodeBuf[ConfigurationManager::kMaxPairingCodeLength + 1];

    // Configure the Weave FabricState object with the local node id.
    err = Impl()->_GetDeviceId(FabricState.LocalNodeId);
    SuccessOrExit(err);

    // Configure the FabricState object with the pairing code string, if present.
    err = Impl()->_GetPairingCode(sPairingCodeBuf, sizeof(sPairingCodeBuf), pairingCodeLen);
    if (err != WEAVE_DEVICE_ERROR_CONFIG_NOT_FOUND)
    {
        SuccessOrExit(err);
        FabricState.PairingCode = sPairingCodeBuf;
    }

    // If the device is a member of a Weave fabric, configure the FabricState object with the fabric id.
    err = Impl()->_GetFabricId(FabricState.FabricId);
    if (err == WEAVE_DEVICE_ERROR_CONFIG_NOT_FOUND)
    {
        FabricState.FabricId = kFabricIdNotSpecified;
        err = WEAVE_NO_ERROR;
    }
    SuccessOrExit(err);

    // Configure the FabricState object with a reference to the GroupKeyStore object.
    FabricState.GroupKeyStore = Impl()->_GetGroupKeyStore();

#if WEAVE_PROGRESS_LOGGING
    Impl()->LogDeviceConfig();
#endif

exit:
    return err;
}

template<class ImplClass>
WEAVE_ERROR GenericConfigurationManagerImpl<ImplClass>::_GetFirmwareRevision(char * buf, size_t bufSize, size_t & outLen)
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

template<class ImplClass>
WEAVE_ERROR GenericConfigurationManagerImpl<ImplClass>::_GetFirmwareBuildTime(uint16_t & year, uint8_t & month, uint8_t & dayOfMonth,
        uint8_t & hour, uint8_t & minute, uint8_t & second)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // TODO: Allow build time to be overridden by compile-time config (e.g. WEAVE_DEVICE_CONFIG_FIRMWARE_BUILD_TIME).

    err = ParseCompilerDateStr(__DATE__, year, month, dayOfMonth);
    SuccessOrExit(err);

    err = Parse24HourTimeStr(__TIME__, hour, minute, second);
    SuccessOrExit(err);

exit:
    return err;
}

template<class ImplClass>
WEAVE_ERROR GenericConfigurationManagerImpl<ImplClass>::_GetDeviceId(uint64_t & deviceId)
{
    WEAVE_ERROR err;

    err = Impl()->ReadConfigValue(ImplClass::kConfigKey_DeviceId, deviceId);

#if WEAVE_DEVICE_CONFIG_ENABLE_TEST_DEVICE_IDENTITY
    if (err == WEAVE_DEVICE_ERROR_CONFIG_NOT_FOUND)
    {
        deviceId = TestDeviceId;
        err = WEAVE_NO_ERROR;
    }
#endif

    return err;
}

template<class ImplClass>
WEAVE_ERROR GenericConfigurationManagerImpl<ImplClass>::_StoreDeviceId(uint64_t deviceId)
{
    return Impl()->WriteConfigValue(ImplClass::kConfigKey_DeviceId, deviceId);
}

template<class ImplClass>
WEAVE_ERROR GenericConfigurationManagerImpl<ImplClass>::_GetSerialNumber(char * buf, size_t bufSize, size_t & serialNumLen)
{
    return Impl()->ReadConfigValueStr(ImplClass::kConfigKey_SerialNum, buf, bufSize, serialNumLen);
}

template<class ImplClass>
WEAVE_ERROR GenericConfigurationManagerImpl<ImplClass>::_StoreSerialNumber(const char * serialNum)
{
    return Impl()->WriteConfigValueStr(ImplClass::kConfigKey_SerialNum, serialNum);
}

template<class ImplClass>
WEAVE_ERROR GenericConfigurationManagerImpl<ImplClass>::_GetPrimaryWiFiMACAddress(uint8_t * buf)
{
    return WEAVE_DEVICE_ERROR_CONFIG_NOT_FOUND;
}

template<class ImplClass>
WEAVE_ERROR GenericConfigurationManagerImpl<ImplClass>::_StorePrimaryWiFiMACAddress(const uint8_t * buf)
{
    return WEAVE_ERROR_UNSUPPORTED_WEAVE_FEATURE;
}

template<class ImplClass>
WEAVE_ERROR GenericConfigurationManagerImpl<ImplClass>::_GetPrimary802154MACAddress(uint8_t * buf)
{
    return WEAVE_DEVICE_ERROR_CONFIG_NOT_FOUND;
}

template<class ImplClass>
WEAVE_ERROR GenericConfigurationManagerImpl<ImplClass>::_StorePrimary802154MACAddress(const uint8_t * buf)
{
    return WEAVE_ERROR_UNSUPPORTED_WEAVE_FEATURE;
}

template<class ImplClass>
WEAVE_ERROR GenericConfigurationManagerImpl<ImplClass>::_GetManufacturingDate(uint16_t& year, uint8_t& month, uint8_t& dayOfMonth)
{
    WEAVE_ERROR err;
    enum {
        kDateStringLength = 10 // YYYY-MM-DD
    };
    char dateStr[kDateStringLength + 1];
    size_t dateLen;
    char *parseEnd;

    err = Impl()->ReadConfigValueStr(ImplClass::kConfigKey_ManufacturingDate, dateStr, sizeof(dateStr), dateLen);
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

template<class ImplClass>
WEAVE_ERROR GenericConfigurationManagerImpl<ImplClass>::_StoreManufacturingDate(const char * mfgDate)
{
    return Impl()->WriteConfigValueStr(ImplClass::kConfigKey_ManufacturingDate, mfgDate);
}

template<class ImplClass>
WEAVE_ERROR GenericConfigurationManagerImpl<ImplClass>::_GetDeviceCertificate(uint8_t * buf, size_t bufSize, size_t & certLen)
{
    WEAVE_ERROR err;

    err = Impl()->ReadConfigValueBin(ImplClass::kConfigKey_DeviceCert, buf, bufSize, certLen);

#if WEAVE_DEVICE_CONFIG_ENABLE_TEST_DEVICE_IDENTITY

    if (err == WEAVE_DEVICE_ERROR_CONFIG_NOT_FOUND)
    {
        certLen = TestDeviceCertLength;
        VerifyOrExit(buf != NULL, err = WEAVE_NO_ERROR);
        VerifyOrExit(TestDeviceCertLength <= bufSize, err = WEAVE_ERROR_BUFFER_TOO_SMALL);
        WeaveLogProgress(DeviceLayer, "Device certificate not found; using default");
        memcpy(buf, TestDeviceCert, TestDeviceCertLength);
        err = WEAVE_NO_ERROR;
    }

#endif // WEAVE_DEVICE_CONFIG_ENABLE_TEST_DEVICE_IDENTITY

    SuccessOrExit(err);

exit:
    return err;
}

template<class ImplClass>
WEAVE_ERROR GenericConfigurationManagerImpl<ImplClass>::_StoreDeviceCertificate(const uint8_t * cert, size_t certLen)
{
    return Impl()->WriteConfigValueBin(ImplClass::kConfigKey_DeviceCert, cert, certLen);
}

template<class ImplClass>
WEAVE_ERROR GenericConfigurationManagerImpl<ImplClass>::_GetDevicePrivateKey(uint8_t * buf, size_t bufSize, size_t & keyLen)
{
    WEAVE_ERROR err;

    err = Impl()->ReadConfigValueBin(ImplClass::kConfigKey_DevicePrivateKey, buf, bufSize, keyLen);

#if WEAVE_DEVICE_CONFIG_ENABLE_TEST_DEVICE_IDENTITY

    if (err == WEAVE_DEVICE_ERROR_CONFIG_NOT_FOUND)
    {
        keyLen = TestDevicePrivateKeyLength;
        VerifyOrExit(buf != NULL, err = WEAVE_NO_ERROR);
        VerifyOrExit(TestDevicePrivateKeyLength <= bufSize, err = WEAVE_ERROR_BUFFER_TOO_SMALL);
        WeaveLogProgress(DeviceLayer, "Device private key not found; using default");
        memcpy(buf, TestDevicePrivateKey, TestDevicePrivateKeyLength);
        err = WEAVE_NO_ERROR;
    }

#endif // WEAVE_DEVICE_CONFIG_ENABLE_TEST_DEVICE_IDENTITY

    SuccessOrExit(err);

exit:
    return err;
}

template<class ImplClass>
WEAVE_ERROR GenericConfigurationManagerImpl<ImplClass>::_StoreDevicePrivateKey(const uint8_t * key, size_t keyLen)
{
    return Impl()->WriteConfigValueBin(ImplClass::kConfigKey_DevicePrivateKey, key, keyLen);
}

template<class ImplClass>
WEAVE_ERROR GenericConfigurationManagerImpl<ImplClass>::_GetPairingCode(char * buf, size_t bufSize, size_t & pairingCodeLen)
{
    WEAVE_ERROR err;

    err = Impl()->ReadConfigValueStr(ImplClass::kConfigKey_PairingCode, buf, bufSize, pairingCodeLen);
    // TODO: change to WEAVE_DEVICE_CONFIG_USE_TEST_PAIRING_CODE??
#ifdef CONFIG_USE_TEST_PAIRING_CODE
    if (CONFIG_USE_TEST_PAIRING_CODE[0] != 0 && err == WEAVE_DEVICE_ERROR_CONFIG_NOT_FOUND)
    {
        VerifyOrExit(sizeof(CONFIG_USE_TEST_PAIRING_CODE) <= bufSize, err = WEAVE_ERROR_BUFFER_TOO_SMALL);
        memcpy(buf, CONFIG_USE_TEST_PAIRING_CODE, sizeof(CONFIG_USE_TEST_PAIRING_CODE));
        pairingCodeLen = sizeof(CONFIG_USE_TEST_PAIRING_CODE) - 1;
        WeaveLogProgress(DeviceLayer, "Pairing code not found; using default: %s", CONFIG_USE_TEST_PAIRING_CODE);
        err = WEAVE_NO_ERROR;
    }
#endif // CONFIG_USE_TEST_PAIRING_CODE
    SuccessOrExit(err);

exit:
    return err;
}

template<class ImplClass>
WEAVE_ERROR GenericConfigurationManagerImpl<ImplClass>::_StorePairingCode(const char * pairingCode)
{
    return Impl()->WriteConfigValueStr(ImplClass::kConfigKey_PairingCode, pairingCode);
}

template<class ImplClass>
WEAVE_ERROR GenericConfigurationManagerImpl<ImplClass>::_GetFabricId(uint64_t & fabricId)
{
    return Impl()->ReadConfigValue(ImplClass::kConfigKey_FabricId, fabricId);
}

template<class ImplClass>
WEAVE_ERROR GenericConfigurationManagerImpl<ImplClass>::_StoreFabricId(uint64_t fabricId)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (fabricId != kFabricIdNotSpecified)
    {
        err = Impl()->WriteConfigValue(ImplClass::kConfigKey_FabricId, fabricId);
        SuccessOrExit(err);
        SetFlag(mFlags, kFlag_IsMemberOfFabric);
    }
    else
    {
        ClearFlag(mFlags, kFlag_IsMemberOfFabric);
        err = Impl()->ClearConfigValue(ImplClass::kConfigKey_FabricId);
        SuccessOrExit(err);
    }

exit:
    return err;
}

template<class ImplClass>
WEAVE_ERROR GenericConfigurationManagerImpl<ImplClass>::_GetServiceId(uint64_t & serviceId)
{
    return Impl()->ReadConfigValue(ImplClass::kConfigKey_ServiceId, serviceId);
}

template<class ImplClass>
WEAVE_ERROR GenericConfigurationManagerImpl<ImplClass>::_GetServiceConfig(uint8_t * buf, size_t bufSize, size_t & serviceConfigLen)
{
    return Impl()->ReadConfigValueBin(ImplClass::kConfigKey_ServiceConfig, buf, bufSize, serviceConfigLen);
}

template<class ImplClass>
WEAVE_ERROR GenericConfigurationManagerImpl<ImplClass>::_StoreServiceConfig(const uint8_t * serviceConfig, size_t serviceConfigLen)
{
    return Impl()->WriteConfigValueBin(ImplClass::kConfigKey_ServiceConfig, serviceConfig, serviceConfigLen);
}

template<class ImplClass>
WEAVE_ERROR GenericConfigurationManagerImpl<ImplClass>::_GetPairedAccountId(char * buf, size_t bufSize, size_t & accountIdLen)
{
    return Impl()->ReadConfigValueStr(ImplClass::kConfigKey_PairedAccountId, buf, bufSize, accountIdLen);
}

template<class ImplClass>
WEAVE_ERROR GenericConfigurationManagerImpl<ImplClass>::_StorePairedAccountId(const char * accountId, size_t accountIdLen)
{
    WEAVE_ERROR err;

    err = Impl()->WriteConfigValueStr(ImplClass::kConfigKey_PairedAccountId, accountId, accountIdLen);
    SuccessOrExit(err);

    SetFlag(mFlags, kFlag_IsPairedToAccount, (accountId != NULL && accountIdLen != 0));

exit:
    return err;
}

template<class ImplClass>
WEAVE_ERROR GenericConfigurationManagerImpl<ImplClass>::_StoreServiceProvisioningData(uint64_t serviceId,
        const uint8_t * serviceConfig, size_t serviceConfigLen,
        const char * accountId, size_t accountIdLen)
{
    WEAVE_ERROR err;

    err = Impl()->WriteConfigValue(ImplClass::kConfigKey_ServiceId, serviceId);
    SuccessOrExit(err);

    err = _StoreServiceConfig(serviceConfig, serviceConfigLen);
    SuccessOrExit(err);

    err = _StorePairedAccountId(accountId, accountIdLen);
    SuccessOrExit(err);

    SetFlag(mFlags, kFlag_IsServiceProvisioned);
    SetFlag(mFlags, kFlag_IsPairedToAccount, (accountId != NULL && accountIdLen != 0));

exit:
    if (err != WEAVE_NO_ERROR)
    {
        Impl()->ClearConfigValue(ImplClass::kConfigKey_ServiceId);
        Impl()->ClearConfigValue(ImplClass::kConfigKey_ServiceConfig);
        Impl()->ClearConfigValue(ImplClass::kConfigKey_PairedAccountId);
        ClearFlag(mFlags, kFlag_IsServiceProvisioned);
        ClearFlag(mFlags, kFlag_IsPairedToAccount);
    }
    return err;
}

template<class ImplClass>
WEAVE_ERROR GenericConfigurationManagerImpl<ImplClass>::_ClearServiceProvisioningData()
{
    Impl()->ClearConfigValue(ImplClass::kConfigKey_ServiceId);
    Impl()->ClearConfigValue(ImplClass::kConfigKey_ServiceConfig);
    Impl()->ClearConfigValue(ImplClass::kConfigKey_PairedAccountId);

    // TODO: Move these behaviors out of configuration manager.

    // If necessary, post an event alerting other subsystems to the change in
    // the account pairing state.
    if (_IsPairedToAccount())
    {
        WeaveDeviceEvent event;
        event.Type = DeviceEventType::kAccountPairingChange;
        event.AccountPairingChange.IsPairedToAccount = false;
        PlatformMgr().PostEvent(&event);
    }

    // If necessary, post an event alerting other subsystems to the change in
    // the service provisioning state.
    if (_IsServiceProvisioned())
    {
        WeaveDeviceEvent event;
        event.Type = DeviceEventType::kServiceProvisioningChange;
        event.ServiceProvisioningChange.IsServiceProvisioned = false;
        event.ServiceProvisioningChange.ServiceConfigUpdated = false;
        PlatformMgr().PostEvent(&event);
    }

    ClearFlag(mFlags, kFlag_IsServiceProvisioned);
    ClearFlag(mFlags, kFlag_IsPairedToAccount);

    return WEAVE_NO_ERROR;
}

template<class ImplClass>
WEAVE_ERROR GenericConfigurationManagerImpl<ImplClass>::_GetFailSafeArmed(bool & val)
{
    return Impl()->ReadConfigValue(ImplClass::kConfigKey_FailSafeArmed, val);
}

template<class ImplClass>
WEAVE_ERROR GenericConfigurationManagerImpl<ImplClass>::_SetFailSafeArmed(bool val)
{
    return Impl()->WriteConfigValue(ImplClass::kConfigKey_FailSafeArmed, val);
}

template<class ImplClass>
WEAVE_ERROR GenericConfigurationManagerImpl<ImplClass>::_GetDeviceDescriptor(::nl::Weave::Profiles::DeviceDescription::WeaveDeviceDescriptor & deviceDesc)
{
    WEAVE_ERROR err;
    size_t outLen;

    deviceDesc.Clear();

    deviceDesc.DeviceId = FabricState.LocalNodeId;

    deviceDesc.FabricId = FabricState.FabricId;

    err = Impl()->_GetVendorId(deviceDesc.VendorId);
    SuccessOrExit(err);

    err = Impl()->_GetProductId(deviceDesc.ProductId);
    SuccessOrExit(err);

    err = Impl()->_GetProductRevision(deviceDesc.ProductRevision);
    SuccessOrExit(err);

    err = Impl()->_GetManufacturingDate(deviceDesc.ManufacturingDate.Year, deviceDesc.ManufacturingDate.Month, deviceDesc.ManufacturingDate.Day);
    if (err == WEAVE_DEVICE_ERROR_CONFIG_NOT_FOUND)
    {
        err = WEAVE_NO_ERROR;
    }
    SuccessOrExit(err);

    err = Impl()->_GetPrimaryWiFiMACAddress(deviceDesc.PrimaryWiFiMACAddress);
    if (err == WEAVE_DEVICE_ERROR_CONFIG_NOT_FOUND)
    {
        err = WEAVE_NO_ERROR;
    }
    SuccessOrExit(err);

    err = Impl()->_GetPrimary802154MACAddress(deviceDesc.Primary802154MACAddress);
    if (err == WEAVE_DEVICE_ERROR_CONFIG_NOT_FOUND)
    {
        err = WEAVE_NO_ERROR;
    }
    SuccessOrExit(err);

    err = Impl()->_GetWiFiAPSSID(deviceDesc.RendezvousWiFiESSID, sizeof(deviceDesc.RendezvousWiFiESSID));
    if (err == WEAVE_DEVICE_ERROR_CONFIG_NOT_FOUND)
    {
        err = WEAVE_NO_ERROR;
    }
    SuccessOrExit(err);

    err = Impl()->_GetSerialNumber(deviceDesc.SerialNumber, sizeof(deviceDesc.SerialNumber), outLen);
    if (err == WEAVE_DEVICE_ERROR_CONFIG_NOT_FOUND)
    {
        err = WEAVE_NO_ERROR;
    }
    SuccessOrExit(err);

    err = Impl()->_GetFirmwareRevision(deviceDesc.SoftwareVersion, sizeof(deviceDesc.SoftwareVersion), outLen);
    if (err == WEAVE_DEVICE_ERROR_CONFIG_NOT_FOUND)
    {
        err = WEAVE_NO_ERROR;
    }
    SuccessOrExit(err);

exit:
    return err;
}

template<class ImplClass>
WEAVE_ERROR GenericConfigurationManagerImpl<ImplClass>::_GetDeviceDescriptorTLV(uint8_t * buf, size_t bufSize, size_t & encodedLen)
{
    WEAVE_ERROR err;
    ::nl::Weave::Profiles::DeviceDescription::WeaveDeviceDescriptor deviceDesc;

    err = Impl()->_GetDeviceDescriptor(deviceDesc);
    SuccessOrExit(err);

    {
        uint32_t tmp = 0;
        err = ::nl::Weave::Profiles::DeviceDescription::WeaveDeviceDescriptor::EncodeTLV(deviceDesc, buf, (uint32_t)bufSize, tmp);
        SuccessOrExit(err);
        encodedLen = tmp;
    }


exit:
    return err;
}

template<class ImplClass>
WEAVE_ERROR GenericConfigurationManagerImpl<ImplClass>::_GetQRCodeString(char * buf, size_t bufSize)
{
    WEAVE_ERROR err;
    ::nl::Weave::Profiles::DeviceDescription::WeaveDeviceDescriptor deviceDesc;
    uint32_t encodedLen;

    err = Impl()->_GetDeviceDescriptor(deviceDesc);
    SuccessOrExit(err);

    strncpy(deviceDesc.PairingCode, FabricState.PairingCode, ::nl::Weave::Profiles::DeviceDescription::WeaveDeviceDescriptor::kMaxPairingCodeLength);
    deviceDesc.PairingCode[::nl::Weave::Profiles::DeviceDescription::WeaveDeviceDescriptor::kMaxPairingCodeLength] = 0;

    err = ::nl::Weave::Profiles::DeviceDescription::WeaveDeviceDescriptor::EncodeText(deviceDesc, buf, (uint32_t)bufSize, encodedLen);
    SuccessOrExit(err);

exit:
    return err;
}

template<class ImplClass>
WEAVE_ERROR GenericConfigurationManagerImpl<ImplClass>::_GetWiFiAPSSID(char * buf, size_t bufSize)
{
    WEAVE_ERROR err;

#ifdef WEAVE_DEVICE_CONFIG_WIFI_AP_SSID_PREFIX

    uint8_t mac[6];

    VerifyOrExit(bufSize >= sizeof(WEAVE_DEVICE_CONFIG_WIFI_AP_SSID_PREFIX) + 4, err = WEAVE_ERROR_BUFFER_TOO_SMALL);

    err = Impl()->_GetPrimaryWiFiMACAddress(mac);
    SuccessOrExit(err);

    snprintf(buf, bufSize, "%s%02X%02X", WEAVE_DEVICE_CONFIG_WIFI_AP_SSID_PREFIX, mac[4], mac[5]);
    buf[bufSize - 1] = 0;

#else // WEAVE_DEVICE_CONFIG_WIFI_AP_SSID_PREFIX

    ExitNow(err = WEAVE_DEVICE_ERROR_CONFIG_NOT_FOUND);

#endif // WEAVE_DEVICE_CONFIG_WIFI_AP_SSID_PREFIX

exit:
    return err;
}

template<class ImplClass>
bool GenericConfigurationManagerImpl<ImplClass>::_IsServiceProvisioned()
{
    return ::nl::GetFlag(mFlags, kFlag_IsServiceProvisioned);
}

template<class ImplClass>
bool GenericConfigurationManagerImpl<ImplClass>::_IsMemberOfFabric()
{
    return ::nl::GetFlag(mFlags, kFlag_IsMemberOfFabric);
}

template<class ImplClass>
bool GenericConfigurationManagerImpl<ImplClass>::_IsPairedToAccount()
{
    return ::nl::GetFlag(mFlags, kFlag_IsPairedToAccount);
}

#if WEAVE_PROGRESS_LOGGING

template<class ImplClass>
void GenericConfigurationManagerImpl<ImplClass>::LogDeviceConfig()
{
    WEAVE_ERROR err;

    WeaveLogProgress(DeviceLayer, "Device Configuration:");

    WeaveLogProgress(DeviceLayer, "  Device Id: %016" PRIX64, FabricState.LocalNodeId);

    {
        char serialNum[ConfigurationManager::kMaxSerialNumberLength];
        size_t serialNumLen;
        err = Impl()->_GetSerialNumber(serialNum, sizeof(serialNum), serialNumLen);
        WeaveLogProgress(DeviceLayer, "  Serial Number: %s", (err == WEAVE_NO_ERROR) ? serialNum : "(not set)");
    }

    {
        uint16_t vendorId;
        if (Impl()->_GetVendorId(vendorId) != WEAVE_NO_ERROR)
        {
            vendorId = 0;
        }
        WeaveLogProgress(DeviceLayer, "  Vendor Id: %" PRId16 " (0x%" PRIX16 ")%s",
                vendorId, vendorId, (vendorId == kWeaveVendor_NestLabs) ? " (Nest)" : "");
    }

    {
        uint16_t productId;
        if (Impl()->_GetProductId(productId) != WEAVE_NO_ERROR)
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

    WeaveLogProgress(DeviceLayer, "  Pairing Code: %s", (FabricState.PairingCode != NULL) ? FabricState.PairingCode : "(none)");
}

#endif // WEAVE_PROGRESS_LOGGING

} // namespace Internal
} // namespace DeviceLayer
} // namespace Weave
} // namespace nl

#endif // GENERIC_CONFIGURATION_MANAGER_IMPL_IPP
