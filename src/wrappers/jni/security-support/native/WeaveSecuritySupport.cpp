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
 *      Native library support for the WeaveSecuritySupport Java package.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <jni.h>

#include <Weave/Core/WeaveCore.h>
#include <Weave/Support/CodeUtils.h>

#include "WeaveSecuritySupport.h"
#include "JNIUtils.h"

namespace nl {
namespace Weave {
namespace SecuritySupport {

extern "C" {
    NL_DLL_EXPORT jint JNI_OnLoad(JavaVM *jvm, void *reserved);
    NL_DLL_EXPORT void JNI_OnUnload(JavaVM *jvm, void *reserved);
}

static const JNILibraryMethod sLibraryMethods[] = {

    // WeaveSecuritySupport Methods
    { "WeaveSecuritySupport", "getLibVersion", "()I", (void *)WeaveSecuritySupport::getLibVersion },

    // WeaveCertificateSupport Methods
    { "WeaveCertificateSupport", "weaveCertificateToX509", "([BII)[B", (void *)WeaveCertificateSupport::weaveCertificateToX509 },
    { "WeaveCertificateSupport", "x509CertificateToWeave", "([BII)[B", (void *)WeaveCertificateSupport::x509CertificateToWeave },

    // PasscodeEncryptionSupport Methods
    { "PasscodeEncryptionSupport", "encryptPasscode", "(IIJLjava/lang/String;[B[B[B)[B", (void *)PasscodeEncryptionSupport::encryptPasscode },
    { "PasscodeEncryptionSupport", "decryptPasscode",  "([B[B[B[B)Ljava/lang/String;", (void *)PasscodeEncryptionSupport::decryptPasscode },
    { "PasscodeEncryptionSupport", "isSupportedPasscodeEncryptionConfig", "(I)Z", (void *)PasscodeEncryptionSupport::isSupportedPasscodeEncryptionConfig },
    { "PasscodeEncryptionSupport", "getEncryptedPasscodeConfig", "([B)I", (void *)PasscodeEncryptionSupport::getEncryptedPasscodeConfig },
    { "PasscodeEncryptionSupport", "getEncryptedPasscodeKeyId", "([B)I", (void *)PasscodeEncryptionSupport::getEncryptedPasscodeKeyId },
    { "PasscodeEncryptionSupport", "getEncryptedPasscodeNonce", "([B)J", (void *)PasscodeEncryptionSupport::getEncryptedPasscodeNonce },
    { "PasscodeEncryptionSupport", "getEncryptedPasscodeFingerprint", "([B)[B", (void *)PasscodeEncryptionSupport::getEncryptedPasscodeFingerprint },

    // WeaveKeyExportClient Methods
    { "WeaveKeyExportClient", "newNativeClient", "()J", (void *)WeaveKeyExportClientNative::newNativeClient },
    { "WeaveKeyExportClient", "releaseNativeClient", "(J)V", (void *)WeaveKeyExportClientNative::releaseNativeClient },
    { "WeaveKeyExportClient", "resetNativeClient", "(J)V", (void *)WeaveKeyExportClientNative::resetNativeClient },
    { "WeaveKeyExportClient", "generateKeyExportRequest", "(JIJ[B[B)[B", (void *)WeaveKeyExportClientNative::generateKeyExportRequest_Cert },
    { "WeaveKeyExportClient", "generateKeyExportRequest", "(JIJ[B)[B", (void *)WeaveKeyExportClientNative::generateKeyExportRequest_AccessToken },
    { "WeaveKeyExportClient", "processKeyExportResponse", "(JJ[B)[B", (void *)WeaveKeyExportClientNative::processKeyExportResponse },
    { "WeaveKeyExportClient", "processKeyExportReconfigure", "(J[B)V", (void *)WeaveKeyExportClientNative::processKeyExportReconfigure },
    { "WeaveKeyExportClient", "allowNestDevelopmentDevices", "(J)Z", (void *)WeaveKeyExportClientNative::allowNestDevelopmentDevices },
    { "WeaveKeyExportClient", "setAllowNestDevelopmentDevices", "(JZ)V", (void *)WeaveKeyExportClientNative::setAllowNestDevelopmentDevices },
    { "WeaveKeyExportClient", "allowSHA1DeviceCertificates", "(J)Z", (void *)WeaveKeyExportClientNative::allowSHA1DeviceCertificates },
    { "WeaveKeyExportClient", "setAllowSHA1DeviceCertificates", "(JZ)V", (void *)WeaveKeyExportClientNative::setAllowSHA1DeviceCertificates },

    // WeaveKeyExportSupport Methods
    { "WeaveKeyExportSupport", "simulateDeviceKeyExport", "([B[B[B[B)[Ljava/lang/Object;", (void *)WeaveKeyExportSupportNative::simulateDeviceKeyExport },
};
static const size_t sNumLibraryMethods = sizeof(sLibraryMethods) / sizeof(JNILibraryMethod);

jint JNI_OnLoad(JavaVM *jvm, void *reserved)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    JNIEnv *env;

    // Get a JNI environment object.
    jvm->GetEnv((void **)&env, JNI_VERSION_1_2);

    // Initialize the JNIUtils package.
    err = JNIUtils::Init(jvm, env, "com/nestlabs/weave/security/WeaveSecuritySupportException");
    SuccessOrExit(err);

    // Register library methods.
    err = JNIUtils::RegisterLibraryMethods(env, "com/nestlabs/weave/security", sLibraryMethods, sNumLibraryMethods);
    SuccessOrExit(err);

exit:
    if (err != WEAVE_NO_ERROR)
    {
        JNI_OnUnload(jvm, reserved);
    }

    return (err == WEAVE_NO_ERROR) ? JNI_VERSION_1_2 : JNI_ERR;
}

void JNI_OnUnload(JavaVM *jvm, void *reserved)
{
    JNIEnv *env;

    // Get a JNI environment object.
    jvm->GetEnv((void **)&env, JNI_VERSION_1_2);

    // Shutdown the JNIUtils package.
    JNIUtils::Shutdown(env);
}

jint WeaveSecuritySupport::getLibVersion(JNIEnv *env, jclass cls)
{
    return LibraryVersion;
}


} // namespace SecuritySupport
} // namespace Weave
} // namespace nl
