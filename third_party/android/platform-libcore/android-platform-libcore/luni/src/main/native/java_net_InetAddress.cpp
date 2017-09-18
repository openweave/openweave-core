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

#define LOG_TAG "InetAddress"

#define LOG_DNS 0

#include "JNIHelp.h"
#include "JniConstants.h"
#include "LocalArray.h"
#include "NetworkUtilities.h"
#include "ScopedLocalRef.h"
#include "ScopedUtfChars.h"
#include "jni.h"
#include "utils/Log.h"

#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

static jstring InetAddress_gethostname(JNIEnv* env, jclass)
{
    char name[256];
    int r = gethostname(name, 256);
    if (r == 0) {
        return env->NewStringUTF(name);
    } else {
        return NULL;
    }
}

#if LOG_DNS
static void logIpString(addrinfo* ai, const char* name)
{
    char ipString[INET6_ADDRSTRLEN];
    int result = getnameinfo(ai->ai_addr, ai->ai_addrlen, ipString,
                             sizeof(ipString), NULL, 0, NI_NUMERICHOST);
    if (result == 0) {
        LOGD("%s: %s (family %d, proto %d)", name, ipString, ai->ai_family,
             ai->ai_protocol);
    } else {
        LOGE("%s: getnameinfo: %s", name, gai_strerror(result));
    }
}
#else
static inline void logIpString(addrinfo*, const char*)
{
}
#endif

static jobjectArray InetAddress_getaddrinfo(JNIEnv* env, jclass, jstring javaName) {
    ScopedUtfChars name(env, javaName);
    if (name.c_str() == NULL) {
        return NULL;
    }

    addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_flags = AI_ADDRCONFIG;
    /*
     * If we don't specify a socket type, every address will appear twice, once
     * for SOCK_STREAM and one for SOCK_DGRAM. Since we do not return the family
     * anyway, just pick one.
     */
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* addressList = NULL;
    jobjectArray addressArray = NULL;
    int result = getaddrinfo(name.c_str(), NULL, &hints, &addressList);
    if (result == 0 && addressList) {
        // Count results so we know how to size the output array.
        int addressCount = 0;
        for (addrinfo* ai = addressList; ai != NULL; ai = ai->ai_next) {
            if (ai->ai_family == AF_INET || ai->ai_family == AF_INET6) {
                addressCount++;
            }
        }

        // Prepare output array.
        addressArray = env->NewObjectArray(addressCount, JniConstants::byteArrayClass, NULL);
        if (addressArray == NULL) {
            // Appropriate exception will be thrown.
            LOGE("getaddrinfo: could not allocate array of size %i", addressCount);
            freeaddrinfo(addressList);
            return NULL;
        }

        // Examine returned addresses one by one, save them in the output array.
        int index = 0;
        for (addrinfo* ai = addressList; ai != NULL; ai = ai->ai_next) {
            sockaddr* address = ai->ai_addr;
            size_t addressLength = 0;
            void* rawAddress;

            switch (ai->ai_family) {
                // Find the raw address length and start pointer.
                case AF_INET6:
                    addressLength = 16;
                    rawAddress = &reinterpret_cast<sockaddr_in6*>(address)->sin6_addr.s6_addr;
                    logIpString(ai, name.c_str());
                    break;
                case AF_INET:
                    addressLength = 4;
                    rawAddress = &reinterpret_cast<sockaddr_in*>(address)->sin_addr.s_addr;
                    logIpString(ai, name.c_str());
                    break;
                default:
                    // Unknown address family. Skip this address.
                    LOGE("getaddrinfo: Unknown address family %d", ai->ai_family);
                    continue;
            }

            // Convert each IP address into a Java byte array.
            ScopedLocalRef<jbyteArray> byteArray(env, env->NewByteArray(addressLength));
            if (byteArray.get() == NULL) {
                // Out of memory error will be thrown on return.
                LOGE("getaddrinfo: Can't allocate %d-byte array", addressLength);
                addressArray = NULL;
                break;
            }
            env->SetByteArrayRegion(byteArray.get(),
                    0, addressLength, reinterpret_cast<jbyte*>(rawAddress));
            env->SetObjectArrayElement(addressArray, index, byteArray.get());
            index++;
        }
    } else if (result == EAI_SYSTEM && errno == EACCES) {
        /* No permission to use network */
        jniThrowException(env, "java/lang/SecurityException",
            "Permission denied (maybe missing INTERNET permission)");
    } else {
        jniThrowException(env, "java/net/UnknownHostException", gai_strerror(result));
    }

    if (addressList) {
        freeaddrinfo(addressList);
    }

    return addressArray;
}

