#include <internal/WeavePlatformInternal.h>
#include <ConnectivityManager.h>
#include <Weave/Profiles/weave-tunneling/WeaveTunnelAgent.h>
#include <new>

using namespace ::nl;
using namespace ::nl::Weave;
using namespace ::nl::Weave::Profiles::WeaveTunnel;
using namespace ::nl::Inet;

namespace WeavePlatform {
namespace Internal {

WeaveTunnelAgent ServiceTunnelAgent;

WEAVE_ERROR InitServiceTunnelAgent()
{
    WEAVE_ERROR err;

    new (&ServiceTunnelAgent) WeaveTunnelAgent();

#define kServiceEndpoint_WeaveTunneling         (0x18B4300200000011ull)     ///< Weave tunneling endpoint

    IPAddress tfeAddr;
    IPAddress::FromString("52.87.170.40", tfeAddr); // tunnel04.weave01.iad02.integration.nestlabs.com

    err = ServiceTunnelAgent.Init(&InetLayer, &ExchangeMgr, kServiceEndpoint_WeaveTunneling,
            tfeAddr, kWeaveAuthMode_CASE_ServiceEndPoint);
    SuccessOrExit(err);

exit:
    return err;
}

} // namespace Internal
} // namespace WeavePlatform


namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveTunnel {
namespace Platform {

} // namespace Platform
} // namespace WeaveTunnel
} // namespace Profiles
} // namespace Weave
} // namespace nl
