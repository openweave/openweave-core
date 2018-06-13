/*
 *
 *    Copyright (c) 2018 Nest Labs, Inc.
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
 *      Native method implementations for the PairingCodeSupport Java wrapper class.
 *
 */

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <jni.h>

#include <Weave/Support/CodeUtils.h>
#include <Weave/Support/verhoeff/Verhoeff.h>
#include <Weave/Support/pairing-code/PairingCodeUtils.h>

#include "WeaveSecuritySupport.h"
#include "JNIUtils.h"

namespace nl {
namespace Weave {
namespace SecuritySupport {

using namespace ::nl::PairingCode;


jboolean PairingCodeSupport::isValidPairingCode(JNIEnv * env, jclass cls, jstring pairingCodeJStr)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    const char * pairingCode = NULL;
    jsize pairingCodeLen;
    jboolean res = JNI_TRUE;

    VerifyOrExit(pairingCodeJStr != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    pairingCode = env->GetStringUTFChars(pairingCodeJStr, NULL);
    pairingCodeLen = env->GetStringUTFLength(pairingCodeJStr);

    res = (VerifyPairingCode(pairingCode, (size_t)pairingCodeLen) == WEAVE_NO_ERROR)
          ? JNI_TRUE
          : JNI_FALSE;

exit:
    if (pairingCode != NULL)
    {
        env->ReleaseStringUTFChars(pairingCodeJStr, pairingCode);
    }
    if (err != WEAVE_NO_ERROR)
    {
        JNIUtils::ThrowError(env, err);
    }
    return res;
}

jstring PairingCodeSupport::normalizePairingCode(JNIEnv * env, jclass cls, jstring pairingCodeJStr)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    const char * pairingCode = NULL;
    size_t pairingCodeLen;
    char * normalizedPairingCode = NULL;
    jstring normalizedPairingCodeJStr = NULL;

    VerifyOrExit(pairingCodeJStr != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    pairingCode = env->GetStringUTFChars(pairingCodeJStr, NULL);
    pairingCodeLen = env->GetStringUTFLength(pairingCodeJStr);

    normalizedPairingCode = strdup(pairingCode);
    VerifyOrExit(normalizedPairingCode != NULL, err = WEAVE_ERROR_NO_MEMORY);

    NormalizePairingCode(normalizedPairingCode, pairingCodeLen);

    normalizedPairingCode[pairingCodeLen] = 0;
    normalizedPairingCodeJStr = env->NewStringUTF((const char *)normalizedPairingCode);
    VerifyOrExit(normalizedPairingCodeJStr != NULL, err = WEAVE_JNI_ERROR_EXCEPTION_THROWN);

exit:
    if (pairingCode != NULL)
    {
        env->ReleaseStringUTFChars(pairingCodeJStr, pairingCode);
    }
    if (normalizedPairingCode != NULL)
    {
        free(normalizedPairingCode);
    }
    if (err != WEAVE_NO_ERROR)
    {
        JNIUtils::ThrowError(env, err);
    }
    return normalizedPairingCodeJStr;
}

jchar PairingCodeSupport::computeCheckChar(JNIEnv * env, jclass cls, jstring valJStr)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    const char * val = NULL;
    jsize valLen;
    jchar res = 0;

    VerifyOrExit(valJStr != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    val = env->GetStringUTFChars(valJStr, NULL);
    valLen = env->GetStringUTFLength(valJStr);

    res = Verhoeff32::ComputeCheckChar(val, (size_t)valLen);
    VerifyOrExit(res != 0, err = WEAVE_ERROR_INVALID_ARGUMENT);

exit:
    if (val != NULL)
    {
        env->ReleaseStringUTFChars(valJStr, val);
    }
    if (err != WEAVE_NO_ERROR)
    {
        JNIUtils::ThrowError(env, err);
    }
    return res;
}

jstring PairingCodeSupport::addCheckChar(JNIEnv * env, jclass cls, jstring valJStr)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    const char * val = NULL;
    size_t len;
    char * pairingCode = NULL;
    jstring pairingCodeJStr = NULL;

