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

#define LOG_TAG "TimeZones"

#include "ErrorCode.h"
#include "JNIHelp.h"
#include "JniConstants.h"
#include "ScopedJavaUnicodeString.h"
#include "ScopedLocalRef.h"
#include "ScopedUtfChars.h"
#include "UniquePtr.h"
#include "unicode/smpdtfmt.h"
#include "unicode/timezone.h"

extern Locale getLocale(JNIEnv* env, jstring localeName);

static jstring formatDate(JNIEnv* env, const SimpleDateFormat& fmt, const UDate& when) {
    UnicodeString str;
    fmt.format(when, str);
    return env->NewString(str.getBuffer(), str.length());
}

static TimeZone* timeZoneFromId(JNIEnv* env, jstring javaZoneId) {
    ScopedJavaUnicodeString zoneID(env, javaZoneId);
    return TimeZone::createTimeZone(zoneID.unicodeString());
}

static jobjectArray TimeZones_forCountryCode(JNIEnv* env, jclass, jstring countryCode) {
    ScopedUtfChars countryChars(env, countryCode);
    if (countryChars.c_str() == NULL) {
        return NULL;
    }

    UniquePtr<StringEnumeration> ids(TimeZone::createEnumeration(countryChars.c_str()));
    if (ids.get() == NULL) {
        return NULL;
    }
    UErrorCode status = U_ZERO_ERROR;
    int32_t idCount = ids->count(status);
    if (U_FAILURE(status)) {
        icu4jni_error(env, status);
        return NULL;
    }

    jobjectArray result = env->NewObjectArray(idCount, JniConstants::stringClass, NULL);
    for (int32_t i = 0; i < idCount; ++i) {
        const UnicodeString* id = ids->snext(status);
        if (U_FAILURE(status)) {
            icu4jni_error(env, status);
            return NULL;
        }
        ScopedLocalRef<jstring> idString(env, env->NewString(id->getBuffer(), id->length()));
        env->SetObjectArrayElement(result, i, idString.get());
    }
    return result;
}

static jstring TimeZones_getDisplayNameImpl(JNIEnv* env, jclass, jstring zoneId, jboolean isDST, jint style, jstring localeId) {
    UniquePtr<TimeZone> zone(timeZoneFromId(env, zoneId));
    Locale locale = getLocale(env, localeId);
    // Try to get the display name of the TimeZone according to the Locale
    UnicodeString displayName;
    zone->getDisplayName((UBool)isDST, (style == 0 ? TimeZone::SHORT : TimeZone::LONG), locale, displayName);
    return env->NewString(displayName.getBuffer(), displayName.length());
}

static void TimeZones_getZoneStringsImpl(JNIEnv* env, jclass, jobjectArray outerArray, jstring localeName) {
    Locale locale = getLocale(env, localeName);

    // We could use TimeZone::getDisplayName, but that's way too slow.
    // The cost of this method goes from 0.5s to 4.5s on a Nexus One.
    // Much of the saving comes from caching SimpleDateFormat instances.
    UErrorCode status = U_ZERO_ERROR;
    UnicodeString longPattern("zzzz", "");
    SimpleDateFormat longFormat(longPattern, locale, status);
    UnicodeString shortPattern("z", "");
    SimpleDateFormat shortFormat(shortPattern, locale, status);

    jobjectArray longStdArray = (jobjectArray) env->GetObjectArrayElement(outerArray, 1);
    jobjectArray shortStdArray = (jobjectArray) env->GetObjectArrayElement(outerArray, 2);
    jobjectArray longDstArray = (jobjectArray) env->GetObjectArrayElement(outerArray, 3);
    jobjectArray shortDstArray = (jobjectArray) env->GetObjectArrayElement(outerArray, 4);

    // 15th January 2008
    UDate date1 = 1203105600000.0;
    // 15th July 2008
    UDate date2 = 1218826800000.0;

    jobjectArray zoneIds = (jobjectArray) env->GetObjectArrayElement(outerArray, 0);
    int zoneIdCount = env->GetArrayLength(zoneIds);
    for (int i = 0; i < zoneIdCount; ++i) {
        ScopedLocalRef<jstring> id(env, reinterpret_cast<jstring>(env->GetObjectArrayElement(zoneIds, i)));
        UniquePtr<TimeZone> tz(timeZoneFromId(env, id.get()));

        longFormat.setTimeZone(*tz);
        shortFormat.setTimeZone(*tz);

        int32_t daylightOffset;
        int32_t rawOffset;
        tz->getOffset(date1, false, rawOffset, daylightOffset, status);
        UDate standardDate;
        UDate daylightSavingDate;
        if (daylightOffset != 0) {
            // The Timezone is reporting that we are in daylight time
            // for the winter date.  The dates are for the wrong hemisphere,
            // swap them.
            standardDate = date2;
            daylightSavingDate = date1;
        } else {
            standardDate = date1;
            daylightSavingDate = date2;
        }

        ScopedLocalRef<jstring> shortStd(env, formatDate(env, shortFormat, standardDate));
        env->SetObjectArrayElement(shortStdArray, i, shortStd.get());

        ScopedLocalRef<jstring> longStd(env, formatDate(env, longFormat, standardDate));
        env->SetObjectArrayElement(longStdArray, i, longStd.get());

        if (tz->useDaylightTime()) {
            ScopedLocalRef<jstring> shortDst(env, formatDate(env, shortFormat, daylightSavingDate));
            env->SetObjectArrayElement(shortDstArray, i, shortDst.get());

            ScopedLocalRef<jstring> longDst(env, formatDate(env, longFormat, daylightSavingDate));
            env->SetObjectArrayElement(longDstArray, i, longDst.get());
        } else {
            env->SetObjectArrayElement(shortDstArray, i, shortStd.get());
            env->SetObjectArrayElement(longDstArray, i, longStd.get());
        }
    }
}

static JNINativeMethod gMethods[] = {
    NATIVE_METHOD(TimeZones, getDisplayNameImpl, "(Ljava/lang/String;ZILjava/lang/String;)Ljava/lang/String;"),
    NATIVE_METHOD(TimeZones, forCountryCode, "(Ljava/lang/String;)[Ljava/lang/String;"),
    NATIVE_METHOD(TimeZones, getZoneStringsImpl, "([[Ljava/lang/String;Ljava/lang/String;)V"),
};
int register_libcore_icu_TimeZones(JNIEnv* env) {
    return jniRegisterNativeMethods(env, "libcore/icu/TimeZones", gMethods, NELEM(gMethods));
}
