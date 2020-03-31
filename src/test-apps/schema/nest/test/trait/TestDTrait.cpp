
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
 *    SOURCE TEMPLATE: trait.cpp
 *    SOURCE PROTO: nest/test/trait/test_d_trait.proto
 *
 */

#include <nest/test/trait/TestDTrait.h>

namespace Schema {
namespace Nest {
namespace Test {
namespace Trait {
namespace TestDTrait {

using namespace ::nl::Weave::Profiles::DataManagement;

//
// Property Table
//

const TraitSchemaEngine::PropertyInfo PropertyMap[] = {
    { kPropertyHandle_Root, 1 }, // tc_a
    { kPropertyHandle_Root, 2 }, // tc_b
    { kPropertyHandle_Root, 3 }, // tc_c
    { kPropertyHandle_TcC, 1 }, // sc_a
    { kPropertyHandle_TcC, 2 }, // sc_b
    { kPropertyHandle_Root, 4 }, // tc_d
    { kPropertyHandle_Root, 32 }, // td_d
    { kPropertyHandle_Root, 33 }, // td_e
    { kPropertyHandle_Root, 34 }, // td_f
};

//
// Supported version
//
const ConstSchemaVersionRange traitVersion = { .mMinVersion = 1, .mMaxVersion = 2 };

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
        8,
#endif
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
#if (TDM_EXTENSION_SUPPORT)
        &Nest::Test::Trait::TestCTrait::TraitSchema,
#endif
#if (TDM_VERSIONING_SUPPORT)
        &traitVersion,
#endif
    }
};

} // namespace TestDTrait
} // namespace Trait
} // namespace Test
} // namespace Nest
} // namespace Schema
