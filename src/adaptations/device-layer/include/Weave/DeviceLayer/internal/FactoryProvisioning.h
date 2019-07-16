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
 *          Support for device factory provisioning.
 */

#ifndef FACTORY_PROVISIONING_H
#define FACTORY_PROVISIONING_H

#include <Weave/DeviceLayer/internal/WeaveDeviceLayerInternal.h>

namespace nl {
namespace Weave {
namespace TLV {
class TLVReader;
} // namespace TLV
} // namespace Weave
} // namespace nl

namespace nl {
namespace Weave {
namespace DeviceLayer {
namespace Internal {

/**
 * Supports device factory provisioning at boot time.
 *
 * The factory provisioning feature allows factory or developer-supplied provisioning information
 * to be injected into a device at boot time and automatically stored in persistent storage.
 * Provisioning information is written into device memory (typically RAM) by an external tool,
 * where it is picked by the OpenWeave initialization code and stored into persistent storage
 * early in the boot process.
 *
 * The factory provisioning feature allows the following values to be set:
 *
 * - Device Serial number
 * - Manufacturer-assigned Device Id
 * - Manufacturer-assigned Device Certificate
 * - Manufacturer-assigned Device Key
 * - Pairing Code
 * - Product Revision
 * - Manufacturing Date
 *
 * This template class provides a default base implementation of the device provisioning feature
 * that can be specialized as needed by compile-time derivation.
 */
template<class DerivedClass>
class FactoryProvisioningBase
{
public:
    WEAVE_ERROR ProvisionDeviceFromRAM(uint8_t * memRangeStart, uint8_t * memRangeEnd);

protected:
    bool LocateProvisioningData(uint8_t * memRangeStart, uint8_t * memRangeEnd, uint8_t * & dataStart, size_t & dataLen);
    WEAVE_ERROR StoreProvisioningData(TLV::TLVReader & reader);
    WEAVE_ERROR StoreProvisioningValue(uint8_t tagNum, TLV::TLVReader & reader);

private:
    DerivedClass * Derived() { return static_cast<DerivedClass *>(this); }
};

/**
 * Default implementation of the device factory provisioning feature.
 */
class FactoryProvisioning
    : public FactoryProvisioningBase<FactoryProvisioning>
{
};

namespace FactoryProvisioningData {

/**
 * Context-specific tags for Device Provisioning Data Weave TLV structure
 */
enum
{
    kTag_SerialNumber           = 0,    // [ utf-8 string ] Serial number
    kTag_DeviceId               = 1,    // [ uint, 64-bit max ] Manufacturer-assigned device id
    kTag_DeviceCert             = 2,    // [ byte string ] Manufacturer-assigned device certificate
    kTag_DevicePrivateKey       = 3,    // [ byte string ] Manufacturer-assigned device key
    kTag_PairingCode            = 4,    // [ utf-8 string ] Pairing code
    kTag_ProductRev             = 5,    // [ uint, 16-bit max ] Product revision
    kTag_MfgDate                = 6,    // [ utf-8 string ] Manufacturing date
    kTag_DeviceICACerts         = 7,    // [ byte string ] Manufacturer-assigned device intermediate CA certificates
};

/**
 * Marker used to mark the location of device provisioning data in memory.
 */
const char kMarker[] = "^OW-PROV-DATA^";
constexpr size_t kMarkerLen = sizeof(kMarker) - 1;

} // namespace FactoryProvisioningData



} // namespace Internal
} // namespace DeviceLayer
} // namespace Weave
} // namespace nl

#endif // FACTORY_PROVISIONING_H
