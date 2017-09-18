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
 *      Native method implementations for the PasscodeEncryptionSupport Java wrapper class.
 *
 */

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <jni.h>

#include <Weave/Core/WeaveCore.h>
#include <Weave/Support/CodeUtils.h>
#include <Weave/Profiles/security/WeavePasscodes.h>

#include "WeaveSecuritySupport.h"
#include "JNIUtils.h"

namespace nl {
namespace Weave {
namespace SecuritySupport {

using namespace nl::Weave::Profiles::Security::Passcodes;

jbyteArray PasscodeEncryptionSupport::encryptPasscode(JNIEnv *env, jclass cls, jint config, jint keyId, jlong nonce, jstring passcode, jbyteArray encKey,
                                                      jbyteArray authKey, jbyteArray fingerprintKey)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    const char *passcodeStr = NULL;
    const uint8_t *encKeyBufPtr = NULL;
    const uint8_t *authKeyBufPtr = NULL;
    const uint8_t *fingerprintKeyBufPtr = NULL;
    jsize passcodeLen = 0;
    jsize encKeyLen = 0;
    jsize authKeyLen = 0;
    jsize fingerprintKeyLen = 0;
    uint8_t encryptedPasscodeBuf[kPasscodeMaxEncryptedLen];
    size_t encryptedPasscodeLen;
    jbyteArray encryptedPasscode = NULL;

    VerifyOrExit(passcode != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    passcodeStr = env->GetStringUTFChars(passcode, NULL);
    passcodeLen = env->GetStringUTFLength(passcode);

    if (encKey != NULL)
    {
        encKeyBufPtr = (uint8_t *)env->GetByteArrayElements(encKey, 0);
        encKeyLen = env->GetArrayLength(encKey);
        VerifyOrExit(encKeyLen == kPasscodeEncryptionKeyLen, err = WEAVE_ERROR_INVALID_ARGUMENT);
    }
    if (authKey != NULL)
    {
        authKeyBufPtr = (uint8_t *)env->GetByteArrayElements(authKey, 0);
        authKeyLen = env->GetArrayLength(authKey);
        VerifyOrExit(authKeyLen == kPasscodeAuthenticationKeyLen, err = WEAVE_ERROR_INVALID_ARGUMENT);
    }
    if (fingerprintKey != NULL)
    {
        fingerprintKeyBufPtr = (uint8_t *)env->GetByteArrayElements(fingerprintKey, 0);
        fingerprintKeyLen = env->GetArrayLength(fingerprintKey);
        VerifyOrExit(fingerprintKeyLen == kPasscodeFingerprintKeyLen, err = WEAVE_ERROR_INVALID_ARGUMENT);
    }

    err = EncryptPasscode((uint8_t)config, (uint32_t)keyId, (uint32_t)nonce, (const uint8_t *)passcodeStr, (size_t)passcodeLen,
                          encKeyBufPtr, authKeyBufPtr, fingerprintKeyBufPtr,
                          encryptedPasscodeBuf, sizeof(encryptedPasscodeBuf), encryptedPasscodeLen);
    SuccessOrExit(err);

    err = JNIUtils::N2J_ByteArray(env, encryptedPasscodeBuf, encryptedPasscodeLen, encryptedPasscode);
    SuccessOrExit(err);

exit:
    if (passcodeStr != NULL)
    {
        env->ReleaseStringUTFChars(passcode, passcodeStr);
    }
    if (encKeyBufPtr != NULL)
    {
        env->ReleaseByteArrayElements(encKey, (jbyte *)encKeyBufPtr, JNI_ABORT);
    }
    if (authKeyBufPtr != NULL)
    {
        env->ReleaseByteArrayElements(authKey, (jbyte *)authKeyBufPtr, JNI_ABORT);
    }
    if (fingerprintKeyBufPtr != NULL)
    {
        env->ReleaseByteArrayElements(fingerprintKey, (jbyte *)fingerprintKeyBufPtr, JNI_ABORT);
    }

    if (err != WEAVE_NO_ERROR)
    {
        JNIUtils::ThrowError(env, err);
    }

    return encryptedPasscode;
}

jstring PasscodeEncryptionSupport::decryptPasscode(JNIEnv *env, jclass cls, jbyteArray encPasscode, jbyteArray encKey, jbyteArray authKey,
                                                   jbyteArray fingerprintKey)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    const uint8_t *encPasscodeBufPtr = NULL;
    const uint8_t *encKeyBufPtr = NULL;
    const uint8_t *authKeyBufPtr = NULL;
    const uint8_t *fingerprintKeyBufPtr = NULL;
    jsize encPasscodeLen = 0;
    jsize encKeyLen = 0;
    jsize authKeyLen = 0;
    jsize fingerprintKeyLen = 0;
    uint8_t passcodeBuf[kPasscodeMaxEncryptedLen + 1];
    size_t passcodeLen = 0;
    jstring passcode = NULL;

