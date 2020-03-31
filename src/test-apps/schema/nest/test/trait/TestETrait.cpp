
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
 *    SOURCE PROTO: nest/test/trait/test_e_trait.proto
 *
 */

#include <nest/test/trait/TestETrait.h>

namespace Schema {
namespace Nest {
namespace Test {
namespace Trait {
namespace TestETrait {

using namespace ::nl::Weave::Profiles::DataManagement;

//
// Property Table
//

const TraitSchemaEngine::PropertyInfo PropertyMap[] = {
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
        NULL,
        NULL,
        NULL,
        NULL,
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

const nl::FieldDescriptor TestEEventFieldDescriptors[] =
{
    {
        NULL, offsetof(TestEEvent, teA), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 1
    },

    {
        NULL, offsetof(TestEEvent, teB), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 2
    },

    {
        NULL, offsetof(TestEEvent, teC), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeBoolean, 0), 3
    },

    {
        NULL, offsetof(TestEEvent, teD), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 4
    },

    {
        &Schema::Nest::Test::Trait::TestETrait::StructE::FieldSchema, offsetof(TestEEvent, teE), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeStructure, 0), 5
    },

    {
        NULL, offsetof(TestEEvent, teF), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 6
    },

    {
        &Schema::Nest::Test::Trait::TestCommon::CommonStructE::FieldSchema, offsetof(TestEEvent, teG), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeStructure, 0), 7
    },

    {
        NULL, offsetof(TestEEvent, teH) + offsetof(nl::SerializedFieldTypeUInt32_array, num), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeArray, 0), 8
    },
    {
        NULL, offsetof(TestEEvent, teH) + offsetof(nl::SerializedFieldTypeUInt32_array, buf), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 8
    },

    {
        NULL, offsetof(TestEEvent, teI) + offsetof(Schema::Nest::Test::Trait::TestCommon::CommonStructE_array, num), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeArray, 0), 9
    },
    {
        &Schema::Nest::Test::Trait::TestCommon::CommonStructE::FieldSchema, offsetof(TestEEvent, teI) + offsetof(Schema::Nest::Test::Trait::TestCommon::CommonStructE_array, buf), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeStructure, 0), 9
    },

    {
        NULL, offsetof(TestEEvent, teJ), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt16, 1), 10
    },

    {
        NULL, offsetof(TestEEvent, teK), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeByteString, 0), 13
    },

    {
        NULL, offsetof(TestEEvent, teL), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 14
    },

    {
        NULL, offsetof(TestEEvent, teM), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt64, 1), 15
    },

    {
        NULL, offsetof(TestEEvent, teN), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeByteString, 1), 16
    },

    {
        NULL, offsetof(TestEEvent, teO), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 17
    },

    {
        NULL, offsetof(TestEEvent, teP), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt64, 1), 18
    },

    {
        NULL, offsetof(TestEEvent, teQ), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt64, 0), 19
    },

    {
        NULL, offsetof(TestEEvent, teR), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 20
    },

    {
        NULL, offsetof(TestEEvent, teS), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 1), 21
    },

    {
        NULL, offsetof(TestEEvent, teT), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 22
    },

};

const nl::SchemaFieldDescriptor TestEEvent::FieldSchema =
{
    .mNumFieldDescriptorElements = sizeof(TestEEventFieldDescriptors)/sizeof(TestEEventFieldDescriptors[0]),
    .mFields = TestEEventFieldDescriptors,
    .mSize = sizeof(TestEEvent)
};
const nl::Weave::Profiles::DataManagement::EventSchema TestEEvent::Schema =
{
    .mProfileId = kWeaveProfileId,
    .mStructureType = 0x1,
    .mImportance = nl::Weave::Profiles::DataManagement::Production,
    .mDataSchemaVersion = 2,
    .mMinCompatibleDataSchemaVersion = 1,
};

const nl::FieldDescriptor TestENullableEventFieldDescriptors[] =
{
    {
        NULL, offsetof(TestENullableEvent, neA), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 1), 1
    },

    {
        NULL, offsetof(TestENullableEvent, neB), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 1), 2
    },

    {
        NULL, offsetof(TestENullableEvent, neC), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeBoolean, 1), 3
    },

    {
        NULL, offsetof(TestENullableEvent, neD), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUTF8String, 1), 4
    },

    {
        NULL, offsetof(TestENullableEvent, neE), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt16, 1), 5
    },

    {
        NULL, offsetof(TestENullableEvent, neF), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 1), 6
    },

    {
        NULL, offsetof(TestENullableEvent, neG), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 1), 7
    },

    {
        NULL, offsetof(TestENullableEvent, neH), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeBoolean, 1), 8
    },

    {
        NULL, offsetof(TestENullableEvent, neI), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUTF8String, 1), 9
    },

    {
        &Schema::Nest::Test::Trait::TestETrait::NullableE::FieldSchema, offsetof(TestENullableEvent, neJ), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeStructure, 1), 10
    },

};

const nl::SchemaFieldDescriptor TestENullableEvent::FieldSchema =
{
    .mNumFieldDescriptorElements = sizeof(TestENullableEventFieldDescriptors)/sizeof(TestENullableEventFieldDescriptors[0]),
    .mFields = TestENullableEventFieldDescriptors,
    .mSize = sizeof(TestENullableEvent)
};
const nl::Weave::Profiles::DataManagement::EventSchema TestENullableEvent::Schema =
{
    .mProfileId = kWeaveProfileId,
    .mStructureType = 0x2,
    .mImportance = nl::Weave::Profiles::DataManagement::Production,
    .mDataSchemaVersion = 2,
    .mMinCompatibleDataSchemaVersion = 1,
};

const nl::SchemaFieldDescriptor TestEEmptyEvent::FieldSchema =
{
    .mNumFieldDescriptorElements = 0,
    .mFields = NULL,
    .mSize =  sizeof(TestEEmptyEvent)
};

const nl::Weave::Profiles::DataManagement::EventSchema TestEEmptyEvent::Schema =
{
    .mProfileId = kWeaveProfileId,
    .mStructureType = 0x3,
    .mImportance = nl::Weave::Profiles::DataManagement::Production,
    .mDataSchemaVersion = 2,
    .mMinCompatibleDataSchemaVersion = 1,
};

//
// Event Structs
//

const nl::FieldDescriptor StructEFieldDescriptors[] =
{
    {
        NULL, offsetof(StructE, seA), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 1
    },

    {
        NULL, offsetof(StructE, seB), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeBoolean, 0), 2
    },

    {
        NULL, offsetof(StructE, seC), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 3
    },

};

const nl::SchemaFieldDescriptor StructE::FieldSchema =
{
    .mNumFieldDescriptorElements = sizeof(StructEFieldDescriptors)/sizeof(StructEFieldDescriptors[0]),
    .mFields = StructEFieldDescriptors,
    .mSize = sizeof(StructE)
};


const nl::FieldDescriptor NullableEFieldDescriptors[] =
{
    {
        NULL, offsetof(NullableE, neA), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 1), 1
    },

    {
        NULL, offsetof(NullableE, neB), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeBoolean, 1), 2
    },

};

const nl::SchemaFieldDescriptor NullableE::FieldSchema =
{
    .mNumFieldDescriptorElements = sizeof(NullableEFieldDescriptors)/sizeof(NullableEFieldDescriptors[0]),
    .mFields = NullableEFieldDescriptors,
    .mSize = sizeof(NullableE)
};

} // namespace TestETrait
} // namespace Trait
} // namespace Test
} // namespace Nest
} // namespace Schema