/**
 * Looks up the name corresponding to an IP address.
 *
 * @param javaAddress: a byte array containing the raw IP address bytes. Must be
 *         4 or 16 bytes long.
 * @return the hostname.
 * @throws UnknownHostException: the IP address has no associated hostname.
 */
static jstring InetAddress_getnameinfo(JNIEnv* env, jclass,
                                         jbyteArray javaAddress)
{
    if (javaAddress == NULL) {
        jniThrowNullPointerException(env, NULL);
        return NULL;
    }

    // Convert the raw address bytes into a socket address structure.
    sockaddr_storage ss;
    memset(&ss, 0, sizeof(ss));

    size_t socklen;
    const size_t addressLength = env->GetArrayLength(javaAddress);
    if (addressLength == 4) {
        sockaddr_in* sin = reinterpret_cast<sockaddr_in*>(&ss);
        sin->sin_family = AF_INET;
        socklen = sizeof(sockaddr_in);
        jbyte* dst = reinterpret_cast<jbyte*>(&sin->sin_addr.s_addr);
        env->GetByteArrayRegion(javaAddress, 0, 4, dst);
    } else if (addressLength == 16) {
        sockaddr_in6* sin6 = reinterpret_cast<sockaddr_in6*>(&ss);
        sin6->sin6_family = AF_INET6;
        socklen = sizeof(sockaddr_in6);
        jbyte* dst = reinterpret_cast<jbyte*>(&sin6->sin6_addr.s6_addr);
        env->GetByteArrayRegion(javaAddress, 0, 16, dst);
    } else {
        // The caller already throws an exception in this case. Don't worry
        // about it here.
        return NULL;
    }

    // Look up the host name from the IP address.
    char name[NI_MAXHOST];
    int ret = getnameinfo(reinterpret_cast<sockaddr*>(&ss), socklen,
                          name, sizeof(name), NULL, 0, NI_NAMEREQD);
    if (ret != 0) {
        jniThrowException(env, "java/net/UnknownHostException", gai_strerror(ret));
        return NULL;
    }

    return env->NewStringUTF(name);
}

/**
 * Convert a Java string representing an IP address to a Java byte array.
 * The formats accepted are:
 * - IPv4:
 *   - 1.2.3.4
 *   - 1.2.4
 *   - 1.4
 *   - 4
 * - IPv6
 *   - Compressed form (2001:db8::1)
 *   - Uncompressed form (2001:db8:0:0:0:0:0:1)
 *   - IPv4-compatible (::192.0.2.0)
 *   - With an embedded IPv4 address (2001:db8::192.0.2.0).
 * IPv6 addresses may appear in square brackets.
 *
 * @param addressByteArray the byte array to convert.
 * @return a string with the textual representation of the address.
 * @throws UnknownHostException the IP address was invalid.
 */
