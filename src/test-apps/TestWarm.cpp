/*
 *
 *    Copyright (c) 2015-2017 Nest Labs, Inc.
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
 *      This file implements a unit test suite for the Weave
 *      Address and Routing Module (WARM).
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <new>

#include <nltest.h>

#include "TestGroupKeyStore.h"
#include <Warm/Warm.h>
#include <Weave/Core/WeaveConfig.h>

using namespace nl::Weave::Warm;

typedef enum {
    kAPITagHostAddress = 0,
    kAPITagHostRoute,
    kAPITagThreadAddress,
    kAPITagThreadAdvertisement,
    kAPITagThreadRoute,
    kAPITagThreadRoutePriority,
    kAPITagCriticalSectionEnter,
    kAPITagCriticalSectionExit,
    kAPITagInitRequestInvokeActions,
    kAPITagInit
} WarmAPITag_t;

static uint32_t sAPICallCounters[] =
{
    /* kAPITagHostAddress */                    0,
    /* kAPITagHostRoute */                      0,
    /* kAPITagThreadAddress */                  0,
    /* kAPITagThreadAdvertisement */            0,
    /* kAPITagThreadRoute */                    0,
    /* kAPITagThreadRoutePriority */            0,
    /* kAPITagCriticalSectionEnter */           0,
    /* kAPITagCriticalSectionExit */            0,
    /* kAPITagInitRequestInvokeActions */       0,
    /* kAPITagInit */                           0
};

static bool sAPIInterfaceStateHostAddress[Warm::kInterfaceTypeMax];
static bool sAPIInterfaceStateHostRoute[Warm::kInterfaceTypeMax];
static bool sAPIInterfaceStateThreadAddress[Warm::kInterfaceTypeMax];
static bool sAPIInterfaceStateThreadAdvertisement[Warm::kInterfaceTypeMax];
static bool sAPIInterfaceStateThreadRoute[Warm::kInterfaceTypeMax];

static nl::Inet::IPAddress sAPIInterfaceAddressHostAddress[Warm::kInterfaceTypeMax];
static nl::Inet::IPAddress zeroIPAddress;

const uint64_t kTestNodeId = 0x18B43000002DCF71ULL;
const uint64_t kTestFabricId = 0x123456789abcdef0ULL;

static WeaveFabricState sFabricState;

// Module Implementation

namespace nl {

namespace Weave {

namespace Warm {

namespace Platform {

/**
 *  Adds | Removes the host stack IP Address from the specified interface.
 *
 *  @brief
 *    This function is called by the WarmCore code to assign or remove an IP Address from
 *    the host TCP/IP stack interface.
 *
 *  @param[in] inInteraceType    Specifies the type of interface to be modified.
 *
 *  @param[in] inAddress         The IP Address to be added to the interface.
 *
 *  @param[in] inPrefixLength    The bit-length of the inAddress'es prefix.
 *
 *  @param[in] inAssign          true == assign the IP Address to the interface. false == remove the IP Address from the interface.
 *
 *  @return kPlatformResultSuccess - If the request was successfully executed.
 *          kPlatformResultFailure - If the request failed.
 *          kPlatformResultInProgress - If the request will complete asynchronously.
 *
 */
PlatformResult AddRemoveHostAddress(InterfaceType inInterfaceType, const Inet::IPAddress &inAddress, uint8_t inPrefixLength, bool inAssign)
{
    sAPICallCounters[kAPITagHostAddress]++;

    sAPIInterfaceStateHostAddress[inInterfaceType] = inAssign;

    sAPIInterfaceAddressHostAddress[inInterfaceType] = inAddress;

    return kPlatformResultSuccess;
}

/**
 *  Adds | Removes the host stack IP Route from the specified interface.
 *
 *  @brief
 *    This function is called by the WarmCore code to assign or remove an IP Route from
 *    the host TCP/IP stack interface.
 *
 *  @param[in] inInteraceType    Specifies the type of interface to be modified.
 *
 *  @param[in] inPrefix          The IP Prefix to be added / removed.
 *
 *  @param[in] inPriority        The priority of the new route.
 *
 *  @param[in] inAssign          true == assign the IP route to the interface. false == remove the IP route from the interface.
 *
 *  @return kPlatformResultSuccess - If the request was successfully executed.
 *          kPlatformResultFailure - If the request failed.
 *          kPlatformResultInProgress - If the request will complete asynchronously.
 *
 */
PlatformResult AddRemoveHostRoute(InterfaceType inInterfaceType, const Inet::IPPrefix &inPrefix, RoutePriority inPriority, bool inAssign)
{
    sAPICallCounters[kAPITagHostRoute]++;

    sAPIInterfaceStateHostRoute[inInterfaceType] = inAssign;

    return kPlatformResultSuccess;
}

/**
 *  Adds | Removes the Thread stack IP Address from the specified interface.
 *
 *  @brief
 *    This function is called by the WarmCore code to assign or remove an IP Address from
 *    the Thread IP stack interface.
 *
 *  @param[in] inInteraceType    Specifies the type of interface to be modified.
 *
 *  @param[in] inAddress         The IP Address to be added / removed.
 *
 *  @param[in] inAssign          true == assign the IP address to the interface. false == remove the IP address from the interface.
 *
 *  @return kPlatformResultSuccess - If the request was successfully executed.
 *          kPlatformResultFailure - If the request failed.
 *          kPlatformResultInProgress - If the request will complete asynchronously.
 *
 */
PlatformResult AddRemoveThreadAddress(InterfaceType inInterfaceType, const Inet::IPAddress &inAddress, bool inAssign)
{
    sAPICallCounters[kAPITagThreadAddress]++;

    sAPIInterfaceStateThreadAddress[inInterfaceType] = inAssign;

    return kPlatformResultSuccess;
}

/**
 *  Configures the Thread stack to start | stop advertising the specified IPAddress
 *
 *  @brief
 *    This function is called by the WarmCore code to command the Thread stack to start or stop
 *    advertising the specified address.
 *
 *  @param[in] inInteraceType    Specifies the type of interface to be modified.
 *
 *  @param[in] inPrefix         The IP Prefix for which advertising should start | stop.
 *
 *  @param[in] inAdvertise       true == start advertising the IP address. false == stop advertising the IP address.
 *
 *  @return kPlatformResultSuccess - If the request was successfully executed.
 *          kPlatformResultFailure - If the request failed.
 *          kPlatformResultInProgress - If the request will complete asynchronously.
 *
 */
PlatformResult StartStopThreadAdvertisement(InterfaceType inInterfaceType, const Inet::IPPrefix &inPrefix, bool inAdvertise)
{
    sAPICallCounters[kAPITagThreadAdvertisement]++;

    sAPIInterfaceStateThreadAdvertisement[inInterfaceType] = inAdvertise;

    return kPlatformResultSuccess;
}

/**
 *  Adds | Removes the Thread stack IP Route from the specified interface.
 *
 *  @brief
 *    This function is called by the WarmCore code to assign or remove an IP Route from
 *    the Thread IP stack interface.
 *
 *  @param[in] inInteraceType    Specifies the type of interface to be modified.
 *
 *  @param[in] inPrefix          The IP Prefix to be added / removed.
 *
 *  @param[in] inPriority        The priority of the new route.
 *
 *  @param[in] inAssign          true == assign the IP route to the interface. false == remove the IP route from the interface.
 *
 *  @return kPlatformResultSuccess - If the request was successfully executed.
 *          kPlatformResultFailure - If the request failed.
 *          kPlatformResultInProgress - If the request will complete asynchronously.
 *
 */
PlatformResult AddRemoveThreadRoute(InterfaceType inInterfaceType, const Inet::IPPrefix &inPrefix, RoutePriority inPriority, bool inAssign)
{
    sAPICallCounters[kAPITagThreadRoute]++;

    sAPIInterfaceStateThreadRoute[inInterfaceType] = inAssign;

    return kPlatformResultSuccess;
}

/**
 *  Changes the Priority of an existing Thread Route.
 *
 *  @brief
 *    This function is called by the WarmCore code to assign or remove an IP Route from
 *    the Thread IP stack interface.
 *
 *  @param[in] inInteraceType    Specifies the type of interface to be modified.
 *
 *  @param[in] inPrefix          The IP Prefix whose priority shall be modified.
 *
 *  @param[in] inPriority        The new priority for the existing route.
 *
 *  @return kPlatformResultSuccess - If the request was successfully executed.
 *          kPlatformResultFailure - If the request failed.
 *          kPlatformResultInProgress - If the request will complete asynchronously.
 *
 */
PlatformResult SetThreadRoutePriority(InterfaceType inInterfaceType, const Inet::IPPrefix &inPrefix, RoutePriority inPriority)
{
    sAPICallCounters[kAPITagThreadRoutePriority]++;

    return kPlatformResultSuccess;
}

/**
 *  Waits to acquire the Critical Section Object. Returns when ownership is granted to the calling thread.
 *
 *  @brief
 *    This function provides mutual exclusion support for resources that require multi-thread protection.
 *    It is complemented by CriticalSectionExit.
 *
 *  @return none.
 *
 */
void CriticalSectionEnter(void)
{
    sAPICallCounters[kAPITagCriticalSectionEnter]++;
}

/**
 *  Releases the Critical Section Object. Returns when ownership has been released.
 *
 *  @brief
 *    This function provides mutual exclusion support for resources that require multi-thread protection.
 *    It is complemented by CriticalSectionEnter.
 *
 *  @return none.
 *
 */
void CriticalSectionExit(void)
{
    sAPICallCounters[kAPITagCriticalSectionExit]++;
}

/**
 *  Called by Warm to notify the platform layer that its state has changed.
 *
 *  @brief
 *    This function allows Warm to notify the platform layer that it should call InvokeActions()
 *    to perform actions as Warms state has changed.  This function will be called by the
 *    task(s) that call the Warm StateChange API's. This function should either call InvokeActions()
 *    or notify the appropriate task to call InvokeActions().
 *
 *  @return none.
 *
 */
void RequestInvokeActions(void)
{
    sAPICallCounters[kAPITagInitRequestInvokeActions]++;

    InvokeActions();
}

/**
 *  Called by Warm Initialize Warm's platform layer.
 *
 *  @return 0 on success, error code otherwise.
 *
 */
WEAVE_ERROR  Init(WarmFabricStateDelegate *inFabricStateDelegate)
{
    sAPICallCounters[kAPITagInit]++;

    return WEAVE_NO_ERROR;
}

}; // namespace Platform

}; // namespace Warm

}; // namespace Weave

}; // namespace nl

