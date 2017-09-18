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
 *      Native method implementations for the WeaveCertificateSupport Java wrapper class.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <jni.h>

#include <Weave/Core/WeaveCore.h>
#include <Weave/Support/CodeUtils.h>
#include <Weave/Profiles/security/WeaveCert.h>

#include "WeaveSecuritySupport.h"
#include "JNIUtils.h"

namespace nl {
namespace Weave {
namespace SecuritySupport {

using namespace nl::Weave::Profiles::Security;

jbyteArray WeaveCertificateSupport::weaveCertificateToX509(JNIEnv *env, jclass cls, jbyteArray certBuf, jint offset, jint len)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    const uint8_t *certBufPtr = NULL;
    jsize certBufArrayLen;
    uint8_t *tmpBuf = NULL;
    uint32_t tmpBufSize;
    uint32_t outCertLen;
    jbyteArray outBuf = NULL;

    enum {
        kMaxInflationFactor = 5 // Maximum ratio of the size of buffer needed to hold an X.509 certificate
                                // relative to the size of buffer needed to hold its Weave counterpart.
                                // This value (5) is conservatively big given that certificates contain large
                                // amounts of incompressible data.  In practice, the factor is going to be
                                // much closer to 1.5.
    };

    VerifyOrExit(certBuf != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    certBufPtr = (uint8_t *)env->GetByteArrayElements(certBuf, 0);
    certBufArrayLen = env->GetArrayLength(certBuf);

    VerifyOrExit(offset >= 0 && len > 0 && (offset + len) <= certBufArrayLen, err = WEAVE_ERROR_INVALID_ARGUMENT);

    tmpBufSize = len * kMaxInflationFactor;
    tmpBuf = (uint8_t *)malloc(tmpBufSize);
    VerifyOrExit(tmpBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    err = ConvertWeaveCertToX509Cert(certBufPtr + offset, (uint32_t)len, tmpBuf, tmpBufSize, outCertLen);
    SuccessOrExit(err);

    err = JNIUtils::N2J_ByteArray(env, tmpBuf, outCertLen, outBuf);
    SuccessOrExit(err);

exit:
    if (certBufPtr != NULL)
    {
        env->ReleaseByteArrayElements(certBuf, (jbyte *)certBufPtr, JNI_ABORT);
    }

    if (tmpBuf != NULL)
    {
        free(tmpBuf);
    }

    if (err != WEAVE_NO_ERROR)
    {
        JNIUtils::ThrowError(env, err);
    }

    return outBuf;
}

jbyteArray WeaveCertificateSupport::x509CertificateToWeave(JNIEnv *env, jclass cls, jbyteArray certBuf, jint offset, jint len)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    const uint8_t *certBufPtr = NULL;
    jsize certBufArrayLen;
    uint8_t *tmpBuf = NULL;
    uint32_t tmpBufSize;
    uint32_t outCertLen;
    jbyteArray outBuf = NULL;

    VerifyOrExit(certBuf != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    certBufPtr = (uint8_t *)env->GetByteArrayElements(certBuf, 0);
    certBufArrayLen = env->GetArrayLength(certBuf);

    VerifyOrExit(offset >= 0 && len > 0 && (offset + len) <= certBufArrayLen, err = WEAVE_ERROR_INVALID_ARGUMENT);

    tmpBufSize = len;
    tmpBuf = (uint8_t *)malloc(tmpBufSize);
    VerifyOrExit(tmpBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    err = ConvertX509CertToWeaveCert(certBufPtr + offset, (uint32_t)len, tmpBuf, tmpBufSize, outCertLen);
    SuccessOrExit(err);

    err = JNIUtils::N2J_ByteArray(env, tmpBuf, outCertLen, outBuf);
    SuccessOrExit(err);

exit:
    if (certBufPtr != NULL)
    {
        env->ReleaseByteArrayElements(certBuf, (jbyte *)certBufPtr, JNI_ABORT);
    }

    if (tmpBuf != NULL)
    {
        free(tmpBuf);
    }

    if (err != WEAVE_NO_ERROR)
    {
        JNIUtils::ThrowError(env, err);
    }

    return outBuf;
}

} // namespace SecuritySupport
} // namespace Weave
} // namespace nl
