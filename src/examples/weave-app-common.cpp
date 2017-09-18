/*
 *
 *    Copyright (c) 2017 Nest Labs, Inc.
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
 *      to and used by Weave example applications.
 *
 */

#include <errno.h>

#include "weave-app-common.h"
#include <Weave/Support/ErrorStr.h>

uint64_t gDestNodeId;
nl::Inet::IPAddress gLocalv6Addr;
nl::Inet::IPAddress gDestv6Addr;
uint16_t gDestPort;

using namespace nl::Inet;
using namespace nl::Weave;
using namespace nl::Weave::System;
using namespace nl::Weave::Profiles;

nl::Weave::System::Layer SystemLayer;

nl::Inet::InetLayer Inet;

void InitializeWeave(bool listenTCP)
{
    WEAVE_ERROR err;
    WeaveMessageLayer::InitContext initContext;

    // Initialize the layers of the Weave stack.

    // Print an error message and exit when not running over Linux sockets.
#if !WEAVE_SYSTEM_CONFIG_USE_SOCKETS
    printf("This application, currently, is only supported over the Linux sockets platform\n");
    exit(EXIT_FAILURE);
#endif // !WEAVE_SYSTEM_CONFIG_USE_SOCKETS

    SystemLayer.Init(NULL);

    Inet.Init(SystemLayer, NULL);

    err = FabricState.Init();
    if (err != WEAVE_NO_ERROR)
    {
        printf("FabricState.Init failed: %s\n", nl::ErrorStr(err));
        exit(EXIT_FAILURE);
    }

    // Set the fabric information from the local weave address.

    FabricState.FabricId = gLocalv6Addr.GlobalId();
    FabricState.LocalNodeId = IPv6InterfaceIdToWeaveNodeId(gLocalv6Addr.InterfaceId());
    FabricState.DefaultSubnet = gLocalv6Addr.Subnet();

    // Initialize the WeaveMessageLayer object.

    initContext.systemLayer = &SystemLayer;
    initContext.inet = &Inet;
    initContext.fabricState = &FabricState;
    initContext.listenTCP = listenTCP;
    initContext.listenUDP = false;

    err = MessageLayer.Init(&initContext);
    if (err != WEAVE_NO_ERROR)
    {
        printf("WeaveMessageLayer.Init failed: %s\n", nl::ErrorStr(err));
        exit(EXIT_FAILURE);
    }

    // Initialize the Exchange Manager object.

    err = ExchangeMgr.Init(&MessageLayer);
    if (err != WEAVE_NO_ERROR)
    {
        printf("WeaveExchangeManager.Init failed: %s\n", nl::ErrorStr(err));
        exit(EXIT_FAILURE);
    }
}

void ShutdownWeave(void)
{
    // Shutdown Weave

    ExchangeMgr.Shutdown();

    MessageLayer.Shutdown();

    Inet.Shutdown();

    SystemLayer.Shutdown();
}

void DriveIO(void)
{
#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS
    struct timeval sleepTime;
    fd_set readFDs, writeFDs, exceptFDs;
    int numFDs = 0;
    int selectRes;

    // Use a sleep value of 100 milliseconds
    sleepTime.tv_sec = 0;
    sleepTime.tv_usec = NETWORK_SLEEP_TIME_MSECS;

    FD_ZERO(&readFDs);
    FD_ZERO(&writeFDs);
    FD_ZERO(&exceptFDs);

    if (SystemLayer.State() == System::kLayerState_Initialized)
        SystemLayer.PrepareSelect(numFDs, &readFDs, &writeFDs, &exceptFDs, sleepTime);

    if (Inet.State == InetLayer::kState_Initialized)
        Inet.PrepareSelect(numFDs, &readFDs, &writeFDs, &exceptFDs, sleepTime);

    selectRes = select(numFDs, &readFDs, &writeFDs, &exceptFDs, &sleepTime);
    if (selectRes < 0)
    {
        printf("select failed: %s\n", nl::ErrorStr(System::MapErrorPOSIX(errno)));
        return;
    }

    if (SystemLayer.State() == System::kLayerState_Initialized)
    {
        SystemLayer.HandleSelectResult(selectRes, &readFDs, &writeFDs, &exceptFDs);
    }

    if (Inet.State == InetLayer::kState_Initialized)
    {
        Inet.HandleSelectResult(selectRes, &readFDs, &writeFDs, &exceptFDs);
    }
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS
}

// Dummy Platform implementations for Platform::PersistedStorage functions
namespace nl {
namespace Weave {
namespace Platform {
namespace PersistedStorage {

WEAVE_ERROR Read(const char *aKey, uint32_t &aValue)
{
    return WEAVE_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND;
}

WEAVE_ERROR Write(const char *aKey, uint32_t aValue)
{
    return WEAVE_NO_ERROR;
}

} // PersistentStorage
} // Platform
} // Weave
} // nl
