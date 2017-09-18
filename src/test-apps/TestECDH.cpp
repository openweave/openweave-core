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
 *      This file implements a process to effect a functional test for
 *      the Weave Elliptic Curve (EC) Diffie-Hellman (DH) ephemeral-
 *      and static-key key agreement protocol interfaces.
 *
 */

#include <Weave/Core/WeaveConfig.h>
#include <stdio.h>
#include "ToolCommon.h"
#include <Weave/Support/crypto/EllipticCurve.h>
#include <Weave/Support/ASN1.h>

#ifndef VERIFY_USING_OPENSSL_API
#define VERIFY_USING_OPENSSL_API WEAVE_WITH_OPENSSL
#endif

#if VERIFY_USING_OPENSSL_API
#include <openssl/objects.h>
#include <openssl/ecdh.h>
#endif

using namespace nl::Weave::ASN1;
using namespace nl::Weave::Crypto;
using namespace nl::Weave::Profiles::Security;

using nl::Weave::Platform::Security::SHA1;

#define VerifyOrFail(TST, MSG) \
do { \
    if (!(TST)) \
    { \
        fprintf(stderr, "%s FAILED: ", __FUNCTION__); \
        fprintf(stderr, MSG); \
        exit(-1); \
    } \
} while (0)

static int sECTestKey_CurveOID = kOID_EllipticCurve_secp224r1;

static uint8_t sECTestKey1_PubKey[] =
{
    0x04, 0x48, 0xc3, 0xf6, 0x29, 0x73, 0x3a, 0x8e, 0xc5, 0x54, 0xa8, 0x2a, 0xee, 0xbd, 0xc3, 0x1b,
    0x4f, 0xb8, 0x28, 0xcf, 0x54, 0x14, 0x77, 0xca, 0xb9, 0x15, 0x4e, 0xdc, 0xae, 0x84, 0xc4, 0x24,
    0x8c, 0x9a, 0xbe, 0xb2, 0x93, 0x48, 0xba, 0x35, 0xb8, 0x43, 0x71, 0x60, 0x82, 0x28, 0x20, 0x84,
    0xfa, 0x23, 0xe1, 0x71, 0xa5, 0x52, 0x2c, 0xec, 0x99
};

static uint8_t sECTestKey1_PrivKey[] =
{
    0x69, 0x31, 0xc5, 0xad, 0xcb, 0xff, 0xb2, 0x55, 0x1c, 0xa2, 0xbf, 0x7c, 0xa7, 0x9f, 0xd3, 0xba,
    0x03, 0x2c, 0x1a, 0xea, 0x10, 0xf9, 0x36, 0xc4, 0xaf, 0xcc, 0x15, 0x7b
};

static uint8_t sECTestKey2_PubKey[] =
{
    0x04, 0x46, 0xff, 0x8b, 0x71, 0xea, 0x26, 0xc0, 0x22, 0x2e, 0x05, 0x83, 0xca, 0xf1, 0xe6, 0x21,
    0xa9, 0x09, 0xc7, 0x54, 0x20, 0x91, 0x66, 0x50, 0xe2, 0x6c, 0xa6, 0xe7, 0x9d, 0xfc, 0x2c, 0x3c,
    0x17, 0xda, 0x32, 0x09, 0x02, 0x83, 0x1a, 0xf7, 0xeb, 0xf1, 0xe4, 0x97, 0xb8, 0x33, 0x87, 0x42,
    0x78, 0xe4, 0x7b, 0xb3, 0xb2, 0x3a, 0xa8, 0x85, 0x88
};

static uint8_t sECTestKey2_PrivKey[] =
{
    0x00, 0xc6, 0x87, 0xf8, 0x40, 0xaf, 0xef, 0xcf, 0x03, 0xdb, 0x49, 0x3c, 0x08, 0x08, 0x68, 0x8e,
    0xfa, 0x3b, 0xe1, 0x20, 0xde, 0x57, 0xdc, 0x3f, 0xa1, 0x76, 0x0f, 0x6e, 0xa4
};

