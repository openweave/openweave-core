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

#define LOG_TAG "ObjectStreamClass"

#include "JNIHelp.h"
#include "JniConstants.h"

static jobject getSignature(JNIEnv* env, jclass c, jobject object) {
    jmethodID mid = env->GetMethodID(c, "getSignature", "()Ljava/lang/String;");
    if (!mid) {
        return NULL;
    }
    jclass objectClass = env->GetObjectClass(object);
    return env->CallNonvirtualObjectMethod(object, objectClass, mid);
}

static jobject ObjectStreamClass_getFieldSignature(JNIEnv* env, jclass, jobject field) {
    return getSignature(env, JniConstants::fieldClass, field);
}

static jobject ObjectStreamClass_getMethodSignature(JNIEnv* env, jclass, jobject method) {
    return getSignature(env, JniConstants::methodClass, method);
}

static jobject ObjectStreamClass_getConstructorSignature(JNIEnv* env, jclass, jobject constructor) {
    return getSignature(env, JniConstants::constructorClass, constructor);
}

static jboolean ObjectStreamClass_hasClinit(JNIEnv * env, jclass, jclass targetClass) {
    jmethodID mid = env->GetStaticMethodID(targetClass, "<clinit>", "()V");
    env->ExceptionClear();
    return (mid != 0);
}

static JNINativeMethod gMethods[] = {
    NATIVE_METHOD(ObjectStreamClass, getConstructorSignature, "(Ljava/lang/reflect/Constructor;)Ljava/lang/String;"),
    NATIVE_METHOD(ObjectStreamClass, getFieldSignature, "(Ljava/lang/reflect/Field;)Ljava/lang/String;"),
    NATIVE_METHOD(ObjectStreamClass, getMethodSignature, "(Ljava/lang/reflect/Method;)Ljava/lang/String;"),
    NATIVE_METHOD(ObjectStreamClass, hasClinit, "(Ljava/lang/Class;)Z"),
};
int register_java_io_ObjectStreamClass(JNIEnv* env) {
    return jniRegisterNativeMethods(env, "java/io/ObjectStreamClass", gMethods, NELEM(gMethods));
}
