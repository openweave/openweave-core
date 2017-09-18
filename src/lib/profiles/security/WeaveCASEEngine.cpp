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

void WeaveCASEEngine::SetAlternateConfigs(BeginSessionRequestMessage& req)
{
    uint32_t altConfig = (req.ProtocolConfig == kCASEConfig_Config1) ? kCASEConfig_Config2 : kCASEConfig_Config1;
    if (IsAllowedConfig(altConfig))
    {
        req.AlternateConfigs[0] = altConfig;
        req.AlternateConfigCount = 1;
    }
}

void WeaveCASEEngine::SetAlternateCurves(BeginSessionRequestMessage& req)
{
    req.AlternateCurveCount = 0;

#if WEAVE_CONFIG_SUPPORT_ELLIPTIC_CURVE_SECP256R1
    if ((mAllowedCurves & kWeaveCurveSet_prime256v1) != 0)
        req.AlternateCurveIds[req.AlternateCurveCount++] = kWeaveCurveId_prime256v1;
#endif // WEAVE_CONFIG_SUPPORT_ELLIPTIC_CURVE_SECP256R1

#if WEAVE_CONFIG_SUPPORT_ELLIPTIC_CURVE_SECP224R1
    if ((mAllowedCurves & kWeaveCurveSet_secp224r1) != 0)
        req.AlternateCurveIds[req.AlternateCurveCount++] = kWeaveCurveId_secp224r1;
#endif // WEAVE_CONFIG_SUPPORT_ELLIPTIC_CURVE_SECP224R1

#if WEAVE_CONFIG_SUPPORT_ELLIPTIC_CURVE_SECP192R1
    if ((mAllowedCurves & kWeaveCurveSet_prime192v1) != 0)
        req.AlternateCurveIds[req.AlternateCurveCount++] = kWeaveCurveId_prime192v1;
#endif // WEAVE_CONFIG_SUPPORT_ELLIPTIC_CURVE_SECP192R1

#if WEAVE_CONFIG_SUPPORT_ELLIPTIC_CURVE_SECP160R1
    if ((mAllowedCurves & kWeaveCurveSet_secp160r1) != 0)
        req.AlternateCurveIds[req.AlternateCurveCount++] = kWeaveCurveId_secp160r1;
#endif // WEAVE_CONFIG_SUPPORT_ELLIPTIC_CURVE_SECP160R1
}

WEAVE_ERROR WeaveCASEEngine::GenerateBeginSessionRequest(BeginSessionRequestMessage& req, PacketBuffer *msgBuf)
{
    WEAVE_ERROR err;

    // Verify there isn't a begin session already outstanding.
    VerifyOrExit(State == kState_Idle, err = WEAVE_ERROR_INCORRECT_STATE);

    // Verify the auth delegate has been set.
    VerifyOrExit(AuthDelegate != NULL, err = WEAVE_ERROR_NO_CASE_AUTH_DELEGATE);

    WeaveLogDetail(SecurityManager, "CASE:GenerateBeginSessionRequest");

    // If a protocol config wasn't specified, choose a default one.
    if (req.ProtocolConfig == CASE::kCASEConfig_NotSpecified)
        req.ProtocolConfig = IsConfig2Allowed() ? kCASEConfig_Config2 : kCASEConfig_Config1;

    // Verify the proposed config is supported.
    VerifyOrExit(IsAllowedConfig(req.ProtocolConfig), err = WEAVE_ERROR_UNSUPPORTED_CASE_CONFIGURATION);

    // If a ECDH curve wasn't specified, choose a default one.
    if (req.CurveId == kWeaveCurveId_NotSpecified)
        req.CurveId = WEAVE_CONFIG_DEFAULT_CASE_CURVE_ID;

    // Verify the proposed ECDH curve is supported.
    VerifyOrExit(IsAllowedCurve(req.CurveId), err = WEAVE_ERROR_UNSUPPORTED_ELLIPTIC_CURVE);

    // Verify the requested key type.
    VerifyOrExit(WeaveKeyId::IsSessionKey(req.SessionKeyId), err = WEAVE_ERROR_WRONG_KEY_TYPE);

    // Verify the requested encryption type.
    VerifyOrExit(req.EncryptionType == kWeaveEncryptionType_AES128CTRSHA1,
            err = WEAVE_ERROR_UNSUPPORTED_ENCRYPTION_TYPE);

    SetIsInitiator(true);

    // Remember various parameters of the session so that we can use them when the responder responds.
    SetSelectedConfig(req.ProtocolConfig);
    mCurveId = req.CurveId;
    SetPerformingKeyConfirm(req.PerformKeyConfirm);
    SessionKeyId = req.SessionKeyId;
    EncryptionType = req.EncryptionType;

    // Since the message contains a number of variable length fields with corresponding length values,
    // we start encoding in the middle of the message, and then go back and fill in the head when we know
    // the final lengths.
    msgBuf->SetDataLength(req.HeadLength());

    // Generate an ephemeral public/private key. Store the public key directly into the message and store
    // the private key in a state variable.
    err = AppendNewECDHKey(req, msgBuf);
    SuccessOrExit(err);

    // Append the local node's certificate information.
    err = AppendCertInfo(req, msgBuf);
    SuccessOrExit(err);

    // Append the initiator's payload, if specified.
    err = AppendPayload(req, msgBuf);
    SuccessOrExit(err);

    // Now encode the head of the message.
    err = req.EncodeHead(msgBuf);
    SuccessOrExit(err);

    // Sign the message using the supplied private key and append the signature to the buffer.
    // Save the message hash for later use in deriving the session keys.
    err = AppendSignature(req, msgBuf, mSecureState.BeforeKeyGen.RequestMsgHash);
    SuccessOrExit(err);

    State = kState_BeginRequestGenerated;

exit:
    return err;
}

