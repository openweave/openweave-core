/*
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */

package libcore.java.text;

import java.text.BreakIterator;
import java.util.Locale;
import junit.framework.TestCase;

public class OldBreakIteratorTest extends TestCase {

    BreakIterator iterator;

    protected void setUp() throws Exception {
        super.setUp();
        iterator = BreakIterator.getCharacterInstance(Locale.US);
    }

    public void testGetAvailableLocales() {
        Locale[] locales = BreakIterator.getAvailableLocales();
        assertTrue("Array available locales is null", locales != null);
        assertTrue("Array available locales is 0-length",
                (locales != null && locales.length != 0));
        boolean found = false;
        for (Locale l : locales) {
            if (l.equals(Locale.US)) {
                // expected
                found = true;
            }
        }
        assertTrue("At least locale " + Locale.US + " must be presented", found);
    }

    public void testGetWordInstanceLocale() {
        BreakIterator it1 = BreakIterator.getWordInstance(Locale.CANADA_FRENCH);
        assertTrue("Incorrect BreakIterator", it1 != BreakIterator.getWordInstance());
        BreakIterator it2 = BreakIterator.getWordInstance(new Locale("bad locale"));
        assertTrue("Incorrect BreakIterator", it2 != BreakIterator.getWordInstance());
    }
}
