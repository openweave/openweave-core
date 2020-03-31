
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
 *    SOURCE TEMPLATE: trait.cpp.h
 *    SOURCE PROTO: nest/test/trait/test_f_trait.proto
 *
 */
#ifndef _NEST_TEST_TRAIT__TEST_F_TRAIT_H_
#define _NEST_TEST_TRAIT__TEST_F_TRAIT_H_

#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Support/SerializationUtils.h>



namespace Schema {
namespace Nest {
namespace Test {
namespace Trait {
namespace TestFTrait {

extern const nl::Weave::Profiles::DataManagement::TraitSchemaEngine TraitSchema;

enum {
      kWeaveProfileId = (0x235aU << 16) | 0xfe07U
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
    //  tf_p_a                              float                                uint32            YES             YES
    //
    kPropertyHandle_TfPA = 2,

    //
    // Enum for last handle
    //
    kLastSchemaHandle = 2,
};

//
// Events
//
struct TestFEvent
{
    int16_t tfA;
    int16_t tfB;
    uint16_t tfC;
    int16_t tfD;

    static const nl::SchemaFieldDescriptor FieldSchema;

    // Statically-known Event Struct Attributes:
    enum {
            kWeaveProfileId = (0x235aU << 16) | 0xfe07U,
        kEventTypeId = 0x1U
    };

    static const nl::Weave::Profiles::DataManagement::EventSchema Schema;
};

struct TestFEvent_array {
    uint32_t num;
    TestFEvent *buf;
};


} // namespace TestFTrait
} // namespace Trait
} // namespace Test
} // namespace Nest
} // namespace Schema
#endif // _NEST_TEST_TRAIT__TEST_F_TRAIT_H_
