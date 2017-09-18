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

#ifndef WEAVE_STATS_H
#define WEAVE_STATS_H

#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveMessageLayer.h>
#include <Weave/Core/WeaveConfig.h>
#include <SystemLayer/SystemStats.h>

namespace nl {
namespace Weave {
namespace Stats {

void UpdateSnapshot(nl::Weave::System::Stats::Snapshot &aSnapshot);

void SetObjects(WeaveMessageLayer *aMessageLayer);

} // namespace Stats
} // namespace Weave
} // namespace nl

#endif // WEAVE_STATS_H
