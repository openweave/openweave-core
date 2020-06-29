/*
 *
 *    Copyright (c) 2019 Google LLC.
 *    Copyright (c) 2014-2018 Nest Labs, Inc.
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
 *      Defines variable sized pbuf pools for standalone LwIP builds.
 *
 */


#if LWIP_PBUF_FROM_CUSTOM_POOLS

// Variable-sized pbuf pools for use when LWIP_PBUF_FROM_CUSTOM_POOLS is enabled.
// Pools must be arranged in decreasing order of size.
LWIP_PBUF_MEMPOOL(PBUF_POOL_LARGE,  PBUF_POOL_SIZE_LARGE,  PBUF_POOL_BUFSIZE_LARGE,  "PBUF_POOL_LARGE")
LWIP_PBUF_MEMPOOL(PBUF_POOL_MEDIUM, PBUF_POOL_SIZE_MEDIUM, PBUF_POOL_BUFSIZE_MEDIUM, "PBUF_POOL_MEDIUM")
LWIP_PBUF_MEMPOOL(PBUF_POOL_SMALL,  PBUF_POOL_SIZE_SMALL,  PBUF_POOL_BUFSIZE_SMALL,  "PBUF_POOL_SMALL")

#endif // LWIP_PBUF_FROM_CUSTOM_POOLS
