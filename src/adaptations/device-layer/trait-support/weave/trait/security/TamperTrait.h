
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
 *    SOURCE PROTO: weave/trait/security/tamper_trait.proto
 *
 */
#ifndef _WEAVE_TRAIT_SECURITY__TAMPER_TRAIT_H_
#define _WEAVE_TRAIT_SECURITY__TAMPER_TRAIT_H_

#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Support/SerializationUtils.h>



namespace Schema {
namespace Weave {
namespace Trait {
namespace Security {
namespace TamperTrait {

extern const nl::Weave::Profiles::DataManagement::TraitSchemaEngine TraitSchema;

enum {
      kWeaveProfileId = (0x0U << 16) | 0xe07U
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
    //  tamper_state                        TamperState                          int               NO              NO
    //
    kPropertyHandle_TamperState = 2,

    //
    //  first_observed_at                   google.protobuf.Timestamp            uint32 seconds    YES             YES
    //
    kPropertyHandle_FirstObservedAt = 3,

    //
    //  first_observed_at_ms                google.protobuf.Timestamp            int64 millisecondsYES             YES
    //
    kPropertyHandle_FirstObservedAtMs = 4,

    //
    // Enum for last handle
    //
    kLastSchemaHandle = 4,
};

//
// Events
//
struct TamperStateChangeEvent
{
    int32_t tamperState;
    int32_t priorTamperState;

    static const nl::SchemaFieldDescriptor FieldSchema;

    // Statically-known Event Struct Attributes:
    enum {
            kWeaveProfileId = (0x0U << 16) | 0xe07U,
        kEventTypeId = 0x1U
    };

    static const nl::Weave::Profiles::DataManagement::EventSchema Schema;
};

struct TamperStateChangeEvent_array {
    uint32_t num;
    TamperStateChangeEvent *buf;
};


//
// Commands
//

enum {
    kResetTamperRequestId = 0x1,
};

//
// Enums
//

enum TamperState {
    TAMPER_STATE_CLEAR = 1,
    TAMPER_STATE_TAMPERED = 2,
    TAMPER_STATE_UNKNOWN = 3,
};

} // namespace TamperTrait
} // namespace Security
} // namespace Trait
} // namespace Weave
} // namespace Schema
#endif // _WEAVE_TRAIT_SECURITY__TAMPER_TRAIT_H_
