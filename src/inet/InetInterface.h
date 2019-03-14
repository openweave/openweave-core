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
 * @file
 *  This file defines the <tt>nl::Inet::InterfaceId</tt> type alias and related
 *  classes for iterating on the list of system network interfaces and the list
 *  of system interface addresses.
 */

#ifndef INETINTERFACE_H
#define INETINTERFACE_H

#include <Weave/Support/NLDLLUtil.h>

#include <InetLayer/IPAddress.h>

#include <stddef.h>
#include <stdint.h>

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
#include <lwip/netif.h>
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS
struct ifaddrs;
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS

namespace nl {
namespace Inet {

#if WEAVE_SYSTEM_CONFIG_USE_LWIP

typedef struct netif *InterfaceId;

#define INET_NULL_INTERFACEID NULL

#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS

typedef unsigned InterfaceId;

#define INET_NULL_INTERFACEID 0

#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS

/**
 * @typedef     InterfaceId
 *
 * @brief       Indicator for system network interfaces.
 *
 * @details
 *  Portability depends on never witnessing this alias. It may be replaced by a
 *  concrete opaque class in the future.
 *
 *  Note Well: The term "interface identifier" also conventionally refers to
 *  the lower 64 bits of an IPv6 address in all the relevant IETF standards
 *  documents, where the abbreviation "IID" is often used. In this text, the
 *  term "interface indicator" refers to values of this type alias.
 */

/**
 * @def     INET_NULL_INTERFACEID
 *
 * @brief   The distinguished value indicating no network interface.
 *
 * @details
 *  Note Well: This is not the indicator of a "null" network interface. This
 *  value can be used to indicate the absence of a specific network interface,
 *  or to specify that any applicable network interface is acceptable. Usage
 *  varies depending on context.
 */

/**
 * @brief   Test \c ID for inequivalence with \c INET_NULL_INTERFACEID
 *
 * @details
 *  This macro resolves to an expression that evaluates \c false if the
 *  argument is equivalent to \c INET_NULL_INTERFACEID and \c true otherwise.
 */
#define IsInterfaceIdPresent(intfId) ((intfId) != INET_NULL_INTERFACEID)

/**
 * @brief   Write the name of the network interface to a memory buffer
 *
 * @param[in]   intfId      a network interface
 * @param[out]  nameBuf     region of memory to write the interface name
 * @param[in]   nameBufSize size of the region denoted by \c nameBuf
 *
 * @retval  INET_NO_ERROR           successful result, interface name written
 * @retval  INET_ERROR_NO_MEMORY    name is too large to be written in buffer
 * @retval  other                   another system or platform error
 *
 * @details
 *  Writes the name of the network interface as \c NUL terminated text string
 *  at \c nameBuf. The name of the unspecified network interface is the empty
 *  string.
 *
 *  The memory at \c nameBuf may be overwritten with nonsense even when the
 *  returned value is not \c INET_NO_ERROR.
 */
extern INET_ERROR GetInterfaceName(InterfaceId intfId, char *nameBuf, size_t nameBufSize);

/**
 * @brief   Search the list of network interfaces for the indicated name.
 *
 * @param[in]   intfName    name of the network interface to find
 * @param[out]  intfId      indicator of the network interface to assign
 *
 * @retval  INET_NO_ERROR                 success, network interface indicated
 * @retval  INET_ERROR_UNKNOWN_INTERFACE  no network interface found
 * @retval  other                   another system or platform error
 *
 * @details
 *  On LwIP, this function must be called with the LwIP stack lock acquired.
 *
 *  The \c intfId parameter is not updated unless the value returned is
 *  \c INET_NO_ERROR. It should be initialized with \c INET_NULL_INTERFACEID
 *  before calling this function.
 */
extern INET_ERROR InterfaceNameToId(const char *intfName, InterfaceId& intfId);

/**
 * @brief   Iterator basis for derived iterators for the list of system
 *          network interfaces or for the list of system network interface
 *          IP addresses.
 *
 */
class InterfaceIteratorBasis
{
 protected:
    /**
     * @brief   Conventional default constructor.
     *
     * @details
     *  Starts the cursor at the first network interface. On some platforms,
     *  this constructor may allocate resources recycled by the destructor.
     *
     *  On LwIP, this constructor must be called with the LwIP stack lock
     *  acquired.
     */
    InterfaceIteratorBasis(void);

    /**
     * @brief   Non-virtual destructor.
     *
     * @details
     *  Recycles any resources allocated by the constructor.
     *
     *  On LwIP, this destructor must be called with the LwIP stack lock
     *  acquired.
     */
    ~InterfaceIteratorBasis(void);

 public:
    /**
     * @brief   Test whether the cursor is not yet positioned beyond the end.
     *
     * @return  \c false if positioned beyond the end, else \c true.
     *
     * @details
     *  Advances the internal cursor either to the next network interface or to
     *  the distinguished position corresponding to no further interfaces.
     *
     *  On LwIP, this method must be called with the LwIP stack lock
     *  acquired.
     */
    bool HasCurrent(void) const
    {
#if WEAVE_SYSTEM_CONFIG_USE_LWIP
        return (curIntf != NULL);
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS
        return (curAddr != NULL);
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS
    }

