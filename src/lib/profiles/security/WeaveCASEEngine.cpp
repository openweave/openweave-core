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
 *      Implementation of WeaveCASEEngine class.
 *
 */

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif
#include <stdint.h>

#include <Weave/Core/WeaveCore.h>
#include "WeaveCASE.h"
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Support/ASN1.h>
#include <Weave/Support/logging/WeaveLogging.h>
#include <Weave/Profiles/security/WeaveSecurity.h>
#include <Weave/Profiles/security/WeaveCert.h>
#include <Weave/Profiles/security/WeaveSig.h>
#include <Weave/Profiles/security/WeavePrivateKey.h>
#include <Weave/Support/crypto/WeaveCrypto.h>
#include <Weave/Support/crypto/HashAlgos.h>
#include <Weave/Support/crypto/EllipticCurve.h>
#include <Weave/Support/CodeUtils.h>
#include <Weave/Support/WeaveFaultInjection.h>


namespace nl {
namespace Weave {
namespace Profiles {
namespace Security {
namespace CASE {

using namespace nl::Weave::Crypto;
using namespace nl::Weave::Encoding;
using namespace nl::Weave::TLV;
using namespace nl::Weave::ASN1;

#undef CASE_PRINT_CRYPTO_DATA
#ifdef CASE_PRINT_CRYPTO_DATA
static void PrintHex(const uint8_t *data, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++)
        printf("%02X", data[i]);
}
#endif

void WeaveCASEEngine::Init()
{
    memset(this, 0, sizeof(*this));
}

void WeaveCASEEngine::Shutdown()
{
    ClearSecretData((uint8_t *)this, sizeof(*this));
}

void WeaveCASEEngine::Reset()
{
    WeaveCASEAuthDelegate *savedAuthDelegate = AuthDelegate;
    ClearSecretData((uint8_t *)this, sizeof(*this));
    AuthDelegate = savedAuthDelegate;
}

void WeaveCASEEngine::SetAlternateConfigs(BeginSessionRequestContext & reqCtx)
{
    uint32_t altConfig = (reqCtx.ProtocolConfig == kCASEConfig_Config1) ? kCASEConfig_Config2 : kCASEConfig_Config1;
    if (IsAllowedConfig(altConfig))
    {
        reqCtx.AlternateConfigs[0] = altConfig;
        reqCtx.AlternateConfigCount = 1;
    }
}

void WeaveCASEEngine::SetAlternateCurves(BeginSessionRequestContext & reqCtx)
{
    reqCtx.AlternateCurveCount = 0;

#if WEAVE_CONFIG_SUPPORT_ELLIPTIC_CURVE_SECP256R1
    if ((mAllowedCurves & kWeaveCurveSet_prime256v1) != 0)
        reqCtx.AlternateCurveIds[reqCtx.AlternateCurveCount++] = kWeaveCurveId_prime256v1;
#endif // WEAVE_CONFIG_SUPPORT_ELLIPTIC_CURVE_SECP256R1

#if WEAVE_CONFIG_SUPPORT_ELLIPTIC_CURVE_SECP224R1
    if ((mAllowedCurves & kWeaveCurveSet_secp224r1) != 0)
        reqCtx.AlternateCurveIds[reqCtx.AlternateCurveCount++] = kWeaveCurveId_secp224r1;
#endif // WEAVE_CONFIG_SUPPORT_ELLIPTIC_CURVE_SECP224R1

#if WEAVE_CONFIG_SUPPORT_ELLIPTIC_CURVE_SECP192R1
    if ((mAllowedCurves & kWeaveCurveSet_prime192v1) != 0)
        reqCtx.AlternateCurveIds[reqCtx.AlternateCurveCount++] = kWeaveCurveId_prime192v1;
#endif // WEAVE_CONFIG_SUPPORT_ELLIPTIC_CURVE_SECP192R1

#if WEAVE_CONFIG_SUPPORT_ELLIPTIC_CURVE_SECP160R1
    if ((mAllowedCurves & kWeaveCurveSet_secp160r1) != 0)
        reqCtx.AlternateCurveIds[reqCtx.AlternateCurveCount++] = kWeaveCurveId_secp160r1;
#endif // WEAVE_CONFIG_SUPPORT_ELLIPTIC_CURVE_SECP160R1
}

WEAVE_ERROR WeaveCASEEngine::GenerateBeginSessionRequest(BeginSessionRequestContext & reqCtx, PacketBuffer * msgBuf)
{
    WEAVE_ERROR err;

    // Verify there isn't a begin session already outstanding.
    VerifyOrExit(State == kState_Idle, err = WEAVE_ERROR_INCORRECT_STATE);

    // Verify the auth delegate has been set.
    VerifyOrExit(AuthDelegate != NULL, err = WEAVE_ERROR_NO_CASE_AUTH_DELEGATE);

    WeaveLogDetail(SecurityManager, "CASE:GenerateBeginSessionRequest");

    // If a protocol config wasn't specified, choose a default one.
    if (reqCtx.ProtocolConfig == CASE::kCASEConfig_NotSpecified)
        reqCtx.ProtocolConfig = IsConfig2Allowed() ? kCASEConfig_Config2 : kCASEConfig_Config1;

    // Verify the proposed config is supported.
    VerifyOrExit(IsAllowedConfig(reqCtx.ProtocolConfig), err = WEAVE_ERROR_UNSUPPORTED_CASE_CONFIGURATION);

    // If a ECDH curve wasn't specified, choose a default one.
    if (reqCtx.CurveId == kWeaveCurveId_NotSpecified)
        reqCtx.CurveId = WEAVE_CONFIG_DEFAULT_CASE_CURVE_ID;

    // Verify the proposed ECDH curve is supported.
    VerifyOrExit(IsAllowedCurve(reqCtx.CurveId), err = WEAVE_ERROR_UNSUPPORTED_ELLIPTIC_CURVE);

    // Verify the requested key type.
    VerifyOrExit(WeaveKeyId::IsSessionKey(reqCtx.SessionKeyId), err = WEAVE_ERROR_WRONG_KEY_TYPE);

    // Verify the requested encryption type.
    VerifyOrExit(WeaveMessageLayer::IsSupportedEncryptionType(reqCtx.EncryptionType),
                 err = WEAVE_ERROR_UNSUPPORTED_ENCRYPTION_TYPE);

    // Record that we are acting as the initiator.
    SetIsInitiator(true);
    reqCtx.SetIsInitiator(true);

    // Remember various parameters of the session so that we can use them when the responder responds.
    SetSelectedConfig(reqCtx.ProtocolConfig);
    mCurveId = reqCtx.CurveId;
    SetPerformingKeyConfirm(reqCtx.PerformKeyConfirm());
    SessionKeyId = reqCtx.SessionKeyId;
    EncryptionType = reqCtx.EncryptionType;

    // Since the message contains a number of variable length fields with corresponding length values,
    // we start encoding in the middle of the message, and then go back and fill in the head when we know
    // the final lengths.
    msgBuf->SetDataLength(reqCtx.HeadLength());

    // Generate an ephemeral public/private key. Store the public key directly into the message and store
    // the private key in a state variable.
    err = AppendNewECDHKey(reqCtx, msgBuf);
    SuccessOrExit(err);

    // Append the local node's certificate information.
    err = AppendCertInfo(reqCtx, msgBuf);
    SuccessOrExit(err);

    // Append the initiator's payload, if specified.
    err = AppendPayload(reqCtx, msgBuf);
    SuccessOrExit(err);

    // Now encode the head of the message.
    err = reqCtx.EncodeHead(msgBuf);
    SuccessOrExit(err);

    // Sign the message using the supplied private key and append the signature to the buffer.
    // Save the message hash for later use in deriving the session keys.
    err = AppendSignature(reqCtx, msgBuf, mSecureState.BeforeKeyGen.RequestMsgHash);
    SuccessOrExit(err);

    State = kState_BeginRequestGenerated;

exit:
    return err;
}

WEAVE_ERROR WeaveCASEEngine::ProcessBeginSessionRequest(PacketBuffer * msgBuf, BeginSessionRequestContext & reqCtx, ReconfigureContext & reconfCtx)
{
    WEAVE_ERROR err;
    bool reconfigRequired = false;

    // Verify there isn't a begin session already outstanding.
    VerifyOrExit(State == kState_Idle, err = WEAVE_ERROR_INCORRECT_STATE);

    WeaveLogDetail(SecurityManager, "CASE:ProcessBeginSessionRequest");

    // Record that we are acting as the responder.
    SetIsInitiator(false);
    reqCtx.SetIsInitiator(false);

    // Decode the head of the message.
    err = reqCtx.DecodeHead(msgBuf);
    SuccessOrExit(err);

    // Verify the protocol parameters proposed by the peer (protocol config and ECDH curve id).  If the proposed
    // values are not acceptable, but an alternative set are, setup the ReconfigureContext with the alternatives
    // and return WEAVE_ERROR_CASE_RECONFIG_REQUIRED.
    reconfCtx.ProtocolConfig = reqCtx.ProtocolConfig;
    reconfCtx.CurveId = reqCtx.CurveId;
    err = VerifyProposedConfig(reqCtx, reconfCtx.ProtocolConfig);
    if (err == WEAVE_ERROR_CASE_RECONFIG_REQUIRED)
        reconfigRequired = true;
    else
        SuccessOrExit(err);
    err = VerifyProposedCurve(reqCtx, reconfCtx.CurveId);
    if (err == WEAVE_ERROR_CASE_RECONFIG_REQUIRED)
        reconfigRequired = true;
    else
        SuccessOrExit(err);
    if (reconfigRequired)
        ExitNow(err = WEAVE_ERROR_CASE_RECONFIG_REQUIRED);

    // Remember various parameters of the session so that we can use them when we respond.
    SetSelectedConfig(reqCtx.ProtocolConfig);
    mCurveId = reqCtx.CurveId;
    SetPerformingKeyConfirm(reqCtx.PerformKeyConfirm());
    SessionKeyId = reqCtx.SessionKeyId;
    EncryptionType = reqCtx.EncryptionType;

    // Verify the message signature.
    err = VerifySignature(reqCtx, msgBuf, mSecureState.BeforeKeyGen.RequestMsgHash);
    SuccessOrExit(err);

    // Verify the requested key type.
    VerifyOrExit(WeaveKeyId::IsSessionKey(reqCtx.SessionKeyId), err = WEAVE_ERROR_WRONG_KEY_TYPE);

    // Verify the requested encryption type.
    VerifyOrExit(WeaveMessageLayer::IsSupportedEncryptionType(EncryptionType),
                 err = WEAVE_ERROR_UNSUPPORTED_ENCRYPTION_TYPE);

    State = kState_BeginRequestProcessed;

exit:
    if (err != WEAVE_NO_ERROR)
        State = kState_Failed;
    return err;
}

WEAVE_ERROR WeaveCASEEngine::GenerateBeginSessionResponse(BeginSessionResponseContext & respCtx, PacketBuffer * msgBuf,
                                                          BeginSessionRequestContext & reqCtx)
{
    WEAVE_ERROR err;
    uint8_t respMsgHash[kMaxHashLength];

    // Verify the correct state.
    VerifyOrExit(State == kState_BeginRequestProcessed, err = WEAVE_ERROR_INCORRECT_STATE);

    WeaveLogDetail(SecurityManager, "CASE:GenerateBeginSessionResponse");

    respCtx.SetIsInitiator(false);

    // If the initiator requested key confirmation, then we must do it.
    // If the initiator DIDN'T requested key confirmation, but local policy requires it, then we force the use
    // of key confirmation.
    if (reqCtx.PerformKeyConfirm() || respCtx.PerformKeyConfirm() || ResponderRequiresKeyConfirm())
        SetPerformingKeyConfirm(true);

    // If performing key confirmation, signal so in the response and set the appropriate key confirmation
    // hash length based on the selected config.
    if (PerformingKeyConfirm())
    {
        respCtx.SetPerformKeyConfirm(true);
        respCtx.KeyConfirmHashLength = ConfigHashLength();
    }

    // Since the message contains variable length fields with corresponding length values, start encoding the tail
    // of the message, and then go back and fill in the head when we know the final lengths.
    msgBuf->SetDataLength(respCtx.HeadLength());

    // Generate an ephemeral public/private key. Append the public key to the message and store the private key
    // in a state variable.
    err = AppendNewECDHKey(respCtx, msgBuf);
    SuccessOrExit(err);

    // Append the local node's certificate information.
    err = AppendCertInfo(respCtx, msgBuf);
    SuccessOrExit(err);

    // Append the responder's payload, if specified.
    err = AppendPayload(respCtx, msgBuf);
    SuccessOrExit(err);

    // Now encode the head of the message.
    err = respCtx.EncodeHead(msgBuf);
    SuccessOrExit(err);

    // Sign the message using the supplied private key and append the signature to the buffer.
    err = AppendSignature(respCtx, msgBuf, respMsgHash);
    SuccessOrExit(err);

    {
        uint8_t responderKeyConfirmHash[kMaxHashLength];

        // Derive the session keys from the initiator's public key and our private key.
        err = DeriveSessionKeys(reqCtx.ECDHPublicKey, respMsgHash, responderKeyConfirmHash);
        SuccessOrExit(err);

        // If performing key confirmation...
        if (PerformingKeyConfirm())
        {
            // Append the responder hash to the BeginSessionResponse message.  The initiator will use this to
            // confirm that we have the correct session keys.
            VerifyOrExit(msgBuf->AvailableDataLength() >= respCtx.KeyConfirmHashLength, err = WEAVE_ERROR_BUFFER_TOO_SMALL);
            uint16_t msgLen = msgBuf->DataLength();
            memcpy(msgBuf->Start() + msgLen, responderKeyConfirmHash, respCtx.KeyConfirmHashLength);
            msgBuf->SetDataLength(msgLen + respCtx.KeyConfirmHashLength);

            State = kState_BeginResponseGenerated;
        }
        else
            State = kState_Complete;
    }

exit:
    if (err != WEAVE_NO_ERROR)
        State = kState_Failed;
    return err;
}

WEAVE_ERROR WeaveCASEEngine::ProcessBeginSessionResponse(PacketBuffer * msgBuf, BeginSessionResponseContext & respCtx)
{
    WEAVE_ERROR err;
    uint8_t respMsgHash[kMaxHashLength];

    VerifyOrExit(State == kState_BeginRequestGenerated, err = WEAVE_ERROR_INCORRECT_STATE);

    WeaveLogDetail(SecurityManager, "CASE:ProcessBeginSessionResponse");

    respCtx.SetIsInitiator(true);

    // Decode the head of the message.
    err = respCtx.DecodeHead(msgBuf);
    SuccessOrExit(err);

    // Verify the message signature.
    err = VerifySignature(respCtx, msgBuf, respMsgHash);
    SuccessOrExit(err);

    // If we asked for key confirmation, verify the responder responded appropriately.
    VerifyOrExit(!PerformingKeyConfirm() || respCtx.PerformKeyConfirm(), err = WEAVE_ERROR_INVALID_CASE_PARAMETER);

    // If the responder asked for key confirmation, we have to do it.
    if (respCtx.PerformKeyConfirm())
        SetPerformingKeyConfirm(true);

    {
        uint8_t responderKeyConfirmHash[kMaxHashLength];

        // Derive the session keys from the responder's public key and our private key.
        err = DeriveSessionKeys(respCtx.ECDHPublicKey, respMsgHash, responderKeyConfirmHash);
        SuccessOrExit(err);

        // If performing key confirmation...
        if (PerformingKeyConfirm())
        {
            uint8_t expectedKeyConfirmHashLen = ConfigHashLength();

            // Check the expected responder hash against the value in the response message.
            // Fail if they do not match.

            WEAVE_FAULT_INJECT(nl::Weave::FaultInjection::kFault_CASEKeyConfirm, ExitNow(err = WEAVE_ERROR_KEY_CONFIRMATION_FAILED));

            VerifyOrExit(respCtx.KeyConfirmHashLength == expectedKeyConfirmHashLen &&
                         ConstantTimeCompare(respCtx.KeyConfirmHash, responderKeyConfirmHash, expectedKeyConfirmHashLen),
                         err = WEAVE_ERROR_KEY_CONFIRMATION_FAILED);

            State = kState_BeginResponseProcessed;
        }

        else
            State = kState_Complete;
    }

exit:
    if (err != WEAVE_NO_ERROR)
        State = kState_Failed;
    return err;
}

WEAVE_ERROR WeaveCASEEngine::GenerateInitiatorKeyConfirm(PacketBuffer * msgBuf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint8_t keyConfirmHashLen = ConfigHashLength();

    VerifyOrExit(State == kState_BeginResponseProcessed && PerformingKeyConfirm(), err = WEAVE_ERROR_INCORRECT_STATE);

    WeaveLogDetail(SecurityManager, "CASE:GenerateInitiatorKeyConfirm");

    memcpy(msgBuf->Start(), mSecureState.AfterKeyGen.InitiatorKeyConfirmHash, keyConfirmHashLen);
    msgBuf->SetDataLength(keyConfirmHashLen);

    State = kState_Complete;

exit:
    if (err != WEAVE_NO_ERROR)
        State = kState_Failed;
    return err;
}

WEAVE_ERROR WeaveCASEEngine::ProcessInitiatorKeyConfirm(PacketBuffer * msgBuf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint8_t expectedKeyConfirmHashLen = ConfigHashLength();

    VerifyOrExit(State == kState_BeginResponseGenerated && PerformingKeyConfirm(), err = WEAVE_ERROR_INCORRECT_STATE);

    WeaveLogDetail(SecurityManager, "CASE:ProcessInitiatorKeyConfirm");

    WEAVE_FAULT_INJECT(nl::Weave::FaultInjection::kFault_CASEKeyConfirm, ExitNow(err = WEAVE_ERROR_KEY_CONFIRMATION_FAILED));

    VerifyOrExit(msgBuf->DataLength() == expectedKeyConfirmHashLen &&
                 ConstantTimeCompare(msgBuf->Start(), mSecureState.AfterKeyGen.InitiatorKeyConfirmHash, expectedKeyConfirmHashLen),
                 err = WEAVE_ERROR_KEY_CONFIRMATION_FAILED);

    State = kState_Complete;

exit:
    if (err != WEAVE_NO_ERROR)
        State = kState_Failed;
    return err;
}

WEAVE_ERROR WeaveCASEEngine::ProcessReconfigure(PacketBuffer * msgBuf, ReconfigureContext & reconfCtx)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    WeaveLogDetail(SecurityManager, "CASE:ProcessReconfigure");

