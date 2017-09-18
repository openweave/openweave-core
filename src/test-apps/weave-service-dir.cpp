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
 *      This file aims to test the service directory profile.
 *
 */

#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <limits.h>

#include "ToolCommon.h"
#include <Weave/WeaveVersion.h>
#include <Weave/Profiles/service-directory/ServiceDirectory.h>
#include "MockSDServer.h"

#define TOOL_NAME "weave-service-dir"

static void HandleConnectionComplete(WeaveConnection *con, WEAVE_ERROR conErr);
static WEAVE_ERROR GetRootDirectoryEntry(uint8_t *, uint16_t);
static void HandleServiceMgrStatus(void *appState, WEAVE_ERROR anError, StatusReport *aReport);
static void HandleTestTimeout(System::Layer* aSystemLayer, void* aAppState, System::Error aError);

enum
{
    kRole_ServiceDirServer = 0,
    kRole_ServiceDirClient,
};

uint8_t Role = kRole_ServiceDirServer;
ServiceDirectory::WeaveServiceManager ServiceMgr;
uint8_t ServiceDirCache[300];
const char *DirectoryServer = NULL;
WeaveAuthMode AuthMode = kWeaveAuthMode_Unauthenticated;
MockServiceDirServer MockSDServer;
static bool sLastIterationFailed = false;
static bool sTimerRunning = false;

static HelpOptions gHelpOptions(
    TOOL_NAME,
    "Usage: " TOOL_NAME " [<options...>]\n"
    "       " TOOL_NAME " [<options...>] --service-dir-server <host>[:<port>]\n",
    WEAVE_VERSION_STRING "\n" WEAVE_TOOL_COPYRIGHT
);

static OptionSet *gToolOptionSets[] =
{
    &gNetworkOptions,
    &gWeaveNodeOptions,
    &gServiceDirClientOptions,
    &gFaultInjectionOptions,
    &gHelpOptions,
    NULL
};

static void HandleTestTimeout(System::Layer* aSystemLayer, void* aAppState, System::Error aError)
{
    printf("test timeout\n");

    sTimerRunning = false;
    Done = true;
    ServiceMgr.cancel(kServiceEndpoint_SoftwareUpdate, static_cast<void *>(&ServiceMgr));
}

static void ExpireTimer(int32_t argument)
{
    SystemLayer.StartTimer(0, HandleTestTimeout, NULL);
}

static int32_t GetNumEventsAvailable(void)
{
    int32_t retval = 0;

    if (Role == kRole_ServiceDirClient && sTimerRunning)
    {
        retval = 1;
    }

    return retval;
}

