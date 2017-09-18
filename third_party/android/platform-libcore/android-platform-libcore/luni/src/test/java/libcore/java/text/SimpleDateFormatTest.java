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

package libcore.java.text;

import java.text.ParsePosition;
import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.Date;
import java.util.Locale;
import java.util.TimeZone;

public class SimpleDateFormatTest extends junit.framework.TestCase {
    // The RI fails this test.
    public void test2DigitYearStartIsCloned() throws Exception {
        // Test that get2DigitYearStart returns a clone.
        SimpleDateFormat sdf = new SimpleDateFormat();
        Date originalDate = sdf.get2DigitYearStart();
        assertNotSame(sdf.get2DigitYearStart(), originalDate);
        assertEquals(sdf.get2DigitYearStart(), originalDate);
        originalDate.setTime(0);
        assertFalse(sdf.get2DigitYearStart().equals(originalDate));
        // Test that set2DigitYearStart takes a clone.
        Date newDate = new Date();
        sdf.set2DigitYearStart(newDate);
        assertNotSame(sdf.get2DigitYearStart(), newDate);
        assertEquals(sdf.get2DigitYearStart(), newDate);
        newDate.setTime(0);
        assertFalse(sdf.get2DigitYearStart().equals(newDate));
    }

    // The RI fails this test because this is an ICU-compatible Android extension.
    // Necessary for correct localization in various languages (http://b/2633414).
    public void testStandAloneNames() throws Exception {
        Locale en = Locale.ENGLISH;
        Locale pl = new Locale("pl");
        Locale ru = new Locale("ru");

        assertEquals("January", formatDate(en, "MMMM"));
        assertEquals("January", formatDate(en, "LLLL"));
        assertEquals("stycznia", formatDate(pl, "MMMM"));
        assertEquals("stycze\u0144", formatDate(pl, "LLLL"));

        assertEquals("Thursday", formatDate(en, "EEEE"));
        assertEquals("Thursday", formatDate(en, "cccc"));
        assertEquals("\u0447\u0435\u0442\u0432\u0435\u0440\u0433", formatDate(ru, "EEEE"));
        assertEquals("\u0427\u0435\u0442\u0432\u0435\u0440\u0433", formatDate(ru, "cccc"));

        assertEquals(Calendar.JUNE, parseDate(en, "yyyy-MMMM-dd", "1980-June-12").get(Calendar.MONTH));
        assertEquals(Calendar.JUNE, parseDate(en, "yyyy-LLLL-dd", "1980-June-12").get(Calendar.MONTH));
        assertEquals(Calendar.JUNE, parseDate(pl, "yyyy-MMMM-dd", "1980-czerwca-12").get(Calendar.MONTH));
        assertEquals(Calendar.JUNE, parseDate(pl, "yyyy-LLLL-dd", "1980-czerwiec-12").get(Calendar.MONTH));

        assertEquals(Calendar.TUESDAY, parseDate(en, "EEEE", "Tuesday").get(Calendar.DAY_OF_WEEK));
        assertEquals(Calendar.TUESDAY, parseDate(en, "cccc", "Tuesday").get(Calendar.DAY_OF_WEEK));
        assertEquals(Calendar.TUESDAY, parseDate(ru, "EEEE", "\u0432\u0442\u043e\u0440\u043d\u0438\u043a").get(Calendar.DAY_OF_WEEK));
        assertEquals(Calendar.TUESDAY, parseDate(ru, "cccc", "\u0412\u0442\u043e\u0440\u043d\u0438\u043a").get(Calendar.DAY_OF_WEEK));
    }

    private String formatDate(Locale l, String fmt) {
        return new SimpleDateFormat(fmt, l).format(new Date(0));
    }

    private Calendar parseDate(Locale l, String fmt, String value) {
        SimpleDateFormat sdf = new SimpleDateFormat(fmt, l);
        ParsePosition pp = new ParsePosition(0);
        Date d = sdf.parse(value, pp);
        if (d == null) {
            fail(pp.toString());
        }
        Calendar c = Calendar.getInstance(TimeZone.getTimeZone("UTC"));
        c.setTime(d);
        return c;
    }
}
