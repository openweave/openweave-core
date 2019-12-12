
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
 *    SOURCE PROTO: nest/test/trait/test_b_trait.proto
 *
 */
#ifndef _NEST_TEST_TRAIT__TEST_B_TRAIT_H_
#define _NEST_TEST_TRAIT__TEST_B_TRAIT_H_

#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Support/SerializationUtils.h>

#include <nest/test/trait/TestATrait.h>
#include <nest/test/trait/TestCommon.h>


namespace Schema {
namespace Nest {
namespace Test {
namespace Trait {
namespace TestBTrait {

extern const nl::Weave::Profiles::DataManagement::TraitSchemaEngine TraitSchema;

enum {
      kWeaveProfileId = (0x235aU << 16) | 0xfe01U
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
    //  ta_a                                nest.test.trait.TestATrait.EnumA     int               NO              NO
    //
    kPropertyHandle_TaA = 2,

    //
    //  ta_b                                nest.test.trait.TestCommon.CommonEnumA int               NO              NO
    //
    kPropertyHandle_TaB = 3,

    //
    //  ta_c                                uint32                               uint32            YES             NO
    //
    kPropertyHandle_TaC = 4,

    //
    //  ta_d                                nest.test.trait.TestATrait.StructA   structure         NO              YES
    //
    kPropertyHandle_TaD = 5,

    //
    //  sa_a                                uint32                               uint32            NO              YES
    //
    kPropertyHandle_TaD_SaA = 6,

    //
    //  sa_b                                bool                                 bool              NO              NO
    //
    kPropertyHandle_TaD_SaB = 7,

    //
    //  ta_e                                repeated uint32                      array             NO              NO
    //
    kPropertyHandle_TaE = 8,

    //
    //  ta_g                                weave.common.StringRef               union             NO              NO
    //
    kPropertyHandle_TaG = 9,

    //
    //  ta_h                                repeated nest.test.trait.TestATrait.StructA array             NO              NO
    //
    kPropertyHandle_TaH = 10,

    //
    //  ta_i                                map <uint32,uint32>                  map <uint16, uint32>NO              NO
    //
    kPropertyHandle_TaI = 11,

    //
    //  ta_j                                map <uint32,nest.test.trait.TestATrait.StructA> map <uint16, structure>NO              NO
    //
    kPropertyHandle_TaJ = 12,

    //
    //  ta_k                                bytes                                bytes             NO              NO
    //
    kPropertyHandle_TaK = 13,

    //
    //  ta_l                                repeated weave.common.DayOfWeek      uint              NO              NO
    //
    kPropertyHandle_TaL = 14,

    //
    //  ta_m                                weave.common.ResourceId              uint64            NO              YES
    //
    kPropertyHandle_TaM = 15,

    //
    //  ta_n                                weave.common.ResourceId              bytes             NO              YES
    //
    kPropertyHandle_TaN = 16,

    //
    //  ta_o                                google.protobuf.Timestamp            uint32 seconds    NO              NO
    //
    kPropertyHandle_TaO = 17,

    //
    //  ta_p                                google.protobuf.Timestamp            int64 millisecondsNO              YES
    //
    kPropertyHandle_TaP = 18,

    //
    //  ta_q                                google.protobuf.Duration             int64 millisecondsNO              NO
    //
    kPropertyHandle_TaQ = 19,

    //
    //  ta_r                                google.protobuf.Duration             uint32 seconds    NO              NO
    //
    kPropertyHandle_TaR = 20,

    //
    //  ta_s                                google.protobuf.Duration             uint32 millisecondsNO              YES
    //
    kPropertyHandle_TaS = 21,

    //
    //  ta_t                                uint32                               uint32            NO              YES
    //
    kPropertyHandle_TaT = 22,

    //
    //  ta_u                                int32                                int32             NO              YES
    //
    kPropertyHandle_TaU = 23,

    //
    //  ta_v                                bool                                 bool              NO              YES
    //
    kPropertyHandle_TaV = 24,

    //
    //  ta_w                                string                               string            NO              YES
    //
    kPropertyHandle_TaW = 25,

    //
    //  ta_x                                float                                int16             NO              YES
    //
    kPropertyHandle_TaX = 26,

    //
    //  tb_a                                uint32                               uint32            NO              NO
    //
    kPropertyHandle_TbA = 27,

    //
    //  tb_b                                StructB                              structure         NO              NO
    //
    kPropertyHandle_TbB = 28,

    //
    //  sb_a                                string                               string            NO              NO
    //
    kPropertyHandle_TbB_SbA = 29,

    //
    //  sb_b                                uint32                               uint32            NO              NO
    //
    kPropertyHandle_TbB_SbB = 30,

    //
    //  tb_c                                StructEA                             structure         NO              NO
    //
    kPropertyHandle_TbC = 31,

    //
    //  sa_a                                uint32                               uint32            NO              YES
    //
    kPropertyHandle_TbC_SaA = 32,

    //
    //  sa_b                                bool                                 bool              NO              NO
    //
    kPropertyHandle_TbC_SaB = 33,

    //
    //  sea_c                               string                               string            NO              NO
    //
    kPropertyHandle_TbC_SeaC = 34,

    //
    //  value                               uint32                               uint32            NO              NO
    //
    kPropertyHandle_TaI_Value = 35,

    //
    //  value                               nest.test.trait.TestATrait.StructA   structure         NO              NO
    //
    kPropertyHandle_TaJ_Value = 36,

    //
    //  sa_a                                uint32                               uint32            NO              YES
    //
    kPropertyHandle_TaJ_Value_SaA = 37,

    //
    //  sa_b                                bool                                 bool              NO              NO
    //
    kPropertyHandle_TaJ_Value_SaB = 38,

    //
    // Enum for last handle
    //
    kLastSchemaHandle = 38,
};

//
// Event Structs
//

struct StructB
{
    const char * sbA;
    uint32_t sbB;

    static const nl::SchemaFieldDescriptor FieldSchema;

};

struct StructB_array {
    uint32_t num;
    StructB *buf;
};

struct StructEA
{
    uint32_t saA;
    void SetSaANull(void);
    void SetSaAPresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsSaAPresent(void);
#endif
    bool saB;
    const char * seaC;
    uint8_t __nullified_fields__[1/8 + 1];

    static const nl::SchemaFieldDescriptor FieldSchema;

};

struct StructEA_array {
    uint32_t num;
    StructEA *buf;
};

inline void StructEA::SetSaANull(void)
{
    SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 0);
}

inline void StructEA::SetSaAPresent(void)
{
    CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 0);
}

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool StructEA::IsSaAPresent(void)
{
    return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 0));
}
#endif
} // namespace TestBTrait
} // namespace Trait
} // namespace Test
} // namespace Nest
} // namespace Schema
#endif // _NEST_TEST_TRAIT__TEST_B_TRAIT_H_
