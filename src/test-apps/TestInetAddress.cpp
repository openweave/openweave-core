/*
 *
 *    Copyright (c) 2018-2019 Google LLC.
 *    Copyright (c) 2015-2018 Nest Labs, Inc.
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
 *     This file implements a unit test suite for <tt>nl::Inet::IPAddress</tt>,
 *     a class to store and format IPV4 and IPV6 Internet Protocol addresses.
 *
 */

#include <InetLayer/IPAddress.h>
#include "ToolCommon.h"

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
#include "lwip/init.h"
#include "lwip/ip_addr.h"

#if LWIP_DONT_PROVIDE_BYTEORDER_FUNCTIONS
#define htonl(x)    lwip_htonl(x)
#endif

#else
#include <sys/socket.h>
#include <netinet/in.h>
#endif

#include <nlunit-test.h>

using namespace nl::Inet;

// Preprocessor Defintions

#define LLA_PREFIX             0xfe800000
#define ULA_PREFIX             0xfd000000
#define MCAST_PREFIX           0xff000000
#define NUM_MCAST_SCOPES       7
#define NUM_MCAST_GROUPS       2
#define NUM_BYTES_IN_IPV6      16
#define ULA_UP_24_BIT_MASK     0xffffff0000
#define ULA_LO_16_BIT_MASK     0x000000ffff
#define NUM_FIELDS_IN_ADDR     sizeof(IPAddress)/sizeof(uint32_t)

// Type Defintions

// Test input vector format.

enum
{
    kTestIsIPv4             = true,
    kTestIsIPv6             = false,

    kTestIsIPv4Multicast    = true,
    kTestIsNotIPv4Multicast = false,

    kTestIsIPv4Broadcast    = true,
    kTestIsNotIPv4Broadcast = false,

    kTestIsMulticast        = true,
    kTestIsNotMulticast     = false,

    kTestIsIPv6Multicast    = true,
    kTestIsNotIPv6Multicast = false,

    kTestIsIPv6ULA          = true,
    kTestIsNotIPv6ULA       = false,

    kTestIsIPv6LLA          = true,
    kTestIsNotIPv6LLA       = false
};

struct TestContext {
    uint32_t                   addr[4];
    const IPAddressType        ipAddrType;

    const char                 *ip;

    bool                       isIPv4;
    bool                       isIPv4Multicast;
    bool                       isIPv4Broadcast;
    bool                       isMulticast;
    bool                       isIPv6Multicast;
    bool                       isIPv6ULA;
    bool                       isIPv6LLA;

    uint64_t                   global;
    uint16_t                   subnet;
    uint64_t                   interface;
};

// Global Variables

// Test input data.

