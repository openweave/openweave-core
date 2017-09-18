/*
 *
 *    Copyright (c) 2017 Nest Labs, Inc.
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
 *      Micro-ecc implementations of elliptic curve functions used by Weave security code.
 *
 */

#include <string.h>

#include "WeaveCrypto.h"
#include "EllipticCurve.h"
#include "HashAlgos.h"
#include <Weave/Core/WeaveEncoding.h>
#include <Weave/Support/CodeUtils.h>

#if WEAVE_CONFIG_USE_MICRO_ECC

namespace nl {
namespace Weave {
namespace Crypto {

using namespace nl::Weave::ASN1;
using namespace nl::Weave::Platform::Security;

static uECC_Curve CurveOID2uECC_Curve(OID curveOID)
{
    switch (curveOID)
    {
#if WEAVE_CONFIG_SUPPORT_ELLIPTIC_CURVE_SECP160R1
    case kOID_EllipticCurve_secp160r1:
        return uECC_secp160r1();
#endif
#if WEAVE_CONFIG_SUPPORT_ELLIPTIC_CURVE_SECP192R1
    case kOID_EllipticCurve_prime192v1:
        return uECC_secp192r1();
#endif
#if WEAVE_CONFIG_SUPPORT_ELLIPTIC_CURVE_SECP224R1
    case kOID_EllipticCurve_secp224r1:
        return uECC_secp224r1();
#endif
#if WEAVE_CONFIG_SUPPORT_ELLIPTIC_CURVE_SECP256R1
    case kOID_EllipticCurve_prime256v1:
        return uECC_secp256r1();
#endif
    default:
        return NULL;
    }
}

int GetCurveSize(const OID curveOID)
{
    uECC_Curve curve = CurveOID2uECC_Curve(curveOID);
    int curveLen = 0;

    if (curve != NULL)
        curveLen = uECC_curve_num_bytes(curve);

    return curveLen;
}

static int GetSecureRandomData_uECC(uint8_t *buf, unsigned len)
{
    if ( len > 0xFFFF )
        return 0;

    if ( GetSecureRandomData(buf, (uint16_t)len) == WEAVE_NO_ERROR )
        return 1;

    return 0;
}

static WEAVE_ERROR DecodeDERInt(const uint8_t *derInt, uint16_t derIntLen, uint8_t *eccInt, const uint16_t eccIntLen)
{
    if ( derIntLen == 0 )
        return WEAVE_ERROR_INVALID_ARGUMENT;

    /* one leading zero is allowed for positive integer in ASN1 DER format */
    if ( *derInt == 0 )
    {
        derInt++;
        derIntLen--;
    }

    if ( (derIntLen > eccIntLen) || (derIntLen > 0 && *derInt == 0) )
        return WEAVE_ERROR_INVALID_ARGUMENT;

    memset(eccInt, 0, (eccIntLen - derIntLen));
    memcpy(eccInt + (eccIntLen - derIntLen), derInt, derIntLen);

    return WEAVE_NO_ERROR;
}

static WEAVE_ERROR EncodeDERInt(const uint8_t *eccInt, uint16_t eccIntLen, uint8_t *derInt, const uint16_t derIntSize, uint16_t& derIntLen)
{
    while (*eccInt == 0)
    {
        eccInt++;
        eccIntLen--;
    }

    if ( *eccInt & 0x80 )     /* Need Leading Zero */
    {
        if ( derIntSize < eccIntLen + 1 )
            return WEAVE_ERROR_BUFFER_TOO_SMALL;
        *derInt++ = 0;
        derIntLen = eccIntLen + 1;
    }
    else
    {
        if ( derIntSize < eccIntLen )
            return WEAVE_ERROR_BUFFER_TOO_SMALL;
        derIntLen = eccIntLen;
    }

    memcpy(derInt, eccInt, eccIntLen);

    return WEAVE_NO_ERROR;
}

static WEAVE_ERROR DecodeECPrivateKey(const EncodedECPrivateKey& encodedPrivKey, uint8_t *privKey, uint16_t privKeyLen)
{
    return DecodeDERInt(encodedPrivKey.PrivKey, encodedPrivKey.PrivKeyLen, privKey, privKeyLen);
}

static WEAVE_ERROR EncodeECPrivateKey(const uint8_t *privKey, uint16_t privKeyLen, EncodedECPrivateKey& encodedPrivKey)
{
    return EncodeDERInt(privKey, privKeyLen, encodedPrivKey.PrivKey, encodedPrivKey.PrivKeyLen, encodedPrivKey.PrivKeyLen);
}

// Generate an ECDSA signature given a message hash and a EC private key.
WEAVE_ERROR GenerateECDSASignature(OID curveOID,
                                   const uint8_t *msgHash, uint8_t msgHashLen,
                                   const EncodedECPrivateKey& encodedPrivKey,
                                   EncodedECDSASignature& encodedSig)
{
    WEAVE_ERROR err;
    uint8_t l_sig[2 * kuECC_MaxByteCount];   /* l_sig = {r, s} */
    uint16_t tmpLen;
    uECC_Curve curve;
    uint16_t curveLen;

    curve = CurveOID2uECC_Curve(curveOID);
    VerifyOrExit(curve != NULL, err = WEAVE_ERROR_UNSUPPORTED_ELLIPTIC_CURVE);

    err = GenerateECDSASignature(curveOID, msgHash, msgHashLen, encodedPrivKey, l_sig);
    SuccessOrExit(err);

    // Curve length in bytes
    curveLen = uECC_curve_num_bytes(curve);

    err = EncodeDERInt(l_sig, curveLen, encodedSig.R, encodedSig.RLen, tmpLen);
    SuccessOrExit(err);

    encodedSig.RLen = tmpLen;

    err = EncodeDERInt(l_sig + curveLen, curveLen, encodedSig.S, encodedSig.SLen, tmpLen);
    SuccessOrExit(err);

    encodedSig.SLen = tmpLen;

exit:
    return err;
}

// Generate an ECDSA signature given a message hash and a EC private key.
WEAVE_ERROR GenerateECDSASignature(OID curveOID,
                                   const uint8_t *msgHash, uint8_t msgHashLen,
                                   const EncodedECPrivateKey& encodedPrivKey,
                                   uint8_t *fixedLenSig)
{
    WEAVE_ERROR err;
    uint8_t privKey[kuECC_MaxByteCount];
    int res;
    uECC_Curve curve;
    uint16_t privKeyLen;

    curve = CurveOID2uECC_Curve(curveOID);
    VerifyOrExit(curve != NULL, err = WEAVE_ERROR_UNSUPPORTED_ELLIPTIC_CURVE);

    // Private key length in bytes
    privKeyLen = uECC_curve_num_n_bytes(curve);

    err = DecodeECPrivateKey(encodedPrivKey, privKey, privKeyLen);
    SuccessOrExit(err);

    // Set GetSecureRandomData_uECC function to be used by Micro-ECC for random bytes generation
    uECC_set_rng(GetSecureRandomData_uECC);

    // Attempt to sign the message, producing the R and S values in the process.
    // uECC_sign repeats the process several times if the generated random number was not suitable for signing.
    res = uECC_sign(privKey, msgHash, msgHashLen, fixedLenSig, curve);
    VerifyOrExit(res == 1, err = WEAVE_ERROR_RANDOM_DATA_UNAVAILABLE);

exit:
    ClearSecretData(privKey, sizeof(privKey));

    return err;
}

// Verify an ECDSA signature.
WEAVE_ERROR VerifyECDSASignature(OID curveOID,
                                 const uint8_t *msgHash, uint8_t msgHashLen,
                                 const EncodedECDSASignature& encodedSig,
                                 const EncodedECPublicKey& encodedPubKey)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint8_t l_sig[2 * kuECC_MaxByteCount];   /* l_sig = {r, s} */
    int res;
    uECC_Curve curve;
    uint16_t curveLen;

