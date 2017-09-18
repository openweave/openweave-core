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
 *      Native method implementations for the WeaveKeyExportClient Java wrapper class.
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
#include <Weave/Core/WeaveKeyIds.h>
#include <Weave/Support/CodeUtils.h>
#include <Weave/Profiles/security/WeaveKeyExportClient.h>

#include "WeaveSecuritySupport.h"
#include "JNIUtils.h"

namespace nl {
namespace Weave {
namespace SecuritySupport {

using namespace nl::Weave::Profiles::Security::KeyExport;

enum
{
    kMaxPubKeySize = (((WEAVE_CONFIG_MAX_EC_BITS + 7) / 8) + 1) * 2,
    kMaxECDSASigSize = kMaxPubKeySize,
};

jlong WeaveKeyExportClientNative::newNativeClient(JNIEnv *env, jclass cls)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveStandAloneKeyExportClient *keyExportClient;

    keyExportClient = new WeaveStandAloneKeyExportClient();
    VerifyOrExit(keyExportClient != NULL, err = WEAVE_ERROR_NO_MEMORY);

    keyExportClient->Init();

exit:
    if (err != WEAVE_NO_ERROR)
    {
        JNIUtils::ThrowError(env, err);
    }

    return (jlong)keyExportClient;
}

void WeaveKeyExportClientNative::releaseNativeClient(JNIEnv *env, jclass cls, jlong nativeClientPtr)
{
    WeaveStandAloneKeyExportClient *keyExportClient = (WeaveStandAloneKeyExportClient *)nativeClientPtr;

    if (keyExportClient != NULL)
    {
        keyExportClient->Reset();
        delete keyExportClient;
    }
}

void WeaveKeyExportClientNative::resetNativeClient(JNIEnv *env, jclass cls, jlong nativeClientPtr)
{
    WeaveStandAloneKeyExportClient *keyExportClient = (WeaveStandAloneKeyExportClient *)nativeClientPtr;

    if (keyExportClient != NULL)
    {
        keyExportClient->Reset();
    }
}

jbyteArray WeaveKeyExportClientNative::generateKeyExportRequest_Cert(JNIEnv *env, jclass cls, jlong nativeClientPtr, jint keyId, jlong responderNodeId,
        jbyteArray clientCert, jbyteArray clientKey)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveStandAloneKeyExportClient *keyExportClient = (WeaveStandAloneKeyExportClient *)nativeClientPtr;
    const uint8_t *clientCertBuf = NULL;
    const uint8_t *clientKeyBuf = NULL;
    jsize clientCertLen = 0;
    jsize clientKeyLen = 0;
    uint8_t *exportReqBuf = NULL;
    size_t exportReqBufSize;
    uint16_t exportReqLen;
    jbyteArray exportReq = NULL;

