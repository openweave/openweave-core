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

jobjectArray WeaveKeyExportSupportNative::simulateDeviceKeyExport(JNIEnv *env, jclass cls, jbyteArray deviceCert, jbyteArray devicePrivKey, jbyteArray trustRootCert, jbyteArray keyExportReq)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    const uint8_t *deviceCertBuf = NULL;
    jsize deviceCertLen = 0;
    const uint8_t *devicePrivKeyBuf = NULL;
    jsize devicePrivKeyLen = 0;
    const uint8_t *trustRootCertBuf = NULL;
    jsize trustRootCertLen = 0;
    const uint8_t *exportReqBuf = NULL;
    jsize exportReqLen = 0;
    jbyteArray exportResp = NULL;
    size_t exportRespBufSize;
    uint8_t *exportRespBuf = NULL;
    uint16_t exportRespLen = 0;
    bool isReconfig = false;
    jstring resultTypeStr = NULL;
    jobjectArray resultArray = NULL;

    VerifyOrExit(keyExportReq != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    deviceCertBuf = (uint8_t *)env->GetByteArrayElements(deviceCert, 0);
    deviceCertLen = env->GetArrayLength(deviceCert);

    devicePrivKeyBuf = (uint8_t *)env->GetByteArrayElements(devicePrivKey, 0);
    devicePrivKeyLen = env->GetArrayLength(devicePrivKey);

    trustRootCertBuf = (uint8_t *)env->GetByteArrayElements(trustRootCert, 0);
    trustRootCertLen = env->GetArrayLength(trustRootCert);

    exportReqBuf = (uint8_t *)env->GetByteArrayElements(keyExportReq, 0);
    exportReqLen = env->GetArrayLength(keyExportReq);

    exportRespBufSize =
            7                       // Key export response header size // TODO: adjust this
          + kMaxPubKeySize          // Ephemeral public key size
          + kMaxECDSASigSize        // Size of bare signature field
          + deviceCertLen           // Size equal to at least the total size of the device certificate
          + 1024;                   // Space for additional signature fields plus encoding overhead

    exportRespBuf = (uint8_t *)malloc(exportRespBufSize);
    VerifyOrExit(exportRespBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    err = SimulateDeviceKeyExport(deviceCertBuf, deviceCertLen,
                                  devicePrivKeyBuf, devicePrivKeyLen,
                                  trustRootCertBuf, trustRootCertLen,
                                  exportReqBuf, exportReqLen,
                                  exportRespBuf, exportRespBufSize, exportRespLen,
                                  isReconfig);
    SuccessOrExit(err);

    err = JNIUtils::N2J_ByteArray(env, exportRespBuf, exportRespLen, exportResp);
    SuccessOrExit(err);

    resultArray = env->NewObjectArray(2, JNIUtils::sJavaObjectClass, NULL);
    VerifyOrExit(resultArray != NULL, err = WEAVE_JNI_ERROR_EXCEPTION_THROWN);

    resultTypeStr = env->NewStringUTF((isReconfig) ? "KeyExportReconfigure" : "KeyExportResponse");
    VerifyOrExit(resultTypeStr != NULL, err = WEAVE_JNI_ERROR_EXCEPTION_THROWN);

    env->SetObjectArrayElement(resultArray, 0, resultTypeStr);

    env->SetObjectArrayElement(resultArray, 1, exportResp);

exit:
    if (deviceCertBuf != NULL)
    {
        env->ReleaseByteArrayElements(deviceCert, (jbyte *)deviceCertBuf, JNI_ABORT);
    }
    if (devicePrivKeyBuf != NULL)
    {
        env->ReleaseByteArrayElements(devicePrivKey, (jbyte *)devicePrivKeyBuf, JNI_ABORT);
    }
    if (trustRootCertBuf != NULL)
    {
        env->ReleaseByteArrayElements(trustRootCert, (jbyte *)trustRootCertBuf, JNI_ABORT);
    }
    if (exportReqBuf != NULL)
    {
        env->ReleaseByteArrayElements(keyExportReq, (jbyte *)exportReqBuf, JNI_ABORT);
    }
    if (exportRespBuf != NULL)
    {
        free(exportRespBuf);
    }
    if (err != WEAVE_NO_ERROR)
    {
        JNIUtils::ThrowError(env, err);
    }
    return resultArray;
}


} // namespace SecuritySupport
} // namespace Weave
} // namespace nl
