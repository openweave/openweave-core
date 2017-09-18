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
import junit.framework.TestCase;
import tests.support.Support_Configuration;

public class OldURLDecoderTest extends TestCase {

    public void test_decodeLjava_lang_String_Ljava_lang_String() {
        String enc = "UTF-8";

        String [] urls = { "http://" + Support_Configuration.HomeAddress +
                           "/test?hl=en&q=te+st",
                           "file://a+b/c/d.e-f*g_+l",
                           "jar:file://a.jar+!/b.c/",
                           "ftp://test:pwd@localhost:2121/%D0%9C",
                           "%D0%A2%D0%B5%D1%81%D1%82+URL+for+test"};

        String [] expected = {"http://" + Support_Configuration.HomeAddress +
                              "/test?hl=en&q=te st",
                              "file://a b/c/d.e-f*g_ l",
                              "jar:file://a.jar !/b.c/"};

        for(int i = 0; i < urls.length - 2; i++) {
            try {
                assertEquals(expected[i], URLDecoder.decode(urls[i], enc));
            } catch (UnsupportedEncodingException e) {
                fail("UnsupportedEncodingException: " + e.getMessage());
            }
        }

        try {
            URLDecoder.decode(urls[urls.length - 2], enc);
            URLDecoder.decode(urls[urls.length - 1], enc);
        } catch (UnsupportedEncodingException e) {
            fail("UnsupportedEncodingException: " + e.getMessage());
        }
    }
}
