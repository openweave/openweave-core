/*
 *
 *    Copyright (c) 2015-2017 Nest Labs, Inc.
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
 *      This file implements objects for initiators and responders for
 *      the Weave Token Authenticated Key Exchange (TAKE) protocol.
 *
 */

#include <Weave/Core/WeaveCore.h>
#include "WeaveTAKE.h"
#include "WeaveSecurity.h"
#include <Weave/Support/crypto/WeaveCrypto.h>
#include <Weave/Support/crypto/HashAlgos.h>
#include <Weave/Support/crypto/EllipticCurve.h>
#include <Weave/Support/CodeUtils.h>
#include <Weave/Support/ErrorStr.h>
#include <Weave/Support/ASN1.h>
#include <Weave/Support/crypto/WeaveCrypto.h>
#include <Weave/Support/crypto/AESBlockCipher.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace Security {
namespace TAKE {

using namespace nl::Weave::Encoding;
using namespace nl::Weave::Crypto;
using namespace nl::Weave::ASN1;

void WeaveTAKEEngine::Init()
{
    State = kState_Reset;
    KeyState = kEncryptionKeyState_Uninitialized;
    SessionKeyId = WeaveKeyId::kNone;
}

void WeaveTAKEEngine::Shutdown()
{
    ClearSecretData(IdentificationKey, sizeof(IdentificationKey));
    ClearSecretData(AuthenticationKey, sizeof(AuthenticationKey));
    ClearSecretData(ECDHPrivateKeyBuffer, sizeof(ECDHPrivateKeyBuffer));
    State = kState_Reset;
    KeyState = kEncryptionKeyState_Uninitialized;
    SessionKeyId = WeaveKeyId::kNone;
}

static WEAVE_ERROR PackControlHeader(uint8_t numOptionalConfigurations, bool encryptAuthPhase, bool encryptCommPhase, bool timeLimitedIK, bool hasChallengerId, uint8_t& controlHeader)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(numOptionalConfigurations < kMaxOptionalConfigurations, err = WEAVE_ERROR_INVALID_ARGUMENT);
    controlHeader = (numOptionalConfigurations << kControlHeader_NumOptionalConfigurationShift) & kControlHeader_NumOptionalConfigurationMask;

    if (encryptAuthPhase)
        controlHeader |= kControlHeader_EncryptAuthenticationPhaseFlag;
    if (encryptCommPhase)
        controlHeader |= kControlHeader_EncryptCommunicationsPhaseFlag;
    if (timeLimitedIK)
        controlHeader |= kControlHeader_TimeLimitFlag;
    if (hasChallengerId)
        controlHeader |= kControlHeader_HasChallengerIdFlag;

exit:
    return err;
}

uint8_t WeaveTAKEEngine::GetNumOptionalConfigurations() const
{
    return static_cast<uint8_t>((ControlHeader & kControlHeader_NumOptionalConfigurationMask) >> kControlHeader_NumOptionalConfigurationShift);
}

bool WeaveTAKEEngine::IsEncryptAuthPhase() const
{
    return (ControlHeader & kControlHeader_EncryptAuthenticationPhaseFlag) != 0;
}

bool WeaveTAKEEngine::IsEncryptCommPhase() const
{
    return (ControlHeader & kControlHeader_EncryptCommunicationsPhaseFlag) != 0;
}

bool WeaveTAKEEngine::IsTimeLimitedIK() const
{
    return (ControlHeader & kControlHeader_TimeLimitFlag) != 0;
}

bool WeaveTAKEEngine::HasSentChallengerId() const
{
    return (ControlHeader & kControlHeader_HasChallengerIdFlag) != 0;
}

WEAVE_ERROR WeaveTAKEEngine::GenerateIdentifyTokenMessage(uint16_t sessionKeyId, uint8_t takeConfig, bool encryptAuthPhase, bool encryptCommPhase, bool timeLimitedIK,
                                                              bool sendChallengerId, uint8_t encryptionType, uint64_t localNodeId, PacketBuffer *msgBuf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint8_t* p = msgBuf->Start();
    uint16_t msgLen;

    VerifyOrExit(State == kState_Reset || State == kState_InitiatorReconfigureProcessed, err = WEAVE_ERROR_INCORRECT_STATE);

    PackControlHeader(0, encryptAuthPhase, encryptCommPhase, timeLimitedIK, sendChallengerId, ControlHeader);

    if (sendChallengerId)
    {
        ChallengerIdLen = kMaxChallengerIdSize;
        err = ChallengerAuthDelegate->GetChallengerID(ChallengerId, ChallengerIdLen);
        SuccessOrExit(err);

        VerifyOrExit(ChallengerIdLen <= kMaxChallengerIdSize, err = WEAVE_ERROR_INVALID_ARGUMENT);
    }
    else
    {
        uint8_t* ChallengerIdPtr = ChallengerId;
        LittleEndian::Write64(ChallengerIdPtr, localNodeId);
        ChallengerIdLen = 8;
    }

    msgLen = kIdentifyTokenMsgMinSize + (sendChallengerId ? 1 + ChallengerIdLen : 0) + GetNumOptionalConfigurations() + (UseSessionKey() ? 2 : 0);
    VerifyOrExit(msgBuf->AvailableDataLength() >= msgLen, err = WEAVE_ERROR_BUFFER_TOO_SMALL);

    err = Platform::Security::GetSecureRandomData(ChallengerNonce, kNonceSize);
    SuccessOrExit(err);

    *p++ = ControlHeader;

    if (sendChallengerId)
    {
        *p++ = ChallengerIdLen;
    }

    EncryptionType = encryptionType;
    *p++ = EncryptionType;

    VerifyOrExit(takeConfig == kTAKEConfig_Config1, err = WEAVE_ERROR_UNSUPPORTED_TAKE_CONFIGURATION);
    ProtocolConfig = takeConfig;
    *p++ = ProtocolConfig;

    // No optional configuration for now

    if (UseSessionKey())
    {
        SessionKeyId = sessionKeyId;
        LittleEndian::Write16(p, SessionKeyId);
    }

    if (sendChallengerId)
    {
        WriteArray(ChallengerId, p, ChallengerIdLen);
    }

    WriteArray(ChallengerNonce, p, kNonceSize);

    msgBuf->SetDataLength(msgLen);
    State = kState_InitiatorIdentifyTokenGenerated;

exit:
    return err;
}

