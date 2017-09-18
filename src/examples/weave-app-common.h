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
 *      This file defines some of the common constants, globals and interfaces
 *      that are common to and used by Weave example applications.
 *
 */


#include <sys/time.h>
#include <SystemLayer/SystemPacketBuffer.h>
#include <Weave/Core/WeaveCore.h>

#define NETWORK_SLEEP_TIME_MSECS  (100 * 1000)

extern uint64_t gDestNodeId;
extern nl::Inet::IPAddress gLocalv6Addr;
extern nl::Inet::IPAddress gDestv6Addr;
extern uint16_t gDestPort;

extern nl::Weave::System::Layer SystemLayer;

extern nl::Inet::InetLayer Inet;

void InitializeWeave(bool listenTCP);

void ShutdownWeave(void);

void DriveIO(void);

inline uint64_t Now(void)
{
    struct timeval now;
    gettimeofday(&now, NULL);
    return ((uint64_t)now.tv_sec * 1000000) + (uint64_t)now.tv_usec;
}
