
/**
 *    Copyright (c) 2019 Nest Labs, Inc.
 *    All rights reserved.
 *
 *    THIS FILE IS GENERATED. DO NOT MODIFY.
 *
 *    SOURCE TEMPLATE: trait.cpp.h
 *    SOURCE PROTO: nest/trait/network/telemetry_network_wpan_trait.proto
 *
 */
#ifndef _NEST_TRAIT_NETWORK__TELEMETRY_NETWORK_WPAN_TRAIT_H_
#define _NEST_TRAIT_NETWORK__TELEMETRY_NETWORK_WPAN_TRAIT_H_

#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Support/SerializationUtils.h>



namespace Schema {
namespace Nest {
namespace Trait {
namespace Network {
namespace TelemetryNetworkWpanTrait {

extern const nl::Weave::Profiles::DataManagement::TraitSchemaEngine TraitSchema;

enum {
      kWeaveProfileId = (0x235aU << 16) | 0x603U
};

//
// Event Structs
//

struct ChannelUtilization
{
    uint8_t channel;
    uint16_t percentBusy;

    static const nl::SchemaFieldDescriptor FieldSchema;

};

struct ChannelUtilization_array {
    uint32_t num;
    ChannelUtilization *buf;
};

struct PerAntennaStats
{
    uint32_t txSuccessCnt;
    uint32_t txFailCnt;
    int8_t avgAckRssi;

    static const nl::SchemaFieldDescriptor FieldSchema;

};

struct PerAntennaStats_array {
    uint32_t num;
    PerAntennaStats *buf;
};

struct TopoEntry
{
    nl::SerializedByteString extAddress;
    uint16_t rloc16;
    uint8_t linkQualityIn;
    int8_t averageRssi;
    uint32_t age;
    bool rxOnWhenIdle;
    bool fullFunction;
    bool secureDataRequest;
    bool fullNetworkData;
    int8_t lastRssi;

    static const nl::SchemaFieldDescriptor FieldSchema;

};

struct TopoEntry_array {
    uint32_t num;
    TopoEntry *buf;
};

struct ChildTableEntry
{
    Schema::Nest::Trait::Network::TelemetryNetworkWpanTrait::TopoEntry topo;
    uint32_t timeout;
    uint8_t networkDataVersion;

    static const nl::SchemaFieldDescriptor FieldSchema;

};

struct ChildTableEntry_array {
    uint32_t num;
    ChildTableEntry *buf;
};

struct NeighborTableEntry
{
    Schema::Nest::Trait::Network::TelemetryNetworkWpanTrait::TopoEntry topo;
    uint32_t linkFrameCounter;
    uint32_t mleFrameCounter;
    bool isChild;

    static const nl::SchemaFieldDescriptor FieldSchema;

};

struct NeighborTableEntry_array {
    uint32_t num;
    NeighborTableEntry *buf;
};

//
// Events
//
struct WpanParentLinkEvent
{
    int8_t rssi;
    uint16_t unicastCcaThresholdFailures;
    uint16_t unicastMacRetryCount;

    static const nl::SchemaFieldDescriptor FieldSchema;

    // Statically-known Event Struct Attributes:
    enum {
            kWeaveProfileId = (0x235aU << 16) | 0x603U,
        kEventTypeId = 0x1U
    };

    static const nl::Weave::Profiles::DataManagement::EventSchema Schema;
};

struct WpanParentLinkEvent_array {
    uint32_t num;
    WpanParentLinkEvent *buf;
};


struct NetworkWpanStatsEvent
{
    int32_t phyRx;
    int32_t phyTx;
    int32_t macUnicastRx;
    int32_t macUnicastTx;
    int32_t macBroadcastRx;
    int32_t macBroadcastTx;
    int32_t macTxAckReq;
    int32_t macTxNoAckReq;
    int32_t macTxAcked;
    int32_t macTxData;
    int32_t macTxDataPoll;
    int32_t macTxBeacon;
    int32_t macTxBeaconReq;
    int32_t macTxOtherPkt;
    int32_t macTxRetry;
    int32_t macRxData;
    int32_t macRxDataPoll;
    int32_t macRxBeacon;
    int32_t macRxBeaconReq;
    int32_t macRxOtherPkt;
    int32_t macRxFilterWhitelist;
    int32_t macRxFilterDestAddr;
    int32_t macTxFailCca;
    int32_t macRxFailDecrypt;
    int32_t macRxFailNoFrame;
    int32_t macRxFailUnknownNeighbor;
    int32_t macRxFailInvalidSrcAddr;
    int32_t macRxFailFcs;
    int32_t macRxFailOther;
    int32_t ipTxSuccess;
    int32_t ipRxSuccess;
    int32_t ipTxFailure;
    int32_t ipRxFailure;
    uint8_t nodeType;
    uint8_t channel;
    int8_t radioTxPower;
    int32_t threadType;
    uint32_t ncpTxTotalTime;
    uint32_t ncpRxTotalTime;
    uint16_t macCcaFailRate;