static uint8_t sExpectedFixedSharedSecret[] =
{
    0x6C, 0x97, 0xF7, 0xD8, 0xB3, 0xC9, 0xD8, 0x9F, 0x33, 0xB4, 0x66, 0x50, 0xCB, 0xC4, 0x83, 0x58,
    0xAD, 0x2A, 0x45, 0x88, 0xE0, 0x36, 0xCC, 0x63, 0x4A, 0x1B, 0xF9, 0xD3
};


#if VERIFY_USING_OPENSSL_API

void ComputeSharedSecretUsingOpenSSL(const EC_GROUP *ecGroup, const uint8_t *encodedPubKey, size_t encodedPubKeyLen, const uint8_t *encodedPrivKey, size_t encodedPrivKeyLen,
        uint8_t *sharedSecretBuf, uint32_t sharedSecretBufSize, uint16_t& sharedSecretLen)
{
    EC_POINT *pubKeyPoint = NULL;
    EC_KEY *privKey = NULL;
    BIGNUM *privKeyBN = NULL;
    int res;

    pubKeyPoint = EC_POINT_new(ecGroup);
    VerifyOrFail(pubKeyPoint != NULL, "EC_POINT_new() failed\n");

    res = EC_POINT_oct2point(ecGroup, pubKeyPoint, encodedPubKey, encodedPubKeyLen, NULL);
    VerifyOrFail(res != 0, "EC_POINT_oct2point() failed\n");

    privKeyBN = BN_bin2bn(encodedPrivKey, encodedPrivKeyLen, NULL);
    VerifyOrFail(privKeyBN != NULL, "BN_bin2bn() failed\n");

    privKey = EC_KEY_new();
    VerifyOrFail(privKey != NULL, "EC_KEY_new() failed\n");

    res = EC_KEY_set_group(privKey, ecGroup);
    VerifyOrFail(res != 0, "EC_KEY_set_group() failed\n");

    res = EC_KEY_set_private_key(privKey, privKeyBN);
    VerifyOrFail(res != 0, "EC_KEY_set_private_key() failed\n");

    sharedSecretLen = ECDH_compute_key(sharedSecretBuf, sharedSecretBufSize, pubKeyPoint, privKey, NULL);
    VerifyOrFail(sharedSecretLen > 0, "ECDH_compute_key() failed\n");
}
#endif /* VERIFY_USING_OPENSSL_API */


