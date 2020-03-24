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
 *      This file defines data types and objects for initiators and
 *      responders for the Weave Certificate Authenticated Session
 *      Establishment (CASE) protocol.
 *
 */

#ifndef WEAVECASE_H_
#define WEAVECASE_H_

#include <Weave/Support/NLDLLUtil.h>
#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveVendorIdentifiers.hpp>
#include <Weave/Profiles/security/WeaveSecurity.h>
#include <Weave/Profiles/security/WeaveCert.h>
#include <Weave/Support/ASN1.h>
#include <Weave/Support/CodeUtils.h>
#include <Weave/Support/crypto/EllipticCurve.h>
#include <Weave/Support/crypto/HashAlgos.h>

/**
 *   @namespace nl::Weave::Profiles::Security::CASE
 *
 *   @brief
 *     This namespace includes all interfaces within Weave for the
 *     Certificate Authenticated Session Establishment (CASE)
 *     protocol within the Weave security profile.
 */

namespace nl {
namespace Weave {
namespace Profiles {
namespace Security {
namespace CASE {

using nl::Weave::ASN1::OID;
using nl::Weave::TLV::TLVWriter;
using nl::Weave::Platform::Security::SHA1;
using nl::Weave::Platform::Security::SHA256;
using nl::Weave::Crypto::EncodedECPublicKey;
using nl::Weave::Crypto::EncodedECPrivateKey;
using nl::Weave::Crypto::EncodedECDSASignature;

class WeaveCASEEngine;


// CASE Protocol Configuration Values
// clang-format off
enum
{
    kCASEConfig_NotSpecified                    = 0,
    kCASEConfig_Config1                         = (kWeaveVendor_NestLabs << 16) | 1,
    kCASEConfig_Config2                         = (kWeaveVendor_NestLabs << 16) | 2,
};

// Bit-field representing a set of allowed CASE protocol configurations
enum
{
    kCASEAllowedConfig_Config1                  = 0x01,
    kCASEAllowedConfig_Config2                  = 0x02,

    kCASEAllowedConfig_Mask                     = 0x03, // NOTE: If you expand this mask, you must reconfigure
                                                        // the mFlags field in WeaveCASEEngine.
};

enum
{
    kCASEKeyConfirmHashLength_0Bytes            = 0x00,
    kCASEKeyConfirmHashLength_32Bytes           = 0x40,
    kCASEKeyConfirmHashLength_20Bytes           = 0x80,
    kCASEKeyConfirmHashLength_Reserved          = 0xC0,
};

// CASE Header Field Definitions
enum
{
    // Control Header Fields
    kCASEHeader_EncryptionTypeMask              = 0x0F,
    kCASEHeader_PerformKeyConfirmFlag           = 0x80,
    kCASEHeader_ControlHeaderUnusedBits         = ~(kCASEHeader_EncryptionTypeMask |
                                                    kCASEHeader_PerformKeyConfirmFlag),

    // Size Header Fields
    kCASEHeader_DHPublicKeyLengthMask           = 0x000000FF,
    kCASEHeader_DHPublicKeyLengthShift          = 0,
    kCASEHeader_SignatureLengthMask             = 0x0000FF00,
    kCASEHeader_SignatureLengthShift            = 8,
    kCASEHeader_AlternateConfigCountMask        = 0x00FF0000,
    kCASEHeader_AlternateConfigCountShift       = 16,
    kCASEHeader_AlternateCurveCountMask         = 0xFF000000,
    kCASEHeader_AlternateCurveCountShift        = 24,

    // Mask for Key Confirm Hash Length field in CASEBeginSessionResponse
    kCASEHeader_KeyConfirmHashLengthMask        = 0xC0
};
// clang-format on

/**
 * Holds context information related to the generation or processing of a CASE begin session messages.
 */
class BeginSessionContext
{
public:
    uint64_t PeerNodeId;
    EncodedECPublicKey ECDHPublicKey;
    const WeaveMessageInfo * MsgInfo;
    uint32_t ProtocolConfig;
    uint32_t CurveId;
    const uint8_t * Payload;
    const uint8_t * CertInfo;
    const uint8_t * Signature;
    uint16_t PayloadLength;
    uint16_t CertInfoLength;
    uint16_t SignatureLength;

