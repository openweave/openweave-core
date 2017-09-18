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

#include <Weave/Support/CodeUtils.h>

#include "MockTimeSyncUtil.h"
#include "MockTimeSyncServer.h"

#if WEAVE_CONFIG_TIME
#if WEAVE_CONFIG_TIME_ENABLE_SERVER

using namespace nl::Weave::Profiles::Time;

MockTimeSyncServer::MockTimeSyncServer()
{
}

WEAVE_ERROR MockTimeSyncServer::Init(WeaveExchangeManager *exchangeMgr)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    err = InitServer(this, exchangeMgr);
    SuccessOrExit(err);

    // declare our existence through multicasting
    // this is not needed for cloud service, as multicast doesn't make sense for it
    // note the encryption type and key id need to be set right on a encrypted network
    // check WEAVE_CONFIG_REQUIRE_AUTH_TIME_SYNC
    MulticastTimeChangeNotification(kWeaveEncryptionType_None, WeaveKeyId::kNone);

exit:
    WeaveLogFunctError(err);

    return err;
}

WEAVE_ERROR MockTimeSyncServer::Shutdown(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    err = TimeSyncNode::Shutdown();

    WeaveLogFunctError(err);

    return err;
}

#endif // WEAVE_CONFIG_TIME_ENABLE_SERVER
#endif // WEAVE_CONFIG_TIME
