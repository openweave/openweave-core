/*
 *
 *    Copyright (c) 2018 Google LLC.
 *    Copyright (c) 2014-2017 Nest Labs, Inc.
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
 *      This file implements the Weave Mock Tunnel Service.
 *
 *      This instantiates a Server that accepts connections from
 *      a border gateway and may perform routing functions
 *      between different border gateways or respond to ping6
 *      over the tunnel.
 *      Beyond the Tunneling profile, the server also understands
 *      private test profiles (@see TestWeaveTunnel.h).
 *      The tunnel client implemented in @see TestWeaveTunnelBR.cpp
 *      uses the private profiles to test various scenarios.
 */

#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <unistd.h>
#include <fstream>

#include "ToolCommon.h"
#include <Weave/Core/WeaveEncoding.h>
#include <Weave/Core/WeaveSecurityMgr.h>
#include <Weave/Profiles/security/WeaveSecurity.h>
#include <Weave/Profiles/service-directory/ServiceDirectory.h>
#include <Weave/Support/CodeUtils.h>
#include <Weave/Support/logging/WeaveLogging.h>
#include <Weave/Profiles/ProfileCommon.h>
#include <Weave/Profiles/echo/WeaveEcho.h>
#include <Weave/Core/WeaveTLV.h>
#include "TestWeaveTunnel.h"
#include "TestWeaveTunnelServer.h"

#if WEAVE_CONFIG_ENABLE_TUNNELING

#define TOOL_NAME "TestWeaveTunnelCASEPersistServer"

#define TUNNEL_SERVICE_INTF "service-tun0"
#define TUNNEL_SERVICE_LL_ADDR "fe80::2"
#define DEFAULT_TFE_NODE_ID (0x18b4300000000002)
#define BUFF_AVAILABLE_SIZE (1024)

#define PERSISTENT_TUNNEL_SESSION_PATH  "./persistentTunnelCASE-Server"

using nl::StatusReportStr;
using namespace nl::Weave::Encoding;
using namespace nl::Weave::Profiles::WeaveTunnel;
using namespace nl::Weave::TLV;

/**
 * Handler for a Weave Tunnel control message.
 */
static void HandleTunnelControlMsg(ExchangeContext *ec, const IPPacketInfo *pktInfo,
                                   const WeaveMessageInfo *msgInfo, uint32_t profileId,
                                   uint8_t msgType, PacketBuffer *payload);

static WEAVE_ERROR TunServerInit (WeaveExchangeManager *exchangeMgr);
static WEAVE_ERROR TunServerShutdown (void);

static void HandleConnectionClosed(WeaveConnection *con, WEAVE_ERROR conErr);

static void HandleConnectionReceived(WeaveMessageLayer *msgLayer, WeaveConnection *con);

static void HandleSecureSessionEstablished(WeaveSecurityManager *sm, WeaveConnection *con,
                                           void *reqState, uint16_t sessionKeyId, uint64_t peerNodeId,
                                           uint8_t encType);
static void HandleSecureSessionError(WeaveSecurityManager *sm, WeaveConnection *con, void *reqState,
                                     WEAVE_ERROR localErr, uint64_t peerNodeId, StatusReport *statusReport);

static WEAVE_ERROR SendStatusReportResponse(ExchangeContext *ec, uint32_t profileId, uint32_t tunStatusCode,
                                            bool isRoutingRestricted = false);

static void HandleSessionPersistOnTunnelClosure(nl::Weave::WeaveConnection *con);
static WEAVE_ERROR RestorePersistedTunnelCASESession(nl::Weave::WeaveConnection *con);
static bool IsPersistentTunnelSessionPresent(uint64_t peerNodeId);

static HelpOptions gHelpOptions(
    TOOL_NAME,
    "Usage: " TOOL_NAME " [<options...>]\n",
    WEAVE_VERSION_STRING "\n" WEAVE_TOOL_COPYRIGHT
);

static OptionSet *gToolOptionSets[] =
{
    &gNetworkOptions,
    &gWeaveNodeOptions,
    &gCASEOptions,
    &gDeviceDescOptions,
    &gFaultInjectionOptions,
    &gHelpOptions,
    NULL
};

