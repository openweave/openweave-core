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

#include "jni.h"

#include <sys/socket.h>

// Convert from byte[] to InetAddress.
jobject byteArrayToInetAddress(JNIEnv* env, jbyteArray byteArray);

// Convert from byte[] to sockaddr_storage.
bool byteArrayToSocketAddress(JNIEnv* env, jclass, jbyteArray byteArray, int port, sockaddr_storage* ss);

// Convert from sockaddr_storage to byte[].
jbyteArray socketAddressToByteArray(JNIEnv* env, sockaddr_storage* ss);

// Convert from sockaddr_storage to InetAddress.
jobject socketAddressToInetAddress(JNIEnv* env, sockaddr_storage* ss);



// Changes 'fd' to be blocking/non-blocking. Returns false and sets errno on failure.
bool setBlocking(int fd, bool blocking);
