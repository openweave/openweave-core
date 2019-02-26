/*
 *
 *    Copyright (c) 2019 Google LLC.
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
 *      This file implements the Device Control Profile, which provides
 *      operations and utilities used during device setup and provisioning.
 *
 *      The Device Control Profile facilitates client-server operations such
 *      that the client (the controlling device) can trigger specific utility
 *      functionality on the server (the device undergoing setup) to assist with
 *      and enable the device setup and provisioning process.  This includes, for
 *      example, resetting the server device's configuration and enabling fail
 *      safes that define the behavior when the setup procedure is prematurely
 *      aborted.
 *
 */
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif // __STDC_FORMAT_MACROS

#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Support/CodeUtils.h>
#include <Weave/Core/WeaveEncoding.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include "DeviceControl.h"
#include <Weave/Profiles/common/CommonProfile.h>
#include <Weave/Profiles/echo/WeaveEcho.h>
#include <Weave/Profiles/device-description/DeviceDescription.h>
#include <Weave/Support/logging/WeaveLogging.h>
#include <Weave/Support/TimeUtils.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace DeviceControl {


using namespace nl::Weave::Crypto;
using namespace nl::Weave::Encoding;
using namespace nl::Weave::TLV;
using namespace nl::Weave::Profiles::DeviceDescription;

DeviceControlServer *DeviceControlServer::sRemotePassiveRendezvousServer = NULL;

DeviceControlServer::DeviceControlServer()
{
    FabricState = NULL;
    ExchangeMgr = NULL;
    mCurClientOp = NULL;
    mDelegate = NULL;
    mRemotePassiveRendezvousOp = NULL;
    mFailSafeToken = 0;
    mFailSafeArmed = false;
    mResetFlags = 0x0000;
    mRemotePassiveRendezvousClientCon = NULL;
    mRemotePassiveRendezvousJoinerCon = NULL;
    mRemotePassiveRendezvousTunnel = NULL;
    mRemotePassiveRendezvousTimeout = 0;
    mTunnelInactivityTimeout = 0;
    mRemotePassiveRendezvousKeyId = 0;
    mRemotePassiveRendezvousEncryptionType = 0;
}

/**
 * Initialize the Device Control Server state and register to receive
 * Device Control messages.
 *
 * @param[in]    exchangeMgr    A pointer to the Weave Exchange Manager.
 *
 * @retval  #WEAVE_ERROR_INCORRECT_STATE                        When a remote passive rendezvous server
 *                                                              has already been registered.
 * @retval  #WEAVE_ERROR_TOO_MANY_UNSOLICITED_MESSAGE_HANDLERS  When too many unsolicited message
 *                                                              handlers are registered.
 * @retval  #WEAVE_NO_ERROR                                     On success.
 */
WEAVE_ERROR DeviceControlServer::Init(WeaveExchangeManager *exchangeMgr)
{
    FabricState = exchangeMgr->FabricState;
    ExchangeMgr = exchangeMgr;
    mCurClientOp = NULL;
    mFailSafeToken = 0;
    mFailSafeArmed = false;
    mResetFlags = 0x0000;

    // Global used here, as used in DeviceManager, to get app state in WeaveMessageLayer::HandleConnectionReceived.
    if (sRemotePassiveRendezvousServer != NULL)
        return WEAVE_ERROR_INCORRECT_STATE;

    sRemotePassiveRendezvousServer = this;

    // Register to receive unsolicited Device Control messages from the exchange manager.
    WEAVE_ERROR err =
        ExchangeMgr->RegisterUnsolicitedMessageHandler(kWeaveProfile_DeviceControl, HandleClientRequest, this);

    return err;
}

/**
 * Shutdown the Device Control Server.
 *
 * @retval #WEAVE_NO_ERROR unconditionally.
 */
// TODO: Additional documentation detail required (i.e. how this function impacts object lifecycle).
WEAVE_ERROR DeviceControlServer::Shutdown()
{
    if (ExchangeMgr != NULL)
        ExchangeMgr->UnregisterUnsolicitedMessageHandler(kWeaveProfile_DeviceControl);

    CloseClientOp();

    FabricState = NULL;
    ExchangeMgr = NULL;
    mFailSafeToken = 0;
    mFailSafeArmed = false;
    mResetFlags = 0x0000;

    // Kill any pending or completed Remote Passive Rendezvous.
    CloseRemotePassiveRendezvous();

    return WEAVE_NO_ERROR;
}

/**
 * Set the delegate to process Device Control Server events.
 *
 * @param[in]   delegate    A pointer to the Device Control Delegate.
 */
void DeviceControlServer::SetDelegate(DeviceControlDelegate *delegate)
{
    mDelegate = delegate;
}

/**
 * Return Remote Passive Rendezvous state.
 *
 * @retval TRUE if Remote Passive Rendezvous in progress.
 * @retval FALSE if Remote Passive Rendezvous is not in progress.
 */
bool DeviceControlServer::IsRemotePassiveRendezvousInProgress() const
{
    return (mRemotePassiveRendezvousClientCon != NULL || mRemotePassiveRendezvousTunnel != NULL);
}

/**
 * Send a success response to a Device Control request.
 *
 * @retval #WEAVE_ERROR_INCORRECT_STATE     If there is no request being processed.
 * @retval #WEAVE_NO_ERROR                  On success.
 * @retval other                            Other Weave or platform-specific error codes indicating that an error
 *                                          occurred preventing the success response from sending.
 */
WEAVE_ERROR DeviceControlServer::SendSuccessResponse()
{
    return SendStatusReport(kWeaveProfile_Common, Common::kStatus_Success);
}

