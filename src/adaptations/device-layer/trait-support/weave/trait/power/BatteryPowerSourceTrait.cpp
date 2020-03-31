
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
 *    SOURCE PROTO: weave/trait/power/battery_power_source_trait.proto
 *
 */

#include <weave/trait/power/BatteryPowerSourceTrait.h>

namespace Schema {
namespace Weave {
namespace Trait {
namespace Power {
namespace BatteryPowerSourceTrait {

using namespace ::nl::Weave::Profiles::DataManagement;

//
// Property Table
//

const TraitSchemaEngine::PropertyInfo PropertyMap[] = {
    { kPropertyHandle_Root, 1 }, // type
    { kPropertyHandle_Root, 2 }, // assessed_voltage
    { kPropertyHandle_Root, 3 }, // assessed_current
    { kPropertyHandle_Root, 4 }, // assessed_frequency
    { kPropertyHandle_Root, 5 }, // condition
    { kPropertyHandle_Root, 6 }, // status
    { kPropertyHandle_Root, 7 }, // present
    { kPropertyHandle_Root, 32 }, // replacement_indicator
    { kPropertyHandle_Root, 33 }, // remaining
    { kPropertyHandle_Remaining, 1 }, // remaining_percent
    { kPropertyHandle_Remaining, 2 }, // remaining_time
    { kPropertyHandle_Remaining_RemainingTime, 1 }, // time
    { kPropertyHandle_Remaining_RemainingTime, 2 }, // time_basis
};

//
// IsOptional Table
//

uint8_t IsOptionalHandleBitfield[] = {
        0x8e, 0x7
};

//
// IsNullable Table
//

uint8_t IsNullableHandleBitfield[] = {
        0xe, 0x7
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
        9,
#endif
        NULL,
        &IsOptionalHandleBitfield[0],
        NULL,
        &IsNullableHandleBitfield[0],
        NULL,
#if (TDM_EXTENSION_SUPPORT)
        &Weave::Trait::Power::PowerSourceTrait::TraitSchema,
#endif
#if (TDM_VERSIONING_SUPPORT)
        NULL,
#endif
    }
};

    //
    // Events
    //

const nl::FieldDescriptor BatteryChangedEventFieldDescriptors[] =
{
    {
        NULL, offsetof(BatteryChangedEvent, condition), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 1
    },

    {
        NULL, offsetof(BatteryChangedEvent, status), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 2
    },

    {
        NULL, offsetof(BatteryChangedEvent, replacementIndicator), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 16
    },

    {
        &Schema::Weave::Trait::Power::BatteryPowerSourceTrait::BatteryRemaining::FieldSchema, offsetof(BatteryChangedEvent, remaining), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeStructure, 0), 17
    },

};

const nl::SchemaFieldDescriptor BatteryChangedEvent::FieldSchema =
{
    .mNumFieldDescriptorElements = sizeof(BatteryChangedEventFieldDescriptors)/sizeof(BatteryChangedEventFieldDescriptors[0]),
    .mFields = BatteryChangedEventFieldDescriptors,
    .mSize = sizeof(BatteryChangedEvent)
};
const nl::Weave::Profiles::DataManagement::EventSchema BatteryChangedEvent::Schema =
{
    .mProfileId = kWeaveProfileId,
    .mStructureType = 0x1,
    .mImportance = nl::Weave::Profiles::DataManagement::Production,
    .mDataSchemaVersion = 1,
    .mMinCompatibleDataSchemaVersion = 1,
};

//
// Event Structs
//

const nl::FieldDescriptor BatteryRemainingFieldDescriptors[] =
{
    {
        NULL, offsetof(BatteryRemaining, remainingPercent), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt8, 1), 1
    },

    {
        &Schema::Weave::Common::Timer::FieldSchema, offsetof(BatteryRemaining, remainingTime), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeStructure, 1), 2
    },

};

const nl::SchemaFieldDescriptor BatteryRemaining::FieldSchema =
{
    .mNumFieldDescriptorElements = sizeof(BatteryRemainingFieldDescriptors)/sizeof(BatteryRemainingFieldDescriptors[0]),
    .mFields = BatteryRemainingFieldDescriptors,
    .mSize = sizeof(BatteryRemaining)
};

} // namespace BatteryPowerSourceTrait
} // namespace Power
} // namespace Trait
} // namespace Weave
} // namespace Schema
