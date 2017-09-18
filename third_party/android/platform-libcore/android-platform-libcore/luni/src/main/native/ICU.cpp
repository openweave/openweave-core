/*
 * Copyright (C) 2008 The Android Open Source Project
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

#define LOG_TAG "ICU"

#include "ErrorCode.h"
#include "JNIHelp.h"
#include "JniConstants.h"
#include "ScopedJavaUnicodeString.h"
#include "ScopedLocalRef.h"
#include "ScopedUtfChars.h"
#include "UniquePtr.h"
#include "cutils/log.h"
#include "unicode/calendar.h"
#include "unicode/datefmt.h"
#include "unicode/dcfmtsym.h"
#include "unicode/decimfmt.h"
#include "unicode/dtfmtsym.h"
#include "unicode/gregocal.h"
#include "unicode/locid.h"
#include "unicode/numfmt.h"
#include "unicode/strenum.h"
#include "unicode/ubrk.h"
#include "unicode/ucal.h"
#include "unicode/uclean.h"
#include "unicode/ucol.h"
#include "unicode/ucurr.h"
#include "unicode/udat.h"
#include "unicode/ustring.h"
#include "ureslocs.h"
#include "valueOf.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

class ScopedResourceBundle {
public:
    ScopedResourceBundle(UResourceBundle* bundle) : mBundle(bundle) {
    }

    ~ScopedResourceBundle() {
        if (mBundle != NULL) {
            ures_close(mBundle);
        }
    }

    UResourceBundle* get() {
        return mBundle;
    }

private:
    UResourceBundle* mBundle;

    // Disallow copy and assignment.
    ScopedResourceBundle(const ScopedResourceBundle&);
    void operator=(const ScopedResourceBundle&);
};

Locale getLocale(JNIEnv* env, jstring localeName) {
    return Locale::createFromName(ScopedUtfChars(env, localeName).c_str());
}

static jint ICU_getCurrencyFractionDigitsNative(JNIEnv* env, jclass, jstring javaCurrencyCode) {
    UErrorCode status = U_ZERO_ERROR;
    UniquePtr<NumberFormat> fmt(NumberFormat::createCurrencyInstance(status));
    if (U_FAILURE(status)) {
        return -1;
    }
    ScopedJavaUnicodeString currencyCode(env, javaCurrencyCode);
    fmt->setCurrency(currencyCode.unicodeString().getBuffer(), status);
    if (U_FAILURE(status)) {
        return -1;
    }
    // for CurrencyFormats the minimum and maximum fraction digits are the same.
    return fmt->getMinimumFractionDigits();
}

static jstring ICU_getCurrencyCodeNative(JNIEnv* env, jclass, jstring javaKey) {
    UErrorCode status = U_ZERO_ERROR;
    ScopedResourceBundle supplData(ures_openDirect(U_ICUDATA_CURR, "supplementalData", &status));
    if (U_FAILURE(status)) {
        return NULL;
    }

    ScopedResourceBundle currencyMap(ures_getByKey(supplData.get(), "CurrencyMap", NULL, &status));
    if (U_FAILURE(status)) {
        return NULL;
    }

    ScopedUtfChars key(env, javaKey);
    ScopedResourceBundle currency(ures_getByKey(currencyMap.get(), key.c_str(), NULL, &status));
    if (U_FAILURE(status)) {
        return NULL;
    }

    ScopedResourceBundle currencyElem(ures_getByIndex(currency.get(), 0, NULL, &status));
    if (U_FAILURE(status)) {
        return env->NewStringUTF("None");
    }

    // check if there is a 'to' date. If there is, the currency isn't used anymore.
    ScopedResourceBundle currencyTo(ures_getByKey(currencyElem.get(), "to", NULL, &status));
    if (!U_FAILURE(status)) {
        // return and let the caller throw an exception
        return NULL;
    }
    // We need to reset 'status'. It works like errno in that ICU doesn't set it
    // to U_ZERO_ERROR on success: it only touches it on error, and the test
    // above means it now holds a failure code.
    status = U_ZERO_ERROR;

    ScopedResourceBundle currencyId(ures_getByKey(currencyElem.get(), "id", NULL, &status));
    if (U_FAILURE(status)) {
        // No id defined for this country
        return env->NewStringUTF("None");
    }

    int length;
    const jchar* id = ures_getString(currencyId.get(), &length, &status);
    if (U_FAILURE(status) || length == 0) {
        return env->NewStringUTF("None");
    }
    return env->NewString(id, length);
}

static jstring ICU_getCurrencySymbolNative(JNIEnv* env, jclass, jstring locale, jstring currencyCode) {
    ScopedUtfChars localeName(env, locale);
    UErrorCode status = U_ZERO_ERROR;
    ScopedResourceBundle currLoc(ures_open(U_ICUDATA_CURR, localeName.c_str(), &status));
    if (U_FAILURE(status)) {
        return NULL;
    }

    ScopedResourceBundle currencies(ures_getByKey(currLoc.get(), "Currencies", NULL, &status));
    if (U_FAILURE(status)) {
        return NULL;
    }

    ScopedUtfChars currency(env, currencyCode);
    ScopedResourceBundle currencyElems(ures_getByKey(currencies.get(), currency.c_str(), NULL, &status));
    if (U_FAILURE(status)) {
        return NULL;
    }

    int currSymbL;
    const jchar* currSymbU = ures_getStringByIndex(currencyElems.get(), 0, &currSymbL, &status);
    if (U_FAILURE(status)) {
        return NULL;
    }

    return (currSymbL == 0) ? NULL : env->NewString(currSymbU, currSymbL);
}

static jstring ICU_getDisplayCountryNative(JNIEnv* env, jclass, jstring targetLocale, jstring locale) {
    Locale loc = getLocale(env, locale);
    Locale targetLoc = getLocale(env, targetLocale);
    UnicodeString str;
    targetLoc.getDisplayCountry(loc, str);
    return env->NewString(str.getBuffer(), str.length());
}

static jstring ICU_getDisplayLanguageNative(JNIEnv* env, jclass, jstring targetLocale, jstring locale) {
    Locale loc = getLocale(env, locale);
    Locale targetLoc = getLocale(env, targetLocale);
    UnicodeString str;
    targetLoc.getDisplayLanguage(loc, str);
    return env->NewString(str.getBuffer(), str.length());
}

static jstring ICU_getDisplayVariantNative(JNIEnv* env, jclass, jstring targetLocale, jstring locale) {
    Locale loc = getLocale(env, locale);
    Locale targetLoc = getLocale(env, targetLocale);
    UnicodeString str;
    targetLoc.getDisplayVariant(loc, str);
    return env->NewString(str.getBuffer(), str.length());
}

static jstring ICU_getISO3CountryNative(JNIEnv* env, jclass, jstring locale) {
    Locale loc = getLocale(env, locale);
    return env->NewStringUTF(loc.getISO3Country());
}

static jstring ICU_getISO3LanguageNative(JNIEnv* env, jclass, jstring locale) {
    Locale loc = getLocale(env, locale);
    return env->NewStringUTF(loc.getISO3Language());
}

static jobjectArray toStringArray(JNIEnv* env, const char* const* strings) {
    size_t count = 0;
    while (strings[count] != NULL) {
        ++count;
    }
    jobjectArray result = env->NewObjectArray(count, JniConstants::stringClass, NULL);
    for (size_t i = 0; i < count; ++i) {
        ScopedLocalRef<jstring> s(env, env->NewStringUTF(strings[i]));
        env->SetObjectArrayElement(result, i, s.get());
    }
    return result;
}

static jobjectArray ICU_getISOCountriesNative(JNIEnv* env, jclass) {
    return toStringArray(env, Locale::getISOCountries());
}

static jobjectArray ICU_getISOLanguagesNative(JNIEnv* env, jclass) {
    return toStringArray(env, Locale::getISOLanguages());
}

template <typename Counter, typename Getter>
static jobjectArray getAvailableLocales(JNIEnv* env, Counter* counter, Getter* getter) {
    size_t count = (*counter)();
    jobjectArray result = env->NewObjectArray(count, JniConstants::stringClass, NULL);
    for (size_t i = 0; i < count; ++i) {
        ScopedLocalRef<jstring> s(env, env->NewStringUTF((*getter)(i)));
        env->SetObjectArrayElement(result, i, s.get());
    }
    return result;
}

static jobjectArray ICU_getAvailableLocalesNative(JNIEnv* env, jclass) {
    return getAvailableLocales(env, uloc_countAvailable, uloc_getAvailable);
}

static jobjectArray ICU_getAvailableBreakIteratorLocalesNative(JNIEnv* env, jclass) {
    return getAvailableLocales(env, ubrk_countAvailable, ubrk_getAvailable);
}

static jobjectArray ICU_getAvailableCalendarLocalesNative(JNIEnv* env, jclass) {
    return getAvailableLocales(env, ucal_countAvailable, ucal_getAvailable);
}

static jobjectArray ICU_getAvailableCollatorLocalesNative(JNIEnv* env, jclass) {
    return getAvailableLocales(env, ucol_countAvailable, ucol_getAvailable);
}

static jobjectArray ICU_getAvailableDateFormatLocalesNative(JNIEnv* env, jclass) {
    return getAvailableLocales(env, udat_countAvailable, udat_getAvailable);
}

static jobjectArray ICU_getAvailableNumberFormatLocalesNative(JNIEnv* env, jclass) {
    return getAvailableLocales(env, unum_countAvailable, unum_getAvailable);
}

static bool getDayIntVector(JNIEnv*, UResourceBundle* gregorian, int* values) {
    // get the First day of week and the minimal days in first week numbers
    UErrorCode status = U_ZERO_ERROR;
    ScopedResourceBundle gregorianElems(ures_getByKey(gregorian, "DateTimeElements", NULL, &status));
    if (U_FAILURE(status)) {
        return false;
    }

    int intVectSize;
    const int* result = ures_getIntVector(gregorianElems.get(), &intVectSize, &status);
    if (U_FAILURE(status) || intVectSize != 2) {
        return false;
    }

    values[0] = result[0];
    values[1] = result[1];
    return true;
}

static jobjectArray getAmPmMarkers(JNIEnv* env, UResourceBundle* gregorian) {
    UErrorCode status = U_ZERO_ERROR;
    ScopedResourceBundle gregorianElems(ures_getByKey(gregorian, "AmPmMarkers", NULL, &status));
    if (U_FAILURE(status)) {
        return NULL;
    }

    int lengthAm, lengthPm;
    const jchar* am = ures_getStringByIndex(gregorianElems.get(), 0, &lengthAm, &status);
    const jchar* pm = ures_getStringByIndex(gregorianElems.get(), 1, &lengthPm, &status);

    if (U_FAILURE(status)) {
        return NULL;
    }

    jobjectArray amPmMarkers = env->NewObjectArray(2, JniConstants::stringClass, NULL);
    ScopedLocalRef<jstring> amU(env, env->NewString(am, lengthAm));
    env->SetObjectArrayElement(amPmMarkers, 0, amU.get());
    ScopedLocalRef<jstring> pmU(env, env->NewString(pm, lengthPm));
    env->SetObjectArrayElement(amPmMarkers, 1, pmU.get());

    return amPmMarkers;
}

static jobjectArray getEras(JNIEnv* env, UResourceBundle* gregorian) {
    UErrorCode status = U_ZERO_ERROR;
    ScopedResourceBundle gregorianElems(ures_getByKey(gregorian, "eras", NULL, &status));
    if (U_FAILURE(status)) {
        return NULL;
    }

    ScopedResourceBundle eraElems(ures_getByKey(gregorianElems.get(), "abbreviated", NULL, &status));
    if (U_FAILURE(status)) {
        return NULL;
    }

    int eraCount = ures_getSize(eraElems.get());
    jobjectArray eras = env->NewObjectArray(eraCount, JniConstants::stringClass, NULL);
    for (int i = 0; i < eraCount; ++i) {
        int eraLength;
        const jchar* era = ures_getStringByIndex(eraElems.get(), i, &eraLength, &status);
        if (U_FAILURE(status)) {
            return NULL;
        }
        ScopedLocalRef<jstring> eraU(env, env->NewString(era, eraLength));
        env->SetObjectArrayElement(eras, i, eraU.get());
    }
    return eras;
}

enum NameType { REGULAR, STAND_ALONE };
enum NameWidth { LONG, SHORT };
static jobjectArray getNames(JNIEnv* env, UResourceBundle* namesBundle, bool months, NameType type, NameWidth width) {
    const char* typeKey = (type == REGULAR) ? "format" : "stand-alone";
    const char* widthKey = (width == LONG) ? "wide" : "abbreviated";
    UErrorCode status = U_ZERO_ERROR;
    ScopedResourceBundle formatBundle(ures_getByKey(namesBundle, typeKey, NULL, &status));
    ScopedResourceBundle valuesBundle(ures_getByKey(formatBundle.get(), widthKey, NULL, &status));
    if (U_FAILURE(status)) {
        return NULL;
    }

    // The months array has a trailing empty string. The days array has a leading empty string.
    int count = ures_getSize(valuesBundle.get());
    jobjectArray result = env->NewObjectArray(count + 1, JniConstants::stringClass, NULL);
    env->SetObjectArrayElement(result, months ? count : 0, env->NewStringUTF(""));
    int arrayOffset = months ? 0 : 1;
    for (int i = 0; i < count; ++i) {
        int nameLength;
        const jchar* name = ures_getStringByIndex(valuesBundle.get(), i, &nameLength, &status);
        if (U_FAILURE(status)) {
            return NULL;
        }
        ScopedLocalRef<jstring> nameString(env, env->NewString(name, nameLength));
        env->SetObjectArrayElement(result, arrayOffset++, nameString.get());
    }
    return result;
}

static jstring getIntCurrencyCode(JNIEnv* env, jstring locale) {
    ScopedUtfChars localeChars(env, locale);

    // Extract the 2-character country name.
    if (strlen(localeChars.c_str()) < 5) {
        return NULL;
    }
    if (localeChars[3] < 'A' || localeChars[3] > 'Z' || localeChars[4] < 'A' || localeChars[4] > 'Z') {
        return NULL;
    }

    char country[3] = { localeChars[3], localeChars[4], 0 };
    return ICU_getCurrencyCodeNative(env, NULL, env->NewStringUTF(country));
}

static void setIntegerField(JNIEnv* env, jobject obj, const char* fieldName, int value) {
    ScopedLocalRef<jobject> integerValue(env, integerValueOf(env, value));
    jfieldID fid = env->GetFieldID(JniConstants::localeDataClass, fieldName, "Ljava/lang/Integer;");
    env->SetObjectField(obj, fid, integerValue.get());
}

static void setStringField(JNIEnv* env, jobject obj, const char* fieldName, jstring value) {
    jfieldID fid = env->GetFieldID(JniConstants::localeDataClass, fieldName, "Ljava/lang/String;");
    env->SetObjectField(obj, fid, value);
}

static void setStringArrayField(JNIEnv* env, jobject obj, const char* fieldName, jobjectArray value) {
    jfieldID fid = env->GetFieldID(JniConstants::localeDataClass, fieldName, "[Ljava/lang/String;");
    env->SetObjectField(obj, fid, value);
}

static void setStringField(JNIEnv* env, jobject obj, const char* fieldName, UResourceBundle* bundle, int index) {
    UErrorCode status = U_ZERO_ERROR;
    int charCount;
    const UChar* chars = ures_getStringByIndex(bundle, index, &charCount, &status);
    if (U_SUCCESS(status)) {
        setStringField(env, obj, fieldName, env->NewString(chars, charCount));
    } else {
        LOGE("Error setting String field %s from ICU resource: %s", fieldName, u_errorName(status));
    }
}

static void setCharField(JNIEnv* env, jobject obj, const char* fieldName, UResourceBundle* bundle, int index) {
    UErrorCode status = U_ZERO_ERROR;
    int charCount;
    const UChar* chars = ures_getStringByIndex(bundle, index, &charCount, &status);
    if (U_SUCCESS(status)) {
        jfieldID fid = env->GetFieldID(JniConstants::localeDataClass, fieldName, "C");
        env->SetCharField(obj, fid, chars[0]);
    } else {
        LOGE("Error setting char field %s from ICU resource: %s", fieldName, u_errorName(status));
    }
}

static jboolean ICU_initLocaleDataImpl(JNIEnv* env, jclass, jstring locale, jobject localeData) {
    ScopedUtfChars localeName(env, locale);
    UErrorCode status = U_ZERO_ERROR;
    ScopedResourceBundle root(ures_open(NULL, localeName.c_str(), &status));
    if (U_FAILURE(status)) {
        LOGE("Error getting ICU resource bundle: %s", u_errorName(status));
        status = U_ZERO_ERROR;
        return JNI_FALSE;
    }

    ScopedResourceBundle calendar(ures_getByKey(root.get(), "calendar", NULL, &status));
    if (U_FAILURE(status)) {
        LOGE("Error getting ICU calendar resource bundle: %s", u_errorName(status));
        return JNI_FALSE;
    }

    ScopedResourceBundle gregorian(ures_getByKey(calendar.get(), "gregorian", NULL, &status));
    if (U_FAILURE(status)) {
        LOGE("Error getting ICU gregorian resource bundle: %s", u_errorName(status));
        return JNI_FALSE;
    }

    int firstDayVals[] = { 0, 0 };
    if (getDayIntVector(env, gregorian.get(), firstDayVals)) {
        setIntegerField(env, localeData, "firstDayOfWeek", firstDayVals[0]);
        setIntegerField(env, localeData, "minimalDaysInFirstWeek", firstDayVals[1]);
    }

    setStringArrayField(env, localeData, "amPm", getAmPmMarkers(env, gregorian.get()));
    setStringArrayField(env, localeData, "eras", getEras(env, gregorian.get()));

    ScopedResourceBundle dayNames(ures_getByKey(gregorian.get(), "dayNames", NULL, &status));
    ScopedResourceBundle monthNames(ures_getByKey(gregorian.get(), "monthNames", NULL, &status));

    // Get the regular month and weekday names.
    jobjectArray longMonthNames = getNames(env, monthNames.get(), true, REGULAR, LONG);
    jobjectArray shortMonthNames = getNames(env, monthNames.get(), true, REGULAR, SHORT);
    jobjectArray longWeekdayNames = getNames(env, dayNames.get(), false, REGULAR, LONG);
    jobjectArray shortWeekdayNames = getNames(env, dayNames.get(), false, REGULAR, SHORT);
    setStringArrayField(env, localeData, "longMonthNames", longMonthNames);
    setStringArrayField(env, localeData, "shortMonthNames", shortMonthNames);
    setStringArrayField(env, localeData, "longWeekdayNames", longWeekdayNames);
    setStringArrayField(env, localeData, "shortWeekdayNames", shortWeekdayNames);

    // Get the stand-alone month and weekday names. If they're not available (as they aren't for
    // English), we reuse the regular names. If we returned null to Java, the usual fallback
    // mechanisms would come into play and we'd end up with the bogus stand-alone names from the
    // root locale ("1" for January, and so on).
    jobjectArray longStandAloneMonthNames = getNames(env, monthNames.get(), true, STAND_ALONE, LONG);
    if (longStandAloneMonthNames == NULL) {
        longStandAloneMonthNames = longMonthNames;
    }
    jobjectArray shortStandAloneMonthNames = getNames(env, monthNames.get(), true, STAND_ALONE, SHORT);
    if (shortStandAloneMonthNames == NULL) {
        shortStandAloneMonthNames = shortMonthNames;
    }
    jobjectArray longStandAloneWeekdayNames = getNames(env, dayNames.get(), false, STAND_ALONE, LONG);
    if (longStandAloneWeekdayNames == NULL) {
        longStandAloneWeekdayNames = longWeekdayNames;
    }
    jobjectArray shortStandAloneWeekdayNames = getNames(env, dayNames.get(), false, STAND_ALONE, SHORT);
    if (shortStandAloneWeekdayNames == NULL) {
        shortStandAloneWeekdayNames = shortWeekdayNames;
    }
    setStringArrayField(env, localeData, "longStandAloneMonthNames", longStandAloneMonthNames);
    setStringArrayField(env, localeData, "shortStandAloneMonthNames", shortStandAloneMonthNames);
    setStringArrayField(env, localeData, "longStandAloneWeekdayNames", longStandAloneWeekdayNames);
    setStringArrayField(env, localeData, "shortStandAloneWeekdayNames", shortStandAloneWeekdayNames);

    ScopedResourceBundle dateTimePatterns(ures_getByKey(gregorian.get(), "DateTimePatterns", NULL, &status));
    if (U_SUCCESS(status)) {
        setStringField(env, localeData, "fullTimeFormat", dateTimePatterns.get(), 0);
        setStringField(env, localeData, "longTimeFormat", dateTimePatterns.get(), 1);
        setStringField(env, localeData, "mediumTimeFormat", dateTimePatterns.get(), 2);
        setStringField(env, localeData, "shortTimeFormat", dateTimePatterns.get(), 3);
        setStringField(env, localeData, "fullDateFormat", dateTimePatterns.get(), 4);
        setStringField(env, localeData, "longDateFormat", dateTimePatterns.get(), 5);
        setStringField(env, localeData, "mediumDateFormat", dateTimePatterns.get(), 6);
        setStringField(env, localeData, "shortDateFormat", dateTimePatterns.get(), 7);
    }
    status = U_ZERO_ERROR;

    ScopedResourceBundle numberElements(ures_getByKey(root.get(), "NumberElements", NULL, &status));
    if (U_SUCCESS(status) && ures_getSize(numberElements.get()) >= 11) {
        setCharField(env, localeData, "zeroDigit", numberElements.get(), 4);
        setCharField(env, localeData, "digit", numberElements.get(), 5);
        setCharField(env, localeData, "decimalSeparator", numberElements.get(), 0);
        setCharField(env, localeData, "groupingSeparator", numberElements.get(), 1);
        setCharField(env, localeData, "patternSeparator", numberElements.get(), 2);
        setCharField(env, localeData, "percent", numberElements.get(), 3);
        setCharField(env, localeData, "perMill", numberElements.get(), 8);
        setCharField(env, localeData, "monetarySeparator", numberElements.get(), 0);
        setCharField(env, localeData, "minusSign", numberElements.get(), 6);
        setStringField(env, localeData, "exponentSeparator", numberElements.get(), 7);
        setStringField(env, localeData, "infinity", numberElements.get(), 9);
        setStringField(env, localeData, "NaN", numberElements.get(), 10);
    }
    status = U_ZERO_ERROR;

    jstring internationalCurrencySymbol = getIntCurrencyCode(env, locale);
    jstring currencySymbol = NULL;
    if (internationalCurrencySymbol != NULL) {
        currencySymbol = ICU_getCurrencySymbolNative(env, NULL, locale, internationalCurrencySymbol);
    } else {
        internationalCurrencySymbol = env->NewStringUTF("XXX");
    }
    if (currencySymbol == NULL) {
        // This is the UTF-8 encoding of U+00A4 (CURRENCY SIGN).
        currencySymbol = env->NewStringUTF("\xc2\xa4");
    }
    setStringField(env, localeData, "currencySymbol", currencySymbol);
    setStringField(env, localeData, "internationalCurrencySymbol", internationalCurrencySymbol);

    ScopedResourceBundle numberPatterns(ures_getByKey(root.get(), "NumberPatterns", NULL, &status));
    if (U_SUCCESS(status) && ures_getSize(numberPatterns.get()) >= 3) {
        setStringField(env, localeData, "numberPattern", numberPatterns.get(), 0);
        setStringField(env, localeData, "currencyPattern", numberPatterns.get(), 1);
        setStringField(env, localeData, "percentPattern", numberPatterns.get(), 2);
    }

    return JNI_TRUE;
}

static jstring ICU_toLowerCase(JNIEnv* env, jclass, jstring javaString, jstring localeName) {
    ScopedJavaUnicodeString scopedString(env, javaString);
    UnicodeString& s(scopedString.unicodeString());
    UnicodeString original(s);
    s.toLower(Locale::createFromName(ScopedUtfChars(env, localeName).c_str()));
    return s == original ? javaString : env->NewString(s.getBuffer(), s.length());
}

static jstring ICU_toUpperCase(JNIEnv* env, jclass, jstring javaString, jstring localeName) {
    ScopedJavaUnicodeString scopedString(env, javaString);
    UnicodeString& s(scopedString.unicodeString());
    UnicodeString original(s);
    s.toUpper(Locale::createFromName(ScopedUtfChars(env, localeName).c_str()));
    return s == original ? javaString : env->NewString(s.getBuffer(), s.length());
}

static JNINativeMethod gMethods[] = {
    NATIVE_METHOD(ICU, getAvailableBreakIteratorLocalesNative, "()[Ljava/lang/String;"),
    NATIVE_METHOD(ICU, getAvailableCalendarLocalesNative, "()[Ljava/lang/String;"),
    NATIVE_METHOD(ICU, getAvailableCollatorLocalesNative, "()[Ljava/lang/String;"),
    NATIVE_METHOD(ICU, getAvailableDateFormatLocalesNative, "()[Ljava/lang/String;"),
    NATIVE_METHOD(ICU, getAvailableLocalesNative, "()[Ljava/lang/String;"),
    NATIVE_METHOD(ICU, getAvailableNumberFormatLocalesNative, "()[Ljava/lang/String;"),
    NATIVE_METHOD(ICU, getCurrencyCodeNative, "(Ljava/lang/String;)Ljava/lang/String;"),
    NATIVE_METHOD(ICU, getCurrencyFractionDigitsNative, "(Ljava/lang/String;)I"),
    NATIVE_METHOD(ICU, getCurrencySymbolNative, "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;"),
    NATIVE_METHOD(ICU, getDisplayCountryNative, "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;"),
    NATIVE_METHOD(ICU, getDisplayLanguageNative, "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;"),
    NATIVE_METHOD(ICU, getDisplayVariantNative, "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;"),
    NATIVE_METHOD(ICU, getISO3CountryNative, "(Ljava/lang/String;)Ljava/lang/String;"),
    NATIVE_METHOD(ICU, getISO3LanguageNative, "(Ljava/lang/String;)Ljava/lang/String;"),
    NATIVE_METHOD(ICU, getISOCountriesNative, "()[Ljava/lang/String;"),
    NATIVE_METHOD(ICU, getISOLanguagesNative, "()[Ljava/lang/String;"),
    NATIVE_METHOD(ICU, initLocaleDataImpl, "(Ljava/lang/String;Lcom/ibm/icu4jni/util/LocaleData;)Z"),
    NATIVE_METHOD(ICU, toLowerCase, "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;"),
    NATIVE_METHOD(ICU, toUpperCase, "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;"),
};
int register_com_ibm_icu4jni_util_ICU(JNIEnv* env) {
    return jniRegisterNativeMethods(env, "com/ibm/icu4jni/util/ICU", gMethods, NELEM(gMethods));
}
