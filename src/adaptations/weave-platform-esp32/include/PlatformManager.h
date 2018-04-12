#ifndef PLATFORM_MANAGER_H
#define PLATFORM_MANAGER_H

#include <esp_event.h>

namespace WeavePlatform {

class ConnectivityManager;
struct WeavePlatformEvent;

enum ConnectivityChange
{
    kConnectivity_Established = 0,
    kConnectivity_Lost,
    kConnectivity_NoChange
};

class PlatformManager
{
public:

    WEAVE_ERROR InitLwIPCoreLock();
    WEAVE_ERROR InitWeaveStack();

    typedef void (*EventHandlerFunct)(const WeavePlatformEvent * event, intptr_t arg);
    WEAVE_ERROR AddEventHandler(EventHandlerFunct handler, intptr_t arg = 0);
    void RemoveEventHandler(EventHandlerFunct handler, intptr_t arg = 0);

    typedef void (*AsyncWorkFunct)(intptr_t arg);
    void ScheduleWork(AsyncWorkFunct workFunct, intptr_t arg = 0);

    void RunEventLoop();

    static esp_err_t HandleESPSystemEvent(void * ctx, system_event_t * event);

private:

    // NOTE: These members are for internal use by the following friend classes.

    friend class ConnectivityManager;
    friend nl::Weave::System::Error nl::Weave::System::Platform::Layer::DispatchEvent(nl::Weave::System::Layer & aLayer, void * aContext, const ::WeavePlatform::WeavePlatformEvent * aEvent);

    void PostEvent(const WeavePlatformEvent * event);

private:

    WEAVE_ERROR InitWeaveEventQueue();
    void DispatchEvent(const WeavePlatformEvent * event);
};

struct WeavePlatformEvent
{
    enum
    {
        kEventType_ESPSystemEvent                               = 0,
        kEventType_WeaveSystemLayerEvent,
        kEventType_CallWorkFunct,
        kEventType_WiFiConnectivityChange,
        kEventType_InternetConnectivityChange,
        kEventType_ServiceConnectivityChange,
        kEventType_FabricMembershipChange,
        kEventType_ServiceProvisioningChange,
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
            PlatformManager::AsyncWorkFunct WorkFunct;
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
    };
};

} // namespace WeavePlatform

#endif // PLATFORM_MANAGER_H
