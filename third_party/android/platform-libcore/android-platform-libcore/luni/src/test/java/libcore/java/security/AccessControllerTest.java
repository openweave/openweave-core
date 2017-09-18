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

package libcore.java.security;

import java.security.AccessControlContext;
import java.security.AccessControlException;
import java.security.AccessController;
import java.security.DomainCombiner;
import java.security.Permission;
import java.security.Permissions;
import java.security.PrivilegedAction;
import java.security.ProtectionDomain;
import junit.framework.TestCase;

public final class AccessControllerTest extends TestCase {

    public void testDoPrivilegedWithCombiner() {
        final Permission permission = new RuntimePermission("do stuff");
        final DomainCombiner union = new DomainCombiner() {
            public ProtectionDomain[] combine(ProtectionDomain[] a, ProtectionDomain[] b) {
                a = (a == null) ? new ProtectionDomain[0] : a;
                b = (b == null) ? new ProtectionDomain[0] : b;
                ProtectionDomain[] union = new ProtectionDomain[a.length + b.length];
                System.arraycopy(a, 0, union, 0, a.length);
                System.arraycopy(b, 0, union, a.length, b.length);
                return union;
            }
        };

        ProtectionDomain protectionDomain = new ProtectionDomain(null, new Permissions());
        AccessControlContext accessControlContext = new AccessControlContext(
                new AccessControlContext(new ProtectionDomain[] { protectionDomain }), union);

        AccessController.doPrivileged(new PrivilegedAction<Void>() {
            public Void run() {
                // in this block we lack our requested permission
                assertSame(union, AccessController.getContext().getDomainCombiner());
                assertPermission(false, permission);

                AccessController.doPrivileged(new PrivilegedAction<Void>() {
                    public Void run() {
                        // nest doPrivileged to get it back.
                        assertNull(AccessController.getContext().getDomainCombiner());
                        assertPermission(true, permission);
                        return null;
                    }
                });

                AccessController.doPrivilegedWithCombiner(new PrivilegedAction<Void>() {
                    public Void run() {
                        // nest doPrivilegedWithCombiner() to get it back and keep the combiner.
                        assertSame(union, AccessController.getContext().getDomainCombiner());
                        assertPermission(true, permission);
                        return null;
                    }
                });

                return null;
            }
        }, accessControlContext);
    }

    private void assertPermission(boolean granted, Permission permission) {
        if (granted) {
            AccessController.getContext().checkPermission(permission);
        } else {
            try {
                AccessController.getContext().checkPermission(permission);
                fail("Had unexpected permission: " + permission);
            } catch (AccessControlException expected) {
            }
        }
    }
}
