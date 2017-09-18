/*
 *
 *    Copyright (c) 2017 Nest Labs, Inc.
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
/**
 * THIS FILE IS GENERATED. DO NOT MODIFY.
 *
 * SOURCE TEMPLATE: Trait.cpp.tpl
 * SOURCE PROTO: maldives_prototype/trait/flintstone/nlnetworktelemetry_wifi_trait.proto
 *
 */

#include "NetworkWiFiTelemetryTrait.h"

namespace Schema {
namespace Maldives_prototype {
namespace Trait {
namespace Flintstone {
namespace NetworkWiFiTelemetryTrait {

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
#if (TDM_DICTIONARY_SUPPORT)
            NULL,
#endif
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

    const nl::FieldDescriptor NetworkWiFiStatsEventFieldDescriptors[] =
    {
        {
            NULL, offsetof(NetworkWiFiStatsEvent, rssi), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 1
        },
        {
            NULL, offsetof(NetworkWiFiStatsEvent, bcnRecvd), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 2
        },
        {
            NULL, offsetof(NetworkWiFiStatsEvent, bcnLost), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 3
        },
        {
            NULL, offsetof(NetworkWiFiStatsEvent, pktMCastRX), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 4
        },
        {
            NULL, offsetof(NetworkWiFiStatsEvent, pktUCastRX), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 5
        },
        {
            NULL, offsetof(NetworkWiFiStatsEvent, currRXRate), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 6
        },
        {
            NULL, offsetof(NetworkWiFiStatsEvent, currTXRate), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 7
        },
        {
            NULL, offsetof(NetworkWiFiStatsEvent, sleepTimePercent), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 8
        },
        {
            NULL, offsetof(NetworkWiFiStatsEvent, bssid), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 9
        },
        {
            NULL, offsetof(NetworkWiFiStatsEvent, freq), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 10
        },
        {
            NULL, offsetof(NetworkWiFiStatsEvent, numOfAP), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 11
        },
    };

    const nl::SchemaFieldDescriptor NetworkWiFiStatsEventFieldSchema =
    {
        .mNumFieldDescriptorElements = 11,
        .mFields = NetworkWiFiStatsEventFieldDescriptors,
        .mSize = sizeof(NetworkWiFiStatsEvent),
    };

    const nl::Weave::Profiles::DataManagement::EventSchema NetworkWiFiStatsEventSchema =
    {
        .mProfileId = kWeaveProfileId,
        .mStructureType = 0x1,
        .mImportance = nl::Weave::Profiles::DataManagement::Production,
        .mDataSchemaVersion = 1,
        .mMinCompatibleDataSchemaVersion = 1,
    };

    const nl::FieldDescriptor NetworkWiFiDeauthEventFieldDescriptors[] =
    {
        {
            NULL, offsetof(NetworkWiFiDeauthEvent, reason), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 1
        },
    };

    const nl::SchemaFieldDescriptor NetworkWiFiDeauthEventFieldSchema =
    {
        .mNumFieldDescriptorElements = 1,
        .mFields = NetworkWiFiDeauthEventFieldDescriptors,
        .mSize = sizeof(NetworkWiFiDeauthEvent),
    };

    const nl::Weave::Profiles::DataManagement::EventSchema NetworkWiFiDeauthEventSchema =
    {
        .mProfileId = kWeaveProfileId,
        .mStructureType = 0x2,
        .mImportance = nl::Weave::Profiles::DataManagement::Production,
        .mDataSchemaVersion = 1,
        .mMinCompatibleDataSchemaVersion = 1,
    };

    const nl::FieldDescriptor NetworkWiFiInvalidKeyEventFieldDescriptors[] =
    {
        {
            NULL, offsetof(NetworkWiFiInvalidKeyEvent, reason), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 1
        },
    };

    const nl::SchemaFieldDescriptor NetworkWiFiInvalidKeyEventFieldSchema =
    {
        .mNumFieldDescriptorElements = 1,
        .mFields = NetworkWiFiInvalidKeyEventFieldDescriptors,
        .mSize = sizeof(NetworkWiFiInvalidKeyEvent),
    };

    const nl::Weave::Profiles::DataManagement::EventSchema NetworkWiFiInvalidKeyEventSchema =
    {
        .mProfileId = kWeaveProfileId,
        .mStructureType = 0x3,
        .mImportance = nl::Weave::Profiles::DataManagement::Production,
        .mDataSchemaVersion = 1,
        .mMinCompatibleDataSchemaVersion = 1,
    };

    const nl::FieldDescriptor NetworkWiFiDHCPFailureEventFieldDescriptors[] =
    {
        {
            NULL, offsetof(NetworkWiFiDHCPFailureEvent, reason), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 1
        },
    };

    const nl::SchemaFieldDescriptor NetworkWiFiDHCPFailureEventFieldSchema =
    {
        .mNumFieldDescriptorElements = 1,
        .mFields = NetworkWiFiDHCPFailureEventFieldDescriptors,
        .mSize = sizeof(NetworkWiFiDHCPFailureEvent),
    };

    const nl::Weave::Profiles::DataManagement::EventSchema NetworkWiFiDHCPFailureEventSchema =
    {
        .mProfileId = kWeaveProfileId,
        .mStructureType = 0x4,
        .mImportance = nl::Weave::Profiles::DataManagement::Production,
        .mDataSchemaVersion = 1,
        .mMinCompatibleDataSchemaVersion = 1,
    };

} // namespace NetworkWiFiTelemetryTrait
}
}
}
}
