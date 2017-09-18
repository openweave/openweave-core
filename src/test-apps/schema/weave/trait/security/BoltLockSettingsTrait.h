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
 *    @file
 *      THIS FILE IS GENERATED. DO NOT MODIFY.
 *
 *      SOURCE PROTO: weave/trait/security/bolt_lock_setting_trait.proto
 * 
 */


#ifndef _BOLT_LOCK_SETTING_TRAIT_H
#define _BOLT_LOCK_SETTING_TRAIT_H

// We want and assume the default managed namespace is Current and that is, explicitly, the managed namespace this code desires.
#include <Weave/Profiles/data-management/TraitData.h>

namespace Weave {
namespace Trait {
namespace Security {
namespace BoltLockSettingTrait {
    extern nl::Weave::Profiles::DataManagement::TraitSchemaEngine TraitSchema;

    enum {
        kWeaveProfileId = 0x00000E08
    };

    enum {
        /*
         * Root = 
         * {
         *       auto_relock_on = 1 (bool),
         */
                 kPropertyHandle_auto_relock_on = 2,

        /*
         *       auto_relock_duration = 2 (uint32)
         */
                 kPropertyHandle_auto_relock_duration = 3,

        /*
         * }
         *
         */
    };
}; // BoltLockSettingTrait
} // Security
} // Trait
} // Weave

#endif // _MOCK_TRAIT_SCHEMAS_H
