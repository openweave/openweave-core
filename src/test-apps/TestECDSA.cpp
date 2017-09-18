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
 *      the Weave Elliptic Curve (EC) Digital Signature Algorithm
 *      (DSA) signing and verification interfaces.
 *
 */

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#include <stdio.h>
#include <stdint.h>

#include "ToolCommon.h"
#include <Weave/Profiles/security/WeaveSig.h>
#include <Weave/Support/crypto/HashAlgos.h>
#include <Weave/Support/crypto/HKDF.h>
#include <Weave/Support/crypto/EllipticCurve.h>
#include <Weave/Support/ASN1.h>

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

static OID sECTestKey_CurveOID = kOID_EllipticCurve_secp224r1;

static uint8_t sECTestKey1_PubKey[] =
{
    0x04, 0x64, 0xd0, 0xa1, 0x65, 0x1f, 0x1e, 0x2f, 0x22, 0xcc, 0xf1, 0xc3, 0xb8, 0x5d, 0x36, 0xdd,
    0x99, 0x48, 0x3f, 0x6b, 0x56, 0x4f, 0x84, 0x83, 0x98, 0xb6, 0xa3, 0x49, 0x21, 0x3d, 0xbb, 0x51,
    0x6b, 0xe6, 0xe4, 0x10, 0xde, 0x7a, 0x91, 0xcc, 0xf7, 0x03, 0x03, 0xe5, 0x5f, 0xb6, 0x72, 0x51,
    0xa3, 0xcc, 0xb2, 0x96, 0xb5, 0x5d, 0xda, 0x74, 0x26
};

static uint8_t sECTestKey1_PrivKey[] =
{
    0x00, 0xfd, 0x95, 0xee, 0xe4, 0xc5, 0x53, 0xd4, 0xcf, 0xb1, 0x7e, 0x61, 0x84, 0x12, 0x03, 0x6a,
    0x45, 0x43, 0x42, 0xb9, 0x90, 0xef, 0x74, 0x6c, 0x9d, 0x23, 0x8e, 0x78, 0x56
};

static uint8_t sECTestKey2_PubKey[] =
{
    0x04, 0xE8, 0x4F, 0xB0, 0xB8, 0xE7, 0x00, 0x0C, 0xB6, 0x57, 0xD7, 0x97, 0x3C, 0xF6, 0xB4, 0x2E,
    0xD7, 0x8B, 0x30, 0x16, 0x74, 0x27, 0x6D, 0xF7, 0x44, 0xAF, 0x13, 0x0B, 0x3E, 0x43, 0x76, 0x67,
    0x5C, 0x6F, 0xC5, 0x61, 0x2C, 0x21, 0xA0, 0xFF, 0x2D, 0x2A, 0x89, 0xD2, 0x98, 0x7D, 0xF7, 0xA2,
    0xBC, 0x52, 0x18, 0x3B, 0x59, 0x82, 0x29, 0x85, 0x55
};

static uint8_t sECTestKey2_PrivKey[] =
{
    0x3F, 0x0C, 0x48, 0x8E, 0x98, 0x7C, 0x80, 0xBE, 0x0F, 0xEE, 0x52, 0x1F, 0x8D, 0x90, 0xBE, 0x60,
    0x34, 0xEC, 0x69, 0xAE, 0x11, 0xCA, 0x72, 0xAA, 0x77, 0x74, 0x81, 0xE8
};

// static uint8_t sECTestKey2_RandK[] =
// {
//     0xA5, 0x48, 0x80, 0x3B, 0x79, 0xDF, 0x17, 0xC4, 0x0C, 0xDE, 0x3F, 0xF0, 0xE3, 0x6D, 0x02, 0x51,
//     0x43, 0xBC, 0xBB, 0xA1, 0x46, 0xEC, 0x32, 0x90, 0x8E, 0xB8, 0x49, 0x37
// };

