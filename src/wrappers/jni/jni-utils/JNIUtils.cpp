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

#include <jni.h>

#include <Weave/Core/WeaveCore.h>
#include <Weave/Support/CodeUtils.h>

#include "JNIUtils.h"


namespace nl {
namespace Weave {

JavaVM *JNIUtils::sJVM;
jclass JNIUtils::sJavaObjectClass;
jclass JNIUtils::sWeaveErrorClass;

WEAVE_ERROR JNIUtils::Init(JavaVM *jvm, JNIEnv *env, const char *weaveErrorClassName)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (sJavaObjectClass == NULL)
    {
        err = GetGlobalClassRef(env, "java/lang/Object", sJavaObjectClass);
        SuccessOrExit(err);
    }

    if (sWeaveErrorClass == NULL)
    {
        err = GetGlobalClassRef(env, weaveErrorClassName, sWeaveErrorClass);
        SuccessOrExit(err);
    }

    sJVM = jvm;

exit:
    return err;
}

void JNIUtils::Shutdown(JNIEnv *env)
{
    if (sJavaObjectClass != NULL)
    {
        env->DeleteGlobalRef(sJavaObjectClass);
        sJavaObjectClass = NULL;
    }
    if (sWeaveErrorClass != NULL)
    {
        env->DeleteGlobalRef(sWeaveErrorClass);
        sWeaveErrorClass = NULL;
    }
}

static WEAVE_ERROR MakeClassName(const char *basePackageName, const char *relativeClassName, char *& classNameBuf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    size_t totalLen = strlen(basePackageName) + strlen(relativeClassName) + 2;

    classNameBuf = (char *)realloc(classNameBuf, totalLen);
    VerifyOrExit(classNameBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    snprintf(classNameBuf, totalLen, "%s/%s", basePackageName, relativeClassName);

exit:
    return err;
}

WEAVE_ERROR JNIUtils::RegisterLibraryMethods(JNIEnv *env, const char *basePackageName, const JNILibraryMethod *libMethods, size_t numLibMethods)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    jint regRes;
    char *className = NULL;

    for (size_t i = 0; i < numLibMethods; i++)
    {
        const JNILibraryMethod& libMethod = libMethods[i];

        err = MakeClassName(basePackageName, libMethod.ClassName, className);
        SuccessOrExit(err);

        jclass cls = env->FindClass(className);
        VerifyOrExit(cls != NULL, err = WEAVE_JNI_ERROR_TYPE_NOT_FOUND);

        JNINativeMethod nativeMethod;
        nativeMethod.name = (char *)libMethod.MethodName;
        nativeMethod.signature = (char *)libMethod.MethodSignature;
        nativeMethod.fnPtr = libMethod.MethodFunction;

        regRes = env->RegisterNatives(cls, &nativeMethod, 1);
        VerifyOrExit(regRes == 0, err = WEAVE_JNI_ERROR_EXCEPTION_THROWN);
    }

exit:
    if (className != NULL)
    {
        free(className);
    }
    return err;
}

void JNIUtils::ThrowError(JNIEnv *env, WEAVE_ERROR errToThrow)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    jthrowable ex;

    // Do nothing for WEAVE_NO_ERROR, or if the error indicates that a Java exception has already
    // been thrown.
    VerifyOrExit(errToThrow != WEAVE_NO_ERROR && errToThrow != WEAVE_JNI_ERROR_EXCEPTION_THROWN, );

    // Convert the error to a throwable.
    err = N2J_Error(env, errToThrow, ex);
    SuccessOrExit(err);

    // Throw it.
    env->Throw(ex);

exit:
    return;
}

WEAVE_ERROR J2N_ByteArray(JNIEnv *env, jbyteArray inArray, uint8_t *& outArray, uint32_t& outArrayLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    outArrayLen = env->GetArrayLength(inArray);

    outArray = (uint8_t *)malloc(outArrayLen);
    VerifyOrExit(outArray != NULL, err = WEAVE_ERROR_NO_MEMORY);

    if (outArrayLen != 0)
    {
        env->ExceptionClear();
        env->GetByteArrayRegion(inArray, 0, outArrayLen, (jbyte *)outArray);
        VerifyOrExit(!env->ExceptionCheck(), err = WEAVE_JNI_ERROR_EXCEPTION_THROWN);
    }

exit:
    if (err != WEAVE_NO_ERROR)
    {
        if (outArray != NULL)
        {
            free(outArray);
            outArray = NULL;
        }
    }

    return err;
}

