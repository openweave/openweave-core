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
 *      This file implements utility functions for reading, parsing,
 *      encoding, and decoding Weave key material.
 *
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "weave-tool.h"
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Support/crypto/EllipticCurve.h>
#include <Weave/Profiles/security/WeaveSecurity.h>
#include <Weave/Profiles/security/WeavePrivateKey.h>

using namespace nl::Weave::Profiles;
using namespace nl::Weave::Profiles::Security;
using namespace nl::Weave::ASN1;
using namespace nl::Weave::TLV;
using namespace nl::Weave::Crypto;

bool ReadPrivateKey(const char *fileName, const char *prompt, EVP_PKEY *& key)
{
    bool res = true;
    uint8_t *keyData = NULL;
    uint32_t keyDataLen;

    key = NULL;

    res = ReadFileIntoMem(fileName, keyData, keyDataLen);
    if (!res)
        ExitNow();

    res = DecodePrivateKey(keyData, keyDataLen, kKeyFormat_Unknown, fileName, prompt, key);

exit:
    if (keyData != NULL)
        free(keyData);
    return res;
}

bool ReadPublicKey(const char *fileName, EVP_PKEY *& key)
{
    bool res = true;
    uint8_t *keyData = NULL;
    uint32_t keyDataLen;

    key = NULL;
    res = ReadFileIntoMem(fileName, keyData, keyDataLen);
    if (!res)
        ExitNow();
    res = DecodePublicKey(keyData, keyDataLen, kKeyFormat_Unknown, fileName, key);

exit:
  if (keyData != NULL)
      free(keyData);
  return res;
}

bool ReadWeavePrivateKey(const char *fileName, uint8_t *& key, uint32_t &keyLen)
{
    bool res = true;
    KeyFormat keyFormat;

    key = NULL;
    keyLen = 0;

    res = ReadFileIntoMem(fileName, key, keyLen);
    if (!res)
        ExitNow();

    if (keyLen > MAX_KEY_SIZE)
    {
        fprintf(stderr, "weave: Error reading %s\nKey too large\n", fileName);
        ExitNow(res = false);
    }

    keyFormat = DetectKeyFormat(key, keyLen);
    if (keyFormat == kKeyFormat_Weave_Base64)
    {
        uint8_t *tmpKeyBuf = Base64Decode(key, keyLen, NULL, 0, keyLen);
        if (tmpKeyBuf == NULL)
            ExitNow(res = false);
        free(key);
        key = tmpKeyBuf;
    }
    else if (keyFormat != kKeyFormat_Weave_Raw)
    {
        fprintf(stderr, "weave: Error reading %s\nUnsupported private key format\n", fileName);
        ExitNow(res = false);
    }

exit:
    if (!res && key != NULL)
    {
        free(key);
        key = NULL;
        keyLen = 0;
    }
    return res;
}

WEAVE_ERROR DecodeWeavePrivateKey(const uint8_t *encodedKeyBuf, uint32_t encodedKeyLen, EVP_PKEY *& key)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TLVReader reader;
    uint64_t tag;

    reader.Init(encodedKeyBuf, encodedKeyLen);

    err = reader.Next();
    SuccessOrExit(err);

    VerifyOrExit(reader.GetType() == kTLVType_Structure, err = WEAVE_ERROR_INVALID_ARGUMENT);

    tag = reader.GetTag();

    if (tag == ProfileTag(kWeaveProfile_Security, kTag_EllipticCurvePrivateKey))
    {
        uint32_t weaveCurveId;
        EC_KEY *ecKey;
        EncodedECPublicKey pubKey;
        EncodedECPrivateKey privKey;

        err = DecodeWeaveECPrivateKey(encodedKeyBuf, encodedKeyLen, weaveCurveId, pubKey, privKey);
        SuccessOrExit(err);

        err = DecodeECKey(WeaveCurveIdToOID(weaveCurveId), &privKey, &pubKey, ecKey);
        SuccessOrExit(err);

        key = EVP_PKEY_new();
        VerifyOrExit(key != NULL, err = WEAVE_ERROR_NO_MEMORY);

        EVP_PKEY_assign_EC_KEY(key, ecKey);
    }

    else if (tag == ProfileTag(kWeaveProfile_Security, kTag_RSAPrivateKey))
    {
        // TODO: implement support for RSA private keys.
        ExitNow(err = WEAVE_ERROR_NOT_IMPLEMENTED);
    }

    else
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);

