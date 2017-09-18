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
 *      This file defines types, classes and interfaces associated with
 *      key export protocol.
 *
 */

#ifndef WEAVEKEYEXPORT_H_
#define WEAVEKEYEXPORT_H_

#include <stdint.h>
#include <string.h>

#include <Weave/Core/WeaveConfig.h>
#include "WeaveApplicationKeys.h"
#include <Weave/Support/NLDLLUtil.h>
#include <Weave/Support/crypto/CTRMode.h>
#include <Weave/Support/crypto/HashAlgos.h>
#include <Weave/Core/WeaveError.h>
#include <Weave/Core/WeaveCore.h>
#include <InetLayer/InetBuffer.h>

/**
 *   @namespace nl::Weave::Profiles::Security::KeyExport
 *
 *   @brief
 *     This namespace includes all interfaces within Weave for the
 *     key export protocol within the Weave security profile.
 */

namespace nl {
namespace Weave {
namespace Profiles {
namespace Security {
namespace KeyExport {

using nl::Inet::InetBuffer;
using nl::Weave::WeaveEncryptionKey;
using nl::Weave::Profiles::Security::AppKeys::WeaveGroupKey;
using nl::Weave::Profiles::Security::AppKeys::GroupKeyStoreBase;

enum
{
    // Protocol configurations.
    kKeyExportConfig_Unspecified                    = 0x00,
    kKeyExportConfig_Config1                        = 0x01,
    kKeyExportConfig_Config2                        = 0x02,
    kKeyExportConfig_ConfigLast                     = kKeyExportConfig_Config2,

    // Supported Configurations Bit Masks.
    kKeyExportSupportedConfig_Config1               = 0x01,
    kKeyExportSupportedConfig_Config2               = 0x02,
    kKeyExportSupportedConfig_All                   = (0x00 |
#if WEAVE_CONFIG_SUPPORT_KEY_EXPORT_CONFIG1
                                                       kKeyExportSupportedConfig_Config1 |
#endif
#if WEAVE_CONFIG_SUPPORT_KEY_EXPORT_CONFIG2
                                                       kKeyExportSupportedConfig_Config2 |
#endif
                                                       0x00)
};

// Protocol Control Header field definitions.
enum
{
    // --- Requester Control Header fields.
    kReqControlHeader_AltConfigCountMask            = 0x07,
    kReqControlHeader_AltConfigCountShift           = 0,
    kReqControlHeader_SignMessagesFlag              = 0x80,
    kReqControlHeader_UnusedBits                    = ~((kReqControlHeader_AltConfigCountMask << kReqControlHeader_AltConfigCountShift ) |
                                                        kReqControlHeader_SignMessagesFlag),

    // --- Responder Control Header fields.
    kResControlHeader_SignMessagesFlag              = 0x80,
    kResControlHeader_UnusedBits                    = ~kResControlHeader_SignMessagesFlag,
};

// Protocol configuration specific sizes (in bytes).
enum
{
    // Sizes declaration for Config1.
    kConfig1_CurveSize                              = 28,
    kConfig1_ECDHPrivateKeySize                     = kConfig1_CurveSize + 1,
    kConfig1_ECDHPublicKeySize                      = 2 * kConfig1_CurveSize + 1,

    // Sizes declaration for Config2.
    kConfig2_CurveSize                              = 32,
    kConfig2_ECDHPrivateKeySize                     = kConfig2_CurveSize + 1,
    kConfig2_ECDHPublicKeySize                      = 2 * kConfig2_CurveSize + 1,

