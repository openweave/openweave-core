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
#define DEFAULT_TFE_NODE_ID 0xC0FFEE
#include <inttypes.h>
#include <unistd.h>

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

#define TOOL_NAME "TestWeaveTunnelServer"

#define TUNNEL_SERVICE_INTF "service-tun0"
#define TUNNEL_SERVICE_LL_ADDR "fe80::2"

using nl::StatusReportStr;
using namespace nl::Weave::Encoding;
using namespace nl::Weave::Profiles::WeaveTunnel;
using namespace nl::Weave::TLV;

static WEAVE_ERROR SendTunnelReconnectMessage(ExchangeContext *ec, const uint16_t port = WEAVE_PORT,
                                              const char *tunnelHostname = NULL, uint16_t hostLen = 0);
static WEAVE_ERROR VerifyAndParseStatusResponse(uint32_t profileId,
                                                uint8_t msgType, PacketBuffer *payload,
                                                StatusReport &outReport);
static WEAVE_ERROR SendStatusReportResponse(ExchangeContext *ec, uint32_t profileId, uint32_t tunStatusCode,
                                            bool isRoutingRestricted = false);

WeaveTunnelServer gTunServer;
WeaveEchoServer gEchoServer;

uint32_t gCurrTestNum = 0;
bool gReconnectSent = false;
bool gStatusReportSuppressed = false;

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

VirtualRouteTable::VirtualRouteTable(void)
{
    memset(RouteTable, 0, sizeof(RouteTable));
}

/* Lookup Route */
int VirtualRouteTable::FindRouteEntry (IPPrefix &ip6Route)
{
    int index = -1;
    for (int i = 0; i < SERVICE_ROUTE_TABLE_SIZE; i++)
    {
        if ((ip6Route == RouteTable[i].prefix))
        {
            index = i;
            break;
        }
    }

    return index;
}

/* Purge entries matching the connection */
void VirtualRouteTable::RemoveRouteEntryByConnection (WeaveConnection *con)
{
    for (int i = 0; i < SERVICE_ROUTE_TABLE_SIZE; i++)
    {
        if (con == RouteTable[i].outgoingCon[0])
        {
            RouteTable[i].outgoingCon[0] = NULL;
            RouteTable[i].priority[0] = 0;
        }
        else if (con == RouteTable[i].outgoingCon[1])
        {
            RouteTable[i].outgoingCon[1] = NULL;
            RouteTable[i].priority[1] = 0;
        }

        if (RouteTable[i].outgoingCon[0] == NULL &&
            RouteTable[i].outgoingCon[1] == NULL)
        {
            memset(&RouteTable[i].prefix, 0, sizeof(IPPrefix));
        }
    }

}

/* Create a new route entry */
int VirtualRouteTable::NewRouteEntry (void)
{
    int retIndex = -1;

    for (int i = 0; i < SERVICE_ROUTE_TABLE_SIZE; i++)
    {
        if (RouteTable[i].prefix.IsZero())
        {
            retIndex = i;
            break;
        }
    }

    return retIndex;
}

/* Free a route entry at a particular index */
WEAVE_ERROR VirtualRouteTable::FreeRouteEntry (int index)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if ((index < 0) || (index >= SERVICE_ROUTE_TABLE_SIZE))
    {
        ExitNow();
    }

    memset(&RouteTable[index].prefix, 0, sizeof(IPPrefix));

    RouteTable[index].routeState = kRouteEntryState_Invalid;
    memset(RouteTable[index].BorderGwList, 0, sizeof(RouteTable[index].BorderGwList));
    RouteTable[index].routeLifetime = INVALID_RT_LIFETIME;

exit:

    return err;
}

WeaveTunnelServer::WeaveTunnelServer()
{
    ExchangeMgr = NULL;
    memset((void *)vRouteDB.RouteTable, 0, sizeof(vRouteDB.RouteTable));
}

void WeaveTunnelServer::HandleConnectionReceived(WeaveMessageLayer *msgLayer, WeaveConnection *con)
{
    char ipAddrStr[64];
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    con->PeerAddr.ToString(ipAddrStr, sizeof(ipAddrStr));

    WeaveLogDetail(WeaveTunnel, "Connection received from node (%s)\n", ipAddrStr);

    con->OnConnectionClosed = HandleConnectionClosed;
    con->AppState = &gTunServer;

    err = gTunServer.mConTable.AddConnection(con);

    if (err != WEAVE_NO_ERROR ||
        gCurrTestNum == kTestNum_TestTunnelConnectionDownReconnect ||
        gCurrTestNum == kTestNum_TestTunnelResetReconnectBackoffImmediately ||
        gCurrTestNum == kTestNum_TestTunnelResetReconnectBackoffRandomized)
    {
        WeaveLogDetail(WeaveTunnel, "Closing Connection for test %d with node (%s)\n",
                       gCurrTestNum, ipAddrStr);

        gTunServer.mConTable.RemoveConnection(con);
        con->Close();
        con = NULL;
    }
}

