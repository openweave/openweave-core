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
 *      This file implements an object for decoding, verifying, and
 *      accessing a Weave device provisioning bundle, consisting of a
 *      certificate, private key, and pairing (entry) code.
 *
 */

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif
#include <stdint.h>

#include <Weave/Core/WeaveCore.h>

#if WEAVE_CONFIG_ENABLE_PROVISIONING_BUNDLE_SUPPORT

#include <Weave/Core/WeaveEncoding.h>
#include <Weave/Support/crypto/WeaveCrypto.h>
#include <Weave/Support/crypto/HMAC.h>
#include <Weave/Support/verhoeff/Verhoeff.h>
#include <Weave/Profiles/security/WeaveSecurity.h>
#include <Weave/Profiles/security/WeaveCert.h>
#include <Weave/Profiles/security/WeavePrivateKey.h>
#include <Weave/Profiles/security/WeaveProvBundle.h>
#include <Weave/Support/ASN1.h>
#include <Weave/Support/CodeUtils.h>
#include <Weave/Support/Base64.h>

#if WEAVE_CONFIG_ENABLE_PROVISIONING_BUNDLE_SUPPORT && !WEAVE_WITH_OPENSSL
#error "INVALID WEAVE CONFIG: Provisioning bundle feature enabled but OpenSSL not available (WEAVE_CONFIG_ENABLE_PROVISIONING_BUNDLE_SUPPORT == 1 && WEAVE_WITH_OPENSSL == 0)."
#endif

#include <openssl/evp.h>
#include <openssl/err.h>

// Weave Provisioning Bundle Format
//                                                Included
// Field              Size           Encrypted    in MAC
//
// Version            2 Byte         N            Y
// Certificate Len    2 Bytes        Y            Y
// Private Key Len    2 Bytes        Y            Y
// Pairing Code Len   2 Bytes        Y            Y
// Device Id/MAC Addr 8 Bytes        Y            Y
// Certificate        variable       Y            Y
// Private Key        variable       Y            Y
// Pairing Code       variable       Y            Y
// MAC                32 bytes       Y            N
// Encryption Padding variable       Y            N
// Encryption IV      32 bytes       N            N
//
// Encryption algorithm is AES-256-CBC with PKCS5 padding.
// MAC algorithm is HMAC-SHA256.
// Encryption and MAC keys are derived from master key using PBDKF2-SHA1, 1000 iterations with fixed salt.
// All numeric values (lengths, MAC address) are encoded little-endian.

namespace nl {
namespace Weave {
namespace Profiles {
namespace Security {

using namespace nl::Weave::Encoding;
using namespace nl::Weave::Crypto;
using namespace nl::Weave::ASN1;

enum {
    kProvisioningBundleVersion = 1,
    kProvisioningBundleKDFIters = 1000
};

const char gProvisioningBundleKDFSalt[] = "Weave Provisioning Bundle v1";
#define gProvisioningBundleKDFSaltLen (sizeof(gProvisioningBundleKDFSalt) - 1)

static WEAVE_ERROR TranslateOpenSSLError(WEAVE_ERROR defaultErr)
{
    if (ERR_GET_REASON(ERR_get_error()) == ERR_R_MALLOC_FAILURE)
        return WEAVE_ERROR_NO_MEMORY;
    return defaultErr;
}

WEAVE_ERROR WeaveProvisioningBundle::Verify(uint64_t expectedDeviceId)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveCertificateSet certSet;
    WeaveCertificateData *certData;
    uint32_t weaveCurveId;
    EncodedECPublicKey pubKey;
    EncodedECPrivateKey privKey;

    // Initialize the certificate set to hold a single certificate.
    err = certSet.Init(1, 1024);
    SuccessOrExit(err);

    // Load the device certificate contained in the bundle.
    err = certSet.LoadCert(Certificate, CertificateLen, 0, certData);
    SuccessOrExit(err);

    // Verify the certificate is indeed a device certificate.
    VerifyOrExit(certData->SubjectDN.AttrOID == kOID_AttributeType_WeaveDeviceId, err = WEAVE_ERROR_INVALID_PROVISIONING_BUNDLE);

    // Verify the certificate is identifies the expected device id.
    VerifyOrExit(certData->SubjectDN.AttrValue.WeaveId == expectedDeviceId, err = WEAVE_ERROR_INVALID_PROVISIONING_BUNDLE);

