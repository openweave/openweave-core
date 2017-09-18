/*
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package org.apache.harmony.luni.platform;

import junit.framework.TestCase;

/**
 * Tests org.apache.harmony.luni.platform.OSMemory.
 */
public class OSMemoryTest extends TestCase {
    public void testMemset() {
        int byteCount = 32;
        int ptr = OSMemory.malloc(byteCount);
        try {
            // Ensure our newly-allocated block isn't zeroed.
            OSMemory.setByte(ptr, (byte) 1);
            assertEquals((byte) 1, OSMemory.getByte(ptr));
            // Check that we can clear memory.
            OSMemory.memset(ptr, (byte) 0, byteCount);
            assertBytesEqual((byte) 0, ptr, byteCount);
            // Check that we can set an arbitrary value.
            OSMemory.memset(ptr, (byte) 1, byteCount);
            assertBytesEqual((byte) 1, ptr, byteCount);
        } finally {
            OSMemory.free(ptr);
        }
    }

    void assertBytesEqual(byte value, int ptr, int byteCount) {
        for (int i = 0; i < byteCount; ++i) {
            assertEquals(value, OSMemory.getByte(ptr + i));
        }
    }

    public void testSetIntArray() {
        int[] values = { 3, 7, 31, 127, 8191, 131071, 524287, 2147483647 };
        int[] swappedValues = new int[values.length];
        for (int i = 0; i < values.length; ++i) {
            swappedValues[i] = swapInt(values[i]);
        }

        int scale = PlatformAddress.SIZEOF_JINT;
        int ptr = OSMemory.malloc(scale * values.length);
        try {
            // Regular copy. Memset first so we start from a known state.
            OSMemory.memset(ptr, (byte) 0, scale * values.length);
            OSMemory.setIntArray(ptr, values, 0, values.length, false);
            assertIntsEqual(values, ptr);

            // Swapped copy.
            OSMemory.memset(ptr, (byte) 0, scale * values.length);
            OSMemory.setIntArray(ptr, values, 0, values.length, true);
            assertIntsEqual(swappedValues, ptr);

            // Swapped copies of slices (to ensure we test non-zero offsets).
            OSMemory.memset(ptr, (byte) 0, scale * values.length);
            for (int i = 0; i < values.length; ++i) {
                OSMemory.setIntArray(ptr + i * scale, values, i, 1, true);
            }
            assertIntsEqual(swappedValues, ptr);
        } finally {
            OSMemory.free(ptr);
        }
    }

    private void assertIntsEqual(int[] expectedValues, int ptr) {
        for (int i = 0; i < expectedValues.length; ++i) {
            assertEquals(expectedValues[i], OSMemory.getInt(ptr + PlatformAddress.SIZEOF_JINT * i));
        }
    }

    private static int swapInt(int n) {
        return (((n >>  0) & 0xff) << 24) |
               (((n >>  8) & 0xff) << 16) |
               (((n >> 16) & 0xff) <<  8) |
               (((n >> 24) & 0xff) <<  0);
    }

    public void testSetShortArray() {
        short[] values = { 0x0001, 0x0020, 0x0300, 0x4000 };
        short[] swappedValues = { 0x0100, 0x2000, 0x0003, 0x0040 };

        int scale = PlatformAddress.SIZEOF_JSHORT;
        int ptr = OSMemory.malloc(scale * values.length);
        try {
            // Regular copy. Memset first so we start from a known state.
            OSMemory.memset(ptr, (byte) 0, scale * values.length);
            OSMemory.setShortArray(ptr, values, 0, values.length, false);
            assertShortsEqual(values, ptr);

            // Swapped copy.
            OSMemory.memset(ptr, (byte) 0, scale * values.length);
            OSMemory.setShortArray(ptr, values, 0, values.length, true);
            assertShortsEqual(swappedValues, ptr);

            // Swapped copies of slices (to ensure we test non-zero offsets).
            OSMemory.memset(ptr, (byte) 0, scale * values.length);
            for (int i = 0; i < values.length; ++i) {
                OSMemory.setShortArray(ptr + i * scale, values, i, 1, true);
            }
            assertShortsEqual(swappedValues, ptr);
        } finally {
            OSMemory.free(ptr);
        }
    }

    private void assertShortsEqual(short[] expectedValues, int ptr) {
        for (int i = 0; i < expectedValues.length; ++i) {
            assertEquals(expectedValues[i], OSMemory.getShort(ptr + PlatformAddress.SIZEOF_JSHORT * i));
        }
    }

    private static short swapShort(short n) {
        return (short) ((((n >>  0) & 0xff) << 8) | (((n >>  8) & 0xff) << 0));
    }
}