/**
 * Send a status report response to a request.
 *
 * @param[in]   statusProfileId     The Weave profile ID this status report pertains to.
 * @param[in]   statusCode          The status code to be included in this response.
 * @param[in]   sysError            The system error code to be included in this response.
 *
 * @retval #WEAVE_ERROR_INCORRECT_STATE     If there is no request being processed.
 * @retval #WEAVE_NO_ERROR                  On success.
 * @retval other                            Other Weave or platform-specific error codes indicating that an error
 *                                          occurred preventing the status report from sending.
 */
WEAVE_ERROR DeviceControlServer::SendStatusReport(uint32_t statusProfileId, uint16_t statusCode, WEAVE_ERROR sysError)
{
    WEAVE_ERROR err;

    VerifyOrExit(mCurClientOp != NULL, err = WEAVE_ERROR_INCORRECT_STATE);

    err = WeaveServerBase::SendStatusReport(mCurClientOp, statusProfileId, statusCode, sysError);

exit:

    CloseClientOp();

    return err;
}

void DeviceControlServer::CloseClientOp()
{
    if (mCurClientOp != NULL)
    {
        mCurClientOp->Close();
        mCurClientOp = NULL;
    }
}

void DeviceControlServer::SystemTestTimeout()
{
    CloseClientOp();
}

void DeviceControlServer::HandleClientRequest(ExchangeContext *ec, const IPPacketInfo *pktInfo,
    const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *msgBuf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    DeviceControlServer *server = (DeviceControlServer *) ec->AppState;
    uint16_t dataLen;
    uint8_t *p;

    // Fail messages for the wrong profile. This shouldn't happen, but better safe than sorry.
    if (profileId != kWeaveProfile_DeviceControl)
    {
        WeaveServerBase::SendStatusReport(ec, kWeaveProfile_Common, Common::kStatus_BadRequest, WEAVE_NO_ERROR);
        ec->Close();
        ExitNow();
    }

    // Call on the delegate to enforce message-level access control.  If policy dictates the message should NOT
    // be processed, then simply end the exchange and return.  If an error response was warranted, the appropriate
    // response will have been sent within EnforceAccessControl().
    if (!server->EnforceAccessControl(ec, profileId, msgType, msgInfo, server->mDelegate))
    {
        ec->Close();
        ExitNow();
    }

    // Handle LookingToRendezvous messages specially, since they can be processed while another message is in progress.
    if (msgType == kMsgType_LookingToRendezvous)
    {
        err = server->HandleLookingToRendezvousMessage(msgInfo, ec);
        ExitNow();
    }

    // Disallow simultaneous requests.
    if (server->mCurClientOp != NULL)
    {
        WeaveServerBase::SendStatusReport(ec, kWeaveProfile_Common, Common::kStatus_Busy, WEAVE_NO_ERROR);
        ec->Close();
        ExitNow();
    }

    // Because the exchange context will be gone when we are waiting for our reset config
    // callback, we must also check mResetFlags to disallow simultaneous requests.
    if (server->mResetFlags != 0x0000) {
        WeaveServerBase::SendStatusReport(ec, kWeaveProfile_Common, Common::kStatus_Busy, WEAVE_NO_ERROR);
        ExitNow();
    }

    // Disallow requests over RPR client connection while listening for joiners on RPR client's behalf.
    if (server->mRemotePassiveRendezvousClientCon != NULL && server->mRemotePassiveRendezvousClientCon == ec->Con)
    {
        // If client misbehaves during RPR, cancel the operation.
        ec->Close();

        if (ec == server->mRemotePassiveRendezvousOp)
        {
            server->mRemotePassiveRendezvousOp = NULL;
        }

        server->CloseRemotePassiveRendezvous();

        ExitNow();
    }

    // Record that we have a request in process.
    server->mCurClientOp = ec;

    dataLen = msgBuf->DataLength();
    p = msgBuf->Start();

    // Decode and dispatch the message.
    switch (msgType)
    {
    case kMsgType_ResetConfig:
        VerifyOrExit(dataLen == kMessageLength_ResetConfig, err = WEAVE_ERROR_INVALID_MESSAGE_LENGTH);

        err = server->HandleResetConfig(p, ec->Con);
        SuccessOrExit(err);
        break;

    case kMsgType_ArmFailSafe:
        VerifyOrExit(dataLen == kMessageLength_ArmFailsafe, err = WEAVE_ERROR_INVALID_MESSAGE_LENGTH);

        err = server->HandleArmFailSafe(p);
        SuccessOrExit(err);
        break;

    case kMsgType_DisarmFailSafe:
        VerifyOrExit(dataLen == kMessageLength_DisarmFailsafe, err = WEAVE_ERROR_INVALID_MESSAGE_LENGTH);

        err = server->HandleDisarmFailSafe();
        SuccessOrExit(err);
        break;

    case kMsgType_EnableConnectionMonitor:
        VerifyOrExit(dataLen == kMessageLength_EnableConnectionMonitor, err = WEAVE_ERROR_INVALID_MESSAGE_LENGTH);

        err = server->HandleEnableConnectionMonitor(p, msgInfo, ec);
        SuccessOrExit(err);
        break;

    case kMsgType_DisableConnectionMonitor:
        VerifyOrExit(dataLen == kMessageLength_DisableConnectionMonitor, err = WEAVE_ERROR_INVALID_MESSAGE_LENGTH);

        err = server->HandleDisableConnectionMonitor(msgInfo, ec);
        SuccessOrExit(err);
        break;

    case kMsgType_RemotePassiveRendezvous:
        VerifyOrExit(dataLen == kMessageLength_RemotePassiveRendezvous, err = WEAVE_ERROR_INVALID_MESSAGE_LENGTH);

        err = server->HandleRemotePassiveRendezvous(p, ec);
        SuccessOrExit(err);
        break;

    case kMsgType_StartSystemTest:
        VerifyOrExit(dataLen == kMessageLength_StartSystemTest, err = WEAVE_ERROR_INVALID_MESSAGE_LENGTH);

        err = server->HandleStartSystemTest(p);
        SuccessOrExit(err);
        break;

    case kMsgType_StopSystemTest:
        VerifyOrExit(dataLen == kMessageLength_StopSystemTest, err = WEAVE_ERROR_INVALID_MESSAGE_LENGTH);

        err = server->HandleStopSystemTest();
        SuccessOrExit(err);
        break;

    default:
        err = server->SendStatusReport(kWeaveProfile_Common, Common::kStatus_BadRequest);
        ExitNow();
        break;
    }

exit:
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);

    if (err != WEAVE_NO_ERROR && server->mCurClientOp != NULL && ec == server->mCurClientOp)
    {
        WeaveLogError(DeviceControl, "Error handling DeviceControl client request, err = %d", err);

        if (msgType == kMsgType_RemotePassiveRendezvous)
        {
            server->CloseRemotePassiveRendezvous();
        }

        uint16_t statusCode = (err == WEAVE_ERROR_INVALID_MESSAGE_LENGTH)
                ? Common::kStatus_BadRequest
                : Common::kStatus_InternalError;
        server->SendStatusReport(kWeaveProfile_Common, statusCode, err);
    }
}

