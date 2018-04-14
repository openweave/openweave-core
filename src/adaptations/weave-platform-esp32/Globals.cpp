#include <internal/WeavePlatformInternal.h>
#include <internal/DeviceControlServer.h>
#include <internal/DeviceDescriptionServer.h>
#include <internal/NetworkProvisioningServer.h>
#include <internal/FabricProvisioningServer.h>
#include <internal/ServiceProvisioningServer.h>
#include <internal/EchoServer.h>
#include <internal/WeavePlatformInternal.h>

namespace WeavePlatform {

PlatformManager PlatformMgr;
ConfigurationManager ConfigurationMgr;
ConnectivityManager ConnectivityMgr;
TimeSyncManager TimeSyncMgr;

nl::Weave::System::Layer SystemLayer;
nl::Inet::InetLayer InetLayer;
nl::Weave::WeaveFabricState FabricState;
nl::Weave::WeaveMessageLayer MessageLayer;
nl::Weave::WeaveExchangeManager ExchangeMgr;
nl::Weave::WeaveSecurityManager SecurityMgr;

namespace Internal {

DeviceControlServer DeviceControlSvr;
DeviceDescriptionServer DeviceDescriptionSvr;
NetworkProvisioningServer NetworkProvisioningSvr;
FabricProvisioningServer FabricProvisioningSvr;
ServiceProvisioningServer ServiceProvisioningSvr;
EchoServer EchoSvr;

const char * const TAG = "weave-platform";

} // namespace Internal
} // namespace WeavePlatform
