
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
 *    SOURCE PROTO: weave/trait/power/battery_power_source_capabilities_trait.proto
 *
 */

#include <weave/trait/power/BatteryPowerSourceCapabilitiesTrait.h>

namespace Schema {
namespace Weave {
namespace Trait {
namespace Power {
namespace BatteryPowerSourceCapabilitiesTrait {

using namespace ::nl::Weave::Profiles::DataManagement;

//
// Property Table
//

const TraitSchemaEngine::PropertyInfo PropertyMap[] = {
    { kPropertyHandle_Root, 1 }, // type
    { kPropertyHandle_Root, 2 }, // description
    { kPropertyHandle_Root, 3 }, // nominal_voltage
    { kPropertyHandle_Root, 4 }, // maximum_current
    { kPropertyHandle_Root, 5 }, // current_type
    { kPropertyHandle_Root, 6 }, // order
    { kPropertyHandle_Root, 7 }, // removable
    { kPropertyHandle_Root, 32 }, // rechargeable
    { kPropertyHandle_Root, 33 }, // capacity
    { kPropertyHandle_Root, 34 }, // chemistry
    { kPropertyHandle_Root, 35 }, // count
    { kPropertyHandle_Root, 36 }, // replaceable
    { kPropertyHandle_Root, 37 }, // designations
    { kPropertyHandle_Designations, 1 }, // designation_description
    { kPropertyHandle_Designations, 2 }, // common_designation_identifier
    { kPropertyHandle_Designations, 3 }, // ansi_designation_identifier
    { kPropertyHandle_Designations, 4 }, // iec_designation_identifier
};

//
// IsOptional Table
//

uint8_t IsOptionalHandleBitfield[] = {
        0xa, 0xf1, 0x1
};

//
// IsNullable Table
//

uint8_t IsNullableHandleBitfield[] = {
        0xa, 0x31, 0x0
};

//
// Schema
//

const TraitSchemaEngine TraitSchema = {
    {
        kWeaveProfileId,
        PropertyMap,
        sizeof(PropertyMap) / sizeof(PropertyMap[0]),
        2,
#if (TDM_EXTENSION_SUPPORT) || (TDM_VERSIONING_SUPPORT)
        9,
#endif
        NULL,
        &IsOptionalHandleBitfield[0],
        NULL,
        &IsNullableHandleBitfield[0],
        NULL,
#if (TDM_EXTENSION_SUPPORT)
        &Weave::Trait::Power::PowerSourceCapabilitiesTrait::TraitSchema,
#endif
#if (TDM_VERSIONING_SUPPORT)
        NULL,
#endif
    }
};

//
// Event Structs
//

const nl::FieldDescriptor BatteryDesignationFieldDescriptors[] =
{
    {
        NULL, offsetof(BatteryDesignation, designationDescription), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUTF8String, 1), 1
    },

    {
        NULL, offsetof(BatteryDesignation, commonDesignationIdentifier), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 2
    },

    {
        NULL, offsetof(BatteryDesignation, ansiDesignationIdentifier), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 3
    },

    {
        NULL, offsetof(BatteryDesignation, iecDesignationIdentifier), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 4
    },

};

const nl::SchemaFieldDescriptor BatteryDesignation::FieldSchema =
{
    .mNumFieldDescriptorElements = sizeof(BatteryDesignationFieldDescriptors)/sizeof(BatteryDesignationFieldDescriptors[0]),
    .mFields = BatteryDesignationFieldDescriptors,
    .mSize = sizeof(BatteryDesignation)
};

} // namespace BatteryPowerSourceCapabilitiesTrait
} // namespace Power
} // namespace Trait
} // namespace Weave
} // namespace Schema
