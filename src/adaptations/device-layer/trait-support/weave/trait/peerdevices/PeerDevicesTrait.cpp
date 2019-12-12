
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
 *    SOURCE TEMPLATE: trait.cpp
 *    SOURCE PROTO: weave/trait/peerdevices/peer_devices_trait.proto
 *
 */

#include <weave/trait/peerdevices/PeerDevicesTrait.h>

namespace Schema {
namespace Weave {
namespace Trait {
namespace Peerdevices {
namespace PeerDevicesTrait {

using namespace ::nl::Weave::Profiles::DataManagement;

//
// Property Table
//

const TraitSchemaEngine::PropertyInfo PropertyMap[] = {
    { kPropertyHandle_Root, 1 }, // peer_devices
    { kPropertyHandle_PeerDevices, 0 }, // value
    { kPropertyHandle_PeerDevices_Value, 1 }, // device_id
    { kPropertyHandle_PeerDevices_Value, 2 }, // resource_type_name
    { kPropertyHandle_PeerDevices_Value, 3 }, // vendor_id
    { kPropertyHandle_PeerDevices_Value, 4 }, // product_id
    { kPropertyHandle_PeerDevices_Value, 5 }, // software_version
    { kPropertyHandle_PeerDevices_Value, 6 }, // device_ready
};

//
// IsDictionary Table
//

uint8_t IsDictionaryTypeHandleBitfield[] = {
        0x1
};

//
// Schema
//

const TraitSchemaEngine TraitSchema = {
    {
        kWeaveProfileId,
        PropertyMap,
        sizeof(PropertyMap) / sizeof(PropertyMap[0]),
        3,
#if (TDM_EXTENSION_SUPPORT) || (TDM_VERSIONING_SUPPORT)
        2,
#endif
        IsDictionaryTypeHandleBitfield,
        NULL,
        NULL,
        NULL,
        NULL,
#if (TDM_EXTENSION_SUPPORT)
        NULL,
#endif
#if (TDM_VERSIONING_SUPPORT)
        NULL,
#endif
    }
};

//
// Event Structs
//

const nl::FieldDescriptor PeerDeviceFieldDescriptors[] =
{
    {
        NULL, offsetof(PeerDevice, deviceId), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt64, 0), 1
    },

    {
        NULL, offsetof(PeerDevice, resourceTypeName), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUTF8String, 0), 2
    },

    {
        NULL, offsetof(PeerDevice, vendorId), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt16, 0), 3
    },

    {
        NULL, offsetof(PeerDevice, productId), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt16, 0), 4
    },

    {
        NULL, offsetof(PeerDevice, softwareVersion), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUTF8String, 0), 5
    },

    {
        NULL, offsetof(PeerDevice, deviceReady), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeBoolean, 0), 6
    },

};

const nl::SchemaFieldDescriptor PeerDevice::FieldSchema =
{
    .mNumFieldDescriptorElements = sizeof(PeerDeviceFieldDescriptors)/sizeof(PeerDeviceFieldDescriptors[0]),
    .mFields = PeerDeviceFieldDescriptors,
    .mSize = sizeof(PeerDevice)
};

} // namespace PeerDevicesTrait
} // namespace Peerdevices
} // namespace Trait
} // namespace Weave
} // namespace Schema
