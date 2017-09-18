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

#ifndef MOCKTIMECOORDINATOR_H_
#define MOCKTIMECOORDINATOR_H_

#if WEAVE_CONFIG_TIME_ENABLE_COORDINATOR

#include <Weave/Profiles/time/WeaveTime.h>

// Simple class which contains one instance of Client and Server, and link them together through callbacks
class MockTimeSyncCoordinator
{
public:
    MockTimeSyncCoordinator();

    WEAVE_ERROR Init(nl::Weave::WeaveExchangeManager *exchangeMgr, const uint8_t encryptionType =
        nl::Weave::kWeaveEncryptionType_None,
        const uint16_t keyId = nl::Weave::WeaveKeyId::kNone);

    WEAVE_ERROR Shutdown(void);

private:
    nl::Weave::Profiles::Time::TimeSyncNode mCoordinator;
};

#endif // WEAVE_CONFIG_TIME_ENABLE_COORDINATOR

#endif /* MOCKTIMECOORDINATOR_H_ */