// A version of J2N_ByteArray that copies into an existing buffer, rather than allocating memory.
WEAVE_ERROR J2N_ByteArrayInPlace(JNIEnv *env, jbyteArray inArray, uint8_t * outArray, uint32_t maxArrayLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint32_t arrayLen;

    VerifyOrExit(outArray != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    arrayLen = env->GetArrayLength(inArray);
    VerifyOrExit(arrayLen <= maxArrayLen, err = WEAVE_ERROR_INVALID_ARGUMENT);

    if (arrayLen != 0)
    {
        env->ExceptionClear();
        env->GetByteArrayRegion(inArray, 0, arrayLen, (jbyte *)outArray);
        VerifyOrExit(!env->ExceptionCheck(), err = WEAVE_JNI_ERROR_EXCEPTION_THROWN);
    }

exit:
    return err;
}

WEAVE_ERROR JNIUtils::N2J_ByteArray(JNIEnv *env, const uint8_t *inArray, uint32_t inArrayLen, jbyteArray& outArray)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    outArray = env->NewByteArray((int)inArrayLen);
    VerifyOrExit(outArray != NULL, err = WEAVE_ERROR_NO_MEMORY);

    env->ExceptionClear();
    env->SetByteArrayRegion(outArray, 0, inArrayLen, (jbyte *)inArray);
    VerifyOrExit(!env->ExceptionCheck(), err = WEAVE_JNI_ERROR_EXCEPTION_THROWN);

exit:
    return err;
}

WEAVE_ERROR J2N_EnumVal(JNIEnv *env, jobject enumObj, int& outVal)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    jclass enumCls = NULL;
    jfieldID fieldId;

    enumCls = env->GetObjectClass(enumObj);
    VerifyOrExit(enumCls != NULL, err = WEAVE_JNI_ERROR_TYPE_NOT_FOUND);

    fieldId = env->GetFieldID(enumCls, "val", "I");
    VerifyOrExit(fieldId != NULL, err = WEAVE_JNI_ERROR_METHOD_NOT_FOUND);

    env->ExceptionClear();
    outVal = env->GetIntField(enumObj, fieldId);
    VerifyOrExit(!env->ExceptionCheck(), err = WEAVE_JNI_ERROR_EXCEPTION_THROWN);

exit:
    env->DeleteLocalRef(enumCls);
    return err;
}

WEAVE_ERROR J2N_EnumFieldVal(JNIEnv *env, jobject obj, const char *fieldName, const char *fieldType, int& outVal)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    jclass objCls = NULL, enumCls = NULL;
    jfieldID fieldId;
    jobject enumObj = NULL;

    objCls = env->GetObjectClass(obj);
    VerifyOrExit(objCls != NULL, err = WEAVE_JNI_ERROR_TYPE_NOT_FOUND);

    fieldId = env->GetFieldID(objCls, fieldName, fieldType);
    VerifyOrExit(fieldId != NULL, err = WEAVE_JNI_ERROR_METHOD_NOT_FOUND);

    env->ExceptionClear();
    enumObj = env->GetObjectField(obj, fieldId);
    VerifyOrExit(!env->ExceptionCheck(), err = WEAVE_JNI_ERROR_EXCEPTION_THROWN);

    if (enumObj != NULL)
    {
        enumCls = env->GetObjectClass(enumObj);
        VerifyOrExit(enumCls != NULL, err = WEAVE_JNI_ERROR_TYPE_NOT_FOUND);

        fieldId = env->GetFieldID(enumCls, "val", "I");
        VerifyOrExit(fieldId != NULL, err = WEAVE_JNI_ERROR_METHOD_NOT_FOUND);

        env->ExceptionClear();
        outVal = env->GetIntField(enumObj, fieldId);
        VerifyOrExit(!env->ExceptionCheck(), err = WEAVE_JNI_ERROR_EXCEPTION_THROWN);
    }
    else
        outVal = -1;

exit:
    env->DeleteLocalRef(enumCls);
    env->DeleteLocalRef(enumObj);
    env->DeleteLocalRef(objCls);
    return err;
}

