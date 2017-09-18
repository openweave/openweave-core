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

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "ToolCommon.h"
#include <Weave/Core/WeaveConfig.h>

#include <nlbyteorder.hpp>

// ============================================================
// Domain parameters for Micro ECC Cryptographic primitives testing.
//
// Parameters and function results are taken from:
//
//     https://www.nsa.gov/ia/_files/nist-routines.pdf
//
// The following curves are supported:
//       secp192r1
//       secp224r1
//       secp256r1
//
// Results for the following cryptographic funcitons are given:
//       Radd = S + T    - point addition
//       Rsub = S - T    - point subtration
//       Rdbl = 2 * S    - point double
//       Rmul = d * S    - point scalar multiplication
//       Rjsm = dS + eT  - joint scalar multiply
//
// ============================================================

#if WEAVE_CONFIG_USE_OPENSSL_ECC

#include <openssl/bn.h>

#define OPENSSL_Constant(NAME, WORDS)                                \
BIGNUM *NAME()                                                       \
{                                                                    \
    uint32_t val[sizeof(WORDS)/sizeof(uint32_t)];                    \
    const size_t numWords = (sizeof(WORDS)/sizeof(uint32_t));        \
                                                                     \
    /* Make local copy of words. */                                  \
    memcpy((uint8_t *)val, (const uint8_t *)(WORDS), sizeof(val));   \
                                                                     \
    /* Swap bytes in words to big-endian order. */                   \
    for (size_t i = 0; i < numWords; i++)                            \
        val[i] = nl::ByteOrder::Swap32HostToBig(val[i]);             \
                                                                     \
    /* Swap words in array to big-endian order. */                   \
    for (size_t i = 0; i < numWords / 2; i++)                        \
    {                                                                \
        uint32_t tmp = val[numWords - 1 - i];                        \
        val[numWords - 1 - i] = val[i];                              \
        val[i] = tmp;                                                \
    }                                                                \
                                                                     \
    /* Construct BIGNUM. */                                          \
    return BN_bin2bn((uint8_t *)val, sizeof(val), NULL);             \
}

