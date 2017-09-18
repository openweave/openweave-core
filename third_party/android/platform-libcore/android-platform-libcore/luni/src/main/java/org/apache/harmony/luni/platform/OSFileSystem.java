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

// BEGIN android-note
// address length was changed from long to int for performance reasons.
// END android-note

package org.apache.harmony.luni.platform;

import java.io.FileDescriptor;
import java.io.FileNotFoundException;
import java.io.IOException;

class OSFileSystem implements IFileSystem {

    private static final OSFileSystem singleton = new OSFileSystem();

    public static OSFileSystem getOSFileSystem() {
        return singleton;
    }

    private OSFileSystem() {
    }

    private final void validateLockArgs(int type, long start, long length) {
        if ((type != IFileSystem.SHARED_LOCK_TYPE)
                && (type != IFileSystem.EXCLUSIVE_LOCK_TYPE)) {
            throw new IllegalArgumentException("Illegal lock type requested.");
        }

        // Start position
        if (start < 0) {
            throw new IllegalArgumentException("start < 0");
        }

        // Length of lock stretch
        if (length < 0) {
            throw new IllegalArgumentException("length < 0");
        }
    }

    private native int lockImpl(int fd, long start, long length, int type, boolean wait);

    /**
     * Returns the granularity for virtual memory allocation.
     * Note that this value for Windows differs from the one for the
     * page size (64K and 4K respectively).
     */
    public native int getAllocGranularity();

    public native long length(int fd);

    public boolean lock(int fd, long start, long length, int type, boolean waitFlag)
            throws IOException {
        // Validate arguments
        validateLockArgs(type, start, length);
        int result = lockImpl(fd, start, length, type, waitFlag);
        return result != -1;
    }

    private native void unlockImpl(int fd, long start, long length) throws IOException;

    public void unlock(int fd, long start, long length) throws IOException {
        // Validate arguments
        validateLockArgs(IFileSystem.SHARED_LOCK_TYPE, start, length);
        unlockImpl(fd, start, length);
    }

    public native void fsync(int fd, boolean metadata) throws IOException;

    /*
     * File position seeking.
     */
    public native long seek(int fd, long offset, int whence) throws IOException;

    /*
     * Direct read/write APIs work on addresses.
     */
    public native long readDirect(int fd, int address, int offset, int length);

    public native long writeDirect(int fd, int address, int offset, int length);

    /*
     * Indirect read/writes work on byte[]'s
     */
    public native long read(int fd, byte[] bytes, int offset, int length) throws IOException;

    public native long write(int fd, byte[] bytes, int offset, int length) throws IOException;

    /*
     * Scatter/gather calls.
     */
    public native long readv(int fd, int[] addresses, int[] offsets, int[] lengths, int size)
            throws IOException;

    public native long writev(int fd, int[] addresses, int[] offsets, int[] lengths, int size)
            throws IOException;

    public native void truncate(int fd, long size) throws IOException;

    public native int open(String path, int mode) throws FileNotFoundException;

    public native long transfer(int fd, FileDescriptor sd, long offset, long count)
            throws IOException;

    public native int ioctlAvailable(FileDescriptor fileDescriptor) throws IOException;
}
