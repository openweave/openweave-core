/*
 *
 *    Copyright (c) 2019 Nest Labs, Inc.
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
 *          Utility functions for working with OpenThread.
 */


#ifndef OPENTHREAD_UTILS_H
#define OPENTHREAD_UTILS_H

#include <openthread/thread.h>
#include <openthread/error.h>
#include <openthread/ip6.h>

namespace nl {
namespace Weave {
namespace DeviceLayer {
namespace Internal {

extern WEAVE_ERROR MapOpenThreadError(otError otErr);

extern bool IsOpenThreadMeshLocalAddress(otInstance * otInst, const IPAddress & addr);
extern const char * OpenThreadRoleToStr(otDeviceRole role);

inline otIp6Address ToOpenThreadIP6Address(const IPAddress & addr)
{
    otIp6Address otAddr;
    memcpy(otAddr.mFields.m32, addr.Addr, sizeof(otAddr.mFields.m32));
    return otAddr;
}

inline IPAddress ToIPAddress(const otIp6Address & otAddr)
{
    IPAddress addr;
    memcpy(addr.Addr, otAddr.mFields.m32, sizeof(addr.Addr));
    return addr;
}

} // namespace Internal
} // namespace DeviceLayer
} // namespace Weave
} // namespace nl

#endif // OPENTHREAD_UTILS_H
