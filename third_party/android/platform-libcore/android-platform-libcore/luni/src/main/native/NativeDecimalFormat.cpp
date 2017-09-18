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

#define LOG_TAG "NativeDecimalFormat"

#include "ErrorCode.h"
#include "JNIHelp.h"
#include "JniConstants.h"
#include "ScopedJavaUnicodeString.h"
#include "ScopedPrimitiveArray.h"
#include "ScopedUtfChars.h"
#include "UniquePtr.h"
#include "cutils/log.h"
#include "digitlst.h"
#include "unicode/decimfmt.h"
#include "unicode/fmtable.h"
#include "unicode/numfmt.h"
#include "unicode/unum.h"
#include "unicode/ustring.h"
#include "valueOf.h"
#include <stdlib.h>
#include <string.h>

static DecimalFormat* toDecimalFormat(jint addr) {
    return reinterpret_cast<DecimalFormat*>(static_cast<uintptr_t>(addr));
}

static UNumberFormat* toUNumberFormat(jint addr) {
    return reinterpret_cast<UNumberFormat*>(static_cast<uintptr_t>(addr));
}

static DecimalFormatSymbols* makeDecimalFormatSymbols(JNIEnv* env,
        jstring currencySymbol0, jchar decimalSeparator, jchar digit, jstring exponentSeparator0,
        jchar groupingSeparator0, jstring infinity0,
        jstring internationalCurrencySymbol0, jchar minusSign,
        jchar monetaryDecimalSeparator, jstring nan0, jchar patternSeparator,
        jchar percent, jchar perMill, jchar zeroDigit) {
    ScopedJavaUnicodeString currencySymbol(env, currencySymbol0);
    ScopedJavaUnicodeString exponentSeparator(env, exponentSeparator0);
    ScopedJavaUnicodeString infinity(env, infinity0);
    ScopedJavaUnicodeString internationalCurrencySymbol(env, internationalCurrencySymbol0);
    ScopedJavaUnicodeString nan(env, nan0);
    UnicodeString groupingSeparator(groupingSeparator0);

    DecimalFormatSymbols* result = new DecimalFormatSymbols;
    result->setSymbol(DecimalFormatSymbols::kCurrencySymbol, currencySymbol.unicodeString());
    result->setSymbol(DecimalFormatSymbols::kDecimalSeparatorSymbol, UnicodeString(decimalSeparator));
    result->setSymbol(DecimalFormatSymbols::kDigitSymbol, UnicodeString(digit));
    result->setSymbol(DecimalFormatSymbols::kExponentialSymbol, exponentSeparator.unicodeString());
    result->setSymbol(DecimalFormatSymbols::kGroupingSeparatorSymbol, groupingSeparator);
    result->setSymbol(DecimalFormatSymbols::kMonetaryGroupingSeparatorSymbol, groupingSeparator);
    result->setSymbol(DecimalFormatSymbols::kInfinitySymbol, infinity.unicodeString());
    result->setSymbol(DecimalFormatSymbols::kIntlCurrencySymbol, internationalCurrencySymbol.unicodeString());
    result->setSymbol(DecimalFormatSymbols::kMinusSignSymbol, UnicodeString(minusSign));
    result->setSymbol(DecimalFormatSymbols::kMonetarySeparatorSymbol, UnicodeString(monetaryDecimalSeparator));
    result->setSymbol(DecimalFormatSymbols::kNaNSymbol, nan.unicodeString());
    result->setSymbol(DecimalFormatSymbols::kPatternSeparatorSymbol, UnicodeString(patternSeparator));
    result->setSymbol(DecimalFormatSymbols::kPercentSymbol, UnicodeString(percent));
    result->setSymbol(DecimalFormatSymbols::kPerMillSymbol, UnicodeString(perMill));
    result->setSymbol(DecimalFormatSymbols::kZeroDigitSymbol, UnicodeString(zeroDigit));
    return result;
}

static void NativeDecimalFormat_setDecimalFormatSymbols(JNIEnv* env, jclass, jint addr,
        jstring currencySymbol, jchar decimalSeparator, jchar digit, jstring exponentSeparator,
        jchar groupingSeparator, jstring infinity,
        jstring internationalCurrencySymbol, jchar minusSign,
        jchar monetaryDecimalSeparator, jstring nan, jchar patternSeparator,
        jchar percent, jchar perMill, jchar zeroDigit) {
    DecimalFormatSymbols* symbols = makeDecimalFormatSymbols(env,
            currencySymbol, decimalSeparator, digit, exponentSeparator, groupingSeparator,
            infinity, internationalCurrencySymbol, minusSign,
            monetaryDecimalSeparator, nan, patternSeparator, percent, perMill,
            zeroDigit);
    toDecimalFormat(addr)->adoptDecimalFormatSymbols(symbols);
}

