
/**
 *    Copyright (c) 2018 Nest Labs, Inc.
 *    All rights reserved.
 *
 *    THIS FILE IS GENERATED. DO NOT MODIFY.
 *
 *    SOURCE TEMPLATE: trait.cpp.h
 *    SOURCE PROTO: nest/test/trait/test_d_trait.proto
 *
 */
#ifndef _NEST_TEST_TRAIT__TEST_D_TRAIT_H_
#define _NEST_TEST_TRAIT__TEST_D_TRAIT_H_

#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Support/SerializationUtils.h>

#include <nest/test/trait/TestCTrait.h>
#include <nest/test/trait/TestCommonVersioned.h>


namespace Schema {
namespace Nest {
namespace Test {
namespace Trait {
namespace TestDTrait {

extern const nl::Weave::Profiles::DataManagement::TraitSchemaEngine TraitSchema;

enum {
      kWeaveProfileId = (0x235aU << 16) | 0xfe05U
};

//
// Properties
//

enum {
    kPropertyHandle_Root = 1,

    //---------------------------------------------------------------------------------------------------------------------------//
    //  Name                                IDL Type                            TLV Type           Optional?       Nullable?     //
    //---------------------------------------------------------------------------------------------------------------------------//

    //
    //  tc_a                                bool                                 bool              NO              NO
    //
    kPropertyHandle_TcA = 2,

    //
    //  tc_b                                nest.test.trait.TestCTrait.EnumC     int               NO              NO
    //
    kPropertyHandle_TcB = 3,

    //
    //  tc_c                                nest.test.trait.TestCTrait.StructC   structure         NO              NO
    //
    kPropertyHandle_TcC = 4,

    //
    //  sc_a                                uint32                               uint32            NO              NO
    //
    kPropertyHandle_TcC_ScA = 5,

    //
    //  sc_b                                bool                                 bool              NO              NO
    //
    kPropertyHandle_TcC_ScB = 6,

    //
    //  tc_d                                uint32                               uint32            NO              NO
    //
    kPropertyHandle_TcD = 7,


    //
    //  td_d                                string                               string            NO              NO
    //
    kPropertyHandle_TdD = 8,

    //
    //  td_e                                int32                                int32             NO              NO
    //
    kPropertyHandle_TdE = 9,

    //
    //  td_f                                bool                                 bool              NO              NO
    //
    kPropertyHandle_TdF = 10,

    //
    // Enum for last handle
    //
    kLastSchemaHandle = 10,
};

} // namespace TestDTrait
} // namespace Trait
} // namespace Test
} // namespace Nest
} // namespace Schema
#endif // _NEST_TEST_TRAIT__TEST_D_TRAIT_H_
