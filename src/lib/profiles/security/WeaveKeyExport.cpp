/*
 *
 *    Copyright (c) 2016-2017 Nest Labs, Inc.
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
 *      This file implements key export protocol engine.
 *
 */

#include <Weave/Core/WeaveCore.h>
#include "WeaveKeyExport.h"
#include "WeaveSecurity.h"
#include <Weave/Profiles/security/WeaveSig.h>
#include <Weave/Support/crypto/WeaveCrypto.h>
#include <Weave/Support/CodeUtils.h>
#include <Weave/Profiles/WeaveProfiles.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace Security {
namespace KeyExport {

using namespace nl::Inet;
using namespace nl::Weave::Encoding;
using namespace nl::Weave::Crypto;
using namespace nl::Weave::Platform::Security;
using namespace nl::Weave::TLV;
using namespace nl::Weave::ASN1;

void WeaveKeyExport::Init(WeaveKeyExportDelegate *keyExportDelegate, GroupKeyStoreBase *groupKeyStore)
{
    State = kState_Reset;
    KeyExportDelegate = keyExportDelegate;
    GroupKeyStore = groupKeyStore;
}

void WeaveKeyExport::Shutdown(void)
{
    Reset();
}

void WeaveKeyExport::Reset(void)
{
    State = kState_Reset;
    ClearSecretData(ECDHPrivateKey, sizeof(ECDHPrivateKey));
}

bool WeaveKeyExport::IsInitiator() const
{
    return (State >= kState_InitiatorGeneratingRequest && State <= kState_InitiatorDone);
}

// Returns true when input config is allowed protocol Config.
bool WeaveKeyExport::IsAllowedConfig(uint8_t config) const
{
    bool retVal;

    if (config == kKeyExportConfig_Unspecified || config > kKeyExportConfig_ConfigLast)
        retVal = false;
    else
        retVal = (((0x01 << (config - 1)) & mAllowedConfigs) != 0x00);

    return retVal;
}

// Generate alternate configs list.
// This method is called by initiator of the key export protocol.
WEAVE_ERROR WeaveKeyExport::GenerateAltConfigsList(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    mAltConfigsCount = 0;
    for (uint8_t config = kKeyExportConfig_Config1; config <= kKeyExportConfig_ConfigLast; config++)
    {
        if (IsAllowedConfig(config) && (config != ProtocolConfig) && (mAltConfigsCount < kMaxAltConfigsCount))
        {
            // Check that ProtocolConfig has valid configuration.
            if (IsAllowedConfig(ProtocolConfig))
            {
                mAltConfigs[mAltConfigsCount] = config;
                mAltConfigsCount++;
            }
            // Otherwise, update ProtocolConfig to a valid configuration.
            else
            {
                ProtocolConfig = config;
            }
        }
    }

    // Check that ProtocolConfig has valid configuration.
    VerifyOrExit(IsAllowedConfig(ProtocolConfig), err = WEAVE_ERROR_INVALID_KEY_EXPORT_CONFIGURATION);

exit:
    return err;
}

// Check whether proposed configuration is allowed. If not then choose allowed
// configuration from the list of alternative configuration and generate reconfiguration
// request. If valid configuration is not found in the list of alternative configurations
// then generate error.
// This method is called by responder of the key export protocol.
WEAVE_ERROR WeaveKeyExport::ValidateProtocolConfig(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // If proposed protocol configuration is not allowed.
    if (!IsAllowedConfig(ProtocolConfig))
    {
        for (uint8_t i = 0; i < mAltConfigsCount; i++)
        {
            if (IsAllowedConfig(mAltConfigs[i]))
            {
                // If valid configuration is found in the list then request reconfiguration.
                ProtocolConfig = mAltConfigs[i];
                ExitNow(err = WEAVE_ERROR_KEY_EXPORT_RECONFIGURE_REQUIRED);
            }
        }

        // Reaching this point means that no valid configuration was found and error is generated.
        ExitNow(err = WEAVE_ERROR_NO_COMMON_KEY_EXPORT_CONFIGURATIONS);
    }

exit:
    return err;
}

WEAVE_ERROR WeaveKeyExport::GenerateKeyExportRequest(uint8_t *buf, uint16_t bufSize, uint16_t& msgLen, uint8_t proposedConfig,
                                                     uint32_t keyId, bool signMessages)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint8_t controlHeader;
    uint8_t *bufStart = buf;

