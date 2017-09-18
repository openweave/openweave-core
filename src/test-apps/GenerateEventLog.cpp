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
 *      Sample log outputs for testing
 *
 */

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#include <new>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <nlbyteorder.h>
#include <nltest.h>

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
#include <Weave/Profiles/time/WeaveTime.h>
#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Support/CodeUtils.h>

#include <MockEvents.h>

using namespace nl::Weave::TLV;
using namespace nl::Weave::Profiles::DataManagement;

static bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg);

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


nl::Weave::Profiles::DataManagement::SubscriptionEngine * nl::Weave::Profiles::DataManagement::SubscriptionEngine::GetInstance()
{
    static nl::Weave::Profiles::DataManagement::SubscriptionEngine gWdmSubscriptionEngine;

    return &gWdmSubscriptionEngine;
}


#define TOOL_NAME "GenerateEventLog"

#define LOG_BUFFER_SIZE 512

static const uint64_t kTestNodeId     = 0x18B4300001408362ULL;

const uint64_t kSubscriptionId = 0xB6C4B7BE2C4B859AULL;

enum WDMTags
{
    kCsTag_SubscriptionId      = 1,
    kCsTag_PossibleLossOfEvent = 20,
    kCsTag_UTCTimestamp        = 21,
    kCsTag_SystemTimestamp     = 22,
    kCsTag_EventList           = 23
};



/************************************************************************/
struct LogContext
{
    LogContext();
    WeaveExchangeManager *mExchangeMgr;
    const char *mOutputFilename;
    size_t mTestNum;
    nl::Weave::Profiles::DataManagement::ImportanceType mLogLevel;
    bool mRaw;
    bool mVerbose;
    bool mBDX;
    bool mWDMOutput;
};

LogContext gLogContext;

LogContext::LogContext() :
    mExchangeMgr(NULL),
    mOutputFilename(NULL),
    mTestNum(0),
    mLogLevel(nl::Weave::Profiles::DataManagement::Production),
    mRaw(false),
    mVerbose(false),
    mBDX(false),
    mWDMOutput(false)
{
}

static int TestSetup(void *inContext)
{
    LogContext *ctx = static_cast<LogContext *>(inContext);
    static WeaveFabricState sFabricState;
    static WeaveExchangeManager sExchangeMgr;

    if (ctx->mBDX)
    {
        InitSystemLayer();
        InitNetwork();
        InitWeaveStack(true, true);

        ctx->mExchangeMgr = &ExchangeMgr;
    }
    else
    {
        // fake Weave exchange layer.  We are not running any
        // networking tests, and at that point the only functionality
        // required by the event logging subsystem is that the
        // ExchageManager has a fabric state with a node id.

        WEAVE_ERROR err = WEAVE_NO_ERROR;

        err = sFabricState.Init();
        if (err != WEAVE_NO_ERROR)
            return FAILURE;

        sFabricState.LocalNodeId = kTestNodeId;
        sExchangeMgr.FabricState = & sFabricState;
        sExchangeMgr.State = WeaveExchangeManager::kState_Initialized;
        ctx->mExchangeMgr = &sExchangeMgr;
    }
    return SUCCESS;
}

static int TestTeardown(void *inContext)
{
    LogContext *ctx = static_cast<LogContext *>(inContext);
    if (ctx->mBDX)
    {
        ShutdownWeaveStack();
        ShutdownNetwork();
        ShutdownSystemLayer();
    }
    return SUCCESS;
}


uint64_t gInfoEventBuffer[LOG_BUFFER_SIZE];
uint64_t gProdEventBuffer[LOG_BUFFER_SIZE];
FILE * gFileOutput = NULL;

void InitializeEventLogging(LogContext *context)
{
    size_t arraySizes[] = { sizeof(gInfoEventBuffer), sizeof(gProdEventBuffer) };

    void *arrays[] = {
        static_cast<void *>(&gInfoEventBuffer[0]),
        static_cast<void *>(&gProdEventBuffer[0]) };

    nl::Weave::Profiles::DataManagement::LoggingManagement::CreateLoggingManagement(context->mExchangeMgr, 2, &arraySizes[0], &arrays[0], NULL, NULL, NULL);

    nl::Weave::Profiles::DataManagement::LoggingConfiguration::GetInstance().mGlobalImportance = context->mLogLevel;
}

