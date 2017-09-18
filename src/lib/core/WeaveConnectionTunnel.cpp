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
 *      This file implements the WeaveConnectionTunnel class to manage
 *      Weave communication tunneled between a pair of WeaveConnection
 *      objects that are coupled together.
 *
 */

#include <Weave/Core/WeaveCore.h>
#include <Weave/Support/logging/WeaveLogging.h>
#include <Weave/Support/CodeUtils.h>

namespace nl {
namespace Weave {

void WeaveConnectionTunnel::Init(WeaveMessageLayer *messageLayer)
{
    // Die if tunnel already initialized.
    VerifyOrDie(mMessageLayer == NULL);

    mMessageLayer = messageLayer;
}

WEAVE_ERROR WeaveConnectionTunnel::MakeTunnelConnected(TCPEndPoint *endPointOne, TCPEndPoint *endPointTwo)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(mMessageLayer != NULL && endPointOne != NULL && endPointTwo != NULL && endPointOne != endPointTwo,
            err = WEAVE_ERROR_INCORRECT_STATE);

    mEPOne = endPointOne;
    mEPTwo = endPointTwo;

    mEPOne->AppState = this;
    mEPTwo->AppState = this;

    // Hang callbacks on TCPEndPoints to set up data and connection closure forwarding between both ends of the tunnel.
    mEPOne->OnDataReceived = HandleTunnelDataReceived;
    mEPOne->OnConnectionClosed = HandleTunnelConnectionClosed;
    mEPOne->OnPeerClose = HandleReceiveShutdown;

    mEPTwo->OnDataReceived = HandleTunnelDataReceived;
    mEPTwo->OnConnectionClosed = HandleTunnelConnectionClosed;
    mEPTwo->OnPeerClose = HandleReceiveShutdown;

exit:
    return err;
}

void WeaveConnectionTunnel::CloseEndPoint(TCPEndPoint **endPoint)
{
    // Close and free specified TCPEndPoint.
    if (*endPoint != NULL)
    {
        if ((*endPoint)->Close() != WEAVE_NO_ERROR)
        {
            (*endPoint)->Abort();
        }

        (*endPoint)->Free();
        *endPoint = NULL;
    }
}

/**
 *  Shutdown the WeaveConnectionTunnel by closing the component endpoints which, in turn,
 *  close the corresponding TCP connections. This function terminates the tunnel and any
 *  further use of a WeaveConnectionTunnel needs to be initiated by a call to
 *  WeaveMessageLayer::NewConnectionTunnel();
 *
 */
void WeaveConnectionTunnel::Shutdown()
{
     WeaveLogProgress(ExchangeManager, "Shutting down tunnel %04X with EP (%04X, %04X)", LogId(),
            mEPOne->LogId(), mEPTwo->LogId());

    // Die if tunnel uninitialized.
    VerifyOrDie(mMessageLayer != NULL);

    CloseEndPoint(&mEPOne);
    CloseEndPoint(&mEPTwo);

    if (OnShutdown != NULL)
    {
        OnShutdown(this);
        OnShutdown = NULL;
    }

    mMessageLayer = NULL;
}

static void PrintTunnelInfo(const WeaveConnectionTunnel &tun, const TCPEndPoint &fromEndPoint, const TCPEndPoint &toEndPoint, const PacketBuffer &data)
{
    IPAddress addr;
    uint16_t port;
    WEAVE_ERROR err;
    char fromIpAddrStr[INET6_ADDRSTRLEN], toIpAddrStr[INET6_ADDRSTRLEN];

    err = fromEndPoint.GetPeerInfo(&addr, &port);
    SuccessOrExit(err);

    addr.ToString(fromIpAddrStr, sizeof(fromIpAddrStr));

    err = toEndPoint.GetPeerInfo(&addr, &port);
    SuccessOrExit(err);

    addr.ToString(toIpAddrStr, sizeof(toIpAddrStr));

    WeaveLogDetail(ExchangeManager, "Forwarding %u bytes on tunnel %04X from %s -> %s\n", data.DataLength(), tun.LogId(), fromIpAddrStr, toIpAddrStr);

exit:
    return;
}

void WeaveConnectionTunnel::HandleTunnelDataReceived(TCPEndPoint *fromEndPoint, PacketBuffer *data)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveConnectionTunnel *tun = (WeaveConnectionTunnel *)fromEndPoint->AppState;
    TCPEndPoint *toEndPoint;

    if (tun == NULL || (fromEndPoint != tun->mEPOne && fromEndPoint != tun->mEPTwo))
    {
        err = WEAVE_ERROR_INCORRECT_STATE;
        goto exit;
    }

    toEndPoint = fromEndPoint == tun->mEPOne ? tun->mEPTwo : tun->mEPOne;

    PrintTunnelInfo(*tun, *fromEndPoint, *toEndPoint, *data);

    // Misnomer - AckReceive doesn't explicitly ack anything, it just enlarges our TCP receive window.
    err = fromEndPoint->AckReceive(data->DataLength());
    SuccessOrExit(err);

    // Forward received data to other end of the tunnel.
    err = toEndPoint->Send(data, true);
    data = NULL;
    SuccessOrExit(err);

exit:
    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogError(ExchangeManager, "Err forwarding data on tunnel %04X, err = %d", (tun ? tun->LogId() : 0), err);

        if (data != NULL)
            PacketBuffer::Free(data);
    }
}

void WeaveConnectionTunnel::HandleTunnelConnectionClosed(TCPEndPoint *endPoint, INET_ERROR err)
{
    WeaveConnectionTunnel *tun = (WeaveConnectionTunnel *)endPoint->AppState;

    if (tun == NULL || (endPoint != tun->mEPOne && endPoint != tun->mEPTwo))
    {
        WeaveLogDetail(ExchangeManager, "Got tunnel endpoint closed with bad state");
        return;
    }

    // Close both ends of the tunnel and free this tunnel.
    tun->Shutdown();
}

void WeaveConnectionTunnel::HandleReceiveShutdown(TCPEndPoint *endPoint)
{
    WeaveConnectionTunnel *tun = (WeaveConnectionTunnel *)endPoint->AppState;

    if (tun == NULL)
    {
        WeaveLogDetail(ExchangeManager, "Null AppState in HandleReceiveShutdown");
        return;
    }

    WeaveLogProgress(ExchangeManager, "Forwarding half-closure on tunnel %04X from EP %04X", tun->LogId(),
            endPoint->LogId());

    // Reflect half-closure on other end of tunnel.
    if (endPoint == tun->mEPOne)
    {
        tun->mEPTwo->Shutdown();
    }
    else if (endPoint == tun->mEPTwo)
    {
        tun->mEPOne->Shutdown();
    }
    else
    {
        WeaveLogDetail(ExchangeManager, "Got half-close on tunnel %04X for unknown endpoint %04X", tun->LogId(),
                endPoint->LogId());
        return;
    }
}

} // namespace nl
} // namespace Weave