static void InitPlatformState(void)
{
    memset(sAPICallCounters,                        0, sizeof(sAPICallCounters));
    memset(sAPIInterfaceStateHostAddress,           0, sizeof(sAPIInterfaceStateHostAddress));
    memset(sAPIInterfaceStateHostRoute,             0, sizeof(sAPIInterfaceStateHostRoute));
    memset(sAPIInterfaceStateHostRoute,             0, sizeof(sAPIInterfaceStateHostRoute));
    memset(sAPIInterfaceStateThreadAddress,         0, sizeof(sAPIInterfaceStateThreadAddress));
    memset(sAPIInterfaceStateThreadAdvertisement,   0, sizeof(sAPIInterfaceStateThreadAdvertisement));
    memset(sAPIInterfaceStateThreadRoute,           0, sizeof(sAPIInterfaceStateThreadRoute));
}

static void Setup(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err;
    static DEFINE_ALIGNED_VAR(sTestGroupKeyStore, sizeof(TestGroupKeyStore), void*);

    err = sFabricState.Init(new (&sTestGroupKeyStore) TestGroupKeyStore());

    sFabricState.LocalNodeId = kTestNodeId;
    sFabricState.FabricId = kTestFabricId;

    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
}

// confirms the proper platform API's are called in response to calling nl::Weave::Warm::Init
static void CheckInit(nlTestSuite *inSuite, void *inContext)
{
    uint32_t callCounterSnapshot[sizeof(sAPICallCounters) / sizeof(sAPICallCounters[0])];
    const WeaveFabricState *fabricState;
    WEAVE_ERROR err;

    InitPlatformState();

    memcpy(callCounterSnapshot, sAPICallCounters, sizeof(sAPICallCounters));

    // Test that the GetFabricState API fails as intended when called prior to calling init.
    err = Warm::GetFabricState(fabricState);
    NL_TEST_ASSERT(inSuite, err == WEAVE_ERROR_INCORRECT_STATE);

    Warm::Init(sFabricState);
    // Test that the expected number of platform API calls are made after calling Init()
    NL_TEST_ASSERT(inSuite, callCounterSnapshot[kAPITagHostAddress]                     == sAPICallCounters[kAPITagHostAddress]);
    NL_TEST_ASSERT(inSuite, callCounterSnapshot[kAPITagHostRoute]                       == sAPICallCounters[kAPITagHostRoute]);
    NL_TEST_ASSERT(inSuite, callCounterSnapshot[kAPITagThreadAddress]                   == sAPICallCounters[kAPITagThreadAddress]);
    NL_TEST_ASSERT(inSuite, callCounterSnapshot[kAPITagThreadAdvertisement]             == sAPICallCounters[kAPITagThreadAdvertisement]);
    NL_TEST_ASSERT(inSuite, callCounterSnapshot[kAPITagThreadRoute]                     == sAPICallCounters[kAPITagThreadRoute]);
    NL_TEST_ASSERT(inSuite, callCounterSnapshot[kAPITagCriticalSectionEnter] + 3        == sAPICallCounters[kAPITagCriticalSectionEnter]);
    NL_TEST_ASSERT(inSuite, callCounterSnapshot[kAPITagCriticalSectionExit] + 3         == sAPICallCounters[kAPITagCriticalSectionExit]);
    NL_TEST_ASSERT(inSuite, sAPICallCounters[kAPITagCriticalSectionEnter]               == sAPICallCounters[kAPITagCriticalSectionExit]);
    NL_TEST_ASSERT(inSuite, callCounterSnapshot[kAPITagInitRequestInvokeActions] + 1    == sAPICallCounters[kAPITagInitRequestInvokeActions]);
    NL_TEST_ASSERT(inSuite, callCounterSnapshot[kAPITagInit] + 1                        == sAPICallCounters[kAPITagInit]);
    // Test the interface state for the API assign

    // Test that the GetFabricState API works as intended.
    err = Warm::GetFabricState(fabricState);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    NL_TEST_ASSERT(inSuite, fabricState == &sFabricState);

    sFabricState.ClearFabricState();

}

