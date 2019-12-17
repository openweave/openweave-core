
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
 *    SOURCE PROTO: weave/trait/heartbeat/liveness_trait.proto
 *
 */

#include <weave/trait/heartbeat/LivenessTrait.h>

namespace Schema {
namespace Weave {
namespace Trait {
namespace Heartbeat {
namespace LivenessTrait {

using namespace ::nl::Weave::Profiles::DataManagement;

//
// Property Table
//

const TraitSchemaEngine::PropertyInfo PropertyMap[] = {
    { kPropertyHandle_Root, 1 }, // status
    { kPropertyHandle_Root, 2 }, // time_status_changed
    { kPropertyHandle_Root, 3 }, // max_inactivity_duration
    { kPropertyHandle_Root, 4 }, // heartbeat_status
    { kPropertyHandle_Root, 5 }, // time_heartbeat_status_changed
    { kPropertyHandle_Root, 6 }, // notify_request_unresponsiveness
    { kPropertyHandle_Root, 7 }, // notify_request_unresponsiveness_time_status_changed
    { kPropertyHandle_Root, 8 }, // command_request_unresponsiveness
    { kPropertyHandle_Root, 9 }, // command_request_unresponsiveness_time_status_changed
};

//
// IsNullable Table
//

uint8_t IsNullableHandleBitfield[] = {
        0xf0, 0x1
};

//
// Supported version
//
const ConstSchemaVersionRange traitVersion = { .mMinVersion = 1, .mMaxVersion = 2 };

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
        &IsNullableHandleBitfield[0],
        NULL,
#if (TDM_EXTENSION_SUPPORT)
        NULL,
#endif
#if (TDM_VERSIONING_SUPPORT)
        &traitVersion,
#endif
    }
};

    //
    // Events
    //

const nl::FieldDescriptor LivenessChangeEventFieldDescriptors[] =
{
    {
        NULL, offsetof(LivenessChangeEvent, status), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 1
    },

    {
        NULL, offsetof(LivenessChangeEvent, heartbeatStatus), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 2
    },

    {
        NULL, offsetof(LivenessChangeEvent, notifyRequestUnresponsiveness), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeBoolean, 1), 3
    },

    {
        NULL, offsetof(LivenessChangeEvent, commandRequestUnresponsiveness), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeBoolean, 1), 4
    },

    {
        NULL, offsetof(LivenessChangeEvent, prevStatus), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 5
    },

};

const nl::SchemaFieldDescriptor LivenessChangeEvent::FieldSchema =
{
    .mNumFieldDescriptorElements = sizeof(LivenessChangeEventFieldDescriptors)/sizeof(LivenessChangeEventFieldDescriptors[0]),
    .mFields = LivenessChangeEventFieldDescriptors,
    .mSize = sizeof(LivenessChangeEvent)
};
const nl::Weave::Profiles::DataManagement::EventSchema LivenessChangeEvent::Schema =
{
    .mProfileId = kWeaveProfileId,
    .mStructureType = 0x1,
    .mImportance = nl::Weave::Profiles::DataManagement::ProductionCritical,
    .mDataSchemaVersion = 2,
    .mMinCompatibleDataSchemaVersion = 1,
};

} // namespace LivenessTrait
} // namespace Heartbeat
} // namespace Trait
} // namespace Weave
} // namespace Schema
