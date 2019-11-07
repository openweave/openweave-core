/*
 *
 *    Copyright (c) 2013-2018 Nest Labs, Inc.
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
 *      This file implements constants, globals and interfaces common
 *      to and used by all Weave test applications and tools.
 *
 *      NOTE: These do not comprise a public part of the Weave API and
 *            are subject to change without notice.
 *
 */

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <inttypes.h>
#include <stdint.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <new>
#include <vector>
#include "ToolCommon.h"
#include "TestGroupKeyStore.h"
#include <Weave/Support/NestCerts.h>

#include <SystemLayer/SystemTimer.h>
#include <SystemLayer/SystemFaultInjection.h>
#include <Weave/Support/WeaveFaultInjection.h>
#include <InetLayer/InetFaultInjection.h>
#include <Weave/Support/crypto/WeaveCrypto.h>
#include <Weave/Support/crypto/WeaveRNG.h>

#if WEAVE_CONFIG_ENABLE_TUNNELING
#include <InetLayer/TunEndPoint.h>
#include <Weave/Profiles/weave-tunneling/WeaveTunnelAgent.h>
#endif // WEAVE_CONFIG_ENABLE_TUNNELING

#include "CASEOptions.h"
#include "KeyExportOptions.h"
#include "TAKEOptions.h"
#include "DeviceDescOptions.h"
#include "MockPlatformClocks.h"

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
#include "TapAddrAutoconf.h"
#include <lwip/netif.h>
#include <netif/etharp.h>
#include "TapInterface.h"
#include <lwip/tcpip.h>
#include <lwip/sys.h>
#include <lwip/dns.h>
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

#if CONFIG_BLE_PLATFORM_BLUEZ

static BleLayer sBle;
static nl::Ble::Platform::BlueZ::BluezBleApplicationDelegate sBleApplicationDelegate;
static nl::Ble::Platform::BlueZ::BluezBlePlatformDelegate sBlePlatformDelegate(&sBle);

nl::Ble::Platform::BlueZ::BluezBleApplicationDelegate *getBluezApplicationDelegate()
{
    return &sBleApplicationDelegate;
}

nl::Ble::Platform::BlueZ::BluezBlePlatformDelegate *getBluezPlatformDelegate()
{
    return &sBlePlatformDelegate;
}

#endif /* CONFIG_BLE_PLATFORM_BLUEZ */

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS
#include <arpa/inet.h>
#include <sys/select.h>
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

#if WEAVE_SYSTEM_CONFIG_USE_LWIP

static sys_mbox* sLwIPEventQueue = NULL;
static unsigned int sLwIPAcquireCount = 0;

static void AcquireLwIP(void)
{
    if (sLwIPAcquireCount++ == 0) {
        sys_mbox_new(&sLwIPEventQueue, 100);
    }
}

static void ReleaseLwIP(void)
{
    if (sLwIPAcquireCount > 0 && --sLwIPAcquireCount == 0) {
        tcpip_finish(NULL, NULL);
    }
}
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

System::Layer SystemLayer;

InetLayer Inet;

#if WEAVE_SYSTEM_CONFIG_USE_LWIP && !WEAVE_SYSTEM_CONFIG_USE_SOCKETS
std::vector<TapInterface> tapIFs;
std::vector<struct netif> netIFs; // interface to filter
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP && !WEAVE_SYSTEM_CONFIG_USE_SOCKETS

#if WEAVE_SYSTEM_CONFIG_USE_LWIP

static bool NetworkIsReady();
static void OnLwIPInitComplete(void *arg);
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

char DefaultTapDeviceName[32];
bool Done = false;
bool gSigusr1Received = false;

uint16_t sTestDefaultUDPSessionKeyId = WeaveKeyId::MakeSessionKeyId(1);
uint16_t sTestDefaultTCPSessionKeyId = WeaveKeyId::MakeSessionKeyId(2);
uint16_t sTestDefaultSessionKeyId = WeaveKeyId::MakeSessionKeyId(42);

bool sSuppressAccessControls = false;

// Perform general *non-network* initialization for test applications
void InitToolCommon()
{
    WEAVE_ERROR err;
    unsigned int randSeed;

    // Initialize the platform secure random data source.  If the underlying random data source is OpenSSL,
    // entropy will be acquired via the standard OpenSSL source and the entropy function argument will be
    // ignored.  If the underlying random source is the Nest DRBG implementation, or another similar platform
    // implementation, entropy will be sourced from /dev/(u)random using the GetDRBGSeedDevRandom() function.
    err = nl::Weave::Platform::Security::InitSecureRandomDataSource(NULL, 64, NULL, 0);
    FAIL_ERROR(err, "InitSecureRandomDataSource() failed");

    // Initialized the rand() generator with a seed from the secure random data source.
    err = nl::Weave::Platform::Security::GetSecureRandomData((uint8_t *)&randSeed, sizeof(randSeed));
    FAIL_ERROR(err, "Random number generator seeding failed");
    srand(randSeed);

    UseStdoutLineBuffering();

    // Force the linker to link the mock versions of the platform time functions.  This overrides the default
    // platform implementations supplied in the Weave library.
    MockPlatform::gMockPlatformClocks.GetClock_Monotonic();
}

static void ExitOnSIGUSR1Handler(int signum)
{
    // exit() allows us a slightly better clean up (gcov data) than SIGINT's exit
    exit(0);
}

// We set a hook to exit when we receive SIGUSR1, SIGTERM or SIGHUP
void SetSIGUSR1Handler(void)
{
    SetSignalHandler(ExitOnSIGUSR1Handler);
}

void DoneOnHandleSIGUSR1(int signum)
{
    Done = true;
    gSigusr1Received = true;
}

void SetSignalHandler(SignalHandler handler)
{
    struct sigaction sa;
    int signals[] = { SIGUSR1 };
    size_t i;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handler;

    for (i = 0; i < sizeof(signals)/sizeof(signals[0]); i++)
    {
        if (sigaction(signals[i], &sa, NULL) == -1)
        {
            perror("Can't catch signal");
            exit(1);
        }
    }
}

void UseStdoutLineBuffering()
{
    // Set stdout to be line buffered with a buffer of 512 (will flush on new line
    // or when the buffer of 512 is exceeded).
    setvbuf(stdout, NULL, _IOLBF, 512);
}

#if WEAVE_CONFIG_ENABLE_TUNNELING

#if !WEAVE_TUNNEL_CONFIG_WILL_OVERRIDE_ADDR_ROUTING_FUNCS

#if !WEAVE_SYSTEM_CONFIG_USE_LWIP
    /*
     * Some structs are defined redundantly in netinet/in.h and linux/ipv6.h.
     * So, cannot include both headers. Define struct in6_ifreq here.
     * Copied from linux/ipv6.h
     */
    struct in6_ifreq
    {
        struct    in6_addr ifr6_addr;
        uint32_t  ifr6_prefixlen;
        int       ifr6_ifindex;
    };
#endif

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
static INET_ERROR InterfaceAddAddress_LwIP(InterfaceId tunIf, IPAddress ipAddr, uint8_t prefixLen)
{
    INET_ERROR err = INET_NO_ERROR;

    // Lock LwIP stack

    LOCK_TCPIP_CORE();

    int8_t index;
    ip6_addr_t ip6_addr;

    ip6_addr = ipAddr.ToIPv6();

    if (ip6_addr_islinklocal(&ip6_addr))
    {
#if LWIP_VERSION_MAJOR > 1
        ip6_addr_set(ip_2_ip6(&tunIf->ip6_addr[0]), &ip6_addr);
#else // LWIP_VERSION_MAJOR <= 1
        ip6_addr_set(&tunIf->ip6_addr[0], &ip6_addr);
#endif // LWIP_VERSION_MAJOR <= 1
        index = 0;
    }
    else
    {
        // Add an address with a prefix route into the route table.

        err = System::MapErrorLwIP(netif_add_ip6_address_with_route(tunIf, &ip6_addr, prefixLen, &index));
    }

    // Explicitly set this address as valid to bypass DAD

    if (index >= 0)
    {
        netif_ip6_addr_set_state(tunIf, index, IP6_ADDR_VALID);
    }

    // UnLock LwIP stack

    UNLOCK_TCPIP_CORE();

    return err;
}

