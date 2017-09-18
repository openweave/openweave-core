/*
 *
 *    Copyright (c) 2017 Nest Labs, Inc.
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
 *      This file implements a weave-echo-requester, for the
 *      Weave Echo Profile.
 *
 *      The Weave Echo Profile implements two simple methods, in the
 *      style of ICMP ECHO REQUEST and ECHO REPLY, in which a sent
 *      payload is turned around by the responder and echoed back to
 *      the originator.
 *
 */

#include "weave-app-common.h"
#include <Weave/Profiles/echo/WeaveEcho.h>
#include <Weave/Support/ErrorStr.h>

// Max number of times, the client will try to connect to the server.
#define MAX_CONNECT_ATTEMPTS      (5)

// Max value for the number of EchoRequests sent.
#define MAX_ECHO_COUNT            (3)

using namespace nl::Inet;
using namespace nl::Weave;
using namespace nl::Weave::Profiles;

// Callback handler when a Weave EchoResponse is received.
static void HandleEchoResponseReceived(uint64_t nodeId, IPAddress nodeAddr, PacketBuffer *payload);

// Callback handler when a connection attempt has been completed
static void HandleConnectionComplete(WeaveConnection *con, WEAVE_ERROR conErr);

// Callback handler when the TCP connection is closed by the peer.
static void HandleConnectionClosed(WeaveConnection *con, WEAVE_ERROR conErr);

// The number of connection attempts performed by this Weave EchoClient.
static int32_t gConnectAttempts = 0;

// The Weave Echo interval time in microseconds.
static int32_t gEchoInterval = 1000000;

// The last time a Weave Echo was attempted to be sent.
static uint64_t gLastEchoTime = 0;

// True, if the EchoClient is waiting for an EchoResponse
// after sending an EchoRequest, false otherwise.
static bool gWaitingForEchoResp = false;

// Count of the number of EchoRequests sent.
static uint64_t gEchoCount = 0;

// Count of the number of EchoResponses received.
static uint64_t gEchoRespCount = 0;

// The WeaveEchoClient object.
static WeaveEchoClient gEchoClient;

// A pointer to a Weave TCP connection object.
static WeaveConnection *gCon = NULL;

// True, if client is still connecting to the server, false otherwise.
static bool gClientConInProgress = false;

// True, once client connection to server is established.
static bool gClientConEstablished = false;

static void StartClientConnection();
static void CloseClientConnection();
static void SendEchoRequest();
static bool EchoIntervalExpired();
static void FormulateEchoRequestBuffer(PacketBuffer*& payloadBuf);

