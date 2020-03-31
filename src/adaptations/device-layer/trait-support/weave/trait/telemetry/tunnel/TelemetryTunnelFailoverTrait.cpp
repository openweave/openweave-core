
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
 *    SOURCE PROTO: weave/trait/telemetry/tunnel/telemetry_tunnel_failover_trait.proto
 *
 */

#include <weave/trait/telemetry/tunnel/TelemetryTunnelFailoverTrait.h>

namespace Schema {
namespace Weave {
namespace Trait {
namespace Telemetry {
namespace Tunnel {
namespace TelemetryTunnelFailoverTrait {

using namespace ::nl::Weave::Profiles::DataManagement;

//
// Property Table
//

const TraitSchemaEngine::PropertyInfo PropertyMap[] = {
};

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

const nl::FieldDescriptor TelemetryTunnelFailoverStatsEventFieldDescriptors[] =
{
    {
        NULL, offsetof(TelemetryTunnelFailoverStatsEvent, txBytesToService), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt64, 0), 1
    },

    {
        NULL, offsetof(TelemetryTunnelFailoverStatsEvent, rxBytesFromService), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt64, 0), 2
    },

    {
        NULL, offsetof(TelemetryTunnelFailoverStatsEvent, txMessagesToService), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 3
    },

    {
        NULL, offsetof(TelemetryTunnelFailoverStatsEvent, rxMessagesFromService), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 4
    },

    {
        NULL, offsetof(TelemetryTunnelFailoverStatsEvent, tunnelDownCount), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 5
    },

    {
        NULL, offsetof(TelemetryTunnelFailoverStatsEvent, tunnelConnAttemptCount), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 6
    },

    {
        NULL, offsetof(TelemetryTunnelFailoverStatsEvent, lastTimeTunnelWentDown), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt64, 0), 7
    },

    {
        NULL, offsetof(TelemetryTunnelFailoverStatsEvent, lastTimeTunnelEstablished), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt64, 0), 8
    },

    {
        NULL, offsetof(TelemetryTunnelFailoverStatsEvent, droppedMessagesCount), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 9
    },

    {
        NULL, offsetof(TelemetryTunnelFailoverStatsEvent, currentTunnelState), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 10
    },

    {
        NULL, offsetof(TelemetryTunnelFailoverStatsEvent, currentActiveTunnel), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 11
    },

    {
        NULL, offsetof(TelemetryTunnelFailoverStatsEvent, backupTxBytesToService), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt64, 0), 32
    },

    {
        NULL, offsetof(TelemetryTunnelFailoverStatsEvent, backupRxBytesFromService), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt64, 0), 33
    },

    {
        NULL, offsetof(TelemetryTunnelFailoverStatsEvent, backupTxMessagesToService), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 34
    },

    {
        NULL, offsetof(TelemetryTunnelFailoverStatsEvent, backupRxMessagesFromService), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 35
    },

    {
        NULL, offsetof(TelemetryTunnelFailoverStatsEvent, backupTunnelDownCount), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 36
    },

    {
        NULL, offsetof(TelemetryTunnelFailoverStatsEvent, backupTunnelConnAttemptCount), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 37
    },

    {
        NULL, offsetof(TelemetryTunnelFailoverStatsEvent, lastTimeBackupTunnelWentDown), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt64, 0), 38
    },

    {
        NULL, offsetof(TelemetryTunnelFailoverStatsEvent, lastTimeBackupTunnelEstablished), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt64, 0), 39
    },

    {
        NULL, offsetof(TelemetryTunnelFailoverStatsEvent, tunnelFailoverCount), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 40
    },

    {
        NULL, offsetof(TelemetryTunnelFailoverStatsEvent, lastTimeForTunnelFailover), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt64, 0), 41
    },

};

const nl::SchemaFieldDescriptor TelemetryTunnelFailoverStatsEvent::FieldSchema =
{
    .mNumFieldDescriptorElements = sizeof(TelemetryTunnelFailoverStatsEventFieldDescriptors)/sizeof(TelemetryTunnelFailoverStatsEventFieldDescriptors[0]),
    .mFields = TelemetryTunnelFailoverStatsEventFieldDescriptors,
    .mSize = sizeof(TelemetryTunnelFailoverStatsEvent)
};
const nl::Weave::Profiles::DataManagement::EventSchema TelemetryTunnelFailoverStatsEvent::Schema =
{
    .mProfileId = kWeaveProfileId,
    .mStructureType = 0x1,
    .mImportance = nl::Weave::Profiles::DataManagement::Debug,
    .mDataSchemaVersion = 1,
    .mMinCompatibleDataSchemaVersion = 1,
};

} // namespace TelemetryTunnelFailoverTrait
} // namespace Tunnel
} // namespace Telemetry
} // namespace Trait
} // namespace Weave
} // namespace Schema
