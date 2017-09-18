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
 *      OpenSSL implementations of elliptic curve functions used by Weave security code.
 *
 */

#ifndef OPENSSL_EXPERIMENTAL_ECJPAKE
#define OPENSSL_EXPERIMENTAL_ECJPAKE
#endif

#include "WeaveCrypto.h"
#include "EllipticCurve.h"
#include <Weave/Core/WeaveEncoding.h>
#include <Weave/Support/CodeUtils.h>

#if WEAVE_WITH_OPENSSL

#include <string.h>

#include <openssl/obj_mac.h>

#if WEAVE_CONFIG_ALLOW_NON_STANDARD_ELLIPTIC_CURVES
#include <openssl/objects.h>
#endif

#if WEAVE_IS_ECJPAKE_ENABLED
#include <openssl/ecjpake.h>
#endif

namespace nl {
namespace Weave {
namespace Crypto {

using namespace nl::Weave::ASN1;
using namespace nl::Weave::Encoding;

// ============================================================
// OpenSSL implementations of primary elliptic curve functions
// used by Weave security code.
// ============================================================

#if WEAVE_CONFIG_USE_OPENSSL_ECC

// Generate an ECDSA signature given a message hash and a EC private key.
NL_DLL_EXPORT WEAVE_ERROR GenerateECDSASignature(OID curveOID,
                                   const uint8_t *msgHash, uint8_t msgHashLen,
                                   const EncodedECPrivateKey& encodedPrivKey,
                                   EncodedECDSASignature& encodedSig)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    EC_KEY *key = NULL;
    ECDSA_SIG *ecSig = NULL;

    // Decode the private key into a EC_KEY object.
    err = DecodeECKey(curveOID, &encodedPrivKey, NULL, key);
    SuccessOrExit(err);

    // Generate the signature for the given message hash.
    ecSig = ECDSA_do_sign(msgHash, msgHashLen, key);
    VerifyOrExit(ecSig != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT); // TODO: use better error

    // Encode the signature into the output structure.
    err = EncodeECDSASignature(ecSig, encodedSig);
    SuccessOrExit(err);

exit:
    ECDSA_SIG_free(ecSig);
    EC_KEY_free(key);

    return err;
}

// Generate an ECDSA signature given a message hash and a EC private key.
WEAVE_ERROR GenerateECDSASignature(OID curveOID,
                                   const uint8_t *msgHash, uint8_t msgHashLen,
                                   const EncodedECPrivateKey& encodedPrivKey,
                                   uint8_t *fixedLenSig)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    EC_KEY *key = NULL;
    ECDSA_SIG *ecSig = NULL;

    // Decode the private key into a EC_KEY object.
    err = DecodeECKey(curveOID, &encodedPrivKey, NULL, key);
    SuccessOrExit(err);

    // Generate the signature for the given message hash.
    ecSig = ECDSA_do_sign(msgHash, msgHashLen, key);
    VerifyOrExit(ecSig != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT); // TODO: use better error

    // Encode the signature into the output structure.
    err = ECDSASigToFixedLenSig(curveOID, ecSig, fixedLenSig);
    SuccessOrExit(err);

exit:
    ECDSA_SIG_free(ecSig);
    EC_KEY_free(key);

    return err;
}

// Verify an ECDSA signature.
NL_DLL_EXPORT WEAVE_ERROR VerifyECDSASignature(OID curveOID,
                                 const uint8_t *msgHash, uint8_t msgHashLen,
                                 const EncodedECDSASignature& encodedSig,
                                 const EncodedECPublicKey& encodedPubKey)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    EC_KEY *pubKey = NULL;
    ECDSA_SIG *sig = NULL;
    int res;

    // Decode the public key into a EC_KEY object.
    err = DecodeECKey(curveOID, NULL, &encodedPubKey, pubKey);
    SuccessOrExit(err);

    err = DecodeECDSASignature(encodedSig, sig);
    SuccessOrExit(err);

    res = ECDSA_do_verify(msgHash, msgHashLen, sig, pubKey);
    VerifyOrExit(res == 1, err = WEAVE_ERROR_INVALID_SIGNATURE);

exit:
    ECDSA_SIG_free(sig);
    EC_KEY_free(pubKey);

    return err;
}

// Verify an ECDSA signature.
WEAVE_ERROR VerifyECDSASignature(OID curveOID,
                                 const uint8_t *msgHash, uint8_t msgHashLen,
                                 const uint8_t *fixedLenSig,
                                 const EncodedECPublicKey& encodedPubKey)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    EC_KEY *pubKey = NULL;
    ECDSA_SIG *sig = NULL;
    int res;

    // Decode the public key into a EC_KEY object.
    err = DecodeECKey(curveOID, NULL, &encodedPubKey, pubKey);
    SuccessOrExit(err);

    // Convert fixed-length signature into a ECDSA_SIG object.
    err = FixedLenSigToECDSASig(curveOID, fixedLenSig, sig);
    SuccessOrExit(err);

    // Verify the signature for the given message hash.
    res = ECDSA_do_verify(msgHash, msgHashLen, sig, pubKey);
    VerifyOrExit(res == 1, err = WEAVE_ERROR_INVALID_SIGNATURE);

exit:
    ECDSA_SIG_free(sig);
    EC_KEY_free(pubKey);

    return err;
}

