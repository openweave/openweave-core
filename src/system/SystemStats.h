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
 * @file
 *  This file declares the Weave API to collect statistics
 *  on the state of Weave, Inet and System resources
 */

#ifndef SYSTEMSTATS_H
#define SYSTEMSTATS_H
// Include standard C library limit macros
#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif
#include <stdint.h>

// Include configuration headers
#include <SystemLayer/SystemConfig.h>

// Include dependent headers
#include <Weave/Support/NLDLLUtil.h>

namespace nl {
namespace Weave {
namespace System {
namespace Stats {

enum
{
    kSystemLayer_NumPacketBufs,
    kSystemLayer_NumTimers,
    kInetLayer_NumRawEps,
    kInetLayer_NumTCPEps,
    kInetLayer_NumUDPEps,
    kInetLayer_NumTunEps,
    kInetLayer_NumDNSResolvers,
    kExchangeMgr_NumContexts,
    kExchangeMgr_NumUMHandlers,
    kMessageLayer_NumConnections,
    kServiceMgr_NumRequests,
    kWDMClient_NumViews,
    kWDMClient_NumSubscribes,
    kWDMClient_NumUpdates,
    kWDMClient_NumCancels,
    kWDMClient_NumBindings,
    kWDMClient_NumTransactions,
    kWDMNext_NumBindings,
    kWDMNext_NumTraits,
    kWDMNext_NumSubscriptionClients,
    kWDMNext_NumSubscriptionHandlers,
    kWDMNext_NumCommands,

    kNumEntries
};

typedef int32_t count_t;

extern count_t ResourcesInUse[kNumEntries];
extern count_t HighWatermarks[kNumEntries];

class Snapshot
{
public:

    count_t mResourcesInUse[kNumEntries];
    count_t mHighWatermarks[kNumEntries];
};

bool Difference(Snapshot &result, Snapshot &after, Snapshot &before);
void UpdateSnapshot(Snapshot &aSnapshot);
count_t *GetResourcesInUse(void);
count_t *GetHighWatermarks(void);
const char **GetStrings(void);

} // namespace Stats
} // namespace System
} // namespace Weave
} // namespace nl

#if WEAVE_SYSTEM_CONFIG_PROVIDE_STATISTICS

#define SYSTEM_STATS_INCREMENT(entry) \
    do { \
        nl::Weave::System::Stats::count_t new_value = ++(nl::Weave::System::Stats::GetResourcesInUse()[entry]); \
        if (nl::Weave::System::Stats::GetHighWatermarks()[entry] < new_value) \
        { \
            nl::Weave::System::Stats::GetHighWatermarks()[entry] = new_value; \
        } \
    } while (0);

#define SYSTEM_STATS_DECREMENT(entry) \
    do { \
        nl::Weave::System::Stats::GetResourcesInUse()[entry]--; \
    } while (0);

#define SYSTEM_STATS_DECREMENT_BY_N(entry, count) \
    do { \
        nl::Weave::System::Stats::GetResourcesInUse()[entry] -= (count); \
    } while (0);

#define SYSTEM_STATS_SET(entry, count) \
    do { \
        nl::Weave::System::Stats::count_t new_value = nl::Weave::System::Stats::GetResourcesInUse()[entry] = (count); \
        if (nl::Weave::System::Stats::GetHighWatermarks()[entry] < new_value) \
        { \
            nl::Weave::System::Stats::GetHighWatermarks()[entry] = new_value; \
        } \
    } while (0);

#define SYSTEM_STATS_RESET(entry) \
    do { \
        nl::Weave::System::Stats::GetResourcesInUse()[entry] = 0; \
    } while (0);



#else // WEAVE_SYSTEM_CONFIG_PROVIDE_STATISTICS

#define SYSTEM_STATS_INCREMENT(entry)

#define SYSTEM_STATS_DECREMENT(entry)

#define SYSTEM_STATS_DECREMENT_BY_N(entry, count)

#define SYSTEM_STATS_RESET(entry)

#endif // WEAVE_SYSTEM_CONFIG_PROVIDE_STATISTICS

#endif // defined(SYSTEMSTATS_H)
