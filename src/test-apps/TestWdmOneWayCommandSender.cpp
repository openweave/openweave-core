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
 *      This file implements the Weave Data Management mock subscription responder.
 *
 */

#define WEAVE_CONFIG_ENABLE_FUNCT_ERROR_LOGGING 1

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif // __STDC_FORMAT_MACROS
#include <inttypes.h>

#include <Weave/Profiles/data-management/Current/WdmManagedNamespace.h>

#include "ToolCommon.h"
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Core/WeaveCore.h>
#include <Weave/WeaveVersion.h>
#include <Weave/Support/CodeUtils.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Profiles/common/CommonProfile.h>
#include <Weave/Profiles/time/WeaveTime.h>
#include <SystemLayer/SystemTimer.h>
#include <Weave/Support/CodeUtils.h>
#include <Weave/Support/TimeUtils.h>

#include <time.h>

#include "TestWdmOneWayCommand.h"

using nl::Weave::System::PacketBuffer;

using namespace nl::Inet;
using namespace nl::Weave;
using namespace nl::Weave::TLV;
using namespace nl::Weave::Profiles;
using namespace nl::Weave::Profiles::DataManagement;

#define TOOL_NAME "TestWdmOneWayCommandSender"
static bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg);
static bool HandleNonOptionArgs(const char *progName, int argc, char *argv[]);

uint64_t DestNodeId;
const char *DestAddr = NULL;
IPAddress DestIPAddr;
uint16_t DestPort;
InterfaceId DestIntf = INET_NULL_INTERFACEID; // only used for UDP

const nl::Weave::Profiles::Time::timesync_t kCommandTimeoutMicroSecs = 3*nl::kMicrosecondsPerSecond;

static void ParseDestAddress();

static TestWdmOneWayCommandSender gWdmOneWayCommandSender;

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


TestWdmOneWayCommandSender * TestWdmOneWayCommandSender::GetInstance ()
{
    return &gWdmOneWayCommandSender;
}

TestWdmOneWayCommandSender::TestWdmOneWayCommandSender() :
    mExchangeMgr(NULL),
    mClientBinding(NULL)
{
}

void TestWdmOneWayCommandSender::BindingEventCallback (void * const apAppState,
                                                       const Binding::EventType aEvent,
                                                       const Binding::InEventParam & aInParam,
                                                       Binding::OutEventParam & aOutParam)
{
    switch (aEvent)
    {

    default:
        Binding::DefaultEventHandler(apAppState, aEvent, aInParam, aOutParam);
    }
}

WEAVE_ERROR TestWdmOneWayCommandSender::Init(WeaveExchangeManager *aExchangeMgr,
                                             const IPAddress & destAddr,
                                             const uint64_t destNodeId)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    mExchangeMgr = aExchangeMgr;
    mClientBinding = mExchangeMgr->NewBinding(BindingEventCallback, this);
    mClientBinding->BeginConfiguration()
                    .Transport_UDP()
                    .TargetAddress_IP(destAddr)
                    .Target_NodeId(destNodeId)
                    .Security_None()
                    .PrepareBinding();

    mCommandSender.Init(mClientBinding, NULL, this);

    return err;
}

WEAVE_ERROR TestWdmOneWayCommandSender::Shutdown(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (NULL != mClientBinding)
    {
        mClientBinding->Release();
        mClientBinding = NULL;
    }

    mCommandSender.Close();

    return err;
}

WEAVE_ERROR TestWdmOneWayCommandSender::SendOneWayCommand(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    PacketBuffer *msgBuf = NULL;
    static CommandSender::SendParams sendParams = {
        NULL,
        ResourceIdentifier(ResourceIdentifier::RESOURCE_TYPE_RESERVED, ResourceIdentifier::SELF_NODE_ID),
        Schema::Nest::Test::Trait::TestATrait::kWeaveProfileId,
        SchemaVersionRange(TEST_SCHEMA_MAX_VER, TEST_SCHEMA_MIN_VER),
        TEST_TRAIT_INSTANCE_ID,
        TEST_COMMAND_TYPE,
        (kCommandFlag_IsOneWay | kCommandFlag_ActionTimeValid | kCommandFlag_ExpiryTimeValid | kCommandFlag_InitiationTimeValid)
    };

    WeaveLogDetail(DataManagement, "TestWdmOnewayCommandSender %s:", __func__);

    VerifyOrExit(NULL != mClientBinding, err = WEAVE_ERROR_INCORRECT_STATE);

    {
        nl::Weave::TLV::TLVWriter writer;
        uint64_t nowMicroSecs, deadline;

        err = System::Layer::GetClock_RealTime(nowMicroSecs);
        SuccessOrExit(err);

        deadline = nowMicroSecs + kCommandTimeoutMicroSecs;

        sendParams.InitiationTimeMicroSecond = nowMicroSecs;
        sendParams.ActionTimeMicroSecond = nowMicroSecs + kCommandTimeoutMicroSecs / 2;
        sendParams.ExpiryTimeMicroSecond = deadline;

        // Add arguments here
        {
            uint32_t dummyUInt = 7;
            bool dummyBool = false;
            nl::Weave::TLV::TLVType dummyType = nl::Weave::TLV::kTLVType_NotSpecified;

            msgBuf = PacketBuffer::New();
            VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

            writer.Init(msgBuf);

            err = writer.StartContainer(AnonymousTag, nl::Weave::TLV::kTLVType_Structure, dummyType);
            SuccessOrExit(err);

            err = writer.Put(nl::Weave::TLV::ContextTag(1), dummyUInt);
            SuccessOrExit(err);

            err = writer.PutBoolean(nl::Weave::TLV::ContextTag(2), dummyBool);
            SuccessOrExit(err);

            err = writer.EndContainer(dummyType);
            SuccessOrExit(err);

            err = writer.Finalize();
            SuccessOrExit(err);
        }
    }

    err = mCommandSender.SendCommand(msgBuf, NULL, sendParams);
    SuccessOrExit(err);

    msgBuf = NULL;

exit:
    if (NULL != msgBuf)
    {
        PacketBuffer::Free(msgBuf);
        msgBuf = NULL;
    }

    mCommandSender.Close();
    return err;
}

