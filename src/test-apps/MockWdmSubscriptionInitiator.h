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
 *      This file declares the Weave Data Management mock subscription initiator.
 *
 */

#ifndef MOCKWDMSUBSCRIPTIONINITIATOR_H_
#define MOCKWDMSUBSCRIPTIONINITIATOR_H_


#include <Weave/Core/WeaveExchangeMgr.h>
#include "ToolCommonOptions.h"
#include "MockWdmNodeOptions.h"

class MockWdmSubscriptionInitiator
{
public:
    static MockWdmSubscriptionInitiator * GetInstance();
    static uint32_t GetNumUpdatableTraits(void);

    virtual WEAVE_ERROR Init(nl::Weave::WeaveExchangeManager *aExchangeMgr,
                             uint32_t aKeyId,
                             uint32_t aTestSecurityMode,
                             const MockWdmNodeOptions &aConfig) = 0;

    virtual WEAVE_ERROR StartTesting(const uint64_t aPublisherNodeId, const uint16_t aSubnetId) = 0;

    virtual int32_t GetNumFaultInjectionEventsAvailable(void) = 0;

    typedef void(*HandleCompleteTestFunct)();
    HandleCompleteTestFunct onCompleteTest;
    HandleCompleteTestFunct onError;

    virtual void PrintVersionsLog() = 0;
    virtual void ClearDataSinkState(void) = 0;
};


#endif /* MOCKWDMSUBSCRIPTIONINITIATOR_H_ */