static const struct TestContext sContext[] = {
    {
        { 0x26200000, 0x10e70400, 0xe83fb28f, 0x9c3a1941 }, kIPAddressType_IPv6,
        "2620:0:10e7:400:e83f:b28f:9c3a:1941",
        kTestIsIPv6, kTestIsNotIPv4Multicast, kTestIsNotIPv4Broadcast, kTestIsNotMulticast, kTestIsNotIPv6Multicast, kTestIsNotIPv6ULA, kTestIsNotIPv6LLA,
        0x0, 0x0, 0x0
    },
    {
        { 0xfe800000, 0x00000000, 0x8edcd4ff, 0xfe3aebfb }, kIPAddressType_IPv6,
        "fe80::8edc:d4ff:fe3a:ebfb",
        kTestIsIPv6, kTestIsNotIPv4Multicast, kTestIsNotIPv4Broadcast, kTestIsNotMulticast, kTestIsNotIPv6Multicast, kTestIsNotIPv6ULA, kTestIsIPv6LLA,
        0x0, 0x0, 0x0
    },
    {
        { 0xff010000, 0x00000000, 0x00000000, 0x00000001 }, kIPAddressType_IPv6,
        "ff01::1",
        kTestIsIPv6, kTestIsNotIPv4Multicast, kTestIsNotIPv4Broadcast, kTestIsMulticast, kTestIsIPv6Multicast, kTestIsNotIPv6ULA, kTestIsNotIPv6LLA,
        0x0, 0x0, 0x0
    },
    {
        { 0xfd000000, 0x00010001, 0x00000000, 0x00000001 }, kIPAddressType_IPv6,
        "fd00:0:1:1::1",
        kTestIsIPv6, kTestIsNotIPv4Multicast, kTestIsNotIPv4Broadcast, kTestIsNotMulticast, kTestIsNotIPv6Multicast, kTestIsIPv6ULA, kTestIsNotIPv6LLA,
        0x1, 1, 1
    },
    {
        { 0xfd123456, 0x0001abcd, 0xabcdef00, 0xfedcba09 }, kIPAddressType_IPv6,
        "fd12:3456:1:abcd:abcd:ef00:fedc:ba09",
        kTestIsIPv6, kTestIsNotIPv4Multicast, kTestIsNotIPv4Broadcast, kTestIsNotMulticast, kTestIsNotIPv6Multicast, kTestIsIPv6ULA, kTestIsNotIPv6LLA,
        0x1234560001, 0xabcd, 0xabcdef00fedcba09
    },
    {
        { 0xfdffffff, 0xffffffff, 0xffffffff, 0xffffffff }, kIPAddressType_IPv6,
        "fdff:ffff:ffff:ffff:ffff:ffff:ffff:ffff",
        kTestIsIPv6, kTestIsNotIPv4Multicast, kTestIsNotIPv4Broadcast, kTestIsNotMulticast, kTestIsNotIPv6Multicast, kTestIsIPv6ULA, kTestIsNotIPv6LLA,
        0xffffffffff, 0xffff, 0xffffffffffffffff
    },
#if INET_CONFIG_ENABLE_IPV4
    // IPv4-only
    {
        { 0x00000000, 0x00000000, 0x0000ffff, 0xffffff00 }, kIPAddressType_IPv4,
        "255.255.255.0",
        kTestIsIPv4, kTestIsNotIPv4Multicast, kTestIsNotIPv4Broadcast, kTestIsNotMulticast, kTestIsNotIPv6Multicast, kTestIsNotIPv6ULA, kTestIsNotIPv6LLA,
        0x0, 0x0, 0x0
    },
    {
        { 0x00000000, 0x00000000, 0x0000ffff, 0x7f000001 }, kIPAddressType_IPv4,
        "127.0.0.1",
        kTestIsIPv4, kTestIsNotIPv4Multicast, kTestIsNotIPv4Broadcast, kTestIsNotMulticast, kTestIsNotIPv6Multicast, kTestIsNotIPv6ULA, kTestIsNotIPv6LLA,
        0x0, 0x0, 0x0
    },
    // IPv4 and IPv4 multicast

    // IPv4 Local subnetwork multicast
    {
        { 0x00000000, 0x00000000, 0x0000ffff, 0xe0000000 }, kIPAddressType_IPv4,
        "224.0.0.0",
        kTestIsIPv4, kTestIsIPv4Multicast, kTestIsNotIPv4Broadcast, kTestIsMulticast, kTestIsNotIPv6Multicast, kTestIsNotIPv6ULA, kTestIsNotIPv6LLA,
        0x0, 0x0, 0x0
    },
    {
        { 0x00000000, 0x00000000, 0x0000ffff, 0xe0000001 }, kIPAddressType_IPv4,
        "224.0.0.1",
        kTestIsIPv4, kTestIsIPv4Multicast, kTestIsNotIPv4Broadcast, kTestIsMulticast, kTestIsNotIPv6Multicast, kTestIsNotIPv6ULA, kTestIsNotIPv6LLA,
        0x0, 0x0, 0x0
    },
    {
        { 0x00000000, 0x00000000, 0x0000ffff, 0xe0000080 }, kIPAddressType_IPv4,
        "224.0.0.128",
        kTestIsIPv4, kTestIsIPv4Multicast, kTestIsNotIPv4Broadcast, kTestIsMulticast, kTestIsNotIPv6Multicast, kTestIsNotIPv6ULA, kTestIsNotIPv6LLA,
        0x0, 0x0, 0x0
    },
    {
        { 0x00000000, 0x00000000, 0x0000ffff, 0xe00000fe }, kIPAddressType_IPv4,
        "224.0.0.254",
        kTestIsIPv4, kTestIsIPv4Multicast, kTestIsNotIPv4Broadcast, kTestIsMulticast, kTestIsNotIPv6Multicast, kTestIsNotIPv6ULA, kTestIsNotIPv6LLA,
        0x0, 0x0, 0x0
    },
    {
        { 0x00000000, 0x00000000, 0x0000ffff, 0xe00000ff }, kIPAddressType_IPv4,
        "224.0.0.255",
        kTestIsIPv4, kTestIsIPv4Multicast, kTestIsNotIPv4Broadcast, kTestIsMulticast, kTestIsNotIPv6Multicast, kTestIsNotIPv6ULA, kTestIsNotIPv6LLA,
        0x0, 0x0, 0x0
    },
    // IPv4 Internetwork control multicast
    {
        { 0x00000000, 0x00000000, 0x0000ffff, 0xe0000100 }, kIPAddressType_IPv4,
        "224.0.1.0",
        kTestIsIPv4, kTestIsIPv4Multicast, kTestIsNotIPv4Broadcast, kTestIsMulticast, kTestIsNotIPv6Multicast, kTestIsNotIPv6ULA, kTestIsNotIPv6LLA,
        0x0, 0x0, 0x0
    },
    {
        { 0x00000000, 0x00000000, 0x0000ffff, 0xe0000101 }, kIPAddressType_IPv4,
        "224.0.1.1",
        kTestIsIPv4, kTestIsIPv4Multicast, kTestIsNotIPv4Broadcast, kTestIsMulticast, kTestIsNotIPv6Multicast, kTestIsNotIPv6ULA, kTestIsNotIPv6LLA,
        0x0, 0x0, 0x0
    },
    {
        { 0x00000000, 0x00000000, 0x0000ffff, 0xe0000180 }, kIPAddressType_IPv4,
        "224.0.1.128",
        kTestIsIPv4, kTestIsIPv4Multicast, kTestIsNotIPv4Broadcast, kTestIsMulticast, kTestIsNotIPv6Multicast, kTestIsNotIPv6ULA, kTestIsNotIPv6LLA,
        0x0, 0x0, 0x0
    },
    {
        { 0x00000000, 0x00000000, 0x0000ffff, 0xe00001fe }, kIPAddressType_IPv4,
        "224.0.1.254",
        kTestIsIPv4, kTestIsIPv4Multicast, kTestIsNotIPv4Broadcast, kTestIsMulticast, kTestIsNotIPv6Multicast, kTestIsNotIPv6ULA, kTestIsNotIPv6LLA,
        0x0, 0x0, 0x0
    },
    {
        { 0x00000000, 0x00000000, 0x0000ffff, 0xe00001ff }, kIPAddressType_IPv4,
        "224.0.1.255",
        kTestIsIPv4, kTestIsIPv4Multicast, kTestIsNotIPv4Broadcast, kTestIsMulticast, kTestIsNotIPv6Multicast, kTestIsNotIPv6ULA, kTestIsNotIPv6LLA,
        0x0, 0x0, 0x0
    },
    // IPv4 AD-HOC block 1 multicast
    {
        { 0x00000000, 0x00000000, 0x0000ffff, 0xe0000200 }, kIPAddressType_IPv4,
        "224.0.2.0",
        kTestIsIPv4, kTestIsIPv4Multicast, kTestIsNotIPv4Broadcast, kTestIsMulticast, kTestIsNotIPv6Multicast, kTestIsNotIPv6ULA, kTestIsNotIPv6LLA,
        0x0, 0x0, 0x0
    },
    {
        { 0x00000000, 0x00000000, 0x0000ffff, 0xe0000201 }, kIPAddressType_IPv4,
        "224.0.2.1",
        kTestIsIPv4, kTestIsIPv4Multicast, kTestIsNotIPv4Broadcast, kTestIsMulticast, kTestIsNotIPv6Multicast, kTestIsNotIPv6ULA, kTestIsNotIPv6LLA,
        0x0, 0x0, 0x0
    },
    {
        { 0x00000000, 0x00000000, 0x0000ffff, 0xe0008100 }, kIPAddressType_IPv4,
        "224.0.129.0",
        kTestIsIPv4, kTestIsIPv4Multicast, kTestIsNotIPv4Broadcast, kTestIsMulticast, kTestIsNotIPv6Multicast, kTestIsNotIPv6ULA, kTestIsNotIPv6LLA,
        0x0, 0x0, 0x0
    },
    {
        { 0x00000000, 0x00000000, 0x0000ffff, 0xe000fffe }, kIPAddressType_IPv4,
        "224.0.255.254",
        kTestIsIPv4, kTestIsIPv4Multicast, kTestIsNotIPv4Broadcast, kTestIsMulticast, kTestIsNotIPv6Multicast, kTestIsNotIPv6ULA, kTestIsNotIPv6LLA,
        0x0, 0x0, 0x0
    },
    {
        { 0x00000000, 0x00000000, 0x0000ffff, 0xe000ffff }, kIPAddressType_IPv4,
        "224.0.255.255",
        kTestIsIPv4, kTestIsIPv4Multicast, kTestIsNotIPv4Broadcast, kTestIsMulticast, kTestIsNotIPv6Multicast, kTestIsNotIPv6ULA, kTestIsNotIPv6LLA,
        0x0, 0x0, 0x0
    },
    // IPv4 AD-HOC block 2 multicast
    {
        { 0x00000000, 0x00000000, 0x0000ffff, 0xe0030000 }, kIPAddressType_IPv4,
        "224.3.0.0",
        kTestIsIPv4, kTestIsIPv4Multicast, kTestIsNotIPv4Broadcast, kTestIsMulticast, kTestIsNotIPv6Multicast, kTestIsNotIPv6ULA, kTestIsNotIPv6LLA,
        0x0, 0x0, 0x0
    },
    {
        { 0x00000000, 0x00000000, 0x0000ffff, 0xe0030001 }, kIPAddressType_IPv4,
        "224.3.0.1",
        kTestIsIPv4, kTestIsIPv4Multicast, kTestIsNotIPv4Broadcast, kTestIsMulticast, kTestIsNotIPv6Multicast, kTestIsNotIPv6ULA, kTestIsNotIPv6LLA,
        0x0, 0x0, 0x0
    },
    {
        { 0x00000000, 0x00000000, 0x0000ffff, 0xe0040000 }, kIPAddressType_IPv4,
        "224.4.0.0",
        kTestIsIPv4, kTestIsIPv4Multicast, kTestIsNotIPv4Broadcast, kTestIsMulticast, kTestIsNotIPv6Multicast, kTestIsNotIPv6ULA, kTestIsNotIPv6LLA,
        0x0, 0x0, 0x0
    },
    {
        { 0x00000000, 0x00000000, 0x0000ffff, 0xe004fffe }, kIPAddressType_IPv4,
        "224.4.255.254",
        kTestIsIPv4, kTestIsIPv4Multicast, kTestIsNotIPv4Broadcast, kTestIsMulticast, kTestIsNotIPv6Multicast, kTestIsNotIPv6ULA, kTestIsNotIPv6LLA,
        0x0, 0x0, 0x0
    },
    {
        { 0x00000000, 0x00000000, 0x0000ffff, 0xe004ffff }, kIPAddressType_IPv4,
        "224.4.255.255",
        kTestIsIPv4, kTestIsIPv4Multicast, kTestIsNotIPv4Broadcast, kTestIsMulticast, kTestIsNotIPv6Multicast, kTestIsNotIPv6ULA, kTestIsNotIPv6LLA,
        0x0, 0x0, 0x0
    },
    // IPv4 source-specific multicast
    {
        { 0x00000000, 0x00000000, 0x0000ffff, 0xe8000000 }, kIPAddressType_IPv4,
        "232.0.0.0",
        kTestIsIPv4, kTestIsIPv4Multicast, kTestIsNotIPv4Broadcast, kTestIsMulticast, kTestIsNotIPv6Multicast, kTestIsNotIPv6ULA, kTestIsNotIPv6LLA,
        0x0, 0x0, 0x0
    },
    {
        { 0x00000000, 0x00000000, 0x0000ffff, 0xe8000001 }, kIPAddressType_IPv4,
        "232.0.0.1",
        kTestIsIPv4, kTestIsIPv4Multicast, kTestIsNotIPv4Broadcast, kTestIsMulticast, kTestIsNotIPv6Multicast, kTestIsNotIPv6ULA, kTestIsNotIPv6LLA,
        0x0, 0x0, 0x0
    },
    {
        { 0x00000000, 0x00000000, 0x0000ffff, 0xe8800000 }, kIPAddressType_IPv4,
        "232.128.0.0",
        kTestIsIPv4, kTestIsIPv4Multicast, kTestIsNotIPv4Broadcast, kTestIsMulticast, kTestIsNotIPv6Multicast, kTestIsNotIPv6ULA, kTestIsNotIPv6LLA,
        0x0, 0x0, 0x0
    },
    {
        { 0x00000000, 0x00000000, 0x0000ffff, 0xe8fffffe }, kIPAddressType_IPv4,
        "232.255.255.254",
        kTestIsIPv4, kTestIsIPv4Multicast, kTestIsNotIPv4Broadcast, kTestIsMulticast, kTestIsNotIPv6Multicast, kTestIsNotIPv6ULA, kTestIsNotIPv6LLA,
        0x0, 0x0, 0x0
    },
    {
        { 0x00000000, 0x00000000, 0x0000ffff, 0xe8ffffff }, kIPAddressType_IPv4,
        "232.255.255.255",
        kTestIsIPv4, kTestIsIPv4Multicast, kTestIsNotIPv4Broadcast, kTestIsMulticast, kTestIsNotIPv6Multicast, kTestIsNotIPv6ULA, kTestIsNotIPv6LLA,
        0x0, 0x0, 0x0
    },
    // IPv4 GLOP addressing multicast
    {
        { 0x00000000, 0x00000000, 0x0000ffff, 0xe9000000 }, kIPAddressType_IPv4,
        "233.0.0.0",
        kTestIsIPv4, kTestIsIPv4Multicast, kTestIsNotIPv4Broadcast, kTestIsMulticast, kTestIsNotIPv6Multicast, kTestIsNotIPv6ULA, kTestIsNotIPv6LLA,
        0x0, 0x0, 0x0
    },
    {
        { 0x00000000, 0x00000000, 0x0000ffff, 0xe9000001 }, kIPAddressType_IPv4,
        "233.0.0.1",
        kTestIsIPv4, kTestIsIPv4Multicast, kTestIsNotIPv4Broadcast, kTestIsMulticast, kTestIsNotIPv6Multicast, kTestIsNotIPv6ULA, kTestIsNotIPv6LLA,
        0x0, 0x0, 0x0
    },
    {
        { 0x00000000, 0x00000000, 0x0000ffff, 0xe97e0000 }, kIPAddressType_IPv4,
        "233.126.0.0",
        kTestIsIPv4, kTestIsIPv4Multicast, kTestIsNotIPv4Broadcast, kTestIsMulticast, kTestIsNotIPv6Multicast, kTestIsNotIPv6ULA, kTestIsNotIPv6LLA,
        0x0, 0x0, 0x0
    },
    {
        { 0x00000000, 0x00000000, 0x0000ffff, 0xe9fbfffe }, kIPAddressType_IPv4,
        "233.251.255.254",
        kTestIsIPv4, kTestIsIPv4Multicast, kTestIsNotIPv4Broadcast, kTestIsMulticast, kTestIsNotIPv6Multicast, kTestIsNotIPv6ULA, kTestIsNotIPv6LLA,
        0x0, 0x0, 0x0
    },
    {
        { 0x00000000, 0x00000000, 0x0000ffff, 0xe9fbffff }, kIPAddressType_IPv4,
        "233.251.255.255",
        kTestIsIPv4, kTestIsIPv4Multicast, kTestIsNotIPv4Broadcast, kTestIsMulticast, kTestIsNotIPv6Multicast, kTestIsNotIPv6ULA, kTestIsNotIPv6LLA,
        0x0, 0x0, 0x0
    },
    // IPv4 AD-HOC block 3 multicast
    {
        { 0x00000000, 0x00000000, 0x0000ffff, 0xe9fc0000 }, kIPAddressType_IPv4,
        "233.252.0.0",
        kTestIsIPv4, kTestIsIPv4Multicast, kTestIsNotIPv4Broadcast, kTestIsMulticast, kTestIsNotIPv6Multicast, kTestIsNotIPv6ULA, kTestIsNotIPv6LLA,
        0x0, 0x0, 0x0
    },
    {
        { 0x00000000, 0x00000000, 0x0000ffff, 0xe9fc0001 }, kIPAddressType_IPv4,
        "233.252.0.1",
        kTestIsIPv4, kTestIsIPv4Multicast, kTestIsNotIPv4Broadcast, kTestIsMulticast, kTestIsNotIPv6Multicast, kTestIsNotIPv6ULA, kTestIsNotIPv6LLA,
        0x0, 0x0, 0x0
    },
    {
        { 0x00000000, 0x00000000, 0x0000ffff, 0xe9fe0000 }, kIPAddressType_IPv4,
        "233.254.0.0",
        kTestIsIPv4, kTestIsIPv4Multicast, kTestIsNotIPv4Broadcast, kTestIsMulticast, kTestIsNotIPv6Multicast, kTestIsNotIPv6ULA, kTestIsNotIPv6LLA,
        0x0, 0x0, 0x0
    },
    {
        { 0x00000000, 0x00000000, 0x0000ffff, 0xe9fffffe }, kIPAddressType_IPv4,
        "233.255.255.254",
        kTestIsIPv4, kTestIsIPv4Multicast, kTestIsNotIPv4Broadcast, kTestIsMulticast, kTestIsNotIPv6Multicast, kTestIsNotIPv6ULA, kTestIsNotIPv6LLA,
        0x0, 0x0, 0x0
    },
    {
        { 0x00000000, 0x00000000, 0x0000ffff, 0xe9ffffff }, kIPAddressType_IPv4,
        "233.255.255.255",
        kTestIsIPv4, kTestIsIPv4Multicast, kTestIsNotIPv4Broadcast, kTestIsMulticast, kTestIsNotIPv6Multicast, kTestIsNotIPv6ULA, kTestIsNotIPv6LLA,
        0x0, 0x0, 0x0
    },
    // IPv4 unicast-prefix-based multicast
    {
        { 0x00000000, 0x00000000, 0x0000ffff, 0xea000000 }, kIPAddressType_IPv4,
        "234.0.0.0",
        kTestIsIPv4, kTestIsIPv4Multicast, kTestIsNotIPv4Broadcast, kTestIsMulticast, kTestIsNotIPv6Multicast, kTestIsNotIPv6ULA, kTestIsNotIPv6LLA,
        0x0, 0x0, 0x0
    },
    {
        { 0x00000000, 0x00000000, 0x0000ffff, 0xea000001 }, kIPAddressType_IPv4,
        "234.0.0.1",
        kTestIsIPv4, kTestIsIPv4Multicast, kTestIsNotIPv4Broadcast, kTestIsMulticast, kTestIsNotIPv6Multicast, kTestIsNotIPv6ULA, kTestIsNotIPv6LLA,
        0x0, 0x0, 0x0
    },
    {
        { 0x00000000, 0x00000000, 0x0000ffff, 0xea800000 }, kIPAddressType_IPv4,
        "234.128.0.0",
        kTestIsIPv4, kTestIsIPv4Multicast, kTestIsNotIPv4Broadcast, kTestIsMulticast, kTestIsNotIPv6Multicast, kTestIsNotIPv6ULA, kTestIsNotIPv6LLA,
        0x0, 0x0, 0x0
    },
    {
        { 0x00000000, 0x00000000, 0x0000ffff, 0xeafffffe }, kIPAddressType_IPv4,
        "234.255.255.254",
        kTestIsIPv4, kTestIsIPv4Multicast, kTestIsNotIPv4Broadcast, kTestIsMulticast, kTestIsNotIPv6Multicast, kTestIsNotIPv6ULA, kTestIsNotIPv6LLA,
        0x0, 0x0, 0x0
    },
    {
        { 0x00000000, 0x00000000, 0x0000ffff, 0xeaffffff }, kIPAddressType_IPv4,
        "234.255.255.255",
        kTestIsIPv4, kTestIsIPv4Multicast, kTestIsNotIPv4Broadcast, kTestIsMulticast, kTestIsNotIPv6Multicast, kTestIsNotIPv6ULA, kTestIsNotIPv6LLA,
        0x0, 0x0, 0x0
    },
    // IPv4 administratively scoped multicast
    {
        { 0x00000000, 0x00000000, 0x0000ffff, 0xef000000 }, kIPAddressType_IPv4,
        "239.0.0.0",
        kTestIsIPv4, kTestIsIPv4Multicast, kTestIsNotIPv4Broadcast, kTestIsMulticast, kTestIsNotIPv6Multicast, kTestIsNotIPv6ULA, kTestIsNotIPv6LLA,
        0x0, 0x0, 0x0
    },
    {
        { 0x00000000, 0x00000000, 0x0000ffff, 0xef000001 }, kIPAddressType_IPv4,
        "239.0.0.1",
        kTestIsIPv4, kTestIsIPv4Multicast, kTestIsNotIPv4Broadcast, kTestIsMulticast, kTestIsNotIPv6Multicast, kTestIsNotIPv6ULA, kTestIsNotIPv6LLA,
        0x0, 0x0, 0x0
    },
    {
        { 0x00000000, 0x00000000, 0x0000ffff, 0xef800000 }, kIPAddressType_IPv4,
        "239.128.0.0",
        kTestIsIPv4, kTestIsIPv4Multicast, kTestIsNotIPv4Broadcast, kTestIsMulticast, kTestIsNotIPv6Multicast, kTestIsNotIPv6ULA, kTestIsNotIPv6LLA,
        0x0, 0x0, 0x0
    },
    {
        { 0x00000000, 0x00000000, 0x0000ffff, 0xeffffffe }, kIPAddressType_IPv4,
        "239.255.255.254",
        kTestIsIPv4, kTestIsIPv4Multicast, kTestIsNotIPv4Broadcast, kTestIsMulticast, kTestIsNotIPv6Multicast, kTestIsNotIPv6ULA, kTestIsNotIPv6LLA,
        0x0, 0x0, 0x0
    },
    {
        { 0x00000000, 0x00000000, 0x0000ffff, 0xefffffff }, kIPAddressType_IPv4,
        "239.255.255.255",
        kTestIsIPv4, kTestIsIPv4Multicast, kTestIsNotIPv4Broadcast, kTestIsMulticast, kTestIsNotIPv6Multicast, kTestIsNotIPv6ULA, kTestIsNotIPv6LLA,
        0x0, 0x0, 0x0
    },
    // IP4 and IPv4 broadcast
    {
        { 0x00000000, 0x00000000, 0x0000ffff, 0xffffffff }, kIPAddressType_IPv4,
        "255.255.255.255",
        kTestIsIPv4, kTestIsNotIPv4Multicast, kTestIsIPv4Broadcast, kTestIsNotMulticast, kTestIsNotIPv6Multicast, kTestIsNotIPv6ULA, kTestIsNotIPv6LLA,
        0x0, 0x0, 0x0
    },
#endif // INET_CONFIG_ENABLE_IPV4
    {
        { 0x00000000, 0x00000000, 0x00000000, 0x00000000 }, kIPAddressType_Any,
        "::",
        kTestIsIPv6, kTestIsNotIPv4Multicast, kTestIsNotIPv4Broadcast, kTestIsNotMulticast, kTestIsNotIPv6Multicast, kTestIsNotIPv6ULA, kTestIsNotIPv6LLA,
        0x0, 0x0, 0x0
    }
};