    // Decode the Reconfigure message.
    err = ReconfigureContext::Decode(msgBuf, reconfCtx);
    SuccessOrExit(err);

    // Fail if the peer has sent more than one Reconfigure message.
    VerifyOrExit(!HasReconfigured(), err = WEAVE_ERROR_TOO_MANY_CASE_RECONFIGURATIONS);
    SetHasReconfigured(true);

    // Verify that the peer's proposed config is allowed.
    VerifyOrExit(IsAllowedConfig(reconfCtx.ProtocolConfig), err = WEAVE_ERROR_UNSUPPORTED_CASE_CONFIGURATION);

    // Verify that the peer's proposed ECDH curve is allowed.
    VerifyOrExit(IsAllowedCurve(reconfCtx.CurveId), err = WEAVE_ERROR_UNSUPPORTED_ELLIPTIC_CURVE);

    // Go back to Idle state so that the engine can be re-used to initiate a new CASE exchange.
    State = kState_Idle;

exit:
    return err;
}

WEAVE_ERROR WeaveCASEEngine::GetSessionKey(const WeaveEncryptionKey *& encKey)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(State == kState_Complete, err = WEAVE_ERROR_INCORRECT_STATE);

    encKey = &mSecureState.AfterKeyGen.EncryptionKey;

exit:
    return err;
}

WEAVE_ERROR WeaveCASEEngine::VerifyProposedConfig(BeginSessionRequestContext & reqCtx, uint32_t & selectedAltConfig)
{
    WEAVE_ERROR err = WEAVE_ERROR_UNSUPPORTED_CASE_CONFIGURATION;

    WeaveLogDetail(SecurityManager, "CASE:VerifyProposedConfig");

    // If Config2 is allowed, use it if it is the proposed config.
    // If Config2 is NOT the proposed config, but is in the peer's list
    // of alternates, force a reconfig selecting Config2.
    if (IsConfig2Allowed())
    {
        if (reqCtx.ProtocolConfig == kCASEConfig_Config2)
            ExitNow(err = WEAVE_NO_ERROR);
        if (reqCtx.IsAltConfig(kCASEConfig_Config2))
        {
            selectedAltConfig = kCASEConfig_Config2;
            ExitNow(err = WEAVE_ERROR_CASE_RECONFIG_REQUIRED);
        }
    }

    // If Config1 is allowed, use it if it is the proposed config.
    // If Config1 is NOT the proposed config, but is in the peer's list
    // of alternates, force a reconfig selecting Config1.
    if (IsConfig1Allowed())
    {
        if (reqCtx.ProtocolConfig == kCASEConfig_Config1)
            ExitNow(err = WEAVE_NO_ERROR);

        // If Config1 is in the peer's list of alternates, force a reconfig selecting Config1.
        if (reqCtx.IsAltConfig(kCASEConfig_Config1))
        {
            selectedAltConfig = kCASEConfig_Config1;
            ExitNow(err = WEAVE_ERROR_CASE_RECONFIG_REQUIRED);
        }
    }

exit:
    return err;
}

WEAVE_ERROR WeaveCASEEngine::VerifyProposedCurve(BeginSessionRequestContext & reqCtx, uint32_t & selectedAltCurve)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    WeaveLogDetail(SecurityManager, "CASE:VerifyProposedCurve");

