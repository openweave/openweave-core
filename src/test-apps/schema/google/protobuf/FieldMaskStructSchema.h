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
 *    SOURCE TEMPLATE: struct.cpp.h
 *    SOURCE PROTO: google/protobuf/field_mask.proto
 *
 */
#ifndef _GOOGLE_PROTOBUF__FIELD_MASK_STRUCT_SCHEMA_H_
#define _GOOGLE_PROTOBUF__FIELD_MASK_STRUCT_SCHEMA_H_

#include <Weave/Support/SerializationUtils.h>
#include <Weave/Profiles/data-management/DataManagement.h>



namespace Schema {
namespace Google {
namespace Protobuf {

struct FieldMask
{
    nl::SerializedFieldTypeUTF8String_array  paths;

    static const nl::SchemaFieldDescriptor FieldSchema;

};

struct FieldMask_array {
    uint32_t num;
    FieldMask *buf;
};



} // namespace Protobuf
} // namespace Google
} // namespace Schema
#endif // _GOOGLE_PROTOBUF__FIELD_MASK_STRUCT_SCHEMA_H_
