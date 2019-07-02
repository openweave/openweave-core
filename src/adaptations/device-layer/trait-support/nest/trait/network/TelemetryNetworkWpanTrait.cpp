
/**
 *    Copyright (c) 2019 Nest Labs, Inc.
 *    All rights reserved.
 *
 *    THIS FILE IS GENERATED. DO NOT MODIFY.
 *
 *    SOURCE TEMPLATE: trait.cpp
 *    SOURCE PROTO: nest/trait/network/telemetry_network_wpan_trait.proto
 *
 */

#include <nest/trait/network/TelemetryNetworkWpanTrait.h>

namespace Schema {
namespace Nest {
namespace Trait {
namespace Network {
namespace TelemetryNetworkWpanTrait {

using namespace ::nl::Weave::Profiles::DataManagement;

//
// Property Table
//

const TraitSchemaEngine::PropertyInfo PropertyMap[] = {
};

//
// Supported version
//
const ConstSchemaVersionRange traitVersion = { .mMinVersion = 1, .mMaxVersion = 3 };

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
        &traitVersion,
#endif
    }
};

    //
    // Events
    //

const nl::FieldDescriptor WpanParentLinkEventFieldDescriptors[] =
{
    {
        NULL, offsetof(WpanParentLinkEvent, rssi), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt8, 0), 1
    },

    {
        NULL, offsetof(WpanParentLinkEvent, unicastCcaThresholdFailures), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt16, 0), 2
    },

    {
        NULL, offsetof(WpanParentLinkEvent, unicastMacRetryCount), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt16, 0), 3
    },

};

const nl::SchemaFieldDescriptor WpanParentLinkEvent::FieldSchema =
{
    .mNumFieldDescriptorElements = sizeof(WpanParentLinkEventFieldDescriptors)/sizeof(WpanParentLinkEventFieldDescriptors[0]),
    .mFields = WpanParentLinkEventFieldDescriptors,
    .mSize = sizeof(WpanParentLinkEvent)
};
const nl::Weave::Profiles::DataManagement::EventSchema WpanParentLinkEvent::Schema =
{
    .mProfileId = kWeaveProfileId,
    .mStructureType = 0x1,
    .mImportance = nl::Weave::Profiles::DataManagement::Debug,
    .mDataSchemaVersion = 3,
    .mMinCompatibleDataSchemaVersion = 1,
};

