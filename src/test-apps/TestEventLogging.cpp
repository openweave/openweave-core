/*
 *
 *    Copyright (c) 2018 Google LLC.
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
 *      Unit tests for Weave Event Logging
 *
 */

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#include <new>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <nlbyteorder.h>
#include <nlunit-test.h>

// Note that the choice of namespace alias must be made up front for each and every compile unit
// This is because many include paths could set the default alias to unintended target.
#include <Weave/Profiles/bulk-data-transfer/Development/BDXManagedNamespace.hpp>
#include <Weave/Profiles/data-management/Current/WdmManagedNamespace.h>

#include "MockExternalEvents.h"
#include "ToolCommon.h"
#include "TestEventLoggingSchemaExamples.h"
#include <InetLayer/Inet.h>
#include <SystemLayer/SystemTimer.h>
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

#include <Weave/Support/TraitEventUtils.h>
#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Support/ErrorStr.h>

#include "TestPersistedStorageImplementation.h"
#include <Weave/Support/PersistedCounter.h>

#include "schema/nest/test/trait/TestETrait.h"
#include "schema/nest/test/trait/TestCommon.h"

#include "MockPlatformClocks.h"

using namespace nl::Weave::TLV;
using namespace nl::Weave::Profiles::DataManagement;

namespace Private {

static bool sRealTimeClockValid = true;

System::Error SetClock_RealTime(uint64_t newCurTime)
{
    if (newCurTime != 0)
    {
        sRealTimeClockValid = true;
    }
    else
    {
        sRealTimeClockValid = false;
    }

    return WEAVE_SYSTEM_NO_ERROR;
}

static System::Error GetClock_RealTime(uint64_t & curTime)
{
    if (sRealTimeClockValid)
    {
        curTime = ::nl::Weave::System::Platform::Layer::GetClock_Monotonic();
        return WEAVE_SYSTEM_NO_ERROR;
    }
    else
    {
        curTime = 0;
        return WEAVE_SYSTEM_ERROR_REAL_TIME_NOT_SYNCED;
    }
}

} // namespace Private

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

namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current) {

class TestSubscriptionHandler : public SubscriptionHandler
{
public:
    TestSubscriptionHandler(void);

    // Make methods from SubscriptionHandler public so we can call them from
    // our test cases.
    using SubscriptionHandler::CheckEventUpToDate;
    using SubscriptionHandler::FindNextImportanceForTransfer;
    using SubscriptionHandler::SetEventLogEndpoint;
    using SubscriptionHandler::ParsePathVersionEventLists;

    bool VerifyTraversingImportance(void);
    nl::Weave::Profiles::DataManagement::event_id_t & GetVendedEvent(nl::Weave::Profiles::DataManagement::ImportanceType inImportance);

    void SetActive(void) { mCurrentState = kState_Subscribing_Evaluating; }
    void SetAborted(void) { mCurrentState = kState_Aborted; }
    void SetEstablishedIdle(void) { mCurrentState = kState_SubscriptionEstablished_Idle; }
    void SetExchangeContext(nl::Weave::ExchangeContext *aEC) { mEC = aEC; }
private:
    /* important: this class must not add any members or declare virtual functions */
};


TestSubscriptionHandler::TestSubscriptionHandler(void)
{
    InitAsFree();
}

bool TestSubscriptionHandler::VerifyTraversingImportance(void)
{
    return FindNextImportanceForTransfer() == nl::Weave::Profiles::DataManagement::kImportanceType_Invalid;
}

nl::Weave::Profiles::DataManagement::event_id_t & TestSubscriptionHandler::GetVendedEvent(nl::Weave::Profiles::DataManagement::ImportanceType inImportance)
{
    return mSelfVendedEvents[inImportance - nl::Weave::Profiles::DataManagement::kImportanceType_First];
}

SubscriptionEngine * SubscriptionEngine::GetInstance()
{
    static SubscriptionEngine gWdmSubscriptionEngine;

    return &gWdmSubscriptionEngine;
}

} // WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current)
} // Profiles
} // Weave
} // nl

#define TOOL_NAME "TestDataLogging"

// forward declarations for networking functions.  They are
// implemented at the end of the file, as they are incidental to the
// test and are irrelevant unless the test is invoked in a mode that
// exercises the upload path (i.e. with a dest node ID, or a dest IP
// address.

struct TestLoggingContext;

static void PrepareBinding(TestLoggingContext *context);
static WEAVE_ERROR InitSubscriptionClient(TestLoggingContext *context);
static void HandleBindingEvent(void *const appState, const Binding::EventType event, const Binding::InEventParam &inParam, Binding::OutEventParam &outParam);
static void StartClientConnection(System::Layer *systemLayer, void *appState, System::Error err);
static void HandleConnectionComplete(WeaveConnection *con, WEAVE_ERROR conErr);
static void HandleConnectionClosed(WeaveConnection *con, WEAVE_ERROR conErr);
static WEAVE_ERROR FetchEventsHelper(TLVReader &aReader, event_id_t aEventId, uint8_t *aBackingStore, size_t aLen, ImportanceType aImportance = nl::Weave::Profiles::DataManagement::Production);

static bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg);

const uint64_t kTestNodeId = 0x18B43000002DCF71ULL;

// Globals used when the test is used in conjunction with BDX

WeaveConnection *Con = NULL;
bool WaitingForBDXResp = false;
bool Listening = false;
bool Upload = true; // download by default
bool Debug = false;
uint32_t ConnectInterval = 200;  //ms
uint32_t ConnectTry = 0;
uint32_t ConnectMaxTry = 3;
bool ClientConEstablished = false;
bool DestHostNameResolved = false;  // only used for UDP

static OptionDef gToolOptionDefs[] =
{
    { "start-event-id", kArgumentRequired,  's' },
    { "block-size",     kArgumentRequired,  'b' },
    { "dest-addr",      kArgumentRequired,  'D' },
    { "parent-node-id", kArgumentRequired,  'p' },
    { "debug",          kNoArgument,        'd' },
    { "tcp",            kNoArgument,        't' },
    { "udp",            kNoArgument,        'u' },
    { NULL }
};

static const char *const gToolOptionHelp =
    "  -p <num>, --parent-node-id <num> \n"
    "       Parent node id; the ID of the node that will receive the event\n"
    "       logs\n"
    "\n"
    "  -D <ip-addr>, --dest-addr <ip-addr>\n"
    "       The IP address or hostname of the parent (the node that will\n"
    "       receive thise event log)\n"
    "  -t, --tcp \n"
    "       Use TCP for BDX session\n"
    "\n"
    "  -u, --udp \n"
    "       Use UDP for BDX session\n"
    "\n"
    "  -s <num>, --start-event-id <num>\n"
    "       Begin the offload of each event sequence at <num> event\n"
    "\n"
    "  -b <num>, --block-size <num>\n"
    "       Block size to use for BDX upload.\n"
    "\n"
    "  -d, --debug \n"
    "       Enable debug messages.\n"
    "\n";

static OptionSet gToolOptions =
{
    HandleOption,
    gToolOptionDefs,
    "GENERAL OPTIONS",
    gToolOptionHelp
};

static HelpOptions gHelpOptions(
    TOOL_NAME,
    "Usage: " TOOL_NAME " [<options...>] <dest-node-id>[@<dest-host>[:<dest-port>][%<interface>]]\n"
    "       " TOOL_NAME " [<options...>] --listen\n",
    WEAVE_VERSION_STRING "\n" WEAVE_TOOL_COPYRIGHT,
    "Test event logging.  Without any options, the program invokes a\n"
    "suite of local log tests.  The options enable testing of a log\n"
    "upload over the BDX path.\n"
);

static OptionSet *gToolOptionSets[] =
{
    &gToolOptions,
    &gNetworkOptions,
    &gWeaveNodeOptions,
    &gFaultInjectionOptions,
    &gHelpOptions,
    NULL
};

struct BDXContext {
    uint64_t DestNodeId;
    IPAddress DestIPAddr;
    const char *DestIPAddrStr;
    uint32_t mStartingBlock;
    bool mUseTCP;
    bool mDone;
};

BDXContext gBDXContext;

// Event test harness contex

struct TestLoggingContext
{
    bool mVerbose;
    bool bdx;
    bool bdxDone;
    bool mReinitializeBDXUpload;
    WeaveExchangeManager *mExchangeMgr;
    Binding *mBinding;
    SubscriptionClient *mSubClient;
    TestLoggingContext();
};

TestLoggingContext gTestLoggingContext;

TestLoggingContext::TestLoggingContext() :
    mVerbose(false),
    bdx(false),
    bdxDone(false),
    mReinitializeBDXUpload(false),
    mExchangeMgr(NULL),
    mBinding(NULL),
    mSubClient(NULL)
{
}

LogBDXUpload gLogBDXUpload;

// Example profiles for logging:
#define OpenCloseProfileID 0x235A00AA
#define kOpenCloseStateTag 0x01
#define kBypassStateTag    0x02

enum OpenCloseStateEnum {
    Unknown = 0,
    Open = 1,
    PartiallyOpen = 2,
    Closed = 3,
};

enum BypassStateEnum {
    BypassInactive = 0,
    BypassActive = 1,
    BypassExpired = 2,
};

struct TestOpenCloseState
{
    TestOpenCloseState();
    void EvolveState();
    uint8_t mState;
    uint8_t mBypass;
};

TestOpenCloseState gTestOpenCloseState;

TestOpenCloseState::TestOpenCloseState()
{
    mState = Closed;
    mBypass = BypassInactive;
}

void TestOpenCloseState::EvolveState(void)
{
    if (mState == Closed)
    {
        mState = Open;
    }
    else
    {
        mState = Closed;
    }
}

const uint32_t gProfileList[] = { OpenCloseProfileID };

WEAVE_ERROR WriteOpenCloseState(nl::Weave::TLV::TLVWriter & writer, uint8_t inDataTag, void * anAppState)
{
    TestOpenCloseState * state = NULL;
    WEAVE_ERROR err =  WEAVE_NO_ERROR;
    nl::Weave::TLV::TLVType openCloseState;

    VerifyOrExit(anAppState != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);
    state = static_cast<TestOpenCloseState *>(anAppState);

    err = writer.StartContainer(ContextTag(nl::Weave::Profiles::DataManagement::kTag_EventData), nl::Weave::TLV::kTLVType_Structure, openCloseState);
    SuccessOrExit(err);

    err = writer.Put(nl::Weave::TLV::ContextTag(kOpenCloseStateTag), state->mState);
    SuccessOrExit(err);

    err = writer.Put(nl::Weave::TLV::ContextTag(kBypassStateTag), state->mBypass);
    SuccessOrExit(err);

    err = writer.EndContainer(openCloseState);
    SuccessOrExit(err);

    err = writer.Finalize();

    state->EvolveState();

exit:
    return err;
}

void SimpleDumpWriter(const char *aFormat, ...)
{
    va_list args;

    va_start(args, aFormat);

    vprintf(aFormat, args);

    va_end(args);
}


WEAVE_ERROR LogBufferConsole(void *inAppState, PacketBuffer* inBuffer)
{
    printf("Log entries:\nTime\tSchema\tEventData\n");
    nl::Weave::TLV::TLVReader reader;
    uint8_t * p = inBuffer->Start();
    uint32_t time = *((uint32_t*) p);
    uint16_t schema = *((uint16_t*) (p+4));

    inBuffer->SetStart(p+6);

    reader.Init(inBuffer, inBuffer->TotalLength());
    printf("%d\t%d\t", time, schema);
    nl::Weave::TLV::Debug::Dump(reader, SimpleDumpWriter);
    return WEAVE_NO_ERROR;

}

// Maximally sized event envelope
#define EVENT_ENVELOPE_SIZE 26
// Larger event payload. structured s.t. it fits in within the
// WEAVE_CONFIG_EVENT_SIZE_RESERVE (with the envelope)
#define EVENT_PAYLOAD_SIZE_1 128
// Larger event payload.  Structured s.t. it fits in the buffer, but
// it is larger than the WEAVE_CONFIG_SIZE_RESERVE
#define EVENT_PAYLOAD_SIZE_2 256
#define EVENT_SIZE_1 EVENT_PAYLOAD_SIZE_1 + EVENT_ENVELOPE_SIZE
// Larger event payload.  Doesn't fit in debug buffer.
#define EVENT_PAYLOAD_SIZE_3 (WEAVE_CONFIG_EVENT_SIZE_RESERVE + EVENT_SIZE_1)

uint64_t gDebugEventBuffer[(sizeof(nl::Weave::Profiles::DataManagement::CircularEventBuffer) + WEAVE_CONFIG_EVENT_SIZE_RESERVE + EVENT_SIZE_1 + 7)/8];
uint64_t gInfoEventBuffer[256];
uint64_t gProdEventBuffer[256];
uint64_t gCritEventBuffer[256];
uint8_t  gLargeMemoryBackingStore[16384];

static const uint32_t sEventIdCounterEpoch = 0x10000;

static const char *sCritEventIdCounterStorageKey = "CritEIDC";
static nl::Weave::PersistedCounter sCritEventIdCounter;
static const char *sProductionEventIdCounterStorageKey = "ProductionEIDC";
static nl::Weave::PersistedCounter sProductionEventIdCounter;
static const char *sInfoEventIdCounterStorageKey = "InfoEIDC";
static nl::Weave::PersistedCounter sInfoEventIdCounter;
static const char *sDebugEventIdCounterStorageKey = "DebugEIDC";
static nl::Weave::PersistedCounter sDebugEventIdCounter;

const char *sCounterKeys[kImportanceType_Last] = {
    sCritEventIdCounterStorageKey,
    sProductionEventIdCounterStorageKey,
    sInfoEventIdCounterStorageKey,
    sDebugEventIdCounterStorageKey,
};

const uint32_t sCounterEpochs[kImportanceType_Last] = {
    sEventIdCounterEpoch,
    sEventIdCounterEpoch,
    sEventIdCounterEpoch,
    sEventIdCounterEpoch,
};

PersistedCounter *sCounterStorage[kImportanceType_Last] = {
    &sCritEventIdCounter,
    &sProductionEventIdCounter,
    &sInfoEventIdCounter,
    &sDebugEventIdCounter,
};

void InitializeEventLogging(TestLoggingContext *context)
{

    size_t arraySizes[] = { sizeof(gDebugEventBuffer), sizeof(gInfoEventBuffer), sizeof(gProdEventBuffer) , sizeof(gCritEventBuffer) };

    void *arrays[] = {
        static_cast<void *>(&gDebugEventBuffer[0]),
        static_cast<void *>(&gInfoEventBuffer[0]),
        static_cast<void *>(&gProdEventBuffer[0]),
        static_cast<void *>(&gCritEventBuffer[0]) };

    nl::Weave::Profiles::DataManagement::LoggingManagement::CreateLoggingManagement(context->mExchangeMgr, sizeof(arrays)/sizeof(arrays[0]), &arraySizes[0], &arrays[0], NULL, NULL, NULL);
    nl::Weave::Profiles::DataManagement::LoggingManagement &instance = nl::Weave::Profiles::DataManagement::LoggingManagement::GetInstance();
    nl::Weave::Profiles::DataManagement::LoggingConfiguration::GetInstance().mGlobalImportance = nl::Weave::Profiles::DataManagement::Debug;
    new (&gLogBDXUpload)nl::Weave::Profiles::DataManagement::LogBDXUpload();
    gLogBDXUpload.Init(&instance);
}

void DestroyEventLogging(TestLoggingContext *context)
{
    nl::Weave::Profiles::DataManagement::LoggingManagement::GetInstance().DestroyLoggingManagement();

}

void InitializeEventLoggingWithPersistedCounters(TestLoggingContext *context, uint32_t startingValue, nl::Weave::Profiles::DataManagement::ImportanceType globalImportance)
{
    size_t arraySizes[] = { sizeof(gDebugEventBuffer), sizeof(gInfoEventBuffer), sizeof(gProdEventBuffer), sizeof(gCritEventBuffer[0]) };

    void *arrays[] = {
        static_cast<void *>(&gDebugEventBuffer[0]),
        static_cast<void *>(&gInfoEventBuffer[0]),
        static_cast<void *>(&gProdEventBuffer[0]),
        static_cast<void *>(&gCritEventBuffer[0]) };

    nl::Weave::Platform::PersistedStorage::Write(sCritEventIdCounterStorageKey, startingValue);
    nl::Weave::Platform::PersistedStorage::Write(sProductionEventIdCounterStorageKey, startingValue);
    nl::Weave::Platform::PersistedStorage::Write(sInfoEventIdCounterStorageKey, startingValue);
    nl::Weave::Platform::PersistedStorage::Write(sDebugEventIdCounterStorageKey, startingValue);

    nl::Weave::Profiles::DataManagement::LoggingManagement::CreateLoggingManagement(context->mExchangeMgr, sizeof(arrays)/sizeof(arrays[0]), &arraySizes[0], &arrays[0], sCounterKeys, sCounterEpochs, sCounterStorage);

    nl::Weave::Profiles::DataManagement::LoggingConfiguration::GetInstance().mGlobalImportance = globalImportance;
}

void DumpEventLog(nlTestSuite *inSuite)
{
    uint8_t backingStore[1024];
    size_t elementCount;
    event_id_t eventId = 1;
    TLVWriter writer;
    TLVReader reader;
    WEAVE_ERROR err;
    writer.Init(backingStore, 1024);

    err = nl::Weave::Profiles::DataManagement::LoggingManagement::GetInstance().FetchEventsSince(writer, nl::Weave::Profiles::DataManagement::Production, eventId);
    if (err == WEAVE_NO_ERROR || err == WEAVE_END_OF_TLV)
    {
        printf("Successfully wrote %u bytes to the log\n", writer.GetLengthWritten());
    }
    else
    {
        printf("Wrote %u bytes to the log, FetchEventsSince returned %s (%d)\n", writer.GetLengthWritten(), ErrorStr(err), err);
    }
    reader.Init(backingStore, 1024);
    nl::Weave::TLV::Utilities::Count(reader, elementCount);
    printf("Fetched %zu elements, last eventID: %u \n", elementCount, eventId);
    nl::Weave::TLV::Debug::Dump(reader, SimpleDumpWriter);
}

void DoBDXUpload(TestLoggingContext *context)
{
    if (! context->bdx )
    {
        return;
    }
    gBDXContext.mDone = false;
    if (gBDXContext.mUseTCP)
    {
        SystemLayer.StartTimer(ConnectInterval, StartClientConnection, &gBDXContext);
    }
    else
    {
        PrepareBinding(context);
    }

    while (!gBDXContext.mDone)
    {
        struct timeval sleepTime;
        sleepTime.tv_sec = 0;
        sleepTime.tv_usec = 100000;

        ServiceNetwork(sleepTime);
        if (gLogBDXUpload.mState == nl::Weave::Profiles::DataManagement::LogBDXUpload::UploaderInitialized)
        {
            gBDXContext.mDone = true;
            for (size_t i = 0; i < 1000; i++)
            {
                sleepTime.tv_sec = 0;
                sleepTime.tv_usec = 1000;

                ServiceNetwork(sleepTime);
            }
        }
    }

    gLogBDXUpload.Shutdown();

}


