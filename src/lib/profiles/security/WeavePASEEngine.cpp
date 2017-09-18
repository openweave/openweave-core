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
 *      This file implements objects for initiators and responders for
 *      the Weave Password Authenticated Session Establishment (PASE)
 *      protocol.
 *
 */

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif
#include <stdint.h>

#ifndef OPENSSL_EXPERIMENTAL_JPAKE
#define OPENSSL_EXPERIMENTAL_JPAKE
#endif

#include <Weave/Core/WeaveCore.h>
#include "WeavePASE.h"
#include "WeaveSecurity.h"
#include <Weave/Support/crypto/WeaveCrypto.h>
#include <Weave/Support/crypto/HashAlgos.h>
#include <Weave/Support/CodeUtils.h>

#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG1
#include <openssl/jpake.h>
#endif

namespace nl {
namespace Weave {
namespace Profiles {
namespace Security {
namespace PASE {

#undef PASE_PRINT_CRYPTO_DATA
#ifdef PASE_PRINT_CRYPTO_DATA
static void PrintHex(const uint8_t *data, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++)
        printf("%02X", data[i]);
}
#endif

using namespace nl::Weave::Encoding;
using namespace nl::Weave::Crypto;
using namespace nl::Weave::Platform::Security;
#if WEAVE_IS_EC_PASE_ENABLED
using namespace nl::Weave::ASN1;
#endif

// Domain parameters for J-PAKE and Schnorr signatures, as used by PASE Configuration 1.
//
// These are 1024-bit p and 160-bit q parameters taken from:
//
//     http://csrc.nist.gov/groups/ST/toolkit/documents/Examples/DSA2_All.pdf
//
// which is referenced by IETF draft-hao-schnorr-00.
//
#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG1

static BIGNUM *PASEConfig1_JPAKE_P()
{
    static const uint8_t P[] =
    {
        0xE0, 0xA6, 0x75, 0x98, 0xCD, 0x1B, 0x76, 0x3B, 0xC9, 0x8C, 0x8A, 0xBB, 0x33, 0x3E, 0x5D, 0xDA,
        0x0C, 0xD3, 0xAA, 0x0E, 0x5E, 0x1F, 0xB5, 0xBA, 0x8A, 0x7B, 0x4E, 0xAB, 0xC1, 0x0B, 0xA3, 0x38,
        0xFA, 0xE0, 0x6D, 0xD4, 0xB9, 0x0F, 0xDA, 0x70, 0xD7, 0xCF, 0x0C, 0xB0, 0xC6, 0x38, 0xBE, 0x33,
        0x41, 0xBE, 0xC0, 0xAF, 0x8A, 0x73, 0x30, 0xA3, 0x30, 0x7D, 0xED, 0x22, 0x99, 0xA0, 0xEE, 0x60,
        0x6D, 0xF0, 0x35, 0x17, 0x7A, 0x23, 0x9C, 0x34, 0xA9, 0x12, 0xC2, 0x02, 0xAA, 0x5F, 0x83, 0xB9,
        0xC4, 0xA7, 0xCF, 0x02, 0x35, 0xB5, 0x31, 0x6B, 0xFC, 0x6E, 0xFB, 0x9A, 0x24, 0x84, 0x11, 0x25,
        0x8B, 0x30, 0xB8, 0x39, 0xAF, 0x17, 0x24, 0x40, 0xF3, 0x25, 0x63, 0x05, 0x6C, 0xB6, 0x7A, 0x86,
        0x11, 0x58, 0xDD, 0xD9, 0x0E, 0x6A, 0x89, 0x4C, 0x72, 0xA5, 0xBB, 0xEF, 0x9E, 0x28, 0x6C, 0x6B,
    };

    return BN_bin2bn(P, sizeof(P), NULL);
}

static BIGNUM *PASEConfig1_JPAKE_Q()
{
    static const uint8_t Q[] =
    {
        0xE9, 0x50, 0x51, 0x1E, 0xAB, 0x42, 0x4B, 0x9A, 0x19, 0xA2, 0xAE, 0xB4, 0xE1, 0x59, 0xB7, 0x84,
        0x4C, 0x58, 0x9C, 0x4F
    };

    return BN_bin2bn(Q, sizeof(Q), NULL);
}

static BIGNUM *PASEConfig1_JPAKE_G()
{
    static const uint8_t G[] =
    {
        0xD2, 0x9D, 0x51, 0x21, 0xB0, 0x42, 0x3C, 0x27, 0x69, 0xAB, 0x21, 0x84, 0x3E, 0x5A, 0x32, 0x40,
        0xFF, 0x19, 0xCA, 0xCC, 0x79, 0x22, 0x64, 0xE3, 0xBB, 0x6B, 0xE4, 0xF7, 0x8E, 0xDD, 0x1B, 0x15,
        0xC4, 0xDF, 0xF7, 0xF1, 0xD9, 0x05, 0x43, 0x1F, 0x0A, 0xB1, 0x67, 0x90, 0xE1, 0xF7, 0x73, 0xB5,
        0xCE, 0x01, 0xC8, 0x04, 0xE5, 0x09, 0x06, 0x6A, 0x99, 0x19, 0xF5, 0x19, 0x5F, 0x4A, 0xBC, 0x58,
        0x18, 0x9F, 0xD9, 0xFF, 0x98, 0x73, 0x89, 0xCB, 0x5B, 0xED, 0xF2, 0x1B, 0x4D, 0xAB, 0x4F, 0x8B,
        0x76, 0xA0, 0x55, 0xFF, 0xE2, 0x77, 0x09, 0x88, 0xFE, 0x2E, 0xC2, 0xDE, 0x11, 0xAD, 0x92, 0x21,
        0x9F, 0x0B, 0x35, 0x18, 0x69, 0xAC, 0x24, 0xDA, 0x3D, 0x7B, 0xA8, 0x70, 0x11, 0xA7, 0x01, 0xCE,
        0x8E, 0xE7, 0xBF, 0xE4, 0x94, 0x86, 0xED, 0x45, 0x27, 0xB7, 0x18, 0x6C, 0xA4, 0x61, 0x0A, 0x75,
    };

    return BN_bin2bn(G, sizeof(G), NULL);
}

// Utility function for initializing a JPAKE_CTX for PASE Config1
static WEAVE_ERROR NewPASEConfig1JPAKECTX(const uint8_t *pw, uint16_t pwLen, const char *localContextStr, const char *peerContextStr, struct JPAKE_CTX *& ctx)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    BIGNUM *secret = NULL;
    BIGNUM *P = NULL;
    BIGNUM *G = NULL;
    BIGNUM *Q = NULL;

    secret = BN_bin2bn(pw, pwLen, NULL);
    VerifyOrExit(secret != NULL, err = WEAVE_ERROR_NO_MEMORY);

    P = PASEConfig1_JPAKE_P();
    VerifyOrExit(P != NULL, err = WEAVE_ERROR_NO_MEMORY);

    G = PASEConfig1_JPAKE_G();
    VerifyOrExit(G != NULL, err = WEAVE_ERROR_NO_MEMORY);

    Q = PASEConfig1_JPAKE_Q();
    VerifyOrExit(Q != NULL, err = WEAVE_ERROR_NO_MEMORY);

    ctx = JPAKE_CTX_new(localContextStr, peerContextStr, P, G, Q, secret);
    VerifyOrExit(ctx != NULL, err = WEAVE_ERROR_NO_MEMORY);

exit:
    BN_free(secret);
    BN_free(P);
    BN_free(G);
    BN_free(Q);
    return err;
}

#endif /* WEAVE_CONFIG_SUPPORT_PASE_CONFIG1 */

#if WEAVE_CONFIG_PASE_MESSAGE_PAYLOAD_ALIGNMENT
static WEAVE_ERROR AlignMessagePayload(PacketBuffer *buf)
{
    // Align message payload on 4-byte boundary
    if (buf->AlignPayload(4))
        return WEAVE_NO_ERROR;

    return WEAVE_ERROR_BUFFER_TOO_SMALL;
}
#else
static inline WEAVE_ERROR AlignMessagePayload(PacketBuffer *buf)
{
    return WEAVE_NO_ERROR;
}
#endif // WEAVE_CONFIG_PASE_MESSAGE_PAYLOAD_ALIGNMENT

static WEAVE_ERROR PackControlHeader(uint8_t pwSrc, uint8_t encType, uint16_t sessionKeyId, bool performKeyConfirm, uint32_t& controlHeader)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(pwSrc < 16, err = WEAVE_ERROR_INVALID_ARGUMENT);
    controlHeader = (((uint32_t)pwSrc) << kPASEHeader_PasswordSourceShift) & kPASEHeader_PasswordSourceMask;

    VerifyOrExit(encType < 16, err = WEAVE_ERROR_INVALID_ARGUMENT);
    controlHeader |= (((uint32_t)encType) << kPASEHeader_EncryptionTypeShift) & kPASEHeader_EncryptionTypeMask;

    controlHeader |= (((uint32_t)sessionKeyId) << kPASEHeader_SessionKeyShift) & kPASEHeader_SessionKeyMask;

    if (performKeyConfirm)
        controlHeader |= kPASEHeader_PerformKeyConfirmFlag;

exit:
    return err;
}

static WEAVE_ERROR UnpackControlHeader(uint32_t controlHeader, uint8_t& pwSrc, uint8_t& encType, uint16_t& sessionKeyId, bool& performKeyConfirm)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit((controlHeader & kPASEHeader_ControlHeaderUnusedBits) == 0, err = WEAVE_ERROR_INVALID_ARGUMENT);

    pwSrc = (uint8_t)((controlHeader & kPASEHeader_PasswordSourceMask) >> kPASEHeader_PasswordSourceShift);
    encType = (uint8_t)((controlHeader & kPASEHeader_EncryptionTypeMask) >> kPASEHeader_EncryptionTypeShift);
    sessionKeyId = (uint16_t)((controlHeader & kPASEHeader_SessionKeyMask) >> kPASEHeader_SessionKeyShift);
    performKeyConfirm = (controlHeader & kPASEHeader_PerformKeyConfirmFlag) != 0;

exit:
    return err;
}

uint32_t WeavePASEEngine::PackSizeHeader(uint8_t altConfigCount)
{
    uint32_t sizeHeader;

#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG0_TEST_ONLY
    if (ProtocolConfig == kPASEConfig_Config0_TEST_ONLY)
    {
        sizeHeader = kPASESizeHeader_MaxConstantSizesConfig0;
    }
    else
#endif
#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG1
    if (ProtocolConfig == kPASEConfig_Config1)
    {
        sizeHeader = kPASESizeHeader_MaxConstantSizesConfig1;
    }
    else
#endif
#if WEAVE_IS_EC_PASE_ENABLED
    {
        uint8_t gxWordCount;
        uint8_t zkpxbWordCount;

        zkpxbWordCount = mEllipticCurveJPAKE.GetCurveSize() / 4;
        gxWordCount = 2 * zkpxbWordCount;

#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG2
        if (ProtocolConfig == kPASEConfig_Config2)
            zkpxbWordCount++;
#endif

        sizeHeader  = (gxWordCount << kPASEHeader_GXWordCountShift) & kPASEHeader_GXWordCountMask;
        sizeHeader |= (gxWordCount << kPASEHeader_ZKPXGRWordCountShift) & kPASEHeader_ZKPXGRWordCountMask;
        sizeHeader |= (zkpxbWordCount << kPASEHeader_ZKPXBWordCountShift) & kPASEHeader_ZKPXBWordCountMask;
    }
#else // WEAVE_IS_EC_PASE_ENABLED
    {
        // Die if ProtocolConfig has an invalid value.
        WeaveDie();
    }
#endif // WEAVE_IS_EC_PASE_ENABLED

    sizeHeader |= (altConfigCount << kPASEHeader_AlternateConfigCountShift) & kPASEHeader_AlternateConfigCountMask;

    return sizeHeader;
}

static void UnpackSizeHeader(uint32_t sizeHeader, uint8_t& gx, uint8_t& zkpxgr, uint8_t& zkpxb, uint8_t& altConfigCount)
{
    gx = (sizeHeader & kPASEHeader_GXWordCountMask) >> kPASEHeader_GXWordCountShift;
    zkpxgr = (sizeHeader & kPASEHeader_ZKPXGRWordCountMask) >> kPASEHeader_ZKPXGRWordCountShift;
    zkpxb = (sizeHeader & kPASEHeader_ZKPXBWordCountMask) >> kPASEHeader_ZKPXBWordCountShift;
    altConfigCount = (sizeHeader & kPASEHeader_AlternateConfigCountMask) >> kPASEHeader_AlternateConfigCountShift;
}

static WEAVE_ERROR UnpackSizeHeader(uint32_t sizeHeader, uint8_t& gx, uint8_t& zkpxgr, uint8_t& zkpxb)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint8_t unused;

