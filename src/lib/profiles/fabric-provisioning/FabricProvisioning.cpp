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
 *      This file implements the Fabric Provisioning Profile, used to
 *      manage membership to Weave Fabrics.
 *
 *      The Fabric Provisioning Profile facilitates client-server operations
 *      such that the client (the controlling device) can trigger specific
 *      functionality on the server (the device undergoing provisioning),
 *      to allow it to create, join, and leave Weave Fabrics.  This includes
 *      communicating Fabric configuration information such as identifiers,
 *      keys, security schemes, and related data.
 *
 */

#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Support/CodeUtils.h>
#include <Weave/Core/WeaveEncoding.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include "FabricProvisioning.h"
#include <Weave/Profiles/common/CommonProfile.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace FabricProvisioning {

using namespace nl::Weave::Crypto;
using namespace nl::Weave::Encoding;
using namespace nl::Weave::TLV;

FabricProvisioningServer::FabricProvisioningServer()
{
    FabricState = NULL;
    ExchangeMgr = NULL;
    mDelegate = NULL;
    mCurClientOp = NULL;
}

/**
 * Initialize the Fabric Provisioning Server state and register to receive
 * Fabric Provisioning messages.
 *
 * @param[in] exchangeMgr   A pointer to the system Weave Exchange Manager.
 *
 * @retval #WEAVE_ERROR_TOO_MANY_UNSOLICITED_MESSAGE_HANDLERS   If too many message handlers have
 *                                                              already been registered.
 * @retval #WEAVE_NO_ERROR                                      On success.
 */
WEAVE_ERROR FabricProvisioningServer::Init(WeaveExchangeManager *exchangeMgr)
{
    FabricState = exchangeMgr->FabricState;
    ExchangeMgr = exchangeMgr;
    mDelegate = NULL;
    mCurClientOp = NULL;

    // Register to receive unsolicited Service Provisioning messages from the exchange manager.
    WEAVE_ERROR err =
        ExchangeMgr->RegisterUnsolicitedMessageHandler(kWeaveProfile_FabricProvisioning, HandleClientRequest, this);

    return err;
}

/**
 * Shutdown the Fabric Provisioning Server.
 *
 * @retval #WEAVE_NO_ERROR unconditionally.
 */
// TODO: Additional documentation detail required (i.e. how this function impacts object lifecycle).
WEAVE_ERROR FabricProvisioningServer::Shutdown()
{
    if (ExchangeMgr != NULL)
        ExchangeMgr->UnregisterUnsolicitedMessageHandler(kWeaveProfile_FabricProvisioning);

    FabricState = NULL;
    ExchangeMgr = NULL;
    mDelegate = NULL;
    mCurClientOp = NULL;

    return WEAVE_NO_ERROR;
}

/**
 * Set the delegate to process Fabric Provisioning events.
 *
 * @param[in]   delegate    A pointer to the Fabric Provisioning Delegate.
 */
void FabricProvisioningServer::SetDelegate(FabricProvisioningDelegate *delegate)
{
    mDelegate = delegate;
}

/**
 * Send a success response to a Fabric Provisioning request.
 *
 * @retval #WEAVE_ERROR_INCORRECT_STATE If there is no request being processed.
 * @retval #WEAVE_NO_ERROR              On success.
 * @retval other                        Other Weave or platform-specific error codes indicating that an error
 *                                      occurred preventing the sending of the success response.
 */
WEAVE_ERROR FabricProvisioningServer::SendSuccessResponse()
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
 * @retval #WEAVE_ERROR_INCORRECT_STATE If there is no request being processed.
 * @retval #WEAVE_NO_ERROR              On success.
 * @retval other                        Other Weave or platform-specific error codes indicating that an error
 *                                      occurred preventing the sending of the status report.
 */
WEAVE_ERROR FabricProvisioningServer::SendStatusReport(uint32_t statusProfileId, uint16_t statusCode, WEAVE_ERROR sysError)
{
    WEAVE_ERROR err;

    VerifyOrExit(mCurClientOp != NULL, err = WEAVE_ERROR_INCORRECT_STATE);

    err = WeaveServerBase::SendStatusReport(mCurClientOp, statusProfileId, statusCode, sysError);

exit:

    if (mCurClientOp != NULL)
    {
        mCurClientOp->Close();
        mCurClientOp = NULL;
    }

    return err;
}

