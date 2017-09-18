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

package libcore.base;

import java.io.IOException;
import java.io.InputStream;
import java.util.concurrent.atomic.AtomicReference;

public final class Streams {
    private static AtomicReference<byte[]> skipBuffer = new AtomicReference<byte[]>();

    private Streams() {}

    public static void skipAll(InputStream in) throws IOException {
        do {
            in.skip(Long.MAX_VALUE);
        } while (in.read() != -1);
    }

    /**
     * Call {@code in.read()} repeatedly until either the stream is exhausted or
     * {@code byteCount} bytes have been read.
     *
     * <p>This method reuses the skip buffer but is careful to never use it at
     * the same time that another stream is using it. Otherwise streams that use
     * the caller's buffer for consistency checks like CRC could be clobbered by
     * other threads. A thread-local buffer is also insufficient because some
     * streams may call other streams in their skip() method, also clobbering the
     * buffer.
     */
    public static long skipByReading(InputStream in, long byteCount) throws IOException {
        // acquire the shared skip buffer.
        byte[] buffer = skipBuffer.getAndSet(null);
        if (buffer == null) {
            buffer = new byte[4096];
        }

        long skipped = 0;
        while (skipped < byteCount) {
            int toRead = (int) Math.min(byteCount - skipped, buffer.length);
            int read = in.read(buffer, 0, toRead);
            if (read == -1) {
                break;
            }
            skipped += read;
            if (read < toRead) {
                break;
            }
        }

        // release the shared skip buffer.
        skipBuffer.set(buffer);

        return skipped;
    }
}