int main(int argc, char *argv[])
{
    WEAVE_ERROR err;
    nl::Weave::System::Stats::Snapshot before;
    nl::Weave::System::Stats::Snapshot after;
    const bool printStats = true;

    InitToolCommon();

    SetupFaultInjectionContext(argc, argv, GetNumEventsAvailable, ExpireTimer);
    SetSignalHandler(DoneOnHandleSIGUSR1);

    gServiceDirClientOptions.ServerHost = NULL;

    if (!ParseArgsFromEnvVar(TOOL_NAME, TOOL_OPTIONS_ENV_VAR_NAME, gToolOptionSets, NULL, true) ||
        !ParseArgs(TOOL_NAME, argc, argv, gToolOptionSets))
    {
        exit(EXIT_FAILURE);
    }

    if (gNetworkOptions.LocalIPv6Addr != IPAddress::Any)
    {
        if (!gNetworkOptions.LocalIPv6Addr.IsIPv6ULA())
        {
            printf("ERROR: Local address must be an IPv6 ULA\n");
            exit(EXIT_FAILURE);
        }

        gWeaveNodeOptions.FabricId = gNetworkOptions.LocalIPv6Addr.GlobalId();
        gWeaveNodeOptions.LocalNodeId = IPv6InterfaceIdToWeaveNodeId(gNetworkOptions.LocalIPv6Addr.InterfaceId());
        gWeaveNodeOptions.SubnetId = gNetworkOptions.LocalIPv6Addr.Subnet();
    }

    Role = (gServiceDirClientOptions.ServerHost != NULL) ? kRole_ServiceDirClient : kRole_ServiceDirServer;

    InitSystemLayer();
    InitNetwork();
    InitWeaveStack(true, true);

    PrintNodeConfig();

    nl::Weave::Stats::UpdateSnapshot(before);

    if (Role == kRole_ServiceDirServer)
    {
        // Initialize the mock service directory server.
        err = MockSDServer.Init(&ExchangeMgr);
        FAIL_ERROR(err, "MockSDServer.Init failed");
    }
    else
    {
        err = ServiceMgr.init(&ExchangeMgr, ServiceDirCache, sizeof(ServiceDirCache),
            GetRootDirectoryEntry, AuthMode);
        if (err != WEAVE_NO_ERROR)
        {
            printf("ServiceMgr.init() failed with error: %s\n", ErrorStr(err));
            exit(EXIT_FAILURE);
        }

    }

    for (uint32_t iteration = 1; iteration <= gFaultInjectionOptions.TestIterations; iteration++)
    {

        if (Role == kRole_ServiceDirClient)
        {
            sLastIterationFailed = false;

            printf("Iteration %u\n", iteration);

            // Ask for a connection to SoftwareUpdate; the MockSDServer responds to both

            SystemLayer.StartTimer(30000, HandleTestTimeout, NULL);
            sTimerRunning = true;

            err = ServiceMgr.connect(kServiceEndpoint_SoftwareUpdate,
                    AuthMode,
                    static_cast<void *>(&ServiceMgr), // you have to put something here, or the status callback is not invoked
                    HandleServiceMgrStatus,
                    HandleConnectionComplete);
            if (err != WEAVE_NO_ERROR)
            {
                printf("WeaveServiceManager.Connect(): failed: %s\n", ErrorStr(err));
                Done = true;
                sLastIterationFailed = true;
            }

        }

        ServiceNetworkUntil(&Done);

        SystemLayer.CancelTimer(HandleTestTimeout, NULL);
        sTimerRunning = false;

        if (sLastIterationFailed)
        {
            // Sleep a couple of seconds; if a new attempt is made too soon, the service process can
            // reject the connection (e.g. if it is restarting after a crash)
            uint32_t waitTimeMs = 2000;

            ServiceNetworkUntil(NULL, &waitTimeMs);
        }

        Done = false;
    }

    ServiceMgr.relocate(WEAVE_NO_ERROR);
    ServiceMgr.reset(WEAVE_NO_ERROR);
    ServiceMgr.unresolve(WEAVE_NO_ERROR);
    ServiceMgr.cancel(kServiceEndpoint_SoftwareUpdate, NULL);

    if (Role == kRole_ServiceDirServer)
    {
        err = MockSDServer.TearDown();
        FAIL_ERROR(err, "MockSDServer.TearDown failed");
    }

    ProcessStats(before, after, printStats, NULL);
    PrintFaultInjectionCounters();

    ShutdownWeaveStack();
    ShutdownNetwork();
    ShutdownSystemLayer();

    return EXIT_SUCCESS;
}

void HandleConnectionComplete(WeaveConnection *con, WEAVE_ERROR conErr)
{
    char ipAddrStr[64];
    con->PeerAddr.ToString(ipAddrStr, sizeof(ipAddrStr));

    if (conErr != WEAVE_NO_ERROR)
    {
        printf("Connection failed to node %" PRIX64 " (%s)\n", con->PeerNodeId, ipAddrStr);
        sLastIterationFailed = true;
    }
    else
        printf("Connection established to node %" PRIX64 " (%s)\n", con->PeerNodeId, ipAddrStr);

    // stop the test
    con->Close();
    Done = true;
}

WEAVE_ERROR GetRootDirectoryEntry(uint8_t *buf, uint16_t bufSize)
{
    return gServiceDirClientOptions.GetRootDirectoryEntry(buf, bufSize);
}

void HandleServiceMgrStatus(void* anAppState, WEAVE_ERROR anError, StatusReport *aReport)
{
    if (aReport)
        printf("service directory status report [%" PRIx32 ", %" PRIx32 "] %s\n", aReport->mProfileId, aReport->mStatusCode,
                nl::StatusReportStr(aReport->mProfileId, aReport->mStatusCode));
    else
    {
        printf("service directory error %" PRIx32 " %s\n", static_cast<uint32_t>(anError), nl::ErrorStr(anError));
        if (anError == WEAVE_ERROR_INVALID_SERVICE_EP)
        {
            ServiceMgr.clearCache();
        }
        sLastIterationFailed = true;
    }

    Done = true;
}
