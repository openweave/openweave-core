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
 *      This file implements memory management functions for the Weave Security Manager.
 *      The implementation is based on the C Standard Library malloc() and free() functions.
 *      This implementation is used when #WEAVE_CONFIG_SECURITY_MGR_MEMORY_MGMT_MALLOC
 *      is enabled (1).
 *
 *    @note This implementation ignores some of the function's input parameters,
 *      which were intended to help with better memory utilization. The assumption
 *      is that platforms that choose this implementation are not memory constrained.
 *
 */

#include "WeaveConfig.h"
#include "WeaveSecurityMgr.h"

#if WEAVE_CONFIG_SECURITY_MGR_MEMORY_MGMT_MALLOC

namespace nl {
namespace Weave {
namespace Platform {
namespace Security {

WEAVE_ERROR MemoryInit(void *buf, size_t bufSize)
{
    return WEAVE_NO_ERROR;
}

void MemoryShutdown()
{
}

void *MemoryAlloc(size_t size)
{
    return MemoryAlloc(size, false);
}

void *MemoryAlloc(size_t size, bool isLongTermAlloc)
{
    return malloc(size);
}

void MemoryFree(void *p)
{
    free(p);
}

} // namespace Security
} // namespace Platform
} // namespace Weave
} // namespace nl

#endif // WEAVE_CONFIG_SECURITY_MGR_MEMORY_MGMT_MALLOC
