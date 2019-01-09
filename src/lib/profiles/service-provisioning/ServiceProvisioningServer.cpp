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
 *      This file implements objects for a Weave Service Provisioning
 *      profile unsolicited responder (server).
 *
 */

#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Support/CodeUtils.h>
#include <Weave/Core/WeaveEncoding.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include "ServiceProvisioning.h"
#include <Weave/Profiles/common/CommonProfile.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace ServiceProvisioning {

using namespace nl::Weave::Encoding;
using namespace nl::Weave::TLV;

ServiceProvisioningServer::ServiceProvisioningServer()
{
    FabricState = NULL;
    ExchangeMgr = NULL;
    mDelegate = NULL;
    mCurClientOp = NULL;
    mCurClientOpBuf = NULL;
    mCurServerOp = NULL;
    mServerOpState = kServerOpState_Idle;
}

WEAVE_ERROR ServiceProvisioningServer::Init(WeaveExchangeManager *exchangeMgr)
{
    FabricState = exchangeMgr->FabricState;
    ExchangeMgr = exchangeMgr;
    mDelegate = NULL;
    mCurClientOp = NULL;
    mCurClientOpBuf = NULL;
    mCurServerOp = NULL;
    mServerOpState = kServerOpState_Idle;

    // Register to receive unsolicited Service Provisioning messages from the exchange manager.
    WEAVE_ERROR err =
        ExchangeMgr->RegisterUnsolicitedMessageHandler(kWeaveProfile_ServiceProvisioning, HandleClientRequest, this);

    return err;
}

WEAVE_ERROR ServiceProvisioningServer::Shutdown()
{
    if (ExchangeMgr != NULL)
        ExchangeMgr->UnregisterUnsolicitedMessageHandler(kWeaveProfile_ServiceProvisioning);

    if (mCurClientOpBuf != NULL)
        PacketBuffer::Free(mCurClientOpBuf);

    FabricState = NULL;
    ExchangeMgr = NULL;
    mDelegate = NULL;
    mCurClientOp = NULL;
    mCurClientOpBuf = NULL;
    mCurServerOp = NULL;
    mServerOpState = kServerOpState_Idle;

    return WEAVE_NO_ERROR;
}

void ServiceProvisioningServer::SetDelegate(ServiceProvisioningDelegate *delegate)
{
    mDelegate = delegate;
}

ServiceProvisioningDelegate* ServiceProvisioningServer::GetDelegate(void) const
{
    return mDelegate;
}

WEAVE_ERROR ServiceProvisioningServer::SendSuccessResponse()
{
    return SendStatusReport(kWeaveProfile_Common, Common::kStatus_Success);
}

WEAVE_ERROR ServiceProvisioningServer::SendStatusReport(uint32_t statusProfileId, uint16_t statusCode, WEAVE_ERROR sysError)
{
    WEAVE_ERROR err;

    VerifyOrExit(mCurClientOp != NULL, err = WEAVE_ERROR_INCORRECT_STATE);

    err = WeaveServerBase::SendStatusReport(mCurClientOp, statusProfileId, statusCode, sysError);

exit:
    mServerOpState = kServerOpState_Idle;

    if (mCurClientOp != NULL)
    {
        mCurClientOp->Close();
        mCurClientOp = NULL;
    }

    if (mCurClientOpBuf != NULL)
    {
        PacketBuffer::Free(mCurClientOpBuf);
        mCurClientOpBuf = NULL;
    }

    return err;
}