static INET_ERROR InterfaceRemoveAddress_LwIP(InterfaceId tunIf, IPAddress ipAddr, uint8_t prefixLen)
{
    INET_ERROR err = INET_NO_ERROR;

    // Lock LwIP stack

    LOCK_TCPIP_CORE();

    ip6_addr_t ip6_addr;
    ip6_addr = ipAddr.ToIPv6();

    if (ip6_addr_islinklocal(&ip6_addr))
    {
#if LWIP_VERSION_MAJOR > 1
        ip6_addr_set_zero(ip_2_ip6(&tunIf->ip6_addr[0]));
#else // LWIP_VERSION_MAJOR <= 1
        ip6_addr_set_zero(&tunIf->ip6_addr[0]);
#endif // LWIP_VERSION_MAJOR <= 1
    }
    else
    {
        /**
         * Look for the address among the list of configured ones on the device
         * and delete it.
         */
        err = System::MapErrorLwIP(netif_remove_ip6_address_with_route(tunIf, &ip6_addr, prefixLen));
    }

    // UnLock LwIP stack

    UNLOCK_TCPIP_CORE();

    return err;
}

static INET_ERROR SetRouteToTunnelInterface_LwIP(InterfaceId tunIf, IPPrefix ipPrefix, nl::Inet::TunEndPoint::RouteOp routeAddDel)
{
    INET_ERROR err = INET_NO_ERROR;
    struct ip6_prefix ip6_prefix;

    // Lock LwIP stack

    LOCK_TCPIP_CORE();

    ip6_prefix.addr = ipPrefix.IPAddr.ToIPv6();
    ip6_prefix.prefix_len = ipPrefix.Length;
    if (routeAddDel == nl::Inet::TunEndPoint::kRouteTunIntf_Add)
    {
        err = System::MapErrorLwIP(ip6_add_route_entry(&ip6_prefix, tunIf, NULL, NULL));
    }
    else
    {
        ip6_remove_route_entry(&ip6_prefix);
    }

    // UnLock LwIP stack

    UNLOCK_TCPIP_CORE();

    return err;
}
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

#if !WEAVE_SYSTEM_CONFIG_USE_LWIP
static INET_ERROR InterfaceAddAddress_Linux(InterfaceId tunIf, IPAddress ipAddr, uint8_t prefixLen)
{
    INET_ERROR err = INET_NO_ERROR;
    int ret    = -1;
    int sockfd = INET_INVALID_SOCKET_FD;
    struct in6_ifreq ifr6;
    uint8_t *p = NULL;

    p = &(ifr6.ifr6_addr.s6_addr[0]);
    for (int i = 0; i < 4; i++)
    {
        nl::Weave::Encoding::LittleEndian::Write32(p, ipAddr.Addr[i]);
    }

    sockfd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_IP);
    if (sockfd < 0)
    {
        ExitNow(err = System::MapErrorPOSIX(errno));
    }

    // Copy over the interface index to the in6_ifreq

    ifr6.ifr6_ifindex   = tunIf;
    ifr6.ifr6_prefixlen = prefixLen;

    // Set the v6 address on the interface

    ret = ioctl(sockfd, SIOCSIFADDR, &ifr6);
    if (ret != 0 && errno != EALREADY && errno != EEXIST)
    {
        ExitNow(err = System::MapErrorPOSIX(errno));
    }

exit:
    if (sockfd >= 0)
    {
        close(sockfd);
    }

    return err;
}

static INET_ERROR InterfaceRemoveAddress_Linux(InterfaceId tunIf, IPAddress ipAddr, uint8_t prefixLen)
{
    INET_ERROR err = INET_NO_ERROR;
    int ret     = -1;
    int sockfd  = INET_INVALID_SOCKET_FD;
    struct in6_ifreq ifr6;
    uint8_t *p = NULL;

    sockfd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_IP);
    if (sockfd < 0)
    {
        ExitNow(err = System::MapErrorPOSIX(errno));
    }

    p = &(ifr6.ifr6_addr.s6_addr[0]);
    for (int i = 0; i < 4; i++)
    {
        nl::Weave::Encoding::LittleEndian::Write32(p, ipAddr.Addr[i]);
    }

    // Copy over the interface index to the in6_ifreq

    ifr6.ifr6_ifindex   = tunIf;
    ifr6.ifr6_prefixlen = prefixLen;

    ret = ioctl(sockfd, SIOCDIFADDR, &ifr6);
    if (ret != 0 && errno != ENOENT)
    {
        ExitNow(err = System::MapErrorPOSIX(errno));
    }

exit:
    if (sockfd >= 0)
    {
        close(sockfd);
    }

    return err;
}

static INET_ERROR SetRouteToTunnelInterface_Linux(InterfaceId tunIf, IPPrefix ipPrefix, nl::Inet::TunEndPoint::RouteOp routeAddDel)
{
    INET_ERROR err = INET_NO_ERROR;
    int ret     = -1;
    int sockfd  = INET_INVALID_SOCKET_FD;
    struct ::in6_rtmsg route;

    memset(&route, 0, sizeof(struct in6_rtmsg));

    route.rtmsg_dst = ipPrefix.IPAddr.ToIPv6();
    route.rtmsg_dst_len = ipPrefix.Length;

    // Fill in in6_rtmsg flags

    route.rtmsg_flags = RTF_UP;
    if (ipPrefix.Length == NL_INET_IPV6_MAX_PREFIX_LEN)
    {
        route.rtmsg_flags |= RTF_HOST;
    }
    route.rtmsg_metric = 1;

    sockfd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_IP);
    if (sockfd < 0)
    {
        ExitNow(err = System::MapErrorPOSIX(errno));
    }

    route.rtmsg_ifindex = tunIf;

    if (routeAddDel == nl::Inet::TunEndPoint::kRouteTunIntf_Add)
    {
        // Add the route to the kernel

        ret = ioctl(sockfd, SIOCADDRT, &route);
        if (ret != 0 && errno != EALREADY && errno != EEXIST)
        {
            ExitNow(err = System::MapErrorPOSIX(errno));
        }
    }
    else
    {
        // Delete the route from the kernel

        ret = ioctl(sockfd, SIOCDELRT, &route);
        if (ret != 0 && errno != EALREADY && errno != ENOENT)
        {
            ExitNow(err = System::MapErrorPOSIX(errno));
        }
    }

exit:
    if (sockfd >= 0)
    {
        close(sockfd);
    }

    return err;
}
#endif // !WEAVE_SYSTEM_CONFIG_USE_LWIP

/**
 * Add an IPv6 address to the tunnel interface.
 *
 * @note
 *  On some platforms, this method returns \c INET_NO_ERROR even when
 *  insufficient space is available for another address.
 *
 * @param[in]  tunIf          The tunnel interface identifier.
 *
 * @param[in] ipAddr          IPAddress object holding the IPv6 address to be assigned.
 *
 * @param[in] prefixLen       prefix length for on-link route for this address.
 *
 * @return INET_NO_ERROR      success: address is added.
 * @retval  other             another system or platform error
 */
INET_ERROR InterfaceAddAddress(InterfaceId tunIf, IPAddress ipAddr, uint8_t prefixLen)
{
    INET_ERROR err = INET_NO_ERROR;
#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    err = InterfaceAddAddress_LwIP(tunIf, ipAddr, prefixLen);
#else
    err = InterfaceAddAddress_Linux(tunIf, ipAddr, prefixLen);
#endif
    SuccessOrExit(err);

exit:
    return err;
}

/**
 * Remove an IPv6 address from the tunnel interface.
 *
 * @param[in]   tunIf         The tunnel interface identifier.
 *
 * @param[in]   ipAddr        the IPAddress to remove.
 *
 * @param[in]   prefixLen     prefix length for on-link route for this address.
 *
 * @retval  INET_NO_ERROR     success: address is removed.
 * @retval  other             another system or platform error.
 */