    // If the requested elliptic curve is not allowed, select an allowed alternate, if available.
    if (!IsAllowedCurve(reqCtx.CurveId))
    {
        uint8_t i = 0;
        for (; i < reqCtx.AlternateCurveCount; i++)
            if (IsAllowedCurve(reqCtx.AlternateCurveIds[i]))
                break;

        VerifyOrExit(i < reqCtx.AlternateCurveCount, err = WEAVE_ERROR_UNSUPPORTED_ELLIPTIC_CURVE);

        selectedAltCurve = reqCtx.AlternateCurveIds[i];
        ExitNow(err = WEAVE_ERROR_CASE_RECONFIG_REQUIRED);
    }

exit:
    return err;
}

WEAVE_ERROR WeaveCASEEngine::AppendNewECDHKey(BeginSessionContext & msgCtx, PacketBuffer * msgBuf)
{
    WEAVE_ERROR err;
    EncodedECPrivateKey privKey;
    uint16_t msgLen = msgBuf->DataLength();

    WeaveLogDetail(SecurityManager, "CASE:AppendNewECDHKey");

    // Generate an ephemeral public/private key. Store the public key directly into the message and store
    // the private key in the provided object.
    msgCtx.ECDHPublicKey.ECPoint = msgBuf->Start() + msgLen;
    msgCtx.ECDHPublicKey.ECPointLen = msgBuf->AvailableDataLength(); // GenerateECDHKey() will update with final length.
    privKey.PrivKey = mSecureState.BeforeKeyGen.ECDHPrivateKey;
    privKey.PrivKeyLen = sizeof(mSecureState.BeforeKeyGen.ECDHPrivateKey);
    err = GenerateECDHKey(WeaveCurveIdToOID(msgCtx.CurveId), msgCtx.ECDHPublicKey, privKey);
    SuccessOrExit(err);

#if WEAVE_CONFIG_SECURITY_TEST_MODE

    // If enabled, override the generated key with a known key pair to allow man-in-the-middle session key recovery for
    // testing purposes.
    // NOTE: We still do the work of generating a proper key pair in this case so that the execution time
    // remains largely the same.
    if (UseKnownECDHKey())
    {
        // Private key is 1.
        privKey.PrivKey[0] = 0x01;
        privKey.PrivKeyLen = 1;

        // Public key is generator.
        msgCtx.ECDHPublicKey.ECPointLen = msgBuf->AvailableDataLength();
        err = GetCurveG(WeaveCurveIdToOID(msgCtx.CurveId), msgCtx.ECDHPublicKey);
        SuccessOrExit(err);

        WeaveLogError(SecurityManager, "WARNING: Using well-known ECDH key in CASE ***** SESSION IS NOT SECURE *****");
    }

#endif // WEAVE_CONFIG_SECURITY_TEST_MODE

    mSecureState.BeforeKeyGen.ECDHPrivateKeyLength = privKey.PrivKeyLen;

    msgBuf->SetDataLength(msgLen + msgCtx.ECDHPublicKey.ECPointLen);

exit:
    return err;
}

WEAVE_ERROR WeaveCASEEngine::AppendCertInfo(BeginSessionContext & msgCtx, PacketBuffer * msgBuf)
{
    WEAVE_ERROR err;

    WeaveLogDetail(SecurityManager, "CASE:AppendCertInfo");

#if !WEAVE_CONFIG_LEGACY_CASE_AUTH_DELEGATE

    // Initialize a TLV writer to write the CertificationInformation structure.
    TLVWriter writer;
    writer.Init(msgBuf);
    writer.ImplicitProfileId = kWeaveProfile_Security;

    // Call the auth delegate to write the certificate information for the local node.
    err = AuthDelegate->EncodeNodeCertInfo(msgCtx, writer);
    SuccessOrExit(err);

    // Finalize the TLV encoding.  Note that this updates the data length of the supplied PacketBuffer.
    err = writer.Finalize();
    SuccessOrExit(err);

    // Capture the encoded length of the CertificateInformation structure, which will be included in
    // the message header.
    msgCtx.CertInfoLength = (uint16_t)writer.GetLengthWritten();

#else // !WEAVE_CONFIG_LEGACY_CASE_AUTH_DELEGATE

    // Call the auth delegate to generate the certificate information for the local node and append it
    // to the message.
    uint16_t msgLen = msgBuf->DataLength();
    err = AuthDelegate->GetNodeCertInfo(IsInitiator(), msgBuf->Start() + msgLen, msgBuf->AvailableDataLength(), msgCtx.CertInfoLength);
    SuccessOrExit(err);
    msgBuf->SetDataLength(msgLen + msgCtx.CertInfoLength);

#endif // WEAVE_CONFIG_LEGACY_CASE_AUTH_DELEGATE

exit:
    return err;
}

WEAVE_ERROR WeaveCASEEngine::AppendPayload(BeginSessionContext & msgCtx, PacketBuffer * msgBuf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint16_t msgLen = msgBuf->DataLength();

    WeaveLogDetail(SecurityManager, "CASE:AppendPayload");

    err = AuthDelegate->EncodeNodePayload(msgCtx, msgBuf->Start() + msgLen, msgBuf->AvailableDataLength(), msgCtx.PayloadLength);
    SuccessOrExit(err);

    msgBuf->SetDataLength(msgLen + msgCtx.PayloadLength);

exit:
    return err;
}

// Generate a signature for an encoded CASE message (in the supplied buffer) and append it to the message.
WEAVE_ERROR WeaveCASEEngine::AppendSignature(BeginSessionContext & msgCtx, PacketBuffer * msgBuf, uint8_t * msgHash)
{
    WEAVE_ERROR err;
    uint8_t * msgStart = msgBuf->Start();
    uint16_t tbsDataLen = msgBuf->DataLength();
    const uint64_t sigTag = ProfileTag(kWeaveProfile_Security, kTag_WeaveCASESignature);

    WeaveLogDetail(SecurityManager, "CASE:AppendSignature");

    // Generate a hash of the to-be-signed portion of the message.
    GenerateHash(msgStart, tbsDataLen, msgHash);

    // Generate a message signature using the local node's private key and append this to the message.
    {
        TLVWriter writer;
        writer.Init(msgBuf);
        writer.ImplicitProfileId = kWeaveProfile_Security;

#if !WEAVE_CONFIG_LEGACY_CASE_AUTH_DELEGATE
        WeaveLogDetail(SecurityManager, "CASE:GenerateSignature");
#endif

        err = AuthDelegate->GenerateNodeSignature(msgCtx, msgHash, ConfigHashLength(), writer, sigTag);
        SuccessOrExit(err);

        err = writer.Finalize();
        SuccessOrExit(err);

        msgCtx.Signature = msgStart + tbsDataLen;
        msgCtx.SignatureLength = writer.GetLengthWritten();
    }

exit:
    return err;
}

// Verify a CASE message signature (for the message in the supplied buffer) against a given public key.
// Returns a hash of the signed portion of the message in the supplied bufer.
WEAVE_ERROR WeaveCASEEngine::VerifySignature(BeginSessionContext & msgCtx, PacketBuffer * msgBuf, uint8_t * msgHash)
{
    WEAVE_ERROR err, validRes;
    WeaveCertificateSet certSet;
    ValidationContext validCtx;
    WeaveDN peerCertDN;
    CertificateKeyId peerCertSubjectKeyId;
    EncodedECDSASignature ecdsaSig;
    TLVReader reader;
    uint16_t signedDataLen;
    uint8_t *msgStart = msgBuf->Start();
    bool callEndValidation = false;

    WeaveLogDetail(SecurityManager, "CASE:VerifySignature");

    // TODO: move validation to separate method

    validCtx.Reset();

    // Invoke the auth delegate to prepare the certificate set and the validation context.
    // This will load the trust anchors into the certificate set and establish the
    // desired validation criteria for the peer's entity certificate.
    err = AuthDelegate->BeginValidation(msgCtx, validCtx, certSet);
    SuccessOrExit(err);
    callEndValidation = true;

    // If the cert type property has been set, set it as the required certificate type in the
    // validation context such that the cert type is enforced during the call to FindValidCert().
    validCtx.RequiredCertType = mCertType;

    WeaveLogDetail(SecurityManager, "CASE:DecodeCertificateInfo");

    // Decode the certificate information supplied by the peer.
    validCtx.RequiredKeyUsages |= kKeyUsageFlag_DigitalSignature;
    err = DecodeCertificateInfo(msgCtx, certSet, peerCertDN, peerCertSubjectKeyId);
    if (err == WEAVE_ERROR_UNSUPPORTED_ELLIPTIC_CURVE || err == WEAVE_ERROR_UNSUPPORTED_SIGNATURE_TYPE)
    {
        err = WEAVE_ERROR_UNSUPPORTED_CERT_FORMAT;
    }
    SuccessOrExit(err);

    // Allow the auth delegate to inspect and possibly update the peer's subject information and/or
    // the set of certificates that will be used for validation.
    err = AuthDelegate->OnPeerCertsLoaded(msgCtx, peerCertDN, peerCertSubjectKeyId, validCtx, certSet);
    SuccessOrExit(err);

    // Search the certificate set for the peer's certificate and validate that it is trusted
    // and suitable for CASE authentication.  This method performs certificate chain construction
    // and validates that there is a path from the peer's certificate to a trust anchor certificate.
    WeaveLogDetail(SecurityManager, "CASE:ValidateCert");
    validRes = certSet.FindValidCert(peerCertDN, peerCertSubjectKeyId, validCtx, validCtx.SigningCert);

    // If a valid certificate was found for the peer capture the type of the peer's certificate.
    if (validRes == WEAVE_NO_ERROR)
    {
        // Update the cert type property with the type of the peer's certificate.
        mCertType = validCtx.SigningCert->CertType;
    }

    // Tell the auth delegate about the results of certificate validation and let it weigh in
    // on the process.  Note that this may alter the validation result (validRes) from successful
    // to unsuccessful.
    err = AuthDelegate->HandleValidationResult(msgCtx, validCtx, certSet, validRes);
    // TODO: verify that validRes not reset to NO_ERROR.
    SuccessOrExit(err);

    // Check that validation succeeded.
    VerifyOrExit(validRes == WEAVE_NO_ERROR, err = validRes);
    VerifyOrExit(validCtx.SigningCert != NULL, err = WEAVE_ERROR_INCORRECT_STATE);

    // Decode the CASE signature from the end of the message.
    reader.Init(msgCtx.Signature, msgCtx.SignatureLength);
    reader.ImplicitProfileId = kWeaveProfile_Security;
    err = reader.Next(kTLVType_Structure, ProfileTag(kWeaveProfile_Security, kTag_WeaveCASESignature));
    SuccessOrExit(err);
    WeaveLogDetail(SecurityManager, "CASE:DecodeWeaveECDSASignature");
    err = DecodeWeaveECDSASignature(reader, ecdsaSig);
    SuccessOrExit(err);
    err = reader.VerifyEndOfContainer();
    SuccessOrExit(err);

    // Compute the length of the signed portion of the message.
    signedDataLen = msgCtx.Signature - msgStart;

    // Generate a hash of the to-be-signed portion of the message.
    GenerateHash(msgStart, signedDataLen, msgHash);

    // Verify the message signature against the computed message hash and the
    // the public key from the peer's certificate.
    WeaveLogDetail(SecurityManager, "CASE:VerifyECDSASignature");
    err = VerifyECDSASignature(WeaveCurveIdToOID(validCtx.SigningCert->PubKeyCurveId), msgHash, ConfigHashLength(),
                               ecdsaSig, validCtx.SigningCert->PublicKey.EC);
    SuccessOrExit(err);

exit:
    if (callEndValidation)
    {
        AuthDelegate->EndValidation(msgCtx, validCtx, certSet);
    }
    certSet.Release();
    return err;
}

WEAVE_ERROR WeaveCASEEngine::DecodeCertificateInfo(BeginSessionContext & msgCtx, WeaveCertificateSet & certSet,
                                                   WeaveDN & entityCertDN, CertificateKeyId & entityCertSubjectKeyId)
{
    WEAVE_ERROR err;
    TLVReader reader;
    WeaveCertificateData *entityCert = NULL;

    // Begin decoding the certificate information structure.
    reader.Init(msgCtx.CertInfo, msgCtx.CertInfoLength);
    reader.ImplicitProfileId = kWeaveProfile_Security;
    err = reader.Next(kTLVType_Structure, ProfileTag(kWeaveProfile_Security, kTag_WeaveCASECertificateInformation));
    SuccessOrExit(err);

    {
        TLVType containerType;

        err = reader.EnterContainer(containerType);
        SuccessOrExit(err);

        err = reader.Next();

        // Look for the element representing the certificate of the authenticating entity (i.e. the peer). If found...
        if (err == WEAVE_NO_ERROR && reader.GetTag() == ContextTag(kTag_CASECertificateInfo_EntityCertificate))
        {
            // Load the authenticating entity's certificate into the certificate set.
            err = certSet.LoadCert(reader, kDecodeFlag_GenerateTBSHash, entityCert);
            SuccessOrExit(err);

            entityCertDN = entityCert->SubjectDN;
            entityCertSubjectKeyId = entityCert->SubjectKeyId;

            err = reader.Next();
        }

        // Look for the EntityCertificateRef element and fail if found.
        // NOTE: This version of the code does not support the use of certificate reference to identify the certificate.
        if (err == WEAVE_NO_ERROR && reader.GetTag() == ContextTag(kTag_CASECertificateInfo_EntityCertificateRef))
        {
            ExitNow(err = WEAVE_ERROR_UNSUPPORTED_CERT_FORMAT); // TODO: use better error
        }

        // Look for the RelatedCertificates element. If found, load the contained certificates into the certificate set.
        if (err == WEAVE_NO_ERROR && reader.GetTag() == ContextTag(kTag_CASECertificateInfo_RelatedCertificates))
        {
            err = certSet.LoadCerts(reader, kDecodeFlag_GenerateTBSHash);
            SuccessOrExit(err);

            err = reader.Next();
        }

        // Skip the TrustAnchors element if present.  This represents information an initiator provides to the
        // responder about what certificates it trusts, allowing the responder to select an appropriate entity
        // certificate to respond with. This code assumes that the local node only has a single entity certificate,
        // and thus it's that or nothing.
        if (err == WEAVE_NO_ERROR && reader.GetTag() == ContextTag(kTag_CASECertificateInfo_TrustAnchors))
        {
            err = reader.Next();
        }

        if (err == WEAVE_NO_ERROR)
            ExitNow(err = WEAVE_ERROR_UNEXPECTED_TLV_ELEMENT);
        if (err != WEAVE_END_OF_TLV)
            ExitNow();

        err = reader.ExitContainer(containerType);
        SuccessOrExit(err);
    }

    err = reader.VerifyEndOfContainer();
    SuccessOrExit(err);

    // Verify that the peer included its entity certificate.
    VerifyOrExit(entityCert != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

exit:
    return err;
}

WEAVE_ERROR WeaveCASEEngine::DeriveSessionKeys(EncodedECPublicKey & pubKey, const uint8_t * respMsgHash,
                                               uint8_t * responderKeyConfirmHash)
{
    WEAVE_ERROR err;
    uint8_t hashLen = ConfigHashLength();
#if WEAVE_CONFIG_SUPPORT_CASE_CONFIG1
    HKDFSHA1Or256 hkdf(IsUsingConfig1());
#else
    HKDFSHA256 hkdf;
#endif

    WeaveLogDetail(SecurityManager, "CASE:DeriveSessionKeys");

    // Double check the encryption type.
    VerifyOrExit(WeaveMessageLayer::IsSupportedEncryptionType(EncryptionType),
                 err = WEAVE_ERROR_UNSUPPORTED_ENCRYPTION_TYPE);

    // Prepare a salt value to be used in the generation of the master key. The salt value
    // is composed from the hashes of the signed portions of the CASE request and response
    // messages.  Thus the salt incorporates the entity certificates for both parties plus
    // their ephemeral DH public keys.
    {
        uint8_t keySalt[2 * kMaxHashLength];

        memcpy(keySalt, mSecureState.BeforeKeyGen.RequestMsgHash, hashLen);
        memcpy(keySalt + hashLen, respMsgHash, hashLen);

#ifdef CASE_PRINT_CRYPTO_DATA
        printf("Salt: "); PrintHex(keySalt, 2 * hashLen); printf("\n");
#endif

        hkdf.BeginExtractKey(keySalt, 2 * hashLen);
    }

    // Generate a master key from which the session keys will be derived...
    {
        EncodedECPrivateKey privKey;
        uint8_t sharedSecretBuf[kMaxECDHSharedSecretSize];
        uint16_t sharedSecretLen;
        OID curveOID;

        // Compute the Diffie-Hellman shared secret from the peer's public key and our private key.
        privKey.PrivKey = mSecureState.BeforeKeyGen.ECDHPrivateKey;
        privKey.PrivKeyLen = mSecureState.BeforeKeyGen.ECDHPrivateKeyLength;
        curveOID = WeaveCurveIdToOID(mCurveId);
        err = ECDHComputeSharedSecret(curveOID, pubKey, privKey, sharedSecretBuf, sizeof(sharedSecretBuf), sharedSecretLen);
        SuccessOrExit(err);

        ClearSecretData(mSecureState.BeforeKeyGen.ECDHPrivateKey, sizeof(mSecureState.BeforeKeyGen.ECDHPrivateKey));

#ifdef CASE_PRINT_CRYPTO_DATA
        printf("Key Material: "); PrintHex(sharedSecretBuf, sharedSecretLen); printf("\n");
#endif

        // Perform HKDF-based key extraction to produce a master pseudo-random key from the
        // ECDH shared secret.
        hkdf.AddKeyMaterial(sharedSecretBuf, sharedSecretLen);
        err = hkdf.FinishExtractKey();
        ClearSecretData(sharedSecretBuf, sizeof(sharedSecretBuf));
        SuccessOrExit(err);
    }

    // Derive the session keys from the master key...
    {
        uint8_t sessionKeyData[WeaveEncryptionKey::MaxKeySize + kMaxHashLength];
        uint8_t msgEncKeyLen;
        uint8_t keyConfirmKeyLen;

#if WEAVE_CONFIG_AES128EAX64 || WEAVE_CONFIG_AES128EAX128
        if (EncryptionType == kWeaveEncryptionType_AES128EAX64 ||
            EncryptionType == kWeaveEncryptionType_AES128EAX128)
        {
            msgEncKeyLen = WeaveEncryptionKey_AES128EAX::KeySize;
        }
        else
#endif // WEAVE_CONFIG_AES128EAX64 || WEAVE_CONFIG_AES128EAX128
        {
            msgEncKeyLen = WeaveEncryptionKey_AES128CTRSHA1::KeySize;
        }

        keyConfirmKeyLen = PerformingKeyConfirm() ? hashLen : 0;

        // Perform HKDF-based key expansion to produce the desired key material.
        err = hkdf.ExpandKey(NULL, 0, msgEncKeyLen + keyConfirmKeyLen, sessionKeyData);
        SuccessOrExit(err);

#ifdef CASE_PRINT_CRYPTO_DATA
        printf("Session Key Data: "); PrintHex(sessionKeyData, msgEncKeyLen + keyConfirmKeyLen); printf("\n");
#endif

        // Copy the generated key data to the appropriate destinations.
#if WEAVE_CONFIG_AES128EAX64 || WEAVE_CONFIG_AES128EAX128
        if (EncryptionType == kWeaveEncryptionType_AES128EAX64 ||
            EncryptionType == kWeaveEncryptionType_AES128EAX128)
        {
            memcpy(mSecureState.AfterKeyGen.EncryptionKey.AES128EAX.Key,
                   sessionKeyData,
                   WeaveEncryptionKey_AES128EAX::KeySize);
        }
        else
#endif // WEAVE_CONFIG_AES128EAX64 || WEAVE_CONFIG_AES128EAX128
        {
            memcpy(mSecureState.AfterKeyGen.EncryptionKey.AES128CTRSHA1.DataKey,
                   sessionKeyData,
                   WeaveEncryptionKey_AES128CTRSHA1::DataKeySize);
            memcpy(mSecureState.AfterKeyGen.EncryptionKey.AES128CTRSHA1.IntegrityKey,
                   sessionKeyData + WeaveEncryptionKey_AES128CTRSHA1::DataKeySize,
                   WeaveEncryptionKey_AES128CTRSHA1::IntegrityKeySize);
        }

        // If performing key confirmation...
        if (PerformingKeyConfirm())
        {
            // Use the key confirmation key to generate key confirmation hashes. Store the initiator hash
            // (the single hash) in state data for later use.  Return the responder hash (the double hash)
            // to the caller.
            uint8_t * keyConfirmKey = sessionKeyData + msgEncKeyLen;
            GenerateKeyConfirmHashes(keyConfirmKey, mSecureState.AfterKeyGen.InitiatorKeyConfirmHash,
                                     responderKeyConfirmHash);
        }

        ClearSecretData(sessionKeyData, sizeof(sessionKeyData));
    }

exit:
    return err;
}

void WeaveCASEEngine::GenerateHash(const uint8_t * inData, uint16_t inDataLen, uint8_t * hash)
{
    if (IsUsingConfig1())
    {
        SHA1 sha;
        sha.Begin();
        sha.AddData(inData, inDataLen);
        sha.Finish(hash);
    }
    else
    {
        SHA256 sha;
        sha.Begin();
        sha.AddData(inData, inDataLen);
        sha.Finish(hash);
    }
}

void WeaveCASEEngine::GenerateKeyConfirmHashes(const uint8_t * keyConfirmKey, uint8_t * singleHash, uint8_t * doubleHash)
{
    uint8_t hashLen = ConfigHashLength();

    // Generate a single hash of the key confirmation key.
    GenerateHash(keyConfirmKey, hashLen, singleHash);

    // Generate a double hash of the key confirmation key.
    GenerateHash(singleHash, hashLen, doubleHash);
}

uint32_t WeaveCASEEngine::SelectedConfig() const
{
    if (IsUsingConfig1())
        return kCASEConfig_Config1;
    else
        return kCASEConfig_Config2;
}

bool WeaveCASEEngine::IsAllowedConfig(uint32_t config) const
{
    return (config == kCASEConfig_Config1 && IsConfig1Allowed()) ||
           (config == kCASEConfig_Config2 && IsConfig2Allowed());
}


#if !WEAVE_CONFIG_LEGACY_CASE_AUTH_DELEGATE

WEAVE_ERROR WeaveCASEAuthDelegate::EncodeNodePayload(const BeginSessionContext & msgCtx,
            uint8_t * payloadBuf, uint16_t payloadBufSize, uint16_t & payloadLen)
{
    payloadLen = 0;
    return WEAVE_NO_ERROR;
}

WEAVE_ERROR WeaveCASEAuthDelegate::OnPeerCertsLoaded(const BeginSessionContext & msgCtx,
            WeaveDN & subjectDN, CertificateKeyId & subjectKeyId, ValidationContext & validCtx,
            WeaveCertificateSet & certSet)
{
    return WEAVE_NO_ERROR;
}

#else // !WEAVE_CONFIG_LEGACY_CASE_AUTH_DELEGATE

WEAVE_ERROR WeaveCASEAuthDelegate::GenerateNodeSignature(const BeginSessionContext & msgCtx,
        const uint8_t * msgHash, uint8_t msgHashLen, TLVWriter & writer, uint64_t tag)
{
    WEAVE_ERROR err;
    const uint8_t * signingKey = NULL;
    uint16_t signingKeyLen;

    WeaveLogDetail(SecurityManager, "CASE:GetNodePrivateKey");

    // Get the local node's private key.
    err = GetNodePrivateKey(msgCtx.IsInitiator(), signingKey, signingKeyLen);
    SuccessOrExit(err);

    WeaveLogDetail(SecurityManager, "CASE:GenerateSignature");

    // Generate a Weave ECDSA signature and append it to the message.
    err = GenerateAndEncodeWeaveECDSASignature(writer, tag, msgHash, msgHashLen, signingKey, signingKeyLen);
    SuccessOrExit(err);

exit:
    // Release the node private key.
    if (signingKey != NULL)
    {
        WEAVE_ERROR relErr = ReleaseNodePrivateKey(signingKey);
        err = (err == WEAVE_NO_ERROR) ? relErr : err;
    }
    return err;
}

#endif // WEAVE_CONFIG_LEGACY_CASE_AUTH_DELEGATE


/**
 * Encodes a WeaveCASECertificateInformation TLV structure
 *
 * This method encodes a WeaveCASECertificateInformation structure in Weave TLV form
 * containing a specified entity certificate and an optional intermediate certificate.
 * The resultant CASE certificate info structure is written to a supplied buffer.
 *
 * @param[in] buf                   The buffer into which the encoded CASE certificate info
 *                                  structure should be written.
 * @param[in] bufSize               The size in bytes of the buffer pointed at by buf.
 * @param[out] certInfoLen          An integer value that will receive the final encoded size
 *                                  of the CASE certificate info structure.  This value is
 *                                  only meaningful in the event that the function succeeds.
 * @param[in] entityCert            A buffer containing the entity certificate to be included
 *                                  in the CASE certificate info structure.  The entity
 *                                  certificate is expected to be encoded in Weave TLV form.
 * @param[in] entityCertLen         The length in bytes of the encoded entity certificate.
 * @param[in] intermediateCert      Optionally, an buffer containing an intermediate certificate
 *                                  to be included as a related certificate within the CASE
 *                                  certificate info structure.  When supplied, the intermediate
 *                                  certificate is expected to be encoded in Weave TLV form.
 *                                  If NULL is given, the generated certificate info structure
 *                                  will not contain any related certificates.
 * @param[in] intermediateCertLen   The length in bytes of the encoded intermediate certificate.
 *
 * @retval #WEAVE_NO_ERROR          If the operation succeeded.
 * @retval other                    Other Weave error codes related to the decoding of the
 *                                  input certificates or the encoding of the CASE certificate
 *                                  info structure.
 */
WEAVE_ERROR EncodeCASECertInfo(uint8_t * buf, uint16_t bufSize, uint16_t& certInfoLen,
        const uint8_t * entityCert, uint16_t entityCertLen,
        const uint8_t * intermediateCert, uint16_t intermediateCertLen)
{
    WEAVE_ERROR err;
    TLVWriter writer;

    writer.Init(buf, bufSize);
    writer.ImplicitProfileId = kWeaveProfile_Security;

    err = EncodeCASECertInfo(writer, entityCert, entityCertLen, intermediateCert, intermediateCertLen);
    SuccessOrExit(err);

    err = writer.Finalize();
    SuccessOrExit(err);

    certInfoLen = writer.GetLengthWritten();

exit:
    return err;
}

/**
 * Encodes a WeaveCASECertificateInformation TLV structure
 *
 * This method encodes a WeaveCASECertificateInformation structure in Weave TLV form
 * containing a specified entity certificate and an optional intermediate certificate.
 * The resultant CASE certificate info structure is written to a supplied TLVWriter.
 *
 * @param[in] writer                The TLVWriter object to which the encoded CASE certificate
 *                                  info structure should be written.
 * @param[in] entityCert            A buffer containing the entity certificate to be included
 *                                  in the CASE certificate info structure.  The entity
 *                                  certificate is expected to be encoded in Weave TLV form.
 * @param[in] entityCertLen         The length in bytes of the encoded entity certificate.
 * @param[in] intermediateCert      Optionally, an buffer containing an intermediate certificate
 *                                  to be included as a related certificate within the CASE
 *                                  certificate info structure.  When supplied, the intermediate
 *                                  certificate is expected to be encoded in Weave TLV form.
 *                                  If NULL is given, the generated certificate info structure
 *                                  will not contain any related certificates.
 * @param[in] intermediateCertLen   The length in bytes of the encoded intermediate certificate.
 *
 * @retval #WEAVE_NO_ERROR          If the operation succeeded.
 * @retval other                    Other Weave error codes related to the decoding of the
 *                                  input certificates or the encoding of the CASE certificate
 *                                  info structure.
 */
WEAVE_ERROR EncodeCASECertInfo(TLVWriter & writer,
        const uint8_t * entityCert, uint16_t entityCertLen,
        const uint8_t * intermediateCert, uint16_t intermediateCertLen)
{
    WEAVE_ERROR err;
    TLVType container;

    // Start the WeaveCASECertificateInformation structure.
    err = writer.StartContainer(ProfileTag(kWeaveProfile_Security, kTag_WeaveCASECertificateInformation), kTLVType_Structure, container);
    SuccessOrExit(err);

    // Copy the supplied entity certificate into CASE certificate info structure using the EntityCertificate tag.
    err = writer.CopyContainer(ContextTag(kTag_CASECertificateInfo_EntityCertificate), entityCert, entityCertLen);
    SuccessOrExit(err);

    // If an intermediate certificate has been supplied...
    if (intermediateCert != NULL)
    {
        TLVType container2;

        // Start the list of RelatedCertificates with the CASE certificate info structure.
        err = writer.StartContainer(ContextTag(kTag_CASECertificateInfo_RelatedCertificates), kTLVType_Path, container2);
        SuccessOrExit(err);

        // Copy the supplied intermediate certificate into the RelatedCertificates list.
        err = writer.CopyContainer(ProfileTag(kWeaveProfile_Security, kTag_WeaveCertificate), intermediateCert, intermediateCertLen);
        SuccessOrExit(err);

        // End the RelatedCertificates list.
        err = writer.EndContainer(container2);
        SuccessOrExit(err);
    }

    // End the CASE certificate info structure.
    err = writer.EndContainer(container);
    SuccessOrExit(err);

exit:
    return err;
}


} // namespace CASE
} // namespace Security
} // namespace Profiles
} // namespace Weave
} // namespace nl