WEAVE_ERROR ServiceProvisioningServer::SendPairDeviceToAccountRequest(WeaveConnection *serverCon, uint64_t serviceId, uint64_t fabricId,
                                                                      const char *accountId, uint16_t accountIdLen,
                                                                      const uint8_t *pairingToken, uint16_t pairingTokenLen,
                                                                      const uint8_t *pairingInitData, uint16_t pairingInitDataLen,
                                                                      const uint8_t *deviceInitData, uint16_t deviceInitDataLen)
{
    WEAVE_ERROR                 err     = WEAVE_NO_ERROR;
    PairDeviceToAccountMessage  msg;
    PacketBuffer*               msgBuf  = NULL;
    uint16_t                    msgLen  = sizeof(accountIdLen) + sizeof(pairingTokenLen) + sizeof(pairingInitDataLen) + sizeof(deviceInitDataLen) +
                                          sizeof(serviceId) + sizeof(fabricId) + accountIdLen + pairingTokenLen + pairingInitDataLen + deviceInitDataLen;

    if (mServerOpState != kServerOpState_Idle)
        return WEAVE_ERROR_INCORRECT_STATE;

    msg.ServiceId = serviceId;
    msg.FabricId = fabricId;
    msg.AccountId = accountId;
    msg.AccountIdLen = accountIdLen;
    msg.PairingToken = pairingToken;
    msg.PairingTokenLen = pairingTokenLen;
    msg.PairingInitData = pairingInitData;
    msg.PairingInitDataLen = pairingInitDataLen;
    msg.DeviceInitData = deviceInitData;
    msg.DeviceInitDataLen = deviceInitDataLen;

    // Allocate a buffer for the message.
    msgBuf = PacketBuffer::NewWithAvailableSize(msgLen);
    VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    // Encode the message.
    err = msg.Encode(msgBuf);
    SuccessOrExit(err);

    // Allocate and initialize an exchange context.
    mCurServerOp = ExchangeMgr->NewContext(serverCon, this);
    VerifyOrExit(mCurServerOp != NULL, err = WEAVE_ERROR_NO_MEMORY);

    mCurServerOp->OnMessageReceived = HandleServerResponse;

    if (mCurServerOp->ResponseTimeout == 0)
    {
        mCurServerOp->ResponseTimeout = WEAVE_CONFIG_SERVICE_PROV_RESPONSE_TIMEOUT;
    }
    mCurServerOp->OnResponseTimeout = HandleServerResponseTimeout;
    mCurServerOp->OnConnectionClosed = HandleServerConnectionClosed;
    mCurServerOp->OnKeyError = HandleServerKeyError;

    // Record that a PairDeviceToAccount request is outstanding.
    mServerOpState = kServerOpState_PairDeviceToAccount;

    // Send the PairDeviceToAccount message to the service.
    err = mCurServerOp->SendMessage(kWeaveProfile_ServiceProvisioning, kMsgType_PairDeviceToAccount, msgBuf, 0);
    msgBuf = NULL;

exit:
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);
    if (err != WEAVE_NO_ERROR)
    {
        if (mCurServerOp != NULL)
        {
            mCurServerOp->Close();
            mCurServerOp = NULL;
        }
        mServerOpState = kServerOpState_Idle;
    }
    return err;
}

