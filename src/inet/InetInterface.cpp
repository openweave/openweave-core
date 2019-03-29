/*
 *
 *    Copyright (c) 2019 Google LLC.
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
 *      Implementation of network interface abstraction layer.
 *
 */

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#include <stdio.h>
#include <string.h>

#include <Weave/Support/NLDLLUtil.h>

#include <InetLayer/InetLayer.h>
#include <InetLayer/InetLayerEvents.h>

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
#include <lwip/sys.h>
#include <lwip/netif.h>
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <net/if.h>
#ifdef __ANDROID__
#include "ifaddrs-android.h"
#else // !defined(__ANDROID__)
#include <ifaddrs.h>
#endif // !defined(__ANDROID__)
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS

namespace nl {
namespace Inet {

NL_DLL_EXPORT INET_ERROR GetInterfaceName(InterfaceId intfId, char *nameBuf, size_t nameBufSize)
{
#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    int status;

    if (intfId != INET_NULL_INTERFACEID)
    {
        status = snprintf(nameBuf, nameBufSize, "%c%c%d", intfId->name[0], intfId->name[1], intfId->num);

        if (status >= static_cast<int>(nameBufSize))
            return INET_ERROR_NO_MEMORY;
    }
    else
        nameBuf[0] = 0;
    return INET_NO_ERROR;
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS
    if (intfId != INET_NULL_INTERFACEID)
    {
        char intfName[IF_NAMESIZE];
        if (if_indextoname(intfId, intfName) == NULL)
            return Weave::System::MapErrorPOSIX(errno);
        if (strlen(intfName) >= nameBufSize)
            return INET_ERROR_NO_MEMORY;
        strcpy(nameBuf, intfName);
    }
    else
    {
        if (nameBufSize < 1)
            return INET_ERROR_NO_MEMORY;
        nameBuf[0] = 0;
    }
    return INET_NO_ERROR;
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS
}

NL_DLL_EXPORT INET_ERROR InterfaceNameToId(const char *intfName, InterfaceId& intfId)
{
#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    if (strlen(intfName) < 3)
        return INET_ERROR_UNKNOWN_INTERFACE;
    char *parseEnd;
    unsigned long intfNum = strtoul(intfName + 2, &parseEnd, 10);
    if (*parseEnd != 0 || intfNum > UINT8_MAX)
        return INET_ERROR_UNKNOWN_INTERFACE;
    for (struct netif *intf = netif_list; intf != NULL; intf = intf->next)
        if (intf->name[0] == intfName[0] && intf->name[1] == intfName[1] && intf->num == (uint8_t)intfNum)
        {
            intfId = intf;
            return INET_NO_ERROR;
        }
    intfId = INET_NULL_INTERFACEID;
    return INET_ERROR_UNKNOWN_INTERFACE;
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS
    intfId = if_nametoindex(intfName);
    if (intfId == 0)
        return (errno == ENXIO) ? INET_ERROR_UNKNOWN_INTERFACE : Weave::System::MapErrorPOSIX(errno);
    return INET_NO_ERROR;
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS
}

InterfaceIteratorBasis::InterfaceIteratorBasis(void)
{
#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    curIntf = netif_list;
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS
    const int rv = getifaddrs(&addrsList);

    if (rv != -1) {
        curAddr = addrsList;
    } else {
        curAddr = addrsList = NULL;
    }
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS
}

InterfaceIteratorBasis::~InterfaceIteratorBasis(void)
{
#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    curIntf = NULL;
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS
    if (addrsList != NULL)
        freeifaddrs(addrsList);
    curAddr = addrsList = NULL;
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS
}

InterfaceId InterfaceIteratorBasis::GetInterface(void)
{
    InterfaceId rv = INET_NULL_INTERFACEID;

    if (HasCurrent())
    {
#if WEAVE_SYSTEM_CONFIG_USE_LWIP
        rv = curIntf;
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS
        rv = if_nametoindex(curAddr->ifa_name);
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS
    }

    return rv;
}

bool InterfaceIteratorBasis::SupportsMulticast(void)
{
    if (HasCurrent())
    {
#if WEAVE_SYSTEM_CONFIG_USE_LWIP
#if LWIP_VERSION_MAJOR > 1 || LWIP_VERSION_MINOR >= 5
        return (curIntf->flags & (NETIF_FLAG_IGMP | NETIF_FLAG_MLD6 | NETIF_FLAG_BROADCAST)) != 0;
#else
        return (curIntf->flags & NETIF_FLAG_POINTTOPOINT) == 0;
#endif // LWIP_VERSION_MAJOR > 1 || LWIP_VERSION_MINOR >= 5
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS
        return (curAddr->ifa_flags & IFF_MULTICAST) != 0;
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS
    }
    else
        return false;
}

InterfaceIterator::InterfaceIterator(void) :
    InterfaceIteratorBasis()
{
    return;
}

InterfaceIterator::~InterfaceIterator(void)
{
    return;
}

bool InterfaceIterator::Next(void)
{
#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    if (curIntf != NULL)
        curIntf = curIntf->next;
    return (curIntf != NULL);
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS
    if (curAddr != NULL)
    {
        const char *lastIntfName = curAddr->ifa_name;
        do
            curAddr = curAddr->ifa_next;
        while (curAddr != NULL && strcmp(curAddr->ifa_name, lastIntfName) == 0);
    }
    return (curAddr != NULL);
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS
}

InterfaceAddressIterator::InterfaceAddressIterator(void) :
    InterfaceIteratorBasis()
{
#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    curAddrIndex = 0;
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS
    // Advance the iterator until we're sure that the current element has
    // a valid AF_INET or AF_INET6 address.
    while ((curAddr != NULL) &&
           ((curAddr->ifa_addr == NULL) ||
            ((curAddr->ifa_addr->sa_family != AF_INET6)
#if INET_CONFIG_ENABLE_IPV4
              && (curAddr->ifa_addr->sa_family != AF_INET)
#endif // INET_CONFIG_ENABLE_IPV4
             )))
    {
        curAddr = curAddr->ifa_next;
    }
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS
}

InterfaceAddressIterator::~InterfaceAddressIterator(void)
{
#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    curAddrIndex = 0;
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP
}

bool InterfaceAddressIterator::Next(void)
{
#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    while (curIntf != NULL)
    {
        curAddrIndex++;
        if (curAddrIndex >= LWIP_IPV6_NUM_ADDRESSES)
        {
            curIntf = curIntf->next;
            curAddrIndex = 0;
            continue;
        }

        if (ip6_addr_isvalid(netif_ip6_addr_state(curIntf, curAddrIndex)))
            return true;
    }
    return false;
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS
    if (curAddr != NULL)
        while (true)
        {
            curAddr = curAddr->ifa_next;
            if (curAddr == NULL)
                break;
            if (curAddr->ifa_addr != NULL &&
                (curAddr->ifa_addr->sa_family == AF_INET6
#if INET_CONFIG_ENABLE_IPV4
                || curAddr->ifa_addr->sa_family == AF_INET
#endif // INET_CONFIG_ENABLE_IPV4
               ))
                return true;
        }
    return false;
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS
}

IPAddress InterfaceAddressIterator::GetAddress(void)
{
    IPAddress rv = IPAddress::Any;

    if (HasCurrent())
    {
#if WEAVE_SYSTEM_CONFIG_USE_LWIP
        rv = IPAddress::FromIPv6(*netif_ip6_addr(curIntf, curAddrIndex));
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS
        if (curAddr->ifa_addr->sa_family == AF_INET6)
        {
            rv = IPAddress::FromIPv6(((struct sockaddr_in6*)curAddr->ifa_addr)->sin6_addr);
        }
#if INET_CONFIG_ENABLE_IPV4
        else if (curAddr->ifa_addr->sa_family == AF_INET)
        {
            rv = IPAddress::FromIPv4(((struct sockaddr_in*)curAddr->ifa_addr)->sin_addr);
        }
#endif // INET_CONFIG_ENABLE_IPV4
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS
    }

    return rv;
}

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS
uint8_t InterfaceAddressIterator::GetIPv6PrefixLength(void)
{
    uint8_t prefixLen = 0;

    if (curAddr->ifa_addr->sa_family == AF_INET6)
    {
        struct sockaddr_in6& netmask = *(struct sockaddr_in6 *)(curAddr->ifa_netmask);
        for (int i = 0; i < 16; i++, prefixLen += 8)
        {
            uint8_t b = netmask.sin6_addr.s6_addr[i];
            if (b != 0xFF)
            {
                if ((b & 0xF0) == 0xF0)
                    prefixLen += 4;
                else
                    b = b >> 4;

                if ((b & 0x0C) == 0x0C)
                    prefixLen += 2;
                else
                    b = b >> 2;

                if ((b & 0x02) == 0x02)
                    prefixLen++;

                break;
            }
        }
    }

    return prefixLen;
}
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS

} // namespace Inet

} // namespace nl
