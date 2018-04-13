#include <internal/WeavePlatformInternal.h>
#include <ConnectivityManager.h>
#include <internal/ServiceDirectoryManager.h>
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

#if CONFIG_ENABLE_FIXED_TUNNEL_SERVER

    {
        IPAddress tunnelServerAddr;
        if (!IPAddress::FromString(CONFIG_TUNNEL_SERVER_ADDRESS, tunnelServerAddr))
        {
            ESP_LOGE(TAG, "Invalid value specified for TUNNEL_SERVER_ADDRESS config: %s", CONFIG_TUNNEL_SERVER_ADDRESS);
            ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
        }

        ESP_LOGW(TAG, "Using fixed tunnel server address: %s", CONFIG_TUNNEL_SERVER_ADDRESS);

        err = ServiceTunnelAgent.Init(&InetLayer, &ExchangeMgr, kServiceEndpoint_WeaveTunneling,
                tunnelServerAddr, kWeaveAuthMode_CASE_ServiceEndPoint);
        SuccessOrExit(err);
    }

#else // CONFIG_ENABLE_FIXED_TUNNEL_SERVER

#if !WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
#error "Weave service directory feature not enabled (WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY)"
#endif

    err = ServiceTunnelAgent.Init(&InetLayer, &ExchangeMgr, kServiceEndpoint_WeaveTunneling,
            kWeaveAuthMode_CASE_ServiceEndPoint, &ServiceDirectoryMgr);
    SuccessOrExit(err);

#endif // CONFIG_ENABLE_FIXED_TUNNEL_SERVER

exit:
    if (err != WEAVE_NO_ERROR)
    {
        ESP_LOGE(TAG, "InitServiceTunnelAgent() failed: %s", ErrorStr(err));
    }
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
