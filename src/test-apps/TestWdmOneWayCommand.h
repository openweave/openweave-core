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

#ifndef TEST_WDM_ONEWAY_COMMAND_H_
#define TEST_WDM_ONEWAY_COMMAND_H_

#include <Weave/Profiles/data-management/DataManagement.h>
#include "MockSourceTraits.h"

#define TEST_TRAIT_INSTANCE_ID       (1)
#define TEST_COMMAND_TYPE            (1)
#define TEST_SCHEMA_MAX_VER          (4)
#define TEST_SCHEMA_MIN_VER          (1)

class TestWdmOneWayCommandReceiver
{
public:

    TestWdmOneWayCommandReceiver();

    static TestWdmOneWayCommandReceiver * GetInstance ();

    WEAVE_ERROR Init (nl::Weave::WeaveExchangeManager *aExchangeMgr);

private:
    nl::Weave::WeaveExchangeManager *mExchangeMgr;

    // publisher side
    nl::Weave::Profiles::DataManagement::SingleResourceSourceTraitCatalog mSourceCatalog;
    nl::Weave::Profiles::DataManagement::SingleResourceSourceTraitCatalog::CatalogItem mSourceCatalogStore[8];

    // source traits
    TestATraitDataSource mTestADataSource;

    enum
    {
        kTestATraitSource0Index = 0,
        kTestATraitSource1Index,

        kNumTraitHandles,
    };

    nl::Weave::Profiles::DataManagement::TraitDataHandle mTraitHandleSet[kNumTraitHandles];

    static void EngineEventCallback (void * const aAppState,
                                     nl::Weave::Profiles::DataManagement::SubscriptionEngine::EventID aEvent,
                                     const nl::Weave::Profiles::DataManagement::SubscriptionEngine::InEventParam & aInParam,
                                     nl::Weave::Profiles::DataManagement::SubscriptionEngine::OutEventParam & aOutParam);

};

class TestWdmOneWayCommandSender
{
public:

    TestWdmOneWayCommandSender();

    static TestWdmOneWayCommandSender * GetInstance ();

    WEAVE_ERROR Init(nl::Weave::WeaveExchangeManager *aExchangeMgr,
                     const nl::Inet::IPAddress & destAddr,
                     const uint64_t destNodeId);

    WEAVE_ERROR SendOneWayCommand(void);

    WEAVE_ERROR Shutdown(void);

private:
    nl::Weave::WeaveExchangeManager *mExchangeMgr;
    Binding *mClientBinding;
    nl::Weave::Profiles::DataManagement::CommandSender mCommandSender;

    static void BindingEventCallback (void * const apAppState,
            const nl::Weave::Binding::EventType aEvent,
            const nl::Weave::Binding::InEventParam & aInParam,
            nl::Weave::Binding::OutEventParam & aOutParam);

    static void CommandEventHandler(void * const aAppState, nl::Weave::Profiles::DataManagement::CommandSender::EventType aEvent,
            const nl::Weave::Profiles::DataManagement::CommandSender::InEventParam &aInParam,
            nl::Weave::Profiles::DataManagement::CommandSender::OutEventParam &aOutEventParam);
};

#endif // TEST_WDM_ONEWAY_COMMAND_H_