void DeviceControlServer::HandleResetConfigConnectionClose(WeaveConnection *aCon, WEAVE_ERROR conErr)
{
    DeviceControlServer *server = (DeviceControlServer *) aCon->AppState;
    DeviceControlDelegate *delegate = server->mDelegate;
    aCon->Close();

    delegate->OnResetConfig(server->mResetFlags);

    server->mFailSafeArmed = false;
    server->mFailSafeToken = 0;

    server->mResetFlags = 0x0000;
}

WEAVE_ERROR DeviceControlServer::SetConnectionMonitor(uint64_t peerNodeId, WeaveConnection *peerCon, uint16_t idleTimeout, uint16_t monitorInterval)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    ExchangeContext *monitorOp;

    // Search for an existing monitor exchange context for this connection.
    //
    monitorOp = ExchangeMgr->FindContext(peerNodeId, peerCon, this, true);

    // If a monitoring interval has been specified...
    if (monitorInterval != 0)
    {
        // Create a monitoring exchange context if needed.
        if (monitorOp == NULL)
        {
            monitorOp = ExchangeMgr->NewContext(peerCon, this);
            VerifyOrExit(monitorOp != NULL, err = WEAVE_ERROR_NO_MEMORY);
            monitorOp->PeerNodeId = peerNodeId;
            monitorOp->OnMessageReceived = HandleMonitorResponse;
            monitorOp->OnConnectionClosed = HandleMonitorConnectionClose;
        }

        // Save the monitoring interval in the context.
        monitorOp->RetransInterval = monitorInterval;

        // Arm the interval timer to send the first monitoring request.
        StartMonitorTimer(monitorOp);
    }

    // Otherwise no active monitoring requested so cancel any previously created
    // monitoring context/timer.
    else
    {
        if (monitorOp != NULL)
        {
            CancelMonitorTimer(monitorOp);
            monitorOp->Close();
            monitorOp = NULL;
        }
    }

    // set the idle timeout on the underlying connection.
    err = peerCon->SetIdleTimeout(idleTimeout);
    SuccessOrExit(err);

exit:
    if (err != WEAVE_NO_ERROR && monitorOp != NULL)
        monitorOp->Close();
    return err;
}

void DeviceControlServer::StartMonitorTimer(ExchangeContext *monitorOp)
{
    System::Layer* lSystemLayer = ExchangeMgr->MessageLayer->SystemLayer;
    lSystemLayer->StartTimer(monitorOp->RetransInterval, HandleMonitorTimer, monitorOp);
}

void DeviceControlServer::CancelMonitorTimer(ExchangeContext *monitorOp)
{
    System::Layer* lSystemLayer = ExchangeMgr->MessageLayer->SystemLayer;
    lSystemLayer->CancelTimer(HandleMonitorTimer, monitorOp);
}

void DeviceControlServer::HandleMonitorTimer(System::Layer* aSystemLayer, void* aAppState, System::Error aError)
{
    ExchangeContext*        monitorOp   = reinterpret_cast<ExchangeContext*>(aAppState);
    DeviceControlServer*    server      = reinterpret_cast<DeviceControlServer*>(monitorOp->AppState);
    PacketBuffer*           msgBuf      = NULL;

    WeaveLogProgress(DeviceControl, "Sending EchoRequest to device manager");

    // Send an Echo Request message to the node at the other end of the monitored connection.
    msgBuf = PacketBuffer::NewWithAvailableSize(0);
    VerifyOrExit(msgBuf != NULL, aError = WEAVE_ERROR_NO_MEMORY);
    aError = monitorOp->SendMessage(kWeaveProfile_Echo, kEchoMessageType_EchoRequest, msgBuf);
    msgBuf = NULL;

exit:
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);

    // If for some reason we got an error sending the EchoRequest, re-arm the interval timer and try again
    // later.
    if (aError != WEAVE_NO_ERROR)
        server->StartMonitorTimer(monitorOp);
}

void DeviceControlServer::HandleMonitorResponse(ExchangeContext *ec, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo, uint32_t profileId,
        uint8_t msgType, PacketBuffer *payload)
{
    DeviceControlServer *server = (DeviceControlServer *)ec->AppState;

    WeaveLogProgress(DeviceControl, "EchoResponse received from device manager");

    PacketBuffer::Free(payload);

    // Re-start the monitoring interval timer. We will send another EchoRequest when this timer expires.
    server->StartMonitorTimer(ec);
}

void DeviceControlServer::HandleMonitorConnectionClose(ExchangeContext *ec, WeaveConnection *con, WEAVE_ERROR conErr)
{
    DeviceControlServer *server = (DeviceControlServer *)ec->AppState;

    WeaveLogProgress(DeviceControl, "Monitored connection closed");

    // When the underlying connection closes, discard the monitoring exchange context and cancel the interval timer.
    server->CancelMonitorTimer(ec);
    ec->Close();
}