static const size_t kTestElements = sizeof(sContext)/sizeof(struct TestContext);

// Utility functions.

/**
 *   Load input test directly into IPAddress.
 */
static void SetupIPAddress(IPAddress& addr, const struct TestContext *inContext)
{
    for (size_t i = 0; i < NUM_FIELDS_IN_ADDR; i++)
    {
        addr.Addr[i] = htonl(inContext->addr[i]);
    }
}

/**
 *   Zero out IP address.
 */
static void ClearIPAddress(IPAddress& addr)
{
    for (size_t i = 0; i < NUM_FIELDS_IN_ADDR; i++)
    {
        addr.Addr[i] = 0;
    }
}

// Test functions invoked from the suite.


/**
 *  Test IP address conversion from a string.
 */
static void CheckFromString(nlTestSuite *inSuite, void *inContext)
{
    const struct TestContext *theContext = static_cast<const struct TestContext *>(inContext);

    for (size_t ith = 0; ith < kTestElements; ith++)
    {
        IPAddress test_addr;

        IPAddress::FromString(theContext->ip, test_addr);

        NL_TEST_ASSERT(inSuite, test_addr.Addr[0] == htonl(theContext->addr[0]));
        NL_TEST_ASSERT(inSuite, test_addr.Addr[1] == htonl(theContext->addr[1]));
        NL_TEST_ASSERT(inSuite, test_addr.Addr[2] == htonl(theContext->addr[2]));
        NL_TEST_ASSERT(inSuite, test_addr.Addr[3] == htonl(theContext->addr[3]));

        char tmpBuf[INET6_ADDRSTRLEN];
        size_t addrStrLen = strlen(theContext->ip);

        memset(tmpBuf, '1', sizeof(tmpBuf));
        memcpy(tmpBuf, theContext->ip, addrStrLen);

        IPAddress::FromString(tmpBuf, addrStrLen, test_addr);

        NL_TEST_ASSERT(inSuite, test_addr.Addr[0] == htonl(theContext->addr[0]));
        NL_TEST_ASSERT(inSuite, test_addr.Addr[1] == htonl(theContext->addr[1]));
        NL_TEST_ASSERT(inSuite, test_addr.Addr[2] == htonl(theContext->addr[2]));
        NL_TEST_ASSERT(inSuite, test_addr.Addr[3] == htonl(theContext->addr[3]));

        theContext++;
    }
}