WEAVE_ERROR WeaveTAKEEngine::ProcessIdentifyTokenMessage(uint64_t peerNodeId, const PacketBuffer *msgBuf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint16_t msgLen = msgBuf->DataLength();
    const uint8_t* p = msgBuf->Start();

    VerifyOrExit(msgLen >= kIdentifyTokenMsgMinSize, err = WEAVE_ERROR_MESSAGE_INCOMPLETE);
    VerifyOrExit(State == kState_Reset, err = WEAVE_ERROR_INCORRECT_STATE);

    ControlHeader = *p++;

    if (HasSentChallengerId())
    {
        ChallengerIdLen = *p++;
        VerifyOrExit(ChallengerIdLen <= kMaxChallengerIdSize, err = WEAVE_ERROR_INVALID_ARGUMENT);
    }
    else
    {
        uint8_t* ChallengerIdPtr = ChallengerId;
        LittleEndian::Write64(ChallengerIdPtr, peerNodeId);
        ChallengerIdLen = 8;
    }

    VerifyOrExit(msgLen == (kIdentifyTokenMsgMinSize + (HasSentChallengerId() ? 1 + ChallengerIdLen : 0) + GetNumOptionalConfigurations() + (UseSessionKey() ? 2 : 0)), err = WEAVE_ERROR_MESSAGE_INCOMPLETE);

    EncryptionType = *p++;
    VerifyOrExit(EncryptionType == kWeaveEncryptionType_AES128CTRSHA1, err = WEAVE_ERROR_UNSUPPORTED_ENCRYPTION_TYPE);

    ProtocolConfig = *p++;

    ReadArray(OptionalConfigurations, p, GetNumOptionalConfigurations() * sizeof(uint8_t));

    // Choosing the first valid configuration, for now choosing the first one which is Config1
    ChosenConfiguration = ProtocolConfig;
    if (ChosenConfiguration != kTAKEConfig_Config1)
    {
        for (int i = 0; i < GetNumOptionalConfigurations(); i++)
        {
            if (OptionalConfigurations[i] == kTAKEConfig_Config1)
            {
                ChosenConfiguration = OptionalConfigurations[i];
                break;
            }
        }
    }

    VerifyOrExit(ChosenConfiguration == kTAKEConfig_Config1, err = WEAVE_ERROR_TAKE_RECONFIGURE_REQUIRED);

    if (UseSessionKey())
    {
        SessionKeyId = LittleEndian::Read16(p);
    }

    if (HasSentChallengerId())
    {
        ReadArray(ChallengerId, p, ChallengerIdLen);
    }

    ReadArray(ChallengerNonce, p, kNonceSize);

    State = kState_ResponderIdentifyTokenProcessed;

exit:
    return err;
}