static jint NativeDecimalFormat_open(JNIEnv* env, jclass, jstring pattern0,
        jstring currencySymbol, jchar decimalSeparator, jchar digit, jstring exponentSeparator,
        jchar groupingSeparator, jstring infinity,
        jstring internationalCurrencySymbol, jchar minusSign,
        jchar monetaryDecimalSeparator, jstring nan, jchar patternSeparator,
        jchar percent, jchar perMill, jchar zeroDigit) {
    if (pattern0 == NULL) {
        jniThrowNullPointerException(env, NULL);
        return 0;
    }
    UErrorCode status = U_ZERO_ERROR;
    UParseError parseError;
    ScopedJavaUnicodeString pattern(env, pattern0);
    DecimalFormatSymbols* symbols = makeDecimalFormatSymbols(env,
            currencySymbol, decimalSeparator, digit, exponentSeparator, groupingSeparator,
            infinity, internationalCurrencySymbol, minusSign,
            monetaryDecimalSeparator, nan, patternSeparator, percent, perMill,
            zeroDigit);
    DecimalFormat* fmt = new DecimalFormat(pattern.unicodeString(), symbols, parseError, status);
    if (fmt == NULL) {
        delete symbols;
    }
    icu4jni_error(env, status);
    return static_cast<jint>(reinterpret_cast<uintptr_t>(fmt));
}

static void NativeDecimalFormat_close(JNIEnv*, jclass, jint addr) {
    delete toDecimalFormat(addr);
}

static void NativeDecimalFormat_setRoundingMode(JNIEnv*, jclass, jint addr, jint mode, jdouble increment) {
    DecimalFormat* fmt = toDecimalFormat(addr);
    fmt->setRoundingMode(static_cast<DecimalFormat::ERoundingMode>(mode));
    fmt->setRoundingIncrement(increment);
}

static void NativeDecimalFormat_setSymbol(JNIEnv* env, jclass, jint addr, jint javaSymbol, jstring javaValue) {
    ScopedJavaUnicodeString value(env, javaValue);
    UnicodeString& s(value.unicodeString());
    UErrorCode status = U_ZERO_ERROR;
    UNumberFormatSymbol symbol = static_cast<UNumberFormatSymbol>(javaSymbol);
    unum_setSymbol(toUNumberFormat(addr), symbol, s.getBuffer(), s.length(), &status);
    icu4jni_error(env, status);
}

static void NativeDecimalFormat_setAttribute(JNIEnv*, jclass, jint addr, jint javaAttr, jint value) {
    UNumberFormatAttribute attr = static_cast<UNumberFormatAttribute>(javaAttr);
    unum_setAttribute(toUNumberFormat(addr), attr, value);
}

static jint NativeDecimalFormat_getAttribute(JNIEnv*, jclass, jint addr, jint javaAttr) {
    UNumberFormatAttribute attr = static_cast<UNumberFormatAttribute>(javaAttr);
    return unum_getAttribute(toUNumberFormat(addr), attr);
}

static void NativeDecimalFormat_setTextAttribute(JNIEnv* env, jclass, jint addr, jint javaAttr, jstring javaValue) {
    ScopedJavaUnicodeString value(env, javaValue);
    UnicodeString& s(value.unicodeString());
    UErrorCode status = U_ZERO_ERROR;
    UNumberFormatTextAttribute attr = static_cast<UNumberFormatTextAttribute>(javaAttr);
    unum_setTextAttribute(toUNumberFormat(addr), attr, s.getBuffer(), s.length(), &status);
    icu4jni_error(env, status);
}

static jstring NativeDecimalFormat_getTextAttribute(JNIEnv* env, jclass, jint addr, jint javaAttr) {
    UErrorCode status = U_ZERO_ERROR;
    UNumberFormat* fmt = toUNumberFormat(addr);
    UNumberFormatTextAttribute attr = static_cast<UNumberFormatTextAttribute>(javaAttr);

    // Find out how long the result will be...
    UniquePtr<UChar[]> chars;
    uint32_t charCount = 0;
    uint32_t desiredCount = unum_getTextAttribute(fmt, attr, chars.get(), charCount, &status);
    if (status == U_BUFFER_OVERFLOW_ERROR) {
        // ...then get it.
        status = U_ZERO_ERROR;
        charCount = desiredCount + 1;
        chars.reset(new UChar[charCount]);
        charCount = unum_getTextAttribute(fmt, attr, chars.get(), charCount, &status);
    }
    return icu4jni_error(env, status) ? NULL : env->NewString(chars.get(), charCount);
}

