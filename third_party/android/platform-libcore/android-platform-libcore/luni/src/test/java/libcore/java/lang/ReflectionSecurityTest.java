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

package libcore.java.lang;

import java.lang.reflect.Member;
import java.security.Permission;
import java.util.ArrayList;
import java.util.Arrays;

public class ReflectionSecurityTest extends junit.framework.TestCase {
    private Enforcer enforcer = new Enforcer();

    private static class Enforcer extends SecurityManager {
        /** whether to deny the next request */
        private boolean deny;

        public ArrayList<String> actions = new ArrayList<String>();

        private Enforcer() {
            deny = false;
        }

        /**
         * Deny the next request.
         */
        public void denyNext() {
            deny = true;
        }

        /**
         * Throw an exception if the instance had been asked to deny a request.
         */
        private void denyIfAppropriate() {
            if (deny) {
                deny = false;
                throw new SecurityException("Denied!");
            }
        }

        @Override public void checkPackageAccess(String pkg) {
            actions.add("checkPackageAccess: " + pkg);
            denyIfAppropriate();
            super.checkPackageAccess(pkg);
        }

        @Override public void checkMemberAccess(Class c, int which) {
            String member;

            switch (which) {
            case Member.DECLARED: member = "DECLARED"; break;
            case Member.PUBLIC:   member = "PUBLIC";   break;
            default:              member = "<" + which + ">?"; break;
            }

            actions.add("checkMemberAccess: " + c.getName() + ", " + member);
            denyIfAppropriate();
            super.checkMemberAccess(c, which);
        }

        @Override public void checkPermission(Permission perm) {
            actions.add("checkPermission: " + perm);
            denyIfAppropriate();
            super.checkPermission(perm);
        }

        @Override public void checkPermission(Permission perm, Object context) {
            actions.add("checkPermission: " + perm + ", " + context);
            denyIfAppropriate();
            super.checkPermission(perm, context);
        }
    }

    private static class Blort {
        private int privateField;
        /*package*/ int packageField;
        protected int protectedField;
        public int publicField;

        private void privateMethod() {
            // This space intentionally left blank.
        }

        /*package*/ void packageMethod() {
            // This space intentionally left blank.
        }

        protected void protectedMethod() {
            // This space intentionally left blank.
        }

        public void publicMethod() {
            // This space intentionally left blank.
        }
    }

    public void testReflectionSecurity() throws Exception {
        System.setSecurityManager(enforcer);

        Class c = Blort.class;

        /*
         * Note: We don't use reflection to factor out these tests,
         * becuase reflection also calls into the SecurityManager, and
         * we don't want to conflate the calls nor assume too much
         * in general about what calls reflection will cause.
         */

        String blortPublic = "checkMemberAccess: libcore.java.lang.ReflectionSecurityTest$Blort, PUBLIC";
        String blortDeclared =
                "checkMemberAccess: libcore.java.lang.ReflectionSecurityTest$Blort, DECLARED";
        String objectDeclared = "checkMemberAccess: java.lang.Object, DECLARED";

        enforcer.actions.clear();
        c.getFields();
        assertEquals(Arrays.asList(blortPublic), enforcer.actions);

        enforcer.denyNext();
        try {
            c.getFields();
            fail();
        } catch (SecurityException expected) {
        }

        enforcer.actions.clear();
        c.getDeclaredFields();
        assertEquals(Arrays.asList(blortDeclared), enforcer.actions);

        enforcer.denyNext();
        try {
            c.getDeclaredFields();
            fail();
        } catch (SecurityException expected) {
        }

        enforcer.actions.clear();
        c.getMethods();
        assertEquals(Arrays.asList(blortPublic), enforcer.actions);

        enforcer.denyNext();
        try {
            c.getMethods();
            fail();
        } catch (SecurityException expected) {
        }

        enforcer.actions.clear();
        c.getDeclaredMethods();
        assertEquals(Arrays.asList(blortDeclared), enforcer.actions);

        enforcer.denyNext();
        try {
            c.getDeclaredMethods();
            fail();
        } catch (SecurityException expected) {
        }

        enforcer.actions.clear();
        c.getConstructors();
        assertEquals(Arrays.asList(blortPublic), enforcer.actions);

        enforcer.denyNext();
        try {
            c.getConstructors();
            fail();
        } catch (SecurityException expected) {
        }

        enforcer.actions.clear();
        c.getDeclaredConstructors();
        assertEquals(Arrays.asList(blortDeclared), enforcer.actions);

        enforcer.denyNext();
        try {
            c.getDeclaredConstructors();
            fail();
        } catch (SecurityException expected) {
        }

        enforcer.actions.clear();
        c.getClasses();
        assertEquals(Arrays.asList(blortPublic, blortDeclared, objectDeclared), enforcer.actions);

        enforcer.denyNext();
        try {
            c.getClasses();
            fail();
        } catch (SecurityException expected) {
        }

        enforcer.actions.clear();
        c.getDeclaredClasses();
        assertEquals(Arrays.asList(blortDeclared), enforcer.actions);

        enforcer.denyNext();
        try {
            c.getDeclaredClasses();
            fail();
        } catch (SecurityException expected) {
        }
    }
}