static uint8_t sECTestKey2_MsgHash[] =
{
    0x1F, 0x1E, 0x1C, 0xF8, 0x92, 0x92, 0x6C, 0xFC, 0xCF, 0xC5, 0xA2, 0x8F, 0xEE, 0xF3, 0xD8, 0x07,
    0xD2, 0x3F, 0x77, 0x80, 0x08, 0xDB, 0xA4, 0xB3, 0x5F, 0x04, 0xB2, 0xFD
};

static uint8_t sECTestKey2_SigR[] =
{
    0xC3, 0xA3, 0xF5, 0xB8, 0x27, 0x12, 0x53, 0x20, 0x04, 0xC6, 0xF6, 0xD1, 0xDB, 0x67, 0x2F, 0x55,
    0xD9, 0x31, 0xC3, 0x40, 0x9E, 0xA1, 0x21, 0x6D, 0x0B, 0xE7, 0x73, 0x80
};

static uint8_t sECTestKey2_SigS[] =
{
    0xC5, 0xAA, 0x1E, 0xAE, 0x60, 0x95, 0xDE, 0xA3, 0x4C, 0x9B, 0xD8, 0x4D, 0xA3, 0x85, 0x2C, 0xCA,
    0x41, 0xA8, 0xBD, 0x9D, 0x55, 0x48, 0xF3, 0x6D, 0xAB, 0xDF, 0x66, 0x17
};

void ECDSATest_SignTest()
{
    WEAVE_ERROR err;
    EncodedECPrivateKey encodedPrivKey;
    EncodedECDSASignature encodedSig;
    uint8_t sigBuf[UINT8_MAX * 2];
    nl::Weave::Platform::Security::SHA1 sha1;
    uint8_t hashBuf[SHA1::kHashLength];
    const char *msg = "This is a test";

    sha1.Begin();
    sha1.AddData((const uint8_t *)msg, strlen(msg));
    sha1.Finish(hashBuf);

    printf("hash = ");
    for (size_t i = 0; i < sizeof(hashBuf); i++)
        printf("%02x", hashBuf[i]);
    printf("\n");

    encodedPrivKey.PrivKey = sECTestKey1_PrivKey;
    encodedPrivKey.PrivKeyLen = sizeof(sECTestKey1_PrivKey);

    encodedSig.R = sigBuf;
    encodedSig.RLen = UINT8_MAX;
    encodedSig.S = sigBuf + UINT8_MAX;
    encodedSig.SLen = UINT8_MAX;

    err = GenerateECDSASignature(sECTestKey_CurveOID,
                                 hashBuf, sizeof(hashBuf),
                                 encodedPrivKey,
                                 encodedSig);
    VerifyOrFail(err == WEAVE_NO_ERROR, "GenerateECDSASignature() failed\n");

    printf("r = ");
    for (uint8_t i = 0; i < encodedSig.RLen; i++)
        printf("%02x", encodedSig.R[i]);
    printf("\n");
    printf("s = ");
    for (uint8_t i = 0; i < encodedSig.SLen; i++)
        printf("%02x", encodedSig.S[i]);
    printf("\n");

    printf("SignTest complete\n");
}

void ECDSATest_VerifyTest()
{
    WEAVE_ERROR err;
    EncodedECPublicKey encodedPubKey;
    EncodedECDSASignature encodedSig;
    nl::Weave::Platform::Security::SHA1 sha1;
    uint8_t hashBuf[SHA1::kHashLength];
    const char *msg = "This is a test";

    static uint8_t sTestSig_r[] =
    {
        0x3c, 0xd0, 0x43, 0xe3, 0xfa, 0xa0, 0x94, 0xe8, 0xdc, 0xd5, 0xc5, 0xdc, 0x71, 0x51, 0x1d, 0x80,
        0x74, 0x4c, 0x1b, 0xd0, 0x28, 0xe4, 0xe2, 0x95, 0xc4, 0x1a, 0x89, 0xc0
    };

    static uint8_t sTestSig_s[] =
    {
        0x15, 0x0a, 0xf4, 0xcd, 0xa0, 0x29, 0xe1, 0x84, 0x0b, 0xf6, 0x7d, 0xbe, 0xf7, 0xb4, 0xae, 0xd9,
        0xa4, 0x1b, 0x10, 0x31, 0x2a, 0x69, 0x62, 0x40, 0x55, 0xed, 0x0d, 0xae
    };

    sha1.Begin();
    sha1.AddData((const uint8_t *)msg, strlen(msg));
    sha1.Finish(hashBuf);

    encodedPubKey.ECPoint = sECTestKey1_PubKey;
    encodedPubKey.ECPointLen = sizeof(sECTestKey1_PubKey);

    encodedSig.R = sTestSig_r;
    encodedSig.RLen = sizeof(sTestSig_r);
    encodedSig.S = sTestSig_s;
    encodedSig.SLen = sizeof(sTestSig_s);

    err = VerifyECDSASignature(sECTestKey_CurveOID, hashBuf, sizeof(hashBuf), encodedSig, encodedPubKey);
    VerifyOrFail(err == WEAVE_NO_ERROR, "VerifyECDSASignature() failed\n");

    printf("VerifyTest complete\n");
}

