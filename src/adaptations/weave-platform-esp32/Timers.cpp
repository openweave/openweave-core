#include <WeavePlatform-ESP32-Internal.h>


namespace WeavePlatform {
namespace Internal {

bool gWeaveTimerActive;
TimeOut_t gNextTimerBaseTime;
TickType_t gNextTimerDurationTicks;

} // namespace Internal
} // namespace WeavePlatform

namespace nl {
namespace Weave {
namespace System {
namespace Platform {
namespace Layer {

using namespace WeavePlatform::Internal;

System::Error StartTimer(System::Layer & aLayer, void * aContext, uint32_t aMilliseconds)
{
    gWeaveTimerActive = true;
    vTaskSetTimeOutState(&gNextTimerBaseTime);
    gNextTimerDurationTicks = pdMS_TO_TICKS(aMilliseconds);

    // TODO: kick event loop thread if this method is called on a different thread.

    return WEAVE_SYSTEM_NO_ERROR;
}



} // namespace Layer
} // namespace Platform
} // namespace System
} // namespace Weave
} // namespace nl
