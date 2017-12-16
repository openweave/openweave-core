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
 *      This file implements the Weave Data Management mock subscription responder.
 *
 */

#define WEAVE_CONFIG_ENABLE_FUNCT_ERROR_LOGGING 1

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif // __STDC_FORMAT_MACROS
#include <inttypes.h>

#include <Weave/Profiles/data-management/Current/WdmManagedNamespace.h>

#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveTLVDebug.hpp>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Profiles/common/CommonProfile.h>
#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Profiles/status-report/StatusReportProfile.h>
#include <Weave/Support/CodeUtils.h>
#include <Weave/Support/TimeUtils.h>
#include <Weave/Support/ASN1.h>
#include "MockSourceTraits.h"

#include "TestWdmOneWayCommand.h"

#define TOOL_NAME "TestWdmOneWayCommandReceiver"

using namespace nl::Weave::TLV;
using namespace nl::Weave::Profiles;
using namespace nl::Weave::Profiles::DataManagement;

using nl::Weave::System::PacketBuffer;

static TestWdmOneWayCommandReceiver gWdmOneWayCommandReceiver;

nl::Weave::Profiles::DataManagement::SubscriptionEngine * nl::Weave::Profiles::DataManagement::SubscriptionEngine::GetInstance()
{
    static nl::Weave::Profiles::DataManagement::SubscriptionEngine gWdmSubscriptionEngine;

    return &gWdmSubscriptionEngine;
}

TestWdmOneWayCommandReceiver * TestWdmOneWayCommandReceiver::GetInstance ()
{
    return &gWdmOneWayCommandReceiver;
}

TestWdmOneWayCommandReceiver::TestWdmOneWayCommandReceiver() :
    mSourceCatalog(ResourceIdentifier(ResourceIdentifier::SELF_NODE_ID), mSourceCatalogStore, sizeof(mSourceCatalogStore) / sizeof(mSourceCatalogStore[0]))
{
}

WEAVE_ERROR TestWdmOneWayCommandReceiver::Init (nl::Weave::WeaveExchangeManager *aExchangeMgr)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    WeaveLogDetail(DataManagement, "TestWdmOneWayCommandReceiver Init");

    mTestADataSource.mTraitTestSet = 0;

    mSourceCatalog.Add(TEST_TRAIT_INSTANCE_ID, &mTestADataSource, mTraitHandleSet[kTestATraitSource0Index]);

    mExchangeMgr = aExchangeMgr;

    err = SubscriptionEngine::GetInstance()->Init(mExchangeMgr, this, EngineEventCallback);
    SuccessOrExit(err);

    err = SubscriptionEngine::GetInstance()->EnablePublisher(NULL, &mSourceCatalog);
    SuccessOrExit(err);

exit:
    return err;
}

void TestWdmOneWayCommandReceiver::EngineEventCallback (void * const aAppState,
    SubscriptionEngine::EventID aEvent,
    const SubscriptionEngine::InEventParam & aInParam, SubscriptionEngine::OutEventParam & aOutParam)
{
    switch (aEvent)
    {
    default:
        SubscriptionEngine::DefaultEventHandler(aEvent, aInParam, aOutParam);
        break;
    }
}

int main(int argc, char *argv[])
{
    InitSystemLayer();
    InitNetwork();
    InitWeaveStack(true, true);

    TestWdmOneWayCommandReceiver::GetInstance()->Init(&ExchangeMgr);

    while (!Done)
    {
        struct timeval sleepTime;
        sleepTime.tv_sec = 0;
        sleepTime.tv_usec = 100000;

        ServiceNetwork(sleepTime);

    }

    ShutdownWeaveStack();
    ShutdownNetwork();
    ShutdownSystemLayer();

    return 0;
}