    // Verify the certificate contains a supported public key type.
    VerifyOrExit(certData->PubKeyAlgoOID == kOID_PubKeyAlgo_ECPublicKey, err = WEAVE_ERROR_INVALID_PROVISIONING_BUNDLE);

    // Decode the private key contained in the bundle.
    err = DecodeWeaveECPrivateKey(PrivateKey, PrivateKeyLen, weaveCurveId, pubKey, privKey);
    SuccessOrExit(err);

    // Verify that the EC curve used by the public key in certificate matches that used
    // by the private key.
    VerifyOrExit(certData->PubKeyCurveId == weaveCurveId, err = WEAVE_ERROR_INVALID_PROVISIONING_BUNDLE);

    // Verify that the public key in the certificate matches that in the private key.
    VerifyOrExit(certData->PublicKey.EC.IsEqual(pubKey), err = WEAVE_ERROR_INVALID_PROVISIONING_BUNDLE);

    // Verify the check digit in the pairing code.
    VerifyOrExit(Verhoeff32::ValidateCheckChar(PairingCode, PairingCodeLen) == true, err = WEAVE_ERROR_INVALID_PROVISIONING_BUNDLE);

exit:
    certSet.Release();
    return err;
}

WEAVE_ERROR WeaveProvisioningBundle::Decode(uint8_t *provBundleBuf, uint32_t provBundleLen, const char *masterKey, uint32_t masterKeyLen, WeaveProvisioningBundle &provBundle)
{
    enum {
        kEncryptKeySize         = 32,
        kMACKeySize             = 32,
        kVersionFieldSize       = 2,
        kFixedHeaderSize        = 2 + 2 + 2 + 8, // cert len + priv key len + pairing code len + device id
        kMACFieldSize           = HMACSHA256::kDigestLength,
        kIVSize                 = 32,
    };
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    EVP_CIPHER_CTX *decryptCtx;
    HMACSHA256 hmac;
    uint8_t expectedMAC[kMACFieldSize];
    uint8_t keyMaterial[kEncryptKeySize + kMACKeySize];
    uint32_t encryptedDataLen, decryptedDataLen, expectedDecryptedDataLen, decryptBufSize;
    uint8_t *decryptBuf = NULL, *decodePoint = provBundleBuf;
    uint16_t version;
    int res, outLen;

    provBundle.Clear();

    decryptCtx = EVP_CIPHER_CTX_new();
    VerifyOrExit(decryptCtx != NULL, err = WEAVE_ERROR_NO_MEMORY);

    EVP_CIPHER_CTX_init(decryptCtx);

    // Un-base64 the provisioning bundle, storing the resultant data back into the input buffer.
    provBundleLen = Base64Decode((char *)provBundleBuf, (uint16_t)provBundleLen, (uint8_t *)provBundleBuf);
    VerifyOrExit(provBundleLen != UINT16_MAX, err = WEAVE_ERROR_INVALID_PROVISIONING_BUNDLE);

    // Verify the provided data is at least big enough to hold the version field and the initialization vector.
    VerifyOrExit(provBundleLen > kVersionFieldSize + kIVSize, err = WEAVE_ERROR_INVALID_PROVISIONING_BUNDLE);

    // Verify the version is supported.
    version = LittleEndian::Read16(decodePoint);
    VerifyOrExit(version == kProvisioningBundleVersion, err = WEAVE_ERROR_INVALID_PROVISIONING_BUNDLE);

    // Allocate a temporary buffer to hold the decrypted data.
    decryptBufSize = provBundleLen;
    decryptBuf = (uint8_t *)malloc(decryptBufSize);

    // Generate the encryption and MAC keys from the master key.
    res = PKCS5_PBKDF2_HMAC(masterKey, (int)masterKeyLen,
                            (const unsigned char *)gProvisioningBundleKDFSalt, gProvisioningBundleKDFSaltLen,
                            kProvisioningBundleKDFIters,
                            EVP_sha1(),
                            sizeof(keyMaterial), keyMaterial);
    if (res != 1)
        ExitNow(err = TranslateOpenSSLError(WEAVE_ERROR_PROVISIONING_BUNDLE_DECRYPTION_ERROR));

    // Initialize the OpenSSL cipher context for decryption using the generated encryption key and
    // the initialization vector from the bundle.
    res = EVP_CipherInit_ex(decryptCtx, EVP_aes_256_cbc(), NULL,
                            keyMaterial,
                            (uint8_t *)provBundleBuf + (provBundleLen - kIVSize),
                            0);
    if (res != 1)
        ExitNow(err = TranslateOpenSSLError(WEAVE_ERROR_PROVISIONING_BUNDLE_DECRYPTION_ERROR));

    // Compute the length of the encrypted portion of the bundle.
    encryptedDataLen = provBundleLen - (kVersionFieldSize + kIVSize);

    // Pass the encrypted data to the decryption function, storing the output in the decryption buffer.
    outLen = provBundleLen;
    res = EVP_CipherUpdate(decryptCtx, decryptBuf, &outLen, decodePoint, (int)encryptedDataLen);
    if (res != 1)
        ExitNow(err = TranslateOpenSSLError(WEAVE_ERROR_PROVISIONING_BUNDLE_DECRYPTION_ERROR));
    decryptedDataLen = outLen;

    // Finalize the decryption process by decrypting and verifying the padding.  Note that EVP_EncryptFinal_ex()
    // does not tell us how long the padding is (although it obviously knows), so decryptedDataLen should be
    // the same as encryptedDataLen.
    outLen = decryptBufSize - decryptedDataLen;
    res = EVP_CipherFinal_ex(decryptCtx, decryptBuf + decryptedDataLen, &outLen);
    if (res != 1)
        ExitNow(err = TranslateOpenSSLError(WEAVE_ERROR_PROVISIONING_BUNDLE_DECRYPTION_ERROR));
    decryptedDataLen += outLen;

    // Verify that the decrypted data is smaller than the encrypted data (this should always be the case because of padding).
    VerifyOrExit(decryptedDataLen < encryptedDataLen, err = WEAVE_ERROR_INVALID_PROVISIONING_BUNDLE);

    // Copy the decrypted data from the temporary buffer back into the input buffer, overwriting
    // the encrypted data in the process.
    memcpy(decodePoint, decryptBuf, decryptedDataLen);

    // Verify that decrypted data is at least as big as the fixed portion of the header.
    VerifyOrExit(decryptedDataLen > kFixedHeaderSize, err = WEAVE_ERROR_INVALID_PROVISIONING_BUNDLE);

    // Decode the fields of the provisioning bundle into the provided object.
    provBundle.CertificateLen = LittleEndian::Read16(decodePoint);
    provBundle.PrivateKeyLen = LittleEndian::Read16(decodePoint);
    provBundle.PairingCodeLen = LittleEndian::Read16(decodePoint);
    provBundle.WeaveDeviceId = LittleEndian::Read64(decodePoint);
    provBundle.Certificate = decodePoint; decodePoint += provBundle.CertificateLen;
    provBundle.PrivateKey = decodePoint; decodePoint += provBundle.PrivateKeyLen;
    provBundle.PairingCode = (const char *)decodePoint; decodePoint += provBundle.PairingCodeLen;

    // Verify that the size of the data within the provisioning bundle matches the length of the data
    // returned by decryption.
    expectedDecryptedDataLen = kFixedHeaderSize + provBundle.CertificateLen + provBundle.PrivateKeyLen + provBundle.PairingCodeLen + kMACFieldSize;
    VerifyOrExit(decryptedDataLen == expectedDecryptedDataLen, err = WEAVE_ERROR_INVALID_PROVISIONING_BUNDLE);

    // Recompute the MAC using the generated MAC key.
    hmac.Begin(keyMaterial + kEncryptKeySize, kMACKeySize);
    hmac.AddData(provBundleBuf, kVersionFieldSize + kFixedHeaderSize + provBundle.CertificateLen + provBundle.PrivateKeyLen + provBundle.PairingCodeLen);
    hmac.Finish(expectedMAC);

    // Fail if the MAC supplied in the bundle does not match the computed MAC.
    if (!ConstantTimeCompare(decodePoint, expectedMAC, kMACFieldSize))
        ExitNow(err = WEAVE_ERROR_INVALID_PROVISIONING_BUNDLE);

exit:
    if (decryptBuf != NULL)
        free(decryptBuf);
    if (decryptCtx != NULL)
        EVP_CIPHER_CTX_free(decryptCtx);
    return err;
}

void WeaveProvisioningBundle::Clear()
{
    WeaveDeviceId = 0;
    Certificate = NULL;
    CertificateLen = 0;
    PrivateKey = NULL;
    PrivateKeyLen = 0;
    PairingCode = NULL;
    PairingCodeLen = 0;
}

} // namespace Security
} // namespace Profiles
} // namespace Weave
} // namespace nl


#endif // WEAVE_CONFIG_ENABLE_PROVISIONING_BUNDLE_SUPPORT
