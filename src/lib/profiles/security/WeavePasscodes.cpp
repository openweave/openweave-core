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
 *      This file implements interfaces for Weave passcodes encryption/decryption.
 *
 */

#include <Weave/Core/WeaveCore.h>
#include "WeavePasscodes.h"
#include "WeaveApplicationKeys.h"
#include <Weave/Core/WeaveEncoding.h>
#include <Weave/Support/crypto/WeaveCrypto.h>
#include <Weave/Support/CodeUtils.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace Security {
namespace Passcodes {

using namespace nl::Weave::Encoding;
using namespace nl::Weave::Crypto;
using namespace nl::Weave::Platform::Security;
using namespace nl::Weave::Profiles::Security::AppKeys;

typedef struct
{
    uint8_t config;
    uint8_t keyId[sizeof(uint32_t)];
    uint8_t nonce[sizeof(uint32_t)];
    uint8_t paddedPasscode[kPasscodePaddedLen];
    uint8_t authenticator[kPasscodeAuthenticatorLen];
    uint8_t fingerprint[kPasscodeFingerprintLen];
} EncryptedPasscodeStruct;

// Key diversifier used for Weave passcode encryption key derivation.
const uint8_t kPasscodeEncKeyDiversifier[] = { 0x1A, 0x65, 0x5D, 0x96 };

// Key diversifier used for Weave passcode encryption key derivation.
const uint8_t kPasscodeFingerprintKeyDiversifier[] = { 0xD1, 0xA1, 0xD9, 0x6C };

#if WEAVE_CONFIG_SUPPORT_PASSCODE_CONFIG1_TEST_ONLY

static void GeneratePasscodeFingerprint_Config1(EncryptedPasscodeStruct& encPasscodeStruct)
{
    SHA1 hash;
    uint8_t digest[hash.kHashLength];

    // Generate passcode fingerprint.
    hash.Begin();
    hash.AddData(encPasscodeStruct.paddedPasscode, sizeof(encPasscodeStruct.paddedPasscode));
    hash.Finish(digest);

    // Copy truncated digest to the fingerprint location in the output buffer.
    memcpy(encPasscodeStruct.fingerprint, digest, sizeof(encPasscodeStruct.fingerprint));
}

static void EncryptPasscode_Config1(EncryptedPasscodeStruct& encPasscodeStruct)
{
    SHA1 hash;
    uint8_t digest[hash.kHashLength];

    // Generate passcode authenticator.
    hash.Begin();
    hash.AddData(&encPasscodeStruct.config, sizeof(encPasscodeStruct.config));
    hash.AddData(encPasscodeStruct.nonce, sizeof(encPasscodeStruct.nonce) + sizeof(encPasscodeStruct.paddedPasscode));
    hash.Finish(digest);

    // Copy truncated digest to the authenticator location in the output buffer.
    memcpy(encPasscodeStruct.authenticator, digest, sizeof(encPasscodeStruct.authenticator));
}

static WEAVE_ERROR VerifyPasscodeFingerprint_Config1(const EncryptedPasscodeStruct& encPasscodeStruct)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    SHA1 hash;
    uint8_t digest[hash.kHashLength];
    int res;

    // Generate passcode fingerprint.
    hash.Begin();
    hash.AddData(encPasscodeStruct.paddedPasscode, sizeof(encPasscodeStruct.paddedPasscode));
    hash.Finish(digest);

    // Verify passcode fingerprint.
    res = memcmp(digest, encPasscodeStruct.fingerprint, sizeof(encPasscodeStruct.fingerprint));
    VerifyOrExit(res == 0, err = WEAVE_ERROR_PASSCODE_FINGERPRINT_FAILED);

exit:
    return err;
}

static WEAVE_ERROR DecryptPasscode_Config1(const EncryptedPasscodeStruct& encPasscodeStruct,
                                           uint8_t decryptedPasscode[kPasscodePaddedLen])
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    SHA1 hash;
    uint8_t digest[hash.kHashLength];
    int res;

    // Generate passcode authenticator.
    hash.Begin();
    hash.AddData(&encPasscodeStruct.config, sizeof(encPasscodeStruct.config));
    hash.AddData(encPasscodeStruct.nonce, sizeof(encPasscodeStruct.nonce) + sizeof(encPasscodeStruct.paddedPasscode));
    hash.Finish(digest);

    // Verify passcode authenticator.
    res = memcmp(digest, encPasscodeStruct.authenticator, sizeof(encPasscodeStruct.authenticator));
    VerifyOrExit(res == 0, err = WEAVE_ERROR_PASSCODE_AUTHENTICATION_FAILED);

    // Copy the passed passcode into the output buffer.
    memcpy(decryptedPasscode, encPasscodeStruct.paddedPasscode, kPasscodePaddedLen);

