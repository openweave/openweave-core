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
 *      This file implements types and objects for managing Weave session
 *      security state.
 *
 */

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif // __STDC_FORMAT_MACROS

#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveServerBase.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include "WeaveSecurityMgr.h"
#include <Weave/Support/CodeUtils.h>
#include <Weave/Profiles/common/CommonProfile.h>
#include <Weave/Profiles/common/WeaveMessage.h>
#include <Weave/Profiles/echo/WeaveEcho.h>
#include <Weave/Support/logging/WeaveLogging.h>
#include <Weave/Profiles/security/WeaveSecurity.h>
#include <Weave/Profiles/status-report/StatusReportProfile.h>
#include <Weave/Profiles/service-directory/ServiceDirectory.h>
#include <Weave/Support/crypto/WeaveCrypto.h>
#include <Weave/Support/WeaveFaultInjection.h>

namespace nl {
namespace Weave {

#if WEAVE_CONFIG_SECURITY_MGR_TIME_ALERTS_DUMMY

namespace Platform {
namespace Security {

static inline void OnTimeConsumingCryptoStart()
{
}

static inline void OnTimeConsumingCryptoDone()
{
}

} // namespace Platform
} // namespace Security

#endif // WEAVE_CONFIG_SECURITY_MGR_TIME_ALERTS_DUMMY

using namespace nl::Weave::Profiles;
using namespace nl::Weave::Profiles::Common;
using namespace nl::Weave::Profiles::StatusReporting;
using namespace nl::Weave::Profiles::Security;
using namespace nl::Weave::Profiles::Security::PASE;
using namespace nl::Weave::Profiles::Security::AppKeys;
using namespace nl::Weave::Encoding;
using namespace nl::Weave::Crypto;

WeaveSecurityManager::WeaveSecurityManager(void)
{
    State = kState_NotInitialized;
    mSystemLayer = NULL;
}

WEAVE_ERROR WeaveSecurityManager::Init(WeaveExchangeManager& aExchangeMgr, System::Layer& aSystemLayer)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (State != kState_NotInitialized)
        return WEAVE_ERROR_INCORRECT_STATE;

    ExchangeManager = &aExchangeMgr;
    mSystemLayer = &aSystemLayer;
    SessionEstablishTimeout = WEAVE_CONFIG_DEFAULT_SECURITY_SESSION_ESTABLISHMENT_TIMEOUT;
    IdleSessionTimeout = WEAVE_CONFIG_DEFAULT_SECURITY_SESSION_IDLE_TIMEOUT;
    FabricState = aExchangeMgr.FabricState;
    OnSessionEstablished = NULL;
    OnSessionError = NULL;
    OnKeyErrorMsgRcvd = NULL;
    mEC = NULL;
    mCon = NULL;
#if WEAVE_CONFIG_ENABLE_PASE_INITIATOR || WEAVE_CONFIG_ENABLE_PASE_RESPONDER
    mPASEEngine = NULL;
#endif
#if WEAVE_CONFIG_ENABLE_CASE_INITIATOR || WEAVE_CONFIG_ENABLE_CASE_RESPONDER
    mCASEEngine = NULL;
    mDefaultAuthDelegate = NULL;
#endif
#if WEAVE_CONFIG_ENABLE_CASE_INITIATOR
    InitiatorCASEConfig = CASE::kCASEConfig_Config2;
    InitiatorCASECurveId = WEAVE_CONFIG_DEFAULT_CASE_CURVE_ID;
    InitiatorAllowedCASEConfigs = CASE::kCASEAllowedConfig_Config2|CASE::kCASEAllowedConfig_Config1;
    InitiatorAllowedCASECurves = WEAVE_CONFIG_DEFAULT_CASE_ALLOWED_CURVES;
#endif
#if WEAVE_CONFIG_ENABLE_CASE_RESPONDER
    ResponderAllowedCASEConfigs = CASE::kCASEAllowedConfig_Config2|CASE::kCASEAllowedConfig_Config1;
    ResponderAllowedCASECurves = WEAVE_CONFIG_DEFAULT_CASE_ALLOWED_CURVES;
#endif
#if WEAVE_CONFIG_ENABLE_TAKE_INITIATOR || WEAVE_CONFIG_ENABLE_TAKE_RESPONDER
    mTAKEEngine = NULL;
#endif
#if WEAVE_CONFIG_ENABLE_TAKE_RESPONDER
    mDefaultTAKETokenAuthDelegate = NULL;
#endif
#if WEAVE_CONFIG_ENABLE_TAKE_INITIATOR
    mDefaultTAKEChallengerAuthDelegate = NULL;
#endif
#if WEAVE_CONFIG_ENABLE_KEY_EXPORT_INITIATOR
    mKeyExport = NULL;
    InitiatorKeyExportConfig = KeyExport::kKeyExportConfig_Config1;
    InitiatorAllowedKeyExportConfigs = KeyExport::kKeyExportSupportedConfig_All;
#endif
#if WEAVE_CONFIG_ENABLE_KEY_EXPORT_RESPONDER
    ResponderAllowedKeyExportConfigs = KeyExport::kKeyExportSupportedConfig_All;
#endif
#if WEAVE_CONFIG_SECURITY_TEST_MODE
    CASEUseKnownECDHKey = false;
#endif
#if WEAVE_CONFIG_ENABLE_KEY_EXPORT_INITIATOR || WEAVE_CONFIG_ENABLE_KEY_EXPORT_RESPONDER
    mDefaultKeyExportDelegate = NULL;
#endif
    mStartSecureSession_OnComplete = NULL;
    mStartSecureSession_OnError = NULL;
    mStartSecureSession_ReqState = NULL;
    mRequestedAuthMode = kWeaveAuthMode_NotSpecified;
    mSessionKeyId = WeaveKeyId::kNone;
    mEncType = kWeaveEncryptionType_None;

    mFlags = 0;

    err = ExchangeManager->RegisterUnsolicitedMessageHandler(kWeaveProfile_Security, HandleUnsolicitedMessage, this);
    SuccessOrExit(err);

    aExchangeMgr.MessageLayer->SecurityMgr = this;

    State = kState_Idle;

exit:
    return err;
}

#if WEAVE_CONFIG_PROVIDE_OBSOLESCENT_INTERFACES
WEAVE_ERROR WeaveSecurityManager::Init(WeaveExchangeManager* aExchangeMgr, InetLayer* aInetLayer)
{
    return aExchangeMgr != NULL && aInetLayer != NULL
        ? Init(*aExchangeMgr, *aInetLayer->SystemLayer())
        : WEAVE_ERROR_INVALID_ARGUMENT;
}
#endif // WEAVE_CONFIG_PROVIDE_OBSOLESCENT_INTERFACES


WEAVE_ERROR WeaveSecurityManager::Shutdown(void)
{
    if (State != kState_NotInitialized)
    {
        ExchangeManager->UnregisterUnsolicitedMessageHandler(kWeaveProfile_Security);
        ExchangeManager = NULL;

        // TODO: clean-up in-progress session establishment

        Reset();

        State = kState_NotInitialized;
    }

    return WEAVE_NO_ERROR;
}

void WeaveSecurityManager::HandleUnsolicitedMessage(ExchangeContext *ec, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo,
        uint32_t profileId, uint8_t msgType, PacketBuffer* msgBuf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveSecurityManager *secMgr = (WeaveSecurityManager *)ec->AppState;

    // Handle Key Error Messages.
    if (profileId == kWeaveProfile_Security && msgType == kMsgType_KeyError)
    {
        secMgr->HandleKeyErrorMsg(ec, msgBuf);
        msgBuf = NULL;
        ec = NULL;

        ExitNow();
    }

    // Verify that we don't already have a session establishment in progress.
    VerifyOrExit(secMgr->State == kState_Idle, err = WEAVE_ERROR_SECURITY_MANAGER_BUSY);

    WEAVE_FAULT_INJECT(nl::Weave::FaultInjection::kFault_SecMgrBusy,
        {
            secMgr->AsyncNotifySecurityManagerAvailable();
            ExitNow(err = WEAVE_ERROR_SECURITY_MANAGER_BUSY);
        });

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    if (!ec->HasPeerRequestedAck())
#endif
    {
        // Reject the request if it did not arrive over a connection.
        VerifyOrExit(ec->Con != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);
    }

    // Handle messages that mark the beginning of a PASE interaction...
    if (profileId == kWeaveProfile_Security && msgType == kMsgType_PASEInitiatorStep1)
    {
#if WEAVE_CONFIG_ENABLE_PASE_RESPONDER
        // Reject the request if it did not arrive over a connection.
        // PASE is not supported over WRMP.
        VerifyOrExit(ec->Con != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

        secMgr->HandlePASESessionStart(ec, pktInfo, msgInfo, msgBuf);
        msgBuf = NULL;
#else
        ExitNow(err = WEAVE_ERROR_NOT_IMPLEMENTED);
#endif
    }

    // Handle messages that mark the beginning of a CASE interaction...
    else if (profileId == kWeaveProfile_Security && msgType == kMsgType_CASEBeginSessionRequest)
    {
#if WEAVE_CONFIG_ENABLE_CASE_RESPONDER
        secMgr->HandleCASESessionStart(ec, pktInfo, msgInfo, msgBuf);
        msgBuf = NULL;
#else
        ExitNow(err = WEAVE_ERROR_NOT_IMPLEMENTED);
#endif
    }

    // Handle messages that mark the beginning of a TAKE interaction...
    else if (profileId == kWeaveProfile_Security && msgType == kMsgType_TAKEIdentifyToken)
    {
#if WEAVE_CONFIG_ENABLE_TAKE_RESPONDER
        // Reject the request if it did not arrive over a connection.
        // TAKE is not supported over WRMP.
        VerifyOrExit(ec->Con != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

        secMgr->HandleTAKESessionStart(ec, pktInfo, msgInfo, msgBuf);
        msgBuf = NULL;
#else
        ExitNow(err = WEAVE_ERROR_NOT_IMPLEMENTED);
#endif
    }

    // Handle messages that requests the secret key export...
    else if (profileId == kWeaveProfile_Security && msgType == kMsgType_KeyExportRequest)
    {
#if WEAVE_CONFIG_ENABLE_KEY_EXPORT_RESPONDER
        secMgr->HandleKeyExportRequest(ec, pktInfo, msgInfo, msgBuf);
        msgBuf = NULL;
#else
        ExitNow(err = WEAVE_ERROR_NOT_IMPLEMENTED);
#endif
    }

    // Reject all other message types.
    else
        ExitNow(err = WEAVE_ERROR_INVALID_MESSAGE_TYPE);

exit:
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);
    if (ec != NULL)
    {
        if (err != WEAVE_NO_ERROR)
            SendStatusReport(err, ec);
        ec->Release();
    }
}

#if WEAVE_CONFIG_ENABLE_PASE_INITIATOR

/**
 * This method is called to establish secure PASE session.
 *
 * @param[in] con                 A pointer to the WeaveConnection object.
 * @param[in] requestedAuthMode   The desired means by which the peer should be authenticated.
 *                                This must be one of the PASE auth modes.
 * @param[in] reqState            A pointer to the requester state.
 * @param[in] onComplete          A pointer to the callback function, which will be called once
 *                                requested secure session is established.
 * @param[in] onError             A pointer to the callback function, which will be called if
 *                                requested session establishment fails.
 * @param[in] pw                  A pointer to the PASE secret password.
 * @param[in] pwLen               Length of the PASE secret password.
 *
 * @retval #WEAVE_NO_ERROR         On success.
 *
 */
WEAVE_ERROR WeaveSecurityManager::StartPASESession(WeaveConnection *con, WeaveAuthMode requestedAuthMode, void *reqState,
                                                   SessionEstablishedFunct onComplete, SessionErrorFunct onError,
                                                   const uint8_t *pw, uint16_t pwLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveSessionKey *sessionKey;
    bool clearStateOnError = false;

    // Verify security manager has been initialized.
    VerifyOrExit(State != kState_NotInitialized, err = WEAVE_ERROR_INCORRECT_STATE);

    // Verify there is no session in process.
    VerifyOrExit(State == kState_Idle, err = WEAVE_ERROR_SECURITY_MANAGER_BUSY);

    WEAVE_FAULT_INJECT(nl::Weave::FaultInjection::kFault_SecMgrBusy,
        {
            AsyncNotifySecurityManagerAvailable();
            ExitNow(err = WEAVE_ERROR_SECURITY_MANAGER_BUSY);
        });

    // Verify correct authentication mode.
    VerifyOrExit(IsPASEAuthMode(requestedAuthMode), err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Reject the request if no connection has been specified.
    // PASE is not yet supported over WRMP.
    VerifyOrExit(con != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    State = kState_PASEInProgress;
    mRequestedAuthMode = requestedAuthMode;
    mEncType = kWeaveEncryptionType_AES128CTRSHA1;
    mCon = con;
    mStartSecureSession_OnComplete = onComplete;
    mStartSecureSession_OnError = onError;
    mStartSecureSession_ReqState = reqState;
    mSessionKeyId = WeaveKeyId::kNone;

    // Any error after this point requires call to the Reset() function.
    clearStateOnError = true;

    // Allocate an entry in the session key table identified by a random key id. The actual
    // key itself will be set once the session is established.
    err = FabricState->AllocSessionKey(con->PeerNodeId, WeaveKeyId::kNone, con, sessionKey);
    SuccessOrExit(err);
    sessionKey->SetLocallyInitiated(true);
    mSessionKeyId = sessionKey->MsgEncKey.KeyId;

    // Create a new exchange context.
    err = NewSessionExchange(mCon->PeerNodeId, mCon->PeerAddr, mCon->PeerPort);
    SuccessOrExit(err);

    // Initialize Weave platform memory.
    err = Platform::Security::MemoryInit();
    SuccessOrExit(err);

    // Allocate and initialize PASE engine object.
    mPASEEngine = (WeavePASEEngine *)Platform::Security::MemoryAlloc(sizeof(WeavePASEEngine), true);
    VerifyOrExit(mPASEEngine != NULL, err = WEAVE_ERROR_NO_MEMORY);
    mPASEEngine->Init();

    // Initialize PASE password if provided.
    if (pw != NULL)
    {
        mPASEEngine->Pw = pw;
        mPASEEngine->PwLen = pwLen;
    }

    // Start PASE session.
    StartPASESession();

exit:
    if (err != WEAVE_NO_ERROR && clearStateOnError)
    {
        if (mSessionKeyId != WeaveKeyId::kNone)
            FabricState->RemoveSessionKey(mSessionKeyId, con->PeerNodeId);

        Reset();
    }

    return err;
}

void WeaveSecurityManager::StartPASESession(void)
{
    WEAVE_ERROR err;

    err = SendPASEInitiatorStep1(kPASEConfig_ConfigDefault);
    SuccessOrExit(err);

    mEC->OnMessageReceived = HandlePASEMessageInitiator;
    mEC->OnConnectionClosed = HandleConnectionClosed;

    // Time limit overall PASE duration.
    StartSessionTimer();

exit:
    if (err != WEAVE_NO_ERROR)
        HandleSessionError(err, NULL);
}

void WeaveSecurityManager::HandlePASEMessageInitiator(ExchangeContext *ec, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo,
        uint32_t profileId, uint8_t msgType, PacketBuffer* msgBuf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveSecurityManager *secMgr = (WeaveSecurityManager *)ec->AppState;

    VerifyOrDie(ec == secMgr->mEC);

    // Abort the PASE interaction immediately if we receive a status report message from the responder.
    // This is a signal that the responder does not want to continue.
    if (profileId == kWeaveProfile_Common && msgType == kMsgType_StatusReport)
    {
#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG1
        // Check if Error Status is coming from the old version of PASE code, which only supports Config1
        StatusReport rcvdStatusReport;
        err = StatusReport::parse(msgBuf, rcvdStatusReport);
        SuccessOrExit(err);
        if (rcvdStatusReport.mStatusCode == Security::kStatusCode_PASESupportsOnlyConfig1)
        {
            // Free the received message buffer so that it can be reused to send the outgoing message.
            PacketBuffer::Free(msgBuf);
            msgBuf = NULL;

            err = secMgr->SendPASEInitiatorStep1(kPASEConfig_Config1);
            ExitNow();
        }
        else
#endif
        {
            ExitNow(err = WEAVE_ERROR_STATUS_REPORT_RECEIVED);
        }
    }

    // All other messages must be part of the Security profile.
    VerifyOrExit(profileId == kWeaveProfile_Security, err = WEAVE_ERROR_INVALID_MESSAGE_TYPE);

    // Handle the responder's message...
    switch (msgType)
    {
    case kMsgType_PASEResponderReconfigure:
        uint32_t newConfig;

        err = secMgr->ProcessPASEResponderReconfigure(msgBuf, newConfig);
        SuccessOrExit(err);

        // Free the received message buffer so that it can be reused to send the outgoing message.
        PacketBuffer::Free(msgBuf);
        msgBuf = NULL;

        err = secMgr->SendPASEInitiatorStep1(newConfig);
        SuccessOrExit(err);

        break;

    case kMsgType_PASEResponderStep1:

        err = secMgr->ProcessPASEResponderStep1(msgBuf);
        SuccessOrExit(err);

        break;

    case kMsgType_PASEResponderStep2:

        err = secMgr->ProcessPASEResponderStep2(msgBuf);
        SuccessOrExit(err);

        // Free the received message buffer so that it can be reused to send the outgoing message.
        PacketBuffer::Free(msgBuf);
        msgBuf = NULL;

        err = secMgr->SendPASEInitiatorStep2();
        SuccessOrExit(err);

        if (secMgr->mPASEEngine->State == WeavePASEEngine::kState_InitiatorDone)
        {
            err = secMgr->HandleSessionEstablished();
            SuccessOrExit(err);

            secMgr->HandleSessionComplete();
        }

        break;

    case kMsgType_PASEResponderKeyConfirm:

        err = secMgr->ProcessPASEResponderKeyConfirm(msgBuf);
        SuccessOrExit(err);

        err = secMgr->HandleSessionEstablished();
        SuccessOrExit(err);

        secMgr->HandleSessionComplete();

        break;

    default:

        ExitNow(err = WEAVE_ERROR_INVALID_MESSAGE_TYPE);
        break;
    }

exit:
    if (err != WEAVE_NO_ERROR)
        secMgr->HandleSessionError(err, (err == WEAVE_ERROR_STATUS_REPORT_RECEIVED) ? msgBuf : NULL);
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);
}

__attribute__((noinline))
WEAVE_ERROR WeaveSecurityManager::SendPASEInitiatorStep1(uint32_t paseConfig)
{
    WEAVE_ERROR     err     = WEAVE_NO_ERROR;
    PacketBuffer*   msgBuf  = NULL;
    uint8_t         pwSource;

    // Allocate buffer.
    msgBuf = PacketBuffer::New();
    VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    // Extract the password source from the requested auth mode.
    pwSource = PasswordSourceFromAuthMode(mRequestedAuthMode);

    // Generate and encode PASE step 1 message.
    Platform::Security::OnTimeConsumingCryptoStart();
    err = mPASEEngine->GenerateInitiatorStep1(msgBuf, paseConfig, FabricState->LocalNodeId, mEC->PeerNodeId, mSessionKeyId, kWeaveEncryptionType_AES128CTRSHA1, pwSource, FabricState, true);
    Platform::Security::OnTimeConsumingCryptoDone();
    SuccessOrExit(err);

    // Send PASE step 1 message.
    err = mEC->SendMessage(kWeaveProfile_Security, kMsgType_PASEInitiatorStep1, msgBuf, 0);
    msgBuf = NULL;
    SuccessOrExit(err);

exit:
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);
    return err;
}

__attribute__((noinline))
WEAVE_ERROR WeaveSecurityManager::ProcessPASEResponderReconfigure(PacketBuffer* msgBuf, uint32_t &newConfig)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // Decode and process the responder's reconfigure message.
    err = mPASEEngine->ProcessResponderReconfigure(msgBuf, newConfig);
    SuccessOrExit(err);

exit:
    return err;
}

__attribute__((noinline))
WEAVE_ERROR WeaveSecurityManager::ProcessPASEResponderStep1(PacketBuffer* msgBuf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // Decode and process the responder's step 1 message.
    Platform::Security::OnTimeConsumingCryptoStart();
    err = mPASEEngine->ProcessResponderStep1(msgBuf);
    Platform::Security::OnTimeConsumingCryptoDone();
    SuccessOrExit(err);

exit:
    return err;
}

__attribute__((noinline))
WEAVE_ERROR WeaveSecurityManager::ProcessPASEResponderStep2(PacketBuffer* msgBuf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // Decode and process the responder's step 2 message.
    Platform::Security::OnTimeConsumingCryptoStart();
    err = mPASEEngine->ProcessResponderStep2(msgBuf);
    Platform::Security::OnTimeConsumingCryptoDone();
    SuccessOrExit(err);

exit:
    return err;
}

__attribute__((noinline))
WEAVE_ERROR WeaveSecurityManager::SendPASEInitiatorStep2(void)
{
    WEAVE_ERROR     err     = WEAVE_NO_ERROR;
    PacketBuffer*   msgBuf  = NULL;

    // Allocate buffer.
    msgBuf = PacketBuffer::New();
    VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    // Generate and encode PASE step 1 message.
    Platform::Security::OnTimeConsumingCryptoStart();
    err = mPASEEngine->GenerateInitiatorStep2(msgBuf);
    Platform::Security::OnTimeConsumingCryptoDone();
    SuccessOrExit(err);

    // Send PASE step 2 message.
    err = mEC->SendMessage(kWeaveProfile_Security, kMsgType_PASEInitiatorStep2, msgBuf, 0);
    msgBuf = NULL;
    SuccessOrExit(err);

exit:
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);
    return err;
}

