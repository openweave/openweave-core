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

package libcore.java.util;

import dalvik.annotation.AndroidOnly;
import dalvik.annotation.TestLevel;
import dalvik.annotation.TestTargetClass;
import dalvik.annotation.TestTargetNew;
import java.util.Date;
import java.util.Locale;
import java.util.TimeZone;
import junit.framework.TestCase;
import tests.support.Support_Locale;

@TestTargetClass(TimeZone.class)
public class OldTimeZoneTest extends TestCase {

    class Mock_TimeZone extends TimeZone {
        @Override
        public int getOffset(int era, int year, int month, int day, int dayOfWeek, int milliseconds) {
            return 0;
        }

        @Override
        public int getRawOffset() {
            return 0;
        }

        @Override
        public boolean inDaylightTime(Date date) {
            return false;
        }

        @Override
        public void setRawOffset(int offsetMillis) {

        }

        @Override
        public boolean useDaylightTime() {
            return false;
        }
    }

    @TestTargetNew(
        level = TestLevel.COMPLETE,
        notes = "",
        method = "TimeZone",
        args = {}
    )
    public void test_constructor() {
        assertNotNull(new Mock_TimeZone());
    }

    @TestTargetNew(
        level = TestLevel.COMPLETE,
        notes = "",
        method = "clone",
        args = {}
    )
    public void test_clone() {
        TimeZone tz1 = TimeZone.getDefault();
        TimeZone tz2 = (TimeZone)tz1.clone();

        assertTrue(tz1.equals(tz2));
    }

    @TestTargetNew(
        level = TestLevel.COMPLETE,
        notes = "",
        method = "getAvailableIDs",
        args = {}
    )
    public void test_getAvailableIDs() {
        String[] str = TimeZone.getAvailableIDs();
        assertNotNull(str);
        assertTrue(str.length != 0);
        for(int i = 0; i < str.length; i++) {
            assertNotNull(TimeZone.getTimeZone(str[i]));
        }
    }

    @TestTargetNew(
        level = TestLevel.COMPLETE,
        notes = "",
        method = "getAvailableIDs",
        args = {int.class}
    )
    public void test_getAvailableIDsI() {
        String[] str = TimeZone.getAvailableIDs(0);
        assertNotNull(str);
        assertTrue(str.length != 0);
        for(int i = 0; i < str.length; i++) {
            assertNotNull(TimeZone.getTimeZone(str[i]));
        }
    }

    @TestTargetNew(
        level = TestLevel.COMPLETE,
        notes = "",
        method = "getDisplayName",
        args = {}
    )
    public void test_getDisplayName() {
        TimeZone tz = TimeZone.getTimeZone("GMT-6");
        assertEquals("GMT-06:00", tz.getDisplayName());
        tz = TimeZone.getTimeZone("America/Los_Angeles");
        assertEquals("Pacific Standard Time", tz.getDisplayName());
    }

    @TestTargetNew(
        level = TestLevel.COMPLETE,
        notes = "",
        method = "getDisplayName",
        args = {java.util.Locale.class}
    )
    public void test_getDisplayNameLjava_util_Locale() {
        Locale[] requiredLocales = {Locale.US, Locale.FRANCE};
        if (!Support_Locale.areLocalesAvailable(requiredLocales)) {
            // locale dependent test, bug 1943269
            return;
        }
        TimeZone tz = TimeZone.getTimeZone("America/Los_Angeles");
        assertEquals("Pacific Standard Time", tz.getDisplayName(new Locale("US")));
        // BEGIN android-note: RI has "Heure", CLDR/ICU has "heure".
        assertEquals("heure normale du Pacifique", tz.getDisplayName(Locale.FRANCE));
    }

