
/**
 *    Copyright (c) 2019 Nest Labs, Inc.
 *    All rights reserved.
 *
 *    THIS FILE IS GENERATED. DO NOT MODIFY.
 *
 *    SOURCE TEMPLATE: trait.cpp
 *    SOURCE PROTO: nest/trait/network/telemetry_network_wifi_trait.proto
 *
 */

#include <nest/trait/network/TelemetryNetworkWifiTrait.h>

namespace Schema {
namespace Nest {
namespace Trait {
namespace Network {
namespace TelemetryNetworkWifiTrait {

using namespace ::nl::Weave::Profiles::DataManagement;

//
// Property Table
//

const TraitSchemaEngine::PropertyInfo PropertyMap[] = {
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

const nl::FieldDescriptor NetworkWiFiStatsEventFieldDescriptors[] =
{
    {
        NULL, offsetof(NetworkWiFiStatsEvent, rssi), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt8, 0), 1
    },

    {
        NULL, offsetof(NetworkWiFiStatsEvent, bcnRecvd), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 2
    },

    {
        NULL, offsetof(NetworkWiFiStatsEvent, bcnLost), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 3
    },

    {
        NULL, offsetof(NetworkWiFiStatsEvent, pktMcastRx), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 4
    },

    {
        NULL, offsetof(NetworkWiFiStatsEvent, pktUcastRx), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 5
    },

    {
        NULL, offsetof(NetworkWiFiStatsEvent, currRxRate), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 6
    },

    {
        NULL, offsetof(NetworkWiFiStatsEvent, currTxRate), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 7
    },

    {
        NULL, offsetof(NetworkWiFiStatsEvent, sleepTimePercent), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt8, 0), 8
    },

    {
        NULL, offsetof(NetworkWiFiStatsEvent, bssid), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt16, 0), 9
    },

    {
        NULL, offsetof(NetworkWiFiStatsEvent, freq), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 10
    },

    {
        NULL, offsetof(NetworkWiFiStatsEvent, numOfAp), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 11
    },

};

const nl::SchemaFieldDescriptor NetworkWiFiStatsEvent::FieldSchema =
{
    .mNumFieldDescriptorElements = sizeof(NetworkWiFiStatsEventFieldDescriptors)/sizeof(NetworkWiFiStatsEventFieldDescriptors[0]),
    .mFields = NetworkWiFiStatsEventFieldDescriptors,
    .mSize = sizeof(NetworkWiFiStatsEvent)
};
const nl::Weave::Profiles::DataManagement::EventSchema NetworkWiFiStatsEvent::Schema =
{
    .mProfileId = kWeaveProfileId,
    .mStructureType = 0x1,
    .mImportance = nl::Weave::Profiles::DataManagement::Debug,
    .mDataSchemaVersion = 2,
    .mMinCompatibleDataSchemaVersion = 1,
};

const nl::FieldDescriptor NetworkWiFiDeauthEventFieldDescriptors[] =
{
    {
        NULL, offsetof(NetworkWiFiDeauthEvent, reason), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 1
    },

};

const nl::SchemaFieldDescriptor NetworkWiFiDeauthEvent::FieldSchema =
{
    .mNumFieldDescriptorElements = sizeof(NetworkWiFiDeauthEventFieldDescriptors)/sizeof(NetworkWiFiDeauthEventFieldDescriptors[0]),
    .mFields = NetworkWiFiDeauthEventFieldDescriptors,
    .mSize = sizeof(NetworkWiFiDeauthEvent)
};
const nl::Weave::Profiles::DataManagement::EventSchema NetworkWiFiDeauthEvent::Schema =
{
    .mProfileId = kWeaveProfileId,
    .mStructureType = 0x2,
    .mImportance = nl::Weave::Profiles::DataManagement::Debug,
    .mDataSchemaVersion = 2,
    .mMinCompatibleDataSchemaVersion = 1,
};

const nl::FieldDescriptor NetworkWiFiInvalidKeyEventFieldDescriptors[] =
{
    {
        NULL, offsetof(NetworkWiFiInvalidKeyEvent, reason), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 1
    },

};

const nl::SchemaFieldDescriptor NetworkWiFiInvalidKeyEvent::FieldSchema =
{
    .mNumFieldDescriptorElements = sizeof(NetworkWiFiInvalidKeyEventFieldDescriptors)/sizeof(NetworkWiFiInvalidKeyEventFieldDescriptors[0]),
    .mFields = NetworkWiFiInvalidKeyEventFieldDescriptors,
    .mSize = sizeof(NetworkWiFiInvalidKeyEvent)
};
const nl::Weave::Profiles::DataManagement::EventSchema NetworkWiFiInvalidKeyEvent::Schema =
{
    .mProfileId = kWeaveProfileId,
    .mStructureType = 0x3,
    .mImportance = nl::Weave::Profiles::DataManagement::Debug,
    .mDataSchemaVersion = 2,
    .mMinCompatibleDataSchemaVersion = 1,
};

const nl::FieldDescriptor NetworkWiFiConnectionStatusChangeEventFieldDescriptors[] =
{
    {
        NULL, offsetof(NetworkWiFiConnectionStatusChangeEvent, isConnected), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeBoolean, 0), 1
    },

};

const nl::SchemaFieldDescriptor NetworkWiFiConnectionStatusChangeEvent::FieldSchema =
{
    .mNumFieldDescriptorElements = sizeof(NetworkWiFiConnectionStatusChangeEventFieldDescriptors)/sizeof(NetworkWiFiConnectionStatusChangeEventFieldDescriptors[0]),
    .mFields = NetworkWiFiConnectionStatusChangeEventFieldDescriptors,
    .mSize = sizeof(NetworkWiFiConnectionStatusChangeEvent)
};
const nl::Weave::Profiles::DataManagement::EventSchema NetworkWiFiConnectionStatusChangeEvent::Schema =
{
    .mProfileId = kWeaveProfileId,
    .mStructureType = 0x4,
    .mImportance = nl::Weave::Profiles::DataManagement::Debug,
    .mDataSchemaVersion = 2,
    .mMinCompatibleDataSchemaVersion = 1,
};

} // namespace TelemetryNetworkWifiTrait
} // namespace Network
} // namespace Trait
} // namespace Nest
} // namespace Schema