    // Maximum possible sizes.
#if WEAVE_CONFIG_SUPPORT_KEY_EXPORT_CONFIG2
    kMaxECDHPrivateKeySize                          = kConfig2_ECDHPrivateKeySize,
    kMaxECDHPublicKeySize                           = kConfig2_ECDHPublicKeySize,
    kMaxECDHSharedSecretSize                        = kConfig2_CurveSize,
#else
    kMaxECDHPrivateKeySize                          = kConfig1_ECDHPrivateKeySize,
    kMaxECDHPublicKeySize                           = kConfig1_ECDHPublicKeySize,
    kMaxECDHSharedSecretSize                        = kConfig1_CurveSize,
#endif
};

// Protocol data sizes (in bytes).
enum
{
    kMaxAltConfigsCount                             = 7,
    kExportedKeyAuthenticatorSize                   = Platform::Security::SHA256::kHashLength,
    kMaxNodePrivateKeySize                          = ((WEAVE_CONFIG_MAX_EC_BITS + 7) / 8) + 1,
    kEncryptionKeySize                              = Crypto::AES128CTRMode::kKeyLength,
    kAuthenticationKeySize                          = Platform::Security::SHA256::kHashLength,
    kEncryptionAndAuthenticationKeySize             = kEncryptionKeySize + kAuthenticationKeySize,
    kMinKeySaltSize                                 = 2 * sizeof(uint8_t) + sizeof(uint32_t),
    kMaxKeySaltSize                                 = kMinKeySaltSize + kMaxAltConfigsCount,
    kKeyExportReconfigureMsgSize                    = sizeof(uint8_t),
};

/**
 *  @class WeaveKeyExportDelegate
 *
 *  @brief
 *    Abstract delegate class called by KeyExport engine to perform various
 *    actions related to authentication during key export.
 *
 */
class WeaveKeyExportDelegate
{
public:
    // Get the key export certificate set for the local node.
    // This method is responsible for initializing certificate set and loading all certificates
    // that will be included in the signature of the message.
    virtual WEAVE_ERROR GetNodeCertSet(bool isInitiator, WeaveCertificateSet& certSet) = 0;

    // Called when the key export engine is done with the certificate set returned by GetNodeCertSet().
    virtual WEAVE_ERROR ReleaseNodeCertSet(bool isInitiator, WeaveCertificateSet& certSet) = 0;

    // Get the local node's private key.
    virtual WEAVE_ERROR GetNodePrivateKey(bool isInitiator, const uint8_t *& weavePrivKey, uint16_t& weavePrivKeyLen) = 0;

    // Called when the key export engine is done with the buffer returned by GetNodePrivateKey().
    virtual WEAVE_ERROR ReleaseNodePrivateKey(bool isInitiator, const uint8_t *& weavePrivKey) = 0;

    // Prepare the supplied certificate set and validation context for use in validating the certificate of a peer.
    // This method is responsible for loading the trust anchors into the certificate set.
    virtual WEAVE_ERROR BeginCertValidation(bool isInitiator, WeaveCertificateSet& certSet, ValidationContext& validContext) = 0;

    // Called with the results of validating the peer's certificate.
    // Responder verifies that requestor is authorized to export the specified key.
    // Requestor verifies that response came from expected node.
    virtual WEAVE_ERROR HandleCertValidationResult(bool isInitiator, WeaveCertificateSet& certSet, ValidationContext& validContext,
                                                   const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo, uint32_t requestedKeyId) = 0;

    // Called when peer certificate validation and request verification are complete.
    virtual WEAVE_ERROR EndCertValidation(bool isInitiator, WeaveCertificateSet& certSet, ValidationContext& validContext) = 0;

    // Called by requestor and responder to verify that received message was appropriately secured when the message isn't signed.
    virtual WEAVE_ERROR ValidateUnsignedKeyExportMessage(bool isInitiator, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo, uint32_t requestedKeyId) = 0;
};

/**
 *  @class WeaveKeyExport
 *
 *  @brief
 *    Implements the core logic of the Weave key export protocol.
 *
 */
class NL_DLL_EXPORT WeaveKeyExport
{
public:
    /**
     *  @enum State
     *
     *  The current state of the WeaveKeyExport object.
     */
    enum
    {
        kState_Reset                                           = 0,  /**< The initial (and final) state of a WeaveKeyExport object. */
        kState_InitiatorGeneratingRequest                      = 10, /**< Initiator state indicating that the key export request message is being generated. */
        kState_InitiatorRequestGenerated                       = 11, /**< Initiator state indicating that the key export request message has been generated. */
        kState_InitiatorReconfigureProcessed                   = 12, /**< Initiator state indicating that the key export reconfigure message was processed. */
        kState_InitiatorDone                                   = 13, /**< Initiator state indicating that the key export response was processed. */
        kState_ResponderProcessingRequest                      = 20, /**< Responder state indicating that the key export request message is being processed. */
        kState_ResponderRequestProcessed                       = 21, /**< Responder state indicating that the key export request message has been processed. */
        kState_ResponderDone                                   = 22  /**< Responder state indicating that the key export response message was generated. */
    } State;                                                         /**< [READ-ONLY] Current state of the WeaveKeyExport object. */

    WeaveKeyExportDelegate *KeyExportDelegate;        /**< Pointer to a key export delegate object. */
    GroupKeyStoreBase *GroupKeyStore;                 /**< Pointer to a platform group key store object. */
    uint8_t ProtocolConfig;                           /**< Key export protocol config. */
    uint32_t KeyId;                                   /**< Exported key Id. */
    bool SignMessages;                                /**< Sign protocol messages flag. */

