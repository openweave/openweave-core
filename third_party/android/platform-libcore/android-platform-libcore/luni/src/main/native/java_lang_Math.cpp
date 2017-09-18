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

#define LOG_TAG "Math"

#include "jni.h"
#include "JNIHelp.h"
#include "JniConstants.h"

#include <stdlib.h>
#include <math.h>

static jdouble Math_sin(JNIEnv*, jclass, jdouble a) {
    return sin(a);
}

static jdouble Math_cos(JNIEnv*, jclass, jdouble a) {
    return cos(a);
}

static jdouble Math_tan(JNIEnv*, jclass, jdouble a) {
    return tan(a);
}

static jdouble Math_asin(JNIEnv*, jclass, jdouble a) {
    return asin(a);
}

static jdouble Math_acos(JNIEnv*, jclass, jdouble a) {
    return acos(a);
}

static jdouble Math_atan(JNIEnv*, jclass, jdouble a) {
    return atan(a);
}

static jdouble Math_exp(JNIEnv*, jclass, jdouble a) {
    return exp(a);
}

static jdouble Math_log(JNIEnv*, jclass, jdouble a) {
    return log(a);
}

static jdouble Math_sqrt(JNIEnv*, jclass, jdouble a) {
    return sqrt(a);
}

static jdouble Math_IEEEremainder(JNIEnv*, jclass, jdouble a, jdouble b) {
    return remainder(a, b);
}

static jdouble Math_floor(JNIEnv*, jclass, jdouble a) {
    return floor(a);
}

static jdouble Math_ceil(JNIEnv*, jclass, jdouble a) {
    return ceil(a);
}

static jdouble Math_rint(JNIEnv*, jclass, jdouble a) {
    return rint(a);
}

static jdouble Math_atan2(JNIEnv*, jclass, jdouble a, jdouble b) {
    return atan2(a, b);
}

static jdouble Math_pow(JNIEnv*, jclass, jdouble a, jdouble b) {
    return pow(a, b);
}

static jdouble Math_sinh(JNIEnv*, jclass, jdouble a) {
    return sinh(a);
}

static jdouble Math_tanh(JNIEnv*, jclass, jdouble a) {
    return tanh(a);
}

static jdouble Math_cosh(JNIEnv*, jclass, jdouble a) {
    return cosh(a);
}

static jdouble Math_log10(JNIEnv*, jclass, jdouble a) {
    return log10(a);
}

static jdouble Math_cbrt(JNIEnv*, jclass, jdouble a) {
    return cbrt(a);
}

static jdouble Math_expm1(JNIEnv*, jclass, jdouble a) {
    return expm1(a);
}

static jdouble Math_hypot(JNIEnv*, jclass, jdouble a, jdouble b) {
    return hypot(a, b);
}

static jdouble Math_log1p(JNIEnv*, jclass, jdouble a) {
    return log1p(a);
}

static jdouble Math_nextafter(JNIEnv*, jclass, jdouble a, jdouble b) {
    return nextafter(a, b);
}

static jfloat Math_nextafterf(JNIEnv*, jclass, jfloat a, jfloat b) {
    return nextafterf(a, b);
}

static JNINativeMethod gMethods[] = {
    NATIVE_METHOD(Math, IEEEremainder, "(DD)D"),
    NATIVE_METHOD(Math, acos, "(D)D"),
    NATIVE_METHOD(Math, asin, "(D)D"),
    NATIVE_METHOD(Math, atan, "(D)D"),
    NATIVE_METHOD(Math, atan2, "(DD)D"),
    NATIVE_METHOD(Math, cbrt, "(D)D"),
    NATIVE_METHOD(Math, ceil, "(D)D"),
    NATIVE_METHOD(Math, cos, "(D)D"),
    NATIVE_METHOD(Math, cosh, "(D)D"),
    NATIVE_METHOD(Math, exp, "(D)D"),
    NATIVE_METHOD(Math, expm1, "(D)D"),
    NATIVE_METHOD(Math, floor, "(D)D"),
    NATIVE_METHOD(Math, hypot, "(DD)D"),
    NATIVE_METHOD(Math, log, "(D)D"),
    NATIVE_METHOD(Math, log10, "(D)D"),
    NATIVE_METHOD(Math, log1p, "(D)D"),
    NATIVE_METHOD(Math, nextafter, "(DD)D"),
    NATIVE_METHOD(Math, nextafterf, "(FF)F"),
    NATIVE_METHOD(Math, pow, "(DD)D"),
    NATIVE_METHOD(Math, rint, "(D)D"),
    NATIVE_METHOD(Math, sin, "(D)D"),
    NATIVE_METHOD(Math, sinh, "(D)D"),
    NATIVE_METHOD(Math, sqrt, "(D)D"),
    NATIVE_METHOD(Math, tan, "(D)D"),
    NATIVE_METHOD(Math, tanh, "(D)D"),
};

int register_java_lang_Math(JNIEnv* env) {
    return jniRegisterNativeMethods(env, "java/lang/Math", gMethods, NELEM(gMethods));
}
