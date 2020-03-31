
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
 *    SOURCE PROTO: weave/trait/heartbeat/liveness_signal_trait.proto
 *
 */
#ifndef _WEAVE_TRAIT_HEARTBEAT__LIVENESS_SIGNAL_TRAIT_H_
#define _WEAVE_TRAIT_HEARTBEAT__LIVENESS_SIGNAL_TRAIT_H_

#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Support/SerializationUtils.h>



namespace Schema {
namespace Weave {
namespace Trait {
namespace Heartbeat {
namespace LivenessSignalTrait {

extern const nl::Weave::Profiles::DataManagement::TraitSchemaEngine TraitSchema;

enum {
      kWeaveProfileId = (0x0U << 16) | 0x25U
};

//
// Events
//
struct LivenessSignalEvent
{
    int32_t signalType;
    int64_t wdmSubscriptionId;
    void SetWdmSubscriptionIdNull(void);
    void SetWdmSubscriptionIdPresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsWdmSubscriptionIdPresent(void);
#endif
    uint8_t __nullified_fields__[1/8 + 1];

    static const nl::SchemaFieldDescriptor FieldSchema;

    // Statically-known Event Struct Attributes:
    enum {
            kWeaveProfileId = (0x0U << 16) | 0x25U,
        kEventTypeId = 0x1U
    };

    static const nl::Weave::Profiles::DataManagement::EventSchema Schema;
};

struct LivenessSignalEvent_array {
    uint32_t num;
    LivenessSignalEvent *buf;
};

inline void LivenessSignalEvent::SetWdmSubscriptionIdNull(void)
{
    SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 0);
}

inline void LivenessSignalEvent::SetWdmSubscriptionIdPresent(void)
{
    CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 0);
}

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool LivenessSignalEvent::IsWdmSubscriptionIdPresent(void)
{
    return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 0));
}
#endif

//
// Enums
//

enum LivenessSignalType {
    LIVENESS_SIGNAL_TYPE_MUTUAL_SUBSCRIPTION_ESTABLISHED = 1,
    LIVENESS_SIGNAL_TYPE_SUBSCRIPTION_HEARTBEAT = 2,
    LIVENESS_SIGNAL_TYPE_NON_SUBSCRIPTION_HEARTBEAT = 3,
    LIVENESS_SIGNAL_TYPE_NOTIFY_REQUEST_UNDELIVERED = 4,
    LIVENESS_SIGNAL_TYPE_COMMAND_REQUEST_UNDELIVERED = 5,
};

} // namespace LivenessSignalTrait
} // namespace Heartbeat
} // namespace Trait
} // namespace Weave
} // namespace Schema
#endif // _WEAVE_TRAIT_HEARTBEAT__LIVENESS_SIGNAL_TRAIT_H_
