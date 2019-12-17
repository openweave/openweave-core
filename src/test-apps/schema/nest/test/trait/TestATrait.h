
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
 *    SOURCE PROTO: nest/test/trait/test_a_trait.proto
 *
 */
#ifndef _NEST_TEST_TRAIT__TEST_A_TRAIT_H_
#define _NEST_TEST_TRAIT__TEST_A_TRAIT_H_

#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Support/SerializationUtils.h>

#include <nest/test/trait/TestCommon.h>


namespace Schema {
namespace Nest {
namespace Test {
namespace Trait {
namespace TestATrait {

extern const nl::Weave::Profiles::DataManagement::TraitSchemaEngine TraitSchema;

enum {
      kWeaveProfileId = (0x235aU << 16) | 0xfe00U
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
    //  ta_a                                EnumA                                int               NO              NO
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
    //  ta_d                                StructA                              structure         NO              YES
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
    //  ta_h                                repeated StructA                     array             NO              NO
    //
    kPropertyHandle_TaH = 10,

    //
    //  ta_i                                map <uint32,uint32>                  map <uint16, uint32>NO              NO
    //
    kPropertyHandle_TaI = 11,

    //
    //  ta_j                                map <uint32,StructA>                 map <uint16, structure>NO              NO
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
    //  value                               uint32                               uint32            NO              NO
    //
    kPropertyHandle_TaI_Value = 27,

    //
    //  value                               StructA                              structure         NO              NO
    //
    kPropertyHandle_TaJ_Value = 28,

    //
    //  sa_a                                uint32                               uint32            NO              YES
    //
    kPropertyHandle_TaJ_Value_SaA = 29,

    //
    //  sa_b                                bool                                 bool              NO              NO
    //
    kPropertyHandle_TaJ_Value_SaB = 30,

    //
    // Enum for last handle
    //
    kLastSchemaHandle = 30,
};

//
// Event Structs
//

struct StructA
{
    uint32_t saA;
    void SetSaANull(void);
    void SetSaAPresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsSaAPresent(void);
#endif
    bool saB;
    uint8_t __nullified_fields__[1/8 + 1];

    static const nl::SchemaFieldDescriptor FieldSchema;

};

struct StructA_array {
    uint32_t num;
    StructA *buf;
};

inline void StructA::SetSaANull(void)
{
    SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 0);
}

inline void StructA::SetSaAPresent(void)
{
    CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 0);
}

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool StructA::IsSaAPresent(void)
{
    return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 0));
}
#endif
//
// Commands
//

enum {
    kCommandARequestId = 0x1,
    kCommandBRequestId = 0x2,
};

enum CommandARequestParameters {
    kCommandARequestParameter_A = 1,
    kCommandARequestParameter_B = 2,
};

enum CommandBRequestParameters {
    kCommandBRequestParameter_A = 1,
    kCommandBRequestParameter_B = 2,
};

enum CommandBResponseParameters {
    kCommandBResponseParameter_A = 1,
    kCommandBResponseParameter_B = 2,
};

//
// Enums
//

enum EnumA {
    ENUM_A_VALUE_1 = 1,
    ENUM_A_VALUE_2 = 2,
    ENUM_A_VALUE_3 = 3,
};

enum EnumAA {
    ENUM_AA_VALUE_1 = 1,
    ENUM_AA_VALUE_2 = 2,
    ENUM_AA_VALUE_3 = 3,
};

//
// Constants
//
#define CONSTANT_A_VALUE_1 {0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
#define CONSTANT_A_VALUE_2 {0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01}
#define CONSTANT_A_VALUE_3 {0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02}
#define CONSTANT_A_VALUE_4 {0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03}
#define CONSTANT_A_VALUE_5 {0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04}
#define CONSTANT_A_VALUE_6 {0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05}
#define CONSTANT_A_VALUE_7 {0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06}
#define CONSTANT_A_VALUE_8 {0x00, 0x08, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88}

enum ConstantA {
        CONSTANT_A_VALUE_1_IMP = 0x0000000000000000ULL, // DEVICE_00000000
        CONSTANT_A_VALUE_2_IMP = 0x0000000000000001ULL, // USER_00000001
        CONSTANT_A_VALUE_3_IMP = 0x0000000000000002ULL, // ACCOUNT_00000002
        CONSTANT_A_VALUE_4_IMP = 0x0000000000000003ULL, // AREA_00000003
        CONSTANT_A_VALUE_5_IMP = 0x0000000000000004ULL, // FIXTURE_00000004
        CONSTANT_A_VALUE_6_IMP = 0x0000000000000005ULL, // GROUP_00000005
        CONSTANT_A_VALUE_7_IMP = 0x0000000000000006ULL, // ANNOTATION_00000006
        CONSTANT_A_VALUE_8_IMP = 0x1122334455667788ULL, // STRUCTURE_1122334455667788
};

} // namespace TestATrait
} // namespace Trait
} // namespace Test
} // namespace Nest
} // namespace Schema
#endif // _NEST_TEST_TRAIT__TEST_A_TRAIT_H_