// Generate a public/private key pair suitable for Elliptic Curve Diffie-Hellman.
WEAVE_ERROR GenerateECDHKey(OID curveOID, EncodedECPublicKey& encodedPubKey, EncodedECPrivateKey& encodedPrivKey)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    EC_GROUP *ecGroup = NULL;
    EC_KEY *key = NULL;
    const BIGNUM *privKey;
    int res, privKeyLen;

    err = GetECGroupForCurve(curveOID, ecGroup);
    SuccessOrExit(err);

    key = EC_KEY_new();
    VerifyOrExit(key != NULL, err = WEAVE_ERROR_NO_MEMORY);

    res = EC_KEY_set_group(key, ecGroup);
    VerifyOrExit(res, err = WEAVE_ERROR_NO_MEMORY);

    res = EC_KEY_generate_key(key);
    VerifyOrExit(res, err = WEAVE_ERROR_NO_MEMORY); // TODO: map OpenSSL error

    err = EncodeX962ECPoint(curveOID, ecGroup, EC_KEY_get0_public_key(key), encodedPubKey.ECPoint, encodedPubKey.ECPointLen, encodedPubKey.ECPointLen);
    SuccessOrExit(err);

    privKey = EC_KEY_get0_private_key(key);
    privKeyLen = BN_num_bytes(privKey);
    VerifyOrExit(encodedPrivKey.PrivKeyLen >= privKeyLen, err = WEAVE_ERROR_BUFFER_TOO_SMALL);

    BN_bn2bin(privKey, encodedPrivKey.PrivKey);
    encodedPrivKey.PrivKeyLen = privKeyLen;

exit:
    EC_GROUP_free(ecGroup);
    EC_KEY_free(key);

    return err;
}

WEAVE_ERROR ECDHComputeSharedSecret(OID curveOID, const EncodedECPublicKey& encodedPubKey, const EncodedECPrivateKey& encodedPrivKey,
                                    uint8_t *sharedSecretBuf, uint16_t sharedSecretBufSize, uint16_t& sharedSecretLen)
{
    WEAVE_ERROR err;
    EC_GROUP *ecGroup = NULL;
    EC_POINT *pubKey = NULL;
    BIGNUM *privKey = NULL;

    err = GetECGroupForCurve(curveOID, ecGroup);
    SuccessOrExit(err);

    err = DecodeX962ECPoint(encodedPubKey.ECPoint, encodedPubKey.ECPointLen, ecGroup, pubKey);
    SuccessOrExit(err);

    privKey = BN_bin2bn(encodedPrivKey.PrivKey, encodedPrivKey.PrivKeyLen, NULL);
    VerifyOrExit(privKey != NULL, err = WEAVE_ERROR_NO_MEMORY);

    err = ECDHComputeSharedSecret(curveOID, ecGroup, pubKey, privKey, sharedSecretBuf, sharedSecretBufSize, sharedSecretLen);
    SuccessOrExit(err);

exit:
    BN_clear_free(privKey);
    EC_POINT_free(pubKey);
    EC_GROUP_free(ecGroup);

    return err;
}

int GetCurveSize(const OID curveOID)
{
    int curveSize = 0;
    EC_GROUP *ecGroup = NULL;

    if (GetECGroupForCurve(curveOID, ecGroup) == WEAVE_NO_ERROR)
        curveSize = GetCurveSize(curveOID, ecGroup);

    EC_GROUP_free(ecGroup);

    return curveSize;
}

WEAVE_ERROR GetCurveG(OID curveOID, EncodedECPublicKey& encodedG)
{
    WEAVE_ERROR err;
    EC_GROUP *ecGroup = NULL;

    err = GetECGroupForCurve(curveOID, ecGroup);
    SuccessOrExit(err);

    err = EncodeX962ECPoint(curveOID, ecGroup, EC_GROUP_get0_generator(ecGroup), encodedG.ECPoint, encodedG.ECPointLen, encodedG.ECPointLen);
    SuccessOrExit(err);

exit:
    EC_GROUP_free(ecGroup);

    return err;
}

#endif // WEAVE_CONFIG_USE_OPENSSL_ECC


// ============================================================
// OpenSSL-specific elliptic curve utility functions.
// ============================================================

// ECDSA_SIG_get0() and ECDSA_SIG_set0() methods were introduced in OpenSSL
// version 1.1.0; for older versions of OpenSSL these methods are defined here.
#if (OPENSSL_VERSION_NUMBER < 0x10100000L)

static void ECDSA_SIG_get0(const ECDSA_SIG *sig, const BIGNUM **pr, const BIGNUM **ps)
{
    if (pr != NULL)
        *pr = sig->r;
    if (ps != NULL)
        *ps = sig->s;
}

static int ECDSA_SIG_set0(ECDSA_SIG *sig, BIGNUM *r, BIGNUM *s)
{
    if (r == NULL || s == NULL)
        return 0;
    BN_clear_free(sig->r);
    BN_clear_free(sig->s);
    sig->r = r;
    sig->s = s;
    return 1;
}

#endif // (OPENSSL_VERSION_NUMBER < 0x10100000L)

int GetCurveSize(const OID curveOID, const EC_GROUP *ecGroup)
{
    return ((EC_GROUP_get_degree(ecGroup) + 7) / 8);
}

