#ifndef WEAVE_PLATFORM_EVENT_H
#define WEAVE_PLATFORM_EVENT_H

#include <esp_event.h>

namespace WeavePlatform {

enum ConnectivityChange
{
    kConnectivity_Established = 0,
    kConnectivity_Lost,
    kConnectivity_NoChange
};

typedef void (*AsyncWorkFunct)(intptr_t arg);

struct WeavePlatformEvent
{
    enum
    {
        kEventType_NoOp                                         = 0,
        kEventType_ESPSystemEvent,
        kEventType_WeaveSystemLayerEvent,
        kEventType_CallWorkFunct,
        kEventType_WiFiConnectivityChange,
        kEventType_InternetConnectivityChange,
        kEventType_ServiceConnectivityChange,
        kEventType_FabricMembershipChange,
        kEventType_ServiceProvisioningChange,
        kEventType_AccountPairingChange,
        kEventType_TimeSyncChange,
    };

    uint16_t Type;

    union
    {
        system_event_t ESPSystemEvent;
        struct
        {
            nl::Weave::System::EventType Type;
            nl::Weave::System::Object * Target;
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
        } ServiceConnectivityChange;
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
    };
};

} // namespace WeavePlatform

#endif // WEAVE_PLATFORM_EVENT_H
