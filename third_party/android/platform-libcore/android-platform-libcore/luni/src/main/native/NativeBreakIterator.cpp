/*
 * Copyright (C) 2006 The Android Open Source Project
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

#define LOG_TAG "NativeBreakIterator"

#include "JNIHelp.h"
#include "JniConstants.h"
#include "ErrorCode.h"
#include "ScopedJavaUnicodeString.h"
#include "ScopedUtfChars.h"
#include "unicode/ubrk.h"
#include "unicode/putil.h"
#include <stdlib.h>

static jint getIterator(JNIEnv* env, jstring locale, UBreakIteratorType type) {
    UErrorCode status = U_ZERO_ERROR;
    ScopedUtfChars localeChars(env, locale);
    if (localeChars.c_str() == NULL) {
        return 0;
    }
    UBreakIterator* it = ubrk_open(type, localeChars.c_str(), NULL, 0, &status);
    icu4jni_error(env, status);
    return reinterpret_cast<uintptr_t>(it);
}

static jint NativeBreakIterator_getCharacterInstanceImpl(JNIEnv* env, jclass, jstring locale) {
    return getIterator(env, locale, UBRK_CHARACTER);
}

static jint NativeBreakIterator_getLineInstanceImpl(JNIEnv* env, jclass, jstring locale) {
    return getIterator(env, locale, UBRK_LINE);
}

static jint NativeBreakIterator_getSentenceInstanceImpl(JNIEnv* env, jclass, jstring locale) {
    return getIterator(env, locale, UBRK_SENTENCE);
}

static jint NativeBreakIterator_getWordInstanceImpl(JNIEnv* env, jclass, jstring locale) {
    return getIterator(env, locale, UBRK_WORD);
}

static UBreakIterator* breakIterator(jint address) {
    return reinterpret_cast<UBreakIterator*>(static_cast<uintptr_t>(address));
}

static void NativeBreakIterator_closeBreakIteratorImpl(JNIEnv*, jclass, jint address) {
    ubrk_close(breakIterator(address));
}

static jint NativeBreakIterator_cloneImpl(JNIEnv* env, jclass, jint address) {
    UErrorCode status = U_ZERO_ERROR;
    jint bufferSize = U_BRK_SAFECLONE_BUFFERSIZE;
    UBreakIterator* it = ubrk_safeClone(breakIterator(address), NULL, &bufferSize, &status);
    icu4jni_error(env, status);
    return reinterpret_cast<uintptr_t>(it);
}

static void NativeBreakIterator_setTextImpl(JNIEnv* env, jclass, jint address, jstring javaText) {
    ScopedJavaUnicodeString text(env, javaText);
    UnicodeString& s(text.unicodeString());
    UErrorCode status = U_ZERO_ERROR;
    ubrk_setText(breakIterator(address), s.getBuffer(), s.length(), &status);
    icu4jni_error(env, status);
}

static jboolean NativeBreakIterator_isBoundaryImpl(JNIEnv*, jclass, jint address, jint offset) {
    return ubrk_isBoundary(breakIterator(address), offset);
}

static jint NativeBreakIterator_nextImpl(JNIEnv*, jclass, jint address, jint n) {
    UBreakIterator* bi = breakIterator(address);
    if (n < 0) {
        while (n++ < -1) {
            ubrk_previous(bi);
        }
        return ubrk_previous(bi);
    } else if (n == 0) {
        return ubrk_current(bi);
    } else {
        while (n-- > 1) {
            ubrk_next(bi);
        }
        return ubrk_next(bi);
    }
    return -1;
}

static jint NativeBreakIterator_precedingImpl(JNIEnv*, jclass, jint address, jint offset) {
    return ubrk_preceding(breakIterator(address), offset);
}

static jint NativeBreakIterator_firstImpl(JNIEnv*, jclass, jint address) {
    return ubrk_first(breakIterator(address));
}

static jint NativeBreakIterator_followingImpl(JNIEnv*, jclass, jint address, jint offset) {
    return ubrk_following(breakIterator(address), offset);
}

static jint NativeBreakIterator_currentImpl(JNIEnv*, jclass, jint address) {
    return ubrk_current(breakIterator(address));
}

static jint NativeBreakIterator_previousImpl(JNIEnv*, jclass, jint address) {
    return ubrk_previous(breakIterator(address));
}

static jint NativeBreakIterator_lastImpl(JNIEnv*, jclass, jint address) {
    return ubrk_last(breakIterator(address));
}

static JNINativeMethod gMethods[] = {
    NATIVE_METHOD(NativeBreakIterator, cloneImpl, "(I)I"),
    NATIVE_METHOD(NativeBreakIterator, closeBreakIteratorImpl, "(I)V"),
    NATIVE_METHOD(NativeBreakIterator, currentImpl, "(I)I"),
    NATIVE_METHOD(NativeBreakIterator, firstImpl, "(I)I"),
    NATIVE_METHOD(NativeBreakIterator, followingImpl, "(II)I"),
    NATIVE_METHOD(NativeBreakIterator, getCharacterInstanceImpl, "(Ljava/lang/String;)I"),
    NATIVE_METHOD(NativeBreakIterator, getLineInstanceImpl, "(Ljava/lang/String;)I"),
    NATIVE_METHOD(NativeBreakIterator, getSentenceInstanceImpl, "(Ljava/lang/String;)I"),
    NATIVE_METHOD(NativeBreakIterator, getWordInstanceImpl, "(Ljava/lang/String;)I"),
    NATIVE_METHOD(NativeBreakIterator, isBoundaryImpl, "(II)Z"),
    NATIVE_METHOD(NativeBreakIterator, lastImpl, "(I)I"),
    NATIVE_METHOD(NativeBreakIterator, nextImpl, "(II)I"),
    NATIVE_METHOD(NativeBreakIterator, precedingImpl, "(II)I"),
    NATIVE_METHOD(NativeBreakIterator, previousImpl, "(I)I"),
    NATIVE_METHOD(NativeBreakIterator, setTextImpl, "(ILjava/lang/String;)V"),
};
int register_com_ibm_icu4jni_text_NativeBreakIterator(JNIEnv* env) {
    return jniRegisterNativeMethods(env, "com/ibm/icu4jni/text/NativeBreakIterator",
            gMethods, NELEM(gMethods));
}