NL_DLL_EXPORT WEAVE_ERROR GetECGroupForCurve(OID curveOID, EC_GROUP *& ecGroup)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    int curveNID;

    switch (curveOID)
    {
    case kOID_EllipticCurve_secp160r1:
        curveNID = NID_secp160r1;
        break;
    case kOID_EllipticCurve_prime192v1:
        curveNID = NID_X9_62_prime192v1;
        break;
    case kOID_EllipticCurve_secp224r1:
        curveNID = NID_secp224r1;
        break;
    case kOID_EllipticCurve_prime256v1:
        curveNID = NID_X9_62_prime256v1;
        break;
    default:
#if WEAVE_CONFIG_ALLOW_NON_STANDARD_ELLIPTIC_CURVES
    {
        ASN1_OBJECT *curveASN1Obj;
        const uint8_t *encodedOID;
        uint16_t encodedOIDLen;
        if (!GetEncodedObjectID(curveOID, encodedOID, encodedOIDLen))
            ExitNow(err = WEAVE_ERROR_UNSUPPORTED_ELLIPTIC_CURVE);
        curveASN1Obj = c2i_ASN1_OBJECT(&curveASN1Obj, &encodedOID, encodedOIDLen);
        VerifyOrExit(curveASN1Obj != NULL, err = WEAVE_ERROR_NO_MEMORY);
        curveNID = OBJ_obj2nid(curveASN1Obj);
        ASN1_OBJECT_free(curveASN1Obj)
        VerifyOrExit(curveNID != NID_undef, err = WEAVE_ERROR_UNSUPPORTED_ELLIPTIC_CURVE);
    }
#endif
    case kOID_Unknown:
        ExitNow(err = WEAVE_ERROR_UNSUPPORTED_ELLIPTIC_CURVE);
        break;
    }

    ecGroup = EC_GROUP_new_by_curve_name(curveNID);
    VerifyOrExit(ecGroup != NULL, err = WEAVE_ERROR_UNSUPPORTED_ELLIPTIC_CURVE);

    // Only include the curve name when generating an ASN.1 encoding of a public key.
    EC_GROUP_set_asn1_flag(ecGroup, OPENSSL_EC_NAMED_CURVE);

exit:
    return err;
}

// Perform the Elliptic Curve Diffie-Hellman computation to generate a shared secret
// from a EC public key and a private key.
NL_DLL_EXPORT WEAVE_ERROR ECDHComputeSharedSecret(OID curveOID, const EC_GROUP *ecGroup,
                                    const EC_POINT *pubKeyPoint, const BIGNUM *privKeyBN,
                                    uint8_t *sharedSecretBuf, uint16_t sharedSecretBufSize, uint16_t& sharedSecretLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    EC_POINT *sharedSecretPoint = NULL;
    BIGNUM *sharedSecretX = NULL;
    BIGNUM *sharedSecretY = NULL;
    int sharedSecretXLen;

    // Determine the output size of the shared key in bytes.  This is equal to the size
    // of the curve prime.
    sharedSecretLen = GetCurveSize(curveOID, ecGroup);
    VerifyOrExit(sharedSecretLen != 0, err = WEAVE_ERROR_UNSUPPORTED_ELLIPTIC_CURVE);
    VerifyOrExit(sharedSecretLen <= sharedSecretBufSize, err = WEAVE_ERROR_BUFFER_TOO_SMALL);

    // Create a EC_POINT object to hold the shared key point.
    sharedSecretPoint = EC_POINT_new(ecGroup);
    VerifyOrExit(sharedSecretPoint != NULL, err = WEAVE_ERROR_NO_MEMORY); // TODO: translate OpenSSL error

    // Multiply the public key point by the private key number to produce the shared key point.
    // (This is the crux of the EC Diffie-Hellman algorithm).
    if (!EC_POINT_mul(ecGroup, sharedSecretPoint, NULL, pubKeyPoint, privKeyBN, NULL))
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT); // TODO: translate OpenSSL error

    // Extract the coordinate values from the shared key point.
    sharedSecretX = BN_new();
    VerifyOrExit(sharedSecretX != NULL, err = WEAVE_ERROR_NO_MEMORY); // TODO: translate OpenSSL error

    sharedSecretY = BN_new();
    VerifyOrExit(sharedSecretX != NULL, err = WEAVE_ERROR_NO_MEMORY); // TODO: translate OpenSSL error

    if (!EC_POINT_get_affine_coordinates_GFp(ecGroup, sharedSecretPoint, sharedSecretX, sharedSecretY, NULL))
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT); // TODO: translate OpenSSL error

    // Determine the size in bytes of the shared key X coordinate.
    sharedSecretXLen = BN_num_bytes(sharedSecretX);
    VerifyOrExit(sharedSecretXLen <= (int)sharedSecretLen, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Convert the shared key X coordinate to a big-endian byte array, padded on the left with 0s
    // to the shared key output size.
    memset(sharedSecretBuf, 0, sharedSecretLen);
    if (sharedSecretXLen != (int)BN_bn2bin(sharedSecretX, sharedSecretBuf + sharedSecretLen - sharedSecretXLen))
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT); // TODO: translate OpenSSL error

exit:
    BN_clear_free(sharedSecretX);
    BN_clear_free(sharedSecretY);
    EC_POINT_clear_free(sharedSecretPoint);

    return err;
}

