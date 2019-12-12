
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
 *    SOURCE PROTO: weave/trait/log/logging_capabilities_trait.proto
 *
 */

#include <weave/trait/log/LoggingCapabilitiesTrait.h>

namespace Schema {
namespace Weave {
namespace Trait {
namespace Log {
namespace LoggingCapabilitiesTrait {

using namespace ::nl::Weave::Profiles::DataManagement;

//
// Property Table
//

const TraitSchemaEngine::PropertyInfo PropertyMap[] = {
    { kPropertyHandle_Root, 1 }, // supported_log_transports
    { kPropertyHandle_Root, 3 }, // supports_streaming
    { kPropertyHandle_Root, 4 }, // supports_nonvolatile_storage
    { kPropertyHandle_Root, 5 }, // supports_per_trait_verbosity
    { kPropertyHandle_Root, 6 }, // logging_volume
    { kPropertyHandle_Root, 7 }, // log_buffering_capacity
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
        NULL,
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

} // namespace LoggingCapabilitiesTrait
} // namespace Log
} // namespace Trait
} // namespace Weave
} // namespace Schema
