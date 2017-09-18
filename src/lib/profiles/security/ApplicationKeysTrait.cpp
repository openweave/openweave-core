/*
 *
 *    Copyright (c) 2016-2017 Nest Labs, Inc.
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

/**
 * THIS FILE IS GENERATED. DO NOT MODIFY.
 *
 * SOURCE TEMPLATE: Trait.cpp.tpl
 * SOURCE PROTO: weave/trait/auth/application_keys_trait.proto
 *
 */

#include "ApplicationKeysTrait.h"

namespace Schema {
namespace Weave {
namespace Trait {
namespace Auth {
namespace ApplicationKeysTrait {

    using namespace ::nl::Weave::Profiles::DataManagement;

    //
    // Property Table
    //

    const TraitSchemaEngine::PropertyInfo PropertyMap[] = {
        { kPropertyHandle_Root, 1 }, // epoch_keys
        { kPropertyHandle_Root, 2 }, // master_keys
    };

    //
    // Schema
    //

    const TraitSchemaEngine TraitSchema = {
        {
            kWeaveProfileId,
            PropertyMap,
            sizeof(PropertyMap) / sizeof(PropertyMap[0]),
            1,
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

} // namespace ApplicationKeysTrait
}
}
}
}
