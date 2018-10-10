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
 *      the Weave cryptographic signature signing and verification.
 *
 */

#include <stdio.h>

#include "ToolCommon.h"
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Profiles/security/WeaveSig.h>
#include <Weave/Profiles/security/WeaveSecurityDebug.h>
#include <Weave/Support/crypto/HashAlgos.h>
#include <Weave/Support/crypto/HKDF.h>
#include <Weave/Support/crypto/EllipticCurve.h>
#include <Weave/Support/ASN1.h>

using namespace nl::Weave::ASN1;
using namespace nl::Weave::Crypto;
using namespace nl::Weave::Profiles::Security;
using namespace nl::Weave::TLV;

#define VerifyOrFail(TST, MSG, ...) \
do { \
    if (!(TST)) \
    { \
        fprintf(stderr, "%s FAILED: ", __FUNCTION__); \
        fprintf(stderr, MSG, ## __VA_ARGS__); \
        exit(-1); \
    } \
} while (0)


static uint8_t sTestRootCert[] =
{
    0xd5, 0x00, 0x00, 0x04, 0x00, 0x01, 0x00, 0x30, 0x01, 0x08, 0x41, 0x62, 0x01, 0xcc, 0x3a, 0x91,
    0x9a, 0x83, 0x24, 0x02, 0x04, 0x57, 0x03, 0x00, 0x27, 0x13, 0x01, 0x00, 0x00, 0xee, 0xee, 0x30,
    0xb4, 0x18, 0x18, 0x26, 0x04, 0xf3, 0x85, 0x65, 0x1a, 0x26, 0x05, 0x73, 0x01, 0xe5, 0x53, 0x57,
    0x06, 0x00, 0x27, 0x13, 0x01, 0x00, 0x00, 0xee, 0xee, 0x30, 0xb4, 0x18, 0x18, 0x24, 0x07, 0x02,
    0x24, 0x08, 0x25, 0x30, 0x0a, 0x39, 0x04, 0x2e, 0x94, 0xd1, 0xc6, 0x49, 0xd9, 0xe4, 0x8b, 0xc4,
    0x6c, 0x8c, 0x8a, 0x6b, 0xaf, 0x0a, 0xbe, 0xc8, 0xca, 0xc5, 0xd1, 0x62, 0x49, 0x6f, 0x6a, 0x64,
    0xdf, 0xf6, 0xc7, 0xb6, 0x51, 0x14, 0x10, 0xcc, 0xff, 0x5c, 0x8e, 0x45, 0xbc, 0x19, 0x7f, 0x5e,
    0xec, 0x74, 0x77, 0xcb, 0x16, 0x3d, 0x25, 0xd7, 0xf0, 0xfe, 0x18, 0xbc, 0xa5, 0x59, 0x62, 0x35,
    0x83, 0x29, 0x01, 0x29, 0x02, 0x18, 0x35, 0x82, 0x29, 0x01, 0x24, 0x02, 0x60, 0x18, 0x35, 0x81,
    0x30, 0x02, 0x08, 0x4a, 0xaa, 0x7b, 0xa4, 0x7a, 0x61, 0x4b, 0x2d, 0x18, 0x35, 0x80, 0x30, 0x02,
    0x08, 0x4a, 0xaa, 0x7b, 0xa4, 0x7a, 0x61, 0x4b, 0x2d, 0x18, 0x35, 0x0c, 0x30, 0x01, 0x1c, 0x44,
    0x26, 0x46, 0xdf, 0xcd, 0xd5, 0xd1, 0x12, 0x8f, 0x85, 0x6d, 0x80, 0x28, 0x28, 0x63, 0x4c, 0xdb,
    0x57, 0xc1, 0x9b, 0x8f, 0xf5, 0x01, 0xf9, 0x23, 0xbf, 0x94, 0x8e, 0x30, 0x02, 0x1c, 0x71, 0xa8,
    0x3c, 0x47, 0xf6, 0xf4, 0x35, 0x75, 0x39, 0x06, 0xbc, 0x8b, 0x7e, 0x49, 0xba, 0xab, 0xf1, 0x3d,
    0x56, 0x03, 0x03, 0x1b, 0x17, 0x40, 0x65, 0x49, 0x8e, 0xa3, 0x18, 0x18
};

static uint8_t sTestRootPublicKey[] =
{
    0x04, 0x2e, 0x94, 0xd1, 0xc6, 0x49, 0xd9, 0xe4, 0x8b, 0xc4, 0x6c, 0x8c, 0x8a, 0x6b, 0xaf, 0x0a,
    0xbe, 0xc8, 0xca, 0xc5, 0xd1, 0x62, 0x49, 0x6f, 0x6a, 0x64, 0xdf, 0xf6, 0xc7, 0xb6, 0x51, 0x14,
    0x10, 0xcc, 0xff, 0x5c, 0x8e, 0x45, 0xbc, 0x19, 0x7f, 0x5e, 0xec, 0x74, 0x77, 0xcb, 0x16, 0x3d,
    0x25, 0xd7, 0xf0, 0xfe, 0x18, 0xbc, 0xa5, 0x59, 0x62
};

static uint8_t sTestRootPublicKeyId[] =
{
    0x4A, 0xAA, 0x7B, 0xA4, 0x7A, 0x61, 0x4B, 0x2D
};

static uint64_t sTestRootCAId = 0x18B430EEEE000001ULL;

static uint32_t kTestRootPublicKeyCurveId = kWeaveCurveId_secp224r1;

static uint8_t sTestIntermediateCert[] =
{
    0xd5, 0x00, 0x00, 0x04, 0x00, 0x01, 0x00, 0x30, 0x01, 0x08, 0x64, 0x56, 0xfa, 0x7b, 0xc6, 0x34,
    0x99, 0x14, 0x24, 0x02, 0x04, 0x57, 0x03, 0x00, 0x27, 0x13, 0x01, 0x00, 0x00, 0xee, 0xee, 0x30,
    0xb4, 0x18, 0x18, 0x26, 0x04, 0xe1, 0x88, 0x65, 0x1a, 0x26, 0x05, 0x61, 0x04, 0xe5, 0x53, 0x57,
    0x06, 0x00, 0x27, 0x13, 0x04, 0x00, 0x00, 0xee, 0xee, 0x30, 0xb4, 0x18, 0x18, 0x24, 0x07, 0x02,
    0x24, 0x08, 0x25, 0x30, 0x0a, 0x39, 0x04, 0x37, 0x6e, 0x80, 0xc6, 0x28, 0x1a, 0x00, 0x55, 0x27,
    0xc9, 0x9f, 0x50, 0x86, 0xab, 0x71, 0x7a, 0x99, 0x6c, 0xdd, 0xdb, 0x95, 0x42, 0xc2, 0x24, 0x37,
    0x7c, 0x76, 0x9b, 0x81, 0xa9, 0xf0, 0xae, 0x30, 0x4e, 0x10, 0x62, 0xe7, 0x58, 0x1c, 0x73, 0xd2,
    0x8e, 0x67, 0xac, 0x41, 0xb5, 0xe4, 0x3d, 0x19, 0x06, 0x50, 0x58, 0x87, 0x01, 0x55, 0xcc, 0x35,
    0x83, 0x29, 0x01, 0x29, 0x02, 0x18, 0x35, 0x82, 0x29, 0x01, 0x24, 0x02, 0x60, 0x18, 0x35, 0x81,
    0x30, 0x02, 0x08, 0x4c, 0x8e, 0x97, 0x19, 0x2e, 0xbc, 0xf8, 0xed, 0x18, 0x35, 0x80, 0x30, 0x02,
    0x08, 0x4a, 0xaa, 0x7b, 0xa4, 0x7a, 0x61, 0x4b, 0x2d, 0x18, 0x35, 0x0c, 0x30, 0x01, 0x1d, 0x00,
    0xab, 0x31, 0xc0, 0xc7, 0xe4, 0xe6, 0x16, 0xd6, 0x67, 0xb4, 0xd5, 0x77, 0xec, 0x67, 0x04, 0xc6,
    0xde, 0x28, 0x05, 0x4b, 0xf5, 0xc9, 0x2a, 0x54, 0xed, 0x7a, 0xdb, 0xc0, 0x30, 0x02, 0x1d, 0x00,
    0xf0, 0xb8, 0x30, 0x73, 0x00, 0xc0, 0xdd, 0xdf, 0x93, 0x45, 0xb5, 0xec, 0x4d, 0x1a, 0x78, 0x5a,
    0xed, 0xa2, 0xf1, 0x20, 0x72, 0xc2, 0x7c, 0x1a, 0xb7, 0xcd, 0x0c, 0x00, 0x18, 0x18
};

static uint8_t sTestSigningCert[] =
{
    0xd5, 0x00, 0x00, 0x04, 0x00, 0x01, 0x00, 0x30, 0x01, 0x08, 0x1a, 0x52, 0x42, 0x5c, 0xaa, 0xeb,
    0x8c, 0x54, 0x24, 0x02, 0x04, 0x57, 0x03, 0x00, 0x27, 0x13, 0x04, 0x00, 0x00, 0xee, 0xee, 0x30,
    0xb4, 0x18, 0x18, 0x26, 0x04, 0x08, 0xad, 0x65, 0x1a, 0x26, 0x05, 0x88, 0xa3, 0x18, 0x2d, 0x57,
    0x06, 0x00, 0x27, 0x14, 0x01, 0x00, 0x00, 0xee, 0x03, 0x30, 0xb4, 0x18, 0x18, 0x24, 0x07, 0x02,
    0x24, 0x08, 0x25, 0x30, 0x0a, 0x39, 0x04, 0xc8, 0x6c, 0x57, 0x99, 0x6f, 0xed, 0x75, 0x9c, 0x2a,
    0x40, 0x50, 0x43, 0x74, 0xae, 0xab, 0x57, 0x42, 0x6e, 0x59, 0x18, 0x4c, 0x33, 0x85, 0xb8, 0x90,
    0x4b, 0x5e, 0x35, 0xa5, 0x46, 0xfa, 0x96, 0x04, 0x7a, 0xd6, 0xe9, 0x4d, 0x59, 0x3f, 0xc5, 0x03,
    0x86, 0x47, 0xc1, 0x93, 0x88, 0x73, 0xb9, 0xcd, 0x4c, 0xc6, 0x06, 0xa3, 0x91, 0xa7, 0x19, 0x35,
    0x83, 0x29, 0x01, 0x18, 0x35, 0x82, 0x29, 0x01, 0x24, 0x02, 0x01, 0x18, 0x35, 0x84, 0x29, 0x01,
    0x36, 0x02, 0x04, 0x03, 0x18, 0x18, 0x35, 0x81, 0x30, 0x02, 0x08, 0x47, 0x26, 0xdd, 0x88, 0x9e,
    0xfb, 0xe8, 0xbf, 0x18, 0x35, 0x80, 0x30, 0x02, 0x08, 0x4c, 0x8e, 0x97, 0x19, 0x2e, 0xbc, 0xf8,
    0xed, 0x18, 0x35, 0x0c, 0x30, 0x01, 0x1c, 0x23, 0xed, 0x40, 0x10, 0x35, 0x91, 0x84, 0x7f, 0xaa,
    0x12, 0xe5, 0xbd, 0x9f, 0xfc, 0xf2, 0xf9, 0x02, 0x16, 0x8f, 0xda, 0x07, 0xac, 0x99, 0x4b, 0x83,
    0xba, 0x71, 0xe9, 0x30, 0x02, 0x1d, 0x00, 0xa9, 0xc8, 0xea, 0xaf, 0xbd, 0x4f, 0x1b, 0xf1, 0x28,
    0x0b, 0x4a, 0xe3, 0x4f, 0xc8, 0xca, 0xfa, 0xd1, 0x30, 0xd3, 0xb7, 0x0b, 0x27, 0xcf, 0xdd, 0xe7,
    0xdb, 0x33, 0xba, 0x18, 0x18
};

static uint8_t sTestSigningCertKey[] =
{
    0xd5, 0x00, 0x00, 0x04, 0x00, 0x02, 0x00, 0x24, 0x01, 0x25, 0x30, 0x02, 0x1c, 0x8c, 0x77, 0xf8,
    0x56, 0x63, 0xcb, 0x4d, 0x62, 0xfc, 0x08, 0xf0, 0xe4, 0xc8, 0xce, 0xc2, 0x28, 0x6f, 0xca, 0x54,
    0x03, 0xab, 0xfb, 0x22, 0x20, 0x42, 0x5d, 0xa0, 0x08, 0x30, 0x03, 0x39, 0x04, 0xc8, 0x6c, 0x57,
    0x99, 0x6f, 0xed, 0x75, 0x9c, 0x2a, 0x40, 0x50, 0x43, 0x74, 0xae, 0xab, 0x57, 0x42, 0x6e, 0x59,
    0x18, 0x4c, 0x33, 0x85, 0xb8, 0x90, 0x4b, 0x5e, 0x35, 0xa5, 0x46, 0xfa, 0x96, 0x04, 0x7a, 0xd6,
    0xe9, 0x4d, 0x59, 0x3f, 0xc5, 0x03, 0x86, 0x47, 0xc1, 0x93, 0x88, 0x73, 0xb9, 0xcd, 0x4c, 0xc6,
    0x06, 0xa3, 0x91, 0xa7, 0x19, 0x18
};

// NOTE: The following hash values were products with the commands shown below:
//
//     echo -n 'Nest Weave' | openssl sha1 -hex
//     echo -n 'Nest Weave' | openssl sha256 -hex
//
// However, because these tests don't actually test the hashing functions, the values don't really matter.
//

static uint8_t sTestMsgHash_SHA1[] =
{
    0x2e, 0x72, 0x13, 0x17, 0x01, 0xf4, 0x2f, 0x27, 0x72, 0x65, 0xc4, 0x73, 0x89, 0x2d, 0x35, 0x19,
    0xae, 0x6d, 0x1a, 0x79
};

static uint8_t sTestMsgHash_SHA256[] =
{
    0xb8, 0x38, 0x5d, 0xd3, 0x2f, 0x1e, 0x94, 0x9e, 0x18, 0x76, 0x9c, 0xf0, 0xfd, 0x2d, 0xa2, 0xe2,
    0xc6, 0x79, 0xd9, 0xae, 0x53, 0xcb, 0x49, 0x65, 0x9c, 0x22, 0x35, 0xf4, 0x2f, 0xd5, 0xac, 0x44
};

static uint8_t sTestWeaveSig[] =
{
    0xD5, 0x00, 0x00, 0x04, 0x00, 0x05, 0x00, 0x35, 0x01, 0x30, 0x01, 0x1D, 0x00, 0xC6, 0x8B, 0x71,
    0x90, 0x4F, 0x96, 0xD7, 0x1C, 0xED, 0xE7, 0xC9, 0x42, 0xB6, 0xD9, 0x50, 0x63, 0xE3, 0xD4, 0x3E,
    0xF7, 0x0D, 0x66, 0x9F, 0xFA, 0xC7, 0x0F, 0x29, 0xDD, 0x30, 0x02, 0x1D, 0x00, 0x8B, 0x38, 0x72,
    0x23, 0x4F, 0x29, 0xEE, 0xB9, 0x87, 0xBA, 0x87, 0x67, 0x2C, 0x9B, 0x4F, 0xFF, 0x47, 0xAC, 0xC1,
    0x4A, 0x54, 0xE5, 0x0B, 0xA5, 0x4C, 0x05, 0xE5, 0xD9, 0x18, 0x36, 0x04, 0x15, 0x30, 0x01, 0x08,
    0x1A, 0x52, 0x42, 0x5C, 0xAA, 0xEB, 0x8C, 0x54, 0x24, 0x02, 0x04, 0x57, 0x03, 0x00, 0x27, 0x13,
    0x04, 0x00, 0x00, 0xEE, 0xEE, 0x30, 0xB4, 0x18, 0x18, 0x26, 0x04, 0x08, 0xAD, 0x65, 0x1A, 0x26,
    0x05, 0x88, 0xA3, 0x18, 0x2D, 0x57, 0x06, 0x00, 0x27, 0x14, 0x01, 0x00, 0x00, 0xEE, 0x03, 0x30,
    0xB4, 0x18, 0x18, 0x24, 0x07, 0x02, 0x24, 0x08, 0x25, 0x30, 0x0A, 0x39, 0x04, 0xC8, 0x6C, 0x57,
    0x99, 0x6F, 0xED, 0x75, 0x9C, 0x2A, 0x40, 0x50, 0x43, 0x74, 0xAE, 0xAB, 0x57, 0x42, 0x6E, 0x59,
    0x18, 0x4C, 0x33, 0x85, 0xB8, 0x90, 0x4B, 0x5E, 0x35, 0xA5, 0x46, 0xFA, 0x96, 0x04, 0x7A, 0xD6,
    0xE9, 0x4D, 0x59, 0x3F, 0xC5, 0x03, 0x86, 0x47, 0xC1, 0x93, 0x88, 0x73, 0xB9, 0xCD, 0x4C, 0xC6,
    0x06, 0xA3, 0x91, 0xA7, 0x19, 0x35, 0x83, 0x29, 0x01, 0x18, 0x35, 0x82, 0x29, 0x01, 0x24, 0x02,
    0x01, 0x18, 0x35, 0x84, 0x29, 0x01, 0x36, 0x02, 0x04, 0x03, 0x18, 0x18, 0x35, 0x81, 0x30, 0x02,
    0x08, 0x47, 0x26, 0xDD, 0x88, 0x9E, 0xFB, 0xE8, 0xBF, 0x18, 0x35, 0x80, 0x30, 0x02, 0x08, 0x4C,
    0x8E, 0x97, 0x19, 0x2E, 0xBC, 0xF8, 0xED, 0x18, 0x35, 0x0C, 0x30, 0x01, 0x1C, 0x23, 0xED, 0x40,
    0x10, 0x35, 0x91, 0x84, 0x7F, 0xAA, 0x12, 0xE5, 0xBD, 0x9F, 0xFC, 0xF2, 0xF9, 0x02, 0x16, 0x8F,
    0xDA, 0x07, 0xAC, 0x99, 0x4B, 0x83, 0xBA, 0x71, 0xE9, 0x30, 0x02, 0x1D, 0x00, 0xA9, 0xC8, 0xEA,
    0xAF, 0xBD, 0x4F, 0x1B, 0xF1, 0x28, 0x0B, 0x4A, 0xE3, 0x4F, 0xC8, 0xCA, 0xFA, 0xD1, 0x30, 0xD3,
    0xB7, 0x0B, 0x27, 0xCF, 0xDD, 0xE7, 0xDB, 0x33, 0xBA, 0x18, 0x18, 0x15, 0x30, 0x01, 0x08, 0x64,
    0x56, 0xFA, 0x7B, 0xC6, 0x34, 0x99, 0x14, 0x24, 0x02, 0x04, 0x57, 0x03, 0x00, 0x27, 0x13, 0x01,
    0x00, 0x00, 0xEE, 0xEE, 0x30, 0xB4, 0x18, 0x18, 0x26, 0x04, 0xE1, 0x88, 0x65, 0x1A, 0x26, 0x05,
    0x61, 0x04, 0xE5, 0x53, 0x57, 0x06, 0x00, 0x27, 0x13, 0x04, 0x00, 0x00, 0xEE, 0xEE, 0x30, 0xB4,
    0x18, 0x18, 0x24, 0x07, 0x02, 0x24, 0x08, 0x25, 0x30, 0x0A, 0x39, 0x04, 0x37, 0x6E, 0x80, 0xC6,
    0x28, 0x1A, 0x00, 0x55, 0x27, 0xC9, 0x9F, 0x50, 0x86, 0xAB, 0x71, 0x7A, 0x99, 0x6C, 0xDD, 0xDB,
    0x95, 0x42, 0xC2, 0x24, 0x37, 0x7C, 0x76, 0x9B, 0x81, 0xA9, 0xF0, 0xAE, 0x30, 0x4E, 0x10, 0x62,
    0xE7, 0x58, 0x1C, 0x73, 0xD2, 0x8E, 0x67, 0xAC, 0x41, 0xB5, 0xE4, 0x3D, 0x19, 0x06, 0x50, 0x58,
    0x87, 0x01, 0x55, 0xCC, 0x35, 0x83, 0x29, 0x01, 0x29, 0x02, 0x18, 0x35, 0x82, 0x29, 0x01, 0x24,
    0x02, 0x60, 0x18, 0x35, 0x81, 0x30, 0x02, 0x08, 0x4C, 0x8E, 0x97, 0x19, 0x2E, 0xBC, 0xF8, 0xED,
    0x18, 0x35, 0x80, 0x30, 0x02, 0x08, 0x4A, 0xAA, 0x7B, 0xA4, 0x7A, 0x61, 0x4B, 0x2D, 0x18, 0x35,
    0x0C, 0x30, 0x01, 0x1D, 0x00, 0xAB, 0x31, 0xC0, 0xC7, 0xE4, 0xE6, 0x16, 0xD6, 0x67, 0xB4, 0xD5,
    0x77, 0xEC, 0x67, 0x04, 0xC6, 0xDE, 0x28, 0x05, 0x4B, 0xF5, 0xC9, 0x2A, 0x54, 0xED, 0x7A, 0xDB,
    0xC0, 0x30, 0x02, 0x1D, 0x00, 0xF0, 0xB8, 0x30, 0x73, 0x00, 0xC0, 0xDD, 0xDF, 0x93, 0x45, 0xB5,
    0xEC, 0x4D, 0x1A, 0x78, 0x5A, 0xED, 0xA2, 0xF1, 0x20, 0x72, 0xC2, 0x7C, 0x1A, 0xB7, 0xCD, 0x0C,
    0x00, 0x18, 0x18, 0x18, 0x18,
};

static uint8_t sTestWeaveSig_CertRef_ECDSAWithSHA256[] =
{
    0xD5, 0x00, 0x00, 0x04, 0x00, 0x05, 0x00, 0x25, 0x05, 0x05, 0x02, 0x35, 0x01, 0x30, 0x01, 0x1C,
    0x4D, 0x0F, 0xC7, 0x61, 0x00, 0x34, 0xBF, 0x6D, 0x0F, 0xD1, 0xB8, 0x2B, 0xCD, 0x8C, 0x79, 0x25,
    0x07, 0x8A, 0x1A, 0x2A, 0x8B, 0xD9, 0xE1, 0xA8, 0x9C, 0x5A, 0xD0, 0x9C, 0x30, 0x02, 0x1C, 0x2D,
    0x81, 0xE4, 0xD9, 0x4E, 0x76, 0x69, 0x89, 0x7F, 0xFE, 0x79, 0x9E, 0xF5, 0x52, 0x14, 0x61, 0x9E,
    0x32, 0x9F, 0x46, 0x9F, 0xFC, 0x6C, 0x7F, 0xA2, 0xA5, 0x86, 0x4C, 0x18, 0x35, 0x03, 0x30, 0x02,
    0x08, 0x47, 0x26, 0xDD, 0x88, 0x9E, 0xFB, 0xE8, 0xBF, 0x18, 0x18,
};


static void LoadRootCert(WeaveCertificateSet& certSet)
{
    WEAVE_ERROR err;
    WeaveCertificateData *cert;

    // Load the root cert and mark it trusted.
    err = certSet.LoadCert(sTestRootCert, sizeof(sTestRootCert), 0, cert);
    VerifyOrFail(err == WEAVE_NO_ERROR, "WeaveSignature::LoadCert() failed: %s\n", nl::ErrorStr(err));
    cert->CertFlags |= kCertFlag_IsTrusted;
}

static void LoadIntermediateCert(WeaveCertificateSet& certSet, uint16_t decodeFlags = 0)
{
    WEAVE_ERROR err;
    WeaveCertificateData *cert;

    // Load the intermediate cert.
    err = certSet.LoadCert(sTestIntermediateCert, sizeof(sTestIntermediateCert), decodeFlags, cert);
    VerifyOrFail(err == WEAVE_NO_ERROR, "WeaveSignature::LoadCert() failed: %s\n", nl::ErrorStr(err));
}

static void LoadSigningCert(WeaveCertificateSet& certSet, uint16_t decodeFlags = 0)
{
    WEAVE_ERROR err;
    WeaveCertificateData *cert;

    // Load the signing cert.
    err = certSet.LoadCert(sTestSigningCert, sizeof(sTestSigningCert), decodeFlags, cert);
    VerifyOrFail(err == WEAVE_NO_ERROR, "WeaveSignature::LoadCert() failed: %s\n", nl::ErrorStr(err));
}

static void LoadAllCerts(WeaveCertificateSet& certSet, uint16_t decodeFlags = 0)
{
    LoadRootCert(certSet);
    LoadIntermediateCert(certSet, decodeFlags);
    LoadSigningCert(certSet, decodeFlags);
}

static void LoadRootKey(WeaveCertificateSet& certSet)
{
    WEAVE_ERROR err;
    EncodedECPublicKey rootPubKey;

    // Add the trusted root key to the certificate set.
    rootPubKey.ECPoint = sTestRootPublicKey;
    rootPubKey.ECPointLen = sizeof(sTestRootPublicKey);
    err = certSet.AddTrustedKey(sTestRootCAId, kTestRootPublicKeyCurveId, rootPubKey, sTestRootPublicKeyId, sizeof(sTestRootPublicKeyId));
    VerifyOrFail(err == WEAVE_NO_ERROR, "WeaveSignature::AddTrustedRootKey() failed: %s\n", nl::ErrorStr(err));
}

static void InitValidationContext(ValidationContext& validContext)
{
    WEAVE_ERROR err;
    ASN1UniversalTime validTime;

    // Arrange to validate the signature for code signing purposes.
    memset(&validContext, 0, sizeof(validContext));
    validContext.RequiredKeyUsages = kKeyUsageFlag_DigitalSignature;
    validContext.RequiredKeyPurposes = kKeyPurposeFlag_CodeSigning;

    // Set the effective validation time.
    validTime.Year = 2013;
    validTime.Month = 10;
    validTime.Day = 20;
    validTime.Hour = validTime.Minute = validTime.Second = 0;
    err = PackCertTime(validTime, validContext.EffectiveTime);
    VerifyOrFail(err == WEAVE_NO_ERROR, "PackCertTime() failed: %s\n", nl::ErrorStr(err));
}

void WeaveSignatureTest_SignTest()
{
    WEAVE_ERROR err;
    enum { kMaxCerts = 4 };
    WeaveCertificateSet certSet;
    ValidationContext validContext;
    uint8_t sigBuf[1024];
    uint16_t sigLen;

    err = certSet.Init(kMaxCerts, 640);
    VerifyOrFail(err == WEAVE_NO_ERROR, "WeaveCertificateSet::InitCert() failed: %s\n", nl::ErrorStr(err));

    LoadAllCerts(certSet);

    {
        WeaveSignatureGenerator sigGen(certSet, sTestSigningCertKey, sizeof(sTestSigningCertKey));
        sigGen.SigAlgoOID = kOID_SigAlgo_ECDSAWithSHA1;
        err = sigGen.GenerateSignature(sTestMsgHash_SHA1, sizeof(sTestMsgHash_SHA1), sigBuf, sizeof(sigBuf), sigLen);
        VerifyOrFail(err == WEAVE_NO_ERROR, "WeaveSignatureGenerator.GenerateSignature() failed: %s\n", nl::ErrorStr(err));
    }

    // DumpMemory(sigBuf, sigLen, "  ", 16);

    // Start over.
    certSet.Clear();

    // Load the root cert and mark it trusted.
    LoadRootCert(certSet);

    // Initialize the validation context.
    InitValidationContext(validContext);

    // Verify the generated signature.
    err = VerifyWeaveSignature(sTestMsgHash_SHA1, sizeof(sTestMsgHash_SHA1),
                               sigBuf, sigLen,
                               certSet,
                               validContext);
    VerifyOrFail(err == WEAVE_NO_ERROR, "VerifyWeaveSignature() failed: %s\n", nl::ErrorStr(err));

    certSet.Release();

    printf("SignTest complete\n");
}

void WeaveSignatureTest_SignTest_CertRef()
{
    WEAVE_ERROR err;
    enum { kMaxCerts = 4 };
    WeaveCertificateSet certSet;
    ValidationContext validContext;
    uint8_t sigBuf[1024];
    uint16_t sigLen;

    err = certSet.Init(kMaxCerts, 640);
    VerifyOrFail(err == WEAVE_NO_ERROR, "WeaveCertificateSet::InitCert() failed: %s\n", nl::ErrorStr(err));

    // Load just the signing certificate.
    LoadSigningCert(certSet);

    {
        WeaveSignatureGenerator sigGen(certSet, sTestSigningCertKey, sizeof(sTestSigningCertKey));
        sigGen.SigAlgoOID = kOID_SigAlgo_ECDSAWithSHA1;
        sigGen.Flags = kGenerateWeaveSignatureFlag_IncludeSigningCertKeyId;
        err = sigGen.GenerateSignature(sTestMsgHash_SHA1, sizeof(sTestMsgHash_SHA1), sigBuf, sizeof(sigBuf), sigLen);
        VerifyOrFail(err == WEAVE_NO_ERROR, "WeaveSignatureGenerator.GenerateSignature() failed: %s\n", nl::ErrorStr(err));
    }

    // DumpMemory(sigBuf, sigLen, "  ", 16);

    // Start over.
    certSet.Clear();

    // Load all certificates.
    LoadAllCerts(certSet, kDecodeFlag_GenerateTBSHash);

    // Initialize the validation context.
    InitValidationContext(validContext);

    // Verify the generated signature.
    err = VerifyWeaveSignature(sTestMsgHash_SHA1, sizeof(sTestMsgHash_SHA1),
                               sigBuf, sigLen,
                               certSet,
                               validContext);
    VerifyOrFail(err == WEAVE_NO_ERROR, "VerifyWeaveSignature() failed: %s\n", nl::ErrorStr(err));

    certSet.Release();

    printf("SignTest_CertRef complete\n");
}

void WeaveSignatureTest_SignTest_ECDSAWithSHA256()
{
    WEAVE_ERROR err;
    enum { kMaxCerts = 4 };
    WeaveCertificateSet certSet;
    ValidationContext validContext;
    uint8_t sigBuf[1024];
    uint16_t sigLen;

    err = certSet.Init(kMaxCerts, 640);
    VerifyOrFail(err == WEAVE_NO_ERROR, "WeaveCertificateSet::InitCert() failed: %s\n", nl::ErrorStr(err));

    // Load just the signing certificate.
    LoadSigningCert(certSet);

    {
        WeaveSignatureGenerator sigGen(certSet, sTestSigningCertKey, sizeof(sTestSigningCertKey));
        sigGen.SigAlgoOID = kOID_SigAlgo_ECDSAWithSHA256;
        sigGen.Flags = kGenerateWeaveSignatureFlag_IncludeSigningCertKeyId;
        err = sigGen.GenerateSignature(sTestMsgHash_SHA256, sizeof(sTestMsgHash_SHA256), sigBuf, sizeof(sigBuf), sigLen);
        VerifyOrFail(err == WEAVE_NO_ERROR, "WeaveSignatureGenerator.GenerateSignature() failed: %s\n", nl::ErrorStr(err));
    }

    // DumpMemory(sigBuf, sigLen, "  ", 16);

    // Start over.
    certSet.Clear();

    // Load all certificates.
    LoadAllCerts(certSet, kDecodeFlag_GenerateTBSHash);

    // Initialize the validation context.
    InitValidationContext(validContext);

    // Verify the generated signature.
    err = VerifyWeaveSignature(sTestMsgHash_SHA256, sizeof(sTestMsgHash_SHA256),
                               sigBuf, sigLen, kOID_SigAlgo_ECDSAWithSHA256,
                               certSet,
                               validContext);
    VerifyOrFail(err == WEAVE_NO_ERROR, "VerifyWeaveSignature() failed: %s\n", nl::ErrorStr(err));

    certSet.Release();

    printf("SignTest_ECDSAWithSHA256 complete\n");
}

void WeaveSignatureTest_VerifyTest()
{
    WEAVE_ERROR err;
    enum { kMaxCerts = 4 };
    WeaveCertificateSet certSet;
    ValidationContext validContext;

    err = certSet.Init(kMaxCerts, 640);
    VerifyOrFail(err == WEAVE_NO_ERROR, "WeaveCertificateSet::InitCert() failed: %s\n", nl::ErrorStr(err));

    // Add the trusted root key to the certificate set.
    LoadRootKey(certSet);

    // Initialize the validation context.
    InitValidationContext(validContext);

    err = VerifyWeaveSignature(sTestMsgHash_SHA1, sizeof(sTestMsgHash_SHA1),
                               sTestWeaveSig, sizeof(sTestWeaveSig),
                               certSet,
                               validContext);
    if (err != WEAVE_NO_ERROR)
    {
        PrintCertValidationResults(stdout, certSet, validContext, 2);
        VerifyOrFail(err == WEAVE_NO_ERROR, "VerifyWeaveSignature() failed: %s\n", nl::ErrorStr(err));
    }

    certSet.Release();

    printf("VerifyTest complete\n");
}

void WeaveSignatureTest_VerifyTest_CertRef_ECDSAWithSHA256()
{
    WEAVE_ERROR err;
    enum { kMaxCerts = 4 };
    WeaveCertificateSet certSet;
    ValidationContext validContext;

    err = certSet.Init(kMaxCerts, 640);
    VerifyOrFail(err == WEAVE_NO_ERROR, "WeaveCertificateSet::InitCert() failed: %s\n", nl::ErrorStr(err));

    // Load all certificates.
    LoadAllCerts(certSet, kDecodeFlag_GenerateTBSHash);

    // Initialize the validation context.
    InitValidationContext(validContext);

    err = VerifyWeaveSignature(sTestMsgHash_SHA256, sizeof(sTestMsgHash_SHA256),
                               sTestWeaveSig_CertRef_ECDSAWithSHA256, sizeof(sTestWeaveSig_CertRef_ECDSAWithSHA256),
                               kOID_SigAlgo_ECDSAWithSHA256,
                               certSet,
                               validContext);
    if (err != WEAVE_NO_ERROR)
    {
        PrintCertValidationResults(stdout, certSet, validContext, 2);
        VerifyOrFail(err == WEAVE_NO_ERROR, "VerifyWeaveSignature() failed: %s\n", nl::ErrorStr(err));
    }

    certSet.Release();

    printf("VerifyTest_CertRef_ECDSAWithSHA256 complete\n");
}

void WeaveSignatureTest_FailureTest_NoCerts()
{
    WEAVE_ERROR err;
    enum { kMaxCerts = 4 };
    WeaveCertificateSet certSet;
    ValidationContext validContext;

    err = certSet.Init(kMaxCerts, 640);
    VerifyOrFail(err == WEAVE_NO_ERROR, "WeaveCertificateSet::InitCert() failed: %s\n", nl::ErrorStr(err));

    // Load no certificates.

    // Initialize the validation context.
    InitValidationContext(validContext);

    err = VerifyWeaveSignature(sTestMsgHash_SHA256, sizeof(sTestMsgHash_SHA256),
                               sTestWeaveSig_CertRef_ECDSAWithSHA256, sizeof(sTestWeaveSig_CertRef_ECDSAWithSHA256),
                               kOID_SigAlgo_ECDSAWithSHA256,
                               certSet,
                               validContext);
    VerifyOrFail(err == WEAVE_ERROR_CERT_NOT_FOUND, "VerifyWeaveSignature() did not returned expected error: %s\n", nl::ErrorStr(err));

    certSet.Release();

    printf("FailureTest_NoCerts complete\n");
}

void WeaveSignatureTest_FailureTest_NoSigningCert()
{
    WEAVE_ERROR err;
    enum { kMaxCerts = 4 };
    WeaveCertificateSet certSet;
    ValidationContext validContext;

    err = certSet.Init(kMaxCerts, 640);
    VerifyOrFail(err == WEAVE_NO_ERROR, "WeaveCertificateSet::InitCert() failed: %s\n", nl::ErrorStr(err));

    // Load root and intermediate cert.
    LoadRootCert(certSet);
    LoadIntermediateCert(certSet, kDecodeFlag_GenerateTBSHash);

    // Initialize the validation context.
    InitValidationContext(validContext);

    err = VerifyWeaveSignature(sTestMsgHash_SHA256, sizeof(sTestMsgHash_SHA256),
                               sTestWeaveSig_CertRef_ECDSAWithSHA256, sizeof(sTestWeaveSig_CertRef_ECDSAWithSHA256),
                               kOID_SigAlgo_ECDSAWithSHA256,
                               certSet,
                               validContext);
    VerifyOrFail(err == WEAVE_ERROR_CERT_NOT_FOUND, "VerifyWeaveSignature() did not returned expected error: %s\n", nl::ErrorStr(err));

    certSet.Release();

    printf("FailureTest_NoSigningCert complete\n");
}

void WeaveSignatureTest_FailureTest_NoIntermediateCert()
{
    WEAVE_ERROR err;
    enum { kMaxCerts = 4 };
    WeaveCertificateSet certSet;
    ValidationContext validContext;

    err = certSet.Init(kMaxCerts, 640);
    VerifyOrFail(err == WEAVE_NO_ERROR, "WeaveCertificateSet::InitCert() failed: %s\n", nl::ErrorStr(err));

    // Load root and signing cert.
    LoadRootCert(certSet);
    LoadSigningCert(certSet, kDecodeFlag_GenerateTBSHash);

    // Initialize the validation context.
    InitValidationContext(validContext);

    err = VerifyWeaveSignature(sTestMsgHash_SHA256, sizeof(sTestMsgHash_SHA256),
                               sTestWeaveSig_CertRef_ECDSAWithSHA256, sizeof(sTestWeaveSig_CertRef_ECDSAWithSHA256),
                               kOID_SigAlgo_ECDSAWithSHA256,
                               certSet,
                               validContext);
    VerifyOrFail(err == WEAVE_ERROR_CA_CERT_NOT_FOUND, "VerifyWeaveSignature() did not returned expected error: %s\n", nl::ErrorStr(err));

    certSet.Release();

    printf("FailureTest_NoIntermediateCert complete\n");
}

void WeaveSignatureTest_FailureTest_BadHashLength()
{
    WEAVE_ERROR err;
    enum { kMaxCerts = 4 };
    WeaveCertificateSet certSet;
    ValidationContext validContext;

    static const uint16_t hashLens[] = {
        0,
        sizeof(sTestMsgHash_SHA1) / 2,
        sizeof(sTestMsgHash_SHA1) - 1,
        sizeof(sTestMsgHash_SHA1) + 1,
        sizeof(sTestMsgHash_SHA1) * 2
    };

    for (size_t i = 0; i < (sizeof(hashLens) / sizeof(hashLens[0])); i++)
    {
        uint16_t hashLen = hashLens[i];

        err = certSet.Init(kMaxCerts, 640);
        VerifyOrFail(err == WEAVE_NO_ERROR, "WeaveCertificateSet::InitCert() failed: %s\n", nl::ErrorStr(err));

        // Add the trusted root key to the certificate set.
        LoadRootKey(certSet);

        // Initialize the validation context.
        InitValidationContext(validContext);

        err = VerifyWeaveSignature(sTestMsgHash_SHA1, hashLen,
                                   sTestWeaveSig, sizeof(sTestWeaveSig),
                                   kOID_SigAlgo_ECDSAWithSHA1,
                                   certSet,
                                   validContext);
        VerifyOrFail(err == WEAVE_ERROR_INVALID_ARGUMENT, "VerifyWeaveSignature() did not returned expected error: %s\n", nl::ErrorStr(err));

        certSet.Release();
    }

    printf("FailureTest_BadHashLength complete\n");
}

void WeaveSignatureTest_FailureTest_BadHash_ECDSAWithSHA1()
{
    WEAVE_ERROR err;
    enum { kMaxCerts = 4 };
    WeaveCertificateSet certSet;
    ValidationContext validContext;

    err = certSet.Init(kMaxCerts, 640);
    VerifyOrFail(err == WEAVE_NO_ERROR, "WeaveCertificateSet::InitCert() failed: %s\n", nl::ErrorStr(err));

    // Add the trusted root key to the certificate set.
    LoadRootKey(certSet);

    // Initialize the validation context.
    InitValidationContext(validContext);

    // Muck with the input message hash.
    sTestMsgHash_SHA1[6] ^= 0x40;

    err = VerifyWeaveSignature(sTestMsgHash_SHA1, sizeof(sTestMsgHash_SHA1),
                               sTestWeaveSig, sizeof(sTestWeaveSig),
                               kOID_SigAlgo_ECDSAWithSHA1,
                               certSet,
                               validContext);
    VerifyOrFail(err == WEAVE_ERROR_INVALID_SIGNATURE, "VerifyWeaveSignature() did not returned expected error: %s\n", nl::ErrorStr(err));

    certSet.Release();

    // Restore message hash.
    sTestMsgHash_SHA1[6] ^= 0x40;

    printf("FailureTest_BadHash_ECDSAWithSHA1 complete\n");
}

void WeaveSignatureTest_FailureTest_BadHash_ECDSAWithSHA256()
{
    WEAVE_ERROR err;
    enum { kMaxCerts = 4 };
    WeaveCertificateSet certSet;
    ValidationContext validContext;

    err = certSet.Init(kMaxCerts, 640);
    VerifyOrFail(err == WEAVE_NO_ERROR, "WeaveCertificateSet::InitCert() failed: %s\n", nl::ErrorStr(err));

    // Load all certificates.
    LoadAllCerts(certSet, kDecodeFlag_GenerateTBSHash);

    // Initialize the validation context.
    InitValidationContext(validContext);

    // Muck with the input message hash.
    sTestMsgHash_SHA256[19] ^= 0x40;

    err = VerifyWeaveSignature(sTestMsgHash_SHA256, sizeof(sTestMsgHash_SHA256),
                               sTestWeaveSig_CertRef_ECDSAWithSHA256, sizeof(sTestWeaveSig_CertRef_ECDSAWithSHA256),
                               kOID_SigAlgo_ECDSAWithSHA256,
                               certSet,
                               validContext);
    VerifyOrFail(err == WEAVE_ERROR_INVALID_SIGNATURE, "VerifyWeaveSignature() did not returned expected error: %s\n", nl::ErrorStr(err));

    certSet.Release();

    // Restore message hash.
    sTestMsgHash_SHA256[19] ^= 0x40;

    printf("FailureTest_BadHash_ECDSAWithSHA256 complete\n");
}

void WeaveSignatureTest_InsertRelatedCertsTest_SingleCert()
{
    WEAVE_ERROR err;
    enum { kMaxCerts = 4 };
    WeaveCertificateSet certSet;
    ValidationContext validContext;
    uint8_t sigBuf[1024];
    uint16_t sigLen;

    err = certSet.Init(kMaxCerts, 640);
    VerifyOrFail(err == WEAVE_NO_ERROR, "WeaveCertificateSet::InitCert() failed: %s\n", nl::ErrorStr(err));

    // Load just the signing certificate.
    LoadSigningCert(certSet);

    {
        WeaveSignatureGenerator sigGen(certSet, sTestSigningCertKey, sizeof(sTestSigningCertKey));
        sigGen.SigAlgoOID = kOID_SigAlgo_ECDSAWithSHA1;
        sigGen.Flags = kGenerateWeaveSignatureFlag_IncludeSigningCertKeyId;
        err = sigGen.GenerateSignature(sTestMsgHash_SHA1, sizeof(sTestMsgHash_SHA1), sigBuf, sizeof(sigBuf), sigLen);
        VerifyOrFail(err == WEAVE_NO_ERROR, "WeaveSignatureGenerator.GenerateSignature() failed: %s\n", nl::ErrorStr(err));
    }

    // DumpMemory(sigBuf, sigLen, "  ", 16);

    // Start over.
    certSet.Clear();

    // Insert the signing certificate into the weave certificate.
    err = InsertRelatedCertificatesIntoWeaveSignature(sigBuf, sigLen, sizeof(sigBuf),
                                                      sTestSigningCert, sizeof(sTestSigningCert),
                                                      sigLen);
    VerifyOrFail(err == WEAVE_NO_ERROR, "InsertRelatedCertificatesIntoWeaveSignature() failed: %s\n", nl::ErrorStr(err));

    // Load the root and intermediate certificates.
    LoadRootCert(certSet);
    LoadIntermediateCert(certSet, kDecodeFlag_GenerateTBSHash);

    // Initialize the validation context.
    InitValidationContext(validContext);

    // Verify the updated signature.
    err = VerifyWeaveSignature(sTestMsgHash_SHA1, sizeof(sTestMsgHash_SHA1),
                               sigBuf, sigLen, kOID_SigAlgo_ECDSAWithSHA1,
                               certSet,
                               validContext);
    VerifyOrFail(err == WEAVE_NO_ERROR, "VerifyWeaveSignature() failed: %s\n", nl::ErrorStr(err));

    certSet.Release();

    printf("InsertRelatedCertsTest_SingleCert complete\n");
}

void WeaveSignatureTest_InsertRelatedCertsTest_MultipleCerts()
{
    WEAVE_ERROR err;
    enum { kMaxCerts = 4 };
    WeaveCertificateSet certSet;
    ValidationContext validContext;
    uint8_t sigBuf[1024];
    uint16_t sigLen;
    uint8_t certListBuf[1024];
    uint16_t certListLen;

    err = certSet.Init(kMaxCerts, 640);
    VerifyOrFail(err == WEAVE_NO_ERROR, "WeaveCertificateSet::InitCert() failed: %s\n", nl::ErrorStr(err));

    // Load just the signing certificate.
    LoadSigningCert(certSet);

    {
        WeaveSignatureGenerator sigGen(certSet, sTestSigningCertKey, sizeof(sTestSigningCertKey));
        sigGen.SigAlgoOID = kOID_SigAlgo_ECDSAWithSHA1;
        sigGen.Flags = kGenerateWeaveSignatureFlag_IncludeSigningCertKeyId;
        err = sigGen.GenerateSignature(sTestMsgHash_SHA1, sizeof(sTestMsgHash_SHA1), sigBuf, sizeof(sigBuf), sigLen);
        VerifyOrFail(err == WEAVE_NO_ERROR, "WeaveSignatureGenerator.GenerateSignature() failed: %s\n", nl::ErrorStr(err));
    }

    // DumpMemory(sigBuf, sigLen, "  ", 16);

    // Clear the cert set.
    certSet.Clear();

    // Load the intermediate certificate and the signing certificate.
    LoadIntermediateCert(certSet);
    LoadSigningCert(certSet);

    // Create a TLV-encoded array containing the intermediate and signing certificates.
    {
        TLVWriter certListWriter;
        TLVType outerContainerType;
        certListWriter.Init(certListBuf, sizeof(certListBuf));
        certListWriter.StartContainer(AnonymousTag, kTLVType_Array, outerContainerType);
        certSet.SaveCerts(certListWriter, NULL, false);
        certListWriter.EndContainer(outerContainerType);
        certListWriter.Finalize();
        certListLen = certListWriter.GetLengthWritten();
    }

    // Insert the certificate list into the weave certificate.
    err = InsertRelatedCertificatesIntoWeaveSignature(sigBuf, sigLen, sizeof(sigBuf),
                                                      certListBuf, certListLen,
                                                      sigLen);
    VerifyOrFail(err == WEAVE_NO_ERROR, "InsertRelatedCertificatesIntoWeaveSignature() failed: %s\n", nl::ErrorStr(err));

    // Clear the cert set.
    certSet.Clear();

    // Load the root key.
    LoadRootKey(certSet);

    // Initialize the validation context.
    InitValidationContext(validContext);

    // Verify the updated signature.
    err = VerifyWeaveSignature(sTestMsgHash_SHA1, sizeof(sTestMsgHash_SHA1),
                               sigBuf, sigLen, kOID_SigAlgo_ECDSAWithSHA1,
                               certSet,
                               validContext);
    VerifyOrFail(err == WEAVE_NO_ERROR, "VerifyWeaveSignature() failed: %s\n", nl::ErrorStr(err));

    certSet.Release();

    printf("InsertRelatedCertsTest_MultipleCerts complete\n");
}

void WeaveSignatureTest_GetWeaveSignatureAlgoTest()
{
    WEAVE_ERROR err;
    OID sigAlgoOID;

    err = GetWeaveSignatureAlgo(sTestWeaveSig, sizeof(sTestWeaveSig), sigAlgoOID);
    VerifyOrFail(err == WEAVE_NO_ERROR, "GetWeaveSignatureAlgo() failed: %s\n", nl::ErrorStr(err));
    VerifyOrFail(sigAlgoOID == kOID_SigAlgo_ECDSAWithSHA1, "GetWeaveSignatureAlgo() returned unexpected signature algorithm");

    err = GetWeaveSignatureAlgo(sTestWeaveSig_CertRef_ECDSAWithSHA256, sizeof(sTestWeaveSig_CertRef_ECDSAWithSHA256), sigAlgoOID);
    VerifyOrFail(err == WEAVE_NO_ERROR, "GetWeaveSignatureAlgo() failed: %s\n", nl::ErrorStr(err));
    VerifyOrFail(sigAlgoOID == kOID_SigAlgo_ECDSAWithSHA256, "GetWeaveSignatureAlgo() returned unexpected signature algorithm");

    printf("GetWeaveSignatureAlgoTest complete\n");
}


int main(int argc, char *argv[])
{
    WEAVE_ERROR err;

    err = nl::Weave::Platform::Security::InitSecureRandomDataSource(NULL, 64, NULL, 0);
    FAIL_ERROR(err, "InitSecureRandomDataSource() failed");

    // Make sure verify works first, since SignTests use it.
    WeaveSignatureTest_VerifyTest();
    WeaveSignatureTest_VerifyTest_CertRef_ECDSAWithSHA256();

    WeaveSignatureTest_SignTest();
    WeaveSignatureTest_SignTest_CertRef();
    WeaveSignatureTest_SignTest_ECDSAWithSHA256();

    WeaveSignatureTest_FailureTest_NoCerts();
    WeaveSignatureTest_FailureTest_NoSigningCert();
    WeaveSignatureTest_FailureTest_NoIntermediateCert();
    WeaveSignatureTest_FailureTest_BadHashLength();
    WeaveSignatureTest_FailureTest_BadHash_ECDSAWithSHA1();
    WeaveSignatureTest_FailureTest_BadHash_ECDSAWithSHA256();

    WeaveSignatureTest_InsertRelatedCertsTest_SingleCert();
    WeaveSignatureTest_InsertRelatedCertsTest_MultipleCerts();

    WeaveSignatureTest_GetWeaveSignatureAlgoTest();

    printf("All tests succeeded\n");
}