    VerifyOrExit(keyExportClient != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);
    VerifyOrExit(clientCert != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);
    VerifyOrExit(clientKey != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    clientCertBuf = (uint8_t *)env->GetByteArrayElements(clientCert, 0);
    clientCertLen = env->GetArrayLength(clientCert);

    clientKeyBuf = (uint8_t *)env->GetByteArrayElements(clientKey, 0);
    clientKeyLen = env->GetArrayLength(clientKey);

    exportReqBufSize =
            7                       // Key export request header size
          + kMaxPubKeySize          // Ephemeral public key size
          + kMaxECDSASigSize        // Size of bare signature field
          + clientCertLen           // Size of client certificate
          + 1024;                   // Space for additional signature fields plus encoding overhead

    VerifyOrExit(exportReqBufSize <= UINT16_MAX, err = WEAVE_ERROR_INVALID_ARGUMENT);

    exportReqBuf = (uint8_t *)malloc(exportReqBufSize);
    VerifyOrExit(exportReqBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    err = keyExportClient->GenerateKeyExportRequest((uint32_t)keyId, (uint64_t)responderNodeId,
            clientCertBuf, clientCertLen, clientKeyBuf, clientKeyLen,
            exportReqBuf, (uint16_t)exportReqBufSize, exportReqLen);
    SuccessOrExit(err);

    err = JNIUtils::N2J_ByteArray(env, exportReqBuf, exportReqLen, exportReq);
    SuccessOrExit(err);

exit:
    if (clientCertBuf != NULL)
    {
        env->ReleaseByteArrayElements(clientCert, (jbyte *)clientCertBuf, JNI_ABORT);
    }
    if (clientKeyBuf != NULL)
    {
        env->ReleaseByteArrayElements(clientKey, (jbyte *)clientKeyBuf, JNI_ABORT);
    }
    if (err != WEAVE_NO_ERROR)
    {
        JNIUtils::ThrowError(env, err);
    }
    return exportReq;
}

jbyteArray WeaveKeyExportClientNative::generateKeyExportRequest_AccessToken(JNIEnv *env, jclass cls, jlong nativeClientPtr, jint keyId, jlong responderNodeId,
        jbyteArray accessToken)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveStandAloneKeyExportClient *keyExportClient = (WeaveStandAloneKeyExportClient *)nativeClientPtr;
    const uint8_t *accessTokenBuf = NULL;
    jsize accessTokenLen = 0;
    uint8_t *exportReqBuf = NULL;
    size_t exportReqBufSize;
    uint16_t exportReqLen;
    jbyteArray exportReq = NULL;

    VerifyOrExit(keyExportClient != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);
    VerifyOrExit(accessToken != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    accessTokenBuf = (uint8_t *)env->GetByteArrayElements(accessToken, 0);
    accessTokenLen = env->GetArrayLength(accessToken);

    exportReqBufSize =
            7                       // Key export request header size
          + kMaxPubKeySize          // Ephemeral public key size
          + kMaxECDSASigSize        // Size of bare signature field
          + accessTokenLen          // Size equal to at least the total size of the client certificates
          + 1024;                   // Space for additional signature fields plus encoding overhead

    VerifyOrExit(exportReqBufSize <= UINT16_MAX, err = WEAVE_ERROR_INVALID_ARGUMENT);

    exportReqBuf = (uint8_t *)malloc(exportReqBufSize);
    VerifyOrExit(exportReqBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    err = keyExportClient->GenerateKeyExportRequest((uint32_t)keyId, (uint64_t)responderNodeId,
            accessTokenBuf, accessTokenLen,
            exportReqBuf, (uint16_t)exportReqBufSize, exportReqLen);
    SuccessOrExit(err);

    err = JNIUtils::N2J_ByteArray(env, exportReqBuf, exportReqLen, exportReq);
    SuccessOrExit(err);

exit:
    if (accessTokenBuf != NULL)
    {
        env->ReleaseByteArrayElements(accessToken, (jbyte *)accessTokenBuf, JNI_ABORT);
    }
    if (exportReqBuf != NULL)
    {
        free(exportReqBuf);
    }
    if (err != WEAVE_NO_ERROR)
    {
        JNIUtils::ThrowError(env, err);
    }
    return exportReq;
}

jbyteArray WeaveKeyExportClientNative::processKeyExportResponse(JNIEnv *env, jclass cls, jlong nativeClientPtr, jlong responderNodeId, jbyteArray exportResp)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveStandAloneKeyExportClient *keyExportClient = (WeaveStandAloneKeyExportClient *)nativeClientPtr;
    const uint8_t *exportRespBuf = NULL;
    jsize exportRespLen = 0;
    uint8_t *exportedKeyBuf = NULL;
    size_t exportedKeyBufSize;
    uint16_t exportedKeyLen;
    jbyteArray exportedKey = NULL;
    uint32_t exportedKeyId;

    VerifyOrExit(keyExportClient != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);
    VerifyOrExit(exportResp != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    exportRespBuf = (uint8_t *)env->GetByteArrayElements(exportResp, 0);
    exportRespLen = env->GetArrayLength(exportResp);

    // Since the exported key is contained within the export response, a buffer of the same size
    // is guaranteed to be sufficient.
    exportedKeyBufSize = exportRespLen;

    exportedKeyBuf = (uint8_t *)malloc(exportedKeyBufSize);
    VerifyOrExit(exportedKeyBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    err = keyExportClient->ProcessKeyExportResponse(exportRespBuf, exportRespLen, (uint64_t)responderNodeId, exportedKeyBuf, exportedKeyBufSize, exportedKeyLen, exportedKeyId);
    SuccessOrExit(err);

    err = JNIUtils::N2J_ByteArray(env, exportedKeyBuf, exportedKeyLen, exportedKey);
    SuccessOrExit(err);

exit:
    if (keyExportClient != NULL)
    {
        keyExportClient->Reset();
    }
    if (exportRespBuf != NULL)
    {
        env->ReleaseByteArrayElements(exportResp, (jbyte *)exportRespBuf, JNI_ABORT);
    }
    if (exportedKeyBuf != NULL)
    {
        free(exportedKeyBuf);
    }
    if (err != WEAVE_NO_ERROR)
    {
        JNIUtils::ThrowError(env, err);
    }
    return exportedKey;
}

void WeaveKeyExportClientNative::processKeyExportReconfigure(JNIEnv *env, jclass cls, jlong nativeClientPtr, jbyteArray reconfig)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveStandAloneKeyExportClient *keyExportClient = (WeaveStandAloneKeyExportClient *)nativeClientPtr;
    const uint8_t *reconfigBuf = NULL;
    jsize reconfigLen = 0;

    VerifyOrExit(keyExportClient != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);
    VerifyOrExit(reconfig != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    reconfigBuf = (uint8_t *)env->GetByteArrayElements(reconfig, 0);
    reconfigLen = env->GetArrayLength(reconfig);

    err = keyExportClient->ProcessKeyExportReconfigure(reconfigBuf, reconfigLen);
    SuccessOrExit(err);

exit:
    if (reconfigBuf != NULL)
    {
        env->ReleaseByteArrayElements(reconfig, (jbyte *)reconfigBuf, JNI_ABORT);
    }
    if (err != WEAVE_NO_ERROR)
    {
        JNIUtils::ThrowError(env, err);
    }
}

jboolean WeaveKeyExportClientNative::allowNestDevelopmentDevices(JNIEnv *env, jclass cls, jlong nativeClientPtr)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveStandAloneKeyExportClient *keyExportClient = (WeaveStandAloneKeyExportClient *)nativeClientPtr;
    jboolean res = JNI_FALSE;

    VerifyOrExit(keyExportClient != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    res = keyExportClient->AllowNestDevelopmentDevices() ? JNI_TRUE : JNI_FALSE;

exit:
    if (err != WEAVE_NO_ERROR)
    {
        JNIUtils::ThrowError(env, err);
    }
    return res;
}

void WeaveKeyExportClientNative::setAllowNestDevelopmentDevices(JNIEnv *env, jclass cls, jlong nativeClientPtr, jboolean val)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveStandAloneKeyExportClient *keyExportClient = (WeaveStandAloneKeyExportClient *)nativeClientPtr;

    VerifyOrExit(keyExportClient != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    keyExportClient->AllowNestDevelopmentDevices(val != JNI_FALSE);

exit:
    if (err != WEAVE_NO_ERROR)
    {
        JNIUtils::ThrowError(env, err);
    }
}

jboolean WeaveKeyExportClientNative::allowSHA1DeviceCertificates(JNIEnv *env, jclass cls, jlong nativeClientPtr)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveStandAloneKeyExportClient *keyExportClient = (WeaveStandAloneKeyExportClient *)nativeClientPtr;
    jboolean res = JNI_FALSE;

    VerifyOrExit(keyExportClient != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    res = keyExportClient->AllowSHA1DeviceCerts() ? JNI_TRUE : JNI_FALSE;

exit:
    if (err != WEAVE_NO_ERROR)
    {
        JNIUtils::ThrowError(env, err);
    }
    return res;
}

void WeaveKeyExportClientNative::setAllowSHA1DeviceCertificates(JNIEnv *env, jclass cls, jlong nativeClientPtr, jboolean val)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveStandAloneKeyExportClient *keyExportClient = (WeaveStandAloneKeyExportClient *)nativeClientPtr;

    VerifyOrExit(keyExportClient != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    keyExportClient->AllowSHA1DeviceCerts(val != JNI_FALSE);

exit:
    if (err != WEAVE_NO_ERROR)
    {
        JNIUtils::ThrowError(env, err);
    }
}


} // namespace SecuritySupport
} // namespace Weave
} // namespace nl
