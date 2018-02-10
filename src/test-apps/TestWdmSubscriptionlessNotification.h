/*
 *
 *    Copyright (c) 2017-2018 Nest Labs, Inc.
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
 *      This file declares the classes for yesting the Weave Data Management OneWayCommand.
 *
 */

#ifndef TEST_WDM_SUBSCRIPTIONLESS_NOTIFICATION_H_
#define TEST_WDM_SUBSCRIPTIONLESS_NOTIFICATION_H_

#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Profiles/data-management/TraitCatalog.h>

#include "MockSourceTraits.h"
#include "MockSinkTraits.h"

#define TEST_TRAIT_INSTANCE_ID       (1)
#define TEST_SCHEMA_MAX_VER          (4)
#define TEST_SCHEMA_MIN_VER          (1)

#if WDM_ENABLE_SUBSCRIPTIONLESS_NOTIFICATION

class TestWdmSubscriptionlessNotificationSender
{
public:

    TestWdmSubscriptionlessNotificationSender();

    static TestWdmSubscriptionlessNotificationSender * GetInstance ();

    WEAVE_ERROR Init (nl::Weave::WeaveExchangeManager *aExchangeMgr,
                      const uint16_t destSubnetId,
                      const uint64_t destNodeId);

    WEAVE_ERROR SendSubscriptionlessNotify(void);

    WEAVE_ERROR Shutdown(void);

private:
    nl::Weave::WeaveExchangeManager *mExchangeMgr;
    Binding *mBinding;

    nl::Weave::Profiles::DataManagement::SingleResourceSourceTraitCatalog mSourceCatalog;
    nl::Weave::Profiles::DataManagement::SingleResourceSourceTraitCatalog::CatalogItem mSourceCatalogStore[8];

    // source traits
    TestATraitDataSource mTestATraitDataSource0;
    TestATraitDataSource mTestATraitDataSource1;
    TestATraitDataSource mTestATraitDataSource2;

    enum
    {
        kTestATraitSource0Index = 0,
        kTestATraitSource1Index,
        kTestATraitSource2Index,
        kMaxNumTraits,
    };
    nl::Weave::Profiles::DataManagement::TraitPath mTraitPaths[4];
    uint32_t mNumPaths;

    static void EngineEventCallback (void * const aAppState,
                                     nl::Weave::Profiles::DataManagement::SubscriptionEngine::EventID aEvent,
                                     const nl::Weave::Profiles::DataManagement::SubscriptionEngine::InEventParam & aInParam,
                                     nl::Weave::Profiles::DataManagement::SubscriptionEngine::OutEventParam & aOutParam);

    static void BindingEventCallback (void * const apAppState,
            const nl::Weave::Binding::EventType aEvent,
            const nl::Weave::Binding::InEventParam & aInParam,
            nl::Weave::Binding::OutEventParam & aOutParam);

};

class TestWdmSubscriptionlessNotificationReceiver
{
public:

    TestWdmSubscriptionlessNotificationReceiver();

    static TestWdmSubscriptionlessNotificationReceiver * GetInstance ();

    WEAVE_ERROR Init(nl::Weave::WeaveExchangeManager *aExchangeMgr);

    typedef void(*HandleTestCompleteFunct)();
    HandleTestCompleteFunct OnTestComplete;
    HandleTestCompleteFunct OnError;

private:
    nl::Weave::WeaveExchangeManager *mExchangeMgr;
    nl::Weave::Profiles::DataManagement::SingleResourceSinkTraitCatalog mSinkCatalog;
    nl::Weave::Profiles::DataManagement::SingleResourceSinkTraitCatalog::CatalogItem mSinkCatalogStore[8];

    TestATraitDataSink mTestATraitDataSink0;
    TestATraitDataSink mTestATraitDataSink1;
    TestATraitDataSink mTestATraitDataSink2;

    static void EngineEventCallback (void * const aAppState,
                                     nl::Weave::Profiles::DataManagement::SubscriptionEngine::EventID aEvent,
                                     const nl::Weave::Profiles::DataManagement::SubscriptionEngine::InEventParam & aInParam,
                                     nl::Weave::Profiles::DataManagement::SubscriptionEngine::OutEventParam & aOutParam);
};

#endif // WDM_ENABLE_SUBSCRIPTIONLESS_NOTIFICATION
#endif // TEST_WDM_SUBSCRIPTIONLESS_NOTIFICATION_H_