/**
 *  Test correct identification of IPv6 ULA addresses.
 */
static void CheckIsIPv6ULA(nlTestSuite *inSuite, void *inContext)
{
    const struct TestContext *theContext = static_cast<const struct TestContext *>(inContext);

    for (size_t ith = 0; ith < kTestElements; ith++)
    {
        IPAddress test_addr;

        SetupIPAddress(test_addr, theContext);

        NL_TEST_ASSERT(inSuite, test_addr.IsIPv6ULA() == theContext->isIPv6ULA);

        theContext++;
    }
}

/**
 *  Test correct identification of IPv6 Link Local addresses.
 */
static void CheckIsIPv6LLA(nlTestSuite *inSuite, void *inContext)
{
    const struct TestContext *theContext = static_cast<const struct TestContext *>(inContext);

    for (size_t ith = 0; ith < kTestElements; ith++)
    {
        IPAddress test_addr;

        SetupIPAddress(test_addr, theContext);

        NL_TEST_ASSERT(inSuite, test_addr.IsIPv6LinkLocal() == theContext->isIPv6LLA);

        theContext++;
    }
}

/**
 *  Test correct identification of IPv6 multicast addresses.
 */
static void CheckIsIPv6Multicast(nlTestSuite *inSuite, void *inContext)
{
    const struct TestContext *theContext = static_cast<const struct TestContext *>(inContext);

    for (size_t ith = 0; ith < kTestElements; ith++)
    {
        IPAddress test_addr;

        SetupIPAddress(test_addr, theContext);

        NL_TEST_ASSERT(inSuite, test_addr.IsIPv6Multicast() == theContext->isIPv6Multicast);

        theContext++;
    }
}