void PrintEventLog()
{
    TLVReader reader;
    size_t elementCount;
    nl::Weave::Profiles::DataManagement::LoggingManagement::GetInstance().GetEventReader(reader, nl::Weave::Profiles::DataManagement::Production);

    nl::Weave::TLV::Utilities::Count(reader, elementCount);
    printf("Found %lu elements\n", elementCount);
    nl::Weave::TLV::Debug::Dump(reader, SimpleDumpWriter);
}

static int TestSetup(void *inContext)
{
    TestLoggingContext *ctx = static_cast<TestLoggingContext *>(inContext);
    static WeaveFabricState sFabricState;
    static WeaveExchangeManager sExchangeMgr;

    InitSystemLayer();

    if (ctx->bdx)
    {
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
#if WEAVE_SYSTEM_CONFIG_USE_LWIP
        tcpip_init(NULL, NULL);
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

        err = sFabricState.Init();
        if (err != WEAVE_NO_ERROR)
            return FAILURE;

        sFabricState.LocalNodeId = kTestNodeId;
        sExchangeMgr.FabricState = & sFabricState;
        sExchangeMgr.State = WeaveExchangeManager::kState_Initialized;
        ctx->mExchangeMgr = &sExchangeMgr;
    }

    SubscriptionEngine::GetInstance()->Init(&ExchangeMgr, NULL, NULL);

    return SUCCESS;
}

static int TestTeardown(void *inContext)
{
    TestLoggingContext *ctx = static_cast<TestLoggingContext *>(inContext);
    if (ctx->bdx)
    {
        ShutdownWeaveStack();
        ShutdownNetwork();
    }

    ShutdownSystemLayer();
    return SUCCESS;
}

static void CheckLogState(nlTestSuite *inSuite,
                          TestLoggingContext *inContext,
                          nl::Weave::Profiles::DataManagement::LoggingManagement &logMgmt,
                          size_t expectedNumEvents)
{
    WEAVE_ERROR err;
    TLVReader reader;
    size_t elementCount;

    err = logMgmt.GetEventReader(reader, nl::Weave::Profiles::DataManagement::Production);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    err = nl::Weave::TLV::Utilities::Count(reader, elementCount, false);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    NL_TEST_ASSERT(inSuite, elementCount == expectedNumEvents);
    if (inContext->mVerbose)
    {
        printf("Num Events: %lu\n", elementCount);
    }
}

static void CheckLogReadOut(nlTestSuite *inSuite,
                            TestLoggingContext *inContext,
                            nl::Weave::Profiles::DataManagement::LoggingManagement &logMgmt,
                            nl::Weave::Profiles::DataManagement::ImportanceType importance,
                            event_id_t startingEventId,
                            size_t expectedNumEvents)
{
    WEAVE_ERROR err;
    TLVReader reader;
    TLVWriter writer;
    uint8_t backingStore[1024];
    size_t elementCount;
    writer.Init(backingStore, 1024);

    err = logMgmt.FetchEventsSince(writer, importance, startingEventId);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR || err == WEAVE_END_OF_TLV);

    reader.Init(backingStore, writer.GetLengthWritten());

    err = nl::Weave::TLV::Utilities::Count(reader, elementCount, false);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    NL_TEST_ASSERT(inSuite, elementCount == expectedNumEvents);

    if (inContext->mVerbose)
    {
        reader.Init(backingStore, writer.GetLengthWritten());
        printf("Starting Event ID: %u, Expected Events: %lu, Num Events: %lu, Num Bytes: %u\n", startingEventId, expectedNumEvents, elementCount, writer.GetLengthWritten());
        nl::Weave::TLV::Debug::Dump(reader, SimpleDumpWriter);
    }
}

static void CheckLogEventBasics(nlTestSuite *inSuite, void *inContext)
{
    TestLoggingContext *context = static_cast<TestLoggingContext *>(inContext);
    event_id_t eid1, eid2, eid3;
    EventSchema schema = {
        OpenCloseProfileID,
        1, // Event type 1
        nl::Weave::Profiles::DataManagement::Production,
        1,
        1
    };

    InitializeEventLogging(context);

    nl::Weave::Profiles::DataManagement::LoggingManagement &logMgmt = nl::Weave::Profiles::DataManagement::LoggingManagement::GetInstance();

    // Sample production events, spaced 10 milliseconds apart
    eid1 = nl::Weave::Profiles::DataManagement::LogEvent(
        schema,
        WriteOpenCloseState,
        static_cast<void*> (&gTestOpenCloseState)
        );
    CheckLogState(inSuite, context, logMgmt, 1);

    usleep(10000);
    eid2 = nl::Weave::Profiles::DataManagement::LogEvent(
        schema,
        WriteOpenCloseState,
        static_cast<void*> (&gTestOpenCloseState)
        );
    CheckLogState(inSuite, context, logMgmt, 2);

    usleep(10000);
    eid3 = nl::Weave::Profiles::DataManagement::LogEvent(
        schema,
        WriteOpenCloseState,
        static_cast<void*> (&gTestOpenCloseState)
        );
    CheckLogState(inSuite, context, logMgmt, 3);

    if (context->mVerbose)
    {
        PrintEventLog();
    }
    NL_TEST_ASSERT(inSuite, (eid1 + 1) == eid2);
    NL_TEST_ASSERT(inSuite, (eid2 + 1) == eid3);

    // Verify that the readout supports the expected volume of events
    CheckLogReadOut(inSuite, context, logMgmt, nl::Weave::Profiles::DataManagement::Production, eid1, 3);
    CheckLogReadOut(inSuite, context, logMgmt, nl::Weave::Profiles::DataManagement::Production, eid2, 2);
    CheckLogReadOut(inSuite, context, logMgmt, nl::Weave::Profiles::DataManagement::Production, eid3, 1);
    if (context->bdx)
    {
        DoBDXUpload(context);
    }
}

static void CheckLogFreeform(nlTestSuite *inSuite, void *inContext)
{
    TestLoggingContext *context = static_cast<TestLoggingContext *>(inContext);
    event_id_t eid1, eid2, eid3;
    size_t counter = 0;
    InitializeEventLogging(context);

    nl::Weave::Profiles::DataManagement::LoggingManagement &logMgmt = nl::Weave::Profiles::DataManagement::LoggingManagement::GetInstance();

    // Sample production events, spaced 10 milliseconds apart
    eid1 = nl::Weave::Profiles::DataManagement::LogFreeform(
        nl::Weave::Profiles::DataManagement::Production,
        "Freeform entry %d", counter++);

    CheckLogState(inSuite, context, logMgmt, 1);

    usleep(10000);

    eid2 = nl::Weave::Profiles::DataManagement::LogFreeform(
        nl::Weave::Profiles::DataManagement::Production,
        "Freeform entry %d", counter++);
    CheckLogState(inSuite, context, logMgmt, 2);

    usleep(10000);
    eid3 = nl::Weave::Profiles::DataManagement::LogFreeform(
        nl::Weave::Profiles::DataManagement::Production,
        "Freeform entry %d", counter++);
    CheckLogState(inSuite, context, logMgmt, 3);

    if (context->mVerbose)
    {
        PrintEventLog();
    }
    NL_TEST_ASSERT(inSuite, (eid1 + 1) == eid2);
    NL_TEST_ASSERT(inSuite, (eid2 + 1) == eid3);

    // Verify that the readout supports the expected volume of events
    CheckLogReadOut(inSuite, context, logMgmt, nl::Weave::Profiles::DataManagement::Production, eid1, 3);
    CheckLogReadOut(inSuite, context, logMgmt, nl::Weave::Profiles::DataManagement::Production, eid2, 2);
    CheckLogReadOut(inSuite, context, logMgmt, nl::Weave::Profiles::DataManagement::Production, eid3, 1);
    if (context->bdx)
    {
        DoBDXUpload(context);
    }
}

static void CheckLogPreformed(nlTestSuite *inSuite, void *inContext)
{
    TestLoggingContext *context = static_cast<TestLoggingContext *>(inContext);
    WEAVE_ERROR err;
    event_id_t eid1, eid2, eid3;
    EventSchema schema = {
        OpenCloseProfileID,
        2, // Event type 2
        nl::Weave::Profiles::DataManagement::Production,
        1,
        1
    };

    uint8_t backingStore[1024];
    TLVWriter writer;
    TLVType containerType;
    TLVReader reader;

    InitializeEventLogging(context);

    nl::Weave::Profiles::DataManagement::LoggingManagement &logMgmt = nl::Weave::Profiles::DataManagement::LoggingManagement::GetInstance();

    writer.Init(backingStore, 1024);
    err = writer.StartContainer(AnonymousTag, kTLVType_Structure, containerType);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    err = writer.Put(ContextTag(1), false);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    err = writer.Put(ContextTag(2), true);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    err = writer.EndContainer(containerType);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    err = writer.Finalize();
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);


    reader.Init(backingStore, writer.GetLengthWritten());

    // Sample production events, spaced 10 milliseconds apart
    eid1 = nl::Weave::Profiles::DataManagement::LogEvent(
        schema,
        reader
        );

    CheckLogState(inSuite, context, logMgmt, 1);

    usleep(10000);
    reader.Init(backingStore, writer.GetLengthWritten());
    eid2 = nl::Weave::Profiles::DataManagement::LogEvent(
        schema,
        reader
        );
    CheckLogState(inSuite, context, logMgmt, 2);

    usleep(10000);
    reader.Init(backingStore, writer.GetLengthWritten());
    eid3 =  nl::Weave::Profiles::DataManagement::LogEvent(
        schema,
        reader
        );
    CheckLogState(inSuite, context, logMgmt, 3);

    if (context->mVerbose)
    {
        PrintEventLog();
    }
    NL_TEST_ASSERT(inSuite, (eid1 + 1) == eid2);
    NL_TEST_ASSERT(inSuite, (eid2 + 1) == eid3);

    // Verify that the readout supports the expected volume of events
    CheckLogReadOut(inSuite, context, logMgmt, nl::Weave::Profiles::DataManagement::Production, eid1, 3);
    CheckLogReadOut(inSuite, context, logMgmt, nl::Weave::Profiles::DataManagement::Production, eid2, 2);
    CheckLogReadOut(inSuite, context, logMgmt, nl::Weave::Profiles::DataManagement::Production, eid3, 1);
    if (context->bdx)
    {
        DoBDXUpload(context);
    }
}

#define kSampleEventTag_State 1
#define kSampleEventTag_Timestamp 2
#define kSampleEventTag_Structure 3
#define kSampleEventTag_Samples 4

#define kEventStructTag_a 1
#define kEventStructTag_b 2

#define kEventStatsTag_str 1

#define kDataManagementTag_EventData 50

static const uint8_t SampleEventEncoding[] =
{
nlWeaveTLV_STRUCTURE(nlWeaveTLV_TAG_FULLY_QUALIFIED_6Bytes(0x0A00, 1)),
    nlWeaveTLV_STRUCTURE(nlWeaveTLV_TAG_CONTEXT_SPECIFIC(kDataManagementTag_EventData)),
        nlWeaveTLV_UINT8(nlWeaveTLV_TAG_CONTEXT_SPECIFIC(kSampleEventTag_State), 5),
        nlWeaveTLV_UINT16(nlWeaveTLV_TAG_CONTEXT_SPECIFIC(kSampleEventTag_Timestamp), 328),
        nlWeaveTLV_STRUCTURE(nlWeaveTLV_TAG_CONTEXT_SPECIFIC(kSampleEventTag_Structure)),
            nlWeaveTLV_BOOL(nlWeaveTLV_TAG_CONTEXT_SPECIFIC(kEventStructTag_a), true),
            nlWeaveTLV_STRUCTURE(nlWeaveTLV_TAG_CONTEXT_SPECIFIC(kEventStructTag_b)),
                nlWeaveTLV_UTF8_STRING_1ByteLength(nlWeaveTLV_TAG_CONTEXT_SPECIFIC(kEventStatsTag_str), 10),
                'b', 'l', 'o', 'o', 'p', 'b', 'l', 'o', 'o', 'p',
            nlWeaveTLV_END_OF_CONTAINER,
        nlWeaveTLV_END_OF_CONTAINER,
        nlWeaveTLV_ARRAY(nlWeaveTLV_TAG_CONTEXT_SPECIFIC(kSampleEventTag_Samples)),
            nlWeaveTLV_UINT8(nlWeaveTLV_TAG_ANONYMOUS, 0),
            nlWeaveTLV_UINT8(nlWeaveTLV_TAG_ANONYMOUS, 1),
            nlWeaveTLV_UINT8(nlWeaveTLV_TAG_ANONYMOUS, 2),
            nlWeaveTLV_UINT8(nlWeaveTLV_TAG_ANONYMOUS, 3),
            nlWeaveTLV_UINT8(nlWeaveTLV_TAG_ANONYMOUS, 4),
            nlWeaveTLV_UINT8(nlWeaveTLV_TAG_ANONYMOUS, 5),
        nlWeaveTLV_END_OF_CONTAINER,
    nlWeaveTLV_END_OF_CONTAINER,
nlWeaveTLV_END_OF_CONTAINER
};

static const uint8_t SampleEmptyArrayEventEncoding[] =
{
nlWeaveTLV_STRUCTURE(nlWeaveTLV_TAG_FULLY_QUALIFIED_6Bytes(0x0A00, 1)),
    nlWeaveTLV_STRUCTURE(nlWeaveTLV_TAG_CONTEXT_SPECIFIC(kDataManagementTag_EventData)),
        nlWeaveTLV_UINT8(nlWeaveTLV_TAG_CONTEXT_SPECIFIC(kSampleEventTag_State), 5),
        nlWeaveTLV_UINT16(nlWeaveTLV_TAG_CONTEXT_SPECIFIC(kSampleEventTag_Timestamp), 328),
        nlWeaveTLV_STRUCTURE(nlWeaveTLV_TAG_CONTEXT_SPECIFIC(kSampleEventTag_Structure)),
            nlWeaveTLV_BOOL(nlWeaveTLV_TAG_CONTEXT_SPECIFIC(kEventStructTag_a), true),
            nlWeaveTLV_STRUCTURE(nlWeaveTLV_TAG_CONTEXT_SPECIFIC(kEventStructTag_b)),
                nlWeaveTLV_UTF8_STRING_1ByteLength(nlWeaveTLV_TAG_CONTEXT_SPECIFIC(kEventStatsTag_str), 10),
                'b', 'l', 'o', 'o', 'p', 'b', 'l', 'o', 'o', 'p',
            nlWeaveTLV_END_OF_CONTAINER,
        nlWeaveTLV_END_OF_CONTAINER,
        nlWeaveTLV_ARRAY(nlWeaveTLV_TAG_CONTEXT_SPECIFIC(kSampleEventTag_Samples)),
        nlWeaveTLV_END_OF_CONTAINER,
    nlWeaveTLV_END_OF_CONTAINER,
nlWeaveTLV_END_OF_CONTAINER
};

static void CheckSchemaGeneratedLogging(nlTestSuite *inSuite, void *inContext)
{
    TestLoggingContext *context = static_cast<TestLoggingContext *>(inContext);
    event_id_t eid1, eid2;
    WEAVE_ERROR err;
    nl::Weave::Profiles::DataManagement::LoggingManagement &logMgmt = nl::Weave::Profiles::DataManagement::LoggingManagement::GetInstance();

    nl::Weave::Profiles::DataManagement::SampleTrait::Event ev;
    nl::Weave::Profiles::DataManagement::OpenCloseTrait::Event ev2;
    nl::StructureSchemaPointerPair appData;
    nl::Weave::TLV::TLVWriter outer, writer;
    uint8_t sBuffer[256];

    InitializeEventLogging(context);

    uint32_t samples[6] = { 0, 1, 2, 3, 4, 5 };
    ev.state = 5;
    ev.timestamp = 328;
    ev.structure.a = true;
    ev.structure.b.str = "bloopbloop\0";
    ev.samples.num_samples = 6;
    ev.samples.samples_buf = samples;

    appData.mStructureData = static_cast<void *>(&ev);
    appData.mFieldSchema = &sampleEventSchema;

    outer.Init(sBuffer, sizeof(sBuffer));

    err = outer.OpenContainer(ProfileTag(0x0A00, 1), kTLVType_Structure, writer);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    err = SerializedDataToTLVWriterHelper(writer, kTag_EventData, &appData);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    err = writer.Finalize();
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    err = outer.CloseContainer(writer);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    err = outer.Finalize();
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    // Verify the encoding

    NL_TEST_ASSERT(inSuite, outer.GetLengthWritten() == sizeof(SampleEventEncoding));
    NL_TEST_ASSERT(inSuite, memcmp(sBuffer, SampleEventEncoding, sizeof(SampleEventEncoding)) == 0);

    eid1 = LogSampleEvent(&ev, nl::Weave::Profiles::DataManagement::Production);
    CheckLogState(inSuite, context, logMgmt, 1);

    ev2.state = 1;
    eid2 = LogOpenCloseEvent(&ev2, nl::Weave::Profiles::DataManagement::Production);
    CheckLogState(inSuite, context, logMgmt, 2);

    CheckLogReadOut(inSuite, context, logMgmt, nl::Weave::Profiles::DataManagement::Production, eid1, 2);
    CheckLogReadOut(inSuite, context, logMgmt, nl::Weave::Profiles::DataManagement::Production, eid2, 1);

    if (context->bdx)
    {
        DoBDXUpload(context);
    }
}

static void CheckByteStringFieldType(nlTestSuite *inSuite, void *inContext)
{
    TestLoggingContext *context = static_cast<TestLoggingContext *>(inContext);
    WEAVE_ERROR err;
    event_id_t eventId;
    ByteStringTestTrait::Event ev, deserializedEv;
    nl::MemoryManagement memMgmt = { malloc, free, realloc };
    nl::SerializationContext serializationContext;
    serializationContext.memMgmt = memMgmt;

    TLVReader testReader;
    uint8_t buf[10];
    ev.byte_string.mLen = sizeof(buf);
    ev.byte_string.mBuf = buf;
    memset(buf, 0xaa, 10);

    InitializeEventLogging(context);

    eventId = LogByteStringTestEvent(&ev);

    err = FetchEventsHelper(testReader, eventId, gLargeMemoryBackingStore, sizeof(gLargeMemoryBackingStore));
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    err = DeserializeByteStringTestEvent(testReader, &deserializedEv, &serializationContext);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    NL_TEST_ASSERT(inSuite, deserializedEv.byte_string.mLen == ev.byte_string.mLen);
    NL_TEST_ASSERT(inSuite, memcmp(deserializedEv.byte_string.mBuf, ev.byte_string.mBuf, ev.byte_string.mLen) == 0);
}