    void Init(WeaveKeyExportDelegate *keyExportDelegate, GroupKeyStoreBase *groupKeyStore = NULL);
    void Reset(void);
    void Shutdown(void);

    bool IsInitiator() const;

    uint8_t AllowedConfigs() const
    {
        return mAllowedConfigs;
    }
    void SetAllowedConfigs(uint8_t allowedConfigs)
    {
        mAllowedConfigs = kKeyExportSupportedConfig_All & allowedConfigs;
    }
    bool IsAllowedConfig(uint8_t config) const;

    WEAVE_ERROR GenerateKeyExportRequest(uint8_t *buf, uint16_t bufSize, uint16_t& msgLen, uint8_t proposedConfig, uint32_t keyId, bool signMessages);
    WEAVE_ERROR ProcessKeyExportRequest(const uint8_t *buf, uint16_t msgSize, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo);
    WEAVE_ERROR GenerateKeyExportResponse(uint8_t *buf, uint16_t bufSize, uint16_t& msgLen);
    WEAVE_ERROR ProcessKeyExportResponse(const uint8_t *buf, uint16_t msgSize, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo,
                                         uint8_t *exportedKeyBuf, uint16_t exportedKeyBufSize, uint16_t &exportedKeyLen, uint32_t &exportedKeyId);
    WEAVE_ERROR GenerateKeyExportReconfigure(uint8_t *buf, uint16_t bufSize, uint16_t& msgLen);
    WEAVE_ERROR ProcessKeyExportReconfigure(const uint8_t *buf, uint16_t msgSize, uint8_t &config);

private:
    uint8_t mAllowedConfigs;                          /**< Allowed protocol configurations. */
    uint8_t mAltConfigsCount;
    uint8_t mAltConfigs[kMaxAltConfigsCount];
    union
    {
        struct
        {
            uint8_t ECDHPublicKey[kMaxECDHPublicKeySize];
            uint8_t ECDHPrivateKey[kMaxECDHPrivateKeySize];
            uint16_t ECDHPrivateKeyLen;
        };
        struct
        {
            uint8_t SharedSecret[kMaxECDHSharedSecretSize];
            uint16_t SharedSecretLen;
        };
        struct
        {
            uint8_t EncryptionAndAuthenticationKey[kEncryptionAndAuthenticationKeySize];
        };
    };

    WEAVE_ERROR AppendNewECDHKey(uint8_t *& buf);
    WEAVE_ERROR AppendSignature(uint8_t *msgStart, uint16_t msgBufSize, uint16_t& msgLen);
    WEAVE_ERROR VerifySignature(const uint8_t *msgStart, uint16_t msgBufSize, uint16_t& msgLen,
                                const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo);

    WEAVE_ERROR EncryptExportedKey(uint8_t *& buf, uint16_t bufSize, uint16_t& msgLen, uint16_t& exportedKeyLen);
    WEAVE_ERROR DecryptExportedKey(const uint8_t *& buf, uint8_t *exportedKey, uint16_t exportedKeyLen);
    WEAVE_ERROR ComputeSharedSecret(const uint8_t *peerPubKey);
    WEAVE_ERROR DeriveKeyEncryptionKey(void);
    void EncryptDecryptKey(const uint8_t *keyIn, uint8_t *keyOut, uint8_t keyLen);
    void AuthenticateKey(const uint8_t *key, uint8_t keyLen, uint8_t* authenticator);

    WEAVE_ERROR GenerateAltConfigsList(void);
    WEAVE_ERROR ValidateProtocolConfig(void);
    uint16_t GetECDHPublicKeyLen(void) const;
    OID GetCurveOID(void) const;
};


extern WEAVE_ERROR SimulateDeviceKeyExport(const uint8_t *deviceCert, uint16_t deviceCertLen,
                                           const uint8_t *devicePrivKey, uint16_t devicePrivKeyLen,
                                           const uint8_t *trustRootCert, uint16_t trustRootCertLen,
                                           const uint8_t *exportReq, uint16_t exportReqLen,
                                           uint8_t *exportRespBuf, uint16_t exportRespBufSize, uint16_t& exportRespLen,
                                           bool& respIsReconfig);


} // namespace KeyExport
} // namespace Security
} // namespace Profiles
} // namespace Weave
} // namespace nl

#endif /* WEAVEKEYEXPORT_H_ */