/**
 *  Test correct identification of multicast addresses.
 */
static void CheckIsMulticast(nlTestSuite *inSuite, void *inContext)
{
    const struct TestContext *theContext = static_cast<const struct TestContext *>(inContext);

    for (size_t ith = 0; ith < kTestElements; ith++)
    {
        IPAddress test_addr;

        SetupIPAddress(test_addr, theContext);

        NL_TEST_ASSERT(inSuite, test_addr.IsMulticast() == theContext->isMulticast);

        theContext++;
    }
}

/**
 *  Test IPAddress equal operator.
 */
static void CheckOperatorEqual(nlTestSuite *inSuite, void *inContext)
{
    struct TestContext *theFirstContext = static_cast<struct TestContext *>(inContext);

    for (size_t ith = 0; ith < kTestElements; ith++)
    {
        IPAddress test_addr_1;
        struct TestContext *theSecondContext = static_cast<struct TestContext *>(inContext);

        SetupIPAddress(test_addr_1, theFirstContext);

        for (size_t jth = 0; jth < kTestElements; jth++)
        {
            IPAddress test_addr_2;

            SetupIPAddress(test_addr_2, theSecondContext);

            if (theFirstContext == theSecondContext)
            {
                NL_TEST_ASSERT(inSuite, test_addr_1 == test_addr_2);
            }
            else
            {
                NL_TEST_ASSERT(inSuite, !(test_addr_1 == test_addr_2));
            }
            theSecondContext++;
        }
        theFirstContext++;
    }
}

/**
 *  Test IPAddress not-equal operator.
 */
static void CheckOperatorNotEqual(nlTestSuite *inSuite, void *inContext)
{
    struct TestContext *theFirstContext = static_cast<struct TestContext *>(inContext);

    for (size_t ith = 0; ith < kTestElements; ith++)
    {
        IPAddress test_addr_1;
        struct TestContext *theSecondContext = static_cast<struct TestContext *>(inContext);

        SetupIPAddress(test_addr_1, theFirstContext);

        for (size_t jth = 0; jth < kTestElements; jth++)
        {
            IPAddress test_addr_2;

            SetupIPAddress(test_addr_2, theSecondContext);

            if (theFirstContext == theSecondContext)
            {
                NL_TEST_ASSERT(inSuite, !(test_addr_1 != test_addr_2));
            }
            else
            {
                NL_TEST_ASSERT(inSuite, test_addr_1 != test_addr_2);
            }
            theSecondContext++;
        }
        theFirstContext++;
    }
}

/**
 *  Test IPAddress assign operator.
 */
static void CheckOperatorAssign(nlTestSuite *inSuite, void *inContext)
{
    struct TestContext *theFirstContext = static_cast<struct TestContext *>(inContext);

    for (size_t ith = 0; ith < kTestElements; ith++)
    {
        struct TestContext *theSecondContext = static_cast<struct TestContext *>(inContext);

        for (size_t jth = 0; jth < kTestElements; jth++)
        {
            IPAddress test_addr_1, test_addr_2;

            ClearIPAddress(test_addr_1);
            SetupIPAddress(test_addr_2, theSecondContext);

            // Use operator to assign IPAddress from test_addr_2 to test_addr_1
            test_addr_1 = test_addr_2;

            NL_TEST_ASSERT(inSuite, test_addr_1.Addr[0] == test_addr_2.Addr[0]);
            NL_TEST_ASSERT(inSuite, test_addr_1.Addr[1] == test_addr_2.Addr[1]);
            NL_TEST_ASSERT(inSuite, test_addr_1.Addr[2] == test_addr_2.Addr[2]);
            NL_TEST_ASSERT(inSuite, test_addr_1.Addr[3] == test_addr_2.Addr[3]);

            theSecondContext++;
        }
        theFirstContext++;
    }
}

/**
 *   Test IPAddress v6 conversion to native representation.
 */
static void CheckToIPv6(nlTestSuite *inSuite, void *inContext)
{
    const struct TestContext *theContext = static_cast<const struct TestContext *>(inContext);

    for (size_t ith = 0; ith < kTestElements; ith++)
    {
        IPAddress test_addr;
        uint32_t addr[4];

        addr[0] = htonl(theContext->addr[0]);
        addr[1] = htonl(theContext->addr[1]);
        addr[2] = htonl(theContext->addr[2]);
        addr[3] = htonl(theContext->addr[3]);

        SetupIPAddress(test_addr, theContext);

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
        ip6_addr_t ip_addr_1, ip_addr_2;
        ip_addr_1 = *(ip6_addr_t *)addr;
#else
        struct in6_addr ip_addr_1, ip_addr_2;
        ip_addr_1 = *(struct in6_addr *)addr;
#endif
        ip_addr_2 = test_addr.ToIPv6();

        NL_TEST_ASSERT(inSuite, !memcmp(&ip_addr_1, &ip_addr_2, sizeof(ip_addr_1)));

        theContext++;
    }
}

/**
 *   Test native IPv6 conversion into IPAddress.
 */
static void CheckFromIPv6(nlTestSuite *inSuite, void *inContext)
{
    const struct TestContext *theContext = static_cast<const struct TestContext *>(inContext);

    for (size_t ith = 0; ith < kTestElements; ith++)
    {
        IPAddress test_addr_1, test_addr_2;
        uint32_t addr[4];

        addr[0] = htonl(theContext->addr[0]);
        addr[1] = htonl(theContext->addr[1]);
        addr[2] = htonl(theContext->addr[2]);
        addr[3] = htonl(theContext->addr[3]);

        SetupIPAddress(test_addr_1, theContext);
        ClearIPAddress(test_addr_2);

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
        ip6_addr_t ip_addr;
        ip_addr = *(ip6_addr_t *)addr;
#else
        struct in6_addr ip_addr;
        ip_addr = *(struct in6_addr *)addr;
#endif
        test_addr_2 = IPAddress::FromIPv6(ip_addr);

        NL_TEST_ASSERT(inSuite, test_addr_1.Addr[0] == test_addr_2.Addr[0]);
        NL_TEST_ASSERT(inSuite, test_addr_1.Addr[1] == test_addr_2.Addr[1]);
        NL_TEST_ASSERT(inSuite, test_addr_1.Addr[2] == test_addr_2.Addr[2]);
        NL_TEST_ASSERT(inSuite, test_addr_1.Addr[3] == test_addr_2.Addr[3]);

        theContext++;
    }
}