void HandleConnectionReceived(WeaveMessageLayer *msgLayer, WeaveConnection *con)
{
    char ipAddrStr[64];

    con->PeerAddr.ToString(ipAddrStr, sizeof(ipAddrStr));

    WeaveLogDetail(WeaveTunnel, "Connection received from node (%s)\n", ipAddrStr);

    con->OnConnectionClosed = HandleConnectionClosed;

    if (IsPersistentTunnelSessionPresent(kServiceEndpoint_WeaveTunneling))
    {
        RestorePersistedTunnelCASESession(con);
    }
}

WEAVE_ERROR TunServerInit (WeaveExchangeManager *exchangeMgr)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    //ExchMgr = exchangeMgr;

    MessageLayer.OnConnectionReceived = HandleConnectionReceived;
    MessageLayer.OnReceiveError = HandleMessageReceiveError;
    MessageLayer.OnAcceptError = HandleAcceptConnectionError;

    ExchangeMgr.RegisterUnsolicitedMessageHandler(kWeaveProfile_Tunneling,
                                                   kMsgType_TunnelOpenV2, HandleTunnelControlMsg,
                                                   NULL);
    ExchangeMgr.RegisterUnsolicitedMessageHandler(kWeaveProfile_Tunneling,
                                                   kMsgType_TunnelRouteUpdate, HandleTunnelControlMsg,
                                                   NULL);
    ExchangeMgr.RegisterUnsolicitedMessageHandler(kWeaveProfile_Tunneling,
                                                   kMsgType_TunnelClose, HandleTunnelControlMsg,
                                                   NULL);
    ExchangeMgr.RegisterUnsolicitedMessageHandler(kWeaveProfile_Tunneling,
                                                   kMsgType_TunnelLiveness, HandleTunnelControlMsg,
                                                   NULL);

    SecurityMgr.OnSessionEstablished = HandleSecureSessionEstablished;
    SecurityMgr.OnSessionError = HandleSecureSessionError;

    return err;
}

WEAVE_ERROR TunServerShutdown (void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    ExchangeMgr.UnregisterUnsolicitedMessageHandler(kWeaveProfile_Tunneling,
                                                   kMsgType_TunnelOpenV2);
    ExchangeMgr.UnregisterUnsolicitedMessageHandler(kWeaveProfile_Tunneling,
                                                   kMsgType_TunnelRouteUpdate);
    ExchangeMgr.UnregisterUnsolicitedMessageHandler(kWeaveProfile_Tunneling,
                                                   kMsgType_TunnelClose);
    ExchangeMgr.UnregisterUnsolicitedMessageHandler(kWeaveProfile_Tunneling,
                                                   kMsgType_TunnelLiveness);

    return err;
}

bool PersistedSessionKeyExists(const char *name)
{
    return (access(name, F_OK) != -1);
}

bool IsPersistentTunnelSessionPresent(uint64_t peerNodeId)
{
    const char * persistentTunnelSessionPath = PERSISTENT_TUNNEL_SESSION_PATH;

    return PersistedSessionKeyExists(persistentTunnelSessionPath);
}

WEAVE_ERROR
SuspendAndPersistTunnelCASESession(nl::Weave::WeaveConnection *con)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    std::ofstream persistentTunOfStream;
    nl::Weave::WeaveSessionKey * persistedTunnelSessionKey;
    const char * persistentTunnelSessionPath = PERSISTENT_TUNNEL_SESSION_PATH;

    VerifyOrExit(con, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // If exist, this functions has already been called
    VerifyOrExit(!PersistedSessionKeyExists(persistentTunnelSessionPath),
                 err = WEAVE_ERROR_SESSION_KEY_SUSPENDED);

    uint8_t buf[BUFF_AVAILABLE_SIZE];
    uint16_t dataLen;

    err = FabricState.FindSessionKey(con->DefaultKeyId, con->PeerNodeId, false,
                                      persistedTunnelSessionKey);
    SuccessOrExit(err);

    // Set the resumptionMsgIdValid flag
    persistedTunnelSessionKey->SetResumptionMsgIdsValid(true);

    // This call suspends the CASE session and returns a serialized byte stream
    err = FabricState.SuspendSession(persistedTunnelSessionKey->MsgEncKey.KeyId,
                                      persistedTunnelSessionKey->NodeId,
                                      buf,
                                      BUFF_AVAILABLE_SIZE,
                                      dataLen);
    SuccessOrExit(err);

    // If success, set goodbit in the internal state flag
    // In case of failure, set failbit.
    persistentTunOfStream.open(persistentTunnelSessionPath, std::ofstream::binary | std::ios::trunc);
    // If not open and associated with this stream object, directly fail
    VerifyOrExit(persistentTunOfStream.is_open(),
                 err = WEAVE_ERROR_PERSISTED_STORAGE_FAIL);
    // If fail, sets badbit or failbit in the internal state flags
    persistentTunOfStream.write((char*)buf, dataLen);
    // In case of failure, set failbit.
    persistentTunOfStream.close();
    // Check the stream's state flags
    VerifyOrExit(persistentTunOfStream.good(),
                 err = WEAVE_ERROR_PERSISTED_STORAGE_FAIL);

    printf("Suspending and persisting of tunnel CASE session successful\n");

exit:
    if (err != WEAVE_NO_ERROR)
    {
        printf("Suspending and persisting of tunnel CASE Session failed with Weave error: %d\n", err);
    }

    return err;
}

