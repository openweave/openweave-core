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
 *      This file implements unit tests for the Weave application keys library.
 *
 */

#include <stdio.h>
#include <nltest.h>
#include <string.h>

#include "ToolCommon.h"
#include "TestGroupKeyStore.h"
#include <Weave/Support/ErrorStr.h>
#include <Weave/Core/WeaveKeyIds.h>
#include <Weave/Core/WeaveEncoding.h>
#include <Weave/Profiles/security/WeaveApplicationKeys.h>
#include <Weave/Profiles/security/WeavePasscodes.h>
#include <Weave/Support/crypto/WeaveCrypto.h>

using namespace nl::Weave::Encoding;
using namespace nl::Weave::Crypto;
using namespace nl::Weave::Platform::Security;
using namespace nl::Weave::Profiles::Security::AppKeys;
using namespace nl::Weave::Profiles::Security::Passcodes;

#define DEBUG_PRINT_ENABLE 0

static const uint8_t sWeavePasscode[] =
{
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x41, 0x42
};

static const uint8_t sWeavePaddedPasscode[] =
{
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x41, 0x42, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t sWeaveEncryptedPasscodeConfig1[] =
{
    0x01, 0x00, 0x00, 0x00, 0x00, 0xC9, 0x25, 0xA8, 0xF4, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36,
    0x37, 0x38, 0x39, 0x41, 0x42, 0x00, 0x00, 0x00, 0x00, 0x7A, 0x3E, 0xD3, 0xA4, 0x18, 0x86, 0x25,
    0x09, 0xCA, 0x96, 0xF7, 0xC9, 0x05, 0x42, 0x13, 0x43,
};

static const uint8_t sWeaveEncryptedPasscodeConfig2_Rotating[] =
{
    0x02, 0x04, 0x54, 0x00, 0x00, 0xC9, 0x25, 0xA8, 0xF4, 0xC7, 0x0A, 0x3E, 0xBA, 0xDF, 0x33, 0xA1,
    0xCE, 0xB4, 0x94, 0xF0, 0xE0, 0xE6, 0x23, 0x98, 0x2F, 0x52, 0xD0, 0xC7, 0xAE, 0xB5, 0x1B, 0xCB,
    0x4D, 0xFD, 0x72, 0x77, 0xE7, 0xA6, 0x95, 0xFB, 0xAC,
};

static const uint8_t sWeaveEncryptedPasscodeConfig2_Static[] =
{
    0x02, 0x04, 0x44, 0x00, 0x00, 0xC9, 0x25, 0xA8, 0xF4, 0x3E, 0x8D, 0xA7, 0x68, 0xC7, 0x67, 0x91,
    0xF9, 0x16, 0xC3, 0x42, 0x2C, 0x82, 0x26, 0x4B, 0xDE, 0x14, 0x39, 0x2B, 0x38, 0x7B, 0xDA, 0x88,
    0xF8, 0xFD, 0x72, 0x77, 0xE7, 0xA6, 0x95, 0xFB, 0xAC,
};

static const uint8_t sLongWeavePasscode[] =
{
    0x5A, 0x59, 0x58, 0x57, 0x56, 0x55, 0x54, 0x53, 0x52, 0x51, 0x50, 0x4F, 0x4E, 0x4D, 0x4C, 0x4B
};


void PasscodeEncryptConfig1_Test1(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err;
    const uint8_t config = kPasscode_Config1_TEST_ONLY;
    uint8_t passcode[kPasscodePaddedLen];
    size_t passcodeLen;
    uint8_t encPasscode[kPasscodeMaxEncryptedLen];
    uint8_t encPasscodeManual[kPasscodeMaxEncryptedLen];
    size_t encPasscodeLen;

    // Encrypt passcode.
    err = EncryptPasscode(config, kPasscodeConfig1_KeyId, sPasscodeEncryptionKeyNonce,
                          sWeavePasscode, sizeof(sWeavePasscode),
                          encPasscode, sizeof(encPasscode), encPasscodeLen, NULL);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    NL_TEST_ASSERT(inSuite, encPasscodeLen == sizeof(sWeaveEncryptedPasscodeConfig1));
    NL_TEST_ASSERT(inSuite, memcmp(encPasscode, sWeaveEncryptedPasscodeConfig1, encPasscodeLen) == 0);

#if DEBUG_PRINT_ENABLE
    printf("Encrypted passcode:\n");
    DumpMemoryCStyle(encPasscode, encPasscodeLen, "    ", 16);
#endif

    // Decrypt passcode.
    err = DecryptPasscode(encPasscode, encPasscodeLen, passcode, sizeof(passcode), passcodeLen, NULL);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    NL_TEST_ASSERT(inSuite, passcodeLen == sizeof(sWeavePasscode));
    NL_TEST_ASSERT(inSuite, memcmp(passcode, sWeavePasscode, passcodeLen) == 0);

    // Use HKDF function manually to encrypt passcode.
    encPasscodeManual[0] = config;
    LittleEndian::Put32(encPasscodeManual + 1, kPasscodeConfig1_KeyId);
    LittleEndian::Put32(encPasscodeManual + 5, sPasscodeEncryptionKeyNonce);

    // -- copy unencrypted passcode.
    memcpy(encPasscodeManual + 9, sWeavePaddedPasscode, kPasscodePaddedLen);

    // -- generate passcode authenticator.
    nl::Weave::Platform::Security::SHA1 hash;
    uint8_t digest[SHA1::kHashLength];
    hash.Begin();
    hash.AddData(encPasscodeManual, sizeof(uint8_t));
    hash.AddData(encPasscodeManual + 5, sizeof(uint32_t) + kPasscodePaddedLen);
    hash.Finish(digest);
    memcpy(encPasscodeManual + 9 + kPasscodePaddedLen, digest, kPasscodeAuthenticatorLen);

    // -- generate passcode fingerprint.
    hash.Begin();
    hash.AddData(sWeavePaddedPasscode, kPasscodePaddedLen);
    hash.Finish(digest);
    memcpy(encPasscodeManual + 9 + kPasscodePaddedLen + kPasscodeAuthenticatorLen, digest, kPasscodeFingerprintLen);

    // -- compare the result.
    NL_TEST_ASSERT(inSuite, memcmp(encPasscode, encPasscodeManual, encPasscodeLen) == 0);

#if DEBUG_PRINT_ENABLE
    printf("Manually generated passcode:\n");
    DumpMemoryCStyle(encPasscodeManual, encPasscodeLen, "    ", 16);
#endif
}

void PasscodeEncryptConfig2_Test1(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err;
    const uint8_t config = kPasscode_Config2;
    TestGroupKeyStore keyStore;
    uint8_t passcode[kPasscodePaddedLen];
    size_t passcodeLen;
    uint8_t encPasscode[kPasscodeMaxEncryptedLen];
    uint8_t encPasscodeManual[kPasscodeMaxEncryptedLen];
    size_t encPasscodeLen;

    // Encrypt passcode.
    err = EncryptPasscode(config, sPasscodeEncRotatingKeyId_CRK_E0_G4, sPasscodeEncryptionKeyNonce,
                          sWeavePasscode, sizeof(sWeavePasscode),
                          encPasscode, sizeof(encPasscode), encPasscodeLen, &keyStore);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    NL_TEST_ASSERT(inSuite, encPasscodeLen == sizeof(sWeaveEncryptedPasscodeConfig2_Rotating));
    NL_TEST_ASSERT(inSuite, memcmp(encPasscode, sWeaveEncryptedPasscodeConfig2_Rotating, encPasscodeLen) == 0);

#if DEBUG_PRINT_ENABLE
    printf("Encrypted passcode:\n");
    DumpMemoryCStyle(encPasscode, encPasscodeLen, "    ", 16);
#endif

    // Decrypt passcode.
    err = DecryptPasscode(encPasscode, encPasscodeLen, passcode, sizeof(passcode), passcodeLen, &keyStore);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    NL_TEST_ASSERT(inSuite, passcodeLen == sizeof(sWeavePasscode));
    NL_TEST_ASSERT(inSuite, memcmp(passcode, sWeavePasscode, passcodeLen) == 0);

    // Use HKDF function manually to encrypt passcode.
    encPasscodeManual[0] = config;
    LittleEndian::Put32(encPasscodeManual + 1, sPasscodeEncRotatingKeyId_CRK_E0_G4);
    LittleEndian::Put32(encPasscodeManual + 5, sPasscodeEncryptionKeyNonce);

    // -- encrypt passcode.
    AES128BlockCipherEnc aes128Enc;
    aes128Enc.SetKey(sPasscodeEncRotatingKey_CRK_E0_G4);
    aes128Enc.EncryptBlock(sWeavePaddedPasscode, encPasscodeManual + 9);

    // -- generate passcode authenticator.
    HMACSHA1 hmac;
    uint8_t digest[SHA1::kHashLength];
    hmac.Begin(sPasscodeEncRotatingKey_CRK_E0_G4 + kPasscodeEncryptionKeyLen, kPasscodeAuthenticationKeyLen);
    hmac.AddData(encPasscodeManual, sizeof(uint8_t));
    hmac.AddData(encPasscodeManual + 5, sizeof(uint32_t) + kPasscodePaddedLen);
    hmac.Finish(digest);
    memcpy(encPasscodeManual + 9 + kPasscodePaddedLen, digest, kPasscodeAuthenticatorLen);

    // -- generate passcode fingerprint.
    hmac.Begin(sPasscodeFingerprintKey_CRK_G4, kPasscodeFingerprintKeyLen);
    hmac.AddData(sWeavePaddedPasscode, kPasscodePaddedLen);
    hmac.Finish(digest);
    memcpy(encPasscodeManual + 9 + kPasscodePaddedLen + kPasscodeAuthenticatorLen, digest, kPasscodeFingerprintLen);

    // -- compare the result.
    NL_TEST_ASSERT(inSuite, memcmp(encPasscode, encPasscodeManual, encPasscodeLen) == 0);

#if DEBUG_PRINT_ENABLE
    printf("Manually generated passcode:\n");
    DumpMemoryCStyle(encPasscodeManual, encPasscodeLen, "    ", 16);
#endif
}

void PasscodeEncryptConfig2_Test2(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err;
    const uint8_t config = kPasscode_Config2;
    TestGroupKeyStore keyStore;
    uint8_t passcode[kPasscodePaddedLen];
    size_t passcodeLen;
    uint8_t encPasscode[kPasscodeMaxEncryptedLen];
    uint8_t encPasscodeManual[kPasscodeMaxEncryptedLen];
    size_t encPasscodeLen;

    // Encrypt passcode.
    err = EncryptPasscode(config, sPasscodeEncStaticKeyId_CRK_G4, sPasscodeEncryptionKeyNonce,
                          sWeavePasscode, sizeof(sWeavePasscode),
                          encPasscode, sizeof(encPasscode), encPasscodeLen, &keyStore);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    NL_TEST_ASSERT(inSuite, encPasscodeLen == sizeof(sWeaveEncryptedPasscodeConfig2_Static));
    NL_TEST_ASSERT(inSuite, memcmp(encPasscode, sWeaveEncryptedPasscodeConfig2_Static, encPasscodeLen) == 0);

#if DEBUG_PRINT_ENABLE
    printf("Encrypted passcode:\n");
    DumpMemoryCStyle(encPasscode, encPasscodeLen, "    ", 16);
#endif

    // Decrypt passcode.
    err = DecryptPasscode(encPasscode, encPasscodeLen, passcode, sizeof(passcode), passcodeLen, &keyStore);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    NL_TEST_ASSERT(inSuite, passcodeLen == sizeof(sWeavePasscode));
    NL_TEST_ASSERT(inSuite, memcmp(passcode, sWeavePasscode, passcodeLen) == 0);

    // Use HKDF function manually to encrypt passcode.
    encPasscodeManual[0] = config;
    LittleEndian::Put32(encPasscodeManual + 1, sPasscodeEncStaticKeyId_CRK_G4);
    LittleEndian::Put32(encPasscodeManual + 5, sPasscodeEncryptionKeyNonce);

    // -- encrypt passcode.
    AES128BlockCipherEnc aes128Enc;
    aes128Enc.SetKey(sPasscodeEncStaticKey_CRK_G4);
    aes128Enc.EncryptBlock(sWeavePaddedPasscode, encPasscodeManual + 9);

    // -- generate passcode authenticator.
    HMACSHA1 hmac;
    uint8_t digest[SHA1::kHashLength];
    hmac.Begin(sPasscodeEncStaticKey_CRK_G4 + kPasscodeEncryptionKeyLen, kPasscodeAuthenticationKeyLen);
    hmac.AddData(encPasscodeManual, sizeof(uint8_t));
    hmac.AddData(encPasscodeManual + 5, sizeof(uint32_t) + kPasscodePaddedLen);
    hmac.Finish(digest);
    memcpy(encPasscodeManual + 9 + kPasscodePaddedLen, digest, kPasscodeAuthenticatorLen);

    // -- generate passcode fingerprint.
    hmac.Begin(sPasscodeFingerprintKey_CRK_G4, kPasscodeFingerprintKeyLen);
    hmac.AddData(sWeavePaddedPasscode, kPasscodePaddedLen);
    hmac.Finish(digest);
    memcpy(encPasscodeManual + 9 + kPasscodePaddedLen + kPasscodeAuthenticatorLen, digest, kPasscodeFingerprintLen);

    // -- compare the result.
    NL_TEST_ASSERT(inSuite, memcmp(encPasscode, encPasscodeManual, encPasscodeLen) == 0);

#if DEBUG_PRINT_ENABLE
    printf("Manually generated passcode:\n");
    DumpMemoryCStyle(encPasscodeManual, encPasscodeLen, "    ", 16);
#endif
}

void PasscodeEncryptConfig2_LongPasscodeTest(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err;
    const uint8_t config = kPasscode_Config2;
    TestGroupKeyStore keyStore;
    uint8_t passcode[kPasscodePaddedLen];
    size_t passcodeLen;
    uint8_t encPasscode[kPasscodeMaxEncryptedLen];
    size_t encPasscodeLen;

    // Encrypt passcode.
    err = EncryptPasscode(config, sPasscodeEncStaticKeyId_CRK_G4, sPasscodeEncryptionKeyNonce,
                          sLongWeavePasscode, sizeof(sLongWeavePasscode),
                          encPasscode, sizeof(encPasscode), encPasscodeLen, &keyStore);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    // Decrypt passcode.
    err = DecryptPasscode(encPasscode, encPasscodeLen, passcode, sizeof(passcode), passcodeLen, &keyStore);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    NL_TEST_ASSERT(inSuite, passcodeLen == sizeof(sLongWeavePasscode));
    NL_TEST_ASSERT(inSuite, memcmp(passcode, sLongWeavePasscode, passcodeLen) == 0);
}

void EncryptedPasscodeUtils_Test1(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err;
    uint8_t config;
    uint32_t keyId, nonce;
    uint8_t fingerprint[kPasscodeFingerprintLen];
    size_t fingerprintLen;

    err = GetEncryptedPasscodeConfig(sWeaveEncryptedPasscodeConfig1, sizeof(sWeaveEncryptedPasscodeConfig1), config);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    NL_TEST_ASSERT(inSuite, config == kPasscode_Config1_TEST_ONLY);

    err = GetEncryptedPasscodeConfig(sWeaveEncryptedPasscodeConfig2_Static, sizeof(sWeaveEncryptedPasscodeConfig2_Static), config);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    NL_TEST_ASSERT(inSuite, config == kPasscode_Config2);

    err = GetEncryptedPasscodeKeyId(sWeaveEncryptedPasscodeConfig2_Rotating, sizeof(sWeaveEncryptedPasscodeConfig2_Rotating), keyId);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    NL_TEST_ASSERT(inSuite, keyId == sPasscodeEncRotatingKeyId_CRK_E0_G4);

    err = GetEncryptedPasscodeNonce(sWeaveEncryptedPasscodeConfig2_Rotating, sizeof(sWeaveEncryptedPasscodeConfig2_Rotating), nonce);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    NL_TEST_ASSERT(inSuite, nonce == sPasscodeEncryptionKeyNonce);

    err = GetEncryptedPasscodeFingerprint(sWeaveEncryptedPasscodeConfig2_Rotating, sizeof(sWeaveEncryptedPasscodeConfig2_Rotating), fingerprint, sizeof(fingerprint), fingerprintLen);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    NL_TEST_ASSERT(inSuite, fingerprintLen == kPasscodeFingerprintLen);
    const uint8_t *expectedFingerprint = sWeaveEncryptedPasscodeConfig2_Rotating + sizeof(sWeaveEncryptedPasscodeConfig2_Rotating) - kPasscodeFingerprintLen;
    NL_TEST_ASSERT(inSuite, memcmp(fingerprint, expectedFingerprint, fingerprintLen) == 0);

#if WEAVE_CONFIG_SUPPORT_PASSCODE_CONFIG1_TEST_ONLY
    NL_TEST_ASSERT(inSuite, true == IsSupportedPasscodeEncryptionConfig(kPasscode_Config1_TEST_ONLY));
#endif
    NL_TEST_ASSERT(inSuite, true == IsSupportedPasscodeEncryptionConfig(kPasscode_Config2));
    NL_TEST_ASSERT(inSuite, false == IsSupportedPasscodeEncryptionConfig(0xFF));
}


int main(int argc, char *argv[])
{
    static const nlTest tests[] = {
#if WEAVE_CONFIG_SUPPORT_PASSCODE_CONFIG1_TEST_ONLY
        NL_TEST_DEF("PasscodeEncryptConfig1_Test1",             PasscodeEncryptConfig1_Test1),
#endif
#if WEAVE_CONFIG_SUPPORT_PASSCODE_CONFIG2
        NL_TEST_DEF("PasscodeEncryptConfig2_Test1",             PasscodeEncryptConfig2_Test1),
        NL_TEST_DEF("PasscodeEncryptConfig2_Test2",             PasscodeEncryptConfig2_Test2),
        NL_TEST_DEF("PasscodeEncryptConfig2_LongPasscodeTest",  PasscodeEncryptConfig2_LongPasscodeTest),
#endif
        NL_TEST_DEF("EncryptedPasscodeUtils_Test1",             EncryptedPasscodeUtils_Test1),
        NL_TEST_SENTINEL()
    };

    static nlTestSuite testSuite = {
        "passcode-encryption",
        &tests[0]
    };

    nl_test_set_output_style(OUTPUT_CSV);

    nlTestRunner(&testSuite, NULL);

    return nlTestRunnerStats(&testSuite);
}
