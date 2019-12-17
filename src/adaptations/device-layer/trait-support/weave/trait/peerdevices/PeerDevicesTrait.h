
/*
 *    Copyright (c) 2019 Google LLC.
 *    Copyright (c) 2016-2018 Nest Labs, Inc.
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

/*
 *    THIS FILE IS GENERATED. DO NOT MODIFY.
 *
 *    SOURCE TEMPLATE: trait.cpp.h
 *    SOURCE PROTO: weave/trait/peerdevices/peer_devices_trait.proto
 *
 */
#ifndef _WEAVE_TRAIT_PEERDEVICES__PEER_DEVICES_TRAIT_H_
#define _WEAVE_TRAIT_PEERDEVICES__PEER_DEVICES_TRAIT_H_

#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Support/SerializationUtils.h>



namespace Schema {
namespace Weave {
namespace Trait {
namespace Peerdevices {
namespace PeerDevicesTrait {

extern const nl::Weave::Profiles::DataManagement::TraitSchemaEngine TraitSchema;

enum {
      kWeaveProfileId = (0x0U << 16) | 0x1301U
};

//
// Properties
//

enum {
    kPropertyHandle_Root = 1,

    //---------------------------------------------------------------------------------------------------------------------------//
    //  Name                                IDL Type                            TLV Type           Optional?       Nullable?     //
    //---------------------------------------------------------------------------------------------------------------------------//

    //
    //  peer_devices                        map <uint32,PeerDevice>              map <uint16, structure>NO              NO
    //
    kPropertyHandle_PeerDevices = 2,

    //
    //  value                               PeerDevice                           structure         NO              NO
    //
    kPropertyHandle_PeerDevices_Value = 3,

    //
    //  device_id                           weave.common.ResourceId              uint64            NO              NO
    //
    kPropertyHandle_PeerDevices_Value_DeviceId = 4,

    //
    //  resource_type_name                  weave.common.ResourceName            string            NO              NO
    //
    kPropertyHandle_PeerDevices_Value_ResourceTypeName = 5,

    //
    //  vendor_id                           uint32                               uint16            NO              NO
    //
    kPropertyHandle_PeerDevices_Value_VendorId = 6,

    //
    //  product_id                          uint32                               uint16            NO              NO
    //
    kPropertyHandle_PeerDevices_Value_ProductId = 7,

    //
    //  software_version                    string                               string            NO              NO
    //
    kPropertyHandle_PeerDevices_Value_SoftwareVersion = 8,

    //
    //  device_ready                        bool                                 bool              NO              NO
    //
    kPropertyHandle_PeerDevices_Value_DeviceReady = 9,

    //
    // Enum for last handle
    //
    kLastSchemaHandle = 9,
};

//
// Event Structs
//

struct PeerDevice
{
    uint64_t deviceId;
    const char * resourceTypeName;
    uint16_t vendorId;
    uint16_t productId;
    const char * softwareVersion;
    bool deviceReady;

    static const nl::SchemaFieldDescriptor FieldSchema;

};

struct PeerDevice_array {
    uint32_t num;
    PeerDevice *buf;
};

} // namespace PeerDevicesTrait
} // namespace Peerdevices
} // namespace Trait
} // namespace Weave
} // namespace Schema
#endif // _WEAVE_TRAIT_PEERDEVICES__PEER_DEVICES_TRAIT_H_