static void CheckByteStringArray(nlTestSuite *inSuite, void *inContext)
{
    TestLoggingContext *context = static_cast<TestLoggingContext *>(inContext);
    WEAVE_ERROR err;
    event_id_t eventId;
    ByteStringArrayTestTrait::Event ev, deserializedEv;
    nl::MemoryManagement memMgmt = { malloc, free, realloc };
    nl::SerializationContext serializationContext;
    serializationContext.memMgmt = memMgmt;

    TLVReader testReader;
    nl::SerializedByteString bytestrings[5];
    uint8_t buf[100];
    int i;

    // some magic numbers to initialize some varied byte strings
    for (i = 0; i < 5; i++)
    {
        memset(&buf[i*5], (i+1)*40, (i+1)*5);
        bytestrings[i].mLen = (i+1)*5;
        bytestrings[i].mBuf = &buf[i*5];
    }
    ev.testArray.num = 5;
    ev.testArray.buf = bytestrings;

    InitializeEventLogging(context);

    eventId = LogByteStringArrayTestEvent(&ev);

    err = FetchEventsHelper(testReader, eventId, gLargeMemoryBackingStore, sizeof(gLargeMemoryBackingStore));
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    err = DeserializeByteStringArrayTestEvent(testReader, &deserializedEv, &serializationContext);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    NL_TEST_ASSERT(inSuite, deserializedEv.testArray.num == ev.testArray.num);
    for (i = 0; i < 5; i++)
    {
        NL_TEST_ASSERT(inSuite, deserializedEv.testArray.buf[i].mLen == ev.testArray.buf[i].mLen);
        NL_TEST_ASSERT(inSuite, memcmp(deserializedEv.testArray.buf[i].mBuf, ev.testArray.buf[i].mBuf, ev.testArray.buf[i].mLen) == 0);
    }
}

struct DebugLogContext
{
    const char *mRegion;
    const char *mFmt;
    va_list mArgs;
};

static event_id_t FastLogFreeform(ImportanceType inImportance, timestamp_t inTimestamp, const char * inFormat, ...)
{
    DebugLogContext context;
    nl::Weave::Profiles::DataManagement::EventOptions options;
    event_id_t eid;
    EventSchema schema = {
        kWeaveProfile_NestDebug,
        kNestDebug_StringLogEntryEvent,
        inImportance,
        1,
        1
    };

    va_start(context.mArgs, inFormat);
    context.mRegion = "";
    context.mFmt = inFormat;

    options = EventOptions(inTimestamp, NULL, 0, nl::Weave::Profiles::DataManagement::kImportanceType_Invalid, false);

    eid = LogEvent(schema, PlainTextWriter, &context, &options);

    va_end(context.mArgs);

    return eid;
}

static void CheckEvict(nlTestSuite *inSuite, void *inContext)
{
    TestLoggingContext *context = static_cast<TestLoggingContext *>(inContext);
    event_id_t eid_prev, eid;
    size_t counter = 0;
    timestamp_t now;
    InitializeEventLogging(context);

    now = System::Layer::GetClock_MonotonicMS();
    eid_prev = FastLogFreeform(
        nl::Weave::Profiles::DataManagement::Production,
        now,
        "Freeform entry %d", counter);
    now += 10;
    for (counter = 0; counter < 100; counter++) {
    // Sample production events, spaced 10 milliseconds apart
        eid = FastLogFreeform(
            nl::Weave::Profiles::DataManagement::Production,
            now,
            "Freeform entry %d", counter);
        now +=10;

        NL_TEST_ASSERT(inSuite, eid > 0);
        NL_TEST_ASSERT(inSuite, eid == (eid_prev+1));

        eid_prev = eid;
    }
    if (context->bdx)
    {
        DoBDXUpload(context);
    }
}

static WEAVE_ERROR ReadFirstEventHeader(TLVReader &aReader, timestamp_t &aTimestamp, utc_timestamp_t &aUtcTimestamp, event_id_t &aEventId)
{
    WEAVE_ERROR err;
    TLVType readerType;
    uint64_t currentContextTag;

    err = aReader.Next();
    SuccessOrExit(err);

    err = aReader.EnterContainer(readerType);
    SuccessOrExit(err);

    currentContextTag = aReader.GetTag();

    while ((currentContextTag != ContextTag(kTag_EventData)) && !err)
    {
        if (currentContextTag == ContextTag(nl::Weave::Profiles::DataManagement::kTag_EventSystemTimestamp))
        {
            err = aReader.Get(aTimestamp);
            SuccessOrExit(err);
        }

        if (currentContextTag == ContextTag(nl::Weave::Profiles::DataManagement::kTag_EventUTCTimestamp))
        {
            err = aReader.Get(aUtcTimestamp);
            SuccessOrExit(err);
        }

        if (currentContextTag == ContextTag(nl::Weave::Profiles::DataManagement::kTag_EventID))
        {
            err = aReader.Get(aEventId);
            SuccessOrExit(err);
        }

        err = aReader.Next();
        SuccessOrExit(err);

        currentContextTag = aReader.GetTag();
    }

    err = aReader.ExitContainer(readerType);

exit:
    return err;
}

static void CheckFetchTimestamps(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err;
    TestLoggingContext *context = static_cast<TestLoggingContext *>(inContext);
    event_id_t eid_prev, eid, event_id_read;
    size_t counter = 0;
    const int k_num_events = 10;
    timestamp_t now;
    utc_timestamp_t test_start;
    InitializeEventLogging(context);

    test_start = static_cast<utc_timestamp_t>(System::Layer::GetClock_MonotonicMS());
    now = static_cast<timestamp_t>(test_start);
    System::Layer::SetClock_RealTime(0);

    eid_prev = FastLogFreeform(
        nl::Weave::Profiles::DataManagement::Production,
        now,
        "%u", now);
    eid_prev = FastLogFreeform(
        nl::Weave::Profiles::DataManagement::Info,
        now,
        "%u", now);
    now += 10;
    for (counter = 1; counter < k_num_events; counter++) {
    // Sample production events, spaced 10 milliseconds apart
        if (counter == k_num_events/2)
        {
            System::Layer::SetClock_RealTime((test_start)*1000);
        }

        eid = FastLogFreeform(
            nl::Weave::Profiles::DataManagement::Info,
            now,
            "%u", now);
        NL_TEST_ASSERT(inSuite, eid > 0);
        eid = FastLogFreeform(
            nl::Weave::Profiles::DataManagement::Production,
            now,
            "%u", now);

        NL_TEST_ASSERT(inSuite, eid > 0);
        NL_TEST_ASSERT(inSuite, eid == (eid_prev+1));

        now += 10;
        eid_prev = eid;
    }

    NL_TEST_ASSERT(inSuite, eid_prev == k_num_events - 1);

    for (counter = 0; counter <= eid_prev; counter++)
    {
        TLVReader testReader;
        TLVWriter testWriter;
        utc_timestamp_t testUtcTimestamp = 0;
        timestamp_t testTimestamp = 0;
        event_id_t testEventID = 0;

        event_id_read = counter;
        testWriter.Init(gLargeMemoryBackingStore, sizeof(gLargeMemoryBackingStore));
        err = nl::Weave::Profiles::DataManagement::LoggingManagement::GetInstance().FetchEventsSince(testWriter, nl::Weave::Profiles::DataManagement::Info, event_id_read);
        NL_TEST_ASSERT(inSuite, err == WEAVE_END_OF_TLV);
        NL_TEST_ASSERT(inSuite, event_id_read == eid_prev + 1);

        if (context->mVerbose)
        {
            TLVReader reader;
            reader.Init(gLargeMemoryBackingStore, testWriter.GetLengthWritten());

            nl::Weave::TLV::Debug::Dump(reader, SimpleDumpWriter);
        }

        testReader.Init(gLargeMemoryBackingStore, testWriter.GetLengthWritten());

        err = ReadFirstEventHeader(testReader, testTimestamp, testUtcTimestamp, testEventID);
        NL_TEST_ASSERT(inSuite, testEventID == counter);
        NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
#if WEAVE_CONFIG_EVENT_LOGGING_UTC_TIMESTAMPS
        if (counter >= k_num_events/2)
        {
            NL_TEST_ASSERT(inSuite, testUtcTimestamp == test_start + (testEventID) * 10);
        }
        else
#endif // WEAVE_CONFIG_EVENT_LOGGING_UTC_TIMESTAMPS
        {
            NL_TEST_ASSERT(inSuite, testTimestamp == static_cast<timestamp_t>(test_start) + (testEventID) * 10);
        }
    }
}

WEAVE_ERROR WriteLargeEvent(nl::Weave::TLV::TLVWriter & writer, uint8_t inDataTag, void * anAppState)
{
    WEAVE_ERROR err =  WEAVE_NO_ERROR;
    nl::Weave::TLV::TLVType containerType;
    uint32_t *payloadEventSize;
    uint8_t *dummyPayload = NULL;

    VerifyOrExit(anAppState  != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);
    payloadEventSize = static_cast<uint32_t *>(anAppState);

    dummyPayload = reinterpret_cast<uint8_t*>(malloc(*payloadEventSize));
    VerifyOrExit(dummyPayload != NULL, err = WEAVE_ERROR_NO_MEMORY);

    memset(dummyPayload, 0xa5, *payloadEventSize);

    err = writer.StartContainer(ContextTag(nl::Weave::Profiles::DataManagement::kTag_EventData), nl::Weave::TLV::kTLVType_Structure, containerType);
    SuccessOrExit(err);

    err = writer.PutBytes(nl::Weave::TLV::ContextTag(1), dummyPayload, *payloadEventSize);
    SuccessOrExit(err);

    err = writer.EndContainer(containerType);
    SuccessOrExit(err);

    err = writer.Finalize();

exit:
    if (dummyPayload != NULL)
    {
        free(dummyPayload);
    }
    return err;
}

static void CheckLargeEvents(nlTestSuite *inSuite, void *inContext)
{
    TestLoggingContext *context = static_cast<TestLoggingContext *>(inContext);
    uint32_t payloadSize;
    event_id_t eid1, eid2, eid3, eid4;
    EventSchema schema = {
        OpenCloseProfileID,
        1, // Event type 1
        nl::Weave::Profiles::DataManagement::Production,
        1,
        1
    };

    InitializeEventLogging(context);

    nl::Weave::Profiles::DataManagement::LoggingManagement &logMgmt = nl::Weave::Profiles::DataManagement::LoggingManagement::GetInstance();

    // we expect this payload to succeed
    payloadSize = EVENT_PAYLOAD_SIZE_1;
    eid1 = nl::Weave::Profiles::DataManagement::LogEvent(
        schema,
        WriteLargeEvent,
        static_cast<void*> (&payloadSize)
        );
    NL_TEST_ASSERT(inSuite, eid1 == 0);

    eid2 = nl::Weave::Profiles::DataManagement::LogEvent(
        schema,
        WriteLargeEvent,
        static_cast<void*> (&payloadSize)
        );
    NL_TEST_ASSERT(inSuite, eid2 == 1);
    CheckLogState(inSuite, context, logMgmt, 2);

    // new test case - events will get retried if they fail
    payloadSize = EVENT_PAYLOAD_SIZE_2;
    eid3 = nl::Weave::Profiles::DataManagement::LogEvent(
        schema,
        WriteLargeEvent,
        static_cast<void*> (&payloadSize)
        );
    NL_TEST_ASSERT(inSuite, eid3 == 2);

    // this event is wider than the debug buffer
    payloadSize = EVENT_PAYLOAD_SIZE_3;
    eid4 = nl::Weave::Profiles::DataManagement::LogEvent(
        schema,
        WriteLargeEvent,
        static_cast<void*> (&payloadSize)
        );
    NL_TEST_ASSERT(inSuite, eid4 == 0);
}

static void CheckDropEvents(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err;
    int counter = 0;
    TestLoggingContext *context = static_cast<TestLoggingContext *>(inContext);
    EventSchema schema = {
        OpenCloseProfileID,
        1, // Event type 1
        nl::Weave::Profiles::DataManagement::Production,
        1,
        1
    };
    event_id_t eid_prev, eid;
    CircularEventBuffer *prodBuf = reinterpret_cast<CircularEventBuffer *>(&gProdEventBuffer[0]);
    uint32_t eventSizes[] = {
        EVENT_ENVELOPE_SIZE,
        EVENT_PAYLOAD_SIZE_1,
        EVENT_PAYLOAD_SIZE_2,
    };
    const uint32_t numSizes = sizeof(eventSizes)/sizeof(uint32_t);
    TLVWriter testWriter;

    InitializeEventLogging(context);

    nl::Weave::Profiles::DataManagement::LoggingManagement &logMgmt = nl::Weave::Profiles::DataManagement::LoggingManagement::GetInstance();

    // register some fake events
    err = LogMockExternalEvents(10, 1);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    eid_prev = prodBuf->mLastEventID;

    while (prodBuf->mFirstEventID <= 10)
    {
        eid = nl::Weave::Profiles::DataManagement::LogEvent(
            schema,
            WriteLargeEvent,
            static_cast<void*> (&eventSizes[counter++ % numSizes])
            );
        NL_TEST_ASSERT(inSuite, eid > eid_prev);

        if (eid_prev >= 10)
        {
            testWriter.Init(gLargeMemoryBackingStore, sizeof(gLargeMemoryBackingStore));
            err = logMgmt.FetchEventsSince(testWriter, nl::Weave::Profiles::DataManagement::Production, eid_prev);
            NL_TEST_ASSERT(inSuite, err == WEAVE_END_OF_TLV);
            NL_TEST_ASSERT(inSuite, eid_prev == eid + 1);
        }

        eid_prev = eid;
    }

    {
        TLVReader testReader;
        event_id_t testEventID, eid_in = 0;
        timestamp_t testTimestamp;
        utc_timestamp_t testUtcTimestamp;

        testWriter.Init(gLargeMemoryBackingStore, sizeof(gLargeMemoryBackingStore));
        err = logMgmt.FetchEventsSince(testWriter, nl::Weave::Profiles::DataManagement::Production, eid_in);
        NL_TEST_ASSERT(inSuite, err == WEAVE_END_OF_TLV);
        NL_TEST_ASSERT(inSuite, eid_in > 10);

        testReader.Init(gLargeMemoryBackingStore, testWriter.GetLengthWritten());
        err = ReadFirstEventHeader(testReader, testTimestamp, testUtcTimestamp, testEventID);
        NL_TEST_ASSERT(inSuite, testEventID >= 10);
    }
}

static void CheckFetchEvents(nlTestSuite *inSuite, void *inContext)
{
    TestLoggingContext *context = static_cast<TestLoggingContext *>(inContext);
    event_id_t eid_prev, eid, eventId;
    size_t counter = 0;
    // small buffer, sized s.t. the events generated below will be
    // larger than a single buffer, but smaller than two buffers.
    uint8_t smallMemoryBackingStore[1280];
    PacketBuffer *pbuf = PacketBuffer::New();
    TLVWriter testWriter;
    WEAVE_ERROR err;
    timestamp_t now;
    InitializeEventLogging(context);
    now = static_cast<timestamp_t>(0);

    eid_prev = FastLogFreeform(
        nl::Weave::Profiles::DataManagement::Production,
        now,
        "Freeform entry %d", counter);

    // The magic number "40" below is selected to be large enough to
    // generate more events than can fit in a single PacketBuffer, but
    // fewer than can fit in 2 packet buffers.  This ensures that we
    // test both the cases when we run out of log before ending the
    // buffer, and the cases when the writer runs out of space before
    // the end of the log.
    now += 10;
    for (counter = 0; counter < 40; counter++) {
    // Sample production events, spaced 10 milliseconds apart
        eid = FastLogFreeform(
            nl::Weave::Profiles::DataManagement::Production,
            now,
            "Freeform entry %d", counter);
        now += 10;

        NL_TEST_ASSERT(inSuite, eid > 0);
        NL_TEST_ASSERT(inSuite, eid == (eid_prev+1));

        eid_prev = eid;
    }

    if (context->mVerbose)
    {
        PrintEventLog();
    }

    // Test that offloading events into large buffer completes and
    // returns WEAVE_END_OF_TLV
    eventId = 0;
    testWriter.Init(gLargeMemoryBackingStore, sizeof(gLargeMemoryBackingStore));
    err = nl::Weave::Profiles::DataManagement::LoggingManagement::GetInstance().FetchEventsSince(testWriter, nl::Weave::Profiles::DataManagement::Production, eventId);
    NL_TEST_ASSERT(inSuite, err == WEAVE_END_OF_TLV);

    // Test that offloading events into a smaller buffer with bounded
    // write length results in WEAVE_ERROR_BUFFER_TOO_SMALL and the
    // correct number of events as indicated by eventId

    eventId = 0;
    eid_prev = eventId;
    testWriter.Init(smallMemoryBackingStore, sizeof(smallMemoryBackingStore));
    err = nl::Weave::Profiles::DataManagement::LoggingManagement::GetInstance().FetchEventsSince(testWriter, nl::Weave::Profiles::DataManagement::Production, eventId);
    NL_TEST_ASSERT(inSuite, err == WEAVE_ERROR_BUFFER_TOO_SMALL);
    {
        TLVReader reader;
        size_t eventCount;
        reader.Init(smallMemoryBackingStore, testWriter.GetLengthWritten());

        err = nl::Weave::TLV::Utilities::Count(reader, eventCount, false);
        NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
        NL_TEST_ASSERT(inSuite, eventId - eid_prev == eventCount);
    }

    // resume event offload; this one should reach the end of the
    // log (by construction)
    testWriter.Init(smallMemoryBackingStore, sizeof(smallMemoryBackingStore));
    err = nl::Weave::Profiles::DataManagement::LoggingManagement::GetInstance().FetchEventsSince(testWriter, nl::Weave::Profiles::DataManagement::Production, eventId);
    NL_TEST_ASSERT(inSuite, err == WEAVE_END_OF_TLV);

    // Test that offloading events into a PacketBuffer-backed writer with the default (unbounded) max write length results in WEAVE_ERROR_NO_MEMORY
    eventId = 0;
    testWriter.Init(pbuf);
    err = nl::Weave::Profiles::DataManagement::LoggingManagement::GetInstance().FetchEventsSince(testWriter, nl::Weave::Profiles::DataManagement::Production, eventId);
    NL_TEST_ASSERT(inSuite, err == WEAVE_ERROR_NO_MEMORY);

    PacketBuffer::Free(pbuf);
    pbuf = PacketBuffer::New();
    testWriter.Init(pbuf);
    err = nl::Weave::Profiles::DataManagement::LoggingManagement::GetInstance().FetchEventsSince(testWriter, nl::Weave::Profiles::DataManagement::Production, eventId);
    NL_TEST_ASSERT(inSuite, err == WEAVE_END_OF_TLV);

    if (context->bdx)
    {
        DoBDXUpload(context);
    }
}

