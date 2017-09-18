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

public class CharacterTest extends junit.framework.TestCase {
    public void test_valueOfC() {
        // The JLS requires caching for chars between "\u0000 to \u007f":
        // http://java.sun.com/docs/books/jls/third_edition/html/conversions.html#5.1.7
        // Harmony caches 0-512 and tests for this behavior, so we suppress that test and use this.
        for (char c = '\u0000'; c <= '\u007f'; ++c) {
            Character e = new Character(c);
            Character a = Character.valueOf(c);
            assertEquals(e, a);
            assertSame(Character.valueOf(c), Character.valueOf(c));
        }
        for (int c = '\u0080'; c <= Character.MAX_VALUE; ++c) {
            assertEquals(new Character((char) c), Character.valueOf((char) c));
        }
    }
}
