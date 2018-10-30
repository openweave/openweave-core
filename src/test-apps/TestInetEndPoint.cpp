/*
 *
 *    Copyright (c) 2016-2017 Nest Labs, Inc.
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
 *     This file implements a unit test suite for InetLayer EndPoint related features
 *
 */

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#include <stdint.h>
#include <string.h>

#include <InetLayer/InetLayer.h>

#include <SystemLayer/SystemError.h>
#include <SystemLayer/SystemTimer.h>

#include <nlunit-test.h>

#include "ToolCommon.h"

using namespace nl::Inet;
using namespace nl::Weave::System;

bool callbackHandlerCalled = false;

void HandleDNSResolveComplete(void *appState, INET_ERROR err, uint8_t addrCount, IPAddress *addrArray)
{
    callbackHandlerCalled = true;

    if (addrCount > 0)
    {
        char destAddrStr[64];
        addrArray->ToString(destAddrStr, sizeof(destAddrStr));
        printf("    DNS name resolution complete: %s\n", destAddrStr);
    }
    else
        printf("    DNS name resolution return no addresses\n");
}

void HandleTimer(Layer* aLayer, void* aAppState, Error aError)
{
    printf("    timer handler\n");
}

// Test before init network, Inet is not initialized
static void TestInetPre(nlTestSuite *inSuite, void *inContext)
{
#if INET_CONFIG_ENABLE_RAW_ENDPOINT
    RawEndPoint *testRawEP = NULL;
#endif // INET_CONFIG_ENABLE_RAW_ENDPOINT
#if INET_CONFIG_ENABLE_UDP_ENDPOINT
    UDPEndPoint *testUDPEP = NULL;
#endif // INET_CONFIG_ENABLE_UDP_ENDPOINT
#if INET_CONFIG_ENABLE_TCP_ENDPOINT
    TCPEndPoint *testTCPEP = NULL;
#endif // INET_CONFIG_ENABLE_TCP_ENDPOINT
#if INET_CONFIG_ENABLE_TUN_ENDPOINT
    TunEndPoint *testTunEP = NULL;
#endif // INET_CONFIG_ENABLE_TUN_ENDPOINT
    INET_ERROR err = INET_NO_ERROR;
    IPAddress testDestAddr = IPAddress::Any;
    char testHostName[20] = "www.nest.com";

#if INET_CONFIG_ENABLE_RAW_ENDPOINT
    err = Inet.NewRawEndPoint(kIPVersion_6, kIPProtocol_ICMPv6, &testRawEP);
    NL_TEST_ASSERT(inSuite, err == INET_ERROR_INCORRECT_STATE);
#endif // INET_CONFIG_ENABLE_RAW_ENDPOINT

#if INET_CONFIG_ENABLE_UDP_ENDPOINT
    err = Inet.NewUDPEndPoint(&testUDPEP);
    NL_TEST_ASSERT(inSuite, err == INET_ERROR_INCORRECT_STATE);
#endif // INET_CONFIG_ENABLE_UDP_ENDPOINT

#if INET_CONFIG_ENABLE_TUN_ENDPOINT
    err = Inet.NewTunEndPoint(&testTunEP);
    NL_TEST_ASSERT(inSuite, err == INET_ERROR_INCORRECT_STATE);
#endif // INET_CONFIG_ENABLE_TUN_ENDPOINT

#if INET_CONFIG_ENABLE_TCP_ENDPOINT
    err = Inet.NewTCPEndPoint(&testTCPEP);
    NL_TEST_ASSERT(inSuite, err == INET_ERROR_INCORRECT_STATE);
#endif // INET_CONFIG_ENABLE_TCP_ENDPOINT

    err = SystemLayer.StartTimer(10, HandleTimer, NULL);
    NL_TEST_ASSERT(inSuite, err == WEAVE_SYSTEM_ERROR_UNEXPECTED_STATE);

#if INET_CONFIG_ENABLE_DNS_RESOLVER
    err = Inet.ResolveHostAddress(testHostName, 1, &testDestAddr, HandleDNSResolveComplete, NULL);
    NL_TEST_ASSERT(inSuite, err == INET_ERROR_INCORRECT_STATE);
#endif // INET_CONFIG_ENABLE_DNS_RESOLVER

    // then init network
    InitSystemLayer();
    InitNetwork();
}