void SimpleDumpWriter(const char *aFormat, ...)
{
    va_list args;

    va_start(args, aFormat);
    if (gFileOutput)
        vfprintf(gFileOutput, aFormat, args);
    else
        vprintf(aFormat, args);

    va_end(args);
}

void DumpEventLog(LogContext *inContext)
{
    uint8_t backingStore[LOG_BUFFER_SIZE*8];
    size_t elementCount;
    event_id_t eventId = 0;
    TLVWriter writer;
    TLVReader reader;
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    FILE *out = stdout;
    TLVType dummyType;
    TLVType dummyType1;

    if (inContext->mOutputFilename)
    {
        gFileOutput = fopen(inContext->mOutputFilename, "w");
    }
    if (gFileOutput)
    {
        out = gFileOutput;
    }

    writer.Init(backingStore, LOG_BUFFER_SIZE*8);

    if (inContext->mWDMOutput)
    {
        err = writer.StartContainer(AnonymousTag, kTLVType_Structure, dummyType);
        SuccessOrExit(err);

        err = writer.Put(ContextTag(kCsTag_SubscriptionId), kSubscriptionId);
        SuccessOrExit(err);

        err = writer.StartContainer(ContextTag(kCsTag_EventList), kTLVType_Array, dummyType1);
        SuccessOrExit(err);
    }

    err = nl::Weave::Profiles::DataManagement::LoggingManagement::GetInstance().FetchEventsSince(writer, nl::Weave::Profiles::DataManagement::Production, eventId);
    if ((err == WEAVE_END_OF_TLV) || (err == WEAVE_ERROR_TLV_UNDERRUN))
    {
        err = WEAVE_NO_ERROR;
    }
    SuccessOrExit(err);
    eventId = 0;
    err = nl::Weave::Profiles::DataManagement::LoggingManagement::GetInstance().FetchEventsSince(writer, nl::Weave::Profiles::DataManagement::Info, eventId);
    if ((err == WEAVE_END_OF_TLV) || (err == WEAVE_ERROR_TLV_UNDERRUN))
    {
        err = WEAVE_NO_ERROR;
    }
    SuccessOrExit(err);
    eventId = 0;
    err = nl::Weave::Profiles::DataManagement::LoggingManagement::GetInstance().FetchEventsSince(writer, nl::Weave::Profiles::DataManagement::Debug, eventId);
    if ((err == WEAVE_END_OF_TLV) || (err == WEAVE_ERROR_TLV_UNDERRUN))
    {
        err = WEAVE_NO_ERROR;
    }
    SuccessOrExit(err);
    eventId = 0;
    err = writer.Finalize();
    SuccessOrExit(err);

    if (inContext->mWDMOutput)
    {
        err = writer.EndContainer(kTLVType_Array);
        SuccessOrExit(err);

        err = writer.EndContainer(kTLVType_Structure);
        SuccessOrExit(err);
    }

    if (inContext->mRaw)
    {
        fwrite(backingStore, 1, writer.GetLengthWritten(), out);
    }
    else
    {
        reader.Init(backingStore, writer.GetLengthWritten());
        fprintf(out, "Wrote %d bytes to the log\n", writer.GetLengthWritten());
        nl::Weave::TLV::Utilities::Count(reader, elementCount);
        fprintf(out, "Fetched %lu elements, last eventID: %u \n", elementCount, eventId);
        nl::Weave::TLV::Debug::Dump(reader, SimpleDumpWriter);
    }
exit:
    if (err != WEAVE_NO_ERROR)
    {
        fprintf(stderr, "Error occurred: %d\n", err);
    }
    return;
}


void SimpleDebugLog(void *inContext)
{
    DebugEventGenerator generator;
    for (size_t i = 0; i < generator.GetNumStates(); i++)
    {
        generator.Generate();
        usleep(10000);
    }
    return;
}

void SimpleHeartbeatLog(void *inContext)
{
    LivenessEventGenerator generator;
    for (size_t i = 0; i < generator.GetNumStates(); i++)
    {
        generator.Generate();
        usleep(10000);
    }
    return;
}