void DeviceControlServer::CloseRemotePassiveRendezvous()
{
    WEAVE_ERROR err;

    WeaveLogProgress(DeviceControl, "Closing RemotePassiveRendezvous.");

    // Close RPR ExchangeContext, if any.
    if (mRemotePassiveRendezvousOp != NULL)
    {
        mRemotePassiveRendezvousOp->Close();
        mRemotePassiveRendezvousOp = NULL;
    }

    // Close open RPR connections or tunnel, if any.
    if (mRemotePassiveRendezvousJoinerCon != NULL)
    {
        if (mRemotePassiveRendezvousJoinerCon->Close() != WEAVE_NO_ERROR)
            mRemotePassiveRendezvousJoinerCon->Abort();

        mRemotePassiveRendezvousJoinerCon = NULL;
    }

    if (mRemotePassiveRendezvousClientCon != NULL)
    {
        if (mRemotePassiveRendezvousClientCon->Close() != WEAVE_NO_ERROR)
            mRemotePassiveRendezvousClientCon->Abort();

        mRemotePassiveRendezvousClientCon = NULL;
    }

    if (mRemotePassiveRendezvousTunnel != NULL)
    {
        mRemotePassiveRendezvousTunnel->Shutdown();
        mRemotePassiveRendezvousTunnel = NULL;
    }

    // Let the application know to clean up state set when we started the Remote Passive Rendezvous,
    // e.g. making 15.4 networks joinable.
    if (mDelegate != NULL)
    {
        mDelegate->WillCloseRemotePassiveRendezvous();
    }

    if (ExchangeMgr && ExchangeMgr->MessageLayer)
    {
        // Cancel our unsecured listen, if enabled.
        err = ExchangeMgr->MessageLayer->ClearUnsecuredConnectionListener(HandleConnectionReceived,
                                                                          HandleUnsecuredConnectionCallbackRemoved);
        if (err != WEAVE_NO_ERROR)
            WeaveLogProgress(DeviceControl, "ClearUnsecuredConnectionListener failed, err = %d", err);

        if (ExchangeMgr->MessageLayer->SystemLayer)
        {
            // Cancel rendezvous timeout.
            ExchangeMgr->MessageLayer->SystemLayer->CancelTimer(HandleRemotePassiveRendezvousTimeout, this);
        }
    }

    // Notify delegate that we're done with the Remote Passive Rendezvous.
    if (mDelegate != NULL)
        mDelegate->OnRemotePassiveRendezvousDone();
}

void DeviceControlServer::HandleRemotePassiveRendezvousConnectionClosed(ExchangeContext *ec, WeaveConnection *con,
        WEAVE_ERROR conErr)
{
    DeviceControlServer *server = (DeviceControlServer *)ec->AppState;

    WeaveLogProgress(DeviceControl, "RemotePassiveRendezvous connection closed");

    server->mRemotePassiveRendezvousClientCon = NULL;
    server->CloseRemotePassiveRendezvous();
}

WEAVE_ERROR DeviceControlServer::CompleteRemotePassiveRendezvous(WeaveConnection *con)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    PacketBuffer *msgBuf = NULL;

    VerifyOrExit(con != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    if (mRemotePassiveRendezvousOp == NULL || mRemotePassiveRendezvousTunnel != NULL)
    {
        if (mRemotePassiveRendezvousOp == NULL)
            WeaveLogError(DeviceControl, "null mRemotePassiveRendezvousOp");
        else
            WeaveLogError(DeviceControl, "Tunnel already established");
        err = WEAVE_ERROR_INCORRECT_STATE;
        ExitNow();
    }

    // Capture rendezvoused connection to joiner.
    mRemotePassiveRendezvousJoinerCon = con;
    con = NULL; // Set con to NULL so we don't close client-half of tunnel at exit.

    // Send RemoteConnectionComplete message to client. Expect no response, as client knows all future data it sends
    // over this connection will be forwarded to the rendezvoused device.
    msgBuf = PacketBuffer::NewWithAvailableSize(0);
    VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    // Send RemoteConnectionComplete.
    err = mRemotePassiveRendezvousOp->SendMessage(kWeaveProfile_DeviceControl, kMsgType_RemoteConnectionComplete,
            msgBuf, 0);
    msgBuf = NULL;
    SuccessOrExit(err);

    mRemotePassiveRendezvousOp->Close();
    mRemotePassiveRendezvousOp = NULL;

    // As we've completed the Remote Passive Rendezvous, stop listening for connections accepted on the
    // unsecured Weave port.
    err = ExchangeMgr->MessageLayer->ClearUnsecuredConnectionListener(HandleConnectionReceived,
            HandleUnsecuredConnectionCallbackRemoved);
    if (err != WEAVE_NO_ERROR)
        WeaveLogProgress(DeviceControl, "ClearUnsecuredConnectionListener failed, err = %d", err);

    // Bind connections between Device Control client and rendezvoused device inside of WeaveMessageLayer, creating
    // WeaveConnectionTunnel to facilitate remote provisioning.
    WeaveLogProgress(DeviceControl, "Creating WeaveConnectionTunnnel...");

    if (mRemotePassiveRendezvousJoinerCon == NULL || mRemotePassiveRendezvousClientCon == NULL)
    {
        if (mRemotePassiveRendezvousJoinerCon == NULL)
            WeaveLogError(DeviceControl, "null RPR joiner connection");
        else
            WeaveLogError(DeviceControl, "null RPR client connection");
        err = WEAVE_ERROR_INCORRECT_STATE;
        ExitNow();
    }

    err = ExchangeMgr->MessageLayer->CreateTunnel(&mRemotePassiveRendezvousTunnel,
            *mRemotePassiveRendezvousJoinerCon, *mRemotePassiveRendezvousClientCon,
            secondsToMilliseconds(mTunnelInactivityTimeout));
    mRemotePassiveRendezvousJoinerCon = NULL;
    mRemotePassiveRendezvousClientCon = NULL;
    SuccessOrExit(err);
    WeaveLogProgress(DeviceControl, "Tunnel created successfully.");

    // Hang pointer to DeviceControlServer off new WeaveConnectionTunenl for use in HandleTunnelShutdown.
    mRemotePassiveRendezvousTunnel->AppState = this;

    // Hang callback for tunnel shutdown, i.e. full closure of underlying TCPEndPoints by WeaveMessageLayer.
    mRemotePassiveRendezvousTunnel->OnShutdown = HandleTunnelShutdown;

    // Cancel rendezvous timeout.
    ExchangeMgr->MessageLayer->SystemLayer->CancelTimer(HandleRemotePassiveRendezvousTimeout, this);

exit:
    if (msgBuf != NULL)
    {
        PacketBuffer::Free(msgBuf);
    }

    if (con != NULL)
    {
        con->Close();
    }

    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogProgress(DeviceControl, "Failed to complete Remote Passive Rendezvous, err = %d", err);

        // If anything fails during tunnel setup, kill whole Remote Passive Rendezvous interaction.
        CloseRemotePassiveRendezvous();
    }

    return err;
}

