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

using nl::Weave::WeaveEncryptionKey;
using nl::Weave::Profiles::Security::AppKeys::WeaveGroupKey;
using nl::Weave::Profiles::Security::AppKeys::GroupKeyStoreBase;

class WeaveKeyExport;

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

    // ===== Abstract Interface methods

#if !WEAVE_CONFIG_LEGACY_KEY_EXPORT_DELEGATE

    /**
     * Get the key export certificate set for the local node.
     *
     * Called when the key export engine is preparing to sign a key export message.  This method
     * is responsible for initializing certificate set and loading all certificates that will be
     * included or referenced in the signature of the message.  The last certificate loaded must
     * be the signing certificate.
     */
    virtual WEAVE_ERROR GetNodeCertSet(WeaveKeyExport * keyExport, WeaveCertificateSet & certSet) = 0;

    /**
     * Release the node's certificate set.
     *
     * Called when the key export engine is done with the certificate set returned by GetNodeCertSet().
     */
    virtual WEAVE_ERROR ReleaseNodeCertSet(WeaveKeyExport * keyExport, WeaveCertificateSet & certSet) = 0;

    /**
     * Generate a signature for a key export message.
     *
     * This method is responsible for computing a signature of the given hash value using the local
     * node's private key and writing the signature to the supplied TLV writer as a WeaveSignature
     * TLV structure.
     */
    virtual WEAVE_ERROR GenerateNodeSignature(WeaveKeyExport * keyExport, const uint8_t * msgHash,
            uint8_t msgHashLen, TLVWriter & writer) = 0;

    /**
     * Prepare for validating the peer's certificate.
     *
     * Called at the start of certificate validation. This method is responsible for preparing the
     * supplied certificate set and validation context for use in validating the peer node's
     * certificate. Implementations must initialize the supplied WeaveCertificateSet object with
     * sufficient resources to handle the upcoming certificate validation.  The implementation
     * must also load any necessary trusted root or CA certificates into the certificate set.
     *
     * The supplied validation context will be initialized with a set of default validation
     * criteria, which the implementation may alter as necessary.  The implementation must
     * either set the EffectiveTime field, or set the appropriate validation flags to suppress
     * certificate lifetime validation.
     *
     * The implementation is required to maintain any resources allocated during BeginCertValidation()
     * until the corresponding EndCertValidation() is called is made.  Implementations are guaranteed
     * that EndCertValidation() will be called exactly once for each successful call to
     * BeginCertValidation().
     */
    virtual WEAVE_ERROR BeginCertValidation(WeaveKeyExport * keyExport, ValidationContext & validCtx,
            WeaveCertificateSet & certSet) = 0;

    /**
     * Process the results of validating the peer's certificate.
     *
     * Called when validation of the peer node's certificate has completed.  This method is only
     * called if certificate validation completes successfully.  Implementations may use this call
     * to inspect the results of validation, and possibly override the result with an error.
     *
     * For a responding node, the method is expected to verify the requestor's authority to export the
     * requested key.
     *
     * For an initiating node, the method is expected to verify that the validated certificate properly
     * identifies the peer to which the key export request was sent.
     */
    virtual WEAVE_ERROR HandleCertValidationResult(WeaveKeyExport * keyExport, ValidationContext & validCtx,
            WeaveCertificateSet & certSet, uint32_t requestedKeyId) = 0;

    /**
     * Release resources associated with peer certificate validation.
     *
     * Called when peer certificate validation and request verification are complete.
     */
    virtual WEAVE_ERROR EndCertValidation(WeaveKeyExport * keyExport, ValidationContext & validCtx,
            WeaveCertificateSet & certSet) = 0;

    /**
     * Verify the security of an unsigned key export message.
     *
     * Called when the node receives a key export message that isn't signed.  The method is expected to
     * verify the security of an unsigned key export message based on the context of its communication,
     * e.g. via the attributes of a security session used to send the message.
     *
     * For a responding node, the method is expected to verify the initiator's authority to export the
     * requested key.
     *
     * For an initiating node, the method is expected to verify the message legitimately originated from
     * the peer to which the key export request was sent.
     */
    virtual WEAVE_ERROR ValidateUnsignedKeyExportMessage(WeaveKeyExport * keyExport, uint32_t requestedKeyId) = 0;


