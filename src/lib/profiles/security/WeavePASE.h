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
 *      responders for the Weave Password Authenticated Session
 *      Establishment (PASE) protocol.
 *
 */

#ifndef WEAVEPASE_H_
#define WEAVEPASE_H_

#include <Weave/Core/WeaveConfig.h>

#define WEAVE_IS_EC_PASE_ENABLED  ( WEAVE_CONFIG_SUPPORT_PASE_CONFIG2 || \
                                    WEAVE_CONFIG_SUPPORT_PASE_CONFIG3 || \
                                    WEAVE_CONFIG_SUPPORT_PASE_CONFIG4 || \
                                    WEAVE_CONFIG_SUPPORT_PASE_CONFIG5 )

#include <Weave/Support/NLDLLUtil.h>
#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveVendorIdentifiers.hpp>
#include <Weave/Core/WeaveEncoding.h>
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Support/crypto/HashAlgos.h>
#include <Weave/Support/crypto/HMAC.h>
#include <Weave/Support/crypto/HKDF.h>

#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG1 && !WEAVE_WITH_OPENSSL
#error "INVALID WEAVE CONFIG: PASE Config1 enabled but OpenSSL not available (WEAVE_CONFIG_SUPPORT_PASE_CONFIG1 == 1 && WEAVE_WITH_OPENSSL == 0)."
#endif

#if WEAVE_IS_EC_PASE_ENABLED
#include <Weave/Support/crypto/EllipticCurve.h>
#endif

#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG1
#include "openssl/bn.h"
struct JPAKE_CTX;
#endif

/**
 *   @namespace nl::Weave::Profiles::Security::PASE
 *
 *   @brief
 *     This namespace includes all interfaces within Weave for the
 *     Password Authenticated Session Establishment (PASE) protocol
 *     within the Weave security profile.
 */

namespace nl {
namespace Weave {
namespace Profiles {
namespace Security {
namespace PASE {

using nl::Weave::System::PacketBuffer;
using nl::Weave::WeaveEncryptionKey;
#if WEAVE_IS_EC_PASE_ENABLED
using nl::Weave::ASN1::OID;
using nl::Weave::Crypto::EllipticCurveJPAKE;
#endif

// PASE Protocol Configurations
enum
{
    // -- PASE Protocol Configuration Values
    kPASEConfig_Unspecified                     = 0,
    kPASEConfig_Config0_TEST_ONLY               = (kWeaveVendor_NestLabs << 16) | 0,
    kPASEConfig_Config1                         = (kWeaveVendor_NestLabs << 16) | 1,
    kPASEConfig_Config2                         = (kWeaveVendor_NestLabs << 16) | 2,
    kPASEConfig_Config3                         = (kWeaveVendor_NestLabs << 16) | 3,
    kPASEConfig_Config4                         = (kWeaveVendor_NestLabs << 16) | 4,
    kPASEConfig_Config5                         = (kWeaveVendor_NestLabs << 16) | 5,
    kPASEConfig_ConfigLast                      = (kWeaveVendor_NestLabs << 16) | 5,
    kPASEConfig_ConfigDefault                   = kPASEConfig_Config4,
    kPASEConfig_ConfigNestNumberMask            = 0x07,

    // -- Security Strength Metric for PASE Configuration
    kPASEConfig_Config0SecurityStrength         = 10,
    kPASEConfig_Config1SecurityStrength         = 80,
    kPASEConfig_Config2SecurityStrength         = 80,
    kPASEConfig_Config3SecurityStrength         = 96,
    kPASEConfig_Config4SecurityStrength         = 112,
    kPASEConfig_Config5SecurityStrength         = 128,

