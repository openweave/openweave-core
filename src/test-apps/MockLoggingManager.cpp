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
 *      Mock setup for event logging
 *
 */

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

// Note that the choice of namespace alias must be made up front for each and every compile unit
// This is because many include paths could set the default alias to unintended target.
#include <Weave/Profiles/bulk-data-transfer/Development/BDXManagedNamespace.hpp>
#include <Weave/Profiles/data-management/Current/WdmManagedNamespace.h>

#include "ToolCommon.h"
#include <InetLayer/Inet.h>
#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveMessageLayer.h>
#include <Weave/Core/WeaveEncoding.h>
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Core/WeaveTLVDebug.hpp>
#include <Weave/Core/WeaveTLVUtilities.hpp>
#include <Weave/Core/WeaveTLVData.hpp>
#include <Weave/Core/WeaveSecurityMgr.h>
#include <Weave/Profiles/security/WeaveSecurity.h>
#include <Weave/Profiles/ProfileCommon.h>
#include <Weave/Profiles/data-management/DataManagement.h>
#include <MockLoggingManager.h>
#include <MockEvents.h>

using namespace nl::Weave::TLV;
using namespace nl::Weave::Profiles::DataManagement;

class MockEventGeneratorImpl: public MockEventGenerator
{
public:
    MockEventGeneratorImpl(void);
    WEAVE_ERROR Init(nl::Weave::WeaveExchangeManager *aExchangeMgr, EventGenerator *aEventGenerator, int aDelayBetweenEvents, bool aWraparound);
    void SetEventGeneratorStop();
    bool IsEventGeneratorStopped();

private:
    static void HandleNextEvent(nl::Weave::System::Layer* aSystemLayer, void *aAppState, ::nl::Weave::System::Error aErr);
    nl::Weave::WeaveExchangeManager *mExchangeMgr;
    int mTimeBetweenEvents; //< delay, in miliseconds, between events.
    bool mEventWraparound; //< does the event generator run indefinitely, or does it stop after iterating through its states
    EventGenerator *mEventGenerator; //< the event generator to use
    int32_t mEventsLeft;
};

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current) {
namespace Platform {
    // for unit tests, the dummy critical section is sufficient.
    void CriticalSectionEnter()
    {
        return;
    }

    void CriticalSectionExit()
    {
        return;
    }
} // Platform
} // WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current)
} // Profiles
} // Weave
} // nl

uint64_t gDebugEventBuffer[192];
uint64_t gInfoEventBuffer[64];
uint64_t gProdEventBuffer[256];
nl::Weave::MonotonicallyIncreasingCounter gDebugEventCounter;
nl::Weave::MonotonicallyIncreasingCounter gInfoEventCounter;
nl::Weave::MonotonicallyIncreasingCounter gProdEventCounter;


bool gMockEventStop = false;
bool gEventIsStopped = false;
bool gEnableMockTimestampInitialCounter = false;

EventGenerator * GetTestDebugGenerator(void)
{
    static DebugEventGenerator gDebugGenerator;
    return &gDebugGenerator;
}

EventGenerator * GetTestLivenessGenerator(void)
{
    static LivenessEventGenerator gLivenessGenerator;
    return &gLivenessGenerator;
}

EventGenerator * GetTestSecurityGenerator(void)
{
    static SecurityEventGenerator gSecurityGenerator;
    return &gSecurityGenerator;
}

EventGenerator * GetTestTelemetryGenerator(void)
{
    static TelemetryEventGenerator gTelemetryGenerator;
    return &gTelemetryGenerator;
}
EventGenerator * GetTestTraitGenerator(void)
{
    static TestTraitEventGenerator gTestTraitGenerator;
    return &gTestTraitGenerator;
}

void EnableMockEventTimestampInitialCounter(void)
{
    gEnableMockTimestampInitialCounter = true;
}

