#include <internal/WeavePlatformInternal.h>
#include <esp_timer.h>

namespace nl {
namespace Weave {
namespace System {
namespace Platform {
namespace Layer {

uint64_t GetSystemTimeMS(void)
{
    return (uint64_t)::esp_timer_get_time()/1000;
}

} // namespace Layer
} // namespace Platform
} // namespace System
} // namespace Weave
} // namespace nl

namespace nl {
namespace Weave {
namespace Platform {
namespace Time {

WEAVE_ERROR GetMonotonicRawTime(int64_t * const p_timestamp_usec)
{
    *p_timestamp_usec = ::esp_timer_get_time();
    return WEAVE_NO_ERROR;
}

WEAVE_ERROR GetSystemTimeMs(int64_t * const p_timestamp_msec)
{
    return WEAVE_ERROR_NOT_IMPLEMENTED;
}

} // namespace Time
} // namespace Platform
} // namespace Weave
} // namespace nl