    // -- PASE Supported Configurations Bit Masks
    kPASEConfig_SupportConfig0Bit_TEST_ONLY     = 0x01,
    kPASEConfig_SupportConfig1Bit               = 0x02,
    kPASEConfig_SupportConfig2Bit               = 0x04,
    kPASEConfig_SupportConfig3Bit               = 0x08,
    kPASEConfig_SupportConfig4Bit               = 0x10,
    kPASEConfig_SupportConfig5Bit               = 0x20,
    kPASEConfig_SupportedConfigs                = (0x00 |
#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG0_TEST_ONLY
                                                   kPASEConfig_SupportConfig0Bit_TEST_ONLY |
#endif
#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG1
                                                   kPASEConfig_SupportConfig1Bit |
#endif
#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG2
                                                   kPASEConfig_SupportConfig2Bit |
#endif
#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG3
                                                   kPASEConfig_SupportConfig3Bit |
#endif
#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG4
                                                   kPASEConfig_SupportConfig4Bit |
#endif
#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG5
                                                   kPASEConfig_SupportConfig5Bit |
#endif
                                                   0x00)
};

// PASE Header Field Definitions
enum
{
    // Control Header Fields
    kPASEHeader_SessionKeyMask                  = 0x0000FFFF,
    kPASEHeader_SessionKeyShift                 = 0,
    kPASEHeader_EncryptionTypeMask              = 0x000F0000,
    kPASEHeader_EncryptionTypeShift             = 16,
    kPASEHeader_PasswordSourceMask              = 0x00F00000,
    kPASEHeader_PasswordSourceShift             = 20,
    kPASEHeader_PerformKeyConfirmFlag           = 0x80000000,
    kPASEHeader_ControlHeaderUnusedBits         = ~(kPASEHeader_SessionKeyMask |
                                                    kPASEHeader_EncryptionTypeMask |
                                                    kPASEHeader_PasswordSourceMask |
                                                    kPASEHeader_PerformKeyConfirmFlag),

    // Size Header Fields and Values
    kPASEHeader_GXWordCountMask                 = 0x000000FF,
    kPASEHeader_GXWordCountShift                = 0,
    kPASEHeader_GXWordCountMaxConfig0           = 16,
    kPASEHeader_GXWordCountMaxConfig1           = 32,
    kPASEHeader_ZKPXGRWordCountMask             = 0x0000FF00,
    kPASEHeader_ZKPXGRWordCountShift            = 8,
    kPASEHeader_ZKPXGRWordCountMaxConfig0       = 16,
    kPASEHeader_ZKPXGRWordCountMaxConfig1       = 32,
    kPASEHeader_ZKPXBWordCountMask              = 0x00FF0000,
    kPASEHeader_ZKPXBWordCountShift             = 16,
    kPASEHeader_ZKPXBWordCountMaxConfig0        = 8,
    kPASEHeader_ZKPXBWordCountMaxConfig1        = 5,
    kPASEHeader_AlternateConfigCountMask        = 0xFF000000,
    kPASEHeader_AlternateConfigCountShift       = 24,
    kPASEHeader_KeyConfirmWordCountMask         = kPASEHeader_AlternateConfigCountMask, // Alternate interpretation of 0xFF000000 field used in
    kPASEHeader_KeyConfirmWordCountShift        = kPASEHeader_AlternateConfigCountShift, //  InitiatorStep2Message.

