/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "NativeNormalizer"

#include "ErrorCode.h"
#include "JNIHelp.h"
#include "JniConstants.h"
#include "ScopedJavaUnicodeString.h"
#include "unicode/normlzr.h"

static jstring NativeNormalizer_normalizeImpl(JNIEnv* env, jclass, jstring s, jint intMode) {
    ScopedJavaUnicodeString src(env, s);
    UNormalizationMode mode = static_cast<UNormalizationMode>(intMode);
    UErrorCode errorCode = U_ZERO_ERROR;
    UnicodeString dst;
    Normalizer::normalize(src.unicodeString(), mode, 0, dst, errorCode);
    icu4jni_error(env, errorCode);
    return dst.isBogus() ? NULL : env->NewString(dst.getBuffer(), dst.length());
}

static jboolean NativeNormalizer_isNormalizedImpl(JNIEnv* env, jclass, jstring s, jint intMode) {
    ScopedJavaUnicodeString src(env, s);
    UNormalizationMode mode = static_cast<UNormalizationMode>(intMode);
    UErrorCode errorCode = U_ZERO_ERROR;
    UBool result = Normalizer::isNormalized(src.unicodeString(), mode, errorCode);
    icu4jni_error(env, errorCode);
    return result;
}

static JNINativeMethod gMethods[] = {
    NATIVE_METHOD(NativeNormalizer, normalizeImpl, "(Ljava/lang/String;I)Ljava/lang/String;"),
    NATIVE_METHOD(NativeNormalizer, isNormalizedImpl, "(Ljava/lang/String;I)Z"),
};
int register_libcore_icu_NativeNormalizer(JNIEnv* env) {
    return jniRegisterNativeMethods(env, "libcore/icu/NativeNormalizer", gMethods, NELEM(gMethods));
}
