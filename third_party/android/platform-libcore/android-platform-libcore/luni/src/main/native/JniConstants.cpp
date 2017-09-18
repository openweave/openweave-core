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

#include "JniConstants.h"

#include <stdlib.h>

jclass JniConstants::bidiRunClass;
jclass JniConstants::bigDecimalClass;
jclass JniConstants::booleanClass;
jclass JniConstants::byteClass;
jclass JniConstants::byteArrayClass;
jclass JniConstants::charsetICUClass;
jclass JniConstants::constructorClass;
jclass JniConstants::datagramPacketClass;
jclass JniConstants::deflaterClass;
jclass JniConstants::doubleClass;
jclass JniConstants::fieldClass;
jclass JniConstants::fieldPositionIteratorClass;
jclass JniConstants::multicastGroupRequestClass;
jclass JniConstants::inetAddressClass;
jclass JniConstants::inflaterClass;
jclass JniConstants::integerClass;
jclass JniConstants::interfaceAddressClass;
jclass JniConstants::localeDataClass;
jclass JniConstants::longClass;
jclass JniConstants::methodClass;
jclass JniConstants::parsePositionClass;
jclass JniConstants::patternSyntaxExceptionClass;
jclass JniConstants::realToStringClass;
jclass JniConstants::socketClass;
jclass JniConstants::socketImplClass;
jclass JniConstants::stringClass;
jclass JniConstants::vmRuntimeClass;

static jclass findClass(JNIEnv* env, const char* name) {
    jclass result = reinterpret_cast<jclass>(env->NewGlobalRef(env->FindClass(name)));
    if (result == NULL) {
        LOGE("failed to find class '%s'", name);
        abort();
    }
    return result;
}

void JniConstants::init(JNIEnv* env) {
    bidiRunClass = findClass(env, "org/apache/harmony/text/BidiRun");
    bigDecimalClass = findClass(env, "java/math/BigDecimal");
    booleanClass = findClass(env, "java/lang/Boolean");
    byteClass = findClass(env, "java/lang/Byte");
    byteArrayClass = findClass(env, "[B");
    charsetICUClass = findClass(env, "com/ibm/icu4jni/charset/CharsetICU");
    constructorClass = findClass(env, "java/lang/reflect/Constructor");
    datagramPacketClass = findClass(env, "java/net/DatagramPacket");
    deflaterClass = findClass(env, "java/util/zip/Deflater");
    doubleClass = findClass(env, "java/lang/Double");
    fieldClass = findClass(env, "java/lang/reflect/Field");
    fieldPositionIteratorClass = findClass(env, "com/ibm/icu4jni/text/NativeDecimalFormat$FieldPositionIterator");
    inetAddressClass = findClass(env, "java/net/InetAddress");
    inflaterClass = findClass(env, "java/util/zip/Inflater");
    integerClass = findClass(env, "java/lang/Integer");
    interfaceAddressClass = findClass(env, "java/net/InterfaceAddress");
    localeDataClass = findClass(env, "com/ibm/icu4jni/util/LocaleData");
    longClass = findClass(env, "java/lang/Long");
    methodClass = findClass(env, "java/lang/reflect/Method");
    multicastGroupRequestClass = findClass(env, "java/net/MulticastGroupRequest");
    parsePositionClass = findClass(env, "java/text/ParsePosition");
    patternSyntaxExceptionClass = findClass(env, "java/util/regex/PatternSyntaxException");
    realToStringClass = findClass(env, "java/lang/RealToString");
    socketClass = findClass(env, "java/net/Socket");
    socketImplClass = findClass(env, "java/net/SocketImpl");
    stringClass = findClass(env, "java/lang/String");
    vmRuntimeClass = findClass(env, "dalvik/system/VMRuntime");
}