WEAVE_ERROR DeviceControlServer::HandleResetConfig(uint8_t *p, WeaveConnection *curCon)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    uint16_t resetFlags = LittleEndian::Get16(p);

    if (mDelegate->IsResetAllowed(resetFlags))
    {
        // If there is a connection, find out if it needs to be closed before
        // calling the OnResetConfig delegate method. If it does, ensure the response
        // is sent and the connection is closed before calling OnResetConfig.
        if (curCon != NULL && mDelegate->ShouldCloseConBeforeResetConfig(resetFlags)) {
            // We must wait until the connection successfully shuts down to perform the reset.
            // So we cache the reset flags in mResetFlags, and register the callback so the
            // reset will be performed.
            mResetFlags = resetFlags;

            curCon->AppState = this;
            curCon->OnConnectionClosed = HandleResetConfigConnectionClose;

            // Send the response that indicates the connection will close, then
            // shut down the connection to ensure the client receives the response
            // and closes the connection.
            err = SendStatusReport(kWeaveProfile_DeviceControl, kStatusCode_ResetSuccessCloseCon);
            SuccessOrExit(err);

            curCon->Shutdown();
        } else {
            err = mDelegate->OnResetConfig(resetFlags);
            if (err == WEAVE_ERROR_NOT_IMPLEMENTED)
            {
                err = SendStatusReport(kWeaveProfile_DeviceControl, kStatusCode_UnsupportedFailSafeMode);
                ExitNow();
            }
            SuccessOrExit(err);

            mFailSafeArmed = false;
            mFailSafeToken = 0;

            err = SendSuccessResponse();
            SuccessOrExit(err);
        }
    }
    else
    {
        err = SendStatusReport(kWeaveProfile_DeviceControl, kStatusCode_ResetNotAllowed);
        ExitNow();
    }

exit:
    if (err != WEAVE_NO_ERROR)
    {
        // Make sure we don't get stuck in a busy state.
        mResetFlags = 0x0000;
    }

    return err;
}

