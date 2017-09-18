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
 *      This file implements the Token Pairing Server, which receives
 *      and processes PairTokenRequest messages, responding as appropriate.
 */

#include <string.h>
#include <ctype.h>

#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveEncoding.h>
#include <Weave/Profiles/token-pairing/TokenPairing.h>
#include <Weave/Support/CodeUtils.h>
#include <Weave/Profiles/common/CommonProfile.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace TokenPairing {

using namespace nl::Weave::Encoding;

TokenPairingServer::TokenPairingServer() :
    mCurClientOp(NULL),
    mDelegate(NULL),
    mCertificateSent(false)
{
    FabricState = NULL;
    ExchangeMgr = NULL;
}

/**
 * Initialize the Token Pairing Server state and register to receive
 * Token Pairing messages.
 *
 * param[in]    exchangeMgr     A pointer to the Weave Exchange Manager.
 *
 * @retval #WEAVE_ERROR_INCORRECT_STATE                         When a token pairing server
 *                                                              has already been registered.
 * @retval #WEAVE_ERROR_TOO_MANY_UNSOLICITED_MESSAGE_HANDLERS   When too many unsolicited message
 *                                                              handlers are registered.
 * @retval #WEAVE_NO_ERROR                                      On success.
 */
WEAVE_ERROR TokenPairingServer::Init(WeaveExchangeManager *exchangeMgr)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (ExchangeMgr != NULL)
        return WEAVE_ERROR_INCORRECT_STATE;

    ExchangeMgr = exchangeMgr;
    FabricState = exchangeMgr->FabricState;
    mCurClientOp = NULL;

    // Register to receive unsolicited Token Pairing messages from the exchange manager.
    err = ExchangeMgr->RegisterUnsolicitedMessageHandler(kWeaveProfile_TokenPairing, kMsgType_PairTokenRequest, HandleClientRequest, this);
    SuccessOrExit(err);
    err = ExchangeMgr->RegisterUnsolicitedMessageHandler(kWeaveProfile_TokenPairing, kMsgType_UnpairTokenRequest, HandleClientRequest, this);

exit:
    return err;
}

/**
 * Shutdown the Token Pairing Server.
 *
 * @retval #WEAVE_NO_ERROR unconditionally.
 */
// TODO: Additional documentation detail required (i.e. how this function impacts object lifecycle).
WEAVE_ERROR TokenPairingServer::Shutdown()
{
    if (ExchangeMgr != NULL)
    {
        ExchangeMgr->UnregisterUnsolicitedMessageHandler(kWeaveProfile_TokenPairing, kMsgType_UnpairTokenRequest);
        ExchangeMgr->UnregisterUnsolicitedMessageHandler(kWeaveProfile_TokenPairing, kMsgType_PairTokenRequest);
        ExchangeMgr = NULL;
    }
    CloseClientOp();
    FabricState = NULL;

    return WEAVE_NO_ERROR;
}

/**
 * Set the delegate to process Device Control Server events.
 *
 * @param[in]   delegate    A pointer to the Device Control Delegate.
 */