void SimpleSecurityLog(void *inContext)
{

    useconds_t delays[] = { 10000,
                            10000,
                            10000,
                            5000,
                            5000,
                            10000,
                            100000,
                            10000,
                            10000,
                            10000,
                            10000,
                            5000,
                            1000,
                            1000,
                            1000,
                            1000
    };
    SecurityEventGenerator generator;

    for (size_t i = 0; i < generator.GetNumStates(); i++)
    {
        generator.Generate();
        usleep(delays[i]);
    }
    return;
}

typedef void (*LogGenerator)(void *);
const LogGenerator gTests [] =
{
    SimpleDebugLog,
    SimpleHeartbeatLog,
    SimpleSecurityLog
};

const size_t gNumTests = sizeof(gTests)/sizeof(void(*)(void *));

static OptionDef gToolOptionDefs[] =
{
    { "loglevel",   kArgumentRequired,  'l' },
    { "output",     kArgumentRequired,  'o' },
    { "raw",        kNoArgument,        'r' },
    { "test",       kArgumentRequired,  't' },
    { "verbose",    kNoArgument,        'V' },
    { "wdm",        kNoArgument,        'w' },
    { NULL }
};

static const char *gToolOptionHelp =
    "  -l, --loglevel <logLevel>\n"
    "       Configured default log level, 1 - PRODUCTION, 2 - INFO, 3 - DEBUG\n"
    "  -o, --output <filename>\n"
    "       Save the output in the file\n"
    "  -r, --raw\n"
    "       Emit raw bytes\n"
    "  -t, --test <num>\n"
    "       The test log to use, valid range: 1 to %d\n"
    "  -V, --verbose\n"
    "       Verbose output\n"
    "  -w, --wdm\n"
    "       Enclose the output in the WDM Notification envelope\n"
    ;

static OptionSet gToolOptions =
{
    HandleOption,
    gToolOptionDefs,
    "GENERAL OPTIONS",
    gToolOptionHelp
};

static HelpOptions gHelpOptions(
    TOOL_NAME,
    "Usage: " TOOL_NAME " [<options...>]\n",
    WEAVE_VERSION_STRING "\n" WEAVE_TOOL_COPYRIGHT,
    "Generate a sample event log showing various features of the event encoding.\n"
);

static OptionSet *gToolOptionSets[] =
{
    &gToolOptions,
    &gFaultInjectionOptions,
    &gHelpOptions,
    NULL
};

bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg)
{
    uint32_t level, testNum;

    switch (id)
    {
    case 'l':
        if (!ParseInt(arg, level) || level == 0 || level > 3)
        {
            PrintArgError("%s: Invalid value specified for logging level: %s\n", progName, arg);
            return false;
        }
        gLogContext.mLogLevel = static_cast<nl::Weave::Profiles::DataManagement::ImportanceType>(level);
        break;

    case 'o':
        gLogContext.mOutputFilename = arg;
        break;

    case 'r':
        gLogContext.mRaw = true;
        break;

    case 't':
        if (!ParseInt(arg, testNum) || testNum == 0 || testNum > gNumTests)
        {
            PrintArgError("%s: Invalid value specified for test number: %s\n", progName, arg);
            return false;
        }
        gLogContext.mTestNum = testNum - 1;
        break;

    case 'V':
        gLogContext.mVerbose = true;
        break;

    case 'w':
        gLogContext.mWDMOutput = true;
        break;

    default:
        PrintArgError("%s: INTERNAL ERROR: Unhandled option: %s\n", progName, name);
        return false;
    }

    return true;
}

int main(int argc, char *argv[])
{
    int status = 0;

    status = asprintf((char **)&gToolOptions.OptionHelp, gToolOptionHelp, gNumTests);
    VerifyOrExit(status >= 0, /* no-op */);

    if (!ParseArgsFromEnvVar(TOOL_NAME, TOOL_OPTIONS_ENV_VAR_NAME, gToolOptionSets, NULL, true) ||
        !ParseArgs(TOOL_NAME, argc, argv, gToolOptionSets))
    {
        exit(EXIT_FAILURE);
    }

    TestSetup(&gLogContext);

    InitializeEventLogging(&gLogContext);

    gTests[gLogContext.mTestNum](&gLogContext);

    DumpEventLog(&gLogContext);

    TestTeardown(&gLogContext);

 exit:
    return ((status >= 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}
