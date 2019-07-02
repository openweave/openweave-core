
/**
 *    Copyright (c) 2019 Nest Labs, Inc.
 *    All rights reserved.
 *
 *    THIS FILE IS GENERATED. DO NOT MODIFY.
 *
 *    SOURCE TEMPLATE: trait.cpp
 *    SOURCE PROTO: weave/trait/telemetry/tunnel/telemetry_tunnel_trait.proto
 *
 */

#include <weave/trait/telemetry/tunnel/TelemetryTunnelTrait.h>

namespace Schema {
namespace Weave {
namespace Trait {
namespace Telemetry {
namespace Tunnel {
namespace TelemetryTunnelTrait {

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

const nl::FieldDescriptor TelemetryTunnelStatsEventFieldDescriptors[] =
{
    {
        NULL, offsetof(TelemetryTunnelStatsEvent, txBytesToService), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt64, 0), 1
    },

    {
        NULL, offsetof(TelemetryTunnelStatsEvent, rxBytesFromService), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt64, 0), 2
    },

    {
        NULL, offsetof(TelemetryTunnelStatsEvent, txMessagesToService), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 3
    },

    {
        NULL, offsetof(TelemetryTunnelStatsEvent, rxMessagesFromService), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 4
    },

    {
        NULL, offsetof(TelemetryTunnelStatsEvent, tunnelDownCount), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 5
    },

    {
        NULL, offsetof(TelemetryTunnelStatsEvent, tunnelConnAttemptCount), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 6
    },

    {
        NULL, offsetof(TelemetryTunnelStatsEvent, lastTimeTunnelWentDown), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt64, 0), 7
    },

    {
        NULL, offsetof(TelemetryTunnelStatsEvent, lastTimeTunnelEstablished), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt64, 0), 8
    },

    {
        NULL, offsetof(TelemetryTunnelStatsEvent, droppedMessagesCount), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 9
    },

    {
        NULL, offsetof(TelemetryTunnelStatsEvent, currentTunnelState), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 10
    },

    {
        NULL, offsetof(TelemetryTunnelStatsEvent, currentActiveTunnel), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 11
    },

};

const nl::SchemaFieldDescriptor TelemetryTunnelStatsEvent::FieldSchema =
{
    .mNumFieldDescriptorElements = sizeof(TelemetryTunnelStatsEventFieldDescriptors)/sizeof(TelemetryTunnelStatsEventFieldDescriptors[0]),
    .mFields = TelemetryTunnelStatsEventFieldDescriptors,
    .mSize = sizeof(TelemetryTunnelStatsEvent)
};
const nl::Weave::Profiles::DataManagement::EventSchema TelemetryTunnelStatsEvent::Schema =
{
    .mProfileId = kWeaveProfileId,
    .mStructureType = 0x1,
    .mImportance = nl::Weave::Profiles::DataManagement::Debug,
    .mDataSchemaVersion = 1,
    .mMinCompatibleDataSchemaVersion = 1,
};

} // namespace TelemetryTunnelTrait
} // namespace Tunnel
} // namespace Telemetry
} // namespace Trait
} // namespace Weave
} // namespace Schema