    static const nl::SchemaFieldDescriptor FieldSchema;

    // Statically-known Event Struct Attributes:
    enum {
            kWeaveProfileId = (0x235aU << 16) | 0x603U,
        kEventTypeId = 0x2U
    };

    static const nl::Weave::Profiles::DataManagement::EventSchema Schema;
};

struct NetworkWpanStatsEvent_array {
    uint32_t num;
    NetworkWpanStatsEvent *buf;
};


struct NetworkWpanTopoMinimalEvent
{
    uint16_t rloc16;
    uint8_t routerId;
    uint8_t leaderRouterId;
    int8_t parentAverageRssi;
    int8_t parentLastRssi;
    uint32_t partitionId;
    nl::SerializedByteString extAddress;
    int8_t instantRssi;

    static const nl::SchemaFieldDescriptor FieldSchema;

    // Statically-known Event Struct Attributes:
    enum {
            kWeaveProfileId = (0x235aU << 16) | 0x603U,
        kEventTypeId = 0x3U
    };

    static const nl::Weave::Profiles::DataManagement::EventSchema Schema;
};

struct NetworkWpanTopoMinimalEvent_array {
    uint32_t num;
    NetworkWpanTopoMinimalEvent *buf;
};


struct NetworkWpanTopoFullEvent
{
    uint16_t rloc16;
    uint8_t routerId;
    uint8_t leaderRouterId;
    nl::SerializedByteString extAddress;
    nl::SerializedByteString leaderAddress;
    uint8_t leaderWeight;
    uint8_t leaderLocalWeight;
    nl::SerializedByteString networkData;
    uint8_t networkDataVersion;
    nl::SerializedByteString stableNetworkData;
    uint8_t stableNetworkDataVersion;
    uint8_t preferredRouterId;
    uint32_t partitionId;
    Schema::Nest::Trait::Network::TelemetryNetworkWpanTrait::ChildTableEntry_array deprecatedChildTable;
    Schema::Nest::Trait::Network::TelemetryNetworkWpanTrait::NeighborTableEntry_array deprecatedNeighborTable;
    uint16_t childTableSize;
    uint16_t neighborTableSize;
    int8_t instantRssi;

    static const nl::SchemaFieldDescriptor FieldSchema;

    // Statically-known Event Struct Attributes:
    enum {
            kWeaveProfileId = (0x235aU << 16) | 0x603U,
        kEventTypeId = 0x4U
    };

    static const nl::Weave::Profiles::DataManagement::EventSchema Schema;
};

struct NetworkWpanTopoFullEvent_array {
    uint32_t num;
    NetworkWpanTopoFullEvent *buf;
};


struct TopoEntryEvent
{
    nl::SerializedByteString extAddress;
    uint16_t rloc16;
    uint8_t linkQualityIn;
    int8_t averageRssi;
    uint32_t age;
    bool rxOnWhenIdle;
    bool fullFunction;
    bool secureDataRequest;
    bool fullNetworkData;
    int8_t lastRssi;
    uint32_t linkFrameCounter;
    uint32_t mleFrameCounter;
    bool isChild;
    uint32_t timeout;
    void SetTimeoutNull(void);
    void SetTimeoutPresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsTimeoutPresent(void);
#endif
    uint8_t networkDataVersion;
    void SetNetworkDataVersionNull(void);
    void SetNetworkDataVersionPresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsNetworkDataVersionPresent(void);
#endif
    uint16_t macFrameErrorRate;
    uint16_t ipMessageErrorRate;
    uint8_t __nullified_fields__[2/8 + 1];