__attribute__((noinline))
WEAVE_ERROR WeaveSecurityManager::ProcessPASEResponderKeyConfirm(PacketBuffer* msgBuf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // Decode and process the responder's key confirmation message.
    err = mPASEEngine->ProcessResponderKeyConfirm(msgBuf);
    SuccessOrExit(err);

exit:
    return err;
}

#else // !WEAVE_CONFIG_ENABLE_PASE_INITIATOR

WEAVE_ERROR WeaveSecurityManager::StartPASESession(WeaveConnection *con, WeaveAuthMode requestedAuthMode, void *reqState,
                                                   SessionEstablishedFunct onComplete, SessionErrorFunct onError,
                                                   const uint8_t *pw, uint16_t pwLen)
{
    return WEAVE_ERROR_NOT_IMPLEMENTED;
}

#endif // WEAVE_CONFIG_ENABLE_PASE_INITIATOR

#if WEAVE_CONFIG_ENABLE_PASE_RESPONDER

void WeaveSecurityManager::HandlePASESessionStart(ExchangeContext *ec, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo, PacketBuffer* msgBuf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // Setup state for the new PASE exchange.
    State = kState_PASEInProgress;
    mEC = ec;
    mCon = ec->Con;
    ec->OnMessageReceived = HandlePASEMessageResponder;
    ec->OnConnectionClosed = HandleConnectionClosed;

    // Ensure the exchange context stays around until we're done with it.
    ec->AddRef();

    // TODO: rate limit unsuccessful PASE exchanges (WEAVE_ERROR_SECURITY_RATE_LIMIT_EXCEEDED)

    // Time limit overall PASE duration.
    StartSessionTimer();

    // Initialize Weave Platform Memory.
    err = Platform::Security::MemoryInit();
    SuccessOrExit(err);

    // Prepare PASE engine and start session
    mPASEEngine = (WeavePASEEngine *)Platform::Security::MemoryAlloc(sizeof(WeavePASEEngine), true);
    VerifyOrExit(mPASEEngine != NULL, err = WEAVE_ERROR_NO_MEMORY);
    mPASEEngine->Init();

    err = ProcessPASEInitiatorStep1(ec, msgBuf);

    // Free the received message buffer so that it can be reused to send the outgoing messages.
    PacketBuffer::Free(msgBuf);
    msgBuf = NULL;

    // Check if ProcessPASEInitiatorStep1 generated Reconfiguration Request
    if (err == WEAVE_ERROR_PASE_RECONFIGURE_REQUIRED)
    {
        err = SendPASEResponderReconfigure();
        SuccessOrExit(err);

        // Reset state.
        Reset();
    }
    else
    {
        SuccessOrExit(err);

        err = SendPASEResponderStep1();
        SuccessOrExit(err);

        err = SendPASEResponderStep2();
        SuccessOrExit(err);
    }

exit:
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);
    if (err != WEAVE_NO_ERROR)
        HandleSessionError(err, NULL);
}

void WeaveSecurityManager::HandlePASEMessageResponder(ExchangeContext *ec, const IPPacketInfo *pktInfo,
        const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer* msgBuf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveSecurityManager *secMgr = (WeaveSecurityManager *)ec->AppState;

    VerifyOrDie(ec == secMgr->mEC);

    // Abort the PASE interaction immediately if we receive a status report message from the initiator.
    // This is a signal that the initiator does not want to continue.
    if (profileId == kWeaveProfile_Common && msgType == kMsgType_StatusReport)
        ExitNow(err = WEAVE_ERROR_STATUS_REPORT_RECEIVED);

    // Otherwise, the only other message expected is an InitiatorStep2.
    VerifyOrExit(profileId == kWeaveProfile_Security && msgType == kMsgType_PASEInitiatorStep2,
                 err = WEAVE_ERROR_INVALID_MESSAGE_TYPE);

    err = secMgr->ProcessPASEInitiatorStep2(msgBuf);
    SuccessOrExit(err);

    // Free the received message buffer so that it can be reused to send the outgoing messages.
    PacketBuffer::Free(msgBuf);
    msgBuf = NULL;

    // If performing key confirmation send a responder key confirmation message.
    if (secMgr->mPASEEngine->PerformKeyConfirmation)
    {
        err = secMgr->SendPASEResponderKeyConfirm();
        SuccessOrExit(err);
    }

    // If we've successfully establish a session, go perform the appropriate actions.
    if (secMgr->mPASEEngine->State == WeavePASEEngine::kState_ResponderDone)
    {
        err = secMgr->HandleSessionEstablished();
        SuccessOrExit(err);

        secMgr->HandleSessionComplete();
    }

exit:
    if (err != WEAVE_NO_ERROR)
        secMgr->HandleSessionError(err, (err == WEAVE_ERROR_STATUS_REPORT_RECEIVED) ? msgBuf : NULL);
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);
}

__attribute__((noinline))
WEAVE_ERROR WeaveSecurityManager::ProcessPASEInitiatorStep1(ExchangeContext *ec, PacketBuffer* msgBuf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveSessionKey *sessionKey;

    // Generate and encode PASE step 1 message.
    Platform::Security::OnTimeConsumingCryptoStart();
    err = mPASEEngine->ProcessInitiatorStep1(msgBuf, FabricState->LocalNodeId, ec->PeerNodeId, FabricState);
    Platform::Security::OnTimeConsumingCryptoDone();
    SuccessOrExit(err);

    // Allocate an entry in the session key table using the key id proposed by the peer.
    // Arrange for the session key to be bound to the connection, such that when the connection
    // closes, the key is removed.
    //
    // If the initiator has proposed a key id that already exists, make sure we don't remove the
    // existing key during the error clean-up process.
    err = FabricState->AllocSessionKey(ec->PeerNodeId, mPASEEngine->SessionKeyId, ec->Con, sessionKey);
    SuccessOrExit(err);
    sessionKey->SetLocallyInitiated(false);
    sessionKey->SetRemoveOnIdle(false); // TODO FUTURE: Set this to true when support for PASE over WRM is implemented.

    // Save the proposed session key id and encryption type.
    mSessionKeyId = mPASEEngine->SessionKeyId;
    mEncType = mPASEEngine->EncryptionType;

exit:
    return err;
}

__attribute__((noinline))
WEAVE_ERROR WeaveSecurityManager::SendPASEResponderReconfigure(void)
{
    WEAVE_ERROR     err     = WEAVE_NO_ERROR;
    PacketBuffer*   msgBuf  = NULL;

    // Allocate buffer.
    msgBuf = PacketBuffer::New();
    VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    // Generate PASE reconfigure message.
    err = mPASEEngine->GenerateResponderReconfigure(msgBuf);
    SuccessOrExit(err);

    // Send PASE reconfigure message.
    err = mEC->SendMessage(kWeaveProfile_Security, kMsgType_PASEResponderReconfigure, msgBuf, 0);
    msgBuf = NULL;
    SuccessOrExit(err);

exit:
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);
    return err;
}

__attribute__((noinline))
WEAVE_ERROR WeaveSecurityManager::SendPASEResponderStep1(void)
{
    WEAVE_ERROR     err     = WEAVE_NO_ERROR;
    PacketBuffer*   msgBuf  = NULL;

    // Allocate buffer.
    msgBuf = PacketBuffer::New();
    VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    // Generate PASE step 1 message.
    Platform::Security::OnTimeConsumingCryptoStart();
    err = mPASEEngine->GenerateResponderStep1(msgBuf);
    Platform::Security::OnTimeConsumingCryptoDone();
    SuccessOrExit(err);

    // Send PASE step 1 message.
    err = mEC->SendMessage(kWeaveProfile_Security, kMsgType_PASEResponderStep1, msgBuf, 0);
    msgBuf = NULL;
    SuccessOrExit(err);

exit:
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);
    return err;
}

__attribute__((noinline))
WEAVE_ERROR WeaveSecurityManager::SendPASEResponderStep2(void)
{
    WEAVE_ERROR     err     = WEAVE_NO_ERROR;
    PacketBuffer*   msgBuf  = NULL;

    // Allocate buffer.
    msgBuf = PacketBuffer::New();
    VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    // Generate PASE step 2 message.
    Platform::Security::OnTimeConsumingCryptoStart();
    err = mPASEEngine->GenerateResponderStep2(msgBuf);
    Platform::Security::OnTimeConsumingCryptoDone();
    SuccessOrExit(err);

    // Send PASE step 2 message.
    err = mEC->SendMessage(kWeaveProfile_Security, kMsgType_PASEResponderStep2, msgBuf, 0);
    msgBuf = NULL;
    SuccessOrExit(err);

exit:
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);
    return err;
}

__attribute__((noinline))
WEAVE_ERROR WeaveSecurityManager::ProcessPASEInitiatorStep2(PacketBuffer* msgBuf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // Decode and process the initiator's step 2 message.
    Platform::Security::OnTimeConsumingCryptoStart();
    err = mPASEEngine->ProcessInitiatorStep2(msgBuf);
    Platform::Security::OnTimeConsumingCryptoDone();
    SuccessOrExit(err);

exit:
    return err;
}

__attribute__((noinline))
WEAVE_ERROR WeaveSecurityManager::SendPASEResponderKeyConfirm(void)
{
    WEAVE_ERROR     err     = WEAVE_NO_ERROR;
    PacketBuffer*   msgBuf  = NULL;

    // Allocate buffer.
    msgBuf = PacketBuffer::New();
    VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    // Generate and encode a key confirmation message.
    err = mPASEEngine->GenerateResponderKeyConfirm(msgBuf);
    SuccessOrExit(err);

    // Send a key confirmation message.
    err = mEC->SendMessage(kWeaveProfile_Security, kMsgType_PASEResponderKeyConfirm, msgBuf, 0);
    msgBuf = NULL;
    SuccessOrExit(err);

exit:
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);
    return err;
}

#endif // WEAVE_CONFIG_ENABLE_PASE_RESPONDER

#if WEAVE_CONFIG_ENABLE_CASE_INITIATOR

/**
 * This method is called to establish new or find existing CASE session.
 *
 * @param[in] con                 A pointer to the WeaveConnection object.
 * @param[in] peerNodeId          The node identifier of the peer.
 * @param[in] peerAddr            The IP address of the peer node.
 * @param[in] peerPort            The port of the peer node.
 * @param[in] requestedAuthMode   The desired means by which the peer should be authenticated.
 *                                This must be one of the CASE auth modes.
 * @param[in] reqState            A pointer to the requester state.
 * @param[in] onComplete          A pointer to the callback function, which will be called once
 *                                requested secure session is established.
 * @param[in] onError             A pointer to the callback function, which will be called if
 *                                requested session establishment fails.
 * @param[in] authDelegate        A pointer to the CASE authentication delegate object.
 * @param[in] terminatingNodeId   The node identifier of the session terminating node.
 *                                When this input is different from kNodeIdNotSpecified that
 *                                indicates that shared secure session was requested.
 *
 * @retval #WEAVE_NO_ERROR         On success.
 *
 */
