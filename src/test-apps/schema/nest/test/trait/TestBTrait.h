/*
 *
 *    Copyright (c) 2017 Nest Labs, Inc.
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
#ifndef _NEST_TEST_TRAIT__TEST_B_TRAIT_H_
#define _NEST_TEST_TRAIT__TEST_B_TRAIT_H_

#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Support/SerializationUtils.h>

#include <weave/common/DayOfWeekEnum.h>

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
        //  ta_a                                nest.test.trait.TestATrait.EnumA    int                NO              NO
        //
        kPropertyHandle_TaA = 2,

        //
        //  ta_b                                nest.test.trait.TestCommonTrait.CommonEnumA int        NO              NO
        //
        kPropertyHandle_TaB = 3,

        //
        //  ta_c                                uint32                              unsigned int       YES             NO
        //
        kPropertyHandle_TaC = 4,

        //
        //  ta_d                                nest.test.trait.TestATrait.StructA  structure          NO              YES
        //
        kPropertyHandle_TaD = 5,

        //
        //  ta_e                                repeated uint32                     array              NO              NO
        //
        kPropertyHandle_TaE = 6,

        //
        //  ta_g                                weave.common.StringRef              union              NO              NO
        //
        kPropertyHandle_TaG = 7,

        //
        //  ta_h                                repeated nest.test.trait.TestATrait.StructA array      NO              NO
        //
        kPropertyHandle_TaH = 8,

        //
        //  ta_i                                map <uint32,uint32>                 map <unsigned int,unsigned int> NO NO
        //
        kPropertyHandle_TaI = 9,

        //
        //  ta_j                                map <uint32,nest.test.trait.TestATrait.StructA> map <unsigned int,structure> NO NO
        //
        kPropertyHandle_TaJ = 10,

        //
        //  ta_k                                bytes                               bytes              NO              NO
        //
        kPropertyHandle_TaK = 11,

        //
        //  ta_l                                repeated weave.common.DayOfWeek     unsigned int       NO              NO
        //
        kPropertyHandle_TaL = 12,

        //
        //  ta_m                                weave.common.ResourceId             unsigned int       NO              YES
        //
        kPropertyHandle_TaM = 13,

        //
        //  ta_n                                weave.common.ResourceId             bytes              NO              YES
        //
        kPropertyHandle_TaN = 14,

        //
        //  ta_o                                google.protobuf.Timestamp           unsigned int       NO              NO
        //
        kPropertyHandle_TaO = 15,

        //
        //  ta_p                                google.protobuf.Timestamp           int                NO              YES
        //
        kPropertyHandle_TaP = 16,

        //
        //  ta_q                                google.protobuf.Duration            int                NO              NO
        //
        kPropertyHandle_TaQ = 17,

        //
        //  ta_r                                google.protobuf.Duration            unsigned int       NO              NO
        //
        kPropertyHandle_TaR = 18,

        //
        //  ta_s                                google.protobuf.Duration            unsigned int       NO              YES
        //
        kPropertyHandle_TaS = 19,

        //
        //  ta_t                                uint32                              unsigned int       NO              YES
        //
        kPropertyHandle_TaT = 20,

        //
        //  ta_u                                int32                               int                NO              YES
        //
        kPropertyHandle_TaU = 21,

        //
        //  ta_v                                bool                                bool               NO              YES
        //
        kPropertyHandle_TaV = 22,

        //
        //  ta_w                                string                              string             NO              YES
        //
        kPropertyHandle_TaW = 23,

        //
        //  ta_x                                float                               int <16 bits>      NO              YES
        //
        kPropertyHandle_TaX = 24,

        //
        //  tb_a                                uint32                              unsigned int       NO              NO
        //
        kPropertyHandle_TbA = 25,

        //
        //  tb_b                                StructB                             structure          NO              NO
        //
        kPropertyHandle_TbB = 26,

        //
        //  tb_c                                StructEA                            structure          NO              NO
        //
        kPropertyHandle_TbC = 27,

        //
        //  sa_a                                uint32                              unsigned int       NO              YES
        //
        kPropertyHandle_TaD_SaA = 28,

        //
        //  sa_b                                bool                                bool               NO              NO
        //
        kPropertyHandle_TaD_SaB = 29,

        //
        //  value                               uint32                              unsigned int       NO              NO
        //
        kPropertyHandle_TaI_Value = 30,

        //
        //  value                               nest.test.trait.TestATrait.StructA  structure          NO              NO
        //
        kPropertyHandle_TaJ_Value = 31,

        //
        //  sb_a                                string                              string             NO              NO
        //
        kPropertyHandle_TbB_SbA = 32,

        //
        //  sb_b                                uint32                              unsigned int       NO              NO
        //
        kPropertyHandle_TbB_SbB = 33,

        //
        //  sa_a                                uint32                              unsigned int       NO              YES
        //
        kPropertyHandle_TbC_SaA = 34,

        //
        //  sa_b                                bool                                bool               NO              NO
        //
        kPropertyHandle_TbC_SaB = 35,

        //
        //  sea_c                               string                              string             NO              NO
        //
        kPropertyHandle_TbC_SeaC = 36,

        //
        //  sa_a                                uint32                              unsigned int       NO              YES
        //
        kPropertyHandle_TaJ_Value_SaA = 37,

        //
        //  sa_b                                bool                                bool               NO              NO
        //
        kPropertyHandle_TaJ_Value_SaB = 38,

    };

} // namespace TestBTrait
}
}
}

}
#endif // _NEST_TEST_TRAIT__TEST_B_TRAIT_H_