#else // !WEAVE_CONFIG_LEGACY_KEY_EXPORT_DELEGATE

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
    virtual WEAVE_ERROR BeginCertValidation(bool isInitiator, WeaveCertificateSet& certSet, ValidationContext& validCtx) = 0;

    // Called with the results of validating the peer's certificate.
    // Responder verifies that requestor is authorized to export the specified key.
    // Requestor verifies that response came from expected node.
    virtual WEAVE_ERROR HandleCertValidationResult(bool isInitiator, WeaveCertificateSet& certSet, ValidationContext& validCtx,
                                                   const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo, uint32_t requestedKeyId) = 0;

    // Called when peer certificate validation and request verification are complete.
    virtual WEAVE_ERROR EndCertValidation(bool isInitiator, WeaveCertificateSet& certSet, ValidationContext& validCtx) = 0;

    // Called by requestor and responder to verify that received message was appropriately secured when the message isn't signed.
    virtual WEAVE_ERROR ValidateUnsignedKeyExportMessage(bool isInitiator, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo, uint32_t requestedKeyId) = 0;

private:

    // ===== Private members that provide API compatibility with the non-legacy interface.

    friend class WeaveKeyExport;

    WEAVE_ERROR GetNodeCertSet(WeaveKeyExport * keyExport, WeaveCertificateSet & certSet);
    WEAVE_ERROR ReleaseNodeCertSet(WeaveKeyExport * keyExport, WeaveCertificateSet & certSet);
    WEAVE_ERROR BeginCertValidation(WeaveKeyExport * keyExport, ValidationContext & validCtx,
            WeaveCertificateSet & certSet);
    WEAVE_ERROR HandleCertValidationResult(WeaveKeyExport * keyExport, ValidationContext & validCtx,
            WeaveCertificateSet & certSet, uint32_t requestedKeyId);
    WEAVE_ERROR EndCertValidation(WeaveKeyExport * keyExport, ValidationContext & validCtx,
            WeaveCertificateSet & certSet);
    WEAVE_ERROR ValidateUnsignedKeyExportMessage(WeaveKeyExport * keyExport, uint32_t requestedKeyId);

#endif // WEAVE_CONFIG_LEGACY_KEY_EXPORT_DELEGATE
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
     *  The current state of the WeaveKeyExport object.
     */
    enum
    {
        kState_Reset                              = 0,  /**< The initial (and final) state of a WeaveKeyExport object. */
        kState_InitiatorGeneratingRequest         = 10, /**< Initiator state indicating that the key export request message is being generated. */
        kState_InitiatorRequestGenerated          = 11, /**< Initiator state indicating that the key export request message has been generated. */
        kState_InitiatorReconfigureProcessed      = 12, /**< Initiator state indicating that the key export reconfigure message was processed. */
        kState_InitiatorDone                      = 13, /**< Initiator state indicating that the key export response was processed. */
        kState_ResponderProcessingRequest         = 20, /**< Responder state indicating that the key export request message is being processed. */
        kState_ResponderRequestProcessed          = 21, /**< Responder state indicating that the key export request message has been processed. */
        kState_ResponderDone                      = 22  /**< Responder state indicating that the key export response message was generated. */
    };

    WeaveKeyExportDelegate * KeyExportDelegate;         /**< Pointer to a key export delegate object. */
    GroupKeyStoreBase * GroupKeyStore;                  /**< Pointer to a platform group key store object. */

    void Init(WeaveKeyExportDelegate * keyExportDelegate, GroupKeyStoreBase * groupKeyStore = NULL);
    void Reset(void);
    void Shutdown(void);

    uint8_t State() const;

    bool IsInitiator() const;

    uint8_t ProtocolConfig() const;

    uint32_t KeyId() const;

    uint8_t AllowedConfigs() const;
    void SetAllowedConfigs(uint8_t allowedConfigs);
    bool IsAllowedConfig(uint8_t config) const;

    bool SignMessages() const;

    const WeaveMessageInfo * MessageInfo() const;

    WEAVE_ERROR GenerateKeyExportRequest(uint8_t *buf, uint16_t bufSize, uint16_t& msgLen, uint8_t proposedConfig, uint32_t keyId, bool signMessages);
    WEAVE_ERROR ProcessKeyExportRequest(const uint8_t *buf, uint16_t msgSize, const WeaveMessageInfo *msgInfo);
    WEAVE_ERROR GenerateKeyExportResponse(uint8_t *buf, uint16_t bufSize, uint16_t& msgLen, const WeaveMessageInfo *msgInfo);
    WEAVE_ERROR ProcessKeyExportResponse(const uint8_t *buf, uint16_t msgSize, const WeaveMessageInfo *msgInfo,
                                         uint8_t *exportedKeyBuf, uint16_t exportedKeyBufSize, uint16_t &exportedKeyLen, uint32_t &exportedKeyId);
    WEAVE_ERROR GenerateKeyExportReconfigure(uint8_t *buf, uint16_t bufSize, uint16_t& msgLen);
    WEAVE_ERROR ProcessKeyExportReconfigure(const uint8_t *buf, uint16_t msgSize, uint8_t &config);