static OptionDef gToolOptionDefs[] =
{
    { "dest-addr",    kArgumentRequired, 'D' },
    { }
};

static const char *const gToolOptionHelp =
    "  -D, --dest-addr <host>[:<port>][%<interface>]\n"
    "       Send Echo Requests to a specific address rather than one\n"
    "       derived from the destination node id. <host> can be a hostname,\n"
    "       an IPv4 address or an IPv6 address. If <port> is specified, Echo\n"
    "       requests will be sent to the specified port. If <interface> is\n"
    "       specified, Echo Requests will be sent over the specified local\n"
    "       interface.\n"
    "\n"
    "       NOTE: When specifying a port with an IPv6 address, the IPv6 address\n"
    "       must be enclosed in brackets, e.g. [fd00:0:1:1::1]:11095.\n"
    "\n"
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
    "Usage: " TOOL_NAME " [<options...>] <dest-node-id>[@<dest-host>[:<dest-port>][%<interface>]]\n"
    WEAVE_VERSION_STRING "\n" WEAVE_TOOL_COPYRIGHT,
    "Send WDM Oneway Commands.\n"
);

static OptionSet *gToolOptionSets[] =
{
    &gToolOptions,
    &gNetworkOptions,
    &gWeaveNodeOptions,
    &gWeaveSecurityMode,
    &gCASEOptions,
    &gGroupKeyEncOptions,
    &gHelpOptions,
    &gGeneralSecurityOptions,
    NULL
};

void ParseDestAddress()
{
    // NOTE: This function is only used when communicating over UDP.  Code in the WeaveConnection object handles
    // parsing the destination node address for TCP connections.

    WEAVE_ERROR err;
    const char *addr;
    uint16_t addrLen;
    const char *intfName;
    uint16_t intfNameLen;

    err = ParseHostPortAndInterface(DestAddr, strlen(DestAddr), addr, addrLen, DestPort, intfName, intfNameLen);
    if (err != INET_NO_ERROR)
    {
        printf("Invalid destination address: %s\n", DestAddr);
        exit(EXIT_FAILURE);
    }

    if (!IPAddress::FromString(addr, addrLen, DestIPAddr))
    {
        printf("Invalid destination address: %s\n", DestAddr);
        exit(EXIT_FAILURE);
    }

    if (intfName != NULL)
    {
        err = InterfaceNameToId(intfName, DestIntf);
        if (err != INET_NO_ERROR)
        {
            printf("Invalid interface name: %s\n", intfName);
            exit(EXIT_FAILURE);
        }
    }
}

int main(int argc, char *argv[])
{
    InitToolCommon();

    if (argc == 1)
    {
        gHelpOptions.PrintBriefUsage(stderr);
        exit(EXIT_FAILURE);
    }

    if (!ParseArgsFromEnvVar(TOOL_NAME, TOOL_OPTIONS_ENV_VAR_NAME, gToolOptionSets, NULL, true) ||
        !ParseArgs(TOOL_NAME, argc, argv, gToolOptionSets, HandleNonOptionArgs) ||
        !ResolveWeaveNetworkOptions(TOOL_NAME, gWeaveNodeOptions, gNetworkOptions))
    {
        exit(EXIT_FAILURE);
    }

    if (WeaveSecurityMode::kGroupEnc == gWeaveSecurityMode.SecurityMode && gGroupKeyEncOptions.GetEncKeyId() == WeaveKeyId::kNone)
    {
        PrintArgError("%s: Please specify a group encryption key id using the --group-enc-... options.\n", TOOL_NAME);
        exit(EXIT_FAILURE);
    }

    InitSystemLayer();

    InitNetwork();

    InitWeaveStack(true, true);

    PrintNodeConfig();

    if (DestAddr != NULL)
    {
        ParseDestAddress();

        TestWdmOneWayCommandSender::GetInstance()->Init(&ExchangeMgr, DestIPAddr, DestNodeId);

        TestWdmOneWayCommandSender::GetInstance()->SendOneWayCommand();

        TestWdmOneWayCommandSender::GetInstance()->Shutdown();
    }
    else
    {
        printf("ERROR: Destination address needs to be specified\n");
        exit(-1);
    }

    ShutdownWeaveStack();
    ShutdownNetwork();
    ShutdownSystemLayer();

    return 0;
}

bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg)
{
    switch (id)
    {
    case 'D':
        DestAddr = arg;
        break;
    default:
        PrintArgError("%s: INTERNAL ERROR: Unhandled option: %s\n", progName, name);
        return false;
    }

    return true;
}

bool HandleNonOptionArgs(const char *progName, int argc, char *argv[])
{
    if (argc > 0)
    {
        if (argc > 1)
        {
            PrintArgError("%s: Unexpected argument: %s\n", progName, argv[1]);
            return false;
        }

        const char *nodeId = argv[0];
        char *p = (char *)strchr(nodeId, '@');
        if (p != NULL)
        {
            *p = 0;
            DestAddr = p+1;
        }

        if (!ParseNodeId(nodeId, DestNodeId))
        {
            PrintArgError("%s: Invalid value specified for destination node-id: %s\n", progName, nodeId);
            return false;
        }
    }

    else
    {
        PrintArgError("%s: Please specify either a node id or --listen\n", progName);
        return false;
    }

    return true;
}