    /**
     * @brief   Extract the indicator of the network interface at the cursor.
     *
     * @retval  INET_NULL_INTERFACEID   if advanced beyond the end of the list.
     * @retval  id                      the indicator of the current interface.
     *
     * @details
     *  On LwIP, this method must be called with the LwIP stack lock
     *  acquired.
     */
    InterfaceId GetInterface(void);

    /**
     * @brief   Inspect whether the current interface supports multicast.
     *
     * @return  \c false if the current interface does not support multicast or
     *      the cursor has advanced beyond the end of the list, else \c true.
     *
     * @details
     *  On LwIP, this method must be called with the LwIP stack lock
     *  acquired.
     */
    bool SupportsMulticast(void);

protected:
#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    struct netif *curIntf;
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS
    struct ifaddrs *addrsList;
    struct ifaddrs *curAddr;
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS
};

/**
 * @brief   Iterator for the list of system network interfaces.
 *
 * @details
 *  Use objects of this class to iterate the list of system network interfaces.
 *
 *  On LwIP systems, it is recommended that the LwIP stack lock be acquired and
 *  not released over the entire lifetime of an object of this class.
 *
 *  On some platforms, network interfaces without any IP addresses attached are
 *  not iterated.
 */
class InterfaceIterator :
    public InterfaceIteratorBasis
{
public:
    /**
     * @brief   Conventional default constructor.
     *
     * @details
     *  Starts the cursor at the first network interface. On some platforms,
     *  this constructor may allocate resources recycled by the destructor.
     *
     *  On LwIP, this constructor must be called with the LwIP stack lock
     *  acquired.
     */
    InterfaceIterator(void);

    /**
     * @brief   Non-virtual destructor.
     *
     * @details
     *  Recycles any resources allocated by the constructor.
     *
     *  On LwIP, this destructor must be called with the LwIP stack lock
     *  acquired.
     */
    ~InterfaceIterator(void);

    /**
     * @brief   Advance the cursor to the next network interface.
     *
     * @return  \c false if advanced beyond the end, else \c true.
     *
     * @details
     *  Advances the internal cursor either to the next network interface or
     *  to the distinguished position corresponding to no further interfaces.
     *
     *  On LwIP, this method must be called with the LwIP stack lock
     *  acquired.
     */
    bool Next(void);
};

/**
 * @brief   Iterator for the list of system network interface IP addresses.
 *
 * @details
 *  Use objects of this class to iterate the list of system network interface
 *  interface IP addresses.
 *
 *  On LwIP systems, it is recommended that the LwIP stack lock be acquired and
 *  not released over the entire lifetime of an object of this class.
 */
class NL_DLL_EXPORT InterfaceAddressIterator :
    public InterfaceIteratorBasis
{
public:
    /**
     * @brief   Conventional default constructor.
     *
     * @details
     *  Starts the cursor at the first network interface. On some platforms,
     *  this constructor may allocate resources recycled by the destructor.
     *
     *  On LwIP, this constructor must be called with the LwIP stack lock
     *  acquired.
     */
    InterfaceAddressIterator(void);

    /**
     * @brief   Non-virtual destructor.
     *
     * @details
     *  Recycles any resources allocated by the constructor.
     *
     *  On LwIP, this destructor must be called with the LwIP stack lock
     *  acquired.
     */
    ~InterfaceAddressIterator(void);

    /**
     * @brief   Advance the cursor to the next network interface IP address.
     *
     * @return  \c false if advanced beyond the end, else \c true.
     *
     * @details
     *  Advances the internal cursor either to the next network interface
     *  address or to the distinguished position corresponding to no further
     *  interface addresses.
     *
     *  On LwIP, this method must be called with the LwIP stack lock
     *  acquired.
     */
    bool Next(void);

    /**
     * @brief   Extract the current interface IP address.
     *
     * @return
     *      The current interface IP address, or \c IPAddress::Any if advanced
     *      beyond the end of the list.
     *
     * @details
     *  On LwIP, this method must be called with the LwIP stack lock
     *  acquired.
     */
    IPAddress GetAddress(void);

    /**
     * @brief   Extract the length of the subnet prefix for the current
     *      network interface IPv6 address.
     *
     * @return  length of the subnet prefix for the current IPv6 address, or
     *      zero if the current address is IPv4 or the iterator has
     *      advanced beyond the end of the list.
     *
     * @details
     *  On LwIP, this method simply returns the hard-coded constant 64.
     *
     *  Note Well: the standard subnet prefix on all links other than PPP
     *  links is 64 bits. On PPP links and some non-broadcast multipoint access
     *  links, the convention is either 127 bits or 128 bits, but it might be
     *  something else. On most platforms, the system's interface address
     *  structure can represent arbitrary prefix lengths between 0 and 128.
     */
    uint8_t GetIPv6PrefixLength(void);

private:
#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    int curAddrIndex;
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP
};

#if WEAVE_SYSTEM_CONFIG_USE_LWIP

inline uint8_t InterfaceAddressIterator::GetIPv6PrefixLength(void)
{
    return 64;
}

#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

} // namespace Inet
} // namespace nl

#endif /* INETINTERFACE_H */
