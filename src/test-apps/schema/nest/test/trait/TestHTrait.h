
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
 *    SOURCE PROTO: nest/test/trait/test_h_trait.proto
 *
 */
#ifndef _NEST_TEST_TRAIT__TEST_H_TRAIT_H_
#define _NEST_TEST_TRAIT__TEST_H_TRAIT_H_

#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Support/SerializationUtils.h>



namespace Schema {
namespace Nest {
namespace Test {
namespace Trait {
namespace TestHTrait {

extern const nl::Weave::Profiles::DataManagement::TraitSchemaEngine TraitSchema;

enum {
      kWeaveProfileId = (0x235aU << 16) | 0xfe08U
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
    //  a                                   uint32                               uint32            NO              NO
    //
    kPropertyHandle_A = 2,

    //
    //  b                                   uint32                               uint32            NO              NO
    //
    kPropertyHandle_B = 3,

    //
    //  c                                   uint32                               uint32            NO              NO
    //
    kPropertyHandle_C = 4,

    //
    //  d                                   uint32                               uint32            NO              NO
    //
    kPropertyHandle_D = 5,

    //
    //  e                                   uint32                               uint32            NO              NO
    //
    kPropertyHandle_E = 6,

    //
    //  f                                   uint32                               uint32            NO              NO
    //
    kPropertyHandle_F = 7,

    //
    //  g                                   uint32                               uint32            NO              NO
    //
    kPropertyHandle_G = 8,

    //
    //  h                                   uint32                               uint32            NO              NO
    //
    kPropertyHandle_H = 9,

    //
    //  i                                   uint32                               uint32            NO              NO
    //
    kPropertyHandle_I = 10,

    //
    //  j                                   uint32                               uint32            NO              NO
    //
    kPropertyHandle_J = 11,

    //
    //  k                                   StructH                              structure         NO              NO
    //
    kPropertyHandle_K = 12,

    //
    //  sa                                  map <uint32,StructDictionary>        map <uint32, structure>NO              NO
    //
    kPropertyHandle_K_Sa = 13,

    //
    //  sb                                  uint32                               uint32            NO              NO
    //
    kPropertyHandle_K_Sb = 14,

    //
    //  sc                                  uint32                               uint32            NO              NO
    //
    kPropertyHandle_K_Sc = 15,

    //
    //  l                                   map <uint32,StructDictionary>        map <uint32, structure>NO              NO
    //
    kPropertyHandle_L = 16,

    //
    //  value                               StructDictionary                     structure         NO              NO
    //
    kPropertyHandle_K_Sa_Value = 17,

    //
    //  da                                  uint32                               uint32            NO              NO
    //
    kPropertyHandle_K_Sa_Value_Da = 18,

    //
    //  db                                  uint32                               uint32            NO              NO
    //
    kPropertyHandle_K_Sa_Value_Db = 19,

    //
    //  dc                                  uint32                               uint32            NO              NO
    //
    kPropertyHandle_K_Sa_Value_Dc = 20,

    //
    //  value                               StructDictionary                     structure         NO              NO
    //
    kPropertyHandle_L_Value = 21,

    //
    //  da                                  uint32                               uint32            NO              NO
    //
    kPropertyHandle_L_Value_Da = 22,

    //
    //  db                                  uint32                               uint32            NO              NO
    //
    kPropertyHandle_L_Value_Db = 23,

    //
    //  dc                                  uint32                               uint32            NO              NO
    //
    kPropertyHandle_L_Value_Dc = 24,

    //
    // Enum for last handle
    //
    kLastSchemaHandle = 24,
};

//
// Event Structs
//

struct StructDictionary
{
    uint32_t da;
    uint32_t db;
    uint32_t dc;

    static const nl::SchemaFieldDescriptor FieldSchema;

};

struct StructDictionary_array {
    uint32_t num;
    StructDictionary *buf;
};

struct StructH
{
    uint32_t sb;
    uint32_t sc;

    static const nl::SchemaFieldDescriptor FieldSchema;

};

struct StructH_array {
    uint32_t num;
    StructH *buf;
};

} // namespace TestHTrait
} // namespace Trait
} // namespace Test
} // namespace Nest
} // namespace Schema
#endif // _NEST_TEST_TRAIT__TEST_H_TRAIT_H_