INET_ERROR InterfaceRemoveAddress(InterfaceId tunIf, IPAddress ipAddr, uint8_t prefixLen)
{
    INET_ERROR err = INET_NO_ERROR;
#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    err = InterfaceRemoveAddress_LwIP(tunIf, ipAddr, prefixLen);
#else
    err = InterfaceRemoveAddress_Linux(tunIf, ipAddr, prefixLen);
#endif
    SuccessOrExit(err);

exit:
    return err;
}

/**
 * Add/Remove an IPv6 route pointing to the tunnel interface.
 *
 * @note
 *  The IP address prefix must be an IPv6 address prefix.
 *
 * @param[in] tunIf             The tunnel interface identifier.
 *
 * @param[in] ipPrefix          IPPrefix to route.
 *
 * @param[in] routeAddDel       Flag indicating route addition or deletion.
 *
 * @retval  INET_NO_ERROR       success: route operation performed
 * @retval  other               another system or platform error
 */
INET_ERROR SetRouteToTunnelInterface(InterfaceId tunIf, IPPrefix ipPrefix, nl::Inet::TunEndPoint::RouteOp routeAddDel)
{
    INET_ERROR err = INET_NO_ERROR;

    // Only allow prefix length up to 64 bits

    if (ipPrefix.Length > NL_INET_IPV6_DEFAULT_PREFIX_LEN)
    {
        ExitNow(err = INET_ERROR_BAD_ARGS);
    }

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    err = SetRouteToTunnelInterface_LwIP(tunIf, ipPrefix, routeAddDel);
#else
    err = SetRouteToTunnelInterface_Linux(tunIf, ipPrefix, routeAddDel);
#endif
    SuccessOrExit(err);

exit:
    return err;
}

/**
 * This is a default implementation in a Standalone setting of the Weave Addressing and
 * Routing(WARM) platform adaptation when the Tunnel interface is brought up. This may
 * be overridden by asserting the preprocessor definition,
 * #WEAVE_TUNNEL_CONFIG_WILL_OVERRIDE_ADDR_ROUTING_FUNCS.
 *
 * @param[in]  tunIf      The tunnel interface identifier.
 *
 */
void nl::Weave::Profiles::WeaveTunnel::Platform::TunnelInterfaceUp(InterfaceId tunIf)
{
   WEAVE_ERROR err = WEAVE_NO_ERROR;
   uint64_t globalId = 0;
   IPAddress tunULAAddr;

   /*
    * Add the WiFi interface ULA address to the tunnel interface to ensure the selection of
    * a Weave ULA as the source address for packets originating on the local node but destined
    * for addresses reachable via the tunnel. Without this, the default IPv6 source address
    * selection algorithm might choose an inappropriate source address, making it impossible
    * for the destination node to respond.
    */
   globalId = WeaveFabricIdToIPv6GlobalId(ExchangeMgr.FabricState->FabricId);
   tunULAAddr = IPAddress::MakeULA(globalId, kWeaveSubnetId_PrimaryWiFi,
                                   nl::Weave::WeaveNodeIdToIPv6InterfaceId(ExchangeMgr.FabricState->LocalNodeId));
   err = InterfaceAddAddress(tunIf, tunULAAddr, NL_INET_IPV6_MAX_PREFIX_LEN);
   if (err != WEAVE_NO_ERROR)
   {
       WeaveLogError(WeaveTunnel, "Failed to add host address to Weave tunnel interface\n");
   }
}

/**
 * This is a default implementation in a Standalone setting of the Weave Addressing and
 * Routing(WARM) platform adaptation when the Tunnel interface is brought down. This may
 * be overridden by asserting the preprocessor definition,
 * #WEAVE_TUNNEL_CONFIG_WILL_OVERRIDE_ADDR_ROUTING_FUNCS.
 *
 * @param[in]  tunIf      The tunnel interface identifier.
 *
 */
void nl::Weave::Profiles::WeaveTunnel::Platform::TunnelInterfaceDown(InterfaceId tunIf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint64_t globalId = 0;
    IPAddress tunULAAddr;

    /*
     * Remove the WiFi interface ULA address to the tunnel interface added during
     * TunnelInterfaceUp() call.
     */
    globalId = WeaveFabricIdToIPv6GlobalId(ExchangeMgr.FabricState->FabricId);
    tunULAAddr = IPAddress::MakeULA(globalId, kWeaveSubnetId_PrimaryWiFi,
                                    nl::Weave::WeaveNodeIdToIPv6InterfaceId(ExchangeMgr.FabricState->LocalNodeId));
    err = InterfaceRemoveAddress(tunIf, tunULAAddr, NL_INET_IPV6_MAX_PREFIX_LEN);
    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogError(WeaveTunnel, "Failed to remove host address from Weave tunnel interface\n");
    }
}

/**
 * This is a default implementation in a Standalone setting of the Weave Addressing and
 * Routing(WARM) platform adaptation when the Service tunnel connection is established.
 * This may be overridden by asserting the preprocessor definition,
 * #WEAVE_TUNNEL_CONFIG_WILL_OVERRIDE_ADDR_ROUTING_FUNCS.
 *
 * @param[in]  tunIf      The tunnel interface identifier.
 *
 * @param[in]  tunMode    The mode(Primary, PrimaryAndBackup, BackupOnly) in which the
 *                         tunnel is established.
 *
 */
void nl::Weave::Profiles::WeaveTunnel::Platform::ServiceTunnelEstablished(InterfaceId tunIf,
                                                                          TunnelAvailabilityMode tunMode)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    IPPrefix prefix;
    uint64_t globalId = 0;
    IPAddress tunULAAddr;

    // Create prefix fd<globalId>::/48 to install route to tunnel interface

    globalId = WeaveFabricIdToIPv6GlobalId(ExchangeMgr.FabricState->FabricId);
    tunULAAddr = IPAddress::MakeULA(globalId, 0, 0);

    prefix.IPAddr = tunULAAddr;
    prefix.Length = WEAVE_ULA_FABRIC_DEFAULT_PREFIX_LEN;

    // Add route to tunnel interface
    err = SetRouteToTunnelInterface(tunIf, prefix, TunEndPoint::kRouteTunIntf_Add);
    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogError(WeaveTunnel, "Failed to add Weave tunnel route\n");
    }
}

/**
 * This is a default implementation in a Standalone setting of the Weave Addressing and
 * Routing(WARM) platform adaptation when Border Routing is enabled in Thread or any
 * peripheral network.
 * This may be overridden by asserting the preprocessor definition,
 * #WEAVE_TUNNEL_CONFIG_WILL_OVERRIDE_ADDR_ROUTING_FUNCS.
 *
 */
void nl::Weave::Profiles::WeaveTunnel::Platform::EnableBorderRouting(void)
{
    WeaveLogDetail(WeaveTunnel, "Border Routing enabled\n");
}

/**
 * This is a default implementation in a Standalone setting of the Weave Addressing and
 * Routing(WARM) platform adaptation when Border Routing is disabled in Thread or any
 * peripheral network.
 * This may be overridden by asserting the preprocessor definition,
 * #WEAVE_TUNNEL_CONFIG_WILL_OVERRIDE_ADDR_ROUTING_FUNCS.
 *
 */
void nl::Weave::Profiles::WeaveTunnel::Platform::DisableBorderRouting(void)
{
    WeaveLogDetail(WeaveTunnel, "Border Routing disabled\n");
}

/**
 * This is a default implementation in a Standalone setting of the Weave Addressing and
 * Routing(WARM) platform adaptation when the Service tunnel connection is torn down.
 * This may be overridden by asserting the preprocessor definition,
 * #WEAVE_TUNNEL_CONFIG_WILL_OVERRIDE_ADDR_ROUTING_FUNCS.
 *
 * @param[in]  tunIf      The tunnel interface identifier.
 *
 */
void nl::Weave::Profiles::WeaveTunnel::Platform::ServiceTunnelDisconnected(InterfaceId tunIf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    IPPrefix prefix;
    uint64_t globalId = 0;
    IPAddress tunULAAddr;

    // Delete route to tunnel interface for prefix fd<globalId>::/48

    globalId = WeaveFabricIdToIPv6GlobalId(ExchangeMgr.FabricState->FabricId);
    tunULAAddr = IPAddress::MakeULA(globalId, 0, 0);

    prefix.IPAddr = tunULAAddr;
    prefix.Length = WEAVE_ULA_FABRIC_DEFAULT_PREFIX_LEN;

    // Delete route

    err = SetRouteToTunnelInterface(tunIf, prefix, TunEndPoint::kRouteTunIntf_Del);
    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogError(WeaveTunnel, "Failed to remove Weave tunnel route\n");
    }
}