#define OPENSSL_PointXYConstants(NAME_BASE, POINT) \
OPENSSL_Constant(NAME_BASE##_X, (POINT)[0])        \
OPENSSL_Constant(NAME_BASE##_Y, (POINT)[1])

#else // WEAVE_CONFIG_USE_OPENSSL_ECC

#define OPENSSL_Constant(NAME, WORDS) /**/
#define OPENSSL_PointXYConstants(NAME_BASE, POINT) /**/

#endif // WEAVE_CONFIG_USE_OPENSSL_ECC


// ============================================================
// NIST-P192
// ============================================================

uint32_t sNIST_P192_ECPointS[2][6] =
{
    { 0x3e97acf8, 0x53a01207, 0xd2467693, 0x0c330266, 0x27ae671b, 0xd458e7d1 },
    { 0x5086df3b, 0x5673a164, 0xcf7fb11b, 0x6bddc050, 0x0d851f33, 0x32593050 }
};

uint32_t sNIST_P192_ECPointT[2][6] =
{
    { 0x9b9753a4, 0xbe16fb05, 0x87fdbd01, 0x67ddecdd, 0x213e9ebe, 0xf22c4395 },
    { 0x691a9c79, 0xa9cecc97, 0xf8dfb41f, 0x7796db48, 0x6af2b359, 0x26442409 }
};

uint32_t sNIST_P192_ScalarD[] =
{
    0xf64c74ee, 0x8255391a, 0xa542463a, 0x5dd41b33, 0x60baec0c, 0xa78a236d
};

uint32_t sNIST_P192_ScalarE[] =
{
    0x85b972bc, 0x9bc393cd, 0xab7cce88, 0x1e4de8ce, 0xec3089e7, 0xc4be3d53
};

uint32_t sNIST_P192_ECPointRadd[2][6] =
{
    { 0xde4d0290, 0x8e843894, 0x77b8abf5, 0xa9d0f1f0, 0x6b9b8e5c, 0x48e1e409 },
    { 0xd7b6aa4b, 0x63c94117, 0xa3648d3d, 0xfb16aa48, 0x797cd7db, 0x408fa77c }
};

uint32_t sNIST_P192_ECPointRsub[2][6] =
{
    { 0xe9f64a2e, 0xc4688f11, 0xc9f61eab, 0x0cc8cc3b, 0x5abfb4fe, 0xfc9683cc },
    { 0x523d816b, 0xd31745d0, 0xa73c23cd, 0x732b1bd2, 0x0fb78269, 0x093e31d0 }
};

uint32_t sNIST_P192_ECPointRdbl[2][6] =
{
    { 0x3f6e6962, 0xba42d25a, 0x14dd8a0e, 0x54b373dc, 0x8c7da253, 0x30c5bc6b },
    { 0xc9d889cf, 0xbbcb2968, 0xf011e2dd, 0xc407aedb, 0x4249a721, 0x0dde14bc }
};

uint32_t sNIST_P192_ECPointRmul[2][6] =
{
    { 0x65bf6d31, 0x62a69529, 0xe3bcec9a, 0x2d0a8f25, 0x5a4f669d, 0x1faee420 },
    { 0x8260fb06, 0x9e7a4d7e, 0x7c696f17, 0x89236708, 0x508a2581, 0x5ff2cdfa }
};

uint32_t sNIST_P192_ECPointRjsm[2][6] =
{
    { 0x778b5bde, 0x60ecb9e1, 0xc17c9bfa, 0xb7dfea82, 0xd8fa9b72, 0x019f64ee },
    { 0x83503644, 0x3c61f35d, 0x800e2a7e, 0x4ced33fb, 0xcd8655fa, 0x16590c5f }
};

OPENSSL_PointXYConstants(NIST_P192_ECPointS, sNIST_P192_ECPointS)
OPENSSL_PointXYConstants(NIST_P192_ECPointT, sNIST_P192_ECPointT)
OPENSSL_Constant(NIST_P192_D, sNIST_P192_ScalarD)
OPENSSL_Constant(NIST_P192_E, sNIST_P192_ScalarE)
OPENSSL_PointXYConstants(NIST_P192_ECPointRadd, sNIST_P192_ECPointRadd)
OPENSSL_PointXYConstants(NIST_P192_ECPointRsub, sNIST_P192_ECPointRsub)
OPENSSL_PointXYConstants(NIST_P192_ECPointRdbl, sNIST_P192_ECPointRdbl)
OPENSSL_PointXYConstants(NIST_P192_ECPointRmul, sNIST_P192_ECPointRmul)
OPENSSL_PointXYConstants(NIST_P192_ECPointRjsm, sNIST_P192_ECPointRjsm)


// ============================================================
// NIST-P224
// ============================================================

uint32_t sNIST_P224_ECPointS[2][7] =
{
    { 0x9a10bb5b, 0xc6fdf16e, 0x95518df3, 0xdd6c97da, 0x43dc814e, 0xa59a9308, 0x6eca814b },
    { 0x64d7bf22, 0x7e05dc6b, 0xd8099414, 0xf259b89c, 0x6aec0ca0, 0x0963bc8b, 0xef4b497f }
};

uint32_t sNIST_P224_ECPointT[2][7] =
{
    { 0x3826bd6d, 0x5d39ac90, 0x8e6ef23c, 0x00296964, 0x88d7e842, 0xa5cb03fb, 0xb72b25ae },
    { 0x34f10c34, 0x1a2dc8b4, 0x33ea729c, 0x1af7dceb, 0x71b5b409, 0x34984f0b, 0xc42a8a4d }
};

uint32_t sNIST_P224_ScalarD[] =
{
    0xaaf1d73b, 0x711e6363, 0x06d37f52, 0x6fbb03df, 0x8e36b2dd, 0xeaca0fcc, 0xa78ccc30
};

uint32_t sNIST_P224_ScalarE[] =
{
    0x6ed35736, 0xa88aa77a, 0x3fc8177f, 0x71e8e070, 0x2519d73e, 0xc08c9659, 0x54d549ff
};

uint32_t sNIST_P224_ECPointRadd[2][7] =
{
    { 0xe8d2fd15, 0xa2162afa, 0x6d2bcfca, 0xd478ee0a, 0x776b107b, 0xe84c2f7d, 0x236f26d9 },
    { 0xbb0fda70, 0xd536ae25, 0xb7d5cdf8, 0x471297a0, 0x746f6a97, 0x904ce6c3, 0xe53cc0a7 }
};

uint32_t sNIST_P224_ECPointRsub[2][7] =
{
    { 0x5210b332, 0x2a793133, 0x61541385, 0xca1054f3, 0x0b36047b, 0xc8f34d4f, 0xdb4112bc },
    { 0x55a8d961, 0x520a0ffb, 0xfacf787a, 0x2396f411, 0x78c1540b, 0x4da48138, 0x90c6e830 }
};

uint32_t sNIST_P224_ECPointRdbl[2][7] =
{
    { 0xcb5cdfc7, 0x2f165e29, 0xd8ee2685, 0xebb46efa, 0x7ca56850, 0x17dee0f2, 0xa9c96f21 },
    { 0x37dfdd7d, 0x49bfbf58, 0x207840bf, 0x417d9579, 0xd76d4930, 0xcf77ced4, 0xadf18c84 }
};

uint32_t sNIST_P224_ECPointRmul[2][7] =
{
    { 0x2702bca4, 0xfdaacc39, 0x736a14c6, 0xdb95777e, 0xff1113ab, 0x92a8d72b, 0x96a7625e },
    { 0xada3ed10, 0xc70d15db, 0x8b43dfad, 0x80191525, 0x13cd2fd5, 0x942a3c5e, 0x0f8e5702 }
};

uint32_t sNIST_P224_ECPointRjsm[2][7] =
{
    { 0xe288c83e, 0xab838d52, 0x18c5b350, 0x3ffd94c9, 0x302a67ea, 0xc7b2cda1, 0xdbfe2958 },
    { 0x147bb18b, 0x7acbc5b8, 0x861aacb8, 0xcc7f0c5a, 0xff4895ab, 0xac3b0549, 0x2f521b83 }
};

OPENSSL_PointXYConstants(NIST_P224_ECPointS, sNIST_P224_ECPointS)
OPENSSL_PointXYConstants(NIST_P224_ECPointT, sNIST_P224_ECPointT)
OPENSSL_Constant(NIST_P224_D, sNIST_P224_ScalarD)
OPENSSL_Constant(NIST_P224_E, sNIST_P224_ScalarE)
OPENSSL_PointXYConstants(NIST_P224_ECPointRadd, sNIST_P224_ECPointRadd)
OPENSSL_PointXYConstants(NIST_P224_ECPointRsub, sNIST_P224_ECPointRsub)
OPENSSL_PointXYConstants(NIST_P224_ECPointRdbl, sNIST_P224_ECPointRdbl)
OPENSSL_PointXYConstants(NIST_P224_ECPointRmul, sNIST_P224_ECPointRmul)
OPENSSL_PointXYConstants(NIST_P224_ECPointRjsm, sNIST_P224_ECPointRjsm)


// ============================================================
// NIST-P256
// ============================================================

uint32_t sNIST_P256_ECPointS[2][8] =
{
    { 0x89da97c9, 0xb77cab39, 0x221a8fa0, 0x617519b3, 0x0f271508, 0x82edd27e, 0xbc8d36e6, 0xde2444be },
    { 0x3042a256, 0xb6350b24, 0x53cec576, 0x702de80f, 0xd1e66659, 0xfc01a5aa, 0xf36e5380, 0xc093ae7f }
};

uint32_t sNIST_P256_ECPointT[2][8] =
{
    { 0x35e0986b, 0xbb8cf92e, 0x61c89575, 0x39540dc8, 0x5316212e, 0x62f6b3b2, 0x8da1d44e, 0x55a8b00f },
    { 0xc8b24316, 0xb656e9d8, 0x598b9e7a, 0xf61a8a52, 0xc4c3dd90, 0x4835d82a, 0x9c2d6c70, 0x5421c320 }
};

uint32_t sNIST_P256_ScalarD[] =
{
    0x2ffb06fd, 0x6522468b, 0x3072708b, 0xd0c7a893, 0x92f43f8d, 0xb6c6a5b9, 0xafdec1e6, 0xc51e4753
};

uint32_t sNIST_P256_ScalarE[] =
{
    0xea029db7, 0xa358016a, 0x37acdd83, 0x5ee8332d, 0xfe3f0b35, 0xf0145cbe, 0xce72a462, 0xd37f628e
};

uint32_t sNIST_P256_ECPointRadd[2][8] =
{
    { 0x545a067e, 0x553cf35a, 0xac476bd4, 0x70349191, 0x8cc5ba69, 0x745195e9, 0x354b6b81, 0x72b13dd4 },
    { 0x744ac264, 0x6d013011, 0x5aa5c9d4, 0xc33b1331, 0x22d7620d, 0x5241a8a1, 0x2e1327d7, 0x8d585cbb }
};

uint32_t sNIST_P256_ECPointRsub[2][8] =
{
    { 0xd6be6021, 0x3e7dc64a, 0xf1c73ea1, 0x837419f8, 0x6129deab, 0x2aad1dbf, 0xb251bb1d, 0xc09ce680 },
    { 0xb1975ef3, 0xce30675f, 0xfdf6c3f4, 0x3414a022, 0x4edab172, 0x6b2f9bad, 0x00bd8833, 0x1a815bf7 }
};

uint32_t sNIST_P256_ECPointRdbl[2][8] =
{
    { 0xdb6127b0, 0x2a860ffc, 0xb17481b8, 0xdf6c22f3, 0xe0024c33, 0xa1a8eef1, 0x1606ee3b, 0x7669e690 },
    { 0xdb61d0c7, 0xe10ca2c1, 0xcd03023d, 0x389ef3ee, 0x072f33de, 0xc39f6ee0, 0x187a54f6, 0xfa878162 }
};

uint32_t sNIST_P256_ECPointRmul[2][8] =
{
    { 0x4eeca03f, 0xacc89ba3, 0xcfc18bed, 0xe62becc3, 0x83c97d11, 0x2946d88d, 0x2d427888, 0x51d08d5f },
    { 0x6a7b41d5, 0x35beca95, 0xa6c0cf30, 0x06f8fcf8, 0x1f6e744e, 0x5b673ab5, 0x8bf626aa, 0x75ee68eb }
};

uint32_t sNIST_P256_ECPointRjsm[2][8] =
{
    { 0x8341f6b8, 0xf857b858, 0x3daacbef, 0xefcf5841, 0xb8046245, 0x34939221, 0x92210092, 0xd867b467 },
    { 0x0f615275, 0x35d54bd8, 0xec7e50dd, 0x106b6607, 0xad69c745, 0x2d22720d, 0xc03cede1, 0xf2504055 }
};

OPENSSL_PointXYConstants(NIST_P256_ECPointS, sNIST_P256_ECPointS)
OPENSSL_PointXYConstants(NIST_P256_ECPointT, sNIST_P256_ECPointT)
OPENSSL_Constant(NIST_P256_D, sNIST_P256_ScalarD)
OPENSSL_Constant(NIST_P256_E, sNIST_P256_ScalarE)
OPENSSL_PointXYConstants(NIST_P256_ECPointRadd, sNIST_P256_ECPointRadd)
OPENSSL_PointXYConstants(NIST_P256_ECPointRsub, sNIST_P256_ECPointRsub)
OPENSSL_PointXYConstants(NIST_P256_ECPointRdbl, sNIST_P256_ECPointRdbl)
OPENSSL_PointXYConstants(NIST_P256_ECPointRmul, sNIST_P256_ECPointRmul)
OPENSSL_PointXYConstants(NIST_P256_ECPointRjsm, sNIST_P256_ECPointRjsm)


// ============================================================
// END
// ============================================================