    UnpackSizeHeader(sizeHeader, gx, zkpxgr, zkpxb, unused);
    VerifyOrExit(unused == 0, err = WEAVE_ERROR_INVALID_ARGUMENT);

exit:
    return err;
}

enum
{
    kMaxContextStringSize         = 64 + kMaxAlternateProtocolConfigs * 9,
    kMaxContextDataSize           = 27 + kMaxAlternateProtocolConfigs * 4
};

// The FormProtocolContextString() function is used to create a string that encodes the context
// a particular PASE interaction from the perspective of one of the participating parties (either
// the initiator or the responder).  The string is incorporated, by means of hashing, into the
// zero-knowledge proofs that are passed in the J-PAKE protocol.  This has the effect of binding
// the success of the protocol to the identities of the parties and the agreed upon protocol
// parameters, preventing man-in-the-middle attacks and certain forms of replay attack.
//
// The generated context string incorporates the following values:
//
//     <Role> -- The role of the target party (I for initiator, R for responder)
//     <LocalNodeId> -- The Weave node id of the party to which the context string applies.
//     <PeerNodeId> -- The Weave node id of the other party.
//     <SessionKeyId> -- The session key id requested by the initiator.
//     <EncryptionType> -- The encryption type requested by the initiator.
//     <PasswordSource> -- The source of the password to be used for authentication (as requested by the initiator).
//     <ConfirmationFlag> -- A boolean (T or F) indicating the initiator has requested key confirmation.
//     <ProtocolConfig> -- The PASE protocol configuration value requested by the initiator.
//     <AltConfigList> -- A list of alternate PASE protocol configuration value supported by the initiator
//
// Note that the inclusion of the AltConfigList value serves to prevent downgrade attacks by ensuring
// that the responder has seen the full list of configurations supported by the initiator.
//
#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG1
WEAVE_ERROR WeavePASEEngine::FormProtocolContextString(uint64_t localNodeId, uint64_t peerNodeId, uint8_t pwSource, uint32_t *altConfigs, uint8_t altConfigsCount, bool isInitiator, char *buf, size_t bufSize)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    char roleChar, confirmKeyChar;
    int res;

    VerifyOrExit(EncryptionType <= (kPASEHeader_EncryptionTypeMask >> kPASEHeader_EncryptionTypeShift), err = WEAVE_ERROR_INVALID_ARGUMENT);
    VerifyOrExit(pwSource <= (kPASEHeader_PasswordSourceMask >> kPASEHeader_PasswordSourceShift), err = WEAVE_ERROR_INVALID_ARGUMENT);

    roleChar = (isInitiator) ? 'I' : 'R';
    confirmKeyChar = (PerformKeyConfirmation) ? 'T' : 'F';

    // !!! IMPORTANT !!!  The format of the context strings CANNOT change without introducing a protocol
    // incompatibility. In practice this means that any change to the string format MUST introduce a new
    // PASE configuration type.
    //
    res = snprintf(buf, bufSize, "%c,%016llX,%016llX,%04X,%X,%X,%c,%08lX,%02X",
                   roleChar, (unsigned long long)localNodeId, (unsigned long long)peerNodeId,
                   SessionKeyId, EncryptionType, pwSource,
                   confirmKeyChar, (unsigned long)ProtocolConfig, altConfigsCount);
    VerifyOrExit(res < (int)bufSize, err = WEAVE_ERROR_BUFFER_TOO_SMALL);

    buf += res;
    bufSize -= res;

    for (uint8_t i = 0; i < altConfigsCount; i++)
    {
        res = snprintf(buf, bufSize, ",%08lX", (unsigned long)altConfigs[i]);
        VerifyOrExit(res < (int)bufSize, err = WEAVE_ERROR_BUFFER_TOO_SMALL);
        buf += res;
        bufSize -= res;
    }

exit:
    return err;
}
#endif // WEAVE_CONFIG_SUPPORT_PASE_CONFIG1

#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG0_TEST_ONLY || WEAVE_IS_EC_PASE_ENABLED
WEAVE_ERROR WeavePASEEngine::FormProtocolContextData(uint64_t localNodeId, uint64_t peerNodeId, uint8_t pwSource, uint32_t *altConfigs, uint8_t altConfigsCount, bool isInitiator, uint8_t *buf, size_t bufSize, uint16_t &contextLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint8_t roleByte, confirmKeyByte;

    VerifyOrExit(EncryptionType <= (kPASEHeader_EncryptionTypeMask >> kPASEHeader_EncryptionTypeShift), err = WEAVE_ERROR_INVALID_ARGUMENT);
    VerifyOrExit(pwSource <= (kPASEHeader_PasswordSourceMask >> kPASEHeader_PasswordSourceShift), err = WEAVE_ERROR_INVALID_ARGUMENT);

    // !!! IMPORTANT !!!  The format of the context data CANNOT change without introducing a protocol
    // incompatibility. In practice this means that any change to the format MUST introduce a new
    // PASE configuration type.
    // Protocol Context Data incorporates the following values (in same order):
    //    <Role>                    - 1 byte
    //    <LocalNodeId>             - 8 bytes
    //    <PeerNodeId>              - 8 bytes
    //    <SessionKeyId>            - 2 bytes
    //    <EncryptionType>          - 1 byte
    //    <PasswordSource>          - 1 byte
    //    <ConfirmKeyByte>          - 1 byte
    //    <ProtocolConfig>          - 4 bytes
    //    <AlternateConfigCount>    - 1 byte
    //    <AlternateConfigs>        - 4 bytes each
    //    Total Number of bytes:      27 + 4 * altConfigsCount
    //
    contextLen = 27 + 4 * altConfigsCount;
    VerifyOrExit(contextLen <= bufSize, err = WEAVE_ERROR_BUFFER_TOO_SMALL);

    roleByte = (uint8_t)((isInitiator) ? 'I' : 'R');
    confirmKeyByte = (uint8_t)((PerformKeyConfirmation) ? 'T' : 'F');

    memset(buf++, roleByte, 1);
    LittleEndian::Write64(buf, localNodeId);
    LittleEndian::Write64(buf, peerNodeId);
    LittleEndian::Write16(buf, SessionKeyId);
    memset(buf++, EncryptionType, 1);
    memset(buf++, pwSource, 1);
    memset(buf++, confirmKeyByte, 1);
    LittleEndian::Write32(buf, ProtocolConfig);
    memset(buf++, altConfigsCount, 1);

    for (uint8_t i = 0; i < altConfigsCount; i++)
        LittleEndian::Write32(buf, altConfigs[i]);

exit:
    return err;
}
#endif // WEAVE_CONFIG_SUPPORT_PASE_CONFIG0_TEST_ONLY || WEAVE_IS_EC_PASE_ENABLED


void WeavePASEEngine::ProtocolHash(const uint8_t *data, const uint16_t dataLen, uint8_t *h)
{
#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG1
    if (ProtocolConfig == kPASEConfig_Config1)
    {
        SHA1   hash;
        hash.Begin();
        hash.AddData(data, dataLen);
        hash.Finish(h);
    }
    else
#endif
#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG0_TEST_ONLY || WEAVE_IS_EC_PASE_ENABLED
    {
        SHA256 hash;
        hash.Begin();
        hash.AddData(data, dataLen);
        hash.Finish(h);
    }
#else
    {
    }
#endif
}

// Returns true when input config is allowed PASE Config
bool WeavePASEEngine::IsAllowedPASEConfig(uint32_t config) const
{
    return (((0x01 << (config & kPASEConfig_ConfigNestNumberMask)) & AllowedPASEConfigs) != 0x00);
}

// Returns PASE config Security Strength
// Returns 0 when given config is not supported
static uint8_t GetPASEConfigSecurityStrength(uint32_t config)
{
#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG0_TEST_ONLY
    if (config == kPASEConfig_Config0_TEST_ONLY)
        return kPASEConfig_Config0SecurityStrength;
    else
#endif
#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG1
    if (config == kPASEConfig_Config1)
        return kPASEConfig_Config1SecurityStrength;
    else
#endif
#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG2
    if (config == kPASEConfig_Config2)
        return kPASEConfig_Config2SecurityStrength;
    else
#endif
#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG3
    if (config == kPASEConfig_Config3)
        return kPASEConfig_Config3SecurityStrength;
    else
#endif
#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG4
    if (config == kPASEConfig_Config4)
        return kPASEConfig_Config4SecurityStrength;
    else
#endif
#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG5
    if (config == kPASEConfig_Config5)
        return kPASEConfig_Config5SecurityStrength;
#endif
        return 0;
}

WEAVE_ERROR WeavePASEEngine::GenerateAltConfigsList(uint32_t *altConfigs, uint8_t &altConfigsCount)
{
    // Generate alternate config list in the following priority order
    //   1 - Config5
    //   2 - Config4
    //   3 - Config3
    //   4 - Config2
    //   5 - Config1
    //   6 - Config0

    uint32_t config = kPASEConfig_ConfigLast;
    bool protocolConfigIsAllowed = IsAllowedPASEConfig(ProtocolConfig);
    altConfigsCount = 0;

    // check configs in the priority order specified above
    while (config >= kPASEConfig_Config0_TEST_ONLY && altConfigsCount < kMaxAlternateProtocolConfigs)
    {
        if (config != ProtocolConfig)
        {
            if (IsAllowedPASEConfig(config))
            {
                // check if ProtocolConfig is allowed config
                if (protocolConfigIsAllowed)
                {
                    altConfigs[altConfigsCount] = config;
                    altConfigsCount++;
                 }
                // update ProtocolConfig if it wasn't valid
                else
                {
                    ProtocolConfig = config;
                    protocolConfigIsAllowed = true;
                }
            }
        }
        config--;
    }

    // generated error if proposed config wasn't allowed and no alternative config was found
    if (!protocolConfigIsAllowed)
        return WEAVE_ERROR_INVALID_PASE_CONFIGURATION;

    return WEAVE_NO_ERROR;
}

