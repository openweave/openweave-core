/*
 * Copyright (C) 2007 The Android Open Source Project
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

package org.apache.harmony.luni.internal.util;

import java.io.IOException;
import java.io.RandomAccessFile;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.channels.FileChannel;
import java.nio.charset.Charsets;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.TimeZone;
import libcore.io.IoUtils;

/**
 * A class used to initialize the time zone database.  This implementation uses the
 * 'zoneinfo' database as the source of time zone information.  However, to conserve
 * disk space the data for all time zones are concatenated into a single file, and a
 * second file is used to indicate the starting position of each time zone record.  A
 * third file indicates the version of the zoneinfo database used to generate the data.
 *
 * {@hide}
 */
public final class ZoneInfoDB {
    /**
     * The directory containing the time zone database files.
     */
    private static final String ZONE_DIRECTORY_NAME =
            System.getenv("ANDROID_ROOT") + "/usr/share/zoneinfo/";

    /**
     * The name of the file containing the concatenated time zone records.
     */
    private static final String ZONE_FILE_NAME = ZONE_DIRECTORY_NAME + "zoneinfo.dat";

    /**
     * The name of the file containing the index to each time zone record within
     * the zoneinfo.dat file.
     */
    private static final String INDEX_FILE_NAME = ZONE_DIRECTORY_NAME + "zoneinfo.idx";

    private static final Object LOCK = new Object();

    private static final String VERSION = readVersion();

    /**
     * The 'ids' array contains time zone ids sorted alphabetically, for binary searching.
     * The other two arrays are in the same order. 'byteOffsets' gives the byte offset
     * into "zoneinfo.dat" of each time zone, and 'rawUtcOffsets' gives the time zone's
     * raw UTC offset.
     */
    private static String[] ids;
    private static int[] byteOffsets;
    private static int[] rawUtcOffsets;
    static {
        readIndex();
    }

    private static ByteBuffer mappedData = mapData();

    private ZoneInfoDB() {}

    /**
     * Reads the file indicating the database version in use.  If the file is not
     * present or is unreadable, we assume a version of "2007h".
     */
    private static String readVersion() {
        RandomAccessFile versionFile = null;
        try {
            versionFile = new RandomAccessFile(ZONE_DIRECTORY_NAME + "zoneinfo.version", "r");
            byte[] buf = new byte[(int) versionFile.length()];
            versionFile.readFully(buf);
            return new String(buf, 0, buf.length, Charsets.ISO_8859_1).trim();
        } catch (IOException ex) {
            throw new RuntimeException(ex);
        } finally {
            IoUtils.closeQuietly(versionFile);
        }
    }

    /**
     * Traditionally, Unix systems have one file per time zone. We have one big data file, which
     * is just a concatenation of regular time zone files. To allow random access into this big
     * data file, we also have an index. We read the index at startup, and keep it in memory so
     * we can binary search by id when we need time zone data.
     *
     * The format of this file is, I believe, Android's own, and undocumented.
     *
     * All this code assumes strings are US-ASCII.
     */
    private static void readIndex() {
        RandomAccessFile indexFile = null;
        try {
            indexFile = new RandomAccessFile(INDEX_FILE_NAME, "r");

            // The database reserves 40 bytes for each id.
            final int SIZEOF_TZNAME = 40;
            // The database uses 32-bit (4 byte) integers.
            final int SIZEOF_TZINT = 4;

            byte[] idBytes = new byte[SIZEOF_TZNAME];

            int numEntries = (int) (indexFile.length() / (SIZEOF_TZNAME + 3*SIZEOF_TZINT));

            char[] idChars = new char[numEntries * SIZEOF_TZNAME];
            int[] idEnd = new int[numEntries];
            int idOffset = 0;

            byteOffsets = new int[numEntries];
            rawUtcOffsets = new int[numEntries];

            for (int i = 0; i < numEntries; i++) {
                indexFile.readFully(idBytes);
                byteOffsets[i] = indexFile.readInt();
                int length = indexFile.readInt();
                if (length < 44) {
                    throw new AssertionError("length in index file < sizeof(tzhead)");
                }
                rawUtcOffsets[i] = indexFile.readInt();

                // Don't include null chars in the String
                int len = idBytes.length;
                for (int j = 0; j < len; j++) {
                    if (idBytes[j] == 0) {
                        break;
                    }
                    idChars[idOffset++] = (char) (idBytes[j] & 0xFF);
                }

                idEnd[i] = idOffset;
            }

            // We create one string containing all the ids, and then break that into substrings.
            // This way, all ids share a single char[] on the heap.
            String allIds = new String(idChars, 0, idOffset);
            ids = new String[numEntries];
            for (int i = 0; i < numEntries; i++) {
                ids[i] = allIds.substring(i == 0 ? 0 : idEnd[i - 1], idEnd[i]);
            }
        } catch (IOException ex) {
            throw new RuntimeException(ex);
        } finally {
            IoUtils.closeQuietly(indexFile);
        }
    }

