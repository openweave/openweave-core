/*
 * Copyright (C) 2009 The Android Open Source Project
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

package libcore.java.nio.charset;

import java.nio.CharBuffer;
import java.nio.charset.Charset;
import java.nio.charset.CharsetEncoder;
import java.nio.charset.CodingErrorAction;
import java.util.Arrays;

public class CharsetEncoderTest extends junit.framework.TestCase {
    // None of the harmony or jtreg tests actually check that replaceWith does the right thing!
    public void test_replaceWith() throws Exception {
        Charset ascii = Charset.forName("US-ASCII");
        CharsetEncoder e = ascii.newEncoder();
        e.onMalformedInput(CodingErrorAction.REPLACE);
        e.onUnmappableCharacter(CodingErrorAction.REPLACE);
        e.replaceWith("=".getBytes("US-ASCII"));
        String input = "hello\u0666world";
        String output = ascii.decode(e.encode(CharBuffer.wrap(input))).toString();
        assertEquals("hello=world", output);
    }

    private void assertReplacementBytesForEncoder(String charset, byte[] bytes) {
        byte[] result = Charset.forName(charset).newEncoder().replacement();
        assertEquals(Arrays.toString(bytes), Arrays.toString(result));
    }

    // For all the guaranteed built-in charsets, check that we have the right default replacements.
    public void test_defaultReplacementBytes() throws Exception {
        assertReplacementBytesForEncoder("ISO-8859-1", new byte[] { (byte) '?' });
        assertReplacementBytesForEncoder("US-ASCII", new byte[] { (byte) '?' });
        assertReplacementBytesForEncoder("UTF-16", new byte[] { (byte) 0xff, (byte) 0xfd });
        assertReplacementBytesForEncoder("UTF-16BE", new byte[] { (byte) 0xff, (byte) 0xfd });
        assertReplacementBytesForEncoder("UTF-16LE", new byte[] { (byte) 0xfd, (byte) 0xff });
        assertReplacementBytesForEncoder("UTF-8", new byte[] { (byte) '?' });
    }
}