WEAVE_ERROR WeaveCASEEngine::ProcessBeginSessionRequest(PacketBuffer *msgBuf, BeginSessionRequestMessage& req, ReconfigureMessage& reconf)
{
    WEAVE_ERROR err;
    bool reconfigRequired = false;

    // Verify there isn't a begin session already outstanding.
    VerifyOrExit(State == kState_Idle, err = WEAVE_ERROR_INCORRECT_STATE);

    WeaveLogDetail(SecurityManager, "CASE:ProcessBeginSessionRequest");

    // Record that we are acting as the responder.
    SetIsInitiator(false);

    // Decode the head of the message.
    err = req.DecodeHead(msgBuf);
    SuccessOrExit(err);

    // Verify the protocol parameters proposed by the peer (protocol config and ECDH curve id).  If the proposed
    // values are not acceptable, but an alternative set are, setup the ReconfigureMessage with the alternatives
    // and return WEAVE_ERROR_CASE_RECONFIG_REQUIRED.
    reconf.ProtocolConfig = req.ProtocolConfig;
    reconf.CurveId = req.CurveId;
    err = VerifyProposedConfig(req, reconf.ProtocolConfig);
    if (err == WEAVE_ERROR_CASE_RECONFIG_REQUIRED)
        reconfigRequired = true;
    else
        SuccessOrExit(err);
    err = VerifyProposedCurve(req, reconf.CurveId);
    if (err == WEAVE_ERROR_CASE_RECONFIG_REQUIRED)
        reconfigRequired = true;
    else
        SuccessOrExit(err);
    if (reconfigRequired)
        ExitNow(err = WEAVE_ERROR_CASE_RECONFIG_REQUIRED);

    // Remember various parameters of the session so that we can use them when we respond.
    SetSelectedConfig(req.ProtocolConfig);
    mCurveId = req.CurveId;
    SetPerformingKeyConfirm(req.PerformKeyConfirm);
    SessionKeyId = req.SessionKeyId;
    EncryptionType = req.EncryptionType;

    // Verify the message signature.
    err = VerifySignature(req, msgBuf, mSecureState.BeforeKeyGen.RequestMsgHash);
    SuccessOrExit(err);

    // Verify the requested key type.
    VerifyOrExit(WeaveKeyId::IsSessionKey(req.SessionKeyId), err = WEAVE_ERROR_WRONG_KEY_TYPE);

    // Verify the requested encryption type.
    VerifyOrExit(req.EncryptionType == kWeaveEncryptionType_AES128CTRSHA1,
                 err = WEAVE_ERROR_UNSUPPORTED_ENCRYPTION_TYPE);

    State = kState_BeginRequestProcessed;

exit:
    if (err != WEAVE_NO_ERROR)
        State = kState_Failed;
    return err;
}

