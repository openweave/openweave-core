/*
 *
 *    Copyright (c) 2013-2017 Nest Labs, Inc.
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
 *      This file defines DNSResolver, the object that abstracts
 *      Domain Name System (DNS) resolution in InetLayer.
 *
 */

#ifndef DNSRESOLVER_H
#define DNSRESOLVER_H

#include <InetLayer/IPAddress.h>
#include <InetLayer/InetError.h>
#include <InetLayer/InetLayerBasis.h>

#define NL_DNS_HOSTNAME_MAX_LEN      (253)

namespace nl {
namespace Inet {

class InetLayer;

/**
 *  @class DNSResolver
 *
 *  @brief
 *    This is an internal class to InetLayer that provides the abstraction of
 *    Domain Name System (DNS) resolution in InetLayer. There is no public
 *    interface available for the application layer.
 *
 */
class DNSResolver: public InetLayerBasis
{
private:
    friend class InetLayer;

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS
#if INET_CONFIG_ENABLE_ASYNC_DNS_SOCKETS
    friend class AsyncDNSResolverSockets;

    /// States of the DNSResolver object with respect to hostname resolution.
    typedef enum DNSResolverState
    {
        kState_Unused                        = 0, ///<Used to indicate that the DNSResolver object is not used.
        kState_Active                        = 2, ///<Used to indicate that a DNS resolution is being performed on the DNSResolver object.
        kState_Complete                      = 3, ///<Used to indicate that the DNS resolution on the DNSResolver object is complete.
        kState_Canceled                      = 4, ///<Used to indicate that the DNS resolution on the DNSResolver has been canceled.
    } DNSResolverState;
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS
#endif // INET_CONFIG_ENABLE_ASYNC_DNS_SOCKETS

    /**
     * @brief   Type of event handling function called when a DNS request completes.
     *
     * @param[in]   appState    Application state pointer.
     * @param[in]   err         Error code.
     * @param[in]   addrCount   Number of IP addresses in the \c addrArray list.
     * @param[in]   addrArray   List of addresses in the answer.
     *
     * @details
     *  Provide a function of this type to the \c Resolve method to process
     *  completion events.
     */
    typedef void (*OnResolveCompleteFunct)(void *appState, INET_ERROR err, uint8_t addrCount, IPAddress *addrArray);

    static Weave::System::ObjectPool<DNSResolver, INET_CONFIG_NUM_DNS_RESOLVERS> sPool;

    /**
     *  A pointer to the callback function when a DNS request is complete.
     */
    OnResolveCompleteFunct OnComplete;

    /**
     *  A pointer to the DNS table that stores a list of resolved addresses.
     */
    IPAddress *AddrArray;

    /**
     *  The maximum number of addresses that could be stored in the DNS table.
     */
    uint8_t MaxAddrs;

    /**
     *  The actual number of addresses that are stored in the DNS table.
     */
    uint8_t NumAddrs;

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS
#if INET_CONFIG_ENABLE_ASYNC_DNS_SOCKETS

    /* Hostname that requires resolution */
    char asyncHostNameBuf[NL_DNS_HOSTNAME_MAX_LEN + 1]; // DNS limits hostnames to 253 max characters.

    INET_ERROR asyncDNSResolveResult;
    /* The next DNSResolver object in the asynchronous DNS resolution queue. */
    DNSResolver *pNextAsyncDNSResolver;

    DNSResolverState mState;

    void HandleAsyncResolveComplete(void);

#endif // INET_CONFIG_ENABLE_ASYNC_DNS_SOCKETS
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS

    INET_ERROR Resolve(const char *hostName, uint16_t hostNameLen, uint8_t maxAddrs, IPAddress *addrArray,
        OnResolveCompleteFunct onComplete, void *appState);
    INET_ERROR Cancel(void);

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    void CopyAddresses(uint8_t numAddrs, const ip_addr_t *addrs);
    void HandleResolveComplete(void);
#if LWIP_VERSION_MAJOR > 1
    static void LwIPHandleResolveComplete(const char *name, const ip_addr_t *ipaddr, void *callback_arg);
#else // LWIP_VERSION_MAJOR <= 1
    static void LwIPHandleResolveComplete(const char *name, ip_addr_t *ipaddr, void *callback_arg);
#endif // LWIP_VERSION_MAJOR <= 1
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP
};

} // namespace Inet
} // namespace nl

#endif // !defined(DNSRESOLVER_H_)