#if INET_CONFIG_ENABLE_DNS_RESOLVER
// Test Inet ResolveHostAddress functionality
static void TestResolveHostAddress(nlTestSuite *inSuite, void *inContext)
{
    char testHostName1[20] = "www.nest.com";
    char testHostName2[20] = "127.0.0.1";
    char testHostName3[20] = "";
    char testHostName4[260];
    struct timeval sleepTime;
    IPAddress testDestAddr[1] = { IPAddress::Any };
    INET_ERROR err;

    sleepTime.tv_sec = 0;
    sleepTime.tv_usec = 10000;

    memset(testHostName4, 'w', sizeof(testHostName4));
    testHostName4[259] = '\0';

    callbackHandlerCalled = false;
    err = Inet.ResolveHostAddress(testHostName1, 1, testDestAddr, HandleDNSResolveComplete, &callbackHandlerCalled);
    NL_TEST_ASSERT(inSuite, err == INET_NO_ERROR);

    while (!callbackHandlerCalled)
    {
        ServiceNetwork(sleepTime);
    }

    callbackHandlerCalled = false;
    err = Inet.ResolveHostAddress(testHostName2, 1, testDestAddr, HandleDNSResolveComplete, &callbackHandlerCalled);
    NL_TEST_ASSERT(inSuite, err == INET_NO_ERROR);

    while (!callbackHandlerCalled)
    {
        ServiceNetwork(sleepTime);
    }

    callbackHandlerCalled = false;
    err = Inet.ResolveHostAddress(testHostName3, 1, testDestAddr, HandleDNSResolveComplete, &callbackHandlerCalled);
    NL_TEST_ASSERT(inSuite, err == INET_NO_ERROR);

    while (!callbackHandlerCalled)
    {
        ServiceNetwork(sleepTime);
    }

    err = Inet.ResolveHostAddress(testHostName2, 0, testDestAddr, HandleDNSResolveComplete, &callbackHandlerCalled);
    NL_TEST_ASSERT(inSuite, err == INET_ERROR_NO_MEMORY);

    err = Inet.ResolveHostAddress(testHostName4, 1, testDestAddr, HandleDNSResolveComplete, &callbackHandlerCalled);
    NL_TEST_ASSERT(inSuite, err == INET_ERROR_HOST_NAME_TOO_LONG);

}
#endif // INET_CONFIG_ENABLE_DNS_RESOLVER

// Test Inet ParseHostPortAndInterface
static void TestParseHost(nlTestSuite *inSuite, void *inContext)
{
    char correctHostName[7][30] = { "10.0.0.1", "10.0.0.1:3000", "www.nest.com", "www.nest.com:3000",
                                    "[fd00:0:1:1::1]:3000", "[fd00:0:1:1::1]:300%wpan0", "%wpan0" };
    char invalidHostName[4][30] = { "[fd00::1]5", "[fd00:0:1:1::1:3000", "10.0.0.1:1234567", "10.0.0.1:er31" };
    const char *host;
    const char *intf;
    uint16_t port, hostlen, intflen;
    INET_ERROR err;

    for (int i = 0; i < 7; i++)
    {
        err = ParseHostPortAndInterface(correctHostName[i], strlen(correctHostName[i]), host, hostlen, port, intf, intflen);
        NL_TEST_ASSERT(inSuite, err == INET_NO_ERROR);
    }
    for (int i = 0; i < 4; i++)
    {
        err = ParseHostPortAndInterface(invalidHostName[i], strlen(invalidHostName[i]), host, hostlen, port, intf, intflen);
        NL_TEST_ASSERT(inSuite, err == INET_ERROR_INVALID_HOST_NAME);
    }
}

static void TestInetError(nlTestSuite *inSuite, void *inContext)
{
    INET_ERROR err = INET_NO_ERROR;

    err = MapErrorPOSIX(1);
    NL_TEST_ASSERT(inSuite, DescribeErrorPOSIX(err));
    NL_TEST_ASSERT(inSuite, IsErrorPOSIX(err));
}

