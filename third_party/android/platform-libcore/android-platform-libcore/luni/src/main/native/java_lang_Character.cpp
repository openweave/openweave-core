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

#define LOG_TAG "Character"

#include "JNIHelp.h"
#include "JniConstants.h"
#include "ScopedUtfChars.h"
#include "unicode/uchar.h"
#include <math.h>
#include <stdlib.h>

static jint Character_digitImpl(JNIEnv*, jclass, jint codePoint, jint radix) {
    return u_digit(codePoint, radix);
}

static jint Character_getTypeImpl(JNIEnv*, jclass, jint codePoint) {
    return u_charType(codePoint);
}

static jbyte Character_getDirectionalityImpl(JNIEnv*, jclass, jint codePoint) {
    return u_charDirection(codePoint);
}

static jboolean Character_isMirroredImpl(JNIEnv*, jclass, jint codePoint) {
    return u_isMirrored(codePoint);
}

static jint Character_getNumericValueImpl(JNIEnv*, jclass, jint codePoint){
    // The letters A-Z in their uppercase ('\u0041' through '\u005A'),
    //                          lowercase ('\u0061' through '\u007A'),
    //             and full width variant ('\uFF21' through '\uFF3A'
    //                                 and '\uFF41' through '\uFF5A') forms
    // have numeric values from 10 through 35. This is independent of the
    // Unicode specification, which does not assign numeric values to these
    // char values.
    if (codePoint >= 0x41 && codePoint <= 0x5A) {
        return codePoint - 0x37;
    }
    if (codePoint >= 0x61 && codePoint <= 0x7A) {
        return codePoint - 0x57;
    }
    if (codePoint >= 0xFF21 && codePoint <= 0xFF3A) {
        return codePoint - 0xFF17;
    }
    if (codePoint >= 0xFF41 && codePoint <= 0xFF5A) {
        return codePoint - 0xFF37;
    }

    double result = u_getNumericValue(codePoint);

    if (result == U_NO_NUMERIC_VALUE) {
        return -1;
    } else if (result < 0 || floor(result + 0.5) != result) {
        return -2;
    }

    return result;
}

static jboolean Character_isDefinedImpl(JNIEnv*, jclass, jint codePoint) {
    return u_isdefined(codePoint);
}

static jboolean Character_isDigitImpl(JNIEnv*, jclass, jint codePoint) {
    return u_isdigit(codePoint);
}

static jboolean Character_isIdentifierIgnorableImpl(JNIEnv*, jclass, jint codePoint) {
    // Java also returns true for U+0085 Next Line (it omits U+0085 from whitespace ISO controls).
    if(codePoint == 0x0085) {
        return JNI_TRUE;
    }
    return u_isIDIgnorable(codePoint);
}

static jboolean Character_isLetterImpl(JNIEnv*, jclass, jint codePoint) {
    return u_isalpha(codePoint);
}

static jboolean Character_isLetterOrDigitImpl(JNIEnv*, jclass, jint codePoint) {
    return u_isalnum(codePoint);
}

static jboolean Character_isSpaceCharImpl(JNIEnv*, jclass, jint codePoint) {
    return u_isJavaSpaceChar(codePoint);
}

static jboolean Character_isTitleCaseImpl(JNIEnv*, jclass, jint codePoint) {
    return u_istitle(codePoint);
}

static jboolean Character_isUnicodeIdentifierPartImpl(JNIEnv*, jclass, jint codePoint) {
    return u_isIDPart(codePoint);
}

static jboolean Character_isUnicodeIdentifierStartImpl(JNIEnv*, jclass, jint codePoint) {
    return u_isIDStart(codePoint);
}

static jboolean Character_isWhitespaceImpl(JNIEnv*, jclass, jint codePoint) {
    // Java omits U+0085
    if(codePoint == 0x0085) {
        return JNI_FALSE;
    }
    return u_isWhitespace(codePoint);
}

static jint Character_toLowerCaseImpl(JNIEnv*, jclass, jint codePoint) {
    return u_tolower(codePoint);
}

static jint Character_toTitleCaseImpl(JNIEnv*, jclass, jint codePoint) {
    return u_totitle(codePoint);
}

static jint Character_toUpperCaseImpl(JNIEnv*, jclass, jint codePoint) {
    return u_toupper(codePoint);
}

static jboolean Character_isUpperCaseImpl(JNIEnv*, jclass, jint codePoint) {
    return u_isupper(codePoint);
}

static jboolean Character_isLowerCaseImpl(JNIEnv*, jclass, jint codePoint) {
    return u_islower(codePoint);
}

static int Character_forNameImpl(JNIEnv* env, jclass, jstring javaBlockName) {
    ScopedUtfChars blockName(env, javaBlockName);
    if (blockName.c_str() == NULL) {
        return 0;
    }
    return u_getPropertyValueEnum(UCHAR_BLOCK, blockName.c_str());
}

static int Character_ofImpl(JNIEnv*, jclass, jint codePoint) {
    return ublock_getCode(codePoint);
}

static JNINativeMethod gMethods[] = {
    NATIVE_METHOD(Character, digitImpl, "(II)I"),
    NATIVE_METHOD(Character, forNameImpl, "(Ljava/lang/String;)I"),
    NATIVE_METHOD(Character, getDirectionalityImpl, "(I)B"),
    NATIVE_METHOD(Character, getNumericValueImpl, "(I)I"),
    NATIVE_METHOD(Character, getTypeImpl, "(I)I"),
    NATIVE_METHOD(Character, isDefinedImpl, "(I)Z"),
    NATIVE_METHOD(Character, isDigitImpl, "(I)Z"),
    NATIVE_METHOD(Character, isIdentifierIgnorableImpl, "(I)Z"),
    NATIVE_METHOD(Character, isLetterImpl, "(I)Z"),
    NATIVE_METHOD(Character, isLetterOrDigitImpl, "(I)Z"),
    NATIVE_METHOD(Character, isLowerCaseImpl, "(I)Z"),
    NATIVE_METHOD(Character, isMirroredImpl, "(I)Z"),
    NATIVE_METHOD(Character, isSpaceCharImpl, "(I)Z"),
    NATIVE_METHOD(Character, isTitleCaseImpl, "(I)Z"),
    NATIVE_METHOD(Character, isUnicodeIdentifierPartImpl, "(I)Z"),
    NATIVE_METHOD(Character, isUnicodeIdentifierStartImpl, "(I)Z"),
    NATIVE_METHOD(Character, isUpperCaseImpl, "(I)Z"),
    NATIVE_METHOD(Character, isWhitespaceImpl, "(I)Z"),
    NATIVE_METHOD(Character, ofImpl, "(I)I"),
    NATIVE_METHOD(Character, toLowerCaseImpl, "(I)I"),
    NATIVE_METHOD(Character, toTitleCaseImpl, "(I)I"),
    NATIVE_METHOD(Character, toUpperCaseImpl, "(I)I"),
};
int register_java_lang_Character(JNIEnv* env) {
    return jniRegisterNativeMethods(env, "java/lang/Character", gMethods, NELEM(gMethods));
}