WEAVE_ERROR J2N_ShortFieldVal(JNIEnv *env, jobject obj, const char *fieldName, jshort& outVal)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    jclass objCls = NULL;
    jfieldID fieldId;

    objCls = env->GetObjectClass(obj);
    VerifyOrExit(objCls != NULL, err = WEAVE_JNI_ERROR_TYPE_NOT_FOUND);

    fieldId = env->GetFieldID(objCls, fieldName, "S");
    VerifyOrExit(fieldId != NULL, err = WEAVE_JNI_ERROR_METHOD_NOT_FOUND);

    env->ExceptionClear();
    outVal = env->GetShortField(obj, fieldId);
    VerifyOrExit(!env->ExceptionCheck(), err = WEAVE_JNI_ERROR_EXCEPTION_THROWN);

exit:
    env->DeleteLocalRef(objCls);
    return err;
}

WEAVE_ERROR J2N_IntFieldVal(JNIEnv *env, jobject obj, const char *fieldName, jint& outVal)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    jclass objCls = NULL;
    jfieldID fieldId;

    objCls = env->GetObjectClass(obj);
    VerifyOrExit(objCls != NULL, err = WEAVE_JNI_ERROR_TYPE_NOT_FOUND);

    fieldId = env->GetFieldID(objCls, fieldName, "I");
    VerifyOrExit(fieldId != NULL, err = WEAVE_JNI_ERROR_METHOD_NOT_FOUND);

    env->ExceptionClear();
    outVal = env->GetIntField(obj, fieldId);
    VerifyOrExit(!env->ExceptionCheck(), err = WEAVE_JNI_ERROR_EXCEPTION_THROWN);

exit:
    env->DeleteLocalRef(objCls);
    return err;
}

WEAVE_ERROR J2N_LongFieldVal(JNIEnv *env, jobject obj, const char *fieldName, jlong& outVal)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    jclass objCls = NULL;
    jfieldID fieldId;

    objCls = env->GetObjectClass(obj);
    VerifyOrExit(objCls != NULL, err = WEAVE_JNI_ERROR_TYPE_NOT_FOUND);

    fieldId = env->GetFieldID(objCls, fieldName, "J");
    VerifyOrExit(fieldId != NULL, err = WEAVE_JNI_ERROR_METHOD_NOT_FOUND);

    env->ExceptionClear();
    outVal = env->GetLongField(obj, fieldId);
    VerifyOrExit(!env->ExceptionCheck(), err = WEAVE_JNI_ERROR_EXCEPTION_THROWN);

exit:
    env->DeleteLocalRef(objCls);
    return err;
}

WEAVE_ERROR J2N_StringFieldVal(JNIEnv *env, jobject obj, const char *fieldName, char *& outVal)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    jclass objCls = NULL;
    jfieldID fieldId;
    jstring strObj = NULL;
    const char *nativeStr;

    objCls = env->GetObjectClass(obj);
    VerifyOrExit(objCls != NULL, err = WEAVE_JNI_ERROR_TYPE_NOT_FOUND);

    fieldId = env->GetFieldID(objCls, fieldName, "Ljava/lang/String;");
    VerifyOrExit(fieldId != NULL, err = WEAVE_JNI_ERROR_METHOD_NOT_FOUND);

    env->ExceptionClear();
    strObj = (jstring)env->GetObjectField(obj, fieldId);
    VerifyOrExit(!env->ExceptionCheck(), err = WEAVE_JNI_ERROR_EXCEPTION_THROWN);

    if (strObj != NULL)
    {
        nativeStr = env->GetStringUTFChars(strObj, 0);
        outVal = strdup(nativeStr);
        env->ReleaseStringUTFChars(strObj, nativeStr);
    }
    else
        outVal = NULL;

exit:
    env->DeleteLocalRef(strObj);
    env->DeleteLocalRef(objCls);
    return err;
}