static void TestInetInterface(nlTestSuite *inSuite, void *inContext)
{
    InterfaceIterator intIterator;
    InterfaceAddressIterator addrIterator;
    char intName[20];
    InterfaceId intId;
    IPAddress addr;
    INET_ERROR err;

    err = InterfaceNameToId("0", intId);
    NL_TEST_ASSERT(inSuite, err != INET_NO_ERROR);

    err = GetInterfaceName((InterfaceId)1, intName, 0);
    NL_TEST_ASSERT(inSuite, err == INET_ERROR_NO_MEMORY);

    err = GetInterfaceName(INET_NULL_INTERFACEID, intName, 0);
    NL_TEST_ASSERT(inSuite, err == INET_ERROR_NO_MEMORY);

    err = Inet.GetInterfaceFromAddr(addr, intId);
    NL_TEST_ASSERT(inSuite, intId == INET_NULL_INTERFACEID);

    err = Inet.GetLinkLocalAddr(intId, NULL);
    NL_TEST_ASSERT(inSuite, err == INET_ERROR_BAD_ARGS);

    printf("    Interfaces:\n");
    for (; intIterator.HasCurrent(); intIterator.Next()) {
        intId = intIterator.GetInterface();
        memset(intName, 0, sizeof(intName));
        GetInterfaceName(intId, intName, sizeof(intName));
        printf("     interface id: %d, interface name: %s, %s mulicast\n", intId,
              intName, intIterator.SupportsMulticast() ? "support":"don't support");

        Inet.GetLinkLocalAddr(intId, &addr);
        Inet.MatchLocalIPv6Subnet(addr);
    }
    NL_TEST_ASSERT(inSuite, !intIterator.SupportsMulticast());

    printf("    Addresses:\n");
    for (; addrIterator.HasCurrent(); addrIterator.Next()) {
        addr = addrIterator.GetAddress();
        char buf[80];
        addr.ToString(buf, 80);
        printf("     %s, %s mulicast, prefix length: %d\n", buf,
               addrIterator.SupportsMulticast() ? "support":"don't support",
               addrIterator.GetIPv6PrefixLength());
    }
    NL_TEST_ASSERT(inSuite, !addrIterator.SupportsMulticast());
}