void ECDHTest_TestEphemeralKeys()
{
    WEAVE_ERROR err;
    uint8_t PubKey1Buf[65];
    uint8_t PubKey2Buf[65];
    uint8_t PrivKey1Buf[33];
    uint8_t PrivKey2Buf[33];
    EncodedECPublicKey encodedPubKey1;
    EncodedECPublicKey encodedPubKey2;
    EncodedECPrivateKey encodedPrivKey1;
    EncodedECPrivateKey encodedPrivKey2;
    uint8_t sharedSecret1[128];
    uint16_t sharedSecret1Len;
    uint8_t sharedSecret2[128];
    uint16_t sharedSecret2Len;

    encodedPubKey1.ECPoint = PubKey1Buf;
    encodedPubKey1.ECPointLen = sizeof(PubKey1Buf);
    encodedPubKey2.ECPoint = PubKey2Buf;
    encodedPubKey2.ECPointLen = sizeof(PubKey2Buf);
    encodedPrivKey1.PrivKey = PrivKey1Buf;
    encodedPrivKey1.PrivKeyLen = sizeof(PrivKey1Buf);
    encodedPrivKey2.PrivKey = PrivKey2Buf;
    encodedPrivKey2.PrivKeyLen = sizeof(PrivKey2Buf);

    err = GenerateECDHKey(sECTestKey_CurveOID, encodedPubKey1, encodedPrivKey1);
    VerifyOrFail(err == WEAVE_NO_ERROR, "GenerateECDHKey() failed\n");

    err = GenerateECDHKey(sECTestKey_CurveOID, encodedPubKey2, encodedPrivKey2);
    VerifyOrFail(err == WEAVE_NO_ERROR, "GenerateECDHKey() failed\n");

    // Compute shared secret from public key 1 and private key 2
    err = ECDHComputeSharedSecret(sECTestKey_CurveOID, encodedPubKey1, encodedPrivKey2, sharedSecret1, sizeof(sharedSecret1), sharedSecret1Len);
    VerifyOrFail(err == WEAVE_NO_ERROR, "ECDHComputeSharedSecret() failed\n");

#if VERIFY_USING_OPENSSL_API
    {
        EC_GROUP *ecGroup = NULL;
        uint8_t opensslSharedSecret[128];
        uint16_t opensslSharedSecretLen;

        err = GetECGroupForCurve(sECTestKey_CurveOID, ecGroup);
        VerifyOrFail(err == WEAVE_NO_ERROR, "GetECGroupForCurve() failed\n");

        ComputeSharedSecretUsingOpenSSL(ecGroup, encodedPubKey1.ECPoint, encodedPubKey1.ECPointLen,
                                                 encodedPrivKey2.PrivKey, encodedPrivKey2.PrivKeyLen,
                                                 opensslSharedSecret, sizeof(opensslSharedSecret), opensslSharedSecretLen);

        VerifyOrFail(opensslSharedSecretLen == sharedSecret1Len, "Shared secret length returned by ECDHComputeSharedSecret does not match OpenSSL\n");
        VerifyOrFail(memcmp(opensslSharedSecret, sharedSecret1, sharedSecret1Len) == 0, "Shared secret returned by ECDHComputeSharedSecret does not match OpenSSL\n");
    }
#endif

    // Compute shared secret from public key 2 and private key 1
    err = ECDHComputeSharedSecret(sECTestKey_CurveOID, encodedPubKey2, encodedPrivKey1, sharedSecret2, sizeof(sharedSecret2), sharedSecret2Len);
    VerifyOrFail(err == WEAVE_NO_ERROR, "ECDHComputeSharedSecret() failed\n");

#if VERIFY_USING_OPENSSL_API
    {
        EC_GROUP *ecGroup = NULL;
        uint8_t opensslSharedSecret[128];
        uint16_t opensslSharedSecretLen;

        err = GetECGroupForCurve(sECTestKey_CurveOID, ecGroup);
        VerifyOrFail(err == WEAVE_NO_ERROR, "GetECGroupForCurve() failed\n");

        ComputeSharedSecretUsingOpenSSL(ecGroup, encodedPubKey2.ECPoint, encodedPubKey2.ECPointLen,
                                                 encodedPrivKey1.PrivKey, encodedPrivKey1.PrivKeyLen,
                                                 opensslSharedSecret, sizeof(opensslSharedSecret), opensslSharedSecretLen);

        VerifyOrFail(opensslSharedSecretLen == sharedSecret2Len, "Shared secret length returned by ECDHComputeSharedSecret does not match OpenSSL\n");
        VerifyOrFail(memcmp(opensslSharedSecret, sharedSecret2, sharedSecret2Len) == 0, "Shared secret returned by ECDHComputeSharedSecret does not match OpenSSL\n");
    }
#endif

    VerifyOrFail(sharedSecret1Len == sharedSecret2Len, "ECDHComputeSharedSecret returned invalid shared secret length\n");
    VerifyOrFail(memcmp(sharedSecret1, sharedSecret2, sharedSecret1Len) == 0, "ECDHComputeSharedSecret returned invalid shared secret\n");

    printf("TestEphemeralKeys complete\n");
}