    bool IsBeginSessionRequest() const;
    void SetIsBeginSessionRequest(bool val);
    bool IsInitiator() const;
    void SetIsInitiator(bool val);
    bool PerformKeyConfirm() const;
    void SetPerformKeyConfirm(bool val);

protected:
    enum
    {
        kFlag_IsBeginSessionRequest     = 0x01,
        kFlag_IsInitiator               = 0x02,
        kFlag_PerformKeyConfirm         = 0x04,
    };

    uint8_t Flags;
};


/**
 * Holds context information related to the generation or processing of a CASE BeginSessionRequest message.
 */
class BeginSessionRequestContext : public BeginSessionContext
{
public:
    enum
    {
        kMaxAlternateProtocolConfigs    = 4,
        kMaxAlternateCurveIds           = 4
    };

    uint32_t AlternateConfigs[kMaxAlternateProtocolConfigs];
    uint32_t AlternateCurveIds[kMaxAlternateCurveIds];
    uint16_t SessionKeyId;
    uint8_t AlternateConfigCount;
    uint8_t AlternateCurveCount;
    uint8_t EncryptionType;

    WEAVE_ERROR EncodeHead(PacketBuffer * msgBuf);
    WEAVE_ERROR DecodeHead(PacketBuffer * msgBuf);
    uint16_t HeadLength(void);
    void Reset(void);
    bool IsAltConfig(uint32_t config) const;
};


/**
 * Holds context information related to the generation or processing of a CASE BeginSessionRequest message.
 */
class BeginSessionResponseContext : public BeginSessionContext
{
public:
    const uint8_t * KeyConfirmHash;
    uint8_t KeyConfirmHashLength;

    WEAVE_ERROR EncodeHead(PacketBuffer * msgBuf);
    WEAVE_ERROR DecodeHead(PacketBuffer * msgBuf);
    uint16_t HeadLength(void);
    void Reset(void);
};


/**
 * Holds information related to the generation or processing of a CASE Reconfigure message.
 */
class ReconfigureContext
{
public:
    uint32_t ProtocolConfig;
    uint32_t CurveId;

    WEAVE_ERROR Encode(PacketBuffer *buf);
    void Reset(void);
    static WEAVE_ERROR Decode(PacketBuffer *buf, ReconfigureContext& msg);
};


/**
 * Abstract interface to which authentication actions are delegated during CASE
 * session establishment.
 */
class WeaveCASEAuthDelegate
{
public:

#if !WEAVE_CONFIG_LEGACY_CASE_AUTH_DELEGATE

    // ===== Abstract Interface methods

    /**
     * Encode CASE Certificate Information for the local node.
     *
     * Implementations can use this call to override the default encoding of the CASE
     * CertificateInformation structure for the local node.  When called, the
     * implementation should write a CertificateInformation structure containing, at
     * a minimum, the local node's entity certificate.  Implementation may optionally
     * include a set of related certificates and/or trust anchors.
     */
    virtual WEAVE_ERROR EncodeNodeCertInfo(const BeginSessionContext & msgCtx, TLVWriter & writer) = 0;

    /** Generate a signature using local node's private key.
     *
     * When invoked, implementations must compute a signature on the given hash value using the node's
     * private key.  The generated signature should then be written in the form of a CASE ECDSASignature
     * structure to the supplied TLV writing using the specified tag.
     *
     * In cases where the node's private key is held in a local buffer, the GenerateAndEncodeWeaveECDSASignature()
     * utility function can be useful for implementing this method.
     */
    virtual WEAVE_ERROR GenerateNodeSignature(const BeginSessionContext & msgCtx,
            const uint8_t * msgHash, uint8_t msgHashLen,
            TLVWriter & writer, uint64_t tag) = 0;