    VerifyOrExit(encPasscode != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    encPasscodeBufPtr = (uint8_t *)env->GetByteArrayElements(encPasscode, 0);
    encPasscodeLen = env->GetArrayLength(encPasscode);

    if (encKey != NULL)
    {
        encKeyBufPtr = (uint8_t *)env->GetByteArrayElements(encKey, 0);
        encKeyLen = env->GetArrayLength(encKey);
        VerifyOrExit(encKeyLen == kPasscodeEncryptionKeyLen, err = WEAVE_ERROR_INVALID_ARGUMENT);
    }
    if (authKey != NULL)
    {
        authKeyBufPtr = (uint8_t *)env->GetByteArrayElements(authKey, 0);
        authKeyLen = env->GetArrayLength(authKey);
        VerifyOrExit(authKeyLen == kPasscodeAuthenticationKeyLen, err = WEAVE_ERROR_INVALID_ARGUMENT);
    }
    if (fingerprintKey != NULL)
    {
        fingerprintKeyBufPtr = (uint8_t *)env->GetByteArrayElements(fingerprintKey, 0);
        fingerprintKeyLen = env->GetArrayLength(fingerprintKey);
        VerifyOrExit(fingerprintKeyLen == kPasscodeFingerprintKeyLen, err = WEAVE_ERROR_INVALID_ARGUMENT);
    }

    err = DecryptPasscode(encPasscodeBufPtr, encPasscodeLen, encKeyBufPtr, authKeyBufPtr, fingerprintKeyBufPtr, passcodeBuf, kPasscodeMaxEncryptedLen, passcodeLen);
    SuccessOrExit(err);

    passcodeBuf[passcodeLen] = 0;
    passcode = env->NewStringUTF((const char *)passcodeBuf);
    VerifyOrExit(passcode != NULL, err = WEAVE_JNI_ERROR_EXCEPTION_THROWN);

exit:
    if (encPasscodeBufPtr != NULL)
    {
        env->ReleaseByteArrayElements(encPasscode, (jbyte *)encPasscodeBufPtr, JNI_ABORT);
    }
    if (encKeyBufPtr != NULL)
    {
        env->ReleaseByteArrayElements(encKey, (jbyte *)encKeyBufPtr, JNI_ABORT);
    }
    if (authKeyBufPtr != NULL)
    {
        env->ReleaseByteArrayElements(authKey, (jbyte *)authKeyBufPtr, JNI_ABORT);
    }
    if (fingerprintKeyBufPtr != NULL)
    {
        env->ReleaseByteArrayElements(fingerprintKey, (jbyte *)fingerprintKeyBufPtr, JNI_ABORT);
    }

    if (err != WEAVE_NO_ERROR)
    {
        JNIUtils::ThrowError(env, err);
    }

    return passcode;
}

jboolean PasscodeEncryptionSupport::isSupportedPasscodeEncryptionConfig(JNIEnv *env, jclass cls, jint config)
{
    if (config < 0 || config > UINT8_MAX)
    {
        return JNI_FALSE;
    }
    return IsSupportedPasscodeEncryptionConfig((uint8_t)config) ? JNI_TRUE : JNI_FALSE;
}

jint PasscodeEncryptionSupport::getEncryptedPasscodeConfig(JNIEnv *env, jclass cls, jbyteArray encryptedPasscode)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    const uint8_t *passcodeBufPtr = NULL;
    jsize passcodeBufArrayLen;
    uint8_t config = 0;