NL_DLL_EXPORT WEAVE_ERROR EncodeX962ECPoint(OID curveOID, EC_GROUP *ecGroup, const EC_POINT *point, uint8_t *buf, uint16_t bufSize, uint16_t& encodedPointLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    BIGNUM *x = NULL;
    BIGNUM *y = NULL;
    int fieldLen, valLen;

    // Encoding point at infinity not supported.
    VerifyOrExit(!EC_POINT_is_at_infinity(ecGroup, point), err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Determine the encoded size of the point's fields. This is equal to the size of the curve prime.
    fieldLen = GetCurveSize(curveOID, ecGroup);
    VerifyOrExit(fieldLen != 0, err = WEAVE_ERROR_UNSUPPORTED_ELLIPTIC_CURVE);

    // Verify that the buffer is big enough.
    encodedPointLen = 1 + (fieldLen * 2);
    VerifyOrExit(bufSize >= encodedPointLen, err = WEAVE_ERROR_BUFFER_TOO_SMALL);

    // Get the point's coordinates.
    x = BN_new();
    VerifyOrExit(x != NULL, err = WEAVE_ERROR_NO_MEMORY);

    y = BN_new();
    VerifyOrExit(y != NULL, err = WEAVE_ERROR_NO_MEMORY);

    if (!EC_POINT_get_affine_coordinates_GFp(ecGroup, point, x, y, NULL))
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT); // TODO: translate OpenSSL error

    // Clear the output buffer. This primes it with zeros in case one of the coordinates
    // is less than fieldLen long, in which case the zeros act as padding.
    memset(buf, 0, encodedPointLen);

    // Encode the format byte (0x04 = uncompressed point).
    buf[0] = kX963EncodedPointFormat_Uncompressed;

    // Encode the X value right aligned in the field length.
    valLen = BN_num_bytes(x);
    VerifyOrExit(valLen <= fieldLen, err = WEAVE_ERROR_INVALID_ARGUMENT);

    BN_bn2bin(x, &buf[1 + fieldLen - valLen]);

    // Encode the Y value, also right aligned.
    valLen = BN_num_bytes(y);
    VerifyOrExit(valLen <= fieldLen, err = WEAVE_ERROR_INVALID_ARGUMENT);

    BN_bn2bin(y, &buf[1 + fieldLen + fieldLen - valLen]);

exit:
    BN_free(x);
    BN_free(y);

    return err;
}

NL_DLL_EXPORT WEAVE_ERROR DecodeX962ECPoint(const uint8_t *encodedPoint, uint16_t encodedPointLen, EC_GROUP *group, EC_POINT *& point)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    BIGNUM *x = NULL;
    BIGNUM *y = NULL;

    point = NULL;

    err = DecodeX962ECPoint(encodedPoint, encodedPointLen, x, y);
    SuccessOrExit(err);

    point = EC_POINT_new(group);
    VerifyOrExit(point != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT); // TODO: translate OpenSSL error

    // TODO: validate point is on curve

    if (!EC_POINT_set_affine_coordinates_GFp(group, point, x, y, NULL))
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT); // TODO: translate OpenSSL error

exit:
    if (err != WEAVE_NO_ERROR && point != NULL)
    {
        EC_POINT_free(point);
        point = NULL;
    }
    BN_free(x);
    BN_free(y);

    return err;
}