    static const nl::SchemaFieldDescriptor FieldSchema;

    // Statically-known Event Struct Attributes:
    enum {
            kWeaveProfileId = (0x235aU << 16) | 0x603U,
        kEventTypeId = 0x5U
    };

    static const nl::Weave::Profiles::DataManagement::EventSchema Schema;
};

struct TopoEntryEvent_array {
    uint32_t num;
    TopoEntryEvent *buf;
};

inline void TopoEntryEvent::SetTimeoutNull(void)
{
    SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 0);
}

inline void TopoEntryEvent::SetTimeoutPresent(void)
{
    CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 0);
}

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool TopoEntryEvent::IsTimeoutPresent(void)
{
    return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 0));
}
#endif
inline void TopoEntryEvent::SetNetworkDataVersionNull(void)
{
    SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 1);
}

inline void TopoEntryEvent::SetNetworkDataVersionPresent(void)
{
    CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 1);
}

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool TopoEntryEvent::IsNetworkDataVersionPresent(void)
{
    return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 1));
}
#endif

struct WpanChannelmonStatsEvent
{
    Schema::Nest::Trait::Network::TelemetryNetworkWpanTrait::ChannelUtilization_array channels;
    uint32_t samples;

    static const nl::SchemaFieldDescriptor FieldSchema;

    // Statically-known Event Struct Attributes:
    enum {
            kWeaveProfileId = (0x235aU << 16) | 0x603U,
        kEventTypeId = 0x6U
    };

    static const nl::Weave::Profiles::DataManagement::EventSchema Schema;
};

struct WpanChannelmonStatsEvent_array {
    uint32_t num;
    WpanChannelmonStatsEvent *buf;
};


struct WpanAntennaStatsEvent
{
    Schema::Nest::Trait::Network::TelemetryNetworkWpanTrait::PerAntennaStats_array antennaStats;
    uint32_t antSwitchCnt;

    static const nl::SchemaFieldDescriptor FieldSchema;

    // Statically-known Event Struct Attributes:
    enum {
            kWeaveProfileId = (0x235aU << 16) | 0x603U,
        kEventTypeId = 0x7U
    };

    static const nl::Weave::Profiles::DataManagement::EventSchema Schema;
};

struct WpanAntennaStatsEvent_array {
    uint32_t num;
    WpanAntennaStatsEvent *buf;
};


struct NetworkWpanTopoParentRespEvent
{
    uint16_t rloc16;
    int8_t rssi;
    int8_t priority;
    nl::SerializedByteString extAddr;
    uint8_t linkQuality3;
    uint8_t linkQuality2;
    uint8_t linkQuality1;

    static const nl::SchemaFieldDescriptor FieldSchema;

    // Statically-known Event Struct Attributes:
    enum {
            kWeaveProfileId = (0x235aU << 16) | 0x603U,
        kEventTypeId = 0x8U
    };

    static const nl::Weave::Profiles::DataManagement::EventSchema Schema;
};

struct NetworkWpanTopoParentRespEvent_array {
    uint32_t num;
    NetworkWpanTopoParentRespEvent *buf;
};


//
// Enums
//

enum NodeType {
    NODE_TYPE_ROUTER = 1,
    NODE_TYPE_END = 2,
    NODE_TYPE_SLEEPY_END = 3,
    NODE_TYPE_MINIMAL_END = 4,
    NODE_TYPE_OFFLINE = 5,
    NODE_TYPE_DISABLED = 6,
    NODE_TYPE_DETACHED = 7,
    NODE_TYPE_NL_LURKER = 16,
    NODE_TYPE_COMMISSIONER = 32,
    NODE_TYPE_LEADER = 64,
};

enum ThreadType {
    THREAD_TYPE_SILABS = 1,
    THREAD_TYPE_OPENTHREAD = 2,
};

} // namespace TelemetryNetworkWpanTrait
} // namespace Network
} // namespace Trait
} // namespace Nest
} // namespace Schema
#endif // _NEST_TRAIT_NETWORK__TELEMETRY_NETWORK_WPAN_TRAIT_H_