    /**
     * Encode an application-specific payload to be included in the CASE message to the peer.
     *
     * Implementing this method is optional.  The default implementation returns a zero-length
     * payload.
     */
    virtual WEAVE_ERROR EncodeNodePayload(const BeginSessionContext & msgCtx,
            uint8_t * payloadBuf, uint16_t payloadBufSize, uint16_t & payloadLen);

    /**
     * Called at the start of certificate validation.
     *
     * Implementations must initialize the supplied WeaveCertificateSet object with sufficient
     * resources to handle the upcoming certificate validation.  At this time Implementations
     * may load trusted root or CA certificates into the certificate set, or wait until
     * OnPeerCertsLoaded() is called.
     *
     * Each certificate loaded into the certificate set will be assigned a default certificate
     * type by the load function.  Implementations should adjust these types as necessary to
     * ensure the correct treatment of the certificate during validation, and the correct
     * assignment of WeaveAuthMode for CASE interactions.
     *
     * The supplied validation context will be initialized with a set of default validation
     * criteria, which the implementation may alter as necessary.  The implementation must
     * either set the EffectiveTime field, or set the appropriate validation flags to suppress
     * certificate lifetime validation.
     *
     * If detailed validation results are desired, the implementation may initialize the
     * CertValidationResults and CertValidationLen fields.
     *
     * Implementations are required to maintain any resources allocated during BeginValidation()
     * until the corresponding EndValidation() is called is made.  Implementations are guaranteed
     * that EndValidation() will be called exactly once for each successful call to BeginValidation().
     */
    virtual WEAVE_ERROR BeginValidation(const BeginSessionContext & msgCtx, ValidationContext & validCtx,
            WeaveCertificateSet & certSet) = 0;

    /**
     * Called after the peer's certificates have been loaded.
     *
     * Implementations may use this call to finalize the input certificates and the validation
     * criteria that will be used to perform validation of the peer's certificate. At call time,
     * the certificates supplied by the peer will have been loaded into the certificate set
     * (including its own certificate, if present). Additionally, the subjectDN and subjectKeyId
     * arguments will have been initialized to values that will be used to resolve the peer's
     * certificate from the certificate set. If the peer supplied its own certificate (rather
     * than a certificate reference) then the EntityCert field within the validCtx argument will
     * contain a pointer to that certificate.
     *
     * During this called, implementations may modify the contents of the certificate set, including
     * adding new certificates. They may also alter the subjectDN, subjectKeyId or validCtx
     * arguments as necessary. Most importantly, implementations should adjust the certificate type
     * fields with the certificate set prior to returning to ensure correct treatment of certificates
     * during validation and subsequent access control checks.
     *
     * NOTE: In the event that the peer supplies a certificate reference for itself, rather than a
     * full certificate, the EntityCert field in the validation context will contain a NULL. If an
     * implementation wishes to support certificate references, it must add a certificate matching
     * the peer's subject DN and key id to the certificate set prior to returning.
     *
     * Implementing this method is optional. The default implementation does nothing.
     */
    virtual WEAVE_ERROR OnPeerCertsLoaded(const BeginSessionContext & msgCtx, WeaveDN & subjectDN,
            CertificateKeyId & subjectKeyId, ValidationContext & validCtx, WeaveCertificateSet& certSet);

    /**
     * Called with the result of certificate validation.
     *
     * Implementations may use this call to inspect, and possibly alter, the result of validation
     * of the peer's certificate.  If validation was successful, validRes will be set to WEAVE_NO_ERROR.
     * In this case, the validation context will contain details regarding the result. In particular,
     * the TrustAnchor field will be set to the trust anchor certificate.
     *
     * If the implementation initialized the CertValidationResults and CertValidationLen fields within
     * the ValidationContext structure during the BeginValidation() called, then these fields will
     * contained detailed validation results for each certificate in the certificate set.
     *
     * Implementations may override this by setting validRes to an error value, thereby causing validation to fail.
     *
     * If validation failed, validRes will reflect the reason for the failure.  Implementations may
     * override the result to a different error value, but MUST NOT set the result to WEAVE_NO_ERROR.
     */
    virtual WEAVE_ERROR HandleValidationResult(const BeginSessionContext & msgCtx, ValidationContext & validCtx,
            WeaveCertificateSet & certSet, WEAVE_ERROR & validRes) = 0;