WEAVE_ERROR DecodeX962ECPoint(const uint8_t *encodedPoint, uint16_t encodedPointLen, BIGNUM *& x, BIGNUM *& y)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint16_t fieldSize;

    x = y = NULL;

    VerifyOrExit(encodedPointLen >= 3, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Verify that the encoding is an uncompressed point (0x04). Point-at-infinity (0x00) and compressed
    // points (0x02, 0x03) are not supported.
    VerifyOrExit(*encodedPoint == kX963EncodedPointFormat_Uncompressed, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Length must be odd (encoding byte plus x and y coordinates, each the same size).
    VerifyOrExit((encodedPointLen & 1) == 1, err = WEAVE_ERROR_INVALID_ARGUMENT);

    fieldSize = (encodedPointLen - 1) / 2;

    x = BN_bin2bn(encodedPoint + 1, fieldSize, NULL);
    VerifyOrExit(x != NULL, err = WEAVE_ERROR_NO_MEMORY);

    y = BN_bin2bn(encodedPoint + 1 + fieldSize, fieldSize, NULL);
    VerifyOrExit(x != NULL, err = WEAVE_ERROR_NO_MEMORY);

exit:
    if (err != WEAVE_NO_ERROR)
    {
        BN_free(x);
        x = NULL;
        BN_free(y);
        y = NULL;
    }

    return err;
}

// Decode an elliptic curve private key or/and public key in X9.62 format, and return an EC_KEY object.
NL_DLL_EXPORT WEAVE_ERROR DecodeECKey(OID curveOID, const EncodedECPrivateKey *encodedPrivKey, const EncodedECPublicKey *encodedPubKey, EC_KEY *& ecKey)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    EC_GROUP *ecGroup = NULL;
    EC_POINT *pubKeyPoint = NULL;
    BIGNUM *privKeyBN = NULL;
    int res;

    ecKey = NULL;

    // Verify that at least one of the (public/private) key inputs is provided.
    VerifyOrExit(encodedPrivKey != NULL || encodedPubKey != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // TODO: optimize this to eliminate duplication of the EC_POINT and EC_GROUP objects.

    err = GetECGroupForCurve(curveOID, ecGroup);
    SuccessOrExit(err);

    ecKey = EC_KEY_new();
    VerifyOrExit(ecKey != NULL, err = WEAVE_ERROR_NO_MEMORY);

    res = EC_KEY_set_group(ecKey, ecGroup);
    VerifyOrExit(res, err = WEAVE_ERROR_NO_MEMORY);

    if (encodedPubKey != NULL)
    {
        VerifyOrExit(encodedPubKey->ECPoint != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

        err = DecodeX962ECPoint(encodedPubKey->ECPoint, encodedPubKey->ECPointLen, ecGroup, pubKeyPoint);
        SuccessOrExit(err);

        res = EC_KEY_set_public_key(ecKey, pubKeyPoint);
        VerifyOrExit(res, err = WEAVE_ERROR_NO_MEMORY);
    }

    if (encodedPrivKey != NULL)
    {
        VerifyOrExit(encodedPrivKey->PrivKey != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

        privKeyBN = BN_bin2bn(encodedPrivKey->PrivKey, encodedPrivKey->PrivKeyLen, NULL);
        VerifyOrExit(privKeyBN != NULL, err = WEAVE_ERROR_NO_MEMORY);

        res = EC_KEY_set_private_key(ecKey, privKeyBN);
        VerifyOrExit(res, err = WEAVE_ERROR_NO_MEMORY);
    }

exit:
    BN_free(privKeyBN);
    EC_POINT_free(pubKeyPoint);
    if (err != WEAVE_NO_ERROR)
    {
        EC_KEY_free(ecKey);
        ecKey = NULL;
    }
    EC_GROUP_free(ecGroup);

    return err;
}

// Encode an OpenSSL ECDSA signature into a pair of buffers.
WEAVE_ERROR EncodeECDSASignature(const ECDSA_SIG *sig, EncodedECDSASignature& encodedSig)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    const BIGNUM *sigR;
    const BIGNUM *sigS;
    int valLen;

    ECDSA_SIG_get0(sig, &sigR, &sigS);

    valLen = BN_num_bytes(sigR);
    VerifyOrExit(valLen <= encodedSig.RLen, err = WEAVE_ERROR_BUFFER_TOO_SMALL);

    BN_bn2bin(sigR, encodedSig.R);

    if ((encodedSig.R[0] & 0x80) != 0)
    {
        VerifyOrExit(valLen < encodedSig.RLen, err = WEAVE_ERROR_BUFFER_TOO_SMALL);
        memmove(encodedSig.R + 1, encodedSig.R, valLen);
        encodedSig.R[0] = 0;
        encodedSig.RLen = valLen + 1;
    }
    else
        encodedSig.RLen = valLen;

    valLen = BN_num_bytes(sigS);
    VerifyOrExit(valLen <= encodedSig.SLen, err = WEAVE_ERROR_BUFFER_TOO_SMALL);

    BN_bn2bin(sigS, encodedSig.S);

    if ((encodedSig.S[0] & 0x80) != 0)
    {
        VerifyOrExit(valLen < encodedSig.SLen, err = WEAVE_ERROR_BUFFER_TOO_SMALL);
        memmove(encodedSig.S + 1, encodedSig.S, valLen);
        encodedSig.S[0] = 0;
        encodedSig.SLen = valLen + 1;
    }
    else
        encodedSig.SLen = valLen;

exit:
    return err;
}

// Decode an ECDSA signature consisting of r and s values encoded as a big-endian sequences of bytes.
WEAVE_ERROR DecodeECDSASignature(const EncodedECDSASignature& encodedSig, ECDSA_SIG *& sig)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    BIGNUM *sigR = NULL;
    BIGNUM *sigS = NULL;
    int retVal;

    sig = ECDSA_SIG_new();
    VerifyOrExit(sig != NULL, err = WEAVE_ERROR_NO_MEMORY);

    sigR = BN_bin2bn(encodedSig.R, encodedSig.RLen, sigR);
    VerifyOrExit(sigR != NULL, err = WEAVE_ERROR_NO_MEMORY);

    sigS = BN_bin2bn(encodedSig.S, encodedSig.SLen, sigS);
    VerifyOrExit(sigS != NULL, err = WEAVE_ERROR_NO_MEMORY);

    retVal = ECDSA_SIG_set0(sig, sigR, sigS);
    VerifyOrExit(retVal != 0, err = WEAVE_ERROR_NO_MEMORY);

exit:
    if (err != WEAVE_NO_ERROR && sig != NULL)
    {
        ECDSA_SIG_free(sig);
        sig = NULL;
    }

    return err;
}

// Convert an OpenSSL ECDSA signature into a fixed-length signature.
WEAVE_ERROR ECDSASigToFixedLenSig(OID curveOID, const ECDSA_SIG *ecSig, uint8_t *fixedLenSig)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    const BIGNUM *sigR;
    const BIGNUM *sigS;
    uint16_t fieldLen;
    int valLen;

    fieldLen = GetCurveSize(curveOID);
    VerifyOrExit(fieldLen != 0, err = WEAVE_ERROR_UNSUPPORTED_ELLIPTIC_CURVE);

    ECDSA_SIG_get0(ecSig, &sigR, &sigS);

    // Clear the output buffer. This primes it with zeros in case one of the signature
    // companents {r, s} is less than fieldLen long, in which case the zeros act as padding.
    memset(fixedLenSig, 0, 2 * fieldLen);

    // Encode the R value right aligned in the field length.
    valLen = BN_num_bytes(sigR);
    VerifyOrExit(valLen <= fieldLen, err = WEAVE_ERROR_INVALID_ARGUMENT);

    BN_bn2bin(sigR, &fixedLenSig[fieldLen - valLen]);

    // Encode the S value, also right aligned.
    valLen = BN_num_bytes(sigS);
    VerifyOrExit(valLen <= fieldLen, err = WEAVE_ERROR_INVALID_ARGUMENT);

    BN_bn2bin(sigS, &fixedLenSig[2 * fieldLen - valLen]);

exit:
    return err;
}

