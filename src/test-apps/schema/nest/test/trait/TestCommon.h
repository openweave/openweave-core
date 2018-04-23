
/**
 *    Copyright (c) 2018 Nest Labs, Inc.
 *    All rights reserved.
 *
 *    THIS FILE IS GENERATED. DO NOT MODIFY.
 *
 *    SOURCE TEMPLATE: typespace.cpp.h
 *    SOURCE PROTO: nest/test/trait/test_common.proto
 *
 */
#ifndef _NEST_TEST_TRAIT__TEST_COMMON_H_
#define _NEST_TEST_TRAIT__TEST_COMMON_H_

#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Support/SerializationUtils.h>



namespace Schema {
namespace Nest {
namespace Test {
namespace Trait {
namespace TestCommon {

  extern const nl::Weave::Profiles::DataManagement::TraitSchemaEngine TraitSchema;

  //
  // Event Structs
  //


struct CommonStructE
{
    uint32_t seA;
    bool seB;

    static const nl::SchemaFieldDescriptor FieldSchema;

};

struct CommonStructE_array {
    uint32_t num;
    CommonStructE *buf;
};




    //
    // Enums
    //

      enum CommonEnumA {
    COMMON_ENUM_A_VALUE_1 = 1,
    COMMON_ENUM_A_VALUE_2 = 2,
    COMMON_ENUM_A_VALUE_3 = 3,
};

      enum CommonEnumE {
    COMMON_ENUM_E_VALUE_1 = 1,
    COMMON_ENUM_E_VALUE_2 = 2,
    COMMON_ENUM_E_VALUE_3 = 3,
};



} // namespace TestCommon

} // namespace Trait
} // namespace Test
} // namespace Nest
} // namespace Schema
#endif // _NEST_TEST_TRAIT__TEST_COMMON_H_
