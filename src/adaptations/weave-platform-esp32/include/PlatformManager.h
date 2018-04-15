#ifndef PLATFORM_MANAGER_H
#define PLATFORM_MANAGER_H

#include <WeavePlatformEvent.h>

namespace WeavePlatform {

class ConnectivityManager;
struct WeavePlatformEvent;

namespace Internal {
class FabricProvisioningServer;
class ServiceProvisioningServer;
} // namespace Internal

class PlatformManager
{
public:

    WEAVE_ERROR InitLocks();
    WEAVE_ERROR InitWeaveStack();

    typedef void (*EventHandlerFunct)(const WeavePlatformEvent * event, intptr_t arg);
    WEAVE_ERROR AddEventHandler(EventHandlerFunct handler, intptr_t arg = 0);
    void RemoveEventHandler(EventHandlerFunct handler, intptr_t arg = 0);

    void ScheduleWork(AsyncWorkFunct workFunct, intptr_t arg = 0);

    void RunEventLoop();
    WEAVE_ERROR StartEventLoopTask();

    void LockWeaveStack();
    void UnlockWeaveStack();

    static esp_err_t HandleESPSystemEvent(void * ctx, system_event_t * event);

private:

    // NOTE: These members are for internal use by the following friends.

    friend class ConnectivityManager;
    friend class TimeSyncManager;
    friend class Internal::FabricProvisioningServer;
    friend class Internal::ServiceProvisioningServer;
    friend nl::Weave::System::Error nl::Weave::System::Platform::Layer::DispatchEvent(nl::Weave::System::Layer & aLayer, void * aContext, const ::WeavePlatform::WeavePlatformEvent * aEvent);
    friend nl::Weave::System::Error nl::Weave::System::Platform::Layer::StartTimer(nl::Weave::System::Layer & aLayer, void * aContext, uint32_t aMilliseconds);

    void PostEvent(const WeavePlatformEvent * event);

private:

    // NOTE: These members are private to the class and should not be used by friends.

    WEAVE_ERROR InitWeaveEventQueue();
    void DispatchEvent(const WeavePlatformEvent * event);
    static void RunEventLoop(void * arg);
};

} // namespace WeavePlatform

#endif // PLATFORM_MANAGER_H