WEAVE_ERROR WeaveTAKEEngine::GenerateIdentifyTokenResponseMessage(PacketBuffer *msgBuf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    Platform::Security::SHA1 sha1;
    uint8_t* p = msgBuf->Start();
    uint8_t identificationRootKey[kIdentificationRootKeySize];
    HKDFSHA1 hkdf;
    uint8_t keySaltLen = ChallengerIdLen + sizeof(uint32_t);
    uint8_t keySalt[kMaxIdentifyTokenResponseKeySaltSize];

    VerifyOrExit(State == kState_ResponderIdentifyTokenProcessed, err = WEAVE_ERROR_INCORRECT_STATE);
    VerifyOrExit(msgBuf->AvailableDataLength() >= kIdentifyTokenResponseMsgSize, err = WEAVE_ERROR_BUFFER_TOO_SMALL);

    err = Platform::Security::GetSecureRandomData(TokenNonce, kNonceSize);
    SuccessOrExit(err);

    err = TokenAuthDelegate->GetIdentificationRootKey(identificationRootKey);
    SuccessOrExit(err);

    *p++ = ChosenConfiguration;

    WriteArray(TokenNonce, p, kNonceSize);

    {
        uint8_t* keySaltPtr = keySalt;
        WriteArray(ChallengerId, keySaltPtr, ChallengerIdLen);
        if (!IsTimeLimitedIK())
        {
            WriteArray(kSaltTimeUnlimitedIdentificationKey, keySaltPtr, sizeof(kSaltTimeUnlimitedIdentificationKey));
        }
        else
        {
            uint32_t time;
            err = TokenAuthDelegate->GetTAKETime(time);
            SuccessOrExit(err);

            LittleEndian::Write32(keySaltPtr, time);
        }

        hkdf.BeginExtractKey(keySalt, keySaltLen);

        hkdf.AddKeyMaterial(identificationRootKey, kIdentificationRootKeySize);

        err = hkdf.FinishExtractKey();
        SuccessOrExit(err);

        err = hkdf.ExpandKey(NULL, 0, kIdentificationKeySize, IdentificationKey);
        SuccessOrExit(err);
    }

    GenerateHMACSignature(IdentificationKey, p);

    msgBuf->SetDataLength(kIdentifyTokenResponseMsgSize);

    if (UseSessionKey())
    {
        err = GenerateProtocolEncryptionKey();
        SuccessOrExit(err);
    }

    State = kState_ResponderIdentifyTokenResponseGenerated;

exit:
    return err;
}

WEAVE_ERROR WeaveTAKEEngine::ProcessIdentifyTokenResponseMessage(const PacketBuffer *msgBuf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint16_t msgLen = msgBuf->DataLength();
    const uint8_t* p = msgBuf->Start();
    bool isAuthorisedIK = false;
    int i, end;
    uint8_t takeConfig;
    uint16_t authKeyLen = kAuthenticationKeySize;
    uint16_t encAuthBlobLen = kTokenEncryptedStateSize;

    VerifyOrExit(msgLen == kIdentifyTokenResponseMsgSize, err = WEAVE_ERROR_MESSAGE_INCOMPLETE);
    VerifyOrExit(State == kState_InitiatorIdentifyTokenGenerated, err = WEAVE_ERROR_INCORRECT_STATE);

    ChosenConfiguration = *p++;

    ReadArray(TokenNonce, p, kNonceSize);

    if (ChosenConfiguration != ProtocolConfig)
    {
        bool isProposedConfiguration = false;
        for (i = 0, end = GetNumOptionalConfigurations(); i < end; i++)
        {
            if (ChosenConfiguration == OptionalConfigurations[i])
            {
                isProposedConfiguration = true;
                break;
            }
        }
        VerifyOrExit(isProposedConfiguration, err = WEAVE_ERROR_UNSUPPORTED_TAKE_CONFIGURATION);
    }

    uint8_t hmacBuffer[kConfig1_HMACSignatureSize];

    ChallengerAuthDelegate->RewindIdentificationKeyIterator();

    while (1)
    {
        uint16_t identificationKeyLen = kIdentificationKeySize; // TODO unused

        err = ChallengerAuthDelegate->GetNextIdentificationKey(TokenId, IdentificationKey, identificationKeyLen);
        SuccessOrExit(err);

        if (TokenId == kNodeIdNotSpecified)
        {
            VerifyOrExit(identificationKeyLen == kIdentificationKeySize, err = WEAVE_ERROR_INVALID_ARGUMENT);
            break;
        }
        GenerateHMACSignature(IdentificationKey, hmacBuffer);

        if (ConstantTimeCompare(hmacBuffer, p, kConfig1_HMACSignatureSize))
        {
            isAuthorisedIK = true;
            break;
        }
    }

    VerifyOrExit(isAuthorisedIK, err = WEAVE_ERROR_TAKE_TOKEN_IDENTIFICATION_FAILED);

    if (UseSessionKey())
    {
        err = GenerateProtocolEncryptionKey();
        SuccessOrExit(err);
    }

    err = ChallengerAuthDelegate->GetTokenAuthData(TokenId, takeConfig, AuthenticationKey, authKeyLen, EncryptedAuthenticationKey, encAuthBlobLen);
    SuccessOrExit(err);

    VerifyOrExit(authKeyLen == kAuthenticationKeySize, err = WEAVE_ERROR_INVALID_ARGUMENT);
    VerifyOrExit(encAuthBlobLen == kTokenEncryptedStateSize, err = WEAVE_ERROR_INVALID_ARGUMENT);

    if (takeConfig == ChosenConfiguration)
    {
        err = WEAVE_ERROR_TAKE_REAUTH_POSSIBLE;
    }

    State = kState_InitiatorIdentifyTokenResponseProcessed;

exit:
    return err;
}

WEAVE_ERROR WeaveTAKEEngine::GenerateTokenReconfigureMessage(PacketBuffer *msgBuf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint8_t* p = msgBuf->Start();

    VerifyOrExit(State == kState_Reset, err = WEAVE_ERROR_INCORRECT_STATE);
    VerifyOrExit(msgBuf->AvailableDataLength() >= kTokenRecongfigureMsgSize, err = WEAVE_ERROR_BUFFER_TOO_SMALL);

    *p++ = kTAKEConfig_Config1;

    msgBuf->SetDataLength(kTokenRecongfigureMsgSize);

    State = kState_ResponderDone;

exit:
    return err;
}

