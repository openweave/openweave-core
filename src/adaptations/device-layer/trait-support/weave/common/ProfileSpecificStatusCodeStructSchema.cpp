/**
 *    Copyright (c) 2019 Nest Labs, Inc.
 *    All rights reserved.
 *
 *    THIS FILE IS GENERATED. DO NOT MODIFY.
 *
 *    SOURCE TEMPLATE: struct.cpp
 *    SOURCE PROTO: weave/common/identifiers.proto
 *
 */

#include <weave/common/ProfileSpecificStatusCodeStructSchema.h>

namespace Schema {
namespace Weave {
namespace Common {


const nl::FieldDescriptor ProfileSpecificStatusCodeFieldDescriptors[] =
{
    {
        NULL, offsetof(ProfileSpecificStatusCode, profileId), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 1
    },

    {
        NULL, offsetof(ProfileSpecificStatusCode, statusCode), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt16, 0), 2
    },

};

const nl::SchemaFieldDescriptor ProfileSpecificStatusCode::FieldSchema =
{
    .mNumFieldDescriptorElements = sizeof(ProfileSpecificStatusCodeFieldDescriptors)/sizeof(ProfileSpecificStatusCodeFieldDescriptors[0]),
    .mFields = ProfileSpecificStatusCodeFieldDescriptors,
    .mSize = sizeof(ProfileSpecificStatusCode)
};



} // namespace Common
} // namespace Weave
} // namespace Schema