static void CheckBasicEventDeserialization(nlTestSuite *inSuite, void *inContext)
{
    TestLoggingContext *context = static_cast<TestLoggingContext *>(inContext);
    WEAVE_ERROR err;

    nl::Weave::Profiles::DataManagement::SampleTrait::Event ev, ev2;
    nl::StructureSchemaPointerPair appData;
    nl::Weave::TLV::TLVWriter outer, writer;
    nl::Weave::TLV::TLVReader reader, outerReader;
    uint8_t sBuffer[256];
    nl::MemoryManagement memMgmt = { malloc, free, realloc };
    nl::SerializationContext serializationContext;
    serializationContext.memMgmt = memMgmt;

    InitializeEventLogging(context);

    uint32_t samples[6] = { 0, 1, 2, 3, 4, 5 };
    ev.state = 5;
    ev.timestamp = 328;
    ev.structure.a = true;
    ev.structure.b.str = "bloopbloop\0";
    ev.samples.num_samples = 6;
    ev.samples.samples_buf = samples;

    appData.mStructureData = static_cast<void *>(&ev);
    appData.mFieldSchema = &sampleEventSchema;

    outer.Init(sBuffer, sizeof(sBuffer));

    err = outer.OpenContainer(ProfileTag(0x0A00, 1), kTLVType_Structure, writer);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    err = SerializedDataToTLVWriterHelper(writer, kTag_EventData, &appData);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    err = writer.Finalize();
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    err = outer.CloseContainer(writer);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    err = outer.Finalize();
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    // Verify the encoding

    NL_TEST_ASSERT(inSuite, outer.GetLengthWritten() == sizeof(SampleEventEncoding));
    NL_TEST_ASSERT(inSuite, memcmp(sBuffer, SampleEventEncoding, sizeof(SampleEventEncoding)) == 0);

    // Now de-serialize.

    outerReader.Init(sBuffer, outer.GetLengthWritten());
    err = outerReader.Next(); // Positions us at the beginning of the first element.
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    appData.mStructureData = static_cast<void *>(&ev2);
    appData.mFieldSchema = &sampleEventSchema;

    err = outerReader.OpenContainer(reader);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    err = reader.Next(); // Positions us at the beginning of the first element.
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    err = nl::Weave::Profiles::DataManagement::DeserializeSampleEvent(reader, &ev2, &serializationContext);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    err = outerReader.CloseContainer(reader);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    NL_TEST_ASSERT(inSuite, ev2.state == ev.state);
    NL_TEST_ASSERT(inSuite, ev2.timestamp == ev.timestamp);
    NL_TEST_ASSERT(inSuite, ev2.structure.a == ev.structure.a);
    NL_TEST_ASSERT(inSuite, strcmp(ev2.structure.b.str, ev.structure.b.str) == 0);
    NL_TEST_ASSERT(inSuite, ev2.samples.num_samples == ev.samples.num_samples);
    for (uint32_t i = 0; i < ev2.samples.num_samples; i++)
    {
        NL_TEST_ASSERT(inSuite, ev2.samples.samples_buf[i] == ev.samples.samples_buf[i]);
    }

    nl::DeallocateDeserializedStructure(&ev2, &sampleEventSchema, &serializationContext);
}

static void CheckComplexEventDeserialization(nlTestSuite *inSuite, void *inContext)
{
    TestLoggingContext *context = static_cast<TestLoggingContext *>(inContext);
    WEAVE_ERROR err;

    Schema::Nest::Test::Trait::TestETrait::TestEEvent ev = { 0 };
    Schema::Nest::Test::Trait::TestETrait::TestEEvent ev2 = { 0 };
    nl::StructureSchemaPointerPair appData;
    nl::Weave::TLV::TLVWriter outer, writer;
    nl::Weave::TLV::TLVReader reader, outerReader;
    uint8_t sBuffer[512];
    nl::MemoryManagement memMgmt = { malloc, free, realloc };
    nl::SerializationContext serializationContext;
    serializationContext.memMgmt = memMgmt;

    InitializeEventLogging(context);

    memset(&ev, 0, sizeof(ev));

    uint32_t numbaz[5];
    numbaz[0] = 1;
    numbaz[1] = 3;
    numbaz[2] = 5;
    numbaz[3] = 7;
    numbaz[4] = 10;
    Schema::Nest::Test::Trait::TestCommon::CommonStructE strukchaz[3];
    strukchaz[0].seA = 1111111;
    strukchaz[0].seB = true;
    strukchaz[1].seA = 2222222;
    strukchaz[1].seB = false;
    strukchaz[2].seA = 3333333;
    strukchaz[2].seB = true;
    ev.teA = 444444;
    ev.teB = -555555;
    ev.teC = true;
    ev.teD = -666666;
    ev.teE.seA = 777777;
    ev.teE.seB = false;
    ev.teE.seC = -888888;
    ev.teF = 999999;
    ev.teG.seA = 101010;
    ev.teG.seB = true;
    ev.teH.num = sizeof(numbaz)/sizeof(numbaz[0]);
    ev.teH.buf = numbaz;
    ev.teI.num = sizeof(strukchaz)/sizeof(strukchaz[0]);
    ev.teI.buf = strukchaz;
    ev.teJ = 12121;

    appData.mStructureData = static_cast<void *>(&ev);
    appData.mFieldSchema = &Schema::Nest::Test::Trait::TestETrait::TestEEvent::FieldSchema;

    outer.Init(sBuffer, sizeof(sBuffer));

    err = outer.OpenContainer(ProfileTag(0x0A00, 1), kTLVType_Structure, writer);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    err = SerializedDataToTLVWriterHelper(writer, kTag_EventData, &appData);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    err = writer.Finalize();
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    err = outer.CloseContainer(writer);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    err = outer.Finalize();
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    // Now de-serialize.

    outerReader.Init(sBuffer, outer.GetLengthWritten());

    err = outerReader.Next(); // Positions us at the beginning of the first element.
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    appData.mStructureData = static_cast<void *>(&ev2);
    appData.mFieldSchema = &Schema::Nest::Test::Trait::TestETrait::TestEEvent::FieldSchema;

    err = outerReader.OpenContainer(reader);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    err = reader.Next(); // Positions us at the beginning of the first element.
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    err = nl::DeserializeEvent(reader, &ev2, &serializationContext);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    SuccessOrExit(err);

    err = outerReader.CloseContainer(reader);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    NL_TEST_ASSERT(inSuite, ev2.teA == ev.teA);
    NL_TEST_ASSERT(inSuite, ev2.teB == ev.teB);
    NL_TEST_ASSERT(inSuite, ev2.teC == ev.teC);
    NL_TEST_ASSERT(inSuite, ev2.teD == ev.teD);
    NL_TEST_ASSERT(inSuite, ev2.teE.seA == ev.teE.seA);
    NL_TEST_ASSERT(inSuite, ev2.teE.seB == ev.teE.seB);
    NL_TEST_ASSERT(inSuite, ev2.teE.seC == ev.teE.seC);
    NL_TEST_ASSERT(inSuite, ev2.teF == ev.teF);
    NL_TEST_ASSERT(inSuite, ev2.teG.seA == ev.teG.seA);
    NL_TEST_ASSERT(inSuite, ev2.teG.seB == ev.teG.seB);
    for (uint32_t i = 0; i < ev2.teH.num; i++)
    {
        NL_TEST_ASSERT(inSuite, ev2.teH.buf[i] == ev.teH.buf[i]);
    }
    for (uint32_t i = 0; i < ev2.teI.num; i++)
    {
        NL_TEST_ASSERT(inSuite, ev2.teI.buf[i].seA == ev.teI.buf[i].seA);
        NL_TEST_ASSERT(inSuite, ev2.teI.buf[i].seB == ev.teI.buf[i].seB);
    }
    NL_TEST_ASSERT(inSuite, ev2.IsTeJPresent());
    NL_TEST_ASSERT(inSuite, ev2.teJ == ev.teJ);

    nl::DeallocateEvent(&ev2, &serializationContext);

exit:
    return;
}

static void CheckEmptyArrayEventDeserialization(nlTestSuite *inSuite, void *inContext)
{
    TestLoggingContext *context = static_cast<TestLoggingContext *>(inContext);
    WEAVE_ERROR err;

    nl::Weave::Profiles::DataManagement::SampleTrait::Event ev, ev2;
    nl::StructureSchemaPointerPair appData;
    nl::Weave::TLV::TLVWriter outer, writer;
    nl::Weave::TLV::TLVReader reader, outerReader;
    uint8_t sBuffer[256];
    nl::MemoryManagement memMgmt = { malloc, free, realloc };
    nl::SerializationContext serializationContext;
    serializationContext.memMgmt = memMgmt;

    InitializeEventLogging(context);

    ev.state = 5;
    ev.timestamp = 328;
    ev.structure.a = true;
    ev.structure.b.str = "bloopbloop\0";
    ev.samples.num_samples = 0;
    ev.samples.samples_buf = NULL;

    appData.mStructureData = static_cast<void *>(&ev);
    appData.mFieldSchema = &sampleEventSchema;

    outer.Init(sBuffer, sizeof(sBuffer));

    err = outer.OpenContainer(ProfileTag(0x0A00, 1), kTLVType_Structure, writer);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    err = SerializedDataToTLVWriterHelper(writer, kTag_EventData, &appData);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    err = writer.Finalize();
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    err = outer.CloseContainer(writer);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    err = outer.Finalize();
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    // Verify the encoding

    NL_TEST_ASSERT(inSuite, outer.GetLengthWritten() == sizeof(SampleEmptyArrayEventEncoding));
    NL_TEST_ASSERT(inSuite, memcmp(sBuffer, SampleEmptyArrayEventEncoding, sizeof(SampleEmptyArrayEventEncoding)) == 0);

    // Now de-serialize.

    outerReader.Init(sBuffer, outer.GetLengthWritten());

    err = outerReader.Next(); // Positions us at the beginning of the first element.
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    appData.mStructureData = static_cast<void *>(&ev2);
    appData.mFieldSchema = &sampleEventSchema;

    err = outerReader.OpenContainer(reader);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    err = reader.Next(); // Positions us at the beginning of the first element.
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    err = nl::Weave::Profiles::DataManagement::DeserializeSampleEvent(reader, &ev2, &serializationContext);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    err = outerReader.CloseContainer(reader);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    NL_TEST_ASSERT(inSuite, ev2.state == ev.state);
    NL_TEST_ASSERT(inSuite, ev2.timestamp == ev.timestamp);
    NL_TEST_ASSERT(inSuite, ev2.structure.a == ev.structure.a);
    NL_TEST_ASSERT(inSuite, strcmp(ev2.structure.b.str, ev.structure.b.str) == 0);
    NL_TEST_ASSERT(inSuite, ev2.samples.num_samples == ev.samples.num_samples);
    NL_TEST_ASSERT(inSuite, ev2.samples.samples_buf == NULL);

    memMgmt.mem_free((void *)ev2.structure.b.str);
}

static WEAVE_ERROR FetchEventsHelper(TLVReader &aReader, event_id_t aEventId, uint8_t *aBackingStore, size_t aLen, ImportanceType aImportance)
{
    WEAVE_ERROR err;
    TLVWriter testWriter;
    TLVType readerType;

    testWriter.Init(aBackingStore, aLen);
    err = nl::Weave::Profiles::DataManagement::LoggingManagement::GetInstance().FetchEventsSince(testWriter, aImportance, aEventId);
    VerifyOrExit(err == WEAVE_END_OF_TLV, err = WEAVE_ERROR_INCORRECT_STATE);

    aReader.Init(aBackingStore, testWriter.GetLengthWritten());

    err = aReader.Next();
    SuccessOrExit(err);

    err = aReader.EnterContainer(readerType);
    SuccessOrExit(err);

    while ((aReader.GetTag() != ContextTag(kTag_EventData)) && (err == WEAVE_NO_ERROR))
    {
        err = aReader.Next();
    }

exit:
    return err;
}

class TestEventProcessor : public EventProcessor {
public:
    TestEventProcessor();
    WEAVE_ERROR ProcessEvent(TLVReader inReader, SubscriptionClient &inClient, const EventHeader &inEventHeader);
    WEAVE_ERROR GapDetected(const EventHeader &inEventHeader);

    SchemaVersionRange mSchemaVersionRange;
};

TestEventProcessor::TestEventProcessor()
    : EventProcessor(0)
{
}

WEAVE_ERROR TestEventProcessor::ProcessEvent(TLVReader inReader, SubscriptionClient &inClient, const EventHeader &inEventHeader)
{
    mSchemaVersionRange = inEventHeader.mDataSchemaVersionRange;
    return WEAVE_NO_ERROR;
}

WEAVE_ERROR TestEventProcessor::GapDetected(const EventHeader &inEventHeader)
{
    return WEAVE_NO_ERROR;
}

static WEAVE_ERROR VersionCompatibilityHelper(void *inContext, SchemaVersionRange &encodedSchemaVersionRange, SchemaVersionRange &decodedSchemaVersionRange)
{
    WEAVE_ERROR err;
    TestLoggingContext *context = static_cast<TestLoggingContext *>(inContext);

    InitializeEventLoggingWithPersistedCounters(context, 1, nl::Weave::Profiles::DataManagement::Production);

    TLVReader testReader;
    uint8_t backingStore[1024];
    uint32_t bytesWritten;
    Schema::Nest::Test::Trait::TestETrait::TestEEvent evN = { 0 };
    EventSchema testSchema = Schema::Nest::Test::Trait::TestETrait::TestEEvent::Schema;
    TestEventProcessor eventProcessor;

    nl::MemoryManagement memMgmt = { malloc, free, realloc };
    nl::SerializationContext serializationContext;
    serializationContext.memMgmt = memMgmt;
    nl::StructureSchemaPointerPair appData;

    PrepareBinding(context);
    InitSubscriptionClient(context);

    testSchema.mMinCompatibleDataSchemaVersion = encodedSchemaVersionRange.mMinVersion;
    testSchema.mDataSchemaVersion = encodedSchemaVersionRange.mMaxVersion;

    appData.mStructureData = static_cast<void *>(&evN);
    appData.mFieldSchema = &Schema::Nest::Test::Trait::TestETrait::TestEEvent::FieldSchema;

    event_id_t eventId = nl::Weave::Profiles::DataManagement::LogEvent(testSchema, nl::SerializedDataToTLVWriterHelper, (void *)&appData);

    err = FetchEventsHelper(testReader, eventId, backingStore, sizeof(backingStore));
    SuccessOrExit(err);

    bytesWritten = testReader.GetRemainingLength() + testReader.GetLengthRead();
    testReader.Init(backingStore, bytesWritten);

    err = eventProcessor.ProcessEvents(testReader, *context->mSubClient);
    SuccessOrExit(err);

    decodedSchemaVersionRange = eventProcessor.mSchemaVersionRange;

    if (context->mVerbose)
    {
        nl::Weave::TLV::Debug::Dump(testReader, SimpleDumpWriter);
    }

exit:
    return err;
}

static void CheckVersion1DataCompatibility(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err;
    SchemaVersionRange encodedSchemaVersionRange, decodedSchemaVersionRange;

    encodedSchemaVersionRange.mMaxVersion = 1;
    encodedSchemaVersionRange.mMinVersion = 1;

    err = VersionCompatibilityHelper(inContext, encodedSchemaVersionRange, decodedSchemaVersionRange);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    NL_TEST_ASSERT(inSuite, encodedSchemaVersionRange == decodedSchemaVersionRange);
}

static void CheckForwardDataCompatibility(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err;
    SchemaVersionRange encodedSchemaVersionRange, decodedSchemaVersionRange;

    encodedSchemaVersionRange.mMaxVersion = 4;
    encodedSchemaVersionRange.mMinVersion = 1;

    err = VersionCompatibilityHelper(inContext, encodedSchemaVersionRange, decodedSchemaVersionRange);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    NL_TEST_ASSERT(inSuite, encodedSchemaVersionRange == decodedSchemaVersionRange);
}

static void CheckDataIncompatibility(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err;
    SchemaVersionRange encodedSchemaVersionRange, decodedSchemaVersionRange;

    encodedSchemaVersionRange.mMaxVersion = 4;
    encodedSchemaVersionRange.mMinVersion = 2;

    err = VersionCompatibilityHelper(inContext, encodedSchemaVersionRange, decodedSchemaVersionRange);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    NL_TEST_ASSERT(inSuite, encodedSchemaVersionRange == decodedSchemaVersionRange);
}

namespace {

class FakeEventProcessor : public EventProcessor {
public:
    FakeEventProcessor();
    void ClearMock();
    WEAVE_ERROR ProcessEvent(TLVReader inReader, SubscriptionClient &inClient, const EventHeader &inEventHeader);
    WEAVE_ERROR GapDetected(const EventHeader &inEventHeader);

    EventHeader mLastEventHeader;
    bool mGapDetected;
    EventHeader mGapEventHeader;
    int mEventsProcessed;
};

FakeEventProcessor::FakeEventProcessor()
    : EventProcessor(0)
    , mLastEventHeader()
    , mGapDetected(false)
    , mGapEventHeader()
    , mEventsProcessed()
{
}

void FakeEventProcessor::ClearMock()
{
    mLastEventHeader = EventHeader();
    mGapDetected = false;
    mGapEventHeader = EventHeader();
    mEventsProcessed = 0;
}

WEAVE_ERROR FakeEventProcessor::ProcessEvent(TLVReader inReader, SubscriptionClient &inClient, const EventHeader &inEventHeader)
{
    mLastEventHeader = inEventHeader;
    mEventsProcessed++;
    return WEAVE_NO_ERROR;
}

WEAVE_ERROR FakeEventProcessor::GapDetected(const EventHeader &inEventHeader)
{
    mGapDetected = true;
    mGapEventHeader = inEventHeader;
    return WEAVE_NO_ERROR;
}

}

static void CheckGapDetection(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err;
    TestLoggingContext *context = static_cast<TestLoggingContext *>(inContext);

    InitializeEventLoggingWithPersistedCounters(context, 1, nl::Weave::Profiles::DataManagement::Production);

    TLVReader testReader;
    uint8_t backingStore[1024];
    uint32_t bytesWritten;
    Schema::Nest::Test::Trait::TestETrait::TestEEvent evN = { 0 };
    EventSchema testSchema = Schema::Nest::Test::Trait::TestETrait::TestEEvent::Schema;
    FakeEventProcessor eventProcessor;

    nl::MemoryManagement memMgmt = { malloc, free, realloc };
    nl::SerializationContext serializationContext;
    serializationContext.memMgmt = memMgmt;
    nl::StructureSchemaPointerPair appData;

    PrepareBinding(context);
    InitSubscriptionClient(context);

    appData.mStructureData = static_cast<void *>(&evN);
    appData.mFieldSchema = &Schema::Nest::Test::Trait::TestETrait::TestEEvent::FieldSchema;

    // Arrange two consecutive events
    event_id_t eventId_A = nl::Weave::Profiles::DataManagement::LogEvent(testSchema, nl::SerializedDataToTLVWriterHelper, (void *)&appData);
    event_id_t eventId_B = nl::Weave::Profiles::DataManagement::LogEvent(testSchema, nl::SerializedDataToTLVWriterHelper, (void *)&appData);

    (void)eventId_B;

    // Arrange testReader with all events from the start
    err = FetchEventsHelper(testReader, eventId_A, backingStore, sizeof(backingStore));
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    bytesWritten = testReader.GetRemainingLength() + testReader.GetLengthRead();
    testReader.Init(backingStore, bytesWritten);
    err = eventProcessor.ProcessEvents(testReader, *context->mSubClient);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    NL_TEST_ASSERT(inSuite, eventProcessor.mGapDetected == false);
    NL_TEST_ASSERT(inSuite, eventProcessor.mEventsProcessed == 2);

    eventProcessor.ClearMock();

    // Arrange two more consecutive events
    event_id_t eventId_C = nl::Weave::Profiles::DataManagement::LogEvent(testSchema, nl::SerializedDataToTLVWriterHelper, (void *)&appData);
    event_id_t eventId_D = nl::Weave::Profiles::DataManagement::LogEvent(testSchema, nl::SerializedDataToTLVWriterHelper, (void *)&appData);

    (void)eventId_C;

    // Arrange testReader skipping eventId_C
    err = FetchEventsHelper(testReader, eventId_D, backingStore, sizeof(backingStore));
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    bytesWritten = testReader.GetRemainingLength() + testReader.GetLengthRead();
    testReader.Init(backingStore, bytesWritten);
    err = eventProcessor.ProcessEvents(testReader, *context->mSubClient);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    NL_TEST_ASSERT(inSuite, eventProcessor.mGapDetected == true);
    NL_TEST_ASSERT(inSuite, eventProcessor.mEventsProcessed == 1);

    eventProcessor.ClearMock();
}