    /**
     * Rather than open, read, and close the big data file each time we look up a time zone,
     * we map the big data file during startup, and then just use the ByteBuffer.
     *
     * At the moment, this "big" data file is about 160 KiB. At some point, that will be small
     * enough that we'll just keep the byte[] in memory.
     */
    private static ByteBuffer mapData() {
        RandomAccessFile file = null;
        try {
            file = new RandomAccessFile(ZONE_FILE_NAME, "r");
            FileChannel channel = file.getChannel();
            ByteBuffer buffer = channel.map(FileChannel.MapMode.READ_ONLY, 0, channel.size());
            buffer.order(ByteOrder.BIG_ENDIAN);
            return buffer;
        } catch (IOException ex) {
            throw new RuntimeException(ex);
        } finally {
            IoUtils.closeQuietly(file);
        }
    }

    private static TimeZone makeTimeZone(String id) throws IOException {
        // Work out where in the big data file this time zone is.
        int index = Arrays.binarySearch(ids, id);
        if (index < 0) {
            return null;
        }
        int start = byteOffsets[index];

        // We duplicate the ByteBuffer to allow unsynchronized access to this shared data,
        // despite Buffer's implicit position.
        ByteBuffer data = mappedData.duplicate();
        data.position(start);

        // Variable names beginning tzh_ correspond to those in "tzfile.h".
        // Check tzh_magic.
        if (data.getInt() != 0x545a6966) { // "TZif"
            return null;
        }

        // Skip the uninteresting part of the header.
        data.position(start + 32);

        // Read the sizes of the arrays we're about to read.
        int tzh_timecnt = data.getInt();
        int tzh_typecnt = data.getInt();
        int tzh_charcnt = data.getInt();

        int[] transitions = new int[tzh_timecnt];
        for (int i = 0; i < tzh_timecnt; ++i) {
            transitions[i] = data.getInt();
        }

        byte[] type = new byte[tzh_timecnt];
        data.get(type);

        int[] gmtOffsets = new int[tzh_typecnt];
        byte[] isDsts = new byte[tzh_typecnt];
        byte[] abbreviationIndexes = new byte[tzh_typecnt];
        for (int i = 0; i < tzh_typecnt; ++i) {
            gmtOffsets[i] = data.getInt();
            isDsts[i] = data.get();
            abbreviationIndexes[i] = data.get();
        }

        byte[] abbreviationList = new byte[tzh_charcnt];
        data.get(abbreviationList);

        return new ZoneInfo(id, transitions, type, gmtOffsets, isDsts,
                abbreviationIndexes, abbreviationList);
    }

    public static String[] getAvailableIDs() {
        return (String[]) ids.clone();
    }

    public static String[] getAvailableIDs(int rawOffset) {
        List<String> matches = new ArrayList<String>();
        for (int i = 0, end = rawUtcOffsets.length; i < end; i++) {
            if (rawUtcOffsets[i] == rawOffset) {
                matches.add(ids[i]);
            }
        }
        return matches.toArray(new String[matches.size()]);
    }

    public static TimeZone getSystemDefault() {
        synchronized (LOCK) {
            TimezoneGetter tzGetter = TimezoneGetter.getInstance();
            String zoneName = tzGetter != null ? tzGetter.getId() : null;
            if (zoneName != null) {
                zoneName = zoneName.trim();
            }
            if (zoneName == null || zoneName.isEmpty()) {
                // use localtime for the simulator
                // TODO: what does that correspond to?
                zoneName = "localtime";
            }
            return TimeZone.getTimeZone(zoneName);
        }
    }

    public static TimeZone getTimeZone(String id) {
        if (id == null) {
            return null;
        }
        try {
            return makeTimeZone(id);
        } catch (IOException ignored) {
            return null;
        }
    }

    public static String getVersion() {
        return VERSION;
    }
}