    // Verify correct state.
    VerifyOrExit(State == kState_Reset ||
                 State == kState_InitiatorReconfigureProcessed, err = WEAVE_ERROR_INCORRECT_STATE);

    // Set new protocol state.
    State = kState_InitiatorGeneratingRequest;

    // Initialize Parameters.
    KeyId = keyId;
    ProtocolConfig = proposedConfig;
    SignMessages = signMessages;

    // Generate list of alternate configs.
    err = GenerateAltConfigsList();
    SuccessOrExit(err);

    // Calculate intermediate data size, which includes Control Header, Protocol Config,
    // Alternate Protocol Configs, Key Id, and ECDH Public Key sizes.
    msgLen = 2 * sizeof(uint8_t) + mAltConfigsCount + sizeof(uint32_t) + GetECDHPublicKeyLen();
    VerifyOrExit(msgLen <= bufSize, err = WEAVE_ERROR_BUFFER_TOO_SMALL);

    // Encode and write the control header fields.
    controlHeader = (mAltConfigsCount << kReqControlHeader_AltConfigCountShift) & kReqControlHeader_AltConfigCountMask;
    if (SignMessages)
        controlHeader |= kReqControlHeader_SignMessagesFlag;
    Write8(buf, controlHeader);

    // Write the protocol configuration field.
    Write8(buf, ProtocolConfig);

    // Write the alternate configurations field.
    for (uint8_t i = 0; i < mAltConfigsCount; i++)
        Write8(buf, mAltConfigs[i]);

    // Write the key id field.
    LittleEndian::Write32(buf, KeyId);

    // Generate an ephemeral ECDH public/private key pair. Store the public key directly into
    // the message buffer and store the private key in the object variable.
    err = AppendNewECDHKey(buf);
    SuccessOrExit(err);

    // Generate the ECDSA signature for the message based on its hash. Store the signature
    // directly into the message buffer.
    if (SignMessages)
    {
        err = AppendSignature(bufStart, bufSize, msgLen);
        SuccessOrExit(err);
    }

    // Set new protocol state.
    State = kState_InitiatorRequestGenerated;

exit:
    return err;
}

