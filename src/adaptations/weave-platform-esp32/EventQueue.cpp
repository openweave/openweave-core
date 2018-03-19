#include <WeavePlatform-ESP32-Internal.h>

namespace WeavePlatform {
namespace Internal {

extern bool gWeaveTimerActive;
extern TimeOut_t gNextTimerBaseTime;
extern TickType_t gNextTimerDurationTicks;

static QueueHandle_t gWeaveEventQueue;

bool InitWeaveEventQueue()
{
    gWeaveEventQueue = xQueueCreate(100, sizeof(WeaveEvent));
    if (gWeaveEventQueue == NULL) {
        ESP_LOGE(TAG, "Failed to allocate Weave event queue");
        return false;
    }

    return true;
}

} // namespace Internal
} // namespace WeavePlatform

namespace nl {
namespace Weave {
namespace System {
namespace Platform {
namespace Layer {

using namespace ::WeavePlatform;
using namespace ::WeavePlatform::Internal;

System::Error PostEvent(Layer & aLayer, void * aContext, Object & aTarget, EventType aType, uintptr_t aArgument)
{
    Error err = WEAVE_SYSTEM_NO_ERROR;
    WeaveEvent event;

    event.Type = aType;
    event.Target = &aTarget;
    event.Argument = aArgument;

    if (!xQueueSend(gWeaveEventQueue, &event, 1)) {
        ESP_LOGE(TAG, "Failed to post event to Weave event queue");
        err = WEAVE_ERROR_NO_MEMORY;
    }

    return err;
}

System::Error DispatchEvents(Layer & aLayer, void * aContext)
{
    Error err;
    WeaveEvent event;

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
            DispatchEvent(aLayer, aContext, event);

            eventReceived = xQueueReceive(gWeaveEventQueue, &event, 0);
        }
    }

    return WEAVE_SYSTEM_NO_ERROR;
}

System::Error DispatchEvent(Layer & aLayer, void * aContext, Event aEvent)
{
    Error err;

    err = SystemLayer.HandleEvent(*(System::Object *)aEvent.Target, aEvent.Type, aEvent.Argument);
    if (err != WEAVE_SYSTEM_NO_ERROR) {
        ESP_LOGE(TAG, "Error handling Weave event: %s", ErrorStr(err));
    }

    return err;
}


} // namespace Layer
} // namespace Platform
} // namespace System
} // namespace Weave
} // namespace nl