#if WARM_CONFIG_SUPPORT_THREAD
static void CheckThread(nlTestSuite *inSuite, void *inContext)
{
    uint32_t callCounterSnapshot[sizeof(sAPICallCounters) / sizeof(sAPICallCounters[0])];
                                                                                       // legacy, thread, wifi, tunnel, cellular
    const bool requiredInterfaceStateHostAddress[Warm::kInterfaceTypeMax]           = { true, true, false, false, false };
    const bool requiredInterfaceStateHostRoute[Warm::kInterfaceTypeMax]             = { false, true, false, false, false };
    const bool requiredInterfaceStateThreadAddress[Warm::kInterfaceTypeMax]         = { true, true, false, false, false };
    const bool requiredInterfaceStateThreadAdvertisement[Warm::kInterfaceTypeMax]   = { false, false, false, false, false };
    const bool requiredInterfaceStateThreadRoute[Warm::kInterfaceTypeMax]           = { false, false, false, false, false };
    const bool requiredInterfaceStateAfterCleanUp[Warm::kInterfaceTypeMax]          = { false, false, false, false, false };
    const uint64_t interfaceId = nl::Weave::WeaveNodeIdToIPv6InterfaceId(sFabricState.LocalNodeId);
    uint64_t globalId;
    nl::Inet::IPAddress address;
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    InitPlatformState();

    memcpy(callCounterSnapshot, sAPICallCounters, sizeof(sAPICallCounters));

    // The API calls for this test

    Warm::ThreadInterfaceStateChange(Warm::kInterfaceStateUp);
    sFabricState.CreateFabric();

    // Test that the expected number of platform API calls are made.

    NL_TEST_ASSERT(inSuite, callCounterSnapshot[kAPITagHostAddress] + 2                     == sAPICallCounters[kAPITagHostAddress]);
    NL_TEST_ASSERT(inSuite, callCounterSnapshot[kAPITagHostRoute] + 1                       == sAPICallCounters[kAPITagHostRoute]);
    NL_TEST_ASSERT(inSuite, callCounterSnapshot[kAPITagThreadAddress] + 2                   == sAPICallCounters[kAPITagThreadAddress]);
    NL_TEST_ASSERT(inSuite, callCounterSnapshot[kAPITagThreadAdvertisement]                 == sAPICallCounters[kAPITagThreadAdvertisement]);
    NL_TEST_ASSERT(inSuite, callCounterSnapshot[kAPITagThreadRoute]                         == sAPICallCounters[kAPITagThreadRoute]);

    NL_TEST_ASSERT(inSuite, sAPICallCounters[kAPITagCriticalSectionEnter]                   == sAPICallCounters[kAPITagCriticalSectionExit]);
    NL_TEST_ASSERT(inSuite, callCounterSnapshot[kAPITagInitRequestInvokeActions] + 2        == sAPICallCounters[kAPITagInitRequestInvokeActions]);

    // Test that the expected platform State exists after making the API calls.

    NL_TEST_ASSERT(inSuite, 0 == memcmp(requiredInterfaceStateHostAddress,         sAPIInterfaceStateHostAddress,         sizeof(sAPIInterfaceStateHostAddress)));
    NL_TEST_ASSERT(inSuite, 0 == memcmp(requiredInterfaceStateHostRoute,           sAPIInterfaceStateHostRoute,           sizeof(sAPIInterfaceStateHostRoute)));
    NL_TEST_ASSERT(inSuite, 0 == memcmp(requiredInterfaceStateThreadAddress,       sAPIInterfaceStateThreadAddress,       sizeof(sAPIInterfaceStateThreadAddress)));
    NL_TEST_ASSERT(inSuite, 0 == memcmp(requiredInterfaceStateThreadAdvertisement, sAPIInterfaceStateThreadAdvertisement, sizeof(sAPIInterfaceStateThreadAdvertisement)));
    NL_TEST_ASSERT(inSuite, 0 == memcmp(requiredInterfaceStateThreadRoute,         sAPIInterfaceStateThreadRoute,         sizeof(sAPIInterfaceStateThreadRoute)));

    // Test that the IP Addresses are set as expected.
    globalId = nl::Weave::WeaveFabricIdToIPv6GlobalId(sFabricState.FabricId);

    address = nl::Inet::IPAddress::MakeULA(globalId, nl::Weave::kWeaveSubnetId_ThreadMesh, interfaceId);
    NL_TEST_ASSERT(inSuite, address == sAPIInterfaceAddressHostAddress[Warm::kInterfaceTypeThread]);

    address = nl::Inet::IPAddress::MakeULA(globalId, nl::Weave::kWeaveSubnetId_ThreadAlarm, interfaceId);
    NL_TEST_ASSERT(inSuite, address == sAPIInterfaceAddressHostAddress[Warm::kInterfaceTypeLegacy6LoWPAN]);

    NL_TEST_ASSERT(inSuite, zeroIPAddress == sAPIInterfaceAddressHostAddress[Warm::kInterfaceTypeWiFi]);
    NL_TEST_ASSERT(inSuite, zeroIPAddress == sAPIInterfaceAddressHostAddress[Warm::kInterfaceTypeTunnel]);
    NL_TEST_ASSERT(inSuite, zeroIPAddress == sAPIInterfaceAddressHostAddress[Warm::kInterfaceTypeCellular]);

    err = Warm::GetULA(Warm::kInterfaceTypeThread, address);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR && address == sAPIInterfaceAddressHostAddress[Warm::kInterfaceTypeThread]);
    err = Warm::GetULA(Warm::kInterfaceTypeLegacy6LoWPAN, address);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR && address == sAPIInterfaceAddressHostAddress[Warm::kInterfaceTypeLegacy6LoWPAN]);

    // Undo the settings for this test

    Warm::ThreadInterfaceStateChange(Warm::kInterfaceStateDown);
    sFabricState.ClearFabricState();

    // Test GetULA during an incorrect state
    err = Warm::GetULA(Warm::kInterfaceTypeThread, address);
    NL_TEST_ASSERT(inSuite, err == WEAVE_ERROR_INCORRECT_STATE);

    // Test that the expected number of platform API calls are made.

    NL_TEST_ASSERT(inSuite, callCounterSnapshot[kAPITagHostAddress] + 4                     == sAPICallCounters[kAPITagHostAddress]);
    NL_TEST_ASSERT(inSuite, callCounterSnapshot[kAPITagHostRoute] + 2                       == sAPICallCounters[kAPITagHostRoute]);
    NL_TEST_ASSERT(inSuite, callCounterSnapshot[kAPITagThreadAddress] + 4                   == sAPICallCounters[kAPITagThreadAddress]);
    NL_TEST_ASSERT(inSuite, callCounterSnapshot[kAPITagThreadAdvertisement]                 == sAPICallCounters[kAPITagThreadAdvertisement]);
    NL_TEST_ASSERT(inSuite, callCounterSnapshot[kAPITagThreadRoute]                         == sAPICallCounters[kAPITagThreadRoute]);

    NL_TEST_ASSERT(inSuite, sAPICallCounters[kAPITagCriticalSectionEnter]                   == sAPICallCounters[kAPITagCriticalSectionExit]);
    NL_TEST_ASSERT(inSuite, callCounterSnapshot[kAPITagInitRequestInvokeActions] + 4        == sAPICallCounters[kAPITagInitRequestInvokeActions]);

    // Test that the expected platform State exists after making the API calls.
    NL_TEST_ASSERT(inSuite, 0 == memcmp(requiredInterfaceStateAfterCleanUp, sAPIInterfaceStateHostAddress,         sizeof(sAPIInterfaceStateHostAddress)));
    NL_TEST_ASSERT(inSuite, 0 == memcmp(requiredInterfaceStateAfterCleanUp, sAPIInterfaceStateHostRoute,           sizeof(sAPIInterfaceStateHostRoute)));
    NL_TEST_ASSERT(inSuite, 0 == memcmp(requiredInterfaceStateAfterCleanUp, sAPIInterfaceStateThreadAddress,       sizeof(sAPIInterfaceStateThreadAddress)));
    NL_TEST_ASSERT(inSuite, 0 == memcmp(requiredInterfaceStateAfterCleanUp, sAPIInterfaceStateThreadAdvertisement, sizeof(sAPIInterfaceStateThreadAdvertisement)));
    NL_TEST_ASSERT(inSuite, 0 == memcmp(requiredInterfaceStateAfterCleanUp, sAPIInterfaceStateThreadRoute,         sizeof(sAPIInterfaceStateThreadRoute)));

    // Test that correct addresses are removed.
    address = nl::Inet::IPAddress::MakeULA(globalId, nl::Weave::kWeaveSubnetId_ThreadMesh, interfaceId);
    NL_TEST_ASSERT(inSuite, address == sAPIInterfaceAddressHostAddress[Warm::kInterfaceTypeThread]);

    address = nl::Inet::IPAddress::MakeULA(globalId, nl::Weave::kWeaveSubnetId_ThreadAlarm, interfaceId);
    NL_TEST_ASSERT(inSuite, address == sAPIInterfaceAddressHostAddress[Warm::kInterfaceTypeLegacy6LoWPAN]);
}