void TokenPairingServer::SetDelegate(TokenPairingDelegate *delegate)
{
    mDelegate = delegate;
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
WEAVE_ERROR TokenPairingServer::SendStatusReport(uint32_t statusProfileId, uint16_t statusCode, WEAVE_ERROR sysError)
{
    WEAVE_ERROR err;

    VerifyOrExit(mCurClientOp != NULL, err = WEAVE_ERROR_INCORRECT_STATE);
    err = WeaveServerBase::SendStatusReport(mCurClientOp, statusProfileId, statusCode, sysError);

exit:
    CloseClientOp();
    return err;
}

WEAVE_ERROR TokenPairingServer::SendTokenCertificateResponse(PacketBuffer *certificateBuf)
{
    WEAVE_ERROR err;

    WeaveLogError(TokenPairing, "SendTokenCertificateResponse");
    VerifyOrExit(mCurClientOp != NULL, err = WEAVE_ERROR_INCORRECT_STATE);

    // The optional TokenCertificateResponse may only be sent once.
    VerifyOrExit(mCertificateSent == false, err = WEAVE_ERROR_INCORRECT_STATE);

    err = mCurClientOp->SendMessage(kWeaveProfile_TokenPairing, kMsgType_TokenCertificateResponse, certificateBuf, 0);
    certificateBuf = NULL;
    mCertificateSent = true;

exit:
    if (certificateBuf != NULL)
    {
        PacketBuffer::Free(certificateBuf);
    }
    return err;
}

WEAVE_ERROR TokenPairingServer::SendTokenPairedResponse(PacketBuffer *tokenBundleBuf)
{
    WEAVE_ERROR err;

    WeaveLogError(TokenPairing, "SendTokenPairedResponse");
    VerifyOrExit(mCurClientOp != NULL, err = WEAVE_ERROR_INCORRECT_STATE);

    err = mCurClientOp->SendMessage(kWeaveProfile_TokenPairing, kMsgType_TokenPairedResponse, tokenBundleBuf, 0);
    tokenBundleBuf = NULL;

exit:
    if (tokenBundleBuf != NULL)
    {
        PacketBuffer::Free(tokenBundleBuf);
    }
    CloseClientOp();
    return err;
}

void TokenPairingServer::CloseClientOp()
{
    if (mCurClientOp != NULL)
    {
        mCurClientOp->Close();
        mCurClientOp = NULL;
    }
    mCertificateSent = false;
}

void TokenPairingServer::HandleClientRequest(ExchangeContext *ec, const IPPacketInfo *pktInfo,
        const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *msgBuf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TokenPairingServer *server = (TokenPairingServer *) ec->AppState;

    // Fail messages for the wrong profile. This shouldn't happen, but better safe than sorry.
    if (profileId != kWeaveProfile_TokenPairing)
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

    // Disallow simultaneous requests.
    if (server->mCurClientOp != NULL)
    {
        WeaveServerBase::SendStatusReport(ec, kWeaveProfile_Common, Common::kStatus_Busy, WEAVE_NO_ERROR);
        ec->Close();
        ExitNow();
    }

    // Record that we have a request in process.
    server->mCurClientOp = ec;
    server->mCertificateSent = false;

    // Decode and dispatch the message.
    switch (msgType)
    {
        case kMsgType_PairTokenRequest:
            if (server->mDelegate != NULL)
            {
                err = server->mDelegate->OnPairTokenRequest((TokenPairingServer *)ec->AppState, msgBuf->Start(), msgBuf->DataLength());
            }
            SuccessOrExit(err);
            break;

        case kMsgType_UnpairTokenRequest:
            if (server->mDelegate != NULL)
            {
                err = server->mDelegate->OnUnpairTokenRequest((TokenPairingServer *)ec->AppState);
            }
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
        WeaveLogError(TokenPairing, "Error handling TokenPairing client request, err = %d", err);
        server->SendStatusReport(kWeaveProfile_Common, Common::kStatus_InternalError, err);
    }
}

void TokenPairingDelegate::EnforceAccessControl(ExchangeContext *ec, uint32_t msgProfileId, uint8_t msgType,
        const WeaveMessageInfo *msgInfo, AccessControlResult& result)
{
    // If the result has not already been determined by a subclass...
    if (result == kAccessControlResult_NotDetermined)
    {
        switch (msgType)
        {
        case kMsgType_PairTokenRequest:
        case kMsgType_UnpairTokenRequest:
#if WEAVE_CONFIG_REQUIRE_AUTH_DEVICE_CONTROL
            if (msgInfo->PeerAuthMode == kWeaveAuthMode_PASE_PairingCode)
            {
                result = kAccessControlResult_Accepted;
            }
#else // WEAVE_CONFIG_REQUIRE_AUTH_DEVICE_CONTROL
            result = kAccessControlResult_Accepted;
#endif // WEAVE_CONFIG_REQUIRE_AUTH_DEVICE_CONTROL
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

} // namespace TokenPairing
} // namespace Profiles
} // namespace Weave
} // namespace nl
