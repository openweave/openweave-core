/*
 *
 *    Copyright (c) 2013-2017 Nest Labs, Inc.
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
 *      This file implements a weave-echo-responder, for the
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

using namespace nl::Inet;
using namespace nl::Weave;
using namespace nl::Weave::Profiles;

// The WeaveEchoServer object.
static WeaveEchoServer gEchoServer;

static bool TestDone = false;

// Callback handler when a Weave EchoRequest is received.
static void HandleEchoRequestReceived(uint64_t nodeId, IPAddress nodeAddr, PacketBuffer *payload);

// Callback handler when a TCP connection request is received.
static void HandleConnectionReceived(WeaveMessageLayer *msgLayer, WeaveConnection *con);

// Callback handler when the TCP connection is closed by the peer.
static void HandleConnectionClosed(WeaveConnection *con, WEAVE_ERROR conErr);


void HandleEchoRequestReceived(uint64_t nodeId, IPAddress nodeAddr, PacketBuffer *payload)
{
    char msgNodeAddrStr[WEAVE_MAX_MESSAGE_SOURCE_STR_LENGTH];

    WeaveNodeAddrToStr(msgNodeAddrStr, sizeof(msgNodeAddrStr) - 1, nodeId, &nodeAddr, 0, NULL);

    printf("Echo Request from node %s, len=%u ... sending response.\n", msgNodeAddrStr,
            payload->DataLength());
}

void HandleConnectionReceived(WeaveMessageLayer *msgLayer, WeaveConnection *con)
{
    char msgNodeAddrStr[WEAVE_MAX_MESSAGE_SOURCE_STR_LENGTH];

    WeaveNodeAddrToStr(msgNodeAddrStr, sizeof(msgNodeAddrStr) - 1, con->PeerNodeId, &con->PeerAddr, 0, con);

    printf("Connection received from node %s\n", msgNodeAddrStr);

    con->OnConnectionClosed = HandleConnectionClosed;
}

void HandleConnectionClosed(WeaveConnection *con, WEAVE_ERROR conErr)
{
    char msgNodeAddrStr[WEAVE_MAX_MESSAGE_SOURCE_STR_LENGTH];

    WeaveNodeAddrToStr(msgNodeAddrStr, sizeof(msgNodeAddrStr) - 1, con->PeerNodeId, &con->PeerAddr, 0, con);

    if (conErr == WEAVE_NO_ERROR)
        printf("Connection closed to node %s\n", msgNodeAddrStr);
    else
        printf("Connection ABORTED to node %s, err: %s\n", msgNodeAddrStr, nl::ErrorStr(conErr));

    con->Close();
}

int main(int argc, char *argv[])
{
    WEAVE_ERROR err;

    // Assign local IPv6 address.

    IPAddress::FromString("fd00:0:1:1::2", gLocalv6Addr);

    // Initialize the Weave stack.

    InitializeWeave(true);

    // Set the callback handler for handling received connections.
    MessageLayer.OnConnectionReceived = HandleConnectionReceived;

    // Initialize the EchoServer application.

    err = gEchoServer.Init(&ExchangeMgr);
    if (err)
    {
        printf("WeaveEchoServer.Init failed, err:%s\n", nl::ErrorStr(err));
        exit(EXIT_FAILURE);
    }

    // Arrange to get a callback whenever an Echo Request is received.
    gEchoServer.OnEchoRequestReceived = HandleEchoRequestReceived;

    printf("Listening for Echo requests...\n");

    while (!TestDone)
    {
        DriveIO();

        fflush(stdout);
    }

    // Shutdown the Weave EchoServer.

    gEchoServer.Shutdown();

    // Shutdown the Weave stack.

    ShutdownWeave();

    return EXIT_SUCCESS;
}
