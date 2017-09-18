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

import java.io.IOException;
import java.nio.ByteOrder;
import java.nio.channels.FileChannel.MapMode;

/**
 * This class enables direct access to OS memory.
 */
final class OSMemory {
    /**
     * Defines the natural byte order for this machine.
     */
    public static final ByteOrder NATIVE_ORDER = ByteOrder.nativeOrder();

    private OSMemory() {
    }

    /**
     * Allocates and returns a pointer to space for a memory block of
     * <code>length</code> bytes. The space is uninitialized and may be larger
     * than the number of bytes requested; however, the guaranteed usable memory
     * block is exactly <code>length</code> bytes int.
     *
     * @param length
     *            number of bytes requested.
     * @return the address of the start of the memory block.
     * @throws OutOfMemoryError
     *             if the request cannot be satisfied.
     */
    public static native int malloc(int length) throws OutOfMemoryError;

    /**
     * Deallocates space for a memory block that was previously allocated by a
     * call to {@link #malloc(int) malloc(int)}. The number of bytes freed is
     * identical to the number of bytes acquired when the memory block was
     * allocated. If <code>address</code> is zero the method does nothing.
     * <p>
     * Freeing a pointer to a memory block that was not allocated by
     * <code>malloc()</code> has unspecified effect.
     * </p>
     *
     * @param address
     *            the address of the memory block to deallocate.
     */
    public static native void free(int address);

    /**
     * Places <code>value</code> into first <code>length</code> bytes of the
     * memory block starting at <code>address</code>.
     * <p>
     * The behavior is unspecified if
     * <code>(address ... address + length)</code> is not wholly within the
     * range that was previously allocated using <code>malloc()</code>.
     * </p>
     *
     * @param address
     *            the address of the first memory location.
     * @param value
     *            the byte value to set at each location.
     * @param length
     *            the number of byte-length locations to set.
     */
    public static native void memset(int address, byte value, long length);

    /**
     * Copies <code>length</code> bytes from <code>srcAddress</code> to
     * <code>destAddress</code>. Where any part of the source memory block
     * and the destination memory block overlap <code>memmove()</code> ensures
     * that the original source bytes in the overlapping region are copied
     * before being overwritten.
     * <p>
     * The behavior is unspecified if
     * <code>(srcAddress ... srcAddress + length)</code> and
     * <code>(destAddress ... destAddress + length)</code> are not both wholly
     * within the range that was previously allocated using
     * <code>malloc()</code>.
     * </p>
     *
     * @param destAddress
     *            the address of the destination memory block.
     * @param srcAddress
     *            the address of the source memory block.
     * @param length
     *            the number of bytes to move.
     */
    public static native void memmove(int destAddress, int srcAddress, long length);

    /**
     * Copies <code>length</code> bytes from the memory block at
     * <code>address</code> into the byte array <code>bytes</code> starting
     * at element <code>offset</code> within the byte array.
     * <p>
     * The behavior of this method is undefined if the range
     * <code>(address ... address + length)</code> is not within a memory
     * block that was allocated using {@link #malloc(int) malloc(int)}.
     * </p>
     *
     * @param address
     *            the address of the OS memory block from which to copy bytes.
     * @param bytes
     *            the byte array into which to copy the bytes.
     * @param offset
     *            the index of the first element in <code>bytes</code> that
     *            will be overwritten.
     * @param length
     *            the total number of bytes to copy into the byte array.
     * @throws NullPointerException
     *             if <code>bytes</code> is <code>null</code>.
     * @throws IndexOutOfBoundsException
     *             if <code>offset + length > bytes.length</code>.
     */
    public static native void getByteArray(int address, byte[] bytes, int offset,
            int length) throws NullPointerException, IndexOutOfBoundsException;