#if WARM_CONFIG_SUPPORT_WIFI
// confirms the proper platform API's are called in response to configuring the system for WiFi + Thread + NO_Routing
static void CheckWiFiThread(nlTestSuite *inSuite, void *inContext)
{
    uint32_t callCounterSnapshot[sizeof(sAPICallCounters) / sizeof(sAPICallCounters[0])];
                                                                                       // legacy, thread, wifi, tunnel, cellular
    const bool requiredInterfaceStateHostAddress[Warm::kInterfaceTypeMax]           = { true, true, true, false, false };
    const bool requiredInterfaceStateHostRoute[Warm::kInterfaceTypeMax]             = { false, true, false, false, false };
    const bool requiredInterfaceStateThreadAddress[Warm::kInterfaceTypeMax]         = { true, true, false, false, false };
    const bool requiredInterfaceStateThreadAdvertisement[Warm::kInterfaceTypeMax]   = { false, false, false, false, false };
    const bool requiredInterfaceStateThreadRoute[Warm::kInterfaceTypeMax]           = { false, false, false, false, false };
    const bool requiredInterfaceStateAfterCleanUp[Warm::kInterfaceTypeMax]          = { false, false, false, false, false };

    const uint64_t interfaceId = nl::Weave::WeaveNodeIdToIPv6InterfaceId(sFabricState.LocalNodeId);
    uint64_t globalId;
    nl::Inet::IPAddress address;
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    InitPlatformState();

    memcpy(callCounterSnapshot, sAPICallCounters, sizeof(sAPICallCounters));

    // The API calls for this test

    Warm::WiFiInterfaceStateChange(Warm::kInterfaceStateUp);
    Warm::ThreadInterfaceStateChange(Warm::kInterfaceStateUp);
    sFabricState.CreateFabric();

    // Test that the expected number of platform API calls are made.

    NL_TEST_ASSERT(inSuite, callCounterSnapshot[kAPITagHostAddress] + 3                     == sAPICallCounters[kAPITagHostAddress]);
    NL_TEST_ASSERT(inSuite, callCounterSnapshot[kAPITagHostRoute] + 1                       == sAPICallCounters[kAPITagHostRoute]);
    NL_TEST_ASSERT(inSuite, callCounterSnapshot[kAPITagThreadAddress] + 2                   == sAPICallCounters[kAPITagThreadAddress]);
    NL_TEST_ASSERT(inSuite, callCounterSnapshot[kAPITagThreadAdvertisement]                 == sAPICallCounters[kAPITagThreadAdvertisement]);
    NL_TEST_ASSERT(inSuite, callCounterSnapshot[kAPITagThreadRoute]                         == sAPICallCounters[kAPITagThreadRoute]);

    NL_TEST_ASSERT(inSuite, sAPICallCounters[kAPITagCriticalSectionEnter]                   == sAPICallCounters[kAPITagCriticalSectionExit]);
    NL_TEST_ASSERT(inSuite, callCounterSnapshot[kAPITagInitRequestInvokeActions] + 3        == sAPICallCounters[kAPITagInitRequestInvokeActions]);

    // Test that the expected platform State exists after making the API calls.

    NL_TEST_ASSERT(inSuite, 0 == memcmp(requiredInterfaceStateHostAddress,         sAPIInterfaceStateHostAddress,         sizeof(sAPIInterfaceStateHostAddress)));
    NL_TEST_ASSERT(inSuite, 0 == memcmp(requiredInterfaceStateHostRoute,           sAPIInterfaceStateHostRoute,           sizeof(sAPIInterfaceStateHostRoute)));
    NL_TEST_ASSERT(inSuite, 0 == memcmp(requiredInterfaceStateThreadAddress,       sAPIInterfaceStateThreadAddress,       sizeof(sAPIInterfaceStateThreadAddress)));
    NL_TEST_ASSERT(inSuite, 0 == memcmp(requiredInterfaceStateThreadAdvertisement, sAPIInterfaceStateThreadAdvertisement, sizeof(sAPIInterfaceStateThreadAdvertisement)));
    NL_TEST_ASSERT(inSuite, 0 == memcmp(requiredInterfaceStateThreadRoute,         sAPIInterfaceStateThreadRoute,         sizeof(sAPIInterfaceStateThreadRoute)));

    // Test that the IP Addresses are set as expected.
    globalId = nl::Weave::WeaveFabricIdToIPv6GlobalId(sFabricState.FabricId);

    address = nl::Inet::IPAddress::MakeULA(globalId, nl::Weave::kWeaveSubnetId_PrimaryWiFi, interfaceId);
    NL_TEST_ASSERT(inSuite, address == sAPIInterfaceAddressHostAddress[Warm::kInterfaceTypeWiFi]);

    address = nl::Inet::IPAddress::MakeULA(globalId, nl::Weave::kWeaveSubnetId_ThreadMesh, interfaceId);
    NL_TEST_ASSERT(inSuite, address == sAPIInterfaceAddressHostAddress[Warm::kInterfaceTypeThread]);

    address = nl::Inet::IPAddress::MakeULA(globalId, nl::Weave::kWeaveSubnetId_ThreadAlarm, interfaceId);
    NL_TEST_ASSERT(inSuite, address == sAPIInterfaceAddressHostAddress[Warm::kInterfaceTypeLegacy6LoWPAN]);

    NL_TEST_ASSERT(inSuite, zeroIPAddress == sAPIInterfaceAddressHostAddress[Warm::kInterfaceTypeTunnel]);
    NL_TEST_ASSERT(inSuite, zeroIPAddress == sAPIInterfaceAddressHostAddress[Warm::kInterfaceTypeCellular]);

    err = Warm::GetULA(Warm::kInterfaceTypeThread, address);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR && address == sAPIInterfaceAddressHostAddress[Warm::kInterfaceTypeThread]);
    err = Warm::GetULA(Warm::kInterfaceTypeLegacy6LoWPAN, address);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR && address == sAPIInterfaceAddressHostAddress[Warm::kInterfaceTypeLegacy6LoWPAN]);
    err = Warm::GetULA(Warm::kInterfaceTypeWiFi, address);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR && address == sAPIInterfaceAddressHostAddress[Warm::kInterfaceTypeWiFi]);

    // Undo the settings for this test.

    Warm::WiFiInterfaceStateChange(Warm::kInterfaceStateDown);
    Warm::ThreadInterfaceStateChange(Warm::kInterfaceStateDown);
    sFabricState.ClearFabricState();

    // Test that the expected number of platform API calls are made.

    NL_TEST_ASSERT(inSuite, callCounterSnapshot[kAPITagHostAddress] + 6                     == sAPICallCounters[kAPITagHostAddress]);
    NL_TEST_ASSERT(inSuite, callCounterSnapshot[kAPITagHostRoute] + 2                       == sAPICallCounters[kAPITagHostRoute]);
    NL_TEST_ASSERT(inSuite, callCounterSnapshot[kAPITagThreadAddress] + 4                   == sAPICallCounters[kAPITagThreadAddress]);
    NL_TEST_ASSERT(inSuite, callCounterSnapshot[kAPITagThreadAdvertisement]                 == sAPICallCounters[kAPITagThreadAdvertisement]);
    NL_TEST_ASSERT(inSuite, callCounterSnapshot[kAPITagThreadRoute]                         == sAPICallCounters[kAPITagThreadRoute]);

    NL_TEST_ASSERT(inSuite, sAPICallCounters[kAPITagCriticalSectionEnter]                   == sAPICallCounters[kAPITagCriticalSectionExit]);
    NL_TEST_ASSERT(inSuite, callCounterSnapshot[kAPITagInitRequestInvokeActions] + 6        == sAPICallCounters[kAPITagInitRequestInvokeActions]);

    // Test that the expected platform State exists after undoing the settings for this test.

    NL_TEST_ASSERT(inSuite, 0 == memcmp(requiredInterfaceStateAfterCleanUp, sAPIInterfaceStateHostAddress,         sizeof(sAPIInterfaceStateHostAddress)));
    NL_TEST_ASSERT(inSuite, 0 == memcmp(requiredInterfaceStateAfterCleanUp, sAPIInterfaceStateHostRoute,           sizeof(sAPIInterfaceStateHostRoute)));
    NL_TEST_ASSERT(inSuite, 0 == memcmp(requiredInterfaceStateAfterCleanUp, sAPIInterfaceStateThreadAddress,       sizeof(sAPIInterfaceStateThreadAddress)));
    NL_TEST_ASSERT(inSuite, 0 == memcmp(requiredInterfaceStateAfterCleanUp, sAPIInterfaceStateThreadAdvertisement, sizeof(sAPIInterfaceStateThreadAdvertisement)));
    NL_TEST_ASSERT(inSuite, 0 == memcmp(requiredInterfaceStateAfterCleanUp, sAPIInterfaceStateThreadRoute,         sizeof(sAPIInterfaceStateThreadRoute)));

    // Test that correct addresses are removed.

    address = nl::Inet::IPAddress::MakeULA(globalId, nl::Weave::kWeaveSubnetId_PrimaryWiFi, interfaceId);
    NL_TEST_ASSERT(inSuite, address == sAPIInterfaceAddressHostAddress[Warm::kInterfaceTypeWiFi]);

    address = nl::Inet::IPAddress::MakeULA(globalId, nl::Weave::kWeaveSubnetId_ThreadMesh, interfaceId);
    NL_TEST_ASSERT(inSuite, address == sAPIInterfaceAddressHostAddress[Warm::kInterfaceTypeThread]);

    address = nl::Inet::IPAddress::MakeULA(globalId, nl::Weave::kWeaveSubnetId_ThreadAlarm, interfaceId);
    NL_TEST_ASSERT(inSuite, address == sAPIInterfaceAddressHostAddress[Warm::kInterfaceTypeLegacy6LoWPAN]);
}