static void TestInetEndPoint(nlTestSuite *inSuite, void *inContext)
{
    INET_ERROR err;
    IPAddress addr_any = IPAddress::Any;
    IPAddress addr;
#if INET_CONFIG_ENABLE_IPV4
    IPAddress addr_v4;
#endif // INET_CONFIG_ENABLE_IPV4
    InterfaceId intId;

    // EndPoint
    RawEndPoint *testRaw6EP = NULL;
#if INET_CONFIG_ENABLE_IPV4
    RawEndPoint *testRaw4EP = NULL;
#endif // INET_CONFIG_ENABLE_IPV4
    UDPEndPoint *testUDPEP = NULL;
#if INET_CONFIG_ENABLE_TUN_ENDPOINT
    TunEndPoint *testTunEP = NULL;
#endif
    TCPEndPoint *testTCPEP1 = NULL;
    PacketBuffer *buf = PacketBuffer::New();

    // init all the EndPoints
    err = Inet.NewRawEndPoint(kIPVersion_6, kIPProtocol_ICMPv6, &testRaw6EP);
    NL_TEST_ASSERT(inSuite, err == INET_NO_ERROR);

#if INET_CONFIG_ENABLE_IPV4
    err = Inet.NewRawEndPoint(kIPVersion_4, kIPProtocol_ICMPv4, &testRaw4EP);
    NL_TEST_ASSERT(inSuite, err == INET_NO_ERROR);
#endif // INET_CONFIG_ENABLE_IPV4

    err = Inet.NewUDPEndPoint(&testUDPEP);
    NL_TEST_ASSERT(inSuite, err == INET_NO_ERROR);

#if INET_CONFIG_ENABLE_TUN_ENDPOINT
    err = Inet.NewTunEndPoint(&testTunEP);
    NL_TEST_ASSERT(inSuite, err == INET_NO_ERROR);
#endif

    err = Inet.NewTCPEndPoint(&testTCPEP1);
    NL_TEST_ASSERT(inSuite, err == INET_NO_ERROR);

    err = Inet.GetLinkLocalAddr(INET_NULL_INTERFACEID, &addr);
    NL_TEST_ASSERT(inSuite, err == INET_NO_ERROR);
    err = Inet.GetInterfaceFromAddr(addr, intId);
    NL_TEST_ASSERT(inSuite, err == INET_NO_ERROR);


    // RawEndPoint special cases to cover the error branch
    uint8_t ICMP6Types[2] = { 128, 129 };
#if INET_CONFIG_ENABLE_IPV4
    NL_TEST_ASSERT(inSuite, IPAddress::FromString("10.0.0.1", addr_v4));
#endif // INET_CONFIG_ENABLE_IPV4

    // error bind cases
    err = testRaw6EP->Bind(kIPAddressType_Unknown, addr_any);
    NL_TEST_ASSERT(inSuite, err == INET_ERROR_WRONG_ADDRESS_TYPE);
#if INET_CONFIG_ENABLE_IPV4
    err = testRaw6EP->Bind(kIPAddressType_IPv4, addr);
    NL_TEST_ASSERT(inSuite, err == INET_ERROR_WRONG_ADDRESS_TYPE);
    err = testRaw6EP->BindIPv6LinkLocal(intId, addr_v4);
    NL_TEST_ASSERT(inSuite, err == INET_ERROR_WRONG_ADDRESS_TYPE);
#endif // INET_CONFIG_ENABLE_IPV4
    err = testRaw6EP->BindIPv6LinkLocal((InterfaceId)-1, addr);
    NL_TEST_ASSERT(inSuite, err != INET_NO_ERROR);
    err = testRaw6EP->BindInterface((InterfaceId)-1);
    NL_TEST_ASSERT(inSuite, err != INET_NO_ERROR);
    err = testRaw6EP->BindInterface(INET_NULL_INTERFACEID);
    NL_TEST_ASSERT(inSuite, err != INET_NO_ERROR);

    err = testRaw6EP->BindIPv6LinkLocal(intId, addr);
    testRaw6EP->Listen();
    testRaw6EP->Listen();
    err = testRaw6EP->Bind(kIPAddressType_IPv6, addr);
    NL_TEST_ASSERT(inSuite, err == INET_ERROR_INCORRECT_STATE);

    // error SetICMPFilter case
    err = testRaw6EP->SetICMPFilter(0, ICMP6Types);
    NL_TEST_ASSERT(inSuite, err == INET_ERROR_BAD_ARGS);

#if INET_CONFIG_ENABLE_IPV4
    // error Sendto case
    err = testRaw4EP->SendTo(addr, buf);
    NL_TEST_ASSERT(inSuite, err == INET_ERROR_WRONG_ADDRESS_TYPE);
    testRaw4EP->Free();
#endif // INET_CONFIG_ENABLE_IPV4

    // UdpEndPoint special cases to cover the error branch
    err = testUDPEP->Listen();
    NL_TEST_ASSERT(inSuite, err == INET_ERROR_INCORRECT_STATE);
    err = testUDPEP->Bind(kIPAddressType_Unknown, addr_any, 3000);
    NL_TEST_ASSERT(inSuite, err == INET_ERROR_WRONG_ADDRESS_TYPE);
    err = testUDPEP->Bind(kIPAddressType_Unknown, addr, 3000);
    NL_TEST_ASSERT(inSuite, err == INET_ERROR_WRONG_ADDRESS_TYPE);
#if INET_CONFIG_ENABLE_IPV4
    err = testUDPEP->Bind(kIPAddressType_IPv4, addr, 3000);
    NL_TEST_ASSERT(inSuite, err == INET_ERROR_WRONG_ADDRESS_TYPE);
#endif // INET_CONFIG_ENABLE_IPV4

    err = testUDPEP->Bind(kIPAddressType_IPv6, addr, 3000, intId);
    err = testUDPEP->BindInterface(kIPAddressType_IPv6, intId);
    InterfaceId id = testUDPEP->GetBoundInterface();
    NL_TEST_ASSERT(inSuite, id == intId);

    err = testUDPEP->Listen();
    err = testUDPEP->Listen();
    err = testUDPEP->Bind(kIPAddressType_IPv6, addr, 3000, intId);
    NL_TEST_ASSERT(inSuite, err == INET_ERROR_INCORRECT_STATE);
    err = testUDPEP->BindInterface(kIPAddressType_IPv6, intId);
    NL_TEST_ASSERT(inSuite, err == INET_ERROR_INCORRECT_STATE);
    testUDPEP->Free();

    err = Inet.NewUDPEndPoint(&testUDPEP);
    NL_TEST_ASSERT(inSuite, err == INET_NO_ERROR);
#if INET_CONFIG_ENABLE_IPV4
    err = testUDPEP->Bind(kIPAddressType_IPv4, addr_v4, 3000, intId);
    NL_TEST_ASSERT(inSuite, err != INET_NO_ERROR);
    buf = PacketBuffer::New();
    err = testUDPEP->SendTo(addr_v4, 3000, buf);
    testUDPEP->Free();
#endif // INET_CONFIG_ENABLE_IPV4

    // TcpEndPoint special cases to cover the error branch
    err = testTCPEP1->GetPeerInfo(NULL, NULL);
    NL_TEST_ASSERT(inSuite, err == INET_ERROR_INCORRECT_STATE);
    buf = PacketBuffer::New();
    err = testTCPEP1->Send(buf, false);
    NL_TEST_ASSERT(inSuite, err == INET_ERROR_INCORRECT_STATE);
    err = testTCPEP1->EnableKeepAlive(10, 100);
    NL_TEST_ASSERT(inSuite, err == INET_ERROR_INCORRECT_STATE);
    err = testTCPEP1->DisableKeepAlive();
    NL_TEST_ASSERT(inSuite, err == INET_ERROR_INCORRECT_STATE);
    err = testTCPEP1->AckReceive(10);
    NL_TEST_ASSERT(inSuite, err == INET_ERROR_INCORRECT_STATE);
    NL_TEST_ASSERT(inSuite, !testTCPEP1->PendingReceiveLength());
    err = testTCPEP1->Listen(4);
    NL_TEST_ASSERT(inSuite, err == INET_ERROR_INCORRECT_STATE);
    err = testTCPEP1->GetLocalInfo(NULL, NULL);
    NL_TEST_ASSERT(inSuite, err == INET_ERROR_INCORRECT_STATE);

    err = testTCPEP1->Bind(kIPAddressType_Unknown, addr_any, 3000, true);
    NL_TEST_ASSERT(inSuite, err == INET_ERROR_WRONG_ADDRESS_TYPE);
#if INET_CONFIG_ENABLE_IPV4
    err = testTCPEP1->Bind(kIPAddressType_IPv4, addr, 3000, true);
    NL_TEST_ASSERT(inSuite, err == INET_ERROR_WRONG_ADDRESS_TYPE);
#endif // INET_CONFIG_ENABLE_IPV4
    err = testTCPEP1->Bind(kIPAddressType_Unknown, addr, 3000, true);
    NL_TEST_ASSERT(inSuite, err == INET_ERROR_WRONG_ADDRESS_TYPE);

    err = testTCPEP1->Bind(kIPAddressType_IPv6, addr_any, 3000, true);
    err = testTCPEP1->Bind(kIPAddressType_IPv6, addr_any, 3000, true);
    NL_TEST_ASSERT(inSuite, err == INET_ERROR_INCORRECT_STATE);
    err = testTCPEP1->Listen(4);
#if INET_CONFIG_ENABLE_IPV4
    err = testTCPEP1->Connect(addr_v4, 4000, intId);
    NL_TEST_ASSERT(inSuite, err == INET_ERROR_INCORRECT_STATE);
#endif // INET_CONFIG_ENABLE_IPV4

#if INET_CONFIG_ENABLE_TUN_ENDPOINT
    // TunEndPoint special cases to cover the error branch
    testTunEP->Init(&Inet);
    InterfaceId tunId = testTunEP->GetTunnelInterfaceId();
    NL_TEST_ASSERT(inSuite, tunId == INET_NULL_INTERFACEID);
    NL_TEST_ASSERT(inSuite, !testTunEP->IsInterfaceUp());
    err = testTunEP->InterfaceUp();
    NL_TEST_ASSERT(inSuite, err != INET_NO_ERROR);
    err = testTunEP->InterfaceDown();
    NL_TEST_ASSERT(inSuite, err != INET_NO_ERROR);
    err = testTunEP->Send(buf);
    NL_TEST_ASSERT(inSuite, err != INET_NO_ERROR);
    testTunEP->Free();
#endif

    testTCPEP1->Shutdown();
}