const nl::FieldDescriptor NetworkWpanStatsEventFieldDescriptors[] =
{
    {
        NULL, offsetof(NetworkWpanStatsEvent, phyRx), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 1
    },

    {
        NULL, offsetof(NetworkWpanStatsEvent, phyTx), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 2
    },

    {
        NULL, offsetof(NetworkWpanStatsEvent, macUnicastRx), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 3
    },

    {
        NULL, offsetof(NetworkWpanStatsEvent, macUnicastTx), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 4
    },

    {
        NULL, offsetof(NetworkWpanStatsEvent, macBroadcastRx), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 5
    },

    {
        NULL, offsetof(NetworkWpanStatsEvent, macBroadcastTx), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 6
    },

    {
        NULL, offsetof(NetworkWpanStatsEvent, macTxAckReq), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 24
    },

    {
        NULL, offsetof(NetworkWpanStatsEvent, macTxNoAckReq), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 25
    },

    {
        NULL, offsetof(NetworkWpanStatsEvent, macTxAcked), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 26
    },

    {
        NULL, offsetof(NetworkWpanStatsEvent, macTxData), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 27
    },

    {
        NULL, offsetof(NetworkWpanStatsEvent, macTxDataPoll), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 28
    },

    {
        NULL, offsetof(NetworkWpanStatsEvent, macTxBeacon), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 29
    },

    {
        NULL, offsetof(NetworkWpanStatsEvent, macTxBeaconReq), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 30
    },

    {
        NULL, offsetof(NetworkWpanStatsEvent, macTxOtherPkt), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 31
    },

    {
        NULL, offsetof(NetworkWpanStatsEvent, macTxRetry), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 32
    },

    {
        NULL, offsetof(NetworkWpanStatsEvent, macRxData), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 33
    },

    {
        NULL, offsetof(NetworkWpanStatsEvent, macRxDataPoll), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 34
    },

    {
        NULL, offsetof(NetworkWpanStatsEvent, macRxBeacon), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 35
    },

    {
        NULL, offsetof(NetworkWpanStatsEvent, macRxBeaconReq), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 36
    },

    {
        NULL, offsetof(NetworkWpanStatsEvent, macRxOtherPkt), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 37
    },

    {
        NULL, offsetof(NetworkWpanStatsEvent, macRxFilterWhitelist), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 38
    },

    {
        NULL, offsetof(NetworkWpanStatsEvent, macRxFilterDestAddr), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 39
    },

    {
        NULL, offsetof(NetworkWpanStatsEvent, macTxFailCca), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 8
    },

    {
        NULL, offsetof(NetworkWpanStatsEvent, macRxFailDecrypt), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 12
    },

    {
        NULL, offsetof(NetworkWpanStatsEvent, macRxFailNoFrame), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 20
    },

    {
        NULL, offsetof(NetworkWpanStatsEvent, macRxFailUnknownNeighbor), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 21
    },

    {
        NULL, offsetof(NetworkWpanStatsEvent, macRxFailInvalidSrcAddr), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 22
    },

    {
        NULL, offsetof(NetworkWpanStatsEvent, macRxFailFcs), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 23
    },

    {
        NULL, offsetof(NetworkWpanStatsEvent, macRxFailOther), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 40
    },

    {
        NULL, offsetof(NetworkWpanStatsEvent, ipTxSuccess), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 41
    },

    {
        NULL, offsetof(NetworkWpanStatsEvent, ipRxSuccess), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 42
    },

    {
        NULL, offsetof(NetworkWpanStatsEvent, ipTxFailure), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 43
    },

    {
        NULL, offsetof(NetworkWpanStatsEvent, ipRxFailure), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 44
    },

    {
        NULL, offsetof(NetworkWpanStatsEvent, nodeType), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt8, 0), 15
    },

    {
        NULL, offsetof(NetworkWpanStatsEvent, channel), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt8, 0), 16
    },

    {
        NULL, offsetof(NetworkWpanStatsEvent, radioTxPower), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt8, 0), 17
    },

    {
        NULL, offsetof(NetworkWpanStatsEvent, threadType), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 18
    },

    {
        NULL, offsetof(NetworkWpanStatsEvent, ncpTxTotalTime), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 45
    },

    {
        NULL, offsetof(NetworkWpanStatsEvent, ncpRxTotalTime), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 46
    },

    {
        NULL, offsetof(NetworkWpanStatsEvent, macCcaFailRate), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt16, 0), 47
    },

};

const nl::SchemaFieldDescriptor NetworkWpanStatsEvent::FieldSchema =
{
    .mNumFieldDescriptorElements = sizeof(NetworkWpanStatsEventFieldDescriptors)/sizeof(NetworkWpanStatsEventFieldDescriptors[0]),
    .mFields = NetworkWpanStatsEventFieldDescriptors,
    .mSize = sizeof(NetworkWpanStatsEvent)
};
const nl::Weave::Profiles::DataManagement::EventSchema NetworkWpanStatsEvent::Schema =
{
    .mProfileId = kWeaveProfileId,
    .mStructureType = 0x2,
    .mImportance = nl::Weave::Profiles::DataManagement::Debug,
    .mDataSchemaVersion = 3,
    .mMinCompatibleDataSchemaVersion = 1,
};

const nl::FieldDescriptor NetworkWpanTopoMinimalEventFieldDescriptors[] =
{
    {
        NULL, offsetof(NetworkWpanTopoMinimalEvent, rloc16), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt16, 0), 1
    },

    {
        NULL, offsetof(NetworkWpanTopoMinimalEvent, routerId), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt8, 0), 2
    },

    {
        NULL, offsetof(NetworkWpanTopoMinimalEvent, leaderRouterId), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt8, 0), 3
    },

    {
        NULL, offsetof(NetworkWpanTopoMinimalEvent, parentAverageRssi), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt8, 0), 4
    },

    {
        NULL, offsetof(NetworkWpanTopoMinimalEvent, parentLastRssi), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt8, 0), 5
    },

    {
        NULL, offsetof(NetworkWpanTopoMinimalEvent, partitionId), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 6
    },

    {
        NULL, offsetof(NetworkWpanTopoMinimalEvent, extAddress), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeByteString, 0), 7
    },

    {
        NULL, offsetof(NetworkWpanTopoMinimalEvent, instantRssi), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt8, 0), 8
    },

};

