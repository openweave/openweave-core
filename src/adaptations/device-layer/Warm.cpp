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

#include <Weave/DeviceLayer/internal/WeaveDeviceLayerInternal.h>
#include <Weave/DeviceLayer/ConnectivityManager.h>
#include <Warm/Warm.h>
#include <lwip/netif.h>
#include <lwip/ip6_route_table.h>

using namespace ::nl::Weave::DeviceLayer;
using namespace ::nl::Weave::DeviceLayer::Internal;

using namespace ::nl;
using namespace ::nl::Weave;
using namespace ::nl::Weave::Warm;

namespace {

extern WEAVE_ERROR GetLwIPNetifForWarmInterfaceType(InterfaceType inInterfaceType, struct netif *& netif);
extern const char * WarmInterfaceTypeToStr(InterfaceType inInterfaceType);
extern const char * CharacterizeIPv6Prefix(const Inet::IPPrefix & inPrefix);

} // unnamed namespace

// ==================== WARM Platform Functions ====================

namespace nl {
namespace Weave {
namespace Warm {
namespace Platform {

WEAVE_ERROR Init(WarmFabricStateDelegate * inFabricStateDelegate)
{
    // Nothing to do.
    return WEAVE_NO_ERROR;
}

void CriticalSectionEnter(void)
{
    // No-op on this platform since all interaction with WARM core happens on the Weave event thread.
}

void CriticalSectionExit(void)
{
    // No-op on this platform since all interaction with WARM core happens on the Weave event thread.
}

void RequestInvokeActions(void)
{
    ::nl::Weave::Warm::InvokeActions();
}

PlatformResult AddRemoveHostAddress(InterfaceType inInterfaceType, const Inet::IPAddress & inAddress, uint8_t inPrefixLength, bool inAdd)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    err_t lwipErr;
    struct netif * netif;
    ip6_addr_t ip6addr = inAddress.ToIPv6();
    bool lockHeld = false;

    // If an address is being added/removed from the tunnel interface, and the address in question
    // is a ULA referring to the Weave Primary WiFi subnet, substitute the Thread Mesh subnet id.
    // This works around a limitation in the current Nest service, which presumes that all devices
    // have a Thread radio, and therefore a Thread Mesh address to which packets can be routed.
    if (inInterfaceType == kInterfaceTypeTunnel && inAddress.IsIPv6ULA() && inAddress.Subnet() == kWeaveSubnetId_PrimaryWiFi)
    {
        Inet::IPAddress altAddr = Inet::IPAddress::MakeULA(inAddress.GlobalId(), kWeaveSubnetId_ThreadMesh, inAddress.InterfaceId());
        ip6addr = altAddr.ToIPv6();
    }

    LOCK_TCPIP_CORE();
    lockHeld = true;

    err = GetLwIPNetifForWarmInterfaceType(inInterfaceType, netif);
    SuccessOrExit(err);

    if (inAdd)
    {
        lwipErr = netif_add_ip6_address_with_route(netif, &ip6addr, inPrefixLength, NULL);
        err = System::MapErrorLwIP(lwipErr);
        if (err != WEAVE_NO_ERROR)
        {
            WeaveLogError(DeviceLayer, "netif_add_ip6_address_with_route() failed for %s interface: %s",
                    WarmInterfaceTypeToStr(inInterfaceType), nl::ErrorStr(err));
            ExitNow();
        }
    }
    else
    {
        lwipErr = netif_remove_ip6_address_with_route(netif, &ip6addr, inPrefixLength);
        err = System::MapErrorLwIP(lwipErr);
        if (err != WEAVE_NO_ERROR)
        {
            WeaveLogError(DeviceLayer, "netif_remove_ip6_address_with_route() failed for %s interface: %s",
                     WarmInterfaceTypeToStr(inInterfaceType), nl::ErrorStr(err));
            ExitNow();
        }
    }

    UNLOCK_TCPIP_CORE();
    lockHeld = false;

#if WEAVE_PROGRESS_LOGGING
    {
        char interfaceName[4];
        GetInterfaceName(netif, interfaceName, sizeof(interfaceName));
        char ipAddrStr[INET6_ADDRSTRLEN];
        inAddress.ToString(ipAddrStr, sizeof(ipAddrStr));
        WeaveLogProgress(DeviceLayer, "%s %s on %s interface (%s): %s/%" PRId8,
                 (inAdd) ? "Adding" : "Removing",
                 CharacterizeIPv6Address(inAddress),
                 WarmInterfaceTypeToStr(inInterfaceType),
                 interfaceName,
                 ipAddrStr, inPrefixLength);
    }
#endif // WEAVE_PROGRESS_LOGGING

exit:
    if (lockHeld)
    {
        UNLOCK_TCPIP_CORE();
    }

    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogError(DeviceLayer, "AddRemoveHostAddress() failed: %s", ::nl::ErrorStr(err));
    }

    return (err == WEAVE_NO_ERROR) ? kPlatformResultSuccess : kPlatformResultFailure;
}

PlatformResult AddRemoveHostRoute(InterfaceType inInterfaceType, const Inet::IPPrefix & inPrefix, RoutePriority inPriority, bool inAdd)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    err_t lwipErr;
    struct netif * netif;
    struct ip6_prefix lwipIP6prefix;
    bool lockHeld = false;

