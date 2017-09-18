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

import java.nio.ByteOrder;

/**
 * The platform address class is an unsafe virtualization of an OS memory block.
 */
public class PlatformAddress implements Comparable {
    public static final int SIZEOF_JBYTE = 1;
    public static final int SIZEOF_JSHORT = 2;
    public static final int SIZEOF_JINT = 4;
    public static final int SIZEOF_JSIZE = 4;
    public static final int SIZEOF_JFLOAT = 4;
    public static final int SIZEOF_JLONG = 8;
    public static final int SIZEOF_JDOUBLE = 8;

    /**
     * This final field defines the sentinel for an unknown address value.
     */
    static final int UNKNOWN = -1;

    /**
     * NULL is the canonical address with address value zero.
     */
    public static final PlatformAddress NULL = new PlatformAddress(0, 0);

    /**
     * INVALID is the canonical address with an invalid value
     * (i.e. a non-address).
     */
    public static final PlatformAddress INVALID = new PlatformAddress(UNKNOWN, UNKNOWN);

    public static final RuntimeMemorySpy memorySpy = new RuntimeMemorySpy();

    final int osaddr;

    final long size;

    PlatformAddress(int address, long size) {
        super();
        osaddr = address;
        this.size = size;
    }

    /**
     * Sending auto free to an address means that, when this subsystem has
     * allocated the memory, it will automatically be freed when this object is
     * collected by the garbage collector if the memory has not already been
     * freed explicitly.
     *
     */
    public final void autoFree() {
        memorySpy.autoFree(this);
    }

    public PlatformAddress duplicate() {
        return PlatformAddressFactory.on(osaddr, size);
    }

    public PlatformAddress offsetBytes(int offset) {
        return PlatformAddressFactory.on(osaddr + offset, size - offset);
    }

    public final void moveTo(PlatformAddress dst, long numBytes) {
        OSMemory.memmove(dst.osaddr, osaddr, numBytes);
    }

    public final boolean equals(Object other) {
        return (other instanceof PlatformAddress)
                && (((PlatformAddress) other).osaddr == osaddr);
    }

    public final int hashCode() {
        return (int) osaddr;
    }

    public final boolean isNULL() {
        return this == NULL;
    }

    public void free() {
        // Memory spys can veto the basic free if they determine the memory was
        // not allocated.
        if (memorySpy.free(this)) {
            OSMemory.free(osaddr);
        }
    }

    public final void setAddress(int offset, PlatformAddress address) {
        OSMemory.setAddress(osaddr + offset, address.osaddr);
    }

    public final PlatformAddress getAddress(int offset) {
        int addr = getInt(offset);
        return PlatformAddressFactory.on(addr);
    }

    public final void setByte(int offset, byte value) {
        memorySpy.rangeCheck(this, offset, SIZEOF_JBYTE);
        OSMemory.setByte(osaddr + offset, value);
    }

    public final void setByteArray(int offset, byte[] bytes, int bytesOffset,
            int length) {
        memorySpy.rangeCheck(this, offset, length * SIZEOF_JBYTE);
        OSMemory.setByteArray(osaddr + offset, bytes, bytesOffset, length);
    }

    // BEGIN android-added
    public final void setShortArray(int offset, short[] shorts,
            int shortsOffset, int length, boolean swap) {
        memorySpy.rangeCheck(this, offset, length * SIZEOF_JSHORT);
        OSMemory.setShortArray(osaddr + offset, shorts, shortsOffset, length,
            swap);
    }

    public final void setIntArray(int offset, int[] ints,
            int intsOffset, int length, boolean swap) {
        memorySpy.rangeCheck(this, offset, length * SIZEOF_JINT);
        OSMemory.setIntArray(osaddr + offset, ints, intsOffset, length, swap);
    }

    public final void setFloatArray(int offset, float[] floats,
            int floatsOffset, int length, boolean swap) {
        memorySpy.rangeCheck(this, offset, length * SIZEOF_JFLOAT);
        OSMemory.setFloatArray(
                osaddr + offset, floats, floatsOffset, length, swap);
    }
    // END android-added

    public final byte getByte(int offset) {
        memorySpy.rangeCheck(this, offset, SIZEOF_JBYTE);
        return OSMemory.getByte(osaddr + offset);
    }

    public final void getByteArray(int offset, byte[] bytes, int bytesOffset,
            int length) {
        memorySpy.rangeCheck(this, offset, length * SIZEOF_JBYTE);
        OSMemory.getByteArray(osaddr + offset, bytes, bytesOffset, length);
    }

