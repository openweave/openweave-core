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
 *      This file implements the Weave Network Provisioning Profile, used
 *      to configure network interfaces.
 *
 *      The Network Provisioning Profile facilitates client-server
 *      operations such that the client (the controlling device) can
 *      trigger specific network functionality on the server (the device
 *      undergoing network provisioning).  These operations revolve around
 *      the steps necessary to provision the server device's network
 *      interfaces (such as 802.15.4/Thread and 802.11/Wi-Fi) provisioned
 *      such that the device may participate in those networks.  This includes
 *      scanning and specifying network names and security credentials.
 *
 */
#include <Weave/Core/WeaveCore.h>
#include <Weave/Support/CodeUtils.h>
#include <Weave/Core/WeaveEncoding.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include "NetworkProvisioning.h"
#include <Weave/Profiles/common/CommonProfile.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace NetworkProvisioning {

using namespace nl::Weave::Encoding;
using namespace nl::Weave::TLV;

NetworkProvisioningServer::NetworkProvisioningServer()
{
    FabricState = NULL;
    ExchangeMgr = NULL;
    mCurOp = NULL;
    mDelegate = NULL;
}

/**
 * Initialize the Network Provisioning Server state and register to receive
 * Network Provisioning messages.
 *
 * @param[in] exchangeMgr   A pointer to the system Weave Exchange Manager.
 *
 * @retval #WEAVE_ERROR_TOO_MANY_UNSOLICITED_MESSAGE_HANDLERS   If too many message handlers have
 *                                                              already been registered.
 * @retval #WEAVE_NO_ERROR                                      On success.
 */
WEAVE_ERROR NetworkProvisioningServer::Init(WeaveExchangeManager *exchangeMgr)
{
    ExchangeMgr = exchangeMgr;
    FabricState = exchangeMgr->FabricState;
    mCurOp = NULL;
    mDelegate = NULL;
    mLastOpResult.StatusProfileId = kWeaveProfile_Common;
    mLastOpResult.StatusCode = Common::kStatus_Success;
    mLastOpResult.SysError = WEAVE_NO_ERROR;

    // Register to receive unsolicited Network Provisioning messages from the exchange manager.
    WEAVE_ERROR err =
        ExchangeMgr->RegisterUnsolicitedMessageHandler(kWeaveProfile_NetworkProvisioning, HandleRequest, this);

    return err;
}
/**
 * Shutdown the Network Provisioning Server.
 *
 * @retval #WEAVE_NO_ERROR  On success.
 */
// TODO: Additional documentation detail required (i.e. how this function impacts object lifecycle).
WEAVE_ERROR NetworkProvisioningServer::Shutdown()
{
    ExchangeMgr = NULL;
    FabricState = NULL;
    mCurOp = NULL;
    mDelegate = NULL;

    return WEAVE_NO_ERROR;
}

/**
 * Set the delegate to process Network Provisioning Server events.
 *
 * @param[in] delegate  A pointer to the Network Provisioning Delegate.
 */
void NetworkProvisioningServer::SetDelegate(NetworkProvisioningDelegate *delegate)
{
    mDelegate = delegate;
}

/**
 * Send a Network Scan Complete response message containing the results of the scan.
 *
 * @param[in]   resultCount     The number of scan results.
 * @param[in]   scanResultsTLV  The scan results.
 *
 * @retval #WEAVE_ERROR_INCORRECT_STATE     If the Network Provisioning Server is not initialized correctly.
 * @retval #WEAVE_ERROR_BUFFER_TOO_SMALL    If the results buffer is not large enough.
 * @retval #WEAVE_NO_ERROR                  On success.
 * @retval other                            Other Weave or platform-specific error codes indicating that an error
 *                                          occurred preventing the device from sending the Scan Complete response.
 */
WEAVE_ERROR NetworkProvisioningServer::SendNetworkScanComplete(uint8_t resultCount, PacketBuffer *scanResultsTLV)
{
    return SendCompleteWithNetworkList(kMsgType_NetworkScanComplete, resultCount, scanResultsTLV);
}