WEAVE_ERROR WeaveSecurityManager::StartCASESession(WeaveConnection *con, uint64_t peerNodeId, const IPAddress &peerAddr,
                                                   uint16_t peerPort, WeaveAuthMode requestedAuthMode, void *reqState,
                                                   SessionEstablishedFunct onComplete, SessionErrorFunct onError,
                                                   WeaveCASEAuthDelegate *authDelegate, uint64_t terminatingNodeId)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveSessionKey *sessionKey = NULL;
    bool clearStateOnError = false;
    bool isSharedSession = (terminatingNodeId != kNodeIdNotSpecified);
    const uint8_t encType = kWeaveEncryptionType_AES128CTRSHA1; // Only one encryption type supported for now.

    // Verify security manager has been initialized.
    VerifyOrExit(State != kState_NotInitialized, err = WEAVE_ERROR_INCORRECT_STATE);

    // Verify correct authentication mode.
    VerifyOrExit(IsCASEAuthMode(requestedAuthMode), err = WEAVE_ERROR_INVALID_ARGUMENT);

    // If requested session is shared...
    if (isSharedSession)
    {
        // Search for an established shared session to the specified terminating node that matches
        // the requested auth mode and encryption type.  If such a session exists...
        sessionKey = FabricState->FindSharedSession(terminatingNodeId, requestedAuthMode, encType);
        if (sessionKey != NULL)
        {
            // Ensure that the shared session is NOT currently in the process of being established.
            // This situation can arise when establishing CASE over Weave Reliable Messaging.  After
            // the initiator has sent a KeyConfirm message it waits for a WRM ACK from the responder.
            // During this time, the session exists in the session table but is not yet considered
            // ready for use.  Until the ACK is received, additional requests to establish the same
            // shared session should be denied with a SECURITY_MANAGER_BUSY error, which will force
            // the concurrent request to wait until the session is fully established.
            //
            // If the located shared session is NOT in the process of being established...
            if (State != kState_CASEInProgress || mEC->PeerNodeId != terminatingNodeId || mSessionKeyId != sessionKey->MsgEncKey.KeyId)
            {
                // Add a new end node to the list of end nodes associated with the session.
                err = FabricState->AddSharedSessionEndNode(sessionKey, peerNodeId);
                SuccessOrExit(err);

                // Add a reservation for the session.
                ReserveSessionKey(sessionKey);

                // Immediately notify the application that the session has been established.
                onComplete(this, con, reqState, sessionKey->MsgEncKey.KeyId, peerNodeId, encType);

                ExitNow();
            }
        }
    }

    // Verify there is no session in process.
    VerifyOrExit(State == kState_Idle, err = WEAVE_ERROR_SECURITY_MANAGER_BUSY);

    WEAVE_FAULT_INJECT(nl::Weave::FaultInjection::kFault_SecMgrBusy,
        {
            AsyncNotifySecurityManagerAvailable();
            ExitNow(err = WEAVE_ERROR_SECURITY_MANAGER_BUSY);
        });

    State = kState_CASEInProgress;
    mRequestedAuthMode = requestedAuthMode;
    mEncType = encType;
    mCon = con;
    mStartSecureSession_OnComplete = onComplete;
    mStartSecureSession_OnError = onError;
    mStartSecureSession_ReqState = reqState;
    mSessionKeyId = WeaveKeyId::kNone;

    // Any error after that would require state clearing in case of error.
    clearStateOnError = true;

    // Allocate an entry in the session key table identified by a random key id. The actual
    // key itself will be set once the session is established.
    err = FabricState->AllocSessionKey((isSharedSession ? terminatingNodeId : peerNodeId),
                                       WeaveKeyId::kNone, con, sessionKey);
    SuccessOrExit(err);
    sessionKey->SetLocallyInitiated(true);
    sessionKey->SetSharedSession(isSharedSession);
    mSessionKeyId = sessionKey->MsgEncKey.KeyId;

    // If requested session is shared.
    if (isSharedSession)
    {
        // Add new shared session end node to the record.
        err = FabricState->AddSharedSessionEndNode(sessionKey, peerNodeId);
        SuccessOrExit(err);
    }

    // Create a new exchange context.
    err = NewSessionExchange((isSharedSession ? terminatingNodeId : peerNodeId), peerAddr, peerPort);
    SuccessOrExit(err);

    // Initialize Weave Platform Memory.
    err = Platform::Security::MemoryInit();
    SuccessOrExit(err);

    // Allocate and Initialize CASE Engine object
    mCASEEngine = (WeaveCASEEngine *)Platform::Security::MemoryAlloc(sizeof(WeaveCASEEngine), true);
    VerifyOrExit(mCASEEngine != NULL, err = WEAVE_ERROR_NO_MEMORY);
    mCASEEngine->Init();

    // Initialize CASE Authentication Delegate
    if (authDelegate == NULL)
        authDelegate = mDefaultAuthDelegate;
    VerifyOrExit(authDelegate != NULL, err = WEAVE_ERROR_NO_CASE_AUTH_DELEGATE);
    mCASEEngine->AuthDelegate = authDelegate;

    // Set the allowed CASE configs and ECDH curves.
    mCASEEngine->SetAllowedConfigs(InitiatorAllowedCASEConfigs);
    mCASEEngine->SetAllowedCurves(InitiatorAllowedCASECurves);

    // Set the expected peer certificate type based on the requested authentication mode.
    mCASEEngine->SetCertType(CertTypeFromAuthMode(requestedAuthMode));

#if WEAVE_CONFIG_SECURITY_TEST_MODE
    mCASEEngine->SetUseKnownECDHKey(CASEUseKnownECDHKey);
#endif

    // Start CASE Session using specified initiator parameters.
    StartCASESession(InitiatorCASEConfig, InitiatorCASECurveId);

exit:
    if (err != WEAVE_NO_ERROR && clearStateOnError)
    {
        if (sessionKey != NULL)
            FabricState->RemoveSessionKey(sessionKey);

        Reset();
    }

    return err;
}

void WeaveSecurityManager::StartCASESession(uint32_t config, uint32_t curveId)
{
    WEAVE_ERROR err;
    PacketBuffer * msgBuf = NULL;
    uint16_t sendFlags = 0;

    // Allocate a buffer to hold the Begin Session message.
    msgBuf = PacketBuffer::New();
    VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    // Generate the CASE Begin Session message.
    {
        CASE::BeginSessionRequestContext reqCtx;

        reqCtx.Reset();
        reqCtx.SetIsInitiator(true);
        reqCtx.PeerNodeId = mEC->PeerNodeId;
        reqCtx.ProtocolConfig = config;
        mCASEEngine->SetAlternateConfigs(reqCtx);
        reqCtx.CurveId = curveId;
        mCASEEngine->SetAlternateCurves(reqCtx);
        reqCtx.SetPerformKeyConfirm(true);
        reqCtx.SessionKeyId = mSessionKeyId;
        reqCtx.EncryptionType = mEncType;

        Platform::Security::OnTimeConsumingCryptoStart();
        err = mCASEEngine->GenerateBeginSessionRequest(reqCtx, msgBuf);
        Platform::Security::OnTimeConsumingCryptoDone();
        SuccessOrExit(err);
    }

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    if (mCon == NULL)
    {
        sendFlags = ExchangeContext::kSendFlag_RequestAck;
    }
#endif

    // Send the message.
    err = mEC->SendMessage(kWeaveProfile_Security, kMsgType_CASEBeginSessionRequest, msgBuf, sendFlags);
    msgBuf = NULL;
    SuccessOrExit(err);

    mEC->OnMessageReceived = HandleCASEMessageInitiator;
    mEC->OnConnectionClosed = HandleConnectionClosed;

    // Time limit overall CASE duration.
    StartSessionTimer();

exit:
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);
    if (err != WEAVE_NO_ERROR)
        HandleSessionError(err, NULL);
}

void WeaveSecurityManager::HandleCASEMessageInitiator(ExchangeContext *ec, const IPPacketInfo *pktInfo,
        const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer* msgBuf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveSecurityManager *secMgr = (WeaveSecurityManager *)ec->AppState;
    uint16_t sendFlags = 0;

    VerifyOrDie(ec == secMgr->mEC);

    // Abort the CASE interaction immediately if we receive a status report message from the responder.
    // This is a signal that the responder does not want to continue.
    if (profileId == kWeaveProfile_Common && msgType == kMsgType_StatusReport)
        ExitNow(err = WEAVE_ERROR_STATUS_REPORT_RECEIVED);

    // All other messages must be part of the Security profile.
    VerifyOrExit(profileId == kWeaveProfile_Security, err = WEAVE_ERROR_INVALID_MESSAGE_TYPE);

    // If the message is a BeginSessionResponse...
    if (msgType == kMsgType_CASEBeginSessionResponse)
    {
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
        // Flush any pending WRM ACKs before we begin the long crypto operation,
        // to prevent the peer from re-transmitting the Begin Session response.
        err = secMgr->mEC->WRMPFlushAcks();
        SuccessOrExit(err);
#endif

        // Decode and process the BeginSessionResponse.
        {
            CASE::BeginSessionResponseContext respCtx;

            respCtx.Reset();
            respCtx.SetIsInitiator(true);
            respCtx.PeerNodeId = ec->PeerNodeId;
            respCtx.MsgInfo = msgInfo;

            Platform::Security::OnTimeConsumingCryptoStart();
            err = secMgr->mCASEEngine->ProcessBeginSessionResponse(msgBuf, respCtx);
            Platform::Security::OnTimeConsumingCryptoDone();
            SuccessOrExit(err);
        }

        // Release the buffer containing the response.
        PacketBuffer::Free(msgBuf);
        msgBuf = NULL;

        // If performing key confirmation...
        if (secMgr->mCASEEngine->PerformingKeyConfirm())
        {
            // Generate and encode an InitiatorKeyConfirm message.
            msgBuf = PacketBuffer::New();
            VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);
            err = secMgr->mCASEEngine->GenerateInitiatorKeyConfirm(msgBuf);
            SuccessOrExit(err);

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
            if (secMgr->mCon == NULL)
            {
                sendFlags = ExchangeContext::kSendFlag_RequestAck;
            }
#endif

            // Send the InitiatorKeyConfirm message to the peer.
            err = secMgr->mEC->SendMessage(kWeaveProfile_Security, kMsgType_CASEInitiatorKeyConfirm, msgBuf, sendFlags);
            msgBuf = NULL;
            SuccessOrExit(err);
        }

        // Initialize the newly established security session.
        err = secMgr->HandleSessionEstablished();
        SuccessOrExit(err);

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
        // Complete the session when any of these is true:
        //     - session establishment was done over a Weave connection
        //     - key confirmation wasn't required
        // For WRMP when key confirmation is required, the session will be completed
        // on one of these events:
        //     - Received Ack from the peer for the last message on this exchange (CASEInitiatorKeyConfirm)
        //     - Received first message from the peer encrypted with established session key (mSessionKeyId)
        if (secMgr->mCon || !secMgr->mCASEEngine->PerformingKeyConfirm())
#endif
        {
            secMgr->HandleSessionComplete();
        }
    }

    // Otherwise, if the message is a Reconfigure...
    else if (msgType == kMsgType_CASEReconfigure)
    {
        // Process the reconfigure message.  If this proposed alternate configuration is not acceptable,
        // the call will fail with an error.
        CASE::ReconfigureContext reconfCtx;
        err = secMgr->mCASEEngine->ProcessReconfigure(msgBuf, reconfCtx);
        SuccessOrExit(err);

        // Release the buffer containing the response.
        PacketBuffer::Free(msgBuf);
        msgBuf = NULL;

        // Create a new exchange context for the new CASE session.  This will result in the old exchange context
        // being closed. (NOTE: We cannot re-use the initial exchange for the new CASE session because the peer
        // believes the exchange ended when the Reconfigure message was sent).
        err = secMgr->NewSessionExchange(ec->PeerNodeId, ec->PeerAddr, ec->PeerPort);
        SuccessOrExit(err);

        // Restart the CASE session using the peer's propose parameters.
        secMgr->StartCASESession(reconfCtx.ProtocolConfig, reconfCtx.CurveId);
    }

    // Fail if the message is unrecognized.
    else
        ExitNow(err = WEAVE_ERROR_INVALID_MESSAGE_TYPE);

exit:
    if (err != WEAVE_NO_ERROR)
        secMgr->HandleSessionError(err, (err == WEAVE_ERROR_STATUS_REPORT_RECEIVED) ? msgBuf : NULL);
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);
}

#else // !WEAVE_CONFIG_ENABLE_CASE_INITIATOR

WEAVE_ERROR WeaveSecurityManager::StartCASESession(WeaveConnection *con, uint64_t peerNodeId, const IPAddress &peerAddr,
                                                   uint16_t peerPort, WeaveAuthMode requestedAuthMode, void *reqState,
                                                   SessionEstablishedFunct onComplete, SessionErrorFunct onError,
                                                   WeaveCASEAuthDelegate *authDelegate, uint64_t terminatingNodeId)
{
    return WEAVE_ERROR_NOT_IMPLEMENTED;
}

#endif // WEAVE_CONFIG_ENABLE_CASE_INITIATOR

#if WEAVE_CONFIG_ENABLE_CASE_RESPONDER