exit:
    return err;
}

bool DecodePrivateKey(const uint8_t *keyData, uint32_t keyDataLen, KeyFormat keyFormat, const char *keySource, const char *prompt, EVP_PKEY *& key)
{
    bool res = true;
    uint8_t *tmpKeyBuf = NULL;
    BIO *keyBIO = NULL;

    key = NULL;

    if (keyFormat == kKeyFormat_Unknown)
        keyFormat = DetectKeyFormat(keyData, keyDataLen);

    if (keyFormat == kKeyFormat_Weave_Base64)
    {
        tmpKeyBuf = Base64Decode(keyData, keyDataLen, NULL, 0, keyDataLen);
        if (tmpKeyBuf == NULL)
            goto exit;
        keyData = tmpKeyBuf;
        keyFormat = kKeyFormat_Weave_Raw;
    }

    if (keyFormat == kKeyFormat_Weave_Raw)
    {
        WEAVE_ERROR err;

        err = DecodeWeavePrivateKey(keyData, keyDataLen, key);
        if (err != WEAVE_NO_ERROR)
        {
            fprintf(stderr, "Failed to decode Weave private key %s: %s\n", keySource, nl::ErrorStr(err));
            ExitNow(res = false);
        }
    }

    else
    {
#ifndef OPENSSL_IS_BORINGSSL
        EVP_set_pw_prompt(prompt);
#endif

        keyBIO = BIO_new_mem_buf((void *)keyData, keyDataLen);
        if (keyBIO == NULL)
        {
            fprintf(stderr, "Memory allocation error\n");
            ExitNow(res = false);
        }

        if (keyFormat == kKeyFormat_PEM || keyFormat == kKeyFormat_PEM_PKCS8)
        {
            if (PEM_read_bio_PrivateKey(keyBIO, &key, NULL, NULL) == NULL)
            {
                fprintf(stderr, "Unable to read %s\n", keySource);
                ReportOpenSSLErrorAndExit("PEM_read_bio_PrivateKey", res = false);
            }
        }
        else
        {
            if (d2i_PrivateKey_bio(keyBIO, &key) == NULL)
            {
                fprintf(stderr, "Unable to read %s\n", keySource);
                ReportOpenSSLErrorAndExit("d2i_PrivateKey_bio", res = false);
            }
        }
    }

exit:
    if (tmpKeyBuf != NULL)
        free(tmpKeyBuf);
    if (keyBIO != NULL)
        BIO_free_all(keyBIO);
    return res;
}

bool DecodePublicKey(const uint8_t *keyData, uint32_t keyDataLen, KeyFormat keyFormat, const char *keySource, EVP_PKEY *& key)
{
    bool res = true;
    uint8_t *tmpKeyBuf = NULL;
    BIO *keyBIO = NULL;
    key = NULL;

    if (keyFormat == kKeyFormat_Unknown)
        keyFormat = DetectKeyFormat(keyData, keyDataLen);

    keyBIO = BIO_new_mem_buf((void *)keyData, keyDataLen);
    if (keyBIO == NULL)
    {
        fprintf(stderr, "Memory allocation error\n");
        ExitNow(res = false);
    }

    if (keyFormat == kKeyFormat_PEM)
    {
        if (PEM_read_bio_PUBKEY(keyBIO, &key, NULL, NULL) == NULL)
        {
            fprintf(stderr, "Unable to read %s\n", keySource);
            ReportOpenSSLErrorAndExit("PEM_read_bio_PUBKEY", res = false);
        }

    }
    else if (keyFormat == kKeyFormat_DER)
    {
        if (d2i_PUBKEY_bio(keyBIO, &key) == NULL)
        {
            fprintf(stderr, "Unable to read %s\n", keySource);
            ReportOpenSSLErrorAndExit("d2i_PUBKEY_bio", res = false);
	}
    }
    else
    {
        fprintf(stderr, "Key type not supported");
        ExitNow(res = false);
    }

exit:
    if (tmpKeyBuf != NULL)
        free(tmpKeyBuf);
    if (keyBIO != NULL)
        BIO_free_all(keyBIO);
    return res;
}

bool DetectKeyFormat(FILE *keyFile, KeyFormat& keyFormat)
{
    uint8_t keyData[16];
    int lenRead;

    fseek(keyFile, 0, SEEK_SET);

    lenRead = fread(keyData, 1, sizeof(keyData), keyFile);
    if (lenRead < 0)
    {
        // TODO: handle error
    }

    fseek(keyFile, 0, SEEK_SET);

    keyFormat = DetectKeyFormat(keyData, (uint32_t)lenRead);
    return true;
}