/**
 * This is a default implementation in a Standalone setting of the Weave Addressing and
 * Routing(WARM) platform adaptation when the Service tunnel connection changes mode.
 * This may be overridden by asserting the preprocessor definition,
 * #WEAVE_TUNNEL_CONFIG_WILL_OVERRIDE_ADDR_ROUTING_FUNCS.
 *
 * @param[in]  tunIf      The tunnel interface identifier.
 *
 * @param[in]  tunMode     The mode(Primary, PrimaryAndBackup, BackupOnly) to which the
 *                         tunnel connection has been changed to.
 *
 */
void nl::Weave::Profiles::WeaveTunnel::Platform::ServiceTunnelModeChange(InterfaceId tunIf,
                                                                         TunnelAvailabilityMode tunMode)
{
    (void)tunIf;
    (void)tunMode;
}
#endif // WEAVE_TUNNEL_CONFIG_WILL_OVERRIDE_ADDR_ROUTING_FUNCS

#endif // WEAVE_CONFIG_ENABLE_TUNNELING

void InitSystemLayer()
{
#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    AcquireLwIP();
    SystemLayer.Init(sLwIPEventQueue);
#else // !WEAVE_SYSTEM_CONFIG_USE_LWIP
    SystemLayer.Init(NULL);
#endif // !WEAVE_SYSTEM_CONFIG_USE_LWIP
}

void ShutdownSystemLayer()
{
    SystemLayer.Shutdown();

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    ReleaseLwIP();
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP
}

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
static void PrintNetworkState()
{
    char intfName[10];

    for (size_t j = 0; j < gNetworkOptions.TapDeviceName.size(); j++)
    {
        struct netif *netIF = &(netIFs[j]);
        TapInterface *tapIF = &(tapIFs[j]);

        GetInterfaceName(netIF, intfName, sizeof(intfName));

        printf("LwIP interface ready\n");
        printf("  Interface Name: %s\n", intfName);
        printf("  Tap Device: %s\n", gNetworkOptions.TapDeviceName[j]);
        printf("  MAC address: %02x:%02x:%02x:%02x:%02x:%02x\n", tapIF->macAddr[0], tapIF->macAddr[1], tapIF->macAddr[2], tapIF->macAddr[3], tapIF->macAddr[4], tapIF->macAddr[5]);
#if INET_CONFIG_ENABLE_IPV4
        printf("  IPv4 Address: %s\n", ipaddr_ntoa(&(netIF->ip_addr)));
        printf("  IPv4 Mask: %s\n", ipaddr_ntoa(&(netIF->netmask)));
        printf("  IPv4 Gateway: %s\n", ipaddr_ntoa(&(netIF->gw)));
#endif // INET_CONFIG_ENABLE_IPV4
        for (int i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++)
        {
            if (!ip6_addr_isany(netif_ip6_addr(netIF, i)))
            {
                printf("  IPv6 address: %s, 0x%02x\n", ip6addr_ntoa(netif_ip6_addr(netIF, i)), netif_ip6_addr_state(netIF, i));
            }
        }
    }
#if INET_CONFIG_ENABLE_DNS_RESOLVER
    char dnsServerAddrStr[DNS_MAX_NAME_LENGTH];
    printf("  DNS Server: %s\n", gNetworkOptions.DNSServerAddr.ToString(dnsServerAddrStr, sizeof(dnsServerAddrStr)));
#endif // INET_CONFIG_ENABLE_DNS_RESOLVER
}
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

void InitNetwork()
{
    void* lContext = NULL;

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS

    tcpip_init(NULL, NULL);

#else // !WEAVE_SYSTEM_CONFIG_USE_SOCKETS

    // If an tap device name hasn't been specified, derive one from the IPv6 interface id.

    if (gNetworkOptions.TapDeviceName.empty())
    {
        for (size_t j = 0; j < gNetworkOptions.LocalIPv6Addr.size(); j++)
        {
            uint64_t iid = gNetworkOptions.LocalIPv6Addr[j].InterfaceId();
            char * tap_name = (char *)malloc(sizeof(DefaultTapDeviceName));
            snprintf(tap_name, sizeof(DefaultTapDeviceName), "weave-dev-%" PRIx64, iid & 0xFFFF);
            tap_name[ sizeof(DefaultTapDeviceName) - 1] = 0;
            gNetworkOptions.TapDeviceName.push_back(tap_name);
        }
    }

    err_t lwipErr;

    tapIFs.clear();
    netIFs.clear();
    for (size_t j = 0; j < gNetworkOptions.TapDeviceName.size(); j++)
    {
        TapInterface tapIF;
        struct netif netIF;
        tapIFs.push_back(tapIF);
        netIFs.push_back(netIF);
    }

    for (size_t j = 0; j < gNetworkOptions.TapDeviceName.size(); j++)
    {
        lwipErr = TapInterface_Init(&(tapIFs[j]), gNetworkOptions.TapDeviceName[j], NULL);
        if (lwipErr != ERR_OK)
        {
            printf("Failed to initialize tap device %s: %s\n", gNetworkOptions.TapDeviceName[j], ErrorStr(System::MapErrorLwIP(lwipErr)));
            exit(EXIT_FAILURE);
        }
    }
    tcpip_init(OnLwIPInitComplete, NULL);

    // Lock LwIP stack
    LOCK_TCPIP_CORE();

    for (size_t j = 0; j < gNetworkOptions.TapDeviceName.size(); j++)
    {
        std::vector<char *>addrsVec;
        addrsVec.clear();
        if (gNetworkOptions.TapUseSystemConfig)
        {
            CollectTapAddresses(addrsVec, gNetworkOptions.TapDeviceName[j]);
        }

#if INET_CONFIG_ENABLE_IPV4

        IPAddress ip4Addr = (j < gNetworkOptions.LocalIPv4Addr.size())
            ? gNetworkOptions.LocalIPv4Addr[j]
            : IPAddress::Any;
        for (size_t n = 0; n < addrsVec.size(); n++)
        {
            IPAddress auto_addr;
            if (IPAddress::FromString(addrsVec[n], auto_addr) && auto_addr.IsIPv4())
            {
                ip4Addr = auto_addr;
            }
        }

        IPAddress ip4Gateway = (j < gNetworkOptions.IPv4GatewayAddr.size())
            ? gNetworkOptions.IPv4GatewayAddr[j]
            : IPAddress::Any;

        {
#if LWIP_VERSION_MAJOR > 1
            ip4_addr_t ip4AddrLwIP, ip4NetmaskLwIP, ip4GatewayLwIP;
#else // LWIP_VERSION_MAJOR <= 1
            ip_addr_t ip4AddrLwIP, ip4NetmaskLwIP, ip4GatewayLwIP;
#endif // LWIP_VERSION_MAJOR <= 1

            ip4AddrLwIP = ip4Addr.ToIPv4();
            IP4_ADDR(&ip4NetmaskLwIP, 255, 255, 255, 0);
            ip4GatewayLwIP = ip4Gateway.ToIPv4();
            netif_add(&(netIFs[j]), &ip4AddrLwIP, &ip4NetmaskLwIP, &ip4GatewayLwIP, &(tapIFs[j]), TapInterface_SetupNetif, tcpip_input);
        }

#endif // INET_CONFIG_ENABLE_IPV4

        netif_create_ip6_linklocal_address(&(netIFs[j]), 1);

        if (j < gNetworkOptions.LocalIPv6Addr.size())
        {
            ip6_addr_t ip6addr = gNetworkOptions.LocalIPv6Addr[j].ToIPv6();
            s8_t index;
            netif_add_ip6_address_with_route(&(netIFs[j]), &ip6addr, 64, &index);
            // add ipv6 route for ipv6 address
            if (j < gNetworkOptions.IPv6GatewayAddr.size())
            {
                static ip6_addr_t br_ip6_addr = gNetworkOptions.IPv6GatewayAddr[j].ToIPv6();
                struct ip6_prefix ip6_prefix;
                ip6_prefix.addr = nl::Inet::IPAddress::Any.ToIPv6();
                ip6_prefix.prefix_len = 0;
                ip6_add_route_entry(&ip6_prefix, &netIFs[j], &br_ip6_addr, NULL);
            }
            if (index >= 0)
            {
                netif_ip6_addr_set_state(&(netIFs[j]), index, IP6_ADDR_PREFERRED);
            }
        }
        for (size_t n = 0; n < addrsVec.size(); n++)
        {
            IPAddress auto_addr;
            if (IPAddress::FromString(addrsVec[n], auto_addr) && !auto_addr.IsIPv4())
            {
                ip6_addr_t ip6addr = auto_addr.ToIPv6();
                s8_t index;
                if (auto_addr.IsIPv6LinkLocal())
                    continue; // skip over the LLA addresses, LwIP is aready adding those
                if (auto_addr.IsIPv6Multicast())
                    continue; // skip over the multicast addresses from host for now.
                netif_add_ip6_address_with_route(&(netIFs[j]), &ip6addr, 64, &index);
                if (index >= 0)
                {
                    netif_ip6_addr_set_state(&(netIFs[j]), index, IP6_ADDR_PREFERRED);
                }
            }
        }

        netif_set_up(&(netIFs[j]));
        netif_set_link_up(&(netIFs[j]));

    }

    netif_set_default(&(netIFs[0]));
    // UnLock LwIP stack

    UNLOCK_TCPIP_CORE();


    while (!NetworkIsReady())
    {
        struct timeval lSleepTime;
        lSleepTime.tv_sec = 0;
        lSleepTime.tv_usec = 100000;
        ServiceEvents(lSleepTime);
    }

    //FIXME: this is kinda nasty :(
    // Force new IP address to be ready, bypassing duplicate detection.

    for (size_t j = 0; j < gNetworkOptions.TapDeviceName.size(); j++)
    {
        if (j < gNetworkOptions.LocalIPv6Addr.size()) {
            netif_ip6_addr_set_state(&(netIFs[j]), 2, 0x30);
        }
        else
        {
            netif_ip6_addr_set_state(&(netIFs[j]), 1, 0x30);
        }
    }

#if INET_CONFIG_ENABLE_DNS_RESOLVER
    if (gNetworkOptions.DNSServerAddr != IPAddress::Any)
    {
#if LWIP_VERSION_MAJOR > 1 || LWIP_VERSION_MINOR >= 5
        ip_addr_t dnsServerAddr = gNetworkOptions.DNSServerAddr.ToLwIPAddr();
#else // LWIP_VERSION_MAJOR <= 1
#if INET_CONFIG_ENABLE_IPV4
        ip_addr_t dnsServerAddr = gNetworkOptions.DNSServerAddr.ToIPv4();
#else // !INET_CONFIG_ENABLE_IPV4
#error "No support for DNS Resolver without IPv4!"
#endif // !INET_CONFIG_ENABLE_IPV4
#endif // LWIP_VERSION_MAJOR > 1 || LWIP_VERSION_MINOR >= 5

        dns_setserver(0, &dnsServerAddr);
    }
#endif // INET_CONFIG_ENABLE_DNS_RESOLVER

    PrintNetworkState();

#endif // !WEAVE_SYSTEM_CONFIG_USE_SOCKETS

    AcquireLwIP();
    lContext = sLwIPEventQueue;

#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

    Inet.Init(SystemLayer, lContext);
}