WEAVE_ERROR WeaveCASEEngine::GenerateBeginSessionResponse(BeginSessionResponseMessage& resp, PacketBuffer *msgBuf,
                                                          BeginSessionRequestMessage& req)
{
    WEAVE_ERROR err;
    uint8_t respMsgHash[kMaxHashLength];

    // Verify the correct state.
    VerifyOrExit(State == kState_BeginRequestProcessed, err = WEAVE_ERROR_INCORRECT_STATE);

    WeaveLogDetail(SecurityManager, "CASE:GenerateBeginSessionResponse");

    // If the initiator requested key confirmation, then we must do it.
    // If the initiator DIDN'T requested key confirmation, but local policy requires it, the we force the use
    // of key confirmation.
    if (req.PerformKeyConfirm || resp.PerformKeyConfirm || ResponderRequiresKeyConfirm())
        SetPerformingKeyConfirm(true);

    // If performing key confirmation, signal so in the response and set the appropriate key confirmation
    // hash length based on the selected config.
    if (PerformingKeyConfirm())
    {
        resp.PerformKeyConfirm = true;
        resp.KeyConfirmHashLength = ConfigHashLength();
    }

    // Since the message contains variable length fields with corresponding length values, start encoding the tail
    // of the message, and then go back and fill in the head when we know the final lengths.
    msgBuf->SetDataLength(resp.HeadLength());

    // Generate an ephemeral public/private key. Append the public key to the message and store the private key
    // in a state variable.
    err = AppendNewECDHKey(resp, msgBuf);
    SuccessOrExit(err);

    // Append the local node's certificate information.
    err = AppendCertInfo(resp, msgBuf);
    SuccessOrExit(err);

    // Append the responder's payload, if specified.
    err = AppendPayload(resp, msgBuf);
    SuccessOrExit(err);

    // Now encode the head of the message.
    err = resp.EncodeHead(msgBuf);
    SuccessOrExit(err);

    // Sign the message using the supplied private key and append the signature to the buffer.
    err = AppendSignature(resp, msgBuf, respMsgHash);
    SuccessOrExit(err);

    {
        uint8_t responderKeyConfirmHash[kMaxHashLength];

        // Derive the session keys from the initiator's public key and our private key.
        err = DeriveSessionKeys(req.ECDHPublicKey, respMsgHash, responderKeyConfirmHash);
        SuccessOrExit(err);

        // If performing key confirmation...
        if (PerformingKeyConfirm())
        {
            // Append the responder hash to the BeginSessionResponse message.  The initiator will use this to
            // confirm that we have the correct session keys.
            VerifyOrExit(msgBuf->AvailableDataLength() >= resp.KeyConfirmHashLength, err = WEAVE_ERROR_BUFFER_TOO_SMALL);
            uint16_t msgLen = msgBuf->DataLength();
            memcpy(msgBuf->Start() + msgLen, responderKeyConfirmHash, resp.KeyConfirmHashLength);
            msgBuf->SetDataLength(msgLen + resp.KeyConfirmHashLength);

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

WEAVE_ERROR WeaveCASEEngine::ProcessBeginSessionResponse(PacketBuffer *msgBuf, BeginSessionResponseMessage& resp)
{
    WEAVE_ERROR err;
    uint8_t respMsgHash[kMaxHashLength];

    VerifyOrExit(State == kState_BeginRequestGenerated, err = WEAVE_ERROR_INCORRECT_STATE);

    WeaveLogDetail(SecurityManager, "CASE:ProcessBeginSessionResponse");

    // Decode the head of the message.
    err = resp.DecodeHead(msgBuf);
    SuccessOrExit(err);

    // Verify the message signature.
    err = VerifySignature(resp, msgBuf, respMsgHash);
    SuccessOrExit(err);

    // If we asked for key confirmation, verify the responder responded appropriately.
    VerifyOrExit(!PerformingKeyConfirm() || resp.PerformKeyConfirm, err = WEAVE_ERROR_INVALID_CASE_PARAMETER);

    // If the responder asked for key confirmation, we have to do it.
    if (resp.PerformKeyConfirm)
        SetPerformingKeyConfirm(true);

    {
        uint8_t responderKeyConfirmHash[kMaxHashLength];

        // Derive the session keys from the responder's public key and our private key.
        err = DeriveSessionKeys(resp.ECDHPublicKey, respMsgHash, responderKeyConfirmHash);
        SuccessOrExit(err);

        // If performing key confirmation...
        if (PerformingKeyConfirm())
        {
            uint8_t expectedKeyConfirmHashLen = ConfigHashLength();

            // Check the expected responder hash against the value in the response message.
            // Fail if they do not match.

            WEAVE_FAULT_INJECT(nl::Weave::FaultInjection::kFault_CASEKeyConfirm, ExitNow(err = WEAVE_ERROR_KEY_CONFIRMATION_FAILED));

            VerifyOrExit(resp.KeyConfirmHashLength == expectedKeyConfirmHashLen &&
                         ConstantTimeCompare(resp.KeyConfirmHash, responderKeyConfirmHash, expectedKeyConfirmHashLen),
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

WEAVE_ERROR WeaveCASEEngine::GenerateInitiatorKeyConfirm(PacketBuffer *msgBuf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint8_t keyConfirmHashLen = ConfigHashLength();

    VerifyOrExit(State == kState_BeginResponseProcessed && PerformingKeyConfirm(), err = WEAVE_ERROR_INCORRECT_STATE);

    memcpy(msgBuf->Start(), mSecureState.AfterKeyGen.InitiatorKeyConfirmHash, keyConfirmHashLen);
    msgBuf->SetDataLength(keyConfirmHashLen);

    State = kState_Complete;

exit:
    if (err != WEAVE_NO_ERROR)
        State = kState_Failed;
    return err;
}

WEAVE_ERROR WeaveCASEEngine::ProcessInitiatorKeyConfirm(PacketBuffer *msgBuf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint8_t expectedKeyConfirmHashLen = ConfigHashLength();

    WeaveLogDetail(SecurityManager, "CASE:ProcessInitiatorKeyConfirm");

    VerifyOrExit(State == kState_BeginResponseGenerated && PerformingKeyConfirm(), err = WEAVE_ERROR_INCORRECT_STATE);

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

WEAVE_ERROR WeaveCASEEngine::ProcessReconfigure(PacketBuffer *msgBuf, ReconfigureMessage& reconf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    WeaveLogDetail(SecurityManager, "CASE:ProcessReconfigure");

    // Decode the Reconfigure message.
    err = ReconfigureMessage::Decode(msgBuf, reconf);
    SuccessOrExit(err);

    // Fail if the peer has sent more than one Reconfigure message.
    VerifyOrExit(!HasReconfigured(), err = WEAVE_ERROR_TOO_MANY_CASE_RECONFIGURATIONS);
    SetHasReconfigured(true);

    // Verify that the peer's proposed config is allowed.
    VerifyOrExit(IsAllowedConfig(reconf.ProtocolConfig), err = WEAVE_ERROR_UNSUPPORTED_CASE_CONFIGURATION);

    // Verify that the peer's proposed ECDH curve is allowed.
    VerifyOrExit(IsAllowedCurve(reconf.CurveId), err = WEAVE_ERROR_UNSUPPORTED_ELLIPTIC_CURVE);

    // Go back to Idle state so that the engine can be re-used to initiate a new CASE exchange.
    State = kState_Idle;

exit:
    return err;
}

WEAVE_ERROR WeaveCASEEngine::GetSessionKey(const WeaveEncryptionKey *& encKey)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(kState_Complete, err = WEAVE_ERROR_INCORRECT_STATE);

    encKey = &mSecureState.AfterKeyGen.EncryptionKey;

exit:
    return err;
}

WEAVE_ERROR WeaveCASEEngine::VerifyProposedConfig(BeginSessionRequestMessage& req, uint32_t& selectedAltConfig)
{
    WEAVE_ERROR err = WEAVE_ERROR_UNSUPPORTED_CASE_CONFIGURATION;

    WeaveLogDetail(SecurityManager, "CASE:VerifyProposedConfig");

    // If Config2 is allowed, use it if it is the proposed config.
    // If Config2 is NOT the proposed config, but is in the peer's list
    // of alternates, force a reconfig selecting Config2.
    if (IsConfig2Allowed())
    {
        if (req.ProtocolConfig == kCASEConfig_Config2)
            ExitNow(err = WEAVE_NO_ERROR);
        if (req.IsAltConfig(kCASEConfig_Config2))
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
        if (req.ProtocolConfig == kCASEConfig_Config1)
            ExitNow(err = WEAVE_NO_ERROR);

        // If Config1 is in the peer's list of alternates, force a reconfig selecting Config1.
        if (req.IsAltConfig(kCASEConfig_Config1))
        {
            selectedAltConfig = kCASEConfig_Config1;
            ExitNow(err = WEAVE_ERROR_CASE_RECONFIG_REQUIRED);
        }
    }

exit:
    return err;
}

WEAVE_ERROR WeaveCASEEngine::VerifyProposedCurve(BeginSessionRequestMessage& req, uint32_t& selectedAltCurve)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    WeaveLogDetail(SecurityManager, "CASE:VerifyProposedCurve");

    // If the requested elliptic curve is not allowed, select an allowed alternate, if available.
    if (!IsAllowedCurve(req.CurveId))
    {
        uint8_t i = 0;
        for (; i < req.AlternateCurveCount; i++)
            if (IsAllowedCurve(req.AlternateCurveIds[i]))
                break;

        VerifyOrExit(i < req.AlternateCurveCount, err = WEAVE_ERROR_UNSUPPORTED_ELLIPTIC_CURVE);

        selectedAltCurve = req.AlternateCurveIds[i];
        ExitNow(err = WEAVE_ERROR_CASE_RECONFIG_REQUIRED);
    }

exit:
    return err;
}

WEAVE_ERROR WeaveCASEEngine::AppendNewECDHKey(BeginSessionMessageBase& msg, PacketBuffer *msgBuf)
{
    WEAVE_ERROR err;
    EncodedECPrivateKey privKey;
    uint16_t msgLen = msgBuf->DataLength();

    WeaveLogDetail(SecurityManager, "CASE:AppendNewECDHKey");

    // Generate an ephemeral public/private key. Store the public key directly into the message and store
    // the private key in the provided object.
    msg.ECDHPublicKey.ECPoint = msgBuf->Start() + msgLen;
    msg.ECDHPublicKey.ECPointLen = msgBuf->AvailableDataLength(); // GenerateECDHKey() will update with final length.
    privKey.PrivKey = mSecureState.BeforeKeyGen.ECDHPrivateKey;
    privKey.PrivKeyLen = sizeof(mSecureState.BeforeKeyGen.ECDHPrivateKey);
    err = GenerateECDHKey(WeaveCurveIdToOID(msg.CurveId), msg.ECDHPublicKey, privKey);
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
        msg.ECDHPublicKey.ECPointLen = msgBuf->AvailableDataLength();
        err = GetCurveG(WeaveCurveIdToOID(msg.CurveId), msg.ECDHPublicKey);
        SuccessOrExit(err);

        WeaveLogError(SecurityManager, "WARNING: Using well-known ECDH key in CASE ***** SESSION IS NOT SECURE *****");
    }

#endif // WEAVE_CONFIG_SECURITY_TEST_MODE

    mSecureState.BeforeKeyGen.ECDHPrivateKeyLength = privKey.PrivKeyLen;

    msgBuf->SetDataLength(msgLen + msg.ECDHPublicKey.ECPointLen);

exit:
    return err;
}

WEAVE_ERROR WeaveCASEEngine::AppendCertInfo(BeginSessionMessageBase& msg, PacketBuffer *msgBuf)
{
    WEAVE_ERROR err;
    uint16_t msgLen = msgBuf->DataLength();

    WeaveLogDetail(SecurityManager, "CASE:AppendCertInfo");

    // Using the auth delegate, generate the certificate information for the local node and append it to the message.
    err = AuthDelegate->GetNodeCertInfo(IsInitiator(), msgBuf->Start() + msgLen, msgBuf->AvailableDataLength(), msg.CertInfoLength);
    SuccessOrExit(err);
    msgBuf->SetDataLength(msgLen + msg.CertInfoLength);

exit:
    return err;
}

WEAVE_ERROR WeaveCASEEngine::AppendPayload(BeginSessionMessageBase& msg, PacketBuffer *msgBuf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint16_t msgLen = msgBuf->DataLength();

    WeaveLogDetail(SecurityManager, "CASE:AppendPayload");

    err = AuthDelegate->GetNodePayload(IsInitiator(), msgBuf->Start() + msgLen, msgBuf->AvailableDataLength(), msg.PayloadLength);
    SuccessOrExit(err);

    msgBuf->SetDataLength(msgLen + msg.PayloadLength);

exit:
    return err;
}

// Generate a signature for an encoded CASE message (in the supplied buffer) and append it to the message.
WEAVE_ERROR WeaveCASEEngine::AppendSignature(BeginSessionMessageBase& msg, PacketBuffer *msgBuf, uint8_t *msgHash)
{
    WEAVE_ERROR err;
    uint32_t privKeyCurveId;
    EncodedECPublicKey pubKey;
    EncodedECPrivateKey privKey;
    EncodedECDSASignature ecdsaSig;
    uint8_t ecdsaRBuf[EncodedECDSASignature::kMaxValueLength];
    uint8_t ecdsaSBuf[EncodedECDSASignature::kMaxValueLength];
    uint8_t *msgStart = msgBuf->Start();
    uint16_t signedDataLen = msgBuf->DataLength();
    const uint8_t *signingKey;
    uint16_t signingKeyLen;

    WeaveLogDetail(SecurityManager, "CASE:AppendSignature");

    // Generate a hash of the signed portion of the message.
    GenerateHash(msgStart, signedDataLen, msgHash);

    WeaveLogDetail(SecurityManager, "CASE:GetNodePrivateKey");

    // Get the private key to sign the message.
    err = AuthDelegate->GetNodePrivateKey(IsInitiator(), signingKey, signingKeyLen);
    SuccessOrExit(err);

    // Decode the supplied private key.
    err = DecodeWeaveECPrivateKey(signingKey, signingKeyLen, privKeyCurveId, pubKey, privKey);
    SuccessOrExit(err);

    // Use temporary buffers to hold the generated signature value until we write it to the end of the message.
    ecdsaSig.R = ecdsaRBuf;
    ecdsaSig.RLen = sizeof(ecdsaRBuf);
    ecdsaSig.S = ecdsaSBuf;
    ecdsaSig.SLen = sizeof(ecdsaSBuf);

    WeaveLogDetail(SecurityManager, "CASE:GenerateECDSASignature");

    // Generate the signature for the message based on its hash.
    err = GenerateECDSASignature(WeaveCurveIdToOID(privKeyCurveId), msgHash, ConfigHashLength(), privKey, ecdsaSig);
    SuccessOrExit(err);

    err = AuthDelegate->ReleaseNodePrivateKey(signingKey);
    SuccessOrExit(err);

    // Append a CASE signature object to the end of the message buffer.
    {
        TLVWriter writer;
        writer.Init(msgBuf);
        writer.ImplicitProfileId = kWeaveProfile_Security;

        WeaveLogDetail(SecurityManager, "CASE:EncodeWeaveECDSASignature");

        err = EncodeWeaveECDSASignature(writer, ecdsaSig, ProfileTag(kWeaveProfile_Security, kTag_WeaveCASESignature));
        SuccessOrExit(err);

        err = writer.Finalize();
        SuccessOrExit(err);

        msg.Signature = msgStart + signedDataLen;
        msg.SignatureLength = writer.GetLengthWritten();
    }

exit:
    return err;
}

// Verify a CASE message signature (for the message in the supplied buffer) against a given public key.
// Returns a hash of the signed portion of the message in the supplied bufer.
WEAVE_ERROR WeaveCASEEngine::VerifySignature(BeginSessionMessageBase& msg, PacketBuffer *msgBuf, uint8_t *msgHash)
{
    WEAVE_ERROR err, validRes;
    WeaveCertificateSet certSet;
    ValidationContext certValidContext;
    WeaveDN peerCertDN;
    CertificateKeyId peerCertSubjectKeyId;
    WeaveCertificateData *peerCert = NULL;
    EncodedECDSASignature ecdsaSig;
    TLVReader reader;
    uint16_t signedDataLen;
    uint8_t *msgStart = msgBuf->Start();
    bool callEndCertValidation = false;

    WeaveLogDetail(SecurityManager, "CASE:VerifySignature");

    // Invoke the auth delegate to prepare the certificate set and the validation context.
    // This will load the trust anchors into the certificate set and establish the
    // desired validation criteria for the peer's entity certificate.
    err = AuthDelegate->BeginCertValidation(IsInitiator(), certSet, certValidContext);
    SuccessOrExit(err);
    callEndCertValidation = true;

    // If the cert type property has been set, set it as the required certificate type in the
    // validation context such that the cert type is enforced during the call to FindValidCert().
    certValidContext.RequiredCertType = mCertType;

    WeaveLogDetail(SecurityManager, "CASE:DecodeCertificateInfo");

    // Decode the certificate information supplied by the peer.
    certValidContext.RequiredKeyUsages |= kKeyUsageFlag_DigitalSignature;
    validRes = DecodeCertificateInfo(msg, certSet, peerCertDN, peerCertSubjectKeyId);

    // If decoding was successful, search the certificate set for the peer's certificate and
    // validate that it is trusted and suitable for CASE authentication.
    if (validRes == WEAVE_NO_ERROR)
    {
        WeaveLogDetail(SecurityManager, "CASE:ValidateCert");
        validRes = certSet.FindValidCert(peerCertDN, peerCertSubjectKeyId, certValidContext, peerCert);
    }

    // If a valid certificate was found for the peer...
    if (validRes == WEAVE_NO_ERROR)
    {
        // Update the cert type property with the type of the peer's certificate.
        mCertType = peerCert->CertType;
    }

    // Tell the auth delegate about the results of basic certificate validation and let it weigh in
    // on the process.  Note that this may alter the validation result (validRes) either from successful
    // to unsuccessful, or in rare cases, from unsuccessful to successful.
    err = AuthDelegate->HandleCertValidationResult(IsInitiator(), validRes, peerCert, msg.PeerNodeId, certSet, certValidContext);
    SuccessOrExit(err);

    // Check that validation succeeded.
    VerifyOrExit((validRes != WEAVE_ERROR_UNSUPPORTED_ELLIPTIC_CURVE &&
                  validRes != WEAVE_ERROR_UNSUPPORTED_SIGNATURE_TYPE),
                  err = WEAVE_ERROR_UNSUPPORTED_CERT_FORMAT);
    VerifyOrExit(validRes == WEAVE_NO_ERROR, err = validRes);
    VerifyOrExit(peerCert != NULL, err = WEAVE_ERROR_INCORRECT_STATE);

    // Decode the CASE signature from the end of the message.
    reader.Init(msg.Signature, msg.SignatureLength);
    reader.ImplicitProfileId = kWeaveProfile_Security;
    err = reader.Next(kTLVType_Structure, ProfileTag(kWeaveProfile_Security, kTag_WeaveCASESignature));
    SuccessOrExit(err);
    WeaveLogDetail(SecurityManager, "CASE:DecodeWeaveECDSASignature");
    err = DecodeWeaveECDSASignature(reader, ecdsaSig);
    SuccessOrExit(err);
    err = reader.VerifyEndOfContainer();
    SuccessOrExit(err);

    // Compute the length of the signed portion of the message.
    signedDataLen = msg.Signature - msgStart;

    // Generate a hash of the signed portion of the message.
    GenerateHash(msgStart, signedDataLen, msgHash);

    // Verify the message signature against the computed message hash and the
    // the public key from the peer's certificate.
    WeaveLogDetail(SecurityManager, "CASE:VerifyECDSASignature");
    err = VerifyECDSASignature(WeaveCurveIdToOID(peerCert->PubKeyCurveId), msgHash, ConfigHashLength(),
                               ecdsaSig, peerCert->PublicKey.EC);
    SuccessOrExit(err);

exit:
    if (callEndCertValidation)
    {
        WEAVE_ERROR endErr = AuthDelegate->EndCertValidation(certSet, certValidContext);
        if (err == WEAVE_NO_ERROR)
            err = endErr;
    }
    certSet.Release();
    return err;
}

WEAVE_ERROR WeaveCASEEngine::DecodeCertificateInfo(BeginSessionMessageBase& msg, WeaveCertificateSet& certSet,
                                                   WeaveDN& entityCertDN, CertificateKeyId& entityCertSubjectKeyId)
{
    WEAVE_ERROR err;
    TLVReader reader;
    WeaveCertificateData *entityCert = NULL;

    // Begin decoding the certificate information structure.
    reader.Init(msg.CertInfo, msg.CertInfoLength);
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
        // and thus its that or nothing.
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

WEAVE_ERROR WeaveCASEEngine::DeriveSessionKeys(EncodedECPublicKey& pubKey, const uint8_t *respMsgHash,
                                               uint8_t *responderKeyConfirmHash)
{
    WEAVE_ERROR err;
    uint8_t hashLen = ConfigHashLength();
#if WEAVE_CONFIG_SUPPORT_CASE_CONFIG1
    HKDFSHA1Or256 hkdf(IsUsingConfig1());
#else
    HKDFSHA256 hkdf;
#endif

    WeaveLogDetail(SecurityManager, "CASE:DeriveSessionKeys");

    // Only AES128CTRSHA1 keys supported for now.
    VerifyOrExit(EncryptionType == kWeaveEncryptionType_AES128CTRSHA1, err = WEAVE_ERROR_UNSUPPORTED_ENCRYPTION_TYPE);

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
        uint8_t sessionKeyData[WeaveEncryptionKey_AES128CTRSHA1::KeySize + kMaxHashLength];
        uint16_t keyLen;

        // If performing key confirmation, arrange to generate enough key data for the session
        // keys (data encryption and integrity) as well as a key to be used in key confirmation.
        if (PerformingKeyConfirm())
            keyLen = WeaveEncryptionKey_AES128CTRSHA1::KeySize + hashLen;
        else
            keyLen = WeaveEncryptionKey_AES128CTRSHA1::KeySize;

        // Perform HKDF-based key expansion to produce the desired key data.
        err = hkdf.ExpandKey(NULL, 0, keyLen, sessionKeyData);
        SuccessOrExit(err);

#ifdef CASE_PRINT_CRYPTO_DATA
        printf("Session Key Data: "); PrintHex(sessionKeyData, keyLen); printf("\n");
#endif

        // Copy the generated key data to the appropriate destinations.
        memcpy(mSecureState.AfterKeyGen.EncryptionKey.AES128CTRSHA1.DataKey,
               sessionKeyData,
               WeaveEncryptionKey_AES128CTRSHA1::DataKeySize);
        memcpy(mSecureState.AfterKeyGen.EncryptionKey.AES128CTRSHA1.IntegrityKey,
               sessionKeyData + WeaveEncryptionKey_AES128CTRSHA1::DataKeySize,
               WeaveEncryptionKey_AES128CTRSHA1::IntegrityKeySize);

        // If performing key confirmation...
        if (PerformingKeyConfirm())
        {
            // Use the key confirmation key to generate key confirmation hashes. Store the initiator hash
            // (the single hash) in state data for later use.  Return the responder hash (the double hash)
            // to the caller.
            uint8_t *keyConfirmKey = sessionKeyData + WeaveEncryptionKey_AES128CTRSHA1::KeySize;
            GenerateKeyConfirmHashes(keyConfirmKey, mSecureState.AfterKeyGen.InitiatorKeyConfirmHash,
                                     responderKeyConfirmHash);
        }

        ClearSecretData(sessionKeyData, sizeof(sessionKeyData));
    }

exit:
    return err;
}

void WeaveCASEEngine::GenerateHash(const uint8_t *inData, uint16_t inDataLen, uint8_t *hash)
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

void WeaveCASEEngine::GenerateKeyConfirmHashes(const uint8_t *keyConfirmKey, uint8_t *singleHash, uint8_t *doubleHash)
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


} // namespace CASE
} // namespace Security
} // namespace Profiles
} // namespace Weave
} // namespace nl