static void NativeDecimalFormat_applyPatternImpl(JNIEnv* env, jclass, jint addr, jboolean localized, jstring pattern0) {
    if (pattern0 == NULL) {
        jniThrowNullPointerException(env, NULL);
        return;
    }
    ScopedJavaUnicodeString pattern(env, pattern0);
    DecimalFormat* fmt = toDecimalFormat(addr);
    UErrorCode status = U_ZERO_ERROR;
    if (localized) {
        fmt->applyLocalizedPattern(pattern.unicodeString(), status);
    } else {
        fmt->applyPattern(pattern.unicodeString(), status);
    }
    icu4jni_error(env, status);
}

static jstring NativeDecimalFormat_toPatternImpl(JNIEnv* env, jclass, jint addr, jboolean localized) {
    DecimalFormat* fmt = toDecimalFormat(addr);
    UnicodeString pattern;
    if (localized) {
        fmt->toLocalizedPattern(pattern);
    } else {
        fmt->toPattern(pattern);
    }
    return env->NewString(pattern.getBuffer(), pattern.length());
}

static jcharArray formatResult(JNIEnv* env, const UnicodeString &str, FieldPositionIterator* fpi, jobject fpIter) {
    static jmethodID gFPI_setData = env->GetMethodID(JniConstants::fieldPositionIteratorClass, "setData", "([I)V");

    if (fpi != NULL) {
        int len = fpi->getData(NULL, 0);
        jintArray data = NULL;
        if (len) {
            data = env->NewIntArray(len);
            ScopedIntArrayRW ints(env, data);
            if (ints.get() == NULL) {
                return NULL;
            }
            fpi->getData(ints.get(), len);
        }
        env->CallVoidMethod(fpIter, gFPI_setData, data);
    }

    jcharArray result = env->NewCharArray(str.length());
    if (result != NULL) {
        env->SetCharArrayRegion(result, 0, str.length(), str.getBuffer());
    }
    return result;
}

template <typename T>
static jcharArray format(JNIEnv* env, jint addr, jobject fpIter, T val) {
    UErrorCode status = U_ZERO_ERROR;
    UnicodeString str;
    DecimalFormat* fmt = toDecimalFormat(addr);
    FieldPositionIterator fpi;
    FieldPositionIterator* pfpi = fpIter ? &fpi : NULL;
    fmt->format(val, str, pfpi, status);
    return formatResult(env, str, pfpi, fpIter);
}

static jcharArray NativeDecimalFormat_formatLong(JNIEnv* env, jclass, jint addr, jlong value, jobject fpIter) {
    return format(env, addr, fpIter, value);
}

static jcharArray NativeDecimalFormat_formatDouble(JNIEnv* env, jclass, jint addr, jdouble value, jobject fpIter) {
    return format(env, addr, fpIter, value);
}

static jcharArray NativeDecimalFormat_formatDigitList(JNIEnv* env, jclass, jint addr, jstring value, jobject fpIter) {
    ScopedUtfChars chars(env, value);
    if (chars.c_str() == NULL) {
        return NULL;
    }
    StringPiece sp(chars.c_str());
    return format(env, addr, fpIter, sp);
}

static jobject newBigDecimal(JNIEnv* env, const char* value, jsize len) {
    static jmethodID gBigDecimal_init = env->GetMethodID(JniConstants::bigDecimalClass, "<init>", "(Ljava/lang/String;)V");

    // this is painful...
    // value is a UTF-8 string of invariant characters, but isn't guaranteed to be
    // null-terminated.  NewStringUTF requires a terminated UTF-8 string.  So we copy the
    // data to jchars using UnicodeString, and call NewString instead.
    UnicodeString tmp(value, len, UnicodeString::kInvariant);
    jobject str = env->NewString(tmp.getBuffer(), tmp.length());
    return env->NewObject(JniConstants::bigDecimalClass, gBigDecimal_init, str);
}

