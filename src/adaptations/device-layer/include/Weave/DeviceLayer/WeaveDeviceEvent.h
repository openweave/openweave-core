/*
 *
 *    Copyright (c) 2018 Nest Labs, Inc.
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

#ifndef WEAVE_DEVICE_EVENT_H
#define WEAVE_DEVICE_EVENT_H

#include <esp_event.h>

namespace nl {
namespace Weave {
namespace DeviceLayer {

enum ConnectivityChange
{
    kConnectivity_Established = 0,
    kConnectivity_Lost,
    kConnectivity_NoChange
};

typedef void (*AsyncWorkFunct)(intptr_t arg);

struct WeaveDeviceEvent
{
    enum
    {
        kPublicEventRange_Start                                 = 0x0000,
        kPublicEventRange_End                                   = 0x7FFF,

        kInternalEventRange_Start                               = 0x8000,
        kInternalEventRange_End                                 = 0xFFFF,
    };

    enum
    {
        kEventType_NoOp                                         = kPublicEventRange_Start,
        kEventType_ESPSystemEvent,
        kEventType_WeaveSystemLayerEvent,
        kEventType_CallWorkFunct,
        kEventType_WiFiConnectivityChange,
        kEventType_InternetConnectivityChange,
        kEventType_ServiceTunnelStateChange,
        kEventType_ServiceConnectivityChange,
        kEventType_ServiceSubscriptionStateChange,
        kEventType_FabricMembershipChange,
        kEventType_ServiceProvisioningChange,
        kEventType_AccountPairingChange,
        kEventType_TimeSyncChange,
        kEventType_SessionEstablished,
        kEventType_WoBLEConnectionEstablished,

        kInternalEventType_WoBLESubscribe                       = kInternalEventRange_Start,
        kInternalEventType_WoBLEUnsubscribe,
        kInternalEventType_WoBLEWriteReceived,
        kInternalEventType_WoBLEIndicateConfirm,
        kInternalEventType_WoBLEConnectionError,
    };

    uint16_t Type;

    union
    {
        system_event_t ESPSystemEvent;
        struct
        {
            ::nl::Weave::System::EventType Type;
            ::nl::Weave::System::Object * Target;
            uintptr_t Argument;
        } WeaveSystemLayerEvent;
        struct
        {
            AsyncWorkFunct WorkFunct;
            intptr_t Arg;
        } CallWorkFunct;
        struct
        {
            ConnectivityChange Result;
        } WiFiConnectivityChange;
        struct
        {
            ConnectivityChange IPv4;
            ConnectivityChange IPv6;
        } InternetConnectivityChange;
        struct
        {
            ConnectivityChange Result;
            bool IsRestricted;
        } ServiceTunnelStateChange;
        struct
        {
            ConnectivityChange Result;
        } ServiceConnectivityChange;
        struct
        {
            ConnectivityChange Result;
        } ServiceSubscriptionStateChange;
        struct
        {
            bool IsMemberOfFabric;
        } FabricMembershipChange;
        struct
        {
            bool IsServiceProvisioned;
            bool ServiceConfigUpdated;
        } ServiceProvisioningChange;
        struct
        {
            bool IsPairedToAccount;
        } AccountPairingChange;
        struct
        {
            bool IsTimeSynchronized;
        } TimeSyncChange;
        struct
        {
            uint64_t PeerNodeId;
            uint16_t SessionKeyId;
            uint8_t EncType;
            ::nl::Weave::WeaveAuthMode AuthMode;
            bool IsCommissioner;
        } SessionEstablished;
        struct
        {
            BLE_CONNECTION_OBJECT ConId;
        } WoBLESubscribe;
        struct
        {
            BLE_CONNECTION_OBJECT ConId;
        } WoBLEUnsubscribe;
        struct
        {
            BLE_CONNECTION_OBJECT ConId;
            PacketBuffer * Data;
        } WoBLEWriteReceived;
        struct
        {
            BLE_CONNECTION_OBJECT ConId;
        } WoBLEIndicateConfirm;
        struct
        {
            BLE_CONNECTION_OBJECT ConId;
            WEAVE_ERROR Reason;
        } WoBLEConnectionError;
    };

    static bool IsInternalEvent(uint16_t eventType);
};

inline bool WeaveDeviceEvent::IsInternalEvent(uint16_t eventType)
{
    return eventType >= kInternalEventRange_Start && eventType < kInternalEventRange_End;
}


} // namespace DeviceLayer
} // namespace Weave
} // namespace nl

#endif // WEAVE_DEVICE_EVENT_H