exit:
    return err;
}

#endif // WEAVE_CONFIG_SUPPORT_PASSCODE_CONFIG1_TEST_ONLY

#if WEAVE_CONFIG_SUPPORT_PASSCODE_CONFIG2

static void GeneratePasscodeFingerprint_Config2(const uint8_t *fingerprintKey, EncryptedPasscodeStruct& encPasscodeStruct)
{
    HMACSHA1 hmac;
    uint8_t digest[hmac.kDigestLength];

    // Generate passcode fingerprint.
    //
    // NOTE: this code assumes that encPasscodeStruct.paddedPasscode contains the *unencrypted* passcode.  Therefore
    // this function must be called before the encrypt function.
    //
    hmac.Begin(fingerprintKey, kPasscodeFingerprintKeyLen);
    hmac.AddData(encPasscodeStruct.paddedPasscode, sizeof(encPasscodeStruct.paddedPasscode));
    hmac.Finish(digest);

    // Copy truncated digest to the fingerprint location in the passcode structure.
    memcpy(encPasscodeStruct.fingerprint, digest, sizeof(encPasscodeStruct.fingerprint));
}

static void EncryptPasscode_Config2(const uint8_t *encKey, const uint8_t *authKey, EncryptedPasscodeStruct& encPasscodeStruct)
{
    AES128BlockCipherEnc aes128Enc;
    HMACSHA1 hmac;
    uint8_t digest[hmac.kDigestLength];

    // Encrypt padded passcode directly into the output buffer.
    aes128Enc.SetKey(encKey);
    aes128Enc.EncryptBlock(encPasscodeStruct.paddedPasscode, encPasscodeStruct.paddedPasscode);

    // Generate passcode authenticator.
    hmac.Begin(authKey, kPasscodeAuthenticationKeyLen);
    hmac.AddData(&encPasscodeStruct.config, sizeof(encPasscodeStruct.config));
    hmac.AddData(encPasscodeStruct.nonce, sizeof(encPasscodeStruct.nonce) + sizeof(encPasscodeStruct.paddedPasscode));
    hmac.Finish(digest);

    // Copy truncated digest to the authenticator location in the output buffer.
    memcpy(encPasscodeStruct.authenticator, digest, sizeof(encPasscodeStruct.authenticator));
}

static WEAVE_ERROR VerifyPasscodeFingerprint_Config2(const uint8_t *fingerprintKey, uint8_t passcode[kPasscodePaddedLen], const EncryptedPasscodeStruct& encPasscodeStruct)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    HMACSHA1 hmac;
    uint8_t digest[hmac.kDigestLength];
    int res;

    // Generate passcode fingerprint.
    hmac.Begin(fingerprintKey, kPasscodeFingerprintKeyLen);
    hmac.AddData(passcode, kPasscodePaddedLen);
    hmac.Finish(digest);

    // Verify passcode fingerprint.
    res = memcmp(digest, encPasscodeStruct.fingerprint, sizeof(encPasscodeStruct.fingerprint));
    VerifyOrExit(res == 0, err = WEAVE_ERROR_PASSCODE_FINGERPRINT_FAILED);

exit:
    return err;
}

static WEAVE_ERROR DecryptPasscode_Config2(const uint8_t *encKey, const uint8_t *authKey,
                                           const EncryptedPasscodeStruct& encPasscodeStruct,
                                           uint8_t decryptedPasscode[kPasscodePaddedLen])
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    AES128BlockCipherDec aes128Dec;
    HMACSHA1 hmac;
    uint8_t digest[hmac.kDigestLength];
    int res;

    // Generate passcode authenticator.
    hmac.Begin(authKey, kPasscodeAuthenticationKeyLen);
    hmac.AddData(&encPasscodeStruct.config, sizeof(encPasscodeStruct.config));
    hmac.AddData(encPasscodeStruct.nonce, sizeof(encPasscodeStruct.nonce) + sizeof(encPasscodeStruct.paddedPasscode));
    hmac.Finish(digest);

    // Verify passcode authenticator.
    res = memcmp(digest, encPasscodeStruct.authenticator, sizeof(encPasscodeStruct.authenticator));
    VerifyOrExit(res == 0, err = WEAVE_ERROR_PASSCODE_AUTHENTICATION_FAILED);

    // Decrypt padded passcode directly into the output buffer.
    aes128Dec.SetKey(encKey);
    aes128Dec.DecryptBlock(encPasscodeStruct.paddedPasscode, decryptedPasscode);

exit:
    return err;
}

#endif // WEAVE_CONFIG_SUPPORT_PASSCODE_CONFIG2

/**
 * Returns true if the supplied passcode encryption configuration is supported by the
 * passcode encryption/decryption APIs.
 */