    @TestTargetNew(
        level = TestLevel.COMPLETE,
        notes = "",
        method = "getDisplayName",
        args = {boolean.class, int.class}
    )
    public void test_getDisplayNameZI() {
        TimeZone tz = TimeZone.getTimeZone("America/Los_Angeles");
        assertEquals("PST",                   tz.getDisplayName(false, 0));
        assertEquals("Pacific Daylight Time", tz.getDisplayName(true, 1));
        assertEquals("Pacific Standard Time", tz.getDisplayName(false, 1));
    }

    @TestTargetNew(
        level = TestLevel.COMPLETE,
        notes = "",
        method = "getDisplayName",
        args = {boolean.class, int.class, java.util.Locale.class}
    )
    @AndroidOnly("fail on RI. See comment below")
    public void test_getDisplayNameZILjava_util_Locale() {
        Locale[] requiredLocales = {Locale.US, Locale.UK, Locale.FRANCE};
        if (!Support_Locale.areLocalesAvailable(requiredLocales)) {
            // locale dependent test, bug 1943269
            return;
        }
        TimeZone tz = TimeZone.getTimeZone("America/Los_Angeles");
        assertEquals("PST",                   tz.getDisplayName(false, 0, Locale.US));
        assertEquals("Pacific Daylight Time", tz.getDisplayName(true,  1, Locale.US));
        assertEquals("Pacific Standard Time", tz.getDisplayName(false, 1, Locale.UK));
        //RI fails on following line. RI always returns short time zone name as "PST"
        assertEquals("UTC-08:00",             tz.getDisplayName(false, 0, Locale.FRANCE));
        // BEGIN android-note: RI has "Heure", CLDR/ICU has "heure".
        assertEquals("heure avanc\u00e9e du Pacifique", tz.getDisplayName(true,  1, Locale.FRANCE));
        assertEquals("heure normale du Pacifique", tz.getDisplayName(false, 1, Locale.FRANCE));
    }

    @TestTargetNew(
        level = TestLevel.COMPLETE,
        notes = "",
        method = "getID",
        args = {}
    )
    public void test_getID() {
        TimeZone tz = TimeZone.getTimeZone("GMT-6");
        assertEquals("GMT-06:00", tz.getID());
        tz = TimeZone.getTimeZone("America/Denver");
        assertEquals("America/Denver", tz.getID());
    }

    @TestTargetNew(
        level = TestLevel.COMPLETE,
        notes = "",
        method = "hasSameRules",
        args = {java.util.TimeZone.class}
    )
    public void test_hasSameRulesLjava_util_TimeZone() {
        TimeZone tz1 = TimeZone.getTimeZone("America/Denver");
        TimeZone tz2 = TimeZone.getTimeZone("America/Phoenix");
        assertEquals(tz1.getDisplayName(false, 0), tz2.getDisplayName(false, 0));
        // Arizona doesn't observe DST. See http://phoenix.about.com/cs/weather/qt/timezone.htm
        assertFalse(tz1.hasSameRules(tz2));
        assertFalse(tz1.hasSameRules(null));
        tz1 = TimeZone.getTimeZone("America/Montreal");
        tz2 = TimeZone.getTimeZone("America/New_York");
        assertEquals(tz1.getDisplayName(), tz2.getDisplayName());
        assertFalse(tz1.getID().equals(tz2.getID()));
        assertTrue(tz2.hasSameRules(tz1));
        assertTrue(tz1.hasSameRules(tz1));
    }

    @TestTargetNew(
        level = TestLevel.COMPLETE,
        notes = "",
        method = "setID",
        args = {java.lang.String.class}
    )
    public void test_setIDLjava_lang_String() {
        TimeZone tz = TimeZone.getTimeZone("GMT-6");
        assertEquals("GMT-06:00", tz.getID());
        tz.setID("New ID for GMT-6");
        assertEquals("New ID for GMT-6", tz.getID());
    }

    Locale loc = null;

    protected void setUp() {
        loc = Locale.getDefault();
        Locale.setDefault(Locale.US);
    }

    protected void tearDown() {
        Locale.setDefault(loc);
    }
}