    curve = CurveOID2uECC_Curve(curveOID);
    VerifyOrExit(curve != NULL, err = WEAVE_ERROR_UNSUPPORTED_ELLIPTIC_CURVE);

    // Curve length in bytes
    curveLen = uECC_curve_num_bytes(curve);

    err = DecodeDERInt(encodedSig.R, encodedSig.RLen, l_sig, curveLen);
    SuccessOrExit(err);

    err = DecodeDERInt(encodedSig.S, encodedSig.SLen, l_sig + curveLen, curveLen);
    SuccessOrExit(err);

    // Verify encoded point size and X9.63 uncompressed format (0x04)
    VerifyOrExit(encodedPubKey.ECPointLen == 2 * curveLen + 1, err = WEAVE_ERROR_INVALID_ARGUMENT);
    VerifyOrExit(*encodedPubKey.ECPoint == kX963EncodedPointFormat_Uncompressed, err = WEAVE_ERROR_INVALID_ARGUMENT);

    res = uECC_verify(encodedPubKey.ECPoint + 1, msgHash, msgHashLen, l_sig, curve);
    VerifyOrExit(res == 1, err = WEAVE_ERROR_INVALID_SIGNATURE);

exit:
    return err;
}

// Verify an ECDSA signature.
WEAVE_ERROR VerifyECDSASignature(OID curveOID,
                                 const uint8_t *msgHash, uint8_t msgHashLen,
                                 const uint8_t *fixedLenSig,
                                 const EncodedECPublicKey& encodedPubKey)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    int res;
    uECC_Curve curve;
    uint16_t curveLen;