WEAVE_ERROR WeavePASEEngine::FindStrongerAltConfig(uint32_t *altConfigs, uint8_t altConfigsCount)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint8_t securityStrength;
    uint8_t highSecurityStrength;

    // Verify the requested protocol config. Here's how that needs to work:
    //
    // * Whenever an initiator sends a Step1 message to a responder, it always includes the proposed protocol
    //   config AND a list of alternate configs it supports
    //
    // * Whenever a responder receives an initiator's Step1 message, it always performs the following actions:
    //
    //     * The responder determines the set of protocol configs supported in common between it and the initiator
    //
    //     * The responder rank orders the set of common configs by their security strength, highest to lowest, and
    //       selects the subset that provides the highest equivalent strength--i.e. where all members of the subset provide
    //       the same level of security, and the members of the subset provide greater security than any other members
    //       of the larger common set.
    //
    //     * The responder determines whether the initiator's proposed config is in the set of high security common
    //       configs.
    //
    //     * If the proposed config IS in the set of high security common configs, the responder proceeds with the next
    //       step of the PASE protocol, using the proposed protocol config.
    //
    //     * If the initiator's proposed config IS NOT in the set of high security common configs, the responder responds
    //       by sending a PASEReconfigure message to the initiator containing the set of high security common configs (in
    //       PASEReconfigure.OfferedConfigs). Note that the responder is free to reduce this set if it has further
    //       preferences beyond security.
    //
    // * Whenever an initiator receives a PASEReconfigure message it must select a config from the set of OfferedConfigs
    //   given in the PASEReconfigure message, and then send a new PASEInitiatorStep1 message to the responder proposing
    //   the newly selected config.

    highSecurityStrength = GetPASEConfigSecurityStrength(ProtocolConfig);

    // Find stronger config
    for (uint8_t i = 0; i < altConfigsCount; i++)
    {
        securityStrength = GetPASEConfigSecurityStrength(altConfigs[i]);
        if (IsAllowedPASEConfig(altConfigs[i]) && securityStrength > highSecurityStrength)
        {
            ProtocolConfig = altConfigs[i];
            highSecurityStrength = securityStrength;
            err = WEAVE_ERROR_PASE_RECONFIGURE_REQUIRED;
        }
    }

    // Check that ProtocolConfig is allowed
    if (highSecurityStrength == 0)
        err =  WEAVE_ERROR_NO_COMMON_PASE_CONFIGURATIONS;

    return err;
}

WEAVE_ERROR WeavePASEEngine::InitState(uint64_t localNodeId, uint64_t peerNodeId, uint8_t pwSource, WeaveFabricState *FabricState, uint32_t *altConfigs, uint8_t altConfigsCount, bool isInitiator)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // If the app wants to use the pairing code, and didn't supply it directly, then
    // fetch it from the fabric state.
    if (Pw == NULL)
    {
        const char *pwChar;
        err = FabricState->GetPassword(pwSource, pwChar, PwLen);
        SuccessOrExit(err);
        Pw = (const uint8_t *)pwChar;
    }

    // Make sure we have a password to authenticate with.
    VerifyOrExit(Pw != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Save the password source.
    PwSource = pwSource;

#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG1
    if ( ProtocolConfig == kPASEConfig_Config1 )
    {
        char localContextStr[kMaxContextStringSize];
        char peerContextStr[kMaxContextStringSize];

        // Create the local and peer protocol context strings.
        err = FormProtocolContextString(localNodeId, peerNodeId, pwSource, altConfigs, altConfigsCount,
                                        isInitiator, localContextStr, sizeof(localContextStr));
        SuccessOrExit(err);
        err = FormProtocolContextString(peerNodeId, localNodeId, pwSource, altConfigs, altConfigsCount,
                                        !isInitiator, peerContextStr, sizeof(peerContextStr));
        SuccessOrExit(err);

        // Initialize a J-PAKE context with the domain parameters for Config1
        err = NewPASEConfig1JPAKECTX(Pw, PwLen, localContextStr, peerContextStr, JPAKECtx);
        SuccessOrExit(err);
    }
    else
#endif // WEAVE_CONFIG_SUPPORT_PASE_CONFIG1
#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG0_TEST_ONLY || WEAVE_IS_EC_PASE_ENABLED
    {
        uint8_t localContextData[kMaxContextDataSize];
        uint8_t peerContextData[kMaxContextDataSize];
        uint16_t localContextLen;
        uint16_t peerContextLen;

        // Create the local and peer protocol context data
        err = FormProtocolContextData(localNodeId, peerNodeId, pwSource, altConfigs, altConfigsCount,
                                      isInitiator, localContextData, sizeof(localContextData), localContextLen);
        SuccessOrExit(err);
        err = FormProtocolContextData(peerNodeId, localNodeId, pwSource, altConfigs, altConfigsCount,
                                      !isInitiator, peerContextData, sizeof(peerContextData), peerContextLen);
        SuccessOrExit(err);

#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG0_TEST_ONLY
        if (ProtocolConfig == kPASEConfig_Config0_TEST_ONLY)
        {
            SHA256 hash;
            hash.Begin();
            if (isInitiator)
            {
                hash.AddData(localContextData, localContextLen);
                hash.AddData(peerContextData, peerContextLen);
            }
            else
            {
                hash.AddData(peerContextData, peerContextLen);
                hash.AddData(localContextData, localContextLen);
            }
            hash.AddData(Pw, PwLen);
            hash.Finish(KeyMeterial_Config0);
        }
        else
#endif /* WEAVE_CONFIG_SUPPORT_PASE_CONFIG0_TEST_ONLY */
#if WEAVE_IS_EC_PASE_ENABLED
        {
            // Initialize a J-PAKE context with the domain parameters for Config2/Config3
            OID curveOID = kOID_NotSpecified;
#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG2
            if (ProtocolConfig == kPASEConfig_Config2)
                curveOID = kOID_EllipticCurve_secp160r1;
            else
#endif
#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG3
            if (ProtocolConfig == kPASEConfig_Config3)
                curveOID = kOID_EllipticCurve_prime192v1;
            else
#endif
#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG4
            if (ProtocolConfig == kPASEConfig_Config4)
                curveOID = kOID_EllipticCurve_secp224r1;
            else
#endif
#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG5
            if (ProtocolConfig == kPASEConfig_Config5)
                curveOID = kOID_EllipticCurve_prime256v1;
            else
#endif
            {
                ExitNow(err = WEAVE_ERROR_UNSUPPORTED_ELLIPTIC_CURVE);
            }

            err = mEllipticCurveJPAKE.Init(curveOID, Pw, PwLen, localContextData, localContextLen, peerContextData, peerContextLen);
            SuccessOrExit(err);
        }
#else /* WEAVE_IS_EC_PASE_ENABLED */
        {
            err = WEAVE_ERROR_INVALID_PASE_PARAMETER;
        }
#endif /* WEAVE_IS_EC_PASE_ENABLED */
    }
#endif /* WEAVE_CONFIG_SUPPORT_PASE_CONFIG0_TEST_ONLY || WEAVE_IS_EC_PASE_ENABLED */
    { }

exit:
    return err;
}

void WeavePASEEngine::Init()
{
    State = kState_Reset;
    Pw = NULL;
    PwSource = kPasswordSource_NotSpecified;
    AllowedPASEConfigs = kPASEConfig_SupportedConfigs;
#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG1
    JPAKECtx = NULL;
#endif
#if WEAVE_IS_EC_PASE_ENABLED
    mEllipticCurveJPAKE.Init();
#endif
}

void WeavePASEEngine::Shutdown()
{
    Reset();
}

void WeavePASEEngine::Reset()
{
#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG1
    if (JPAKECtx != NULL)
    {
        JPAKE_CTX_free(JPAKECtx);
        JPAKECtx = NULL;
    }
#endif
#if WEAVE_IS_EC_PASE_ENABLED
    mEllipticCurveJPAKE.Reset();
#endif
    State = kState_Reset;
    ProtocolConfig = kPASEConfig_Unspecified;
    Pw = NULL;
    PwLen = 0;
    PwSource = kPasswordSource_NotSpecified;
    SessionKeyId = WeaveKeyId::kNone;
    EncryptionType = 0;
    AllowedPASEConfigs = kPASEConfig_SupportedConfigs;
    PerformKeyConfirmation = 0;
    ClearSecretData((uint8_t *)&EncryptionKey, sizeof(EncryptionKey));
    ClearSecretData((uint8_t *)&ResponderStep2ZKPXGRHash, sizeof(ResponderStep2ZKPXGRHash));
}

bool WeavePASEEngine::IsInitiator() const
{
    return (State >= kState_InitiatorStatesBase && State <= kState_InitiatorStatesEnd);
}

bool WeavePASEEngine::IsResponder() const
{
    return (State >= kState_ResponderStatesBase && State <= kState_ResponderStatesEnd);
}

WEAVE_ERROR WeavePASEEngine::GenerateInitiatorStep1(PacketBuffer *buf, uint32_t proposedPASEConfig, uint64_t localNodeId, uint64_t peerNodeId, uint16_t sessionKeyId, uint8_t encType, uint8_t pwSrc, WeaveFabricState *FabricState, bool confirmKey)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    union
    {
        uint32_t controlHeader;
        uint32_t sizeHeader;
    };
    uint32_t altConfigs[kMaxAlternateProtocolConfigs];
    uint8_t altConfigsCount;
    uint8_t *p;
    uint16_t stepDataLen;

    // Verify correct state. Three options are possible:
    //     kState_Reset                       - Initial Step1 Message Generation
    //     kState_ResponderReconfigProcessed  - Responder generated reconfigure request
    //     kState_InitiatorStep1Generated     - Responder supports only Config1
    VerifyOrExit(State == kState_Reset ||
                 State == kState_ResponderReconfigProcessed ||
                 State == kState_InitiatorStep1Generated, err = WEAVE_ERROR_INCORRECT_STATE);

    // Align payload on 4-byte boundary if needed
    err = AlignMessagePayload(buf);
    SuccessOrExit(err);

    // Initialize pointer to the buffer payload start
    p = buf->Start();

    // Clear/Release J-PAKE CTX Data created by previous GenerateInitiatorStep1
    if (State != kState_Reset)
    {
        // Verify that the new proposed Config is not the one that was already used
        VerifyOrExit(ProtocolConfig != proposedPASEConfig, err = WEAVE_ERROR_INVALID_ARGUMENT);

#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG1
        if (JPAKECtx != NULL)
        {
            JPAKE_CTX_free(JPAKECtx);
            JPAKECtx = NULL;
        }
#endif
#if WEAVE_IS_EC_PASE_ENABLED
        mEllipticCurveJPAKE.Reset();
#endif
    }

    // Init Parameters
    ProtocolConfig = proposedPASEConfig;
    SessionKeyId = sessionKeyId;
    EncryptionType = encType;
    PerformKeyConfirmation = confirmKey;

    // Generate list of alternate configs
    err = GenerateAltConfigsList(altConfigs, altConfigsCount);
    SuccessOrExit(err);

    // Init Protocol Data
    err = InitState(localNodeId, peerNodeId, pwSrc, FabricState, altConfigs, altConfigsCount, true);
    SuccessOrExit(err);

    stepDataLen = 4 * (3 + altConfigsCount);
    VerifyOrExit(stepDataLen <= buf->AvailableDataLength(), err = WEAVE_ERROR_BUFFER_TOO_SMALL);

    // Write the control header field.
    err = PackControlHeader(pwSrc, EncryptionType, SessionKeyId, PerformKeyConfirmation, controlHeader);
    SuccessOrExit(err);
    LittleEndian::Write32(p, controlHeader);

    // Write the size header field.
    sizeHeader = PackSizeHeader(altConfigsCount);
    LittleEndian::Write32(p, sizeHeader);

    // Write the protocol configuration field.
    LittleEndian::Write32(p, ProtocolConfig);

    // Write the alternate configurations field.
    for (uint8_t i = 0; i < altConfigsCount; i++)
        LittleEndian::Write32(p, altConfigs[i]);

