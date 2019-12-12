
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
 *    SOURCE PROTO: weave/trait/power/power_source_trait.proto
 *
 */

#include <weave/trait/power/PowerSourceTrait.h>

namespace Schema {
namespace Weave {
namespace Trait {
namespace Power {
namespace PowerSourceTrait {

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
};

//
// IsOptional Table
//

uint8_t IsOptionalHandleBitfield[] = {
        0xe
};

//
// IsNullable Table
//

uint8_t IsNullableHandleBitfield[] = {
        0xe
};

//
// Schema
//

const TraitSchemaEngine TraitSchema = {
    {
        kWeaveProfileId,
        PropertyMap,
        sizeof(PropertyMap) / sizeof(PropertyMap[0]),
        1,
#if (TDM_EXTENSION_SUPPORT) || (TDM_VERSIONING_SUPPORT)
        2,
#endif
        NULL,
        &IsOptionalHandleBitfield[0],
        NULL,
        &IsNullableHandleBitfield[0],
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
    // Events
    //

const nl::FieldDescriptor PowerSourceChangedEventFieldDescriptors[] =
{
    {
        NULL, offsetof(PowerSourceChangedEvent, condition), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 1
    },

    {
        NULL, offsetof(PowerSourceChangedEvent, status), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 2
    },

};

const nl::SchemaFieldDescriptor PowerSourceChangedEvent::FieldSchema =
{
    .mNumFieldDescriptorElements = sizeof(PowerSourceChangedEventFieldDescriptors)/sizeof(PowerSourceChangedEventFieldDescriptors[0]),
    .mFields = PowerSourceChangedEventFieldDescriptors,
    .mSize = sizeof(PowerSourceChangedEvent)
};
const nl::Weave::Profiles::DataManagement::EventSchema PowerSourceChangedEvent::Schema =
{
    .mProfileId = kWeaveProfileId,
    .mStructureType = 0x1,
    .mImportance = nl::Weave::Profiles::DataManagement::Production,
    .mDataSchemaVersion = 1,
    .mMinCompatibleDataSchemaVersion = 1,
};

} // namespace PowerSourceTrait
} // namespace Power
} // namespace Trait
} // namespace Weave
} // namespace Schema