/**
 * Send a Get Networks Complete message containing the previously scanned networks.
 *
 * @param[in]   resultCount     The number of scan results.
 * @param[in]   scanResultsTLV  The scan results.
 *
 * @retval #WEAVE_ERROR_INCORRECT_STATE     If the Network Provisioning Server is not initialized correctly.
 * @retval #WEAVE_ERROR_BUFFER_TOO_SMALL    If the results buffer is not large enough.
 * @retval #WEAVE_NO_ERROR                  On success.
 * @retval other                            Other Weave or platform-specific error codes indicating that an error
 *                                          occurred preventing the device from sending the Get Networks Complete message.
 */
WEAVE_ERROR NetworkProvisioningServer::SendGetNetworksComplete(uint8_t resultCount, PacketBuffer *scanResultsTLV)
{
    return SendCompleteWithNetworkList(kMsgType_GetNetworksComplete, resultCount, scanResultsTLV);
}

WEAVE_ERROR NetworkProvisioningServer::SendCompleteWithNetworkList(uint8_t msgType, int8_t resultCount, PacketBuffer *resultTLV)
{
    WEAVE_ERROR err;
    uint8_t *p;

    VerifyOrExit(mDelegate != NULL, err = WEAVE_ERROR_INCORRECT_STATE);
    VerifyOrExit(mCurOp != NULL, err = WEAVE_ERROR_INCORRECT_STATE);

    if (!resultTLV->EnsureReservedSize(WEAVE_SYSTEM_CONFIG_HEADER_RESERVE_SIZE + 1))
        ExitNow(err = WEAVE_ERROR_BUFFER_TOO_SMALL);

    p = resultTLV->Start() - 1;
    resultTLV->SetStart(p);

    *p = resultCount;

    err = mCurOp->SendMessage(kWeaveProfile_NetworkProvisioning, msgType, resultTLV, 0);
    resultTLV = NULL;

    mLastOpResult.StatusProfileId = kWeaveProfile_Common;
    mLastOpResult.StatusCode = Common::kStatus_Success;
    mLastOpResult.SysError = WEAVE_NO_ERROR;

exit:
    if (mCurOp != NULL)
    {
        mCurOp->Close();
        mCurOp = NULL;
    }
    if (resultTLV != NULL)
        PacketBuffer::Free(resultTLV);
    return err;
}

/**
 * Send an Add Network Complete message if the network was successfully added.
 *
 * @param[in]   networkId   The ID of the added network.
 *
 * @retval #WEAVE_ERROR_INCORRECT_STATE     If the Network Provisioning Server is not initialized correctly.
 * @retval #WEAVE_ERROR_NO_MEMORY           On failure to allocate an PacketBuffer.
 * @retval #WEAVE_NO_ERROR                  On success.
 * @retval other                            Other Weave or platform-specific error codes indicating that an error
 *                                          occurred preventing the device from sending the Add Network Complete message.
 */
