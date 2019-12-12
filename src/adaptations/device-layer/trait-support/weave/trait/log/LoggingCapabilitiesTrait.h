
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
 *    SOURCE TEMPLATE: trait.cpp.h
 *    SOURCE PROTO: weave/trait/log/logging_capabilities_trait.proto
 *
 */
#ifndef _WEAVE_TRAIT_LOG__LOGGING_CAPABILITIES_TRAIT_H_
#define _WEAVE_TRAIT_LOG__LOGGING_CAPABILITIES_TRAIT_H_

#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Support/SerializationUtils.h>



namespace Schema {
namespace Weave {
namespace Trait {
namespace Log {
namespace LoggingCapabilitiesTrait {

extern const nl::Weave::Profiles::DataManagement::TraitSchemaEngine TraitSchema;

enum {
      kWeaveProfileId = (0x0U << 16) | 0xa02U
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
    //  supported_log_transports            repeated LogTransport                array             NO              NO
    //
    kPropertyHandle_SupportedLogTransports = 2,

    //
    //  supports_streaming                  bool                                 bool              NO              NO
    //
    kPropertyHandle_SupportsStreaming = 3,

    //
    //  supports_nonvolatile_storage        bool                                 bool              NO              NO
    //
    kPropertyHandle_SupportsNonvolatileStorage = 4,

    //
    //  supports_per_trait_verbosity        bool                                 bool              NO              NO
    //
    kPropertyHandle_SupportsPerTraitVerbosity = 5,

    //
    //  logging_volume                      uint32                               uint32            NO              NO
    //
    kPropertyHandle_LoggingVolume = 6,

    //
    //  log_buffering_capacity              uint32                               uint32            NO              NO
    //
    kPropertyHandle_LogBufferingCapacity = 7,

    //
    // Enum for last handle
    //
    kLastSchemaHandle = 7,
};

//
// Enums
//

enum LogTransport {
    LOG_TRANSPORT_BDX = 1,
    LOG_TRANSPORT_HTTP = 2,
    LOG_TRANSPORT_HTTPS = 3,
};

} // namespace LoggingCapabilitiesTrait
} // namespace Log
} // namespace Trait
} // namespace Weave
} // namespace Schema
#endif // _WEAVE_TRAIT_LOG__LOGGING_CAPABILITIES_TRAIT_H_