WEAVE_ERROR WeaveKeyExport::ProcessKeyExportRequest(const uint8_t *buf, uint16_t msgSize, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint8_t controlHeader;
    const uint8_t *bufStart = buf;
    uint16_t msgLen;

    // Verify correct state
    VerifyOrExit(State == kState_Reset, err = WEAVE_ERROR_INCORRECT_STATE);

    // Set new protocol state.
    State = kState_ResponderProcessingRequest;

    // Verify the key export delegate has been set.
    VerifyOrExit(KeyExportDelegate != NULL, err = WEAVE_ERROR_NO_KEY_EXPORT_DELEGATE);

    // Verify that the message has at least 2 bytes for Control Header and Protocol Config fields.
    msgLen = 2 * sizeof(uint8_t);
    VerifyOrExit(msgLen <= msgSize, err = WEAVE_ERROR_MESSAGE_INCOMPLETE);

    // Read the control header field.
    controlHeader = Read8(buf);

    // Verify correctness of the control header field.
    VerifyOrExit((controlHeader & kReqControlHeader_UnusedBits) == 0x00, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Decode number of alternate configuration in the message.
    mAltConfigsCount = (controlHeader & kReqControlHeader_AltConfigCountMask) >> kReqControlHeader_AltConfigCountShift;

    // Verify correctness of the control header field.
    VerifyOrExit(mAltConfigsCount <= kMaxAltConfigsCount, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Decode ECDSA signature flag.
    SignMessages = (controlHeader & kReqControlHeader_SignMessagesFlag) != 0;

    // Read the proposed protocol configuration field.
    ProtocolConfig = Read8(buf);

    // Verify that the message has sufficient size to read Alternate Protocol Configs field.
    msgLen += mAltConfigsCount;
    VerifyOrExit(msgLen <= msgSize, err = WEAVE_ERROR_MESSAGE_INCOMPLETE);

    // Read the list of alternate protocol configurations.
    for (uint8_t i = 0; i < mAltConfigsCount; i++)
        mAltConfigs[i] = Read8(buf);

    // Validate proposed protocol configuration. Function returns:
    //   WEAVE_ERROR_KEY_EXPORT_RECONFIGURE_REQUIRED - protocol reconfiguration
    //   is requested with one of the proposed alternative configurations.
    //   WEAVE_ERROR_NO_COMMON_KEY_EXPORT_CONFIGURATIONS - proposed and alternative
    //   configs are not allowed.
    //   WEAVE_NO_ERROR - proposed configuration is valid.
    err = ValidateProtocolConfig();
    SuccessOrExit(err);

    // Verify that the message has sufficient size to read Key Id and ECDH Public Key fields.
    msgLen += sizeof(uint32_t) + GetECDHPublicKeyLen();
    VerifyOrExit(msgLen <= msgSize, err = WEAVE_ERROR_MESSAGE_INCOMPLETE);

    // Read key id.
    KeyId = LittleEndian::Read32(buf);

    // Read requester ECDH public key.
    memcpy(ECDHPublicKey, buf, GetECDHPublicKeyLen());

    // Update buffer pointer.
    buf += GetECDHPublicKeyLen();

    // If message is signed.
    if (SignMessages)
    {
        // Verify the ECDSA signature for the message based on its hash.
        err = VerifySignature(bufStart, msgSize, msgLen, pktInfo, msgInfo);
    }
    // If message is not signed.
    else
    {
        // Verify that the requestor is authorized to export the key.
        err = KeyExportDelegate->ValidateUnsignedKeyExportMessage(IsInitiator(), pktInfo, msgInfo, KeyId);
    }
    SuccessOrExit(err);

    // Verify correct message length.
    VerifyOrExit(msgLen == msgSize, err = WEAVE_ERROR_INVALID_ARGUMENT);

exit:
    // This assignment is in the exit section because state should be also updated for the
    // reconfiguration case, where err == WEAVE_ERROR_KEY_EXPORT_RECONFIGURE_REQUIRED.
    State = kState_ResponderRequestProcessed;

    return err;
}

WEAVE_ERROR WeaveKeyExport::GenerateKeyExportResponse(uint8_t *buf, uint16_t bufSize, uint16_t& msgLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint16_t exportedKeyLen;
    uint8_t controlHeader;
    uint8_t *bufStart = buf;
    uint8_t *keyIdAndLenFields = buf + sizeof(uint8_t);

    // Verify correct state.
    VerifyOrExit(State == kState_ResponderRequestProcessed, err = WEAVE_ERROR_INCORRECT_STATE);

    // Verify that message buffer has enough memory to store control header, key Id, key length and responder ECDH public key fields.
    msgLen = sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint16_t) + GetECDHPublicKeyLen();
    VerifyOrExit(msgLen <= bufSize, err = WEAVE_ERROR_BUFFER_TOO_SMALL);

    // Encode and write the control header fields.
    if (SignMessages)
        controlHeader = kResControlHeader_SignMessagesFlag;
    else
        controlHeader = 0;
    Write8(buf, controlHeader);

    // Skip writing the key Id and key length fields.
    // These fields will be written after EncryptExportedKey() call sets their proper values.
    buf += sizeof(uint32_t) + sizeof(uint16_t);

    // Generate an ephemeral ECDH public/private key pair. Store the public key directly into
    // the message buffer and store the private key in the object variable.
    err = AppendNewECDHKey(buf);
    SuccessOrExit(err);

    // Encrypt exported key directly into the message buffer.
    err = EncryptExportedKey(buf, bufSize, msgLen, exportedKeyLen);
    SuccessOrExit(err);

    // Write the key id field.
    LittleEndian::Write32(keyIdAndLenFields, KeyId);

    // Write the key length field.
    LittleEndian::Put16(keyIdAndLenFields, exportedKeyLen);

    // Generate the ECDSA signature for the message based on its hash. Store the signature
    // directly into the message buffer.
    if (SignMessages)
    {
        err = AppendSignature(bufStart, bufSize, msgLen);
        SuccessOrExit(err);
    }

    // Set new protocol state.
    State = kState_ResponderDone;

exit:
    return err;
}

WEAVE_ERROR WeaveKeyExport::ProcessKeyExportResponse(const uint8_t *buf, uint16_t msgSize, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo,
                                                     uint8_t *exportedKeyBuf, uint16_t exportedKeyBufSize, uint16_t &exportedKeyLen, uint32_t &exportedKeyId)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint8_t controlHeader;
    bool signMessages;
    const uint8_t *bufStart = buf;
    uint16_t msgLen;

    // Verify correct state.
    VerifyOrExit(State == kState_InitiatorRequestGenerated, err = WEAVE_ERROR_INCORRECT_STATE);

    // Verify the key export delegate has been set.
    VerifyOrExit(KeyExportDelegate != NULL, err = WEAVE_ERROR_NO_KEY_EXPORT_DELEGATE);

    // Verify that message has control header, key Id and key length fields.
    msgLen = sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint16_t);
    VerifyOrExit(msgLen <= msgSize, err = WEAVE_ERROR_MESSAGE_INCOMPLETE);

    // Read the control header field.
    controlHeader = Read8(buf);

    // Verify correctness of the control header field.
    VerifyOrExit((controlHeader & kResControlHeader_UnusedBits) == 0x00, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Decode ECDSA signature flag.
    signMessages = (controlHeader & kResControlHeader_SignMessagesFlag) != 0;

    // Verify that the flag matches the original setting in the key export request message.
    VerifyOrExit(signMessages == SignMessages, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Read the key id field.
    exportedKeyId = LittleEndian::Read32(buf);

    // If requested key was of a logical current type.
    if (WeaveKeyId::UsesCurrentEpochKey(KeyId))
    {
        // Verify that received key Id matches requested logical current key type.
        VerifyOrExit(!WeaveKeyId::UsesCurrentEpochKey(exportedKeyId) &&
                     (KeyId == WeaveKeyId::ConvertToCurrentAppKeyId(exportedKeyId)), err = WEAVE_ERROR_INVALID_ARGUMENT);
    }
    // Otherwise, verify that received key Id matches requested key Id.
    else
    {
        VerifyOrExit(exportedKeyId == KeyId, err = WEAVE_ERROR_INVALID_ARGUMENT);
    }

    // Read the key length field.
    exportedKeyLen = LittleEndian::Read16(buf);

    // Verify the exported key buffer size is sufficient.
    VerifyOrExit(exportedKeyLen <= exportedKeyBufSize, err = WEAVE_ERROR_BUFFER_TOO_SMALL);

    // Verify that the message buffer is large enough to contain ECDH public key,
    // encrypted exported key and authenticator.
    // Note that these field will be used by the DecryptExportedKey() function
    // after message signature is verified by VerifySignature().
    msgLen += GetECDHPublicKeyLen() + exportedKeyLen + kExportedKeyAuthenticatorSize;
    VerifyOrExit(msgLen <= msgSize, err = WEAVE_ERROR_BUFFER_TOO_SMALL);

    // If message is signed.
    if (SignMessages)
    {
        // Verify the ECDSA signature for the message based on its hash.
        err = VerifySignature(bufStart, msgSize, msgLen, pktInfo, msgInfo);
    }
    // If message is not signed.
    else
    {
        // Verify that the requestor is authorized to export the key.
        err = KeyExportDelegate->ValidateUnsignedKeyExportMessage(IsInitiator(), pktInfo, msgInfo, KeyId);
    }
    SuccessOrExit(err);

    // Decrypt exported key directly from the message buffer.
    err = DecryptExportedKey(buf, exportedKeyBuf, exportedKeyLen);
    SuccessOrExit(err);

    // Verify correct message length.
    VerifyOrExit(msgLen == msgSize, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Set new protocol state.
    State = kState_InitiatorDone;

exit:
    return err;
}

WEAVE_ERROR WeaveKeyExport::GenerateKeyExportReconfigure(uint8_t *buf, uint16_t bufSize, uint16_t& msgLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // Verify correct state.
    VerifyOrExit(State == kState_ResponderRequestProcessed, err = WEAVE_ERROR_INCORRECT_STATE);

    // Verify that message buffer size is sufficient.
    VerifyOrExit(kKeyExportReconfigureMsgSize <= bufSize, err = WEAVE_ERROR_BUFFER_TOO_SMALL);

    // Verify that protocol config for proposed for reconfiguration is valid.
    VerifyOrExit(IsAllowedConfig(ProtocolConfig), err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Write alternative protocol config.
    Put8(buf, ProtocolConfig);

    // Set message length.
    msgLen = kKeyExportReconfigureMsgSize;

    // Set new protocol state.
    State = kState_ResponderDone;

exit:
    return err;
}

WEAVE_ERROR WeaveKeyExport::ProcessKeyExportReconfigure(const uint8_t *buf, uint16_t msgSize, uint8_t &config)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // Verify correct state.
    VerifyOrExit(State == kState_InitiatorRequestGenerated, err = WEAVE_ERROR_INCORRECT_STATE);

    // Verify correct message size.
    VerifyOrExit(msgSize == kKeyExportReconfigureMsgSize, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Read alternative proposed protocol config.
    config = Get8(buf);

    // Verify that proposed config is allowed.
    VerifyOrExit(IsAllowedConfig(config), err = WEAVE_ERROR_INVALID_KEY_EXPORT_CONFIGURATION);

    // Set new protocol state.
    State = kState_InitiatorReconfigureProcessed;

exit:
    return err;
}

WEAVE_ERROR WeaveKeyExport::AppendNewECDHKey(uint8_t *& buf)
{
    WEAVE_ERROR err;
    EncodedECPrivateKey ecdhPrivKey;
    EncodedECPublicKey ecdhPubKey;

    // Generate an ephemeral ECDH public/private key pair. Store the public key directly into
    // the message buffer and store the private key in the object variable.
    ecdhPubKey.ECPoint = buf;
    ecdhPubKey.ECPointLen = GetECDHPublicKeyLen();    // GenerateECDHKey() will update with the actual length.
    ecdhPrivKey.PrivKey = ECDHPrivateKey;
    ecdhPrivKey.PrivKeyLen = sizeof(ECDHPrivateKey);  // GenerateECDHKey() will update with the actual length.
    err = GenerateECDHKey(GetCurveOID(), ecdhPubKey, ecdhPrivKey);
    SuccessOrExit(err);

    // Update length of generated private key.
    ECDHPrivateKeyLen = ecdhPrivKey.PrivKeyLen;

    // Update buffer pointer.
    buf += ecdhPubKey.ECPointLen;

exit:
    return err;
}

// Generate a signature for the message (in the supplied buffer) and append it to the message.
WEAVE_ERROR WeaveKeyExport::AppendSignature(uint8_t *msgStart, uint16_t msgBufSize, uint16_t& msgLen)
{
    WEAVE_ERROR err;
    SHA256 sha256;
    uint8_t msgHash[SHA256::kHashLength];
    WeaveCertificateSet certSet;
    const uint8_t *signingKey;
    uint16_t signingKeyLen;
    uint8_t *msgSigStart;
    uint16_t msgSigLen;
    bool isInitiator = IsInitiator();
    bool callReleaseNodeCertSet = false;
    bool callReleaseNodePrivateKey = false;

    // Verify the key export delegate has been set.
    VerifyOrExit(KeyExportDelegate != NULL, err = WEAVE_ERROR_NO_KEY_EXPORT_DELEGATE);

    // Generate a SHA256 hash of the signed portion of the message.
    sha256.Begin();
    sha256.AddData(msgStart, msgLen);
    sha256.Finish(msgHash);

    // Get the certificate information for the local node.
    err = KeyExportDelegate->GetNodeCertSet(isInitiator, certSet);
    SuccessOrExit(err);
    callReleaseNodeCertSet = true;

    // Get the private key to sign the message.
    err = KeyExportDelegate->GetNodePrivateKey(isInitiator, signingKey, signingKeyLen);
    SuccessOrExit(err);
    callReleaseNodePrivateKey = true;

    // Set signature start and length parameters.
    msgSigStart = msgStart + msgLen;
    msgSigLen = msgBufSize - msgLen;

    // Generate WeaveSignature.
    err = GenerateWeaveSignature(msgHash, sizeof(msgHash),
                                 *certSet.LastCert(), certSet,
                                 signingKey, signingKeyLen,
                                 kOID_SigAlgo_ECDSAWithSHA256, kGenerateWeaveSignatureFlag_IncludeRelatedCertificates,
                                 msgSigStart, msgSigLen, msgSigLen);
    SuccessOrExit(err);

    // Update message size.
    msgLen += msgSigLen;

exit:
    if (callReleaseNodePrivateKey)
    {
        WEAVE_ERROR endErr = KeyExportDelegate->ReleaseNodePrivateKey(isInitiator, signingKey);
        if (err == WEAVE_NO_ERROR)
            err = endErr;
    }

    if (callReleaseNodeCertSet)
    {
        WEAVE_ERROR endErr = KeyExportDelegate->ReleaseNodeCertSet(isInitiator, certSet);
        if (err == WEAVE_NO_ERROR)
            err = endErr;
    }

    return err;
}

// Verify a key export message signature (for the message in the supplied buffer) against a given public key.
WEAVE_ERROR WeaveKeyExport::VerifySignature(const uint8_t *msgStart, uint16_t msgBufSize, uint16_t& msgLen,
                                            const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo)
{
    WEAVE_ERROR err;
    SHA256 sha256;
    uint8_t msgHash[SHA256::kHashLength];
    WeaveCertificateSet certSet;
    ValidationContext certValidContext;
    const uint8_t *msgSigStart;
    uint16_t msgSigLen;
    bool isInitiator = IsInitiator();
    bool callEndCertValidation = false;

    // Generate a SHA256 hash of the signed portion of the message.
    sha256.Begin();
    sha256.AddData(msgStart, msgLen);
    sha256.Finish(msgHash);

    // Invoke the auth delegate to prepare the certificate set and the validation context.
    // This will load the trust anchors into the certificate set and establish the
    // desired validation criteria for the peer's entity certificate.
    err = KeyExportDelegate->BeginCertValidation(isInitiator, certSet, certValidContext);
    SuccessOrExit(err);
    callEndCertValidation = true;

    // Set signature start and length parameters.
    msgSigStart = msgStart + msgLen;
    msgSigLen = msgBufSize - msgLen;

    // Verify the generated signature.
    err = VerifyWeaveSignature(msgHash, sizeof(msgHash),
                               msgSigStart, msgSigLen, kOID_SigAlgo_ECDSAWithSHA256,
                               certSet, certValidContext);
    SuccessOrExit(err);

    // Update message size.
    msgLen += msgSigLen;

    // Handle peer's certificate validation result.
    err = KeyExportDelegate->HandleCertValidationResult(isInitiator, certSet, certValidContext, pktInfo, msgInfo, KeyId);
    SuccessOrExit(err);

exit:
    if (callEndCertValidation)
    {
        WEAVE_ERROR endErr = KeyExportDelegate->EndCertValidation(isInitiator, certSet, certValidContext);
        if (err == WEAVE_NO_ERROR)
            err = endErr;
    }

    return err;
}

WEAVE_ERROR WeaveKeyExport::EncryptExportedKey(uint8_t *& buf, uint16_t bufSize, uint16_t& msgLen, uint16_t& exportedKeyLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveGroupKey groupKey;

    // Verify platform group key store object is provided.
    VerifyOrExit(GroupKeyStore != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Generate ECDH shared secret.
    err = ComputeSharedSecret(ECDHPublicKey);
    SuccessOrExit(err);

    // Derive key encryption and authentication key from the ECDH shared secret.
    err = DeriveKeyEncryptionKey();
    SuccessOrExit(err);

    // Get Client Root Key.
    err = GroupKeyStore->GetGroupKey(KeyId, groupKey);
    SuccessOrExit(err);

    // Set exported key length.
    exportedKeyLen = groupKey.KeyLen;

    // Update message length.
    msgLen += exportedKeyLen + kExportedKeyAuthenticatorSize;
    VerifyOrExit(msgLen <= bufSize, err = WEAVE_ERROR_BUFFER_TOO_SMALL);

    // Encrypt exported key.
    EncryptDecryptKey(groupKey.Key, buf, exportedKeyLen);

    // Generate authenticator for encrypted exported key.
    AuthenticateKey(buf, exportedKeyLen, buf + exportedKeyLen);

    // Update key Id value. This is needed if the requested key was of a current logical type.
    KeyId = groupKey.KeyId;

    // Update buffer pointer.
    buf += exportedKeyLen + kExportedKeyAuthenticatorSize;

exit:
    ClearSecretData(groupKey.Key, groupKey.MaxKeySize);

    return err;
}

WEAVE_ERROR WeaveKeyExport::DecryptExportedKey(const uint8_t *& buf, uint8_t *exportedKey, uint16_t exportedKeyLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint8_t authenticator[kExportedKeyAuthenticatorSize];
    int res;

    // Generate ECDH shared secret.
    err = ComputeSharedSecret(buf);
    SuccessOrExit(err);

    // Derive key encryption and authentication key from the ECDH shared secret.
    err = DeriveKeyEncryptionKey();
    SuccessOrExit(err);

    // Update buffer pointer.
    buf += GetECDHPublicKeyLen();

    // Generate authenticator for encrypted exported key.
    AuthenticateKey(buf, exportedKeyLen, authenticator);

    // Verify exported key authenticator.
    res = memcmp(buf + exportedKeyLen, authenticator, kExportedKeyAuthenticatorSize);
    VerifyOrExit(res == 0, err = WEAVE_ERROR_EXPORTED_KEY_AUTHENTICATION_FAILED);

    // Decrypt exported key.
    EncryptDecryptKey(buf, exportedKey, exportedKeyLen);

    // Update buffer pointer.
    buf += exportedKeyLen + kExportedKeyAuthenticatorSize;

exit:
    return err;
}

WEAVE_ERROR WeaveKeyExport::ComputeSharedSecret(const uint8_t *peerPubKey)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    EncodedECPrivateKey ecdhPrivKey;
    EncodedECPublicKey ecdhPubKey;

    ecdhPubKey.ECPoint = (uint8_t *)peerPubKey;
    ecdhPubKey.ECPointLen = GetECDHPublicKeyLen();
    ecdhPrivKey.PrivKey = ECDHPrivateKey;
    ecdhPrivKey.PrivKeyLen = ECDHPrivateKeyLen;
    err = ECDHComputeSharedSecret(GetCurveOID(), ecdhPubKey, ecdhPrivKey, SharedSecret, sizeof(SharedSecret), SharedSecretLen);
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR WeaveKeyExport::DeriveKeyEncryptionKey(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    HKDFSHA256 hkdf;
    uint8_t keySalt[kMaxKeySaltSize];

    // Prepare a salt value to be used in the generation of the key encryption key.
    keySalt[0] = ProtocolConfig;
    keySalt[1] = mAltConfigsCount;
    memcpy(keySalt + 2 * sizeof(uint8_t), mAltConfigs, mAltConfigsCount);
    LittleEndian::Put32(keySalt + 2 * sizeof(uint8_t) + mAltConfigsCount, KeyId);

    // Perform HKDF-based key expansion to produce the desired key data.
    err = hkdf.DeriveKey(keySalt, kMinKeySaltSize + mAltConfigsCount,
                         SharedSecret, SharedSecretLen,
                         NULL, 0, NULL, 0,
                         EncryptionAndAuthenticationKey, kEncryptionAndAuthenticationKeySize, kEncryptionAndAuthenticationKeySize);
    SuccessOrExit(err);

exit:
    return err;
}

void WeaveKeyExport::EncryptDecryptKey(const uint8_t *keyIn, uint8_t *keyOut, uint8_t keyLen)
{
    AES128CTRMode aes128CTR;

    // Initialize AES128CTR cipher with counter set to zero.
    aes128CTR.SetKey(EncryptionAndAuthenticationKey);

    // Encrypt/Decrypt input key material.
    aes128CTR.EncryptData(keyIn, keyLen, keyOut);

    // Reset AES engine to clear secret key material.
    aes128CTR.Reset();
}

void WeaveKeyExport::AuthenticateKey(const uint8_t *key, uint8_t keyLen, uint8_t* authenticator)
{
    HMACSHA256 hmac;

    // Initialize HMACSHA256 engine with authentication key.
    hmac.Begin(EncryptionAndAuthenticationKey + kEncryptionKeySize, kAuthenticationKeySize);

    // Add input key to digest.
    hmac.AddData(key, keyLen);

    // Get HMAC result.
    hmac.Finish(authenticator);

    // Reset HMAC engine to clear secret key material.
    hmac.Reset();
}

uint16_t WeaveKeyExport::GetECDHPublicKeyLen(void) const
{
    // NOTE: Should be reviewed/updated when new protocol configs are introduced.
#if WEAVE_CONFIG_SUPPORT_KEY_EXPORT_CONFIG2
    if (ProtocolConfig == kKeyExportConfig_Config2)
        return kConfig2_ECDHPublicKeySize;
    else
#endif
        return kConfig1_ECDHPublicKeySize;
}

OID WeaveKeyExport::GetCurveOID(void) const
{
    // NOTE: Should be reviewed/updated when new protocol configs are introduced.
#if WEAVE_CONFIG_SUPPORT_KEY_EXPORT_CONFIG2
    if (ProtocolConfig == kKeyExportConfig_Config2)
        return kOID_EllipticCurve_prime256v1;
    else
#endif
        return kOID_EllipticCurve_secp224r1;
}

} // namespace KeyExport
} // namespace Security
} // namespace Profiles
} // namespace Weave
} // namespace nl
