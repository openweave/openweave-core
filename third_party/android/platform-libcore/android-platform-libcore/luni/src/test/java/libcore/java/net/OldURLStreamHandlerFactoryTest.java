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

package libcore.java.net;

import java.io.IOException;
import java.lang.reflect.Field;
import java.net.MalformedURLException;
import java.net.URL;
import java.net.URLConnection;
import java.net.URLStreamHandler;
import java.net.URLStreamHandlerFactory;
import junit.framework.TestCase;
import tests.support.Support_Configuration;

public final class OldURLStreamHandlerFactoryTest extends TestCase {

    URLStreamHandlerFactory oldFactory = null;
    Field factoryField = null;

    boolean isTestable = false;

    boolean isOpenConnectionCalled = false;
    boolean isCreateURLStreamHandlerCalled = false;

    public void test_createURLStreamHandler() throws MalformedURLException {

        if(isTestable) {

            TestURLStreamHandlerFactory shf = new TestURLStreamHandlerFactory();
            assertFalse(isCreateURLStreamHandlerCalled);
            URL.setURLStreamHandlerFactory(shf);
            URL url = new URL("http://" +
                    Support_Configuration.SpecialInetTestAddress);

            try {
                url.openConnection();
                assertTrue(isCreateURLStreamHandlerCalled);
                assertTrue(isOpenConnectionCalled);
            } catch (Exception e) {
                fail("Exception during test : " + e.getMessage());

            }

            try {
                URL.setURLStreamHandlerFactory(shf);
                fail("java.lang.Error was not thrown.");
            } catch(java.lang.Error e) {
                //expected
            }

            try {
                URL.setURLStreamHandlerFactory(null);
                fail("java.lang.Error was not thrown.");
            } catch(java.lang.Error e) {
                //expected
            }

        } else {
            TestURLStreamHandlerFactory shf = new TestURLStreamHandlerFactory();
            URLStreamHandler sh = shf.createURLStreamHandler("");
            assertNotNull(sh.toString());
        }
    }

    public void setUp() {
        Field [] fields = URL.class.getDeclaredFields();
        int counter = 0;
        for (Field field : fields) {
            if (URLStreamHandlerFactory.class.equals(field.getType())) {
                counter++;
                factoryField = field;
            }
        }

        if(counter == 1) {

            isTestable = true;

            factoryField.setAccessible(true);
            try {
                oldFactory = (URLStreamHandlerFactory) factoryField.get(null);
            } catch (IllegalArgumentException e) {
                fail("IllegalArgumentException was thrown during setUp: "
                        + e.getMessage());
            } catch (IllegalAccessException e) {
                fail("IllegalAccessException was thrown during setUp: "
                        + e.getMessage());
            }
        }
    }

    public void tearDown() throws IllegalAccessException {
        if(isTestable) {
            factoryField.set(null, null);
            URL.setURLStreamHandlerFactory(oldFactory);
        }
    }

    class TestURLStreamHandlerFactory implements URLStreamHandlerFactory {

        public URLStreamHandler createURLStreamHandler(String protocol) {
            isCreateURLStreamHandlerCalled = true;
            return new TestURLStreamHandler();
        }
    }

    class TestURLStreamHandler extends URLStreamHandler {
        @Override
        protected URLConnection openConnection(URL u) throws IOException {
            isOpenConnectionCalled = true;
            return null;
        }
    }
}