    kPASESizeHeader_MaxConstantSizesConfig0     =
       ((kPASEHeader_GXWordCountMaxConfig0 << kPASEHeader_GXWordCountShift) & kPASEHeader_GXWordCountMask) |
       ((kPASEHeader_ZKPXGRWordCountMaxConfig0 << kPASEHeader_ZKPXGRWordCountShift) & kPASEHeader_ZKPXGRWordCountMask) |
       ((kPASEHeader_ZKPXBWordCountMaxConfig0 << kPASEHeader_ZKPXBWordCountShift) & kPASEHeader_ZKPXBWordCountMask),
    kPASESizeHeader_MaxConstantSizesConfig1     =
       ((kPASEHeader_GXWordCountMaxConfig1 << kPASEHeader_GXWordCountShift) & kPASEHeader_GXWordCountMask) |
       ((kPASEHeader_ZKPXGRWordCountMaxConfig1 << kPASEHeader_ZKPXGRWordCountShift) & kPASEHeader_ZKPXGRWordCountMask) |
       ((kPASEHeader_ZKPXBWordCountMaxConfig1 << kPASEHeader_ZKPXBWordCountShift) & kPASEHeader_ZKPXBWordCountMask)

};

// PASE Config0 Parameters
#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG0_TEST_ONLY
enum
{
    kPASEConfig0_GXByteCount                    = 4 * kPASEHeader_GXWordCountMaxConfig0,
    kPASEConfig0_ZKPXGRByteCount                = 4 * kPASEHeader_ZKPXGRWordCountMaxConfig0,
    kPASEConfig0_ZKPXBByteCount                 = 4 * kPASEHeader_ZKPXBWordCountMaxConfig0,
    kPASEConfig0_GXStep1p1Value                 = 0x3A,
    kPASEConfig0_ZKPXGRStep1p1Value             = 0xF1,
    kPASEConfig0_ZKPXBStep1p1Value              = 0xAA,
    kPASEConfig0_GXStep1p2Value                 = 0x5C,
    kPASEConfig0_ZKPXGRStep1p2Value             = 0x55,
    kPASEConfig0_ZKPXBStep1p2Value              = 0x6B,
    kPASEConfig0_GXStep2Value                   = 0x9E,
    kPASEConfig0_ZKPXGRStep2Value               = 0x37,
    kPASEConfig0_ZKPXBStep2Value                = 0xDA
};
#endif

enum
{
    // Key Meterial Length for Config 0 and Ellipric Curve Configs
    kKeyMaterialLength_Config0_EC               = Platform::Security::SHA256::kHashLength,

    // Hash Length of ZKP_GR value
    kStep2ZKPXGRHashLength_Config1              = Platform::Security::SHA1::kHashLength,
    kStep2ZKPXGRHashLength_Config0_EC           = Platform::Security::SHA256::kHashLength,
    kStep2ZKPXGRHashLengthMax                   = Platform::Security::SHA256::kHashLength,

    // Length of Key Confirmation Key, which is used to generate Key Configmation Hashes
    kKeyConfirmKeyLength_Config1                = Platform::Security::SHA1::kHashLength,
    kKeyConfirmKeyLength_Config0_EC             = Platform::Security::SHA256::kHashLength,
    kKeyConfirmKeyLengthMax                     = Platform::Security::SHA256::kHashLength,

    // Length of Key Confirmation Hash
    kKeyConfirmHashLength_Config1               = Platform::Security::SHA1::kHashLength,
    kKeyConfirmHashLength_Config0_EC            = Platform::Security::SHA256::kHashLength,
    kKeyConfirmHashLengthMax                    = Platform::Security::SHA256::kHashLength
};

enum
{
    kMaxAlternateProtocolConfigs                = 3,
};

// Implements the core logic of the Weave PASE protocol.
class NL_DLL_EXPORT WeavePASEEngine
{
public:

    enum EngineState
    {
        kState_Reset                            = 0,

        // Initiator States
        kState_InitiatorStatesBase              = 10,
        kState_InitiatorStatesEnd               = 19,
        kState_InitiatorStep1Generated          = kState_InitiatorStatesBase + 0,
        kState_ResponderReconfigProcessed       = kState_InitiatorStatesBase + 1,
        kState_ResponderStep1Processed          = kState_InitiatorStatesBase + 2,
        kState_ResponderStep2Processed          = kState_InitiatorStatesBase + 3,
        kState_InitiatorStep2Generated          = kState_InitiatorStatesBase + 4,
        kState_InitiatorDone                    = kState_InitiatorStatesBase + 5,
        kState_InitiatorFailed                  = kState_InitiatorStatesBase + 6,