// Convert a fixed-length signature into an OpenSSL ECDSA signature format.
extern WEAVE_ERROR FixedLenSigToECDSASig(OID curveOID, const uint8_t *fixedLenSig, ECDSA_SIG *& ecSig)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    BIGNUM *sigR = NULL;
    BIGNUM *sigS = NULL;
    int retVal;
    uint16_t fieldLen;

    ecSig = ECDSA_SIG_new();
    VerifyOrExit(ecSig != NULL, err = WEAVE_ERROR_NO_MEMORY);

    fieldLen = GetCurveSize(curveOID);
    VerifyOrExit(fieldLen != 0, err = WEAVE_ERROR_UNSUPPORTED_ELLIPTIC_CURVE);

    sigR = BN_bin2bn(fixedLenSig, fieldLen, sigR);
    VerifyOrExit(sigR != NULL, err = WEAVE_ERROR_NO_MEMORY);

    sigS = BN_bin2bn(fixedLenSig + fieldLen, fieldLen, sigS);
    VerifyOrExit(sigS != NULL, err = WEAVE_ERROR_NO_MEMORY);

    retVal = ECDSA_SIG_set0(ecSig, sigR, sigS);
    VerifyOrExit(retVal != 0, err = WEAVE_ERROR_NO_MEMORY);

exit:
    if (err != WEAVE_NO_ERROR && ecSig != NULL)
    {
        ECDSA_SIG_free(ecSig);
        ecSig = NULL;
    }

    return err;
}


// ============================================================
// Elliptic Curve JPAKE Class Functions
// ============================================================

#if WEAVE_IS_ECJPAKE_ENABLED && WEAVE_CONFIG_USE_OPENSSL_ECC

static int GetCurveWordCount(const EC_GROUP *ecGroup)
{
    int curveNID = EC_GROUP_get_curve_name(ecGroup);

    switch (curveNID)
    {
#if WEAVE_CONFIG_SUPPORT_ELLIPTIC_CURVE_SECP160R1
    case NID_secp160r1:
        return 5;
#endif

#if WEAVE_CONFIG_SUPPORT_ELLIPTIC_CURVE_SECP192R1
    case NID_X9_62_prime192v1:
        return 6;
#endif

#if WEAVE_CONFIG_SUPPORT_ELLIPTIC_CURVE_SECP224R1
    case NID_secp224r1:
        return 7;
#endif

#if WEAVE_CONFIG_SUPPORT_ELLIPTIC_CURVE_SECP256R1
    case NID_X9_62_prime256v1:
        return 8;
#endif

    default:
#if WEAVE_CONFIG_ALLOW_NON_STANDARD_ELLIPTIC_CURVES
        return ((EC_GROUP_get_degree(ecGroup) + 31) / 32);
#else
        return 0;
#endif
    }
}

static int GetOrderWordCount(const EC_GROUP *ecGroup)
{
    int curveNID = EC_GROUP_get_curve_name(ecGroup);

    switch (curveNID)
    {
#if WEAVE_CONFIG_SUPPORT_ELLIPTIC_CURVE_SECP160R1
    case NID_secp160r1:
        return 6;
#endif

#if WEAVE_CONFIG_SUPPORT_ELLIPTIC_CURVE_SECP192R1
    case NID_X9_62_prime192v1:
        return 6;
#endif

#if WEAVE_CONFIG_SUPPORT_ELLIPTIC_CURVE_SECP224R1
    case NID_secp224r1:
        return 7;
#endif

#if WEAVE_CONFIG_SUPPORT_ELLIPTIC_CURVE_SECP256R1
    case NID_X9_62_prime256v1:
        return 8;
#endif

    default:
#if WEAVE_CONFIG_ALLOW_NON_STANDARD_ELLIPTIC_CURVES
        BIGNUM *order = NULL;
        int ret = 0;
        if ((order = BN_new()) == NULL) goto err;
        if (!EC_GROUP_get_order(ecGroup, order, NULL)) goto err;
        ret = order->top;

    err:
        BN_free(order);
        return ret;
#else
        return 0;
#endif
    }
}

static WEAVE_ERROR EncodeECPointValue(const EC_GROUP *ecGroup, const EC_POINT *ecPoint, const uint8_t wordCount, uint8_t *& p)
{
    WEAVE_ERROR err;
    BIGNUM *ecPointX = NULL;
    BIGNUM *ecPointY = NULL;

    ecPointX = BN_new();
    VerifyOrExit(ecPointX != NULL, err = WEAVE_ERROR_NO_MEMORY);
    ecPointY = BN_new();
    VerifyOrExit(ecPointY != NULL, err = WEAVE_ERROR_NO_MEMORY);

    if (!EC_POINT_get_affine_coordinates_GFp(ecGroup, ecPoint, ecPointX, ecPointY, NULL))
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);

    err = EncodeBIGNUMValueLE(*ecPointX, wordCount * sizeof(uint32_t), p);
    SuccessOrExit(err);
    err = EncodeBIGNUMValueLE(*ecPointY, wordCount * sizeof(uint32_t), p);
    SuccessOrExit(err);

exit:
    BN_free(ecPointY);
    BN_free(ecPointX);
    return err;
}

static WEAVE_ERROR DecodeECPointValue(const EC_GROUP *ecGroup, EC_POINT *ecPoint, const uint8_t wordCount, const uint8_t *& p)
{
    WEAVE_ERROR err;
    BIGNUM *ecPointX = NULL;
    BIGNUM *ecPointY = NULL;

    ecPointX = BN_new();
    VerifyOrExit(ecPointX != NULL, err = WEAVE_ERROR_NO_MEMORY);
    ecPointY = BN_new();
    VerifyOrExit(ecPointY != NULL, err = WEAVE_ERROR_NO_MEMORY);

    err = DecodeBIGNUMValueLE(*ecPointX, wordCount * sizeof(uint32_t), p);
    SuccessOrExit(err);
    err = DecodeBIGNUMValueLE(*ecPointY, wordCount * sizeof(uint32_t), p);
    SuccessOrExit(err);

    if (!EC_POINT_set_affine_coordinates_GFp(ecGroup, ecPoint, ecPointX, ecPointY, NULL))
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);