#if WARM_CONFIG_SUPPORT_THREAD_ROUTING
// confirms the proper platform API's are called in response to configuring the system for WiFi + Thread + ThreadRouting
static void CheckWiFiThreadRoute(nlTestSuite *inSuite, void *inContext)
{
    uint32_t callCounterSnapshot[sizeof(sAPICallCounters) / sizeof(sAPICallCounters[0])];
                                                                                       // legacy, thread, wifi, tunnel, cellular
    const bool requiredInterfaceStateHostAddress[Warm::kInterfaceTypeMax]           = { true, true, true, false, false };
    const bool requiredInterfaceStateHostRoute[Warm::kInterfaceTypeMax]             = { false, true, false, false, false };
    const bool requiredInterfaceStateThreadAddress[Warm::kInterfaceTypeMax]         = { true, true, false, false, false };
    const bool requiredInterfaceStateThreadAdvertisement[Warm::kInterfaceTypeMax]   = { false, true, false, false, false };
    const bool requiredInterfaceStateThreadRoute[Warm::kInterfaceTypeMax]           = { false, false, false, false, false };
    const bool requiredInterfaceStateAfterCleanUp[Warm::kInterfaceTypeMax]          = { false, false, false, false, false };
    const uint64_t interfaceId = nl::Weave::WeaveNodeIdToIPv6InterfaceId(sFabricState.LocalNodeId);
    uint64_t globalId;
    nl::Inet::IPAddress address;
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    InitPlatformState();

    memcpy(callCounterSnapshot, sAPICallCounters, sizeof(sAPICallCounters));

    // The API calls for this test

    sFabricState.CreateFabric();
    Warm::WiFiInterfaceStateChange(Warm::kInterfaceStateUp);
    Warm::ThreadInterfaceStateChange(Warm::kInterfaceStateUp);
    Warm::ThreadRoutingStateChange(Warm::kInterfaceStateUp);

    // Test that the expected number of platform API calls are made.

    NL_TEST_ASSERT(inSuite, callCounterSnapshot[kAPITagHostAddress] + 3                     == sAPICallCounters[kAPITagHostAddress]);
    NL_TEST_ASSERT(inSuite, callCounterSnapshot[kAPITagHostRoute] + 1                       == sAPICallCounters[kAPITagHostRoute]);
    NL_TEST_ASSERT(inSuite, callCounterSnapshot[kAPITagThreadAddress] + 2                   == sAPICallCounters[kAPITagThreadAddress]);
    NL_TEST_ASSERT(inSuite, callCounterSnapshot[kAPITagThreadAdvertisement] + 1             == sAPICallCounters[kAPITagThreadAdvertisement]);
    NL_TEST_ASSERT(inSuite, callCounterSnapshot[kAPITagThreadRoute]                         == sAPICallCounters[kAPITagThreadRoute]);

    NL_TEST_ASSERT(inSuite, sAPICallCounters[kAPITagCriticalSectionEnter]                   == sAPICallCounters[kAPITagCriticalSectionExit]);
    NL_TEST_ASSERT(inSuite, callCounterSnapshot[kAPITagInitRequestInvokeActions] + 4        == sAPICallCounters[kAPITagInitRequestInvokeActions]);

    // Test that the expected platform State exists after making the API calls.

    NL_TEST_ASSERT(inSuite, 0 == memcmp(requiredInterfaceStateHostAddress,         sAPIInterfaceStateHostAddress,         sizeof(sAPIInterfaceStateHostAddress)));
    NL_TEST_ASSERT(inSuite, 0 == memcmp(requiredInterfaceStateHostRoute,           sAPIInterfaceStateHostRoute,           sizeof(sAPIInterfaceStateHostRoute)));
    NL_TEST_ASSERT(inSuite, 0 == memcmp(requiredInterfaceStateThreadAddress,       sAPIInterfaceStateThreadAddress,       sizeof(sAPIInterfaceStateThreadAddress)));
    NL_TEST_ASSERT(inSuite, 0 == memcmp(requiredInterfaceStateThreadAdvertisement, sAPIInterfaceStateThreadAdvertisement, sizeof(sAPIInterfaceStateThreadAdvertisement)));
    NL_TEST_ASSERT(inSuite, 0 == memcmp(requiredInterfaceStateThreadRoute,         sAPIInterfaceStateThreadRoute,         sizeof(sAPIInterfaceStateThreadRoute)));

    // Test that the IP Addresses are set as expected.
    globalId = nl::Weave::WeaveFabricIdToIPv6GlobalId(sFabricState.FabricId);

    address = nl::Inet::IPAddress::MakeULA(globalId, nl::Weave::kWeaveSubnetId_PrimaryWiFi, interfaceId);
    NL_TEST_ASSERT(inSuite, address == sAPIInterfaceAddressHostAddress[Warm::kInterfaceTypeWiFi]);

    address = nl::Inet::IPAddress::MakeULA(globalId, nl::Weave::kWeaveSubnetId_ThreadMesh, interfaceId);
    NL_TEST_ASSERT(inSuite, address == sAPIInterfaceAddressHostAddress[Warm::kInterfaceTypeThread]);

    address = nl::Inet::IPAddress::MakeULA(globalId, nl::Weave::kWeaveSubnetId_ThreadAlarm, interfaceId);
    NL_TEST_ASSERT(inSuite, address == sAPIInterfaceAddressHostAddress[Warm::kInterfaceTypeLegacy6LoWPAN]);

    NL_TEST_ASSERT(inSuite, zeroIPAddress == sAPIInterfaceAddressHostAddress[Warm::kInterfaceTypeTunnel]);
    NL_TEST_ASSERT(inSuite, zeroIPAddress == sAPIInterfaceAddressHostAddress[Warm::kInterfaceTypeCellular]);

    err = Warm::GetULA(Warm::kInterfaceTypeThread, address);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR && address == sAPIInterfaceAddressHostAddress[Warm::kInterfaceTypeThread]);
    err = Warm::GetULA(Warm::kInterfaceTypeLegacy6LoWPAN, address);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR && address == sAPIInterfaceAddressHostAddress[Warm::kInterfaceTypeLegacy6LoWPAN]);
    err = Warm::GetULA(Warm::kInterfaceTypeWiFi, address);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR && address == sAPIInterfaceAddressHostAddress[Warm::kInterfaceTypeWiFi]);

    // Undo the settings for this test.

    sFabricState.ClearFabricState();
    Warm::WiFiInterfaceStateChange(Warm::kInterfaceStateDown);
    Warm::ThreadInterfaceStateChange(Warm::kInterfaceStateDown);
    Warm::ThreadRoutingStateChange(Warm::kInterfaceStateDown);

    // Test that the expected number of platform API calls are made.

    NL_TEST_ASSERT(inSuite, callCounterSnapshot[kAPITagHostAddress] + 6                     == sAPICallCounters[kAPITagHostAddress]);
    NL_TEST_ASSERT(inSuite, callCounterSnapshot[kAPITagHostRoute] + 2                       == sAPICallCounters[kAPITagHostRoute]);
    NL_TEST_ASSERT(inSuite, callCounterSnapshot[kAPITagThreadAddress] + 4                   == sAPICallCounters[kAPITagThreadAddress]);
    NL_TEST_ASSERT(inSuite, callCounterSnapshot[kAPITagThreadAdvertisement] + 2             == sAPICallCounters[kAPITagThreadAdvertisement]);
    NL_TEST_ASSERT(inSuite, callCounterSnapshot[kAPITagThreadRoute]                         == sAPICallCounters[kAPITagThreadRoute]);

    NL_TEST_ASSERT(inSuite, sAPICallCounters[kAPITagCriticalSectionEnter]                   == sAPICallCounters[kAPITagCriticalSectionExit]);
    NL_TEST_ASSERT(inSuite, callCounterSnapshot[kAPITagInitRequestInvokeActions] + 8        == sAPICallCounters[kAPITagInitRequestInvokeActions]);

    // Test that the expected platform State exists after undoing the settings for this test.

    NL_TEST_ASSERT(inSuite, 0 == memcmp(requiredInterfaceStateAfterCleanUp, sAPIInterfaceStateHostAddress,         sizeof(sAPIInterfaceStateHostAddress)));
    NL_TEST_ASSERT(inSuite, 0 == memcmp(requiredInterfaceStateAfterCleanUp, sAPIInterfaceStateHostRoute,           sizeof(sAPIInterfaceStateHostRoute)));
    NL_TEST_ASSERT(inSuite, 0 == memcmp(requiredInterfaceStateAfterCleanUp, sAPIInterfaceStateThreadAddress,       sizeof(sAPIInterfaceStateThreadAddress)));
    NL_TEST_ASSERT(inSuite, 0 == memcmp(requiredInterfaceStateAfterCleanUp, sAPIInterfaceStateThreadAdvertisement, sizeof(sAPIInterfaceStateThreadAdvertisement)));
    NL_TEST_ASSERT(inSuite, 0 == memcmp(requiredInterfaceStateAfterCleanUp, sAPIInterfaceStateThreadRoute,         sizeof(sAPIInterfaceStateThreadRoute)));

    // Test that correct addresses are removed.

    address = nl::Inet::IPAddress::MakeULA(globalId, nl::Weave::kWeaveSubnetId_PrimaryWiFi, interfaceId);
    NL_TEST_ASSERT(inSuite, address == sAPIInterfaceAddressHostAddress[Warm::kInterfaceTypeWiFi]);

    address = nl::Inet::IPAddress::MakeULA(globalId, nl::Weave::kWeaveSubnetId_ThreadMesh, interfaceId);
    NL_TEST_ASSERT(inSuite, address == sAPIInterfaceAddressHostAddress[Warm::kInterfaceTypeThread]);

    address = nl::Inet::IPAddress::MakeULA(globalId, nl::Weave::kWeaveSubnetId_ThreadAlarm, interfaceId);
    NL_TEST_ASSERT(inSuite, address == sAPIInterfaceAddressHostAddress[Warm::kInterfaceTypeLegacy6LoWPAN]);
}