WEAVE_ERROR DeviceControlServer::HandleArmFailSafe(uint8_t *p)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    uint8_t armMode = Read8(p);
    uint32_t failSafeToken = LittleEndian::Read32(p);

    switch (armMode)
    {
    case kArmMode_New:
        if (mFailSafeArmed)
        {
            err = SendStatusReport(kWeaveProfile_DeviceControl, kStatusCode_FailSafeAlreadyActive);
            ExitNow();
        }
        break;

    case kArmMode_Reset:
        if (mDelegate != NULL)
        {
            if (mDelegate->IsResetAllowed(kResetConfigFlag_All))
            {
                err = mDelegate->OnResetConfig(kResetConfigFlag_All);
                SuccessOrExit(err);
            }
            else
            {
                err = SendStatusReport(kWeaveProfile_DeviceControl, kStatusCode_ResetNotAllowed);
                ExitNow();
            }
        }
        break;

    case kArmMode_ResumeExisting:
        if (mFailSafeArmed)
        {
            if (failSafeToken != mFailSafeToken)
            {
                err = SendStatusReport(kWeaveProfile_DeviceControl, kStatusCode_NoMatchingFailSafeActive);
            }
            else
            {
                err = SendSuccessResponse();
            }
            ExitNow();
        }
        break;

    default:
        SendStatusReport(kWeaveProfile_DeviceControl, kStatusCode_UnsupportedFailSafeMode);
        ExitNow();
        break;
    }

    mFailSafeArmed = true;
    mFailSafeToken = failSafeToken;

    if (mDelegate != NULL)
    {
        err = mDelegate->OnFailSafeArmed();
        SuccessOrExit(err);
    }

    err = SendSuccessResponse();
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR DeviceControlServer::HandleDisarmFailSafe()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (!mFailSafeArmed)
    {
        err = SendStatusReport(kWeaveProfile_DeviceControl, kStatusCode_NoFailSafeActive);
        ExitNow();
    }

    mFailSafeArmed = false;
    mFailSafeToken = 0;

    if (mDelegate != NULL)
    {
        err = mDelegate->OnFailSafeDisarmed();
        SuccessOrExit(err);
    }

    err = SendSuccessResponse();
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR DeviceControlServer::HandleEnableConnectionMonitor(uint8_t *p, const WeaveMessageInfo *msgInfo,
    ExchangeContext *ec)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // Parse the monitoring interval and timeout from the message.
    uint16_t idleTimeout = LittleEndian::Read16(p);
    uint16_t monitorInterval = LittleEndian::Read16(p);

    err = SetConnectionMonitor(msgInfo->SourceNodeId, ec->Con, idleTimeout, monitorInterval);
    SuccessOrExit(err);

    err = SendSuccessResponse();
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR DeviceControlServer::HandleDisableConnectionMonitor(const WeaveMessageInfo *msgInfo,
    ExchangeContext *ec)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    err = SetConnectionMonitor(msgInfo->SourceNodeId, ec->Con, 0, 0);
    SuccessOrExit(err);

    err = SendSuccessResponse();
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR DeviceControlServer::HandleRemotePassiveRendezvous(uint8_t *p, ExchangeContext *ec)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // Fail if we're already listening for a renendezvous connection on behalf of another client, or if we're
    // currently monitoring an active tunnel.
    if (IsRemotePassiveRendezvousInProgress())
    {
        WeaveLogProgress(DeviceControl, "RemotePassiveRendezvous already in progress, sending busy status reply");
        err = SendStatusReport(kWeaveProfile_Common, Common::kStatus_Busy);
        ExitNow();
    }

    // RPR request's ExchangeContext must have an open WeaveConnection.
    if (ec->Con == NULL)
    {
        WeaveLogProgress(DeviceControl, "RemotePassiveRendezvous requires WeaveConnection");
        err = SendStatusReport(kWeaveProfile_Common, Common::kStatus_UnexpectedMessage);
        ExitNow();
    }

    // Clear mCurClientOp. We eventually close the referenced exchange as mRemotePassiveRendezvousOp.
    mCurClientOp = NULL;

    // Capture connection over which this message was received.
    mRemotePassiveRendezvousClientCon = ec->Con;

    // Capture ExchangeContext over which to eventually send RemoteConnectionComplete.
    mRemotePassiveRendezvousOp = ec;
    ec->OnConnectionClosed = HandleRemotePassiveRendezvousConnectionClosed;

    // Parse Remote Passive Rendezvous request body.
    WeaveLogProgress(DeviceControl, "Parsing RPR request");
    mRemotePassiveRendezvousTimeout = LittleEndian::Read16(p);
    mTunnelInactivityTimeout = LittleEndian::Read16(p);

    // Decode joiner filter address.
    IPAddress::ReadAddress(const_cast<const uint8_t *&>(p), mRemotePassiveRendezvousJoinerAddr);

    WeaveLogProgress(DeviceControl, "Got rendezvous timeout = %d, inactivity timeout = %d",
             mRemotePassiveRendezvousTimeout, mTunnelInactivityTimeout);

    // Arm timer to reset our connection to the client if no rendezvous connection has been accepted after
    // a client-specified interval.
    err = ArmRemotePassiveRendezvousTimer();
    SuccessOrExit(err);

    // Set up callback to intercept new connections received on the unsecured Weave port. Fail if someone else
    // such as the Device Manager has already set this callback.
    err = ExchangeMgr->MessageLayer->SetUnsecuredConnectionListener(HandleConnectionReceived,
            HandleUnsecuredConnectionCallbackRemoved, false, this);
    SuccessOrExit(err);

    if (mDelegate != NULL)
    {
        // Let delegate know to set up any state required for Remote Passive Rendezvous, e.g. making 15.4
        // networks joinable.
        mDelegate->WillStartRemotePassiveRendezvous();
        SuccessOrExit(err);
    }

    err = WeaveServerBase::SendStatusReport(ec, kWeaveProfile_Common, Common::kStatus_Success, WEAVE_NO_ERROR);
    SuccessOrExit(err);

    // Inform delegate that we've started the Remote Passive Rendezvous.
    if (mDelegate != NULL)
        mDelegate->OnRemotePassiveRendezvousStarted();

exit:
    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogError(DeviceControl, "HandleRemotePassiveRendezvousFailed, err = %d", err);

        // If we nulled mCurClientOp earlier in this function, we started a new RPR which we now must close.
        // If we failed at IsRemotePassiveRendezvousInProgress(), mCurClientOp will not be NULL, and we must
        // NOT close the RPR already in progress.
        if (mCurClientOp == NULL)
        {
            CloseRemotePassiveRendezvous();
        }
    }

    return err;
}

WEAVE_ERROR DeviceControlServer::HandleStartSystemTest(uint8_t *p)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    uint32_t profileId = LittleEndian::Read32(p);
    uint32_t testId = LittleEndian::Read32(p);

    if (mDelegate != NULL)
    {
        err = mDelegate->OnSystemTestStarted(profileId, testId);
    }
    else
    {
        err = SendStatusReport(kWeaveProfile_DeviceControl, kStatusCode_NoSystemTestDelegate);
    }

    return err;
}

WEAVE_ERROR DeviceControlServer::HandleStopSystemTest()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (mDelegate != NULL)
    {
        err = mDelegate->OnSystemTestStopped();
    }
    else
    {
        err = SendStatusReport(kWeaveProfile_DeviceControl, kStatusCode_NoSystemTestDelegate);
    }

    return err;
}