const nl::SchemaFieldDescriptor NetworkWpanTopoMinimalEvent::FieldSchema =
{
    .mNumFieldDescriptorElements = sizeof(NetworkWpanTopoMinimalEventFieldDescriptors)/sizeof(NetworkWpanTopoMinimalEventFieldDescriptors[0]),
    .mFields = NetworkWpanTopoMinimalEventFieldDescriptors,
    .mSize = sizeof(NetworkWpanTopoMinimalEvent)
};
const nl::Weave::Profiles::DataManagement::EventSchema NetworkWpanTopoMinimalEvent::Schema =
{
    .mProfileId = kWeaveProfileId,
    .mStructureType = 0x3,
    .mImportance = nl::Weave::Profiles::DataManagement::Debug,
    .mDataSchemaVersion = 3,
    .mMinCompatibleDataSchemaVersion = 1,
};

const nl::FieldDescriptor NetworkWpanTopoFullEventFieldDescriptors[] =
{
    {
        NULL, offsetof(NetworkWpanTopoFullEvent, rloc16), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt16, 0), 1
    },

    {
        NULL, offsetof(NetworkWpanTopoFullEvent, routerId), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt8, 0), 2
    },

    {
        NULL, offsetof(NetworkWpanTopoFullEvent, leaderRouterId), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt8, 0), 3
    },

    {
        NULL, offsetof(NetworkWpanTopoFullEvent, extAddress), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeByteString, 0), 15
    },

    {
        NULL, offsetof(NetworkWpanTopoFullEvent, leaderAddress), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeByteString, 0), 4
    },

    {
        NULL, offsetof(NetworkWpanTopoFullEvent, leaderWeight), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt8, 0), 5
    },

    {
        NULL, offsetof(NetworkWpanTopoFullEvent, leaderLocalWeight), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt8, 0), 6
    },

    {
        NULL, offsetof(NetworkWpanTopoFullEvent, networkData), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeByteString, 0), 9
    },

    {
        NULL, offsetof(NetworkWpanTopoFullEvent, networkDataVersion), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt8, 0), 10
    },

    {
        NULL, offsetof(NetworkWpanTopoFullEvent, stableNetworkData), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeByteString, 0), 11
    },

    {
        NULL, offsetof(NetworkWpanTopoFullEvent, stableNetworkDataVersion), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt8, 0), 12
    },

    {
        NULL, offsetof(NetworkWpanTopoFullEvent, preferredRouterId), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt8, 0), 13
    },

    {
        NULL, offsetof(NetworkWpanTopoFullEvent, partitionId), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 14
    },

    {
        NULL, offsetof(NetworkWpanTopoFullEvent, deprecatedChildTable) + offsetof(Schema::Nest::Trait::Network::TelemetryNetworkWpanTrait::ChildTableEntry_array, num), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeArray, 0), 7
    },
    {
        &Schema::Nest::Trait::Network::TelemetryNetworkWpanTrait::ChildTableEntry::FieldSchema, offsetof(NetworkWpanTopoFullEvent, deprecatedChildTable) + offsetof(Schema::Nest::Trait::Network::TelemetryNetworkWpanTrait::ChildTableEntry_array, buf), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeStructure, 0), 7
    },

    {
        NULL, offsetof(NetworkWpanTopoFullEvent, deprecatedNeighborTable) + offsetof(Schema::Nest::Trait::Network::TelemetryNetworkWpanTrait::NeighborTableEntry_array, num), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeArray, 0), 8
    },
    {
        &Schema::Nest::Trait::Network::TelemetryNetworkWpanTrait::NeighborTableEntry::FieldSchema, offsetof(NetworkWpanTopoFullEvent, deprecatedNeighborTable) + offsetof(Schema::Nest::Trait::Network::TelemetryNetworkWpanTrait::NeighborTableEntry_array, buf), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeStructure, 0), 8
    },

    {
        NULL, offsetof(NetworkWpanTopoFullEvent, childTableSize), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt16, 0), 16
    },

    {
        NULL, offsetof(NetworkWpanTopoFullEvent, neighborTableSize), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt16, 0), 17
    },

    {
        NULL, offsetof(NetworkWpanTopoFullEvent, instantRssi), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt8, 0), 18
    },

};

