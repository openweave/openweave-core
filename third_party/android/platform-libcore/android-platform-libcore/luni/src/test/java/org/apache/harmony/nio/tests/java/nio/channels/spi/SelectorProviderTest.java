/* Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package org.apache.harmony.nio.tests.java.nio.channels.spi;

import dalvik.annotation.TestTargets;
import dalvik.annotation.TestLevel;
import dalvik.annotation.TestTargetNew;
import dalvik.annotation.TestTargetClass;

import java.nio.channels.spi.SelectorProvider;

import junit.framework.TestCase;
@TestTargetClass(SelectorProvider.class)
public class SelectorProviderTest extends TestCase {

    /**
     * @tests SelectorProvider#openDatagramChannel()
     * @tests SelectorProvider#openPipe()
     * @tests SelectorProvider#openServerSocketChannel()
     * @tests SelectorProvider#openSocketChannel()
     * @tests SelectorProvider#openSelector()
     */
    @TestTargets({
        @TestTargetNew(
            level = TestLevel.COMPLETE,
            notes = "",
            method = "SelectorProvider",
            args = {}
        ),
        @TestTargetNew(
            level = TestLevel.PARTIAL_COMPLETE,
            notes = "",
            method = "provider",
            args = {}
        ),
        @TestTargetNew(
            level = TestLevel.COMPLETE,
            notes = "",
            method = "inheritedChannel",
            args = {}
        ),
        @TestTargetNew(
            level = TestLevel.COMPLETE,
            notes = "",
            method = "openDatagramChannel",
            args = {}
        ),
        @TestTargetNew(
            level = TestLevel.COMPLETE,
            notes = "",
            method = "openPipe",
            args = {}
        ),
        @TestTargetNew(
            level = TestLevel.COMPLETE,
            notes = "",
            method = "openServerSocketChannel",
            args = {}
        ),
        @TestTargetNew(
            level = TestLevel.PARTIAL_COMPLETE,
            notes = "",
            method = "openSocketChannel",
            args = {}
        ),
        @TestTargetNew(
            level = TestLevel.COMPLETE,
            notes = "",
            method = "openSelector",
            args = {}
        )
    })
    public void test_open_methods() throws Exception {
        // calling #provider to see if it returns without Exception.
        assertNotNull(SelectorProvider.provider());

        // calling #inheritedChannel to see if this already throws an exception.
        SelectorProvider.provider().inheritedChannel();

        assertNotNull(SelectorProvider.provider().openDatagramChannel());
        assertNotNull(SelectorProvider.provider().openPipe());
        assertNotNull(SelectorProvider.provider().openServerSocketChannel());
        assertNotNull(SelectorProvider.provider().openSocketChannel());
        assertNotNull(SelectorProvider.provider().openSelector());
    }
}