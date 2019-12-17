
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
 *    SOURCE PROTO: nest/test/trait/test_a_trait.proto
 *
 */

#include <nest/test/trait/TestATrait.h>

namespace Schema {
namespace Nest {
namespace Test {
namespace Trait {
namespace TestATrait {

using namespace ::nl::Weave::Profiles::DataManagement;

//
// Property Table
//

const TraitSchemaEngine::PropertyInfo PropertyMap[] = {
    { kPropertyHandle_Root, 1 }, // ta_a
    { kPropertyHandle_Root, 2 }, // ta_b
    { kPropertyHandle_Root, 3 }, // ta_c
    { kPropertyHandle_Root, 4 }, // ta_d
    { kPropertyHandle_TaD, 1 }, // sa_a
    { kPropertyHandle_TaD, 2 }, // sa_b
    { kPropertyHandle_Root, 5 }, // ta_e
    { kPropertyHandle_Root, 8 }, // ta_g
    { kPropertyHandle_Root, 10 }, // ta_h
    { kPropertyHandle_Root, 11 }, // ta_i
    { kPropertyHandle_Root, 12 }, // ta_j
    { kPropertyHandle_Root, 14 }, // ta_k
    { kPropertyHandle_Root, 15 }, // ta_l
    { kPropertyHandle_Root, 16 }, // ta_m
    { kPropertyHandle_Root, 9 }, // ta_n
    { kPropertyHandle_Root, 7 }, // ta_o
    { kPropertyHandle_Root, 17 }, // ta_p
    { kPropertyHandle_Root, 18 }, // ta_q
    { kPropertyHandle_Root, 19 }, // ta_r
    { kPropertyHandle_Root, 20 }, // ta_s
    { kPropertyHandle_Root, 13 }, // ta_t
    { kPropertyHandle_Root, 21 }, // ta_u
    { kPropertyHandle_Root, 22 }, // ta_v
    { kPropertyHandle_Root, 23 }, // ta_w
    { kPropertyHandle_Root, 24 }, // ta_x
    { kPropertyHandle_TaI, 0 }, // value
    { kPropertyHandle_TaJ, 0 }, // value
    { kPropertyHandle_TaJ_Value, 1 }, // sa_a
    { kPropertyHandle_TaJ_Value, 2 }, // sa_b
};

//
// IsDictionary Table
//

uint8_t IsDictionaryTypeHandleBitfield[] = {
        0x0, 0x6, 0x0, 0x0
};

//
// IsOptional Table
//

uint8_t IsOptionalHandleBitfield[] = {
        0x4, 0x0, 0x0, 0x0
};

//
// IsNullable Table
//

uint8_t IsNullableHandleBitfield[] = {
        0x18, 0x60, 0xf9, 0x9
};

//
// IsEphemeral Table
//

uint8_t IsEphemeralHandleBitfield[] = {
        0x18, 0x0, 0x10, 0x8
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
        3,
#if (TDM_EXTENSION_SUPPORT) || (TDM_VERSIONING_SUPPORT)
        2,
#endif
        IsDictionaryTypeHandleBitfield,
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
// Event Structs
//

const nl::FieldDescriptor StructAFieldDescriptors[] =
{
    {
        NULL, offsetof(StructA, saA), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 1), 1
    },

    {
        NULL, offsetof(StructA, saB), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeBoolean, 0), 2
    },

};

const nl::SchemaFieldDescriptor StructA::FieldSchema =
{
    .mNumFieldDescriptorElements = sizeof(StructAFieldDescriptors)/sizeof(StructAFieldDescriptors[0]),
    .mFields = StructAFieldDescriptors,
    .mSize = sizeof(StructA)
};

} // namespace TestATrait
} // namespace Trait
} // namespace Test
} // namespace Nest
} // namespace Schema
