#include <internal/WeavePlatformInternal.h>
#include <esp_timer.h>

#include <Weave/Support/TimeUtils.h>

using namespace ::WeavePlatform::Internal;

namespace nl {
namespace Weave {
namespace System {
namespace Platform {
namespace Layer {

uint64_t GetClock_Monotonic(void)
{
    return (uint64_t)::esp_timer_get_time();
}

uint64_t GetClock_MonotonicMS(void)
{
    return (uint64_t)::esp_timer_get_time() / 1000;
}

uint64_t GetClock_MonotonicHiRes(void)
{
    return (uint64_t)::esp_timer_get_time();
}

Error GetClock_RealTime(uint64_t & curTime)
{
    struct timeval tv;
    int res = gettimeofday(&tv, NULL);
    if (res != 0)
    {
        return MapErrorPOSIX(errno);
    }
    if (tv.tv_sec < WEAVE_SYSTEM_CONFIG_VALID_REAL_TIME_THRESHOLD)
    {
        return WEAVE_SYSTEM_ERROR_REAL_TIME_NOT_SYNCED;
    }
    curTime = (tv.tv_sec * UINT64_C(1000000)) + tv.tv_usec;
    return WEAVE_SYSTEM_NO_ERROR;
}

Error GetClock_RealTimeMS(uint64_t & curTime)
{
    struct timeval tv;
    int res = gettimeofday(&tv, NULL);
    if (res != 0)
    {
        return MapErrorPOSIX(errno);
    }
    if (tv.tv_sec < WEAVE_SYSTEM_CONFIG_VALID_REAL_TIME_THRESHOLD)
    {
        return WEAVE_SYSTEM_ERROR_REAL_TIME_NOT_SYNCED;
    }
    curTime = (tv.tv_sec * UINT64_C(1000)) + (tv.tv_usec / 1000);
    return WEAVE_SYSTEM_NO_ERROR;
}

Error SetClock_RealTime(uint64_t newCurTime)
{
    struct timeval tv;
    tv.tv_sec = static_cast<time_t>(newCurTime / UINT64_C(1000000));
    tv.tv_usec = static_cast<long>(newCurTime % UINT64_C(1000000));
    int res = settimeofday(&tv, NULL);
    if (res != 0)
    {
        return (errno == EPERM) ? WEAVE_SYSTEM_ERROR_ACCESS_DENIED : MapErrorPOSIX(errno);
    }
    if (LOG_LOCAL_LEVEL >= ESP_LOG_INFO)
    {
        uint16_t year;
        uint8_t month, dayOfMonth, hour, minute, second;
        SecondsSinceEpochToCalendarTime(tv.tv_sec, year, month, dayOfMonth, hour, minute, second);
        ESP_LOGI(TAG, "Real time clock set to %ld (%04" PRId16 "/%02" PRId8 "/%02" PRId8 " %02" PRId8 ":%02" PRId8 ":%02" PRId8 ")",
                 tv.tv_sec, year, month, dayOfMonth, hour, minute, second);
    }
    return WEAVE_SYSTEM_NO_ERROR;
}

} // namespace Layer
} // namespace Platform
} // namespace System
} // namespace Weave
} // namespace nl
