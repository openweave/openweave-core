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
 *    SOURCE PROTO: weave/common/identifiers.proto
 *
 */

#include <weave/common/InterfaceNameStructSchema.h>

namespace Schema {
namespace Weave {
namespace Common {


const nl::FieldDescriptor InterfaceNameFieldDescriptors[] =
{
    {
        NULL, offsetof(InterfaceName, interfaceName), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUTF8String, 0), 1
    },

};

const nl::SchemaFieldDescriptor InterfaceName::FieldSchema =
{
    .mNumFieldDescriptorElements = sizeof(InterfaceNameFieldDescriptors)/sizeof(InterfaceNameFieldDescriptors[0]),
    .mFields = InterfaceNameFieldDescriptors,
    .mSize = sizeof(InterfaceName)
};



} // namespace Common
} // namespace Weave
} // namespace Schema
