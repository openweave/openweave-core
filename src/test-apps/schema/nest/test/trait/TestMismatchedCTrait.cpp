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
#include <nest/test/trait/TestMismatchedCTrait.h>

namespace Schema {
namespace Nest {
namespace Test {
namespace Trait {
namespace TestMismatchedCTrait {

    using namespace ::nl::Weave::Profiles::DataManagement;

    //
    // Property Table
    //

    const TraitSchemaEngine::PropertyInfo PropertyMap[] = {
        { kPropertyHandle_Root, 1 }, // tc_a
        { kPropertyHandle_Root, 2 }, // tc_b
        { kPropertyHandle_Root, 3 }, // tc_c
        { kPropertyHandle_Root, 4 }, // tc_d
        { kPropertyHandle_Root, 5 }, // tc_e
        { kPropertyHandle_TcC, 1 }, // sc_a
        { kPropertyHandle_TcC, 2 }, // sc_b
        { kPropertyHandle_TcC, 3 }, // sc_c
        { kPropertyHandle_TcE, 1 }, // sc_a
        { kPropertyHandle_TcE, 2 }, // sc_b
        { kPropertyHandle_TcE, 3 }, // sc_c
    };

    //
    // Schema
    //

    const TraitSchemaEngine TraitSchema = {
        {
            kWeaveProfileId,
            PropertyMap,
            sizeof(PropertyMap) / sizeof(PropertyMap[0]),
            2,
#if (TDM_EXTENSION_SUPPORT) || (TDM_VERSIONING_SUPPORT)
            2,
#endif
#if (TDM_DICTIONARY_SUPPORT)
            NULL,
#endif
            NULL,
            NULL,
            NULL,
            NULL,
#if (TDM_EXTENSION_SUPPORT)
            NULL,
#endif
#if (TDM_VERSIONING_SUPPORT)
            NULL,
#endif
        }
    };

} // namespace TestCTrait
}
}
}
}
