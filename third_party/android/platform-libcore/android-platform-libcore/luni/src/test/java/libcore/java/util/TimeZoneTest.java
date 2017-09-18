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

package libcore.java.util;

import java.util.Date;
import java.util.TimeZone;
import java.util.SimpleTimeZone;

public class TimeZoneTest extends junit.framework.TestCase {
    // http://code.google.com/p/android/issues/detail?id=877
    public void test_useDaylightTime_Taiwan() {
        TimeZone asiaTaipei = TimeZone.getTimeZone("Asia/Taipei");
        assertFalse("Taiwan doesn't use DST", asiaTaipei.useDaylightTime());
    }

    // http://code.google.com/p/android/issues/detail?id=8016
    public void test_useDaylightTime_Iceland() {
        TimeZone atlanticReykjavik = TimeZone.getTimeZone("Atlantic/Reykjavik");
        assertFalse("Reykjavik doesn't use DST", atlanticReykjavik.useDaylightTime());
    }

    // http://code.google.com/p/android/issues/detail?id=11542
    public void test_clone_SimpleTimeZone() {
        SimpleTimeZone stz = new SimpleTimeZone(21600000, "Central Standard Time");
        stz.setStartYear(1000);
        stz.inDaylightTime(new Date());
        stz.clone();
    }
}
