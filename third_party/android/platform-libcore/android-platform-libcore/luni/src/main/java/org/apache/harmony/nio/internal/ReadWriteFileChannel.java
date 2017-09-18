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

/*
 * Android Notice
 * In this class the address length was changed from long to int.
 * This is due to performance optimizations for the device.
 */

package org.apache.harmony.nio.internal;

import java.io.IOException;
import java.nio.MappedByteBuffer;

public final class ReadWriteFileChannel extends FileChannelImpl {
    public ReadWriteFileChannel(Object stream, int handle) {
        super(stream, handle);
    }

    public final MappedByteBuffer map(MapMode mode, long position, long size) throws IOException {
        openCheck();
        if (mode == null) {
            throw new NullPointerException();
        }
        if (position < 0 || size < 0 || size > Integer.MAX_VALUE) {
            throw new IllegalArgumentException();
        }
        return mapImpl(mode, position, size);
    }
}