void ServiceEvents(::timeval& aSleepTime)
{
    static bool printed = false;
    if (!printed)
    {
#if WEAVE_SYSTEM_CONFIG_USE_LWIP && !WEAVE_SYSTEM_CONFIG_USE_SOCKETS
        if (NetworkIsReady())
#endif
        {
            printf("Weave Node ready to service events; PID: %d; PPID: %d\n", getpid(), getppid());
            fflush(stdout);
            printed = true;
        }
    }
#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS
    fd_set readFDs, writeFDs, exceptFDs;
    int numFDs = 0;

    FD_ZERO(&readFDs);
    FD_ZERO(&writeFDs);
    FD_ZERO(&exceptFDs);

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS
    if (SystemLayer.State() == System::kLayerState_Initialized)
        SystemLayer.PrepareSelect(numFDs, &readFDs, &writeFDs, &exceptFDs, aSleepTime);
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS
    if (Inet.State == InetLayer::kState_Initialized)
        Inet.PrepareSelect(numFDs, &readFDs, &writeFDs, &exceptFDs, aSleepTime);
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS

    int selectRes = select(numFDs, &readFDs, &writeFDs, &exceptFDs, &aSleepTime);
    if (selectRes < 0)
    {
        printf("select failed: %s\n", ErrorStr(System::MapErrorPOSIX(errno)));
        return;
    }
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS

    if (SystemLayer.State() == System::kLayerState_Initialized)
    {
#if WEAVE_SYSTEM_CONFIG_USE_LWIP
        static uint32_t sRemainingSystemLayerEventDelay = 0;
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS

        SystemLayer.HandleSelectResult(selectRes, &readFDs, &writeFDs, &exceptFDs);

#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
        if (SystemLayer.State() == System::kLayerState_Initialized)
        {
            if (sRemainingSystemLayerEventDelay == 0)
            {
                SystemLayer.DispatchEvents();
                sRemainingSystemLayerEventDelay = gNetworkOptions.EventDelay;

            }
            else
                sRemainingSystemLayerEventDelay--;

            // TODO: Currently timers are delayed by aSleepTime above. A improved solution would have a mechanism to reduce
            // aSleepTime according to the next timer.

            SystemLayer.HandlePlatformTimer();

        }
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP
    }

#if WEAVE_SYSTEM_CONFIG_USE_LWIP && !WEAVE_SYSTEM_CONFIG_USE_SOCKETS
    TapInterface_Select(&(tapIFs[0]), &(netIFs[0]), aSleepTime, gNetworkOptions.TapDeviceName.size());
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP && !WEAVE_SYSTEM_CONFIG_USE_SOCKETS

    if (Inet.State == InetLayer::kState_Initialized)
    {
#if INET_CONFIG_PROVIDE_OBSOLESCENT_INTERFACES && WEAVE_SYSTEM_CONFIG_USE_LWIP
        static uint32_t sRemainingInetLayerEventDelay = 0;
#endif // INET_CONFIG_PROVIDE_OBSOLESCENT_INTERFACES && WEAVE_SYSTEM_CONFIG_USE_LWIP

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS

        Inet.HandleSelectResult(selectRes, &readFDs, &writeFDs, &exceptFDs);

#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS

#if INET_CONFIG_PROVIDE_OBSOLESCENT_INTERFACES && WEAVE_SYSTEM_CONFIG_USE_LWIP
        if (Inet.State == InetLayer::kState_Initialized)
        {
            if (sRemainingInetLayerEventDelay == 0)
            {
                Inet.DispatchEvents();
                sRemainingInetLayerEventDelay = gNetworkOptions.EventDelay;
            }
            else
                sRemainingInetLayerEventDelay--;

            // TODO: Currently timers are delayed by aSleepTime above. A improved solution would have a mechanism to reduce
            // aSleepTime according to the next timer.

            Inet.HandlePlatformTimer();

        }
#endif // INET_CONFIG_PROVIDE_OBSOLESCENT_INTERFACES && WEAVE_SYSTEM_CONFIG_USE_LWIP
    }
}

#if WEAVE_SYSTEM_CONFIG_USE_LWIP && !WEAVE_SYSTEM_CONFIG_USE_SOCKETS

static bool NetworkIsReady()
{
    bool ready = true;

    for (size_t j = 0; j < gNetworkOptions.TapDeviceName.size(); j++)
    {
        for (int i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++)
        {
            if (!ip6_addr_isany(netif_ip6_addr(&(netIFs[j]), i)) && ip6_addr_istentative(netif_ip6_addr_state(&(netIFs[j]), i)))
            {
                ready = false;
                break;
            }
        }
    }
    return ready;
}

