/*
 *
 *    Copyright (c) 2018 Nest Labs, Inc.
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include <Weave/DeviceLayer/internal/WeaveDeviceInternal.h>
#include <Weave/DeviceLayer/ConnectivityManager.h>
#include <Weave/DeviceLayer/internal/ServiceDirectoryManager.h>
#include <Weave/Profiles/weave-tunneling/WeaveTunnelAgent.h>
#include <new>

using namespace ::nl;
using namespace ::nl::Weave;
using namespace ::nl::Weave::Profiles::WeaveTunnel;
using namespace ::nl::Inet;

namespace nl {
namespace Weave {
namespace Device {
namespace Internal {

WeaveTunnelAgent ServiceTunnelAgent;

WEAVE_ERROR InitServiceTunnelAgent()
{
    WEAVE_ERROR err;

    new (&ServiceTunnelAgent) WeaveTunnelAgent();

#if WEAVE_DEVICE_CONFIG_ENABLE_FIXED_TUNNEL_SERVER

    {
        IPAddress tunnelServerAddr;
        const char * tunnelServerAddrStr;
        uint16_t tunnelServerAddrStrLen;
        uint16_t tunnelServerPort;

        err = ParseHostAndPort(WEAVE_DEVICE_CONFIG_TUNNEL_SERVER_ADDRESS, strlen(WEAVE_DEVICE_CONFIG_TUNNEL_SERVER_ADDRESS),
                tunnelServerAddrStr, tunnelServerAddrStrLen, tunnelServerPort);
        if (err != WEAVE_NO_ERROR || !IPAddress::FromString(tunnelServerAddrStr, tunnelServerAddrStrLen, tunnelServerAddr))
        {
            WeaveLogError(DeviceLayer, "Invalid value specified for TUNNEL_SERVER_ADDRESS config: %s", CONFIG_TUNNEL_SERVER_ADDRESS);
            ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
        }

        if (tunnelServerPort == 0)
        {
            tunnelServerPort = WEAVE_PORT;
        }

        ESP_LOGW(TAG, "Using fixed tunnel server address: %s", CONFIG_TUNNEL_SERVER_ADDRESS);

        err = ServiceTunnelAgent.Init(&InetLayer, &ExchangeMgr, kServiceEndpoint_WeaveTunneling,
                tunnelServerAddr, kWeaveAuthMode_CASE_ServiceEndPoint);
        SuccessOrExit(err);

        // This is necessary because the Init() function doesn't provide a way to specify the port.
        ServiceTunnelAgent.SetDestination(kServiceEndpoint_WeaveTunneling, tunnelServerAddr, tunnelServerPort);
    }

#else // WEAVE_DEVICE_CONFIG_ENABLE_FIXED_TUNNEL_SERVER

#if !WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
#error "Weave service directory feature not enabled (WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY)"
#endif

    err = ServiceTunnelAgent.Init(&InetLayer, &ExchangeMgr, kServiceEndpoint_WeaveTunneling,
            kWeaveAuthMode_CASE_ServiceEndPoint, &ServiceDirectoryMgr);
    SuccessOrExit(err);

#endif // WEAVE_DEVICE_CONFIG_ENABLE_FIXED_TUNNEL_SERVER

exit:
    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogError(DeviceLayer, "InitServiceTunnelAgent() failed: %s", ErrorStr(err));
    }
    return err;
}

} // namespace Internal
} // namespace Device
} // namespace Weave
} // namespace nl


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