#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG0_TEST_ONLY
    if (ProtocolConfig == kPASEConfig_Config0_TEST_ONLY)
    {
        err = GenerateStep1Data_Config0_TEST_ONLY(buf, stepDataLen);
    }
    else
#endif
#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG1
    if (ProtocolConfig == kPASEConfig_Config1)
    {
        err = GenerateStep1Data_Config1(buf, stepDataLen);
    }
    else
#endif
#if WEAVE_IS_EC_PASE_ENABLED
    {
        err = GenerateStep1Data_ConfigEC(buf, stepDataLen);
    }
#else
    {
        err = WEAVE_ERROR_INVALID_PASE_PARAMETER;
    }
#endif
    SuccessOrExit(err);

    // Set message length
    buf->SetDataLength(stepDataLen);

    // Set new PASE state
    State = kState_InitiatorStep1Generated;

exit:
    return err;
}

WEAVE_ERROR WeavePASEEngine::GenerateResponderStep1(PacketBuffer *buf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint32_t sizeHeader;
    uint16_t stepDataLen;
    uint8_t *p;

    // Verify correct state
    VerifyOrExit(State == kState_InitiatorStep1Processed, err = WEAVE_ERROR_INCORRECT_STATE);

    // Align payload on 4-byte boundary if needed
    err = AlignMessagePayload(buf);
    SuccessOrExit(err);

    // Initialize pointer to the buffer payload start
    p = buf->Start();

    stepDataLen = 4;
    VerifyOrExit(stepDataLen < buf->AvailableDataLength(), err = WEAVE_ERROR_BUFFER_TOO_SMALL);

    // Write the size header field.
    sizeHeader = PackSizeHeader(0);
    LittleEndian::Write32(p, sizeHeader);

#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG0_TEST_ONLY
    if (ProtocolConfig == kPASEConfig_Config0_TEST_ONLY)
    {
        err = GenerateStep1Data_Config0_TEST_ONLY(buf, stepDataLen);
    }
    else
#endif
#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG1
    if (ProtocolConfig == kPASEConfig_Config1)
    {
        err = GenerateStep1Data_Config1(buf, stepDataLen);
    }
    else
#endif
#if WEAVE_IS_EC_PASE_ENABLED
    {
        err = GenerateStep1Data_ConfigEC(buf, stepDataLen);
    }
#else
    {
        err = WEAVE_ERROR_INVALID_PASE_PARAMETER;
    }
#endif
    SuccessOrExit(err);

    // Set message length
    buf->SetDataLength(stepDataLen);

    // Set new PASE state
    State = kState_ResponderStep1Generated;

exit:
    return err;
}

