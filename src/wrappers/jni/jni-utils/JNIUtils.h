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
 *      Utility functions for building Java native libraries.
 *
 */

#ifndef JNIUTILS_H
#define JNIUTILS_H

#include <jni.h>

#include <Weave/Core/WeaveCore.h>

namespace nl {
namespace Weave {

#ifndef WEAVE_JNI_ERROR_MIN
#define WEAVE_JNI_ERROR_MIN     10000
#endif // WEAVE_JNI_ERROR_MIN

#ifndef WEAVE_JNI_ERROR_MAX
#define WEAVE_JNI_ERROR_MAX     10999
#endif // WEAVE_JNI_ERROR_MAX

#ifndef _WEAVE_JNI_ERROR
#define _WEAVE_JNI_ERROR(e) (WEAVE_JNI_ERROR_MIN + (e))
#endif // _WEAVE_JNI_ERROR

#define WEAVE_JNI_ERROR_EXCEPTION_THROWN          _WEAVE_JNI_ERROR(0)
#define WEAVE_JNI_ERROR_TYPE_NOT_FOUND            _WEAVE_JNI_ERROR(1)
#define WEAVE_JNI_ERROR_METHOD_NOT_FOUND          _WEAVE_JNI_ERROR(2)
#define WEAVE_JNI_ERROR_FIELD_NOT_FOUND           _WEAVE_JNI_ERROR(3)

typedef struct
{
    const char *ClassName;
    const char *MethodName;
    const char *MethodSignature;
    void *MethodFunction;
} JNILibraryMethod;

class JNIUtils
{
public:
    static JavaVM *sJVM;
    static jclass sJavaObjectClass;

    static WEAVE_ERROR Init(JavaVM *jvm, JNIEnv *env, const char *weaveErrorClassName);
    static void Shutdown(JNIEnv *env);

    static WEAVE_ERROR RegisterLibraryMethods(JNIEnv *env, const char *basePackageName, const JNILibraryMethod *libMethods, size_t numLibMethods);

    static WEAVE_ERROR GetGlobalClassRef(JNIEnv *env, const char *clsType, jclass& outCls);

    static void ThrowError(JNIEnv *env, WEAVE_ERROR errToThrow);

    static WEAVE_ERROR J2N_ByteArray(JNIEnv *env, jbyteArray inArray, uint8_t *& outArray, uint32_t& outArrayLen);
    static WEAVE_ERROR J2N_ByteArrayInPlace(JNIEnv *env, jbyteArray inArray, uint8_t * outArray, uint32_t maxArrayLen);
    static WEAVE_ERROR N2J_ByteArray(JNIEnv *env, const uint8_t *inArray, uint32_t inArrayLen, jbyteArray& outArray);
    static WEAVE_ERROR J2N_EnumVal(JNIEnv *env, jobject enumObj, int& outVal);
    static WEAVE_ERROR J2N_EnumFieldVal(JNIEnv *env, jobject obj, const char *fieldName, const char *fieldType, int& outVal);
    static WEAVE_ERROR J2N_ShortFieldVal(JNIEnv *env, jobject obj, const char *fieldName, jshort& outVal);
    static WEAVE_ERROR J2N_IntFieldVal(JNIEnv *env, jobject obj, const char *fieldName, jint& outVal);
    static WEAVE_ERROR J2N_LongFieldVal(JNIEnv *env, jobject obj, const char *fieldName, jlong& outVal);
    static WEAVE_ERROR J2N_StringFieldVal(JNIEnv *env, jobject obj, const char *fieldName, char *& outVal);
    static WEAVE_ERROR J2N_ByteArrayFieldVal(JNIEnv *env, jobject obj, const char *fieldName, uint8_t *& outArray, uint32_t& outArrayLen);
    static WEAVE_ERROR N2J_Error(JNIEnv *env, WEAVE_ERROR inErr, jthrowable& outEx);

private:
    static jclass sWeaveErrorClass;
};

} // namespace Weave
} // namespave nl

#endif // JNIUTILS_H