#if WARM_CONFIG_SUPPORT_WEAVE_TUNNEL && WARM_CONFIG_SUPPORT_BORDER_ROUTING
// confirms the proper platform API's are called in response to configuring the system for WiFi + Thread + ThreadRouting + BorderRouting + Tunnel
static void CheckWiFiThreadRouteBorderTunnel(nlTestSuite *inSuite, void *inContext)
{
    uint32_t callCounterSnapshot[sizeof(sAPICallCounters) / sizeof(sAPICallCounters[0])];
                                                                              // legacy, thread, wifi, tunnel, cellular
    bool requiredInterfaceStateHostAddress[Warm::kInterfaceTypeMax]           = { true,  true,  true,  true,  false };
    bool requiredInterfaceStateHostRoute[Warm::kInterfaceTypeMax]             = { false, true,  false, true,  false };
    bool requiredInterfaceStateThreadAddress[Warm::kInterfaceTypeMax]         = { true,  true,  false, false, false };
    bool requiredInterfaceStateThreadAdvertisement[Warm::kInterfaceTypeMax]   = { false, true,  false, false, false };
    bool requiredInterfaceStateThreadRoute[Warm::kInterfaceTypeMax]           = { false, true,  false, false, false };
    const bool requiredInterfaceStateAfterCleanUp[Warm::kInterfaceTypeMax]    = { false, false, false, false, false };
    const uint64_t interfaceId = nl::Weave::WeaveNodeIdToIPv6InterfaceId(sFabricState.LocalNodeId);
    uint64_t globalId;
    nl::Inet::IPAddress address;

    InitPlatformState();

    memcpy(callCounterSnapshot, sAPICallCounters, sizeof(sAPICallCounters));

    // The API calls for this test

    sFabricState.CreateFabric();

    Warm::WiFiInterfaceStateChange(Warm::kInterfaceStateUp);
    Warm::ThreadInterfaceStateChange(Warm::kInterfaceStateUp);
    Warm::ThreadRoutingStateChange(Warm::kInterfaceStateUp);
    // Profiles::WeaveTunnel::Platform::TunnelInterfaceUp calls Warm::TunnelInterfaceStateChange(Warm::kInterfaceStateUp)
    Profiles::WeaveTunnel::Platform::TunnelInterfaceUp((InterfaceId)0);
    // Profiles::WeaveTunnel::Platform::ServiceTunnelEstablished calls Warm::TunnelServiceStateChange(Warm::kInterfaceStateUp, Profiles::WeaveTunnel::Platform::kMode_Primary)
    Profiles::WeaveTunnel::Platform::ServiceTunnelEstablished((InterfaceId)0, Profiles::WeaveTunnel::Platform::kMode_Primary);
    // Profiles::WeaveTunnel::Platform::ServiceTunnelModeChange calls Warm::ServiceTunnelModeChange(InterfaceId tunIf, TunnelAvailabilityMode tunMode)
    Profiles::WeaveTunnel::Platform::ServiceTunnelModeChange((InterfaceId)0, Profiles::WeaveTunnel::Platform::kMode_PrimaryAndBackup);
    Warm::BorderRouterStateChange(Warm::kInterfaceStateUp);

    // Test that the expected number of platform API calls are made.

    NL_TEST_ASSERT(inSuite, callCounterSnapshot[kAPITagHostAddress] + 4                     == sAPICallCounters[kAPITagHostAddress]);
    NL_TEST_ASSERT(inSuite, callCounterSnapshot[kAPITagHostRoute] + 2                       == sAPICallCounters[kAPITagHostRoute]);
    NL_TEST_ASSERT(inSuite, callCounterSnapshot[kAPITagThreadAddress] + 2                   == sAPICallCounters[kAPITagThreadAddress]);
    NL_TEST_ASSERT(inSuite, callCounterSnapshot[kAPITagThreadAdvertisement] + 1             == sAPICallCounters[kAPITagThreadAdvertisement]);
    NL_TEST_ASSERT(inSuite, callCounterSnapshot[kAPITagThreadRoute] + 1                     == sAPICallCounters[kAPITagThreadRoute]);

    NL_TEST_ASSERT(inSuite, sAPICallCounters[kAPITagCriticalSectionEnter]                   == sAPICallCounters[kAPITagCriticalSectionExit]);
    NL_TEST_ASSERT(inSuite, callCounterSnapshot[kAPITagInitRequestInvokeActions] + 8        == sAPICallCounters[kAPITagInitRequestInvokeActions]);
    // Test that the expected platform State exists after making the API calls.

    NL_TEST_ASSERT(inSuite, 0 == memcmp(requiredInterfaceStateHostAddress,         sAPIInterfaceStateHostAddress,         sizeof(sAPIInterfaceStateHostAddress)));
    NL_TEST_ASSERT(inSuite, 0 == memcmp(requiredInterfaceStateHostRoute,           sAPIInterfaceStateHostRoute,           sizeof(sAPIInterfaceStateHostRoute)));
    NL_TEST_ASSERT(inSuite, 0 == memcmp(requiredInterfaceStateThreadAddress,       sAPIInterfaceStateThreadAddress,       sizeof(sAPIInterfaceStateThreadAddress)));
    NL_TEST_ASSERT(inSuite, 0 == memcmp(requiredInterfaceStateThreadAdvertisement, sAPIInterfaceStateThreadAdvertisement, sizeof(sAPIInterfaceStateThreadAdvertisement)));
    NL_TEST_ASSERT(inSuite, 0 == memcmp(requiredInterfaceStateThreadRoute,         sAPIInterfaceStateThreadRoute,         sizeof(sAPIInterfaceStateThreadRoute)));

    // Test that the IP Addresses are set as expected.
    globalId = nl::Weave::WeaveFabricIdToIPv6GlobalId(sFabricState.FabricId);

    address = nl::Inet::IPAddress::MakeULA(globalId, nl::Weave::kWeaveSubnetId_PrimaryWiFi, interfaceId);
    NL_TEST_ASSERT(inSuite, address == sAPIInterfaceAddressHostAddress[Warm::kInterfaceTypeWiFi]);

    address = nl::Inet::IPAddress::MakeULA(globalId, nl::Weave::kWeaveSubnetId_ThreadMesh, interfaceId);
    NL_TEST_ASSERT(inSuite, address == sAPIInterfaceAddressHostAddress[Warm::kInterfaceTypeThread]);

    address = nl::Inet::IPAddress::MakeULA(globalId, nl::Weave::kWeaveSubnetId_ThreadAlarm, interfaceId);
    NL_TEST_ASSERT(inSuite, address == sAPIInterfaceAddressHostAddress[Warm::kInterfaceTypeLegacy6LoWPAN]);

    address = nl::Inet::IPAddress::MakeULA(globalId, nl::Weave::kWeaveSubnetId_ThreadMesh, interfaceId);
    NL_TEST_ASSERT(inSuite, address == sAPIInterfaceAddressHostAddress[Warm::kInterfaceTypeTunnel]);

    NL_TEST_ASSERT(inSuite, zeroIPAddress == sAPIInterfaceAddressHostAddress[Warm::kInterfaceTypeCellular]);

    // Now try to disable features and re-test.

    Warm::BorderRouterStateChange(Warm::kInterfaceStateDown);

    {
        requiredInterfaceStateThreadRoute[Warm::kInterfaceTypeThread] = false;

        NL_TEST_ASSERT(inSuite, 0 == memcmp(requiredInterfaceStateHostAddress,         sAPIInterfaceStateHostAddress,         sizeof(sAPIInterfaceStateHostAddress)));
        NL_TEST_ASSERT(inSuite, 0 == memcmp(requiredInterfaceStateHostRoute,           sAPIInterfaceStateHostRoute,           sizeof(sAPIInterfaceStateHostRoute)));
        NL_TEST_ASSERT(inSuite, 0 == memcmp(requiredInterfaceStateThreadAddress,       sAPIInterfaceStateThreadAddress,       sizeof(sAPIInterfaceStateThreadAddress)));
        NL_TEST_ASSERT(inSuite, 0 == memcmp(requiredInterfaceStateThreadAdvertisement, sAPIInterfaceStateThreadAdvertisement, sizeof(sAPIInterfaceStateThreadAdvertisement)));
        NL_TEST_ASSERT(inSuite, 0 == memcmp(requiredInterfaceStateThreadRoute,         sAPIInterfaceStateThreadRoute,         sizeof(sAPIInterfaceStateThreadRoute)));
    }

    //Profiles::WeaveTunnel::Platform::TunnelInterfaceDown Calls--> Warm::TunnelInterfaceStateChange(Warm::kInterfaceStateDown);
    Profiles::WeaveTunnel::Platform::TunnelInterfaceDown((InterfaceId)0);

    {
        requiredInterfaceStateHostAddress[Warm::kInterfaceTypeTunnel] = false;
        requiredInterfaceStateHostRoute[Warm::kInterfaceTypeTunnel] = false;

        NL_TEST_ASSERT(inSuite, 0 == memcmp(requiredInterfaceStateHostAddress,         sAPIInterfaceStateHostAddress,         sizeof(sAPIInterfaceStateHostAddress)));
        NL_TEST_ASSERT(inSuite, 0 == memcmp(requiredInterfaceStateHostRoute,           sAPIInterfaceStateHostRoute,           sizeof(sAPIInterfaceStateHostRoute)));
        NL_TEST_ASSERT(inSuite, 0 == memcmp(requiredInterfaceStateThreadAddress,       sAPIInterfaceStateThreadAddress,       sizeof(sAPIInterfaceStateThreadAddress)));
        NL_TEST_ASSERT(inSuite, 0 == memcmp(requiredInterfaceStateThreadAdvertisement, sAPIInterfaceStateThreadAdvertisement, sizeof(sAPIInterfaceStateThreadAdvertisement)));
        NL_TEST_ASSERT(inSuite, 0 == memcmp(requiredInterfaceStateThreadRoute,         sAPIInterfaceStateThreadRoute,         sizeof(sAPIInterfaceStateThreadRoute)));
    }

    Warm::WiFiInterfaceStateChange(Warm::kInterfaceStateDown);

    {
        requiredInterfaceStateHostAddress[Warm::kInterfaceTypeWiFi] = false;

        NL_TEST_ASSERT(inSuite, 0 == memcmp(requiredInterfaceStateHostAddress,         sAPIInterfaceStateHostAddress,         sizeof(sAPIInterfaceStateHostAddress)));
        NL_TEST_ASSERT(inSuite, 0 == memcmp(requiredInterfaceStateHostRoute,           sAPIInterfaceStateHostRoute,           sizeof(sAPIInterfaceStateHostRoute)));
        NL_TEST_ASSERT(inSuite, 0 == memcmp(requiredInterfaceStateThreadAddress,       sAPIInterfaceStateThreadAddress,       sizeof(sAPIInterfaceStateThreadAddress)));
        NL_TEST_ASSERT(inSuite, 0 == memcmp(requiredInterfaceStateThreadAdvertisement, sAPIInterfaceStateThreadAdvertisement, sizeof(sAPIInterfaceStateThreadAdvertisement)));
        NL_TEST_ASSERT(inSuite, 0 == memcmp(requiredInterfaceStateThreadRoute,         sAPIInterfaceStateThreadRoute,         sizeof(sAPIInterfaceStateThreadRoute)));
    }

    // Undo the settings for this test.

    sFabricState.ClearFabricState();
    Warm::WiFiInterfaceStateChange(Warm::kInterfaceStateDown);
    Warm::ThreadInterfaceStateChange(Warm::kInterfaceStateDown);
    Warm::ThreadRoutingStateChange(Warm::kInterfaceStateDown);
    //Profiles::WeaveTunnel::Platform::TunnelInterfaceDown Calls--> Warm::TunnelInterfaceStateChange(Warm::kInterfaceStateDown);
    Profiles::WeaveTunnel::Platform::TunnelInterfaceDown((InterfaceId)0);
    // Profiles::WeaveTunnel::Platform::ServiceTunnelDisconnected calls Warm::TunnelServiceStateChange(Warm::kInterfaceStateDown, Profiles::WeaveTunnel::Platform::kState_Normal)
    Profiles::WeaveTunnel::Platform::ServiceTunnelDisconnected((InterfaceId)0);
    Warm::BorderRouterStateChange(Warm::kInterfaceStateDown);

    // Test that the expected number of platform API calls are made.

    NL_TEST_ASSERT(inSuite, callCounterSnapshot[kAPITagHostAddress] + 8                     == sAPICallCounters[kAPITagHostAddress]);
    NL_TEST_ASSERT(inSuite, callCounterSnapshot[kAPITagHostRoute] + 4                       == sAPICallCounters[kAPITagHostRoute]);
    NL_TEST_ASSERT(inSuite, callCounterSnapshot[kAPITagThreadAddress] + 4                   == sAPICallCounters[kAPITagThreadAddress]);
    NL_TEST_ASSERT(inSuite, callCounterSnapshot[kAPITagThreadAdvertisement] + 2             == sAPICallCounters[kAPITagThreadAdvertisement]);
    NL_TEST_ASSERT(inSuite, callCounterSnapshot[kAPITagThreadRoute] + 2                     == sAPICallCounters[kAPITagThreadRoute]);

    NL_TEST_ASSERT(inSuite, sAPICallCounters[kAPITagCriticalSectionEnter]                   == sAPICallCounters[kAPITagCriticalSectionExit]);
    NL_TEST_ASSERT(inSuite, callCounterSnapshot[kAPITagInitRequestInvokeActions] + 15       == sAPICallCounters[kAPITagInitRequestInvokeActions]);
    // Test that the expected platform State exists after undoing the settings.

    NL_TEST_ASSERT(inSuite, 0 == memcmp(requiredInterfaceStateAfterCleanUp, sAPIInterfaceStateHostAddress,         sizeof(sAPIInterfaceStateHostAddress)));
    NL_TEST_ASSERT(inSuite, 0 == memcmp(requiredInterfaceStateAfterCleanUp, sAPIInterfaceStateHostRoute,           sizeof(sAPIInterfaceStateHostRoute)));
    NL_TEST_ASSERT(inSuite, 0 == memcmp(requiredInterfaceStateAfterCleanUp, sAPIInterfaceStateThreadAddress,       sizeof(sAPIInterfaceStateThreadAddress)));
    NL_TEST_ASSERT(inSuite, 0 == memcmp(requiredInterfaceStateAfterCleanUp, sAPIInterfaceStateThreadAdvertisement, sizeof(sAPIInterfaceStateThreadAdvertisement)));
    NL_TEST_ASSERT(inSuite, 0 == memcmp(requiredInterfaceStateAfterCleanUp, sAPIInterfaceStateThreadRoute,         sizeof(sAPIInterfaceStateThreadRoute)));

    // Test that correct addresses are removed.

    address = nl::Inet::IPAddress::MakeULA(globalId, nl::Weave::kWeaveSubnetId_PrimaryWiFi, interfaceId);
    NL_TEST_ASSERT(inSuite, address == sAPIInterfaceAddressHostAddress[Warm::kInterfaceTypeWiFi]);

    address = nl::Inet::IPAddress::MakeULA(globalId, nl::Weave::kWeaveSubnetId_ThreadMesh, interfaceId);
    NL_TEST_ASSERT(inSuite, address == sAPIInterfaceAddressHostAddress[Warm::kInterfaceTypeThread]);

    address = nl::Inet::IPAddress::MakeULA(globalId, nl::Weave::kWeaveSubnetId_ThreadAlarm, interfaceId);
    NL_TEST_ASSERT(inSuite, address == sAPIInterfaceAddressHostAddress[Warm::kInterfaceTypeLegacy6LoWPAN]);

    address = nl::Inet::IPAddress::MakeULA(globalId, nl::Weave::kWeaveSubnetId_ThreadMesh, interfaceId);
    NL_TEST_ASSERT(inSuite, address == sAPIInterfaceAddressHostAddress[Warm::kInterfaceTypeTunnel]);
}
#endif // WARM_CONFIG_SUPPORT_WEAVE_TUNNEL && WARM_CONFIG_SUPPORT_BORDER_ROUTING
#endif // WARM_CONFIG_SUPPORT_THREAD_ROUTING
#endif // WARM_CONFIG_SUPPORT_WIFI
#endif // WARM_CONFIG_SUPPORT_THREAD

