/*
 *
 *    Copyright (c) 2019 Google LLC.
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
 *      This file implements the Certificate Provisioning Protocol, used to
 *      get new Weave operational device certificate from the CA service.
 *
 */

#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Support/CodeUtils.h>
#include <Weave/Core/WeaveEncoding.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include "WeaveCertProvisioning.h"
#include <Weave/Profiles/common/CommonProfile.h>
#include <Weave/Support/crypto/HashAlgos.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace Security {
namespace CertProvisioning {

using namespace nl::Weave::Crypto;
using namespace nl::Weave::Encoding;
using namespace nl::Weave::TLV;
using namespace nl::Weave::Platform::Security;

/**
 * @brief
 *   Generate GetCertificateRequest message.
 *
 * @details
 *   This function generates Weave GetCertificateRequest stracture encoded in the Weave TLV format.
 *
 *  @param[in]  msgBuf               A pointer to the PacketBuffer object holding the GetCertificateRequest message.
 *  @param[in]  reqType              Get certificate request type.
 *  @param[in]  opAuthDelegate       A pointer to the Certificate Provisioning operational authentication delegate object.
 *  @param[in]  manufAttestDelegate  A pointer to the Certificate Provisioning manufacturer attestation delegate object.
 *  @param[in]  eventCallback        A pointer to a function that will be called by this fanction to deliver API
 *                                   events to the application.
 *  @param[in]  appState             A pointer to an application-defined object which will be passed back
 *                                   to the application whenever an API event occurs.
 *
 *  @retval #WEAVE_NO_ERROR          If GetCertificateRequest was successfully generated.
 */
NL_DLL_EXPORT WEAVE_ERROR GenerateGetCertificateRequest(PacketBuffer * msgBuf, uint8_t reqType,
                                                        WeaveCertProvAuthDelegate * opAuthDelegate,
                                                        WeaveCertProvAuthDelegate * manufAttestDelegate,
                                                        EventCallback eventCallback, void * appState)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TLVWriter writer;
    TLVType containerType;
    SHA256 sha256;
    uint8_t *tbsDataStart;
    uint16_t tbsDataStartOffset;
    uint16_t tbsDataLen;
    uint8_t tbsHash[SHA256::kHashLength];
    bool manufAttestRequired = (reqType == kReqType_GetInitialOpDeviceCert);

    WeaveLogDetail(SecurityManager, "CertProvisioning:GenerateGetCertificateRequest");

    VerifyOrExit(reqType == kReqType_GetInitialOpDeviceCert ||
                 reqType == kReqType_RotateCert, err = WEAVE_ERROR_INVALID_ARGUMENT);