WEAVE_ERROR WeaveTAKEEngine::ProcessTokenReconfigureMessage(uint8_t& config, const PacketBuffer *msgBuf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint16_t msgLen = msgBuf->DataLength();
    const uint8_t* p = msgBuf->Start();

    VerifyOrExit(msgLen == kTokenRecongfigureMsgSize, err = WEAVE_ERROR_MESSAGE_INCOMPLETE);
    VerifyOrExit(State == kState_InitiatorIdentifyTokenGenerated, err = WEAVE_ERROR_INCORRECT_STATE);

    config = *p++;

    VerifyOrExit(config == kTAKEConfig_Config1, err = WEAVE_ERROR_UNSUPPORTED_TAKE_CONFIGURATION);

    State = kState_InitiatorReconfigureProcessed;

exit:
    return err;
}

WEAVE_ERROR WeaveTAKEEngine::GenerateAuthenticateTokenMessage(PacketBuffer *msgBuf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint8_t* hmacSignature = msgBuf->Start();
    uint8_t* challengerECDHPublicKey = hmacSignature + kConfig1_HMACSignatureSize;
    uint16_t msgLen = kConfig1_HMACSignatureSize + GetECPointLen();

    VerifyOrExit(State == kState_InitiatorIdentifyTokenResponseProcessed, err = WEAVE_ERROR_INCORRECT_STATE);
    VerifyOrExit(msgBuf->AvailableDataLength() >= msgLen, err = WEAVE_ERROR_BUFFER_TOO_SMALL);

    EncodedECPrivateKey privKey;
    EncodedECPublicKey pubKey;
    pubKey.ECPoint = ECDHPublicKeyBuffer;
    pubKey.ECPointLen = sizeof(ECDHPublicKeyBuffer);
    privKey.PrivKey = ECDHPrivateKeyBuffer;
    privKey.PrivKeyLen = sizeof(ECDHPrivateKeyBuffer);
    err = GenerateECDHKey(GetCurveOID(), pubKey, privKey);
    SuccessOrExit(err);

    ECDHPrivateKeyLength = privKey.PrivKeyLen;

    memcpy(challengerECDHPublicKey, ECDHPublicKeyBuffer, GetECPointLen());

    GenerateHMACSignature(IdentificationKey, hmacSignature, ECDHPublicKeyBuffer, GetECPointLen());

    msgBuf->SetDataLength(msgLen);

    State = kState_InitiatorAuthenticateTokenGenerated;

exit:
    return err;
}

WEAVE_ERROR WeaveTAKEEngine::ProcessAuthenticateTokenMessage(const PacketBuffer *msgBuf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint16_t msgLen = msgBuf->DataLength();
    uint8_t* p = msgBuf->Start();
    bool res;
    uint8_t* challengerECDHPublicKey = p + kConfig1_HMACSignatureSize;

    VerifyOrExit(msgLen >= kAuthenticateTokenMsgMinSize, err = WEAVE_ERROR_MESSAGE_INCOMPLETE);
    VerifyOrExit(State == kState_ResponderIdentifyTokenResponseGenerated, err = WEAVE_ERROR_INCORRECT_STATE);

    uint8_t hmacBuffer[kConfig1_HMACSignatureSize];

    GenerateHMACSignature(IdentificationKey, hmacBuffer, challengerECDHPublicKey, GetECPointLen());

    res = ConstantTimeCompare(hmacBuffer, p, kConfig1_HMACSignatureSize);
    VerifyOrExit(res == true, err = WEAVE_ERROR_INVALID_TAKE_PARAMETER);

    memcpy(ECDHPublicKeyBuffer, challengerECDHPublicKey, GetECPointLen());

    State = kState_ResponderAuthenticateTokenProcessed;

exit:
    return err;
}