KeyFormat DetectKeyFormat(const uint8_t *key, uint32_t keyLen)
{
    static const uint8_t ecWeaveRawPrefix[] = { 0xD5, 0x00, 0x00, 0x04, 0x00, 0x02, 0x00 };
    static const char *ecWeaveB64Prefix = "1QAABAAC";
    static const uint32_t ecWeaveB64PrefixLen = sizeof(ecWeaveB64Prefix) - 1;
    static const char *ecPEMMarker = "-----BEGIN EC PRIVATE KEY-----";

    static const uint8_t rsaWeaveRawPrefix[] = { 0xD5, 0x00, 0x00, 0x04, 0x00, 0x03, 0x00 };
    static const char *rsaWeaveB64Prefix = "1QAABAAD";
    static const uint32_t rsaWeaveB64PrefixLen = sizeof(rsaWeaveB64Prefix) - 1;
    static const char *rsaPEMMarker = "-----BEGIN RSA PRIVATE KEY-----";

    static const char *pkcs8PEMMarker = "-----BEGIN PRIVATE KEY-----";

    static const char *ecPUBPEMMarker = "-----BEGIN PUBLIC KEY-----";

    if (keyLen > sizeof(ecWeaveRawPrefix) && memcmp(key, ecWeaveRawPrefix, sizeof(ecWeaveRawPrefix)) == 0)
        return kKeyFormat_Weave_Raw;

    if (keyLen > sizeof(rsaWeaveRawPrefix) && memcmp(key, rsaWeaveRawPrefix, sizeof(rsaWeaveRawPrefix)) == 0)
        return kKeyFormat_Weave_Raw;

    if (keyLen > ecWeaveB64PrefixLen && memcmp(key, ecWeaveB64Prefix, ecWeaveB64PrefixLen) == 0)
        return kKeyFormat_Weave_Base64;

    if (keyLen > rsaWeaveB64PrefixLen && memcmp(key, rsaWeaveB64Prefix, rsaWeaveB64PrefixLen) == 0)
        return kKeyFormat_Weave_Base64;

    if (ContainsPEMMarker(ecPEMMarker, key, keyLen))
        return kKeyFormat_PEM;

    if (ContainsPEMMarker(rsaPEMMarker, key, keyLen))
        return kKeyFormat_PEM;

    if (ContainsPEMMarker(pkcs8PEMMarker, key, keyLen))
        return kKeyFormat_PEM_PKCS8;

    if (ContainsPEMMarker(ecPUBPEMMarker, key, keyLen))
        return kKeyFormat_PEM;

    return kKeyFormat_DER;
}

bool GenerateKeyPair(const char *curveName, EVP_PKEY *& key)
{
    bool res = true;
    EC_KEY *ecKey = NULL;
    EC_GROUP *ecGroup = NULL;
    int curveNID;

    key = NULL;

    curveNID = OBJ_sn2nid(curveName);
    if (curveNID == 0)
    {
        fprintf(stderr, "Unknown elliptic curve: %s\n", curveName);
        ExitNow(res = false);
    }

    ecGroup = EC_GROUP_new_by_curve_name(curveNID);
    if (ecGroup == NULL)
        ReportOpenSSLErrorAndExit("EC_GROUP_new_by_curve_name", res = false);

    // Only include the curve name in the ASN.1 encoding of the public key.
    EC_GROUP_set_asn1_flag(ecGroup, OPENSSL_EC_NAMED_CURVE);

    ecKey = EC_KEY_new();
    if (ecKey == NULL)
        ReportOpenSSLErrorAndExit("EC_KEY_new", res = false);

    if (EC_KEY_set_group(ecKey, ecGroup) == 0)
        ReportOpenSSLErrorAndExit("EC_KEY_set_group", res = false);

    if (!EC_KEY_generate_key(ecKey))
        ReportOpenSSLErrorAndExit("EC_KEY_generate_key", res = false);

    key = EVP_PKEY_new();
    if (key == NULL)
        ReportOpenSSLErrorAndExit("EVP_PKEY_new", res = false);

    if (!EVP_PKEY_assign_EC_KEY(key, ecKey))
        ReportOpenSSLErrorAndExit("EVP_PKEY_assign_EC_KEY", res = false);

    ecKey = NULL;

exit:
    if (ecKey != NULL)
        EC_KEY_free(ecKey);
    if (key != NULL && !res)
    {
        EVP_PKEY_free(key);
        key = NULL;
    }
    return res;
}

