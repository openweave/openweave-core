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


#include <Weave/DeviceLayer/internal/WeaveDeviceLayerInternal.h>
#include <Weave/DeviceLayer/OpenThread/OpenThreadUtils.h>

namespace nl {
namespace Weave {
namespace DeviceLayer {
namespace Internal {

WEAVE_ERROR MapOpenThreadError(otError otErr)
{
    // TODO: implement me
    return WEAVE_ERROR_NOT_IMPLEMENTED;
}

bool IsOpenThreadMeshLocalAddress(otInstance * otInst, const IPAddress & addr)
{
    const otMeshLocalPrefix * otMeshPrefix = otThreadGetMeshLocalPrefix(otInst);

    return otMeshPrefix != NULL && memcmp(otMeshPrefix->m8, addr.Addr, OT_MESH_LOCAL_PREFIX_SIZE) == 0;
}

const char * OpenThreadRoleToStr(otDeviceRole role)
{
    switch (role)
    {
    case OT_DEVICE_ROLE_DISABLED:
        return "DISABLED";
    case OT_DEVICE_ROLE_DETACHED:
        return "DETACHED";
    case OT_DEVICE_ROLE_CHILD:
        return "CHILD";
    case OT_DEVICE_ROLE_ROUTER:
        return "ROUTER";
    case OT_DEVICE_ROLE_LEADER:
        return "LEADER";
    default:
        return "(unknown)";
    }
}


} // namespace Internal
} // namespace DeviceLayer
} // namespace Weave
} // namespace nl
