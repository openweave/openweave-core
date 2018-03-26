#ifndef WEAVE_PLATFORM_ESP32_H__
#define WEAVE_PLATFORM_ESP32_H__

#include <Weave/Core/WeaveCore.h>
#include <PlatformManager.h>
#include <ConfigurationManager.h>
#include <ConnectivityManager.h>
#include <WeavePlatformError.h>

namespace WeavePlatform {

extern PlatformManager PlatformMgr;
extern ConfigurationManager ConfigurationMgr;
extern ConnectivityManager ConnectivityMgr;
extern nl::Weave::System::Layer SystemLayer;
extern nl::Inet::InetLayer InetLayer;
extern nl::Weave::WeaveFabricState FabricState;
extern nl::Weave::WeaveMessageLayer MessageLayer;
extern nl::Weave::WeaveExchangeManager ExchangeMgr;
extern nl::Weave::WeaveSecurityManager SecurityMgr;

} // namespace WeavePlatform

#endif // WEAVE_PLATFORM_ESP32_H__