#if INET_CONFIG_ENABLE_IPV4
/**
 *  Test correct identification of IPv4 addresses.
 */
static void CheckIsIPv4(nlTestSuite *inSuite, void *inContext)
{
    const struct TestContext *theContext = static_cast<const struct TestContext *>(inContext);

    for (size_t ith = 0; ith < kTestElements; ith++)
    {
        IPAddress test_addr;

        SetupIPAddress(test_addr, theContext);

        NL_TEST_ASSERT(inSuite, test_addr.IsIPv4() == theContext->isIPv4);

        theContext++;
    }
}

/**
 *  Test correct identification of IPv4 multicast addresses.
 */
static void CheckIsIPv4Multicast(nlTestSuite *inSuite, void *inContext)
{
    const struct TestContext *theContext = static_cast<const struct TestContext *>(inContext);

    for (size_t ith = 0; ith < kTestElements; ith++)
    {
        IPAddress test_addr;

        SetupIPAddress(test_addr, theContext);

        NL_TEST_ASSERT(inSuite, test_addr.IsIPv4Multicast() == theContext->isIPv4Multicast);

        theContext++;
    }
}

/**
 *  Test correct identification of IPv4 broadcast addresses.
 */
static void CheckIsIPv4Broadcast(nlTestSuite *inSuite, void *inContext)
{
    const struct TestContext *theContext = static_cast<const struct TestContext *>(inContext);

    for (size_t ith = 0; ith < kTestElements; ith++)
    {
        IPAddress test_addr;

        SetupIPAddress(test_addr, theContext);

        NL_TEST_ASSERT(inSuite, test_addr.IsIPv4Broadcast() == theContext->isIPv4Broadcast);

        theContext++;
    }
}

/**
 *   Test IPAddress v4 conversion to native representation.
 */
static void CheckToIPv4(nlTestSuite *inSuite, void *inContext)
{
    const struct TestContext *theContext = static_cast<const struct TestContext *>(inContext);

    for (size_t ith = 0; ith < kTestElements; ith++)
    {
        IPAddress test_addr;

        SetupIPAddress(test_addr, theContext);

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
        ip4_addr_t ip_addr_1, ip_addr_2;

        ip_addr_1.addr = htonl(theContext->addr[3]);
#else
        struct in_addr ip_addr_1, ip_addr_2;

        ip_addr_1.s_addr = htonl(theContext->addr[3]);
#endif
        ip_addr_2 = test_addr.ToIPv4();

        NL_TEST_ASSERT(inSuite, !memcmp(&ip_addr_1, &ip_addr_2, sizeof(ip_addr_1)));

        theContext++;
    }
}

/**
 *   Test native IPv4 conversion into IPAddress.
 */
static void CheckFromIPv4(nlTestSuite *inSuite, void *inContext)
{
    const struct TestContext *theContext = static_cast<const struct TestContext *>(inContext);

    for (size_t ith = 0; ith < kTestElements; ith++)
    {
        IPAddress test_addr_1, test_addr_2;

        SetupIPAddress(test_addr_1, theContext);
        ClearIPAddress(test_addr_2);

        // Convert to IPv4 (test_addr_1);
        test_addr_1.Addr[0] = 0;
        test_addr_1.Addr[1] = 0;

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
        ip4_addr_t ip_addr;
        ip_addr.addr = htonl(theContext->addr[3]);
        test_addr_1.Addr[2] = lwip_htonl(0xffff);
#else
        struct in_addr ip_addr;
        ip_addr.s_addr = htonl(theContext->addr[3]);
        test_addr_1.Addr[2] = htonl(0xffff);
#endif
        test_addr_2 = IPAddress::FromIPv4(ip_addr);

        NL_TEST_ASSERT(inSuite, test_addr_1.Addr[0] == test_addr_2.Addr[0]);
        NL_TEST_ASSERT(inSuite, test_addr_1.Addr[1] == test_addr_2.Addr[1]);
        NL_TEST_ASSERT(inSuite, test_addr_1.Addr[2] == test_addr_2.Addr[2]);
        NL_TEST_ASSERT(inSuite, test_addr_1.Addr[3] == test_addr_2.Addr[3]);

        theContext++;
    }
}
#endif // INET_CONFIG_ENABLE_IPV4

/**
 *   Test IPAddress address conversion from socket.
 */
static void CheckFromSocket(nlTestSuite *inSuite, void *inContext)
{
#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    (void)inSuite;
    // This test is only supported for non LWIP stack.
#else // INET_LWIP
    const struct TestContext *theContext = static_cast<const struct TestContext *>(inContext);

    for (size_t ith = 0; ith < kTestElements; ith++)
    {
        IPAddress test_addr_1, test_addr_2;
        uint32_t addr[4];
        struct sockaddr_in6 sock_v6;
#if INET_CONFIG_ENABLE_IPV4
        struct sockaddr_in sock_v4;
#endif // INET_CONFIG_ENABLE_IPV4

        addr[0] = htonl(theContext->addr[0]);
        addr[1] = htonl(theContext->addr[1]);
        addr[2] = htonl(theContext->addr[2]);
        addr[3] = htonl(theContext->addr[3]);

        SetupIPAddress(test_addr_1, theContext);
        ClearIPAddress(test_addr_2);

        switch (theContext->ipAddrType)
        {
#if INET_CONFIG_ENABLE_IPV4
        case kIPAddressType_IPv4:
            memset(&sock_v4, 0, sizeof(struct sockaddr_in));
            sock_v4.sin_family = AF_INET;
            memcpy(&sock_v4.sin_addr.s_addr, &addr[3], sizeof(struct in_addr));
            test_addr_2 = IPAddress::FromSockAddr((struct sockaddr &)sock_v4);
            break;
#endif // INET_CONFIG_ENABLE_IPV4

        case kIPAddressType_IPv6:
            memset(&sock_v6, 0, sizeof(struct sockaddr_in6));
            sock_v6.sin6_family = AF_INET6;
            memcpy(&sock_v6.sin6_addr.s6_addr, addr, sizeof(struct in6_addr));
            test_addr_2 = IPAddress::FromSockAddr((struct sockaddr &)sock_v6);
            break;

        case kIPAddressType_Any:
            memset(&sock_v6, 0, sizeof(struct sockaddr_in6));
            sock_v6.sin6_family = 0;
            memcpy(&sock_v6.sin6_addr.s6_addr, addr, sizeof(struct in6_addr));
            test_addr_2 = IPAddress::FromSockAddr((struct sockaddr &)sock_v6);
            break;

        default:
            continue;
        }

        NL_TEST_ASSERT(inSuite, test_addr_1.Addr[0] == test_addr_2.Addr[0]);
        NL_TEST_ASSERT(inSuite, test_addr_1.Addr[1] == test_addr_2.Addr[1]);
        NL_TEST_ASSERT(inSuite, test_addr_1.Addr[2] == test_addr_2.Addr[2]);
        NL_TEST_ASSERT(inSuite, test_addr_1.Addr[3] == test_addr_2.Addr[3]);

        theContext++;
    }
#endif // INET_LWIP
}

/**
 *  Test IP address type.
 */
static void CheckType(nlTestSuite *inSuite, void *inContext)
{
    const struct TestContext *theContext = static_cast<const struct TestContext *>(inContext);

    for (size_t ith = 0; ith < kTestElements; ith++)
    {
        IPAddress test_addr;

        SetupIPAddress(test_addr, theContext);

        NL_TEST_ASSERT(inSuite, test_addr.Type() == theContext->ipAddrType);

        theContext++;
    }
}

