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

#include "MockTimeSyncUtil.h"
#include "MockTimeSyncCoordinator.h"

#include <Weave/Support/logging/WeaveLogging.h>

#if WEAVE_CONFIG_TIME
#if WEAVE_CONFIG_TIME_ENABLE_COORDINATOR

using namespace nl::Weave::Profiles::Time;

MockTimeSyncCoordinator::MockTimeSyncCoordinator()
{
}

WEAVE_ERROR MockTimeSyncCoordinator::Init(nl::Weave::WeaveExchangeManager *exchangeMgr, const uint8_t encryptionType,
    const uint16_t keyId)
{

    // Sync period: 5 seconds
    // Discovery period: 120 seconds
    // Discovery period in the existence of communication error: 30 seconds
    return mCoordinator.InitCoordinator(exchangeMgr, encryptionType, keyId, 5000
#if WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
        , 120000, 30000
#endif // WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
        );
}

WEAVE_ERROR MockTimeSyncCoordinator::Shutdown(void)
{
    return mCoordinator.Shutdown();
}

#endif // WEAVE_CONFIG_TIME_ENABLE_COORDINATOR
#endif // WEAVE_CONFIG_TIME