    /**
     * Copies <code>length</code> bytes from the byte array <code>bytes</code>
     * into the memory block at <code>address</code>, starting at element
     * <code>offset</code> within the byte array.
     * <p>
     * The behavior of this method is undefined if the range
     * <code>(address ... address + length)</code> is not within a memory
     * block that was allocated using {@link #malloc(int) malloc(int)}.
     * </p>
     *
     * @param address
     *            the address of the OS memory block into which to copy the
     *            bytes.
     * @param bytes
     *            the byte array from which to copy the bytes.
     * @param offset
     *            the index of the first element in <code>bytes</code> that
     *            will be read.
     * @param length
     *            the total number of bytes to copy from <code>bytes</code>
     *            into the memory block.
     * @throws NullPointerException
     *             if <code>bytes</code> is <code>null</code>.
     * @throws IndexOutOfBoundsException
     *             if <code>offset + length > bytes.length</code>.
     */
    public static native void setByteArray(int address, byte[] bytes, int offset,
            int length) throws NullPointerException, IndexOutOfBoundsException;

    /**
     * Copies <code>length</code> shorts from the short array <code>shorts</code>
     * into the memory block at <code>address</code>, starting at element
     * <code>offset</code> within the short array.
     * <p>
     * The behavior of this method is undefined if the range
     * <code>(address ... address + 2*length)</code> is not within a memory
     * block that was allocated using {@link #malloc(int) malloc(int)}.
     * </p>
     *
     * @param address
     *            the address of the OS memory block into which to copy the
     *            shorts.
     * @param shorts
     *            the short array from which to copy the shorts.
     * @param offset
     *            the index of the first element in <code>shorts</code> that
     *            will be read.
     * @param length
     *            the total number of shorts to copy from <code>shorts</code>
     *            into the memory block.
     * @param swap
     *            true if the shorts should be written in reverse byte order.
     * @throws NullPointerException
     *             if <code>bytes</code> is <code>null</code>.
     * @throws IndexOutOfBoundsException
     *             if <code>offset + length > bytes.length</code>.
     */
    public static native void setShortArray(int address, short[] shorts, int offset,
            int length, boolean swap) throws NullPointerException,
            IndexOutOfBoundsException;

    /**
     * Copies <code>length</code> ints from the int array <code>ints</code>
     * into the memory block at <code>address</code>, starting at element
     * <code>offset</code> within the int array.
     * <p>
     * The behavior of this method is undefined if the range
     * <code>(address ... address + 2*length)</code> is not within a memory
     * block that was allocated using {@link #malloc(int) malloc(int)}.
     * </p>
     *
     * @param address
     *            the address of the OS memory block into which to copy the
     *            ints.
     * @param ints
     *            the int array from which to copy the ints.
     * @param offset
     *            the index of the first element in <code>ints</code> that
     *            will be read.
     * @param length
     *            the total number of ints to copy from <code>ints</code>
     *            into the memory block.
     * @param swap
     *            true if the ints should be written in reverse byte order.
     * @throws NullPointerException
     *             if <code>bytes</code> is <code>null</code>.
     * @throws IndexOutOfBoundsException
     *             if <code>offset + length > bytes.length</code>.
     */
    public static native void setIntArray(int address, int[] ints, int offset,
            int length, boolean swap) throws NullPointerException,
            IndexOutOfBoundsException;

    public static native void setFloatArray(int address, float[] floats, int offset,
            int length, boolean swap) throws NullPointerException,
            IndexOutOfBoundsException;

    /**
     * Gets the value of the single byte at the given address.
     * <p>
     * The behavior is unspecified if <code>address</code> is not in the range
     * that was previously allocated using <code>malloc()</code>.
     * </p>
     *
     * @param address
     *            the platform address of the byte.
     * @return the byte value.
     */
    public static native byte getByte(int address);

    /**
     * Sets the given single byte value at the given address.
     * <p>
     * The behavior is unspecified if <code>address</code> is not in the range
     * that was previously allocated using <code>malloc()</code>.
     * </p>
     *
     * @param address
     *            the address at which to set the byte value.
     * @param value
     *            the value to set.
     */
    public static native void setByte(int address, byte value);