WEAVE_ERROR ServiceProvisioningServer::SendPairDeviceToAccountRequest(Binding *binding, uint64_t serviceId, uint64_t fabricId,
                                                                      const char *accountId, uint16_t accountIdLen,
                                                                      const uint8_t *pairingToken, uint16_t pairingTokenLen,
                                                                      const uint8_t *pairingInitData, uint16_t pairingInitDataLen,
                                                                      const uint8_t *deviceInitData, uint16_t deviceInitDataLen)
{
    WEAVE_ERROR                 err     = WEAVE_NO_ERROR;
    PairDeviceToAccountMessage  msg;
    PacketBuffer*               msgBuf  = NULL;
    uint16_t                    flags   = 0;
    uint16_t                    msgLen  = sizeof(accountIdLen) + sizeof(pairingTokenLen) + sizeof(pairingInitDataLen) + sizeof(deviceInitDataLen) +
                                          sizeof(serviceId) + sizeof(fabricId) + accountIdLen + pairingTokenLen + pairingInitDataLen + deviceInitDataLen;

    if (mServerOpState != kServerOpState_Idle)
        return WEAVE_ERROR_INCORRECT_STATE;

    msg.ServiceId = serviceId;
    msg.FabricId = fabricId;
    msg.AccountId = accountId;
    msg.AccountIdLen = accountIdLen;
    msg.PairingToken = pairingToken;
    msg.PairingTokenLen = pairingTokenLen;
    msg.PairingInitData = pairingInitData;
    msg.PairingInitDataLen = pairingInitDataLen;
    msg.DeviceInitData = deviceInitData;
    msg.DeviceInitDataLen = deviceInitDataLen;

    // Allocate a buffer for the message.
    msgBuf = PacketBuffer::NewWithAvailableSize(msgLen);
    VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    // Encode the message.
    err = msg.Encode(msgBuf);
    SuccessOrExit(err);

    // Allocate and initialize an exchange context.
    err = binding->NewExchangeContext(mCurServerOp);
    SuccessOrExit(err);

    mCurServerOp->AppState = this;
    mCurServerOp->OnMessageReceived = HandleServerResponse;

    if (mCurServerOp->ResponseTimeout == 0)
    {
        mCurServerOp->ResponseTimeout = WEAVE_CONFIG_SERVICE_PROV_RESPONSE_TIMEOUT;
    }
    mCurServerOp->OnResponseTimeout = HandleServerResponseTimeout;
    mCurServerOp->OnConnectionClosed = HandleServerConnectionClosed;
    mCurServerOp->OnKeyError = HandleServerKeyError;

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    mCurServerOp->OnSendError = HandleServerSendError;
#endif // #if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING

    // Record that a PairDeviceToAccount request is outstanding.
    mServerOpState = kServerOpState_PairDeviceToAccount;

    // TODO: [TT] Not needed for Jay's new bindings WEAV-1870.
    flags = (mCurServerOp->Con == NULL) ? ExchangeContext::kSendFlag_RequestAck : 0;

    // Send the PairDeviceToAccount message to the service.
    err = mCurServerOp->SendMessage(kWeaveProfile_ServiceProvisioning, kMsgType_PairDeviceToAccount, msgBuf, flags);
    msgBuf = NULL;

exit:
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);
    if (err != WEAVE_NO_ERROR)
    {
        mServerOpState = kServerOpState_Idle;
        if (mCurServerOp != NULL)
        {
            mCurServerOp->Close();
            mCurServerOp = NULL;
        }
    }
    return err;
}

#if WEAVE_CONFIG_ENABLE_IFJ_SERVICE_FABRIC_JOIN
WEAVE_ERROR ServiceProvisioningServer::SendIFJServiceFabricJoinRequest(Binding *binding, uint64_t serviceId, uint64_t fabricId,
                                                                       const uint8_t *deviceInitData, uint16_t deviceInitDataLen)
{
    WEAVE_ERROR                 err     = WEAVE_NO_ERROR;
    IFJServiceFabricJoinMessage msg;
    PacketBuffer*               msgBuf  = NULL;
    uint16_t                    msgLen  = sizeof(deviceInitDataLen) + sizeof(serviceId) + sizeof(fabricId) + deviceInitDataLen;

    if (mServerOpState != kServerOpState_Idle)
        return WEAVE_ERROR_INCORRECT_STATE;

    msg.ServiceId = serviceId;
    msg.FabricId = fabricId;
    msg.DeviceInitData = deviceInitData;
    msg.DeviceInitDataLen = deviceInitDataLen;

    // Allocate a buffer for the message.
    msgBuf = PacketBuffer::NewWithAvailableSize(msgLen);
    VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    // Encode the message.
    err = msg.Encode(msgBuf);
    SuccessOrExit(err);

    // Allocate and initialize an exchange context.
    err = binding->NewExchangeContext(mCurServerOp);
    SuccessOrExit(err);

    mCurServerOp->AppState = this;
    mCurServerOp->OnMessageReceived = HandleServerResponse;

    if (mCurServerOp->ResponseTimeout == 0)
    {
        mCurServerOp->ResponseTimeout = WEAVE_CONFIG_SERVICE_PROV_RESPONSE_TIMEOUT;
    }
    mCurServerOp->OnResponseTimeout = HandleServerResponseTimeout;
    mCurServerOp->OnKeyError = HandleServerKeyError;

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    mCurServerOp->OnSendError = HandleServerSendError;
#endif // #if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING

    // Record that a IFJServiceFabricJoin request is outstanding.
    mServerOpState = kServerOpState_IFJServiceFabricJoin;

    // Send the IFJServiceFabricJoin message to the service.
    err = mCurServerOp->SendMessage(kWeaveProfile_ServiceProvisioning, kMsgType_IFJServiceFabricJoin, msgBuf,
                                    nl::Weave::ExchangeContext::kSendFlag_ExpectResponse);
    msgBuf = NULL;

exit:
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);
    if (err != WEAVE_NO_ERROR)
    {
        mServerOpState = kServerOpState_Idle;
        if (mCurServerOp != NULL)
        {
            mCurServerOp->Close();
            mCurServerOp = NULL;
        }
    }

    return err;
}
#endif // WEAVE_CONFIG_ENABLE_IFJ_SERVICE_FABRIC_JOIN

