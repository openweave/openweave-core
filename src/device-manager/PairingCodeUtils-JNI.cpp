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
 *      JNI implementation of PairingCodeUtils native methods.
 *
 */

#include <jni.h>

#include <string.h>

#include <Weave/Support/CodeUtils.h>
#include <Weave/Support/pairing-code/PairingCodeUtils.h>

extern "C"
{
	NL_DLL_EXPORT jboolean Java_nl_Weave_DeviceManager_PairingCodeUtils_isValidPairingCode(JNIEnv *, jclass, jstring);
    NL_DLL_EXPORT jstring Java_nl_Weave_DeviceManager_PairingCodeUtils_normalizePairingCode(JNIEnv *, jclass, jstring);
    NL_DLL_EXPORT jlong Java_nl_Weave_DeviceManager_PairingCodeUtils_nevisPairingCodeToDeviceId(JNIEnv *, jclass, jstring);
    NL_DLL_EXPORT jstring Java_nl_Weave_DeviceManager_PairingCodeUtils_nevisDeviceIdToPairingCode(JNIEnv *, jclass, jlong);
    NL_DLL_EXPORT jlong Java_nl_Weave_DeviceManager_PairingCodeUtils_kryptonitePairingCodeToDeviceId(JNIEnv *, jclass, jstring);
    NL_DLL_EXPORT jstring Java_nl_Weave_DeviceManager_PairingCodeUtils_kryptoniteDeviceIdToPairingCode(JNIEnv *, jclass, jlong);
}

jboolean Java_nl_Weave_DeviceManager_PairingCodeUtils_isValidPairingCode(JNIEnv *env, jclass cls, jstring pairingCodeObj)
{
    WEAVE_ERROR err;
    const char *pairingCodeStr = NULL;

    pairingCodeStr = env->GetStringUTFChars(pairingCodeObj, 0);

    err = nl::PairingCode::VerifyPairingCode(pairingCodeStr, strlen(pairingCodeStr));

    env->ReleaseStringUTFChars(pairingCodeObj, pairingCodeStr);

    return (err == WEAVE_NO_ERROR) ? JNI_TRUE : JNI_FALSE;
}

jstring Java_nl_Weave_DeviceManager_PairingCodeUtils_normalizePairingCode(JNIEnv *env, jclass cls, jstring pairingCodeObj)
{
    const char *pairingCodeStr = NULL;
    char *normalizedPairingCodeStr = NULL;
    jstring normalizedPairingCodeObj = NULL;
    size_t pairingCodeLen;

    pairingCodeStr = env->GetStringUTFChars(pairingCodeObj, 0);

    pairingCodeLen = strlen(pairingCodeStr);

    normalizedPairingCodeStr = strdup(pairingCodeStr);

    if (normalizedPairingCodeStr != NULL)
    {
        nl::PairingCode::NormalizePairingCode(normalizedPairingCodeStr, pairingCodeLen);

        normalizedPairingCodeObj = env->NewStringUTF(normalizedPairingCodeStr);

        free(normalizedPairingCodeStr);
    }

    env->ReleaseStringUTFChars(pairingCodeObj, pairingCodeStr);

    return normalizedPairingCodeObj;
}

jlong Java_nl_Weave_DeviceManager_PairingCodeUtils_nevisPairingCodeToDeviceId(JNIEnv *env, jclass cls, jstring pairingCodeObj)
{
    WEAVE_ERROR err;
    const char *pairingCodeStr = NULL;
    uint64_t res;

    pairingCodeStr = env->GetStringUTFChars(pairingCodeObj, 0);

    err = nl::PairingCode::NevisPairingCodeToDeviceId(pairingCodeStr, res);
    if (err != WEAVE_NO_ERROR)
        res = 0;

    env->ReleaseStringUTFChars(pairingCodeObj, pairingCodeStr);

    return (jlong)res;
}

jstring Java_nl_Weave_DeviceManager_PairingCodeUtils_nevisDeviceIdToPairingCode(JNIEnv *env, jclass cls, jlong deviceId)
{
    WEAVE_ERROR err;
    char pairingCodeStr[nl::PairingCode::kStandardPairingCodeLength + 1];

    err = nl::PairingCode::NevisDeviceIdToPairingCode((uint64_t) deviceId, pairingCodeStr, sizeof(pairingCodeStr));

    if (err != WEAVE_NO_ERROR)
    {
        return NULL;
    }

    jstring pairingCode = env->NewStringUTF(pairingCodeStr);

    return pairingCode;
}


jlong Java_nl_Weave_DeviceManager_PairingCodeUtils_kryptonitePairingCodeToDeviceId(JNIEnv *env, jclass cls, jstring pairingCodeObj)
{
    WEAVE_ERROR err;
    const char *pairingCodeStr = NULL;
    uint64_t res;

    pairingCodeStr = env->GetStringUTFChars(pairingCodeObj, 0);

    err = nl::PairingCode::KryptonitePairingCodeToDeviceId(pairingCodeStr, res);
    if (err != WEAVE_NO_ERROR)
        res = 0;

    env->ReleaseStringUTFChars(pairingCodeObj, pairingCodeStr);

    return (jlong)res;
}

jstring Java_nl_Weave_DeviceManager_PairingCodeUtils_kryptoniteDeviceIdToPairingCode(JNIEnv *env, jclass cls, jlong deviceId)
{
    WEAVE_ERROR err;
    char pairingCodeStr[nl::PairingCode::kKryptonitePairingCodeLength + 1];

    err = nl::PairingCode::KryptoniteDeviceIdToPairingCode((uint64_t) deviceId, pairingCodeStr, sizeof(pairingCodeStr));

    if (err != WEAVE_NO_ERROR)
    {
        return NULL;
    }

    jstring pairingCode = env->NewStringUTF(pairingCodeStr);

    return pairingCode;
}
