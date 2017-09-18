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
#ifndef _NEST_TEST_TRAIT__TEST_MISMATCHED_C_TRAIT_H_
#define _NEST_TEST_TRAIT__TEST_MISMATCHED_C_TRAIT_H_

#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Support/SerializationUtils.h>

namespace Schema {
namespace Nest {
namespace Test {
namespace Trait {
namespace TestMismatchedCTrait {

    extern const nl::Weave::Profiles::DataManagement::TraitSchemaEngine TraitSchema;

    enum {
        kWeaveProfileId = (0x235aU << 16) | 0xfe03U
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
        //  tc_a                                bool                                bool               NO              NO
        //
        kPropertyHandle_TcA = 2,

        //
        //  tc_b                                nest.test.trait.EnumC               int                NO              NO
        //
        kPropertyHandle_TcB = 3,

        //
        //  tc_c                                nest.test.trait.StructC             structure          NO              NO
        //
        kPropertyHandle_TcC = 4,

        //
        //  tc_d                                uint32                              unsigned int       NO              NO
        //
        kPropertyHandle_TcD = 5,

        //
        //  tc_e                                nest.test.trait.StructC             structure          NO              NO
        //
        kPropertyHandle_TcE = 6,

        //
        //  sc_a                                uint32                              unsigned int       NO              NO
        //
        kPropertyHandle_TcC_ScA = 7,

        //
        //  sc_b                                bool                                bool               NO              NO
        //
        kPropertyHandle_TcC_ScB = 8,

        //
        //  sc_c                                uint32                              unsigned int       NO              NO
        //
        kPropertyHandle_TcC_ScC = 9,

        //
        //  sc_a                                uint32                              unsigned int       NO              NO
        //
        kPropertyHandle_TcE_ScA = 10,

        //
        //  sc_b                                bool                                bool               NO              NO
        //
        kPropertyHandle_TcE_ScB = 11,

        //
        //  sc_c                                uint32                              unsigned int       NO              NO
        //
        kPropertyHandle_TcE_ScC = 12,


    };

} // namespace TestCTrait
}
}
}

}
#endif // _NEST_TEST_TRAIT__TEST_MISMATCHED_C_TRAIT_H_
