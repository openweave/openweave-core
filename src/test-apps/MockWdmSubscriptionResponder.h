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
 *      This file declare the Weave Data Management mock subscription responder.
 *
 */

#ifndef MOCKWDMSUBSCRIPTIONRESPONDER_H_
#define MOCKWDMSUBSCRIPTIONRESPONDER_H_


class MockWdmSubscriptionResponder
{
public:
    static MockWdmSubscriptionResponder * GetInstance ();

    virtual WEAVE_ERROR Init (nl::Weave::WeaveExchangeManager *aExchangeMgr, const bool aMutualSubscription,
                              const char * const aTestCaseId, const char * const aNumDataChangeBeforeCancellation,
                              const char * const aFinalStatus, const char * const aTimeBetweenDataChangeMsec,
                              const bool aEnableDataFlip, const char * const aTimeBetweenLivenessCheckSec) = 0;

    typedef void(*HandleCompleteTestFunct)();
    HandleCompleteTestFunct onCompleteTest;
    HandleCompleteTestFunct onError;

    virtual void PrintVersionsLog() = 0;
    virtual void ClearDataSinkState(void) = 0;
};


#endif /* MOCKWDMSUBSCRIPTIONRESPONDER_H_ */
