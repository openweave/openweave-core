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

package libcore.java.lang;

import java.io.PrintWriter;
import java.io.StringWriter;
import junit.framework.TestCase;

public class ThrowableTest extends TestCase {
    private static class NoStackTraceException extends Exception {
        @Override
        public synchronized Throwable fillInStackTrace() {
            return null;
        }
    }
    public void testNullStackTrace() {
        try {
            throw new NoStackTraceException();
        } catch (NoStackTraceException ex) {
            // We used to throw NullPointerException when printing an exception with no stack trace.
            ex.printStackTrace(new PrintWriter(new StringWriter()));
        }
    }
}