static void CheckDropOverlap(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err;
    TestLoggingContext *context = static_cast<TestLoggingContext *>(inContext);

    InitializeEventLoggingWithPersistedCounters(context, 1, nl::Weave::Profiles::DataManagement::Production);

    TLVReader testReader;
    uint8_t backingStore[1024];
    uint32_t bytesWritten;
    Schema::Nest::Test::Trait::TestETrait::TestEEvent evN = { 0 };
    EventSchema testSchema = Schema::Nest::Test::Trait::TestETrait::TestEEvent::Schema;
    FakeEventProcessor eventProcessor;

    nl::MemoryManagement memMgmt = { malloc, free, realloc };
    nl::SerializationContext serializationContext;
    serializationContext.memMgmt = memMgmt;
    nl::StructureSchemaPointerPair appData;

    PrepareBinding(context);
    InitSubscriptionClient(context);

    appData.mStructureData = static_cast<void *>(&evN);
    appData.mFieldSchema = &Schema::Nest::Test::Trait::TestETrait::TestEEvent::FieldSchema;

    // Arrange two consecutive events
    event_id_t eventId_A = nl::Weave::Profiles::DataManagement::LogEvent(testSchema, nl::SerializedDataToTLVWriterHelper, (void *)&appData);
    event_id_t eventId_B = nl::Weave::Profiles::DataManagement::LogEvent(testSchema, nl::SerializedDataToTLVWriterHelper, (void *)&appData);

    // Arrange testReader with all events from the start
    err = FetchEventsHelper(testReader, eventId_A, backingStore, sizeof(backingStore));
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    bytesWritten = testReader.GetRemainingLength() + testReader.GetLengthRead();
    testReader.Init(backingStore, bytesWritten);

    err = eventProcessor.ProcessEvents(testReader, *context->mSubClient);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    NL_TEST_ASSERT(inSuite, eventProcessor.mGapDetected == false);
    NL_TEST_ASSERT(inSuite, eventProcessor.mEventsProcessed == 2);

    eventProcessor.ClearMock();

    // Arrange two more consecutive events
    event_id_t eventId_C = nl::Weave::Profiles::DataManagement::LogEvent(testSchema, nl::SerializedDataToTLVWriterHelper, (void *)&appData);
    event_id_t eventId_D = nl::Weave::Profiles::DataManagement::LogEvent(testSchema, nl::SerializedDataToTLVWriterHelper, (void *)&appData);

    (void)eventId_C;
    (void)eventId_D;

    // Arrange testReader overlapping eventId_B
    err = FetchEventsHelper(testReader, eventId_B, backingStore, sizeof(backingStore));
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    bytesWritten = testReader.GetRemainingLength() + testReader.GetLengthRead();
    testReader.Init(backingStore, bytesWritten);

    err = eventProcessor.ProcessEvents(testReader, *context->mSubClient);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    NL_TEST_ASSERT(inSuite, eventProcessor.mGapDetected == false);
    NL_TEST_ASSERT(inSuite, eventProcessor.mEventsProcessed == 2);

    eventProcessor.ClearMock();

    // Arrange testReader overlapping all events
    err = FetchEventsHelper(testReader, eventId_A, backingStore, sizeof(backingStore));
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    bytesWritten = testReader.GetRemainingLength() + testReader.GetLengthRead();
    testReader.Init(backingStore, bytesWritten);

    err = eventProcessor.ProcessEvents(testReader, *context->mSubClient);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    NL_TEST_ASSERT(inSuite, eventProcessor.mGapDetected == false);
    NL_TEST_ASSERT(inSuite, eventProcessor.mEventsProcessed == 0);

    eventProcessor.ClearMock();
}

static void CheckNullableFieldsSimple(nlTestSuite *inSuite, void *inContext)
{
    TestLoggingContext *context = static_cast<TestLoggingContext *>(inContext);
    WEAVE_ERROR err;

    TLVReader testReader;
    uint8_t backingStore[1024];
    Schema::Nest::Test::Trait::TestETrait::TestEEvent evN = { 0 };
    Schema::Nest::Test::Trait::TestETrait::TestEEvent deserializedEvN = { 0 };

    nl::MemoryManagement memMgmt = { malloc, free, realloc };
    nl::SerializationContext serializationContext;
    serializationContext.memMgmt = memMgmt;

    InitializeEventLogging(context);

    evN.teA = 10;
    evN.SetTeJNull();

    event_id_t eventId = nl::LogEvent(&evN);

    err = FetchEventsHelper(testReader, eventId, backingStore, sizeof(backingStore));
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    if (context->mVerbose)
    {
        nl::Weave::TLV::Debug::Dump(testReader, SimpleDumpWriter);
    }

    err = nl::DeserializeEvent(testReader, &deserializedEvN, &serializationContext);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    NL_TEST_ASSERT(inSuite, deserializedEvN.teA == evN.teA);
    NL_TEST_ASSERT(inSuite, GET_FIELD_NULLIFIED_BIT(deserializedEvN.__nullified_fields__, 0));
    NL_TEST_ASSERT(inSuite, deserializedEvN.IsTeJPresent() == false);
}

static void CheckNullableFieldsComplex(nlTestSuite *inSuite, void *inContext)
{
    // pattern: for each bit in nullified fields, set and check
    // for array of nullable structs, set and check
    TestLoggingContext *context = static_cast<TestLoggingContext *>(inContext);
    WEAVE_ERROR err;

    uint8_t backingStore[1024];
    Schema::Nest::Test::Trait::TestETrait::TestENullableEvent teN_s = { 0 };

    teN_s.neA = 0xAAAAAAAA;
    teN_s.neB = -1;
    teN_s.neC = true;
    teN_s.neD = "bar\0";
    teN_s.neE = 5;
    teN_s.neF = 0x77777777;
    teN_s.neG = -30;
    teN_s.neH = false;
    teN_s.neI = "foo\0";
    teN_s.neJ.neA = 88;
    teN_s.neJ.neB = true;

    nl::MemoryManagement memMgmt = { malloc, free, realloc };
    nl::SerializationContext serializationContext;
    serializationContext.memMgmt = memMgmt;

    InitializeEventLogging(context);

    // hardcoded number nullable fields
    for (int i = 0; i < 10; i++)
    {
        Schema::Nest::Test::Trait::TestETrait::TestENullableEvent teN_d = { 0 };
        event_id_t eventId;
        TLVReader testReader;

        memset(teN_s.__nullified_fields__, 0, sizeof(teN_s.__nullified_fields__));
        memset(teN_s.neJ.__nullified_fields__, 0, sizeof(teN_s.neJ.__nullified_fields__));
        SET_FIELD_NULLIFIED_BIT(teN_s.__nullified_fields__, i);

        eventId = nl::LogEvent(&teN_s);

        err = FetchEventsHelper(testReader, eventId, backingStore, sizeof(backingStore));
        NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

        err = nl::DeserializeEvent(testReader, &teN_d, &serializationContext);
        NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

        NL_TEST_ASSERT(inSuite, GET_FIELD_NULLIFIED_BIT(teN_d.__nullified_fields__, i));

        if (i != 0)
        {
            NL_TEST_ASSERT(inSuite, teN_d.neA == teN_s.neA);
        }
        else
        {
            NL_TEST_ASSERT(inSuite, teN_d.IsNeAPresent() == false);
        }

        if (i != 1)
        {
            NL_TEST_ASSERT(inSuite, teN_d.neB == teN_s.neB);
        }
        else
        {
            NL_TEST_ASSERT(inSuite, teN_d.IsNeBPresent() == false);
        }

        if (i != 2)
        {
            NL_TEST_ASSERT(inSuite, teN_d.neC == teN_s.neC);
        }
        else
        {
            NL_TEST_ASSERT(inSuite, teN_d.IsNeCPresent() == false);
        }

        if (i != 3)
        {
            NL_TEST_ASSERT(inSuite, strcmp(teN_d.neD, teN_s.neD) == 0);
        }
        else
        {
            NL_TEST_ASSERT(inSuite, teN_d.IsNeDPresent() == false);
        }

        if (i != 4)
        {
            NL_TEST_ASSERT(inSuite, teN_d.neE == teN_s.neE);
        }
        else
        {
            NL_TEST_ASSERT(inSuite, teN_d.IsNeEPresent() == false);
        }

        if (i != 5)
        {
            NL_TEST_ASSERT(inSuite, teN_d.neF == teN_s.neF);
        }
        else
        {
            NL_TEST_ASSERT(inSuite, teN_d.IsNeFPresent() == false);
        }

        if (i != 6)
        {
            NL_TEST_ASSERT(inSuite, teN_d.neG == teN_s.neG);
        }
        else
        {
            NL_TEST_ASSERT(inSuite, teN_d.IsNeGPresent() == false);
        }

        if (i != 7)
        {
            NL_TEST_ASSERT(inSuite, teN_d.neH == teN_s.neH);
        }
        else
        {
            NL_TEST_ASSERT(inSuite, teN_d.IsNeHPresent() == false);
        }

        if (i != 8)
        {
            NL_TEST_ASSERT(inSuite, strcmp(teN_d.neI, teN_s.neI) == 0);
        }
        else
        {
            NL_TEST_ASSERT(inSuite, teN_d.IsNeIPresent() == false);
        }

        if (i != 9)
        {
            NL_TEST_ASSERT(inSuite, teN_d.neJ.neA == teN_s.neJ.neA);
            NL_TEST_ASSERT(inSuite, teN_d.neJ.neB == teN_s.neJ.neB);
        }
        else
        {
            NL_TEST_ASSERT(inSuite, teN_d.IsNeJPresent() == false);
        }

        err = nl::DeallocateEvent(&teN_d, &serializationContext);
        NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    }

    for (int i = 0; i < 2; i++)
    {
        Schema::Nest::Test::Trait::TestETrait::TestENullableEvent teN_d = { 0 };
        event_id_t eventId;
        TLVReader testReader;

        memset(teN_s.__nullified_fields__, 0, sizeof(teN_s.__nullified_fields__));
        memset(teN_s.neJ.__nullified_fields__, 0, sizeof(teN_s.neJ.__nullified_fields__));
        SET_FIELD_NULLIFIED_BIT(teN_s.neJ.__nullified_fields__, i);

        eventId = nl::LogEvent(&teN_s);

        err = FetchEventsHelper(testReader, eventId, backingStore, sizeof(backingStore));
        NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

        err = nl::DeserializeEvent(testReader, &teN_d, &serializationContext);
        NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

        NL_TEST_ASSERT(inSuite, GET_FIELD_NULLIFIED_BIT(teN_d.neJ.__nullified_fields__, i));

        NL_TEST_ASSERT(inSuite, teN_d.neA == teN_s.neA);
        NL_TEST_ASSERT(inSuite, teN_d.neB == teN_s.neB);
        NL_TEST_ASSERT(inSuite, teN_d.neC == teN_s.neC);
        NL_TEST_ASSERT(inSuite, strcmp(teN_d.neD, teN_s.neD) == 0);
        NL_TEST_ASSERT(inSuite, teN_d.neE == teN_s.neE);
        NL_TEST_ASSERT(inSuite, teN_d.neF == teN_s.neF);
        NL_TEST_ASSERT(inSuite, teN_d.neG == teN_s.neG);
        NL_TEST_ASSERT(inSuite, teN_d.neH == teN_s.neH);
        NL_TEST_ASSERT(inSuite, strcmp(teN_d.neI, teN_s.neI) == 0);

        if (i == 1)
        {
            NL_TEST_ASSERT(inSuite, teN_d.neJ.neA == teN_s.neJ.neA);
            NL_TEST_ASSERT(inSuite, teN_d.neJ.IsNeBPresent() == false);
        }
        else
        {
            NL_TEST_ASSERT(inSuite, teN_d.neJ.neB == teN_s.neJ.neB);
            NL_TEST_ASSERT(inSuite, teN_d.neJ.IsNeAPresent() == false);
        }
    }
}

static void CheckWDMOffloadTrigger(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    timestamp_t now;
    size_t counter = 0;
    uint32_t eventSize;
    ::nl::Weave::Profiles::DataManagement::TestSubscriptionHandler *testSubHandler;
    ::nl::Weave::Profiles::DataManagement::SubscriptionHandler *subHandler;
    nl::Weave::Profiles::DataManagement::LoggingManagement &logger =
        nl::Weave::Profiles::DataManagement::LoggingManagement::GetInstance();

    event_id_t eid, eid_prev;
    TestLoggingContext *context = static_cast<TestLoggingContext *>(inContext);

    InitializeEventLogging(context);

    // Each event is about 40 bytes; write 40 of those to ensure we
    // override the default WDM event byte threshold

    now = System::Layer::GetClock_MonotonicMS();
    eid_prev = FastLogFreeform(
        nl::Weave::Profiles::DataManagement::Production,
        now,
        "Freeform entry %d", counter++);
    eventSize = logger.GetBytesWritten();

    for (size_t expectedBufferSize = 0; expectedBufferSize < WEAVE_CONFIG_EVENT_LOGGING_BYTE_THRESHOLD; expectedBufferSize += eventSize)
    {
        now += 10;
        eid = FastLogFreeform(
            nl::Weave::Profiles::DataManagement::Production,
            now,
            "Freeform entry %d", counter++);
        NL_TEST_ASSERT(inSuite, eid == (eid_prev + 1));
        eid_prev = eid;
    }

    // subscription engine has no subscription handlers, we should not be running the WDM
    NL_TEST_ASSERT(inSuite, logger.CheckShouldRunWDM() == false);

    // create a fake subscription, and start messing with it to check that WDM trigger will run
    err = ::nl::Weave::Profiles::DataManagement::SubscriptionEngine::GetInstance()->NewSubscriptionHandler(&subHandler);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    testSubHandler = static_cast<::nl::Weave::Profiles::DataManagement::TestSubscriptionHandler *>(subHandler);
    new (testSubHandler) ::nl::Weave::Profiles::DataManagement::TestSubscriptionHandler();

    NL_TEST_ASSERT(inSuite, testSubHandler->IsFree());

    NL_TEST_ASSERT(inSuite, logger.CheckShouldRunWDM() == false);

    testSubHandler->SetActive();
    NL_TEST_ASSERT(inSuite, logger.CheckShouldRunWDM() == true);

    testSubHandler->SetEventLogEndpoint(logger);
    NL_TEST_ASSERT(inSuite, logger.CheckShouldRunWDM() == false);

    // A single event at this point should not trigger the engine
    eid = FastLogFreeform(
        nl::Weave::Profiles::DataManagement::Production,
        now,
        "Freeform entry %d", counter++);
    NL_TEST_ASSERT(inSuite, eid == (eid_prev + 1));
    NL_TEST_ASSERT(inSuite, logger.CheckShouldRunWDM() == false);
}


// Mock'd Events (would be autogen'd by phoenix)
struct CurrentEvent
{
    int32_t enumState;
    bool boolState;
    static const nl::SchemaFieldDescriptor FieldSchema;
    enum { kProfileId = 0x1U, kEventTypeId = 0x1U };
    static const nl::Weave::Profiles::DataManagement::EventSchema Schema;
};
const nl::FieldDescriptor CurrentEventFieldDescriptors[] =
{
    { NULL, offsetof(CurrentEvent, enumState), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 1 },
    { NULL, offsetof(CurrentEvent, boolState), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeBoolean, 0), 32 },
};
const nl::SchemaFieldDescriptor CurrentEvent::FieldSchema = { .mNumFieldDescriptorElements = sizeof(CurrentEventFieldDescriptors)/sizeof(CurrentEventFieldDescriptors[0]), .mFields = CurrentEventFieldDescriptors, .mSize = sizeof(CurrentEvent) };
const nl::Weave::Profiles::DataManagement::EventSchema CurrentEvent::Schema = { .mProfileId = kProfileId, .mStructureType = 0x1, .mImportance = nl::Weave::Profiles::DataManagement::ProductionCritical, .mDataSchemaVersion = 1, .mMinCompatibleDataSchemaVersion = 1 };

struct FutureEventNewBaseField
{
    int32_t enumState;
    int32_t otherEnumState;
    bool boolState;
    static const nl::SchemaFieldDescriptor FieldSchema;
    enum { kProfileId = 0x1U, kEventTypeId = 0x1U };
    static const nl::Weave::Profiles::DataManagement::EventSchema Schema;
};
const nl::FieldDescriptor FutureEventNewBaseFieldFieldDescriptors[] =
{
    { NULL, offsetof(FutureEventNewBaseField, enumState), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 1 },
    { NULL, offsetof(FutureEventNewBaseField, otherEnumState), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 2 },
    { NULL, offsetof(FutureEventNewBaseField, boolState), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeBoolean, 0), 32 },
};
const nl::SchemaFieldDescriptor FutureEventNewBaseField::FieldSchema = { .mNumFieldDescriptorElements = sizeof(FutureEventNewBaseFieldFieldDescriptors)/sizeof(FutureEventNewBaseFieldFieldDescriptors[0]), .mFields = FutureEventNewBaseFieldFieldDescriptors, .mSize = sizeof(FutureEventNewBaseField) };
const nl::Weave::Profiles::DataManagement::EventSchema FutureEventNewBaseField::Schema = { .mProfileId = kProfileId, .mStructureType = 0x1, .mImportance = nl::Weave::Profiles::DataManagement::ProductionCritical, .mDataSchemaVersion = 2, .mMinCompatibleDataSchemaVersion = 1 };

static void CheckDeserializingNewerVersion(nlTestSuite *inSuite, void *inContext)
{
    TestLoggingContext *context = static_cast<TestLoggingContext *>(inContext);
    WEAVE_ERROR err;
    nl::MemoryManagement memMgmt = { malloc, free, realloc };
    nl::SerializationContext serializationContext;
    serializationContext.memMgmt = memMgmt;
    uint8_t backingStore[1024];
    InitializeEventLogging(context);

    FutureEventNewBaseField externalEv = { 0 };
    externalEv.enumState = 10;
    externalEv.otherEnumState = 20;
    externalEv.boolState = true;

    event_id_t eventId = nl::LogEvent(&externalEv);

    TLVReader testReader;
    err = FetchEventsHelper(testReader, eventId, backingStore, sizeof(backingStore), nl::Weave::Profiles::DataManagement::ProductionCritical);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    if (context->mVerbose)
    {
        nl::Weave::TLV::Debug::Dump(testReader, SimpleDumpWriter);
    }

    CurrentEvent deserializedEv = { 0 };
    nl::StructureSchemaPointerPair structureSchemaPair;
    structureSchemaPair.mStructureData = &deserializedEv;
    structureSchemaPair.mFieldSchema = &CurrentEvent::FieldSchema;

    err = nl::TLVReaderToDeserializedDataHelper(testReader, nl::Weave::Profiles::DataManagement::kTag_EventData,
               (void *)&structureSchemaPair, &serializationContext);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    NL_TEST_ASSERT(inSuite, deserializedEv.enumState == externalEv.enumState);
    NL_TEST_ASSERT(inSuite, deserializedEv.boolState == externalEv.boolState);
}