WEAVE_ERROR WeavePASEEngine::ProcessInitiatorStep1(PacketBuffer *buf, uint64_t localNodeId, uint64_t peerNodeId, WeaveFabricState *FabricState)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    union
    {
        uint32_t controlHeader;
        uint32_t sizeHeader;
        uint32_t altConfigs[kMaxAlternateProtocolConfigs];
    };
    uint8_t altConfigsCount;
    uint8_t gxWordCount, zkpxgrWordCount, zkpxbWordCount;
    uint8_t pwSrc;
    uint16_t stepDataLen;
    const uint8_t *p;
    const uint16_t bufSize = buf->DataLength();

    // Verify correct state
    VerifyOrExit(State == kState_Reset, err = WEAVE_ERROR_INCORRECT_STATE);

    // Align payload on 4-byte boundary if needed
    err = AlignMessagePayload(buf);
    SuccessOrExit(err);

    // Initialize pointer to the buffer payload start
    p = buf->Start();

    stepDataLen = 12;
    VerifyOrExit(stepDataLen <= bufSize, err = WEAVE_ERROR_MESSAGE_INCOMPLETE);

    // Read and decode the control header field.
    controlHeader = LittleEndian::Read32(p);
    err = UnpackControlHeader(controlHeader, pwSrc, EncryptionType, SessionKeyId, PerformKeyConfirmation);
    SuccessOrExit(err);

    // Verify the requested key type.
    VerifyOrExit(WeaveKeyId::IsSessionKey(SessionKeyId), err = WEAVE_ERROR_UNSUPPORTED_ENCRYPTION_TYPE);

    // Verify the requested encryption type.
    VerifyOrExit(EncryptionType == kWeaveEncryptionType_AES128CTRSHA1, err = WEAVE_ERROR_UNSUPPORTED_ENCRYPTION_TYPE);

    // Read and Decode the size header field.
    sizeHeader = LittleEndian::Read32(p);
    UnpackSizeHeader(sizeHeader, gxWordCount, zkpxgrWordCount, zkpxbWordCount, altConfigsCount);
    VerifyOrExit(altConfigsCount <= kMaxAlternateProtocolConfigs, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Read the proposed protocol configuration field.
    ProtocolConfig = LittleEndian::Read32(p);

    // Decode the size header - decoding sizeHeader after ProtocolConfig is set

    // Read the list of alternate protocol configurations.
    stepDataLen += altConfigsCount * 4;
    VerifyOrExit(stepDataLen <= bufSize, err = WEAVE_ERROR_MESSAGE_INCOMPLETE);
    for (uint8_t i = 0; i < altConfigsCount; i++)
        altConfigs[i] = LittleEndian::Read32(p);

    // Check if stronger config is in the alternate Configs list. Function returns:
    //   - reconfigure request if stronger config is found
    //   - error if the proposed config is not allowed and no altertative was found
    //   - no error if proposed config is acceptable
    err = FindStrongerAltConfig(altConfigs, altConfigsCount);
    SuccessOrExit(err);

    // Init Protocol Data
    err = InitState(localNodeId, peerNodeId, pwSrc, FabricState, altConfigs, altConfigsCount, false);
    SuccessOrExit(err);

#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG0_TEST_ONLY
    if (ProtocolConfig == kPASEConfig_Config0_TEST_ONLY)
    {
        err = ProcessStep1Data_Config0_TEST_ONLY(buf, stepDataLen, gxWordCount, zkpxgrWordCount, zkpxbWordCount);
    }
    else
#endif
#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG1
    if (ProtocolConfig == kPASEConfig_Config1)
    {
        err = ProcessStep1Data_Config1(buf, stepDataLen, gxWordCount, zkpxgrWordCount, zkpxbWordCount);
    }
    else
#endif
#if WEAVE_IS_EC_PASE_ENABLED
    {
        err = ProcessStep1Data_ConfigEC(buf, stepDataLen, gxWordCount, zkpxgrWordCount, zkpxbWordCount);
    }
#else
    {
        err = WEAVE_ERROR_INVALID_PASE_PARAMETER;
    }
#endif
    SuccessOrExit(err);

    // Verify correct message length
    VerifyOrExit(stepDataLen == bufSize, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Set new PASE state
    State = kState_InitiatorStep1Processed;

exit:
    return err;
}

WEAVE_ERROR WeavePASEEngine::ProcessResponderStep1(PacketBuffer *buf)
{
    WEAVE_ERROR err;
    uint32_t sizeHeader;
    uint8_t gxWordCount, zkpxgrWordCount, zkpxbWordCount;
    uint16_t stepDataLen;
    const uint16_t bufSize = buf->DataLength();
    const uint8_t *p;

    // Verify correct state
    VerifyOrExit(State == kState_InitiatorStep1Generated, err = WEAVE_ERROR_INCORRECT_STATE);

    // Align payload on 4-byte boundary if needed
    err = AlignMessagePayload(buf);
    SuccessOrExit(err);

    // Initialize pointer to the buffer payload start
    p = buf->Start();

    // Read and decode the size header field.
    stepDataLen = 4;
    VerifyOrExit(stepDataLen <= bufSize, err = WEAVE_ERROR_MESSAGE_INCOMPLETE);
    sizeHeader = LittleEndian::Read32(p);
    err = UnpackSizeHeader(sizeHeader, gxWordCount, zkpxgrWordCount, zkpxbWordCount);
    SuccessOrExit(err);

#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG0_TEST_ONLY
    if (ProtocolConfig == kPASEConfig_Config0_TEST_ONLY)
    {
        err = ProcessStep1Data_Config0_TEST_ONLY(buf, stepDataLen, gxWordCount, zkpxgrWordCount, zkpxbWordCount);
    }
    else
#endif
#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG1
    if (ProtocolConfig == kPASEConfig_Config1)
    {
        err = ProcessStep1Data_Config1(buf, stepDataLen, gxWordCount, zkpxgrWordCount, zkpxbWordCount);
    }
    else
#endif
#if WEAVE_IS_EC_PASE_ENABLED
    {
        err = ProcessStep1Data_ConfigEC(buf, stepDataLen, gxWordCount, zkpxgrWordCount, zkpxbWordCount);
    }
#else
    {
        err = WEAVE_ERROR_INVALID_PASE_PARAMETER;
    }
#endif
    SuccessOrExit(err);

    // Verify correct message length
    VerifyOrExit(stepDataLen == bufSize, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Set new PASE state
    State = kState_ResponderStep1Processed;

exit:
    return err;
}

WEAVE_ERROR WeavePASEEngine::GenerateResponderStep2(PacketBuffer *buf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint32_t sizeHeader;
    uint16_t stepDataLen;
    uint8_t *p;

    // Verify correct state
    VerifyOrExit(State == kState_ResponderStep1Generated, err = WEAVE_ERROR_INCORRECT_STATE);

    // Align payload on 4-byte boundary if needed
    err = AlignMessagePayload(buf);
    SuccessOrExit(err);

    // Initialize pointer to the buffer payload start
    p = buf->Start();

    stepDataLen = 4;
    VerifyOrExit(stepDataLen < buf->AvailableDataLength(), err = WEAVE_ERROR_BUFFER_TOO_SMALL);

    // Write the size header.
    sizeHeader = PackSizeHeader(0);
    LittleEndian::Write32(p, sizeHeader);

#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG0_TEST_ONLY
    if (ProtocolConfig == kPASEConfig_Config0_TEST_ONLY)
    {
        err = GenerateStep2Data_Config0_TEST_ONLY(buf, stepDataLen, ResponderStep2ZKPXGRHash);
    }
    else
#endif
#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG1
    if (ProtocolConfig == kPASEConfig_Config1)
    {
        err = GenerateStep2Data_Config1(buf, stepDataLen, ResponderStep2ZKPXGRHash);
    }
    else
#endif
#if WEAVE_IS_EC_PASE_ENABLED
    {
        err = GenerateStep2Data_ConfigEC(buf, stepDataLen, ResponderStep2ZKPXGRHash);
    }
#else
    {
        err = WEAVE_ERROR_INVALID_PASE_PARAMETER;
    }
#endif
    SuccessOrExit(err);

    // Set message length
    buf->SetDataLength(stepDataLen);

    // Set new PASE state
    State = kState_ResponderStep2Generated;

exit:
    return err;
}

WEAVE_ERROR WeavePASEEngine::ProcessResponderStep2(PacketBuffer *buf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint32_t sizeHeader;
    uint8_t gxWordCount, zkpxgrWordCount, zkpxbWordCount;
    uint16_t stepDataLen;
    const uint16_t bufSize = buf->DataLength();
    const uint8_t *p;

    // Verify correct state
    VerifyOrExit(State == kState_ResponderStep1Processed, err = WEAVE_ERROR_INCORRECT_STATE);

    // Align payload on 4-byte boundary if needed
    err = AlignMessagePayload(buf);
    SuccessOrExit(err);

    // Initialize pointer to the buffer payload start
    p = buf->Start();

    // Read size Header
    stepDataLen = 4;
    VerifyOrExit(stepDataLen <= bufSize, err = WEAVE_ERROR_MESSAGE_INCOMPLETE);
    sizeHeader = LittleEndian::Read32(p);
    err = UnpackSizeHeader(sizeHeader, gxWordCount, zkpxgrWordCount, zkpxbWordCount);
    SuccessOrExit(err);

#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG0_TEST_ONLY
    if (ProtocolConfig == kPASEConfig_Config0_TEST_ONLY)
    {
        err = ProcessStep2Data_Config0_TEST_ONLY(buf, stepDataLen, gxWordCount, zkpxgrWordCount, zkpxbWordCount, ResponderStep2ZKPXGRHash);
    }
    else
#endif
#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG1
    if (ProtocolConfig == kPASEConfig_Config1)
    {
        err = ProcessStep2Data_Config1(buf, stepDataLen, gxWordCount, zkpxgrWordCount, zkpxbWordCount, ResponderStep2ZKPXGRHash);
    }
    else
#endif
#if WEAVE_IS_EC_PASE_ENABLED
    {
        err = ProcessStep2Data_ConfigEC(buf, stepDataLen, gxWordCount, zkpxgrWordCount, zkpxbWordCount, ResponderStep2ZKPXGRHash);
    }
#else
    {
        err = WEAVE_ERROR_INVALID_PASE_PARAMETER;
    }
#endif
    SuccessOrExit(err);

    // Verify correct message length
    VerifyOrExit(stepDataLen == bufSize, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Set new PASE state
    State = kState_ResponderStep2Processed;

exit:
    return err;
}

WEAVE_ERROR WeavePASEEngine::GenerateInitiatorStep2(PacketBuffer *buf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint32_t sizeHeader;
    uint16_t stepDataLen;
    uint8_t keyConfirmKey[kKeyConfirmKeyLengthMax];
    uint8_t initiatorStep2ZKPXGRHash[kStep2ZKPXGRHashLengthMax];
    uint8_t *keyConfirmHash = NULL;
    uint8_t step2ZKPXGRHashLength = kStep2ZKPXGRHashLength_Config0_EC;
    uint8_t keyConfirmKeyLength = 0;
    uint8_t keyConfirmHashLength = 0;
    uint8_t *p;

    // Verify correct state
    VerifyOrExit(State == kState_ResponderStep2Processed, err = WEAVE_ERROR_INCORRECT_STATE);

    // Align payload on 4-byte boundary if needed
    err = AlignMessagePayload(buf);
    SuccessOrExit(err);

    // Initialize pointer to the buffer payload start
    p = buf->Start();

#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG1
    if (ProtocolConfig == kPASEConfig_Config1)
        step2ZKPXGRHashLength = kStep2ZKPXGRHashLength_Config1;
#endif

    if (PerformKeyConfirmation)
    {
#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG1
        if (ProtocolConfig == kPASEConfig_Config1)
        {
            keyConfirmKeyLength = kKeyConfirmKeyLength_Config1;
            keyConfirmHashLength = kKeyConfirmHashLength_Config1;
        }
        else
#endif
#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG0_TEST_ONLY || WEAVE_IS_EC_PASE_ENABLED
        {
            keyConfirmKeyLength = kKeyConfirmKeyLength_Config0_EC;
            keyConfirmHashLength = kKeyConfirmHashLength_Config0_EC;
        }
#else
        {
            ExitNow(err = WEAVE_ERROR_INVALID_PASE_PARAMETER);
        }
#endif
    }

    stepDataLen = 4;
    VerifyOrExit(stepDataLen < buf->AvailableDataLength(), err = WEAVE_ERROR_BUFFER_TOO_SMALL);

    // Write the size header.
    sizeHeader = PackSizeHeader( keyConfirmHashLength / 4 );
    LittleEndian::Write32(p, sizeHeader);

#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG0_TEST_ONLY
    if (ProtocolConfig == kPASEConfig_Config0_TEST_ONLY)
    {
        err = GenerateStep2Data_Config0_TEST_ONLY(buf, stepDataLen, initiatorStep2ZKPXGRHash);
    }
    else
#endif
#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG1
    if (ProtocolConfig == kPASEConfig_Config1)
    {
        err = GenerateStep2Data_Config1(buf, stepDataLen, initiatorStep2ZKPXGRHash);
    }
    else
#endif
#if WEAVE_IS_EC_PASE_ENABLED
    {
        err = GenerateStep2Data_ConfigEC(buf, stepDataLen, initiatorStep2ZKPXGRHash);
    }
#else
    {
        err = WEAVE_ERROR_INVALID_PASE_PARAMETER;
    }
#endif
    SuccessOrExit(err);

    keyConfirmHash = buf->Start() + stepDataLen;

    stepDataLen += keyConfirmHashLength;
    VerifyOrExit(buf->AvailableDataLength() >= stepDataLen, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Set message length
    buf->SetDataLength(stepDataLen);

    err = DeriveKeys(initiatorStep2ZKPXGRHash, step2ZKPXGRHashLength, keyConfirmKey, keyConfirmKeyLength);
    SuccessOrExit(err);

    if (PerformKeyConfirmation)
    {
        GenerateKeyConfirmHashes(keyConfirmKey, keyConfirmKeyLength, keyConfirmHash, ResponderKeyConfirmHash, keyConfirmHashLength);
        // Set new PASE state
        State = kState_InitiatorStep2Generated;
    }
    else
    {
        // Set new PASE state
        State = kState_InitiatorDone;
    }

exit:
    return err;
}

WEAVE_ERROR WeavePASEEngine::ProcessInitiatorStep2(PacketBuffer *buf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint32_t sizeHeader;
    uint8_t gxWordCount, zkpxgrWordCount, zkpxbWordCount, expectedKeyConfirmHashWordCount;
    uint16_t stepDataLen;
    uint8_t keyConfirmKey[kKeyConfirmKeyLengthMax];
    uint8_t expectedInitiatorKeyConfirmHash[kKeyConfirmHashLengthMax];
    uint8_t initiatorStep2ZKPXGRHash[kStep2ZKPXGRHashLengthMax];
    uint8_t *keyConfirmHash = NULL;
    uint8_t step2ZKPXGRHashLength = kStep2ZKPXGRHashLength_Config0_EC;
    uint8_t keyConfirmKeyLength = 0;
    uint8_t keyConfirmHashLength = 0;
    const uint8_t *p;

    // Verify correct state
    VerifyOrExit(State == kState_ResponderStep2Generated, err = WEAVE_ERROR_INCORRECT_STATE);

    // Align payload on 4-byte boundary if needed
    err = AlignMessagePayload(buf);
    SuccessOrExit(err);

    // Initialize pointer to the buffer payload start
    p = buf->Start();

#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG1
    if (ProtocolConfig == kPASEConfig_Config1)
        step2ZKPXGRHashLength = kStep2ZKPXGRHashLength_Config1;
#endif

    if (PerformKeyConfirmation)
    {
#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG1
        if (ProtocolConfig == kPASEConfig_Config1)
        {
            keyConfirmKeyLength = kKeyConfirmKeyLength_Config1;
            keyConfirmHashLength = kKeyConfirmHashLength_Config1;
        }
        else
#endif
#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG0_TEST_ONLY || WEAVE_IS_EC_PASE_ENABLED
        {
            keyConfirmKeyLength = kKeyConfirmKeyLength_Config0_EC;
            keyConfirmHashLength = kKeyConfirmHashLength_Config0_EC;
        }
#else
        {
            ExitNow(err = WEAVE_ERROR_INVALID_PASE_PARAMETER);
        }
#endif
    }

    // Read size Header
    stepDataLen = 4;
    VerifyOrExit(stepDataLen <= buf->DataLength(), err = WEAVE_ERROR_MESSAGE_INCOMPLETE);
    sizeHeader = LittleEndian::Read32(p);
    UnpackSizeHeader(sizeHeader, gxWordCount, zkpxgrWordCount, zkpxbWordCount, expectedKeyConfirmHashWordCount);
    // Verify correct key confirm hash length
    VerifyOrExit(4 * expectedKeyConfirmHashWordCount == keyConfirmHashLength, err = WEAVE_ERROR_INVALID_ARGUMENT);

#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG0_TEST_ONLY
    if (ProtocolConfig == kPASEConfig_Config0_TEST_ONLY)
    {
        err = ProcessStep2Data_Config0_TEST_ONLY(buf, stepDataLen, gxWordCount, zkpxgrWordCount, zkpxbWordCount, initiatorStep2ZKPXGRHash);
    }
    else
#endif
#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG1
    if (ProtocolConfig == kPASEConfig_Config1)
    {
        err = ProcessStep2Data_Config1(buf, stepDataLen, gxWordCount, zkpxgrWordCount, zkpxbWordCount, initiatorStep2ZKPXGRHash);
    }
    else
#endif
#if WEAVE_IS_EC_PASE_ENABLED
    {
        err = ProcessStep2Data_ConfigEC(buf, stepDataLen, gxWordCount, zkpxgrWordCount, zkpxbWordCount, initiatorStep2ZKPXGRHash);
    }
#else
    {
        err = WEAVE_ERROR_INVALID_PASE_PARAMETER;
    }
#endif
    SuccessOrExit(err);

    VerifyOrExit(buf->DataLength() == stepDataLen + keyConfirmHashLength, err = WEAVE_ERROR_INVALID_ARGUMENT);

    keyConfirmHash = buf->Start() + stepDataLen;

    err = DeriveKeys(initiatorStep2ZKPXGRHash, step2ZKPXGRHashLength, keyConfirmKey, keyConfirmKeyLength);
    SuccessOrExit(err);

    if (PerformKeyConfirmation)
    {
        GenerateKeyConfirmHashes(keyConfirmKey, keyConfirmKeyLength, expectedInitiatorKeyConfirmHash, ResponderKeyConfirmHash, keyConfirmHashLength);

        if (!ConstantTimeCompare(keyConfirmHash, expectedInitiatorKeyConfirmHash, keyConfirmHashLength))
            ExitNow(err = WEAVE_ERROR_KEY_CONFIRMATION_FAILED);

        // Set new PASE state
        State = kState_InitiatorStep2Processed;
    }
    else
    {
        // Set new PASE state
        State = kState_ResponderDone;
    }

exit:
    return err;
}

WEAVE_ERROR WeavePASEEngine::GenerateResponderKeyConfirm(PacketBuffer *buf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint8_t keyConfirmHashLength = kKeyConfirmHashLength_Config0_EC;

    // Verify correct state
    VerifyOrExit(State == kState_InitiatorStep2Processed, err = WEAVE_ERROR_INCORRECT_STATE);

    VerifyOrExit(PerformKeyConfirmation, err = WEAVE_ERROR_INCORRECT_STATE);

#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG1
    if (ProtocolConfig == kPASEConfig_Config1)
        keyConfirmHashLength = kKeyConfirmHashLength_Config1;
#endif

    VerifyOrExit(keyConfirmHashLength <= buf->AvailableDataLength(), err = WEAVE_ERROR_BUFFER_TOO_SMALL);

    memcpy(buf->Start(), ResponderKeyConfirmHash, keyConfirmHashLength);

    // Set message length
    buf->SetDataLength(keyConfirmHashLength);

    // Set new PASE state
    State = kState_ResponderDone;

exit:
    return err;
}

WEAVE_ERROR WeavePASEEngine::ProcessResponderKeyConfirm(PacketBuffer *buf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint8_t keyConfirmHashLength = kKeyConfirmHashLength_Config0_EC;

    // Verify correct state
    VerifyOrExit(State == kState_InitiatorStep2Generated, err = WEAVE_ERROR_INCORRECT_STATE);

    VerifyOrExit(PerformKeyConfirmation, err = WEAVE_ERROR_INCORRECT_STATE);

#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG1
    if (ProtocolConfig == kPASEConfig_Config1)
        keyConfirmHashLength = kKeyConfirmHashLength_Config1;
#endif

    VerifyOrExit(keyConfirmHashLength == buf->DataLength(), err = WEAVE_ERROR_INVALID_ARGUMENT);

    if (!ConstantTimeCompare(buf->Start(), ResponderKeyConfirmHash, keyConfirmHashLength))
        ExitNow(err = WEAVE_ERROR_KEY_CONFIRMATION_FAILED);

    // Set new PASE state
    State = kState_InitiatorDone;

exit:
    return err;
}

WEAVE_ERROR WeavePASEEngine::GenerateResponderReconfigure(PacketBuffer *buf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint8_t *p = buf->Start();
    const uint16_t stepDataLen = 4;

    // Verify correct state
    VerifyOrExit(State == kState_Reset, err = WEAVE_ERROR_INCORRECT_STATE);

    // Verify buffer size
    VerifyOrExit(stepDataLen <= buf->AvailableDataLength(), err = WEAVE_ERROR_BUFFER_TOO_SMALL);

    // Verify that proposed reconfiguration is allowed
    VerifyOrExit(IsAllowedPASEConfig(ProtocolConfig), err = WEAVE_ERROR_INVALID_PASE_PARAMETER);

    // Write proposed reconfiguration protocol option
    LittleEndian::Write32(p, ProtocolConfig);

    // Set message length
    buf->SetDataLength(stepDataLen);

    // Set new PASE state
    State = kState_ResponderDone;

exit:
    return err;
}

WEAVE_ERROR WeavePASEEngine::ProcessResponderReconfigure(PacketBuffer *buf, uint32_t & proposedPASEConfig)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint8_t *p = buf->Start();
    const uint16_t stepDataLen = 4;

    // Verify correct state
    VerifyOrExit(State == kState_InitiatorStep1Generated, err = WEAVE_ERROR_INCORRECT_STATE);

    // Verify correct message length
    VerifyOrExit(stepDataLen == buf->DataLength(), err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Read proposed reconfiguration protocol option
    proposedPASEConfig = LittleEndian::Read32(p);

    // Verify that proposed config is allowed
    VerifyOrExit(IsAllowedPASEConfig(proposedPASEConfig), err = WEAVE_ERROR_INVALID_PASE_CONFIGURATION);

    // Set new PASE state
    State = kState_ResponderReconfigProcessed;

exit:
    return err;
}

WEAVE_ERROR WeavePASEEngine::GetSessionKey(const WeaveEncryptionKey *& encKey)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // Verify correct state
    VerifyOrExit(State == kState_InitiatorDone || State == kState_ResponderDone, err = WEAVE_ERROR_INCORRECT_STATE);

    encKey = &EncryptionKey;

exit:
    return err;
}

#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG0_TEST_ONLY

WEAVE_ERROR WeavePASEEngine::GenerateStep1Data_Config0_TEST_ONLY(PacketBuffer *buf, uint16_t &stepDataLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint8_t *p = buf->Start() + stepDataLen;

    stepDataLen += 2 * (kPASEConfig0_GXByteCount + kPASEConfig0_ZKPXGRByteCount + kPASEConfig0_ZKPXBByteCount);

    VerifyOrExit(stepDataLen <= buf->AvailableDataLength(), err = WEAVE_ERROR_BUFFER_TOO_SMALL);

    memset(p, kPASEConfig0_GXStep1p1Value, kPASEConfig0_GXByteCount);
    p += kPASEConfig0_GXByteCount;
    memset(p, kPASEConfig0_ZKPXGRStep1p1Value, kPASEConfig0_ZKPXGRByteCount);
    p += kPASEConfig0_ZKPXGRByteCount;
    memset(p, kPASEConfig0_ZKPXBStep1p1Value, kPASEConfig0_ZKPXBByteCount);
    p += kPASEConfig0_ZKPXBByteCount;
    memset(p, kPASEConfig0_GXStep1p2Value, kPASEConfig0_GXByteCount);
    p += kPASEConfig0_GXByteCount;
    memset(p, kPASEConfig0_ZKPXGRStep1p2Value, kPASEConfig0_ZKPXGRByteCount);
    p += kPASEConfig0_ZKPXGRByteCount;
    memset(p, kPASEConfig0_ZKPXBStep1p2Value, kPASEConfig0_ZKPXBByteCount);

exit:
    return err;
}

WEAVE_ERROR WeavePASEEngine::ProcessStep1Data_Config0_TEST_ONLY(PacketBuffer *buf, uint16_t &stepDataLen, uint8_t gxWordCount, uint8_t zkpxgrWordCount, uint8_t zkpxbWordCount)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint8_t *p = buf->Start() + stepDataLen;
    uint8_t res;

    VerifyOrExit( gxWordCount == kPASEHeader_GXWordCountMaxConfig0 &&
                  zkpxgrWordCount == kPASEHeader_ZKPXGRWordCountMaxConfig0 &&
                  zkpxbWordCount == kPASEHeader_ZKPXBWordCountMaxConfig0, err = WEAVE_ERROR_INVALID_ARGUMENT);

    stepDataLen += 2 * (kPASEConfig0_GXByteCount + kPASEConfig0_ZKPXGRByteCount + kPASEConfig0_ZKPXBByteCount);

    VerifyOrExit(stepDataLen <= buf->DataLength(), err = WEAVE_ERROR_MESSAGE_INCOMPLETE);

    // Verify GX Step1 Part1 Value
    {
        uint8_t tmpBuf[kPASEConfig0_GXByteCount];
        memset(tmpBuf, kPASEConfig0_GXStep1p1Value, kPASEConfig0_GXByteCount);
        res = memcmp(p, tmpBuf, kPASEConfig0_GXByteCount);
        VerifyOrExit(res == 0, err = WEAVE_ERROR_INVALID_PASE_PARAMETER);
        p += kPASEConfig0_GXByteCount;
    }
    // Verify ZKP GR Step1 Part1 Value
    {
        uint8_t tmpBuf[kPASEConfig0_ZKPXGRByteCount];
        memset(tmpBuf, kPASEConfig0_ZKPXGRStep1p1Value, kPASEConfig0_ZKPXGRByteCount);
        res = memcmp(p, tmpBuf, kPASEConfig0_ZKPXGRByteCount);
        VerifyOrExit(res == 0, err = WEAVE_ERROR_INVALID_PASE_PARAMETER);
        p += kPASEConfig0_ZKPXGRByteCount;
    }
    // Verify ZKP B Step1 Part1 Value
    {
        uint8_t tmpBuf[kPASEConfig0_ZKPXBByteCount];
        memset(tmpBuf, kPASEConfig0_ZKPXBStep1p1Value, kPASEConfig0_ZKPXBByteCount);
        res = memcmp(p, tmpBuf, kPASEConfig0_ZKPXBByteCount);
        VerifyOrExit(res == 0, err = WEAVE_ERROR_INVALID_PASE_PARAMETER);
        p += kPASEConfig0_ZKPXBByteCount;
    }
    // Verify GX Step1 Part2 Value
    {
        uint8_t tmpBuf[kPASEConfig0_GXByteCount];
        memset(tmpBuf, kPASEConfig0_GXStep1p2Value, kPASEConfig0_GXByteCount);
        res = memcmp(p, tmpBuf, kPASEConfig0_GXByteCount);
        VerifyOrExit(res == 0, err = WEAVE_ERROR_INVALID_PASE_PARAMETER);
        p += kPASEConfig0_GXByteCount;
    }
    // Verify ZKP GR Step1 Part2 Value
    {
        uint8_t tmpBuf[kPASEConfig0_ZKPXGRByteCount];
        memset(tmpBuf, kPASEConfig0_ZKPXGRStep1p2Value, kPASEConfig0_ZKPXGRByteCount);
        res = memcmp(p, tmpBuf, kPASEConfig0_ZKPXGRByteCount);
        VerifyOrExit(res == 0, err = WEAVE_ERROR_INVALID_PASE_PARAMETER);
        p += kPASEConfig0_ZKPXGRByteCount;
    }
    // Verify ZKP B Step1 Part2 Value
    {
        uint8_t tmpBuf[kPASEConfig0_ZKPXBByteCount];
        memset(tmpBuf, kPASEConfig0_ZKPXBStep1p2Value, kPASEConfig0_ZKPXBByteCount);
        res = memcmp(p, tmpBuf, kPASEConfig0_ZKPXBByteCount);
        VerifyOrExit(res == 0, err = WEAVE_ERROR_INVALID_PASE_PARAMETER);
    }

exit:
    return err;
}

WEAVE_ERROR WeavePASEEngine::GenerateStep2Data_Config0_TEST_ONLY(PacketBuffer *buf, uint16_t &stepDataLen, uint8_t *step2ZKPXGRHash)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint8_t *p = buf->Start() + stepDataLen;

    stepDataLen += kPASEConfig0_GXByteCount + kPASEConfig0_ZKPXGRByteCount + kPASEConfig0_ZKPXBByteCount;

    VerifyOrExit(stepDataLen <= buf->AvailableDataLength(), err = WEAVE_ERROR_BUFFER_TOO_SMALL);

    memset(p, kPASEConfig0_GXStep2Value, kPASEConfig0_GXByteCount);
    p += kPASEConfig0_GXByteCount;

    memset(p, kPASEConfig0_ZKPXGRStep2Value, kPASEConfig0_ZKPXGRByteCount);
    // Compute and save a hash of the Gr value of the ZKP for x4*s.
    // This will be used later in deriving the session keys.
    ProtocolHash(p, kPASEConfig0_ZKPXGRByteCount, step2ZKPXGRHash);
    p += kPASEConfig0_ZKPXGRByteCount;

    memset(p, kPASEConfig0_ZKPXBStep2Value, kPASEConfig0_ZKPXBByteCount);

exit:
    return err;
}

WEAVE_ERROR WeavePASEEngine::ProcessStep2Data_Config0_TEST_ONLY(PacketBuffer *buf, uint16_t &stepDataLen, uint8_t gxWordCount, uint8_t zkpxgrWordCount, uint8_t zkpxbWordCount, uint8_t *step2ZKPXGRHash)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint8_t *p = buf->Start() + stepDataLen;
    uint8_t res;

    VerifyOrExit( gxWordCount == kPASEHeader_GXWordCountMaxConfig0 &&
                  zkpxgrWordCount == kPASEHeader_ZKPXGRWordCountMaxConfig0 &&
                  zkpxbWordCount == kPASEHeader_ZKPXBWordCountMaxConfig0, err = WEAVE_ERROR_INVALID_ARGUMENT);

    stepDataLen += kPASEConfig0_GXByteCount + kPASEConfig0_ZKPXGRByteCount + kPASEConfig0_ZKPXBByteCount;

    VerifyOrExit(stepDataLen <= buf->DataLength(), err = WEAVE_ERROR_MESSAGE_INCOMPLETE);

    // Verify GX Step2 Value
    {
        uint8_t tmpBuf[kPASEConfig0_GXByteCount];
        memset(tmpBuf, kPASEConfig0_GXStep2Value, kPASEConfig0_GXByteCount);
        res = memcmp(p, tmpBuf, kPASEConfig0_GXByteCount);
        VerifyOrExit(res == 0, err = WEAVE_ERROR_INVALID_PASE_PARAMETER);
        p += kPASEConfig0_GXByteCount;
    }
    // Verify ZKP GR Step2 Value
    {
        uint8_t tmpBuf[kPASEConfig0_ZKPXGRByteCount];
        memset(tmpBuf, kPASEConfig0_ZKPXGRStep2Value, kPASEConfig0_ZKPXGRByteCount);
        res = memcmp(p, tmpBuf, kPASEConfig0_ZKPXGRByteCount);
        VerifyOrExit(res == 0, err = WEAVE_ERROR_INVALID_PASE_PARAMETER);
        // Compute and save a hash of the Gr value of the ZKP for x4*s.
        // This will be used later in deriving the session keys.
        ProtocolHash(p, kPASEConfig0_ZKPXGRByteCount, step2ZKPXGRHash);
        p += kPASEConfig0_ZKPXGRByteCount;
    }
    // Verify ZKP B Step2 Value
    {
        uint8_t tmpBuf[kPASEConfig0_ZKPXBByteCount];
        memset(tmpBuf, kPASEConfig0_ZKPXBStep2Value, kPASEConfig0_ZKPXBByteCount);
        res = memcmp(p, tmpBuf, kPASEConfig0_ZKPXBByteCount);
        VerifyOrExit(res == 0, err = WEAVE_ERROR_INVALID_PASE_PARAMETER);
    }

exit:
    return err;
}
#endif /* WEAVE_CONFIG_SUPPORT_PASE_CONFIG0_TEST_ONLY */