void ECDSATest_FixedLenSignVerifyTest()
{
    WEAVE_ERROR err;
    EncodedECPublicKey encodedPubKey;
    EncodedECPrivateKey encodedPrivKey;
    uint8_t signature[UINT8_MAX];

    encodedPubKey.ECPoint = sECTestKey2_PubKey;
    encodedPubKey.ECPointLen = sizeof(sECTestKey2_PubKey);

    encodedPrivKey.PrivKey = sECTestKey2_PrivKey;
    encodedPrivKey.PrivKeyLen = sizeof(sECTestKey2_PrivKey);

    err = GenerateECDSASignature(sECTestKey_CurveOID,
                                 sECTestKey2_MsgHash, sizeof(sECTestKey2_MsgHash),
                                 encodedPrivKey, signature);
    VerifyOrFail(err == WEAVE_NO_ERROR, "GenerateECDSASignature() failed\n");

    err = VerifyECDSASignature(sECTestKey_CurveOID,
                               sECTestKey2_MsgHash, sizeof(sECTestKey2_MsgHash),
                               signature, encodedPubKey);
    VerifyOrFail(err == WEAVE_NO_ERROR, "VerifyECDSASignature() failed\n");

    printf("r = ");
    for (uint8_t i = 0; i < sizeof(sECTestKey2_SigR); i++)
        printf("%02x", signature[i]);
    printf("\n");
    printf("s = ");
    for (uint8_t i = 0; i < sizeof(sECTestKey2_SigS); i++)
        printf("%02x", signature[i + sizeof(sECTestKey2_SigR)]);
    printf("\n");

    printf("FixedLenSignVerifyTest complete\n");
}

void ECDSATest_FixedLenVerifyTest()
{
    WEAVE_ERROR err;
    EncodedECPublicKey encodedPubKey;
    uint8_t signature[UINT8_MAX];

    encodedPubKey.ECPoint = sECTestKey2_PubKey;
    encodedPubKey.ECPointLen = sizeof(sECTestKey2_PubKey);

    memcpy(signature, sECTestKey2_SigR, sizeof(sECTestKey2_SigR));
    memcpy(signature + sizeof(sECTestKey2_SigR), sECTestKey2_SigS, sizeof(sECTestKey2_SigS));

    err = VerifyECDSASignature(sECTestKey_CurveOID,
                               sECTestKey2_MsgHash, sizeof(sECTestKey2_MsgHash),
                               signature, encodedPubKey);
    VerifyOrFail(err == WEAVE_NO_ERROR, "VerifyECDSASignature() failed\n");

    printf("FixedLenVerifyTest complete\n");
}

int main(int argc, char *argv[])
{
    WEAVE_ERROR err;

    err = nl::Weave::Platform::Security::InitSecureRandomDataSource(NULL, 64, NULL, 0);
    FAIL_ERROR(err, "InitSecureRandomDataSource() failed");

    ECDSATest_SignTest();
    ECDSATest_VerifyTest();
    ECDSATest_FixedLenSignVerifyTest();
    ECDSATest_FixedLenVerifyTest();
    printf("All tests succeeded\n");
}
