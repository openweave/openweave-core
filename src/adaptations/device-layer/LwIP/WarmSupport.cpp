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

/**
 *    @file
 *          Provides implementations of platform functions for the Weave
 *          Addressing and Routing Module (WARM) for use on LwIP-based
 *          platforms.
 */

#include <Weave/DeviceLayer/internal/WeaveDeviceLayerInternal.h>
#include <Weave/DeviceLayer/ConnectivityManager.h>
#include <Weave/DeviceLayer/LwIP/WarmSupport.h>
#include <lwip/netif.h>
#include <lwip/ip6_route_table.h>

#if WARM_CONFIG_SUPPORT_THREAD
#include <Weave/DeviceLayer/ThreadStackManager.h>
#include <Weave/DeviceLayer/OpenThread/OpenThreadUtils.h>
#include <openthread/ip6.h>
#endif // WARM_CONFIG_SUPPORT_THREAD


using namespace ::nl::Weave::DeviceLayer;
using namespace ::nl::Weave::DeviceLayer::Internal;

using namespace ::nl;
using namespace ::nl::Weave;
using namespace ::nl::Weave::Warm;

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
    // have a Thread radio, and therefore a Thread subnet Weave ULA to which packets can be routed.
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
        s8_t addrIdx;
        lwipErr = netif_add_ip6_address_with_route(netif, &ip6addr, inPrefixLength, &addrIdx);
        err = System::MapErrorLwIP(lwipErr);
        if (err != WEAVE_NO_ERROR)
        {
            WeaveLogError(DeviceLayer, "netif_add_ip6_address_with_route() failed for %s interface: %s",
                    WarmInterfaceTypeToStr(inInterfaceType), nl::ErrorStr(err));
            ExitNow();
        }
        netif_ip6_addr_set_state(netif, addrIdx, IP6_ADDR_PREFERRED);
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
        WeaveLogProgress(DeviceLayer, "%s %s %s LwIP %s interface (%s): %s/%" PRId8,
                 (inAdd) ? "Adding" : "Removing",
                 CharacterizeIPv6Address(inAddress),
                 (inAdd) ? "to" : "from",
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
    struct netif * netif;
    bool lockHeld = false;

    LOCK_TCPIP_CORE();
    lockHeld = true;

    err = GetLwIPNetifForWarmInterfaceType(inInterfaceType, netif);
    SuccessOrExit(err);

    // if requested, set/unset the default route...
    if (inPrefix.Length == 0)
    {
        // Only bother to set the default route.
        if (inAdd)
        {
            netif_set_default(netif);
        }
    }

    // otherwise a more specific route is being added/removed, so...
    else
    {
#if WARM_CONFIG_SUPPORT_WIFI || WARM_CONFIG_SUPPORT_CELLULAR

        // On platforms that support WiFi and/or cellular, this code supports full manipulation of the
        // local routing table.  Note that this requires a custom version of LwIP that support the
        // LWIP_IPV6_ROUTE_TABLE_SUPPORT extension.

        struct ip6_prefix lwipIP6prefix;
        lwipIP6prefix.addr = inPrefix.IPAddr.ToIPv6();
        lwipIP6prefix.prefix_len = inPrefix.Length;

        if (inAdd)
        {
            err_t lwipErr = ip6_add_route_entry(&lwipIP6prefix, netif, NULL, NULL);
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

#elif WARM_CONFIG_SUPPORT_THREAD

        // On platforms that only support Thread there is only one interface, and thus no need
        // for a generalized routing table or adding/removing routes. In this situation, WARM
        // will only call this function to set the default route.  Since that case was handled
        // above, we fail with an error here.

        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);

#endif // WARM_CONFIG_SUPPORT_THREAD
    }

    UNLOCK_TCPIP_CORE();
    lockHeld = false;

#if WEAVE_PROGRESS_LOGGING
    {
        char interfaceName[4];
        GetInterfaceName(netif, interfaceName, sizeof(interfaceName));
        if (inPrefix.Length != 0)
        {
            char prefixAddrStr[INET6_ADDRSTRLEN];
            inPrefix.IPAddr.ToString(prefixAddrStr, sizeof(prefixAddrStr));
            const char * prefixDesc = CharacterizeIPv6Prefix(inPrefix);
            WeaveLogProgress(DeviceLayer, "IPv6 route%s%s %s LwIP %s interface (%s): %s/%" PRId8,
                     (prefixDesc != NULL) ? " for " : "",
                     (prefixDesc != NULL) ? prefixDesc : "",
                     (inAdd) ? "added to" : "removed from",
                     WarmInterfaceTypeToStr(inInterfaceType),
                     interfaceName,
                     prefixAddrStr, inPrefix.Length);
        }
        else
        {
            WeaveLogProgress(DeviceLayer, "LwIP default interface set to %s interface (%s)",
                     WarmInterfaceTypeToStr(inInterfaceType),
                     interfaceName);
        }
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

#if WARM_CONFIG_SUPPORT_THREAD

PlatformResult AddRemoveThreadAddress(InterfaceType inInterfaceType, const Inet::IPAddress &inAddress, bool inAdd)
{
    otError otErr;
    otNetifAddress otAddress;

    memset(&otAddress, 0, sizeof(otAddress));
    otAddress.mAddress = ToOpenThreadIP6Address(inAddress);
    otAddress.mPrefixLength = 64;
    otAddress.mValid = true;
    otAddress.mPreferred = true;

    ThreadStackMgrImpl().LockThreadStack();

    if (inAdd)
    {
        otErr = otIp6AddUnicastAddress(ThreadStackMgrImpl().OTInstance(), &otAddress);
    }
    else
    {
        otErr = otIp6RemoveUnicastAddress(ThreadStackMgrImpl().OTInstance(), &otAddress.mAddress);
    }

    ThreadStackMgrImpl().UnlockThreadStack();

    if (otErr == OT_ERROR_NONE)
    {
#if WEAVE_PROGRESS_LOGGING
        char ipAddrStr[INET6_ADDRSTRLEN];
        inAddress.ToString(ipAddrStr, sizeof(ipAddrStr));
        WeaveLogProgress(DeviceLayer, "%s %s %s OpenThread stack: %s/64",
                 (inAdd) ? "Adding" : "Removing",
                 CharacterizeIPv6Address(inAddress),
                 (inAdd) ? "to" : "from",
                 ipAddrStr);
#endif // WEAVE_PROGRESS_LOGGING
    }
    else
    {
        WeaveLogError(DeviceLayer, "AddRemoveThreadAddress() failed: %s", ::nl::ErrorStr(MapOpenThreadError(otErr)));
    }

    return (otErr == OT_ERROR_NONE) ? kPlatformResultSuccess : kPlatformResultFailure;
}

#endif // WARM_CONFIG_SUPPORT_THREAD

#if WARM_CONFIG_SUPPORT_THREAD_ROUTING

#error "Weave Thread router support not implemented"

PlatformResult StartStopThreadAdvertisement(InterfaceType inInterfaceType, const Inet::IPPrefix &inPrefix, bool inStart)
{
    // TODO: implement me
}

#endif // WARM_CONFIG_SUPPORT_THREAD_ROUTING

#if WARM_CONFIG_SUPPORT_BORDER_ROUTING

#error "Weave border router support not implemented"

PlatformResult AddRemoveThreadRoute(InterfaceType inInterfaceType, const Inet::IPPrefix &inPrefix, RoutePriority inPriority, bool inAdd)
{
    // TODO: implement me
}

PlatformResult SetThreadRoutePriority(InterfaceType inInterfaceType, const Inet::IPPrefix &inPrefix, RoutePriority inPriority)
{
    // TODO: implement me
}

#endif // WARM_CONFIG_SUPPORT_BORDER_ROUTING

} // namespace Platform
} // namespace Warm
} // namespace Weave
} // namespace nl