#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG1

static void BigNumHash(const BIGNUM& point, uint8_t *h)
{
    SHA1   hash;
    hash.Begin();
    hash.AddData(point);
    hash.Finish(h);
}

WEAVE_ERROR WeavePASEEngine::GenerateStep1Data_Config1(PacketBuffer *buf, uint16_t &stepDataLen)
{
    WEAVE_ERROR err;
    JPAKE_STEP1 step1;
    uint8_t *p = buf->Start() + stepDataLen;

    // Generate the J-PAKE step 1 information to be sent to the peer.
    //
    // For an initiator, this consists of the following values:
    //     g^x1
    //     g^x2
    //     zero-knowledge proof of x1 [ g^r and b values ]
    //     zero-knowledge proof of x2 [ g^r and b values ]
    //
    // For a responder, this consists of:
    //     g^x3
    //     g^x4
    //     zero-knowledge proof of x3 [ g^r and b values ]
    //     zero-knowledge proof of x4 [ g^r and b values ]

    JPAKE_STEP1_init(&step1);

    // Verify space in output buffer.
    const size_t fieldDataLen = (kPASEHeader_GXWordCountMaxConfig1 +
                                 kPASEHeader_GXWordCountMaxConfig1 +
                                 kPASEHeader_ZKPXGRWordCountMaxConfig1 +
                                 kPASEHeader_ZKPXBWordCountMaxConfig1 +
                                 kPASEHeader_ZKPXGRWordCountMaxConfig1 +
                                 kPASEHeader_ZKPXBWordCountMaxConfig1) * sizeof(uint32_t);
    VerifyOrExit(stepDataLen + fieldDataLen <= buf->AvailableDataLength(), err = WEAVE_ERROR_BUFFER_TOO_SMALL);

    // Generate STEP1 data struct
    JPAKE_STEP1_generate(&step1, JPAKECtx);

    // Encode STEP1 Fields
    err = EncodeBIGNUMValueLE(*step1.p1.gx, kPASEHeader_GXWordCountMaxConfig1 * sizeof(uint32_t), p);           // GXa
    SuccessOrExit(err);
    err = EncodeBIGNUMValueLE(*step1.p2.gx, kPASEHeader_GXWordCountMaxConfig1 * sizeof(uint32_t), p);           // GXb
    SuccessOrExit(err);
    err = EncodeBIGNUMValueLE(*step1.p1.zkpx.gr, kPASEHeader_ZKPXGRWordCountMaxConfig1 * sizeof(uint32_t), p);  // ZKPXaGR
    SuccessOrExit(err);
    err = EncodeBIGNUMValueLE(*step1.p1.zkpx.b, kPASEHeader_ZKPXBWordCountMaxConfig1 * sizeof(uint32_t), p);    // ZKPXaB
    SuccessOrExit(err);
    err = EncodeBIGNUMValueLE(*step1.p2.zkpx.gr, kPASEHeader_ZKPXGRWordCountMaxConfig1 * sizeof(uint32_t), p);  // ZKPXbGR
    SuccessOrExit(err);
    err = EncodeBIGNUMValueLE(*step1.p2.zkpx.b, kPASEHeader_ZKPXBWordCountMaxConfig1 * sizeof(uint32_t), p);    // ZKPXbB
    SuccessOrExit(err);

    stepDataLen = (uint16_t)(p - buf->Start());

exit:
    JPAKE_STEP1_release(&step1);
    return err;
}