WEAVE_ERROR WeaveTunnelServer::Init (WeaveExchangeManager *exchangeMgr)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    ExchangeMgr = exchangeMgr;

    MessageLayer.OnConnectionReceived = HandleConnectionReceived;
    MessageLayer.OnReceiveError = HandleMessageReceiveError;
    MessageLayer.OnAcceptError = HandleAcceptConnectionError;

    ExchangeMgr->RegisterUnsolicitedMessageHandler(kWeaveProfile_TunnelTest_Start,
                                                   HandleTunnelControlMsg,
                                                   this);
    ExchangeMgr->RegisterUnsolicitedMessageHandler(kWeaveProfile_TunnelTest_End,
                                                   HandleTunnelControlMsg,
                                                   this);
    ExchangeMgr->RegisterUnsolicitedMessageHandler(kWeaveProfile_TunnelTest_RequestTunnelConnDrop,
                                                   HandleTunnelControlMsg,
                                                   this);
    ExchangeMgr->RegisterUnsolicitedMessageHandler(kWeaveProfile_Tunneling,
                                                   kMsgType_TunnelOpenV2, HandleTunnelControlMsg,
                                                   this);
    ExchangeMgr->RegisterUnsolicitedMessageHandler(kWeaveProfile_Tunneling,
                                                   kMsgType_TunnelRouteUpdate, HandleTunnelControlMsg,
                                                   this);
    ExchangeMgr->RegisterUnsolicitedMessageHandler(kWeaveProfile_Tunneling,
                                                   kMsgType_TunnelClose, HandleTunnelControlMsg,
                                                   this);
    ExchangeMgr->RegisterUnsolicitedMessageHandler(kWeaveProfile_Tunneling,
                                                   kMsgType_TunnelLiveness, HandleTunnelControlMsg,
                                                   this);

    //Create Tunnel EndPoint and populate into member mTunEP
    err = CreateServiceTunEndPoint();
    SuccessOrExit(err);

    err = SetupServiceTunEndPoint();
    SuccessOrExit(err);

    //Register Recv function for TunEndPoint
    mTunEP->OnPacketReceived = RecvdFromServiceTunEndPoint;

    //Set the TunEndPoint appState to the WeaveTunnelServer.
    mTunEP->AppState = this;

    // Initialize the gEchoServer application.
    err = gEchoServer.Init(ExchangeMgr);
    FAIL_ERROR(err, "WeaveEchoServer.Init failed");

    // Arrange to get a callback whenever an Echo Request is received.
    gEchoServer.OnEchoRequestReceived = HandleEchoRequestReceived;

    SecurityMgr.OnSessionEstablished = HandleSecureSessionEstablished;
    SecurityMgr.OnSessionError = HandleSecureSessionError;

exit:

    return err;
}

WEAVE_ERROR WeaveTunnelServer::Shutdown (void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    CloseConnections();

    ExchangeMgr->UnregisterUnsolicitedMessageHandler(kWeaveProfile_Tunneling,
                                                   kMsgType_TunnelOpenV2);
    ExchangeMgr->UnregisterUnsolicitedMessageHandler(kWeaveProfile_Tunneling,
                                                   kMsgType_TunnelRouteUpdate);
    ExchangeMgr->UnregisterUnsolicitedMessageHandler(kWeaveProfile_Tunneling,
                                                   kMsgType_TunnelClose);
    ExchangeMgr->UnregisterUnsolicitedMessageHandler(kWeaveProfile_Tunneling,
                                                   kMsgType_TunnelLiveness);
    ExchangeMgr->UnregisterUnsolicitedMessageHandler(kWeaveProfile_TunnelTest_Start);
    ExchangeMgr->UnregisterUnsolicitedMessageHandler(kWeaveProfile_TunnelTest_End);
    ExchangeMgr->UnregisterUnsolicitedMessageHandler(kWeaveProfile_TunnelTest_RequestTunnelConnDrop);

    //Tear down the tun endpoint setup
    err = TeardownServiceTunEndPoint();

    gEchoServer.Shutdown();

    return err;
}

