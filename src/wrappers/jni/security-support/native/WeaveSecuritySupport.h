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

#ifndef WEAVESECURITYSUPPORT_H_
#define WEAVESECURITYSUPPORT_H_

#include <jni.h>

namespace nl {
namespace Weave {
namespace SecuritySupport {

class WeaveSecuritySupport
{
public:
    enum {
        LibraryVersion = 1
    };

    static jint getLibVersion(JNIEnv *env, jclass cls);
};

class WeaveCertificateSupport
{
public:
    static jbyteArray weaveCertificateToX509(JNIEnv *env, jclass cls, jbyteArray certBuf, jint offset, jint length);
    static jbyteArray x509CertificateToWeave(JNIEnv *env, jclass cls, jbyteArray certBuf, jint offset, jint length);
};

class PasscodeEncryptionSupport
{
public:
    static jbyteArray encryptPasscode(JNIEnv *env, jclass cls, jint config, jint keyId, jlong nonce, jstring passcode, jbyteArray encKey, jbyteArray authKey, jbyteArray fingerprintKey);
    static jstring decryptPasscode(JNIEnv *env, jclass cls, jbyteArray encryptedPasscode, jbyteArray encKey, jbyteArray authKey, jbyteArray fingerprintKey);
    static jboolean isSupportedPasscodeEncryptionConfig(JNIEnv *env, jclass cls, jint config);
    static jint getEncryptedPasscodeConfig(JNIEnv *env, jclass cls, jbyteArray encryptedPasscode);
    static jint getEncryptedPasscodeKeyId(JNIEnv *env, jclass cls, jbyteArray encryptedPasscode);
    static jlong getEncryptedPasscodeNonce(JNIEnv *env, jclass cls, jbyteArray encryptedPasscode);
    static jbyteArray getEncryptedPasscodeFingerprint(JNIEnv *env, jclass cls, jbyteArray encryptedPasscode);
};

class WeaveKeyExportClientNative
{
public:
    static jlong newNativeClient(JNIEnv *env, jclass cls);
    static void releaseNativeClient(JNIEnv *env, jclass cls, jlong nativeClientPtr);
    static void resetNativeClient(JNIEnv *env, jclass cls, jlong nativeClientPtr);
    static jbyteArray generateKeyExportRequest_Cert(JNIEnv *env, jclass cls, jlong nativeClientPtr, jint keyId, jlong responderNodeId,
            jbyteArray clientCert, jbyteArray clientKey);
    static jbyteArray generateKeyExportRequest_AccessToken(JNIEnv *env, jclass cls, jlong nativeClientPtr, jint keyId, jlong responderNodeId,
            jbyteArray accessToken);
    static jbyteArray processKeyExportResponse(JNIEnv *env, jclass cls, jlong nativeClientPtr, jlong responderNodeId, jbyteArray exportResp);
    static void processKeyExportReconfigure(JNIEnv *env, jclass cls, jlong nativeClientPtr, jbyteArray reconfig);
    static jboolean allowNestDevelopmentDevices(JNIEnv *env, jclass cls, jlong nativeClientPtr);
    static void setAllowNestDevelopmentDevices(JNIEnv *env, jclass cls, jlong nativeClientPtr, jboolean val);
    static jboolean allowSHA1DeviceCertificates(JNIEnv *env, jclass cls, jlong nativeClientPtr);
    static void setAllowSHA1DeviceCertificates(JNIEnv *env, jclass cls, jlong nativeClientPtr, jboolean val);
};

class WeaveKeyExportSupportNative
{
public:
    static jobjectArray simulateDeviceKeyExport(JNIEnv *env, jclass cls, jbyteArray deviceCert, jbyteArray devicePrivKey, jbyteArray trustRootCert, jbyteArray keyExportReq);
};


} // namespace SecuritySupport
} // namespace Weave
} // namespace nl


#endif /* WEAVESECURITYSUPPORT_H_ */