    /**
     * Gets the value of the signed two-byte integer stored in platform byte
     * order at the given address.
     * <p>
     * The behavior is unspecified if <code>(address ... address + 2)</code>
     * is not wholly within the range that was previously allocated using
     * <code>malloc()</code>.
     * </p>
     *
     * @param address
     *            the platform address of the start of the two-byte value.
     * @return the value of the two-byte integer as a Java <code>short</code>.
     */
    public static native short getShort(int address);

    public static short getShort(int address, ByteOrder byteOrder) {
        return (byteOrder == NATIVE_ORDER)
                ? getShort(address)
                : Short.reverseBytes(getShort(address));
    }

    /**
     * Sets the value of the signed two-byte integer at the given address in
     * platform byte order.
     * <p>
     * The behavior is unspecified if <code>(address ... address + 2)</code>
     * is not wholly within the range that was previously allocated using
     * <code>malloc()</code>.
     * </p>
     *
     * @param address
     *            the platform address of the start of the two-byte value.
     * @param value
     *            the value of the two-byte integer as a Java <code>short</code>.
     */
    public static native void setShort(int address, short value);

    public static void setShort(int address, short value, ByteOrder byteOrder) {
        if (byteOrder == NATIVE_ORDER) {
            setShort(address, value);
        } else {
            setShort(address, Short.reverseBytes(value));
        }
    }

    /**
     * Gets the value of the signed four-byte integer stored in platform
     * byte-order at the given address.
     * <p>
     * The behavior is unspecified if <code>(address ... address + 4)</code>
     * is not wholly within the range that was previously allocated using
     * <code>malloc()</code>.
     * </p>
     *
     * @param address
     *            the platform address of the start of the four-byte value.
     * @return the value of the four-byte integer as a Java <code>int</code>.
     */
    public static native int getInt(int address);

    public static int getInt(int address, ByteOrder byteOrder) {
        return (byteOrder == NATIVE_ORDER)
                ? getInt(address)
                : Integer.reverseBytes(getInt(address));
    }

    /**
     * Sets the value of the signed four-byte integer at the given address in
     * platform byte order.
     * <p>
     * The behavior is unspecified if <code>(address ... address + 4)</code>
     * is not wholly within the range that was previously allocated using
     * <code>malloc()</code>.
     * </p>
     *
     * @param address
     *            the platform address of the start of the four-byte value.
     * @param value
     *            the value of the four-byte integer as a Java <code>int</code>.
     */
    public static native void setInt(int address, int value);

    public static void setInt(int address, int value, ByteOrder byteOrder) {
        if (byteOrder == NATIVE_ORDER) {
            setInt(address, value);
        } else {
            setInt(address, Integer.reverseBytes(value));
        }
    }

    /**
     * Gets the value of the signed eight-byte integer stored in platform byte
     * order at the given address.
     * <p>
     * The behavior is unspecified if <code>(address ... address + 8)</code>
     * is not wholly within the range that was previously allocated using
     * <code>malloc()</code>.
     * </p>
     *
     * @param address
     *            the platform address of the start of the eight-byte value.
     * @return the value of the eight-byte integer as a Java <code>long</code>.
     */
    public static native long getLong(int address);

    public static long getLong(int address, ByteOrder byteOrder) {
        return (byteOrder == NATIVE_ORDER)
                ? getLong(address)
                : Long.reverseBytes(getLong(address));
    }

    /**
     * Sets the value of the signed eight-byte integer at the given address in
     * the platform byte order.
     * <p>
     * The behavior is unspecified if <code>(address ... address + 8)</code>
     * is not wholly within the range that was previously allocated using
     * <code>malloc()</code>.
     * </p>
     *
     * @param address
     *            the platform address of the start of the eight-byte value.
     * @param value
     *            the value of the eight-byte integer as a Java
     *            <code>long</code>.
     */
    public static native void setLong(int address, long value);

    public static void setLong(int address, long value, ByteOrder byteOrder) {
        if (byteOrder == NATIVE_ORDER) {
            setLong(address, value);
        } else {
            setLong(address, Long.reverseBytes(value));
        }
    }

