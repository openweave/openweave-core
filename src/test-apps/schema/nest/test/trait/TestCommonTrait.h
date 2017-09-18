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
#ifndef _NEST_TEST_TRAIT__TEST_COMMON_TRAIT_H_
#define _NEST_TEST_TRAIT__TEST_COMMON_TRAIT_H_

#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Support/SerializationUtils.h>

namespace Schema {
namespace Nest {
namespace Test {
namespace Trait {
namespace TestCommonTrait {

    extern const nl::Weave::Profiles::DataManagement::TraitSchemaEngine TraitSchema;

    enum {
        kWeaveProfileId = (0x235aU << 16) | 0x0U
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

} // namespace TestCommonTrait
}
}
}

}
#endif // _NEST_TEST_TRAIT__TEST_COMMON_TRAIT_H_