const nl::SchemaFieldDescriptor NetworkWpanTopoFullEvent::FieldSchema =
{
    .mNumFieldDescriptorElements = sizeof(NetworkWpanTopoFullEventFieldDescriptors)/sizeof(NetworkWpanTopoFullEventFieldDescriptors[0]),
    .mFields = NetworkWpanTopoFullEventFieldDescriptors,
    .mSize = sizeof(NetworkWpanTopoFullEvent)
};
const nl::Weave::Profiles::DataManagement::EventSchema NetworkWpanTopoFullEvent::Schema =
{
    .mProfileId = kWeaveProfileId,
    .mStructureType = 0x4,
    .mImportance = nl::Weave::Profiles::DataManagement::Debug,
    .mDataSchemaVersion = 3,
    .mMinCompatibleDataSchemaVersion = 1,
};

const nl::FieldDescriptor TopoEntryEventFieldDescriptors[] =
{
    {
        NULL, offsetof(TopoEntryEvent, extAddress), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeByteString, 0), 1
    },

    {
        NULL, offsetof(TopoEntryEvent, rloc16), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt16, 0), 2
    },

    {
        NULL, offsetof(TopoEntryEvent, linkQualityIn), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt8, 0), 3
    },

    {
        NULL, offsetof(TopoEntryEvent, averageRssi), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt8, 0), 4
    },

    {
        NULL, offsetof(TopoEntryEvent, age), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 5
    },

    {
        NULL, offsetof(TopoEntryEvent, rxOnWhenIdle), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeBoolean, 0), 6
    },

    {
        NULL, offsetof(TopoEntryEvent, fullFunction), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeBoolean, 0), 7
    },

    {
        NULL, offsetof(TopoEntryEvent, secureDataRequest), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeBoolean, 0), 8
    },

    {
        NULL, offsetof(TopoEntryEvent, fullNetworkData), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeBoolean, 0), 9
    },

    {
        NULL, offsetof(TopoEntryEvent, lastRssi), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt8, 0), 10
    },

    {
        NULL, offsetof(TopoEntryEvent, linkFrameCounter), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 11
    },

    {
        NULL, offsetof(TopoEntryEvent, mleFrameCounter), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 12
    },

    {
        NULL, offsetof(TopoEntryEvent, isChild), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeBoolean, 0), 13
    },

    {
        NULL, offsetof(TopoEntryEvent, timeout), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 1), 14
    },

    {
        NULL, offsetof(TopoEntryEvent, networkDataVersion), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt8, 1), 15
    },

    {
        NULL, offsetof(TopoEntryEvent, macFrameErrorRate), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt16, 0), 16
    },

    {
        NULL, offsetof(TopoEntryEvent, ipMessageErrorRate), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt16, 0), 17
    },

};

const nl::SchemaFieldDescriptor TopoEntryEvent::FieldSchema =
{
    .mNumFieldDescriptorElements = sizeof(TopoEntryEventFieldDescriptors)/sizeof(TopoEntryEventFieldDescriptors[0]),
    .mFields = TopoEntryEventFieldDescriptors,
    .mSize = sizeof(TopoEntryEvent)
};
const nl::Weave::Profiles::DataManagement::EventSchema TopoEntryEvent::Schema =
{
    .mProfileId = kWeaveProfileId,
    .mStructureType = 0x5,
    .mImportance = nl::Weave::Profiles::DataManagement::Debug,
    .mDataSchemaVersion = 3,
    .mMinCompatibleDataSchemaVersion = 1,
};

const nl::FieldDescriptor WpanChannelmonStatsEventFieldDescriptors[] =
{
    {
        NULL, offsetof(WpanChannelmonStatsEvent, channels) + offsetof(Schema::Nest::Trait::Network::TelemetryNetworkWpanTrait::ChannelUtilization_array, num), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeArray, 0), 1
    },
    {
        &Schema::Nest::Trait::Network::TelemetryNetworkWpanTrait::ChannelUtilization::FieldSchema, offsetof(WpanChannelmonStatsEvent, channels) + offsetof(Schema::Nest::Trait::Network::TelemetryNetworkWpanTrait::ChannelUtilization_array, buf), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeStructure, 0), 1
    },

    {
        NULL, offsetof(WpanChannelmonStatsEvent, samples), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 2
    },

};

