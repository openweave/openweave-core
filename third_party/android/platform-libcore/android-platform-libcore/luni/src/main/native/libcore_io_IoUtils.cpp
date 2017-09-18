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

#define LOG_TAG "IoUtils"

#include "JNIHelp.h"
#include "JniConstants.h"
#include "NetworkUtilities.h"
#include "ScopedPrimitiveArray.h"

#include <errno.h>
#include <unistd.h>

static void IoUtils_close(JNIEnv* env, jclass, jobject fileDescriptor) {
    int fd = jniGetFDFromFileDescriptor(env, fileDescriptor);
    jint rc = TEMP_FAILURE_RETRY(close(fd));
    if (rc == -1) {
        jniThrowIOException(env, errno);
    }
    jniSetFileDescriptorOfFD(env, fileDescriptor, -1);
}

static jint IoUtils_getFd(JNIEnv* env, jclass, jobject fileDescriptor) {
    return jniGetFDFromFileDescriptor(env, fileDescriptor);
}

static void IoUtils_pipe(JNIEnv* env, jclass, jintArray javaFds) {
    ScopedIntArrayRW fds(env, javaFds);
    if (fds.get() == NULL) {
        return;
    }
    int rc = pipe(&fds[0]);
    if (rc == -1) {
        jniThrowIOException(env, errno);
        return;
    }
}

static void IoUtils_setFd(JNIEnv* env, jclass, jobject fileDescriptor, jint newValue) {
    return jniSetFileDescriptorOfFD(env, fileDescriptor, newValue);
}

static void IoUtils_setBlocking(JNIEnv* env, jclass, jobject fileDescriptor, jboolean blocking) {
    int fd = jniGetFDFromFileDescriptor(env, fileDescriptor);
    if (fd == -1) {
        return;
    }
    if (!setBlocking(fd, blocking)) {
        jniThrowIOException(env, errno);
    }
}

static JNINativeMethod gMethods[] = {
    NATIVE_METHOD(IoUtils, close, "(Ljava/io/FileDescriptor;)V"),
    NATIVE_METHOD(IoUtils, getFd, "(Ljava/io/FileDescriptor;)I"),
    NATIVE_METHOD(IoUtils, pipe, "([I)V"),
    NATIVE_METHOD(IoUtils, setFd, "(Ljava/io/FileDescriptor;I)V"),
    NATIVE_METHOD(IoUtils, setBlocking, "(Ljava/io/FileDescriptor;Z)V"),
};
int register_libcore_io_IoUtils(JNIEnv* env) {
    return jniRegisterNativeMethods(env, "libcore/io/IoUtils", gMethods, NELEM(gMethods));
}
