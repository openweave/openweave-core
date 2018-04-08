#include <internal/WeavePlatformInternal.h>
#include <ConnectivityManager.h>
#include <Warm/Warm.h>
#include <lwip/netif.h>
#include <lwip/ip6_route_table.h>

using namespace ::WeavePlatform;
using namespace ::WeavePlatform::Internal;

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
            ESP_LOGE(TAG, "netif_add_ip6_address_with_route() failed for %s interface: %s",
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
            ESP_LOGE(TAG, "netif_remove_ip6_address_with_route() failed for %s interface: %s",
                     WarmInterfaceTypeToStr(inInterfaceType), nl::ErrorStr(err));
            ExitNow();
        }
    }

    UNLOCK_TCPIP_CORE();
    lockHeld = false;

    if (LOG_LOCAL_LEVEL >= ESP_LOG_INFO)
    {
        char ipAddrStr[INET6_ADDRSTRLEN];
        inAddress.ToString(ipAddrStr, sizeof(ipAddrStr));
        ESP_LOGI(TAG, "%s %s on %s interface: %s/%" PRId8,
                 (inAdd) ? "Adding" : "Removing",
                 CharacterizeIPv6Address(inAddress),
                 WarmInterfaceTypeToStr(inInterfaceType),
                 ipAddrStr, inPrefixLength);
    }

exit:
    if (lockHeld)
    {
        UNLOCK_TCPIP_CORE();
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
            ESP_LOGE(TAG, "ip6_add_route_entry() failed for %s interface: %s",
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

    if (LOG_LOCAL_LEVEL >= ESP_LOG_INFO)
    {
        char prefixAddrStr[INET6_ADDRSTRLEN];
        inPrefix.IPAddr.ToString(prefixAddrStr, sizeof(prefixAddrStr));
        const char * prefixDesc = CharacterizeIPv6Prefix(inPrefix);
        ESP_LOGI(TAG, "IPv6 route%s%s %s %s interface: %s/%" PRId8,
                 (prefixDesc != NULL) ? " for " : "",
                 (prefixDesc != NULL) ? prefixDesc : "",
                 (inAdd) ? "added to" : "removed from",
                 WarmInterfaceTypeToStr(inInterfaceType),
                 prefixAddrStr, inPrefix.Length);
    }

exit:
    if (lockHeld)
    {
        UNLOCK_TCPIP_CORE();
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
    WEAVE_ERROR err;

    // Get the LwIP netif structure for the specified interface type.
    if (inInterfaceType == kInterfaceTypeWiFi)
    {
        err = tcpip_adapter_get_netif(TCPIP_ADAPTER_IF_STA, (void **)&netif);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "tcpip_adapter_get_netif(TCPIP_ADAPTER_IF_STA) failed: %s", nl::ErrorStr(err));
            ExitNow();
        }
    }
    else if (inInterfaceType == kInterfaceTypeTunnel)
    {
        // TODO: implement this
        ExitNow(err = WEAVE_ERROR_NOT_IMPLEMENTED);
    }
    else
    {
        ESP_LOGE(TAG, "Invalid interface type: %d", (int)inInterfaceType);
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
    }

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
        if (::WeavePlatform::FabricState.FabricId != kFabricIdNotSpecified &&
            inPrefix.IPAddr.GlobalId() == nl::Weave::WeaveFabricIdToIPv6GlobalId(::WeavePlatform::FabricState.FabricId))
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