exit:
    BN_free(ecPointY);
    BN_free(ecPointX);
    return err;
}

static WEAVE_ERROR EncodeStepPartFields(const ECJPAKE_CTX *ctx, const ECJPAKE_STEP_PART *stepPart, const uint8_t *buf, const uint16_t bufSize, uint16_t& stepDataLen)
{
    WEAVE_ERROR err;
    uint8_t gWordCount, bWordCount;
    uint8_t *p = (uint8_t *)buf + stepDataLen;

    gWordCount = GetCurveWordCount(ECJPAKE_get_ecGroup(ctx));
    VerifyOrExit(gWordCount != 0, err = WEAVE_ERROR_UNSUPPORTED_ELLIPTIC_CURVE);

    bWordCount = GetOrderWordCount(ECJPAKE_get_ecGroup(ctx));
    VerifyOrExit(bWordCount != 0, err = WEAVE_ERROR_UNSUPPORTED_ELLIPTIC_CURVE);

    stepDataLen += (4 * gWordCount + bWordCount) * sizeof(uint32_t);
    VerifyOrExit(stepDataLen <= bufSize, err = WEAVE_ERROR_BUFFER_TOO_SMALL);

    err = EncodeECPointValue(ECJPAKE_get_ecGroup(ctx), stepPart->Gx, gWordCount, p);
    SuccessOrExit(err);
    err = EncodeECPointValue(ECJPAKE_get_ecGroup(ctx), stepPart->zkpx.Gr, gWordCount, p);
    SuccessOrExit(err);
    err = EncodeBIGNUMValueLE(*stepPart->zkpx.b, bWordCount * sizeof(uint32_t), p);
    SuccessOrExit(err);

exit:
    return err;
}

static WEAVE_ERROR DecodeStepPartFields(const ECJPAKE_CTX *ctx, ECJPAKE_STEP_PART *stepPart, const uint8_t *buf, const uint16_t bufSize, uint16_t& stepDataLen)
{
    WEAVE_ERROR err;
    uint8_t gWordCount, bWordCount;
    const uint8_t *p = buf + stepDataLen;

    gWordCount = GetCurveWordCount(ECJPAKE_get_ecGroup(ctx));
    VerifyOrExit(gWordCount != 0, err = WEAVE_ERROR_UNSUPPORTED_ELLIPTIC_CURVE);

    bWordCount = GetOrderWordCount(ECJPAKE_get_ecGroup(ctx));
    VerifyOrExit(bWordCount != 0, err = WEAVE_ERROR_UNSUPPORTED_ELLIPTIC_CURVE);

    stepDataLen += (4 * gWordCount + bWordCount) * sizeof(uint32_t);
    VerifyOrExit(stepDataLen <= bufSize, err = WEAVE_ERROR_BUFFER_TOO_SMALL);

    err = DecodeECPointValue(ECJPAKE_get_ecGroup(ctx), stepPart->Gx, gWordCount, p);
    SuccessOrExit(err);
    err = DecodeECPointValue(ECJPAKE_get_ecGroup(ctx), stepPart->zkpx.Gr, gWordCount, p);
    SuccessOrExit(err);
    err = DecodeBIGNUMValueLE(*stepPart->zkpx.b, bWordCount * sizeof(uint32_t), p);
    SuccessOrExit(err);

exit:
    return err;
}

/* This function will be used by ECJPAKE to calculate SHA256 hash of Elliptic Curve Point */
static int ECJPAKE_HashECPoint(ECJPAKE_CTX *ctx, SHA256_CTX *sha, const EC_POINT *ecPoint)
{
    WEAVE_ERROR err;
    uint8_t fieldWordCount;
    uint8_t *ecPointEncoded = NULL;
    uint8_t *p = NULL;
    int ret = 1;

    fieldWordCount = GetCurveWordCount(ECJPAKE_get_ecGroup(ctx));
    VerifyOrExit(fieldWordCount != 0, ret = 0);

    ecPointEncoded = (uint8_t *)OPENSSL_malloc(2 * sizeof(uint32_t) * fieldWordCount);
    VerifyOrExit(ecPointEncoded != NULL, ret = 0);

    // Assign another pointer because the value is updated by EncodeECPointValue function
    p = ecPointEncoded;
    err = EncodeECPointValue(ECJPAKE_get_ecGroup(ctx), ecPoint, fieldWordCount, p);
    VerifyOrExit(err == WEAVE_NO_ERROR, ret = 0);

    SHA256_Update(sha, ecPointEncoded, 2 * sizeof(uint32_t) * fieldWordCount);

exit:
    if (ecPointEncoded != NULL) OPENSSL_free(ecPointEncoded);
    return ret;
}

void EllipticCurveJPAKE::Init()
{
    ECJPAKECtx = NULL;
}

/* clear secret content of JPAKE CTX Struct */
void EllipticCurveJPAKE::Shutdown()
{
    Reset();
}

/* clear secret content of JPAKE CTX Struct */
void EllipticCurveJPAKE::Reset()
{
    if (ECJPAKECtx != NULL)
    {
        EC_GROUP *ecGroup = (EC_GROUP *)ECJPAKE_get_ecGroup(ECJPAKECtx);

        if (ecGroup != NULL)
        {
            EC_GROUP_free(ecGroup);
        }

        ECJPAKE_CTX_free(ECJPAKECtx);
        ECJPAKECtx = NULL;
    }
}