    /** Called at the end of certificate validation.
     *
     * Implementations may use this call to perform cleanup after certification validation completes.
     * Implementations are guaranteed that EndValidation() will be called exactly once for each
     * successful call to BeginValidation().
     */
    virtual void EndValidation(const BeginSessionContext & msgCtx, ValidationContext & validCtx,
            WeaveCertificateSet & certSet) = 0;

#else // !WEAVE_CONFIG_LEGACY_CASE_AUTH_DELEGATE

    // ===== Legacy interface methods

    // Get the CASE Certificate Information structure for the local node.
    virtual WEAVE_ERROR GetNodeCertInfo(bool isInitiator, uint8_t * buf, uint16_t bufSize, uint16_t & certInfoLen) = 0;

    // Get the local node's private key.
    virtual WEAVE_ERROR GetNodePrivateKey(bool isInitiator, const uint8_t *& weavePrivKey, uint16_t & weavePrivKeyLen) = 0;

    // Called when the CASE engine is done with the buffer returned by GetNodePrivateKey().
    virtual WEAVE_ERROR ReleaseNodePrivateKey(const uint8_t * weavePrivKey) = 0;

    // Get payload information, if any, to be included in the message to the peer.
    virtual WEAVE_ERROR GetNodePayload(bool isInitiator, uint8_t * buf, uint16_t bufSize, uint16_t & payloadLen) = 0;

    // Prepare the supplied certificate set and validation context for use in validating the certificate of a peer.
    // This method is responsible for loading the trust anchors into the certificate set.
    virtual WEAVE_ERROR BeginCertValidation(bool isInitiator, WeaveCertificateSet & certSet, ValidationContext & validCtx) = 0;

    // Called with the results of validating the peer's certificate. If basic cert validation was successful, this method can
    // cause validation to fail by setting validRes, e.g. in the event that the peer's certificate is somehow unacceptable.
    virtual WEAVE_ERROR HandleCertValidationResult(bool isInitiator, WEAVE_ERROR & validRes, WeaveCertificateData * peerCert,
            uint64_t peerNodeId, WeaveCertificateSet & certSet, ValidationContext & validCtx) = 0;

    // Called when peer certificate validation is complete.
    virtual WEAVE_ERROR EndCertValidation(WeaveCertificateSet & certSet, ValidationContext & validCtx) = 0;

private:

    // ===== Private members that provide compatibility with the non-legacy interface.

    friend class WeaveCASEEngine;

    WEAVE_ERROR GenerateNodeSignature(const BeginSessionContext & msgCtx,
            const uint8_t * msgHash, uint8_t msgHashLen, TLVWriter & writer, uint64_t tag);
    WEAVE_ERROR EncodeNodePayload(const BeginSessionContext & msgCtx,
                uint8_t * payloadBuf, uint16_t payloadBufSize, uint16_t & payloadLen);
    WEAVE_ERROR BeginValidation(const BeginSessionContext & msgCtx, ValidationContext & validCtx,
            WeaveCertificateSet & certSet);
    WEAVE_ERROR OnPeerCertsLoaded(const BeginSessionContext & msgCtx, WeaveDN & subjectDN,
            CertificateKeyId & subjectKeyId, ValidationContext & validCtx, WeaveCertificateSet& certSet);
    WEAVE_ERROR HandleValidationResult(const BeginSessionContext & msgCtx, ValidationContext & validCtx,
            WeaveCertificateSet & certSet, WEAVE_ERROR & validRes);
    void EndValidation(const BeginSessionContext & msgCtx, ValidationContext & validCtx,
            WeaveCertificateSet & certSet);

#endif // WEAVE_CONFIG_LEGACY_CASE_AUTH_DELEGATE
};


/**
 * Implements the core logic of the Weave CASE protocol.
 */
class NL_DLL_EXPORT WeaveCASEEngine
{
public:
    enum EngineState
    {
        kState_Idle                             = 0,
        kState_BeginRequestGenerated            = 1,
        kState_BeginResponseProcessed           = 2,
        kState_BeginRequestProcessed            = 3,
        kState_BeginResponseGenerated           = 4,
        kState_Complete                         = 5,
        kState_Failed                           = 6
    };

