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
 *      This file tests DNS resolution using the InetLayer APIs.
 *
 */

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "ToolCommon.h"
#include <nltest.h>
#include <SystemLayer/SystemTimer.h>

using namespace nl::Inet;

#define TOOL_NAME "TestAsyncDNS"
#define DEFAULT_TEST_DURATION_MILLISECS               (10000)
#define DEFAULT_CANCEL_TEST_DURATION_MILLISECS        (2000)

static void HandleSIGUSR1(int sig);

const char *destHostName = NULL;
uint64_t gTestStartTime = 0;
int32_t gNumOfResolutionDone = 0;
int32_t gMaxNumResolve = 4; // Max number of DNS Resolutions being performed.
uint64_t gMaxTestDurationMillisecs = DEFAULT_TEST_DURATION_MILLISECS;
bool gTestSucceeded = false;
IPAddress DestAddr = IPAddress::Any;
IPAddress DestAddrPool[2] = { IPAddress::Any, IPAddress::Any };

#if INET_CONFIG_ENABLE_DNS_RESOLVER
static void HandleDNSResolveComplete(void *appState, INET_ERROR err, uint8_t addrCount, IPAddress *addrArray);
static void HandleDNSCancel(void *appState, INET_ERROR err, uint8_t addrCount, IPAddress *addrArray);
static void HandleDNSInvalid(void *appState, INET_ERROR err, uint8_t addrCount, IPAddress *addrArray);
#endif // INET_CONFIG_ENABLE_DNS_RESOLVER

static void TestDNSResolution(nlTestSuite *inSuite, void *inContext)
{
    INET_ERROR err = INET_NO_ERROR;
    Done = false;
    char testHostName1[20] = "www.nest.com";
    char testHostName2[20] = "10.0.0.1";
    char testHostName3[20] = "www.google.com";
    char testHostName4[20] = "pool.ntp.org";
    gTestSucceeded = false;
    gMaxTestDurationMillisecs = DEFAULT_TEST_DURATION_MILLISECS;
    gTestStartTime = Now();

#if INET_CONFIG_ENABLE_DNS_RESOLVER
    printf("Resolving hostname %s\n", testHostName1);
    err = Inet.ResolveHostAddress(testHostName1, 1, &DestAddr, HandleDNSResolveComplete, testHostName1);
    SuccessOrExit(err);

    printf("Resolving hostname %s\n", testHostName2);
    err = Inet.ResolveHostAddress(testHostName2, 1, &DestAddr, HandleDNSResolveComplete, testHostName2);
    SuccessOrExit(err);

    printf("Resolving hostname %s\n", testHostName3);
    err = Inet.ResolveHostAddress(testHostName3, 1, &DestAddr, HandleDNSResolveComplete, testHostName3);
    SuccessOrExit(err);

    printf("Resolving hostname %s\n", testHostName4);
    err = Inet.ResolveHostAddress(testHostName4, 2, DestAddrPool, HandleDNSResolveComplete, testHostName4);
    SuccessOrExit(err);
#endif // INET_CONFIG_ENABLE_DNS_RESOLVER

    while (!Done)
    {
        struct timeval sleepTime;
        sleepTime.tv_sec = 0;
        sleepTime.tv_usec = 10000;

        ServiceNetwork(sleepTime);

        if (Now() < gTestStartTime + gMaxTestDurationMillisecs * System::kTimerFactor_micro_per_milli)
        {
            if (gTestSucceeded)
            {
                Done = true;
            }
            else
            {
                continue;
            }
        }
        else // Time's up
        {
            gTestSucceeded = false;
            Done = true;
        }
    }


exit:

    NL_TEST_ASSERT(inSuite, err == INET_NO_ERROR);
    NL_TEST_ASSERT(inSuite, gTestSucceeded == true);
}

static void TestDNSCancel(nlTestSuite *inSuite, void *inContext)
{
    INET_ERROR err = INET_NO_ERROR;
    Done = false;
    char testHostName1[20] = "www.nest.com";
    gTestSucceeded = false;
    gMaxTestDurationMillisecs = DEFAULT_CANCEL_TEST_DURATION_MILLISECS;
    gTestStartTime = Now();

#if INET_CONFIG_ENABLE_DNS_RESOLVER
    printf("Resolving hostname %s\n", testHostName1);
    err = Inet.ResolveHostAddress(testHostName1, 1, &DestAddr, HandleDNSCancel, NULL);
    SuccessOrExit(err);

    // Cancel the DNS request.
    Inet.CancelResolveHostAddress(HandleDNSCancel, NULL);

    gTestSucceeded = true;

    while (Now() < gTestStartTime + gMaxTestDurationMillisecs * System::kTimerFactor_micro_per_milli)
    {
        struct timeval sleepTime;
        sleepTime.tv_sec = 0;
        sleepTime.tv_usec = 10000;

        ServiceNetwork(sleepTime);
    }

#endif // INET_CONFIG_ENABLE_DNS_RESOLVER

exit:

    NL_TEST_ASSERT(inSuite, err == INET_NO_ERROR);
    NL_TEST_ASSERT(inSuite, gTestSucceeded == true);
}

