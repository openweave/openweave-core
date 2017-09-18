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

import junit.framework.TestCase;

public class SystemTest extends TestCase {

    public void testArrayCopyTargetNotArray() {
        try {
            System.arraycopy(new char[5], 0, "Hello", 0, 3);
            fail();
        } catch (ArrayStoreException e) {
            assertEquals("source and destination must be arrays, but were "
                    + "[C and Ljava/lang/String;", e.getMessage());
        }
    }

    public void testArrayCopySourceNotArray() {
        try {
            System.arraycopy("Hello", 0, new char[5], 0, 3);
            fail();
        } catch (ArrayStoreException e) {
            assertEquals("source and destination must be arrays, but were "
                    + "Ljava/lang/String; and [C", e.getMessage());
        }
    }

    public void testArrayCopyArrayTypeMismatch() {
        try {
            System.arraycopy(new char[5], 0, new Object[5], 0, 3);
            fail();
        } catch (ArrayStoreException e) {
            assertEquals("source and destination arrays are incompatible: "
                    + "[C and [Ljava/lang/Object;", e.getMessage());
        }
    }

    public void testArrayCopyElementTypeMismatch() {
        try {
            System.arraycopy(new Object[] { null, 5, "hello" }, 0,
                    new Integer[] { 1, 2, 3, null, null }, 0, 3);
            fail();
        } catch (ArrayStoreException e) {
            assertEquals("source[2] of type Ljava/lang/String; cannot be "
                    + "stored in destination array of type [Ljava/lang/Integer;",
                    e.getMessage());
        }
    }

    /**
     * http://b/issue?id=2136462
     */
    public void testBackFromTheDead() {
        try {
            new ConstructionFails();
        } catch (AssertionError expected) {
        }

        for (int i = 0; i < 20; i++) {
            if (ConstructionFails.INSTANCE != null) {
                fail("finalize() called, even though constructor failed!");
            }

            induceGc(i);
        }
    }

    private void induceGc(int rev) {
        System.gc();
        try {
            byte[] b = new byte[1024 << rev];
        } catch (OutOfMemoryError e) {
        }
    }

    static class ConstructionFails {
        private static ConstructionFails INSTANCE;

        ConstructionFails() {
            throw new AssertionError();
        }

        @Override protected void finalize() throws Throwable {
            INSTANCE = this;
            new AssertionError("finalize() called, even though constructor failed!")
                    .printStackTrace();
        }
    }
}