WEAVE_ERROR
RestorePersistedTunnelCASESession(nl::Weave::WeaveConnection *con)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    std::ifstream persistentCASE;
    const char * persistentTunnelSessionPath = PERSISTENT_TUNNEL_SESSION_PATH;

    if (PersistedSessionKeyExists(persistentTunnelSessionPath))
    {
        printf("persistent tunnel CASE session exists\n");
        uint8_t buf[BUFF_AVAILABLE_SIZE];

        // In case of failure, set failbit.
        persistentCASE.open(persistentTunnelSessionPath, std::ifstream::binary);
        VerifyOrExit(persistentCASE.is_open(),
                     err = WEAVE_ERROR_PERSISTED_STORAGE_FAIL);

        persistentCASE.seekg(0, persistentCASE.end);
        uint16_t dataLen = persistentCASE.tellg();
        persistentCASE.seekg(0, persistentCASE.beg);

        VerifyOrExit(dataLen <= BUFF_AVAILABLE_SIZE,
                     err = WEAVE_ERROR_BUFFER_TOO_SMALL);

        persistentCASE.read((char*) buf, dataLen);
        // In case of failure, set failbit.
        persistentCASE.close();

        VerifyOrExit(!persistentCASE.fail(),
                     err = WEAVE_ERROR_PERSISTED_STORAGE_FAIL);

        // delete persist storage before restore session
        VerifyOrExit(std::remove(persistentTunnelSessionPath) == 0,
                     err = WEAVE_ERROR_PERSISTED_STORAGE_FAIL);

        con->AuthMode = kWeaveAuthModeCategory_CASE;
        err = FabricState.RestoreSession(buf, dataLen, con);
        SuccessOrExit(err);

        printf("Restored persistent tunnel CASE session successfully\n");
    }
    else
    {
        printf("Persistent tunnel CASE Session doesn't exist\n");
    }
exit:
    if (persistentCASE.is_open())
    {
        persistentCASE.close();
    }

    if (err != WEAVE_NO_ERROR)
    {
        printf("Restore Persistent CASE Session Failed with weave err: %d\n", err);
    }

    return err;
}

void HandleSessionPersistOnTunnelClosure(nl::Weave::WeaveConnection *con)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    err = SuspendAndPersistTunnelCASESession(con);

    if (err != WEAVE_NO_ERROR)
    {
        printf("Suspending and persisting Tunnel CASE Session failed with Weave error: %d\n", err);
    }
}

/* Send a tunnel control status report message */
WEAVE_ERROR SendStatusReportResponse(ExchangeContext *ec, uint32_t profileId, uint32_t tunStatusCode,
                                     bool isRoutingRestricted)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    StatusReport tunStatusReport;
    PacketBuffer *msgBuf = NULL;
    nl::Weave::TLV::TLVWriter tunWriter;
    nl::Weave::TLV::TLVType containerType;
    uint8_t *p = NULL;

    msgBuf = PacketBuffer::New();
    VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    p = msgBuf->Start();

    // Encode the profile id and status code.
    LittleEndian::Write32(p, profileId);
    LittleEndian::Write16(p, tunStatusCode);
    msgBuf->SetDataLength(4 + 2);

    if (isRoutingRestricted)
    {
        // Encode the Tunnel TLVData.
        tunWriter.Init(msgBuf);

        // Start the anonymous container that wraps the contents.
        err = tunWriter.StartContainer(AnonymousTag, kTLVType_Structure, containerType);
        SuccessOrExit(err);

        // Write the boolean tag
        err = tunWriter.PutBoolean(ProfileTag(kWeaveProfile_Tunneling, kTag_TunnelRoutingRestricted), true);
        SuccessOrExit(err);

        // End the anonymous container that wraps the contents.
        err = tunWriter.EndContainer(containerType);
        SuccessOrExit(err);

        err = tunWriter.Finalize();
        SuccessOrExit(err);
    }

    err = ec->SendMessage(kWeaveProfile_Common, Common::kMsgType_StatusReport, msgBuf, 0);
    msgBuf = NULL;

