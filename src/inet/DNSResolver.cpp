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
 *      This file implements DNSResolver, the object that abstracts
 *      Domain Name System (DNS) resolution in InetLayer.
 *
 */

#include <InetLayer/DNSResolver.h>

#include <InetLayer/InetLayer.h>
#include <InetLayer/InetLayerEvents.h>

#include <string.h>

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
#include <lwip/dns.h>
#include <lwip/tcpip.h>
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS
#include <netdb.h>
#include <errno.h>
#include <stdlib.h>
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS

namespace nl {
namespace Inet {

Weave::System::ObjectPool<DNSResolver, INET_CONFIG_NUM_DNS_RESOLVERS> DNSResolver::sPool;

/**
 *  This method revolves a host name into a list of IP addresses.
 *
 *  @note
 *     Even if the operation completes successfully,
 *     the result might be a zero-length list of IP addresses.
 *     Most of the error generated are returned via the
 *     application callback.
 *
 *  @param[in]  hostName    A pointer to a C string representing the host name
 *                          to be queried.
 *  @param[in]  hostNameLen The string length of host name.
 *  @param[in]  maxAddrs    The maximum number of addresses to store in the DNS
 *                          table.
 *  @param[in]  addrArray   A pointer to the DNS table.
 *  @param[in]  onComplete  A pointer to the callback function when a DNS
 *                          request is complete.
 *  @param[in]  appState    A pointer to the application state to be passed to
 *                          onComplete when a DNS request is complete.
 *
 *  @retval INET_NO_ERROR                   if a DNS request is handled
 *                                          successfully.
 *
 *  @retval INET_ERROR_NOT_IMPLEMENTED      if DNS resolution is not enabled on
 *                                          the underlying platform.
 *
 *  @retval _other_                         if other POSIX network or OS error
 *                                          was returned by the underlying DNS
 *                                          resolver implementation.
 *
 */
INET_ERROR DNSResolver::Resolve(const char *hostName, uint16_t hostNameLen, uint8_t maxAddrs, IPAddress *addrArray,
    DNSResolver::OnResolveCompleteFunct onComplete, void *appState)
{
#if !WEAVE_SYSTEM_CONFIG_USE_SOCKETS && !LWIP_DNS
    Release();
    return INET_ERROR_NOT_IMPLEMENTED;
#endif // !WEAVE_SYSTEM_CONFIG_USE_SOCKETS && !LWIP_DNS

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS || (WEAVE_SYSTEM_CONFIG_USE_LWIP && LWIP_DNS)

    // TODO: Eliminate the need for a local buffer when running on LwIP by changing
    // the LwIP DNS interface to support non-nul terminated strings.

    char hostNameBuf[NL_DNS_HOSTNAME_MAX_LEN + 1]; // DNS limits hostnames to 253 max characters.

    memcpy(hostNameBuf, hostName, hostNameLen);
    hostNameBuf[hostNameLen] = 0;

#if WEAVE_SYSTEM_CONFIG_USE_LWIP

    INET_ERROR res = INET_NO_ERROR;

    AppState = appState;

    if (maxAddrs > INET_CONFIG_MAX_DNS_ADDRS)
        maxAddrs = INET_CONFIG_MAX_DNS_ADDRS;

    AddrArray = addrArray;
    MaxAddrs = maxAddrs;
    NumAddrs = 0;
    OnComplete = onComplete;

    // Lock LwIP stack
    LOCK_TCPIP_CORE();

    ip_addr_t lwipAddrArray[INET_CONFIG_MAX_DNS_ADDRS];

    err_t lwipErr = dns_gethostbyname(hostNameBuf, lwipAddrArray, LwIPHandleResolveComplete, this);

    // Unlock LwIP stack
    UNLOCK_TCPIP_CORE();

    if (lwipErr == ERR_OK)
    {
        Weave::System::Layer& lSystemLayer = SystemLayer();

        CopyAddresses(1, lwipAddrArray);
        lSystemLayer.PostEvent(*this, kInetEvent_DNSResolveComplete, 0);
    }
    else if (lwipErr != ERR_INPROGRESS)
    {
        res = Weave::System::MapErrorLwIP(lwipErr);
        Release();
    }

    return res;

#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS

    INET_ERROR err = INET_NO_ERROR;
    struct addrinfo hints;
    struct addrinfo *lookupRes = NULL;
    int getaddrinfoRes;

    NumAddrs = 0;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 6;
    hints.ai_flags = AI_ADDRCONFIG;

    getaddrinfoRes = getaddrinfo(hostNameBuf, NULL, &hints, &lookupRes);

    if (getaddrinfoRes == 0)
    {
        for (struct addrinfo *addr = lookupRes; addr != NULL && NumAddrs < maxAddrs; addr = addr->ai_next, NumAddrs++)
            addrArray[NumAddrs] = IPAddress::FromSockAddr(*addr->ai_addr);
    }
    else
    {
        switch (getaddrinfoRes)
        {
        case EAI_NODATA:
            err = INET_NO_ERROR;
            break;
        case EAI_NONAME:
            err = INET_ERROR_HOST_NOT_FOUND;
            break;
        case EAI_AGAIN:
            err = INET_ERROR_DNS_TRY_AGAIN;
            break;
        case EAI_SYSTEM:
            err = Weave::System::MapErrorPOSIX(errno);
            break;
        default:
            err = INET_ERROR_DNS_NO_RECOVERY;
            break;
        }
    }

    if (lookupRes != NULL)
        freeaddrinfo(lookupRes);

    onComplete(appState, err, NumAddrs, addrArray);

    Release();

    return INET_NO_ERROR;

#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS || (WEAVE_SYSTEM_CONFIG_USE_LWIP && LWIP_DNS)
}

/**
 *  This method cancels DNS requests that are in progress.
 *
 *  @retval INET_NO_ERROR.
 *
 */
INET_ERROR DNSResolver::Cancel()
{
#if WEAVE_SYSTEM_CONFIG_USE_LWIP

    // NOTE: LwIP does not support canceling DNS requests that are in progress.  As a consequence,
    // we can't release the DNSResolver object until LwIP calls us back (because LwIP retains a
    // pointer to the the DNSResolver object while the request is active).  However, now that the
    // application has called Cancel() we have to make sure to NOT call their OnComplete function
    // when the request completes.
    //
    // To ensure the right thing happens, we NULL the OnComplete pointer here, which signals the
    // code in HandleResolveComplete() and LwIPHandleResolveComplete() to not interact with the
    // application's state data (AddrArray) and to not call the application's callback. This has
    // to happen with the LwIP lock held, since LwIPHandleResolveComplete() runs on LwIP's thread.

    // Lock LwIP stack
    LOCK_TCPIP_CORE();

    // Signal that the request has been canceled by clearing the state of the resolver object.
    OnComplete = NULL;
    AddrArray = NULL;
    MaxAddrs = 0;
    NumAddrs = 0;

    // Unlock LwIP stack
    UNLOCK_TCPIP_CORE();

#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS
#if INET_CONFIG_ENABLE_ASYNC_DNS_SOCKETS
    // NOTE: DNS lookups can be canceled only when using the asynchronous mode.

    InetLayer& inet = Layer();

    OnComplete = NULL;
    AppState = NULL;
    inet.mAsyncDNSResolver.Cancel(*this);

#endif // INET_CONFIG_ENABLE_ASYNC_DNS_SOCKETS
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS

    return INET_NO_ERROR;
}

#if WEAVE_SYSTEM_CONFIG_USE_LWIP

/**
 *  This method is called by InetLayer on success, failure, or timeout of a
 *  DNS request.
 *
 */
void DNSResolver::HandleResolveComplete()
{
    // Call the application's completion handler if the request hasn't been canceled.
    if (OnComplete != NULL)
        OnComplete(AppState, (NumAddrs > 0) ? INET_NO_ERROR : INET_ERROR_HOST_NOT_FOUND, NumAddrs, AddrArray);

    // Release the resolver object.
    Release();
}


/**
 *  This method is called by LwIP network stack on success, failure, or timeout
 *  of a DNS request.
 *
 *  @param[in]  name            A pointer to a NULL-terminated C string
 *                              representing the host name that is queried.
 *  @param[in]  ipaddr          A pointer to a list of resolved IP addresses.
 *  @param[in]  callback_arg    A pointer to the arguments that are passed to
 *                              the callback function.
 *
 */
#if LWIP_VERSION_MAJOR > 1
void DNSResolver::LwIPHandleResolveComplete(const char *name, const ip_addr_t *ipaddr, void *callback_arg)
#else // LWIP_VERSION_MAJOR <= 1
void DNSResolver::LwIPHandleResolveComplete(const char *name, ip_addr_t *ipaddr, void *callback_arg)
#endif // LWIP_VERSION_MAJOR <= 1
{
    DNSResolver *resolver = (DNSResolver *)callback_arg;

    if (resolver != NULL)
    {
        Weave::System::Layer& lSystemLayer = resolver->SystemLayer();

        // Copy the resolved address to the application supplied buffer, but only if the request hasn't been canceled.
        if (resolver->OnComplete != NULL)
            resolver->CopyAddresses((ipaddr != NULL) ? 1 : 0, ipaddr);

        lSystemLayer.PostEvent(*resolver, kInetEvent_DNSResolveComplete, 0);
    }
}

/**
 *  This method copies a list of resolved IP addresses to the DNS table.
 *
 *  @param[in]  numAddrs    The number of addresses in the list.
 *  @param[in]  addrs       A list of resolved IP addresses.
 *
 */
void DNSResolver::CopyAddresses(uint8_t numAddrs, const ip_addr_t *addrs)
{
    if (numAddrs > MaxAddrs)
        numAddrs = MaxAddrs;

    for (uint8_t i = 0; i < numAddrs; i++)
    {
#if LWIP_VERSION_MAJOR > 1
        AddrArray[i] = IPAddress::FromLwIPAddr(addrs[i]);
#else // LWIP_VERSION_MAJOR <= 1
        AddrArray[i] = IPAddress::FromIPv4(addrs[i]);
#endif // LWIP_VERSION_MAJOR <= 1
    }

    NumAddrs = numAddrs;
}

#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS
#if INET_CONFIG_ENABLE_ASYNC_DNS_SOCKETS

void DNSResolver::HandleAsyncResolveComplete(void)
{
    // Copy the resolved address to the application supplied buffer, but only if the request hasn't been canceled.
    if (OnComplete && mState != kState_Canceled)
    {
        OnComplete(AppState, asyncDNSResolveResult, NumAddrs, AddrArray);
    }

    Release();
}
#endif // INET_CONFIG_ENABLE_ASYNC_DNS_SOCKETS
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS

} // namespace Inet
} // namespace nl
