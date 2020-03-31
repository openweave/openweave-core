
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
 *    SOURCE PROTO: weave/trait/telemetry/tunnel/telemetry_tunnel_trait.proto
 *
 */
#ifndef _WEAVE_TRAIT_TELEMETRY_TUNNEL__TELEMETRY_TUNNEL_TRAIT_H_
#define _WEAVE_TRAIT_TELEMETRY_TUNNEL__TELEMETRY_TUNNEL_TRAIT_H_

#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Support/SerializationUtils.h>



namespace Schema {
namespace Weave {
namespace Trait {
namespace Telemetry {
namespace Tunnel {
namespace TelemetryTunnelTrait {

extern const nl::Weave::Profiles::DataManagement::TraitSchemaEngine TraitSchema;

enum {
      kWeaveProfileId = (0x0U << 16) | 0x1701U
};

//
// Events
//
struct TelemetryTunnelStatsEvent
{
    uint64_t txBytesToService;
    uint64_t rxBytesFromService;
    uint32_t txMessagesToService;
    uint32_t rxMessagesFromService;
    uint32_t tunnelDownCount;
    uint32_t tunnelConnAttemptCount;
    int64_t lastTimeTunnelWentDown;
    int64_t lastTimeTunnelEstablished;
    uint32_t droppedMessagesCount;
    int32_t currentTunnelState;
    int32_t currentActiveTunnel;

    static const nl::SchemaFieldDescriptor FieldSchema;

    // Statically-known Event Struct Attributes:
    enum {
            kWeaveProfileId = (0x0U << 16) | 0x1701U,
        kEventTypeId = 0x1U
    };

    static const nl::Weave::Profiles::DataManagement::EventSchema Schema;
};

struct TelemetryTunnelStatsEvent_array {
    uint32_t num;
    TelemetryTunnelStatsEvent *buf;
};


//
// Enums
//

enum TunnelType {
    TUNNEL_TYPE_NONE = 1,
    TUNNEL_TYPE_PRIMARY = 2,
    TUNNEL_TYPE_BACKUP = 3,
    TUNNEL_TYPE_SHORTCUT = 4,
};

enum TunnelState {
    TUNNEL_STATE_NO_TUNNEL = 1,
    TUNNEL_STATE_PRIMARY_ESTABLISHED = 2,
    TUNNEL_STATE_BACKUP_ONLY_ESTABLISHED = 3,
    TUNNEL_STATE_PRIMARY_AND_BACKUP_ESTABLISHED = 4,
};

} // namespace TelemetryTunnelTrait
} // namespace Tunnel
} // namespace Telemetry
} // namespace Trait
} // namespace Weave
} // namespace Schema
#endif // _WEAVE_TRAIT_TELEMETRY_TUNNEL__TELEMETRY_TUNNEL_TRAIT_H_