exit:
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);

    return err;
}

void HandleTunnelControlMsg (ExchangeContext *ec, const IPPacketInfo *pktInfo,
                             const WeaveMessageInfo *msgInfo, uint32_t profileId,
                             uint8_t msgType, PacketBuffer *payload)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveTunnelRoute tunRoute;
    uint64_t msgFabricId = 0;
    uint8_t *p = NULL;
    Role role;
    TunnelType tunnelType;
    SrcInterfaceType srcIntfType;
    LivenessStrategy livenessStrategy;
    uint16_t livenessTimeout;
    bool isRoutingRestricted = false;

    if (profileId == kWeaveProfile_Tunneling)
    {
        switch (msgType)
        {
            case kMsgType_TunnelOpenV2:
                //Decode the Tunnel Device Role, TunnelType and Source Interface
                p = payload->Start();

                role = static_cast<Role>(nl::Weave::Encoding::Read8(p));

                tunnelType = static_cast<TunnelType>(nl::Weave::Encoding::Read8(p));

                srcIntfType = static_cast<SrcInterfaceType>(nl::Weave::Encoding::Read8(p));

                livenessStrategy = static_cast<LivenessStrategy>(nl::Weave::Encoding::Read8(p));

                livenessTimeout = nl::Weave::Encoding::LittleEndian::Read16(p);

                WeaveLogDetail(WeaveTunnel, "Received TunOpenV2 message for Tunnel role :%u, type :%u, \
                                             srcIntf :%u, livenessStrategy :%u, livenessTimeout:%u\n",
                                            role, tunnelType, srcIntfType, livenessStrategy, livenessTimeout);

                // Set the buffer start pointer for the subsequent parsing of the fabric and routes
                payload->SetStart(p);

                //Save the routes and connection object
                memset(&tunRoute, 0, sizeof(tunRoute));
                err = WeaveTunnelRoute::DecodeFabricTunnelRoutes(&msgFabricId, &tunRoute, payload);
                SuccessOrExit(err);

                // Send a status report

                err = SendStatusReportResponse(ec, kWeaveProfile_Common, Common::kStatus_Success, isRoutingRestricted);
                SuccessOrExit(err);

                break;
            case kMsgType_TunnelRouteUpdate:

                // The reason this is not implemented yet is because for all practical purposes of developmental testing
                // we have not needed to modify the routes that were already sent with the TunnelOpen messages. However,
                // this message keeps that possibility open to modify the routes that have been sent before.

                // Send a status report

                err = SendStatusReportResponse(ec, kWeaveProfile_Common, Common::kStatus_Success);
                SuccessOrExit(err);

                break;
            case kMsgType_TunnelClose:

                err = WeaveTunnelRoute::DecodeFabricTunnelRoutes(&msgFabricId, &tunRoute, payload);
                SuccessOrExit(err);

                err = SendStatusReportResponse(ec, kWeaveProfile_Common, Common::kStatus_Success);
                SuccessOrExit(err);

                break;
            case kMsgType_TunnelLiveness:

                err = SendStatusReportResponse(ec, kWeaveProfile_Common, Common::kStatus_Success);
                SuccessOrExit(err);

                break;
        }
    }

exit:
    // Discard the exchange context.
    ec->Close();

    if (payload != NULL)
    {
        PacketBuffer::Free(payload);
    }
}