WEAVE_ERROR WeaveTAKEEngine::GenerateAuthenticateTokenResponseMessage(PacketBuffer *msgBuf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    Platform::Security::AES256BlockCipherEnc aes256BlockCipherEnc;
    uint8_t *encryptedState = msgBuf->Start();
    uint8_t *tokenECDHPublicKey =  encryptedState + kTokenEncryptedStateSize;
    uint8_t *ecdsaSignature = tokenECDHPublicKey + GetECPointLen();
    EncodedECPrivateKey tokenPrivKey;
    union
    {
        uint8_t tokenMasterKey[kTokenMasterKeySize];
        uint8_t tokenPrivateKeyBuffer[kMaxTokenPrivateKeySize];
    };
    tokenPrivKey.PrivKey = tokenPrivateKeyBuffer;
    tokenPrivKey.PrivKeyLen = kMaxTokenPrivateKeySize;
    OID tokenPrivKeyOID;
    uint16_t curveSize;
    uint16_t msgLen = kTokenEncryptedStateSize + kMaxECDSASignatureSize + GetECPointLen();

    VerifyOrExit(State == kState_ResponderAuthenticateTokenProcessed, err = WEAVE_ERROR_INCORRECT_STATE);
    VerifyOrExit(msgBuf->AvailableDataLength() >= msgLen, err = WEAVE_ERROR_BUFFER_TOO_SMALL);

    EncodedECPrivateKey privKey;
    EncodedECPublicKey pubKey;
    pubKey.ECPoint = tokenECDHPublicKey;
    pubKey.ECPointLen = GetECPointLen();
    privKey.PrivKey = ECDHPrivateKeyBuffer;
    privKey.PrivKeyLen = sizeof(ECDHPrivateKeyBuffer);
    err = GenerateECDHKey(GetCurveOID(), pubKey, privKey);
    SuccessOrExit(err);

    err = GenerateAuthenticationKey(ChallengerId, ECDHPrivateKeyBuffer, ECDHPublicKeyBuffer, privKey.PrivKeyLen);
    SuccessOrExit(err);

    err = TokenAuthDelegate->GetTokenMasterKey(tokenMasterKey);
    SuccessOrExit(err);

    aes256BlockCipherEnc.SetKey(tokenMasterKey);
    aes256BlockCipherEnc.EncryptBlock(AuthenticationKey, encryptedState);

    err = TokenAuthDelegate->GetTokenPrivateKey(tokenPrivKeyOID, tokenPrivKey);
    SuccessOrExit(err);

    curveSize = GetCurveSize(tokenPrivKeyOID);
    VerifyOrExit(curveSize != 0, err = WEAVE_ERROR_UNSUPPORTED_ELLIPTIC_CURVE);

    err = GenerateSignatureForAuthenticateTokenResponse(ecdsaSignature, ECDHPublicKeyBuffer, tokenECDHPublicKey, tokenPrivKey, encryptedState, tokenPrivKeyOID);
    SuccessOrExit(err);

    msgBuf->SetDataLength(kTokenEncryptedStateSize + 2 * curveSize + GetECPointLen());

    State = kState_ResponderAuthenticateTokenResponseGenerated;

exit:
    ClearSecretData(tokenMasterKey, sizeof(tokenMasterKey));
    ClearSecretData(tokenPrivateKeyBuffer, sizeof(tokenPrivateKeyBuffer));
    ClearSecretData(ECDHPrivateKeyBuffer, sizeof(ECDHPrivateKeyBuffer));
    aes256BlockCipherEnc.Reset();

    return err;
}

WEAVE_ERROR WeaveTAKEEngine::ProcessAuthenticateTokenResponseMessage(const PacketBuffer *msgBuf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint16_t msgLen = msgBuf->DataLength();
    uint8_t *encryptedState = msgBuf->Start();
    uint8_t *tokenECDHPublicKey =  encryptedState + kTokenEncryptedStateSize;
    uint8_t *ecdsaSignature = tokenECDHPublicKey + GetECPointLen();
    EncodedECPublicKey encodedPubKey;
    uint8_t TPub[GetECPointLen()];
    OID tokenPubKeyOID;
    uint16_t curveSize;

    VerifyOrExit(State == kState_InitiatorAuthenticateTokenGenerated, err = WEAVE_ERROR_INCORRECT_STATE);

    encodedPubKey.ECPoint = TPub;
    encodedPubKey.ECPointLen = GetECPointLen();

    err = ChallengerAuthDelegate->GetTokenPublicKey(TokenId, tokenPubKeyOID, encodedPubKey);
    SuccessOrExit(err);

    curveSize = GetCurveSize(tokenPubKeyOID);
    VerifyOrExit(curveSize != 0, err = WEAVE_ERROR_UNSUPPORTED_ELLIPTIC_CURVE);

    VerifyOrExit(msgLen == kAuthenticateTokenResponseMsgMinSize + 2 * curveSize + GetECPointLen(), err = WEAVE_ERROR_MESSAGE_INCOMPLETE);

    err = GenerateAuthenticationKey(ChallengerId, ECDHPrivateKeyBuffer, tokenECDHPublicKey, ECDHPrivateKeyLength);
    SuccessOrExit(err);

    err = ChallengerAuthDelegate->StoreTokenAuthData(TokenId, ChosenConfiguration, AuthenticationKey, kAuthenticationKeySize, encryptedState, kTokenEncryptedStateSize);
    SuccessOrExit(err);

    err = VerifySignatureForAuthenticateTokenResponse(ecdsaSignature, ECDHPublicKeyBuffer, tokenECDHPublicKey, encryptedState, tokenPubKeyOID, encodedPubKey);
    SuccessOrExit(err);

    State = kState_InitiatorAuthenticateTokenResponseProcessed;

exit:
    return err;
}