void InitializeEventLogging(WeaveExchangeManager *inMgr)
{
    size_t arraySizes[] = { sizeof(gDebugEventBuffer), sizeof(gInfoEventBuffer), sizeof(gProdEventBuffer) };

    void *arrays[] = {
        static_cast<void *>(&gDebugEventBuffer[0]),
        static_cast<void *>(&gInfoEventBuffer[0]),
        static_cast<void *>(&gProdEventBuffer[0]) };

    if (gEnableMockTimestampInitialCounter)
    {
        uint32_t startingCounter = time(NULL);
        WeaveLogDetail(EventLogging, "Event counter is initialized with timestamp %08" PRIx32 , startingCounter);
        gDebugEventCounter.Init(startingCounter);
        gInfoEventCounter.Init(startingCounter);
        gProdEventCounter.Init(startingCounter);

        nl::Weave::MonotonicallyIncreasingCounter *nWeaveCounter[] = { &gDebugEventCounter, &gInfoEventCounter, &gProdEventCounter };

        nl::Weave::Profiles::DataManagement::LoggingManagement::CreateLoggingManagement(inMgr, 3, &arraySizes[0],
                                                                                        &arrays[0], nWeaveCounter);
    }
    else
    {
        nl::Weave::Profiles::DataManagement::LoggingManagement::CreateLoggingManagement(inMgr, 3, &arraySizes[0], &arrays[0], NULL, NULL, NULL);
    }
}

MockEventGenerator * MockEventGenerator::GetInstance(void)
{
    static MockEventGeneratorImpl gMockEventGenerator;
    return &gMockEventGenerator;
}

MockEventGeneratorImpl::MockEventGeneratorImpl(void) :
    mExchangeMgr(NULL),
    mTimeBetweenEvents(0),
    mEventWraparound(false),
    mEventGenerator(NULL),
    mEventsLeft(0)
{
}

WEAVE_ERROR MockEventGeneratorImpl::Init(nl::Weave::WeaveExchangeManager *aExchangeMgr, EventGenerator *aEventGenerator, int aDelayBetweenEvents, bool aWraparound)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    mExchangeMgr = aExchangeMgr;
    mEventGenerator = aEventGenerator;
    mTimeBetweenEvents = aDelayBetweenEvents;
    mEventWraparound = aWraparound;

    if (mEventWraparound)
        mEventsLeft = INT32_MAX;
    else
        mEventsLeft = mEventGenerator->GetNumStates();

    if (mTimeBetweenEvents != 0)
        mExchangeMgr->MessageLayer->SystemLayer->StartTimer(mTimeBetweenEvents, HandleNextEvent, this);

    return err;
}

void MockEventGeneratorImpl::HandleNextEvent(nl::Weave::System::Layer* aSystemLayer, void *aAppState, ::nl::Weave::System::Error aErr)
{
    MockEventGeneratorImpl * generator = static_cast<MockEventGeneratorImpl *>(aAppState);
    if (gMockEventStop)
    {
        gEventIsStopped = true;
        aSystemLayer->CancelTimer(HandleNextEvent, generator);
    }
    else
    {
        generator->mEventGenerator->Generate();
        generator->mEventsLeft--;
        if ((generator->mEventWraparound) || (generator->mEventsLeft > 0))
        {
            aSystemLayer->StartTimer(generator->mTimeBetweenEvents, HandleNextEvent, generator);
        }
    }
}

void MockEventGeneratorImpl::SetEventGeneratorStop()
{
    gMockEventStop = true;

    // If the timer is running, make it expire right away.
    // This helps quit the standalone app in an orderly way without
    // spurious leaked timers.
    if (mTimeBetweenEvents != 0)
        mExchangeMgr->MessageLayer->SystemLayer->StartTimer(0, HandleNextEvent, this);
}

bool MockEventGeneratorImpl::IsEventGeneratorStopped()
{
    if (gEventIsStopped)
    {
        gMockEventStop = false;
        gEventIsStopped = false;
        return true;
    }
    else
    {
        return false;
    }
}
