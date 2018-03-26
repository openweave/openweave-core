#ifndef PLATFORM_MANAGER_H
#define PLATFORM_MANAGER_H

#include <esp_event.h>

namespace WeavePlatform {

namespace Internal {
struct WeavePlatformEvent;
} // namespace Internal

class PlatformManager
{
    friend nl::Weave::System::Error nl::Weave::System::Platform::Layer::DispatchEvent(nl::Weave::System::Layer & aLayer, void * aContext, const ::WeavePlatform::Internal::WeavePlatformEvent * aEvent);

public:
    WEAVE_ERROR InitLwIPCoreLock();
    WEAVE_ERROR InitWeaveStack();

    void RunEventLoop();

    static esp_err_t HandleESPSystemEvent(void * ctx, system_event_t * event);

private:
    WEAVE_ERROR InitWeaveEventQueue();
    void DispatchEvent(const Internal::WeavePlatformEvent * event);
};

} // namespace WeavePlatform

#endif // PLATFORM_MANAGER_H
