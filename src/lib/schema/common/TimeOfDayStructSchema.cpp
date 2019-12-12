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
 *    SOURCE PROTO: weave/common/time.proto
 *
 */

#include <weave/common/TimeOfDayStructSchema.h>

namespace Schema {
namespace Weave {
namespace Common {


const nl::FieldDescriptor TimeOfDayFieldDescriptors[] =
{
    {
        NULL, offsetof(TimeOfDay, hour), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt8, 0), 5
    },

    {
        NULL, offsetof(TimeOfDay, minute), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt8, 0), 6
    },

    {
        NULL, offsetof(TimeOfDay, second), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt8, 0), 7
    },

};

const nl::SchemaFieldDescriptor TimeOfDay::FieldSchema =
{
    .mNumFieldDescriptorElements = sizeof(TimeOfDayFieldDescriptors)/sizeof(TimeOfDayFieldDescriptors[0]),
    .mFields = TimeOfDayFieldDescriptors,
    .mSize = sizeof(TimeOfDay)
};



} // namespace Common
} // namespace Weave
} // namespace Schema