const nl::SchemaFieldDescriptor WpanChannelmonStatsEvent::FieldSchema =
{
    .mNumFieldDescriptorElements = sizeof(WpanChannelmonStatsEventFieldDescriptors)/sizeof(WpanChannelmonStatsEventFieldDescriptors[0]),
    .mFields = WpanChannelmonStatsEventFieldDescriptors,
    .mSize = sizeof(WpanChannelmonStatsEvent)
};
const nl::Weave::Profiles::DataManagement::EventSchema WpanChannelmonStatsEvent::Schema =
{
    .mProfileId = kWeaveProfileId,
    .mStructureType = 0x6,
    .mImportance = nl::Weave::Profiles::DataManagement::Debug,
    .mDataSchemaVersion = 3,
    .mMinCompatibleDataSchemaVersion = 1,
};

const nl::FieldDescriptor WpanAntennaStatsEventFieldDescriptors[] =
{
    {
        NULL, offsetof(WpanAntennaStatsEvent, antennaStats) + offsetof(Schema::Nest::Trait::Network::TelemetryNetworkWpanTrait::PerAntennaStats_array, num), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeArray, 0), 1
    },
    {
        &Schema::Nest::Trait::Network::TelemetryNetworkWpanTrait::PerAntennaStats::FieldSchema, offsetof(WpanAntennaStatsEvent, antennaStats) + offsetof(Schema::Nest::Trait::Network::TelemetryNetworkWpanTrait::PerAntennaStats_array, buf), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeStructure, 0), 1
    },

    {
        NULL, offsetof(WpanAntennaStatsEvent, antSwitchCnt), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 2
    },

};

const nl::SchemaFieldDescriptor WpanAntennaStatsEvent::FieldSchema =
{
    .mNumFieldDescriptorElements = sizeof(WpanAntennaStatsEventFieldDescriptors)/sizeof(WpanAntennaStatsEventFieldDescriptors[0]),
    .mFields = WpanAntennaStatsEventFieldDescriptors,
    .mSize = sizeof(WpanAntennaStatsEvent)
};
const nl::Weave::Profiles::DataManagement::EventSchema WpanAntennaStatsEvent::Schema =
{
    .mProfileId = kWeaveProfileId,
    .mStructureType = 0x7,
    .mImportance = nl::Weave::Profiles::DataManagement::Debug,
    .mDataSchemaVersion = 3,
    .mMinCompatibleDataSchemaVersion = 1,
};

const nl::FieldDescriptor NetworkWpanTopoParentRespEventFieldDescriptors[] =
{
    {
        NULL, offsetof(NetworkWpanTopoParentRespEvent, rloc16), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt16, 0), 1
    },

    {
        NULL, offsetof(NetworkWpanTopoParentRespEvent, rssi), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt8, 0), 2
    },

    {
        NULL, offsetof(NetworkWpanTopoParentRespEvent, priority), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt8, 0), 3
    },

    {
        NULL, offsetof(NetworkWpanTopoParentRespEvent, extAddr), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeByteString, 0), 4
    },

    {
        NULL, offsetof(NetworkWpanTopoParentRespEvent, linkQuality3), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt8, 0), 5
    },

    {
        NULL, offsetof(NetworkWpanTopoParentRespEvent, linkQuality2), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt8, 0), 6
    },

    {
        NULL, offsetof(NetworkWpanTopoParentRespEvent, linkQuality1), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt8, 0), 7
    },

};

const nl::SchemaFieldDescriptor NetworkWpanTopoParentRespEvent::FieldSchema =
{
    .mNumFieldDescriptorElements = sizeof(NetworkWpanTopoParentRespEventFieldDescriptors)/sizeof(NetworkWpanTopoParentRespEventFieldDescriptors[0]),
    .mFields = NetworkWpanTopoParentRespEventFieldDescriptors,
    .mSize = sizeof(NetworkWpanTopoParentRespEvent)
};
const nl::Weave::Profiles::DataManagement::EventSchema NetworkWpanTopoParentRespEvent::Schema =
{
    .mProfileId = kWeaveProfileId,
    .mStructureType = 0x8,
    .mImportance = nl::Weave::Profiles::DataManagement::Debug,
    .mDataSchemaVersion = 3,
    .mMinCompatibleDataSchemaVersion = 1,
};

//
// Event Structs
//

const nl::FieldDescriptor ChannelUtilizationFieldDescriptors[] =
{
    {
        NULL, offsetof(ChannelUtilization, channel), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt8, 0), 1
    },

    {
        NULL, offsetof(ChannelUtilization, percentBusy), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt16, 0), 2
    },

};