// ==================== WARM Utility Functions ====================

namespace nl {
namespace Weave {
namespace DeviceLayer {
namespace Internal {

WEAVE_ERROR GetLwIPNetifForWarmInterfaceType(InterfaceType inInterfaceType, struct netif *& netif)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    for (netif = netif_list; netif != NULL; netif = netif->next)
    {
        if (inInterfaceType == kInterfaceTypeWiFi &&
            netif->name[0] == WEAVE_DEVICE_CONFIG_LWIP_WIFI_STATION_IF_NAME[0] &&
            netif->name[1] == WEAVE_DEVICE_CONFIG_LWIP_WIFI_STATION_IF_NAME[1])
        {
            ExitNow(err = WEAVE_NO_ERROR);
        }

        if (inInterfaceType == kInterfaceTypeTunnel &&
            netif->name[0] == WEAVE_DEVICE_CONFIG_LWIP_SERVICE_TUN_IF_NAME[0] &&
            netif->name[1] == WEAVE_DEVICE_CONFIG_LWIP_SERVICE_TUN_IF_NAME[1])
        {
            ExitNow(err = WEAVE_NO_ERROR);
        }

        if (inInterfaceType == kInterfaceTypeThread &&
            netif->name[0] == WEAVE_DEVICE_CONFIG_LWIP_THREAD_IF_NAME[0] &&
            netif->name[1] == WEAVE_DEVICE_CONFIG_LWIP_THREAD_IF_NAME[1])
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

} // namespace Internal
} // namespace DeviceLayer
} // namespace Weave
} // namespace nl