/* Init JPAKE CTX Struct */
WEAVE_ERROR EllipticCurveJPAKE::Init(const OID curveOID, const uint8_t *pw, const uint16_t pwLen,
                                                         const uint8_t *localName, const uint16_t localNameLen,
                                                         const uint8_t *peerName, const uint16_t peerNameLen)
{
    WEAVE_ERROR err;
    EC_GROUP *ecGroup = NULL;
    BIGNUM *secret = NULL;

    // TODO: should we limit MAX length and check these inputs: pwLen, localNameLen, peerNameLen ?

    err = GetECGroupForCurve(curveOID, ecGroup);
    SuccessOrExit(err);

    secret = BN_new();
    VerifyOrExit(secret != NULL, err = WEAVE_ERROR_NO_MEMORY);

    BN_bin2bn(pw, pwLen, secret);

    ECJPAKECtx = ECJPAKE_CTX_new(ecGroup, secret, localName, localNameLen, peerName, peerNameLen);
    VerifyOrExit(ECJPAKECtx != NULL, err = WEAVE_ERROR_NO_MEMORY);

    // Set ECJPAKE_HashECPoint function to be used by ECJPAKE for EC Point Hash
    ECJPAKE_Set_HashECPoint(ECJPAKE_HashECPoint);

exit:
    BN_clear_free(secret);
    return err;
}

WEAVE_ERROR EllipticCurveJPAKE::GenerateStep1(const uint8_t *buf, const uint16_t bufSize, uint16_t& stepDataLen)
{
    WEAVE_ERROR err;
    ECJPAKE_STEP1 step1Data;
    int res;

    res = ECJPAKE_STEP1_init(&step1Data, ECJPAKECtx);
    VerifyOrExit(res, err = WEAVE_ERROR_NO_MEMORY);

    res = ECJPAKE_STEP1_generate(&step1Data, ECJPAKECtx);
    VerifyOrExit(res, err = WEAVE_ERROR_NO_MEMORY);

    err = EncodeStepPartFields(ECJPAKECtx, &step1Data.p1, buf, bufSize, stepDataLen);
    SuccessOrExit(err);

    err = EncodeStepPartFields(ECJPAKECtx, &step1Data.p2, buf, bufSize, stepDataLen);
    SuccessOrExit(err);

exit:
    ECJPAKE_STEP1_release(&step1Data);
    return err;
}

WEAVE_ERROR EllipticCurveJPAKE::ProcessStep1(const uint8_t *buf, const uint16_t bufSize, uint16_t& stepDataLen)
{
    WEAVE_ERROR err;
    ECJPAKE_STEP1 step1Data;
    int res;

    res = ECJPAKE_STEP1_init(&step1Data, ECJPAKECtx);
    VerifyOrExit(res, err = WEAVE_ERROR_NO_MEMORY);

    err = DecodeStepPartFields(ECJPAKECtx, &step1Data.p1, buf, bufSize, stepDataLen);
    SuccessOrExit(err);

    err = DecodeStepPartFields(ECJPAKECtx, &step1Data.p2, buf, bufSize, stepDataLen);
    SuccessOrExit(err);

    res = ECJPAKE_STEP1_process(ECJPAKECtx, &step1Data);
    VerifyOrExit(res, err = WEAVE_ERROR_INVALID_ARGUMENT);

exit:
    ECJPAKE_STEP1_release(&step1Data);
    return err;
}

WEAVE_ERROR EllipticCurveJPAKE::GenerateStep2(const uint8_t *buf, const uint16_t bufSize, uint16_t& stepDataLen)
{
    WEAVE_ERROR err;
    ECJPAKE_STEP2 step2Data;
    int res;

    res = ECJPAKE_STEP2_init(&step2Data, ECJPAKECtx);
    VerifyOrExit(res, err = WEAVE_ERROR_NO_MEMORY);

    res = ECJPAKE_STEP2_generate(&step2Data, ECJPAKECtx);
    VerifyOrExit(res, err = WEAVE_ERROR_NO_MEMORY);

    err = EncodeStepPartFields(ECJPAKECtx, &step2Data, buf, bufSize, stepDataLen);
    SuccessOrExit(err);

exit:
    ECJPAKE_STEP2_release(&step2Data);
    return err;
}

WEAVE_ERROR EllipticCurveJPAKE::ProcessStep2(const uint8_t *buf, const uint16_t bufSize, uint16_t& stepDataLen)
{
    WEAVE_ERROR err;
    ECJPAKE_STEP2 step2Data;
    int res;

    res = ECJPAKE_STEP2_init(&step2Data, ECJPAKECtx);
    VerifyOrExit(res, err = WEAVE_ERROR_NO_MEMORY);

    err = DecodeStepPartFields(ECJPAKECtx, &step2Data, buf, bufSize, stepDataLen);
    SuccessOrExit(err);

    res = ECJPAKE_STEP2_process(ECJPAKECtx, &step2Data);
    VerifyOrExit(res, err = WEAVE_ERROR_INVALID_ARGUMENT);

exit:
    ECJPAKE_STEP2_release(&step2Data);
    return err;
}

uint8_t *EllipticCurveJPAKE::GetSharedSecret()
{
    return (uint8_t *)ECJPAKE_get_shared_key(ECJPAKECtx);
}

int EllipticCurveJPAKE::GetCurveSize()
{
    return (4 * GetCurveWordCount(ECJPAKE_get_ecGroup(ECJPAKECtx)));
}

#endif /* WEAVE_IS_ECJPAKE_ENABLED && WEAVE_CONFIG_USE_OPENSSL_ECC */

} // namespace Crypto
} // namespace Weave
} // namespace nl

#endif /* WEAVE_WITH_OPENSSL */