        // Responder States
        kState_ResponderStatesBase              = 20,
        kState_ResponderStatesEnd               = 29,
        kState_InitiatorStep1Processed          = kState_ResponderStatesBase + 0,
        kState_ResponderStep1Generated          = kState_ResponderStatesBase + 1,
        kState_ResponderStep2Generated          = kState_ResponderStatesBase + 2,
        kState_InitiatorStep2Processed          = kState_ResponderStatesBase + 3,
        kState_ResponderDone                    = kState_ResponderStatesBase + 4,
        kState_ResponderFailed                  = kState_ResponderStatesBase + 5
    };

#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG1
    struct JPAKE_CTX *JPAKECtx;
#endif
#if WEAVE_IS_EC_PASE_ENABLED
    EllipticCurveJPAKE mEllipticCurveJPAKE;
#endif
    EngineState State;
    uint32_t ProtocolConfig;
    const uint8_t *Pw;
    uint16_t PwLen;
    uint16_t SessionKeyId;
    uint8_t EncryptionType;
    uint8_t AllowedPASEConfigs;
    uint8_t PwSource;
    bool PerformKeyConfirmation;

    void Init(void);
    void Shutdown(void);
    void Reset(void);
    bool IsInitiator(void) const;
    bool IsResponder(void) const;

    WEAVE_ERROR GenerateInitiatorStep1(PacketBuffer *buf, uint32_t proposedPASEConfig, uint64_t localNodeId, uint64_t peerNodeId, uint16_t sessionKeyId, uint8_t encType, uint8_t pwSrc, WeaveFabricState *FabricState, bool confirmKey);
    WEAVE_ERROR ProcessInitiatorStep1(PacketBuffer *buf, uint64_t localNodeId, uint64_t peerNodeId, WeaveFabricState *FabricState);
    WEAVE_ERROR GenerateResponderStep1(PacketBuffer *buf);
    WEAVE_ERROR GenerateResponderStep2(PacketBuffer *buf);
    WEAVE_ERROR ProcessResponderStep1(PacketBuffer *buf);
    WEAVE_ERROR ProcessResponderStep2(PacketBuffer *buf);
    WEAVE_ERROR GenerateInitiatorStep2(PacketBuffer *buf);
    WEAVE_ERROR ProcessInitiatorStep2(PacketBuffer *buf);
    WEAVE_ERROR GenerateResponderKeyConfirm(PacketBuffer *buf);
    WEAVE_ERROR ProcessResponderKeyConfirm(PacketBuffer *buf);
    WEAVE_ERROR GenerateResponderReconfigure(PacketBuffer *buf);
    WEAVE_ERROR ProcessResponderReconfigure(PacketBuffer *buf, uint32_t & proposedPASEConfig);
    WEAVE_ERROR GetSessionKey(const WeaveEncryptionKey *& encKey);

private:
    union
    {
        WeaveEncryptionKey EncryptionKey;
#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG0_TEST_ONLY
        uint8_t KeyMeterial_Config0[kKeyMaterialLength_Config0_EC];
#endif
    };
    union
    {
        uint8_t ResponderKeyConfirmHash[kKeyConfirmHashLengthMax];
        uint8_t ResponderStep2ZKPXGRHash[kStep2ZKPXGRHashLengthMax];
    };

