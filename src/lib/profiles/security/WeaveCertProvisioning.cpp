/*
 *
 *    Copyright (c) 2019-2020 Google LLC.
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

// ==================== Public Members ====================

/**
 * Initialize a WeaveCertProvEngine object in preparation for sending get certificate request message.
 *
 *  @param[in]  binding              A Binding object that will be used to establish communication with the
 *                                   peer node.
 *  @param[in]  opAuthDelegate       A pointer to a operational authentication delegate object that will be used to
 *                                   construct and sign using node's operatational credentials.
 *  @param[in]  mfrAttestDelegate    A pointer to a manufacturer attestation delegate object that will be used to
 *                                   construct and sign request using node's manufacturer provisioned credentials.
 *  @param[in]  eventCallback        A pointer to a function that will be called by the WeaveCertProvEngine object
 *                                   to deliver API events to the application.
 *  @param[in]  appState             A pointer to an application-defined object which will be passed back
 *                                   to the application whenever an API event occurs.
 */
WEAVE_ERROR WeaveCertProvEngine::Init(Binding * binding, WeaveNodeOpAuthDelegate * opAuthDelegate, WeaveNodeMfrAttestDelegate * mfrAttestDelegate, EventCallback eventCallback, void * appState)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(opAuthDelegate != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    VerifyOrExit(eventCallback != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    AppState = appState;
    mReqType = kReqType_NotSpecified;
    mDoMfrAttest = false;
    mBinding = binding;
    mOpAuthDelegate = opAuthDelegate;
    mMfrAttestDelegate = mfrAttestDelegate;
    mEventCallback = eventCallback;
    mEC = NULL;

    // Retain a reference to the binding object.
    if (binding != NULL)
    {
        binding->AddRef();
    }

    mState = kState_Idle;

exit:
    return err;
}

/**
 * Shutdown a previously initialized WeaveCertProvEngine object.
 *
 * Note that this method can only be called if the Init() method has been called previously.
 */
void WeaveCertProvEngine::Shutdown(void)
{
    // Stop request if currently in progress.
    HandleRequestDone();

    // Release the reference to the binding.
    if (mBinding != NULL)
    {
        mBinding->Release();
        mBinding = NULL;
    }

    mOpAuthDelegate = NULL;
    mMfrAttestDelegate = NULL;
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
 * If the Binding object is not in the Ready state when this method is called, a request
 * will be made to Binding::RequestPrepare() method to initiate on-demand preparation.
 * The request operation will then be waiting until this process completes. Any call to
 * StartCertificateProvisioning() while there is a previous request in process will be ignored.
 *
 *  @param[in]  reqType              Get certificate request type.
 *  @param[in]  doMfrAttest          A boolean flag that indicates whether protocol should
 *                                   include manufacturer attestation data.
 *
 *  @retval #WEAVE_NO_ERROR          If StartCertificateProvisioning() was processed successfully.
 */
WEAVE_ERROR WeaveCertProvEngine::StartCertificateProvisioning(uint8_t reqType, bool doMfrAttest)
{
    WEAVE_ERROR err;

    VerifyOrExit(mState == kState_Idle, err = WEAVE_ERROR_INCORRECT_STATE);

    VerifyOrExit(mBinding != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    mReqType = reqType;
    mDoMfrAttest = doMfrAttest;

    err = SendGetCertificateRequest();
    SuccessOrExit(err);

exit:
    return err;
}

/**
 * Generate GetCertificateRequest message.
 *
 * This method generates Weave GetCertificateRequest structure encoded in the Weave TLV format.
 *
 * When forming the GetCertificateRequest message, the method makes a request to the application,
 * via WeaveNodeOpAuthDelegate and WeaveNodeMfrAttestDelegate functions and
 * the PrepareAuthorizeInfo API event, to prepare the payload of the message.
 *
 *  @param[in]  msgBuf               A pointer to the PacketBuffer object holding the GetCertificateRequest message.
 *  @param[in]  reqType              Get certificate request type.
 *  @param[in]  doMfrAttest          A boolean flag that indicates whether request should
 *                                   include manufacturer attestation data.
 *
 *  @retval #WEAVE_NO_ERROR          If GetCertificateRequest was successfully generated.
 */
WEAVE_ERROR WeaveCertProvEngine::GenerateGetCertificateRequest(PacketBuffer * msgBuf, uint8_t reqType, bool doMfrAttest)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TLVWriter writer;
    TLVType containerType;
    SHA256 sha256;
    uint8_t *tbsDataStart;
    uint16_t tbsDataStartOffset;
    uint16_t tbsDataLen;
    uint8_t tbsHash[SHA256::kHashLength];

    WeaveLogDetail(SecurityManager, "WeaveCertProvEngine:GenerateGetCertificateRequest");

    VerifyOrExit(mState == kState_Idle, err = WEAVE_ERROR_INCORRECT_STATE);

    VerifyOrExit(reqType == kReqType_GetInitialOpDeviceCert ||
                 reqType == kReqType_RotateOpDeviceCert, err = WEAVE_ERROR_INVALID_ARGUMENT);

    reqType = kReqType_NotSpecified; // TODO: Remove, this is temporary for testing
    mReqType = reqType;
    mDoMfrAttest = doMfrAttest;

    if (doMfrAttest)
    {
        VerifyOrExit(mMfrAttestDelegate != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);
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
    {
        InEventParam inParam;
        OutEventParam outParam;
        inParam.Clear();
        inParam.Source = this;
        inParam.PrepareAuthorizeInfo.Writer = &writer;
        outParam.Clear();
        mEventCallback(AppState, kEvent_PrepareAuthorizeInfo, inParam, outParam);
        err = outParam.PrepareAuthorizeInfo.Error;
        SuccessOrExit(err);
    }

    // Call the delegate to write the local node Weave operational certificate.
    err = mOpAuthDelegate->EncodeOpCert(writer, ContextTag(kTag_GetCertReqMsg_OpDeviceCert));
    SuccessOrExit(err);

    // Call the delegate to write the local node Weave operational related certificate.
    err = mOpAuthDelegate->EncodeOpRelatedCerts(writer, ContextTag(kTag_GetCertReqMsg_OpRelatedCerts));
    SuccessOrExit(err);

    // Call the delegate to write the local node manufacturer attestation information.
    if (doMfrAttest)
    {
        err = mMfrAttestDelegate->EncodeMAInfo(writer);
        SuccessOrExit(err);
    }

    tbsDataLen = writer.GetLengthWritten() - tbsDataStartOffset;

    // Calculate TBS hash.
    sha256.Begin();
    sha256.AddData(tbsDataStart, tbsDataLen);
    sha256.Finish(tbsHash);

    // Operational signature algorithm.
    err = writer.Put(ContextTag(kTag_GetCertReqMsg_OpDeviceSigAlgo), static_cast<uint16_t>(ASN1::kOID_SigAlgo_ECDSAWithSHA256));
    SuccessOrExit(err);

    // Generate and encode operational device signature.
    err = mOpAuthDelegate->GenerateAndEncodeOpSig(tbsHash, SHA256::kHashLength, writer, ContextTag(kTag_GetCertReqMsg_OpDeviceSig_ECDSA));
    SuccessOrExit(err);

    // Generate and encode manufacturer attestation device signature.
    if (doMfrAttest)
    {
        err = mMfrAttestDelegate->GenerateAndEncodeMASig(tbsDataStart, tbsDataLen, writer);
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
 * Process GetCertificateResponse message.
 *
 * This method processes Weave GetCertificateResponse structure encoded in the Weave TLV format.
 *
 * When processing of the GetCertificateResponse message is complete successfully, the method makes a call
 * to the application, via the ResponseReceived API event, to deliver the result.
 *
 * If processing of the GetCertificateResponse message fails, the method makes a call to the application,
 * via the CommunicationError API event, to report the error.
 *
 *  @param[in]  msgBuf               A pointer to the PacketBuffer object holding the GetCertificateResponse message.
 *
 *  @retval #WEAVE_NO_ERROR          If GetCertificateResponse message was successfully processed.
 */
WEAVE_ERROR WeaveCertProvEngine::ProcessGetCertificateResponse(PacketBuffer * msgBuf)
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

    WeaveLogDetail(SecurityManager, "WeaveCertProvEngine:ProcessGetCertificateResponse");

    // Move message data to the end of the message buffer.
    // This is done so the data can be manipulated in place without need to allocate additional
    // memory buffer. Specifically, this function changes context-specific tags to Security-profile
    // tags for device certificate structure and related certificates array.
    memmove(dataMove, dataStart, dataLen);

    // Initialize a TLV reader to read the data that was moved to the end of the buffer.
    reader.Init(dataMove, dataLen);

    // Initialize a TLV writer to the beginning of the buffer.
    writer.Init(dataStart, dataLen + availableDataLen);

    err = reader.Next(kTLVType_Structure, AnonymousTag);
    SuccessOrExit(err);

    err = reader.EnterContainer(outerContainer);
    SuccessOrExit(err);

    err = reader.Next(kTLVType_Structure, ContextTag(kTag_GetCertRespMsg_OpDeviceCert));
    SuccessOrExit(err);

    // Copy device certificate with new tag.
    err = writer.CopyContainer(ProfileTag(kWeaveProfile_Security, kTag_WeaveCertificate), reader);
    SuccessOrExit(err);

    cert = dataStart;
    certLen = writer.GetLengthWritten();

    err = reader.Next(kTLVType_Array, ContextTag(kTag_GetCertRespMsg_OpRelatedCerts));

    // Optional Weave intermediate certificates list.
    if (err == WEAVE_NO_ERROR)
    {
        relatedCerts = cert + certLen;

        // Copy device related certificates with new tag.
        err = writer.CopyContainer(ProfileTag(kWeaveProfile_Security, kTag_WeaveCertificateList), reader);
        SuccessOrExit(err);

        relatedCertsLen = writer.GetLengthWritten() - certLen;

        err = reader.Next();
        VerifyOrExit(err == WEAVE_END_OF_TLV, err = WEAVE_ERROR_UNEXPECTED_TLV_ELEMENT);
    }

    err = reader.VerifyEndOfContainer();
    SuccessOrExit(err);

    err = reader.ExitContainer(outerContainer);
    SuccessOrExit(err);

    err = reader.Next();
    VerifyOrExit(err == WEAVE_END_OF_TLV, err = WEAVE_ERROR_UNEXPECTED_TLV_ELEMENT);
    err = WEAVE_NO_ERROR;

    VerifyOrExit(reader.GetLengthRead() == dataLen, err = WEAVE_ERROR_INVALID_MESSAGE_LENGTH);

exit:
    {
        if (err != WEAVE_NO_ERROR)
        {
            // Deliver a CommunicationError API event to the application.
            DeliverCommunicationError(err);
        }
        else
        {
            InEventParam inParam;
            OutEventParam outParam;
            inParam.Clear();
            outParam.Clear();
            inParam.Source = this;
            inParam.ResponseReceived.ReplaceCert = true;
            inParam.ResponseReceived.Cert = cert;
            inParam.ResponseReceived.CertLen = certLen;
            inParam.ResponseReceived.RelatedCerts = relatedCerts;
            inParam.ResponseReceived.RelatedCertsLen = relatedCertsLen;

            // Deliver a ResponseReceived API event to the application.
            mEventCallback(AppState, kEvent_ResponseReceived, inParam, outParam);
            err = outParam.ResponseReceived.Error;
        }
    }

    return err;
}

// ==================== Private Members ====================

/**
 * Performs the work of initiating Binding preparation and sending a GetCertificateRequest message.
 */
WEAVE_ERROR WeaveCertProvEngine::SendGetCertificateRequest(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    PacketBuffer * msgBuf = NULL;

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
        err = GenerateGetCertificateRequest(msgBuf, mReqType, mDoMfrAttest);
        SuccessOrExit(err);

        // Enter the RequestInProgress state.
        mState = kState_RequestInProgress;

        // Send a GetCertificateRequest message to the peer.
        err = mEC->SendMessage(kWeaveProfile_Security, kMsgType_GetCertificateRequest, msgBuf, ExchangeContext::kSendFlag_ExpectResponse);
        msgBuf = NULL;
        SuccessOrExit(err);
    }

    // Otherwise, if the binding needs to be prepared...
    else
    {
        // Enter the PreparingBinding state. Once binding preparation is complete, the binding will call back to
        // the WeaveCertProvEngine, which will result in SendGetCertificateRequest() being called again.
        mState = kState_PreparingBinding;

        // If the binding is in a state where it can be prepared, ask the application to prepare it by delivering a
        // PrepareRequested API event via the binding's callback.  At some point the binding will call back to
        // WeaveCertProvEngine::HandleBindingEvent() signaling that preparation has completed (successfully or otherwise).
        // Note that this callback *may* happen synchronously within the RequestPrepare() method, in which case this
        // method (SendGetCertificateRequest()) will recurse.
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
    // Free the payload buffer if it hasn't been sent.
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);

    if (err != WEAVE_NO_ERROR)
    {
        // Clear the request state and enter Idle.
        HandleRequestDone();

        // Deliver a CommunicationError API event to the application. Note that this may result in the state
        // of the client object changing (e.g. as a result of the application calling AbortCertificateProvisioning()).
        // So the code here shouldn't presume anything about the current state after the callback.
        DeliverCommunicationError(err);
    }

    return err;
}

/**
 * Called when a GetCertificate exchange completes.
 */
void WeaveCertProvEngine::HandleRequestDone(void)
{
    // Clear the request state.
    mReqType = kReqType_NotSpecified;
    mDoMfrAttest = false;

    // Abort any in-progress exchange and release the exchange context.
    if (mEC != NULL)
    {
        mEC->Abort();
        mEC = NULL;
    }

    // Set Idle state.
    mState = kState_Idle;
}

/**
 * Deliver a CommunicationError API event to the application.
 */
void WeaveCertProvEngine::DeliverCommunicationError(WEAVE_ERROR err)
{
    InEventParam inParam;
    OutEventParam outParam;

    inParam.Clear();
    outParam.Clear();
    inParam.Source = this;
    inParam.CommunicationError.Reason = err;

    mEventCallback(AppState, kEvent_CommunicationError, inParam, outParam);
}

/**
 * Process an event delivered from the binding associated with a WeaveCertProvEngine.
 */
void WeaveCertProvEngine::HandleBindingEvent(void * const appState, const Binding::EventType eventType, const Binding::InEventParam & inParam,
                                             Binding::OutEventParam & outParam)
{
    WeaveCertProvEngine * client = (WeaveCertProvEngine *)appState;

    switch (eventType)
    {
    case Binding::kEvent_BindingReady:

        // When the binding is ready, if the client is still in the PreparingBinding state,
        // initiate sending of a GetCertificateRequest.
        if (client->mState == kState_PreparingBinding)
        {
            client->mState = kState_Idle;

            client->SendGetCertificateRequest();
        }

        break;

    case Binding::kEvent_PrepareFailed:

        // If binding preparation fails and the client is still in the PreparingBinding state...
        if (client->mState == kState_PreparingBinding)
        {
            // Clean-up the request state and enter Idle state.
            client->HandleRequestDone();

            // Deliver a CommunicationError API event to the application.
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
void WeaveCertProvEngine::HandleResponse(ExchangeContext * ec, const IPPacketInfo * pktInfo, const WeaveMessageInfo * msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer * msgBuf)
{
    WeaveCertProvEngine * client = (WeaveCertProvEngine *)ec->AppState;

    VerifyOrDie(client->mState == kState_RequestInProgress);
    VerifyOrDie(client->mEC == ec);

    // Process status report message from the CA service.
    if (profileId == kWeaveProfile_Common && msgType == Common::kMsgType_StatusReport)
    {
        StatusReport statusReport;
        WEAVE_ERROR parseErr = StatusReport::parse(msgBuf, statusReport);

        // Free the received message buffer so that it can be reused to initiate additional communication.
        PacketBuffer::Free(msgBuf);
        msgBuf = NULL;

        SuccessOrExit(parseErr);

        VerifyOrExit(statusReport.mProfileId == kWeaveProfile_Security, /* no-op */);

        InEventParam inParam;
        OutEventParam outParam;
        inParam.Clear();
        outParam.Clear();
        inParam.Source = client;

        if (statusReport.mStatusCode == Security::kStatusCode_NoNewOperationalCertRequired)
        {
            inParam.ResponseReceived.ReplaceCert = false;

            // Deliver a ResponseReceived API event to the application.
            client->mEventCallback(client->AppState, kEvent_ResponseReceived, inParam, outParam);
        }
        else
        {
            inParam.CommunicationError.Reason = WEAVE_ERROR_STATUS_REPORT_RECEIVED;
            inParam.CommunicationError.RcvdStatusReport = &statusReport;

            // Deliver a CommunicationError API event to the application.
            client->mEventCallback(client->AppState, kEvent_CommunicationError, inParam, outParam);
        }
    }

    // Process GetCertificateResponse message from the CA service.
    else if (profileId == kWeaveProfile_Security && msgType == kMsgType_GetCertificateResponse)
    {
        // Close current exchange, allowing it to send ack message.
        client->mEC->Close();
        client->mEC = NULL;

        // Clean-up the request state and enter Idle state.
        client->HandleRequestDone();

        // Process message and deliver result to the application using API event callback function.
        // Note that, in this context, any error returned by ProcessGetCertificateResponse()
        // has already been seen by the application. Thus we can safely ignore it here.
        client->ProcessGetCertificateResponse(msgBuf);
    }

    // Ignore any other messages.

exit:
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);

    return;
}

/**
 * Called by the Weave messaging layer when a timeout occurs while waiting for an GetCertificateResponse message.
 */
void WeaveCertProvEngine::HandleResponseTimeout(ExchangeContext * ec)
{
    WeaveCertProvEngine * client = (WeaveCertProvEngine *)ec->AppState;

    VerifyOrDie(client->mState == kState_RequestInProgress);
    VerifyOrDie(client->mEC == ec);

    // Clear the request state and enter Idle.
    client->HandleRequestDone();

    // Deliver a CommunicationError API event to the application.
    client->DeliverCommunicationError(WEAVE_ERROR_TIMEOUT);
}

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING

/**
 * Called by the Weave messaging layer when a WRM ACK is received for a GetCertificateRequest message.
 */
void WeaveCertProvEngine::HandleAckRcvd(ExchangeContext * ec, void * msgCtxt)
{
    WeaveCertProvEngine * client = (WeaveCertProvEngine *)ec->AppState;

    VerifyOrDie(client->mState == kState_RequestInProgress);
    VerifyOrDie(client->mEC == ec);
}

#endif // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING

/**
 * Called by the Weave messaging layer when an error occurs while sending a GetCertificateRequest message.
 */
void WeaveCertProvEngine::HandleSendError(ExchangeContext * ec, WEAVE_ERROR sendErr, void * msgCtxt)
{
    WeaveCertProvEngine * client = (WeaveCertProvEngine *)ec->AppState;

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
void WeaveCertProvEngine::HandleKeyError(ExchangeContext * ec, WEAVE_ERROR keyErr)
{
    // Treat this the same as if a send error had occurred.
    HandleSendError(ec, keyErr, NULL);
}

/**
 * Called by the Weave messaging layer if an underlying Weave connection closes while waiting for an GetCertificateResponse message.
 */
void WeaveCertProvEngine::HandleConnectionClosed(ExchangeContext *ec, WeaveConnection *con, WEAVE_ERROR conErr)
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
 * @fn void WeaveCertProvEngine::AbortCertificateProvisioning(void)
 *
 * Stops any GetCertificate exchange in progress.
 */

/**
 * @fn State WeaveCertProvEngine::GetState(void) const
 *
 * Retrieve the current state of the WeaveCertProvEngine object.
 */

/**
 * @fn uint8_t WeaveCertProvEngine::GetReqType(void) const
 *
 * Retrieve the current request type.
 */

/**
 * @fn Binding * WeaveCertProvEngine::GetBinding(void) const
 *
 * Returns a pointer to the Binding object associated with the WeaveCertProvEngine.
 */

/**
 * @fn void WeaveCertProvEngine::SetBinding(Binding * binding)
 *
 * Sets the binding object on the WeaveCertProvEngine object.
 */

/**
 * @fn WeaveNodeOpAuthDelegate * WeaveCertProvEngine::GetOpAuthDelegate(void) const
 *
 * Returns a pointer to the operational authentication delegate object currently configured on the WeaveCertProvEngine object.
 */

/**
 * @fn void WeaveCertProvEngine::SetOpAuthDelegate(WeaveNodeOpAuthDelegate * opAuthDelegate)
 *
 * Sets the operational authentication delegate object on the WeaveCertProvEngine object.
 */

/**
 * @fn WeaveNodeMfrAttestDelegate * WeaveCertProvEngine::GetMfrAttestDelegate(void) const
 *
 * Returns a pointer to the manufacturer attestation delegate object currently configured on the WeaveCertProvEngine object.
 */

/**
 * @fn void WeaveCertProvEngine::SetMfrAttestDelegate(WeaveNodeMfrAttestDelegate * mfrAttestDelegate)
 *
 * Sets the manufacturer attestation delegate object on the WeaveCertProvEngine object.
 */

/**
 * @fn WeaveCertProvEngine::EventCallback WeaveCertProvEngine::GetEventCallback(void) const
 *
 * Returns a pointer to the API event callback function currently configured on the WeaveCertProvEngine object.
 */

/**
 * @fn void WeaveCertProvEngine::SetEventCallback(WeaveCertProvEngine::EventCallback eventCallback)
 *
 * Sets the API event callback function on the WeaveCertProvEngine object.
 */


} // namespace CertProvisioning
} // namespace Security
} // namespace Profiles
} // namespace Weave
} // namespace nl