    VerifyOrExit(opAuthDelegate != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    if (manufAttestRequired)
    {
        VerifyOrExit(manufAttestDelegate != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);
    }

    writer.Init(msgBuf, msgBuf->AvailableDataLength());

    err = writer.StartContainer(AnonymousTag, kTLVType_Structure, containerType);
    SuccessOrExit(err);

    // Record the offset and the start of the TBS (To-Be-Signed) portion of the message.
    tbsDataStartOffset = writer.GetLengthWritten();
    tbsDataStart = msgBuf->Start() + tbsDataStartOffset;

    // Request type.
    err = writer.Put(ContextTag(kTag_GetCertReqMsg_ReqType), reqType);
    SuccessOrExit(err);

    // Get Certificate authorization information.
    if (eventCallback != NULL)
    {
        InEventParam inParam;
        OutEventParam outParam;
        inParam.Clear();
        inParam.Source = NULL;
        inParam.PrepareAuthorizeInfo.Writer = &writer;
        outParam.Clear();
        eventCallback(appState, kEvent_PrepareAuthorizeInfo, inParam, outParam);
        err = outParam.PrepareAuthorizeInfo.Error;
        SuccessOrExit(err);
    }

    // Call the delegate to write the local node Weave operational certificate.
    err = opAuthDelegate->EncodeNodeCert(writer);
    SuccessOrExit(err);

    // Manufacturer attestation information.
    if (manufAttestRequired)
    {
        // Call the delegate to write the local node manufacturer attestation information.
        err = manufAttestDelegate->EncodeNodeCert(writer);
        SuccessOrExit(err);
    }

    tbsDataLen = writer.GetLengthWritten() - tbsDataStartOffset;

    // Calculate TBS hash.
    sha256.Begin();
    sha256.AddData(tbsDataStart, tbsDataLen);
    sha256.Finish(tbsHash);

    // Generate and encode operational device signature.
    err = opAuthDelegate->GenerateNodeSig(tbsHash, SHA256::kHashLength, writer);
    SuccessOrExit(err);

    // Generate and encode manufacturer attestation device signature.
    if (manufAttestRequired)
    {
        err = manufAttestDelegate->GenerateNodeSig(tbsHash, SHA256::kHashLength, writer);
        SuccessOrExit(err);
    }

    err = writer.EndContainer(containerType);
    SuccessOrExit(err);

    err = writer.Finalize();
    SuccessOrExit(err);

exit:
    return err;
}

/**
 * @brief
 *   Process GetCertificateResponse message.
 *
 * @details
 *   This function processes Weave GetCertificateResponse stracture encoded in the Weave TLV format.
 *
 *  @param[in]  msgBuf               A pointer to the PacketBuffer object holding the GetCertificateResponse message.
 *  @param[in]  eventCallback        A pointer to a function that will be called by this fanction to deliver API
 *                                   events to the application.
 *  @param[in]  appState             A pointer to an application-defined object which will be passed back
 *                                   to the application whenever an API event occurs.
 *
 *  @retval #WEAVE_NO_ERROR          If GetCertificateRequest was successfully generated.
 */
NL_DLL_EXPORT WEAVE_ERROR ProcessGetCertificateResponse(PacketBuffer * msgBuf, EventCallback eventCallback, void * appState)
{
    WEAVE_ERROR err;
    TLVReader reader;
    TLVWriter writer;
    TLVType outerContainer;
    uint16_t dataLen = msgBuf->DataLength();
    uint16_t availableDataLen = msgBuf->AvailableDataLength();
    uint8_t * dataStart = msgBuf->Start();
    uint8_t * dataMove = dataStart + availableDataLen;
    uint8_t * cert;
    uint16_t certLen;
    uint8_t * relatedCerts = NULL;
    uint16_t relatedCertsLen = 0;

    WeaveLogDetail(SecurityManager, "CertProvisioning:ProcessGetCertificateResponse");

    // Move message data to the end of message buffer.
    memmove(dataMove, dataStart, dataLen);

    reader.Init(dataMove, dataLen);

    writer.Init(dataStart, dataLen + availableDataLen);

    err = reader.Next(kTLVType_Structure, AnonymousTag);
    SuccessOrExit(err);

    err = reader.EnterContainer(outerContainer);
    SuccessOrExit(err);

    err = reader.Next(kTLVType_Structure, ContextTag(kTag_GetCertRespMsg_OpDeviceCert));
    SuccessOrExit(err);

    err = writer.CopyContainer(ProfileTag(kWeaveProfile_Security, kTag_WeaveCertificate), reader);
    SuccessOrExit(err);

    cert = dataStart;
    certLen = writer.GetLengthWritten();

    err = reader.Next(kTLVType_Array, ContextTag(kTag_GetCertRespMsg_RelatedCerts));

    // Optional Weave intermediate certificates list.
    if (err == WEAVE_NO_ERROR)
    {
        relatedCerts = cert + certLen;

        err = writer.CopyContainer(ProfileTag(kWeaveProfile_Security, kTag_WeaveCertificateList), reader);
        SuccessOrExit(err);

        relatedCertsLen = writer.GetLengthWritten() - certLen;

        err = reader.Next();
    }

    err = reader.VerifyEndOfContainer();
    SuccessOrExit(err);

    err = reader.ExitContainer(outerContainer);
    SuccessOrExit(err);

    err = reader.Next();
    VerifyOrExit(err == WEAVE_END_OF_TLV, err = WEAVE_ERROR_UNEXPECTED_TLV_ELEMENT);
    err = WEAVE_NO_ERROR;

    VerifyOrExit(reader.GetLengthRead() == dataLen, err = WEAVE_ERROR_INVALID_MESSAGE_LENGTH);

    // Deliver a ResponseReceived API event to the application.
    {
        InEventParam inParam;
        OutEventParam outParam;
        inParam.Clear();
        inParam.Source = NULL;
        inParam.ResponseReceived.Cert = cert;
        inParam.ResponseReceived.CertLen = certLen;
        inParam.ResponseReceived.RelatedCerts = relatedCerts;
        inParam.ResponseReceived.RelatedCertsLen = relatedCertsLen;
        outParam.Clear();
        eventCallback(appState, kEvent_ResponseReceived, inParam, outParam);
    }

exit:
    return err;
}

// ==================== Public Members ====================

/**
 * Initialize a WeaveCertProvClient object.
 *
 * Initialize a WeaveCertProvClient object in preparation for sending get certificate message.
 *
 *  @param[in]  binding              A Binding object that will be used to establish communication with the
 *                                   peer node.
 *  @param[in]  opAuthDelegate       A pointer to a operational authentication delegate object that will be used to
 *                                   construct and sign using node's operatational credentials.
 *  @param[in]  manufAttestDelegate  A pointer to a manufacturer attestation delegate object that will be used to
 *                                   construct and sign request using node's manufacturer provisioned credentials.
 *  @param[in]  eventCallback        A pointer to a function that will be called by the WeaveCertProvClient object
 *                                   to deliver API events to the application.
 *  @param[in]  appState             A pointer to an application-defined object which will be passed back
 *                                   to the application whenever an API event occurs.
 */
WEAVE_ERROR WeaveCertProvClient::Init(Binding * binding, WeaveCertProvAuthDelegate * opAuthDelegate, WeaveCertProvAuthDelegate * manufAttestDelegate, EventCallback eventCallback, void * appState)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(binding != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);
    VerifyOrExit(opAuthDelegate != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);
    VerifyOrExit(eventCallback != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    AppState = appState;
    mReqType = kReqType_NotSpecified;
    mBinding = binding;
    mOpAuthDelegate = opAuthDelegate;
    mManufAttestDelegate = manufAttestDelegate;
    mEventCallback = eventCallback;
    mEC = NULL;

    // Retain a reference to the binding object.
    binding->AddRef();

    mState = kState_Idle;

exit:
    return err;
}

/**
 * Shutdown a previously initialized WeaveCertProvClient object.
 *
 * Note that this method can only be called if the Init() method has been called previously.
 */
void WeaveCertProvClient::Shutdown(void)
{
    // Stop any request that is currently in progress.
    // Stop();

    // Release the reference to the binding.
    if (mBinding != NULL)
    {
        mBinding->Release();
        mBinding = NULL;
    }

    mOpAuthDelegate = NULL;
    mManufAttestDelegate = NULL;
    mEventCallback = NULL;
    mState = kState_NotInitialized;
}

/**
 * Start Certificate Provisioning Protocol.
 *
 * This method initiates the process of sending a GetCertificateRequest message to the
 * CA service. If and when a corresponding GetCertificateResponse message is received it will be
 * delivered to the application via the ResponseReceived API event.
 *
 * When forming the GetCertificateRequest message, the WeaveCertProvClient makes a request
 * to the application, via WeaveCertProvAuthDelegate functions and the PrepareAuthorizeInfo
 * API event, to prepare the payload of the message.
 *
 * If the Binding object is not in the Ready state when this method is called, a request
 * will be made to Binding::RequestPrepare() method to initiate on-demand preparation.
 * The request operation will then be waiting until this process completes. Any call to
 * StartCertificateProvisioning() while there is a previous request in process will be ignored.
 */
WEAVE_ERROR WeaveCertProvClient::StartCertificateProvisioning(uint8_t reqType)
{
    WEAVE_ERROR err;

    VerifyOrExit(mState == kState_Idle, err = WEAVE_ERROR_INCORRECT_STATE);

    mReqType = reqType;

    err = SendGetCertificateRequest();
    SuccessOrExit(err);

exit:
    return err;
}


// ==================== Private Members ====================

/**
 * Performs the work of initiating Binding preparation and sending a GetCertificateRequest message.
 */
WEAVE_ERROR WeaveCertProvClient::SendGetCertificateRequest(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    PacketBuffer * msgBuf = NULL;
    bool reqSent = false;

    // Set the protocol callback on the binding object.  NOTE: This needs to happen after the
    // app has explicitly started the sending process by calling StartCertificateProvisioning().
    // Otherwise the client object might receive callbacks from the binding before its ready.
    // Even though it happens redundantly, we do it here to ensure it happens at the right point.
    mBinding->SetProtocolLayerCallback(HandleBindingEvent, this);

    // If the binding is ready ...
    if (mBinding->IsReady())
    {
        // Allocate and initialize a new exchange context.
        err = mBinding->NewExchangeContext(mEC);
        SuccessOrExit(err);
        mEC->AppState = this;
        mEC->OnMessageReceived = HandleResponse;
        mEC->OnResponseTimeout = HandleResponseTimeout;
        mEC->OnKeyError = HandleKeyError;
        mEC->OnConnectionClosed = HandleConnectionClosed;
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
        mEC->OnAckRcvd = HandleAckRcvd;
        mEC->OnSendError = HandleSendError;
#endif // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING

        // Allocate buffer.
        msgBuf = PacketBuffer::New();
        VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

        // Generate a GetCertificateRequest message.
        err = GenerateGetCertificateRequest(msgBuf, mReqType, mOpAuthDelegate, mManufAttestDelegate, mEventCallback, AppState);
        SuccessOrExit(err);

        // Enter the RequestInProgress state.
        mState = kState_RequestInProgress;

        // Send a GetCertificateRequest message to the peer.
        err = mEC->SendMessage(kWeaveProfile_Security, kMsgType_GetCertificateRequest, msgBuf, ExchangeContext::kSendFlag_ExpectResponse);
        msgBuf = NULL;
        SuccessOrExit(err);

        reqSent = true;
    }

    // Otherwise, if the binding needs to be prepared...
    else
    {
        // Enter the PreparingBinding state. Once binding preparation is complete, the binding will call back to
        // the WeaveCertProvClient, which will result in SendGetCertificateRequest() being called again.
        mState = kState_PreparingBinding;

        // If the binding is in a state where it can be prepared, ask the application to prepare it by delivering a
        // PrepareRequested API event via the binding's callback.  At some point the binding will call back to
        // WeaveCertProvClient::HandleBindingEvent() signaling that preparation has completed (successfully or otherwise).
        // Note that this callback *may* happen synchronously within the RequestPrepare() method, in which case this
        // method (DoSend) will recurse.
        if (mBinding->CanBePrepared())
        {
            err = mBinding->RequestPrepare();
            SuccessOrExit(err);
        }

        // Fail if the binding is in some state, other than Ready, that doesn't allow it to be prepared.
        else
        {
            VerifyOrExit(mBinding->IsPreparing(), err = WEAVE_ERROR_INCORRECT_STATE);
        }
    }

exit:

    // If something failed, enter Idle state.
    if (err != WEAVE_NO_ERROR)
    {
        mState = kState_Idle;
    }

    // Free the payload buffer if it hasn't been sent.
    PacketBuffer::Free(msgBuf);

    // If needed, deliver a CommuncationError API event to the application. Note that this may result
    // in the state of the client object changing. So the code here shouldn't presume anything about
    // the current state after the callback.
    if (err != WEAVE_NO_ERROR)
    {
        DeliverCommunicationError(err);
    }

    // Otherwise, if a request was sent, deliver a RequestSent API event to the application. As above,
    // this code shouldn't presume anything about the current state after the callback.
    else if (reqSent)
    {
        InEventParam inParam;
        OutEventParam outParam;
        inParam.Clear();
        inParam.Source = this;
        outParam.Clear();
        mEventCallback(AppState, kEvent_RequestSent, inParam, outParam);
    }

    return err;
}

/**
 * Called when a GetCertificate exchange completes.
 */
void WeaveCertProvClient::HandleRequestDone(void)
{
    // Clear the request state.
    ClearRequestState();

    // Set Idle state.
    mState = kState_Idle;
}

/**
 * Aborts any in-progress exchange and resets the receive state.
 */
void WeaveCertProvClient::ClearRequestState(void)
{
    mReqType = kReqType_NotSpecified;

    // Abort any in-progress exchange and release the exchange context.
    if (mEC != NULL)
    {
        mEC->Abort();
        mEC = NULL;
    }
}

/**
 * Deliver a CommuncationError API event to the application.
 */
void WeaveCertProvClient::DeliverCommunicationError(WEAVE_ERROR err)
{
    InEventParam inParam;
    OutEventParam outParam;

    inParam.Clear();
    inParam.Source = this;
    inParam.CommuncationError.Reason = err;

    outParam.Clear();

    mEventCallback(AppState, kEvent_CommuncationError, inParam, outParam);
}

/**
 * Process an event delivered from the binding associated with a WeaveCertProvClient.
 */
void WeaveCertProvClient::HandleBindingEvent(void * const appState, const Binding::EventType eventType, const Binding::InEventParam & inParam,
                                             Binding::OutEventParam & outParam)
{
    WeaveCertProvClient * client = (WeaveCertProvClient *)appState;

    switch (eventType)
    {
    case Binding::kEvent_BindingReady:

        // When the binding is ready, if the client is still in the PreparingBinding state,
        // initiate sending of a GetCertificateRequest.
        if (client->mState == kState_PreparingBinding)
        {
            client->SendGetCertificateRequest();
        }

        break;

    case Binding::kEvent_PrepareFailed:

        // If binding preparation fails and the client is still in the PreparingBinding state...
        if (client->mState == kState_PreparingBinding)
        {
            // Clean-up the request state and enter Idle state.
            client->HandleRequestDone();

            // Celiver a CommuncationError API event to the application.
            client->DeliverCommunicationError(inParam.PrepareFailed.Reason);
        }

        break;

    default:
        Binding::DefaultEventHandler(appState, eventType, inParam, outParam);
    }
}

/**
 * Called by the Weave messaging layer when a response is received on an GetCertificate exchange.
 */
void WeaveCertProvClient::HandleResponse(ExchangeContext * ec, const IPPacketInfo * pktInfo, const WeaveMessageInfo * msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer * payload)
{
    WeaveCertProvClient * client = (WeaveCertProvClient *)ec->AppState;

    VerifyOrDie(client->mState == kState_RequestInProgress);
    VerifyOrDie(client->mEC == ec);

    // Ignore any messages other than GetCertificateResponse.
    if (profileId != kWeaveProfile_Security || msgType != kMsgType_GetCertificateResponse)
    {
        PacketBuffer::Free(payload);
        return;
    }

    // If the request is now complete, clear the request state and enter Idle state.
    client->HandleRequestDone();

    // Process message and deliver result to the application using API event callback function.
    ProcessGetCertificateResponse(payload, client->mEventCallback, client->AppState);
}

/**
 * Called by the Weave messaging layer when a timeout occurs while waiting for an GetCertificateResponse message.
 */
void WeaveCertProvClient::HandleResponseTimeout(ExchangeContext * ec)
{
    WeaveCertProvClient * client = (WeaveCertProvClient *)ec->AppState;

    VerifyOrDie(client->mState == kState_RequestInProgress);
    VerifyOrDie(client->mEC == ec);

    // Clear the request state and enter Idle.
    client->HandleRequestDone();

    // Deliver a ResponseTimeout API event to the application.
    {
        InEventParam inParam;
        OutEventParam outParam;
        inParam.Clear();
        inParam.Source = client;
        outParam.Clear();
        client->mEventCallback(client->AppState, kEvent_ResponseTimeout, inParam, outParam);
    }
}

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING

/**
 * Called by the Weave messaging layer when a WRM ACK is received for a GetCertificateRequest message.
 */
void WeaveCertProvClient::HandleAckRcvd(ExchangeContext * ec, void * msgCtxt)
{
    WeaveCertProvClient * client = (WeaveCertProvClient *)ec->AppState;

    VerifyOrDie(client->mState == kState_RequestInProgress);
    VerifyOrDie(client->mEC == ec);
}

#endif // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING

/**
 * Called by the Weave messaging layer when an error occurs while sending a GetCertificateRequest message.
 */
void WeaveCertProvClient::HandleSendError(ExchangeContext * ec, WEAVE_ERROR sendErr, void * msgCtxt)
{
    WeaveCertProvClient * client = (WeaveCertProvClient *)ec->AppState;

    VerifyOrDie(client->mState == kState_RequestInProgress);
    VerifyOrDie(client->mEC == ec);

    // Clear the request state and enter Idle.
    client->HandleRequestDone();

    // Deliver a CommunicationError API event to the application.
    client->DeliverCommunicationError(sendErr);
}

/**
 * Called by the Weave messaging layer when the peer response to a GetCertificateRequest message with a key error.
 */
void WeaveCertProvClient::HandleKeyError(ExchangeContext * ec, WEAVE_ERROR keyErr)
{
    // Treat this the same as if a send error had occurred.
    HandleSendError(ec, keyErr, NULL);
}

/**
 * Called by the Weave messaging layer if an underlying Weave connection closes while waiting for an GetCertificateResponse message.
 */
void WeaveCertProvClient::HandleConnectionClosed(ExchangeContext *ec, WeaveConnection *con, WEAVE_ERROR conErr)
{
    // If the other side simply closed the connection without responding, deliver a "closed unexpectedly"
    // error to the application.
    if (conErr == WEAVE_NO_ERROR)
    {
        conErr = WEAVE_ERROR_CONNECTION_CLOSED_UNEXPECTEDLY;
    }

    // Treat this the same as if a send error had occurred.
    HandleSendError(ec, conErr, NULL);
}


// ==================== Documentation for Inline Public Members ====================

/**
 * @fn State WeaveCertProvClient::GetState(void) const
 *
 * Retrieve the current state of the WeaveCertProvClient object.
 */

/**
 * @fn bool WeaveCertProvClient::RequestInProgress() const
 *
 * Returns true if a GetCertificateRequest has been sent and the WeaveCertProvClient object is awaiting a response.
 */

/**
 * @fn Binding * WeaveCertProvClient::GetBinding(void) const
 *
 * Returns a pointer to the Binding object associated with the WeaveCertProvClient.
 */

/**
 * @fn WeaveCertProvAuthDelegate * WeaveCertProvClient::GetOpAuthDelegate(void) const
 *
 * Returns a pointer to the operational authentication delegate object currently configured on the WeaveCertProvClient object.
 */

/**
 * @fn void WeaveCertProvClient::SetOpAuthDelegate(WeaveCertProvAuthDelegate * opAuthDelegate)
 *
 * Sets the operational authentication delegate object on the WeaveCertProvClient object.
 */

/**
 * @fn WeaveCertProvAuthDelegate * WeaveCertProvClient::GetManufAttestDelegate(void) const
 *
 * Returns a pointer to the manufacturer attestation delegate object currently configured on the WeaveCertProvClient object.
 */

/**
 * @fn void WeaveCertProvClient::SetManufAttestDelegate(WeaveCertProvAuthDelegate * manufAttestDelegate)
 *
 * Sets the manufacturer attestation delegate object on the WeaveCertProvClient object.
 */

/**
 * @fn EventCallback WeaveCertProvClient::GetEventCallback(void) const
 *
 * Returns a pointer to the API event callback function currently configured on the WeaveCertProvClient object.
 */

/**
 * @fn void WeaveCertProvClient::SetEventCallback(EventCallback eventCallback)
 *
 * Sets the API event callback function on the WeaveCertProvClient object.
 */


} // namespace CertProvisioning
} // namespace Security
} // namespace Profiles
} // namespace Weave
} // namespace nl
