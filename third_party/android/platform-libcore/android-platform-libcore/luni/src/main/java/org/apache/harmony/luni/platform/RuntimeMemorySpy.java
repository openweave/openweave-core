/* Licensed to the Apache Software Foundation (ASF) under one or more
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

import java.lang.ref.PhantomReference;
import java.lang.ref.Reference;
import java.lang.ref.ReferenceQueue;
import java.util.HashMap;
import java.util.Map;

final class RuntimeMemorySpy {

    final class AddressWrapper {
        final PlatformAddress shadow;

        final PhantomReference<PlatformAddress> wrAddress;

        volatile boolean autoFree = false;

        AddressWrapper(PlatformAddress address) {
            this.shadow = address.duplicate();
            this.wrAddress = new PhantomReference<PlatformAddress>(address, notifyQueue);
        }
    }

    // TODO: figure out how to prevent this being a synchronization bottleneck
    private final Map<PlatformAddress, AddressWrapper> memoryInUse = new HashMap<PlatformAddress, AddressWrapper>(); // Shadow to Wrapper

    private final Map<Reference, PlatformAddress> refToShadow = new HashMap<Reference, PlatformAddress>(); // Reference to Shadow

    private final ReferenceQueue<Object> notifyQueue = new ReferenceQueue<Object>();

    public RuntimeMemorySpy() {
    }

    public void alloc(PlatformAddress address) {
        // Pay a tax on the allocation to see if there are any frees pending.
        Reference ref = notifyQueue.poll(); // non-blocking check
        while (ref != null) {
            orphanedMemory(ref);
            ref = notifyQueue.poll();
        }

        AddressWrapper wrapper = new AddressWrapper(address);
        synchronized (this) {
            memoryInUse.put(wrapper.shadow, wrapper);
            refToShadow.put(wrapper.wrAddress, wrapper.shadow);
        }
    }

    // Has a veto: true == do free,false = don't
    public boolean free(PlatformAddress address) {
        AddressWrapper wrapper;
        synchronized (this) {
            wrapper = memoryInUse.remove(address);
            if (wrapper != null) {
                refToShadow.remove(wrapper.wrAddress);
            }
        }
        if (wrapper == null) {
            // Attempt to free memory we didn't alloc
            System.err.println("Memory Spy! Fixed attempt to free memory that was not allocated " + address);
        }
        return wrapper != null;
    }

    public void rangeCheck(PlatformAddress address, int offset, int length) throws IndexOutOfBoundsException {
        // Do nothing
    }

    /**
     * Requests that the given address is freed automatically when it becomes
     * garbage. If the address is already freed, or has not been notified as
     * allocated via this memory spy, then this call has no effect and completes
     * quietly.
     *
     * @param address
     *            the address to be freed.
     */
    public void autoFree(PlatformAddress address) {
        AddressWrapper wrapper;
        synchronized (this) {
            wrapper = memoryInUse.get(address);
        }
        if (wrapper != null) {
            wrapper.autoFree = true;
        }
    }

    private void orphanedMemory(Reference ref) {
        synchronized (this) {
            PlatformAddress shadow = refToShadow.remove(ref);
            AddressWrapper wrapper = memoryInUse.get(shadow);
            if (wrapper != null) {
                // There is a leak if we were not auto-freeing this memory.
                if (!wrapper.autoFree) {
                    System.err.println("Memory Spy! Fixed memory leak by freeing " + wrapper.shadow);
                }
                wrapper.shadow.free();
            }
        }
        ref.clear();
    }
}
