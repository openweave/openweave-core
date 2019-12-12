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
 *    SOURCE TEMPLATE: struct.cpp
 *    SOURCE PROTO: google/protobuf/field_mask.proto
 *
 */

#include <google/protobuf/FieldMaskStructSchema.h>

namespace Schema {
namespace Google {
namespace Protobuf {


const nl::FieldDescriptor FieldMaskFieldDescriptors[] =
{
    {
        NULL, offsetof(FieldMask, paths) + offsetof(nl::SerializedFieldTypeUTF8String_array, num), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeArray, 0), 1
    },
    {
        NULL, offsetof(FieldMask, paths) + offsetof(nl::SerializedFieldTypeUTF8String_array, buf), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUTF8String, 0), 1
    },

};

const nl::SchemaFieldDescriptor FieldMask::FieldSchema =
{
    .mNumFieldDescriptorElements = sizeof(FieldMaskFieldDescriptors)/sizeof(FieldMaskFieldDescriptors[0]),
    .mFields = FieldMaskFieldDescriptors,
    .mSize = sizeof(FieldMask)
};



} // namespace Protobuf
} // namespace Google
} // namespace Schema
