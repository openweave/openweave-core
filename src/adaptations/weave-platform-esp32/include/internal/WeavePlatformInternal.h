#ifndef WEAVE_PLATFORM_INTERNAL_H
#define WEAVE_PLATFORM_INTERNAL_H

#include <WeavePlatform.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/tcpip.h"
#include "arch/sys_arch.h"

#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"


using namespace ::nl::Weave;

namespace WeavePlatform {
namespace Internal {

struct WeavePlatformEvent
{
    enum
    {
        kEventType_WeaveSystemLayerEvent                = 0,
        kEventType_ESPSystemEvent                       = 1,
    };

    union
    {
        struct
        {
            nl::Weave::System::EventType Type;
            nl::Weave::System::Object * Target;
            uintptr_t Argument;
        } WeaveSystemLayerEvent;
        system_event_t ESPSystemEvent;
    };
    uint8_t Type;
};

extern const char * const TAG;

} // namespace Internal
} // namespace WeavePlatform

#endif // WEAVE_PLATFORM_ESP32_INTERNAL_H
