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

#ifndef MOCKTIMESYNCSERVER_H_
#define MOCKTIMESYNCSERVER_H_

#if WEAVE_CONFIG_TIME_ENABLE_SERVER

#include <Weave/Profiles/time/WeaveTime.h>

namespace nl {

namespace Weave {

namespace Profiles {

namespace Time {

// Inherent the TimeSyncNode for the sake of demonstration
// we can also just aggregate an instance of TimeSyncNode, just like what has been
// demonstrated in MockTimeSyncServer
class MockTimeSyncServer:
    TimeSyncNode
{
public:
    MockTimeSyncServer();

    WEAVE_ERROR Init(nl::Weave::WeaveExchangeManager *exchangeMgr);
    WEAVE_ERROR Shutdown(void);
};

} // namespace Time

} // namespace Profiles

} // namespace Weave

} // namespace nl

#endif // WEAVE_CONFIG_TIME_ENABLE_SERVER

#endif /* MOCKTIMESYNCSERVER_H_ */
