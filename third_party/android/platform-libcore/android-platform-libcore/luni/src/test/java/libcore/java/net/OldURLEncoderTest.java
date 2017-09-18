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

package libcore.java.net;

import java.io.UnsupportedEncodingException;
import java.net.URLDecoder;
import java.net.URLEncoder;
import junit.framework.TestCase;
import tests.support.Support_Configuration;

public class OldURLEncoderTest extends TestCase {

    public void test_encodeLjava_lang_StringLjava_lang_String() {

        String enc = "UTF-8";

        String [] urls = {"http://" + Support_Configuration.HomeAddress +
                              "/test?hl=en&q=te st",
                              "file://a b/c/d.e-f*g_ l",
                              "jar:file://a.jar !/b.c/\u1052",
                              "ftp://test:pwd@localhost:2121/%D0%9C"};

        String [] expected = { "http%3A%2F%2Fjcltest.apache.org%2Ftest%3Fhl%" +
                "3Den%26q%3Dte+st",
                "file%3A%2F%2Fa+b%2Fc%2Fd.e-f*g_+l",
                "jar%3Afile%3A%2F%2Fa.jar+%21%2Fb.c%2F%E1%81%92"};

        for(int i = 0; i < urls.length-1; i++) {
            try {
                String encodedString = URLEncoder.encode(urls[i], enc);
                assertEquals(expected[i], encodedString);
                assertEquals(urls[i], URLDecoder.decode(encodedString, enc));
            } catch (UnsupportedEncodingException e) {
                fail("UnsupportedEncodingException: " + e.getMessage());
            }
        }

        try {
            String encodedString = URLEncoder.encode(urls[urls.length - 1], enc);
            assertEquals(urls[urls.length - 1], URLDecoder.decode(encodedString, enc));
        } catch (UnsupportedEncodingException e) {
            fail("UnsupportedEncodingException: " + e.getMessage());
        }

        try {
            URLDecoder.decode("", "");
            fail("UnsupportedEncodingException expected");
        } catch (UnsupportedEncodingException e) {
            //expected
        }
    }
}
