#include <WeavePlatform-ESP32-Internal.h>

#include <SystemLayer/SystemEvent.h>

namespace WeavePlatform {
namespace Internal {

static QueueHandle_t gWeaveEventQueue;

extern bool gWeaveTimerActive;
extern TimeOut_t gNextTimerBaseTime;
extern TickType_t gNextTimerDurationTicks;

bool InitWeaveEventQueue()
{
    // TODO: make queue size configurable
    gWeaveEventQueue = xQueueCreate(100, sizeof(WeavePlatformEvent));
    if (gWeaveEventQueue == NULL)
    {
        ESP_LOGE(TAG, "Failed to allocate Weave event queue");
        return false;
    }

    return true;
}

void DispatchEvent(const WeavePlatformEvent * event)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // If the event is a Weave System or Inet Layer event, dispatch it to the SystemLayer event handler.
    if (event->Type == WeavePlatformEvent::kType_WeaveSystemEvent)
    {
        err = SystemLayer.HandleEvent(*event->WeaveSystemEvent.Target, event->WeaveSystemEvent.Type, event->WeaveSystemEvent.Argument);
        if (err != WEAVE_SYSTEM_NO_ERROR)
        {
            ESP_LOGE(TAG, "Error handling Weave System Layer event (type %d): %s", event->Type, nl::ErrorStr(err));
        }
    }

    else if (event->Type == WeavePlatformEvent::kType_ESPSystemEvent)
    {
        ConnectivityMgr.OnPlatformEvent(event);
    }
}

} // namespace Internal

using namespace ::WeavePlatform::Internal;

esp_err_t HandleESPSystemEvent(void * ctx, system_event_t * espEvent)
{
    if (gWeaveEventQueue != NULL)
    {
        WeavePlatformEvent event;

        event.Type = WeavePlatformEvent::kType_ESPSystemEvent;
        event.ESPSystemEvent = *espEvent;

        if (!xQueueSend(gWeaveEventQueue, &event, 1))
        {
            ESP_LOGE(TAG, "Failed to post event to Weave Platform event queue");
        }
    }

    return ESP_OK;
}

} // namespace WeavePlatform

namespace nl {
namespace Weave {
namespace System {
namespace Platform {
namespace Layer {

using namespace ::WeavePlatform;
using namespace ::WeavePlatform::Internal;

System::Error PostEvent(System::Layer & aLayer, void * aContext, System::Object & aTarget, System::EventType aType, uintptr_t aArgument)
{
    Error err = WEAVE_SYSTEM_NO_ERROR;
    WeavePlatformEvent event;

    event.Type = WeavePlatformEvent::kType_WeaveSystemEvent;
    event.WeaveSystemEvent.Type = aType;
    event.WeaveSystemEvent.Target = &aTarget;
    event.WeaveSystemEvent.Argument = aArgument;

    if (!xQueueSend(gWeaveEventQueue, &event, 1)) {
        ESP_LOGE(TAG, "Failed to post event to Weave Platform event queue");
        err = WEAVE_ERROR_NO_MEMORY;
    }

    return err;
}

System::Error DispatchEvents(Layer & aLayer, void * aContext)
{
    Error err;
    WeavePlatformEvent event;

    while (true)
    {
        TickType_t waitTime;

        // If one or more Weave timers are active...
        if (gWeaveTimerActive) {

            // Adjust the base time and remaining duration for the next scheduled timer based on the
            // amount of time that has elapsed since it was started.
            // IF the timer's expiration time has already arrived...
            if (xTaskCheckForTimeOut(&gNextTimerBaseTime, &gNextTimerDurationTicks) == pdTRUE) {

                // Reset the 'timer active' flag.  This will be set to true again by HandlePlatformTimer()
                // if there are further timers beyond the expired one that are still active.
                gWeaveTimerActive = false;

                // Call into the system layer to dispatch the callback functions for all timers
                // that have expired.
                err = SystemLayer.HandlePlatformTimer();
                if (err != WEAVE_SYSTEM_NO_ERROR) {
                    ESP_LOGE(TAG, "Error handling Weave timers: %s", ErrorStr(err));
                }

                // When processing the event queue below, do not wait if the queue is empty.  Instead
                // immediately loop around and process timers again
                waitTime = 0;
            }

            // If there is still time before the next timer expires, arrange to wait on the event queue
            // until that timer expires.
            else {
                waitTime = gNextTimerDurationTicks;
            }
        }

        // Otherwise no Weave timers are active, so wait indefinitely for an event to arrive on the event
        // queue.
        else {
            waitTime = portMAX_DELAY;
        }

        // TODO: unlock Weave stack

        BaseType_t eventReceived = xQueueReceive(gWeaveEventQueue, &event, waitTime);

        // TODO: lock Weave stack

        // If an event was received, dispatch it.  Continue receiving events from the queue and
        // dispatching them until the queue is empty.
        while (eventReceived == pdTRUE) {

            DispatchEvent(&event);

            eventReceived = xQueueReceive(gWeaveEventQueue, &event, 0);
        }
    }

    return WEAVE_SYSTEM_NO_ERROR;
}

System::Error DispatchEvent(System::Layer & aLayer, void * aContext, const WeavePlatformEvent * aEvent)
{
    DispatchEvent(aEvent);
    return WEAVE_NO_ERROR;
}


} // namespace Layer
} // namespace Platform
} // namespace System
} // namespace Weave
} // namespace nl
