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
 * SOURCE TEMPLATE: Trait.h.tpl
 * SOURCE PROTO: weave/trait/auth/application_keys_trait.proto
 *
 */

#ifndef _WEAVE_TRAIT_AUTH__APPLICATION_KEYS_TRAIT_H_
#define _WEAVE_TRAIT_AUTH__APPLICATION_KEYS_TRAIT_H_

#include <Weave/Profiles/data-management/TraitData.h>
#include <Weave/Support/SerializationUtils.h>

namespace Schema {
namespace Weave {
namespace Trait {
namespace Auth {
namespace ApplicationKeysTrait {

    extern const nl::Weave::Profiles::DataManagement::TraitSchemaEngine TraitSchema;

    enum {
        kWeaveProfileId = (0x0U << 16) | 0x1dU
    };

    enum {
        kPropertyHandle_Root = 1,

        //---------------------------------------------------------------------------------------------------------------------------//
        //  Name                                IDL Type                            TLV Type           Optional?       Nullable?     //
        //---------------------------------------------------------------------------------------------------------------------------//

        //
        //  epoch_keys                          repeated EpochKey                   array              NO              NO
        //
        kPropertyHandle_EpochKeys = 2,

        //
        //  master_keys                         repeated ApplicationMasterKey       array              NO              NO
        //
        kPropertyHandle_MasterKeys = 3,

    };

} // namespace ApplicationKeysTrait
}
}
}

}

#endif // _WEAVE_TRAIT_AUTH__APPLICATION_KEYS_TRAIT_H_
