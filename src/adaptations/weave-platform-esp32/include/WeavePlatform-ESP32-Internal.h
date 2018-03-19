#ifndef WEAVE_PLATFORM_ESP32_INTERNAL_H__
#define WEAVE_PLATFORM_ESP32_INTERNAL_H__

#include <ConfigurationManager.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/tcpip.h"
#include "arch/sys_arch.h"

#include "esp_system.h"
#include "esp_log.h"

#include <WeavePlatform-ESP32.h>

using namespace ::nl::Weave;

namespace WeavePlatform {
namespace Internal {

extern const char * const TAG;

extern const uint64_t gTestDeviceId;
extern const char * gTestPairingCode;
extern const uint8_t gTestDeviceCert[];
extern const uint16_t gTestDeviceCertLength;
extern const uint8_t gTestDevicePrivateKey[];
extern const uint16_t gTestDevicePrivateKeyLength;

extern bool InitWeaveEventQueue();
extern bool InitWeaveServers();
extern bool InitCASEAuthDelegate();
extern bool InitFabricProvisioningServer();
extern int GetEntropy_ESP32(uint8_t *buf, size_t bufSize);

} // namespace Internal
} // namespace WeavePlatform

#endif // WEAVE_PLATFORM_ESP32_INTERNAL_H__