/**
 *  Test IP address interface ID.
 */
static void CheckInterface(nlTestSuite *inSuite, void *inContext)
{
    const struct TestContext *theContext = static_cast<const struct TestContext *>(inContext);

    for (size_t ith = 0; ith < kTestElements; ith++)
    {
        IPAddress test_addr;

        SetupIPAddress(test_addr, theContext);

        NL_TEST_ASSERT(inSuite, test_addr.InterfaceId() == theContext->interface);

        theContext++;
    }
}

/**
 *  Test IP address subnet.
 */
static void CheckSubnet(nlTestSuite *inSuite, void *inContext)
{
    const struct TestContext *theContext = static_cast<const struct TestContext *>(inContext);

    for (size_t ith = 0; ith < kTestElements; ith++)
    {
        IPAddress test_addr;

        SetupIPAddress(test_addr, theContext);

        NL_TEST_ASSERT(inSuite, test_addr.Subnet() == theContext->subnet);

        theContext++;
    }
}

/**
 *  Test IP address global ID.
 */
static void CheckGlobal(nlTestSuite *inSuite, void *inContext)
{
    const struct TestContext *theContext = static_cast<const struct TestContext *>(inContext);

    for (size_t ith = 0; ith < kTestElements; ith++)
    {
        IPAddress test_addr;

        SetupIPAddress(test_addr, theContext);

        NL_TEST_ASSERT(inSuite, test_addr.GlobalId() == theContext->global);

        theContext++;
    }
}

/**
 *  Test address encoding with Weave::Encoding.
 */
static void CheckEncoding(nlTestSuite *inSuite, void *inContext)
{
    const struct TestContext *theContext = static_cast<const struct TestContext *>(inContext);

    for (size_t ith = 0; ith < kTestElements; ith++)
    {
        IPAddress test_addr;
        uint8_t *p;
        uint8_t buffer[NUM_BYTES_IN_IPV6];

        SetupIPAddress(test_addr, theContext);
        memset(&buffer, 0, NUM_BYTES_IN_IPV6);
        p = buffer;

        // Call EncodeAddress function that we test.
        test_addr.WriteAddress(p);

        // buffer has address in network byte order
        NL_TEST_ASSERT(inSuite, buffer[3] == (uint8_t)(theContext->addr[0]));
        NL_TEST_ASSERT(inSuite, buffer[2] == (uint8_t)(theContext->addr[0] >> 8));
        NL_TEST_ASSERT(inSuite, buffer[1] == (uint8_t)(theContext->addr[0] >> 16));
        NL_TEST_ASSERT(inSuite, buffer[0] == (uint8_t)(theContext->addr[0] >> 24));

        NL_TEST_ASSERT(inSuite, buffer[7] == (uint8_t)(theContext->addr[1]));
        NL_TEST_ASSERT(inSuite, buffer[6] == (uint8_t)(theContext->addr[1] >> 8));
        NL_TEST_ASSERT(inSuite, buffer[5] == (uint8_t)(theContext->addr[1] >> 16));
        NL_TEST_ASSERT(inSuite, buffer[4] == (uint8_t)(theContext->addr[1] >> 24));

        NL_TEST_ASSERT(inSuite, buffer[11] == (uint8_t)(theContext->addr[2]));
        NL_TEST_ASSERT(inSuite, buffer[10] == (uint8_t)(theContext->addr[2] >> 8));
        NL_TEST_ASSERT(inSuite, buffer[9] == (uint8_t)(theContext->addr[2] >> 16));
        NL_TEST_ASSERT(inSuite, buffer[8] == (uint8_t)(theContext->addr[2] >> 24));

        NL_TEST_ASSERT(inSuite, buffer[15] == (uint8_t)(theContext->addr[3]));
        NL_TEST_ASSERT(inSuite, buffer[14] == (uint8_t)(theContext->addr[3] >> 8));
        NL_TEST_ASSERT(inSuite, buffer[13] == (uint8_t)(theContext->addr[3] >> 16));
        NL_TEST_ASSERT(inSuite, buffer[12] == (uint8_t)(theContext->addr[3] >> 24));

        theContext++;
    }
}

/**
 *  Test address decoding with Weave::Decoding.
 */
static void CheckDecoding(nlTestSuite *inSuite, void *inContext)
{
    const struct TestContext *theContext = static_cast<const struct TestContext *>(inContext);

    for (size_t ith = 0; ith < kTestElements; ith++)
    {
        IPAddress test_addr_1, test_addr_2;
        uint8_t buffer[NUM_BYTES_IN_IPV6];
        uint8_t *p;
        uint8_t b;

        SetupIPAddress(test_addr_1, theContext);
        ClearIPAddress(test_addr_2);
        memset(&buffer, 0, NUM_BYTES_IN_IPV6);
        p = buffer;

        for (b = 0; b < NUM_BYTES_IN_IPV6; b++)
        {
            buffer[b] = (uint8_t)(theContext->addr[b / 4] >> ((3 - b % 4) * 8));
        }

        // Call ReadAddress function that we test.
        IPAddress::ReadAddress(const_cast<const uint8_t *&>(p), test_addr_2);

        NL_TEST_ASSERT(inSuite, test_addr_1.Addr[0] == test_addr_2.Addr[0]);
        NL_TEST_ASSERT(inSuite, test_addr_1.Addr[1] == test_addr_2.Addr[1]);
        NL_TEST_ASSERT(inSuite, test_addr_1.Addr[2] == test_addr_2.Addr[2]);
        NL_TEST_ASSERT(inSuite, test_addr_1.Addr[3] == test_addr_2.Addr[3]);

        theContext++;
    }
}

/**
 *  Test address symmetricity of encoding and decoding with Weave::(De/En)code.
 */
static void CheckEcodeDecodeSymmetricity(nlTestSuite *inSuite, void *inContext)
{
    const struct TestContext *theContext = static_cast<const struct TestContext *>(inContext);

    for (size_t ith = 0; ith < kTestElements; ith++)
    {
        IPAddress test_addr_1, test_addr_2;
        uint8_t buffer[NUM_BYTES_IN_IPV6];
        uint8_t *p;

        SetupIPAddress(test_addr_1, theContext);
        ClearIPAddress(test_addr_2);
        memset(&buffer, 0, NUM_BYTES_IN_IPV6);

        p = buffer;

        // Call EncodeAddress function that we test.
        test_addr_1.WriteAddress(p);

        // Move pointer back to the beginning of the buffer.
        p = buffer;

        // Call ReadAddress function that we test.
        IPAddress::ReadAddress(const_cast<const uint8_t *&>(p), test_addr_2);

        NL_TEST_ASSERT(inSuite, test_addr_1.Addr[0] == test_addr_2.Addr[0]);
        NL_TEST_ASSERT(inSuite, test_addr_1.Addr[1] == test_addr_2.Addr[1]);
        NL_TEST_ASSERT(inSuite, test_addr_1.Addr[2] == test_addr_2.Addr[2]);
        NL_TEST_ASSERT(inSuite, test_addr_1.Addr[3] == test_addr_2.Addr[3]);

        theContext++;
    }
}

/**
 *  Test assembling ULA address.
 */
