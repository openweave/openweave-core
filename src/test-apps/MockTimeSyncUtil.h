/*
 *
 *    Copyright (c) 2014-2017 Nest Labs, Inc.
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

#ifndef MOCKTIMESYNCUTIL_H_
#define MOCKTIMESYNCUTIL_H_

#include <Weave/Profiles/time/WeaveTime.h>

// the roles a mock device could choose to play
enum MockTimeSyncRole
{
    kMockTimeSyncRole_None = 0,
    kMockTimeSyncRole_Server,
    kMockTimeSyncRole_Client,
    kMockTimeSyncRole_Coordinator,
};

// the time sync modes
enum OperatingMode
{
    kOperatingMode_Auto = 0,
    kOperatingMode_AssignedLocalNodes,

#if WEAVE_CONFIG_TIME_CLIENT_CONNECTION_FOR_SERVICE
    kOperatingMode_Service,
    kOperatingMode_ServiceOverTunnel
#endif // WEAVE_CONFIG_TIME_CLIENT_CONNECTION_FOR_SERVICE
};

// add struct and static member function to mimic namespace
// so the usage could be similar to other components in mock device
struct MockTimeSync
{

// Set the role this mock device shall be playing
// this function is called at the cmd line argument parsing stage of mock-device
static WEAVE_ERROR SetRole(const MockTimeSyncRole role);

// Set the Time sync mode
// this function is called at the cmd line argument parsing stage of mock-device
static WEAVE_ERROR SetMode(const OperatingMode mode);

// Initialize this mock device for Time Services, according to the role that was set earlier
static WEAVE_ERROR Init(nl::Weave::WeaveExchangeManager * const exchangeMgr, uint64_t serviceNodeId, const char * serviceNodeAddr);

// Shutdown this mock device for Time Services, according to the role that was set earlier
static WEAVE_ERROR Shutdown();

};

#endif /* MOCKTIMESYNCUTIL_H_ */
