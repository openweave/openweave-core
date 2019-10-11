
/**
 *    Copyright (c) 2018 Nest Labs, Inc.
 *    All rights reserved.
 *
 *    THIS FILE IS GENERATED. DO NOT MODIFY.
 *
 *    SOURCE TEMPLATE: typespace.cpp
 *    SOURCE PROTO: nest/test/trait/test_common.proto
 *
 */

#include <nest/test/trait/TestCommon.h>

namespace Schema {
namespace Nest {
namespace Test {
namespace Trait {
namespace TestCommon {

    using namespace ::nl::Weave::Profiles::DataManagement;

  //
  // Event Structs
  //

const nl::FieldDescriptor CommonStructEFieldDescriptors[] =
{
    {
        NULL, offsetof(CommonStructE, seA), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 1
    },

    {
        NULL, offsetof(CommonStructE, seB), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeBoolean, 0), 2
    },

};

const nl::SchemaFieldDescriptor CommonStructE::FieldSchema =
{
    .mNumFieldDescriptorElements = sizeof(CommonStructEFieldDescriptors)/sizeof(CommonStructEFieldDescriptors[0]),
    .mFields = CommonStructEFieldDescriptors,
    .mSize = sizeof(CommonStructE)
};


} // namespace TestCommon

} // namespace Trait
} // namespace Test
} // namespace Nest
} // namespace Schema