static void CheckMakeULA(nlTestSuite *inSuite, void *inContext)
{
    const struct TestContext *theContext = static_cast<const struct TestContext *>(inContext);

    for (size_t ith = 0; ith < kTestElements; ith++)
    {
        IPAddress test_addr;

        // Call MakeULA function that we test.
        test_addr = IPAddress::MakeULA(theContext->global, theContext->subnet,
            theContext->interface);

        NL_TEST_ASSERT(inSuite, test_addr.Addr[0] ==
                htonl(ULA_PREFIX | (theContext->global & ULA_UP_24_BIT_MASK) >> 16));
        NL_TEST_ASSERT(inSuite,    test_addr.Addr[1] ==
                htonl((theContext->global & ULA_LO_16_BIT_MASK) << 16 | theContext->subnet));
        NL_TEST_ASSERT(inSuite,    test_addr.Addr[2] == htonl(theContext->interface >> 32));
        NL_TEST_ASSERT(inSuite,    test_addr.Addr[3] == htonl(theContext->interface));

        theContext++;
    }
}

/**
 *  Test assembling LLA address.
 */
static void CheckMakeLLA(nlTestSuite *inSuite, void *inContext)
{
    const struct TestContext *theContext = static_cast<const struct TestContext *>(inContext);

    for (size_t ith = 0; ith < kTestElements; ith++)
    {
        IPAddress test_addr;

        // Call MakeLLA function that we test.
        test_addr = IPAddress::MakeLLA(theContext->interface);

        NL_TEST_ASSERT(inSuite, test_addr.Addr[0] == htonl(LLA_PREFIX));
        NL_TEST_ASSERT(inSuite, test_addr.Addr[1] == 0);
        NL_TEST_ASSERT(inSuite, test_addr.Addr[2] == htonl(theContext->interface >> 32));
        NL_TEST_ASSERT(inSuite, test_addr.Addr[3] == htonl(theContext->interface));

        theContext++;
    }
}

/**
 *  Test assembling IPv6 Multicast address.
 */
static void CheckMakeIPv6Multicast(nlTestSuite *inSuite, void *inContext)
{
    int s, g;

    IPv6MulticastScope scope[NUM_MCAST_SCOPES] =
    {
        kIPv6MulticastScope_Interface,
        kIPv6MulticastScope_Link,
#if INET_CONFIG_ENABLE_IPV4
        kIPv6MulticastScope_IPv4,
#endif // INET_CONFIG_ENABLE_IPV4
        kIPv6MulticastScope_Admin,
        kIPv6MulticastScope_Site,
        kIPv6MulticastScope_Organization,
        kIPv6MulticastScope_Global
    };

    IPV6MulticastGroup group[NUM_MCAST_GROUPS] =
    {
        kIPV6MulticastGroup_AllNodes,
        kIPV6MulticastGroup_AllRouters
    };

    for (s = 0; s < NUM_MCAST_SCOPES; s++)
    {
        for (g = 0; g < NUM_MCAST_GROUPS; g++)
        {
            IPAddress test_addr;

            // Call MakeIPv6Multicast function that we test.
            test_addr = IPAddress::MakeIPv6Multicast(scope[s], group[g]);

            NL_TEST_ASSERT(inSuite, test_addr.Addr[0] == htonl(MCAST_PREFIX | (scope[s] << 16)));
            NL_TEST_ASSERT(inSuite, test_addr.Addr[1] == 0);
            NL_TEST_ASSERT(inSuite, test_addr.Addr[2] == 0);
            NL_TEST_ASSERT(inSuite, test_addr.Addr[3] == htonl(group[g]));
        }
    }
}

/**
 *  Test IPPrefix.
 */
static void CheckIPPrefix(nlTestSuite *inSuite, void *inContext)
{
    struct TestContext *theFirstContext = static_cast<struct TestContext *>(inContext);

    for (size_t ith = 0; ith < kTestElements; ith++)
    {
        IPPrefix ipprefix_1, ipprefix_2;
        IPAddress test_addr_1;

        SetupIPAddress(test_addr_1, theFirstContext);
        ipprefix_1.IPAddr = test_addr_1;
        ipprefix_1.Length = 128 - ith;
        ipprefix_2 = ipprefix_1;

        NL_TEST_ASSERT(inSuite, !ipprefix_1.IsZero());
        NL_TEST_ASSERT(inSuite, !ipprefix_2.IsZero());
        NL_TEST_ASSERT(inSuite, ipprefix_1 == ipprefix_2);
        NL_TEST_ASSERT(inSuite, !(ipprefix_1 != ipprefix_2));
#if !WEAVE_SYSTEM_CONFIG_USE_LWIP
        NL_TEST_ASSERT(inSuite, ipprefix_1.MatchAddress(test_addr_1));
#endif
        theFirstContext++;
    }
}
// Test Suite


/**
 *   Test Suite. It lists all the test functions.
 */
static const nlTest sTests[] = {
    NL_TEST_DEF("From String Conversion",               CheckFromString),
#if INET_CONFIG_ENABLE_IPV4
    NL_TEST_DEF("IPv4 Detection",                       CheckIsIPv4),
    NL_TEST_DEF("IPv4 Multicast Detection",             CheckIsIPv4Multicast),
    NL_TEST_DEF("IPv4 Broadcast Detection",             CheckIsIPv4Broadcast),
    NL_TEST_DEF("Convert IPv4 to IPAddress",            CheckFromIPv4),
    NL_TEST_DEF("Convert IPAddress to IPv4",            CheckToIPv4),
#endif // INET_CONFIG_ENABLE_IPV4
    NL_TEST_DEF("IPv6 ULA Detection",                   CheckIsIPv6ULA),
    NL_TEST_DEF("IPv6 Link Local Detection",            CheckIsIPv6LLA),
    NL_TEST_DEF("IPv6 Multicast Detection",             CheckIsIPv6Multicast),
    NL_TEST_DEF("Multicast Detection",                  CheckIsMulticast),
    NL_TEST_DEF("Equivalence Operator",                 CheckOperatorEqual),
    NL_TEST_DEF("Non-Equivalence Operator",             CheckOperatorNotEqual),
    NL_TEST_DEF("Assign Operator",                      CheckOperatorAssign),
    NL_TEST_DEF("Convert IPv6 to IPAddress",            CheckFromIPv6),
    NL_TEST_DEF("Convert IPAddress to IPv6",            CheckToIPv6),
    NL_TEST_DEF("Assign address from socket",           CheckFromSocket),
    NL_TEST_DEF("Address Type",                         CheckType),
    NL_TEST_DEF("Address Interface ID",                 CheckInterface),
    NL_TEST_DEF("Address Subnet",                       CheckSubnet),
    NL_TEST_DEF("Address Global ID",                    CheckGlobal),
    NL_TEST_DEF("Assemble IPv6 ULA address",            CheckMakeULA),
    NL_TEST_DEF("Assemble IPv6 LLA address",            CheckMakeLLA),
    NL_TEST_DEF("Assemble IPv6 Multicast address",      CheckMakeIPv6Multicast),
    NL_TEST_DEF("Weave Encoding",                       CheckEncoding),
    NL_TEST_DEF("Weave Decoding",                       CheckDecoding),
    NL_TEST_DEF("Weave Encode / Decode Symmetricity",   CheckEcodeDecodeSymmetricity),
    NL_TEST_DEF("IPPrefix test",                        CheckIPPrefix),
    NL_TEST_SENTINEL()
};

/**
 *  Set up the test suite.
 */
static int TestSetup(void *inContext)
{
    return (SUCCESS);
}

/**
 *  Tear down the test suite.
 */
static int TestTeardown(void *inContext)
{
    return (SUCCESS);
}

int main(void)
{
    nlTestSuite theSuite = {
        "inet-address",
        &sTests[0],
        TestSetup,
        TestTeardown
    };

    // Generate machine-readable, comma-separated value (CSV) output.
    nl_test_set_output_style(OUTPUT_CSV);

    // Run test suit againt one context.
    nlTestRunner(&theSuite, const_cast<struct TestContext *>(&sContext[0]));

    return nlTestRunnerStats(&theSuite);
}