void FabricProvisioningServer::HandleClientRequest(ExchangeContext *ec, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo,
                                                   uint32_t profileId, uint8_t msgType, PacketBuffer *msgBuf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    FabricProvisioningServer *server = (FabricProvisioningServer *) ec->AppState;
    FabricProvisioningDelegate *delegate = server->mDelegate;

    // Fail messages for the wrong profile. This shouldn't happen, but better safe than sorry.
    if (profileId != kWeaveProfile_FabricProvisioning)
    {
        WeaveServerBase::SendStatusReport(ec, kWeaveProfile_Common, Common::kStatus_BadRequest, WEAVE_NO_ERROR);
        ec->Close();
        goto exit;
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
        goto exit;
    }

    // Record that we have a request in process.
    server->mCurClientOp = ec;

    // Decode and dispatch the message.
    switch (msgType)
    {
    case kMsgType_CreateFabric:

        // Return an error if the node is already a member of a fabric.
        if (server->FabricState->FabricId != 0)
        {
            server->SendStatusReport(kWeaveProfile_FabricProvisioning, kStatusCode_AlreadyMemberOfFabric);
            ExitNow();
        }

        // Create a new fabric.
        err = server->FabricState->CreateFabric();
        SuccessOrExit(err);

        // Call the application to perform any creation-time operations (such as address assignment).
        // Note that this can fail, in which case we abort the fabric creation.
        err = delegate->HandleCreateFabric();
        if (err != WEAVE_NO_ERROR)
            server->FabricState->ClearFabricState();
        SuccessOrExit(err);

        break;

    case kMsgType_LeaveFabric:

        // Return an error if the node is not a member of a fabric.
        if (server->FabricState->FabricId == 0)
        {
            server->SendStatusReport(kWeaveProfile_FabricProvisioning, kStatusCode_NotMemberOfFabric);
            ExitNow();
        }

        // Clear the fabric state.
        server->FabricState->ClearFabricState();

        // Call the application to perform any leave-time operations.
        err = delegate->HandleLeaveFabric();
        SuccessOrExit(err);

        break;

    case kMsgType_GetFabricConfig:

        // Return an error if the node is not a member of a fabric.
        if (server->FabricState->FabricId == 0)
        {
            server->SendStatusReport(kWeaveProfile_FabricProvisioning, kStatusCode_NotMemberOfFabric);
            ExitNow();
        }

        // Get the encoded fabric state from the fabric state object.
        PacketBuffer::Free(msgBuf);
        msgBuf = PacketBuffer::New();
        VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);
        uint32_t fabricStateLen;
        err = server->FabricState->GetFabricState(msgBuf->Start(), msgBuf->AvailableDataLength(), fabricStateLen);
        SuccessOrExit(err);
        msgBuf->SetDataLength(fabricStateLen);

        // Send the get fabric config response.
        err = server->mCurClientOp->SendMessage(kWeaveProfile_FabricProvisioning, kMsgType_GetFabricConfigComplete, msgBuf, 0);
        SuccessOrExit(err);
        msgBuf = NULL;

        server->mCurClientOp->Close();
        server->mCurClientOp = NULL;

        err = delegate->HandleGetFabricConfig();
        SuccessOrExit(err);

        break;

    case kMsgType_JoinExistingFabric:

        // Return an error if the node is already a member of a fabric.
        if (server->FabricState->FabricId != 0)
        {
            server->SendStatusReport(kWeaveProfile_FabricProvisioning, kStatusCode_AlreadyMemberOfFabric);
            ExitNow();
        }

        // Join an existing fabric identified by the supplied fabric state.  Right now the only possible
        // reason for this to fail is bad input data.
        err = server->FabricState->JoinExistingFabric(msgBuf->Start(), msgBuf->DataLength());
        if (err != WEAVE_NO_ERROR)
        {
            server->SendStatusReport(kWeaveProfile_FabricProvisioning, kStatusCode_InvalidFabricConfig);
            ExitNow();
        }

        // Call the application to perform any join-time operations (such as address assignment).
        // Note that this can fail, in which case we abort the fabric join.
        err = delegate->HandleJoinExistingFabric();
        if (err != WEAVE_NO_ERROR)
            server->FabricState->ClearFabricState();
        SuccessOrExit(err);

        break;

    default:
        server->SendStatusReport(kWeaveProfile_Common, Common::kStatus_BadRequest);
        break;
    }

exit:

    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);

    if (err != WEAVE_NO_ERROR && server->mCurClientOp != NULL && ec == server->mCurClientOp)
    {
        uint16_t statusCode = (err == WEAVE_ERROR_INVALID_MESSAGE_LENGTH)
                ? Common::kStatus_BadRequest
                : Common::kStatus_InternalError;
        server->SendStatusReport(kWeaveProfile_Common, statusCode, err);
    }
}

void FabricProvisioningDelegate::EnforceAccessControl(ExchangeContext *ec, uint32_t msgProfileId, uint8_t msgType,
        const WeaveMessageInfo *msgInfo, AccessControlResult& result)
{
    // If the result has not already been determined by a subclass...
    if (result == kAccessControlResult_NotDetermined)
    {
        switch (msgType)
        {
#if WEAVE_CONFIG_REQUIRE_AUTH_FABRIC_PROV

        case kMsgType_CreateFabric:
        case kMsgType_JoinExistingFabric:
            if (msgInfo->PeerAuthMode == kWeaveAuthMode_CASE_AccessToken ||
                (msgInfo->PeerAuthMode == kWeaveAuthMode_PASE_PairingCode && !IsPairedToAccount()))
            {
                result = kAccessControlResult_Accepted;
            }
            break;

        case kMsgType_LeaveFabric:
        case kMsgType_GetFabricConfig:
            if (msgInfo->PeerAuthMode == kWeaveAuthMode_CASE_AccessToken)
            {
                result = kAccessControlResult_Accepted;
            }
            break;

#else // WEAVE_CONFIG_REQUIRE_AUTH_FABRIC_PROV

        case kMsgType_CreateFabric:
        case kMsgType_JoinExistingFabric:
        case kMsgType_LeaveFabric:
        case kMsgType_GetFabricConfig:
            result = kAccessControlResult_Accepted;
            break;

#endif // WEAVE_CONFIG_REQUIRE_AUTH_FABRIC_PROV

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
bool FabricProvisioningDelegate::IsPairedToAccount() const
{
    return false;
}




} /* namespace FabricProvisioning */
} /* namespace Profiles */
} /* namespace Weave */
} /* namespace nl */