void WeaveSecurityManager::HandleCASESessionStart(ExchangeContext *ec, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo, PacketBuffer* msgBuf)
{
    WEAVE_ERROR err;
    WeaveSessionKey * sessionKey;
    CASE::BeginSessionRequestContext reqCtx;
    CASE::ReconfigureContext reconfCtx;
    PacketBuffer * respMsgBuf = NULL;
    uint16_t sendFlags = 0;

    State = kState_CASEInProgress;
    mEC = ec;
    mCon = ec->Con;
    ec->OnMessageReceived = HandleCASEMessageResponder;
    ec->OnConnectionClosed = HandleConnectionClosed;

    // Ensure the exchange context stays around until we're done with it.
    ec->AddRef();

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    if (mCon == NULL)
    {
        mEC->OnAckRcvd = WRMPHandleAckRcvd;
        mEC->OnSendError = WRMPHandleSendError;

        // Flush any pending WRM ACKs before we begin the long crypto operation,
        // to prevent the peer from re-transmitting the Begin Session request.
        err = mEC->WRMPFlushAcks();
        SuccessOrExit(err);

        sendFlags |= ExchangeContext::kSendFlag_RequestAck;
    }
#endif

    // Initialize Weave Platform Memory
    err = Platform::Security::MemoryInit();
    SuccessOrExit(err);

    // Allocate and initialize a CASE engine.
    mCASEEngine = (WeaveCASEEngine *)Platform::Security::MemoryAlloc(sizeof(WeaveCASEEngine), true);
    VerifyOrExit(mCASEEngine != NULL, err = WEAVE_ERROR_NO_MEMORY);
    mCASEEngine->Init();

    // Since this session is being initiated by a remote node, use the default auth delegate.
    // Reject the request if no auth delegate has been set.
    VerifyOrExit(mDefaultAuthDelegate != NULL, err = WEAVE_ERROR_NO_CASE_AUTH_DELEGATE);
    mCASEEngine->AuthDelegate = mDefaultAuthDelegate;

    // Set the allowed protocol options for a responder.
    mCASEEngine->SetAllowedConfigs(ResponderAllowedCASEConfigs);
    mCASEEngine->SetAllowedCurves(ResponderAllowedCASECurves);
    mCASEEngine->SetResponderRequiresKeyConfirm(true);

#if WEAVE_CONFIG_SECURITY_TEST_MODE
    mCASEEngine->SetUseKnownECDHKey(CASEUseKnownECDHKey);
#endif

    // Process the BeginSessionRequest
    reqCtx.Reset();
    reqCtx.PeerNodeId = ec->PeerNodeId;
    reqCtx.MsgInfo = msgInfo;
    reconfCtx.Reset();
    Platform::Security::OnTimeConsumingCryptoStart();
    err = mCASEEngine->ProcessBeginSessionRequest(msgBuf, reqCtx, reconfCtx);
    Platform::Security::OnTimeConsumingCryptoDone();
    if (err != WEAVE_ERROR_CASE_RECONFIG_REQUIRED)
        SuccessOrExit(err);

    // If a reconfigure is required...
    if (err == WEAVE_ERROR_CASE_RECONFIG_REQUIRED)
    {
        // Discard the request buffer.
        PacketBuffer::Free(msgBuf);
        msgBuf = NULL;

        // Encode a CASE Reconfigure message into a new buffer.
        respMsgBuf = PacketBuffer::New();
        VerifyOrExit(respMsgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);
        err = reconfCtx.Encode(respMsgBuf);
        SuccessOrExit(err);

        // Send the Reconfigure message to the peer.
        err = ec->SendMessage(kWeaveProfile_Security, kMsgType_CASEReconfigure, respMsgBuf, sendFlags);
        respMsgBuf = NULL;
        SuccessOrExit(err);

        // Reset the security manager.
        Reset();
    }

    // Otherwise the proposed protocol parameters are acceptable, so...
    else
    {
        // Allocate an entry in the session key table using the key id proposed by the peer.
        // If the session is being established over a Weave connection, arrange for the session key to
        // be bound to the connection, such that when the connection closes, the key is removed.
        // Set the RemoveOnIdle flag so that the session will be automatically removed after a period of
        // inactivity (note that this only applies to sessions that are NOT bound to connections).
        err = FabricState->AllocSessionKey(ec->PeerNodeId, reqCtx.SessionKeyId, ec->Con, sessionKey);
        SuccessOrExit(err);
        sessionKey->SetLocallyInitiated(false);
        sessionKey->SetRemoveOnIdle(true);

        // Save the proposed session key id and encryption type.
        mSessionKeyId = reqCtx.SessionKeyId;
        mEncType = reqCtx.EncryptionType;

        // Allocate a buffer to hold the encoded BeginSessionResponse message.
        respMsgBuf = PacketBuffer::New();
        VerifyOrExit(respMsgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

        // Generate the BeginSessionResponse message.
        {
            CASE::BeginSessionResponseContext respCtx;

            respCtx.Reset();
            respCtx.PeerNodeId = ec->PeerNodeId;
            respCtx.MsgInfo = msgInfo;
            respCtx.ProtocolConfig = reqCtx.ProtocolConfig;
            respCtx.CurveId = reqCtx.CurveId;
            respCtx.SetPerformKeyConfirm(true);

            Platform::Security::OnTimeConsumingCryptoStart();
            err = mCASEEngine->GenerateBeginSessionResponse(respCtx, respMsgBuf, reqCtx);
            Platform::Security::OnTimeConsumingCryptoDone();
            SuccessOrExit(err);
        }

        // Send the BeginSessionResponse message to the peer.
        err = ec->SendMessage(kWeaveProfile_Security, kMsgType_CASEBeginSessionResponse, respMsgBuf, sendFlags);
        respMsgBuf = NULL;
        SuccessOrExit(err);

        // Start a timer to limit the overall duration of session establishment.
        StartSessionTimer();

        // If the CASE interaction is complete...
        // (NOTE: this will only be true if the initiator didn't request key confirmation).
        if (mCASEEngine->State == CASE::WeaveCASEEngine::kState_Complete)
        {
            // Initialize the new session.
            err = HandleSessionEstablished();
            SuccessOrExit(err);

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
            // 1. Complete the session now if it was established over a connection.
            // 2. For WRMP the session will be completed on one of these events:
            //     - Received Ack from the peer for the last message on this exchange (CASEBeginSessionResponse)
            //     - Received first message from the peer encrypted with established session key (mSessionKeyId)
            if (mCon)
#endif
            {
                HandleSessionComplete();
            }
        }
    }

exit:
    if (err != WEAVE_NO_ERROR)
        HandleSessionError(err, NULL);
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);
    if (respMsgBuf != NULL)
        PacketBuffer::Free(respMsgBuf);
}

void WeaveSecurityManager::HandleCASEMessageResponder(ExchangeContext *ec, const IPPacketInfo *pktInfo,
        const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer* msgBuf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveSecurityManager *secMgr = (WeaveSecurityManager *)ec->AppState;

    VerifyOrDie(ec == secMgr->mEC);

    // Abort the CASE interaction immediately if we receive a status report message from the initiator.
    // This is a signal that the initiator does not want to continue.
    if (profileId == kWeaveProfile_Common && msgType == kMsgType_StatusReport)
        ExitNow(err = WEAVE_ERROR_STATUS_REPORT_RECEIVED);

    // Otherwise, the only other message expected is an InitiatorKeyConfirm.
    VerifyOrExit(profileId == kWeaveProfile_Security && msgType == kMsgType_CASEInitiatorKeyConfirm,
                 err = WEAVE_ERROR_INVALID_MESSAGE_TYPE);

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    // Flush any pending WRM ACKs to give sooner notification to the peer that current
    // CASE session establishment can be finalized.
    err = secMgr->mEC->WRMPFlushAcks();
    SuccessOrExit(err);
#endif

    // Process the initiator's key confirm message.
    // NOTE: No need to initialize crypto memory for this call.
    err = secMgr->mCASEEngine->ProcessInitiatorKeyConfirm(msgBuf);
    SuccessOrExit(err);

    // At this point the session is established.
    err = secMgr->HandleSessionEstablished();
    SuccessOrExit(err);

    // Complete the session and notify the user.
    secMgr->HandleSessionComplete();

exit:
    if (err != WEAVE_NO_ERROR)
        secMgr->HandleSessionError(err, (err == WEAVE_ERROR_STATUS_REPORT_RECEIVED) ? msgBuf : NULL);
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);
}

#endif // WEAVE_CONFIG_ENABLE_CASE_RESPONDER

#if WEAVE_CONFIG_ENABLE_TAKE_INITIATOR

/**
 * This method is called to establish secure TAKE session.
 *
 * @param[in] con                 A pointer to the WeaveConnection object.
 * @param[in] requestedAuthMode   The desired means by which the peer should be authenticated.
 *                                This must be one of the TAKE authentication modes.
 * @param[in] reqState            A pointer to the requester state.
 * @param[in] onComplete          A pointer to the callback function, which will be called once
 *                                requested secure session is established.
 * @param[in] onError             A pointer to the callback function, which will be called if
 *                                requested session establishment fails.
 * @param[in] encryptAuthPhase    A boolean flag that indicates whether protocol authentication
 *                                phase should be encrypted.
 * @param[in] encryptCommPhase    A boolean flag that indicates whether protocol communication
 *                                phase should be encrypted.
 * @param[in] timeLimitedIK       A boolean flag that indicates whether Identification Key (IK)
 *                                is time limited.
 * @param[in] sendChallengerId    A boolean flag that indicates whether challenger identification
 *                                should be included in the message. If it is not included the
 *                                Weave node ID value is used as a challenger ID.
 * @param[in] authDelegate        A pointer to the TAKE challenger authentication delegate object.
 *
 * @retval #WEAVE_NO_ERROR        On success.
 *
 */
WEAVE_ERROR WeaveSecurityManager::StartTAKESession(WeaveConnection *con, WeaveAuthMode requestedAuthMode, void *reqState,
                                                   SessionEstablishedFunct onComplete, SessionErrorFunct onError,
                                                   bool encryptAuthPhase, bool encryptCommPhase,
                                                   bool timeLimitedIK, bool sendChallengerId,
                                                   WeaveTAKEChallengerAuthDelegate *authDelegate)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    bool useSessionKeyID = encryptAuthPhase || encryptCommPhase;
    bool clearStateOnError = false;

    // Verify security manager has been initialized.
    VerifyOrExit(State != kState_NotInitialized, err = WEAVE_ERROR_INCORRECT_STATE);

    // Verify there is no session in process.
    VerifyOrExit(State == kState_Idle, err = WEAVE_ERROR_SECURITY_MANAGER_BUSY);

    WEAVE_FAULT_INJECT(nl::Weave::FaultInjection::kFault_SecMgrBusy,
        {
            AsyncNotifySecurityManagerAvailable();
            ExitNow(err = WEAVE_ERROR_SECURITY_MANAGER_BUSY);
        });

    // Verify correct authentication mode (only one supported currently).
    VerifyOrExit(requestedAuthMode == kWeaveAuthMode_TAKE_IdentificationKey, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Reject the request if no connection has been specified.
    VerifyOrExit(con != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    State = kState_TAKEInProgress;
    mRequestedAuthMode = requestedAuthMode;
    mEncType = kWeaveEncryptionType_AES128CTRSHA1;
    mCon = con;
    mStartSecureSession_OnComplete = onComplete;
    mStartSecureSession_OnError = onError;
    mStartSecureSession_ReqState = reqState;
    mSessionKeyId = WeaveKeyId::kNone;

    // Any error after this point requires call to the Reset() function.
    clearStateOnError = true;

    // Allocate an entry in the session key table identified by a random key id. The actual
    // key itself will be set once the session is established.
    if (useSessionKeyID)
    {
        WeaveSessionKey *sessionKey;
        err = FabricState->AllocSessionKey(con->PeerNodeId, WeaveKeyId::kNone, con, sessionKey);
        SuccessOrExit(err);
        sessionKey->SetLocallyInitiated(true);
        mSessionKeyId = sessionKey->MsgEncKey.KeyId;
    }

    // Create a new exchange context.
    err = NewSessionExchange(mCon->PeerNodeId, mCon->PeerAddr, mCon->PeerPort);
    SuccessOrExit(err);

    // Initialize Weave platform memory.
    err = Platform::Security::MemoryInit();
    SuccessOrExit(err);

    // Allocate and initialize TAKE engine object.
    mTAKEEngine = (WeaveTAKEEngine *)Platform::Security::MemoryAlloc(sizeof(WeaveTAKEEngine), true);
    VerifyOrExit(mTAKEEngine != NULL, err = WEAVE_ERROR_NO_MEMORY);
    mTAKEEngine->Init();

    if (authDelegate == NULL)
        authDelegate = mDefaultTAKEChallengerAuthDelegate;
    VerifyOrExit(authDelegate != NULL, err = WEAVE_ERROR_NO_TAKE_AUTH_DELEGATE);
    mTAKEEngine->ChallengerAuthDelegate = authDelegate;

    // Start TAKE session.
    StartTAKESession(encryptAuthPhase, encryptCommPhase, timeLimitedIK, sendChallengerId);

exit:
    if (err != WEAVE_NO_ERROR && clearStateOnError)
    {
        FabricState->RemoveSessionKey(mSessionKeyId, con->PeerNodeId);

        Reset();
    }

    return err;
}

void WeaveSecurityManager::StartTAKESession(bool encryptAuthPhase, bool encryptCommPhase, bool timeLimitedIK, bool sendChallengerId)
{
    WEAVE_ERROR err;

    err = SendTAKEIdentifyToken(TAKE::kTAKEConfig_Config1, encryptAuthPhase, encryptCommPhase, timeLimitedIK, sendChallengerId);
    SuccessOrExit(err);

    mEncType = mTAKEEngine->GetEncryptionType();

    mEC->OnMessageReceived = HandleTAKEMessageInitiator;
    mEC->OnConnectionClosed = HandleConnectionClosed;

    // Using a smaller timeout may help prevent Relay Attack.
    // TODO: consider reducing the timeout, and using different values of timeout
    // for first and subsequent authentication.
    StartSessionTimer();

exit:
    if (err != WEAVE_NO_ERROR)
        HandleSessionError(err, NULL);
}


void WeaveSecurityManager::HandleTAKEMessageInitiator(ExchangeContext *ec, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo,
        uint32_t profileId, uint8_t msgType, PacketBuffer* msgBuf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveSecurityManager *secMgr = (WeaveSecurityManager *)ec->AppState;

    VerifyOrDie(ec == secMgr->mEC);

    // Abort the TAKE interaction immediately if we receive a status report message from the responder.
    // This is a signal that the responder does not want to continue.
    if (profileId == kWeaveProfile_Common && msgType == kMsgType_StatusReport)
    {
        ExitNow(err = WEAVE_ERROR_STATUS_REPORT_RECEIVED);
    }

    // All other messages must be part of the Security profile.
    VerifyOrExit(profileId == kWeaveProfile_Security, err = WEAVE_ERROR_INVALID_MESSAGE_TYPE);

    // Handle the responder's message...
    switch (msgType)
    {
    case kMsgType_TAKEIdentifyTokenResponse:
    {
        err = secMgr->ProcessTAKEIdentifyTokenResponse(msgBuf);
        bool doReauth = err == WEAVE_ERROR_TAKE_REAUTH_POSSIBLE;

        if (!doReauth)
            SuccessOrExit(err);

        if (secMgr->mTAKEEngine->IsEncryptAuthPhase())
        {
            err = secMgr->CreateTAKESecureSession();
            SuccessOrExit(err);
        }

        // Free the received message buffer so that it can be reused to send the outgoing message.
        PacketBuffer::Free(msgBuf);
        msgBuf = NULL;

        if (doReauth)
        {
            err = secMgr->SendTAKEReAuthenticateToken();
        }
        else
        {
            err = secMgr->SendTAKEAuthenticateToken();
        }
        SuccessOrExit(err);
        break;
    }
    case kMsgType_TAKETokenReconfigure:
        uint8_t newConfig;

        err = secMgr->ProcessTAKETokenReconfigure(newConfig, msgBuf);
        SuccessOrExit(err);

        // Free the received message buffer so that it can be reused to send the outgoing message.
        PacketBuffer::Free(msgBuf);
        msgBuf = NULL;

        err = secMgr->SendTAKEIdentifyToken(newConfig, secMgr->mTAKEEngine->IsEncryptAuthPhase(),
                secMgr->mTAKEEngine->IsEncryptCommPhase(), secMgr->mTAKEEngine->IsTimeLimitedIK(), secMgr->mTAKEEngine->HasSentChallengerId());
        SuccessOrExit(err);
        break;

    case kMsgType_TAKEAuthenticateTokenResponse:
        err = secMgr->ProcessTAKEAuthenticateTokenResponse(msgBuf);
        SuccessOrExit(err);

        // Free the received message buffer so that it can be reused to send the outgoing message.
        PacketBuffer::Free(msgBuf);
        msgBuf = NULL;

        err = secMgr->FinishTAKESetUp();
        SuccessOrExit(err);

        secMgr->HandleSessionComplete();
        break;

    case kMsgType_TAKEReAuthenticateTokenResponse:
        err = secMgr->ProcessTAKEReAuthenticateTokenResponse(msgBuf);
        SuccessOrExit(err);

        // Free the received message buffer so that it can be reused to send the outgoing message.
        PacketBuffer::Free(msgBuf);
        msgBuf = NULL;

        err = secMgr->FinishTAKESetUp();
        SuccessOrExit(err);

        secMgr->HandleSessionComplete();
        break;

    default:
        ExitNow(err = WEAVE_ERROR_INVALID_MESSAGE_TYPE);
        break;
    }

exit:
    if (err != WEAVE_NO_ERROR)
        secMgr->HandleSessionError(err, (err == WEAVE_ERROR_STATUS_REPORT_RECEIVED) ? msgBuf : NULL);
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);
}

WEAVE_ERROR WeaveSecurityManager::SendTAKEIdentifyToken(uint8_t takeConfig, bool encryptAuthPhase, bool encryptCommPhase, bool timeLimitedIK, bool sendChallengerId)
{
    WEAVE_ERROR     err;
    PacketBuffer*   msgBuf = NULL;

    /// Generate, encode and send our Token Identify message.
    msgBuf = PacketBuffer::New();
    VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    err = mTAKEEngine->GenerateIdentifyTokenMessage(mSessionKeyId, takeConfig, encryptAuthPhase, encryptCommPhase, timeLimitedIK, sendChallengerId, kWeaveEncryptionType_AES128CTRSHA1, FabricState->LocalNodeId, msgBuf);
    SuccessOrExit(err);

    // Send the message.
    err = mEC->SendMessage(kWeaveProfile_Security, kMsgType_TAKEIdentifyToken, msgBuf, 0);
    msgBuf = NULL;
    SuccessOrExit(err);

exit:
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);
    return err;
}