WEAVE_ERROR NetworkProvisioningServer::SendAddNetworkComplete(uint32_t networkId)
{
    WEAVE_ERROR     err;
    PacketBuffer*   respBuf = NULL;
    uint8_t*        p;
    uint8_t         respLen = 4;

    VerifyOrExit(mDelegate != NULL, err = WEAVE_ERROR_INCORRECT_STATE);
    VerifyOrExit(mCurOp != NULL, err = WEAVE_ERROR_INCORRECT_STATE);

    respBuf = PacketBuffer::NewWithAvailableSize(respLen);
    VerifyOrExit(respBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    p = respBuf->Start();
    LittleEndian::Write32(p, networkId);
    respBuf->SetDataLength(respLen);

    err = mCurOp->SendMessage(kWeaveProfile_NetworkProvisioning, kMsgType_AddNetworkComplete, respBuf, 0);
    respBuf = NULL;

    mLastOpResult.StatusProfileId = kWeaveProfile_Common;
    mLastOpResult.StatusCode = Common::kStatus_Success;
    mLastOpResult.SysError = WEAVE_NO_ERROR;

exit:
    if (mCurOp != NULL)
    {
        mCurOp->Close();
        mCurOp = NULL;
    }
    if (respBuf != NULL)
        PacketBuffer::Free(respBuf);
    return err;
}

/**
 * Send a success response to a Network Provisioning request.
 *
 * @retval #WEAVE_ERROR_INCORRECT_STATE If there is no request being processed.
 * @retval #WEAVE_NO_ERROR              On success.
 * @retval other                        Other Weave or platform-specific error codes indicating that an error
 *                                      occurred preventing the device from sending the success response.
 */
WEAVE_ERROR NetworkProvisioningServer::SendSuccessResponse()
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
 *                                      occurred preventing the device from sending the status report.
 */
WEAVE_ERROR NetworkProvisioningServer::SendStatusReport(uint32_t statusProfileId, uint16_t statusCode, WEAVE_ERROR sysError)
{
    WEAVE_ERROR err;

    VerifyOrExit(mDelegate != NULL, err = WEAVE_ERROR_INCORRECT_STATE);
    VerifyOrExit(mCurOp != NULL, err = WEAVE_ERROR_INCORRECT_STATE);

    err = WeaveServerBase::SendStatusReport(mCurOp, statusProfileId, statusCode, sysError);

    mLastOpResult.StatusProfileId = statusProfileId;
    mLastOpResult.StatusCode = statusCode;
    mLastOpResult.SysError = sysError;

exit:

    if (mCurOp != NULL)
    {
        mCurOp->Close();
        mCurOp = NULL;
    }

    return err;
}

void NetworkProvisioningServer::HandleRequest(ExchangeContext *ec, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo, uint32_t profileId,
        uint8_t msgType, PacketBuffer *payload)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    NetworkProvisioningServer *server = (NetworkProvisioningServer *) ec->AppState;
    NetworkProvisioningDelegate *delegate = server->mDelegate;
    uint32_t networkId;
    uint16_t rendezvousMode;
    uint8_t networkType;
    uint8_t flags;
    uint16_t dataLen;
    const uint8_t *p;

    // Fail messages for the wrong profile. This shouldn't happen, but better safe than sorry.
    if (profileId != kWeaveProfile_NetworkProvisioning)
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
    if (server->mCurOp != NULL)
    {
        WeaveServerBase::SendStatusReport(ec, kWeaveProfile_Common, Common::kStatus_Busy, WEAVE_NO_ERROR);
        ec->Close();
        ExitNow();
    }

    // Record that we have a request in process.
    server->mCurOp = ec;
    server->mCurOpType = msgType;

    dataLen = payload->DataLength();
    p = payload->Start();

    // Decode and dispatch the message.
    switch (msgType)
    {
    case kMsgType_ScanNetworks:
        VerifyOrExit(dataLen >= 1, err = WEAVE_ERROR_INVALID_MESSAGE_LENGTH);
        networkType = Get8(p);
        PacketBuffer::Free(payload);
        payload = NULL;
        err = delegate->HandleScanNetworks(networkType);
        break;

    case kMsgType_AddNetwork:
        VerifyOrExit(dataLen >= 1, err = WEAVE_ERROR_INVALID_MESSAGE_LENGTH);
        err = delegate->HandleAddNetwork(payload);
        payload = NULL;
        break;

    case kMsgType_UpdateNetwork:
        VerifyOrExit(dataLen >= 1, err = WEAVE_ERROR_INVALID_MESSAGE_LENGTH);
        err = delegate->HandleUpdateNetwork(payload);
        payload = NULL;
        break;

    case kMsgType_RemoveNetwork:
    case kMsgType_EnableNetwork:
    case kMsgType_DisableNetwork:
    case kMsgType_TestConnectivity:
        VerifyOrExit(dataLen >= 4, err = WEAVE_ERROR_INVALID_MESSAGE_LENGTH);
        networkId = LittleEndian::Read32(p);
        PacketBuffer::Free(payload);
        payload = NULL;
        if (msgType == kMsgType_RemoveNetwork)
            err = delegate->HandleRemoveNetwork(networkId);
        else if (msgType == kMsgType_EnableNetwork)
            err = delegate->HandleEnableNetwork(networkId);
        else if (msgType == kMsgType_DisableNetwork)
            err = delegate->HandleDisableNetwork(networkId);
        else if (msgType == kMsgType_TestConnectivity)
            err = delegate->HandleTestConnectivity(networkId);
        break;

    case kMsgType_GetNetworks:
        VerifyOrExit(dataLen >= 1, err = WEAVE_ERROR_INVALID_MESSAGE_LENGTH);
        flags = *p;
        PacketBuffer::Free(payload);
        payload = NULL;

#if WEAVE_CONFIG_REQUIRE_AUTH_NETWORK_PROV

        // According to Weave Device Access Control Policy,
        // When servicing a GetNetworks message from a peer that has authenticated using PASE/PairingCode,
        // a device in an unpaired state must reject the message with an access denied error if the peer has
        // set the IncludeCredentials flag.
        if (msgInfo->PeerAuthMode == kWeaveAuthMode_PASE_PairingCode && !delegate->IsPairedToAccount()
                && (flags & kGetNetwork_IncludeCredentials) != 0)
        {
            server->SendStatusReport(kWeaveProfile_Common, Common::kStatus_AccessDenied);
            break;
        }
#endif // WEAVE_CONFIG_REQUIRE_AUTH_NETWORK_PROV

        err = delegate->HandleGetNetworks(flags);
        break;

    case kMsgType_SetRendezvousMode:
        VerifyOrExit(dataLen >= 2, err = WEAVE_ERROR_INVALID_MESSAGE_LENGTH);
        rendezvousMode = LittleEndian::Read16(p);

        PacketBuffer::Free(payload);
        payload = NULL;

#if WEAVE_CONFIG_REQUIRE_AUTH_NETWORK_PROV

        // Per device access control policy, when servicing a SetRendezvousMode message from a peer that has authenticated using
        // PASE/PairingCode, a device in a paired state should reject a SetRendezvousMode message with an access denied error if
        // the requested mode is not 0--i.e. if the peer requests to *enable* any rendezvous mode.
        if (rendezvousMode != 0 && msgInfo->PeerAuthMode == kWeaveAuthMode_PASE_PairingCode && delegate->IsPairedToAccount())
        {
            server->SendStatusReport(kWeaveProfile_Common, Common::kStatus_AccessDenied);
            break;
        }

#endif // WEAVE_CONFIG_REQUIRE_AUTH_NETWORK_PROV

        err = delegate->HandleSetRendezvousMode(rendezvousMode);
        break;

    case kMsgType_GetLastResult:
        err = server->SendStatusReport(server->mLastOpResult.StatusProfileId, server->mLastOpResult.StatusCode, server->mLastOpResult.SysError);
        break;

    default:
        server->SendStatusReport(kWeaveProfile_Common, Common::kStatus_BadRequest);
        break;
    }

exit:

    if (err != WEAVE_NO_ERROR && server->mCurOp != NULL)
    {
        uint16_t statusCode =
                (err == WEAVE_ERROR_INVALID_MESSAGE_LENGTH) ? Common::kStatus_BadRequest : Common::kStatus_InternalError;
        server->SendStatusReport(kWeaveProfile_Common, statusCode, err);
    }

    if (payload != NULL)
        PacketBuffer::Free(payload);
}

