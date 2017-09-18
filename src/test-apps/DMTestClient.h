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

/**
 * @file
 * @brief Definitions for the DMTestClient class.
 *
 * This file defines DMTestClient, a general-purpose test client for
 * Weave data management.
 */

#ifndef _DM_TEST_CLIENT_H
#define _DM_TEST_CLIENT_H

#include <Weave/Profiles/data-management/ProfileDatabase.h>

/**
 * @class DMTestClient
 *
 * It implements all the required confirm and indication handlers
 * using the Done global provided by ToolCommon.h as a way of
 * controlling execution. It also has a "pluggable" ProfileDatabase
 * object in which it stores data as necessary.
 */

class DMTestClient :

    public DMClient

{
public:

    WEAVE_ERROR ViewConfirm(const uint64_t &aResponderId, StatusReport &aStatus, uint16_t aTxnId);

    WEAVE_ERROR ViewConfirm(const uint64_t &aResponderId, ReferencedTLVData &aDataList, uint16_t aTxnId);

    WEAVE_ERROR SubscribeConfirm(const uint64_t &aResponderId, StatusReport &aStatus, uint16_t aTxnId);

    WEAVE_ERROR SubscribeConfirm(const uint64_t &aResponderId, const TopicIdentifier &aTopicId, ReferencedTLVData &aDataList, uint16_t aTxnId);

    WEAVE_ERROR UnsubscribeIndication(const uint64_t &aPublisherId, const TopicIdentifier &aTopicId, StatusReport &aReport);

    WEAVE_ERROR UpdateConfirm(const uint64_t &aResponderId, StatusReport &aStatus, uint16_t aTxnId);

    WEAVE_ERROR CancelSubscriptionConfirm(const uint64_t &aResponderId, const TopicIdentifier &aTopicId, StatusReport &aStatus, uint16_t aTxnId);

    WEAVE_ERROR NotifyIndication(const TopicIdentifier &aTopicId, ReferencedTLVData &aDataList);

    /*
     * the client comes with a profile database
     */

    ProfileDatabase mDatabase;
};

#endif // __DM_TEST_CLIENT_H