WEAVE_ERROR DeviceControlServer::HandleLookingToRendezvousMessage(const WeaveMessageInfo *msgInfo, ExchangeContext *ec)
{
    WEAVE_ERROR err = WEAVE_ERROR_INCORRECT_STATE;
    System::Layer* lSystemLayer = ExchangeMgr->MessageLayer->SystemLayer;

    // We are going to be dealing with the connection closing here rather than in the timer close
    lSystemLayer->CancelTimer(HandleLookingToRendezvousTimeout, ec->Con);

    // LookingToRendezvous message is not authenticated
    // but we only pay attention to it while:
    if (IsRemotePassiveRendezvousInProgress() &&  // in remote passive rendezvous
        (mRemotePassiveRendezvousJoinerCon == NULL) && // and before we completed the tunnel
        (ec->Con != NULL) && // and it has an associated connection
        (ec->Con != mRemotePassiveRendezvousClientCon)) // sanity: the request did not come from the client
    {
        // && of course, we actually match the RPR joiner address
        if (mRemotePassiveRendezvousJoinerAddr == nl::Inet::IPAddress::MakeLLA(WeaveNodeIdToIPv6InterfaceId(msgInfo->SourceNodeId)))
        {
            WeaveLogProgress(DeviceControl, "LookingToRendezvous successfully matched client filter");
            err = CompleteRemotePassiveRendezvous(ec->Con);
            SuccessOrExit(err);

            ec->Close();
            ec = NULL;
        }
        else
        {
#if WEAVE_DETAIL_LOGGING
            char joinerAddr[INET6_ADDRSTRLEN] = { 0 };
            mRemotePassiveRendezvousJoinerAddr.ToString(joinerAddr, sizeof(joinerAddr));
            WeaveLogProgress(DeviceControl, "LookingToRendezvous failed filter: Joiner node id: %" PRIX64 " expected address %s", msgInfo->SourceNodeId, joinerAddr);
#else
            WeaveLogProgress(DeviceControl, "LookingToRendezvous failed to matched client filter");
#endif
        }
    }
    else
    {
        WeaveLogProgress(DeviceControl, "LookingToRendezvous message received in unexpected state");
    }

exit:
    if (ec != NULL)
    {
        if ((err != WEAVE_NO_ERROR) && (ec->Con != NULL))
        {
            ec->Con->Close();
        }
        ec->Close();
    }

    return err;
}


void DeviceControlServer::HandleConnectionReceived(WeaveMessageLayer *msgLayer, WeaveConnection *con)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    DeviceControlServer *server = sRemotePassiveRendezvousServer;

    VerifyOrExit(server->mRemotePassiveRendezvousJoinerCon == NULL, err = WEAVE_ERROR_INCORRECT_STATE);

    // Steal new connection received on the unsecured Weave port.
    if (server->mRemotePassiveRendezvousJoinerAddr != IPAddress::Any)
    {
        if (server->mRemotePassiveRendezvousJoinerAddr == con->PeerAddr)
        {
            WeaveLogProgress(DeviceControl, "Remote device addr successfully matched client filter");
            err = server->CompleteRemotePassiveRendezvous(con);
            con = NULL; // Downstack call transfers responsibility for WeaveConnection object.
            SuccessOrExit(err);
        }
        else
        {
            System::Layer* lSystemLayer = server->ExchangeMgr->MessageLayer->SystemLayer;

            WeaveLogProgress(DeviceControl, "Remote device addr failed to match client filter");
            WeaveLogProgress(DeviceControl, "Awaiting looking to rendezvous message");
            server->ExchangeMgr->AllowUnsolicitedMessages(con);
            err = lSystemLayer->StartTimer(secondsToMilliseconds(server->mTunnelInactivityTimeout), HandleLookingToRendezvousTimeout, con);
            SuccessOrExit(err);
            con->OnConnectionClosed = HandleLookingToRendezvousClosed;
            con = NULL;
        }
    }
    else
    {
        // If we're not filtering rendezvoused devices, immediately complete Remote Passive Rendezvous
        // connection.
        err = server->CompleteRemotePassiveRendezvous(con);
        con = NULL; // Downstack call transfers responsibility for WeaveConnection object.
        SuccessOrExit(err);
    }

exit:
    if (err != WEAVE_NO_ERROR && con != NULL)
    {
        // Close connection.
        con->Close();
    }
}

/**
 * Static handler of type WeaveConnection::ConnectionClosedFunct.
 * This handler is installed on connections waiting for the LookingToRendezVous message.
 * Its purpose is to stop the timer that guards the connection in case the connection object is closed by the
 * lower layers.
 * This handler will be in effect until either:
 * - the connection closes itself and the handler is executed;
 * - the LookingToRandezvous message is never received and the timer fires, making this node close the connection;
 * - the message is received, and the WeaveConnection object is given to the WeaveConnectionTunnel object, which
 *   takes ownership of the TCPEndPoint and destroys the WeaveConnection.
 *
 * @param[in] con    The connection.
 * @param[in] conErr An error code that explains why the connection was closed.
 */
void DeviceControlServer::HandleLookingToRendezvousClosed(WeaveConnection *con, WEAVE_ERROR conErr)
{
    DeviceControlServer *_this =  sRemotePassiveRendezvousServer;
    System::Layer* lSystemLayer = _this->ExchangeMgr->MessageLayer->SystemLayer;

    WeaveLogProgress(DeviceControl, "Connection waiting for LookingToRendezvous message self-closed with error %d", conErr);

    lSystemLayer->CancelTimer(HandleLookingToRendezvousTimeout, con);

    con->Close();
}

void DeviceControlServer::HandleLookingToRendezvousTimeout(System::Layer* aSystemLayer, void* aAppState, System::Error aError)
{
    WeaveConnection * con = static_cast<WeaveConnection *>(aAppState);
    DeviceControlServer *_this =  sRemotePassiveRendezvousServer;

    if (con != _this->mRemotePassiveRendezvousJoinerCon)
    {
        WeaveLogProgress(DeviceControl, "Failed to receive a matching LookingToRendezvous message");
        con->Abort();
    }
}

/**
 * Send a LookingToRendezvous message to the peer.
 *
 * @param[in] ec          ExchangeContext to use to send the message
 *
 * @retval #WEAVE_NO_ERROR On success.
 *
 * @retval #WEAVE_ERROR_NO_MEMORY If we could not allocate a buffer for the message.
 *
 * @retval other          Other errors returned by nl::Weave::ExchangeContext::SendMessage
 */
WEAVE_ERROR SendLookingToRendezvous(ExchangeContext *ec)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    PacketBuffer *buf = NULL;

    buf = PacketBuffer::NewWithAvailableSize(0);
    VerifyOrExit(buf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    err = ec->SendMessage(kWeaveProfile_DeviceControl, kMsgType_LookingToRendezvous, buf);
    buf = NULL;

exit:
    if (buf == NULL)
    {
        PacketBuffer::Free(buf);
    }
    return err;
}

