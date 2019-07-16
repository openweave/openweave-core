/*
 *
 *    Copyright (c) 2019 Google LLC.
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
 *          Contains non-inline method definitions for the FactoryProvisioningBase
 *          template.
 */

#ifndef FACTORY_PROVISIONING_IPP
#define FACTORY_PROVISIONING_IPP

#include <Weave/DeviceLayer/internal/WeaveDeviceLayerInternal.h>
#include <Weave/DeviceLayer/internal/FactoryProvisioning.h>

namespace nl {
namespace Weave {
namespace DeviceLayer {
namespace Internal {

// Fully instantiate the default FactoryProvisioningBase class.
template class FactoryProvisioningBase<FactoryProvisioning>;

template<class DerivedClass>
WEAVE_ERROR FactoryProvisioningBase<DerivedClass>::ProvisionDeviceFromRAM(uint8_t * memRangeStart, uint8_t * memRangeEnd)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint8_t * dataStart;
    size_t dataLen;

    // Search the given RAM region for device provisioning data.  If found...
    if (Derived()->LocateProvisioningData(memRangeStart, memRangeEnd, dataStart, dataLen))
    {
        // Wipe the provisioning data marker so that it will not be found again should the device reboot.
        memset(dataStart - FactoryProvisioningData::kMarkerLen, 0, FactoryProvisioningData::kMarkerLen);

        // Parse the provisioning data TLV and write the values to persistent storage.
        TLV::TLVReader reader;
        reader.Init(dataStart, dataLen);
        err = Derived()->StoreProvisioningData(reader);
        SuccessOrExit(err);

        // Wipe the provisioning data itself.
        Crypto::ClearSecretData(dataStart, dataLen);
    }

exit:
    return err;
}

template<class DerivedClass>
bool FactoryProvisioningBase<DerivedClass>::LocateProvisioningData(uint8_t * memRangeStart, uint8_t * memRangeEnd, uint8_t * & dataStart, size_t & dataLen)
{
    using HashAlgo = Platform::Security::SHA256;

    constexpr size_t kLenFieldLen = sizeof(uint32_t);

    WeaveLogProgress(DeviceLayer, "Searching for factory provisioning data (0x%08" PRIX32 " - 0x%08" PRIX32 ")",
            memRangeStart, memRangeEnd);

    for (uint8_t * p = memRangeStart; p < memRangeEnd; p++)
    {
        // Search for the provisioning data marker within the given memory range.
        p = (uint8_t *)memchr(p, FactoryProvisioningData::kMarker[0], memRangeEnd - p);
        if (p == NULL)
            break;
        if (strncmp((char *)p, FactoryProvisioningData::kMarker, FactoryProvisioningData::kMarkerLen) != 0)
            continue;

        // Read the provisioning data length located immediately after the marker.
        dataLen = (size_t)Encoding::LittleEndian::Get32(p + FactoryProvisioningData::kMarkerLen);

        // Locate the start of the TLV-encoded provisioning data.
        dataStart = p + FactoryProvisioningData::kMarkerLen + kLenFieldLen;

        // Continue searching if the stated data length + the hash length exceeds the given memory range.
        if ((dataLen + HashAlgo::kHashLength) >= (size_t)(memRangeEnd - dataStart))
            break;

        const uint8_t * givenHash = dataStart + dataLen;

        // Compute the expected hash value.
        uint8_t expectedHash[HashAlgo::kHashLength];
        {
            HashAlgo hash;
            hash.Begin();
            hash.AddData(p, FactoryProvisioningData::kMarkerLen + kLenFieldLen + dataLen);
            hash.Finish(expectedHash);
        }

        // If the hash values match, return success.
        if (memcmp(givenHash, expectedHash, HashAlgo::kHashLength) == 0)
        {
            WeaveLogProgress(DeviceLayer, "Found factory provisioning data at 0x%08" PRIX32 " (len %" PRIu32 ")",
                    dataStart, dataLen);
            return true;
        }
    }

    WeaveLogProgress(DeviceLayer, "No factory provisioning data found");

    // No provisioning data found.
    return false;
}

template<class DerivedClass>
WEAVE_ERROR FactoryProvisioningBase<DerivedClass>::StoreProvisioningData(TLV::TLVReader & reader)
{
    WEAVE_ERROR err;
    TLV::TLVType outerContainer;

    err = reader.Next();
    SuccessOrExit(err);

    err = reader.EnterContainer(outerContainer);
    SuccessOrExit(err);

    // Iterate over the fields in the provisioning data container, calling the StoreProvisioningValue()
    // method for each.
    while ((err = reader.Next()) == WEAVE_NO_ERROR)
    {
        uint64_t tag = reader.GetTag();

        // Ignore non-context tags
        if (!TLV::IsContextTag(tag))
            continue;

        err = Derived()->StoreProvisioningValue((uint8_t)TLV::TagNumFromTag(tag), reader);
        SuccessOrExit(err);
    }
    if (err == WEAVE_END_OF_TLV)
        err = WEAVE_NO_ERROR;
    SuccessOrExit(err);

exit:
    return err;
}

template<class DerivedClass>
WEAVE_ERROR FactoryProvisioningBase<DerivedClass>::StoreProvisioningValue(uint8_t tagNum, TLV::TLVReader & reader)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // Based on the supplied tag number, call the appropriate method on the COnfigurationManager
    // object to store the provisioned value...
    switch (tagNum)
    {

    case FactoryProvisioningData::kTag_SerialNumber:
    {
        const uint8_t * serialNum;
        err = reader.GetDataPtr(serialNum);
        SuccessOrExit(err);
        err = ConfigurationMgr().StoreSerialNumber((const char *)serialNum, reader.GetLength());
        SuccessOrExit(err);
        break;
    }

    case FactoryProvisioningData::kTag_DeviceId:
    {
        uint64_t deviceId;
        err = reader.Get(deviceId);
        SuccessOrExit(err);
        err = ConfigurationMgr().StoreManufAttestDeviceId(deviceId);
        SuccessOrExit(err);
        break;
    }

    case FactoryProvisioningData::kTag_DeviceCert:
    {
        const uint8_t * cert;
        err = reader.GetDataPtr(cert);
        SuccessOrExit(err);
        err = ConfigurationMgr().StoreManufAttestDeviceCertificate(cert, reader.GetLength());
        SuccessOrExit(err);
        break;
    }

    case FactoryProvisioningData::kTag_DeviceICACerts:
    {
        const uint8_t * certs;
        err = reader.GetDataPtr(certs);
        SuccessOrExit(err);
        err = ConfigurationMgr().StoreManufAttestDeviceICACerts(certs, reader.GetLength());
        SuccessOrExit(err);
        break;
    }

    case FactoryProvisioningData::kTag_DevicePrivateKey:
    {
        const uint8_t * privKey;
        err = reader.GetDataPtr(privKey);
        SuccessOrExit(err);
        err = ConfigurationMgr().StoreManufAttestDevicePrivateKey(privKey, reader.GetLength());
        SuccessOrExit(err);
        break;
    }

    case FactoryProvisioningData::kTag_PairingCode:
    {
        const uint8_t * pairingCode;
        err = reader.GetDataPtr(pairingCode);
        SuccessOrExit(err);
        err = ConfigurationMgr().StorePairingCode((const char *)pairingCode, reader.GetLength());
        SuccessOrExit(err);
        break;
    }

    case FactoryProvisioningData::kTag_MfgDate:
    {
        const uint8_t * mfgDate;
        err = reader.GetDataPtr(mfgDate);
        SuccessOrExit(err);
        err = ConfigurationMgr().StoreManufacturingDate((const char *)mfgDate, reader.GetLength());
        SuccessOrExit(err);
        break;
    }

    case FactoryProvisioningData::kTag_ProductRev:
    {
        uint32_t productRev;
        err = reader.Get(productRev);
        SuccessOrExit(err);
        err = ConfigurationMgr().StoreProductRevision((uint16_t)productRev);
        SuccessOrExit(err);
        break;
    }

    // Ignore unrecognized/supported tags.
    default:
        break;
    }

exit:
    return err;
}


} // namespace Internal
} // namespace DeviceLayer
} // namespace Weave
} // namespace nl

#endif // FACTORY_PROVISIONING_IPP