WEAVE_ERROR J2N_ByteArrayFieldVal(JNIEnv *env, jobject obj, const char *fieldName, uint8_t *& outArray, uint32_t& outArrayLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    jclass objCls = NULL;
    jfieldID fieldId;
    jbyteArray byteArrayObj = NULL;

    objCls = env->GetObjectClass(obj);
    VerifyOrExit(objCls != NULL, err = WEAVE_JNI_ERROR_TYPE_NOT_FOUND);

    fieldId = env->GetFieldID(objCls, fieldName, "[B");
    VerifyOrExit(fieldId != NULL, err = WEAVE_JNI_ERROR_METHOD_NOT_FOUND);

    env->ExceptionClear();
    byteArrayObj = (jbyteArray)env->GetObjectField(obj, fieldId);
    VerifyOrExit(!env->ExceptionCheck(), err = WEAVE_JNI_ERROR_EXCEPTION_THROWN);

    if (byteArrayObj != NULL)
    {
        err = J2N_ByteArray(env, byteArrayObj, outArray, outArrayLen);
        SuccessOrExit(err);
    }
    else
    {
        outArray = NULL;
        outArrayLen = 0;
    }

exit:
    env->DeleteLocalRef(byteArrayObj);
    env->DeleteLocalRef(objCls);
    return err;
}

WEAVE_ERROR JNIUtils::N2J_Error(JNIEnv *env, WEAVE_ERROR inErr, jthrowable& outEx)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    jmethodID constructor;
    const char *errStr = NULL;
    jstring errStrObj = NULL;
    jclass exCls = NULL;

    if (inErr == WEAVE_ERROR_INVALID_ARGUMENT)
    {
        exCls = env->FindClass("java/lang/IllegalArgumentException");
        VerifyOrExit(exCls != NULL, err = WEAVE_JNI_ERROR_TYPE_NOT_FOUND);

        constructor = env->GetMethodID(exCls, "<init>", "()V");
        VerifyOrExit(constructor != NULL, err = WEAVE_JNI_ERROR_EXCEPTION_THROWN);

        env->ExceptionClear();
        outEx = (jthrowable)env->NewObject(exCls, constructor);
        VerifyOrExit(!env->ExceptionCheck(), err = WEAVE_JNI_ERROR_EXCEPTION_THROWN);
    }

    else if (inErr == WEAVE_ERROR_INCORRECT_STATE)
    {
        exCls = env->FindClass("java/lang/IllegalStateException");
        VerifyOrExit(exCls != NULL, err = WEAVE_JNI_ERROR_TYPE_NOT_FOUND);

        constructor = env->GetMethodID(exCls, "<init>", "()V");
        VerifyOrExit(constructor != NULL, err = WEAVE_JNI_ERROR_EXCEPTION_THROWN);

        env->ExceptionClear();
        outEx = (jthrowable)env->NewObject(exCls, constructor);
        VerifyOrExit(!env->ExceptionCheck(), err = WEAVE_JNI_ERROR_EXCEPTION_THROWN);
    }

    else {
        constructor = env->GetMethodID(sWeaveErrorClass, "<init>", "(ILjava/lang/String;)V");
        VerifyOrExit(constructor != NULL, err = WEAVE_JNI_ERROR_EXCEPTION_THROWN);

        switch (inErr)
        {
        case WEAVE_JNI_ERROR_TYPE_NOT_FOUND          : errStr = "JNI type not found"; break;
        case WEAVE_JNI_ERROR_METHOD_NOT_FOUND        : errStr = "JNI method not found"; break;
        case WEAVE_JNI_ERROR_FIELD_NOT_FOUND         : errStr = "JNI field not found"; break;
        default:                                     ; errStr = nl::ErrorStr(inErr); break;
        }
        errStrObj = (errStr != NULL) ? env->NewStringUTF(errStr) : NULL;

        env->ExceptionClear();
        outEx = (jthrowable)env->NewObject(sWeaveErrorClass, constructor, (jint)inErr, errStrObj);
        VerifyOrExit(!env->ExceptionCheck(), err = WEAVE_JNI_ERROR_EXCEPTION_THROWN);
    }

exit:
    env->DeleteLocalRef(errStrObj);
    env->DeleteLocalRef(exCls);
    return err;
}

WEAVE_ERROR JNIUtils::GetGlobalClassRef(JNIEnv *env, const char *clsType, jclass& outCls)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    jclass cls = NULL;

    cls = env->FindClass(clsType);
    VerifyOrExit(cls != NULL, err = WEAVE_JNI_ERROR_TYPE_NOT_FOUND);

    outCls = (jclass)env->NewGlobalRef((jobject)cls);
    VerifyOrExit(outCls != NULL, err = WEAVE_JNI_ERROR_TYPE_NOT_FOUND);

exit:
    env->DeleteLocalRef(cls);
    return err;
}


} // namespace Weave
} // namespave nl