bool EncodePrivateKey(EVP_PKEY *key, KeyFormat keyFormat, uint8_t *& encodedKey, uint32_t& encodedKeyLen)
{
    bool res = true;
    PKCS8_PRIV_KEY_INFO *pkcs8Key = NULL;
    EC_KEY *ecKey;
    BIO *outBIO = NULL;
    BUF_MEM *memBuf;

    encodedKey = NULL;

    if (EVP_PKEY_type(EVP_PKEY_id(key)) == EVP_PKEY_RSA)
    {
        fprintf(stderr, "Encoding of RSA private keys not yet supported\n");
        ExitNow(res = false);
    }
    if (EVP_PKEY_type(EVP_PKEY_id(key)) != EVP_PKEY_EC)
    {
        fprintf(stderr, "Unsupported private key type\n");
        ExitNow(res = false);
    }

    ecKey = EVP_PKEY_get1_EC_KEY(key);

    if (keyFormat == kKeyFormat_DER || keyFormat == kKeyFormat_DER_PKCS8 ||
        keyFormat == kKeyFormat_PEM || keyFormat == kKeyFormat_PEM_PKCS8)
    {
        outBIO = BIO_new(BIO_s_mem());
        if (outBIO == NULL)
        {
            fprintf(stderr, "Memory allocation error\n");
            ExitNow(res = false);
        }

        if (keyFormat == kKeyFormat_DER_PKCS8 || keyFormat == kKeyFormat_PEM_PKCS8)
        {
            pkcs8Key = EVP_PKEY2PKCS8(key);
            if (pkcs8Key == NULL)
            {
                fprintf(stderr, "Memory allocation error\n");
                ExitNow(res = false);
            }

            if (keyFormat == kKeyFormat_DER_PKCS8)
            {
                if (i2d_PKCS8_PRIV_KEY_INFO_bio(outBIO, pkcs8Key) < 0)
                    ReportOpenSSLErrorAndExit("i2d_PKCS8_PRIV_KEY_INFO_bio", res = false);
            }
            else
            {
                if (!PEM_write_bio_PKCS8_PRIV_KEY_INFO(outBIO, pkcs8Key))
                    ReportOpenSSLErrorAndExit("PEM_write_bio_PKCS8_PRIV_KEY_INFO", res = false);
            }
        }

        else
        {
            if (keyFormat == kKeyFormat_DER)
            {
                if (i2d_ECPrivateKey_bio(outBIO, ecKey) < 0)
                    ReportOpenSSLErrorAndExit("i2d_PrivateKey_bio", res = false);
            }
            else
            {
                if (!PEM_write_bio_ECPrivateKey(outBIO, ecKey, NULL, NULL, 0, NULL, NULL))
                    ReportOpenSSLErrorAndExit("PEM_write_bio_ECPrivateKey", res = false);
            }
        }

        BIO_get_mem_ptr(outBIO, &memBuf);

        encodedKey = (uint8_t *)malloc(memBuf->length);
        if (encodedKey == NULL)
        {
            fprintf(stderr, "Memory allocation error\n");
            ExitNow(res = false);
        }

        memcpy(encodedKey, memBuf->data, memBuf->length);
        encodedKeyLen = memBuf->length;
    }

    else
    {
        uint8_t *tmpEncodedKey;
        uint32_t tmpEncodedKeyLen;

        res = WeaveEncodeECPrivateKey(ecKey, true, tmpEncodedKey, tmpEncodedKeyLen);
        VerifyOrExit(res == true, (void)0);

        if (keyFormat == kKeyFormat_Weave_Raw)
        {
            encodedKey = tmpEncodedKey;
            encodedKeyLen = tmpEncodedKeyLen;
        }
        else
        {
            encodedKey = Base64Encode(tmpEncodedKey, tmpEncodedKeyLen, NULL, 0, encodedKeyLen);
            free(tmpEncodedKey);
            if (encodedKey == NULL)
            {
                fprintf(stderr, "Memory allocation failure\n");
                ExitNow(res = false);
            }
        }
    }

exit:
    if (outBIO != NULL)
        BIO_free(outBIO);
    if (pkcs8Key != NULL)
    {
        PKCS8_PRIV_KEY_INFO_free(pkcs8Key);
    }
    return res;
}

