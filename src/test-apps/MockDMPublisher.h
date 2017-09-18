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
 *    @file
 *      This file defines a derived Weave Data Management (WDM)
 *      publisher for the Weave Data Management profile and protocol
 *      used for the Weave mock device command line functional testing
 *      tool.
 *
 */

#include "TestProfile.h"
#include "ToolCommon.h"

/*
 * In order to use the Weave Data Management framework we must create
 * a test publisher and, it should have, by rights, a subscription
 * table.
 */

class MockDMPublisher : public nl::Weave::Profiles::DataManagement::DMPublisher
{
public:
    /*
     * these are the concrete methods required in order to implement
     * a publisher (strictly, a data manager).
     */

    WEAVE_ERROR ViewIndication(ExchangeContext *aResponseCtx, ReferencedTLVData &aPathList);

    WEAVE_ERROR UpdateIndication(ExchangeContext *aResponseCtx, ReferencedTLVData &aDataList);

    void IncompleteIndication(const uint64_t &aPeerNodeId, StatusReport &aReport);

#if WEAVE_CONFIG_WDM_ALLOW_PUBLISHER_SUBSCRIPTION

    WEAVE_ERROR SubscribeIndication(ExchangeContext *aResponseCtx, const nl::Weave::Profiles::DataManagement::TopicIdentifier &aTopicId);

    WEAVE_ERROR SubscribeIndication(ExchangeContext *aResponseCtx, ReferencedTLVData &aPathList);

    WEAVE_ERROR UnsubscribeIndication(const uint64_t &aClientId,
                                      const nl::Weave::Profiles::DataManagement::TopicIdentifier &aTopicId,
                                      StatusReport &aReport);

    WEAVE_ERROR CancelSubscriptionIndication(ExchangeContext *aResponseCtx, const nl::Weave::Profiles::DataManagement::TopicIdentifier &aTopicId);

    WEAVE_ERROR NotifyConfirm(const uint64_t &aResponderId,
                              const nl::Weave::Profiles::DataManagement::TopicIdentifier &aTopicId,
                              StatusReport &aStatus,
                              uint16_t aTxnId);

    /*
     * this method is "special" to the mock publisher and essentially
     * is called to modify the test data in some way and issue a
     * notification at predetermined intervals.
     */

    WEAVE_ERROR Republish(void);

    uint16_t       mRepublicationCounter;

#endif // WEAVE_CONFIG_WDM_ALLOW_PUBLISHER_SUBSCRIPTION

    /*
     * and the publisher, like the client, has its own copy of the test database
     * plus a counter that tells it how often to republish.
     */

    TestProfileDB  mDatabase;
};