void ServiceProvisioningServer::HandleClientRequest(ExchangeContext *ec, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo,
                                                    uint32_t profileId, uint8_t msgType, PacketBuffer *payload)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    ServiceProvisioningServer *server = (ServiceProvisioningServer *) ec->AppState;
    ServiceProvisioningDelegate *delegate = server->mDelegate;
    uint16_t dataLen = payload->DataLength();
    uint8_t *p = payload->Start();

    // Fail messages for the wrong profile. This shouldn't happen, but better safe than sorry.
    if (profileId != kWeaveProfile_ServiceProvisioning)
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
    case kMsgType_RegisterServicePairAccount:

        err = RegisterServicePairAccountMessage::Decode(payload, server->mCurClientOpMsg.RegisterServicePairAccount);
        SuccessOrExit(err);

        server->mCurClientOpBuf = payload;
        payload = NULL;

        err = delegate->HandleRegisterServicePairAccount(server->mCurClientOpMsg.RegisterServicePairAccount);
        SuccessOrExit(err);

        break;

    case kMsgType_UpdateService:
        err = UpdateServiceMessage::Decode(payload, server->mCurClientOpMsg.UpdateService);
        SuccessOrExit(err);

        server->mCurClientOpBuf = payload;
        payload = NULL;

        err = delegate->HandleUpdateService(server->mCurClientOpMsg.UpdateService);
        SuccessOrExit(err);

        break;

    case kMsgType_UnregisterService:
    {
        uint64_t serviceId;

        VerifyOrExit(dataLen == 8, err = WEAVE_ERROR_INVALID_MESSAGE_LENGTH);
        serviceId = LittleEndian::Get64(p);

        err = delegate->HandleUnregisterService(serviceId);
        SuccessOrExit(err);

        break;
    }

    default:
        server->SendStatusReport(kWeaveProfile_Common, Common::kStatus_BadRequest);
        break;
    }

exit:

    if (payload != NULL)
        PacketBuffer::Free(payload);

    if (err != WEAVE_NO_ERROR && server->mCurClientOp != NULL && ec == server->mCurClientOp)
    {
        uint16_t statusCode = (err == WEAVE_ERROR_INVALID_MESSAGE_LENGTH)
                ? Common::kStatus_BadRequest
                : Common::kStatus_InternalError;
        server->SendStatusReport(kWeaveProfile_Common, statusCode, err);
    }
}

void ServiceProvisioningServer::HandleServerResponse(ExchangeContext *ec, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo,
                                                     uint32_t profileId, uint8_t msgType, PacketBuffer *payload)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WEAVE_ERROR delegateErr = WEAVE_NO_ERROR;
    StatusReport statusReport;
    ServiceProvisioningServer *server = (ServiceProvisioningServer *) ec->AppState;

    // Sanity check that the passed-in exchange context is in fact the one that represents the current
    // outstanding server operation. If not, it'll get closed at exit. If it does match, we'll null ec
    // to prevent it from getting closed at exit.
    VerifyOrExit(ec == server->mCurServerOp, err = WEAVE_NO_ERROR);
    ec = NULL;

    // Verify the message is expected.
    VerifyOrExit((profileId == kWeaveProfile_Common && msgType == nl::Weave::Profiles::Common::kMsgType_StatusReport) ||
                 (profileId == kWeaveProfile_StatusReport_Deprecated),
                 err = WEAVE_ERROR_INVALID_MESSAGE_TYPE);

    err = StatusReport::parse(payload, statusReport);
    SuccessOrExit(err);

    // Free the payload here to reduce buffer pressure.
    PacketBuffer::Free(payload);
    payload = NULL;

    delegateErr = (statusReport.mProfileId == kWeaveProfile_Common && statusReport.mStatusCode == Common::kStatus_Success)
        ? WEAVE_NO_ERROR
        : WEAVE_ERROR_STATUS_REPORT_RECEIVED;

    server->HandleServiceProvisioningOpResult(delegateErr, statusReport.mProfileId, statusReport.mStatusCode);