    public final void setShort(int offset, short value, ByteOrder order) {
        memorySpy.rangeCheck(this, offset, SIZEOF_JSHORT);
        OSMemory.setShort(osaddr + offset, value, order);
    }

    public final void setShort(int offset, short value) {
        memorySpy.rangeCheck(this, offset, SIZEOF_JSHORT);
        OSMemory.setShort(osaddr + offset, value);
    }

    public final short getShort(int offset, ByteOrder order) {
        memorySpy.rangeCheck(this, offset, SIZEOF_JSHORT);
        return OSMemory.getShort(osaddr + offset, order);
    }

    public final short getShort(int offset) {
        memorySpy.rangeCheck(this, offset, SIZEOF_JSHORT);
        return OSMemory.getShort(osaddr + offset);
    }

    public final void setInt(int offset, int value, ByteOrder order) {
        memorySpy.rangeCheck(this, offset, SIZEOF_JINT);
        OSMemory.setInt(osaddr + offset, value, order);
    }

    public final void setInt(int offset, int value) {
        memorySpy.rangeCheck(this, offset, SIZEOF_JINT);
        OSMemory.setInt(osaddr + offset, value);
    }

    public final int getInt(int offset, ByteOrder order) {
        memorySpy.rangeCheck(this, offset, SIZEOF_JINT);
        return OSMemory.getInt(osaddr + offset, order);
    }

    public final int getInt(int offset) {
        memorySpy.rangeCheck(this, offset, SIZEOF_JINT);
        return OSMemory.getInt(osaddr + offset);
    }

    public final void setLong(int offset, long value, ByteOrder order) {
        memorySpy.rangeCheck(this, offset, SIZEOF_JLONG);
        OSMemory.setLong(osaddr + offset, value, order);
    }

    public final void setLong(int offset, long value) {
        memorySpy.rangeCheck(this, offset, SIZEOF_JLONG);
        OSMemory.setLong(osaddr + offset, value);
    }

    public final long getLong(int offset, ByteOrder order) {
        memorySpy.rangeCheck(this, offset, SIZEOF_JLONG);
        return OSMemory.getLong(osaddr + offset, order);
    }

    public final long getLong(int offset) {
        memorySpy.rangeCheck(this, offset, SIZEOF_JLONG);
        return OSMemory.getLong(osaddr + offset);
    }

    public final void setFloat(int offset, float value, ByteOrder order) {
        memorySpy.rangeCheck(this, offset, SIZEOF_JFLOAT);
        OSMemory.setFloat(osaddr + offset, value, order);
    }

    public final void setFloat(int offset, float value) {
        memorySpy.rangeCheck(this, offset, SIZEOF_JFLOAT);
        OSMemory.setFloat(osaddr + offset, value);
    }

    public final float getFloat(int offset, ByteOrder order) {
        memorySpy.rangeCheck(this, offset, SIZEOF_JFLOAT);
        return OSMemory.getFloat(osaddr + offset, order);
    }

    public final float getFloat(int offset) {
        memorySpy.rangeCheck(this, offset, SIZEOF_JFLOAT);
        return OSMemory.getFloat(osaddr + offset);
    }

    public final void setDouble(int offset, double value, ByteOrder order) {
        memorySpy.rangeCheck(this, offset, SIZEOF_JDOUBLE);
        OSMemory.setDouble(osaddr + offset, value, order);
    }

    public final void setDouble(int offset, double value) {
        memorySpy.rangeCheck(this, offset, SIZEOF_JDOUBLE);
        OSMemory.setDouble(osaddr + offset, value);
    }

    public final double getDouble(int offset, ByteOrder order) {
        memorySpy.rangeCheck(this, offset, SIZEOF_JDOUBLE);
        return OSMemory.getDouble(osaddr + offset, order);
    }

    public final double getDouble(int offset) {
        memorySpy.rangeCheck(this, offset, SIZEOF_JDOUBLE);
        return OSMemory.getDouble(osaddr + offset);
    }

    // BEGIN android-added
    public final int toInt() {
        return osaddr;
    }
    // END android-added

    public final long toLong() {
        return osaddr;
    }

    public final String toString() {
        return "PlatformAddress[" + osaddr + "]";
    }

    public final long getSize() {
        return size;
    }

    public final int compareTo(Object other) {
        if (other == null) {
            throw new NullPointerException(); // per spec.
        }
        if (other instanceof PlatformAddress) {
            int otherPA = ((PlatformAddress) other).osaddr;
            if (osaddr == otherPA) {
                return 0;
            }
            return osaddr < otherPA ? -1 : 1;
        }

        throw new ClassCastException(); // per spec.
    }
}