static void TestDNSInvalid(nlTestSuite *inSuite, void *inContext)
{
    INET_ERROR err = INET_NO_ERROR;
    Done = false;
    char testInvalidHostName[20] = "www.google.invalid.";
    gTestSucceeded = false;
    gMaxTestDurationMillisecs = DEFAULT_CANCEL_TEST_DURATION_MILLISECS;
    gTestStartTime = Now();

#if INET_CONFIG_ENABLE_DNS_RESOLVER
    printf("Resolving hostname %s\n", testInvalidHostName);
    err = Inet.ResolveHostAddress(testInvalidHostName, 1, &DestAddr, HandleDNSInvalid, NULL);
    SuccessOrExit(err);

    while (!Done)
    {
        struct timeval sleepTime;
        sleepTime.tv_sec = 0;
        sleepTime.tv_usec = 10000;

        ServiceNetwork(sleepTime);

        if (Now() < gTestStartTime + gMaxTestDurationMillisecs * System::kTimerFactor_micro_per_milli)
        {
            if (gTestSucceeded)
            {
                Done = true;
            }
            else
            {
                continue;
            }
        }
        else // Time's up
        {
            gTestSucceeded = false;
            Done = true;
        }
    }

#endif // INET_CONFIG_ENABLE_DNS_RESOLVER

exit:

    NL_TEST_ASSERT(inSuite, err == INET_NO_ERROR);
    NL_TEST_ASSERT(inSuite, gTestSucceeded == true);
}

static const nlTest DNSTests[] = {
    NL_TEST_DEF("TestDNSResolution", TestDNSResolution),
    NL_TEST_DEF("TestDNSCancel", TestDNSCancel),
    NL_TEST_DEF("TestDNSInvalid", TestDNSInvalid),
    NL_TEST_SENTINEL()
};

int main(int argc, char *argv[])
{
#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS
    SetSignalHandler(HandleSIGUSR1);

    nlTestSuite DNSTestSuite = {
        "DNS",
        &DNSTests[0]
    };

    nl_test_set_output_style(OUTPUT_CSV);

    InitSystemLayer();
    InitNetwork();

    // Run all tests in Suite

    nlTestRunner(&DNSTestSuite, NULL);

    ShutdownNetwork();
    ShutdownSystemLayer();

    return nlTestRunnerStats(&DNSTestSuite);
#else // !WEAVE_SYSTEM_CONFIG_USE_SOCKETS
    return 0;
#endif // !WEAVE_SYSTEM_CONFIG_USE_SOCKETS
}

void HandleSIGUSR1(int sig)
{
    Inet.Shutdown();
    exit(0);
}

#if INET_CONFIG_ENABLE_DNS_RESOLVER
static void HandleDNSCancel(void *appState, INET_ERROR err, uint8_t addrCount, IPAddress *addrArray)
{
    printf("DNS Cancel failed: Callback should not have been called\n");
    gTestSucceeded = false;
}

static void HandleDNSInvalid(void *appState, INET_ERROR err, uint8_t addrCount, IPAddress *addrArray)
{
    if (err != INET_NO_ERROR)
      gTestSucceeded = true;
}

void HandleDNSResolveComplete(void *appState, INET_ERROR err, uint8_t addrCount, IPAddress *addrArray)
{

    FAIL_ERROR(err, "DNS name resolution failed");
    char *hostName = static_cast<char *>(appState);

    gNumOfResolutionDone++;

    if (addrCount > 0)
    {
        for (int i = 0; i < addrCount; i++)
        {
            char destAddrStr[64];
            addrArray[i].ToString(destAddrStr, sizeof(destAddrStr));
            printf("DNS name resolution complete for %s: %s\n", hostName, destAddrStr);
        }
    }
    else
        printf("DNS name resolution return no addresses\n");

    if (gNumOfResolutionDone == gMaxNumResolve)
    {
       gTestSucceeded = true;
    }
}
#endif // INET_CONFIG_ENABLE_DNS_RESOLVER