WEAVE_ERROR WeaveSecurityManager::ProcessTAKEIdentifyTokenResponse(const PacketBuffer* msgBuf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    err = mTAKEEngine->ProcessIdentifyTokenResponseMessage(msgBuf);
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR WeaveSecurityManager::ProcessTAKETokenReconfigure(uint8_t& config, const PacketBuffer* msgBuf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    err = mTAKEEngine->ProcessTokenReconfigureMessage(config, msgBuf);
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR WeaveSecurityManager::SendTAKEAuthenticateToken(void)
{
    WEAVE_ERROR     err = WEAVE_NO_ERROR;
    PacketBuffer*   msgBuf = NULL;

    msgBuf = PacketBuffer::New();
    VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    Platform::Security::OnTimeConsumingCryptoStart();
    err = mTAKEEngine->GenerateAuthenticateTokenMessage(msgBuf);
    Platform::Security::OnTimeConsumingCryptoDone();
    SuccessOrExit(err);

    err = mEC->SendMessage(kWeaveProfile_Security, kMsgType_TAKEAuthenticateToken, msgBuf, 0);
    msgBuf = NULL;
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR WeaveSecurityManager::ProcessTAKEAuthenticateTokenResponse(const PacketBuffer* msgBuf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    Platform::Security::OnTimeConsumingCryptoStart();
    err = mTAKEEngine->ProcessAuthenticateTokenResponseMessage(msgBuf);
    Platform::Security::OnTimeConsumingCryptoDone();
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR WeaveSecurityManager::SendTAKEReAuthenticateToken(void)
{
    WEAVE_ERROR     err = WEAVE_NO_ERROR;
    PacketBuffer*   msgBuf = NULL;

    msgBuf = PacketBuffer::New();
    VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    err = mTAKEEngine->GenerateReAuthenticateTokenMessage(msgBuf);
    SuccessOrExit(err);

    err = mEC->SendMessage(kWeaveProfile_Security, kMsgType_TAKEReAuthenticateToken, msgBuf, 0);
    msgBuf = NULL;
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR WeaveSecurityManager::ProcessTAKEReAuthenticateTokenResponse(const PacketBuffer* msgBuf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    err = mTAKEEngine->ProcessReAuthenticateTokenResponseMessage(msgBuf);
    SuccessOrExit(err);

exit:
    return err;
}

#else // !WEAVE_CONFIG_ENABLE_TAKE_INITIATOR

WEAVE_ERROR WeaveSecurityManager::StartTAKESession(WeaveConnection *con, WeaveAuthMode authMode, void *reqState,
                                                   SessionEstablishedFunct onComplete, SessionErrorFunct onError,
                                                   bool encryptAuthPhase, bool encryptCommPhase,
                                                   bool timeLimitedIK, bool sendChallengerId,
                                                   WeaveTAKEChallengerAuthDelegate *authDelegate)
{
    return WEAVE_ERROR_NOT_IMPLEMENTED;
}

#endif // WEAVE_CONFIG_ENABLE_TAKE_INITIATOR

#if WEAVE_CONFIG_ENABLE_TAKE_RESPONDER

void WeaveSecurityManager::HandleTAKESessionStart(ExchangeContext *ec, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo, PacketBuffer* msgBuf)
{
    WEAVE_ERROR     err = WEAVE_NO_ERROR;
    PacketBuffer*   respMsgBuf = NULL;

    VerifyOrExit(mDefaultTAKETokenAuthDelegate != NULL, err = WEAVE_ERROR_NO_TAKE_AUTH_DELEGATE);

    // Setup state for the new TAKE exchange.
    State = kState_TAKEInProgress;
    mEC = ec;
    mCon = ec->Con;

    ec->OnMessageReceived = HandleTAKEMessageResponder;
    ec->OnConnectionClosed = HandleConnectionClosed;

    // Ensure the exchange context stays around until we're done with it.
    ec->AddRef();

    StartSessionTimer();

    // Initialize Weave Platform Memory
    err = Platform::Security::MemoryInit();
    SuccessOrExit(err);

    // Prepare TAKE engine and start session
    mTAKEEngine = (WeaveTAKEEngine *)Platform::Security::MemoryAlloc(sizeof(WeaveTAKEEngine), true);
    VerifyOrExit(mTAKEEngine != NULL, err = WEAVE_ERROR_NO_MEMORY);
    mTAKEEngine->Init();

    mTAKEEngine->TokenAuthDelegate = mDefaultTAKETokenAuthDelegate;

    err = mTAKEEngine->ProcessIdentifyTokenMessage(ec->PeerNodeId, msgBuf);
    PacketBuffer::Free(msgBuf);
    msgBuf = NULL;

    if (err == WEAVE_ERROR_TAKE_RECONFIGURE_REQUIRED)
    {
        err = SendTAKETokenReconfigure();
        SuccessOrExit(err);

        // Reset state.
        Reset();

        ExitNow();
    }

    SuccessOrExit(err);

    if (mTAKEEngine->UseSessionKey())
    {
        WeaveSessionKey *sessionKey;
        err = FabricState->AllocSessionKey(ec->PeerNodeId, mTAKEEngine->SessionKeyId, ec->Con, sessionKey);
        SuccessOrExit(err);
        sessionKey->SetLocallyInitiated(false);
        sessionKey->SetRemoveOnIdle(true);
        mSessionKeyId = mTAKEEngine->SessionKeyId;
        mEncType = mTAKEEngine->GetEncryptionType();
    }

    respMsgBuf = PacketBuffer::New();
    VerifyOrExit(respMsgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    err = mTAKEEngine->GenerateIdentifyTokenResponseMessage(respMsgBuf);
    SuccessOrExit(err);

    err = ec->SendMessage(kWeaveProfile_Security, kMsgType_TAKEIdentifyTokenResponse, respMsgBuf);
    respMsgBuf = NULL;
    SuccessOrExit(err);

    if (mTAKEEngine->IsEncryptAuthPhase())
    {
        err = CreateTAKESecureSession();
        SuccessOrExit(err);
    }

exit:
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);
    if (respMsgBuf != NULL)
        PacketBuffer::Free(respMsgBuf);
    if (err != WEAVE_NO_ERROR)
        HandleSessionError(err, NULL);
}

void WeaveSecurityManager::HandleTAKEMessageResponder(ExchangeContext *ec, const IPPacketInfo *pktInfo,
        const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer* msgBuf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveSecurityManager *secMgr = (WeaveSecurityManager *)ec->AppState;

    VerifyOrDie(ec == secMgr->mEC);

    // Abort the TAKE interaction immediately if we receive a status report message from the initiator.
    // This is a signal that the initiator does not want to continue.
    if (profileId == kWeaveProfile_Common && msgType == kMsgType_StatusReport)
        ExitNow(err = WEAVE_ERROR_STATUS_REPORT_RECEIVED);

    // Otherwise, the only other message expected is of profile security
    VerifyOrExit(profileId == kWeaveProfile_Security, err = WEAVE_ERROR_INVALID_MESSAGE_TYPE);

    switch (msgType)
    {
    case kMsgType_TAKEAuthenticateToken:
        err = secMgr->ProcessTAKEAuthenticateToken(msgBuf);
        SuccessOrExit(err);

        err = secMgr->SendTAKEAuthenticateTokenResponse();
        SuccessOrExit(err);

        // freeing the buffer after the generation of the next message in order to not copy the gx array
        PacketBuffer::Free(msgBuf);
        msgBuf = NULL;

        err = secMgr->FinishTAKESetUp();
        SuccessOrExit(err);

        secMgr->HandleSessionComplete();
        break;

    case kMsgType_TAKEReAuthenticateToken:
        err = secMgr->ProcessTAKEReAuthenticateToken(msgBuf);
        SuccessOrExit(err);

        PacketBuffer::Free(msgBuf);
        msgBuf = NULL;

        err = secMgr->SendTAKEReAuthenticateTokenResponse();
        SuccessOrExit(err);

        err = secMgr->FinishTAKESetUp();
        SuccessOrExit(err);

        secMgr->HandleSessionComplete();
        break;

    default:
        ExitNow(err = WEAVE_ERROR_INVALID_MESSAGE_TYPE);

    }

    // Free the received message buffer so that it can be reused to send the outgoing messages.
    PacketBuffer::Free(msgBuf);
    msgBuf = NULL;

exit:
    if (err != WEAVE_NO_ERROR)
        secMgr->HandleSessionError(err, (err == WEAVE_ERROR_STATUS_REPORT_RECEIVED) ? msgBuf : NULL);
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);
}

WEAVE_ERROR WeaveSecurityManager::ProcessTAKEAuthenticateToken(const PacketBuffer* msgBuf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    Platform::Security::OnTimeConsumingCryptoStart();
    err = mTAKEEngine->ProcessAuthenticateTokenMessage(msgBuf);
    Platform::Security::OnTimeConsumingCryptoDone();
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR WeaveSecurityManager::SendTAKETokenReconfigure(void)
{
    WEAVE_ERROR     err     = WEAVE_NO_ERROR;
    PacketBuffer*   msgBuf  = NULL;

    // Generate, encode and send reconfigure message.
    msgBuf = PacketBuffer::New();
    VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    err = mTAKEEngine->GenerateTokenReconfigureMessage(msgBuf);
    SuccessOrExit(err);

    err = mEC->SendMessage(kWeaveProfile_Security, kMsgType_TAKETokenReconfigure, msgBuf, 0);
    msgBuf = NULL;
    SuccessOrExit(err);

exit:
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);
    return err;
}

WEAVE_ERROR WeaveSecurityManager::SendTAKEAuthenticateTokenResponse(void)
{
    WEAVE_ERROR     err     = WEAVE_NO_ERROR;
    PacketBuffer*   msgBuf  = NULL;

    msgBuf = PacketBuffer::New();
    VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    Platform::Security::OnTimeConsumingCryptoStart();
    err = mTAKEEngine->GenerateAuthenticateTokenResponseMessage(msgBuf);
    Platform::Security::OnTimeConsumingCryptoDone();
    SuccessOrExit(err);

    err = mEC->SendMessage(kWeaveProfile_Security, kMsgType_TAKEAuthenticateTokenResponse, msgBuf, 0);
    msgBuf = NULL;
    SuccessOrExit(err);

exit:
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);
    return err;
}

WEAVE_ERROR WeaveSecurityManager::ProcessTAKEReAuthenticateToken(const PacketBuffer* msgBuf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    err = mTAKEEngine->ProcessReAuthenticateTokenMessage(msgBuf);
    SuccessOrExit(err);

exit:
    return err;
}


WEAVE_ERROR WeaveSecurityManager::SendTAKEReAuthenticateTokenResponse(void)
{
    WEAVE_ERROR     err     = WEAVE_NO_ERROR;
    PacketBuffer*   msgBuf  = NULL;

    msgBuf = PacketBuffer::New();
    VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    err = mTAKEEngine->GenerateReAuthenticateTokenResponseMessage(msgBuf);
    SuccessOrExit(err);

    err = mEC->SendMessage(kWeaveProfile_Security, kMsgType_TAKEReAuthenticateTokenResponse, msgBuf, 0);
    msgBuf = NULL;
    SuccessOrExit(err);

exit:
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);
    return err;
}

#endif // WEAVE_CONFIG_ENABLE_TAKE_RESPONDER

#if WEAVE_CONFIG_ENABLE_TAKE_INITIATOR || WEAVE_CONFIG_ENABLE_TAKE_RESPONDER

WEAVE_ERROR WeaveSecurityManager::CreateTAKESecureSession(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    err = HandleSessionEstablished();
    SuccessOrExit(err);

    mEC->KeyId = mSessionKeyId;
    mEC->EncryptionType = mEncType;

    // Add a reservation for the new session key and configure the ExchangeContext to automatically release
    // the key when the context is freed.  This will ensure the key is not removed until rest of the TAKE
    // exchange completes.
    ReserveKey(mEC->PeerNodeId, mEC->KeyId);
    mEC->SetAutoReleaseKey(true);

exit:
    return err;
}

WEAVE_ERROR WeaveSecurityManager::FinishTAKESetUp(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (mTAKEEngine->IsEncryptCommPhase())
    {
        err = HandleSessionEstablished();
        SuccessOrExit(err);
    }
    else
    {
        if (mTAKEEngine->IsEncryptAuthPhase())
        {
            err = FabricState->RemoveSessionKey(mSessionKeyId, mEC->PeerNodeId);
            SuccessOrExit(err);
        }
        mEncType = kWeaveEncryptionType_None;
        mSessionKeyId = WeaveKeyId::kNone;
    }

exit:
    return err;
}

#endif // WEAVE_CONFIG_ENABLE_TAKE_INITIATOR || WEAVE_CONFIG_ENABLE_TAKE_RESPONDER

#if WEAVE_CONFIG_ENABLE_KEY_EXPORT_INITIATOR

WEAVE_ERROR WeaveSecurityManager::StartKeyExport(WeaveConnection *con, uint64_t peerNodeId, const IPAddress &peerAddr, uint16_t peerPort,
        uint32_t keyId, bool signMessage, void *reqState,
        KeyExportCompleteFunct onComplete, KeyExportErrorFunct onError, WeaveKeyExportDelegate *keyExportDelegate)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // Verify we've been initialized and that we don't already have a session in process.
    if (State == kState_NotInitialized)
        return WEAVE_ERROR_INCORRECT_STATE;
    if (State != kState_Idle)
        return WEAVE_ERROR_SECURITY_MANAGER_BUSY;

    State = kState_KeyExportInProgress;

    mCon = con;

    // Create a new exchange context.
    err = NewSessionExchange(peerNodeId, peerAddr, peerPort);
    SuccessOrExit(err);

    // Initialize key export delegate.
    if (keyExportDelegate == NULL)
        keyExportDelegate = mDefaultKeyExportDelegate;

    // Initialize Weave Platform Memory.
    err = Platform::Security::MemoryInit();
    SuccessOrExit(err);

    // Allocate and initialize KeyExport object.
    mKeyExport = (WeaveKeyExport *)Platform::Security::MemoryAlloc(sizeof(WeaveKeyExport), true);
    VerifyOrExit(mKeyExport != NULL, err = WEAVE_ERROR_NO_MEMORY);
    mKeyExport->Init(keyExportDelegate);

    // Set the allowed key export protocol configurations.
    mKeyExport->SetAllowedConfigs(InitiatorAllowedKeyExportConfigs);

    // Send key export request message.
    err = SendKeyExportRequest(InitiatorKeyExportConfig, keyId, signMessage);
    SuccessOrExit(err);

    mStartKeyExport_OnComplete = onComplete;
    mStartKeyExport_OnError = onError;
    mStartKeyExport_ReqState = reqState;

    mEC->OnMessageReceived = HandleKeyExportMessageInitiator;
    mEC->OnConnectionClosed = HandleConnectionClosed;

    // Time limit overall Key Export duration.
    StartSessionTimer();

exit:
    if (err != WEAVE_NO_ERROR)
        HandleKeyExportError(err, NULL);

    return err;
}

void WeaveSecurityManager::HandleKeyExportMessageInitiator(ExchangeContext *ec, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo,
        uint32_t profileId, uint8_t msgType, PacketBuffer *msgBuf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveSecurityManager *secMgr = (WeaveSecurityManager *)ec->AppState;

    VerifyOrDie(ec == secMgr->mEC);

    // Abort the key export interaction immediately if we receive a status report message from the responder.
    // This is a signal that the responder does not want to continue.
    if (profileId == kWeaveProfile_Common && msgType == kMsgType_StatusReport)
    {
        ExitNow(err = WEAVE_ERROR_STATUS_REPORT_RECEIVED);
    }

    // All other messages must be part of the Security profile.
    VerifyOrExit(profileId == kWeaveProfile_Security, err = WEAVE_ERROR_INVALID_MESSAGE_TYPE);

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    // Flush any pending WRM ACKs before we begin the long crypto operation,
    // to prevent the peer from re-transmitting message.
    err = secMgr->mEC->WRMPFlushAcks();
    SuccessOrExit(err);
#endif

    // Handle the responder's message...
    switch (msgType)
    {
    case kMsgType_KeyExportReconfigure:
        uint8_t newConfig;

        err = secMgr->mKeyExport->ProcessKeyExportReconfigure(msgBuf->Start(), msgBuf->DataLength(), newConfig);
        SuccessOrExit(err);

        // Free the received message buffer so that it can be reused to send the outgoing message.
        PacketBuffer::Free(msgBuf);
        msgBuf = NULL;

        err = secMgr->SendKeyExportRequest(newConfig, secMgr->mKeyExport->KeyId(), secMgr->mKeyExport->SignMessages());
        SuccessOrExit(err);

        break;

    case kMsgType_KeyExportResponse:
        uint32_t exportedKeyId;
        uint16_t exportedKeyLen;
        uint8_t exportedKey[kWeaveFabricSecretSize];

        err = secMgr->mKeyExport->ProcessKeyExportResponse(msgBuf->Start(), msgBuf->DataLength(), msgInfo,
                                                           exportedKey, sizeof(exportedKey), exportedKeyLen, exportedKeyId);
        SuccessOrExit(err);

        // Call the user's completion function.
        if (secMgr->mStartKeyExport_OnComplete != NULL)
        {
            secMgr->mStartKeyExport_OnComplete(secMgr, secMgr->mCon, secMgr->mStartKeyExport_ReqState, exportedKeyId, exportedKey, exportedKeyLen);
        }

        // Reset state.
        secMgr->Reset();

        break;

    default:

        ExitNow(err = WEAVE_ERROR_INVALID_MESSAGE_TYPE);
        break;
    }

exit:
    if (err != WEAVE_NO_ERROR)
        secMgr->HandleKeyExportError(err, (err == WEAVE_ERROR_STATUS_REPORT_RECEIVED) ? msgBuf : NULL);

    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);
}

