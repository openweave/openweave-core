
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
 *    SOURCE PROTO: weave/trait/security/pincode_input_trait.proto
 *
 */

#include <weave/trait/security/PincodeInputTrait.h>

namespace Schema {
namespace Weave {
namespace Trait {
namespace Security {
namespace PincodeInputTrait {

using namespace ::nl::Weave::Profiles::DataManagement;

//
// Property Table
//

const TraitSchemaEngine::PropertyInfo PropertyMap[] = {
    { kPropertyHandle_Root, 1 }, // pincode_input_state
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
    // Events
    //

const nl::FieldDescriptor KeypadEntryEventFieldDescriptors[] =
{
    {
        NULL, offsetof(KeypadEntryEvent, pincodeCredentialEnabled), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeBoolean, 1), 1
    },

    {
        NULL, offsetof(KeypadEntryEvent, userId), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeByteString, 1), 2
    },

    {
        NULL, offsetof(KeypadEntryEvent, invalidEntryCount), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 3
    },

    {
        NULL, offsetof(KeypadEntryEvent, pincodeEntryResult), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 4
    },

};

const nl::SchemaFieldDescriptor KeypadEntryEvent::FieldSchema =
{
    .mNumFieldDescriptorElements = sizeof(KeypadEntryEventFieldDescriptors)/sizeof(KeypadEntryEventFieldDescriptors[0]),
    .mFields = KeypadEntryEventFieldDescriptors,
    .mSize = sizeof(KeypadEntryEvent)
};
const nl::Weave::Profiles::DataManagement::EventSchema KeypadEntryEvent::Schema =
{
    .mProfileId = kWeaveProfileId,
    .mStructureType = 0x1,
    .mImportance = nl::Weave::Profiles::DataManagement::ProductionCritical,
    .mDataSchemaVersion = 1,
    .mMinCompatibleDataSchemaVersion = 1,
};

const nl::FieldDescriptor PincodeInputStateChangeEventFieldDescriptors[] =
{
    {
        NULL, offsetof(PincodeInputStateChangeEvent, pincodeInputState), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 1
    },

    {
        NULL, offsetof(PincodeInputStateChangeEvent, userId), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt64, 1), 2
    },

};

const nl::SchemaFieldDescriptor PincodeInputStateChangeEvent::FieldSchema =
{
    .mNumFieldDescriptorElements = sizeof(PincodeInputStateChangeEventFieldDescriptors)/sizeof(PincodeInputStateChangeEventFieldDescriptors[0]),
    .mFields = PincodeInputStateChangeEventFieldDescriptors,
    .mSize = sizeof(PincodeInputStateChangeEvent)
};
const nl::Weave::Profiles::DataManagement::EventSchema PincodeInputStateChangeEvent::Schema =
{
    .mProfileId = kWeaveProfileId,
    .mStructureType = 0x2,
    .mImportance = nl::Weave::Profiles::DataManagement::ProductionCritical,
    .mDataSchemaVersion = 1,
    .mMinCompatibleDataSchemaVersion = 1,
};

} // namespace PincodeInputTrait
} // namespace Security
} // namespace Trait
} // namespace Weave
} // namespace Schema