WEAVE_ERROR WeaveTAKEEngine::GenerateReAuthenticateTokenMessage(PacketBuffer *msgBuf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint8_t* tokenEncryptedState = msgBuf->Start();
    uint8_t* hmacSignature = tokenEncryptedState + kTokenEncryptedStateSize;

    VerifyOrExit(State == kState_InitiatorIdentifyTokenResponseProcessed, err = WEAVE_ERROR_INCORRECT_STATE);
    VerifyOrExit(msgBuf->AvailableDataLength() >= kReAuthenticateTokenMsgSize, err = WEAVE_ERROR_BUFFER_TOO_SMALL);

    memcpy(tokenEncryptedState, EncryptedAuthenticationKey, kTokenEncryptedStateSize);

    GenerateHMACSignature(IdentificationKey, hmacSignature, tokenEncryptedState, kTokenEncryptedStateSize);

    msgBuf->SetDataLength(kReAuthenticateTokenMsgSize);

    State = kState_InitiatorReAuthenticateTokenGenerated;

exit:
    return err;
}

WEAVE_ERROR WeaveTAKEEngine::GenerateProtocolEncryptionKey()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint8_t sessionKey[WeaveEncryptionKey_AES128CTRSHA1::KeySize];
    HKDFSHA1 hkdf;

    uint8_t keySaltLen = sizeof(ControlHeader) + sizeof(EncryptionType) + sizeof(ProtocolConfig) +
        sizeof(uint8_t) * GetNumOptionalConfigurations() + sizeof(SessionKeyId) + sizeof(ChosenConfiguration) + kNonceSize + kNonceSize +
        sizeof(kSaltProtocolEncryption);
    uint8_t keySalt[kMaxProtocolEncryptionKeySaltSize];
    uint8_t* p = keySalt;

    WriteArray(&ControlHeader, p, sizeof(ControlHeader));
    WriteArray(&EncryptionType, p, sizeof(EncryptionType));
    WriteArray(&ProtocolConfig, p, sizeof(ProtocolConfig));
    WriteArray(OptionalConfigurations, p, sizeof(uint8_t) * GetNumOptionalConfigurations());
    LittleEndian::Write16(p, SessionKeyId);
    WriteArray(&ChosenConfiguration, p, sizeof(ChosenConfiguration));
    WriteArray(ChallengerNonce, p, kNonceSize);
    WriteArray(TokenNonce, p, kNonceSize);
    WriteArray(kSaltProtocolEncryption, p, sizeof(kSaltProtocolEncryption));

    hkdf.BeginExtractKey(keySalt, keySaltLen);

    hkdf.AddKeyMaterial(IdentificationKey, kIdentificationKeySize);

    err = hkdf.FinishExtractKey();
    SuccessOrExit(err);

    err = hkdf.ExpandKey(NULL, 0, WeaveEncryptionKey_AES128CTRSHA1::KeySize, sessionKey);
    SuccessOrExit(err);

    memcpy(EncryptionKey.AES128CTRSHA1.DataKey,
        sessionKey,
        WeaveEncryptionKey_AES128CTRSHA1::DataKeySize);
    memcpy(EncryptionKey.AES128CTRSHA1.IntegrityKey,
        sessionKey + WeaveEncryptionKey_AES128CTRSHA1::DataKeySize,
        WeaveEncryptionKey_AES128CTRSHA1::IntegrityKeySize);

    KeyState = kEncryptionKeyState_Initialized;

exit:
    ClearSecretData(sessionKey, sizeof(sessionKey));

    return err;
}

WEAVE_ERROR WeaveTAKEEngine::ProcessReAuthenticateTokenMessage(const PacketBuffer *msgBuf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint16_t msgLen = msgBuf->DataLength();
    uint8_t* tokenEncryptedState = msgBuf->Start();
    uint8_t* hmacSignature = tokenEncryptedState + kTokenEncryptedStateSize;
    bool res;
    uint8_t tokenMasterKey[kTokenMasterKeySize];
    Platform::Security::AES256BlockCipherDec aes256BlockCipherDec;

    VerifyOrExit(msgLen == kReAuthenticateTokenMsgSize, err = WEAVE_ERROR_MESSAGE_INCOMPLETE);
    VerifyOrExit(State == kState_ResponderIdentifyTokenResponseGenerated, err = WEAVE_ERROR_INCORRECT_STATE);

    uint8_t hmacBuffer[kConfig1_HMACSignatureSize];
    GenerateHMACSignature(IdentificationKey, hmacBuffer, tokenEncryptedState, kTokenEncryptedStateSize);

    res = ConstantTimeCompare(hmacBuffer, hmacSignature, kConfig1_HMACSignatureSize);
    VerifyOrExit(res == true, err = WEAVE_ERROR_INVALID_SIGNATURE);

    err = TokenAuthDelegate->GetTokenMasterKey(tokenMasterKey);
    SuccessOrExit(err);

    aes256BlockCipherDec.SetKey(tokenMasterKey);
    aes256BlockCipherDec.DecryptBlock(tokenEncryptedState, AuthenticationKey);

    State = kState_ResponderReAuthenticateTokenProcessed;

exit:
    ClearSecretData(tokenMasterKey, sizeof(tokenMasterKey));
    aes256BlockCipherDec.Reset();

    return err;
}

