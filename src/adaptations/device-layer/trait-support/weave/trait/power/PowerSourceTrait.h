
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
 *    SOURCE PROTO: weave/trait/power/power_source_trait.proto
 *
 */
#ifndef _WEAVE_TRAIT_POWER__POWER_SOURCE_TRAIT_H_
#define _WEAVE_TRAIT_POWER__POWER_SOURCE_TRAIT_H_

#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Support/SerializationUtils.h>

#include <weave/trait/power/PowerSourceCapabilitiesTrait.h>


namespace Schema {
namespace Weave {
namespace Trait {
namespace Power {
namespace PowerSourceTrait {

extern const nl::Weave::Profiles::DataManagement::TraitSchemaEngine TraitSchema;

enum {
      kWeaveProfileId = (0x0U << 16) | 0x19U
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
    //  type                                weave.trait.power.PowerSourceCapabilitiesTrait.PowerSourceType int               NO              NO
    //
    kPropertyHandle_Type = 2,

    //
    //  assessed_voltage                    float                                uint32            YES             YES
    //
    kPropertyHandle_AssessedVoltage = 3,

    //
    //  assessed_current                    float                                uint32            YES             YES
    //
    kPropertyHandle_AssessedCurrent = 4,

    //
    //  assessed_frequency                  float                                uint16            YES             YES
    //
    kPropertyHandle_AssessedFrequency = 5,

    //
    //  condition                           PowerSourceCondition                 int               NO              NO
    //
    kPropertyHandle_Condition = 6,

    //
    //  status                              PowerSourceStatus                    int               NO              NO
    //
    kPropertyHandle_Status = 7,

    //
    //  present                             bool                                 bool              NO              NO
    //
    kPropertyHandle_Present = 8,

    //
    // Enum for last handle
    //
    kLastSchemaHandle = 8,
};

//
// Events
//
struct PowerSourceChangedEvent
{
    int32_t condition;
    int32_t status;

    static const nl::SchemaFieldDescriptor FieldSchema;

    // Statically-known Event Struct Attributes:
    enum {
            kWeaveProfileId = (0x0U << 16) | 0x19U,
        kEventTypeId = 0x1U
    };

    static const nl::Weave::Profiles::DataManagement::EventSchema Schema;
};

struct PowerSourceChangedEvent_array {
    uint32_t num;
    PowerSourceChangedEvent *buf;
};


//
// Enums
//

enum PowerSourceCondition {
    POWER_SOURCE_CONDITION_NOMINAL = 1,
    POWER_SOURCE_CONDITION_CRITICAL = 2,
};

enum PowerSourceStatus {
    POWER_SOURCE_STATUS_ACTIVE = 1,
    POWER_SOURCE_STATUS_STANDBY = 2,
    POWER_SOURCE_STATUS_INACTIVE = 3,
};

} // namespace PowerSourceTrait
} // namespace Power
} // namespace Trait
} // namespace Weave
} // namespace Schema
#endif // _WEAVE_TRAIT_POWER__POWER_SOURCE_TRAIT_H_
