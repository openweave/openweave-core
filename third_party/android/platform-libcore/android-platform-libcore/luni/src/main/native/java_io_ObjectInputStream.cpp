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

#define LOG_TAG "ObjectInputStream"

#include "JNIHelp.h"
#include "JniConstants.h"
#include "ScopedUtfChars.h"

#define SETTER(FUNCTION_NAME, JNI_C_TYPE, JNI_TYPE_STRING, JNI_SETTER_FUNCTION) \
    static void FUNCTION_NAME(JNIEnv* env, jclass, jobject instance, \
            jclass declaringClass, jstring javaFieldName, JNI_C_TYPE newValue) { \
        if (instance == NULL) { \
            return; \
        } \
        ScopedUtfChars fieldName(env, javaFieldName); \
        if (fieldName.c_str() == NULL) { \
            return; \
        } \
        jfieldID fid = env->GetFieldID(declaringClass, fieldName.c_str(), JNI_TYPE_STRING); \
        if (fid != 0) { \
            env->JNI_SETTER_FUNCTION(instance, fid, newValue); \
        } \
    }

SETTER(ObjectInputStream_setFieldBool,   jboolean, "Z", SetBooleanField)
SETTER(ObjectInputStream_setFieldByte,   jbyte,    "B", SetByteField)
SETTER(ObjectInputStream_setFieldChar,   jchar,    "C", SetCharField)
SETTER(ObjectInputStream_setFieldDouble, jdouble,  "D", SetDoubleField)
SETTER(ObjectInputStream_setFieldFloat,  jfloat,   "F", SetFloatField)
SETTER(ObjectInputStream_setFieldInt,    jint,     "I", SetIntField)
SETTER(ObjectInputStream_setFieldLong,   jlong,    "J", SetLongField)
SETTER(ObjectInputStream_setFieldShort,  jshort,   "S", SetShortField)

static void ObjectInputStream_setFieldObject(JNIEnv* env, jclass, jobject instance,
        jclass declaringClass, jstring javaFieldName, jstring javaFieldTypeName, jobject newValue) {
    if (instance == NULL) {
        return;
    }
    ScopedUtfChars fieldName(env, javaFieldName);
    if (fieldName.c_str() == NULL) {
        return;
    }
    ScopedUtfChars fieldTypeName(env, javaFieldTypeName);
    if (fieldTypeName.c_str() == NULL) {
        return;
    }
    jfieldID fid = env->GetFieldID(declaringClass, fieldName.c_str(), fieldTypeName.c_str());
    if (fid != 0) {
        env->SetObjectField(instance, fid, newValue);
    }
}

static jobject ObjectInputStream_newInstance(JNIEnv* env, jclass,
        jclass instantiationClass, jclass constructorClass) {
    jmethodID mid = env->GetMethodID(constructorClass, "<init>", "()V");
    if (mid == 0) {
        return NULL;
    }
    return env->NewObject(instantiationClass, mid);
}

static JNINativeMethod gMethods[] = {
    NATIVE_METHOD(ObjectInputStream, newInstance, "(Ljava/lang/Class;Ljava/lang/Class;)Ljava/lang/Object;"),
    NATIVE_METHOD(ObjectInputStream, setFieldObject, "(Ljava/lang/Object;Ljava/lang/Class;Ljava/lang/String;Ljava/lang/String;Ljava/lang/Object;)V"),
    NATIVE_METHOD(ObjectInputStream, setFieldByte, "(Ljava/lang/Object;Ljava/lang/Class;Ljava/lang/String;B)V"),
    NATIVE_METHOD(ObjectInputStream, setFieldChar, "(Ljava/lang/Object;Ljava/lang/Class;Ljava/lang/String;C)V"),
    NATIVE_METHOD(ObjectInputStream, setFieldDouble, "(Ljava/lang/Object;Ljava/lang/Class;Ljava/lang/String;D)V"),
    NATIVE_METHOD(ObjectInputStream, setFieldFloat, "(Ljava/lang/Object;Ljava/lang/Class;Ljava/lang/String;F)V"),
    NATIVE_METHOD(ObjectInputStream, setFieldInt, "(Ljava/lang/Object;Ljava/lang/Class;Ljava/lang/String;I)V"),
    NATIVE_METHOD(ObjectInputStream, setFieldLong, "(Ljava/lang/Object;Ljava/lang/Class;Ljava/lang/String;J)V"),
    NATIVE_METHOD(ObjectInputStream, setFieldShort, "(Ljava/lang/Object;Ljava/lang/Class;Ljava/lang/String;S)V"),
    NATIVE_METHOD(ObjectInputStream, setFieldBool, "(Ljava/lang/Object;Ljava/lang/Class;Ljava/lang/String;Z)V"),
};
int register_java_io_ObjectInputStream(JNIEnv* env) {
    return jniRegisterNativeMethods(env, "java/io/ObjectInputStream", gMethods, NELEM(gMethods));
}