static void CheckDeserializingOlderVersion(nlTestSuite *inSuite, void *inContext)
{
    TestLoggingContext *context = static_cast<TestLoggingContext *>(inContext);
    WEAVE_ERROR err;
    nl::MemoryManagement memMgmt = { malloc, free, realloc };
    nl::SerializationContext serializationContext;
    serializationContext.memMgmt = memMgmt;
    uint8_t backingStore[1024];
    InitializeEventLogging(context);

    CurrentEvent externalEv = { 0 };
    externalEv.enumState = 10;
    externalEv.boolState = true;

    event_id_t eventId = nl::LogEvent(&externalEv);

    TLVReader testReader;
    err = FetchEventsHelper(testReader, eventId, backingStore, sizeof(backingStore), nl::Weave::Profiles::DataManagement::ProductionCritical);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    if (context->mVerbose)
    {
        nl::Weave::TLV::Debug::Dump(testReader, SimpleDumpWriter);
    }

    FutureEventNewBaseField deserializedEv = { 0 };
    nl::StructureSchemaPointerPair structureSchemaPair;
    structureSchemaPair.mStructureData = &deserializedEv;
    structureSchemaPair.mFieldSchema = &FutureEventNewBaseField::FieldSchema;

    err = nl::TLVReaderToDeserializedDataHelper(testReader, nl::Weave::Profiles::DataManagement::kTag_EventData,
               (void *)&structureSchemaPair, &serializationContext);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    NL_TEST_ASSERT(inSuite, deserializedEv.enumState == externalEv.enumState);
    NL_TEST_ASSERT(inSuite, deserializedEv.otherEnumState == 0);
    NL_TEST_ASSERT(inSuite, deserializedEv.boolState == externalEv.boolState);
}

// ---------------
struct CurrentNullableEvent
{
    int32_t baseEnum;
    void SetBaseEnumNull(void);
    void SetBaseEnumPresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsBaseEnumPresent(void);
#endif

    int32_t extendedEnum;
    void SetExtendedEnumNull(void);
    void SetExtendedEnumPresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsExtendedEnumPresent(void);
#endif
    uint8_t __nullified_fields__[2 /8 + 1];
    static const nl::SchemaFieldDescriptor FieldSchema;
    enum { kProfileId = 0x1U, kEventTypeId = 0x1U };
    static const nl::Weave::Profiles::DataManagement::EventSchema Schema;
};
const nl::FieldDescriptor CurrentNullableEventFieldDescriptors[] =
{
    { NULL, offsetof(CurrentNullableEvent, baseEnum), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 1), 1 },
    { NULL, offsetof(CurrentNullableEvent, extendedEnum), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 1), 32 },
};
const nl::SchemaFieldDescriptor CurrentNullableEvent::FieldSchema = { .mNumFieldDescriptorElements = sizeof(CurrentNullableEventFieldDescriptors)/sizeof(CurrentNullableEventFieldDescriptors[0]), .mFields = CurrentNullableEventFieldDescriptors, .mSize = sizeof(CurrentNullableEvent) };
const nl::Weave::Profiles::DataManagement::EventSchema CurrentNullableEvent::Schema = { .mProfileId = kProfileId, .mStructureType = 0x1, .mImportance = nl::Weave::Profiles::DataManagement::ProductionCritical, .mDataSchemaVersion = 2, .mMinCompatibleDataSchemaVersion = 1 };
inline void CurrentNullableEvent::SetBaseEnumNull(void) { SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 0); }
inline void CurrentNullableEvent::SetBaseEnumPresent(void) { CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 0); }
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool CurrentNullableEvent::IsBaseEnumPresent(void) { return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 0)); }
#endif
inline void CurrentNullableEvent::SetExtendedEnumNull(void) { SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 1); }
inline void CurrentNullableEvent::SetExtendedEnumPresent(void) { CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 1); }
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool CurrentNullableEvent::IsExtendedEnumPresent(void) { return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 1)); }
#endif

struct FutureNullableEvent
{
    int32_t baseEnum;
    void SetBaseEnumNull(void);
    void SetBaseEnumPresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsBaseEnumPresent(void);
#endif

    int32_t futureEnum;
    void SetFutureEnumNull(void);
    void SetFutureEnumPresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsFutureEnumPresent(void);
#endif

    int32_t extendedEnum;
    void SetExtendedEnumNull(void);
    void SetExtendedEnumPresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsExtendedEnumPresent(void);
#endif

    int32_t futureExtendedEnum;
    void SetFutureExtendedEnumNull(void);
    void SetFutureExtendedEnumPresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsFutureExtendedEnumPresent(void);
#endif

    uint8_t __nullified_fields__[4 /8 + 1];
    static const nl::SchemaFieldDescriptor FieldSchema;
    enum { kProfileId = 0x1U, kEventTypeId = 0x1U };
    static const nl::Weave::Profiles::DataManagement::EventSchema Schema;
};
const nl::FieldDescriptor FutureNullableEventFieldDescriptors[] =
{
    { NULL, offsetof(FutureNullableEvent, baseEnum), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 1), 1 },
    { NULL, offsetof(FutureNullableEvent, futureEnum), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 1), 2 },
    { NULL, offsetof(FutureNullableEvent, extendedEnum), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 1), 32 },
    { NULL, offsetof(FutureNullableEvent, futureExtendedEnum), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 1), 33 },
};
const nl::SchemaFieldDescriptor FutureNullableEvent::FieldSchema = { .mNumFieldDescriptorElements = sizeof(FutureNullableEventFieldDescriptors)/sizeof(FutureNullableEventFieldDescriptors[0]), .mFields = FutureNullableEventFieldDescriptors, .mSize = sizeof(FutureNullableEvent) };
const nl::Weave::Profiles::DataManagement::EventSchema FutureNullableEvent::Schema = { .mProfileId = kProfileId, .mStructureType = 0x1, .mImportance = nl::Weave::Profiles::DataManagement::ProductionCritical, .mDataSchemaVersion = 2, .mMinCompatibleDataSchemaVersion = 1 };
inline void FutureNullableEvent::SetBaseEnumNull(void) { SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 0); }
inline void FutureNullableEvent::SetBaseEnumPresent(void) { CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 0); }
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool FutureNullableEvent::IsBaseEnumPresent(void) { return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 0)); }
#endif
inline void FutureNullableEvent::SetFutureEnumNull(void) { SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 1); }
inline void FutureNullableEvent::SetFutureEnumPresent(void) { CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 1); }
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool FutureNullableEvent::IsFutureEnumPresent(void) { return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 1)); }
#endif
inline void FutureNullableEvent::SetExtendedEnumNull(void) { SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 2); }
inline void FutureNullableEvent::SetExtendedEnumPresent(void) { CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 2); }
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool FutureNullableEvent::IsExtendedEnumPresent(void) { return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 2)); }
#endif
inline void FutureNullableEvent::SetFutureExtendedEnumNull(void) { SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 3); }
inline void FutureNullableEvent::SetFutureExtendedEnumPresent(void) { CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 3); }
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool FutureNullableEvent::IsFutureExtendedEnumPresent(void) { return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 3)); }
#endif

static void CheckDeserializingNewerVersionNullable(nlTestSuite *inSuite, void *inContext)
{
    TestLoggingContext *context = static_cast<TestLoggingContext *>(inContext);
    WEAVE_ERROR err;
    nl::MemoryManagement memMgmt = { malloc, free, realloc };
    nl::SerializationContext serializationContext;
    serializationContext.memMgmt = memMgmt;
    uint8_t backingStore[1024];
    InitializeEventLogging(context);

    FutureNullableEvent externalEv = { 0 };
    externalEv.baseEnum = 50;
    externalEv.SetBaseEnumPresent();
    externalEv.SetFutureEnumNull();
    externalEv.extendedEnum = 70;
    externalEv.SetExtendedEnumPresent();
    externalEv.SetFutureExtendedEnumNull();

    event_id_t eventId = nl::LogEvent(&externalEv);

    TLVReader testReader;
    err = FetchEventsHelper(testReader, eventId, backingStore, sizeof(backingStore), nl::Weave::Profiles::DataManagement::ProductionCritical);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    if (context->mVerbose)
    {
        nl::Weave::TLV::Debug::Dump(testReader, SimpleDumpWriter);
    }

    CurrentNullableEvent deserializedEv = { 0 };
    nl::StructureSchemaPointerPair structureSchemaPair;
    structureSchemaPair.mStructureData = &deserializedEv;
    structureSchemaPair.mFieldSchema = &CurrentNullableEvent::FieldSchema;

    err = nl::TLVReaderToDeserializedDataHelper(testReader, nl::Weave::Profiles::DataManagement::kTag_EventData,
               (void *)&structureSchemaPair, &serializationContext);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    NL_TEST_ASSERT(inSuite, deserializedEv.IsBaseEnumPresent());
    NL_TEST_ASSERT(inSuite, deserializedEv.IsBaseEnumPresent() == externalEv.IsBaseEnumPresent());
    NL_TEST_ASSERT(inSuite, deserializedEv.baseEnum == externalEv.baseEnum);

    NL_TEST_ASSERT(inSuite, deserializedEv.IsExtendedEnumPresent());
    NL_TEST_ASSERT(inSuite, deserializedEv.IsExtendedEnumPresent() == externalEv.IsExtendedEnumPresent());
    NL_TEST_ASSERT(inSuite, deserializedEv.extendedEnum == externalEv.extendedEnum);
}

static void CheckDeserializingOlderVersionNullable(nlTestSuite *inSuite, void *inContext)
{
    TestLoggingContext *context = static_cast<TestLoggingContext *>(inContext);
    WEAVE_ERROR err;
    nl::MemoryManagement memMgmt = { malloc, free, realloc };
    nl::SerializationContext serializationContext;
    serializationContext.memMgmt = memMgmt;
    uint8_t backingStore[1024];
    InitializeEventLogging(context);

    CurrentNullableEvent externalEv = { 0 };
    externalEv.baseEnum = 50;
    externalEv.SetBaseEnumPresent();
    externalEv.SetExtendedEnumNull();

    event_id_t eventId = nl::LogEvent(&externalEv);

    TLVReader testReader;
    err = FetchEventsHelper(testReader, eventId, backingStore, sizeof(backingStore), nl::Weave::Profiles::DataManagement::ProductionCritical);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    if (context->mVerbose)
    {
        nl::Weave::TLV::Debug::Dump(testReader, SimpleDumpWriter);
    }

    FutureNullableEvent deserializedEv = { 0 };
    nl::StructureSchemaPointerPair structureSchemaPair;
    structureSchemaPair.mStructureData = &deserializedEv;
    structureSchemaPair.mFieldSchema = &FutureNullableEvent::FieldSchema;

    err = nl::TLVReaderToDeserializedDataHelper(testReader, nl::Weave::Profiles::DataManagement::kTag_EventData,
               (void *)&structureSchemaPair, &serializationContext);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    NL_TEST_ASSERT(inSuite, deserializedEv.IsBaseEnumPresent());
    NL_TEST_ASSERT(inSuite, deserializedEv.IsBaseEnumPresent() == externalEv.IsBaseEnumPresent());
    NL_TEST_ASSERT(inSuite, deserializedEv.baseEnum == externalEv.baseEnum);

    NL_TEST_ASSERT(inSuite, deserializedEv.IsFutureEnumPresent() == false);
    NL_TEST_ASSERT(inSuite, deserializedEv.IsExtendedEnumPresent() == false);
    NL_TEST_ASSERT(inSuite, deserializedEv.IsFutureExtendedEnumPresent() == false);
}

static void CheckSubscriptionHandlerHelper(nlTestSuite *inSuite, TestLoggingContext *context, bool inLogInfoEvents)
{
    WEAVE_ERROR err;
    timestamp_t now;
    size_t counter = 0;
    ::nl::Weave::Profiles::DataManagement::TestSubscriptionHandler subHandler;
    nl::Weave::Profiles::DataManagement::LoggingManagement &logger =
        nl::Weave::Profiles::DataManagement::LoggingManagement::GetInstance();
    nl::Weave::Profiles::DataManagement::ImportanceType importance;
    TLVWriter writer;
    uint8_t backingStore[1024];

    event_id_t eid_init_prod, eid_prev_prod, eid_init_info, eid_prev_info, eid;

    now = System::Layer::GetClock_MonotonicMS();
    eid_init_prod = FastLogFreeform(
        nl::Weave::Profiles::DataManagement::Production,
        now,
        "Freeform entry %d", counter++);
    if (inLogInfoEvents)
    {
        eid_init_info = FastLogFreeform(
            nl::Weave::Profiles::DataManagement::Info,
            now+5,
            "Freeform entry %d", counter++);
    }

    eid_prev_prod = FastLogFreeform(
        nl::Weave::Profiles::DataManagement::Production,
        now+10,
        "Freeform entry %d", counter++);

    if (inLogInfoEvents)
    {
        eid_prev_info = FastLogFreeform(
            nl::Weave::Profiles::DataManagement::Info,
            now+15,
            "Freeform entry %d", counter++);
    }

    NL_TEST_ASSERT(inSuite, (eid_init_prod + 1) == eid_prev_prod);
    if (inLogInfoEvents)
    {
        if (nl::Weave::Profiles::DataManagement::LoggingConfiguration::GetInstance().mGlobalImportance >= nl::Weave::Profiles::DataManagement::Info)
        {
            NL_TEST_ASSERT(inSuite, (eid_init_info + 1) == eid_prev_info);
        }
        else
        {
            NL_TEST_ASSERT(inSuite, eid_prev_info == 0 && eid_init_info == 0);
        }
    }

    NL_TEST_ASSERT(inSuite, subHandler.CheckEventUpToDate(logger) == false);
    subHandler.SetEventLogEndpoint(logger);

    importance = subHandler.FindNextImportanceForTransfer();
    NL_TEST_ASSERT(inSuite, importance == nl::Weave::Profiles::DataManagement::Production);
    writer.Init(backingStore, 1024);
    CheckLogReadOut(inSuite, context, logger, importance, eid_init_prod, 2);

    err = logger.FetchEventsSince(writer, importance, subHandler.GetVendedEvent(importance));
    NL_TEST_ASSERT(inSuite, err == WEAVE_END_OF_TLV || err == WEAVE_NO_ERROR);

    // If we expect to have logged the Info events above, check the Info logs
    if (inLogInfoEvents && (nl::Weave::Profiles::DataManagement::LoggingConfiguration::GetInstance().mGlobalImportance >= nl::Weave::Profiles::DataManagement::Info))
    {
        importance = subHandler.FindNextImportanceForTransfer();
        NL_TEST_ASSERT(inSuite, importance == nl::Weave::Profiles::DataManagement::Info);
        writer.Init(backingStore, 1024);
        CheckLogReadOut(inSuite, context, logger, importance, eid_init_info, 2);
        err = logger.FetchEventsSince(writer, importance, subHandler.GetVendedEvent(importance));
        NL_TEST_ASSERT(inSuite, err == WEAVE_END_OF_TLV || err == WEAVE_NO_ERROR);
    }

    importance = subHandler.FindNextImportanceForTransfer();
    NL_TEST_ASSERT(inSuite, subHandler.VerifyTraversingImportance());

    while (importance != nl::Weave::Profiles::DataManagement::kImportanceType_Invalid)
    {
        err = logger.FetchEventsSince(writer, importance,  subHandler.GetVendedEvent(importance));
        NL_TEST_ASSERT(inSuite, err == WEAVE_END_OF_TLV || err == WEAVE_NO_ERROR);
        importance = subHandler.FindNextImportanceForTransfer();
    }

    // Verify that events are retrieved.
    NL_TEST_ASSERT(inSuite, subHandler.VerifyTraversingImportance());
    NL_TEST_ASSERT(inSuite, subHandler.CheckEventUpToDate(logger));

    // Check that a single event will trigger the up to date check

    eid = FastLogFreeform(
        nl::Weave::Profiles::DataManagement::Production,
        now + 10,
        "Freeform entry %d", counter++);

    NL_TEST_ASSERT(inSuite, (eid_prev_prod + 1) == eid);
    NL_TEST_ASSERT(inSuite, subHandler.CheckEventUpToDate(logger) == false);
    subHandler.SetEventLogEndpoint(logger);

    importance = subHandler.FindNextImportanceForTransfer();
    NL_TEST_ASSERT(inSuite, importance == nl::Weave::Profiles::DataManagement::Production);

    // Verify that the read operation will retrieve a single event
    eid_init_prod = subHandler.GetVendedEvent(importance);
    CheckLogReadOut(inSuite, context, logger, importance, eid_init_prod, 1);

    writer.Init(backingStore, 1024);
    while (importance != nl::Weave::Profiles::DataManagement::kImportanceType_Invalid)
    {
        err = logger.FetchEventsSince(writer, importance,  subHandler.GetVendedEvent(importance));
        NL_TEST_ASSERT(inSuite, err == WEAVE_END_OF_TLV || err == WEAVE_NO_ERROR);
        importance = subHandler.FindNextImportanceForTransfer();
    }

    //Verify that the all events are retrieved
    NL_TEST_ASSERT(inSuite, subHandler.VerifyTraversingImportance());
    NL_TEST_ASSERT(inSuite, subHandler.CheckEventUpToDate(logger));
}

static void CheckSubscriptionHandler(nlTestSuite *inSuite, void *inContext)
{
    TestLoggingContext *context = static_cast<TestLoggingContext *>(inContext);
    const bool aLogInfoEvents = false;

    InitializeEventLogging(context);

    CheckSubscriptionHandlerHelper(inSuite, context, aLogInfoEvents);
}

static void CheckSubscriptionHandlerCountersStartAtZeroProd(nlTestSuite *inSuite, void *inContext)
{
    TestLoggingContext *context = static_cast<TestLoggingContext *>(inContext);
    const bool aLogInfoEvents = false;

    InitializeEventLoggingWithPersistedCounters(context, 0, nl::Weave::Profiles::DataManagement::Production);

    CheckSubscriptionHandlerHelper(inSuite, context, aLogInfoEvents);
}

static void CheckSubscriptionHandlerCountersStartAtZeroTwoDifferentImportancesProd(nlTestSuite *inSuite, void *inContext)
{
    TestLoggingContext *context = static_cast<TestLoggingContext *>(inContext);
    const bool aLogInfoEvents = true;

    InitializeEventLoggingWithPersistedCounters(context, 0, nl::Weave::Profiles::DataManagement::Production);

    CheckSubscriptionHandlerHelper(inSuite, context, aLogInfoEvents);
}

static void CheckSubscriptionHandlerCountersStartAtNonZeroProd(nlTestSuite *inSuite, void *inContext)
{
    TestLoggingContext *context = static_cast<TestLoggingContext *>(inContext);
    const bool aLogInfoEvents = false;

    InitializeEventLoggingWithPersistedCounters(context, sEventIdCounterEpoch, nl::Weave::Profiles::DataManagement::Production);

    CheckSubscriptionHandlerHelper(inSuite, context, aLogInfoEvents);
}

