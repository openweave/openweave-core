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
 * SOURCE TEMPLATE: Trait.h.tpl
 * SOURCE PROTO: maldives_prototype/trait/flintstone/nlnetworktelemetry_wifi_trait.proto
 *
 */

#ifndef _MALDIVES_PROTOTYPE_TRAIT_FLINTSTONE__NETWORK_WI_FI_TELEMETRY_TRAIT_H_
#define _MALDIVES_PROTOTYPE_TRAIT_FLINTSTONE__NETWORK_WI_FI_TELEMETRY_TRAIT_H_

// We want and assume the default managed namespace is Current and that is, explicitly, the managed namespace this code desires.
#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Support/SerializationUtils.h>

namespace Schema {
namespace Maldives_prototype {
namespace Trait {
namespace Flintstone {
namespace NetworkWiFiTelemetryTrait {

    extern const nl::Weave::Profiles::DataManagement::TraitSchemaEngine TraitSchema;

    enum {
        kWeaveProfileId = (0x235aU << 16) | 0x401U
    };

    enum {
        kPropertyHandle_Root = 1,

        //---------------------------------------------------------------------------------------------------------------------------//
        //  Name                                IDL Type                            TLV Type           Optional?       Nullable?     //
        //---------------------------------------------------------------------------------------------------------------------------//

    };

    struct NetworkWiFiStatsEvent
    {
        uint32_t rssi;
        uint32_t bcnRecvd;
        uint32_t bcnLost;
        uint32_t pktMCastRX;
        uint32_t pktUCastRX;
        uint32_t currRXRate;
        uint32_t currTXRate;
        uint32_t sleepTimePercent;
        uint32_t bssid;
        uint32_t freq;
        uint32_t numOfAP;
    };

    extern const nl::Weave::Profiles::DataManagement::EventSchema NetworkWiFiStatsEventSchema;
    extern const nl::SchemaFieldDescriptor NetworkWiFiStatsEventFieldSchema;

    inline WEAVE_ERROR LogNetworkWiFiStatsEvent(NetworkWiFiStatsEvent *aEvent, nl::Weave::Profiles::DataManagement::ImportanceType aImportance)
    {
        nl::StructureSchemaPointerPair structureSchemaPair = {(void *)aEvent, &NetworkWiFiStatsEventFieldSchema};
        nl::Weave::Profiles::DataManagement::LogEvent(NetworkWiFiStatsEventSchema, nl::SerializedDataToTLVWriterHelper, (void *)&structureSchemaPair);
        return WEAVE_NO_ERROR;
    }

    struct NetworkWiFiDeauthEvent
    {
        uint32_t reason;
    };

    extern const nl::Weave::Profiles::DataManagement::EventSchema NetworkWiFiDeauthEventSchema;
    extern const nl::SchemaFieldDescriptor NetworkWiFiDeauthEventFieldSchema;

    inline WEAVE_ERROR LogNetworkWiFiDeauthEvent(NetworkWiFiDeauthEvent *aEvent, nl::Weave::Profiles::DataManagement::ImportanceType aImportance)
    {
        nl::StructureSchemaPointerPair structureSchemaPair = {(void *)aEvent, &NetworkWiFiDeauthEventFieldSchema};
        nl::Weave::Profiles::DataManagement::LogEvent(NetworkWiFiDeauthEventSchema, nl::SerializedDataToTLVWriterHelper, (void *)&structureSchemaPair);
        return WEAVE_NO_ERROR;
    }

    struct NetworkWiFiInvalidKeyEvent
    {
        uint32_t reason;
    };

    extern const nl::Weave::Profiles::DataManagement::EventSchema NetworkWiFiInvalidKeyEventSchema;
    extern const nl::SchemaFieldDescriptor NetworkWiFiInvalidKeyEventFieldSchema;

    inline WEAVE_ERROR LogNetworkWiFiInvalidKeyEvent(NetworkWiFiInvalidKeyEvent *aEvent, nl::Weave::Profiles::DataManagement::ImportanceType aImportance)
    {
        nl::StructureSchemaPointerPair structureSchemaPair = {(void *)aEvent, &NetworkWiFiInvalidKeyEventFieldSchema};
        nl::Weave::Profiles::DataManagement::LogEvent(NetworkWiFiInvalidKeyEventSchema, nl::SerializedDataToTLVWriterHelper, (void *)&structureSchemaPair);
        return WEAVE_NO_ERROR;
    }

    struct NetworkWiFiDHCPFailureEvent
    {
        uint32_t reason;
    };

    extern const nl::Weave::Profiles::DataManagement::EventSchema NetworkWiFiDHCPFailureEventSchema;
    extern const nl::SchemaFieldDescriptor NetworkWiFiDHCPFailureEventFieldSchema;

    inline WEAVE_ERROR LogNetworkWiFiDHCPFailureEvent(NetworkWiFiDHCPFailureEvent *aEvent, nl::Weave::Profiles::DataManagement::ImportanceType aImportance)
    {
        nl::StructureSchemaPointerPair structureSchemaPair = {(void *)aEvent, &NetworkWiFiDHCPFailureEventFieldSchema};
        nl::Weave::Profiles::DataManagement::LogEvent(NetworkWiFiDHCPFailureEventSchema, nl::SerializedDataToTLVWriterHelper, (void *)&structureSchemaPair);
        return WEAVE_NO_ERROR;
    }

} // namespace NetworkWiFiTelemetryTrait
}
}
}

}
#endif // _MALDIVES_PROTOTYPE_TRAIT_FLINTSTONE__NETWORK_WI_FI_TELEMETRY_TRAIT_H_