private:
    uint32_t mKeyId;                                    /**< Exported key Id. */
    const WeaveMessageInfo * mMsgInfo;
    union
    {
        struct
        {
            uint16_t ECDHPrivateKeyLen;
            uint8_t ECDHPublicKey[kMaxECDHPublicKeySize];
            uint8_t ECDHPrivateKey[kMaxECDHPrivateKeySize];
        };
        struct
        {
            uint16_t SharedSecretLen;
            uint8_t SharedSecret[kMaxECDHSharedSecretSize];
        };
        struct
        {
            uint8_t EncryptionAndAuthenticationKey[kEncryptionAndAuthenticationKeySize];
        };
    };
    uint8_t mState;                                     /**< Current state of the WeaveKeyExport object. */
    uint8_t mProtocolConfig;                            /**< Selected key export protocol config. */
    uint8_t mAllowedConfigs;                            /**< Allowed protocol configurations. */
    uint8_t mAltConfigsCount;
    uint8_t mAltConfigs[kMaxAltConfigsCount];
    bool mSignMessages;                                 /**< Sign protocol messages flag. */

    WEAVE_ERROR AppendNewECDHKey(uint8_t *& buf);
    WEAVE_ERROR AppendSignature(uint8_t *msgStart, uint16_t msgBufSize, uint16_t& msgLen);
    WEAVE_ERROR VerifySignature(const uint8_t *msgStart, uint16_t msgBufSize, uint16_t& msgLen,
                                const WeaveMessageInfo *msgInfo);

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




#if WEAVE_CONFIG_LEGACY_KEY_EXPORT_DELEGATE

inline WEAVE_ERROR WeaveKeyExportDelegate::GetNodeCertSet(WeaveKeyExport * keyExport, WeaveCertificateSet & certSet)
{
    return GetNodeCertSet(keyExport->IsInitiator(), certSet);
}

inline WEAVE_ERROR WeaveKeyExportDelegate::ReleaseNodeCertSet(WeaveKeyExport * keyExport, WeaveCertificateSet & certSet)
{
    return ReleaseNodeCertSet(keyExport->IsInitiator(), certSet);
}

inline WEAVE_ERROR WeaveKeyExportDelegate::BeginCertValidation(WeaveKeyExport * keyExport, ValidationContext & validCtx,
        WeaveCertificateSet & certSet)
{
    return BeginCertValidation(keyExport->IsInitiator(), certSet, validCtx);
}

inline WEAVE_ERROR WeaveKeyExportDelegate::HandleCertValidationResult(WeaveKeyExport * keyExport, ValidationContext & validCtx,
        WeaveCertificateSet & certSet, uint32_t requestedKeyId)
{
    return HandleCertValidationResult(keyExport->IsInitiator(), certSet, validCtx, NULL, keyExport->MessageInfo(), requestedKeyId);
}

inline WEAVE_ERROR WeaveKeyExportDelegate::EndCertValidation(WeaveKeyExport * keyExport, ValidationContext & validCtx,
        WeaveCertificateSet & certSet)
{
    return EndCertValidation(keyExport->IsInitiator(), certSet, validCtx);
}

inline WEAVE_ERROR WeaveKeyExportDelegate::ValidateUnsignedKeyExportMessage(WeaveKeyExport * keyExport, uint32_t requestedKeyId)
{
    return ValidateUnsignedKeyExportMessage(keyExport->IsInitiator(), NULL, keyExport->MessageInfo(), requestedKeyId);
}

#endif // WEAVE_CONFIG_LEGACY_KEY_EXPORT_DELEGATE

inline uint8_t WeaveKeyExport::State() const
{
    return mState;
}

inline uint8_t WeaveKeyExport::ProtocolConfig() const
{
    return mProtocolConfig;
}

inline uint32_t WeaveKeyExport::KeyId() const
{
    return mKeyId;
}

inline uint8_t WeaveKeyExport::AllowedConfigs() const
{
    return mAllowedConfigs;
}

inline void WeaveKeyExport::SetAllowedConfigs(uint8_t allowedConfigs)
{
    mAllowedConfigs = kKeyExportSupportedConfig_All & allowedConfigs;
}

inline const WeaveMessageInfo * WeaveKeyExport::MessageInfo() const
{
    return mMsgInfo;
}

inline bool WeaveKeyExport::SignMessages() const
{
    return mSignMessages;
}

} // namespace KeyExport
} // namespace Security
} // namespace Profiles
} // namespace Weave
} // namespace nl

#endif /* WEAVEKEYEXPORT_H_ */