// Test the InetLayer resource limitation
static void TestInetEndPointLimit(nlTestSuite *inSuite, void *inContext)
{
    RawEndPoint *testRawEP = NULL;
    UDPEndPoint *testUDPEP = NULL;
#if INET_CONFIG_ENABLE_TUN_ENDPOINT
    TunEndPoint *testTunEP = NULL;
#endif
    TCPEndPoint *testTCPEP = NULL;
    INET_ERROR err;
    char numTimersTest[WEAVE_SYSTEM_CONFIG_NUM_TIMERS + 1];

    for (int i = 0; i < INET_CONFIG_NUM_RAW_ENDPOINTS + 1; i++)
        err = Inet.NewRawEndPoint(kIPVersion_6, kIPProtocol_ICMPv6, &testRawEP);
    NL_TEST_ASSERT(inSuite, err == INET_ERROR_NO_ENDPOINTS);

    for (int i = 0; i < INET_CONFIG_NUM_UDP_ENDPOINTS + 1; i++)
        err = Inet.NewUDPEndPoint(&testUDPEP);
    NL_TEST_ASSERT(inSuite, err == INET_ERROR_NO_ENDPOINTS);

#if INET_CONFIG_ENABLE_TUN_ENDPOINT
    for (int i = 0; i < INET_CONFIG_NUM_TUN_ENDPOINTS + 1; i++)
        err = Inet.NewTunEndPoint(&testTunEP);
    NL_TEST_ASSERT(inSuite, err == INET_ERROR_NO_ENDPOINTS);
#endif

    for (int i = 0; i < INET_CONFIG_NUM_TCP_ENDPOINTS + 1; i++)
        err = Inet.NewTCPEndPoint(&testTCPEP);
    NL_TEST_ASSERT(inSuite, err == INET_ERROR_NO_ENDPOINTS);

    // Verify same aComplete and aAppState args do not exhaust timer pool
    for (int i = 0; i < WEAVE_SYSTEM_CONFIG_NUM_TIMERS + 1; i++)
    {
        err = SystemLayer.StartTimer(10, HandleTimer, NULL);
        NL_TEST_ASSERT(inSuite, err == WEAVE_SYSTEM_NO_ERROR);
    }

    for (int i = 0; i < WEAVE_SYSTEM_CONFIG_NUM_TIMERS + 1; i++)
        err = SystemLayer.StartTimer(10, HandleTimer, &numTimersTest[i]);
    NL_TEST_ASSERT(inSuite, err == WEAVE_SYSTEM_ERROR_NO_MEMORY);

    ShutdownNetwork();
    ShutdownSystemLayer();
}