    WeaveCASEAuthDelegate *AuthDelegate;                ///< Authentication delegate object
    uint8_t State;                                      ///< [READ-ONLY] Current protocol state
    uint8_t EncryptionType;                             ///< [READ-ONLY] Proposed Weave encryption type
    uint16_t SessionKeyId;                              ///< [READ-ONLY] Proposed session key id

    void Init(void);
    void Shutdown(void);
    void Reset(void);

    void SetAlternateConfigs(BeginSessionRequestContext & reqCtx);

    void SetAlternateCurves(BeginSessionRequestContext & reqCtx);

    WEAVE_ERROR GenerateBeginSessionRequest(BeginSessionRequestContext & reqCtx, PacketBuffer * msgBuf);

    WEAVE_ERROR ProcessBeginSessionRequest(PacketBuffer * msgBuf, BeginSessionRequestContext & reqCtx, ReconfigureContext & reconfCtx);

    WEAVE_ERROR GenerateBeginSessionResponse(BeginSessionResponseContext & respCtx, PacketBuffer * msgBuf,
                                             BeginSessionRequestContext & reqCtx);

    WEAVE_ERROR ProcessBeginSessionResponse(PacketBuffer * msgBuf, BeginSessionResponseContext & respCtx);

    WEAVE_ERROR GenerateInitiatorKeyConfirm(PacketBuffer * msgBuf);

    WEAVE_ERROR ProcessInitiatorKeyConfirm(PacketBuffer * msgBuf);

    WEAVE_ERROR ProcessReconfigure(PacketBuffer * msgBuf, ReconfigureContext & reconfCtx);

    WEAVE_ERROR GetSessionKey(const WeaveEncryptionKey *& encKey);

    bool IsInitiator() const;
    uint32_t SelectedConfig() const;
    uint32_t SelectedCurve() const;
    bool PerformingKeyConfirm() const;

    uint8_t AllowedConfigs() const;
    void SetAllowedConfigs(uint8_t allowedConfigs);
    bool IsAllowedConfig(uint32_t config) const;

    uint8_t AllowedCurves() const;
    void SetAllowedCurves(uint8_t allowedCurves);
    bool IsAllowedCurve(uint32_t curveId) const;

    bool ResponderRequiresKeyConfirm() const;
    void SetResponderRequiresKeyConfirm(bool val);

    uint8_t CertType() const;
    void SetCertType(uint8_t certType);

#if WEAVE_CONFIG_SECURITY_TEST_MODE
    bool UseKnownECDHKey() const;
    void SetUseKnownECDHKey(bool val);
#endif

private:
    // clang-format off
    enum
    {
        kMaxHashLength                          = SHA256::kHashLength,
        kMaxECDHPrivateKeySize                  = ((WEAVE_CONFIG_MAX_EC_BITS + 7) / 8) + 1,
        kMaxECDHSharedSecretSize                = kMaxECDHPrivateKeySize
    };

    enum
    {
        kFlag_IsInitiator                       = 0x80,
        kFlag_PerformingKeyConfirm              = 0x40,
        kFlag_IsUsingConfig1                    = 0x20,
        kFlag_ResponderRequiresKeyConfirm       = 0x10,
        kFlag_HasReconfigured                   = 0x08,

#if WEAVE_CONFIG_SECURITY_TEST_MODE
        kFlag_UseKnownECDHKey                   = 0x04,
#endif

