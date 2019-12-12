
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
 *    SOURCE PROTO: nest/test/trait/test_f_trait.proto
 *
 */

#include <nest/test/trait/TestFTrait.h>

namespace Schema {
namespace Nest {
namespace Test {
namespace Trait {
namespace TestFTrait {

using namespace ::nl::Weave::Profiles::DataManagement;

//
// Property Table
//

const TraitSchemaEngine::PropertyInfo PropertyMap[] = {
    { kPropertyHandle_Root, 1 }, // tf_p_a
};

//
// IsOptional Table
//

uint8_t IsOptionalHandleBitfield[] = {
        0x1
};

//
// IsNullable Table
//

uint8_t IsNullableHandleBitfield[] = {
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

const nl::FieldDescriptor TestFEventFieldDescriptors[] =
{
    {
        NULL, offsetof(TestFEvent, tfA), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt16, 0), 1
    },

    {
        NULL, offsetof(TestFEvent, tfB), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt16, 0), 2
    },

    {
        NULL, offsetof(TestFEvent, tfC), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt16, 0), 3
    },

    {
        NULL, offsetof(TestFEvent, tfD), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt16, 0), 4
    },

};

const nl::SchemaFieldDescriptor TestFEvent::FieldSchema =
{
    .mNumFieldDescriptorElements = sizeof(TestFEventFieldDescriptors)/sizeof(TestFEventFieldDescriptors[0]),
    .mFields = TestFEventFieldDescriptors,
    .mSize = sizeof(TestFEvent)
};
const nl::Weave::Profiles::DataManagement::EventSchema TestFEvent::Schema =
{
    .mProfileId = kWeaveProfileId,
    .mStructureType = 0x1,
    .mImportance = nl::Weave::Profiles::DataManagement::Production,
    .mDataSchemaVersion = 1,
    .mMinCompatibleDataSchemaVersion = 1,
};

} // namespace TestFTrait
} // namespace Trait
} // namespace Test
} // namespace Nest
} // namespace Schema