    curve = CurveOID2uECC_Curve(curveOID);
    VerifyOrExit(curve != NULL, err = WEAVE_ERROR_UNSUPPORTED_ELLIPTIC_CURVE);

    // Curve length in bytes
    curveLen = uECC_curve_num_bytes(curve);

    // Verify encoded point size and X9.63 uncompressed format (0x04)
    VerifyOrExit(encodedPubKey.ECPointLen == 2 * curveLen + 1, err = WEAVE_ERROR_INVALID_ARGUMENT);
    VerifyOrExit(*encodedPubKey.ECPoint == kX963EncodedPointFormat_Uncompressed, err = WEAVE_ERROR_INVALID_ARGUMENT);

    res = uECC_verify(encodedPubKey.ECPoint + 1, msgHash, msgHashLen, fixedLenSig, curve);
    VerifyOrExit(res == 1, err = WEAVE_ERROR_INVALID_SIGNATURE);

exit:
    return err;
}

#if WEAVE_CONFIG_SECURITY_TEST_MODE
// Constant-time check if the supplied private key has the integer value of 1 (big endian).
static bool IsOneKey(const uint8_t *privKey, uint16_t len)
{
    uint8_t keyBits = 0;
    int i = 0;

    for (; i < len - 1; i++)
        keyBits |= privKey[i];
    keyBits |= (privKey[i] - 1);

    return (keyBits == 0);
}
#endif

WEAVE_ERROR ECDHComputeSharedSecret(OID curveOID, const EncodedECPublicKey& encodedPubKey, const EncodedECPrivateKey& encodedPrivKey,
                                    uint8_t *sharedSecretBuf, uint16_t sharedSecretBufSize, uint16_t& sharedSecretLen)
{
    WEAVE_ERROR err;
    uint8_t privKey[kuECC_MaxByteCount];
    uECC_Curve curve;
    uint16_t curveLen;
    int res;

    curve = CurveOID2uECC_Curve(curveOID);
    VerifyOrExit(curve != NULL, err = WEAVE_ERROR_UNSUPPORTED_ELLIPTIC_CURVE);

    // Curve length in bytes
    curveLen = uECC_curve_num_bytes(curve);

    VerifyOrExit(sharedSecretBufSize >= curveLen, err = WEAVE_ERROR_BUFFER_TOO_SMALL);

    // Verify encoded point size and X9.63 uncompressed format (0x04)
    VerifyOrExit(encodedPubKey.ECPointLen >= 2 * curveLen + 1, err = WEAVE_ERROR_INVALID_ARGUMENT);
    VerifyOrExit(*encodedPubKey.ECPoint == kX963EncodedPointFormat_Uncompressed, err = WEAVE_ERROR_INVALID_ARGUMENT);

    err = DecodeECPrivateKey(encodedPrivKey, privKey, curveLen);
    SuccessOrExit(err);

    res = uECC_shared_secret(encodedPubKey.ECPoint + 1, privKey, sharedSecretBuf, curve);
#if WEAVE_CONFIG_SECURITY_TEST_MODE
    // uECC does not handle multiplying a point by 1.  So if the private key is the well-known
    // test private key (1), ignore the result of uECC_shared_secret() and set the derived
    // shared secret to the X value of the peer's public key.
    if (IsOneKey(privKey, curveLen))
    {
        memcpy(sharedSecretBuf, encodedPubKey.ECPoint + 1, curveLen);
        res = 1;
    }
#endif
    VerifyOrExit(res != 0, err = WEAVE_ERROR_INVALID_ARGUMENT);

    sharedSecretLen = curveLen;

exit:
    ClearSecretData(privKey, sizeof(privKey));

    return err;
}

WEAVE_ERROR GenerateECDHKey(OID curveOID, EncodedECPublicKey& encodedPubKey, EncodedECPrivateKey& encodedPrivKey)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint8_t privKey[kuECC_MaxByteCount];
    int res;
    uECC_Curve curve;
    uint16_t curveLen;