bool IsSupportedPasscodeEncryptionConfig(uint8_t config)
{
    switch (config)
    {
#if WEAVE_CONFIG_SUPPORT_PASSCODE_CONFIG1_TEST_ONLY
    case kPasscode_Config1_TEST_ONLY:
        return true;
#endif
#if WEAVE_CONFIG_SUPPORT_PASSCODE_CONFIG2
    case kPasscode_Config2:
        return true;
#endif
    default:
        return false;
    }
}

/**
 * Get the configuration type of an encrypted passcode.
 *
 * @param[in]   encPasscode         Pointer to a buffer containing the encrypted passcode.
 * @param[in]   encPasscodeLen      Length of the encrypted passcode.
 * @param[out]  config              The Weave passcode encryption configuration used by the encrypted passcode.
 */
WEAVE_ERROR GetEncryptedPasscodeConfig(const uint8_t *encPasscode, size_t encPasscodeLen, uint8_t& config)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    const EncryptedPasscodeStruct &encPasscodeStruct = *(const EncryptedPasscodeStruct *)encPasscode;

    // Verify the encrypted passcode is the correct length.
    VerifyOrExit(encPasscodeLen == sizeof(EncryptedPasscodeStruct), err = WEAVE_ERROR_INVALID_ARGUMENT);

    config = encPasscodeStruct.config;

exit:
    return err;
}

/**
 * Get the id of the key used to encrypt an encrypted passcode.
 *
 * @param[in]   encPasscode         Pointer to a buffer containing the encrypted passcode.
 * @param[in]   encPasscodeLen      Length of the encrypted passcode.
 * @param[out]  keyId               The id of the key used to encrypt the encrypted passcode.
 */
WEAVE_ERROR GetEncryptedPasscodeKeyId(const uint8_t *encPasscode, size_t encPasscodeLen, uint32_t& keyId)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    const EncryptedPasscodeStruct &encPasscodeStruct = *(const EncryptedPasscodeStruct *)encPasscode;

    // Verify the encrypted passcode is the correct length.
    VerifyOrExit(encPasscodeLen == sizeof(EncryptedPasscodeStruct), err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Verify supported encryption config.
    VerifyOrExit(IsSupportedPasscodeEncryptionConfig(encPasscodeStruct.config), err = WEAVE_ERROR_UNSUPPORTED_PASSCODE_CONFIG);

    // Read and return the key id field.
    keyId = LittleEndian::Get32(encPasscodeStruct.keyId);

exit:
    return err;
}

/**
 * Get the nonce value associated with an encrypted passcode.
 *
 * @param[in]   encPasscode         Pointer to a buffer containing the encrypted passcode.
 * @param[in]   encPasscodeLen      Length of the encrypted passcode.
 * @param[out]  nonce               The nonce value associated with an encrypted passcode.
 */
WEAVE_ERROR GetEncryptedPasscodeNonce(const uint8_t *encPasscode, size_t encPasscodeLen, uint32_t& nonce)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    const EncryptedPasscodeStruct &encPasscodeStruct = *(const EncryptedPasscodeStruct *)encPasscode;

    // Verify the encrypted passcode is the correct length.
    VerifyOrExit(encPasscodeLen == sizeof(EncryptedPasscodeStruct), err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Verify supported encryption config.
    VerifyOrExit(IsSupportedPasscodeEncryptionConfig(encPasscodeStruct.config), err = WEAVE_ERROR_UNSUPPORTED_PASSCODE_CONFIG);

    // Read and return the nonce field.
    nonce = LittleEndian::Get32(encPasscodeStruct.nonce);

exit:
    return err;
}

/**
 * Get the fingerprint value associated with an encrypted passcode.
 *
 * @param[in]   encPasscode         Pointer to a buffer containing the encrypted passcode.
 * @param[in]   encPasscodeLen      Length of the encrypted passcode.
 * @param[in]   fingerprintBuf      A buffer to receive the fingerprint value.
 * @param[in]   fingerprintBufSize  The size of the buffer pointed at by fingerprintBuf.
 * @param[out]  fingerprintLen      The length of the returned fingerprint value.
 */
