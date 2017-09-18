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


// CASE Protocol Configuration Values
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


// Base class for CASE Begin Session Request/Response messages.
class BeginSessionMessageBase
{
public:
    uint64_t PeerNodeId;
    EncodedECPublicKey ECDHPublicKey;
    uint32_t ProtocolConfig;
    uint32_t CurveId;
    const uint8_t *Payload;
    const uint8_t *CertInfo;
    const uint8_t *Signature;
    uint16_t PayloadLength;
    uint16_t CertInfoLength;
    uint16_t SignatureLength;
    bool PerformKeyConfirm;
};


// In-memory representation of a CASE BeginSessionRequest message.
class BeginSessionRequestMessage : public BeginSessionMessageBase
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

    WEAVE_ERROR EncodeHead(PacketBuffer *msgBuf);
    WEAVE_ERROR DecodeHead(PacketBuffer *msgBuf);
    uint16_t HeadLength(void) { return 18 + (AlternateConfigCount * 4) + (AlternateCurveCount * 4); }
    void Reset(void) { memset(this, 0, sizeof(*this)); }
    bool IsAltConfig(uint32_t config) const;
};


// In-memory representation of a CASE BeginSessionResponse message.
class BeginSessionResponseMessage : public BeginSessionMessageBase
{
public:
    const uint8_t *KeyConfirmHash;
    uint8_t KeyConfirmHashLength;

    WEAVE_ERROR EncodeHead(PacketBuffer *msgBuf);
    WEAVE_ERROR DecodeHead(PacketBuffer *msgBuf);
    uint16_t HeadLength(void) { return 6; }
    void Reset(void) { memset(this, 0, sizeof(*this)); }
};


// In-memory representation of a CASE Reconfigure message.
class ReconfigureMessage
{
public:
    uint32_t ProtocolConfig;
    uint32_t CurveId;

    WEAVE_ERROR Encode(PacketBuffer *buf);
    void Reset(void) { memset(this, 0, sizeof(*this)); }
    static WEAVE_ERROR Decode(PacketBuffer *buf, ReconfigureMessage& msg);
};


// Abstract delegate class called by CASE engine to perform various
// actions related to authentication during a CASE exchange.
class WeaveCASEAuthDelegate
{
public:
    // Get the CASE Certificate Information structure for the local node.
    virtual WEAVE_ERROR GetNodeCertInfo(bool isInitiator, uint8_t *buf, uint16_t bufSize, uint16_t& certInfoLen) = 0;

    // Get the local node's private key.
    virtual WEAVE_ERROR GetNodePrivateKey(bool isInitiator, const uint8_t *& weavePrivKey, uint16_t& weavePrivKeyLen) = 0;

    // Called when the CASE engine is done with the buffer returned by GetNodePrivateKey().
    virtual WEAVE_ERROR ReleaseNodePrivateKey(const uint8_t *weavePrivKey) = 0;

    // Get payload information, if any, to be included in the message to the peer.
    virtual WEAVE_ERROR GetNodePayload(bool isInitiator, uint8_t *buf, uint16_t bufSize, uint16_t& payloadLen) = 0;

    // Prepare the supplied certificate set and validation context for use in validating the certificate of a peer.
    // This method is responsible for loading the trust anchors into the certificate set.
    virtual WEAVE_ERROR BeginCertValidation(bool isInitiator, WeaveCertificateSet& certSet, ValidationContext& validContext) = 0;

    // Called with the results of validating the peer's certificate. If basic cert validation was successful, this method can
    // cause validation to fail by setting validRes, e.g. in the event that the peer's certificate is somehow unacceptable.
    virtual WEAVE_ERROR HandleCertValidationResult(bool isInitiator, WEAVE_ERROR& validRes, WeaveCertificateData *peerCert,
            uint64_t peerNodeId, WeaveCertificateSet& certSet, ValidationContext& validContext) = 0;

    // Called when peer certificate validation is complete.
    virtual WEAVE_ERROR EndCertValidation(WeaveCertificateSet& certSet, ValidationContext& validContext) = 0;
};



// Implements the core logic of the Weave CASE protocol.
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

    WeaveCASEAuthDelegate *AuthDelegate;                // Authentication delegate object
    uint8_t State;                                      // [READ-ONLY] Current protocol state
    uint8_t EncryptionType;                             // [READ-ONLY] Proposed Weave encryption type
    uint16_t SessionKeyId;                              // [READ-ONLY] Proposed session key id

    void Init(void);
    void Shutdown(void);
    void Reset(void);

    void SetAlternateConfigs(BeginSessionRequestMessage& req);

    void SetAlternateCurves(BeginSessionRequestMessage& req);

    WEAVE_ERROR GenerateBeginSessionRequest(BeginSessionRequestMessage& req, PacketBuffer *msgBuf);

    WEAVE_ERROR ProcessBeginSessionRequest(PacketBuffer *msgBuf, BeginSessionRequestMessage& req, ReconfigureMessage& reconf);

    WEAVE_ERROR GenerateBeginSessionResponse(BeginSessionResponseMessage& resp, PacketBuffer *msgBuf,
                                             BeginSessionRequestMessage& req);

    WEAVE_ERROR ProcessBeginSessionResponse(PacketBuffer *msgBuf, BeginSessionResponseMessage& resp);

    WEAVE_ERROR GenerateInitiatorKeyConfirm(PacketBuffer *msgBuf);

    WEAVE_ERROR ProcessInitiatorKeyConfirm(PacketBuffer *msgBuf);

    WEAVE_ERROR ProcessReconfigure(PacketBuffer *msgBuf, ReconfigureMessage& reconf);

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
    WEAVE_ERROR VerifyProposedConfig(BeginSessionRequestMessage& req, uint32_t& selectedAltConfig);
    WEAVE_ERROR VerifyProposedCurve(BeginSessionRequestMessage& req, uint32_t& selectedAltCurve);
    WEAVE_ERROR AppendNewECDHKey(BeginSessionMessageBase& msg, PacketBuffer *msgBuf);
    WEAVE_ERROR AppendCertInfo(BeginSessionMessageBase& msg, PacketBuffer *msgBuf);
    WEAVE_ERROR AppendPayload(BeginSessionMessageBase& msg, PacketBuffer *msgBuf);
    WEAVE_ERROR AppendSignature(BeginSessionMessageBase& msg, PacketBuffer *msgBuf, uint8_t *msgHash);
    WEAVE_ERROR VerifySignature(BeginSessionMessageBase& msg, PacketBuffer *msgBuf, uint8_t *msgHash);
    static WEAVE_ERROR DecodeCertificateInfo(BeginSessionMessageBase& msg, WeaveCertificateSet& certSet,
            WeaveDN& entityCertDN, CertificateKeyId& entityCertSubjectKeyId);
    WEAVE_ERROR DeriveSessionKeys(EncodedECPublicKey& pubKey, const uint8_t *respMsgHash, uint8_t *responderKeyConfirmHash);
    void GenerateHash(const uint8_t *inData, uint16_t inDataLen, uint8_t *hash);
    void GenerateKeyConfirmHashes(const uint8_t *keyConfirmKey, uint8_t *singleHash, uint8_t *doubleHash);
};

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