void HandleConnectionClosed (WeaveConnection *con, WEAVE_ERROR conErr)
{
    char ipAddrStr[64];

    con->PeerAddr.ToString(ipAddrStr, sizeof(ipAddrStr));

    if (conErr == WEAVE_NO_ERROR)
    {
        WeaveLogDetail(WeaveTunnel, "Connection closed with node %" PRIx64 " (%s)\n",
                       con->PeerNodeId, ipAddrStr);
    }
    else
    {
        WeaveLogError(WeaveTunnel, "Connection ABORTED with node %" PRIx64 " (%s): %ld\n",
                      con->PeerNodeId, ipAddrStr, (long)conErr);
    }

    con->Close();
}

void HandleSecureSessionEstablished (WeaveSecurityManager *sm, WeaveConnection *con,
                                     void *reqState, uint16_t sessionKeyId,
                                     uint64_t peerNodeId, uint8_t encType)
{
    char ipAddrStr[64] = "";

    if (con)
    {
        con->PeerAddr.ToString(ipAddrStr, sizeof(ipAddrStr));
        con->DefaultKeyId = sessionKeyId;
        con->PeerNodeId = peerNodeId;
    }
    WeaveLogDetail(WeaveTunnel, "Secure session established with node %" PRIX64 " (%s)\n", peerNodeId, ipAddrStr);
}

void HandleSecureSessionError (WeaveSecurityManager *sm, WeaveConnection *con,
                               void *reqState, WEAVE_ERROR localErr,
                               uint64_t peerNodeId, StatusReport *statusReport)
{
    char ipAddrStr[64] = "";

    if (con)
        con->PeerAddr.ToString(ipAddrStr, sizeof(ipAddrStr));

    if (localErr == WEAVE_ERROR_STATUS_REPORT_RECEIVED && statusReport != NULL)
        WeaveLogError(WeaveTunnel, "FAILED to establish secure session to node %" PRIX64 " (%s): %s\n", peerNodeId,
               ipAddrStr, StatusReportStr(statusReport->mProfileId, statusReport->mStatusCode));
    else
        WeaveLogDetail(WeaveTunnel, "FAILED to establish secure session to node %" PRIX64 " (%s): %s\n", peerNodeId,
               ipAddrStr, ErrorStr(localErr));
}

#endif //WEAVE_CONFIG_ENABLE_TUNNELING


int main(int argc, char *argv[])
{

#if WEAVE_CONFIG_ENABLE_TUNNELING
    WEAVE_ERROR err;
    nl::Weave::System::Stats::Snapshot before;
    nl::Weave::System::Stats::Snapshot after;
    const bool printStats = true;

    gWeaveNodeOptions.LocalNodeId = DEFAULT_TFE_NODE_ID;

    SetupFaultInjectionContext(argc, argv);
    SetSignalHandler(DoneOnHandleSIGUSR1);

    if (!ParseArgsFromEnvVar(TOOL_NAME, TOOL_OPTIONS_ENV_VAR_NAME, gToolOptionSets, NULL, true) ||
        !ParseArgs(TOOL_NAME, argc, argv, gToolOptionSets) ||
        !ResolveWeaveNetworkOptions(TOOL_NAME, gWeaveNodeOptions, gNetworkOptions))
    {
        exit(EXIT_FAILURE);
    }

    InitSystemLayer();
    InitNetwork();
    InitWeaveStack(true, true);

    FabricState.BoundConnectionClosedForSession = HandleSessionPersistOnTunnelClosure;

    WeaveLogDetail(WeaveTunnel, "Weave Node Configuration:\n");
    WeaveLogDetail(WeaveTunnel, "Fabric Id: %" PRIX64 "\n", FabricState.FabricId);
    WeaveLogDetail(WeaveTunnel, "Subnet Number: %X\n", FabricState.DefaultSubnet);
    WeaveLogDetail(WeaveTunnel, "Node Id: %" PRIX64 "\n", FabricState.LocalNodeId);

    nl::Weave::Stats::UpdateSnapshot(before);

    err = TunServerInit(&ExchangeMgr);
    FAIL_ERROR(err, "TunnelServer.Init failed");

    while (!Done)
    {
        struct timeval sleepTime;
        sleepTime.tv_sec = 0;
        sleepTime.tv_usec = 100000;

        ServiceNetwork(sleepTime);

    }

    TunServerShutdown();

    ProcessStats(before, after, printStats, NULL);
    PrintFaultInjectionCounters();

    ShutdownWeaveStack();
    ShutdownNetwork();
    ShutdownSystemLayer();

#endif //WEAVE_CONFIG_ENABLE_TUNNELING
    return 0;
}