    curve = CurveOID2uECC_Curve(curveOID);
    VerifyOrExit(curve != NULL, err = WEAVE_ERROR_UNSUPPORTED_ELLIPTIC_CURVE);

    // Curve length in bytes
    curveLen = uECC_curve_num_bytes(curve);

    VerifyOrExit(encodedPubKey.ECPointLen >= 2 * curveLen + 1, err = WEAVE_ERROR_BUFFER_TOO_SMALL);

    // Set GetSecureRandomData_uECC function to be used by Micro-ECC for random bytes generation
    uECC_set_rng(GetSecureRandomData_uECC);

    // uECC_make_key repeats the process 16 times if the generated random number was not suitable for signing
    res = uECC_make_key(encodedPubKey.ECPoint + 1, privKey, curve);
    VerifyOrExit(res == 1, err = WEAVE_ERROR_RANDOM_DATA_UNAVAILABLE);

    // Encode EC Point to X9.63 uncompressed format.
    *encodedPubKey.ECPoint = kX963EncodedPointFormat_Uncompressed;
    encodedPubKey.ECPointLen = 2 * curveLen + 1;

    err = EncodeECPrivateKey(privKey, curveLen, encodedPrivKey);
    SuccessOrExit(err);

exit:
    ClearSecretData(privKey, sizeof(privKey));

    return err;
}

WEAVE_ERROR GetCurveG(OID curveOID, EncodedECPublicKey& encodedG)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uECC_Curve curve;
    uint16_t curveLen, encodedPointLen;
    const uECC_word_t *g;

    curve = CurveOID2uECC_Curve(curveOID);
    VerifyOrExit(curve != NULL, err = WEAVE_ERROR_UNSUPPORTED_ELLIPTIC_CURVE);

    // Curve length in bytes
    curveLen = uECC_curve_num_bytes(curve);

    // Compute the encoded point length.
    encodedPointLen = 2 * curveLen + 1;

    VerifyOrExit(encodedG.ECPointLen >= encodedPointLen, err = WEAVE_ERROR_BUFFER_TOO_SMALL);

    g = uECC_curve_G(curve);

    *encodedG.ECPoint = kX963EncodedPointFormat_Uncompressed;
    uECC_vli_nativeToBytes(encodedG.ECPoint + 1, curveLen, g);
    uECC_vli_nativeToBytes(encodedG.ECPoint + 1 + curveLen, curveLen, g + uECC_curve_num_words(curve));
    encodedG.ECPointLen = encodedPointLen;

exit:
    return err;
}

// ============================================================
// Elliptic Curve JPAKE Class Functions
// ============================================================

#if WEAVE_IS_ECJPAKE_ENABLED

/* Compares points: returns 1 if left == right */
#define uECC_point_equal(left, right, num_words) \
        uECC_vli_equal((left), (right), 2 * (num_words))

/* Sets point: result = point */
#define uECC_point_set(result, point, num_words) \
        uECC_vli_set((result), (point), 2 * (num_words))

/* Checks if point is zero: returns 1 if point is zero */
#define uECC_point_isZero(point, num_words) \
        uECC_vli_isZero((point), 2 * (num_words))

/* Zeroizes point: point = 0 */
#define uECC_point_clear(point, num_words) \
        uECC_vli_clear((point), 2 * (num_words))