void DeviceControlServer::HandleUnsecuredConnectionCallbackRemoved(void *appState)
{
    WeaveLogProgress(DeviceControl, "OnUnsecuredConnectionReceived callback pre-empted");

    DeviceControlServer *server = static_cast<DeviceControlServer *>(appState);

    server->WeaveServerBase::SendStatusReport(server->mRemotePassiveRendezvousOp, kWeaveProfile_DeviceControl,
            kStatusCode_UnsecuredListenPreempted, WEAVE_NO_ERROR);

    server->CloseRemotePassiveRendezvous();
}

void DeviceControlServer::HandleRemotePassiveRendezvousTimeout(System::Layer* aSystemLayer, void* aAppState, System::Error aError)
{
    DeviceControlServer* server = reinterpret_cast<DeviceControlServer*>(aAppState);
    WeaveLogProgress(DeviceControl, "Remote Passive Rendezvous timed out");

    server->WeaveServerBase::SendStatusReport(server->mRemotePassiveRendezvousOp, kWeaveProfile_DeviceControl,
            kStatusCode_RemotePassiveRendezvousTimedOut, WEAVE_NO_ERROR);

    // Close Remote Passive Rendezvous, but leave connection open for additional messages from client.
    server->mRemotePassiveRendezvousClientCon = NULL;
    server->CloseRemotePassiveRendezvous();
}

WEAVE_ERROR DeviceControlServer::ArmRemotePassiveRendezvousTimer()
{
    System::Layer*  lSystemLayer                        = ExchangeMgr->MessageLayer->SystemLayer;
    uint32_t        mRemotePassiveRendezvousTimeoutMs   =
        static_cast<uint32_t>(secondsToMilliseconds(mRemotePassiveRendezvousTimeout));

    WeaveLogProgress(DeviceControl, "Arming Remote Passive Rendezvous timer %lu ms",
            static_cast<unsigned long>(mRemotePassiveRendezvousTimeoutMs));

    return lSystemLayer->StartTimer(mRemotePassiveRendezvousTimeoutMs, HandleRemotePassiveRendezvousTimeout, this);
}

void DeviceControlServer::HandleTunnelShutdown(WeaveConnectionTunnel *tun)
{
    WeaveLogProgress(DeviceControl, "Remote Passive Rendezvous tunnel shut down.");
    DeviceControlServer *server = (DeviceControlServer *)tun->AppState;

    if (tun != server->mRemotePassiveRendezvousTunnel)
        return;

    // Avoid double-shutdown.
    server->mRemotePassiveRendezvousTunnel = NULL;

    server->CloseRemotePassiveRendezvous();
}

void DeviceControlDelegate::EnforceAccessControl(ExchangeContext *ec, uint32_t msgProfileId, uint8_t msgType,
        const WeaveMessageInfo *msgInfo, AccessControlResult& result)
{
    // If the result has not already been determined by a subclass...
    if (result == kAccessControlResult_NotDetermined)
    {
        WeaveAuthMode authMode = msgInfo->PeerAuthMode;

        switch (msgType)
        {
#if WEAVE_CONFIG_REQUIRE_AUTH_DEVICE_CONTROL

        case kMsgType_ResetConfig:
        case kMsgType_ArmFailSafe:
            if (authMode == kWeaveAuthMode_CASE_AccessToken ||
                (authMode == kWeaveAuthMode_PASE_PairingCode && !IsPairedToAccount()))
            {
                result = kAccessControlResult_Accepted;
            }
            break;

        case kMsgType_DisarmFailSafe:
        case kMsgType_EnableConnectionMonitor:
        case kMsgType_DisableConnectionMonitor:
            if (authMode == kWeaveAuthMode_CASE_AccessToken || authMode == kWeaveAuthMode_PASE_PairingCode)
            {
                result = kAccessControlResult_Accepted;
            }
            break;

        case kMsgType_RemotePassiveRendezvous:
        case kMsgType_StartSystemTest:
        case kMsgType_StopSystemTest:
            if (authMode == kWeaveAuthMode_CASE_AccessToken)
            {
                result = kAccessControlResult_Accepted;
            }
            break;

#else // WEAVE_CONFIG_REQUIRE_AUTH_DEVICE_CONTROL

        case kMsgType_ResetConfig:
        case kMsgType_ArmFailSafe:
        case kMsgType_DisarmFailSafe:
        case kMsgType_EnableConnectionMonitor:
        case kMsgType_DisableConnectionMonitor:
        case kMsgType_RemotePassiveRendezvous:
        case kMsgType_StartSystemTest:
        case kMsgType_StopSystemTest:
            result = kAccessControlResult_Accepted;
            break;

#endif // WEAVE_CONFIG_REQUIRE_AUTH_DEVICE_CONTROL

        case kMsgType_LookingToRendezvous:
            if (authMode == kWeaveAuthMode_None)
            {
                result = kAccessControlResult_Accepted;
            }
            else
            {
                WeaveServerBase::SendStatusReport(ec, kWeaveProfile_Common, Common::kStatus_BadRequest, WEAVE_NO_ERROR);
                result = kAccessControlResult_Rejected_RespSent;
            }
            break;

        default:
            WeaveServerBase::SendStatusReport(ec, kWeaveProfile_Common, Common::kStatus_UnsupportedMessage, WEAVE_NO_ERROR);
            result = kAccessControlResult_Rejected_RespSent;
            break;
        }
    }

    // Call up to the base class.
    WeaveServerDelegateBase::EnforceAccessControl(ec, msgProfileId, msgType, msgInfo, result);
}

// TODO: eliminate this method when device code provides appropriate implementations.
bool DeviceControlDelegate::IsPairedToAccount() const
{
    return false;
}

} // namespace DeviceControl
} // namespace Profiles
} // namespace Weave
} // namespace nl
