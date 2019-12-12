
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
 *    SOURCE PROTO: weave/trait/schedule/basic_user_schedules_settings_trait.proto
 *
 */

#include <weave/trait/schedule/BasicUserSchedulesSettingsTrait.h>

namespace Schema {
namespace Weave {
namespace Trait {
namespace Schedule {
namespace BasicUserSchedulesSettingsTrait {

using namespace ::nl::Weave::Profiles::DataManagement;

//
// Property Table
//

const TraitSchemaEngine::PropertyInfo PropertyMap[] = {
    { kPropertyHandle_Root, 1 }, // basic_user_schedules
    { kPropertyHandle_BasicUserSchedules, 0 }, // value
    { kPropertyHandle_BasicUserSchedules_Value, 1 }, // user_id
    { kPropertyHandle_BasicUserSchedules_Value, 2 }, // daily_repeating_schedules
    { kPropertyHandle_BasicUserSchedules_Value, 3 }, // time_box_schedules
};

//
// IsDictionary Table
//

uint8_t IsDictionaryTypeHandleBitfield[] = {
        0x1
};

//
// Schema
//

const TraitSchemaEngine TraitSchema = {
    {
        kWeaveProfileId,
        PropertyMap,
        sizeof(PropertyMap) / sizeof(PropertyMap[0]),
        3,
#if (TDM_EXTENSION_SUPPORT) || (TDM_VERSIONING_SUPPORT)
        2,
#endif
        IsDictionaryTypeHandleBitfield,
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

    //
    // Events
    //

const nl::FieldDescriptor OfflineDeviceSyncSchedulesEventFieldDescriptors[] =
{
    {
        &Schema::Google::Protobuf::FieldMask::FieldSchema, offsetof(OfflineDeviceSyncSchedulesEvent, stateMask), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeStructure, 0), 2
    },

    {
        NULL, offsetof(OfflineDeviceSyncSchedulesEvent, stateVersion), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt64, 0), 3
    },

    {
        NULL, offsetof(OfflineDeviceSyncSchedulesEvent, acceptedTimestamp), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt64, 0), 4
    },

    {
        NULL, offsetof(OfflineDeviceSyncSchedulesEvent, confirmedTimestamp), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt64, 0), 5
    },

};

const nl::SchemaFieldDescriptor OfflineDeviceSyncSchedulesEvent::FieldSchema =
{
    .mNumFieldDescriptorElements = sizeof(OfflineDeviceSyncSchedulesEventFieldDescriptors)/sizeof(OfflineDeviceSyncSchedulesEventFieldDescriptors[0]),
    .mFields = OfflineDeviceSyncSchedulesEventFieldDescriptors,
    .mSize = sizeof(OfflineDeviceSyncSchedulesEvent)
};
const nl::Weave::Profiles::DataManagement::EventSchema OfflineDeviceSyncSchedulesEvent::Schema =
{
    .mProfileId = kWeaveProfileId,
    .mStructureType = 0x1,
    .mImportance = nl::Weave::Profiles::DataManagement::ProductionCritical,
    .mDataSchemaVersion = 1,
    .mMinCompatibleDataSchemaVersion = 1,
};

//
// Event Structs
//

const nl::FieldDescriptor DailyRepeatingScheduleItemFieldDescriptors[] =
{
    {
        NULL, offsetof(DailyRepeatingScheduleItem, daysOfWeek), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 1
    },

    {
        &Schema::Weave::Common::TimeOfDay::FieldSchema, offsetof(DailyRepeatingScheduleItem, startTime), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeStructure, 0), 2
    },

    {
        NULL, offsetof(DailyRepeatingScheduleItem, duration), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 3
    },

};

const nl::SchemaFieldDescriptor DailyRepeatingScheduleItem::FieldSchema =
{
    .mNumFieldDescriptorElements = sizeof(DailyRepeatingScheduleItemFieldDescriptors)/sizeof(DailyRepeatingScheduleItemFieldDescriptors[0]),
    .mFields = DailyRepeatingScheduleItemFieldDescriptors,
    .mSize = sizeof(DailyRepeatingScheduleItem)
};


const nl::FieldDescriptor TimeboxScheduleItemFieldDescriptors[] =
{
    {
        NULL, offsetof(TimeboxScheduleItem, startTime), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 1
    },

    {
        NULL, offsetof(TimeboxScheduleItem, endTime), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 1), 2
    },

};

const nl::SchemaFieldDescriptor TimeboxScheduleItem::FieldSchema =
{
    .mNumFieldDescriptorElements = sizeof(TimeboxScheduleItemFieldDescriptors)/sizeof(TimeboxScheduleItemFieldDescriptors[0]),
    .mFields = TimeboxScheduleItemFieldDescriptors,
    .mSize = sizeof(TimeboxScheduleItem)
};


const nl::FieldDescriptor BasicUserScheduleFieldDescriptors[] =
{
    {
        NULL, offsetof(BasicUserSchedule, userId), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeByteString, 0), 1
    },

    {
        NULL, offsetof(BasicUserSchedule, dailyRepeatingSchedules) + offsetof(Schema::Weave::Trait::Schedule::BasicUserSchedulesSettingsTrait::DailyRepeatingScheduleItem_array, num), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeArray, 0), 2
    },
    {
        &Schema::Weave::Trait::Schedule::BasicUserSchedulesSettingsTrait::DailyRepeatingScheduleItem::FieldSchema, offsetof(BasicUserSchedule, dailyRepeatingSchedules) + offsetof(Schema::Weave::Trait::Schedule::BasicUserSchedulesSettingsTrait::DailyRepeatingScheduleItem_array, buf), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeStructure, 0), 2
    },

    {
        NULL, offsetof(BasicUserSchedule, timeBoxSchedules) + offsetof(Schema::Weave::Trait::Schedule::BasicUserSchedulesSettingsTrait::TimeboxScheduleItem_array, num), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeArray, 0), 3
    },
    {
        &Schema::Weave::Trait::Schedule::BasicUserSchedulesSettingsTrait::TimeboxScheduleItem::FieldSchema, offsetof(BasicUserSchedule, timeBoxSchedules) + offsetof(Schema::Weave::Trait::Schedule::BasicUserSchedulesSettingsTrait::TimeboxScheduleItem_array, buf), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeStructure, 0), 3
    },

};

const nl::SchemaFieldDescriptor BasicUserSchedule::FieldSchema =
{
    .mNumFieldDescriptorElements = sizeof(BasicUserScheduleFieldDescriptors)/sizeof(BasicUserScheduleFieldDescriptors[0]),
    .mFields = BasicUserScheduleFieldDescriptors,
    .mSize = sizeof(BasicUserSchedule)
};

} // namespace BasicUserSchedulesSettingsTrait
} // namespace Schedule
} // namespace Trait
} // namespace Weave
} // namespace Schema