// TODO: remove this
bool WeaveEncodePrivateKey(EVP_PKEY *key, uint8_t *& encodedKey, uint32_t& encodedKeyLen)
{
    bool res = true;

    if (EVP_PKEY_type(EVP_PKEY_id(key)) == EVP_PKEY_RSA)
    {
        fprintf(stderr, "Encoding of RSA private keys not yet supported\n");
        ExitNow(res = false);
    }
    else if (EVP_PKEY_type(EVP_PKEY_id(key)) == EVP_PKEY_EC)
    {
        EC_KEY *ecKey = EVP_PKEY_get1_EC_KEY(key);
        res = WeaveEncodeECPrivateKey(ecKey, true, encodedKey, encodedKeyLen);
    }
    else
    {
        fprintf(stderr, "Unsupported private key type\n");
        ExitNow(res = false);
    }

exit:
    return res;
}

bool WeaveEncodeECPrivateKey(EC_KEY *key, bool includePubKey, uint8_t *& encodedKey, uint32_t& encodedKeyLen)
{
    bool res = true;
    WEAVE_ERROR err;
    EncodedECPublicKey encodedPubKey;
    EncodedECPrivateKey encodedPrivKey;
    uint32_t encodedKeyBufLen;
    OID curveOID;

    encodedPubKey.ECPoint = NULL;
    encodedPrivKey.PrivKey = NULL;
    encodedKey = NULL;

    curveOID = NIDToWeaveOID(EC_GROUP_get_curve_name(EC_KEY_get0_group(key)));
    if (curveOID == kOID_Unknown)
    {
        fprintf(stderr, "Unsupported elliptic curve: %d\n", EC_GROUP_get_curve_name(EC_KEY_get0_group(key)));
        ExitNow(res = false);
    }

    encodedPrivKey.PrivKeyLen = (uint16_t)BN_num_bytes(EC_KEY_get0_private_key(key));
    encodedPrivKey.PrivKey = (uint8_t *)malloc(encodedPrivKey.PrivKeyLen);
    if (encodedPrivKey.PrivKey == NULL)
    {
        fprintf(stderr, "Memory allocation failure\n");
        ExitNow(res = false);
    }
    if (!BN_bn2bin(EC_KEY_get0_private_key(key), encodedPrivKey.PrivKey))
        ReportOpenSSLErrorAndExit("BN_bn2bin", res = false);

    if (includePubKey)
    {
        encodedPubKey.ECPointLen = (uint32_t)EC_POINT_point2oct(EC_KEY_get0_group(key), EC_KEY_get0_public_key(key), POINT_CONVERSION_UNCOMPRESSED, NULL, 0, NULL);
        encodedPubKey.ECPoint = (uint8_t *)malloc(encodedPubKey.ECPointLen);
        if (encodedPubKey.ECPoint == NULL)
        {
            fprintf(stderr, "Memory allocation failure\n");
            ExitNow(res = false);
        }
        if (EC_POINT_point2oct(EC_KEY_get0_group(key), EC_KEY_get0_public_key(key), POINT_CONVERSION_UNCOMPRESSED, encodedPubKey.ECPoint, encodedPubKey.ECPointLen, NULL) == 0)
            ReportOpenSSLErrorAndExit("EC_POINT_point2oct", res = false);
    }
    else
        encodedPubKey.ECPointLen = 0;

    encodedKeyBufLen = 64 + encodedPrivKey.PrivKeyLen + encodedPubKey.ECPointLen;

    encodedKey = (uint8_t *)malloc(encodedKeyBufLen);
    if (encodedKey == NULL)
    {
        fprintf(stderr, "Memory allocation failure\n");
        ExitNow(res = false);
    }

    err = EncodeWeaveECPrivateKey(OIDToWeaveCurveId(curveOID), &encodedPubKey, encodedPrivKey, encodedKey, encodedKeyBufLen, encodedKeyLen);
    if (err != WEAVE_NO_ERROR)
    {
        fprintf(stderr, "Failed to Weave encode EC private key: %s\n", nl::ErrorStr(err));
        ExitNow(res = false);
    }

exit:
    if (encodedPrivKey.PrivKey != NULL)
        free(encodedPrivKey.PrivKey);
    if (encodedPubKey.ECPoint != NULL)
        free(encodedPubKey.ECPoint);
    if (encodedKey != NULL && !res)
    {
        free(encodedKey);
        encodedKey = NULL;
    }
    return res;
}