/* Points addition: result = left + right */
extern void uECC_point_add(uECC_word_t *result,
                           const uECC_word_t *left,
                           const uECC_word_t *right,
                           uECC_Curve curve)
{
    uECC_word_t Rx[kuECC_MaxWordCount];
    uECC_word_t Ry[kuECC_MaxWordCount];
    uECC_word_t l[kuECC_MaxWordCount] = { 0 };

    const uECC_word_t *curve_p = uECC_curve_p(curve);
    const wordcount_t num_words = uECC_curve_num_words(curve);

    /* if left == 0, then result = right */
    if (uECC_point_isZero(left, num_words)) {
        uECC_point_set(result, right, num_words);
        return;
    }

    /* if right == 0, then result = left */
    if (uECC_point_isZero(right, num_words)) {
        uECC_point_set(result, left, num_words);
        return;
    }

    /* if left == right, then result = 2 * left */
    if (uECC_point_equal(left, right, num_words)) {
        l[0] = 0x02;
        uECC_point_mult(result, left, l, curve);
        return;
    }

    /* if left == - right, then result = 0 */
    if (uECC_vli_equal(left, right, num_words)) {
        uECC_point_clear(result, num_words);
        return;
    }

    /**
     * Calculate:
     *     (Rx, Ry) = (leftX, leftY) + (rightX, rightY)
     * Method for two Elliptic Curve Points addition:
     *     lambda (l) = (rightY - leftY) / (rightX - leftX)
     * then:
     *     resultX = lambda ^ 2 - rightX - leftX
     *     resultY = lambda * (leftX - Rx) - leftY
     */

    /* calculate l = (rightY - leftY) / (rightX - leftX) */
    uECC_vli_modSub(l, right, left, curve_p, num_words);
    uECC_vli_modSub(Rx, right + num_words, left + num_words,
                    curve_p, num_words);
    uECC_vli_modInv(l, l, curve_p, num_words);
    uECC_vli_modMult_fast(l, l, Rx, curve);

    /* calculate Rx = l ^ 2 - rightX - leftX */
    uECC_vli_modMult_fast(Rx, l, l, curve);
    uECC_vli_modSub(Rx, Rx, right, curve_p, num_words);
    uECC_vli_modSub(Rx, Rx, left, curve_p, num_words);

    /* calculate Ry = l * (leftX - Rx) - leftY */
    uECC_vli_modSub(Ry, left, Rx, curve_p, num_words);
    uECC_vli_modMult_fast(Ry, Ry, l, curve);
    uECC_vli_modSub(Ry, Ry, left + num_words, curve_p, num_words);

    /* assign output */
    uECC_vli_set(result, Rx, num_words);
    uECC_vli_set(result + num_words, Ry, num_words);
}

/* Converts long integer in big-endian form (input) into uECC native VLI form (modulo n) */
static void uECC_vli_bytesToNative_mod_n(uECC_word_t *result,
                                         const uint8_t *input,
                                         wordcount_t input_len,
                                         uECC_Curve curve)
{
    const uint8_t num_n_words = uECC_curve_num_n_words(curve);
    uint32_t input_vli[2 * kuECC_MaxWordCount] = { 0 };

    /* Convert to VLI (native) form */
    uECC_vli_bytesToNative(input_vli, input, input_len);

    if (((input_len + 3) / 4) < num_n_words) {
        /* Copy result */
        memcpy((uint8_t *)result, (uint8_t *)input_vli, 4 * num_n_words);
    } else {
        /* Modulo reduction: result = input_vli % n */
        uECC_vli_mmod(result, input_vli, uECC_curve_n(curve), num_n_words);
    }
}

void EllipticCurveJPAKE::Init()
{
}

void EllipticCurveJPAKE::Shutdown()
{
    Reset();
}

/* Clear secret content and Reset algorithm parameters */
void EllipticCurveJPAKE::Reset()
{
    memset(this, 0, sizeof(*this));
}