static void OnLwIPInitComplete(void *arg)
{
    printf("Waiting for addresses assignment...\n");
}

#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP && !WEAVE_SYSTEM_CONFIG_USE_SOCKETS

void InitWeaveStack(bool listen, bool initExchangeMgr)
{
    WEAVE_ERROR res;
    WeaveMessageLayer::InitContext initContext;
    static nlDEFINE_ALIGNED_VAR(sTestGroupKeyStore, sizeof(TestGroupKeyStore), void*);

#if CONFIG_BLE_PLATFORM_BLUEZ
    // Initialize the BleLayer object.
    res = sBle.Init(&sBlePlatformDelegate, &sBleApplicationDelegate, &SystemLayer);

    if (res != WEAVE_NO_ERROR)
    {
        printf("sBle.Init failed: %s\n", ErrorStr(res));
        exit(-1);
    }
#endif /* CONFIG_BLE_PLATFORM_BLUEZ */

    nl::Weave::Stats::SetObjects(&MessageLayer);

    // Seed the random number generator

    System::Timer::Epoch now = System::Timer::GetCurrentEpoch();
    srand((unsigned int) now);

    // Initialize the FabricState object.

    res = FabricState.Init(new (&sTestGroupKeyStore) TestGroupKeyStore());
    if (res != WEAVE_NO_ERROR)
    {
        printf("FabricState.Init failed: %s\n", ErrorStr(res));
        exit(EXIT_FAILURE);
    }

    FabricState.FabricId = gWeaveNodeOptions.FabricId;
    FabricState.LocalNodeId = gWeaveNodeOptions.LocalNodeId;
    FabricState.DefaultSubnet = gWeaveNodeOptions.SubnetId;
    FabricState.PairingCode = gWeaveNodeOptions.PairingCode;

    // When using sockets we must listen on specific addresses, rather than ANY. Otherwise you will only be
    // able to run a single Weave application per system.

#if WEAVE_CONFIG_ENABLE_TARGETED_LISTEN
#if INET_CONFIG_ENABLE_IPV4
    if (!gNetworkOptions.LocalIPv4Addr.empty())
        FabricState.ListenIPv4Addr = gNetworkOptions.LocalIPv4Addr[0];
#endif // INET_CONFIG_ENABLE_IPV4

    if (!gNetworkOptions.LocalIPv6Addr.empty())
        FabricState.ListenIPv6Addr = gNetworkOptions.LocalIPv6Addr[0];
#endif

#if WEAVE_CONFIG_SECURITY_TEST_MODE
    FabricState.LogKeys = true;
#endif

    // Initialize the WeaveMessageLayer object.
    // TODO mock-device BLE support?

    initContext.systemLayer = &SystemLayer;
    initContext.inet = &Inet;
    initContext.fabricState = &FabricState;
    initContext.listenTCP = listen;
    initContext.listenUDP = true;
#if WEAVE_CONFIG_ENABLE_EPHEMERAL_UDP_PORT
    initContext.enableEphemeralUDPPort = gWeaveNodeOptions.UseEphemeralUDPPort;
#endif // WEAVE_CONFIG_ENABLE_EPHEMERAL_UDP_PORT

#if CONFIG_BLE_PLATFORM_BLUEZ
    initContext.ble = &sBle;
    initContext.listenBLE = true;
#endif /* CONFIG_NETWORK_LAYER_BLE */

    res = MessageLayer.Init(&initContext);
    if (res != WEAVE_NO_ERROR)
    {
        printf("WeaveMessageLayer.Init failed: %s\n", ErrorStr(res));
        exit(EXIT_FAILURE);
    }

    if (initExchangeMgr)
    {
        // Initialize the Exchange Manager object.

        res = ExchangeMgr.Init(&MessageLayer);
        if (res != WEAVE_NO_ERROR)
        {
            printf("WeaveExchangeManager.Init failed: %s\n", ErrorStr(res));
            exit(EXIT_FAILURE);
        }

        res = SecurityMgr.Init(ExchangeMgr, SystemLayer);
        if (res != WEAVE_NO_ERROR)
        {
            printf("WeaveSecurityManager.Init failed: %s\n", ErrorStr(res));
            exit(EXIT_FAILURE);
        }
        SecurityMgr.IdleSessionTimeout = gGeneralSecurityOptions.GetIdleSessionTimeout();
        SecurityMgr.SessionEstablishTimeout = gGeneralSecurityOptions.GetSessionEstablishmentTimeout();

        if (gTAKEOptions.ForceReauth)
        {
            res = gTAKEOptions.PrepopulateTokenData();
            if (res != WEAVE_NO_ERROR)
            {
                printf("MockTAKEChallengerDelegate::StoreTokenAuthData failed: %s\n", ErrorStr(res));
                exit(EXIT_FAILURE);
            }
        }

        SecurityMgr.SetCASEAuthDelegate(&gCASEOptions);
        SecurityMgr.SetKeyExportDelegate(&gKeyExportOptions);
        SecurityMgr.SetTAKEAuthDelegate(&gMockTAKEChallengerDelegate);
        SecurityMgr.SetTAKETokenAuthDelegate(&gMockTAKETokenDelegate);

#if WEAVE_CONFIG_ENABLE_CASE_INITIATOR
        if (gCASEOptions.InitiatorCASEConfig != kCASEConfig_NotSpecified)
            SecurityMgr.InitiatorCASEConfig = gCASEOptions.InitiatorCASEConfig;
#endif
        if (gCASEOptions.AllowedCASEConfigs != 0)
        {
#if WEAVE_CONFIG_ENABLE_CASE_INITIATOR
            SecurityMgr.InitiatorAllowedCASEConfigs = gCASEOptions.AllowedCASEConfigs;
#endif
#if WEAVE_CONFIG_ENABLE_CASE_RESPONDER
            SecurityMgr.ResponderAllowedCASEConfigs = gCASEOptions.AllowedCASEConfigs;
#endif
        }

#if WEAVE_CONFIG_ENABLE_KEY_EXPORT_RESPONDER
        if (gKeyExportOptions.AllowedKeyExportConfigs != 0)
            SecurityMgr.ResponderAllowedKeyExportConfigs = gKeyExportOptions.AllowedKeyExportConfigs;
#endif

#if WEAVE_CONFIG_SECURITY_TEST_MODE
        SecurityMgr.CASEUseKnownECDHKey = gCASEOptions.UseKnownECDHKey;
#endif
    }
}

void PrintNodeConfig()
{

    printf("Weave Node Configuration:\n");
    printf("  Fabric Id: %" PRIX64 "\n", FabricState.FabricId);
    printf("  Subnet Number: %X\n", FabricState.DefaultSubnet);
    printf("  Node Id: %" PRIX64 "\n", FabricState.LocalNodeId);

    if (MessageLayer.IsListening)
    {
        printf("  Listening Addresses:");
#if WEAVE_CONFIG_ENABLE_TARGETED_LISTEN
        char nodeAddrStr[64];

        if (FabricState.ListenIPv6Addr == IPAddress::Any
#if INET_CONFIG_ENABLE_IPV4
            && FabricState.ListenIPv4Addr == IPAddress::Any
#endif // INET_CONFIG_ENABLE_IPV4
            )
            printf(" any\n");
        else
        {
            printf("\n");
            if (FabricState.ListenIPv6Addr != IPAddress::Any)
            {
                FabricState.ListenIPv6Addr.ToString(nodeAddrStr, sizeof(nodeAddrStr));
                printf("      %s (ipv6)\n", nodeAddrStr);
            }

#if INET_CONFIG_ENABLE_IPV4
            if (FabricState.ListenIPv4Addr != IPAddress::Any)
            {
                FabricState.ListenIPv4Addr.ToString(nodeAddrStr, sizeof(nodeAddrStr));
                printf("      %s (ipv4)\n", nodeAddrStr);
            }
#endif // INET_CONFIG_ENABLE_IPV4
        }
#else // WEAVE_CONFIG_ENABLE_TARGETED_LISTEN
        printf(" any\n");
#endif // WEAVE_CONFIG_ENABLE_TARGETED_LISTEN
    }
}