// Test Suite


/**
 *   Test Suite. It lists all the test functions.
 */
static const nlTest sTests[] = {
    NL_TEST_DEF("InetEndPoint::PreTest",             TestInetPre),
#if INET_CONFIG_ENABLE_DNS_RESOLVER
    NL_TEST_DEF("InetEndPoint::ResolveHostAddress",  TestResolveHostAddress),
#endif // INET_CONFIG_ENABLE_DNS_RESOLVER
    NL_TEST_DEF("InetEndPoint::TestParseHost",       TestParseHost),
    NL_TEST_DEF("InetEndPoint::TestInetError",       TestInetError),
    NL_TEST_DEF("InetEndPoint::TestInetInterface",   TestInetInterface),
    NL_TEST_DEF("InetEndPoint::TestInetEndPoint",    TestInetEndPoint),
    NL_TEST_DEF("InetEndPoint::TestEndPointLimit",   TestInetEndPointLimit),
    NL_TEST_SENTINEL()
};

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS
/**
 *  Set up the test suite.
 *  This is a work-around to initiate PacketBuffer protected class instance's
 *  data and set it to a known state, before an instance is created.
 */
static int TestSetup(void *inContext)
{
    return (SUCCESS);
}

/**
 *  Tear down the test suite.
 *  Free memory reserved at TestSetup.
 */
static int TestTeardown(void *inContext)
{
    return (SUCCESS);
}
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS

int main(int argc, char *argv[])
{
#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS
    nlTestSuite theSuite = {
        "inet-endpoint",
        &sTests[0],
        TestSetup,
        TestTeardown
    };

    // Generate machine-readable, comma-separated value (CSV) output.
    nl_test_set_output_style(OUTPUT_CSV);

    // Run test suit againt one context.
    nlTestRunner(&theSuite, NULL);

    return nlTestRunnerStats(&theSuite);
#else // !WEAVE_SYSTEM_CONFIG_USE_SOCKETS
    return 0;
#endif // !WEAVE_SYSTEM_CONFIG_USE_SOCKETS
}