WEAVE_ERROR WeaveTunnelServer::ProcessIPv6Message (WeaveConnection *con, const WeaveMessageInfo *recvMsgInfo,
                                                   PacketBuffer *msg)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    char ipAddrStr[64];
    uint8_t *p = NULL;
    struct ip6_hdr *ip6hdr = NULL;
    WeaveTunnelHeader tunHeader;
    WeaveMessageInfo msgInfo;
    IPAddress destIP6Addr;
    IPAddress srcIP6Addr;
    IPPrefix  ip6Prefix;
    WeaveConnection *outgoingWeaveCon = NULL;
    int8_t index = -1;

    VerifyOrExit(con != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    p = msg->Start();
    ip6hdr = (struct ip6_hdr *)p;

    //Check destination address
#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    ip6_addr_t tempAddr6;
    ip6_addr_copy(tempAddr6, ip6hdr->dest);
    destIP6Addr = IPAddress::FromIPv6(tempAddr6);

    ip6_addr_copy(tempAddr6, ip6hdr->src);
    srcIP6Addr = IPAddress::FromIPv6(tempAddr6);
#else
    destIP6Addr = IPAddress::FromIPv6(ip6hdr->ip6_dst);
    srcIP6Addr  = IPAddress::FromIPv6(ip6hdr->ip6_src);
#endif

    //Prepare the msg header
    msgInfo.Clear();
    //Set message version to V2
    msgInfo.MessageVersion = kWeaveMessageVersion_V2;

    //Set the tunneling flag
    msgInfo.Flags |= kWeaveMessageFlag_TunneledData;

    if (destIP6Addr.Subnet() == kWeaveSubnetId_Service)
    {
        //Send down Tunnel Endpoint to the network stack to be
        //routed back up InetLayer to Weave.
        mTunEP->Send(msg);
        msg = NULL;
    }
    else
    {
        //Perform some sanity checks on the destination address
        if (!destIP6Addr.IsIPv6ULA())
        {
            ExitNow();
        }

        if (destIP6Addr.Subnet() != kWeaveSubnetId_MobileDevice &&
            destIP6Addr.Subnet() != kWeaveSubnetId_PrimaryWiFi &&
            destIP6Addr.Subnet() != kWeaveSubnetId_ThreadMesh)
        {
            WeaveLogError(WeaveTunnel, "Received packet's destination unknown. Discarding\n");
            ExitNow();
        }

        //Prepare IPPrefix for look-up in virtual route table
        if (destIP6Addr.Subnet() == kWeaveSubnetId_MobileDevice)
        {
            ip6Prefix.IPAddr = destIP6Addr;
            ip6Prefix.Length = NL_INET_IPV6_MAX_PREFIX_LEN;
        }
        else
        {
            ip6Prefix.IPAddr = IPAddress::MakeULA(destIP6Addr.GlobalId(), destIP6Addr.Subnet(), 0);
            ip6Prefix.Length = NL_INET_IPV6_DEFAULT_PREFIX_LEN;
        }

        //Lookup virtual table
        index = vRouteDB.FindRouteEntry(ip6Prefix);
        if (index >= 0)
        {
            //Ensure Reserved size
            msg->EnsureReservedSize(sizeof(tunHeader) + sizeof(msgInfo));
            //Set version to V1
            tunHeader.Version = kWeaveTunnelVersion_V1;

            err = WeaveTunnelHeader::EncodeTunnelHeader(&tunHeader, msg);
            SuccessOrExit(err);

            outgoingWeaveCon = GetOutgoingConn(index);

            outgoingWeaveCon->PeerAddr.ToString(ipAddrStr, sizeof(ipAddrStr));
            WeaveLogDetail(WeaveTunnel, "Received Message:Forwarding to node %" PRIX64 " (%s): len=%u.\n",
                    outgoingWeaveCon->PeerNodeId, ipAddrStr, msg->DataLength());

            //Encrypt message
            msgInfo.EncryptionType = vRouteDB.RouteTable[index].encryptionType;
            msgInfo.KeyId = vRouteDB.RouteTable[index].keyId;

            //Set the source and destination node ids;
            msgInfo.SourceNodeId = recvMsgInfo->DestNodeId;
            msgInfo.DestNodeId = outgoingWeaveCon->PeerNodeId;

            err = outgoingWeaveCon->SendTunneledMessage(&msgInfo, msg);
            msg = NULL;
        }
        else
        {
            WeaveLogDetail(WeaveTunnel, "No route to host\n");
            //Send No Route to host;
        }
    }

exit:
    if (msg != NULL)
    {
        PacketBuffer::Free(msg);
    }

    return err;

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

void WeaveTunnelServer::StoreGatewayInfoForPriority(WeaveConnection *conn, uint8_t rtIndex, uint8_t priorityIndex,
                                                    uint8_t priorityVal, const IPPacketInfo *pktInfo,
                                                    const WeaveMessageInfo *msgInfo)
{

    vRouteDB.RouteTable[rtIndex].priority[priorityIndex] = priorityVal;
    vRouteDB.RouteTable[rtIndex].outgoingCon[priorityIndex] = conn;
    //Set the Tunnel Data handler
    vRouteDB.RouteTable[rtIndex].outgoingCon[priorityIndex]->OnTunneledMessageReceived = HandleTunnelDataMessage;
    //Set the PeerNodeId in connection object
    vRouteDB.RouteTable[rtIndex].outgoingCon[priorityIndex]->PeerNodeId = msgInfo->SourceNodeId;
    vRouteDB.RouteTable[rtIndex].outgoingCon[priorityIndex]->PeerAddr = pktInfo->SrcAddress;
}

/**
 * Send a reconnect message to the border gateway.
 */
WEAVE_ERROR SendTunnelReconnectMessage(ExchangeContext *ec, const uint16_t port,
                                       const char *tunnelHostname, uint16_t hostLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    PacketBuffer *msg = NULL;
    uint8_t *p = NULL;

    msg = PacketBuffer::New();
    VerifyOrExit(msg, err = WEAVE_ERROR_NO_MEMORY);

    if (tunnelHostname)
    {
        p = msg->Start();
        // Add a new hostname and port for connection

        LittleEndian::Write16(p, port);

        memcpy(p, tunnelHostname, hostLen);

        msg->SetDataLength(p + hostLen - msg->Start());
    }

    // Send an Tunnel Reconnect message.

    err = ec->SendMessage(kWeaveProfile_Tunneling, kMsgType_TunnelReconnect, msg);
    msg = NULL;

exit:

    return err;
}

WEAVE_ERROR VerifyAndParseStatusResponse(uint32_t profileId,
                                         uint8_t msgType, PacketBuffer *payload,
                                         StatusReport &outReport)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // Verify that message is a StatusReport

    VerifyOrExit(profileId == kWeaveProfile_Common, err = WEAVE_ERROR_INVALID_PROFILE_ID);

    VerifyOrExit(msgType == Common::kMsgType_StatusReport, err = WEAVE_ERROR_INVALID_MESSAGE_TYPE);

    // Parse the StatusReport.

    err = StatusReport::parse(payload, outReport);
    SuccessOrExit(err);

exit:
    return err;
}