void ShutdownNetwork()
{
    Inet.Shutdown();
#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    ReleaseLwIP();
#endif
}

void ShutdownWeaveStack()
{
    SecurityMgr.Shutdown();
    ExchangeMgr.Shutdown();
    MessageLayer.Shutdown();
    FabricState.Shutdown();
}

void DumpMemory(const uint8_t *mem, uint32_t len, const char *prefix, uint32_t rowWidth)
{
    (void) DumpMemory;

    int indexWidth = snprintf(NULL, 0, "%X", len);
    if (indexWidth < 4)
        indexWidth = 4;

    for (uint32_t i = 0; i < len; i += rowWidth)
    {
        printf("%s%0*X: ", prefix, indexWidth, i);

        uint32_t rowEnd = i + rowWidth;

        uint32_t j = i;
        for (; j < rowEnd && j < len; j++)
            printf("%02X ", mem[j]);

        for (; j < rowEnd; j++)
            printf("   ");

        for (j = i; j < rowEnd && j < len; j++)
            if (isprint((char) mem[j]))
                printf("%c", mem[j]);
            else
                printf(".");

        printf("\n");
    }
}

void DumpMemoryCStyle(const uint8_t *mem, uint32_t len, const char *prefix, uint32_t rowWidth)
{
    (void) DumpMemoryCStyle;

    for (uint32_t i = 0; i < len; i += rowWidth)
    {
        printf("%s", prefix);

        uint32_t rowEnd = i + rowWidth;

        uint32_t j = i;
        for (; j < rowEnd && j < len; j++)
            printf("0x%02X, ", mem[j]);

        printf("\n");
    }
}

bool IsZeroBytes(const uint8_t *buf, uint32_t len)
{
    for (; len > 0; len--, buf++)
        if (*buf != 0)
            return false;
    return true;
}

void PrintMACAddress(const uint8_t *buf, uint32_t len)
{
    for (; len > 0; buf++, len--)
    {
        if (len != 1)
            printf("%02X:", (unsigned)*buf);
        else
            printf("%02X", (unsigned)*buf);
    }
}

void PrintAddresses()
{
    InterfaceAddressIterator iterator;
    printf("Valid addresses: \n");
    for (; iterator.HasCurrent(); iterator.Next()) {
        IPAddress addr = iterator.GetAddress();
        char buf[80];
        addr.ToString(buf, 80);
        printf("%s\n", buf);
    }
}

