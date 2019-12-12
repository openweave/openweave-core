
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
 *    SOURCE PROTO: nest/test/trait/test_h_trait.proto
 *
 */

#include <nest/test/trait/TestHTrait.h>

namespace Schema {
namespace Nest {
namespace Test {
namespace Trait {
namespace TestHTrait {

using namespace ::nl::Weave::Profiles::DataManagement;

//
// Property Table
//

const TraitSchemaEngine::PropertyInfo PropertyMap[] = {
    { kPropertyHandle_Root, 1 }, // a
    { kPropertyHandle_Root, 2 }, // b
    { kPropertyHandle_Root, 3 }, // c
    { kPropertyHandle_Root, 4 }, // d
    { kPropertyHandle_Root, 5 }, // e
    { kPropertyHandle_Root, 6 }, // f
    { kPropertyHandle_Root, 7 }, // g
    { kPropertyHandle_Root, 8 }, // h
    { kPropertyHandle_Root, 9 }, // i
    { kPropertyHandle_Root, 10 }, // j
    { kPropertyHandle_Root, 11 }, // k
    { kPropertyHandle_K, 1 }, // sa
    { kPropertyHandle_K, 2 }, // sb
    { kPropertyHandle_K, 3 }, // sc
    { kPropertyHandle_Root, 12 }, // l
    { kPropertyHandle_K_Sa, 0 }, // value
    { kPropertyHandle_K_Sa_Value, 1 }, // da
    { kPropertyHandle_K_Sa_Value, 2 }, // db
    { kPropertyHandle_K_Sa_Value, 3 }, // dc
    { kPropertyHandle_L, 0 }, // value
    { kPropertyHandle_L_Value, 1 }, // da
    { kPropertyHandle_L_Value, 2 }, // db
    { kPropertyHandle_L_Value, 3 }, // dc
};

//
// IsDictionary Table
//

uint8_t IsDictionaryTypeHandleBitfield[] = {
        0x0, 0x48, 0x0
};

//
// Schema
//

const TraitSchemaEngine TraitSchema = {
    {
        kWeaveProfileId,
        PropertyMap,
        sizeof(PropertyMap) / sizeof(PropertyMap[0]),
        4,
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

const nl::FieldDescriptor StructDictionaryFieldDescriptors[] =
{
    {
        NULL, offsetof(StructDictionary, da), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 1
    },

    {
        NULL, offsetof(StructDictionary, db), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 2
    },

    {
        NULL, offsetof(StructDictionary, dc), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 3
    },

};

const nl::SchemaFieldDescriptor StructDictionary::FieldSchema =
{
    .mNumFieldDescriptorElements = sizeof(StructDictionaryFieldDescriptors)/sizeof(StructDictionaryFieldDescriptors[0]),
    .mFields = StructDictionaryFieldDescriptors,
    .mSize = sizeof(StructDictionary)
};


const nl::FieldDescriptor StructHFieldDescriptors[] =
{
    {
        NULL, offsetof(StructH, sb), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 2
    },

    {
        NULL, offsetof(StructH, sc), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 3
    },

};

const nl::SchemaFieldDescriptor StructH::FieldSchema =
{
    .mNumFieldDescriptorElements = sizeof(StructHFieldDescriptors)/sizeof(StructHFieldDescriptors[0]),
    .mFields = StructHFieldDescriptors,
    .mSize = sizeof(StructH)
};

} // namespace TestHTrait
} // namespace Trait
} // namespace Test
} // namespace Nest
} // namespace Schema
