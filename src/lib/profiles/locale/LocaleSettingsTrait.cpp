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
/**
 *    @file
 *      THIS FILE IS GENERATED. DO NOT MODIFY.
 *
 *      SOURCE PROTO: weave/trait/locale/locale_settings_trait.proto
 *
 */

#include "LocaleSettingsTrait.h"

using namespace nl::Weave::Profiles::DataManagement;

namespace nl {

    namespace Weave {

        namespace Profiles {
            namespace Locale {
                namespace LocaleSettingsTrait {
                    TraitSchemaEngine::PropertyInfo gSchemaMap[] = {
                            /*  ParentHandle                ContextTag */
                            {kPropertyHandle_Root, 1}
                    };

                    TraitSchemaEngine TraitSchema = {
                            {
                                    kWeaveProfileId,
                                    gSchemaMap,
                                    sizeof(gSchemaMap) / sizeof(gSchemaMap[0]),
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

                }; // LocaleSettingsTrait
            };
}; // Locale
}; // Trait
}; // Weave