const nl::SchemaFieldDescriptor ChannelUtilization::FieldSchema =
{
    .mNumFieldDescriptorElements = sizeof(ChannelUtilizationFieldDescriptors)/sizeof(ChannelUtilizationFieldDescriptors[0]),
    .mFields = ChannelUtilizationFieldDescriptors,
    .mSize = sizeof(ChannelUtilization)
};


const nl::FieldDescriptor PerAntennaStatsFieldDescriptors[] =
{
    {
        NULL, offsetof(PerAntennaStats, txSuccessCnt), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 1
    },

    {
        NULL, offsetof(PerAntennaStats, txFailCnt), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 2
    },

    {
        NULL, offsetof(PerAntennaStats, avgAckRssi), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt8, 0), 3
    },

};

const nl::SchemaFieldDescriptor PerAntennaStats::FieldSchema =
{
    .mNumFieldDescriptorElements = sizeof(PerAntennaStatsFieldDescriptors)/sizeof(PerAntennaStatsFieldDescriptors[0]),
    .mFields = PerAntennaStatsFieldDescriptors,
    .mSize = sizeof(PerAntennaStats)
};


const nl::FieldDescriptor TopoEntryFieldDescriptors[] =
{
    {
        NULL, offsetof(TopoEntry, extAddress), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeByteString, 0), 1
    },

    {
        NULL, offsetof(TopoEntry, rloc16), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt16, 0), 2
    },

    {
        NULL, offsetof(TopoEntry, linkQualityIn), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt8, 0), 3
    },

    {
        NULL, offsetof(TopoEntry, averageRssi), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt8, 0), 4
    },

    {
        NULL, offsetof(TopoEntry, age), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 5
    },

    {
        NULL, offsetof(TopoEntry, rxOnWhenIdle), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeBoolean, 0), 6
    },

    {
        NULL, offsetof(TopoEntry, fullFunction), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeBoolean, 0), 7
    },

    {
        NULL, offsetof(TopoEntry, secureDataRequest), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeBoolean, 0), 8
    },

    {
        NULL, offsetof(TopoEntry, fullNetworkData), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeBoolean, 0), 9
    },

    {
        NULL, offsetof(TopoEntry, lastRssi), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt8, 0), 10
    },

};

const nl::SchemaFieldDescriptor TopoEntry::FieldSchema =
{
    .mNumFieldDescriptorElements = sizeof(TopoEntryFieldDescriptors)/sizeof(TopoEntryFieldDescriptors[0]),
    .mFields = TopoEntryFieldDescriptors,
    .mSize = sizeof(TopoEntry)
};


const nl::FieldDescriptor ChildTableEntryFieldDescriptors[] =
{
    {
        &Schema::Nest::Trait::Network::TelemetryNetworkWpanTrait::TopoEntry::FieldSchema, offsetof(ChildTableEntry, topo), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeStructure, 0), 1
    },

    {
        NULL, offsetof(ChildTableEntry, timeout), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 2
    },

    {
        NULL, offsetof(ChildTableEntry, networkDataVersion), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt8, 0), 3
    },

};

const nl::SchemaFieldDescriptor ChildTableEntry::FieldSchema =
{
    .mNumFieldDescriptorElements = sizeof(ChildTableEntryFieldDescriptors)/sizeof(ChildTableEntryFieldDescriptors[0]),
    .mFields = ChildTableEntryFieldDescriptors,
    .mSize = sizeof(ChildTableEntry)
};


const nl::FieldDescriptor NeighborTableEntryFieldDescriptors[] =
{
    {
        &Schema::Nest::Trait::Network::TelemetryNetworkWpanTrait::TopoEntry::FieldSchema, offsetof(NeighborTableEntry, topo), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeStructure, 0), 1
    },

    {
        NULL, offsetof(NeighborTableEntry, linkFrameCounter), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 2
    },

    {
        NULL, offsetof(NeighborTableEntry, mleFrameCounter), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 3
    },

    {
        NULL, offsetof(NeighborTableEntry, isChild), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeBoolean, 0), 4
    },

};

const nl::SchemaFieldDescriptor NeighborTableEntry::FieldSchema =
{
    .mNumFieldDescriptorElements = sizeof(NeighborTableEntryFieldDescriptors)/sizeof(NeighborTableEntryFieldDescriptors[0]),
    .mFields = NeighborTableEntryFieldDescriptors,
    .mSize = sizeof(NeighborTableEntry)
};

} // namespace TelemetryNetworkWpanTrait
} // namespace Network
} // namespace Trait
} // namespace Nest
} // namespace Schema