    LOCK_TCPIP_CORE();
    lockHeld = true;

    err = GetLwIPNetifForWarmInterfaceType(inInterfaceType, netif);
    SuccessOrExit(err);

    lwipIP6prefix.addr = inPrefix.IPAddr.ToIPv6();
    lwipIP6prefix.prefix_len = inPrefix.Length;

    if (inAdd)
    {
        lwipErr = ip6_add_route_entry(&lwipIP6prefix, netif, NULL, NULL);
        err = System::MapErrorLwIP(lwipErr);
        if (err != WEAVE_NO_ERROR)
        {
            WeaveLogError(DeviceLayer, "ip6_add_route_entry() failed for %s interface: %s",
                     WarmInterfaceTypeToStr(inInterfaceType), nl::ErrorStr(err));
            ExitNow();
        }
    }
    else
    {
        ip6_remove_route_entry(&lwipIP6prefix);
    }

    UNLOCK_TCPIP_CORE();
    lockHeld = false;

#if WEAVE_PROGRESS_LOGGING
    {
        char interfaceName[4];
        GetInterfaceName(netif, interfaceName, sizeof(interfaceName));
        char prefixAddrStr[INET6_ADDRSTRLEN];
        inPrefix.IPAddr.ToString(prefixAddrStr, sizeof(prefixAddrStr));
        const char * prefixDesc = CharacterizeIPv6Prefix(inPrefix);
        WeaveLogProgress(DeviceLayer, "IPv6 route%s%s %s %s interface (%s): %s/%" PRId8,
                 (prefixDesc != NULL) ? " for " : "",
                 (prefixDesc != NULL) ? prefixDesc : "",
                 (inAdd) ? "added to" : "removed from",
                 WarmInterfaceTypeToStr(inInterfaceType),
                 interfaceName,
                 prefixAddrStr, inPrefix.Length);
    }
#endif // WEAVE_PROGRESS_LOGGING

exit:
    if (lockHeld)
    {
        UNLOCK_TCPIP_CORE();
    }

    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogError(DeviceLayer, "AddRemoveHostRoute() failed: %s", ::nl::ErrorStr(err));
    }

    return (err == WEAVE_NO_ERROR) ? kPlatformResultSuccess : kPlatformResultFailure;
}

} // namespace Platform
} // namespace Warm
} // namespace Weave
} // namespace nl

// ==================== Local Utility Functions ====================

namespace {

WEAVE_ERROR GetLwIPNetifForWarmInterfaceType(InterfaceType inInterfaceType, struct netif *& netif)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    for (netif = netif_list; netif != NULL; netif = netif->next)
    {
        if (inInterfaceType == kInterfaceTypeWiFi && netif->name[0] == 's' && netif->name[1] == 't')
        {
            ExitNow(err = WEAVE_NO_ERROR);
        }

        if (inInterfaceType == kInterfaceTypeTunnel && netif->name[0] == 't' && netif->name[1] == 'n')
        {
            ExitNow(err = WEAVE_NO_ERROR);
        }
    }

    ExitNow(err = INET_ERROR_UNKNOWN_INTERFACE);

exit:
    return err;
}

const char * WarmInterfaceTypeToStr(InterfaceType inInterfaceType)
{
    switch (inInterfaceType)
    {
    case kInterfaceTypeLegacy6LoWPAN:
        return "Legacy 6LoWPAN";
    case kInterfaceTypeThread:
        return "Thread";
    case kInterfaceTypeWiFi:
        return "WiFi station";
    case kInterfaceTypeTunnel:
        return "Tunnel";
    case kInterfaceTypeCellular:
        return "Cellular";
    default:
        return "(unknown)";
    }
}

const char * CharacterizeIPv6Prefix(const Inet::IPPrefix & inPrefix)
{
    if (inPrefix.IPAddr.IsIPv6ULA())
    {
        if (::nl::Weave::DeviceLayer::FabricState.FabricId != kFabricIdNotSpecified &&
            inPrefix.IPAddr.GlobalId() == nl::Weave::WeaveFabricIdToIPv6GlobalId(::nl::Weave::DeviceLayer::FabricState.FabricId))
        {
            if (inPrefix.Length == 48)
            {
                return "Weave fabric prefix";
            }
            if (inPrefix.Length == 64)
            {
                switch (inPrefix.IPAddr.Subnet())
                {
                case kWeaveSubnetId_PrimaryWiFi:
                    return "Weave WiFi prefix";
                case kWeaveSubnetId_Service:
                    return "Weave Service prefix";
                case kWeaveSubnetId_ThreadMesh:
                    return "Weave Thread prefix";
                case kWeaveSubnetId_ThreadAlarm:
                    return "Weave Thread Alarm prefix";
                case kWeaveSubnetId_WiFiAP:
                    return "Weave WiFi AP prefix";
                case kWeaveSubnetId_MobileDevice:
                    return "Weave Mobile prefix";
                default:
                    return "Weave IPv6 prefix";
                }
            }
        }
    }
    return NULL;
}

} // unnamed namespace

