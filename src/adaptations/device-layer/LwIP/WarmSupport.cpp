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
#if LWIP_IPV6_MLD
#include "lwip/mld6.h"
#endif /* LWIP_IPV6_MLD */

#if WARM_CONFIG_SUPPORT_THREAD
#include <Weave/DeviceLayer/ThreadStackManager.h>
#include <Weave/DeviceLayer/OpenThread/OpenThreadUtils.h>
#include <openthread/ip6.h>
#if WARM_CONFIG_SUPPORT_THREAD_ROUTING
#include <openthread/border_router.h>
#endif // WARM_CONFIG_SUPPORT_THREAD_ROUTING
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

#if LWIP_IPV6_MLD
        // If the interface supports IPv6 MLD, join the solicited-node multicast group associated
        // with the assigned address.
        if (LwIPNetifSupportsMLD(netif))
        {
            ip6_addr_t solNodeAddr;
            ip6_addr_set_solicitednode(&solNodeAddr, ip6addr.addr[3]);
            lwipErr = mld6_joingroup_netif(netif, &solNodeAddr);
            err = System::MapErrorLwIP(lwipErr);
            if (err != WEAVE_NO_ERROR)
            {
                WeaveLogError(DeviceLayer, "mld6_joingroup_netif() failed for %s interface: %s",
                        WarmInterfaceTypeToStr(inInterfaceType), nl::ErrorStr(err));
                ExitNow();
            }
        }
#endif /* LWIP_IPV6_MLD */
    }
    else
    {
        lwipErr = netif_remove_ip6_address_with_route(netif, &ip6addr, inPrefixLength);
        // There are two possible errors from netif_remove_ip6_address: ERR_ARG
        // if call was made with wrong arguments, or ERR_VAL if the action could
        // not be performed (e.g. the address was already removed).  We squash
        // ERR_VAL, and return SUCCESS so that WARM can set its state correctly.
        if (lwipErr == ERR_VAL)
        {
            WeaveLogProgress(DeviceLayer, "netif_remove_ip6_address_with_route: Already removed");
            lwipErr = ERR_OK;
        }
        err = System::MapErrorLwIP(lwipErr);
        if (err != WEAVE_NO_ERROR)
        {
            WeaveLogError(DeviceLayer, "netif_remove_ip6_address_with_route() failed for %s interface: %s",
                     WarmInterfaceTypeToStr(inInterfaceType), nl::ErrorStr(err));
            ExitNow();
        }

#if LWIP_IPV6_MLD
        // Leave the solicited-node multicast group associated with the removed address.
        {
            ip6_addr_t solNodeAddr;
            ip6_addr_set_solicitednode(&solNodeAddr, ip6addr.addr[3]);
            mld6_leavegroup_netif(netif, &solNodeAddr);
        }
#endif /* LWIP_IPV6_MLD */
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
        // There are two possible errors from otIp6RemoveUnicastAddress:
        // OT_ERROR_INVALID_ARGS if the address was a multicast address, and
        // OT_ERROR_NOT_FOUND if the address does not exist on the thread
        // interface.  We squash the OT_ERROR_NOT_FOUND so that WARM sets its
        // state correctly.
        if (otErr == OT_ERROR_NOT_FOUND)
        {
            WeaveLogProgress(DeviceLayer, "otIp6RemoveUnicastAddress: already removed");
            otErr = OT_ERROR_NONE;
        }
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

PlatformResult StartStopThreadAdvertisement(InterfaceType inInterfaceType, const Inet::IPPrefix &inPrefix, bool inStart)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    otError otErr;
    otBorderRouterConfig brConfig;

    VerifyOrExit(inInterfaceType == kInterfaceTypeThread, err = WEAVE_ERROR_INVALID_ARGUMENT);
    VerifyOrExit((inPrefix.Length & 7) == 0, err = WEAVE_ERROR_INVALID_ADDRESS);

    ThreadStackMgrImpl().LockThreadStack();

    brConfig.mConfigure = false;
    brConfig.mDefaultRoute = false;
    brConfig.mDhcp = false;
    brConfig.mOnMesh = true;
    brConfig.mPreference = 0;
    brConfig.mPreferred = true;
    brConfig.mPrefix.mLength = inPrefix.Length;
    brConfig.mRloc16 = otThreadGetRloc16(ThreadStackMgrImpl().OTInstance());
    brConfig.mSlaac = false;
    brConfig.mStable = true;
    memcpy(brConfig.mPrefix.mPrefix.mFields.m8, inPrefix.IPAddr.Addr, sizeof(brConfig.mPrefix.mPrefix.mFields));

    if (inStart)
    {
        otErr = otBorderRouterAddOnMeshPrefix(ThreadStackMgrImpl().OTInstance(), &brConfig);
    }
    else
    {
        otErr = otBorderRouterRemoveOnMeshPrefix(ThreadStackMgrImpl().OTInstance(), &brConfig.mPrefix);
        if (otErr == OT_ERROR_NOT_FOUND)
        {
            WeaveLogProgress(DeviceLayer, "otBorderRouterRemoveOnMeshPrefix: already removed");
            otErr = OT_ERROR_NONE;
        }
    }

    ThreadStackMgrImpl().UnlockThreadStack();

    err = MapOpenThreadError(otErr);

exit:
    if (err == WEAVE_NO_ERROR)
    {
#if WEAVE_PROGRESS_LOGGING
        char ipAddrStr[INET6_ADDRSTRLEN];
        inPrefix.IPAddr.ToString(ipAddrStr, sizeof(ipAddrStr));
        WeaveLogProgress(DeviceLayer, "OpenThread OnMesh Prefix %s: %s/%d",
                 (inStart) ? "Added" : "Removed",
                 ipAddrStr, inPrefix.Length);
#endif // WEAVE_PROGRESS_LOGGING
    }
    else
    {
        WeaveLogError(DeviceLayer, "StartStopThreadAdvertisement() failed: %s", ::nl::ErrorStr(err));
    }

    return (err == WEAVE_NO_ERROR) ? kPlatformResultSuccess : kPlatformResultFailure;
}