static jobject NativeDecimalFormat_parse(JNIEnv* env, jclass, jint addr, jstring text,
        jobject position, jboolean parseBigDecimal) {

    static jmethodID gPP_getIndex = env->GetMethodID(JniConstants::parsePositionClass, "getIndex", "()I");
    static jmethodID gPP_setIndex = env->GetMethodID(JniConstants::parsePositionClass, "setIndex", "(I)V");
    static jmethodID gPP_setErrorIndex = env->GetMethodID(JniConstants::parsePositionClass, "setErrorIndex", "(I)V");

    // make sure the ParsePosition is valid. Actually icu4c would parse a number
    // correctly even if the parsePosition is set to -1, but since the RI fails
    // for that case we have to fail too
    int parsePos = env->CallIntMethod(position, gPP_getIndex, NULL);
    if (parsePos < 0 || parsePos > env->GetStringLength(text)) {
        return NULL;
    }

    Formattable res;
    ParsePosition pp(parsePos);
    ScopedJavaUnicodeString src(env, text);
    DecimalFormat* fmt = toDecimalFormat(addr);
    fmt->parse(src.unicodeString(), res, pp);

    if (pp.getErrorIndex() == -1) {
        env->CallVoidMethod(position, gPP_setIndex, (jint) pp.getIndex());
    } else {
        env->CallVoidMethod(position, gPP_setErrorIndex, (jint) pp.getErrorIndex());
        return NULL;
    }

    if (parseBigDecimal) {
        UErrorCode status = U_ZERO_ERROR;
        StringPiece str = res.getDecimalNumber(status);
        if (U_SUCCESS(status)) {
            int len = str.length();
            const char* data = str.data();
            if (strncmp(data, "NaN", 3) == 0 ||
                strncmp(data, "Inf", 3) == 0 ||
                strncmp(data, "-Inf", 4) == 0) {
                double resultDouble = res.getDouble(status);
                return doubleValueOf(env, (jdouble) resultDouble);
            }
            return newBigDecimal(env, data, len);
        }
        return NULL;
    }

    Formattable::Type numType = res.getType();
        switch(numType) {
        case Formattable::kDouble: {
            double resultDouble = res.getDouble();
            return doubleValueOf(env, (jdouble) resultDouble);
        }
        case Formattable::kLong: {
            long resultLong = res.getLong();
            return longValueOf(env, (jlong) resultLong);
        }
        case Formattable::kInt64: {
            int64_t resultInt64 = res.getInt64();
            return longValueOf(env, (jlong) resultInt64);
        }
        default: {
            return NULL;
        }
    }
}

static jint NativeDecimalFormat_cloneImpl(JNIEnv*, jclass, jint addr) {
    DecimalFormat* fmt = toDecimalFormat(addr);
    return static_cast<jint>(reinterpret_cast<uintptr_t>(fmt->clone()));
}

static JNINativeMethod gMethods[] = {
    NATIVE_METHOD(NativeDecimalFormat, applyPatternImpl, "(IZLjava/lang/String;)V"),
    NATIVE_METHOD(NativeDecimalFormat, cloneImpl, "(I)I"),
    NATIVE_METHOD(NativeDecimalFormat, close, "(I)V"),
    NATIVE_METHOD(NativeDecimalFormat, formatDouble, "(IDLcom/ibm/icu4jni/text/NativeDecimalFormat$FieldPositionIterator;)[C"),
    NATIVE_METHOD(NativeDecimalFormat, formatLong, "(IJLcom/ibm/icu4jni/text/NativeDecimalFormat$FieldPositionIterator;)[C"),
    NATIVE_METHOD(NativeDecimalFormat, formatDigitList, "(ILjava/lang/String;Lcom/ibm/icu4jni/text/NativeDecimalFormat$FieldPositionIterator;)[C"),
    NATIVE_METHOD(NativeDecimalFormat, getAttribute, "(II)I"),
    NATIVE_METHOD(NativeDecimalFormat, getTextAttribute, "(II)Ljava/lang/String;"),
    NATIVE_METHOD(NativeDecimalFormat, open, "(Ljava/lang/String;Ljava/lang/String;CCLjava/lang/String;CLjava/lang/String;Ljava/lang/String;CCLjava/lang/String;CCCC)I"),
    NATIVE_METHOD(NativeDecimalFormat, parse, "(ILjava/lang/String;Ljava/text/ParsePosition;Z)Ljava/lang/Number;"),
    NATIVE_METHOD(NativeDecimalFormat, setAttribute, "(III)V"),
    NATIVE_METHOD(NativeDecimalFormat, setDecimalFormatSymbols, "(ILjava/lang/String;CCLjava/lang/String;CLjava/lang/String;Ljava/lang/String;CCLjava/lang/String;CCCC)V"),
    NATIVE_METHOD(NativeDecimalFormat, setRoundingMode, "(IID)V"),
    NATIVE_METHOD(NativeDecimalFormat, setSymbol, "(IILjava/lang/String;)V"),
    NATIVE_METHOD(NativeDecimalFormat, setTextAttribute, "(IILjava/lang/String;)V"),
    NATIVE_METHOD(NativeDecimalFormat, toPatternImpl, "(IZ)Ljava/lang/String;"),
};
int register_com_ibm_icu4jni_text_NativeDecimalFormat(JNIEnv* env) {
    return jniRegisterNativeMethods(env, "com/ibm/icu4jni/text/NativeDecimalFormat", gMethods,
            NELEM(gMethods));
}