    /**
     * Gets the value of the IEEE754-format four-byte float stored in platform
     * byte order at the given address.
     * <p>
     * The behavior is unspecified if <code>(address ... address + 4)</code>
     * is not wholly within the range that was previously allocated using
     * <code>malloc()</code>.
     * </p>
     *
     * @param address
     *            the platform address of the start of the eight-byte value.
     * @return the value of the four-byte float as a Java <code>float</code>.
     */
    public static native float getFloat(int address);

    public static float getFloat(int address, ByteOrder byteOrder) {
        if (byteOrder == NATIVE_ORDER) {
            return getFloat(address);
        }
        int floatBits = Integer.reverseBytes(getInt(address));
        return Float.intBitsToFloat(floatBits);
    }

    /**
     * Sets the value of the IEEE754-format four-byte float stored in platform
     * byte order at the given address.
     * <p>
     * The behavior is unspecified if <code>(address ... address + 4)</code>
     * is not wholly within the range that was previously allocated using
     * <code>malloc()</code>.
     * </p>
     *
     * @param address
     *            the platform address of the start of the eight-byte value.
     * @param value
     *            the value of the four-byte float as a Java <code>float</code>.
     */
    public static native void setFloat(int address, float value);

    public static void setFloat(int address, float value, ByteOrder byteOrder) {
        if (byteOrder == NATIVE_ORDER) {
            setFloat(address, value);
        } else {
            int floatBits = Float.floatToIntBits(value);
            setInt(address, Integer.reverseBytes(floatBits));
        }
    }

    /**
     * Gets the value of the IEEE754-format eight-byte float stored in platform
     * byte order at the given address.
     * <p>
     * The behavior is unspecified if <code>(address ... address + 8)</code>
     * is not wholly within the range that was previously allocated using
     * <code>malloc()</code>.
     * </p>
     *
     * @param address
     *            the platform address of the start of the eight-byte value.
     * @return the value of the eight-byte float as a Java <code>double</code>.
     */
    public static native double getDouble(int address);

    public static double getDouble(int address, ByteOrder byteOrder) {
        if (byteOrder == NATIVE_ORDER) {
            return getDouble(address);
        }
        long doubleBits = Long.reverseBytes(getLong(address));
        return Double.longBitsToDouble(doubleBits);
    }

    /**
     * Sets the value of the IEEE754-format eight-byte float store in platform
     * byte order at the given address.
     * <p>
     * The behavior is unspecified if <code>(address ... address + 8)</code>
     * is not wholly within the range that was previously allocated using
     * <code>malloc()</code>.
     * </p>
     *
     * @param address
     *            the platform address of the start of the eight-byte value.
     * @param value
     *            the value of the eight-byte float as a Java
     *            <code>double</code>.
     */
    public static native void setDouble(int address, double value);

    public static void setDouble(int address, double value, ByteOrder byteOrder) {
        if (byteOrder == NATIVE_ORDER) {
            setDouble(address, value);
        } else {
            long doubleBits = Double.doubleToLongBits(value);
            setLong(address, Long.reverseBytes(doubleBits));
        }
    }

    // "get uintptr_t"
    public static native int getAddress(int address);

    // "put uintptr_t"
    public static native void setAddress(int address, int value);

    private static native int mmapImpl(int fd, long offset, long size, int mapMode);

    public static int mmap(int fd, long offset, long size, MapMode mapMode) throws IOException {
        // Check just those errors mmap(2) won't detect.
        if (offset < 0 || size < 0 || offset > Integer.MAX_VALUE || size > Integer.MAX_VALUE) {
            throw new IllegalArgumentException("offset=" + offset + " size=" + size);
        }
        int intMode = 0; // MapMode.PRIVATE
        if (mapMode == MapMode.READ_ONLY) {
            intMode = 1;
        } else if (mapMode == MapMode.READ_WRITE) {
            intMode = 2;
        }
        return mmapImpl(fd, offset, size, intMode);
    }
    // END android-changed

    public static native void unmap(int addr, long size);

    public static native void load(int addr, long size);

    public static native boolean isLoaded(int addr, long size);

    public static native void flush(int addr, long size);
}