void NetworkProvisioningDelegate::EnforceAccessControl(ExchangeContext *ec, uint32_t msgProfileId, uint8_t msgType,
        const WeaveMessageInfo *msgInfo, AccessControlResult& result)
{
    // If the result has not already been determined by a subclass...
    if (result == kAccessControlResult_NotDetermined)
    {
        switch (msgType)
        {
        case kMsgType_ScanNetworks:
        case kMsgType_AddNetwork:
        case kMsgType_UpdateNetwork:
        case kMsgType_RemoveNetwork:
        case kMsgType_EnableNetwork:
        case kMsgType_DisableNetwork:
        case kMsgType_TestConnectivity:
        case kMsgType_GetNetworks:
        case kMsgType_GetLastResult:
#if WEAVE_CONFIG_REQUIRE_AUTH_NETWORK_PROV
            if (msgInfo->PeerAuthMode == kWeaveAuthMode_CASE_AccessToken ||
                (msgInfo->PeerAuthMode == kWeaveAuthMode_PASE_PairingCode && !IsPairedToAccount()))
#endif // WEAVE_CONFIG_REQUIRE_AUTH_NETWORK_PROV
            {
                result = kAccessControlResult_Accepted;
            }
            break;
        case kMsgType_SetRendezvousMode:
#if WEAVE_CONFIG_REQUIRE_AUTH_NETWORK_PROV
            if (msgInfo->PeerAuthMode == kWeaveAuthMode_CASE_AccessToken ||
                msgInfo->PeerAuthMode == kWeaveAuthMode_PASE_PairingCode)
#endif // WEAVE_CONFIG_REQUIRE_AUTH_NETWORK_PROV
            {
                result = kAccessControlResult_Accepted;
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
bool NetworkProvisioningDelegate::IsPairedToAccount() const
{
    return false;
}

} // NetworkProvisioning
} // namespace Profiles
} // namespace Weave
} // namespace nl
