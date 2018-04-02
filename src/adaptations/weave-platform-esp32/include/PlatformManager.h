#ifndef PLATFORM_MANAGER_H
#define PLATFORM_MANAGER_H

#include <esp_event.h>

namespace WeavePlatform {

class ConnectivityManager;

namespace Internal {
struct WeavePlatformEvent;
} // namespace Internal

class PlatformManager
{
public:

    WEAVE_ERROR InitLwIPCoreLock();
    WEAVE_ERROR InitWeaveStack();

    void RunEventLoop();

    static esp_err_t HandleESPSystemEvent(void * ctx, system_event_t * event);

    typedef void (*AsyncWorkFunct)(intptr_t arg);
    void ScheduleWork(AsyncWorkFunct workFunct, intptr_t arg = 0);

private:

    // NOTE: These members are for internal use by the following friend classes.

    friend class ConnectivityManager;
    friend nl::Weave::System::Error nl::Weave::System::Platform::Layer::DispatchEvent(nl::Weave::System::Layer & aLayer, void * aContext, const ::WeavePlatform::Internal::WeavePlatformEvent * aEvent);

    void PostEvent(const Internal::WeavePlatformEvent * event);

private:

    WEAVE_ERROR InitWeaveEventQueue();
    void DispatchEvent(const Internal::WeavePlatformEvent * event);
};

} // namespace WeavePlatform

#endif // PLATFORM_MANAGER_H