    VerifyOrExit(valJStr != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    val = env->GetStringUTFChars(valJStr, NULL);
    len = (size_t)env->GetStringUTFLength(valJStr);

    pairingCode = (char *)malloc(len + 2); // +1 for check character, +1 for nul terminator
    VerifyOrExit(pairingCode != NULL, err = WEAVE_ERROR_NO_MEMORY);

    memcpy(pairingCode, val, len);

    pairingCode[len] = Verhoeff32::ComputeCheckChar(val, len);
    VerifyOrExit(pairingCode[len] != 0, err = WEAVE_ERROR_INVALID_ARGUMENT);

    pairingCode[len+1] = 0;
    pairingCodeJStr = env->NewStringUTF((const char *)pairingCode);
    VerifyOrExit(pairingCodeJStr != NULL, err = WEAVE_JNI_ERROR_EXCEPTION_THROWN);

exit:
    if (val != NULL)
    {
        env->ReleaseStringUTFChars(valJStr, val);
    }
    if (pairingCode != NULL)
    {
        free(pairingCode);
    }
    if (err != WEAVE_NO_ERROR)
    {
        JNIUtils::ThrowError(env, err);
    }
    return pairingCodeJStr;
}

jstring PairingCodeSupport::intToPairingCode(JNIEnv * env, jclass cls, jlong val, jint pairingCodeLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    char * pairingCode = NULL;
    jstring pairingCodeJStr = NULL;

    VerifyOrExit(pairingCodeLen >= kPairingCodeLenMin && pairingCodeLen <= UINT8_MAX, err = WEAVE_ERROR_INVALID_ARGUMENT);

    pairingCode = (char *)malloc((size_t)pairingCodeLen + 1); // +1 for nul terminator
    VerifyOrExit(pairingCode != NULL, err = WEAVE_ERROR_NO_MEMORY);

    err = IntToPairingCode((uint64_t)val, (uint8_t)pairingCodeLen, pairingCode);
    SuccessOrExit(err);

    pairingCode[pairingCodeLen] = 0;
    pairingCodeJStr = env->NewStringUTF((const char *)pairingCode);
    VerifyOrExit(pairingCodeJStr != NULL, err = WEAVE_JNI_ERROR_EXCEPTION_THROWN);

exit:
    if (pairingCode != NULL)
    {
        free(pairingCode);
    }
    if (err != WEAVE_NO_ERROR)
    {
        JNIUtils::ThrowError(env, err);
    }
    return pairingCodeJStr;
}

jlong PairingCodeSupport::pairingCodeToInt(JNIEnv * env, jclass cls, jstring pairingCodeJStr)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    const char * pairingCode = NULL;
    size_t pairingCodeLen;
    uint64_t intVal;

    VerifyOrExit(pairingCodeJStr != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    pairingCode = env->GetStringUTFChars(pairingCodeJStr, NULL);
    pairingCodeLen = env->GetStringUTFLength(pairingCodeJStr);

    err = PairingCodeToInt(pairingCode, pairingCodeLen, intVal);
    SuccessOrExit(err);

exit:
    if (pairingCode != NULL)
    {
        env->ReleaseStringUTFChars(pairingCodeJStr, pairingCode);
    }
    if (err != WEAVE_NO_ERROR)
    {
        JNIUtils::ThrowError(env, err);
    }
    return (jlong)intVal;
}

jboolean PairingCodeSupport::isValidPairingCodeChar(JNIEnv * env, jclass cls, jchar ch)
{
    return IsValidPairingCodeChar(ch) ? JNI_TRUE : JNI_FALSE;
}

jstring PairingCodeSupport::generatePairingCode(JNIEnv * env, jclass cls, jint pairingCodeLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    char * pairingCode = NULL;
    jstring pairingCodeJStr = NULL;

    VerifyOrExit(pairingCodeLen >= kPairingCodeLenMin && pairingCodeLen <= UINT8_MAX, err = WEAVE_ERROR_INVALID_ARGUMENT);

    pairingCode = (char *)malloc((size_t)pairingCodeLen + 1); // +1 for nul terminator
    VerifyOrExit(pairingCode != NULL, err = WEAVE_ERROR_NO_MEMORY);

    err = GeneratePairingCode((uint8_t)pairingCodeLen, pairingCode);
    SuccessOrExit(err);

    pairingCode[pairingCodeLen] = 0;
    pairingCodeJStr = env->NewStringUTF((const char *)pairingCode);
    VerifyOrExit(pairingCodeJStr != NULL, err = WEAVE_JNI_ERROR_EXCEPTION_THROWN);

exit:
    if (pairingCode != NULL)
    {
        free(pairingCode);
    }
    if (err != WEAVE_NO_ERROR)
    {
        JNIUtils::ThrowError(env, err);
    }
    return pairingCodeJStr;
}

} // namespace SecuritySupport
} // namespace Weave
} // namespace nl
