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
 *      The Weave::Stats class allows the application to monitor the
 *      resources used by Weave features.
 *
 */

#include <Weave/Core/WeaveCore.h>
#include <Weave/Support/CodeUtils.h>
#include <Weave/Core/WeaveExchangeMgr.h>
#include <Weave/Core/WeaveStats.h>
#include <InetLayer/InetLayer.h>

namespace nl {
namespace Weave {
namespace Stats {

static WeaveMessageLayer *sMessageLayer;

/**
 * Registers weave objects with the stats infra.
 *
 * Collecting some of the statistics requires access to a specific Weave
 * object instance owned by the application.
 *
 * @param[in]   aMessageLayer   Pointer to the MessageLayer object from
 *                              which stats are to be collected.
 */
void SetObjects(WeaveMessageLayer *aMessageLayer)
{
    sMessageLayer = aMessageLayer;
}

/**
 * Updates a Snapshot instance with the current state of the system resources.
 *
 * @param[in] aSnapshot     The Snapshot to be updated.
 */
void UpdateSnapshot(nl::Weave::System::Stats::Snapshot &aSnapshot)
{
    // Always start from System
    nl::Weave::System::Stats::UpdateSnapshot(aSnapshot);

    nl::Inet::InetLayer::UpdateSnapshot(aSnapshot);

    if (sMessageLayer)
    {
        sMessageLayer->GetConnectionPoolStats(aSnapshot.mResourcesInUse[nl::Weave::System::Stats::kMessageLayer_NumConnections]);
    }
}

} // namespace Stats
} // namespace Weave
} // namespace nl
