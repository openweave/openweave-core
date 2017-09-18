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

package libcore.java.security;

import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.Arrays;
import junit.framework.TestCase;

public final class MessageDigestTest extends TestCase {

    private final byte[] sha_456 = {
            -24,   9, -59, -47,  -50,  -92, 123, 69, -29,  71,
              1, -46,  63,  96, -118, -102,  88,  3,  77, -55
    };

    public void testShaReset() throws NoSuchAlgorithmException {
        MessageDigest sha = MessageDigest.getInstance("SHA");
        sha.update(new byte[] { 1, 2, 3 });
        sha.reset();
        sha.update(new byte[] { 4, 5, 6 });
        assertEquals(Arrays.toString(sha_456), Arrays.toString(sha.digest()));
    }
}