WEAVE_ERROR WeavePASEEngine::ProcessStep1Data_Config1(PacketBuffer *buf, uint16_t &stepDataLen, uint8_t gxWordCount, uint8_t zkpxgrWordCount, uint8_t zkpxbWordCount)
{
    WEAVE_ERROR err;
    int res;
    JPAKE_STEP1 step1;
    const uint8_t *p = buf->Start() + stepDataLen;

    // Init STEP1 data struct
    JPAKE_STEP1_init(&step1);

    // Verify the input buffer contains the expected amount of data.
    const size_t expectedFieldDataLen = (gxWordCount +
                                         gxWordCount +
                                         zkpxgrWordCount +
                                         zkpxbWordCount +
                                         zkpxgrWordCount +
                                         zkpxbWordCount) * sizeof(uint32_t);
    VerifyOrExit(buf->DataLength() >= stepDataLen + expectedFieldDataLen, err = WEAVE_ERROR_INVALID_MESSAGE_LENGTH);

    // Decode STEP1 data fields
    err = DecodeBIGNUMValueLE(*step1.p1.gx, gxWordCount * sizeof(uint32_t), p);           // GXa
    SuccessOrExit(err);
    err = DecodeBIGNUMValueLE(*step1.p2.gx, gxWordCount * sizeof(uint32_t), p);           // GXb
    SuccessOrExit(err);
    err = DecodeBIGNUMValueLE(*step1.p1.zkpx.gr, zkpxgrWordCount * sizeof(uint32_t), p);  // ZKPXaGR
    SuccessOrExit(err);
    err = DecodeBIGNUMValueLE(*step1.p1.zkpx.b, zkpxbWordCount * sizeof(uint32_t), p);    // ZKPXaB
    SuccessOrExit(err);
    err = DecodeBIGNUMValueLE(*step1.p2.zkpx.gr, zkpxgrWordCount * sizeof(uint32_t), p);  // ZKPXbGR
    SuccessOrExit(err);
    err = DecodeBIGNUMValueLE(*step1.p2.zkpx.b, zkpxbWordCount * sizeof(uint32_t), p);    // ZKPXbB
    SuccessOrExit(err);

    stepDataLen = (uint16_t)(p - buf->Start());

    // Process the J-PAKE STEP1 parameters sent to us from the peer.
    res = JPAKE_STEP1_process(JPAKECtx, &step1);
    VerifyOrExit(res == 1, err = WEAVE_ERROR_INVALID_PASE_PARAMETER);

exit:
    JPAKE_STEP1_release(&step1);
    return err;
}

WEAVE_ERROR WeavePASEEngine::GenerateStep2Data_Config1(PacketBuffer *buf, uint16_t &stepDataLen, uint8_t *step2ZKPXGRHash)
{
    WEAVE_ERROR err;
    JPAKE_STEP2 step2;
    uint8_t *p = buf->Start() + stepDataLen;

    // Generate the J-PAKE step 2 information to be sent to the peer.
    //
    // For an initiator, this consists of the following values:
    //
    //     A value [ equal to g^((x1 + x2 + x4) * x2 * s) ]
    //     zero-knowledge proof of x2 * s [ g^r and b values ]
    //
    // For a responder, this consists of:
    //
    //     B value [ equal to g^((x1 + x2 + x3) * x4 * s) ]
    //     zero-knowledge proof of x4 * s [ g^r and b values ]

    JPAKE_STEP2_init(&step2);

    // Verify space in output buffer.
    const size_t fieldDataLen = (kPASEHeader_GXWordCountMaxConfig1 +
                                 kPASEHeader_ZKPXGRWordCountMaxConfig1 +
                                 kPASEHeader_ZKPXBWordCountMaxConfig1) * sizeof(uint32_t);
    VerifyOrExit(stepDataLen + fieldDataLen <= buf->AvailableDataLength(), err = WEAVE_ERROR_BUFFER_TOO_SMALL);

    // Generate STEP2 data struct
    JPAKE_STEP2_generate(&step2, JPAKECtx);

    // Encode STEP2 data fields
    err = EncodeBIGNUMValueLE(*step2.gx, kPASEHeader_GXWordCountMaxConfig1 * sizeof(uint32_t), p);          // GX
    SuccessOrExit(err);
    err = EncodeBIGNUMValueLE(*step2.zkpx.gr, kPASEHeader_ZKPXGRWordCountMaxConfig1 * sizeof(uint32_t), p); // ZKPXGR
    SuccessOrExit(err);
    err = EncodeBIGNUMValueLE(*step2.zkpx.b, kPASEHeader_ZKPXBWordCountMaxConfig1 * sizeof(uint32_t), p);   // ZKPXB
    SuccessOrExit(err);

    stepDataLen = (uint16_t)(p - buf->Start());

    // Compute and save a hash of the g^r value of the ZKP for x4*s.
    // This will be used later in deriving the session keys.
    BigNumHash(*step2.zkpx.gr, step2ZKPXGRHash);

exit:
    JPAKE_STEP2_release(&step2);
    return err;
}

