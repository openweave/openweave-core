#include <WeavePlatform-ESP32-Internal.h>


using namespace WeavePlatform::Internal;

namespace WeavePlatform {

namespace Internal {

static SemaphoreHandle_t gLwIPCoreLock;

} // namespace Internal

bool InitLwIPCoreLock()
{
    gLwIPCoreLock = xSemaphoreCreateMutex();
    if (gLwIPCoreLock == NULL) {
        ESP_LOGE(TAG, "Failed to create LwIP core lock");
        return false;
    }

    return true;
}

} // namespace WeavePlatform

extern "C" void lock_lwip_core()
{
    xSemaphoreTake(gLwIPCoreLock, portMAX_DELAY);
}

extern "C" void unlock_lwip_core()
{
    xSemaphoreGive(gLwIPCoreLock);
}