bool EchoIntervalExpired(void)
{
    if (Now() >= gLastEchoTime + gEchoInterval)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void FormulateEchoRequestBuffer(PacketBuffer*& payloadBuf)
{
    payloadBuf = PacketBuffer::New();
    if (payloadBuf == NULL)
    {
        printf("Unable to allocate PacketBuffer\n");
        return;
    }

    // Add some application payload data in the buffer.
    char *p = (char *) payloadBuf->Start();
    int32_t len = sprintf(p, "Echo Message %" PRIu64 "\n", gEchoCount);

    // Set the datalength in the buffer appropriately.
    payloadBuf->SetDataLength((uint16_t) len);

}

void SendEchoRequest(void)
{
    WEAVE_ERROR err;
    PacketBuffer *payloadBuf = NULL;

    gLastEchoTime = Now();

    FormulateEchoRequestBuffer(payloadBuf);
    if (payloadBuf == NULL)
    {
        return;
    }

    if (gClientConEstablished)
    {
        err = gEchoClient.SendEchoRequest(gCon, payloadBuf);
        // Set the local buffer to NULL after passing it down to
        // the lower layers who are now responsible for freeing
        // the buffer.
        payloadBuf = NULL;

        if (err == WEAVE_NO_ERROR)
        {
            gWaitingForEchoResp = true;
            gEchoCount++;
        }
        else
        {
            printf("WeaveEchoClient.SendEchoRequest() failed, err: %s\n", nl::ErrorStr(err));
        }
    }
}

void HandleEchoResponseReceived(uint64_t nodeId, IPAddress nodeAddr, PacketBuffer *payload)
{
    uint32_t respTime = Now();
    uint32_t transitTime = respTime - gLastEchoTime;

    gWaitingForEchoResp = false;
    gEchoRespCount++;

    char msgNodeAddrStr[WEAVE_MAX_MESSAGE_SOURCE_STR_LENGTH];

    WeaveNodeAddrToStr(msgNodeAddrStr, sizeof(msgNodeAddrStr) - 1, nodeId, &nodeAddr, gDestPort, gCon);

    printf("Echo Response from node %s : %" PRIu64 "/%" PRIu64 "(%.2f%%) len=%u time=%.3fms\n", msgNodeAddrStr,
            gEchoRespCount, gEchoCount, ((double) gEchoRespCount) * 100 / gEchoCount, payload->DataLength(),
            ((double) transitTime) / 1000);
}

void StartClientConnection()
{
    WEAVE_ERROR err;

    // Previous connection attempt underway.
    if (gClientConInProgress)
        return;

    gClientConEstablished = false;

    gCon = MessageLayer.NewConnection();
    if (gCon == NULL)
    {
        printf("MessageLayer.NewConnection failed, err: %s\n", nl::ErrorStr(WEAVE_ERROR_NO_MEMORY));
        gLastEchoTime = Now();
        return;
    }

    gCon->OnConnectionComplete = HandleConnectionComplete;
    gCon->OnConnectionClosed = HandleConnectionClosed;

    // Attempt to connect to the peer.
    err = gCon->Connect(gDestNodeId, gDestv6Addr, gDestPort);
    if (err != WEAVE_NO_ERROR)
    {
        printf("WeaveConnection.Connect failed, err: %s\n", nl::ErrorStr(err));
        gLastEchoTime = Now();
        CloseClientConnection();
        gConnectAttempts++;
        return;
    }

    gClientConInProgress = true;
}


void CloseClientConnection(void)
{
    if (gCon)
    {
        gCon->Close();
        gCon = NULL;
        printf("Connection closed\n");
    }
    gClientConEstablished = false;
    gClientConInProgress = false;
}

void HandleConnectionComplete(WeaveConnection *con, WEAVE_ERROR conErr)
{
    char msgNodeAddrStr[WEAVE_MAX_MESSAGE_SOURCE_STR_LENGTH];

    WeaveNodeAddrToStr(msgNodeAddrStr, sizeof(msgNodeAddrStr) - 1, con->PeerNodeId, &con->PeerAddr, gDestPort, con);

    if (conErr != WEAVE_NO_ERROR)
    {
        printf("Connection FAILED to node %s, err: %s\n", msgNodeAddrStr, nl::ErrorStr(conErr));
        gLastEchoTime = Now();
        CloseClientConnection();
        gConnectAttempts++;
        return;
    }

    printf("Connection established to node %s\n", msgNodeAddrStr);

    gCon = con;

    con->OnConnectionClosed = HandleConnectionClosed;

    gClientConEstablished = true;
    gClientConInProgress = false;
}

void HandleConnectionClosed(WeaveConnection *con, WEAVE_ERROR conErr)
{
    char msgNodeAddrStr[WEAVE_MAX_MESSAGE_SOURCE_STR_LENGTH];

    WeaveNodeAddrToStr(msgNodeAddrStr, sizeof(msgNodeAddrStr) - 1, con->PeerNodeId, &con->PeerAddr, gDestPort, con);

    if (conErr == WEAVE_NO_ERROR)
        printf("Connection closed to node %s\n", msgNodeAddrStr);
    else
        printf("Connection ABORTED to node %s, err: %s\n", msgNodeAddrStr, nl::ErrorStr(conErr));

    gWaitingForEchoResp = false;

    if (con == gCon)
    {
        CloseClientConnection();
    }
}

int main(int argc, char *argv[])
{
    WEAVE_ERROR err;

    // Assign local IPv6 address.

    IPAddress::FromString("fd00:0:1:1::1", gLocalv6Addr);

    // Initialize the Weave stack as the client.

    InitializeWeave(false);

    // Initialize the EchoClient application.
    err = gEchoClient.Init(&ExchangeMgr);
    if (err != WEAVE_NO_ERROR)
    {
        printf("WeaveEchoClient.Init failed: %s\n", nl::ErrorStr(err));
        exit(EXIT_FAILURE);
    }

    // Arrange to get a callback whenever an Echo Response is received.
    gEchoClient.OnEchoResponseReceived = HandleEchoResponseReceived;

    // Set the destination fields before initiating the Weave connection.

    IPAddress::FromString("fd00:0:1:1::2", gDestv6Addr);

    // Derive the destination node id from the IPv6 address.
    gDestNodeId = IPv6InterfaceIdToWeaveNodeId(gDestv6Addr.InterfaceId());

    // Set the destination port to be the Weave port.
    gDestPort = WEAVE_PORT;

    // Wait until the Weave connection is established.
    while (!gClientConEstablished)
    {
        // Start the Weave connection to the weave echo responder.
        StartClientConnection();

        DriveIO();

        if (gConnectAttempts > MAX_CONNECT_ATTEMPTS)
        {
            exit(EXIT_FAILURE);
        }
    }

    // Connection has been established. Now send the EchoRequests.
    for (int i = 0; i < MAX_ECHO_COUNT; i++)
    {
        // Send an EchoRequest message.
        SendEchoRequest();

        // Wait for response until the Echo interval.
        while (!EchoIntervalExpired())
        {
            DriveIO();

            fflush(stdout);
        }

        // Check if expected response was received.
        if (gWaitingForEchoResp)
        {
            printf("No response received\n");

            gWaitingForEchoResp = false;
        }
    }

    // Done with all EchoRequests and EchoResponses.
    CloseClientConnection();

    // Shutdown the EchoClient.
    gEchoClient.Shutdown();

    // Shutdown the Weave stack.
    ShutdownWeave();

    return EXIT_SUCCESS;
}