uint8_t *ReadFileArg(const char *fileName, uint32_t& len, uint32_t maxLen)
{
    FILE *file = NULL;
    uint8_t *fileData = NULL;
    long fileLen;
    size_t fileStatus;

    file = fopen(fileName, "r");
    if (file == NULL)
    {
        printf("Unable to open %s\n%s\n", fileName, strerror(errno));
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    fileLen = ftell(file);
    fseek(file, 0, SEEK_SET);
    if (ferror(file))
    {
        printf("Unable to read %s\n%s\n", fileName, strerror(errno));
        fclose(file);
        return NULL;
    }

    len = (uint32_t) fileLen;
    if (len > maxLen)
    {
        printf("File too big: %s\n", fileName);
        fclose(file);
        return NULL;
    }

    fileData = (uint8_t *)malloc((size_t)len);
    if (fileData == NULL)
    {
        printf("Out of memory reading %s\n", fileName);
        fclose(file);
        return NULL;
    }

    fileStatus = fread(fileData, 1, (size_t)len, file);

    if ((fileStatus != len) || ferror(file))
    {
        printf("Unable to read %s\n%s", fileName, strerror(errno));
        fclose(file);
        free(fileData);
        return NULL;
    }

    fclose(file);

    return fileData;
}

void HandleMessageReceiveError(WeaveMessageLayer *msgLayer, WEAVE_ERROR err, const IPPacketInfo *pktInfo)
{
    const char * const default_format = "Error receiving message from %s: %s\n";

    if (pktInfo != NULL)
    {
        char ipAddrStr[INET6_ADDRSTRLEN];

        pktInfo->SrcAddress.ToString(ipAddrStr, sizeof(ipAddrStr));
        if (err == WEAVE_ERROR_INVALID_DESTINATION_NODE_ID && pktInfo->DestAddress.IsMulticast())
        {
            printf("Ignoring multicast message from %s addressed to different node id\n", ipAddrStr);
        }
        else if (err == WEAVE_ERROR_INVALID_ADDRESS && pktInfo->DestAddress.IsMulticast())
        {
            printf("Ignoring multicast message from %s using non-local source address\n", ipAddrStr);
        }
        else
        {
            printf(default_format, ipAddrStr, ErrorStr(err));
        }
    }
    else
    {
        // general, catch-all error message

        printf(default_format, "(unknown)", ErrorStr(err));
    }
}

void HandleAcceptConnectionError(WeaveMessageLayer *msgLayer, WEAVE_ERROR err)
{
    printf("Error accepting incoming connection: %s\n", ErrorStr(err));
}

#if CONFIG_BLE_PLATFORM_BLUEZ

void *WeaveBleIOLoop(void *arg)
{
    if (!nl::Ble::Platform::BlueZ::RunBluezIOThread((nl::Ble::Platform::BlueZ::BluezPeripheralArgs *)arg))
    {
        exit(EXIT_FAILURE);
    }

    return NULL;
}
#endif /* CONFIG_BLE_PLATFORM_BLUEZ */

void PrintStatsCounters(nl::Weave::System::Stats::count_t *counters, const char *aPrefix)
{
    size_t i;
    const nl::Weave::System::Stats::Label *strings = nl::Weave::System::Stats::GetStrings();
    const char *prefix = aPrefix ? aPrefix : "";

    for (i = 0; i < nl::Weave::System::Stats::kNumEntries; i++)
    {
        printf("%s%s:\t\t%" PRI_WEAVE_SYS_STATS_COUNT "\n", prefix, strings[i], counters[i]);
    }
}

bool ProcessStats(nl::Weave::System::Stats::Snapshot &aBefore, nl::Weave::System::Stats::Snapshot &aAfter, bool aPrint, const char *aPrefix)
{
    bool leak = false;
    nl::Weave::System::Stats::Snapshot delta;
    const char *prefix = aPrefix ? aPrefix : "";
    struct timeval sleepTime;
    uint64_t nowUsec;
    uint64_t upperBoundUsec;

    // If the current snapshot shows a leak when compared to the first one,
    // we service the network for a few more rounds, to give the system
    // a chance to release any "delayed-release" object that might have events
    // pending. There might also be one last WRM ACK in flight after the last
    // message between two nodes.
    // Note that the test harnesses we have been using give the process one
    // second to quit after sending a SIGUSR1, so this loop needs to complete
    // well before a second in case there is a leak. If the process takes too
    // long to quit, the test harness usually kills it. That causes
    // the output to be truncated which in turn makes the test failure harder to
    // understand.
    // The loop runs for 800 milliseconds in case there is an actual leak.
    // Fault-injection tests can require a longer time, since sometimes an EC is
    // freed only after the max number of retransmissions.
    // To allow extra time in this loop, see gFaultInjectionOptions.ExtraCleanupTimeMsec.
    sleepTime.tv_sec = 0;
    sleepTime.tv_usec = 100000;

    nl::Weave::Stats::UpdateSnapshot(aAfter);

    nowUsec = Now();
    upperBoundUsec = nowUsec + 800000 + (gFaultInjectionOptions.ExtraCleanupTimeMsec * 1000);

    while (Now() < upperBoundUsec)
    {
        leak = nl::Weave::System::Stats::Difference(delta, aAfter, aBefore);
        if (leak == false)
        {
            break;
        }

        ServiceNetwork(sleepTime);

        nl::Weave::Stats::UpdateSnapshot(aAfter);
    }

    if (aPrint)
    {
        if (gFaultInjectionOptions.DebugResourceUsage)
        {
            printf("\n%sResources in use before:\n", prefix);
            PrintStatsCounters(aBefore.mResourcesInUse, prefix);

            printf("\n%sResources in use after:\n", prefix);
            PrintStatsCounters(aAfter.mResourcesInUse, prefix);
        }

        printf("\n%sResource leak %sdetected\n", prefix, (leak ? "" : "not "));
        if (leak)
        {
            printf("%sDelta resources in use:\n", prefix);
            PrintStatsCounters(delta.mResourcesInUse, prefix);
            printf("%sEnd of delta resources in use\n", prefix);
        }

        if (gFaultInjectionOptions.DebugResourceUsage)
        {
            printf("\nHigh watermarks:\n");
            PrintStatsCounters(aAfter.mHighWatermarks, prefix);
        }
    }

    return leak;
}

void PrintFaultInjectionCounters(void)
{
    size_t i;
    nl::FaultInjection::Identifier faultId;
    nl::FaultInjection::GetManagerFn faultMgrTable[] = {
        nl::Weave::FaultInjection::GetManager,
        nl::Inet::FaultInjection::GetManager,
        nl::Weave::System::FaultInjection::GetManager
    };

    if (!gFaultInjectionOptions.PrintFaultCounters)
    {
        return;
    }

    printf("\nFaultInjection counters:\n");
    for (i = 0; i < sizeof(faultMgrTable) / sizeof(faultMgrTable[0]); i++)
    {
        nl::FaultInjection::Manager &mgr = faultMgrTable[i]();

        for (faultId = 0; faultId < mgr.GetNumFaults(); faultId++)
        {
            printf("%s_%s: %u\n", mgr.GetName(), mgr.GetFaultNames()[faultId],
                    mgr.GetFaultRecords()[faultId].mNumTimesChecked);
        }
    }
    printf("End of FaultInjection counters\n");

}

struct RestartCallbackContext {
    int mArgc;
    char **mArgv;
};
static struct RestartCallbackContext gRestartCallbackCtx;

static void RebootCallbackFn(void)
{
    char *lArgv[gRestartCallbackCtx.mArgc +2];
    int i;
    int j = 0;

    if (gSigusr1Received)
    {
        printf("** skipping restart case after SIGUSR1 **\n");
        ExitNow();
    }

    for (i = 0; gRestartCallbackCtx.mArgv[i] != NULL; i++)
    {
        if (strcmp(gRestartCallbackCtx.mArgv[i], "--faults") == 0)
        {
            // Skip the --faults argument for now
            i++;
            continue;
        }
        lArgv[j++] = gRestartCallbackCtx.mArgv[i];
    }

    lArgv[j] = NULL;

    for (i = 0; lArgv[i] != NULL; i++)
    {
        printf("argv[%d]: %s\n", i, lArgv[i]);
    }

    // Need to close any open file descriptor above stdin/out/err.
    // There is no portable way to get the max fd number.
    // Given that Weave's test apps don't open a large number of files,
    // FD_SETSIZE should be a reasonable upper bound (see the documentation
    // of select).
    for (i = 3; i < FD_SETSIZE; i++)
    {
        close(i);
    }

    printf("********** Restarting *********\n");
    fflush(stdout);
    execvp(lArgv[0], lArgv);

exit:
    return;
}

static void PostInjectionCallbackFn(nl::FaultInjection::Manager *aManager,
                             nl::FaultInjection::Identifier aId,
                             nl::FaultInjection::Record *aFaultRecord)
{
    uint16_t numargs = aFaultRecord->mNumArguments;
    uint16_t i;

    printf("***** Injecting fault %s_%s, instance number: %u; reboot: %s",
                            aManager->GetName(), aManager->GetFaultNames()[aId],
                            aFaultRecord->mNumTimesChecked, aFaultRecord->mReboot ? "yes" : "no");
    if (numargs)
    {
        printf(" with %u args:", numargs);

        for (i = 0; i < numargs; i++)
        {
            printf(" %d", aFaultRecord->mArguments[i]);
        }
    }

    printf("\n");
}

static nl::FaultInjection::GlobalContext gFaultInjectionGlobalContext = {
    {
        RebootCallbackFn,
        PostInjectionCallbackFn
    }
};

static nl::FaultInjection::Callback sFuzzECHeaderCb;
static nl::FaultInjection::Callback sAsyncEventCb;

static bool PrintFaultInjectionMaxArgCbFn(nl::FaultInjection::Manager &mgr, nl::FaultInjection::Identifier aId, nl::FaultInjection::Record *aFaultRecord, void *aContext)
{
    const char *faultName = mgr.GetFaultNames()[aId];

    if (gFaultInjectionOptions.PrintFaultCounters && aFaultRecord->mNumArguments)
    {
        printf("FI_instance_params: %s_%s_s%u maxArg: %u;\n", mgr.GetName(), faultName, aFaultRecord->mNumTimesChecked,
                aFaultRecord->mArguments[0]);
    }

    return false;
}

static bool PrintWeaveFaultInjectionMaxArgCbFn(nl::FaultInjection::Identifier aId, nl::FaultInjection::Record *aFaultRecord, void *aContext)
{
    nl::FaultInjection::Manager &mgr = nl::Weave::FaultInjection::GetManager();

    return PrintFaultInjectionMaxArgCbFn(mgr, aId, aFaultRecord, aContext);
}

static bool PrintSystemFaultInjectionMaxArgCbFn(nl::FaultInjection::Identifier aId, nl::FaultInjection::Record *aFaultRecord, void *aContext)
{
    nl::FaultInjection::Manager &mgr = nl::Weave::System::FaultInjection::GetManager();

    return PrintFaultInjectionMaxArgCbFn(mgr, aId, aFaultRecord, aContext);
}

void SetupFaultInjectionContext(int argc, char *argv[])
{
    SetupFaultInjectionContext(argc, argv, NULL, NULL);
}

void SetupFaultInjectionContext(int argc, char *argv[], int32_t (*aNumEventsAvailable)(void), void (*aInjectAsyncEvents)(int32_t index))
{
    nl::FaultInjection::Manager &weavemgr = nl::Weave::FaultInjection::GetManager();
    nl::FaultInjection::Manager &systemmgr = nl::Weave::System::FaultInjection::GetManager();

    gRestartCallbackCtx.mArgc = argc;
    gRestartCallbackCtx.mArgv = argv;

    nl::FaultInjection::SetGlobalContext(&gFaultInjectionGlobalContext);

    memset(&sFuzzECHeaderCb, 0, sizeof(sFuzzECHeaderCb));
    sFuzzECHeaderCb.mCallBackFn = PrintWeaveFaultInjectionMaxArgCbFn;
    weavemgr.InsertCallbackAtFault(nl::Weave::FaultInjection::kFault_FuzzExchangeHeaderTx, &sFuzzECHeaderCb);

    if (aNumEventsAvailable && aInjectAsyncEvents)
    {
        memset(&sAsyncEventCb, 0, sizeof(sAsyncEventCb));
        sAsyncEventCb.mCallBackFn = PrintSystemFaultInjectionMaxArgCbFn;
        systemmgr.InsertCallbackAtFault(nl::Weave::System::FaultInjection::kFault_AsyncEvent, &sAsyncEventCb);

        nl::Weave::System::FaultInjection::SetAsyncEventCallbacks(aNumEventsAvailable, aInjectAsyncEvents);
    }
}

/**
 * Process network events until a given boolean becomes true and
 * a given amount of time has elapsed. Both conditions are optional.
 *
 * @param[in] aDone         pointer to the boolean; if NULL, the parameter is ignored.
 *
 * @param[in] aIntervalMs   pointer to the number of milliseconds. The function will
 *                          process events until at least this number of milliseconds
 *                          has elapsed. If NULL, the parameter is ignored.
 */
void ServiceNetworkUntil(const bool *aDone, const uint32_t *aIntervalMs)
{
    uint64_t startTimeMs = NowMs();
    uint64_t elapsedMs = 0;
    struct timeval sleepTime;

    sleepTime.tv_sec = 0;
    sleepTime.tv_usec = 100000;

    while (((aDone != NULL) && !(*aDone)) ||
           ((aIntervalMs != NULL) && (elapsedMs < *aIntervalMs)))
    {
        ServiceNetwork(sleepTime);

        elapsedMs = NowMs() - startTimeMs;
    }
}