static const nlTest sTests[] = {
    NL_TEST_DEF("Setup",                     Setup),
    NL_TEST_DEF("init",                      CheckInit),
#if WARM_CONFIG_SUPPORT_THREAD
    NL_TEST_DEF("Thread",                    CheckThread),
#if WARM_CONFIG_SUPPORT_WIFI
    NL_TEST_DEF("WiFi+Thread",               CheckWiFiThread),
#if WARM_CONFIG_SUPPORT_THREAD_ROUTING
    NL_TEST_DEF("WiFi+Thread+Route",         CheckWiFiThreadRoute),
#if WARM_CONFIG_SUPPORT_WEAVE_TUNNEL && WARM_CONFIG_SUPPORT_BORDER_ROUTING
    NL_TEST_DEF("WiFi+Thread+Route+Tunnel",  CheckWiFiThreadRouteBorderTunnel),
#endif // WARM_CONFIG_SUPPORT_WEAVE_TUNNEL && WARM_CONFIG_SUPPORT_BORDER_ROUTING
#endif // WARM_CONFIG_SUPPORT_THREAD_ROUTING
#endif // WARM_CONFIG_SUPPORT_WIFI
#endif // WARM_CONFIG_SUPPORT_THREAD
    NL_TEST_SENTINEL()
};

int main(void)
{
    WEAVE_ERROR err;

    err = nl::Weave::Platform::Security::InitSecureRandomDataSource(NULL, 64, NULL, 0);
    FAIL_ERROR(err, "InitSecureRandomDataSource() failed");

    nlTestSuite theSuite = {
        "warm",
        &sTests[0]
    };

    nl_test_set_output_style(OUTPUT_CSV);

    nlTestRunner(&theSuite, NULL);

    return nlTestRunnerStats(&theSuite);
}
