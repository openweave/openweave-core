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

package libcore.icu;

import java.util.HashMap;
import java.util.Locale;
import java.util.TimeZone;
import java.util.logging.Logger;

/**
 * Provides access to ICU's time zone data.
 */
public final class TimeZones {
    private static final String[] availableTimeZones = TimeZone.getAvailableIDs();

    private TimeZones() {}

    /**
     * Implements TimeZone.getDisplayName by asking ICU.
     */
    public static String getDisplayName(String id, boolean daylight, int style, Locale locale) {
        // If we already have the strings, linear search through them is 10x quicker than
        // calling ICU for just the one we want.
        if (CachedTimeZones.locale.equals(locale)) {
            String result = lookupDisplayName(CachedTimeZones.names, id, daylight, style);
            if (result != null) {
                return result;
            }
        }
        return getDisplayNameImpl(id, daylight, style, locale.toString());
    }

    public static String lookupDisplayName(String[][] zoneStrings, String id, boolean daylight, int style) {
        for (String[] row : zoneStrings) {
            if (row[0].equals(id)) {
                if (daylight) {
                    return (style == TimeZone.LONG) ? row[3] : row[4];
                } else {
                    return (style == TimeZone.LONG) ? row[1] : row[2];
                }
            }
        }
        return null;
    }

    /**
     * Initialization holder for default time zone names. This class will
     * be preloaded by the zygote to share the time and space costs of setting
     * up the list of time zone names, so although it looks like the lazy
     * initialization idiom, it's actually the opposite.
     */
    private static class CachedTimeZones {
        /**
         * Name of default locale at the time this class was initialized.
         */
        private static final Locale locale = Locale.getDefault();

        /**
         * Names of time zones for the default locale.
         */
        private static final String[][] names = createZoneStringsFor(locale);
    }

    /**
     * Creates array of time zone names for the given locale.
     * This method takes about 2s to run on a 400MHz ARM11.
     */
    private static String[][] createZoneStringsFor(Locale locale) {
        long start = System.currentTimeMillis();

        /*
         * The following code is optimized for fast native response (the time a
         * method call can be in native code is limited). It prepares an empty
         * array to keep native code from having to create new Java objects. It
         * also fill in the time zone IDs to speed things up a bit. There's one
         * array for each time zone name type. (standard/long, standard/short,
         * daylight/long, daylight/short) The native method that fetches these
         * strings is faster if it can do all entries of one type, before having
         * to change to the next type. That's why the array passed down to
         * native has 5 entries, each providing space for all time zone names of
         * one type. Likely this access to the fields is much faster in the
         * native code because there's less array access overhead.
         */
        String[][] arrayToFill = new String[5][];
        arrayToFill[0] = availableTimeZones.clone();
        arrayToFill[1] = new String[availableTimeZones.length];
        arrayToFill[2] = new String[availableTimeZones.length];
        arrayToFill[3] = new String[availableTimeZones.length];
        arrayToFill[4] = new String[availableTimeZones.length];

        // Don't be distracted by all the code either side of this line: this is the expensive bit!
        getZoneStringsImpl(arrayToFill, locale.toString());

        // Reorder the entries so we get the expected result.
        // We also take the opportunity to de-duplicate the names (http://b/2672057).
        HashMap<String, String> internTable = new HashMap<String, String>();
        String[][] result = new String[availableTimeZones.length][5];
        for (int i = 0; i < availableTimeZones.length; ++i) {
            result[i][0] = arrayToFill[0][i];
            for (int j = 1; j <= 4; ++j) {
                String original = arrayToFill[j][i];
                String nonDuplicate = internTable.get(original);
                if (nonDuplicate == null) {
                    internTable.put(original, original);
                    nonDuplicate = original;
                }
                result[i][j] = nonDuplicate;
            }
        }

        long duration = System.currentTimeMillis() - start;
        Logger.global.info("Loaded time zone names for " + locale + " in " + duration + "ms.");

        return result;
    }

    /**
     * Returns an array of time zone strings, as used by DateFormatSymbols.getZoneStrings.
     */
    public static String[][] getZoneStrings(Locale locale) {
        if (locale == null) {
            locale = Locale.getDefault();
        }

        // TODO: We should force a reboot if the default locale changes.
        if (CachedTimeZones.locale.equals(locale)) {
            return clone2dStringArray(CachedTimeZones.names);
        }

        return createZoneStringsFor(locale);
    }

    public static String[][] clone2dStringArray(String[][] array) {
        String[][] result = new String[array.length][];
        for (int i = 0; i < array.length; ++i) {
            result[i] = array[i].clone();
        }
        return result;
    }

    /**
     * Returns an array containing the time zone ids in use in the country corresponding to
     * the given locale. This is not necessary for Java API, but is used by telephony as a
     * fallback.
     */
    public static String[] forLocale(Locale locale) {
        return forCountryCode(locale.getCountry());
    }

    private static native String[] forCountryCode(String countryCode);
    private static native void getZoneStringsImpl(String[][] arrayToFill, String locale);
    private static native String getDisplayNameImpl(String id, boolean isDST, int style, String locale);
}