        kFlag_Reserved                          = kCASEAllowedConfig_Mask // Bottom 2 bits reserved for allowed configs flags
    };
    // clang-format on

    union
    {
        struct
        {
            uint16_t ECDHPrivateKeyLength;
            uint8_t ECDHPrivateKey[kMaxECDHPrivateKeySize];
            uint8_t RequestMsgHash[kMaxHashLength];
        } BeforeKeyGen;
        struct
        {
            WeaveEncryptionKey EncryptionKey;
            uint8_t InitiatorKeyConfirmHash[kMaxHashLength];
        } AfterKeyGen;
    } mSecureState;
    uint32_t mCurveId;
    uint8_t mAllowedCurves;
    uint8_t mFlags;
    uint8_t mCertType;

    bool IsUsingConfig1() const;
    void SetSelectedConfig(uint32_t config);
    void SetIsInitiator(bool val);
    bool HasReconfigured() const;
    void SetHasReconfigured(bool val);
    void SetPerformingKeyConfirm(bool val);
    bool IsConfig1Allowed() const;
    bool IsConfig2Allowed() const;
    uint8_t ConfigHashLength() const;
    WEAVE_ERROR VerifyProposedConfig(BeginSessionRequestContext & reqCtx, uint32_t & selectedAltConfig);
    WEAVE_ERROR VerifyProposedCurve(BeginSessionRequestContext & reqCtx, uint32_t & selectedAltCurve);
    WEAVE_ERROR AppendNewECDHKey(BeginSessionContext & msgCtx, PacketBuffer * msgBuf);
    WEAVE_ERROR AppendCertInfo(BeginSessionContext & msgCtx, PacketBuffer * msgBuf);
    WEAVE_ERROR AppendPayload(BeginSessionContext & msgCtx, PacketBuffer * msgBuf);
    WEAVE_ERROR AppendSignature(BeginSessionContext & msgCtx, PacketBuffer * msgBuf, uint8_t * msgHash);
    WEAVE_ERROR VerifySignature(BeginSessionContext & msgCtx, PacketBuffer * msgBuf, uint8_t * msgHash);
    static WEAVE_ERROR DecodeCertificateInfo(BeginSessionContext & msgCtx, WeaveCertificateSet & certSet,
            WeaveDN & entityCertDN, CertificateKeyId & entityCertSubjectKeyId);
    WEAVE_ERROR DeriveSessionKeys(EncodedECPublicKey & pubKey, const uint8_t * respMsgHash, uint8_t * responderKeyConfirmHash);
    void GenerateHash(const uint8_t * inData, uint16_t inDataLen, uint8_t * hash);
    void GenerateKeyConfirmHashes(const uint8_t * keyConfirmKey, uint8_t * singleHash, uint8_t * doubleHash);
};


// Utility Functions

WEAVE_ERROR EncodeCASECertInfo(uint8_t * buf, uint16_t bufSize, uint16_t& certInfoLen,
        const uint8_t * entityCert, uint16_t entityCertLen,
        const uint8_t * intermediateCert, uint16_t intermediateCertLen);

WEAVE_ERROR EncodeCASECertInfo(TLVWriter & writer,
        const uint8_t * entityCert, uint16_t entityCertLen,
        const uint8_t * intermediateCert, uint16_t intermediateCertLen);


// Inline Methods

inline bool BeginSessionContext::IsBeginSessionRequest() const
{
    return GetFlag(Flags, kFlag_IsBeginSessionRequest);
}

inline void BeginSessionContext::SetIsBeginSessionRequest(bool val)
{
    SetFlag(Flags, kFlag_IsBeginSessionRequest, val);
}

inline bool BeginSessionContext::IsInitiator() const
{
    return GetFlag(Flags, kFlag_IsInitiator);
}

inline void BeginSessionContext::SetIsInitiator(bool val)
{
    SetFlag(Flags, kFlag_IsInitiator, val);
}

inline bool BeginSessionContext::PerformKeyConfirm() const
{
    return GetFlag(Flags, kFlag_PerformKeyConfirm);
}

inline void BeginSessionContext::SetPerformKeyConfirm(bool val)
{
    SetFlag(Flags, kFlag_PerformKeyConfirm, val);
}

inline uint16_t BeginSessionRequestContext::HeadLength(void)
{
    return (1 +                                 // control header
            1 +                                 // alternate config count
            1 +                                 // alternate curve count
            1 +                                 // DH public key length
            2 +                                 // certificate information length
            2 +                                 // payload length
            4 +                                 // proposed protocol config
            4 +                                 // proposed elliptic curve
            2 +                                 // session key id
            (AlternateConfigCount * 4) +        // alternate config list
            (AlternateCurveCount * 4));         // alternate curve list
}

inline void BeginSessionRequestContext::Reset(void)
{
    memset(this, 0, sizeof(*this));
    Flags = kFlag_IsBeginSessionRequest;
}

inline uint16_t BeginSessionResponseContext::HeadLength(void)
{
    return (1 +                                 // control header
            1 +                                 // DH public key length
            2 +                                 // certificate information length
            2);                                 // payload length
}

inline void BeginSessionResponseContext::Reset(void)
{
    memset(this, 0, sizeof(*this));
}

inline void ReconfigureContext::Reset(void)
{
    memset(this, 0, sizeof(*this));
}

#if WEAVE_CONFIG_LEGACY_CASE_AUTH_DELEGATE

inline WEAVE_ERROR WeaveCASEAuthDelegate::EncodeNodePayload(const BeginSessionContext & msgCtx,
        uint8_t * payloadBuf, uint16_t payloadBufSize, uint16_t & payloadLen)
{
    return GetNodePayload(msgCtx.IsInitiator(), payloadBuf, payloadBufSize, payloadLen);
}

inline WEAVE_ERROR WeaveCASEAuthDelegate::BeginValidation(const BeginSessionContext & msgCtx, ValidationContext & validCtx,
        WeaveCertificateSet & certSet)
{
    return BeginCertValidation(msgCtx.IsInitiator(), certSet, validCtx);
}

inline WEAVE_ERROR WeaveCASEAuthDelegate::OnPeerCertsLoaded(const BeginSessionContext & msgCtx, WeaveDN & subjectDN,
        CertificateKeyId & subjectKeyId, ValidationContext & validCtx, WeaveCertificateSet& certSet)
{
    return WEAVE_NO_ERROR;
}

inline WEAVE_ERROR WeaveCASEAuthDelegate::HandleValidationResult(const BeginSessionContext & msgCtx, ValidationContext & validCtx,
        WeaveCertificateSet & certSet, WEAVE_ERROR & validRes)
{
    return HandleCertValidationResult(msgCtx.IsInitiator(), validRes, validCtx.SigningCert, msgCtx.PeerNodeId, certSet, validCtx);
}

inline void WeaveCASEAuthDelegate::EndValidation(const BeginSessionContext & msgCtx, ValidationContext & validCtx,
        WeaveCertificateSet & certSet)
{
    EndCertValidation(certSet, validCtx);
}

#endif // WEAVE_CONFIG_LEGACY_CASE_AUTH_DELEGATE

inline bool WeaveCASEEngine::IsUsingConfig1() const
{
#if WEAVE_CONFIG_SUPPORT_CASE_CONFIG1
    return (mFlags & kFlag_IsUsingConfig1) != 0;
#else
    return false;
#endif
}

inline void WeaveCASEEngine::SetSelectedConfig(uint32_t config)
{
#if WEAVE_CONFIG_SUPPORT_CASE_CONFIG1
    if (config == kCASEConfig_Config1)
        mFlags |= kFlag_IsUsingConfig1;
    else
        mFlags &= ~kFlag_IsUsingConfig1;
#else
    IgnoreUnusedVariable(config);
#endif
}

inline uint32_t WeaveCASEEngine::SelectedCurve() const
{
    return mCurveId;
}

inline uint8_t WeaveCASEEngine::AllowedConfigs() const
{
    return mFlags & kCASEAllowedConfig_Mask;
}

inline void WeaveCASEEngine::SetAllowedConfigs(uint8_t allowedConfigs)
{
    mFlags = (mFlags & ~kCASEAllowedConfig_Mask) | (allowedConfigs & kCASEAllowedConfig_Mask);
}

inline bool WeaveCASEEngine::IsConfig1Allowed() const
{
#if WEAVE_CONFIG_SUPPORT_CASE_CONFIG1
    return (mFlags & kCASEAllowedConfig_Config1) != 0;
#else
    return false;
#endif
}

inline bool WeaveCASEEngine::IsConfig2Allowed() const
{
    return (mFlags & kCASEAllowedConfig_Config2) != 0;
}

inline uint8_t WeaveCASEEngine::AllowedCurves() const
{
    return mAllowedCurves;
}

inline void WeaveCASEEngine::SetAllowedCurves(uint8_t allowedCurves)
{
    mAllowedCurves = allowedCurves;
}

inline bool WeaveCASEEngine::IsAllowedCurve(uint32_t curveId) const
{
    return IsCurveInSet(curveId, mAllowedCurves);
}

inline bool WeaveCASEEngine::IsInitiator() const
{
    return (mFlags & kFlag_IsInitiator) != 0;
}

inline void WeaveCASEEngine::SetIsInitiator(bool val)
{
    if (val)
        mFlags |= kFlag_IsInitiator;
    else
        mFlags &= ~kFlag_IsInitiator;
}

inline bool WeaveCASEEngine::PerformingKeyConfirm() const
{
    return (mFlags & kFlag_PerformingKeyConfirm) != 0;
}

inline void WeaveCASEEngine::SetPerformingKeyConfirm(bool val)
{
    if (val)
        mFlags |= kFlag_PerformingKeyConfirm;
    else
        mFlags &= ~kFlag_PerformingKeyConfirm;
}

inline bool WeaveCASEEngine::ResponderRequiresKeyConfirm() const
{
    return (mFlags & kFlag_ResponderRequiresKeyConfirm) != 0;
}

inline void WeaveCASEEngine::SetResponderRequiresKeyConfirm(bool val)
{
    if (val)
        mFlags |= kFlag_ResponderRequiresKeyConfirm;
    else
        mFlags &= ~kFlag_ResponderRequiresKeyConfirm;
}

inline bool WeaveCASEEngine::HasReconfigured() const
{
    return (mFlags & kFlag_HasReconfigured) != 0;
}

inline void WeaveCASEEngine::SetHasReconfigured(bool val)
{
    if (val)
        mFlags |= kFlag_HasReconfigured;
    else
        mFlags &= ~kFlag_HasReconfigured;
}

inline uint8_t WeaveCASEEngine::ConfigHashLength() const
{
    if (IsUsingConfig1())
        return SHA1::kHashLength;
    else
        return SHA256::kHashLength;
}

inline uint8_t WeaveCASEEngine::CertType() const
{
    return mCertType;
}

inline void WeaveCASEEngine::SetCertType(uint8_t certType)
{
    mCertType = certType;
}

#if WEAVE_CONFIG_SECURITY_TEST_MODE

inline bool WeaveCASEEngine::UseKnownECDHKey() const
{
    return (mFlags & kFlag_UseKnownECDHKey) != 0;
}

inline void WeaveCASEEngine::SetUseKnownECDHKey(bool val)
{
    if (val)
        mFlags |= kFlag_UseKnownECDHKey;
    else
        mFlags &= ~kFlag_UseKnownECDHKey;
}

#endif // WEAVE_CONFIG_SECURITY_TEST_MODE


} // namespace CASE
} // namespace Security
} // namespace Profiles
} // namespace Weave
} // namespace nl

#endif /* WEAVECASE_H_ */