/* Init EllipticCurveJPAKE algorithm parameters */
WEAVE_ERROR EllipticCurveJPAKE::Init(const OID curveOID,
                                     const uint8_t *pw, const uint16_t pwLen,
                                     const uint8_t *localName, const uint16_t localNameLen,
                                     const uint8_t *peerName, const uint16_t peerNameLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    Curve = CurveOID2uECC_Curve(curveOID);
    VerifyOrExit(Curve != NULL, err = WEAVE_ERROR_UNSUPPORTED_ELLIPTIC_CURVE);

    // verify valid length inputs
    VerifyOrExit(pwLen <= kECJPAKE_MaxPasswordLength &&
                 localNameLen <= kECJPAKE_MaxNameLength &&
                 peerNameLen <= kECJPAKE_MaxNameLength, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // convert password into VLI integer form
    uECC_vli_bytesToNative_mod_n(XbS, pw, pwLen, Curve);

    // init local and peer names
    memcpy(LocalName, localName, localNameLen);
    memcpy(PeerName, peerName, peerNameLen);
    LocalNameLen = localNameLen;
    PeerNameLen = peerNameLen;

    // clear Gxacd/Gxabc as protocol assumes they are zero at initialization
    memset((uint8_t *)Gxacd, 0, sizeof(EccPoint));
    memset((uint8_t *)Gxabc, 0, sizeof(EccPoint));

    // Set GetSecureRandomData_uECC function to be used by Micro-ECC for random bytes generation
    uECC_set_rng(GetSecureRandomData_uECC);

exit:
    if (err != WEAVE_NO_ERROR)
        Reset();

    return err;
}

/* h = hash(G, G*r, G*x, name) */
static void ZeroKnowledgeProofHash(const wordcount_t words, uint8_t *h, const EccPoint zkpG, const ECJPAKEStepPart *stepPart, const uint8_t *name, const uint16_t nameLen)
{
    SHA256 hash;
    const uint16_t pointLen = 2 * kuECC_WordSize * words;

    hash.Begin();
    hash.AddData((uint8_t *)zkpG, pointLen);          // Hash G  point
    hash.AddData((uint8_t *)stepPart->Gr, pointLen);  // Hash Gr point
    hash.AddData((uint8_t *)stepPart->Gx, pointLen);  // Hash Gx point
    hash.AddData(name, nameLen);                      // Hash name string
    hash.Finish(h);
}

/* Find Step Part Data Fields in the Buffer */
WEAVE_ERROR EllipticCurveJPAKE::FindStepPartDataPointers(ECJPAKEStepPart *stepPart, const uint8_t *buf, const uint16_t bufSize, uint16_t& stepDataLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    const uint8_t pointWordCount = 2 * uECC_curve_num_words(Curve);
    const uint8_t stepPartByteCount = (2 * pointWordCount + uECC_curve_num_n_words(Curve)) * kuECC_WordSize;

    VerifyOrExit(stepPartByteCount <= bufSize - stepDataLen, err = WEAVE_ERROR_BUFFER_TOO_SMALL);

    stepPart->Gx = (uECC_word_t *)(buf + stepDataLen);
    stepPart->Gr = stepPart->Gx + pointWordCount;
    stepPart->b  = stepPart->Gr + pointWordCount;

    stepDataLen += stepPartByteCount;

exit:
    return err;
}

WEAVE_ERROR EllipticCurveJPAKE::VerifyZeroKnowledgeProof(const ECJPAKEStepPart *stepPart, const EccPoint zkpG, const uint8_t *name, const uint16_t nameLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    union
    {
        uint8_t hash[kECJPAKE_HashLength];
        uECC_word_t hashVLI[kuECC_MaxWordCount];
    };
    EccPoint ecPoint1;
    EccPoint ecPoint2;

    /* hash = hash(G, G*r, G*x, name) */
    ZeroKnowledgeProofHash(uECC_curve_num_words(Curve), hash, zkpG, stepPart, name, nameLen);
    /* Convert ZKP Hash result into VLI format */
    uECC_vli_bytesToNative_mod_n(hashVLI, hash, kECJPAKE_HashLength, Curve);
    /* ecPoint1 = G*b */
    uECC_point_mult(ecPoint1, zkpG, stepPart->b, Curve);
    /* ecPoint2 = (G*x)*h = G*{h*x} */
    uECC_point_mult(ecPoint2, stepPart->Gx, hashVLI, Curve);
    /* ecPoint2 = ecPoint1 + ecPoint2 = G*{hx} + G*b = G*{hx+b} = G*r (allegedly) */
    uECC_point_add(ecPoint2, ecPoint1, ecPoint2, Curve);
    /* verify ecPoint2 == G*r (ecPoint1) */
    if (!uECC_point_equal(ecPoint2, stepPart->Gr, uECC_curve_num_words(Curve)))
        err = WEAVE_ERROR_INVALID_PASE_PARAMETER;

    return err;
}

/* Prove knowledge of x (in G*x) */
WEAVE_ERROR EllipticCurveJPAKE::GenerateZeroKnowledgeProof(ECJPAKEStepPart *stepPart, const uECC_word_t *x, const EccPoint zkpG, const uint8_t *name, const uint16_t nameLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    union
    {
        uint8_t hash[kECJPAKE_HashLength];
        uECC_word_t hashVLI[kuECC_MaxWordCount];
    };
    const uECC_word_t *curve_n = uECC_curve_n(Curve);
    uint8_t num_n_words = uECC_curve_num_n_words(Curve);

    /* Generate random number r in [1, n-1] */
    /* uses b as temp storage for random number */
    VerifyOrExit(uECC_generate_random_int(stepPart->b, curve_n, num_n_words),
                     err = WEAVE_ERROR_RANDOM_DATA_UNAVAILABLE);
    /* G*r */
    uECC_point_mult(stepPart->Gr, zkpG, stepPart->b, Curve);
    /* hash = hash(G, G*r, G*x, name) */
    ZeroKnowledgeProofHash(uECC_curve_num_words(Curve), hash, zkpG, stepPart, name, nameLen);
    /* Convert ZKP Hash result into VLI format */
    uECC_vli_bytesToNative_mod_n(hashVLI, hash, kECJPAKE_HashLength, Curve);
    /* b = (r - x * h) % n */
    uECC_vli_modMult(hashVLI, x, hashVLI, curve_n, num_n_words);
    uECC_vli_modSub(stepPart->b, stepPart->b, hashVLI, curve_n, num_n_words);

exit:
    return err;
}

WEAVE_ERROR EllipticCurveJPAKE::GenerateStepPart(ECJPAKEStepPart *stepPart, const uECC_word_t *x, const EccPoint G, const uint8_t *name, const uint16_t nameLen)
{
    uECC_point_mult(stepPart->Gx, G, x, Curve);
    return GenerateZeroKnowledgeProof(stepPart, x, G, name, nameLen);
}

WEAVE_ERROR EllipticCurveJPAKE::GenerateStep1(const uint8_t *buf, const uint16_t bufSize, uint16_t& stepDataLen)
{
    WEAVE_ERROR err;
    ECJPAKEStepPart stepPart;
    const uECC_word_t *curve_n = uECC_curve_n(Curve);
    uint8_t num_n_words = uECC_curve_num_n_words(Curve);

    /* Find Step1 (Part1) Data Pointers */
    err = FindStepPartDataPointers(&stepPart, buf, bufSize, stepDataLen);
    SuccessOrExit(err);
    /* Generate Random xa */
    /* xa is temporary stored at Xb and then overriden with Xb value */
    VerifyOrExit(uECC_generate_random_int(Xb, curve_n, num_n_words),
                     err = WEAVE_ERROR_RANDOM_DATA_UNAVAILABLE);
    /* Generate STEP1 Part 1 */
    err = GenerateStepPart(&stepPart, Xb, uECC_curve_G(Curve), LocalName, LocalNameLen);
    SuccessOrExit(err);
    /* add Gxa to Gxacd and to Gxabc */
    uECC_point_add(Gxacd, Gxacd, stepPart.Gx, Curve);
    uECC_point_add(Gxabc, Gxabc, stepPart.Gx, Curve);

    /* Find Step1 (Part2) Data Pointers */
    err = FindStepPartDataPointers(&stepPart, buf, bufSize, stepDataLen);
    SuccessOrExit(err);
    /* Generate Random xb */
    VerifyOrExit(uECC_generate_random_int(Xb,  curve_n, num_n_words),
                     err = WEAVE_ERROR_RANDOM_DATA_UNAVAILABLE);
    /* Generate STEP1 Part 2 */
    err = GenerateStepPart(&stepPart, Xb, uECC_curve_G(Curve), LocalName, LocalNameLen);
    SuccessOrExit(err);
    /* add Gxb to Gxabc */
    uECC_point_add(Gxabc, Gxabc, stepPart.Gx, Curve);

    /* Calculate and Store (Xb * Secret % n) for future use in STEP2 Generate/Process */
    /* NOTE: XbS is initialized with secret value in VLI format */
    uECC_vli_modMult(XbS, Xb, XbS,  curve_n, num_n_words);

exit:
    return err;
}

WEAVE_ERROR EllipticCurveJPAKE::ProcessStep1(const uint8_t *buf, const uint16_t bufSize, uint16_t& stepDataLen)
{
    WEAVE_ERROR err;
    ECJPAKEStepPart stepPart;

    /* Find Step1 (Part1) Data Pointers */
    err = FindStepPartDataPointers(&stepPart, buf, bufSize, stepDataLen);
    SuccessOrExit(err);
    /* check Gxc is legal point */
    VerifyOrExit(uECC_valid_point(stepPart.Gx, Curve), err = WEAVE_ERROR_INVALID_ARGUMENT);
    /* verify ZKP(xc) */
    err = VerifyZeroKnowledgeProof(&stepPart, uECC_curve_G(Curve), PeerName, PeerNameLen);
    SuccessOrExit(err);
    /* add Gxc to Gxacd and to Gxabc */
    uECC_point_add(Gxacd, Gxacd, stepPart.Gx, Curve);
    uECC_point_add(Gxabc, Gxabc, stepPart.Gx, Curve);

    /* Find Step1 (Part2) Data Pointers */
    err = FindStepPartDataPointers(&stepPart, buf, bufSize, stepDataLen);
    SuccessOrExit(err);
    /* check Gxd is legal point */
    VerifyOrExit(uECC_valid_point(stepPart.Gx, Curve), err = WEAVE_ERROR_INVALID_ARGUMENT);
    /* verify ZKP(xd) */
    err = VerifyZeroKnowledgeProof(&stepPart, uECC_curve_G(Curve), PeerName, PeerNameLen);
    SuccessOrExit(err);
    /* add Gxd to Gxacd and store at Gxd */
    uECC_point_add(Gxacd, Gxacd, stepPart.Gx, Curve);
    uECC_point_set(Gxd, stepPart.Gx, uECC_curve_num_words(Curve));

exit:
    return err;
}

WEAVE_ERROR EllipticCurveJPAKE::GenerateStep2(const uint8_t *buf, const uint16_t bufSize, uint16_t& stepDataLen)
{
    WEAVE_ERROR err;
    ECJPAKEStepPart stepPart;

    /**
     * For STEP2 the generator is:
     *        G2 = G * {xa + xc + xd}   ( Gxacd )
     * and the random value is:
     *        x2 = xb * s             ( XbS )
     */

    /* Find Step2 Data Fields */
    err = FindStepPartDataPointers(&stepPart, buf, bufSize, stepDataLen);
    SuccessOrExit(err);

    /* Generate Step2 Data with ZKP(xb * s) */
    err = GenerateStepPart(&stepPart, XbS, Gxacd, LocalName, LocalNameLen);

exit:
    return err;
}

/* Gx = G * {(xc + xa + xb) * xd * s} */
void EllipticCurveJPAKE::ComputeSharedSecret(const EccPoint Gx)
{
    SHA256 hash;
    EccPoint ecPoint;
    const wordcount_t num_words = uECC_curve_num_words(Curve);

    /**
     * K = (Gx - G * {xb * xd * s}) * xb
     *   = (G * {(xc + xa + xb) * xd * s - xb * xd * s}) * xb
     *   = (G * {(xc + xa) * xd * s}) * xb
     *   =  G * {(xa + xc) * xb * xd * s}
     * [which is the same regardless of who calculates it]
     */

    /* ecPoint = G * {xb * xd * s} */
    uECC_point_mult(ecPoint, Gxd, XbS, Curve);
    /* ecPoint = - ecPoint */
    uECC_vli_sub(ecPoint + num_words, uECC_curve_p(Curve), ecPoint + num_words, num_words);
    /* ecPoint = Gx - G * {xb * xd * s} = G * {(xc + xa) * xd * s} */
    uECC_point_add(ecPoint, Gx, ecPoint, Curve);
    /* ecPoint = ecPoint * xb = G * {(xa + xc) * xb * xd * s} */
    uECC_point_mult(ecPoint, ecPoint, Xb, Curve);
    /* HASH ecPoint to get Shared Key */
    hash.Begin();
    hash.AddData((uint8_t *)ecPoint, 2 * kuECC_WordSize * uECC_curve_num_words(Curve));
    hash.Finish(SharedSecret);
}

WEAVE_ERROR EllipticCurveJPAKE::ProcessStep2(const uint8_t *buf, const uint16_t bufSize, uint16_t& stepDataLen)
{
    WEAVE_ERROR err;
    ECJPAKEStepPart stepPart;

    /* Find Step2 Data Fields Pointers */
    err = FindStepPartDataPointers(&stepPart, buf, bufSize, stepDataLen);
    SuccessOrExit(err);
    /* verify ZKP(xd * s),  where  G' = G*{xc + xa + xb} = Gxabc */
    err = VerifyZeroKnowledgeProof(&stepPart, Gxabc, PeerName, PeerNameLen);
    SuccessOrExit(err);
    /* Compute Key */
    ComputeSharedSecret(stepPart.Gx);

exit:
    return err;
}

uint8_t *EllipticCurveJPAKE::GetSharedSecret()
{
    return SharedSecret;
}

int EllipticCurveJPAKE::GetCurveSize()
{
    return (kuECC_WordSize * uECC_curve_num_words(Curve));
}

#endif /* WEAVE_IS_ECJPAKE_ENABLED */

} // namespace Crypto
} // namespace Weave
} // namespace nl
#endif /* WEAVE_CONFIG_USE_MICRO_ECC */
