#ifndef WEAVE_PLATFORM_ESP32_H__
#define WEAVE_PLATFORM_ESP32_H__

#include <Weave/Core/WeaveCore.h>
#include <WeavePlatformError.h>
#include <ConfigurationManager.h>
#include <ConnectivityManager.h>

#include <esp_event.h>

namespace WeavePlatform {

extern nl::Weave::System::Layer SystemLayer;
extern nl::Inet::InetLayer InetLayer;
extern nl::Weave::WeaveFabricState FabricState;
extern nl::Weave::WeaveMessageLayer MessageLayer;
extern nl::Weave::WeaveExchangeManager ExchangeMgr;
extern nl::Weave::WeaveSecurityManager SecurityMgr;
extern ConfigurationManager ConfigMgr;
extern ConnectivityManager ConnectivityMgr;

extern bool InitLwIPCoreLock();
extern bool InitWeaveStack();

extern esp_err_t HandleESPSystemEvent(void * ctx, system_event_t * event);

} // namespace WeavePlatform

#endif // WEAVE_PLATFORM_ESP32_H__