WEAVE_ERROR WeavePASEEngine::ProcessStep2Data_Config1(PacketBuffer *buf, uint16_t &stepDataLen, uint8_t gxWordCount, uint8_t zkpxgrWordCount, uint8_t zkpxbWordCount, uint8_t *step2ZKPXGRHash)
{
    WEAVE_ERROR err;
    int res;
    JPAKE_STEP2 step2;
    const uint8_t *p = buf->Start() + stepDataLen;

    // Init STEP2 data struct
    JPAKE_STEP2_init(&step2);

    // Verify the input buffer contains the expected amount of data.
    const size_t expectedFieldDataLen = (gxWordCount +
                                         zkpxgrWordCount +
                                         zkpxbWordCount) * sizeof(uint32_t);
    VerifyOrExit(buf->DataLength() >= stepDataLen + expectedFieldDataLen, err = WEAVE_ERROR_INVALID_MESSAGE_LENGTH);

    // Decode STEP2 data fields
    err = DecodeBIGNUMValueLE(*step2.gx, gxWordCount * sizeof(uint32_t), p);           // GX
    SuccessOrExit(err);
    err = DecodeBIGNUMValueLE(*step2.zkpx.gr, zkpxgrWordCount * sizeof(uint32_t), p);  // ZKPXGR
    SuccessOrExit(err);
    err = DecodeBIGNUMValueLE(*step2.zkpx.b, zkpxbWordCount * sizeof(uint32_t), p);    // ZKPXB
    SuccessOrExit(err);

    // Process the J-PAKE STEP2 parameters sent to us from the peer.
    res = JPAKE_STEP2_process(JPAKECtx, &step2);
    VerifyOrExit(res == 1, err = WEAVE_ERROR_INVALID_PASE_PARAMETER);

    // Compute and save a hash of the g^r value of the ZKP for x4*s.
    // This will be used later in deriving the session keys.
    BigNumHash(*step2.zkpx.gr, step2ZKPXGRHash);

    // update data length
    stepDataLen = (uint16_t)(p - buf->Start());

exit:
    JPAKE_STEP2_release(&step2);
    return err;
}
#endif /* WEAVE_CONFIG_SUPPORT_PASE_CONFIG1 */


#if WEAVE_IS_EC_PASE_ENABLED
WEAVE_ERROR WeavePASEEngine::GenerateStep1Data_ConfigEC(PacketBuffer *buf, uint16_t &stepDataLen)
{
    return mEllipticCurveJPAKE.GenerateStep1(buf->Start(), buf->AvailableDataLength(), stepDataLen);
}

WEAVE_ERROR WeavePASEEngine::ProcessStep1Data_ConfigEC(PacketBuffer *buf, uint16_t &stepDataLen, uint8_t gxWordCount, uint8_t zkpxgrWordCount, uint8_t zkpxbWordCount)
{
    WEAVE_ERROR err;
    uint8_t ecjpakeScalarWordCount;
    uint8_t ecjpakePointWordCount;

    ecjpakeScalarWordCount = mEllipticCurveJPAKE.GetCurveSize() / 4;
    ecjpakePointWordCount = 2 * ecjpakeScalarWordCount;

#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG2
    if (ProtocolConfig == kPASEConfig_Config2)
        ecjpakeScalarWordCount++;
#endif

    VerifyOrExit( gxWordCount == ecjpakePointWordCount &&
                  zkpxgrWordCount == ecjpakePointWordCount &&
                  zkpxbWordCount == ecjpakeScalarWordCount, err = WEAVE_ERROR_INVALID_ARGUMENT);

    err = mEllipticCurveJPAKE.ProcessStep1(buf->Start(), buf->DataLength(), stepDataLen);

exit:
    return err;
}

WEAVE_ERROR WeavePASEEngine::GenerateStep2Data_ConfigEC(PacketBuffer *buf, uint16_t &stepDataLen, uint8_t *step2ZKPXGRHash)
{
    WEAVE_ERROR err;
    uint16_t ecjpakePointByteCount;

    err = mEllipticCurveJPAKE.GenerateStep2(buf->Start(), buf->AvailableDataLength(), stepDataLen);
    SuccessOrExit(err);

    // Compute and save a hash of the Gr value of the ZKP for x4*s.
    // This will be used later in deriving the session keys.
    ecjpakePointByteCount = 2 * mEllipticCurveJPAKE.GetCurveSize();
    VerifyOrExit(ecjpakePointByteCount != 0, err = WEAVE_ERROR_UNSUPPORTED_ELLIPTIC_CURVE);
    ProtocolHash(buf->Start() + ecjpakePointByteCount, ecjpakePointByteCount, step2ZKPXGRHash);

exit:
    return err;
}

WEAVE_ERROR WeavePASEEngine::ProcessStep2Data_ConfigEC(PacketBuffer *buf, uint16_t &stepDataLen, uint8_t gxWordCount, uint8_t zkpxgrWordCount, uint8_t zkpxbWordCount, uint8_t *step2ZKPXGRHash)
{
    WEAVE_ERROR err;
    uint8_t ecjpakeScalarWordCount;
    uint8_t ecjpakePointWordCount;
    uint16_t ecjpakePointByteCount;

    ecjpakeScalarWordCount = mEllipticCurveJPAKE.GetCurveSize() / 4;
    ecjpakePointWordCount = 2 * ecjpakeScalarWordCount;
    ecjpakePointByteCount = 4 * ecjpakePointWordCount;

    VerifyOrExit(ecjpakePointByteCount != 0, err = WEAVE_ERROR_UNSUPPORTED_ELLIPTIC_CURVE);

#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG2
    if (ProtocolConfig == kPASEConfig_Config2)
        ecjpakeScalarWordCount++;
#endif

    VerifyOrExit( gxWordCount == ecjpakePointWordCount &&
                  zkpxgrWordCount == ecjpakePointWordCount &&
                  zkpxbWordCount == ecjpakeScalarWordCount, err = WEAVE_ERROR_INVALID_ARGUMENT);

    err = mEllipticCurveJPAKE.ProcessStep2(buf->Start(), buf->DataLength(), stepDataLen);
    SuccessOrExit(err);

    // Compute and save a hash of the Gr value of the ZKP for x4*s.
    // This will be used later in deriving the session keys.
    ProtocolHash(buf->Start() + ecjpakePointByteCount, ecjpakePointByteCount, step2ZKPXGRHash);

exit:
    return err;
}
#endif /* WEAVE_IS_EC_PASE_ENABLED */


WEAVE_ERROR WeavePASEEngine::DeriveKeys(const uint8_t *initiatorStep2ZKPXGRHash, const uint8_t step2ZKPXGRHashLength, uint8_t *keyConfirmKey, const uint8_t keyConfirmKeyLength)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    HKDFSHA1 hkdf;
    union
    {
        uint8_t keySalt[2 * kStep2ZKPXGRHashLengthMax];
        uint8_t sessionKeyData[WeaveEncryptionKey_AES128CTRSHA1::KeySize + kKeyConfirmKeyLengthMax];
    };
    uint16_t keyLen;

    // Only AES128CTRSHA1 keys supported for now.
    VerifyOrExit(EncryptionType == kWeaveEncryptionType_AES128CTRSHA1, err = WEAVE_ERROR_UNSUPPORTED_ENCRYPTION_TYPE);

    // Produce a salt value to be used in generating a master key. The salt is constructed by concatenating the
    // ZKP g^r value for x2*s (generated by the initiator in round 2) and the ZKP g^r value for x4*s (generated
    // by the responder in round 2). Both values are randomly generated and authenticated by each party as part
    // of the J-PAKE protocol. This is similar to the way TLS generates keys, and provides a measure of safety
    // in the event that one party has a bad random number generator.
    memcpy(keySalt, initiatorStep2ZKPXGRHash, step2ZKPXGRHashLength);
    memcpy(keySalt + step2ZKPXGRHashLength, ResponderStep2ZKPXGRHash, step2ZKPXGRHashLength);

    // Perform HKDF-based key extraction to produce a master pseudo-random key from the
    // J-PAKE key material.
    hkdf.BeginExtractKey(keySalt, 2 * step2ZKPXGRHashLength);

    // Retrieve the shared key material produced as a result of the J-PAKE interaction.
#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG0_TEST_ONLY
    if (ProtocolConfig == kPASEConfig_Config0_TEST_ONLY)
    {
        hkdf.AddKeyMaterial(KeyMeterial_Config0, kKeyMaterialLength_Config0_EC);
    }
    else
#endif
#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG1
    if (ProtocolConfig == kPASEConfig_Config1)
    {
        const BIGNUM *keyMaterial = JPAKE_get_shared_key(JPAKECtx);
        VerifyOrExit(keyMaterial != NULL, err = WEAVE_ERROR_INCORRECT_STATE);

        hkdf.AddKeyMaterial(*keyMaterial);
    }
    else
#endif
#if WEAVE_IS_EC_PASE_ENABLED
    {
        const uint8_t *keyMaterial = mEllipticCurveJPAKE.GetSharedSecret();
        VerifyOrExit(keyMaterial != NULL, err = WEAVE_ERROR_INCORRECT_STATE);

        hkdf.AddKeyMaterial(keyMaterial, kKeyMaterialLength_Config0_EC);
    }
#else
    {
        ExitNow(err = WEAVE_ERROR_INVALID_PASE_PARAMETER);
    }
#endif

    // Generate a master key from which the session keys will be derived...
    err = hkdf.FinishExtractKey();
    SuccessOrExit(err);

#ifdef PASE_PRINT_CRYPTO_DATA
        printf("Master PRK: "); PrintHex(hkdf.PseudoRandomKey, HKDFSHA1::kPseudoRandomKeyLength); printf("\n");
#endif

    // Derive the session keys from the master key...
    // If performing key confirmation, arrange to generate enough key data for the session
    // keys (data encryption and integrity) as well as a key to be used in key confirmation.
    keyLen = WeaveEncryptionKey_AES128CTRSHA1::KeySize + keyConfirmKeyLength;

    // Perform HKDF-based key expansion to produce the desired key data.
    err = hkdf.ExpandKey(NULL, 0, keyLen, sessionKeyData);
    SuccessOrExit(err);

#ifdef PASE_PRINT_CRYPTO_DATA
        printf("Session Key Data: "); PrintHex(sessionKeyData, keyLen); printf("\n");
#endif

    // Copy the generated key data to the appropriate destinations.
    memcpy(EncryptionKey.AES128CTRSHA1.DataKey,
           sessionKeyData,
           WeaveEncryptionKey_AES128CTRSHA1::DataKeySize);
    memcpy(EncryptionKey.AES128CTRSHA1.IntegrityKey,
           sessionKeyData + WeaveEncryptionKey_AES128CTRSHA1::DataKeySize,
           WeaveEncryptionKey_AES128CTRSHA1::IntegrityKeySize);
    memcpy(keyConfirmKey,
           sessionKeyData + WeaveEncryptionKey_AES128CTRSHA1::DataKeySize
                          + WeaveEncryptionKey_AES128CTRSHA1::IntegrityKeySize,
           keyConfirmKeyLength);

    ClearSecretData(sessionKeyData, keyLen);

exit:
    return err;
}

void WeavePASEEngine::GenerateKeyConfirmHashes(const uint8_t *keyConfirmKey, const uint8_t keyConfirmKeyLength, uint8_t *initiatorHash, uint8_t *responderHash, const uint8_t keyConfirmHashLength)
{
    // Generate a single hash of the key confirmation key to use as the responder's key confirmation hash.
    ProtocolHash(keyConfirmKey, keyConfirmKeyLength, responderHash);

    // Generate a double hash of the key confirmation key to use as the initiators's key confirmation hash.
    ProtocolHash(responderHash, keyConfirmHashLength, initiatorHash);
}


} // namespace PASE
} // namespace Security
} // namespace Profiles
} // namespace Weave
} // namespace nl
