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

    enum {
        kPropertyHandle_Root = 1,

        //---------------------------------------------------------------------------------------------------------------------------//
        //  Name                                IDL Type                            TLV Type           Optional?       Nullable?     //
        //---------------------------------------------------------------------------------------------------------------------------//

        //
        //  a                                   uint32                              unsigned int       NO              NO
        //
        kPropertyHandle_A = 2,

        //
        //  b                                   uint32                              unsigned int       NO              NO
        //
        kPropertyHandle_B = 3,

        //
        //  c                                   uint32                              unsigned int       NO              NO
        //
        kPropertyHandle_C = 4,

        //
        //  d                                   uint32                              unsigned int       NO              NO
        //
        kPropertyHandle_D = 5,

        //
        //  e                                   uint32                              unsigned int       NO              NO
        //
        kPropertyHandle_E = 6,

        //
        //  f                                   uint32                              unsigned int       NO              NO
        //
        kPropertyHandle_F = 7,

        //
        //  g                                   uint32                              unsigned int       NO              NO
        //
        kPropertyHandle_G = 8,

        //
        //  h                                   uint32                              unsigned int       NO              NO
        //
        kPropertyHandle_H = 9,

        //
        //  i                                   uint32                              unsigned int       NO              NO
        //
        kPropertyHandle_I = 10,

        //
        //  j                                   uint32                              unsigned int       NO              NO
        //
        kPropertyHandle_J = 11,

        //
        //  k                                   nest.test.trait.StructH             structure          NO              NO
        //
        kPropertyHandle_K = 12,

        //
        //  l                                   map <uint32,nest.test.trait.StructDictionary> map <unsigned int,structure> NO NO
        //
        kPropertyHandle_L = 13,

        //
        //  sa                                  map <uint32,nest.test.trait.StructDictionary> map <unsigned int,structure> NO NO
        //
        kPropertyHandle_K_Sa = 14,

        //
        //  sb                                  uint32                              unsigned int       NO              NO
        //
        kPropertyHandle_K_Sb = 15,

        //
        //  sc                                  uint32                              unsigned int       NO              NO
        //
        kPropertyHandle_K_Sc = 16,

        //
        //  value                               nest.test.trait.StructDictionary    structure          NO              NO
        //
        kPropertyHandle_L_Value = 17,

        //
        //  value                               nest.test.trait.StructDictionary    structure          NO              NO
        //
        kPropertyHandle_K_Sa_Value = 18,

        //
        //  da                                  uint32                              unsigned int       NO              NO
        //
        kPropertyHandle_L_Value_Da = 19,

        //
        //  db                                  uint32                              unsigned int       NO              NO
        //
        kPropertyHandle_L_Value_Db = 20,

        //
        //  dc                                  uint32                              unsigned int       NO              NO
        //
        kPropertyHandle_L_Value_Dc = 21,

        //
        //  da                                  uint32                              unsigned int       NO              NO
        //
        kPropertyHandle_K_Sa_Value_Da = 22,

        //
        //  db                                  uint32                              unsigned int       NO              NO
        //
        kPropertyHandle_K_Sa_Value_Db = 23,

        //
        //  dc                                  uint32                              unsigned int       NO              NO
        //
        kPropertyHandle_K_Sa_Value_Dc = 24,

    };

} // namespace TestHTrait
}
}
}

}
#endif // _NEST_TEST_TRAIT__TEST_H_TRAIT_H_