static jbyteArray InetAddress_ipStringToByteArray(JNIEnv* env, jobject, jstring javaString) {
    // Convert the String to UTF-8 bytes.
    ScopedUtfChars chars(env, javaString);
    if (chars.c_str() == NULL) {
        return NULL;
    }
    size_t byteCount = chars.size();
    LocalArray<INET6_ADDRSTRLEN> bytes(byteCount + 1);
    char* ipString = &bytes[0];
    strcpy(ipString, chars.c_str());

    // Accept IPv6 addresses (only) in square brackets for compatibility.
    if (ipString[0] == '[' && ipString[byteCount - 1] == ']' && strchr(ipString, ':') != NULL) {
        memmove(ipString, ipString + 1, byteCount - 2);
        ipString[byteCount - 2] = '\0';
    }

    jbyteArray result = NULL;
    addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = AI_NUMERICHOST;

    sockaddr_storage ss;
    memset(&ss, 0, sizeof(ss));

    addrinfo* res = NULL;
    int ret = getaddrinfo(ipString, NULL, &hints, &res);
    if (ret == 0 && res) {
        // Convert IPv4-mapped addresses to IPv4 addresses.
        // The RI states "Java will never return an IPv4-mapped address".
        sockaddr_in6* sin6 = reinterpret_cast<sockaddr_in6*>(res->ai_addr);
        if (res->ai_family == AF_INET6 && IN6_IS_ADDR_V4MAPPED(&sin6->sin6_addr)) {
            sockaddr_in* sin = reinterpret_cast<sockaddr_in*>(&ss);
            sin->sin_family = AF_INET;
            sin->sin_port = sin6->sin6_port;
            memcpy(&sin->sin_addr.s_addr, &sin6->sin6_addr.s6_addr[12], 4);
            result = socketAddressToByteArray(env, &ss);
        } else {
            result = socketAddressToByteArray(env, reinterpret_cast<sockaddr_storage*>(res->ai_addr));
        }
    } else {
        // For backwards compatibility, deal with address formats that
        // getaddrinfo does not support. For example, 1.2.3, 1.3, and even 3 are
        // valid IPv4 addresses according to the Java API. If getaddrinfo fails,
        // try to use inet_aton.
        sockaddr_in* sin = reinterpret_cast<sockaddr_in*>(&ss);
        if (inet_aton(ipString, &sin->sin_addr)) {
            sin->sin_family = AF_INET;
            sin->sin_port = 0;
            result = socketAddressToByteArray(env, &ss);
        }
    }

    if (res) {
        freeaddrinfo(res);
    }

    if (!result) {
        env->ExceptionClear();
        jniThrowException(env, "java/net/UnknownHostException", gai_strerror(ret));
    }

    return result;
}

static jstring InetAddress_byteArrayToIpString(JNIEnv* env, jobject, jbyteArray byteArray) {
    if (byteArray == NULL) {
        jniThrowNullPointerException(env, NULL);
        return NULL;
    }
    sockaddr_storage ss;
    if (!byteArrayToSocketAddress(env, NULL, byteArray, 0, &ss)) {
        return NULL;
    }
    // TODO: getnameinfo seems to want its length parameter to be exactly
    // sizeof(sockaddr_in) for an IPv4 address and sizeof (sockaddr_in6) for an
    // IPv6 address. Fix getnameinfo so it accepts sizeof(sockaddr_storage), and
    // then remove this hack.
    int sa_size;
    if (ss.ss_family == AF_INET) {
        sa_size = sizeof(sockaddr_in);
    } else if (ss.ss_family == AF_INET6) {
        sa_size = sizeof(sockaddr_in6);
    } else {
        // byteArrayToSocketAddress already threw.
        return NULL;
    }
    char ipString[INET6_ADDRSTRLEN];
    int rc = getnameinfo(reinterpret_cast<sockaddr*>(&ss), sa_size,
            ipString, sizeof(ipString), NULL, 0, NI_NUMERICHOST);
    if (rc != 0) {
        jniThrowException(env, "java/net/UnknownHostException", gai_strerror(rc));
        return NULL;
    }
    return env->NewStringUTF(ipString);
}

static JNINativeMethod gMethods[] = {
    NATIVE_METHOD(InetAddress, byteArrayToIpString, "([B)Ljava/lang/String;"),
    NATIVE_METHOD(InetAddress, getaddrinfo, "(Ljava/lang/String;)[[B"),
    NATIVE_METHOD(InetAddress, gethostname, "()Ljava/lang/String;"),
    NATIVE_METHOD(InetAddress, getnameinfo, "([B)Ljava/lang/String;"),
    NATIVE_METHOD(InetAddress, ipStringToByteArray, "(Ljava/lang/String;)[B"),
};
int register_java_net_InetAddress(JNIEnv* env) {
    return jniRegisterNativeMethods(env, "java/net/InetAddress", gMethods, NELEM(gMethods));
}