    VerifyOrExit(encryptedPasscode != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    passcodeBufPtr = (uint8_t *)env->GetByteArrayElements(encryptedPasscode, 0);
    passcodeBufArrayLen = env->GetArrayLength(encryptedPasscode);

    err = GetEncryptedPasscodeConfig(passcodeBufPtr, (uint8_t)passcodeBufArrayLen, config);
    SuccessOrExit(err);

exit:
    if (passcodeBufPtr != NULL)
    {
        env->ReleaseByteArrayElements(encryptedPasscode, (jbyte *)passcodeBufPtr, JNI_ABORT);
    }

    if (err != WEAVE_NO_ERROR)
    {
        JNIUtils::ThrowError(env, err);
    }

    return (jint)config;
}

jint PasscodeEncryptionSupport::getEncryptedPasscodeKeyId(JNIEnv *env, jclass cls, jbyteArray encryptedPasscode)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    const uint8_t *passcodeBufPtr = NULL;
    jsize passcodeBufArrayLen;
    uint32_t keyId = 0;

    VerifyOrExit(encryptedPasscode != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    passcodeBufPtr = (uint8_t *)env->GetByteArrayElements(encryptedPasscode, 0);
    passcodeBufArrayLen = env->GetArrayLength(encryptedPasscode);

    err = GetEncryptedPasscodeKeyId(passcodeBufPtr, (uint8_t)passcodeBufArrayLen, keyId);
    SuccessOrExit(err);

exit:
    if (passcodeBufPtr != NULL)
    {
        env->ReleaseByteArrayElements(encryptedPasscode, (jbyte *)passcodeBufPtr, JNI_ABORT);
    }

    if (err != WEAVE_NO_ERROR)
    {
        JNIUtils::ThrowError(env, err);
    }

    return (jint)keyId;
}

jlong PasscodeEncryptionSupport::getEncryptedPasscodeNonce(JNIEnv *env, jclass cls, jbyteArray encryptedPasscode)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    const uint8_t *passcodeBufPtr = NULL;
    jsize passcodeBufArrayLen;
    uint32_t nonce = 0;

    VerifyOrExit(encryptedPasscode != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    passcodeBufPtr = (uint8_t *)env->GetByteArrayElements(encryptedPasscode, 0);
    passcodeBufArrayLen = env->GetArrayLength(encryptedPasscode);

    using nl::Weave::Profiles::Security::Passcodes::GetEncryptedPasscodeNonce;
    err = GetEncryptedPasscodeNonce(passcodeBufPtr, (uint8_t)passcodeBufArrayLen, nonce);
    SuccessOrExit(err);

exit:
    if (passcodeBufPtr != NULL)
    {
        env->ReleaseByteArrayElements(encryptedPasscode, (jbyte *)passcodeBufPtr, JNI_ABORT);
    }

    if (err != WEAVE_NO_ERROR)
    {
        JNIUtils::ThrowError(env, err);
    }

    return (jint)nonce;
}

jbyteArray PasscodeEncryptionSupport::getEncryptedPasscodeFingerprint(JNIEnv *env, jclass cls, jbyteArray encryptedPasscode)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    const uint8_t *passcodeBufPtr = NULL;
    jsize passcodeBufArrayLen;
    uint8_t fingerprintBuf[kPasscodeFingerprintLen];
    size_t fingerprintLen;
    jbyteArray fingerprint = NULL;

    VerifyOrExit(encryptedPasscode != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    passcodeBufPtr = (uint8_t *)env->GetByteArrayElements(encryptedPasscode, 0);
    passcodeBufArrayLen = env->GetArrayLength(encryptedPasscode);

    using nl::Weave::Profiles::Security::Passcodes::GetEncryptedPasscodeFingerprint;
    err = GetEncryptedPasscodeFingerprint(passcodeBufPtr, (uint8_t)passcodeBufArrayLen,
                                          fingerprintBuf, sizeof(fingerprintBuf), fingerprintLen);
    SuccessOrExit(err);

    err = JNIUtils::N2J_ByteArray(env, fingerprintBuf, fingerprintLen, fingerprint);
    SuccessOrExit(err);

exit:
    if (passcodeBufPtr != NULL)
    {
        env->ReleaseByteArrayElements(encryptedPasscode, (jbyte *)passcodeBufPtr, JNI_ABORT);
    }

    if (err != WEAVE_NO_ERROR)
    {
        JNIUtils::ThrowError(env, err);
    }

    return fingerprint;
}

} // namespace SecuritySupport
} // namespace Weave
} // namespace nl