void ECDHTest_TestFixedKeys()
{
    WEAVE_ERROR err;
    EncodedECPublicKey encodedPubKey1;
    EncodedECPublicKey encodedPubKey2;
    EncodedECPrivateKey encodedPrivKey1;
    EncodedECPrivateKey encodedPrivKey2;
    uint8_t sharedSecret[128];
    uint16_t sharedSecretLen;

    encodedPubKey1.ECPoint = sECTestKey1_PubKey;
    encodedPubKey1.ECPointLen = sizeof(sECTestKey1_PubKey);
    encodedPubKey2.ECPoint = sECTestKey2_PubKey;
    encodedPubKey2.ECPointLen = sizeof(sECTestKey2_PubKey);
    encodedPrivKey1.PrivKey = sECTestKey1_PrivKey;
    encodedPrivKey1.PrivKeyLen = sizeof(sECTestKey1_PrivKey);
    encodedPrivKey2.PrivKey = sECTestKey2_PrivKey;
    encodedPrivKey2.PrivKeyLen = sizeof(sECTestKey2_PrivKey);

    // Compute shared secret from public key 1 and private key 2
    err = ECDHComputeSharedSecret(sECTestKey_CurveOID, encodedPubKey1, encodedPrivKey2, sharedSecret, sizeof(sharedSecret), sharedSecretLen);
    VerifyOrFail(err == WEAVE_NO_ERROR, "ECDHComputeSharedSecret() failed\n");

    VerifyOrFail(sharedSecretLen == sizeof(sExpectedFixedSharedSecret), "ECDHComputeSharedSecret returned invalid shared secret length\n");
    VerifyOrFail(memcmp(sharedSecret, sExpectedFixedSharedSecret, sharedSecretLen) == 0, "ECDHComputeSharedSecret returned invalid shared secret\n");

    // Compute shared secret from public key 2 and private key 1
    err = ECDHComputeSharedSecret(sECTestKey_CurveOID, encodedPubKey2, encodedPrivKey1, sharedSecret, sizeof(sharedSecret), sharedSecretLen);
    VerifyOrFail(err == WEAVE_NO_ERROR, "ECDHComputeSharedSecret() failed\n");

    VerifyOrFail(sharedSecretLen == sizeof(sExpectedFixedSharedSecret), "ECDHComputeSharedSecret returned invalid shared secret length\n");
    VerifyOrFail(memcmp(sharedSecret, sExpectedFixedSharedSecret, sharedSecretLen) == 0, "ECDHComputeSharedSecret returned invalid shared secret\n");

#if VERIFY_USING_OPENSSL_API
    {
        EC_GROUP *ecGroup = NULL;

        err = GetECGroupForCurve(sECTestKey_CurveOID, ecGroup);
        VerifyOrFail(err == WEAVE_NO_ERROR, "GetECGroupForCurve() failed\n");

        ComputeSharedSecretUsingOpenSSL(ecGroup, sECTestKey1_PubKey, sizeof(sECTestKey1_PubKey),
                                                 sECTestKey2_PrivKey, sizeof(sECTestKey2_PrivKey),
                                                 sharedSecret, sizeof(sharedSecret), sharedSecretLen);

        VerifyOrFail(sharedSecretLen == sizeof(sExpectedFixedSharedSecret), "ECDHComputeSharedSecret returned invalid shared secret length\n");
        VerifyOrFail(memcmp(sharedSecret, sExpectedFixedSharedSecret, sharedSecretLen) == 0, "ECDHComputeSharedSecret returned invalid shared secret\n");

#if WEAVE_CONFIG_DEBUG_TEST_ECDH
        DumpMemory(sharedSecret, sharedSecretLen, "  ", 16);
        printf("\n");
#endif // WEAVE_CONFIG_DEBUG_TEST_ECDH
    }
#endif // VERIFY_USING_OPENSSL_API

    printf("TestFixedKeys complete\n");
}


int main(int argc, char *argv[])
{
    WEAVE_ERROR err;

    err = nl::Weave::Platform::Security::InitSecureRandomDataSource(NULL, 64, NULL, 0);
    FAIL_ERROR(err, "InitSecureRandomDataSource() failed");

    ECDHTest_TestFixedKeys();
    ECDHTest_TestEphemeralKeys();
    printf("All tests succeeded\n");
}
