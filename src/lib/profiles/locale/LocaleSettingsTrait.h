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
 *      SOURCE PROTO: weave/trait/locale/locale_settings_trait.proto
 *
 */

#ifndef _LOCALE_SETTINGS_TRAIT_H
#define _LOCALE_SETTINGS_TRAIT_H

// We want and assume the default managed namespace is Current and that is, explicitly, the managed namespace this code desires.
#include <Weave/Profiles/data-management/TraitData.h>

namespace nl {

    namespace Weave {

        namespace Profiles {
            namespace Locale {
                namespace LocaleSettingsTrait {
                    extern nl::Weave::Profiles::DataManagement::TraitSchemaEngine TraitSchema;

                    enum {
                        kWeaveProfileId = 0x00000014
                    };

                    enum {
                        kPropertyHandle_Root = 1,
                        /*
                         * Root =
                         * {
                         *       active_locale = 1 (string),
                         */
                                kPropertyHandle_active_locale = 2,

                        /*
                         * }
                         *
                         */
                    };
                }; // LocaleSettingsTrait
            }
} // Locale
} // Trait
} // Weave

#endif // _LOCALE_SETTINGS_TRAIT_H