void WeaveTunnelServer::HandleReconnectResponse(ExchangeContext *ec, const IPPacketInfo *pktInfo,
                                                const WeaveMessageInfo *msgInfo, uint32_t profileId,
                                                uint8_t msgType, PacketBuffer *payload)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    StatusReport report;
    WeaveTunnelServer *tunServer = static_cast<WeaveTunnelServer *>(ec->AppState);

    err = VerifyAndParseStatusResponse(profileId, msgType, payload, report);
    SuccessOrExit(err);

    if (report.mProfileId == kWeaveProfile_Common && report.mStatusCode == Common::kStatus_Success)
    {
        // Received a Success Status report

        WeaveLogDetail(WeaveTunnel, "Received Status Success for TunnelReconnect message for test %d\n", gCurrTestNum);

    }
    else
    {
        err = WEAVE_ERROR_STATUS_REPORT_RECEIVED;
    }

exit:
    // Free the payload buffer.

    if (payload)
    {
        PacketBuffer::Free(payload);
    }

    // Drop the connection
    tunServer->CloseConnections();

    // Discard the exchange context.
    if (ec)
    {
        ec->Close();
        ec = NULL;
    }

    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogError(WeaveTunnel, "HandleReconnectResponse FAILED with error: %ld\n", (long)err);
    }

    return;

}

void WeaveTunnelServer::CloseConnections(void)
{
    WeaveConnection *con;

    printf("closing connections\n");

    for (int i = 0; i < CONNECTION_TABLE_SIZE; i++)
    {
        con = mConTable.mTable[i].mConnection;
        if (con)
        {
            vRouteDB.RemoveRouteEntryByConnection(con);
            con->Close();
            mConTable.mTable[i].mConnection = NULL;
        }
    }
}

