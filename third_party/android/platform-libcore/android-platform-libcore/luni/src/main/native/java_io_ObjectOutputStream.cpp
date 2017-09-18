/*
 *  Licensed to the Apache Software Foundation (ASF) under one or more
 *  contributor license agreements.  See the NOTICE file distributed with
 *  this work for additional information regarding copyright ownership.
 *  The ASF licenses this file to You under the Apache License, Version 2.0
 *  (the "License"); you may not use this file except in compliance with
 *  the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#define LOG_TAG "ObjectOutputStream"

#include "JNIHelp.h"
#include "JniConstants.h"
#include "ScopedUtfChars.h"

#define GETTER(FUNCTION_NAME, JNI_C_TYPE, JNI_TYPE_STRING, JNI_GETTER_FUNCTION) \
    static JNI_C_TYPE FUNCTION_NAME(JNIEnv* env, jclass, jobject instance, jclass declaringClass, \
            jstring javaFieldName) { \
        if (instance == NULL) { \
            return JNI_C_TYPE(); \
        } \
        ScopedUtfChars fieldName(env, javaFieldName); \
        if (fieldName.c_str() == NULL) { \
            return JNI_C_TYPE(); \
        } \
        jfieldID fid = env->GetFieldID(declaringClass, fieldName.c_str(), JNI_TYPE_STRING); \
        if (fid == 0) { \
            return JNI_C_TYPE(); \
        } \
        return env->JNI_GETTER_FUNCTION(instance, fid); \
    }

GETTER(ObjectOutputStream_getFieldBool,   jboolean, "Z", GetBooleanField)
GETTER(ObjectOutputStream_getFieldByte,   jbyte,    "B", GetByteField)
GETTER(ObjectOutputStream_getFieldChar,   jchar,    "C", GetCharField)
GETTER(ObjectOutputStream_getFieldDouble, jdouble,  "D", GetDoubleField)
GETTER(ObjectOutputStream_getFieldFloat,  jfloat,   "F", GetFloatField)
GETTER(ObjectOutputStream_getFieldInt,    jint,     "I", GetIntField)
GETTER(ObjectOutputStream_getFieldLong,   jlong,    "J", GetLongField)
GETTER(ObjectOutputStream_getFieldShort,  jshort,   "S", GetShortField)

static jobject ObjectOutputStream_getFieldObj(JNIEnv* env, jclass, jobject instance,
        jclass declaringClass, jstring javaFieldName, jstring javaFieldTypeName) {
    ScopedUtfChars fieldName(env, javaFieldName);
    if (fieldName.c_str() == NULL) {
        return NULL;
    }
    ScopedUtfChars fieldTypeName(env, javaFieldTypeName);
    if (fieldTypeName.c_str() == NULL) {
        return NULL;
    }
    jfieldID fid = env->GetFieldID(declaringClass, fieldName.c_str(), fieldTypeName.c_str());
    if (fid == 0) {
        return NULL;
    }
    return env->GetObjectField(instance, fid);
}

static JNINativeMethod gMethods[] = {
    NATIVE_METHOD(ObjectOutputStream, getFieldBool, "(Ljava/lang/Object;Ljava/lang/Class;Ljava/lang/String;)Z"),
    NATIVE_METHOD(ObjectOutputStream, getFieldByte, "(Ljava/lang/Object;Ljava/lang/Class;Ljava/lang/String;)B"),
    NATIVE_METHOD(ObjectOutputStream, getFieldChar, "(Ljava/lang/Object;Ljava/lang/Class;Ljava/lang/String;)C"),
    NATIVE_METHOD(ObjectOutputStream, getFieldDouble, "(Ljava/lang/Object;Ljava/lang/Class;Ljava/lang/String;)D"),
    NATIVE_METHOD(ObjectOutputStream, getFieldFloat, "(Ljava/lang/Object;Ljava/lang/Class;Ljava/lang/String;)F"),
    NATIVE_METHOD(ObjectOutputStream, getFieldInt, "(Ljava/lang/Object;Ljava/lang/Class;Ljava/lang/String;)I"),
    NATIVE_METHOD(ObjectOutputStream, getFieldLong, "(Ljava/lang/Object;Ljava/lang/Class;Ljava/lang/String;)J"),
    NATIVE_METHOD(ObjectOutputStream, getFieldObj, "(Ljava/lang/Object;Ljava/lang/Class;Ljava/lang/String;Ljava/lang/String;)Ljava/lang/Object;"),
    NATIVE_METHOD(ObjectOutputStream, getFieldShort, "(Ljava/lang/Object;Ljava/lang/Class;Ljava/lang/String;)S"),
};
int register_java_io_ObjectOutputStream(JNIEnv* env) {
    return jniRegisterNativeMethods(env, "java/io/ObjectOutputStream", gMethods, NELEM(gMethods));
}
