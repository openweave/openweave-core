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
 *      This file implements a mock external event generator
 *
 */

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif
#include <new>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Core/WeaveTLVDebug.hpp>
#include <Weave/Core/WeaveTLVUtilities.hpp>
#include <Weave/Profiles/data-management/DataManagement.h>

#include <SystemLayer/SystemTimer.h>

#include <MockExternalEvents.h>

using namespace nl::Weave::TLV;
using namespace nl::Weave::Profiles::DataManagement;

static WEAVE_ERROR FetchMockExternalEvents(EventLoadOutContext *aContext);
static WEAVE_ERROR FetchWithBlitEvent(EventLoadOutContext *aContext);

#define NUM_CALLBACKS 3
event_id_t extEvtPtrs[NUM_CALLBACKS];
size_t numEvents[NUM_CALLBACKS];

event_id_t blitEvtPtr;


WEAVE_ERROR LogMockExternalEvents(size_t aNumEvents, int numCallback)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    nl::Weave::Profiles::DataManagement::LoggingManagement &logger =
        nl::Weave::Profiles::DataManagement::LoggingManagement::GetInstance();

    if ((numCallback > 0) && (numCallback <= NUM_CALLBACKS))
    {
        err = logger.RegisterEventCallbackForImportance(nl::Weave::Profiles::DataManagement::Production, FetchMockExternalEvents, aNumEvents, &extEvtPtrs[numCallback - 1]);
        if (err == WEAVE_NO_ERROR)
        {
            numEvents[numCallback - 1] = aNumEvents;
        }
    }
    else if (numCallback == 0)
    {
        err = logger.RegisterEventCallbackForImportance(Production, FetchWithBlitEvent, aNumEvents, &blitEvtPtr);
    }

    return err;
}

WEAVE_ERROR LogMockDebugExternalEvents(size_t aNumEvents, int numCallback)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    nl::Weave::Profiles::DataManagement::LoggingManagement &logger =
        nl::Weave::Profiles::DataManagement::LoggingManagement::GetInstance();

    if ((numCallback > 0) && (numCallback <= NUM_CALLBACKS))
    {
        err = logger.RegisterEventCallbackForImportance(nl::Weave::Profiles::DataManagement::Debug, FetchMockExternalEvents, aNumEvents, &extEvtPtrs[numCallback - 1]);
        if (err == WEAVE_NO_ERROR)
        {
            numEvents[numCallback - 1] = aNumEvents;
        }
    }
    else if (numCallback == 0)
    {
        err = logger.RegisterEventCallbackForImportance(nl::Weave::Profiles::DataManagement::Debug, FetchWithBlitEvent, aNumEvents, &blitEvtPtr);
    }

    return err;
}

void ClearMockExternalEvents(int numCallback)
{
    nl::Weave::Profiles::DataManagement::LoggingManagement &logger =
        nl::Weave::Profiles::DataManagement::LoggingManagement::GetInstance();

    logger.UnregisterEventCallbackForImportance(nl::Weave::Profiles::DataManagement::Production, extEvtPtrs[numCallback - 1]);
}

WEAVE_ERROR FetchMockExternalEvents(EventLoadOutContext *aContext)
{
    int i;
    for (i = 0; i < 3; i++)
    {
        if ((aContext->mExternalEvents != NULL) &&
            aContext->mExternalEvents->IsValid() &&
            aContext->mExternalEvents->mLastEventID == extEvtPtrs[i])
        {
            aContext->mCurrentEventID = extEvtPtrs[i] + 1;
            break;
        }
    }

    return WEAVE_NO_ERROR;
}

struct DebugLogContext
{
    const char *mRegion;
    const char *mFmt;
    va_list mArgs;
};

WEAVE_ERROR FetchWithBlitEvent(EventLoadOutContext *aContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    DebugLogContext context;
    EventSchema schema = { kWeaveProfile_NestDebug,
                           kNestDebug_StringLogEntryEvent,
                           Production,
                           1,
                           1 };
    EventOptions evOpts = EventOptions(static_cast<utc_timestamp_t>(nl::Weave::System::Timer::GetCurrentEpoch()));
    LoggingManagement &logger = LoggingManagement::GetInstance();

    char testString[50];
    memset(testString, 'x', sizeof(testString));
    testString[49] = '\0';

    context.mFmt = testString;
    context.mRegion = "";

    while ((err == WEAVE_NO_ERROR) && (aContext->mCurrentEventID <= blitEvtPtr))
    {
        err = logger.BlitEvent(aContext, schema, PlainTextWriter, &context, &evOpts);
    }
    return err;
}