WEAVE_ERROR GetEncryptedPasscodeFingerprint(const uint8_t *encPasscode, size_t encPasscodeLen,
                                            uint8_t *fingerprintBuf, size_t fingerprintBufSize, size_t& fingerprintLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    const EncryptedPasscodeStruct &encPasscodeStruct = *(const EncryptedPasscodeStruct *)encPasscode;

    // Verify the encrypted passcode is the correct length.
    VerifyOrExit(encPasscodeLen == sizeof(EncryptedPasscodeStruct), err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Verify supported encryption config.
    VerifyOrExit(IsSupportedPasscodeEncryptionConfig(encPasscodeStruct.config), err = WEAVE_ERROR_UNSUPPORTED_PASSCODE_CONFIG);

    // Verify the supplied buffer is big enough to hold the fingerprint.
    VerifyOrExit(fingerprintBufSize >= kPasscodeFingerprintLen, err = WEAVE_ERROR_BUFFER_TOO_SMALL);

    // Return the fingerprint field.
    memcpy(fingerprintBuf, encPasscodeStruct.fingerprint, kPasscodeFingerprintLen);
    fingerprintLen = kPasscodeFingerprintLen;

exit:
    return err;
}

/**
 * Encrypt a passcode using the Nest Passcode Encryption scheme.
 *
 * @param[in]   config              The passcode encryption configuration to be used.
 * @param[in]   keyId               The requested passcode encryption key Id.
 * @param[in]   nonce               An unique value assigned to the encrypted passcode.
 * @param[in]   passcode            A pointer to the passcode to be encrypted.
 * @param[in]   passcodeLen         The passcode length.
 * @param[out]  encPasscode         A pointer to the buffer to store encrypted passcode.
 * @param[in]   encPasscodeBufSize  The size of the buffer for encrypted passcode storage.
 * @param[out]  encPasscodeLen      The encrypted passcode length.
 * @param[in]   groupKeyStore       A pointer to the group key store object.
 *
 * @retval #WEAVE_NO_ERROR       On success.
 * @retval #WEAVE_ERROR_UNSUPPORTED_PASSCODE_CONFIG
 *                               If specified passcode configuration is not supported.
 * @retval #WEAVE_ERROR_BUFFER_TOO_SMALL
 *                               If provided output buffer is too small for encrypted passcode.
 * @retval #WEAVE_ERROR_INVALID_KEY_ID
 *                               If the requested key has invalid key Id.
 * @retval #WEAVE_ERROR_INVALID_ARGUMENT
 *                               If the supplied passcode is too short or too long;
 *                               Or if pointer to the group key store is not provided
 *                               or platform key store returns invalid key parameters.
 * @retval other                 Other platform-specific errors returned by the platform
 *                               key store APIs.
 *
 */
WEAVE_ERROR EncryptPasscode(uint8_t config, uint32_t keyId, uint32_t nonce, const uint8_t *passcode, size_t passcodeLen,
                            uint8_t *encPasscode, size_t encPasscodeBufSize, size_t& encPasscodeLen,
                            GroupKeyStoreBase *groupKeyStore)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
#if WEAVE_CONFIG_SUPPORT_PASSCODE_CONFIG2
    uint8_t appKey[kPasscodeTotalDerivedKeyLen];
#endif
    EncryptedPasscodeStruct &encPasscodeStruct = *(EncryptedPasscodeStruct *)encPasscode;

    // Verify supported encryption config.
    VerifyOrExit(IsSupportedPasscodeEncryptionConfig(config), err = WEAVE_ERROR_UNSUPPORTED_PASSCODE_CONFIG);

    // Verify output buffer is large enough to store encrypted passcode.
    VerifyOrExit(encPasscodeBufSize >= sizeof(EncryptedPasscodeStruct), err = WEAVE_ERROR_BUFFER_TOO_SMALL);

    // Verify valid passcode length input.
    VerifyOrExit(passcodeLen > 0 && passcodeLen <= sizeof(encPasscodeStruct.paddedPasscode), err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Write the config field.
    encPasscodeStruct.config = config;

    // Write the nonce field.
    LittleEndian::Put32(encPasscodeStruct.nonce, nonce);

    // Pad passcode to the AES block size (16 bytes) directly into the output buffer.
    memset(encPasscodeStruct.paddedPasscode, 0, sizeof(encPasscodeStruct.paddedPasscode));
    memcpy(encPasscodeStruct.paddedPasscode, passcode, passcodeLen);

#if WEAVE_CONFIG_SUPPORT_PASSCODE_CONFIG1_TEST_ONLY
    if (config == kPasscode_Config1_TEST_ONLY)
    {
        // Verify the caller supplied the proper key id for config1.
        VerifyOrExit(keyId == kPasscodeConfig1_KeyId, err = WEAVE_ERROR_INVALID_KEY_ID);

        // Generate passcode fingerprint.
        GeneratePasscodeFingerprint_Config1(encPasscodeStruct);

        // "Encrypt" padded passcode and generate the passcode authenticator.
        EncryptPasscode_Config1(encPasscodeStruct);
    }
    else
#endif // WEAVE_CONFIG_SUPPORT_PASSCODE_CONFIG1_TEST_ONLY
    {
#if WEAVE_CONFIG_SUPPORT_PASSCODE_CONFIG2
        uint32_t fingerprintKeyId;
        uint8_t keyDiversifier[kPasscodeEncKeyDiversifierSize];
        uint32_t appGroupGlobalId;

        // Verify the group key store object is provided.
        VerifyOrExit(groupKeyStore != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

        // Set fingerprint key id (should be of static application key type).
        fingerprintKeyId = WeaveKeyId::ConvertToStaticAppKeyId(keyId);

        // Derive passcode fingerprint key.
        err = groupKeyStore->DeriveApplicationKey(fingerprintKeyId, NULL, 0,
                                                  kPasscodeFingerprintKeyDiversifier, kPasscodeFingerprintKeyDiversifierSize,
                                                  appKey, sizeof(appKey), kPasscodeFingerprintKeyLen, appGroupGlobalId);
        SuccessOrExit(err);

        // Generate passcode fingerprint.
        GeneratePasscodeFingerprint_Config2(appKey, encPasscodeStruct);

        // Prepare the passcode encryption and authentication key diversifier parameter.
        memcpy(keyDiversifier, kPasscodeEncKeyDiversifier, sizeof(kPasscodeEncKeyDiversifier));
        keyDiversifier[sizeof(kPasscodeEncKeyDiversifier)] = config;

        // Derive passcode encryption application key data.
        err = groupKeyStore->DeriveApplicationKey(keyId, encPasscodeStruct.nonce, sizeof(encPasscodeStruct.nonce),
                                                  keyDiversifier, kPasscodeEncKeyDiversifierSize,
                                                  appKey, sizeof(appKey), kPasscodeTotalDerivedKeyLen, appGroupGlobalId);
        SuccessOrExit(err);

        // Encrypt padded passcode and generate the passcode authenticator.
        EncryptPasscode_Config2(appKey, appKey + kPasscodeEncryptionKeyLen, encPasscodeStruct);

#endif // WEAVE_CONFIG_SUPPORT_PASSCODE_CONFIG2
    }

    // Write the key id field.
    // NOTE: we write the key id after encryption has taken place because the act of deriving the encryption keys
    // may have also resolved the key id to a more specific form.  E.g., a key id identifying the "current" epoch
    // key may have been resolved to a key id identifying the specific epoch key that is currently active.
    LittleEndian::Put32(encPasscodeStruct.keyId, keyId);

    // Set encrypted passcode length.
    encPasscodeLen = sizeof(EncryptedPasscodeStruct);

exit:
#if WEAVE_CONFIG_SUPPORT_PASSCODE_CONFIG2
    ClearSecretData(appKey, sizeof(appKey));
#endif

    if (err != WEAVE_NO_ERROR)
        ClearSecretData(encPasscode, encPasscodeBufSize);

    return err;
}

/**
 * Encrypt a passcode using the Nest Passcode Encryption scheme.
 *
 * @param[in]   config              The Weave passcode encryption configuration to be used.
 * @param[in]   keyId               The requested passcode encryption key Id.
 * @param[in]   nonce               An unique value assigned to the passcode.
 * @param[in]   passcode            A pointer to the passcode to be encrypted.
 * @param[in]   passcodeLen         The passcode length.
 * @param[in]   encKey              A pointer to the key to be used to encrypt the passcode.
 *                                  The length of the key must match the encryption algorithm
 *                                  associated with the specified configuration.
 * @param[in]   authKey             A pointer to the key to be used to authenticate the passcode.
 *                                  The length of the key must match the authentication algorithm
 *                                  associated with the specified configuration.
 * @param[in]   fingerprintKey      A pointer to the key to be used to generate the passcode
 *                                  fingerprint. The length of the key must match the fingerprint
 *                                  algorithm associated with the specified configuration.
 * @param[out]  encPasscode         A pointer to a buffer into which the encrypted passcode will
 *                                  be stored. This buffer must be at least kPasscodeMaxEncryptedLen
 *                                  in size.
 * @param[in]   encPasscodeBufSize  The size of the buffer pointed to by encPasscode.
 * @param[out]  encPasscodeLen      The encrypted passcode length.
 *
 * @retval #WEAVE_NO_ERROR       On success.
 * @retval #WEAVE_ERROR_UNSUPPORTED_PASSCODE_CONFIG
 *                               If specified passcode configuration is not supported.
 * @retval #WEAVE_ERROR_BUFFER_TOO_SMALL
 *                               If provided output buffer is too small for encrypted passcode.
 * @retval #WEAVE_ERROR_INVALID_ARGUMENT
 *                               If the supplied passcode is too short or too long.
 *
 */
WEAVE_ERROR EncryptPasscode(uint8_t config, uint32_t keyId, uint32_t nonce, const uint8_t *passcode, size_t passcodeLen,
                            const uint8_t *encKey, const uint8_t *authKey, const uint8_t *fingerprintKey,
                            uint8_t *encPasscode, size_t encPasscodeBufSize, size_t& encPasscodeLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    EncryptedPasscodeStruct &encPasscodeStruct = *(EncryptedPasscodeStruct *)encPasscode;

    // Verify supported passcode config.
    VerifyOrExit(IsSupportedPasscodeEncryptionConfig(config), err = WEAVE_ERROR_UNSUPPORTED_PASSCODE_CONFIG);

    // Verify output buffer is large enough to store encrypted passcode.
    VerifyOrExit(encPasscodeBufSize >= sizeof(EncryptedPasscodeStruct), err = WEAVE_ERROR_BUFFER_TOO_SMALL);

    // Verify valid passcode length input.
    VerifyOrExit(passcodeLen > 0 && passcodeLen <= kPasscodeMaxLen, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Write the config field.
    encPasscodeStruct.config = config;

    // Write the key id field.
    LittleEndian::Put32(encPasscodeStruct.keyId, keyId);

    // Write the nonce field.
    LittleEndian::Put32(encPasscodeStruct.nonce, nonce);

    // Pad passcode to the AES block size (16 bytes) directly into the output buffer.
    memset(encPasscodeStruct.paddedPasscode, 0, sizeof(encPasscodeStruct.paddedPasscode));
    memcpy(encPasscodeStruct.paddedPasscode, passcode, passcodeLen);

#if WEAVE_CONFIG_SUPPORT_PASSCODE_CONFIG1_TEST_ONLY
    if (config == kPasscode_Config1_TEST_ONLY)
    {
        // Generate passcode fingerprint.
        GeneratePasscodeFingerprint_Config1(encPasscodeStruct);

        // Encrypt padded passcode and generate the passcode authenticator.
        EncryptPasscode_Config1(encPasscodeStruct);
    }
    else
#endif // WEAVE_CONFIG_SUPPORT_PASSCODE_CONFIG1_TEST_ONLY
    {
#if WEAVE_CONFIG_SUPPORT_PASSCODE_CONFIG2

        // Generate passcode fingerprint.
        GeneratePasscodeFingerprint_Config2(fingerprintKey, encPasscodeStruct);

        // Encrypt padded passcode and generate the passcode authenticator.
        EncryptPasscode_Config2(encKey, authKey, encPasscodeStruct);

#endif // WEAVE_CONFIG_SUPPORT_PASSCODE_CONFIG2
    }

    // Return the encrypted passcode length.
    encPasscodeLen = sizeof(EncryptedPasscodeStruct);

exit:
    if (err != WEAVE_NO_ERROR)
        ClearSecretData(encPasscode, encPasscodeBufSize);
    return err;
}

/**
 * Decrypt a passcode that was encrypted using the Nest Passcode Encryption scheme.
 *
 * @param[in]   encPasscode         A pointer to the encrypted passcode buffer.
 * @param[in]   encPasscodeLen      The encrypted passcode length.
 * @param[in]   passcodeBuf         A pointer to a buffer to receive the decrypted passcode.
 * @param[in]   passcodeBufSize     The size of the buffer pointed at by passcodeBuf.
 * @param[out]  passcodeLen         Set to the length of the decrypted passcode.
 * @param[in]   groupKeyStore       A pointer to the group key store object.
 *
 * @retval #WEAVE_NO_ERROR       On success.
 * @retval #WEAVE_ERROR_UNSUPPORTED_PASSCODE_CONFIG
 *                               If specified passcode configuration is not supported.
 * @retval #WEAVE_ERROR_PASSCODE_AUTHENTICATION_FAILED
 *                               If passcode authentication fails.
 * @retval #WEAVE_ERROR_PASSCODE_FINGERPRINT_FAILED
 *                               If passcode fingerprint check fail-es.
 * @retval #WEAVE_ERROR_INVALID_KEY_ID
 *                               If the requested key has invalid key Id.
 * @retval #WEAVE_ERROR_BUFFER_TOO_SMALL
 *                               If the supplied passcode buffer is too small.
 * @retval #WEAVE_ERROR_INVALID_ARGUMENT
 *                               If the encrytped passcode is too short or too long;
 *                               Or if pointer to the group key store is not provided
 *                               or platform key store returns invalid key parameters.
 * @retval other                 Other platform-specific errors returned by the platform
 *                               key store APIs.
 *
 */
WEAVE_ERROR DecryptPasscode(const uint8_t *encPasscode, size_t encPasscodeLen,
                            uint8_t *passcodeBuf, size_t passcodeBufSize, size_t& passcodeLen,
                            GroupKeyStoreBase *groupKeyStore)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint32_t keyId;
#if WEAVE_CONFIG_SUPPORT_PASSCODE_CONFIG2
    uint8_t appKey[kPasscodeTotalDerivedKeyLen];
#endif
    const EncryptedPasscodeStruct &encPasscodeStruct = *(const EncryptedPasscodeStruct *)encPasscode;
    uint8_t decryptedPasscode[kPasscodePaddedLen];
    uint8_t *passcodeEnd;

    // Verify the encrypted passcode is the correct length.
    VerifyOrExit(encPasscodeLen == sizeof(EncryptedPasscodeStruct), err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Verify supported passcode config.
    VerifyOrExit(IsSupportedPasscodeEncryptionConfig(encPasscodeStruct.config), err = WEAVE_ERROR_UNSUPPORTED_PASSCODE_CONFIG);

    // Read the key id field.
    keyId = LittleEndian::Get32(encPasscodeStruct.keyId);

#if WEAVE_CONFIG_SUPPORT_PASSCODE_CONFIG1_TEST_ONLY
    if (encPasscodeStruct.config == kPasscode_Config1_TEST_ONLY)
    {
        // Verify correct key id.
        VerifyOrExit(keyId == kPasscodeConfig1_KeyId, err = WEAVE_ERROR_INVALID_KEY_ID);

        // "Decrypt" passcode and verify authenticator.
        err = DecryptPasscode_Config1(encPasscodeStruct, decryptedPasscode);
        SuccessOrExit(err);

        // Verify passcode fingerprint.
        err = VerifyPasscodeFingerprint_Config1(encPasscodeStruct);
        SuccessOrExit(err);
    }
    else
#endif // WEAVE_CONFIG_SUPPORT_PASSCODE_CONFIG1_TEST_ONLY
    {
#if WEAVE_CONFIG_SUPPORT_PASSCODE_CONFIG2
        uint8_t keyDiversifier[kPasscodeEncKeyDiversifierSize];
        uint32_t appGroupGlobalId;

        // Verify the group key store object is provided.
        VerifyOrExit(groupKeyStore != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

        // Set passcode encryption and authentication key diversifier parameter.
        memcpy(keyDiversifier, kPasscodeEncKeyDiversifier, sizeof(kPasscodeEncKeyDiversifier));
        keyDiversifier[sizeof(kPasscodeEncKeyDiversifier)] = encPasscodeStruct.config;

        // Derive passcode encryption application key data.
        err = groupKeyStore->DeriveApplicationKey(keyId, encPasscodeStruct.nonce, sizeof(encPasscodeStruct.nonce),
                                                  keyDiversifier, kPasscodeEncKeyDiversifierSize,
                                                  appKey, sizeof(appKey), kPasscodeTotalDerivedKeyLen, appGroupGlobalId);
        SuccessOrExit(err);

        // Decrypt and verify the passcode.
        err = DecryptPasscode_Config2(appKey, appKey + kPasscodeEncryptionKeyLen, encPasscodeStruct, decryptedPasscode);
        SuccessOrExit(err);

        // Set fingerprint key id (should be of static application key type).
        keyId = WeaveKeyId::ConvertToStaticAppKeyId(keyId);

        // Derive passcode fingerprint key.
        err = groupKeyStore->DeriveApplicationKey(keyId, NULL, 0,
                                                  kPasscodeFingerprintKeyDiversifier, kPasscodeFingerprintKeyDiversifierSize,
                                                  appKey, sizeof(appKey), kPasscodeFingerprintKeyLen, appGroupGlobalId);
        SuccessOrExit(err);

        // Verify the passcode fingerprint.
        err = VerifyPasscodeFingerprint_Config2(appKey, decryptedPasscode, encPasscodeStruct);
        SuccessOrExit(err);

#endif // WEAVE_CONFIG_SUPPORT_PASSCODE_CONFIG2
    }

    // Determine the length of the passcode.
    passcodeEnd = (uint8_t *)memchr(decryptedPasscode, 0, sizeof(decryptedPasscode));
    passcodeLen = (passcodeEnd != NULL) ? passcodeEnd - decryptedPasscode : sizeof(decryptedPasscode);

    // Verify the output buffer is large enough to hold the decrypted passcode.
    VerifyOrExit(passcodeBufSize >= passcodeLen, err = WEAVE_ERROR_BUFFER_TOO_SMALL);

    // Copy passcode to the output buffer.
    memcpy(passcodeBuf, decryptedPasscode, passcodeLen);

exit:
#if WEAVE_CONFIG_SUPPORT_PASSCODE_CONFIG2
    ClearSecretData(appKey, sizeof(appKey));
#endif
    ClearSecretData(decryptedPasscode, sizeof(decryptedPasscode));

    return err;
}

/**
 * Decrypt a passcode that was encrypted using the Nest Passcode Encryption scheme.
 *
 * @param[in]   encPasscode         A pointer to the encrypted passcode buffer.
 * @param[in]   encPasscodeLen      The encrypted passcode length.
 * @param[in]   encKey              A pointer to the key to be used to encrypt the passcode.
 *                                  The length of the key must match the encryption algorithm
 *                                  associated with the specified configuration.
 * @param[in]   authKey             A pointer to the key to be used to authenticate the passcode.
 *                                  The length of the key must match the authentication algorithm
 *                                  associated with the specified configuration.
 * @param[in]   fingerprintKey      A pointer to the key to be used to generate the passcode
 *                                  fingerprint. The length of the key must match the fingerprint
 *                                  algorithm associated with the specified configuration.
 * @param[in]   passcodeBuf         A pointer to a buffer to receive the decrypted passcode.
 * @param[in]   passcodeBufSize     The size of the buffer pointed at by passcodeBuf.
 * @param[out]  passcodeLen         Set to the length of the decrypted passcode.
 *
 * @retval #WEAVE_NO_ERROR       On success.
 * @retval #WEAVE_ERROR_UNSUPPORTED_PASSCODE_CONFIG
 *                               If specified passcode configuration is not supported.
 * @retval #WEAVE_ERROR_PASSCODE_AUTHENTICATION_FAILED
 *                               If passcode authentication fails.
 * @retval #WEAVE_ERROR_PASSCODE_FINGERPRINT_FAILED
 *                               If passcode fingerprint check fail-es.
 * @retval #WEAVE_ERROR_BUFFER_TOO_SMALL
 *                               If the supplied passcode buffer is too small.
 * @retval #WEAVE_ERROR_INVALID_ARGUMENT
 *                               If the encrytped passcode is too short or too long.
 *
 */
WEAVE_ERROR DecryptPasscode(const uint8_t *encPasscode, size_t encPasscodeLen,
                            const uint8_t *encKey, const uint8_t *authKey, const uint8_t *fingerprintKey,
                            uint8_t *passcodeBuf, size_t passcodeBufSize, size_t& passcodeLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    const EncryptedPasscodeStruct &encPasscodeStruct = *(const EncryptedPasscodeStruct *)encPasscode;
    uint8_t decryptedPasscode[kPasscodePaddedLen];
    uint8_t *passcodeEnd;

    // Verify the encrypted passcode is the correct length.
    VerifyOrExit(encPasscodeLen == sizeof(EncryptedPasscodeStruct), err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Verify supported encryption config.
    VerifyOrExit(IsSupportedPasscodeEncryptionConfig(encPasscodeStruct.config), err = WEAVE_ERROR_UNSUPPORTED_PASSCODE_CONFIG);

#if WEAVE_CONFIG_SUPPORT_PASSCODE_CONFIG1_TEST_ONLY
    if (encPasscodeStruct.config == kPasscode_Config1_TEST_ONLY)
    {
        // Verify correct key id.
        uint32_t keyId = LittleEndian::Get32(encPasscodeStruct.keyId);
        VerifyOrExit(keyId == kPasscodeConfig1_KeyId, err = WEAVE_ERROR_INVALID_KEY_ID);

        // "Decrypt" passcode and verify authenticator.
        err = DecryptPasscode_Config1(encPasscodeStruct, decryptedPasscode);
        SuccessOrExit(err);

        // Verify passcode fingerprint.
        err = VerifyPasscodeFingerprint_Config1(encPasscodeStruct);
        SuccessOrExit(err);
    }
    else
#endif // WEAVE_CONFIG_SUPPORT_PASSCODE_CONFIG1_TEST_ONLY
    {
#if WEAVE_CONFIG_SUPPORT_PASSCODE_CONFIG2

        // Decrypt and verify the passcode.
        err = DecryptPasscode_Config2(encKey, authKey, encPasscodeStruct, decryptedPasscode);
        SuccessOrExit(err);

        // Verify the passcode fingerprint.
        err = VerifyPasscodeFingerprint_Config2(fingerprintKey, decryptedPasscode, encPasscodeStruct);
        SuccessOrExit(err);

#endif // WEAVE_CONFIG_SUPPORT_PASSCODE_CONFIG2
    }

    // Determine the length of the passcode.
    passcodeEnd = (uint8_t *)memchr(decryptedPasscode, 0, sizeof(decryptedPasscode));
    passcodeLen = (passcodeEnd != NULL) ? passcodeEnd - decryptedPasscode : sizeof(decryptedPasscode);

    // Verify the output buffer is large enough to hold the decrypted passcode.
    VerifyOrExit(passcodeBufSize >= passcodeLen, err = WEAVE_ERROR_BUFFER_TOO_SMALL);

    // Copy passcode to the output buffer.
    memcpy(passcodeBuf, decryptedPasscode, passcodeLen);

exit:
    ClearSecretData(decryptedPasscode, sizeof(decryptedPasscode));
    return err;
}


} // namespace Passcodes
} // namespace Security
} // namespace Profiles
} // namespace Weave
} // namespace nl
