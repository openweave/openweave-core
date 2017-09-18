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

#define LOG_TAG "Pattern"

#include <stdlib.h>

#include "ErrorCode.h"
#include "JNIHelp.h"
#include "JniConstants.h"
#include "ScopedJavaUnicodeString.h"
#include "jni.h"
#include "unicode/parseerr.h"
#include "unicode/regex.h"

// ICU documentation: http://icu-project.org/apiref/icu4c/classRegexPattern.html

static RegexPattern* toRegexPattern(jint addr) {
    return reinterpret_cast<RegexPattern*>(static_cast<uintptr_t>(addr));
}

static void throwPatternSyntaxException(JNIEnv* env, UErrorCode status, jstring pattern, UParseError error) {
    static jmethodID method = env->GetMethodID(JniConstants::patternSyntaxExceptionClass,
            "<init>", "(Ljava/lang/String;Ljava/lang/String;I)V");
    jstring message = env->NewStringUTF(u_errorName(status));
    jclass exceptionClass = JniConstants::patternSyntaxExceptionClass;
    jobject exception = env->NewObject(exceptionClass, method, message, pattern, error.offset);
    env->Throw(reinterpret_cast<jthrowable>(exception));
}

static void Pattern_closeImpl(JNIEnv*, jclass, jint addr) {
    delete toRegexPattern(addr);
}

static jint Pattern_compileImpl(JNIEnv* env, jclass, jstring javaRegex, jint flags) {
    flags |= UREGEX_ERROR_ON_UNKNOWN_ESCAPES;

    UErrorCode status = U_ZERO_ERROR;
    UParseError error;
    error.offset = -1;

    ScopedJavaUnicodeString regex(env, javaRegex);
    UnicodeString& regexString(regex.unicodeString());
    RegexPattern* result = RegexPattern::compile(regexString, flags, error, status);
    if (!U_SUCCESS(status)) {
        throwPatternSyntaxException(env, status, javaRegex, error);
    }
    return static_cast<jint>(reinterpret_cast<uintptr_t>(result));
}

static JNINativeMethod gMethods[] = {
    NATIVE_METHOD(Pattern, closeImpl, "(I)V"),
    NATIVE_METHOD(Pattern, compileImpl, "(Ljava/lang/String;I)I"),
};
int register_java_util_regex_Pattern(JNIEnv* env) {
    return jniRegisterNativeMethods(env, "java/util/regex/Pattern", gMethods, NELEM(gMethods));
}
