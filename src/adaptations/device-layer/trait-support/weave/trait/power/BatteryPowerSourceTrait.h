
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
 *    SOURCE PROTO: weave/trait/power/battery_power_source_trait.proto
 *
 */
#ifndef _WEAVE_TRAIT_POWER__BATTERY_POWER_SOURCE_TRAIT_H_
#define _WEAVE_TRAIT_POWER__BATTERY_POWER_SOURCE_TRAIT_H_

#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Support/SerializationUtils.h>

#include <weave/common/TimerStructSchema.h>
#include <weave/trait/power/PowerSourceCapabilitiesTrait.h>
#include <weave/trait/power/PowerSourceTrait.h>


namespace Schema {
namespace Weave {
namespace Trait {
namespace Power {
namespace BatteryPowerSourceTrait {

extern const nl::Weave::Profiles::DataManagement::TraitSchemaEngine TraitSchema;

enum {
      kWeaveProfileId = (0x0U << 16) | 0x1cU
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
    //  condition                           weave.trait.power.PowerSourceTrait.PowerSourceCondition int               NO              NO
    //
    kPropertyHandle_Condition = 6,

    //
    //  status                              weave.trait.power.PowerSourceTrait.PowerSourceStatus int               NO              NO
    //
    kPropertyHandle_Status = 7,

    //
    //  present                             bool                                 bool              NO              NO
    //
    kPropertyHandle_Present = 8,

    //
    //  replacement_indicator               BatteryReplacementIndicator          int               YES             NO
    //
    kPropertyHandle_ReplacementIndicator = 9,

    //
    //  remaining                           BatteryRemaining                     structure         YES             YES
    //
    kPropertyHandle_Remaining = 10,

    //
    //  remaining_percent                   float                                uint8             YES             YES
    //
    kPropertyHandle_Remaining_RemainingPercent = 11,

    //
    //  remaining_time                      weave.common.Timer                   structure         YES             YES
    //
    kPropertyHandle_Remaining_RemainingTime = 12,

    //
    //  time                                google.protobuf.Duration             int64 millisecondsNO              NO
    //
    kPropertyHandle_Remaining_RemainingTime_Time = 13,

    //
    //  time_basis                          google.protobuf.Timestamp            int64 millisecondsNO              NO
    //
    kPropertyHandle_Remaining_RemainingTime_TimeBasis = 14,

    //
    // Enum for last handle
    //
    kLastSchemaHandle = 14,
};

//
// Event Structs
//

struct BatteryRemaining
{
    uint8_t remainingPercent;
    void SetRemainingPercentNull(void);
    void SetRemainingPercentPresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsRemainingPercentPresent(void);
#endif
    Schema::Weave::Common::Timer remainingTime;
    void SetRemainingTimeNull(void);
    void SetRemainingTimePresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsRemainingTimePresent(void);
#endif
    uint8_t __nullified_fields__[2/8 + 1];

    static const nl::SchemaFieldDescriptor FieldSchema;

};

struct BatteryRemaining_array {
    uint32_t num;
    BatteryRemaining *buf;
};

inline void BatteryRemaining::SetRemainingPercentNull(void)
{
    SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 0);
}

inline void BatteryRemaining::SetRemainingPercentPresent(void)
{
    CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 0);
}

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool BatteryRemaining::IsRemainingPercentPresent(void)
{
    return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 0));
}
#endif
inline void BatteryRemaining::SetRemainingTimeNull(void)
{
    SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 1);
}

inline void BatteryRemaining::SetRemainingTimePresent(void)
{
    CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 1);
}

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool BatteryRemaining::IsRemainingTimePresent(void)
{
    return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 1));
}
#endif
//
// Events
//
struct BatteryChangedEvent
{
    int32_t condition;
    int32_t status;
    int32_t replacementIndicator;
    Schema::Weave::Trait::Power::BatteryPowerSourceTrait::BatteryRemaining remaining;

    static const nl::SchemaFieldDescriptor FieldSchema;

    // Statically-known Event Struct Attributes:
    enum {
            kWeaveProfileId = (0x0U << 16) | 0x1cU,
        kEventTypeId = 0x1U
    };

    static const nl::Weave::Profiles::DataManagement::EventSchema Schema;
};

struct BatteryChangedEvent_array {
    uint32_t num;
    BatteryChangedEvent *buf;
};


//
// Enums
//

enum BatteryReplacementIndicator {
    BATTERY_REPLACEMENT_INDICATOR_NOT_AT_ALL = 1,
    BATTERY_REPLACEMENT_INDICATOR_SOON = 2,
    BATTERY_REPLACEMENT_INDICATOR_IMMEDIATELY = 3,
};

} // namespace BatteryPowerSourceTrait
} // namespace Power
} // namespace Trait
} // namespace Weave
} // namespace Schema
#endif // _WEAVE_TRAIT_POWER__BATTERY_POWER_SOURCE_TRAIT_H_