WEAVE_ERROR WeaveTAKEEngine::GenerateReAuthenticateTokenResponseMessage(PacketBuffer *msgBuf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    Crypto::HMACSHA1 hmac;
    uint8_t* p = msgBuf->Start();

    VerifyOrExit(State == kState_ResponderReAuthenticateTokenProcessed, err = WEAVE_ERROR_INCORRECT_STATE);
    VerifyOrExit(msgBuf->AvailableDataLength() >= kReAuthenticateTokenResponseMsgSize, err = WEAVE_ERROR_BUFFER_TOO_SMALL);

    GenerateHMACSignature(AuthenticationKey, p, NULL, 0, kAuthenticationKeySize);

    msgBuf->SetDataLength(kReAuthenticateTokenResponseMsgSize);

    State = kState_ResponderReAuthenticateTokenResponseGenerated;

exit:
    return err;
}

WEAVE_ERROR WeaveTAKEEngine::ProcessReAuthenticateTokenResponseMessage(const PacketBuffer *msgBuf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint8_t* hmacSignature = msgBuf->Start();
    uint16_t msgLen = msgBuf->DataLength();
    bool res;

    VerifyOrExit(msgLen == kReAuthenticateTokenResponseMsgSize, err = WEAVE_ERROR_MESSAGE_INCOMPLETE);
    VerifyOrExit(State == kState_InitiatorReAuthenticateTokenGenerated, err = WEAVE_ERROR_INCORRECT_STATE);

    uint8_t hmacBuffer[kConfig1_HMACSignatureSize];
    GenerateHMACSignature(AuthenticationKey, hmacBuffer, NULL, 0, kAuthenticationKeySize);

    res = ConstantTimeCompare(hmacBuffer, hmacSignature, kConfig1_HMACSignatureSize);
    VerifyOrExit(res == true, err = WEAVE_ERROR_INVALID_SIGNATURE);

    State = kState_InitiatorReAuthenticateTokenResponseProcessed;

exit:
    return err;
}

WEAVE_ERROR WeaveTAKEEngine::GetSessionKey(const WeaveEncryptionKey *& encKey) const
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(KeyState == kEncryptionKeyState_Initialized, err = WEAVE_ERROR_INCORRECT_STATE);

    encKey = &EncryptionKey;

exit:
    return err;
}

uint8_t WeaveTAKEEngine::GetEncryptionType()
{
    return EncryptionType;
}

void WeaveTAKEEngine::GenerateHMACSignature(const uint8_t* key, uint8_t* dest, const uint8_t* additionalField, uint8_t additionalFieldLength, uint16_t keyLength)
{
    Crypto::HMACSHA1 hmac;

    uint8_t SessionKeyIdArray[sizeof(SessionKeyId)];
    LittleEndian::Put16(SessionKeyIdArray, SessionKeyId);

    hmac.Begin(key, keyLength);
    hmac.AddData(&ControlHeader, sizeof(ControlHeader));
    hmac.AddData(&EncryptionType, sizeof(EncryptionType));
    hmac.AddData(&ProtocolConfig, sizeof(ProtocolConfig));
    hmac.AddData(OptionalConfigurations, sizeof(uint8_t) * GetNumOptionalConfigurations());
    hmac.AddData(SessionKeyIdArray, sizeof(SessionKeyId));
    hmac.AddData(&ChosenConfiguration, sizeof(ChosenConfiguration));
    hmac.AddData(ChallengerNonce, kNonceSize);
    hmac.AddData(TokenNonce, kNonceSize);
    if (additionalFieldLength != 0)
    {
        hmac.AddData(additionalField, additionalFieldLength);
    }
    hmac.Finish(dest);
}

WEAVE_ERROR WeaveTAKEEngine::GenerateAuthenticationKey(const uint8_t* challengerId, uint8_t* privateKey, uint8_t* publicKey, uint16_t privateKeyLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    HKDFSHA1 hkdf;
    uint8_t sharedSecret[kMaxCurveSize];
    uint16_t sharedSecretLen;
    uint8_t keySaltLen = ChallengerIdLen + sizeof(ControlHeader) + sizeof(EncryptionType) + sizeof(ProtocolConfig) +
        sizeof(uint8_t) * GetNumOptionalConfigurations() + sizeof(SessionKeyId) + sizeof(ChosenConfiguration) + kNonceSize + kNonceSize;
    uint8_t keySalt[kMaxAuthenticationKeySaltSize];
    uint8_t* p = keySalt;

    EncodedECPrivateKey encodedPrivKey;
    EncodedECPublicKey encodedPubKey;

    encodedPubKey.ECPoint = publicKey;
    encodedPubKey.ECPointLen = GetECPointLen();
    encodedPrivKey.PrivKey = privateKey;
    encodedPrivKey.PrivKeyLen = privateKeyLen;

    err = ECDHComputeSharedSecret(GetCurveOID(), encodedPubKey, encodedPrivKey, sharedSecret, sizeof(sharedSecret), sharedSecretLen);
    SuccessOrExit(err);

    WriteArray(challengerId, p, ChallengerIdLen);
    WriteArray(&ControlHeader, p, sizeof(ControlHeader));
    WriteArray(&EncryptionType, p, sizeof(EncryptionType));
    WriteArray(&ProtocolConfig, p, sizeof(ProtocolConfig));
    WriteArray(OptionalConfigurations, p, sizeof(uint8_t) * GetNumOptionalConfigurations());
    LittleEndian::Write16(p, SessionKeyId);
    WriteArray(&ChosenConfiguration, p, sizeof(ChosenConfiguration));
    WriteArray(ChallengerNonce, p, kNonceSize);
    WriteArray(TokenNonce, p, kNonceSize);

    hkdf.BeginExtractKey(keySalt, keySaltLen);
    hkdf.AddKeyMaterial(sharedSecret, sharedSecretLen);

    err = hkdf.FinishExtractKey();
    SuccessOrExit(err);

    err = hkdf.ExpandKey(NULL, 0, kAuthenticationKeySize, AuthenticationKey);
    SuccessOrExit(err);

exit:
    ClearSecretData(sharedSecret, sizeof(sharedSecret));

    return err;
}