static void CheckSubscriptionHandlerCountersStartAtNonZeroTwoDifferentImportancesProd(nlTestSuite *inSuite, void *inContext)
{
    TestLoggingContext *context = static_cast<TestLoggingContext *>(inContext);
    const bool aLogInfoEvents = true;

    InitializeEventLoggingWithPersistedCounters(context, sEventIdCounterEpoch, nl::Weave::Profiles::DataManagement::Production);

    CheckSubscriptionHandlerHelper(inSuite, context, aLogInfoEvents);
}

static void CheckSubscriptionHandlerCountersStartAtZeroInfo(nlTestSuite *inSuite, void *inContext)
{
    TestLoggingContext *context = static_cast<TestLoggingContext *>(inContext);
    const bool aLogInfoEvents = false;

    InitializeEventLoggingWithPersistedCounters(context, 0, nl::Weave::Profiles::DataManagement::Info);

    CheckSubscriptionHandlerHelper(inSuite, context, aLogInfoEvents);
}

static void CheckSubscriptionHandlerCountersStartAtZeroTwoDifferentImportancesInfo(nlTestSuite *inSuite, void *inContext)
{
    TestLoggingContext *context = static_cast<TestLoggingContext *>(inContext);
    const bool aLogInfoEvents = true;

    InitializeEventLoggingWithPersistedCounters(context, 0, nl::Weave::Profiles::DataManagement::Info);

    CheckSubscriptionHandlerHelper(inSuite, context, aLogInfoEvents);
}

static void CheckSubscriptionHandlerCountersStartAtNonZeroInfo(nlTestSuite *inSuite, void *inContext)
{
    TestLoggingContext *context = static_cast<TestLoggingContext *>(inContext);
    const bool aLogInfoEvents = false;

    InitializeEventLoggingWithPersistedCounters(context, sEventIdCounterEpoch, nl::Weave::Profiles::DataManagement::Info);

    CheckSubscriptionHandlerHelper(inSuite, context, aLogInfoEvents);
}

static void CheckSubscriptionHandlerCountersStartAtNonZeroTwoDifferentImportancesInfo(nlTestSuite *inSuite, void *inContext)
{
    TestLoggingContext *context = static_cast<TestLoggingContext *>(inContext);
    const bool aLogInfoEvents = true;

    InitializeEventLoggingWithPersistedCounters(context, sEventIdCounterEpoch, nl::Weave::Profiles::DataManagement::Info);

    CheckSubscriptionHandlerHelper(inSuite, context, aLogInfoEvents);
}

static void CheckExternalEvents(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TLVWriter testWriter;
    TLVReader testReader;
    event_id_t eid_in, eid = 0;
    int i;
    TestLoggingContext *context = static_cast<TestLoggingContext *>(inContext);

    InitializeEventLogging(context);

    for (i = 0; i < 10; i++)
    {
        eid_in = nl::Weave::Profiles::DataManagement::LogFreeform(
            nl::Weave::Profiles::DataManagement::Production,
            "Freeform entry %d", i);
    }

    // register callback
    err = LogMockExternalEvents(10, 1);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    for (i = 0; i < 10; i++)
    {
        eid_in = nl::Weave::Profiles::DataManagement::LogFreeform(
            nl::Weave::Profiles::DataManagement::Production,
            "Freeform entry %d", i+10);
    }

    // positive case where events lie within event range in importance buffer
    // retrieve all events in order
    for (int j = 0; j < 3; j++)
    {
        testWriter.Init(gLargeMemoryBackingStore, sizeof(gLargeMemoryBackingStore));
        err = nl::Weave::Profiles::DataManagement::LoggingManagement::GetInstance().FetchEventsSince(testWriter, nl::Weave::Profiles::DataManagement::Production, eid);
        NL_TEST_ASSERT(inSuite, eid == 10*(static_cast<event_id_t>(j) + 1));
        NL_TEST_ASSERT(inSuite, err == WEAVE_END_OF_TLV || err == WEAVE_NO_ERROR);

        if (context->mVerbose)
        {
            testReader.Init(gLargeMemoryBackingStore, testWriter.GetLengthWritten());
            nl::Weave::TLV::Debug::Dump(testReader, SimpleDumpWriter);
        }
    }

    // retrieve events starting in the middle of external events
    eid = 14;
    for (int x = 0; x < 2; x++)
    {
        testWriter.Init(gLargeMemoryBackingStore, sizeof(gLargeMemoryBackingStore));
        err = nl::Weave::Profiles::DataManagement::LoggingManagement::GetInstance().FetchEventsSince(testWriter, nl::Weave::Profiles::DataManagement::Production, eid);
        NL_TEST_ASSERT(inSuite, eid == 10*(static_cast<event_id_t>(x) + 2));
        NL_TEST_ASSERT(inSuite, err == WEAVE_END_OF_TLV || err == WEAVE_NO_ERROR);

        if (context->mVerbose)
        {
            testReader.Init(gLargeMemoryBackingStore, testWriter.GetLengthWritten());
            nl::Weave::TLV::Debug::Dump(testReader, SimpleDumpWriter);
        }
    }

    // log many events so no longer trying to fetch external events
    for (i = 0; i < 100; i++)
    {
        eid_in = nl::Weave::Profiles::DataManagement::LogFreeform(
            nl::Weave::Profiles::DataManagement::Production,
            "Freeform entry %d", i);
    }

    {
        utc_timestamp_t utc_tmp;
        timestamp_t time_tmp;
        event_id_t eid_tmp;

        eid = 0;
        testWriter.Init(gLargeMemoryBackingStore, sizeof(gLargeMemoryBackingStore));
        err = nl::Weave::Profiles::DataManagement::LoggingManagement::GetInstance().FetchEventsSince(testWriter, nl::Weave::Profiles::DataManagement::Production, eid);
        NL_TEST_ASSERT(inSuite, err == WEAVE_END_OF_TLV);
        NL_TEST_ASSERT(inSuite, eid == eid_in + 1);

        testReader.Init(gLargeMemoryBackingStore, testWriter.GetLengthWritten());
        ReadFirstEventHeader(testReader, time_tmp, utc_tmp, eid_tmp);
        NL_TEST_ASSERT(inSuite, eid_tmp >= 20);
    }
}

static void CheckExternalEventsMultipleCallbacks(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TLVWriter testWriter;
    TLVReader testReader;
    event_id_t eid = 0;
    TestLoggingContext *context = static_cast<TestLoggingContext *>(inContext);

    InitializeEventLogging(context);

    err = LogMockExternalEvents(10, 1);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    for (int i = 0; i < 10; i++)
    {
        (void)nl::Weave::Profiles::DataManagement::LogFreeform(
            nl::Weave::Profiles::DataManagement::Production,
            "Freeform entry %d", i);
    }

    err = LogMockExternalEvents(10, 2);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    err = LogMockExternalEvents(10, 3);
    NL_TEST_ASSERT(inSuite, err == WEAVE_ERROR_NO_MEMORY);

    ClearMockExternalEvents(1);

    // even after clearing the first callback, we should receive 3 separate error codes.
    for (int j = 0; j < 3; j++)
    {
        testWriter.Init(gLargeMemoryBackingStore, sizeof(gLargeMemoryBackingStore));
        err = nl::Weave::Profiles::DataManagement::LoggingManagement::GetInstance().FetchEventsSince(testWriter, nl::Weave::Profiles::DataManagement::Production, eid);
        NL_TEST_ASSERT(inSuite, eid == 10*(static_cast<event_id_t>(j) + 1));
        NL_TEST_ASSERT(inSuite, err == WEAVE_END_OF_TLV || err == WEAVE_NO_ERROR);

        if (context->mVerbose)
        {
            testReader.Init(gLargeMemoryBackingStore, testWriter.GetLengthWritten());
            nl::Weave::TLV::Debug::Dump(testReader, SimpleDumpWriter);
        }
    }
}

static void RegressionWatchdogBug(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TLVWriter testWriter;
    //TLVReader testReader;
    event_id_t eid = 0;
    ::nl::Weave::Profiles::DataManagement::TestSubscriptionHandler subHandler;
    TestLoggingContext *context = static_cast<TestLoggingContext *>(inContext);
    nl::Weave::Profiles::DataManagement::LoggingManagement &logger =
        nl::Weave::Profiles::DataManagement::LoggingManagement::GetInstance();
    nl::Weave::Profiles::DataManagement::ImportanceType importance;

    InitializeEventLogging(context);

    err = LogMockExternalEvents(10, 1);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    err = LogMockExternalEvents(10, 2);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    ClearMockExternalEvents(1);
    ClearMockExternalEvents(2);
    eid = nl::Weave::Profiles::DataManagement::LogFreeform(
        nl::Weave::Profiles::DataManagement::Production,
        "Freeform entry");

    NL_TEST_ASSERT(inSuite, eid == 20);

    testWriter.Init(gLargeMemoryBackingStore, sizeof(gLargeMemoryBackingStore));

    subHandler.SetEventLogEndpoint(logger);

    importance = subHandler.FindNextImportanceForTransfer();
    NL_TEST_ASSERT(inSuite, importance == nl::Weave::Profiles::DataManagement::Production);
    while (importance != nl::Weave::Profiles::DataManagement::kImportanceType_Invalid)
    {
        err = logger.FetchEventsSince(testWriter, importance,  subHandler.GetVendedEvent(importance));
        NL_TEST_ASSERT(inSuite, err == WEAVE_END_OF_TLV || err == WEAVE_NO_ERROR);
        importance = subHandler.FindNextImportanceForTransfer();
    }
    // Verify that events are retrieved.
    NL_TEST_ASSERT(inSuite, subHandler.VerifyTraversingImportance());
    NL_TEST_ASSERT(inSuite, subHandler.CheckEventUpToDate(logger));

}

static void RegressionWatchdogBug_EventRemoval(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TLVWriter testWriter;
    //TLVReader testReader;
    event_id_t eid = 0;
    timestamp_t now;
    ::nl::Weave::Profiles::DataManagement::TestSubscriptionHandler subHandler;
    TestLoggingContext *context = static_cast<TestLoggingContext *>(inContext);
    nl::Weave::Profiles::DataManagement::LoggingManagement &logger =
        nl::Weave::Profiles::DataManagement::LoggingManagement::GetInstance();
    nl::Weave::Profiles::DataManagement::ImportanceType importance;

    InitializeEventLogging(context);

    err = LogMockDebugExternalEvents(10, 1);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    err = LogMockDebugExternalEvents(10, 2);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    eid = nl::Weave::Profiles::DataManagement::LogFreeform(
        nl::Weave::Profiles::DataManagement::Debug,
        "Freeform entry");
    NL_TEST_ASSERT(inSuite, eid == 20);

    eid = nl::Weave::Profiles::DataManagement::LogFreeform(
        nl::Weave::Profiles::DataManagement::Debug,
        "Freeform entry");
    NL_TEST_ASSERT(inSuite, eid == 21);

    eid = nl::Weave::Profiles::DataManagement::LogFreeform(
        nl::Weave::Profiles::DataManagement::Debug,
        "Freeform entry");
    NL_TEST_ASSERT(inSuite, eid == 22);

    now = System::Layer::GetClock_MonotonicMS();
    for (size_t counter=0; counter < 100; counter++)
    {
        eid = FastLogFreeform(
            nl::Weave::Profiles::DataManagement::Production,
            now,
            "Freeform entry %d", counter);
        NL_TEST_ASSERT(inSuite, eid == counter);

        now+=10;
    }

    testWriter.Init(gLargeMemoryBackingStore, sizeof(gLargeMemoryBackingStore));

    subHandler.SetEventLogEndpoint(logger);

    importance = subHandler.FindNextImportanceForTransfer();
    NL_TEST_ASSERT(inSuite, importance == nl::Weave::Profiles::DataManagement::Production);
    while (importance != nl::Weave::Profiles::DataManagement::kImportanceType_Invalid)
    {
        err = logger.FetchEventsSince(testWriter, importance,  subHandler.GetVendedEvent(importance));
        NL_TEST_ASSERT(inSuite, err == WEAVE_END_OF_TLV || err == WEAVE_NO_ERROR);
        importance = subHandler.FindNextImportanceForTransfer();
    }
    // Verify that events are retrieved.
    NL_TEST_ASSERT(inSuite, subHandler.VerifyTraversingImportance());
    NL_TEST_ASSERT(inSuite, subHandler.CheckEventUpToDate(logger));

}

static void RegressionWatchdogBug_ExternalEventState(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TLVWriter testWriter;
    //TLVReader testReader;
    event_id_t eid = 0;
    timestamp_t now;
    ::nl::Weave::Profiles::DataManagement::TestSubscriptionHandler subHandler;
    TestLoggingContext *context = static_cast<TestLoggingContext *>(inContext);
    nl::Weave::Profiles::DataManagement::LoggingManagement &logger =
        nl::Weave::Profiles::DataManagement::LoggingManagement::GetInstance();
    nl::Weave::Profiles::DataManagement::ImportanceType importance;

    InitializeEventLogging(context);

    err = LogMockExternalEvents(10, 1);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    err = LogMockExternalEvents(10, 2);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    eid = nl::Weave::Profiles::DataManagement::LogFreeform(
        nl::Weave::Profiles::DataManagement::Production,
        "F");

    NL_TEST_ASSERT(inSuite, eid == 20);

    ClearMockExternalEvents(1);

    ClearMockExternalEvents(2);

    now = System::Layer::GetClock_MonotonicMS();
    for (size_t counter=0; counter < 100; counter++)
    {
        eid = FastLogFreeform(
            nl::Weave::Profiles::DataManagement::Production,
            now,
            "Freeform entry %d", counter);
        NL_TEST_ASSERT(inSuite, eid == (counter + 21));
        now+=10;
    }

    testWriter.Init(gLargeMemoryBackingStore, sizeof(gLargeMemoryBackingStore));

    subHandler.SetEventLogEndpoint(logger);

    importance = subHandler.FindNextImportanceForTransfer();
    NL_TEST_ASSERT(inSuite, importance == nl::Weave::Profiles::DataManagement::Production);
    while (importance != nl::Weave::Profiles::DataManagement::kImportanceType_Invalid)
    {
        err = logger.FetchEventsSince(testWriter, importance,  subHandler.GetVendedEvent(importance));
        NL_TEST_ASSERT(inSuite, err == WEAVE_END_OF_TLV || err == WEAVE_NO_ERROR);
        importance = subHandler.FindNextImportanceForTransfer();
    }
    // Verify that events are retrieved.
    NL_TEST_ASSERT(inSuite, subHandler.VerifyTraversingImportance());
    NL_TEST_ASSERT(inSuite, subHandler.CheckEventUpToDate(logger));

}

static void CheckExternalEventsMultipleFetches(nlTestSuite *inSuite, void *inContext)
{
    uint8_t smallMemoryBackingStore[256];
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TLVWriter testWriter;
    TLVReader testReader;
    event_id_t fetchId = 0;
    TestLoggingContext *context = static_cast<TestLoggingContext *>(inContext);

    InitializeEventLogging(context);

    err = LogMockExternalEvents(10, 0);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    while ((fetchId < 10) && (err == WEAVE_NO_ERROR))
    {
        timestamp_t time_tmp;
        utc_timestamp_t utc_tmp = 0;
        event_id_t eid_tmp = 0;

        testWriter.Init(smallMemoryBackingStore, sizeof(smallMemoryBackingStore));
        err = LoggingManagement::GetInstance().FetchEventsSince(testWriter, Production, fetchId);
        if (fetchId < 10)
        {
            NL_TEST_ASSERT(inSuite, err == WEAVE_ERROR_BUFFER_TOO_SMALL);
        }
        else
        {
        NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
        }

        if (err == WEAVE_ERROR_BUFFER_TOO_SMALL)
        {
            err = WEAVE_NO_ERROR;
        }

        testReader.Init(smallMemoryBackingStore, testWriter.GetLengthWritten());
        ReadFirstEventHeader(testReader, time_tmp, utc_tmp, eid_tmp);
        // eid_tmp is unsigned and so always positive
        NL_TEST_ASSERT(inSuite, eid_tmp < fetchId);
        NL_TEST_ASSERT(inSuite, utc_tmp != 0);
    }

    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
}

static void CheckShutdownLogic(nlTestSuite *inSuite, void *inContext)
{
    event_id_t eid = 0;
    int counter = 1;
    timestamp_t now;

    TestLoggingContext *context = static_cast<TestLoggingContext *>(inContext);

    InitializeEventLogging(context);
    DestroyEventLogging(context);

    now = System::Layer::GetClock_MonotonicMS();

    eid = FastLogFreeform(
        nl::Weave::Profiles::DataManagement::Production,
        now,
        "Freeform entry %d", counter);

    NL_TEST_ASSERT(inSuite, eid == 0);
}

static WEAVE_ERROR BuildSubscribeRequest(
        nl::Weave::TLV::TLVWriter& writer,
        const nl::Weave::Profiles::DataManagement::SubscriptionClient::OutEventParam& outSubscribeParam)
{
    WEAVE_ERROR err;
    SubscribeRequest::Builder request;

    err = request.Init(&writer);
    SuccessOrExit(err);

    {
        PathList::Builder & pathList = request.CreatePathListBuilder();

        pathList.EndOfPathList();
        SuccessOrExit(err = pathList.GetError());
    }

    {
        VersionList::Builder & versionList = request.CreateVersionListBuilder();

        versionList.EndOfVersionList();
        SuccessOrExit(err = versionList.GetError());
    }

    if (outSubscribeParam.mSubscribeRequestPrepareNeeded.mNeedAllEvents)
    {
        request.SubscribeToAllEvents(true);

        if (outSubscribeParam.mSubscribeRequestPrepareNeeded.mLastObservedEventListSize > 0)
        {
            EventList::Builder & eventList = request.CreateLastObservedEventIdListBuilder();

            for (size_t n = 0; n < outSubscribeParam.mSubscribeRequestPrepareNeeded.mLastObservedEventListSize; ++n)
            {
                Event::Builder & event = eventList.CreateEventBuilder();
                event.SourceId(outSubscribeParam.mSubscribeRequestPrepareNeeded.mLastObservedEventList[n].mSourceId)
                    .Importance(outSubscribeParam.mSubscribeRequestPrepareNeeded.mLastObservedEventList[n].mImportance)
                    .EventId(outSubscribeParam.mSubscribeRequestPrepareNeeded.mLastObservedEventList[n].mEventId)
                    .EndOfEvent();
                SuccessOrExit(err = event.GetError());
            }

            eventList.EndOfEventList();
            SuccessOrExit(err = eventList.GetError());
        }
    }

    request.EndOfRequest();
    SuccessOrExit(err = request.GetError());

    err = writer.Finalize();
    SuccessOrExit(err);

exit:
    return err;
}

static void MockSubscribeRequest(
        nlTestSuite* inSuite,
        ::nl::Weave::Profiles::DataManagement::TestSubscriptionHandler& aSubHandler,
        const nl::Weave::Profiles::DataManagement::SubscriptionClient::OutEventParam& outSubscribeParam)
{
    WEAVE_ERROR err;
    uint8_t backingStore[1024];
    TLVWriter writer;
    TLVReader reader;
    SubscribeRequest::Parser request;
    uint32_t rejectReasonProfileId = 0;
    uint16_t rejectReasonStatusCode = 0;

    writer.Init(backingStore, sizeof(backingStore));

    err = BuildSubscribeRequest(writer, outSubscribeParam);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    reader.Init(backingStore, writer.GetLengthWritten());

    err = reader.Next();
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    err = request.Init(reader);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    err = aSubHandler.ParsePathVersionEventLists(request, rejectReasonProfileId, rejectReasonStatusCode);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
}

