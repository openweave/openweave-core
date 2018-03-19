#ifndef WEAVE_PLATFORM_ESP32_H__
#define WEAVE_PLATFORM_ESP32_H__

#include <Weave/Core/WeaveCore.h>

namespace WeavePlatform {

class ConfigurationManager;

extern nl::Weave::System::Layer SystemLayer;
extern nl::Inet::InetLayer InetLayer;
extern nl::Weave::WeaveFabricState FabricState;
extern nl::Weave::WeaveMessageLayer MessageLayer;
extern nl::Weave::WeaveExchangeManager ExchangeMgr;
extern nl::Weave::WeaveSecurityManager SecurityMgr;
extern ConfigurationManager ConfigMgr;

extern bool InitLwIPCoreLock();
extern bool InitWeaveStack();

} // namespace WeavePlatform

#endif // WEAVE_PLATFORM_ESP32_H__