void WeaveTAKEEngine::GenerateHashForAuthenticateTokenResponse(uint8_t* dest, const uint8_t* challengerECDHPublicKey, const uint8_t* tokenECDHPublicKey, const uint8_t* encryptedState)
{
    Platform::Security::SHA1 sha1;

    uint8_t SessionKeyIdArray[sizeof(SessionKeyId)];
    LittleEndian::Put16(SessionKeyIdArray, SessionKeyId);

    sha1.Begin();
    sha1.AddData(&ControlHeader, sizeof(ControlHeader));
    sha1.AddData(&EncryptionType, sizeof(EncryptionType));
    sha1.AddData(&ProtocolConfig, sizeof(ProtocolConfig));
    sha1.AddData(OptionalConfigurations, sizeof(uint8_t) * GetNumOptionalConfigurations());
    sha1.AddData(SessionKeyIdArray, sizeof(SessionKeyId));
    sha1.AddData(&ChosenConfiguration, sizeof(ChosenConfiguration));
    sha1.AddData(ChallengerNonce, kNonceSize);
    sha1.AddData(TokenNonce, kNonceSize);
    sha1.AddData(challengerECDHPublicKey, GetECPointLen());
    sha1.AddData(tokenECDHPublicKey, GetECPointLen());
    sha1.AddData(encryptedState, kTokenEncryptedStateSize);
    sha1.Finish(dest);
}

WEAVE_ERROR WeaveTAKEEngine::GenerateSignatureForAuthenticateTokenResponse(uint8_t* dest, const uint8_t* challengerECDHPublicKey, const uint8_t* tokenECDHPublicKey,
                                                                           EncodedECPrivateKey TPriv, const uint8_t* encryptedState, OID& curveOID)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint8_t messageHash[kConfig1_HMACSignatureSize];

    GenerateHashForAuthenticateTokenResponse(messageHash, challengerECDHPublicKey, tokenECDHPublicKey, encryptedState);

    err = GenerateECDSASignature(curveOID, messageHash, kConfig1_HMACSignatureSize, TPriv, dest);
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR WeaveTAKEEngine::VerifySignatureForAuthenticateTokenResponse(const uint8_t* signature, const uint8_t* challengerECDHPublicKey, const uint8_t* tokenECDHPublicKey,
                                                                         const uint8_t* encryptedState, OID& curveOID, EncodedECPublicKey& encodedPubKey)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint8_t messageHash[kConfig1_HMACSignatureSize];

    GenerateHashForAuthenticateTokenResponse(messageHash, challengerECDHPublicKey, tokenECDHPublicKey, encryptedState);

    err = VerifyECDSASignature(curveOID, messageHash, kConfig1_HMACSignatureSize, signature, encodedPubKey);
    SuccessOrExit(err);

exit:
    return err;
}

void WeaveTAKEEngine::ReadArray(uint8_t* dest, const uint8_t*& src, uint8_t length)
{
    memcpy(dest, src, length);
    src += length;
}

void WeaveTAKEEngine::WriteArray(const uint8_t* src, uint8_t*& dest, uint8_t length)
{
    memcpy(dest, src, length);
    dest += length;
}

bool WeaveTAKEEngine::UseSessionKey() const
{
    return IsEncryptAuthPhase() || IsEncryptCommPhase();
}

uint16_t WeaveTAKEEngine::GetCurveLen() const
{
    // NOTE: Should be reviewed/updated when new TAKE Configs are introduced
    return kConfig1_CurveSize;
}

uint16_t WeaveTAKEEngine::GetPrivKeyLen() const
{
    // NOTE: Should be reviewed/updated when new TAKE Configs are introduced
    return kConfig1_PrivKeySize;
}

uint16_t WeaveTAKEEngine::GetECPointLen() const
{
    // NOTE: Should be reviewed/updated when new TAKE Configs are introduced
    return kConfig1_ECPointX962FormatSize;
}

OID WeaveTAKEEngine::GetCurveOID() const
{
    // NOTE: Should be reviewed/updated when new TAKE Configs are introduced
    return kOID_EllipticCurve_secp224r1;
}

} // namespace TAKE
} // namespace Security
} // namespace Profiles
} // namespace Weave
} // namespace nl
