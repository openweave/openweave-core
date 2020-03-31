
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
 *    SOURCE PROTO: weave/trait/security/tamper_trait.proto
 *
 */

#include <weave/trait/security/TamperTrait.h>

namespace Schema {
namespace Weave {
namespace Trait {
namespace Security {
namespace TamperTrait {

using namespace ::nl::Weave::Profiles::DataManagement;

//
// Property Table
//

const TraitSchemaEngine::PropertyInfo PropertyMap[] = {
    { kPropertyHandle_Root, 1 }, // tamper_state
    { kPropertyHandle_Root, 2 }, // first_observed_at
    { kPropertyHandle_Root, 3 }, // first_observed_at_ms
};

//
// IsOptional Table
//

uint8_t IsOptionalHandleBitfield[] = {
        0x6
};

//
// IsNullable Table
//

uint8_t IsNullableHandleBitfield[] = {
        0x6
};

//
// IsEphemeral Table
//

uint8_t IsEphemeralHandleBitfield[] = {
        0x4
};

//
// Supported version
//
const ConstSchemaVersionRange traitVersion = { .mMinVersion = 1, .mMaxVersion = 2 };

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
        &IsEphemeralHandleBitfield[0],
#if (TDM_EXTENSION_SUPPORT)
        NULL,
#endif
#if (TDM_VERSIONING_SUPPORT)
        &traitVersion,
#endif
    }
};

    //
    // Events
    //

const nl::FieldDescriptor TamperStateChangeEventFieldDescriptors[] =
{
    {
        NULL, offsetof(TamperStateChangeEvent, tamperState), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 1
    },

    {
        NULL, offsetof(TamperStateChangeEvent, priorTamperState), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 2
    },

};

const nl::SchemaFieldDescriptor TamperStateChangeEvent::FieldSchema =
{
    .mNumFieldDescriptorElements = sizeof(TamperStateChangeEventFieldDescriptors)/sizeof(TamperStateChangeEventFieldDescriptors[0]),
    .mFields = TamperStateChangeEventFieldDescriptors,
    .mSize = sizeof(TamperStateChangeEvent)
};
const nl::Weave::Profiles::DataManagement::EventSchema TamperStateChangeEvent::Schema =
{
    .mProfileId = kWeaveProfileId,
    .mStructureType = 0x1,
    .mImportance = nl::Weave::Profiles::DataManagement::ProductionCritical,
    .mDataSchemaVersion = 2,
    .mMinCompatibleDataSchemaVersion = 1,
};

} // namespace TamperTrait
} // namespace Security
} // namespace Trait
} // namespace Weave
} // namespace Schema
