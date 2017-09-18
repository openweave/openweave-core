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

package libcore.io;

import java.io.Closeable;
import java.io.FileDescriptor;
import java.io.IOException;

public final class IoUtils {
    private IoUtils() {
    }

    /**
     * Calls close(2) on 'fd'. Also resets the internal int to -1.
     */
    public static native void close(FileDescriptor fd) throws IOException;

    /**
     * Closes 'closeable', ignoring any exceptions. Does nothing if 'closeable' is null.
     */
    public static void closeQuietly(Closeable closeable) {
        if (closeable != null) {
            try {
                closeable.close();
            } catch (IOException ignored) {
            }
        }
    }

    /**
     * Returns the int file descriptor from within the given FileDescriptor 'fd'.
     */
    public static native int getFd(FileDescriptor fd);

    /**
     * Returns a new FileDescriptor whose internal integer is set to 'fd'.
     */
    public static FileDescriptor newFileDescriptor(int fd) {
        FileDescriptor result = new FileDescriptor();
        setFd(result, fd);
        return result;
    }

    /**
     * Creates a pipe by calling pipe(2), returning the two file descriptors in
     * elements 0 and 1 of the array 'fds'. fds[0] is the read end of the pipe.
     * fds[1] is the write end of the pipe. Throws an appropriate IOException on failure.
     */
    public static native void pipe(int[] fds) throws IOException;

    /**
     * Sets the int file descriptor within the given FileDescriptor 'fd' to 'newValue'.
     */
    public static native void setFd(FileDescriptor fd, int newValue);

    /**
     * Sets 'fd' to be blocking or non-blocking, according to the state of 'blocking'.
     */
    public static native void setBlocking(FileDescriptor fd, boolean blocking) throws IOException;
}