void WeaveTunnelServer::HandleTunnelControlMsg (ExchangeContext *ec, const IPPacketInfo *pktInfo,
                                                const WeaveMessageInfo *msgInfo, uint32_t profileId,
                                                uint8_t msgType, PacketBuffer *payload)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveTunnelServer *tunServer = static_cast<WeaveTunnelServer *>(ec->AppState);
    WeaveTunnelRoute tunRoute;
    uint64_t msgFabricId = 0;
    uint8_t *p = NULL;
    int index = -1;
    Role role;
    TunnelType tunnelType;
    SrcInterfaceType srcIntfType;
    LivenessStrategy livenessStrategy;
    uint16_t livenessTimeout;
    bool isRoutingRestricted = false;
    ExchangeContext *exchangeCtx = NULL;

    VerifyOrExit(tunServer, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Test for WeaveTunnelTest Profile and Test messages

    if (profileId == kWeaveProfile_TunnelTest_Start)
    {
        gCurrTestNum = msgType;
        WeaveLogDetail(WeaveTunnel, "Received message for starting test %d\n", gCurrTestNum);
        if (gCurrTestNum == kTestNum_TestTunnelConnectionDownReconnect)
        {
            WeaveLogDetail(WeaveTunnel, "TestTunnelConnectionDownReconnect: closing connections if any is already open\n");
            gTunServer.CloseConnections();
        }
    }
    else if (profileId == kWeaveProfile_TunnelTest_End)
    {
        WeaveLogDetail(WeaveTunnel, "Received message for stopping test %d\n", gCurrTestNum);
        gCurrTestNum = 0;
    }
    else if (profileId == kWeaveProfile_TunnelTest_RequestTunnelConnDrop)
    {
        if (gCurrTestNum == kTestNum_TestQueueingOfTunneledPackets)
        {
            // Drop the connection
            gTunServer.CloseConnections();

        }
    }
    else if (profileId == kWeaveProfile_Tunneling)
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

                if (gCurrTestNum == kTestNum_TestTunnelNoStatusReportReconnect ||
                    gCurrTestNum == kTestNum_TestTunnelNoStatusReportResetReconnectBackoff)
                {
                    gStatusReportSuppressed = true;

                    WeaveLogDetail(WeaveTunnel, "Received TunOpenV2 message for test %d\n", gCurrTestNum);
                    ExitNow();
                }

                if (gCurrTestNum == kTestNum_TestTunnelErrorStatusReportReconnect)
                {
                    WeaveLogDetail(WeaveTunnel, "Sending error StatusReport message for test %d\n", gCurrTestNum);

                    SendStatusReportResponse(ec, kWeaveProfile_Common, Common::kStatus_UnexpectedMessage);
                    ExitNow();
                }

                if (gCurrTestNum == kTestNum_TestTunnelRestrictedRoutingOnTunnelOpen)
                {
                    isRoutingRestricted = true;
                }

                for (int i = 0; i < tunRoute.numOfPrefixes; i++)
                {
                    index = tunServer->vRouteDB.FindRouteEntry(tunRoute.tunnelRoutePrefix[i]);
                    if (index < 0)
                    {
                        //Not found; Create a new entry
                        index = tunServer->vRouteDB.NewRouteEntry();

                        //Fill in the details at the index
                        tunServer->vRouteDB.RouteTable[index].prefix = tunRoute.tunnelRoutePrefix[i];
                        tunServer->vRouteDB.RouteTable[index].fabricId = msgFabricId;
                        tunServer->vRouteDB.RouteTable[index].routeState = VirtualRouteTable::kRouteEntryState_Valid;

                        //Set encryption type and key id for the connection
                        tunServer->vRouteDB.RouteTable[index].keyId = msgInfo->KeyId;
                        tunServer->vRouteDB.RouteTable[index].encryptionType = msgInfo->EncryptionType;

                        tunServer->StoreGatewayInfoForPriority(ec->Con, index, 0, tunRoute.priority[i], pktInfo,
                                                                msgInfo);
                    }
                    else
                    {
                        // Route already exists
                        if (tunServer->vRouteDB.RouteTable[index].priority[0] == 0)
                        {
                            // Add a different priority entry

                            tunServer->StoreGatewayInfoForPriority(ec->Con, index, 0, tunRoute.priority[i], pktInfo,
                                                                    msgInfo);
                        }
                        else
                        {
                            tunServer->StoreGatewayInfoForPriority(ec->Con, index, 1, tunRoute.priority[i], pktInfo,
                                                                    msgInfo);
                        }
                    }
                }

                // Send a status report

                err = SendStatusReportResponse(ec, kWeaveProfile_Common, Common::kStatus_Success, isRoutingRestricted);
                SuccessOrExit(err);

                if (gCurrTestNum == kTestNum_TestReceiveReconnectFromService && !gReconnectSent)
                {
                    // Wait for a short while and then send a Tunnel Reconnect message

                    sleep(1); // Sleep for a second

                    // Create a new ExchangeContext

                    exchangeCtx = tunServer->ExchangeMgr->NewContext(ec->Con, tunServer);

                    VerifyOrExit(exchangeCtx != NULL, err = WEAVE_ERROR_NO_MEMORY);

                    // Assign the appropriate message receipt handler to the callback.

                    exchangeCtx->OnMessageReceived = HandleReconnectResponse;

                    err = SendTunnelReconnectMessage(exchangeCtx, WEAVE_PORT,
                                                     TEST_TUNNEL_RECONNECT_HOSTNAME,
                                                     strlen(TEST_TUNNEL_RECONNECT_HOSTNAME));
                    SuccessOrExit(err);

                    gReconnectSent = true;

                }

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
                if (gCurrTestNum == kTestNum_TestTunnelErrorStatusReportOnTunnelClose)
                {
                    WeaveLogDetail(WeaveTunnel, "Sending error StatusReport message for test %d\n", gCurrTestNum);

                    SendStatusReportResponse(ec, kWeaveProfile_Common, Common::kStatus_UnexpectedMessage);
                    ExitNow();
                }

                err = WeaveTunnelRoute::DecodeFabricTunnelRoutes(&msgFabricId, &tunRoute, payload);
                SuccessOrExit(err);

                err = SendStatusReportResponse(ec, kWeaveProfile_Common, Common::kStatus_Success);
                SuccessOrExit(err);

                break;
            case kMsgType_TunnelLiveness:
                if (gCurrTestNum == kTestNum_TestTunnelLivenessDisconnectOnNoResponse)
                {
                    WeaveLogDetail(WeaveTunnel, "Received Tunnel Liveness message for test %d\n", gCurrTestNum);
                    ExitNow();
                }

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

void WeaveTunnelServer::HandleTunnelDataMessage (WeaveConnection *con, const WeaveMessageInfo *recvMsgInfo,
                                                 PacketBuffer *msg)
{
    char ipAddrStr[64];
    WEAVE_ERROR err =  WEAVE_NO_ERROR;
    WeaveTunnelHeader tunHeader;
    WeaveTunnelServer *tunServer = static_cast<WeaveTunnelServer *>(con->AppState);
    //Decrypt payload

    //Decapsulate Tunnel header and metadata
    err = WeaveTunnelHeader::DecodeTunnelHeader(&tunHeader, msg);
    SuccessOrExit(err);

    con->PeerAddr.ToString(ipAddrStr, sizeof(ipAddrStr));

    WeaveLogDetail(WeaveTunnel, "Message from node %" PRIX64 " (%s): len=%u.\n",
           con->PeerNodeId, ipAddrStr, msg->DataLength());

    if (tunServer)
    {
        tunServer->ProcessIPv6Message(con, recvMsgInfo, msg);
    }

exit:

    return;
}

void WeaveTunnelServer::HandleConnectionClosed (WeaveConnection *con, WEAVE_ERROR conErr)
{
    char ipAddrStr[64];
    WeaveTunnelServer *tServer = static_cast<WeaveTunnelServer *>(con->AppState);

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

    if (tServer)
    {
        //Remove route table entry
        tServer->vRouteDB.RemoveRouteEntryByConnection(con);

        tServer->mConTable.RemoveConnection(con);
    }

    con->Close();
}

/* Create a new Tunnel endpoint */
WEAVE_ERROR WeaveTunnelServer::CreateServiceTunEndPoint (void)
{
    WEAVE_ERROR res = WEAVE_NO_ERROR;

    res = Inet.NewTunEndPoint(&mTunEP);
    SuccessOrExit(res);

    mTunEP->Init(&Inet);

exit:

    return res;
}

/* Setup the TunEndPoint interface and configure the link-local address and fabric default route */
WEAVE_ERROR WeaveTunnelServer::SetupServiceTunEndPoint (void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
#if !WEAVE_TUNNEL_CONFIG_WILL_OVERRIDE_ADDR_ROUTING_FUNCS
    IPPrefix prefix;
    uint64_t globalId = 0;
    IPAddress tunULAAddr;
#endif // WEAVE_TUNNEL_CONFIG_WILL_OVERRIDE_ADDR_ROUTING_FUNCS

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    err = mTunEP->Open();
#else
    err = mTunEP->Open(TUNNEL_SERVICE_INTF);
#endif
    SuccessOrExit(err);

    if (!mTunEP->IsInterfaceUp())
    {
        //Bring interface up
        err = mTunEP->InterfaceUp();
        SuccessOrExit(err);
    }

#if !WEAVE_TUNNEL_CONFIG_WILL_OVERRIDE_ADDR_ROUTING_FUNCS
    //Create prefix fd<globalId>::/48 to install route to tunnel interface
    globalId = WeaveFabricIdToIPv6GlobalId(ExchangeMgr->FabricState->FabricId);
    tunULAAddr = IPAddress::MakeULA(globalId, 0, 0);

    prefix.IPAddr = tunULAAddr;
    prefix.Length = 48;

    //Add route to tunnel interface
    err = SetRouteToTunnelInterface(mTunEP->GetTunnelInterfaceId(), prefix, TunEndPoint::kRouteTunIntf_Add);
    SuccessOrExit(err);
#endif // WEAVE_TUNNEL_CONFIG_WILL_OVERRIDE_ADDR_ROUTING_FUNCS

exit:
    if (err != WEAVE_NO_ERROR)
    {
        mTunEP->Free();
        mTunEP = NULL;
    }

    return err;
}

/* Tear down TunEndpoint interface and remove the link-local address and fabric default route */
WEAVE_ERROR WeaveTunnelServer::TeardownServiceTunEndPoint (void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

#if !WEAVE_TUNNEL_CONFIG_WILL_OVERRIDE_ADDR_ROUTING_FUNCS
    uint64_t globalId = 0;
    IPAddress tunULAAddr;
    IPPrefix prefix;

    //Delete route to tunnel interface for prefix fd<globalId>::/48
    globalId = WeaveFabricIdToIPv6GlobalId(ExchangeMgr->FabricState->FabricId);
    tunULAAddr = IPAddress::MakeULA(globalId, 0, 0);

    prefix.IPAddr = tunULAAddr;
    prefix.Length = 48;
    //Delete route
    err = SetRouteToTunnelInterface(mTunEP->GetTunnelInterfaceId(), prefix, TunEndPoint::kRouteTunIntf_Del);
    SuccessOrExit(err);
#endif // WEAVE_TUNNEL_CONFIG_WILL_OVERRIDE_ADDR_ROUTING_FUNCS

    if (mTunEP->IsInterfaceUp())
    {
        //Bring interface down
        err = mTunEP->InterfaceDown();
        SuccessOrExit(err);
    }

exit:
    //Free Tunnel Endpoint
    if (mTunEP != NULL)
    {
        mTunEP->Free();
        mTunEP = NULL;
    }

    return err;
}

void WeaveTunnelServer::RecvdFromServiceTunEndPoint (TunEndPoint *tunEP, PacketBuffer *msg)
{
    WEAVE_ERROR err        = WEAVE_NO_ERROR;
    uint8_t     *p         = NULL;
    struct ip6_hdr *ip6hdr = NULL;
    WeaveTunnelHeader tunHeader;
    WeaveMessageInfo msgInfo;
    IPAddress destIP6Addr;
    IPPrefix  ip6Prefix;
    int8_t index = -1;
    WeaveConnection *outgoingWeaveCon = NULL;
#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    ip6_addr_t tempAddr6;
#endif
    WeaveTunnelServer *tServer = static_cast<WeaveTunnelServer *>(tunEP->AppState);

    //Extract the IPv6 header to look at the destination address
    p = msg->Start();
    ip6hdr = (struct ip6_hdr *)p;

    //Check destination address
#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    ip6_addr_copy(tempAddr6, ip6hdr->dest);
    destIP6Addr = IPAddress::FromIPv6(tempAddr6);
#else
    destIP6Addr = IPAddress::FromIPv6(ip6hdr->ip6_dst);
#endif

    if ((destIP6Addr.Subnet() == kWeaveSubnetId_PrimaryWiFi) ||
        (destIP6Addr.Subnet() == kWeaveSubnetId_ThreadMesh))
    {
        //Prepare the msg header
        msgInfo.Clear();
        //Set message version to V2
        msgInfo.MessageVersion = kWeaveMessageVersion_V2;

        //Ensure Reserved size
        msg->EnsureReservedSize(sizeof(tunHeader) + sizeof(msgInfo));

        //Set version to V1
        tunHeader.Version = kWeaveTunnelVersion_V1;

        err = WeaveTunnelHeader::EncodeTunnelHeader(&tunHeader, msg);
        SuccessOrExit(err);

        //Prepare prefix for route table lookup
        ip6Prefix.IPAddr = IPAddress::MakeULA(destIP6Addr.GlobalId(), destIP6Addr.Subnet(), 0);
        ip6Prefix.Length = NL_INET_IPV6_DEFAULT_PREFIX_LEN;

        index = tServer->vRouteDB.FindRouteEntry(ip6Prefix);
        if (index >= 0)
        {
            //Encrypt message
            msgInfo.EncryptionType = tServer->vRouteDB.RouteTable[index].encryptionType;
            msgInfo.KeyId = tServer->vRouteDB.RouteTable[index].keyId;

            outgoingWeaveCon = tServer->GetOutgoingConn(index);
            if (outgoingWeaveCon)
            {
                //Set the source and destination node ids; Interchange and copy from recvd msgInfo
                msgInfo.SourceNodeId = tServer->ExchangeMgr->FabricState->LocalNodeId;
                msgInfo.DestNodeId = outgoingWeaveCon->PeerNodeId;

                //Send over TCP Connection
                err = outgoingWeaveCon->SendTunneledMessage(&msgInfo, msg);
                msg = NULL;
                SuccessOrExit(err);
            }
            else
            {
                printf("No appropriate outgoing connection found; Discarding message\n");
            }
        }
        else
        {
            printf("No entry in route table for connection\n");
        }
    }

exit:
    if (msg != NULL)
    {
        PacketBuffer::Free(msg);
        msg = NULL;
    }

    return;
}

WeaveConnection * WeaveTunnelServer::GetOutgoingConn(uint8_t index)
{
    WeaveConnection *outgoingCon = NULL;

    VirtualRouteTable::RouteEntry rtEntry = vRouteDB.RouteTable[index];

    if (rtEntry.outgoingCon[0] && !rtEntry.outgoingCon[1])
    {
        outgoingCon = rtEntry.outgoingCon[0];
    }
    else if (rtEntry.outgoingCon[1] && !rtEntry.outgoingCon[0])
    {
        outgoingCon = rtEntry.outgoingCon[1];
    }
    else if (rtEntry.priority[0] < rtEntry.priority[1])
    {
        outgoingCon = rtEntry.outgoingCon[0];
    }
    else
    {
        outgoingCon = rtEntry.outgoingCon[1];
    }

    return outgoingCon;
}

void WeaveTunnelServer::HandleEchoRequestReceived (uint64_t nodeId, IPAddress nodeAddr,
                                                   PacketBuffer *payload)
{
    char ipAddrStr[64];
    nodeAddr.ToString(ipAddrStr, sizeof(ipAddrStr));

    WeaveLogDetail(WeaveTunnel, "Echo Request from node %" PRIX64 " (%s): len=%u ... sending response.\n", nodeId, ipAddrStr,
            payload->DataLength());
}

void WeaveTunnelServer::HandleSecureSessionEstablished (WeaveSecurityManager *sm, WeaveConnection *con,
                                                        void *reqState, uint16_t sessionKeyId,
                                                        uint64_t peerNodeId, uint8_t encType)
{
    char ipAddrStr[64] = "";

    if (con)
        con->PeerAddr.ToString(ipAddrStr, sizeof(ipAddrStr));

    WeaveLogDetail(WeaveTunnel, "Secure session established with node %" PRIX64 " (%s)\n", peerNodeId, ipAddrStr);
}

void WeaveTunnelServer::HandleSecureSessionError (WeaveSecurityManager *sm, WeaveConnection *con,
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

ConnectionTable::ConnectionTable(void)
{
    memset(&mTable, 0, sizeof(mTable));
}

WEAVE_ERROR ConnectionTable::AddConnection(WeaveConnection *aCon)
{
    size_t i;
    size_t freeEntry = CONNECTION_TABLE_SIZE;
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    for (i = 0; i < CONNECTION_TABLE_SIZE; i++)
    {
        // If the entry exists already, exit and return success
        VerifyOrExit(mTable[i].mConnection != aCon, /* no-op */);

        if (freeEntry == CONNECTION_TABLE_SIZE &&
                mTable[i].mConnection == NULL)
        {
            freeEntry = i;
        }
    }

    VerifyOrExit(freeEntry < CONNECTION_TABLE_SIZE, err = WEAVE_ERROR_NO_MEMORY);

    mTable[freeEntry].mConnection = aCon;

exit:
    return err;
}

void ConnectionTable::RemoveConnection(WeaveConnection *aCon)
{
    size_t i;

    for (i = 0; i < CONNECTION_TABLE_SIZE; i++)
    {
        if (mTable[i].mConnection == aCon)
        {
            memset(&(mTable[i]), 0, sizeof(mTable[i]));
            break;
        }
    }
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
        !ParseArgs(TOOL_NAME, argc, argv, gToolOptionSets))
    {
        exit(EXIT_FAILURE);
    }

    if (gNetworkOptions.LocalIPv6Addr != IPAddress::Any)
    {
        if (!gNetworkOptions.LocalIPv6Addr.IsIPv6ULA())
        {
            WeaveLogDetail(WeaveTunnel, "ERROR: Local address must be an IPv6 ULA\n");
            exit(-1);
        }

        gWeaveNodeOptions.FabricId = gNetworkOptions.LocalIPv6Addr.GlobalId();
        gWeaveNodeOptions.LocalNodeId = IPv6InterfaceIdToWeaveNodeId(gNetworkOptions.LocalIPv6Addr.InterfaceId());
        gWeaveNodeOptions.SubnetId = gNetworkOptions.LocalIPv6Addr.Subnet();
    }

    InitSystemLayer();
    InitNetwork();
    InitWeaveStack(true, true);

    WeaveLogDetail(WeaveTunnel, "Weave Node Configuration:\n");
    WeaveLogDetail(WeaveTunnel, "Fabric Id: %" PRIX64 "\n", FabricState.FabricId);
    WeaveLogDetail(WeaveTunnel, "Subnet Number: %X\n", FabricState.DefaultSubnet);
    WeaveLogDetail(WeaveTunnel, "Node Id: %" PRIX64 "\n", FabricState.LocalNodeId);

    nl::Weave::Stats::UpdateSnapshot(before);

    err = gTunServer.Init(&ExchangeMgr);
    FAIL_ERROR(err, "TunnelServer.Init failed");

    while (!Done)
    {
        struct timeval sleepTime;
        sleepTime.tv_sec = 0;
        sleepTime.tv_usec = 100000;

        ServiceNetwork(sleepTime);

    }

    gTunServer.Shutdown();

    ProcessStats(before, after, printStats, NULL);
    PrintFaultInjectionCounters();

    ShutdownWeaveStack();
    ShutdownNetwork();
    ShutdownSystemLayer();

#endif //WEAVE_CONFIG_ENABLE_TUNNELING
    return 0;
}