#endif // WARM_CONFIG_SUPPORT_THREAD_ROUTING

#if WARM_CONFIG_SUPPORT_BORDER_ROUTING

PlatformResult AddRemoveThreadRoute(InterfaceType inInterfaceType, const Inet::IPPrefix &inPrefix, RoutePriority inPriority, bool inAdd)
{
    otError otErr;
    otExternalRouteConfig routeConfig;

    int ot_priority = OT_ROUTE_PREFERENCE_HIGH;
    switch (inPriority)
    {
    case kRoutePriorityLow:
        ot_priority = OT_ROUTE_PREFERENCE_LOW;
        break;
    case kRoutePriorityHigh:
        ot_priority = OT_ROUTE_PREFERENCE_HIGH;
        break;
    case kRoutePriorityMedium:
        // Let's set route priority to medium by default.
    default:
        ot_priority = OT_ROUTE_PREFERENCE_MED;
        break;
    }

    ThreadStackMgrImpl().LockThreadStack();

    otBorderRouterRegister(ThreadStackMgrImpl().OTInstance());

    memcpy(routeConfig.mPrefix.mPrefix.mFields.m8, inPrefix.IPAddr.Addr, sizeof(routeConfig.mPrefix.mPrefix.mFields));
    routeConfig.mPrefix.mLength = inPrefix.Length;
    routeConfig.mStable = true;
    routeConfig.mPreference = ot_priority;

    if (inAdd)
    {
        otErr = otBorderRouterAddRoute(ThreadStackMgrImpl().OTInstance(), &routeConfig);
    }
    else
    {
        otErr = otBorderRouterRemoveRoute(ThreadStackMgrImpl().OTInstance(), &routeConfig.mPrefix);
    }

    ThreadStackMgrImpl().UnlockThreadStack();

    if (otErr == OT_ERROR_NONE)
    {
#if WEAVE_PROGRESS_LOGGING
        char ipAddrStr[INET6_ADDRSTRLEN];
        inPrefix.IPAddr.ToString(ipAddrStr, sizeof(ipAddrStr));
        WeaveLogProgress(DeviceLayer, "OpenThread Border Router Route %s: %s/%d",
                 (inAdd) ? "Added" : "Removed",
                 ipAddrStr, inPrefix.Length);
#endif // WEAVE_PROGRESS_LOGGING
    }
    else
    {
        WeaveLogError(DeviceLayer, "AddRemoveThreadRoute() failed: %s", ::nl::ErrorStr(MapOpenThreadError(otErr)));
    }

    return (otErr == OT_ERROR_NONE) ? kPlatformResultSuccess : kPlatformResultFailure;
}

PlatformResult SetThreadRoutePriority(InterfaceType inInterfaceType, const Inet::IPPrefix &inPrefix, RoutePriority inPriority)
{
    return AddRemoveThreadRoute(inInterfaceType, inPrefix, inPriority, true);
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

bool LwIPNetifSupportsMLD(struct netif * netif)
{
    // Determine if the given netif supports IPv6 MLD.  Unfortunately, the LwIP MLD6 netif flag
    // is an unreliable indication of MLD support in older versions of LwIP.
    return (((netif->flags & NETIF_FLAG_MLD6) != 0) ||
            (netif->name[0] == WEAVE_DEVICE_CONFIG_LWIP_WIFI_STATION_IF_NAME[0] &&
             netif->name[1] == WEAVE_DEVICE_CONFIG_LWIP_WIFI_STATION_IF_NAME[1]));
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
