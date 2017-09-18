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

#define LOG_TAG "FileDescriptor"

#include "JNIHelp.h"
#include "JniConstants.h"

#include <errno.h>
#include <unistd.h>

static void FileDescriptor_syncImpl(JNIEnv* env, jobject obj) {
    int fd = jniGetFDFromFileDescriptor(env, obj);
    int rc = fsync(fd);
    if (rc == -1) {
        /*
         * If fd is a socket, then fsync(fd) is defined to fail with
         * errno EINVAL. This isn't actually cause for concern.
         * TODO: Look into not bothering to call fsync() at all if
         * we know we are dealing with a socket.
         */
        if (errno != EINVAL) {
            char buf[BUFSIZ];
            jniThrowException(env, "java/io/SyncFailedException",
                    jniStrError(errno, buf, sizeof(buf)));
        }
    }
}

static JNINativeMethod gMethods[] = {
    NATIVE_METHOD(FileDescriptor, syncImpl, "()V"),
};
int register_java_io_FileDescriptor(JNIEnv* env) {
    return jniRegisterNativeMethods(env, "java/io/FileDescriptor", gMethods, NELEM(gMethods));
}
