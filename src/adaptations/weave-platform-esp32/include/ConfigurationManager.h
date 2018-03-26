#ifndef CONFIGURATION_MANAGER_H
#define CONFIGURATION_MANAGER_H

#include <Weave/Profiles/device-description/DeviceDescription.h>

namespace WeavePlatform {

class ConfigurationManager
{
    friend class PlatformManager;

public:
    enum
    {
        kMaxPairingCodeLength = 15
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

    WEAVE_ERROR GetPersistedCounter(const char * key, uint32_t & value);
    WEAVE_ERROR StorePersistedCounter(const char * key, uint32_t value);

    WEAVE_ERROR GetDeviceDescriptor(nl::Weave::Profiles::DeviceDescription::WeaveDeviceDescriptor& deviceDesc);
    WEAVE_ERROR GetDeviceDescriptorTLV(uint8_t *buf, size_t bufSize, size_t & encodedLen);

    bool IsServiceProvisioned();

private:
    char mPairingCode[kMaxPairingCodeLength + 1];

    WEAVE_ERROR Init();
    WEAVE_ERROR ConfigureWeaveStack();
};

} // namespace WeavePlatform

#endif // CONFIGURATION_MANAGER_H