exit:
    // Free the payload first to reduce buffer pressure.
    if (payload != NULL)
        PacketBuffer::Free(payload);
    if (err != WEAVE_NO_ERROR)
        server->HandleServiceProvisioningOpResult(err, 0, 0);
    if (ec != NULL)
        ec->Close();
}

void ServiceProvisioningServer::HandleServerResponseTimeout(ExchangeContext *ec)
{
    ServiceProvisioningServer *server = (ServiceProvisioningServer *) ec->AppState;

    // Sanity check that the passed-in exchange context is in fact the one that represents the current
    // outstanding server operation. If not, it'll get closed at exit. If it does match, we'll null ec
    // to prevent it from getting closed at exit.
    VerifyOrExit(ec == server->mCurServerOp, );
    ec = NULL;

    server->HandleServiceProvisioningOpResult(WEAVE_ERROR_TIMEOUT, 0, 0);

exit:
    if (ec != NULL)
        ec->Close();
}

void ServiceProvisioningServer::HandleServerConnectionClosed(ExchangeContext *ec, WeaveConnection *con, WEAVE_ERROR conErr)
{
    ServiceProvisioningServer *server = (ServiceProvisioningServer *) ec->AppState;

    // Sanity check that the passed-in exchange context is in fact the one that represents the current
    // outstanding server operation. If not, it'll get closed at exit. If it does match, we'll null ec
    // to prevent it from getting closed at exit.
    VerifyOrExit(ec == server->mCurServerOp, );
    ec = NULL;

    // No error on connection close means the service simply closed the connection without responding. In that case
    // deliver WEAVE_ERROR_CONNECTION_CLOSED_UNEXPECTEDLY to the delegate.
    if (conErr == WEAVE_NO_ERROR)
        conErr = WEAVE_ERROR_CONNECTION_CLOSED_UNEXPECTEDLY;

    server->HandleServiceProvisioningOpResult(conErr, 0, 0);

exit:
    if (ec != NULL)
        ec->Close();
}

void ServiceProvisioningServer::HandleServerKeyError(ExchangeContext *ec, WEAVE_ERROR keyErr)
{
    ServiceProvisioningServer *server = (ServiceProvisioningServer *) ec->AppState;

    // Sanity check that the passed-in exchange context is in fact the one that represents the current
    // outstanding server operation. If not, it'll get closed at exit. If it does match, we'll null ec
    // to prevent it from getting closed at exit.
    VerifyOrExit(ec == server->mCurServerOp, );
    ec = NULL;

    server->HandleServiceProvisioningOpResult(keyErr, 0, 0);

exit:
    if (ec != NULL)
        ec->Close();
}

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
void ServiceProvisioningServer::HandleServerSendError(ExchangeContext *ec, WEAVE_ERROR err, void *msgCtxt)
{
    ServiceProvisioningServer *server = (ServiceProvisioningServer *) ec->AppState;

    // Sanity check that the passed-in exchange context is in fact the one that represents the current
    // outstanding server operation. If not, it'll get closed at exit. If it does match, we'll null ec
    // to prevent it from getting closed at exit.
    VerifyOrExit(ec == server->mCurServerOp, );
    ec = NULL;

    server->HandleServiceProvisioningOpResult(err, 0, 0);

exit:
    if (ec != NULL)
        ec->Close();
}
#endif // #if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING

void ServiceProvisioningServer::HandleServiceProvisioningOpResult(WEAVE_ERROR localErr, uint32_t serverStatusProfileId, uint16_t serverStatusCode)
{
#if WEAVE_CONFIG_ENABLE_IFJ_SERVICE_FABRIC_JOIN
    uint8_t prevOpState = mServerOpState;
#endif
    mServerOpState = kServerOpState_Idle;
    mCurServerOp->Close();
    mCurServerOp = NULL;

    if (mDelegate)
    {
#if WEAVE_CONFIG_ENABLE_IFJ_SERVICE_FABRIC_JOIN
        if (prevOpState == kServerOpState_IFJServiceFabricJoin)
        {
            mDelegate->HandleIFJServiceFabricJoinResult(localErr, serverStatusProfileId, serverStatusCode);
        }
        else
#endif // WEAVE_CONFIG_ENABLE_IFJ_SERVICE_FABRIC_JOIN
        {
            mDelegate->HandlePairDeviceToAccountResult(localErr, serverStatusProfileId, serverStatusCode);
        }
    }
}