/*
 * This test validates that if a peer specified X as the last observed event
 * ID, the subscription handler publishes X+1 for the next event.
 */
static void CheckLastObservedEventId(nlTestSuite *inSuite, void *inContext)
{
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

    event_id_t prod_eids[3];
    event_id_t info_eids[3];

    TestLoggingContext *context = static_cast<TestLoggingContext *>(inContext);

    InitializeEventLogging(context);

    // Mock 3 production events and 3 info events
    timestamp_t now = System::Layer::GetClock_MonotonicMS();
    for (int i = 0; i < 3; i++)
    {
        prod_eids[i] = FastLogFreeform(
            nl::Weave::Profiles::DataManagement::Production,
            now + i*10,
            "Prod entry %d", i);

        info_eids[i] = FastLogFreeform(
            nl::Weave::Profiles::DataManagement::Info,
            now + i*10 + 5,
            "Info entry %d", i);
    }

    ::nl::Weave::Profiles::DataManagement::TestSubscriptionHandler subHandler;
    nl::Weave::Profiles::DataManagement::LoggingManagement &logger =
        nl::Weave::Profiles::DataManagement::LoggingManagement::GetInstance();

    subHandler.SetEventLogEndpoint(logger);

    // Make sure we logged all events
    CheckLogReadOut(inSuite, context, logger, nl::Weave::Profiles::DataManagement::Production, prod_eids[0], 3);
    CheckLogReadOut(inSuite, context, logger, nl::Weave::Profiles::DataManagement::Info, info_eids[0], 3);

    // We still have events to upload
    NL_TEST_ASSERT(inSuite, subHandler.CheckEventUpToDate(logger) == false);

    // No events have been observed so the next importance should be
    // Production.
    NL_TEST_ASSERT(inSuite, subHandler.FindNextImportanceForTransfer() == nl::Weave::Profiles::DataManagement::Production);

    // Create a dummy exchange context so that SubscriptionHandler can verify
    // the local node id when parsing the Last Observed Event List in the
    // subscribe request.
    nl::Weave::ExchangeContext ec;
    ec.ExchangeMgr = static_cast<TestLoggingContext *>(inContext)->mExchangeMgr;
    subHandler.SetExchangeContext(&ec);

    // Mock Last Observed Event List
    {
        nl::Weave::Profiles::DataManagement::SubscriptionClient::OutEventParam outParam;
        nl::Weave::Profiles::DataManagement::SubscriptionClient::LastObservedEvent lastObservedEventList[2];

        // Production event
        lastObservedEventList[0].mSourceId = kTestNodeId;
        lastObservedEventList[0].mImportance = nl::Weave::Profiles::DataManagement::Production;
        lastObservedEventList[0].mEventId = prod_eids[2];

        // Info event
        lastObservedEventList[1].mSourceId = kTestNodeId;
        lastObservedEventList[1].mImportance = nl::Weave::Profiles::DataManagement::Info;
        lastObservedEventList[1].mEventId = info_eids[1];

        outParam.mSubscribeRequestPrepareNeeded.mNeedAllEvents = true;
        outParam.mSubscribeRequestPrepareNeeded.mLastObservedEventList = lastObservedEventList;
        outParam.mSubscribeRequestPrepareNeeded.mLastObservedEventListSize = ARRAY_SIZE(lastObservedEventList);

        MockSubscribeRequest(inSuite, subHandler, outParam);
    }

    // We still have events to process
    NL_TEST_ASSERT(inSuite, subHandler.CheckEventUpToDate(logger) == false);

    // Since Production events were all observed, the next importance should be
    // Info
    NL_TEST_ASSERT(inSuite, subHandler.FindNextImportanceForTransfer() == nl::Weave::Profiles::DataManagement::Info);

    // Make sure vended EIDs are what we expect
    NL_TEST_ASSERT(inSuite, subHandler.GetVendedEvent(nl::Weave::Profiles::DataManagement::Production) == prod_eids[2] + 1);
    NL_TEST_ASSERT(inSuite, subHandler.GetVendedEvent(nl::Weave::Profiles::DataManagement::Info) == info_eids[1]+ 1);

    // Now mock another subscribe request where all events are observed
    {
        nl::Weave::Profiles::DataManagement::SubscriptionClient::OutEventParam outParam;
        nl::Weave::Profiles::DataManagement::SubscriptionClient::LastObservedEvent lastObservedEventList[2];

        // Production event
        lastObservedEventList[0].mSourceId = kTestNodeId;
        lastObservedEventList[0].mImportance = nl::Weave::Profiles::DataManagement::Production;
        lastObservedEventList[0].mEventId = prod_eids[2];

        // Info event
        lastObservedEventList[1].mSourceId = kTestNodeId;
        lastObservedEventList[1].mImportance = nl::Weave::Profiles::DataManagement::Info;
        lastObservedEventList[1].mEventId = info_eids[2];

        outParam.mSubscribeRequestPrepareNeeded.mNeedAllEvents = true;
        outParam.mSubscribeRequestPrepareNeeded.mLastObservedEventList = lastObservedEventList;
        outParam.mSubscribeRequestPrepareNeeded.mLastObservedEventListSize = ARRAY_SIZE(lastObservedEventList);

        MockSubscribeRequest(inSuite, subHandler, outParam);
    }

    // No events to process
    NL_TEST_ASSERT(inSuite, subHandler.CheckEventUpToDate(logger) == true);
    NL_TEST_ASSERT(inSuite, subHandler.FindNextImportanceForTransfer() == nl::Weave::Profiles::DataManagement::kImportanceType_Invalid);

    // Log a new event and confirm that there's more events to process
    (void)FastLogFreeform(
        nl::Weave::Profiles::DataManagement::Production,
        now + 1000,
        "Last Prod entry");

    subHandler.SetEventLogEndpoint(logger);

    NL_TEST_ASSERT(inSuite, subHandler.CheckEventUpToDate(logger) == false);
    NL_TEST_ASSERT(inSuite, subHandler.FindNextImportanceForTransfer() == nl::Weave::Profiles::DataManagement::Production);

    DestroyEventLogging(context);
}


//Test Suite

/**
 *  Test Suite that lists all the test functions.
 */
static const nlTest sTests[] = {
    NL_TEST_DEF("Simple Event Log Test", CheckLogEventBasics),
    NL_TEST_DEF("Simple Freeform Log Test", CheckLogFreeform),
    NL_TEST_DEF("Simple Pre-formatted Log Test", CheckLogPreformed),
    NL_TEST_DEF("Schema Generated Log Test", CheckSchemaGeneratedLogging),
    NL_TEST_DEF("Check Byte String Field Type", CheckByteStringFieldType),
    NL_TEST_DEF("Check Byte String Array", CheckByteStringArray),
    NL_TEST_DEF("Check Log eviction", CheckEvict),
    NL_TEST_DEF("Check Fetch Events", CheckFetchEvents),
    NL_TEST_DEF("Check Large Events", CheckLargeEvents),
    NL_TEST_DEF("Check Fetch Event Timestamps", CheckFetchTimestamps),
    NL_TEST_DEF("Basic Deserialization Test", CheckBasicEventDeserialization),
    NL_TEST_DEF("Complex Deserialization Test", CheckComplexEventDeserialization),
    NL_TEST_DEF("Empty Array Deserialization Test", CheckEmptyArrayEventDeserialization),
    NL_TEST_DEF("Simple Nullable Fields Test", CheckNullableFieldsSimple),
    NL_TEST_DEF("Complex Nullable Fields Test", CheckNullableFieldsComplex),
    NL_TEST_DEF("Check Deserializing an Event from a Newer Version", CheckDeserializingNewerVersion),
    NL_TEST_DEF("Check Deserializing an Event from an Older Version", CheckDeserializingOlderVersion),
    NL_TEST_DEF("Check Deserializing an Event from a Newer Version with Nullables", CheckDeserializingNewerVersionNullable),
    NL_TEST_DEF("Check Deserializing an Event from an Older Version with Nullables", CheckDeserializingOlderVersionNullable),
    NL_TEST_DEF("Subscription Handler accounting", CheckSubscriptionHandler),
    NL_TEST_DEF("Subscription Handler accounting, PersistedCounters start at zero, same importances, Production global importance", CheckSubscriptionHandlerCountersStartAtZeroProd),
    NL_TEST_DEF("Subscription Handler accounting, PersistedCounters start at zero, two different importances, Production global importance", CheckSubscriptionHandlerCountersStartAtZeroTwoDifferentImportancesProd),
    NL_TEST_DEF("Subscription Handler accounting, PersistedCounters start at non-zero, same importances, Production global importance", CheckSubscriptionHandlerCountersStartAtNonZeroProd),
    NL_TEST_DEF("Subscription Handler accounting, PersistedCounters start at non-zero, two different importances, Production global importance", CheckSubscriptionHandlerCountersStartAtNonZeroTwoDifferentImportancesProd),
    NL_TEST_DEF("Subscription Handler accounting, PersistedCounters start at zero, same importances, Info global importance", CheckSubscriptionHandlerCountersStartAtZeroInfo),
    NL_TEST_DEF("Subscription Handler accounting, PersistedCounters start at zero, two different importances, Info global importance", CheckSubscriptionHandlerCountersStartAtZeroTwoDifferentImportancesInfo),
    NL_TEST_DEF("Subscription Handler accounting, PersistedCounters start at non-zero, same importances, Info global importance", CheckSubscriptionHandlerCountersStartAtNonZeroInfo),
    NL_TEST_DEF("Subscription Handler accounting, PersistedCounters start at non-zero, two different importances, Info global importance", CheckSubscriptionHandlerCountersStartAtNonZeroTwoDifferentImportancesInfo),
    NL_TEST_DEF("Check External Events Basic", CheckExternalEvents),
    NL_TEST_DEF("Check External Events Multiple Callbacks", CheckExternalEventsMultipleCallbacks),
    NL_TEST_DEF("Check External Events Multiple Fetches", CheckExternalEventsMultipleFetches),
    NL_TEST_DEF("Check Drop Events", CheckDropEvents),
    NL_TEST_DEF("Check Shutdown Logic", CheckShutdownLogic),
    NL_TEST_DEF("Check WDM offload trigger", CheckWDMOffloadTrigger),
    NL_TEST_DEF("Regression: watchdog bug", RegressionWatchdogBug),
    NL_TEST_DEF("Regression: external event cleanup", RegressionWatchdogBug_EventRemoval),
    NL_TEST_DEF("Regression: external event, external clear call", RegressionWatchdogBug_ExternalEventState),
    NL_TEST_DEF("Check version 1 data schema compatibility encoding + decoding", CheckVersion1DataCompatibility),
    NL_TEST_DEF("Check forward data compatibility encoding + decoding", CheckForwardDataCompatibility),
    NL_TEST_DEF("Check data incompatible encoding + decoding", CheckDataIncompatibility),
    NL_TEST_DEF("Check Gap detection", CheckGapDetection),
    NL_TEST_DEF("Check Drop Overlapping Event Id Ranges", CheckDropOverlap),
    NL_TEST_DEF("Check Last Observed Event Id", CheckLastObservedEventId),
    NL_TEST_SENTINEL()
};

int main(int argc, char *argv[])
{
    MockPlatform::gMockPlatformClocks.GetClock_RealTime = Private::GetClock_RealTime;
    MockPlatform::gMockPlatformClocks.SetClock_RealTime = Private::SetClock_RealTime;

    if (!ParseArgsFromEnvVar(TOOL_NAME, TOOL_OPTIONS_ENV_VAR_NAME, gToolOptionSets, NULL, true) ||
        !ParseArgs(TOOL_NAME, argc, argv, gToolOptionSets))
    {
        exit(EXIT_FAILURE);
    }

    nlTestSuite theSuite = {
        "weave-event-log",
        &sTests[0],
        TestSetup,
        TestTeardown
    };

    gTestLoggingContext.mReinitializeBDXUpload = true;

    // Generate machine-readable, comma-separated value (CSV) output.
    nl_test_set_output_style(OUTPUT_CSV);

    // Run test suit against one context
    nlTestRunner(&theSuite, &gTestLoggingContext);

    return nlTestRunnerStats(&theSuite);
}

bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg)
{
    switch (id)
    {
    case 't':
        gBDXContext.mUseTCP = true;
        break;
    case 'u':
        gBDXContext.mUseTCP = false;
        break;
    case 'D':
        gBDXContext.DestIPAddrStr = arg;
        gTestLoggingContext.bdx = true;
        break;
    case 'p':
        if (!ParseInt(arg, gBDXContext.DestNodeId))
        {
            PrintArgError("%s: Invalid value specified for destination node id: %s\n", progName, arg);
            return false;
        }
        gTestLoggingContext.bdx = true;
        break;
    case 'd':
        gTestLoggingContext.mVerbose = true;
        break;
    case 's':
        if (!ParseInt(arg, gBDXContext.mStartingBlock))
        {
            PrintArgError("%s: Invalid value specified for start block: %s\n", progName, arg);
            return false;
        }
        break;
    default:
        PrintArgError("%s: INTERNAL ERROR: Unhandled option: %s\n", progName, name);
        return false;
    }

    return true;
}

static void PrepareBinding(TestLoggingContext *context)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    Binding *binding = NULL;

    if (!context->mBinding) {
        binding = context->mExchangeMgr->NewBinding(HandleBindingEvent, context);
        if (binding == NULL)
        {
            printf("NewBinding failed\n");
            return;
        }

        Binding::Configuration bindingConfig = binding->BeginConfiguration()
            .Target_NodeId(gBDXContext.DestNodeId)
            .Transport_UDP()
            .Security_None();

        if (gBDXContext.DestIPAddrStr != NULL && nl::Inet::IPAddress::FromString(gBDXContext.DestIPAddrStr, gBDXContext.DestIPAddr))
        {
            bindingConfig.TargetAddress_IP(gBDXContext.DestIPAddr);

            err = bindingConfig.PrepareBinding();
            if (err != WEAVE_NO_ERROR)
            {
                printf("PrepareBinding failed\n");
                return;
            }
        }

        context->mBinding = binding;
    }
}

static WEAVE_ERROR InitSubscriptionClient(TestLoggingContext *context)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (!context->mSubClient) {
        err = SubscriptionEngine::GetInstance()->NewClient(&context->mSubClient, context->mBinding, NULL, NULL, NULL, 0);
    }

    return err;
}

static void HandleBindingEvent(void *const appState, const Binding::EventType event, const Binding::InEventParam &inParam, Binding::OutEventParam &outParam)
{
    TestLoggingContext *context = static_cast<TestLoggingContext *>(appState);

    switch (event)
    {
        case Binding::kEvent_BindingReady:
            gLogBDXUpload.StartUpload(context->mBinding);
            break;
        case Binding::kEvent_PrepareFailed:
            printf("Binding Prepare failed\n");
            break;
        default:
            Binding::DefaultEventHandler(appState, event, inParam, outParam);
            break;
    }
}

static void StartClientConnection(System::Layer *systemLayer, void *appState, System::Error error)
{
    BDXContext *ctx = static_cast<BDXContext *>(appState);

    printf("@@@ 0 StartClientConnection entering (Con: %p)\n", Con);

    if (Con != NULL && Con->State == WeaveConnection::kState_Closed)
    {
        printf("@@@ 1 remove previous con (currently closed)\n");
        Con->Close();
        Con = NULL;
    }

    // Do nothing if a connect attempt is already in progress.
    if (Con != NULL)
    {
        printf("@@@ 2 (Con: %p) previous Con likely hanging\n", Con);
        return;
    }

    //TODO: move this to BDX logic
    Con = MessageLayer.NewConnection();
    if (Con == NULL)
    {
        printf("@@@ 3 WeaveConnection.Connect failed: no memory\n");
        return;
    }
    printf("@@@ 3+ (Con: %p)\n", Con);
    Con->OnConnectionComplete = HandleConnectionComplete;
    Con->OnConnectionClosed = HandleConnectionClosed;

    printf("@@@ 3++ (DestNodeId: %" PRIX64 ", DestIPAddrStr: %s)\n", ctx->DestNodeId, ctx->DestIPAddrStr);

    WEAVE_ERROR err;
    if (ctx->DestIPAddrStr)
    {
        IPAddress::FromString(ctx->DestIPAddrStr, ctx->DestIPAddr);
        err = Con->Connect(ctx->DestNodeId, kWeaveAuthMode_Unauthenticated, ctx->DestIPAddr);
    }
    else // not specified, derive from NodeID
    {
        err = Con->Connect(ctx->DestNodeId);
    }

    if (err != WEAVE_NO_ERROR)
    {
        printf("@@@ 4 WeaveConnection.Connect failed: %X (%s)\n", err, ErrorStr(err));
        Con->Close();
        Con = NULL;
        return;
    }

    ConnectTry++;
    printf("@@@ 5 StartClientConnection exiting\n");
}

void HandleConnectionComplete(WeaveConnection *con, WEAVE_ERROR conErr)
{
    printf("@@@ 1 HandleConnectionComplete entering\n");

    WEAVE_ERROR err = WEAVE_NO_ERROR;
    char ipAddrStr[64];

    con->PeerAddr.ToString(ipAddrStr, sizeof(ipAddrStr));

    if (conErr != WEAVE_NO_ERROR)
    {
        printf("Connection FAILED to node %" PRIX64 " (%s): %s\n", con->PeerNodeId, ipAddrStr, ErrorStr(conErr));
        con->Close();
        Con = NULL;

        if (ConnectTry < ConnectMaxTry)
        {
            err = SystemLayer.StartTimer(ConnectInterval, StartClientConnection, &gBDXContext);
            if (err != WEAVE_NO_ERROR)
            {
                printf("Inet.StartTimer failed\n");
                exit(-1);
            }
        }
        else
        {
            printf("Connection FAILED to node %" PRIX64 " (%s) after %d attempts\n", con->PeerNodeId, ipAddrStr, ConnectTry);
            exit(-1);
        }

        ClientConEstablished = false;
        return;
    }

    printf("Connection established to node %" PRIX64 " (%s)\n", con->PeerNodeId, ipAddrStr);

    ClientConEstablished = true;

    //Send the ReceiveInit or SendInit request
    if (Con != NULL)
    {
        // Kick LogBDXUpload
    }
    else
    {
        printf("Non-connection Init Requests not supported!\n");
        exit(-1);
    }

    if (err == WEAVE_NO_ERROR)
    {
        WaitingForBDXResp = true;
    }

    printf("@@@ 7 HandleConnectionComplete exiting\n");
}

void HandleConnectionClosed(WeaveConnection *con, WEAVE_ERROR conErr)
{
    char ipAddrStr[64];
    con->PeerAddr.ToString(ipAddrStr, sizeof(ipAddrStr));

    if (conErr == WEAVE_NO_ERROR)
        printf("Connection closed to node %" PRIX64 " (%s)\n", con->PeerNodeId, ipAddrStr);
    else
        printf("Connection ABORTED to node %" PRIX64 " (%s): %s\n", con->PeerNodeId, ipAddrStr, ErrorStr(conErr));

    WaitingForBDXResp = false;

    if (Listening)
        con->Close();
    else if (con == Con)
    {
        con->Close();
        Con = NULL;
    }
}
