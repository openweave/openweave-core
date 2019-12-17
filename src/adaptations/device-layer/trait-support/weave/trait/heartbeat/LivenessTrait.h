
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
 *    SOURCE PROTO: weave/trait/heartbeat/liveness_trait.proto
 *
 */
#ifndef _WEAVE_TRAIT_HEARTBEAT__LIVENESS_TRAIT_H_
#define _WEAVE_TRAIT_HEARTBEAT__LIVENESS_TRAIT_H_

#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Support/SerializationUtils.h>



namespace Schema {
namespace Weave {
namespace Trait {
namespace Heartbeat {
namespace LivenessTrait {

extern const nl::Weave::Profiles::DataManagement::TraitSchemaEngine TraitSchema;

enum {
      kWeaveProfileId = (0x0U << 16) | 0x22U
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
    //  status                              LivenessDeviceStatus                 int               NO              NO
    //
    kPropertyHandle_Status = 2,

    //
    //  time_status_changed                 google.protobuf.Timestamp            uint              NO              NO
    //
    kPropertyHandle_TimeStatusChanged = 3,

    //
    //  max_inactivity_duration             google.protobuf.Duration             uint32 seconds    NO              NO
    //
    kPropertyHandle_MaxInactivityDuration = 4,

    //
    //  heartbeat_status                    LivenessDeviceStatus                 int               NO              NO
    //
    kPropertyHandle_HeartbeatStatus = 5,

    //
    //  time_heartbeat_status_changed       google.protobuf.Timestamp            uint              NO              YES
    //
    kPropertyHandle_TimeHeartbeatStatusChanged = 6,

    //
    //  notify_request_unresponsiveness     bool                                 bool              NO              YES
    //
    kPropertyHandle_NotifyRequestUnresponsiveness = 7,

    //
    //  notify_request_unresponsiveness_time_status_changedgoogle.protobuf.Timestamp            uint              NO              YES
    //
    kPropertyHandle_NotifyRequestUnresponsivenessTimeStatusChanged = 8,

    //
    //  command_request_unresponsiveness    bool                                 bool              NO              YES
    //
    kPropertyHandle_CommandRequestUnresponsiveness = 9,

    //
    //  command_request_unresponsiveness_time_status_changedgoogle.protobuf.Timestamp            uint              NO              YES
    //
    kPropertyHandle_CommandRequestUnresponsivenessTimeStatusChanged = 10,

    //
    // Enum for last handle
    //
    kLastSchemaHandle = 10,
};

//
// Events
//
struct LivenessChangeEvent
{
    int32_t status;
    int32_t heartbeatStatus;
    bool notifyRequestUnresponsiveness;
    void SetNotifyRequestUnresponsivenessNull(void);
    void SetNotifyRequestUnresponsivenessPresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsNotifyRequestUnresponsivenessPresent(void);
#endif
    bool commandRequestUnresponsiveness;
    void SetCommandRequestUnresponsivenessNull(void);
    void SetCommandRequestUnresponsivenessPresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsCommandRequestUnresponsivenessPresent(void);
#endif
    int32_t prevStatus;
    uint8_t __nullified_fields__[2/8 + 1];

    static const nl::SchemaFieldDescriptor FieldSchema;

    // Statically-known Event Struct Attributes:
    enum {
            kWeaveProfileId = (0x0U << 16) | 0x22U,
        kEventTypeId = 0x1U
    };

    static const nl::Weave::Profiles::DataManagement::EventSchema Schema;
};

struct LivenessChangeEvent_array {
    uint32_t num;
    LivenessChangeEvent *buf;
};

inline void LivenessChangeEvent::SetNotifyRequestUnresponsivenessNull(void)
{
    SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 0);
}

inline void LivenessChangeEvent::SetNotifyRequestUnresponsivenessPresent(void)
{
    CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 0);
}

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool LivenessChangeEvent::IsNotifyRequestUnresponsivenessPresent(void)
{
    return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 0));
}
#endif
inline void LivenessChangeEvent::SetCommandRequestUnresponsivenessNull(void)
{
    SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 1);
}

inline void LivenessChangeEvent::SetCommandRequestUnresponsivenessPresent(void)
{
    CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 1);
}

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool LivenessChangeEvent::IsCommandRequestUnresponsivenessPresent(void)
{
    return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 1));
}
#endif

//
// Enums
//

enum LivenessDeviceStatus {
    LIVENESS_DEVICE_STATUS_ONLINE = 1,
    LIVENESS_DEVICE_STATUS_UNREACHABLE = 2,
    LIVENESS_DEVICE_STATUS_UNINITIALIZED = 3,
    LIVENESS_DEVICE_STATUS_REBOOTING = 4,
    LIVENESS_DEVICE_STATUS_UPGRADING = 5,
    LIVENESS_DEVICE_STATUS_SCHEDULED_DOWN = 6,
};

} // namespace LivenessTrait
} // namespace Heartbeat
} // namespace Trait
} // namespace Weave
} // namespace Schema
#endif // _WEAVE_TRAIT_HEARTBEAT__LIVENESS_TRAIT_H_
