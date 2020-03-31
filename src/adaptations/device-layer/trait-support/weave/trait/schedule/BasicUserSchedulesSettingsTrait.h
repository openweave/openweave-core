
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
 *    SOURCE PROTO: weave/trait/schedule/basic_user_schedules_settings_trait.proto
 *
 */
#ifndef _WEAVE_TRAIT_SCHEDULE__BASIC_USER_SCHEDULES_SETTINGS_TRAIT_H_
#define _WEAVE_TRAIT_SCHEDULE__BASIC_USER_SCHEDULES_SETTINGS_TRAIT_H_

#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Support/SerializationUtils.h>

#include <weave/common/TimeOfDayStructSchema.h>
#include <google/protobuf/FieldMaskStructSchema.h>


namespace Schema {
namespace Weave {
namespace Trait {
namespace Schedule {
namespace BasicUserSchedulesSettingsTrait {

extern const nl::Weave::Profiles::DataManagement::TraitSchemaEngine TraitSchema;

enum {
      kWeaveProfileId = (0x0U << 16) | 0xd02U
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
    //  basic_user_schedules                map <uint32,BasicUserSchedule>       map <uint16, structure>NO              NO
    //
    kPropertyHandle_BasicUserSchedules = 2,

    //
    //  value                               BasicUserSchedule                    structure         NO              NO
    //
    kPropertyHandle_BasicUserSchedules_Value = 3,

    //
    //  user_id                             weave.common.ResourceId              bytes             NO              NO
    //
    kPropertyHandle_BasicUserSchedules_Value_UserId = 4,

    //
    //  daily_repeating_schedules           repeated DailyRepeatingScheduleItem  array             NO              NO
    //
    kPropertyHandle_BasicUserSchedules_Value_DailyRepeatingSchedules = 5,

    //
    //  time_box_schedules                  repeated TimeboxScheduleItem         array             NO              NO
    //
    kPropertyHandle_BasicUserSchedules_Value_TimeBoxSchedules = 6,

    //
    // Enum for last handle
    //
    kLastSchemaHandle = 6,
};

//
// Event Structs
//

struct DailyRepeatingScheduleItem
{
    uint32_t daysOfWeek;
    Schema::Weave::Common::TimeOfDay startTime;
    uint32_t duration;

    static const nl::SchemaFieldDescriptor FieldSchema;

};

struct DailyRepeatingScheduleItem_array {
    uint32_t num;
    DailyRepeatingScheduleItem *buf;
};

struct TimeboxScheduleItem
{
    uint32_t startTime;
    uint32_t endTime;
    void SetEndTimeNull(void);
    void SetEndTimePresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsEndTimePresent(void);
#endif
    uint8_t __nullified_fields__[1/8 + 1];

    static const nl::SchemaFieldDescriptor FieldSchema;

};

struct TimeboxScheduleItem_array {
    uint32_t num;
    TimeboxScheduleItem *buf;
};

inline void TimeboxScheduleItem::SetEndTimeNull(void)
{
    SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 0);
}

inline void TimeboxScheduleItem::SetEndTimePresent(void)
{
    CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 0);
}

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool TimeboxScheduleItem::IsEndTimePresent(void)
{
    return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 0));
}
#endif
struct BasicUserSchedule
{
    nl::SerializedByteString userId;
    Schema::Weave::Trait::Schedule::BasicUserSchedulesSettingsTrait::DailyRepeatingScheduleItem_array dailyRepeatingSchedules;
    Schema::Weave::Trait::Schedule::BasicUserSchedulesSettingsTrait::TimeboxScheduleItem_array timeBoxSchedules;

    static const nl::SchemaFieldDescriptor FieldSchema;

};

struct BasicUserSchedule_array {
    uint32_t num;
    BasicUserSchedule *buf;
};

//
// Events
//
struct OfflineDeviceSyncSchedulesEvent
{
    Schema::Google::Protobuf::FieldMask stateMask;
    uint64_t stateVersion;
    int64_t acceptedTimestamp;
    int64_t confirmedTimestamp;

    static const nl::SchemaFieldDescriptor FieldSchema;

    // Statically-known Event Struct Attributes:
    enum {
            kWeaveProfileId = (0x0U << 16) | 0xd02U,
        kEventTypeId = 0x1U
    };

    static const nl::Weave::Profiles::DataManagement::EventSchema Schema;
};

struct OfflineDeviceSyncSchedulesEvent_array {
    uint32_t num;
    OfflineDeviceSyncSchedulesEvent *buf;
};


//
// Commands
//

enum {
    kSetUserScheduleRequestId = 0x1,
    kGetUserScheduleRequestId = 0x2,
    kDeleteUserScheduleRequestId = 0x3,
};

enum SetUserScheduleRequestParameters {
    kSetUserScheduleRequestParameter_UserSchedule = 2,
};

enum GetUserScheduleRequestParameters {
    kGetUserScheduleRequestParameter_UserId = 1,
};

enum DeleteUserScheduleRequestParameters {
    kDeleteUserScheduleRequestParameter_UserId = 1,
};

enum SetUserScheduleResponseParameters {
    kSetUserScheduleResponseParameter_Status = 1,
};

enum GetUserScheduleResponseParameters {
    kGetUserScheduleResponseParameter_Status = 1,
    kGetUserScheduleResponseParameter_UserSchedule = 2,
};

enum DeleteUserScheduleResponseParameters {
    kDeleteUserScheduleResponseParameter_Status = 1,
};

//
// Enums
//

enum ScheduleErrorCodes {
    SCHEDULE_ERROR_CODES_SUCCESS_STATUS = 1,
    SCHEDULE_ERROR_CODES_DUPLICATE_ENTRY = 2,
    SCHEDULE_ERROR_CODES_INDEX_OUT_OF_RANGE = 3,
    SCHEDULE_ERROR_CODES_EMPTY_SCHEDULE_ENTRY = 4,
    SCHEDULE_ERROR_CODES_INVALID_SCHEDULE = 5,
};

} // namespace BasicUserSchedulesSettingsTrait
} // namespace Schedule
} // namespace Trait
} // namespace Weave
} // namespace Schema
#endif // _WEAVE_TRAIT_SCHEDULE__BASIC_USER_SCHEDULES_SETTINGS_TRAIT_H_