void WeaveSecurityManager::HandleKeyExportError(WEAVE_ERROR err, PacketBuffer *statusReportMsgBuf)
{
    // If session establishment in progress...
    //
    // NOTE: This check is necessary because it is possible for HandleKeyExportError() to be
    // called twice in certain situations: If a call to ExchangeContext::SendMessage() fails
    // because the underlying connection has been closed, it will trigger a callback, while
    // in the SendMessage() function, to HandleConnectionClosed() which calls this function.
    // Then when SendMessage() returns, the function that called it will also call this
    // function with the error returned by SendMessage().
    //
    if (State != kState_Idle)
    {
        WeaveConnection *con = mCon;
        KeyExportErrorFunct userOnError = mStartKeyExport_OnError;
        void *reqState = mStartKeyExport_ReqState;
        StatusReport rcvdStatusReport;
        StatusReport *statusReportPtr = NULL;

        // If a status report was received from the peer, parse it and arrange to pass it
        // to the callbacks.
        if (err == WEAVE_ERROR_STATUS_REPORT_RECEIVED)
        {
            WEAVE_ERROR parseErr = StatusReport::parse(statusReportMsgBuf, rcvdStatusReport);
            if (parseErr == WEAVE_NO_ERROR)
                statusReportPtr = &rcvdStatusReport;
            else
                err = parseErr;
        }

        // Reset state.
        Reset();

        // Call the user's error handler.
        if (userOnError != NULL)
            userOnError(this, con, reqState, err, statusReportPtr);
    }
}

__attribute__((noinline))
WEAVE_ERROR WeaveSecurityManager::SendKeyExportRequest(uint8_t keyExportConfig, uint32_t keyId, bool signMessage)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    PacketBuffer *msgBuf = NULL;
    uint16_t dataLen;
    uint16_t sendFlags = 0;

    // Allocate buffer.
    msgBuf = PacketBuffer::New();
    VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    // Generate key export request.
    err = mKeyExport->GenerateKeyExportRequest(msgBuf->Start(), msgBuf->AvailableDataLength(), dataLen, keyExportConfig, keyId, signMessage);
    SuccessOrExit(err);

    // Set message length.
    msgBuf->SetDataLength(dataLen);

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    if (mCon == NULL)
    {
        sendFlags = ExchangeContext::kSendFlag_RequestAck;
    }
#endif

    // Send key export request message.
    err = mEC->SendMessage(kWeaveProfile_Security, kMsgType_KeyExportRequest, msgBuf, sendFlags);
    msgBuf = NULL;
    SuccessOrExit(err);

exit:
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);

    return err;
}

#endif // WEAVE_CONFIG_ENABLE_KEY_EXPORT_INITIATOR

#if WEAVE_CONFIG_ENABLE_KEY_EXPORT_RESPONDER

void WeaveSecurityManager::HandleKeyExportRequest(ExchangeContext *ec, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo, PacketBuffer *msgBuf)
{
    WEAVE_ERROR err;
    WeaveKeyExport keyExport;

    State = kState_KeyExportInProgress;
    mEC = ec;
    mCon = ec->Con;

    // Ensure the exchange context stays around until we're done with it.
    ec->AddRef();

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    if (mCon == NULL)
    {
        // Do nothing on the Ack received from the requestor.
        // mEC->OnAckRcvd is not initialized.
        // Do nothing on the message send error.
        // mEC->OnSendError is not initialized.

        // Flush any pending WRM ACKs before we begin the long crypto operation,
        // to prevent the peer from re-transmitting the Key Export request.
        err = mEC->WRMPFlushAcks();
        SuccessOrExit(err);
    }
#endif

    // Initialize Weave Platform Memory.
    err = Platform::Security::MemoryInit();
    SuccessOrExit(err);

    // Prepare key export engine.
    keyExport.Init(mDefaultKeyExportDelegate, FabricState->GroupKeyStore);

    // Set the allowed key export protocol configurations.
    keyExport.SetAllowedConfigs(ResponderAllowedKeyExportConfigs);

    // Process key export request message.
    err = keyExport.ProcessKeyExportRequest(msgBuf->Start(), msgBuf->DataLength(), msgInfo);

    // Free the received message buffer so that it can be reused to send the outgoing message.
    PacketBuffer::Free(msgBuf);
    msgBuf = NULL;

    // Check if reconfiguration was requested.
    if (err == WEAVE_ERROR_KEY_EXPORT_RECONFIGURE_REQUIRED)
    {
        err = SendKeyExportResponse(keyExport, kMsgType_KeyExportReconfigure, msgInfo);
    }
    else if (err == WEAVE_NO_ERROR)
    {
        err = SendKeyExportResponse(keyExport, kMsgType_KeyExportResponse, msgInfo);
    }
    SuccessOrExit(err);

exit:
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);

    // Send status report message in case of an error.
    if (err != WEAVE_NO_ERROR)
        SendStatusReport(err, ec);

    // Shutdown and clean key export engine memory.
    keyExport.Shutdown();

    // Reset state.
    Reset();
}

__attribute__((noinline))
WEAVE_ERROR WeaveSecurityManager::SendKeyExportResponse(WeaveKeyExport& keyExport, uint8_t msgType, const WeaveMessageInfo *msgInfo)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    PacketBuffer *msgBuf = NULL;
    uint16_t dataLen;
    uint16_t sendFlags = 0;

    // Allocate buffer.
    msgBuf = PacketBuffer::New();
    VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    // Generate key export reconfigure or response message.
    if (msgType == kMsgType_KeyExportReconfigure)
        err = keyExport.GenerateKeyExportReconfigure(msgBuf->Start(), msgBuf->AvailableDataLength(), dataLen);
    else if (msgType == kMsgType_KeyExportResponse)
        err = keyExport.GenerateKeyExportResponse(msgBuf->Start(), msgBuf->AvailableDataLength(), dataLen, msgInfo);
    else
        err = WEAVE_ERROR_INVALID_MESSAGE_TYPE;
    SuccessOrExit(err);

    // Set message length.
    msgBuf->SetDataLength(dataLen);

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    if (mCon == NULL)
    {
        sendFlags = ExchangeContext::kSendFlag_RequestAck;
    }
#endif

    // Send key export response message.
    err = mEC->SendMessage(kWeaveProfile_Security, msgType, msgBuf, sendFlags);
    msgBuf = NULL;
    SuccessOrExit(err);

exit:
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);

    return err;
}

#endif // WEAVE_CONFIG_ENABLE_KEY_EXPORT_RESPONDER

/**
 * Checks if the specified Weave error code is one of the key error codes.
 * This function is called to determine whether key error message should be sent
 * to the initiator of the message that failed to find a correct key during decoding.
 *
 * @param[in]  err        A Weave error code.
 *
 * @retval     true       If specified Weave error code is a key error.
 * @retval     false      Otherwise.
 *
 */
bool WeaveSecurityManager::IsKeyError(WEAVE_ERROR err)
{
    return (err == WEAVE_ERROR_KEY_NOT_FOUND ||
            err == WEAVE_ERROR_WRONG_ENCRYPTION_TYPE ||
            err == WEAVE_ERROR_UNKNOWN_KEY_TYPE ||
            err == WEAVE_ERROR_INVALID_USE_OF_SESSION_KEY ||
            err == WEAVE_ERROR_UNSUPPORTED_ENCRYPTION_TYPE);
}

/**
 * Send key error message.
 * This function is called when received Weave message decoding fails due to key error.
 *
 * @param[in]  rcvdMsgInfo        A pointer to the message information for the received Weave message.
 *
 * @param[in]  rcvdMsgPacketInfo  A pointer to the IPPacketInfo object of the received Weave message.
 *
 * @param[in]  con                A pointer to the WeaveConnection object.
 *
 * @param[in]  keyErr             Weave key error code.
 *
 * @retval  #WEAVE_ERROR_NO_MEMORY         If memory could not be allocated for the new
 *                                         exchange context or new message buffer.
 * @retval  #WEAVE_ERROR_BUFFER_TOO_SMALL  If buffer is too small
 * @retval  #WEAVE_NO_ERROR                If the method succeeded.
 *
 */
WEAVE_ERROR WeaveSecurityManager::SendKeyErrorMsg(WeaveMessageInfo *rcvdMsgInfo, const IPPacketInfo *rcvdMsgPacketInfo, WeaveConnection *con, WEAVE_ERROR keyErr)
{
    WEAVE_ERROR         err         = WEAVE_NO_ERROR;
    ExchangeContext*    ec          = NULL;
    PacketBuffer*       msgBuf      = NULL;
    uint8_t*            p;
    uint16_t            keyErrCode;

    // Create new exchange context.
    if (con == NULL)
    {
        VerifyOrExit(rcvdMsgPacketInfo != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);
        ec = ExchangeManager->NewContext(rcvdMsgInfo->SourceNodeId, rcvdMsgPacketInfo->SrcAddress, rcvdMsgPacketInfo->SrcPort, rcvdMsgPacketInfo->Interface, this);
    }
    else
    {
        ec = ExchangeManager->NewContext(con, this);
    }
    VerifyOrExit(ec != NULL, err = WEAVE_ERROR_NO_MEMORY);

    // Encode key error status code.
    switch (keyErr)
    {
    case WEAVE_ERROR_KEY_NOT_FOUND:
        keyErrCode = kStatusCode_KeyNotFound;
        break;
    case WEAVE_ERROR_WRONG_ENCRYPTION_TYPE:
        keyErrCode = kStatusCode_WrongEncryptionType;
        break;
    case WEAVE_ERROR_UNKNOWN_KEY_TYPE:
        keyErrCode = kStatusCode_UnknownKeyType;
        break;
    case WEAVE_ERROR_INVALID_USE_OF_SESSION_KEY:
        keyErrCode = kStatusCode_InvalidUseOfSessionKey;
        break;
    case WEAVE_ERROR_UNSUPPORTED_ENCRYPTION_TYPE:
        keyErrCode = kStatusCode_UnsupportedEncryptionType;
        break;
    default:
        keyErrCode = kStatusCode_InternalKeyError;
        break;
    }

    // Allocate new buffer.
    msgBuf = PacketBuffer::New();
    VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    // Verify that the buffer is big enough for this message.
    VerifyOrExit(msgBuf->AvailableDataLength() >= kWeaveKeyErrorMessageSize, err = WEAVE_ERROR_BUFFER_TOO_SMALL);

    // Initialize pointer to the buffer payload start.
    p = msgBuf->Start();

    // Write the key Id field (2 bytes).
    LittleEndian::Write16(p, rcvdMsgInfo->KeyId);

    // Write the encryption type field (1 byte).
    Write8(p, rcvdMsgInfo->EncryptionType);

    // Write the message Id field (4 bytes).
    LittleEndian::Write32(p, rcvdMsgInfo->MessageId);

    // Write the key error code field (2 bytes).
    LittleEndian::Write16(p, keyErrCode);

    // Set message length.
    msgBuf->SetDataLength(kWeaveKeyErrorMessageSize);

    // Send key error message.
    err = ec->SendMessage(kWeaveProfile_Security, kMsgType_KeyError, msgBuf, 0);
    msgBuf = NULL;

exit:
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);

    if (ec != NULL)
        ec->Close();

    return err;
}

void WeaveSecurityManager::HandleKeyErrorMsg(ExchangeContext *ec, PacketBuffer* msgBuf)
{
    WEAVE_ERROR err;
    WeaveSessionKey *sessionKey;
    uint8_t *p = msgBuf->Start();
    uint64_t srcNodeId = ec->PeerNodeId;
    uint16_t keyId;
    uint8_t encType;
    uint16_t keyErrCode;
    uint32_t messageId;
    WEAVE_ERROR keyErr;
    uint64_t endNodeIds[WEAVE_CONFIG_MAX_END_NODES_PER_SHARED_SESSION + 1];
    uint8_t endNodeIdsCount = 0;

    // Verify correct message size.
    if (msgBuf->DataLength() != kWeaveKeyErrorMessageSize)
        ExitNow();

    // Read the message fields.
    keyId = LittleEndian::Read16(p);
    encType = Read8(p);
    messageId = LittleEndian::Read32(p);
    keyErrCode = LittleEndian::Read16(p);

    // Free the received message buffer so that it can be reused to initiate additional communication.
    PacketBuffer::Free(msgBuf);
    msgBuf = NULL;

    // Close exchange context that delivered key error message.
    ec->Close();
    ec = NULL;

    // Decode error status code.
    switch (keyErrCode)
    {
    case kStatusCode_KeyNotFound:
        keyErr = WEAVE_ERROR_KEY_NOT_FOUND_FROM_PEER;
        break;
    case kStatusCode_WrongEncryptionType:
        keyErr = WEAVE_ERROR_WRONG_ENCRYPTION_TYPE_FROM_PEER;
        break;
    case kStatusCode_UnknownKeyType:
        keyErr = WEAVE_ERROR_UNKNOWN_KEY_TYPE_FROM_PEER;
        break;
    case kStatusCode_InvalidUseOfSessionKey:
        keyErr = WEAVE_ERROR_INVALID_USE_OF_SESSION_KEY_FROM_PEER;
        break;
    case kStatusCode_UnsupportedEncryptionType:
        keyErr = WEAVE_ERROR_UNSUPPORTED_ENCRYPTION_TYPE_FROM_PEER;
        break;
    default:
        keyErr = WEAVE_ERROR_INTERNAL_KEY_ERROR_FROM_PEER;
        break;
    }

    // If the failed key is a session key...
    if (WeaveKeyId::IsSessionKey(keyId))
    {
        // Attempt to find the referenced session key.  If found...
        err = FabricState->FindSessionKey(keyId, srcNodeId, false, sessionKey);
        if (err == WEAVE_NO_ERROR)
        {
            // If the key refers to a shared session, add the shared session end nodes to the list
            // of peer nodes associated with the key.
            if (sessionKey->IsSharedSession())
            {
                FabricState->GetSharedSessionEndNodeIds(sessionKey, endNodeIds, sizeof(endNodeIds) / sizeof(uint64_t), endNodeIdsCount);
            }

            // Add the terminating node to the list of peer nodes associated with the key.
            endNodeIds[endNodeIdsCount++] = sessionKey->NodeId;

            // Discard the failed session key.
            FabricState->RemoveSessionKey(keyId, srcNodeId);
        }
    }

    // Otherwise, the key is some other form of key (e.g. a group key), so add the node that sent the
    // key error to the list of peers associated with the key.
    else
    {
        endNodeIds[endNodeIdsCount++] = srcNodeId;
    }

    // For each peer node associated with the key, notify the exchange manager that the key has failed
    // with respect to that peer.
    for (int i = 0; i < endNodeIdsCount; i++)
        ExchangeManager->NotifyKeyFailed(endNodeIds[i], keyId, keyErr);

    // TODO: fail the current in-progress session if the key error identifies the proposed key.

    // Call the general key error message handler.
    if (OnKeyErrorMsgRcvd != NULL)
        OnKeyErrorMsgRcvd(keyId, encType, messageId, srcNodeId, keyErr);

exit:
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);

    if (ec != NULL)
        ec->Close();

    return;
}