    WEAVE_ERROR InitState(uint64_t localNodeId, uint64_t peerNodeId, uint8_t pwSrc, WeaveFabricState *FabricState, uint32_t *altConfigs, uint8_t altConfigsCount, bool isInitiator);
#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG1
    WEAVE_ERROR FormProtocolContextString(uint64_t localNodeId, uint64_t peerNodeId, uint8_t pwSrc, uint32_t *altConfigs, uint8_t altConfigsCount, bool isInitiator, char *buf, size_t bufSize);
    WEAVE_ERROR GenerateStep1Data_Config1(PacketBuffer *buf, uint16_t &stepDataLen);
    WEAVE_ERROR ProcessStep1Data_Config1(PacketBuffer *buf, uint16_t &stepDataLen, uint8_t gxWordCount, uint8_t zkpxgrWordCount, uint8_t zpkxbWordCount);
    WEAVE_ERROR GenerateStep2Data_Config1(PacketBuffer *buf, uint16_t &stepDataLen, uint8_t *Step2ZKPXGRHash);
    WEAVE_ERROR ProcessStep2Data_Config1(PacketBuffer *buf, uint16_t &stepDataLen, uint8_t gxWordCount, uint8_t zkpxgrWordCount, uint8_t zpkxbWordCount, uint8_t *step2ZKPXGRHash);
#endif
#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG0_TEST_ONLY || WEAVE_IS_EC_PASE_ENABLED
    WEAVE_ERROR FormProtocolContextData(uint64_t localNodeId, uint64_t peerNodeId, uint8_t pwSrc, uint32_t *altConfigs, uint8_t altConfigsCount, bool isInitiator, uint8_t *buf, size_t bufSize, uint16_t &contextLen);
#endif
#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG0_TEST_ONLY
    WEAVE_ERROR GenerateStep1Data_Config0_TEST_ONLY(PacketBuffer *buf, uint16_t &stepDataLen);
    WEAVE_ERROR ProcessStep1Data_Config0_TEST_ONLY(PacketBuffer *buf, uint16_t &stepDataLen, uint8_t gxWordCount, uint8_t zkpxgrWordCount, uint8_t zkpxbWordCount);
    WEAVE_ERROR GenerateStep2Data_Config0_TEST_ONLY(PacketBuffer *buf, uint16_t &stepDataLen, uint8_t *Step2ZKPXGRHash);
    WEAVE_ERROR ProcessStep2Data_Config0_TEST_ONLY(PacketBuffer *buf, uint16_t &stepDataLen, uint8_t gxWordCount, uint8_t zkpxgrWordCount, uint8_t zkpxbWordCount, uint8_t *Step2ZKPXGRHash);
#endif
#if WEAVE_IS_EC_PASE_ENABLED
    WEAVE_ERROR GenerateStep1Data_ConfigEC(PacketBuffer *buf, uint16_t &stepDataLen);
    WEAVE_ERROR ProcessStep1Data_ConfigEC(PacketBuffer *buf, uint16_t &stepDataLen, uint8_t gxWordCount, uint8_t zkpxgrWordCount, uint8_t zkpxbWordCount);
    WEAVE_ERROR GenerateStep2Data_ConfigEC(PacketBuffer *buf, uint16_t &stepDataLen, uint8_t *Step2ZKPXGRHash);
    WEAVE_ERROR ProcessStep2Data_ConfigEC(PacketBuffer *buf, uint16_t &stepDataLen, uint8_t gxWordCount, uint8_t zkpxgrWordCount, uint8_t zkpxbWordCount, uint8_t *Step2ZKPXGRHash);
#endif
    void ProtocolHash(const uint8_t *data, const uint16_t dataLen, uint8_t *h);
    WEAVE_ERROR DeriveKeys(const uint8_t *initiatorStep2ZKPXGRHash, const uint8_t step2ZKPXGRHashLength, uint8_t *keyConfirmKey, const uint8_t keyConfirmKeyLength);
    void GenerateKeyConfirmHashes(const uint8_t *keyConfirmKey, const uint8_t keyConfirmKeyLength, uint8_t *initiatorHash, uint8_t *responderHash, const uint8_t keyConfirmHashLength);
    bool IsAllowedPASEConfig(uint32_t config) const;
    uint32_t PackSizeHeader(uint8_t altConfigCount);
    WEAVE_ERROR GenerateAltConfigsList(uint32_t *altConfigs, uint8_t &altConfigsCount);
    WEAVE_ERROR FindStrongerAltConfig(uint32_t *altConfigs, uint8_t altConfigsCount);
};

} // namespace PASE
} // namespace Security
} // namespace Profiles
} // namespace Weave
} // namespace nl


#endif /* WEAVEPASE_H_ */