bool ServiceProvisioningServer::IsValidServiceConfig(const uint8_t *serviceConfig, uint16_t serviceConfigLen)
{
    WEAVE_ERROR err;
    nl::Weave::TLV::TLVReader reader;

    reader.Init(serviceConfig, serviceConfigLen);

    err = reader.Next();
    SuccessOrExit(err);

    VerifyOrExit(reader.GetTag() == ProfileTag(kWeaveProfile_ServiceProvisioning, kTag_ServiceConfig), err = WEAVE_ERROR_INVALID_TLV_TAG);

    {
        TLVType topLevelContainer;
        bool caCertsPresent = false, dirEndPointPresent = false;

        err = reader.EnterContainer(topLevelContainer);
        SuccessOrExit(err);

        while ((err = reader.Next()) == WEAVE_NO_ERROR)
        {
            uint64_t elemTag = reader.GetTag();

            if (!IsContextTag(elemTag))
                continue;

            switch (TagNumFromTag(elemTag))
            {
            case kTag_ServiceConfig_CACerts:
                VerifyOrExit(reader.GetType() == kTLVType_Array, err = WEAVE_ERROR_INVALID_TLV_ELEMENT);
                caCertsPresent = true;
                break;
            case kTag_ServiceConfig_DirectoryEndPoint:
                VerifyOrExit(reader.GetType() == kTLVType_Structure, err = WEAVE_ERROR_INVALID_TLV_ELEMENT);
                dirEndPointPresent = true;
                break;
            default:
                // Ignore unknown elements
                break;
            }
        }

        if (err != WEAVE_END_OF_TLV)
            ExitNow();

        err = reader.ExitContainer(topLevelContainer);
        SuccessOrExit(err);

        VerifyOrExit(caCertsPresent && dirEndPointPresent, err = WEAVE_ERROR_MISSING_TLV_ELEMENT);
    }

exit:
    return err == WEAVE_NO_ERROR;
}

void ServiceProvisioningDelegate::EnforceAccessControl(ExchangeContext *ec, uint32_t msgProfileId, uint8_t msgType,
        const WeaveMessageInfo *msgInfo, AccessControlResult& result)
{
    enum { kServiceProvisioningEndpointId = 0x18B4300200000010ull };

    // If the result has not already been determined by a subclass...
    if (result == kAccessControlResult_NotDetermined)
    {
        switch (msgType)
        {
#if WEAVE_CONFIG_REQUIRE_AUTH_SERVICE_PROV

        case kMsgType_RegisterServicePairAccount:
            if (msgInfo->PeerAuthMode == kWeaveAuthMode_PASE_PairingCode && !IsPairedToAccount())
            {
                result = kAccessControlResult_Accepted;
            }
            break;

        case kMsgType_UpdateService:
            if (msgInfo->PeerAuthMode == kWeaveAuthMode_CASE_AccessToken)
            {
                result = kAccessControlResult_Accepted;
            }
            break;

        case kMsgType_UnregisterService:
            if (msgInfo->PeerAuthMode == kWeaveAuthMode_CASE_AccessToken ||
                (msgInfo->PeerAuthMode == kWeaveAuthMode_CASE_ServiceEndPoint && msgInfo->SourceNodeId == kServiceProvisioningEndpointId))
            {
                result = kAccessControlResult_Accepted;
            }
            break;

#else // WEAVE_CONFIG_REQUIRE_AUTH_SERVICE_PROV

        case kMsgType_RegisterServicePairAccount:
        case kMsgType_UpdateService:
        case kMsgType_UnregisterService:
            result = kAccessControlResult_Accepted;
            break;

#endif // WEAVE_CONFIG_REQUIRE_AUTH_SERVICE_PROV

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
bool ServiceProvisioningDelegate::IsPairedToAccount() const
{
    return false;
}

} // ServiceProvisioning
} // namespace Profiles
} // namespace Weave
} // namespace nl