WEAVE_ERROR WeaveSecurityManager::NewSessionExchange(uint64_t peerNodeId, IPAddress peerAddr, uint16_t peerPort)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (mEC != NULL)
    {
        mEC->Close();
        mEC = NULL;
    }

    // Create a new exchange context.
    if (mCon)
    {
        mEC = ExchangeManager->NewContext(mCon, this);
        VerifyOrExit(mEC != NULL, err = WEAVE_ERROR_NO_MEMORY);
    }
    else
    {
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
        VerifyOrExit(peerNodeId != kNodeIdNotSpecified && peerNodeId != kAnyNodeId, err = WEAVE_ERROR_INVALID_ARGUMENT);

        mEC = ExchangeManager->NewContext(peerNodeId, peerAddr, peerPort, INET_NULL_INTERFACEID, this);
        VerifyOrExit(mEC != NULL, err = WEAVE_ERROR_NO_MEMORY);

        mEC->OnAckRcvd = WRMPHandleAckRcvd;
        mEC->OnSendError = WRMPHandleSendError;
#else
        // Reject the request if no connection has been specified.
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
#endif
    }

exit:
    return err;
}

#if WEAVE_CONFIG_USE_APP_GROUP_KEYS_FOR_MSG_ENC

// Create and initialize new exchange for the message counter synchronization request/response messages.
WEAVE_ERROR WeaveSecurityManager::NewMsgCounterSyncExchange(const WeaveMessageInfo *rcvdMsgInfo, const IPPacketInfo *rcvdMsgPacketInfo, ExchangeContext *& ec)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // Verify input arguments.
    VerifyOrExit(rcvdMsgInfo != NULL && rcvdMsgPacketInfo != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Message counter synchronization protocol is only applicable for application group keys.
    VerifyOrExit(WeaveKeyId::IsAppGroupKey(rcvdMsgInfo->KeyId), err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Create new exchange context.
    ec = ExchangeManager->NewContext(rcvdMsgInfo->SourceNodeId, rcvdMsgPacketInfo->SrcAddress, rcvdMsgPacketInfo->SrcPort, rcvdMsgPacketInfo->Interface, this);
    VerifyOrExit(ec != NULL, err = WEAVE_ERROR_NO_MEMORY);

    // Set encryption type and key Id.
    ec->EncryptionType = rcvdMsgInfo->EncryptionType;
    ec->KeyId = rcvdMsgInfo->KeyId;

exit:
    return err;
}

/**
 * Send peer message counter synchronization request.
 * This function is called while processing a message encrypted with an application key from a peer whose message counter is not synchronized.
 * This message is sent on a newly created exchange, which is closed immediately after.
 *
 * @param[in]  rcvdMsgInfo        A pointer to the message information for the received Weave message.
 *
 * @param[in]  rcvdMsgPacketInfo  A pointer to the IPPacketInfo object of the received Weave message.
 *
 * @retval  #WEAVE_ERROR_INVALID_ARGUMENT  If keyId input has invalid value.
 * @retval  #WEAVE_ERROR_NO_MEMORY         If memory could not be allocated for the new
 *                                         exchange context or new message buffer.
 * @retval  #WEAVE_NO_ERROR                On success.
 *
 */
WEAVE_ERROR WeaveSecurityManager::SendSolitaryMsgCounterSyncReq(const WeaveMessageInfo *rcvdMsgInfo, const IPPacketInfo *rcvdMsgPacketInfo)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    ExchangeContext *ec = NULL;

    // Create and initialize new exchange.
    err = NewMsgCounterSyncExchange(rcvdMsgInfo, rcvdMsgPacketInfo, ec);
    SuccessOrExit(err);

    // Send the message counter synchronization request in a Common::Null message.
    err = ec->SendCommonNullMessage();
    SuccessOrExit(err);

exit:
    if (ec != NULL)
        ec->Close();

    return err;
}

/**
 * Send message counter synchronization response message.
 * This function is called when Weave node receives message with message counter synchronization request flag.
 * This message is sent on a newly created exchange, which is closed immediately after.
 *
 * @param[in]  rcvdMsgInfo        A pointer to the message information for the received Weave message.
 *
 * @param[in]  rcvdMsgPacketInfo  A pointer to the IPPacketInfo object of the received Weave message.
 *
 * @retval  #WEAVE_ERROR_INVALID_ARGUMENT  If received message with message counter synchronization
 *                                         request was unencrypted.
 * @retval  #WEAVE_ERROR_NO_MEMORY         If memory could not be allocated for the new
 *                                         exchange context or new message buffer.
 * @retval  #WEAVE_ERROR_BUFFER_TOO_SMALL  If allocated message buffer is too small.
 * @retval  #WEAVE_NO_ERROR                On success.
 *
 */
WEAVE_ERROR WeaveSecurityManager::SendMsgCounterSyncResp(const WeaveMessageInfo *rcvdMsgInfo, const IPPacketInfo *rcvdMsgPacketInfo)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    ExchangeContext *ec = NULL;
    PacketBuffer *msgBuf = NULL;

    // Create and initialize new exchange.
    err = NewMsgCounterSyncExchange(rcvdMsgInfo, rcvdMsgPacketInfo, ec);
    SuccessOrExit(err);

    // Allocate new buffer.
    msgBuf = PacketBuffer::New();
    VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    // Verify that the buffer is big enough for this message.
    VerifyOrExit(msgBuf->AvailableDataLength() >= kWeaveMsgCounterSyncRespMsgSize, err = WEAVE_ERROR_BUFFER_TOO_SMALL);

    // Write the requester message id (counter) field.
    LittleEndian::Put32(msgBuf->Start(), rcvdMsgInfo->MessageId);

    // Set message length.
    msgBuf->SetDataLength(kWeaveMsgCounterSyncRespMsgSize);

    // Send message counter synchronization response message.
    err = ec->SendMessage(kWeaveProfile_Security, kMsgType_MsgCounterSyncResp, msgBuf);
    msgBuf = NULL;

exit:
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);

    if (ec != NULL)
        ec->Close();

    return err;
}

/**
 * Handle message counter synchronization response message.
 *
 * @param[in]  msgInfo        A pointer to the message information for the received Weave message.
 *
 * @param[in]  msgBuf         A pointer to the PacketBuffer object holding the received Weave message.
 *
 * @retval None.
 *
 */
void WeaveSecurityManager::HandleMsgCounterSyncRespMsg(WeaveMessageInfo *msgInfo, PacketBuffer *msgBuf)
{
    // Verify correct message size and that the message was encrypted with application group key.
    VerifyOrExit(msgBuf->DataLength() == kWeaveMsgCounterSyncRespMsgSize && WeaveKeyId::IsAppGroupKey(msgInfo->KeyId), /* no-op */);

    // Initialize/synchronize peer's message counter.
    FabricState->OnMsgCounterSyncRespRcvd(msgInfo->SourceNodeId, msgInfo->MessageId, LittleEndian::Get32(msgBuf->Start()));

exit:
    PacketBuffer::Free(msgBuf);

    return;
}

#endif // WEAVE_CONFIG_USE_APP_GROUP_KEYS_FOR_MSG_ENC

WEAVE_ERROR WeaveSecurityManager::HandleSessionEstablished(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint64_t peerNodeId = mEC->PeerNodeId;
    uint16_t sessionKeyId = mSessionKeyId;
    uint8_t encType = mEncType;
    const WeaveEncryptionKey *sessionKey;
    WeaveAuthMode authMode;

    switch (State)
    {
#if WEAVE_CONFIG_ENABLE_CASE_INITIATOR || WEAVE_CONFIG_ENABLE_CASE_RESPONDER
    case kState_CASEInProgress:

        // Get the derived session key.
        err = mCASEEngine->GetSessionKey(sessionKey);
        SuccessOrExit(err);

        // Form the key auth mode based on the type of certificate that was used by the peer.
        //
        // Note that, in the initiator case, the key mode may be different than the auth mode that
        // was requested by the application.  For example, if the app requested kWeaveAuthMode_CASE_AnyCert
        // then the final key auth mode will reflect the actual certificate type used by the peer.
        //
        authMode = CASEAuthMode(mCASEEngine->CertType());

        break;
#endif

#if WEAVE_CONFIG_ENABLE_PASE_INITIATOR || WEAVE_CONFIG_ENABLE_PASE_RESPONDER
    case kState_PASEInProgress:

        // Get the derived session key.
        err = mPASEEngine->GetSessionKey(sessionKey);
        SuccessOrExit(err);

        // Form the key auth mode based on the password source.
        authMode = PASEAuthMode(mPASEEngine->PwSource);

        break;
#endif

#if WEAVE_CONFIG_ENABLE_TAKE_INITIATOR || WEAVE_CONFIG_ENABLE_TAKE_RESPONDER
    case kState_TAKEInProgress:

        // Get the derived session key.
        err = mTAKEEngine->GetSessionKey(sessionKey);
        SuccessOrExit(err);

        // Currently only one key auth mode is supported for TAKE.
        authMode = kWeaveAuthMode_TAKE_IdentificationKey;

        break;
#endif

    default:
        ExitNow(err = WEAVE_ERROR_INCORRECT_STATE);
    }

    // Save the session key into the session key table.
    err = FabricState->SetSessionKey(sessionKeyId, peerNodeId, encType, authMode, sessionKey);
    SuccessOrExit(err);

exit:
    return err;
}

void WeaveSecurityManager::HandleSessionComplete(void)
{
    WeaveConnection *con = mCon;
    uint64_t peerNodeId = mEC->PeerNodeId;
    uint16_t sessionKeyId = mSessionKeyId;
    uint8_t encType = mEncType;
    SessionEstablishedFunct userOnComplete = mStartSecureSession_OnComplete;
    void *reqState = mStartSecureSession_ReqState;

    // Reset state.
    Reset();

    // Call the general session established handler.
    if (OnSessionEstablished != NULL)
        OnSessionEstablished(this, con, NULL, sessionKeyId, peerNodeId, encType);

    // Call the user's completion function.
    if (userOnComplete != NULL)
        userOnComplete(this, con, reqState, sessionKeyId, peerNodeId, encType);

    // If the session was initiated the remote party, release the reservation that was
    // made when the session key record was allocated.  Provided that the application
    // hasn't increased the reservation count during one of the above callbacks,
    // this will result in the reservation count going to zero, which will make
    // eligible for removal if it remains idle past the idle timeout period.
    {
        WeaveSessionKey *sessionKey;
        if (FabricState->FindSessionKey(sessionKeyId, peerNodeId, false, sessionKey) == WEAVE_NO_ERROR &&
            !sessionKey->IsLocallyInitiated())
        {
            ReleaseSessionKey(sessionKey);
        }
    }

    // Asynchronously notify other subsystems that the security manager is now available
    // for initiating additional sessions.
    AsyncNotifySecurityManagerAvailable();
}

void WeaveSecurityManager::HandleSessionError(WEAVE_ERROR err, PacketBuffer* statusReportMsgBuf)
{
    // If session establishment in progress...
    //
    // NOTE: This check is necessary because it is possible for HandleSessionError() to be
    // called twice in certain situations: If a call to ExchangeContext::SendMessage() fails
    // because the underlying connection has been closed, it will trigger a callback, while
    // in the SendMessage() function, to HandleConnectionClosed() which calls this function.
    // Then when SendMessage() returns, the function that called it will also call this
    // function with the error returned by SendMessage().
    //
    if (State != kState_Idle)
    {
        WeaveConnection *con = mCon;
        uint64_t peerNodeId = mEC->PeerNodeId;
        uint16_t sessionKeyId = mSessionKeyId;
        SessionErrorFunct userOnError = mStartSecureSession_OnError;
        void *reqState = mStartSecureSession_ReqState;
        StatusReport rcvdStatusReport;
        StatusReport *statusReportPtr = NULL;

        // If a status report was received from the peer, parse it and arrange to pass it
        // to the callbacks.
        if (err == WEAVE_ERROR_STATUS_REPORT_RECEIVED)
        {
            WEAVE_ERROR parseErr = StatusReport::parse(statusReportMsgBuf, rcvdStatusReport);
            if (parseErr == WEAVE_NO_ERROR)
                statusReportPtr = &rcvdStatusReport;
            else
                err = parseErr;
        }

        // Otherwise, send a status report to the peer with our reason for the failure.
        else
            SendStatusReport(err, mEC);

        // Remove the session key from the key table.
        FabricState->RemoveSessionKey(sessionKeyId, peerNodeId);

        // Reset state.
        Reset();

        // Call the general session error handler.
        if (OnSessionError != NULL)
            OnSessionError(this, con, NULL, err, peerNodeId, statusReportPtr);

        // Call the user's error handler.
        if (userOnError != NULL)
            userOnError(this, con, reqState, err, peerNodeId, statusReportPtr);

        // Asynchronously notify other subsystems that the security manager is now available
        // for initiating another session.
        AsyncNotifySecurityManagerAvailable();
    }
}

void WeaveSecurityManager::HandleConnectionClosed(ExchangeContext *ec, WeaveConnection *con, WEAVE_ERROR conErr)
{
    WeaveSecurityManager *secMgr = (WeaveSecurityManager *)ec->AppState;

    if (conErr == WEAVE_NO_ERROR)
        conErr = WEAVE_ERROR_CONNECTION_CLOSED_UNEXPECTEDLY;

    // Clean-up the local state and invoke the appropriate callbacks.
#if WEAVE_CONFIG_ENABLE_KEY_EXPORT_INITIATOR
    if (secMgr->State == kState_KeyExportInProgress)
        secMgr->HandleKeyExportError(conErr, NULL);
    else
#endif
        secMgr->HandleSessionError(conErr, NULL);
}

WEAVE_ERROR WeaveSecurityManager::SendStatusReport(WEAVE_ERROR localErr, ExchangeContext *ec)
{
    WEAVE_ERROR     err;
    uint32_t        profileId;
    uint16_t        statusCode;
    uint16_t        sendFlags;

    // Verify that specified exchange isn't closed.
    VerifyOrExit(ec != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    if (ec->Con != NULL)
    {
        // Verify that underlying connection isn't closed.
        VerifyOrExit(!ec->IsConnectionClosed(), err = WEAVE_ERROR_INVALID_ARGUMENT);
        sendFlags = 0;
    }
    else
    {
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
        sendFlags = ExchangeContext::kSendFlag_RequestAck;
#else
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
#endif
    }

    // TODO: map CASE errors

    switch (localErr)
    {
    case WEAVE_ERROR_INCORRECT_STATE:
    case WEAVE_ERROR_INVALID_MESSAGE_TYPE:
        profileId = kWeaveProfile_Common;
        statusCode = kStatus_UnexpectedMessage;
        break;
    case WEAVE_ERROR_NOT_IMPLEMENTED:
        profileId = kWeaveProfile_Common;
        statusCode = kStatus_UnsupportedMessage;
        break;
    case WEAVE_ERROR_SECURITY_MANAGER_BUSY:
    case WEAVE_ERROR_RATE_LIMIT_EXCEEDED:
        profileId = kWeaveProfile_Common;
        statusCode = kStatus_Busy;
        break;
    case WEAVE_ERROR_TIMEOUT:
        profileId = kWeaveProfile_Common;
        statusCode = kStatus_Timeout;
        break;
    case WEAVE_ERROR_UNSUPPORTED_ENCRYPTION_TYPE:
        profileId = kWeaveProfile_Security;
        statusCode = kStatusCode_UnsupportedEncryptionType;
        break;
    case WEAVE_ERROR_WRONG_KEY_TYPE:
        profileId = kWeaveProfile_Security;
        statusCode = kStatusCode_InvalidKeyId;
        break;
    case WEAVE_ERROR_DUPLICATE_KEY_ID:
        profileId = kWeaveProfile_Security;
        statusCode = kStatusCode_DuplicateKeyId;
        break;
    case WEAVE_ERROR_KEY_CONFIRMATION_FAILED:
        profileId = kWeaveProfile_Security;
        statusCode = kStatusCode_KeyConfirmationFailed;
        break;
    case WEAVE_ERROR_INVALID_PASE_PARAMETER:
    case WEAVE_ERROR_CERT_USAGE_NOT_ALLOWED:
    case WEAVE_ERROR_CERT_PATH_LEN_CONSTRAINT_EXCEEDED:
    case WEAVE_ERROR_CERT_NOT_VALID_YET:
    case WEAVE_ERROR_CERT_EXPIRED:
    case WEAVE_ERROR_CERT_PATH_TOO_LONG:
    case WEAVE_ERROR_CA_CERT_NOT_FOUND:
    case WEAVE_ERROR_INVALID_SIGNATURE:
    case WEAVE_ERROR_CERT_NOT_TRUSTED:
    case WEAVE_ERROR_WRONG_CERT_SUBJECT:
    case WEAVE_ERROR_WRONG_CERT_TYPE:
        profileId = kWeaveProfile_Security;
        statusCode = kStatusCode_AuthenticationFailed;
        break;
    case WEAVE_ERROR_PASE_SUPPORTS_ONLY_CONFIG1:
        profileId = kWeaveProfile_Security;
        statusCode = kStatusCode_PASESupportsOnlyConfig1;
        break;
    case WEAVE_ERROR_NO_COMMON_PASE_CONFIGURATIONS:
        profileId = kWeaveProfile_Security;
        statusCode = kStatusCode_NoCommonPASEConfigurations;
        break;
    case WEAVE_ERROR_UNSUPPORTED_CASE_CONFIGURATION:
        profileId = kWeaveProfile_Security;
        statusCode = kStatusCode_UnsupportedCASEConfiguration;
        break;
    case WEAVE_ERROR_UNSUPPORTED_CERT_FORMAT:
        profileId = kWeaveProfile_Security;
        statusCode = kStatusCode_UnsupportedCertificate;
        break;
#if WEAVE_CONFIG_ENABLE_KEY_EXPORT_RESPONDER
    case WEAVE_ERROR_NO_COMMON_KEY_EXPORT_CONFIGURATIONS:
        profileId = kWeaveProfile_Security;
        statusCode = kStatusCode_NoCommonKeyExportConfiguration;
        break;
    case WEAVE_ERROR_UNAUTHORIZED_KEY_EXPORT_REQUEST:
        profileId = kWeaveProfile_Security;
        statusCode = kStatusCode_UnathorizedKeyExportRequest;
        break;
#endif // WEAVE_CONFIG_ENABLE_KEY_EXPORT_RESPONDER
    default:
        WeaveLogError(SecurityManager, "Internal security error %d", localErr);
        profileId = kWeaveProfile_Security;
        statusCode = kStatusCode_InternalError;
        break;
    }

    // TODO: add support for conveying system error
    // (HOWEVER, be careful to suppress error information that should not be revealed to the peer).

    err = nl::Weave::WeaveServerBase::SendStatusReport(ec, profileId, statusCode, WEAVE_NO_ERROR, sendFlags);
    SuccessOrExit(err);

exit:
    return err;
}

void WeaveSecurityManager::Reset(void)
{
    if (mEC != NULL)
    {
        mEC->Abort();
        mEC = NULL;
    }

    switch (State)
    {
#if WEAVE_CONFIG_ENABLE_PASE_INITIATOR || WEAVE_CONFIG_ENABLE_PASE_RESPONDER
    case kState_PASEInProgress:
        if (mPASEEngine != NULL)
        {
            mPASEEngine->Shutdown();
            Platform::Security::MemoryFree(mPASEEngine);
            mPASEEngine = NULL;
        }
        break;
#endif
#if WEAVE_CONFIG_ENABLE_TAKE_INITIATOR || WEAVE_CONFIG_ENABLE_TAKE_RESPONDER
    case kState_TAKEInProgress:
        if (mTAKEEngine != NULL)
        {
            mTAKEEngine->Shutdown();
            Platform::Security::MemoryFree(mTAKEEngine);
            mTAKEEngine = NULL;
        }
        break;
#endif
#if WEAVE_CONFIG_ENABLE_CASE_INITIATOR || WEAVE_CONFIG_ENABLE_CASE_RESPONDER
    case kState_CASEInProgress:
        if (mCASEEngine != NULL)
        {
            mCASEEngine->Shutdown();
            Platform::Security::MemoryFree(mCASEEngine);
            mCASEEngine = NULL;
        }
        break;
#endif
#if WEAVE_CONFIG_ENABLE_KEY_EXPORT_INITIATOR
    case kState_KeyExportInProgress:
        if (mKeyExport != NULL)
        {
            mKeyExport->Shutdown();
            Platform::Security::MemoryFree(mKeyExport);
            mKeyExport = NULL;
        }
        break;
#endif
    default:
        break;
    }

    Platform::Security::MemoryShutdown();

    CancelSessionTimer();

    State = kState_Idle;
    mCon = NULL;
    mRequestedAuthMode = kWeaveAuthMode_NotSpecified;
    mSessionKeyId = WeaveKeyId::kNone;
    mEncType = kWeaveEncryptionType_None;
    mStartSecureSession_OnComplete = NULL;
    mStartSecureSession_OnError = NULL;
    mStartSecureSession_ReqState = NULL;
}

void WeaveSecurityManager::StartSessionTimer(void)
{
    WeaveLogProgress(SecurityManager, "%s", __FUNCTION__);

    if (SessionEstablishTimeout != 0)
    {
        mSystemLayer->StartTimer(SessionEstablishTimeout, HandleSessionTimeout, this);
    }
}

void WeaveSecurityManager::CancelSessionTimer(void)
{
    WeaveLogProgress(SecurityManager, "%s", __FUNCTION__);
    mSystemLayer->CancelTimer(HandleSessionTimeout, this);
}

void WeaveSecurityManager::HandleSessionTimeout(System::Layer* aSystemLayer, void* aAppState, System::Error aError)
{
    WeaveLogProgress(SecurityManager, "%s", __FUNCTION__);

    WeaveSecurityManager* securityMgr = reinterpret_cast<WeaveSecurityManager*>(aAppState);
    if (securityMgr)
    {
        securityMgr->HandleSessionError(WEAVE_ERROR_TIMEOUT, NULL);
    }
}

void WeaveSecurityManager::StartIdleSessionTimer(void)
{
    if (IdleSessionTimeout != 0 && !GetFlag(mFlags, kFlag_IdleSessionTimerRunning))
    {
        System::Layer *systemLayer = FabricState->MessageLayer->SystemLayer;
        System::Error err = systemLayer->StartTimer(IdleSessionTimeout, HandleIdleSessionTimeout, this);
        if (err == WEAVE_SYSTEM_NO_ERROR)
        {
            WeaveLogDetail(SecurityManager, "Session idle timer started");
            SetFlag(mFlags, kFlag_IdleSessionTimerRunning);
        }
    }
}

void WeaveSecurityManager::StopIdleSessionTimer(void)
{
    System::Layer *systemLayer = FabricState->MessageLayer->SystemLayer;
    systemLayer->CancelTimer(HandleIdleSessionTimeout, this);
    ClearFlag(mFlags, kFlag_IdleSessionTimerRunning);
    WeaveLogDetail(SecurityManager, "Session idle timer stopped");
}

void WeaveSecurityManager::HandleIdleSessionTimeout(System::Layer* aLayer, void* aAppState, System::Error aError)
{
    WeaveSecurityManager *_this = (WeaveSecurityManager *)aAppState;
    bool unreservedSessionsExist;

    ClearFlag(_this->mFlags, kFlag_IdleSessionTimerRunning);

    unreservedSessionsExist = _this->FabricState->RemoveIdleSessionKeys();

    if (unreservedSessionsExist)
    {
        _this->StartIdleSessionTimer();
    }
}

void WeaveSecurityManager::OnEncryptedMsgRcvd(uint16_t sessionKeyId, uint64_t peerNodeId, uint8_t encType)
{
    // Check if corresponding secure session is established but not completed - if true then complete this session.
    // This function is needed in WRMP case when first message from the peer encrypted with sessionKeyId
    // is received before the Ack for the last message on the session establishment exchange.
    // In that case there is no need to wait for the Ack and the session can be completed.
#if WEAVE_CONFIG_ENABLE_CASE_INITIATOR || WEAVE_CONFIG_ENABLE_CASE_RESPONDER
    if (State == kState_CASEInProgress &&
        mCASEEngine->State == WeaveCASEEngine::kState_Complete &&
        mSessionKeyId == sessionKeyId &&
        mEC->PeerNodeId == peerNodeId &&
        mEncType == encType)
    {
        HandleSessionComplete();
    }
#endif
}

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING

void WeaveSecurityManager::WRMPHandleAckRcvd(ExchangeContext *ec, void *msgCtxt)
{
    WeaveLogProgress(SecurityManager, "%s", __FUNCTION__);
    WeaveSecurityManager *secMgr = (WeaveSecurityManager *)ec->AppState;

    if (secMgr->State == kState_CASEInProgress &&
        secMgr->mCASEEngine->State == WeaveCASEEngine::kState_Complete)
    {
        secMgr->HandleSessionComplete();
    }
}

void WeaveSecurityManager::WRMPHandleSendError(ExchangeContext *ec, WEAVE_ERROR err, void *msgCtxt)
{
    WeaveLogProgress(SecurityManager, "%s", __FUNCTION__);
    WeaveSecurityManager *secMgr = (WeaveSecurityManager *)ec->AppState;

#if WEAVE_CONFIG_ENABLE_KEY_EXPORT_INITIATOR
    if (secMgr->State == kState_KeyExportInProgress)
    {
        secMgr->HandleKeyExportError(err, NULL);
    }
    else
#endif
    {
        secMgr->HandleSessionError(err, NULL);
    }
}

#endif // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING

void WeaveSecurityManager::AsyncNotifySecurityManagerAvailable()
{
    mSystemLayer->ScheduleWork(DoNotifySecurityManagerAvailable, this);
}

void WeaveSecurityManager::DoNotifySecurityManagerAvailable(System::Layer *systemLayer, void *appState, System::Error err)
{
    WeaveSecurityManager *_this = (WeaveSecurityManager *)appState;
    if (_this->State == kState_Idle)
    {
        _this->ExchangeManager->NotifySecurityManagerAvailable();
    }
}

/**
 * Cancel an in-progress session establishment.
 *
 * @param[in]  reqState         A pointer value that matches the value supplied by the application
 *                              when the session was started.
 *
 * @retval #WEAVE_NO_ERROR      If a matching in-progress session establishment was found and canceled.
 *
 * @retval #WEAVE_ERROR_INCORRECT_STATE   If there was no session establishment in progress, or the
 *                              in-progress session did not match the supplied request state pointer.
 */
WEAVE_ERROR WeaveSecurityManager::CancelSessionEstablishment(void *reqState)
{
    // If a session establishment is in progress and the supplied request state matches what was provided
    // when the session was started...
    if ((State == kState_CASEInProgress || State == kState_PASEInProgress || State == kState_TAKEInProgress) &&
        reqState == mStartSecureSession_ReqState)
    {
        // Clear the application's OnError handler to prevent a callback.
        mStartSecureSession_OnError = NULL;

        // Fail the session with a canceled error.
        HandleSessionError(WEAVE_ERROR_TRANSACTION_CANCELED, NULL);

        return WEAVE_NO_ERROR;
    }

    // Otherwise, tell the caller there was no match.
    else
    {
        return WEAVE_ERROR_INCORRECT_STATE;
    }
}

/**
 * Place a reservation on a message encryption key.
 *
 * Key reservations are used to signal that a particular key is actively in use and should be retained.
 * Note that placing reservation on a key does not guarantee that the key wont be removed by an explicit
 * action such as the reception of a KeyError message.
 *
 * For every reservation placed on a particular key, a corresponding call to ReleaseKey() must be made.
 *
 * This method accepts any form of key id, including None. Key ids that do not name actual keys are ignored.
 *
 * @param[in]  peerNodeId       The Weave node id of the peer with which the key shared.
 *
 * @param[in]  keyId            The id of the key to be reserved.
 *
 */
void WeaveSecurityManager::ReserveKey(uint64_t peerNodeId, uint16_t keyId)
{
    // If the key is a session key, attempt to locate the specified key and increase its reservation count.
    // (Currently reservations only apply to session keys).
    if (WeaveKeyId::IsSessionKey(keyId))
    {
        WeaveSessionKey *sessionKey;
        if (FabricState->FindSessionKey(keyId, peerNodeId, false, sessionKey) == WEAVE_NO_ERROR)
        {
            ReserveSessionKey(sessionKey);
        }
    }
}

/**
 * Release a message encryption key reservation.
 *
 * Release a reservations that was previously placed on a message encryption key.
 *
 * For every reservation placed on a particular key, the ReleaseKey() method must be called no more than once.
 *
 * This method accepts any form of key id, including None. Key ids that do not name actual keys are ignored.
 *
 * @param[in]  peerNodeId       The Weave node id of the peer with which the key shared.
 *
 * @param[in]  keyId            The id of the key whose reservation should be released.
 *
 */
void WeaveSecurityManager::ReleaseKey(uint64_t peerNodeId, uint16_t keyId)
{
    // If the key is a session key, attempt to locate the specified key and decrease its reservation count.
    // (Currently reservations only apply to session keys).
    if (WeaveKeyId::IsSessionKey(keyId))
    {
        WeaveSessionKey *sessionKey;
        if (FabricState->FindSessionKey(keyId, peerNodeId, false, sessionKey) == WEAVE_NO_ERROR)
        {
            ReleaseSessionKey(sessionKey);
        }
    }
}

/**
 * Place a reservation on a session key.
 *
 * @param[in]  sessionKey       A pointer to the session key to be reserved.
 *
 */
void WeaveSecurityManager::ReserveSessionKey(WeaveSessionKey *sessionKey)
{
    VerifyOrDie(sessionKey->ReserveCount < UINT8_MAX);
    sessionKey->ReserveCount++;
    sessionKey->MarkRecentlyActive();
    WeaveLogDetail(SecurityManager, "Reserve session key: Id=%04" PRIX16 " Peer=%016" PRIX64 " Reserve=%" PRId8,
            sessionKey->MsgEncKey.KeyId, sessionKey->NodeId, sessionKey->ReserveCount);
}

/**
 * Release a reservation on a session key.
 *
 * @param[in]  sessionKey       A pointer to the session key to be released.
 *
 */
void WeaveSecurityManager::ReleaseSessionKey(WeaveSessionKey *sessionKey)
{
    VerifyOrDie(sessionKey->ReserveCount > 0);

    sessionKey->ReserveCount--;

    WeaveLogDetail(SecurityManager, "Release session key: Id=%04" PRIX16 " Peer=%016" PRIX64 " Reserve=%" PRId8,
            sessionKey->MsgEncKey.KeyId, sessionKey->NodeId, sessionKey->ReserveCount);

    // If the session key is subject to automatic removal and its reserve count is now zero...
    if (sessionKey->BoundCon == NULL &&
        sessionKey->IsKeySet() &&
        sessionKey->ReserveCount == 0)
    {
        // If the session key is marked remove-on-idle, enable the idle session timer and mark the key as
        // recently active.  This will give it the maximum lifetime before it gets removed for inactivity.
        if (sessionKey->IsRemoveOnIdle())
        {
            StartIdleSessionTimer();
            sessionKey->MarkRecentlyActive();
        }

        // Otherwise remove the session key immediately.
        else
        {
            FabricState->RemoveSessionKey(sessionKey);
        }
    }
}

} // namespace Weave
} // namespace nl
