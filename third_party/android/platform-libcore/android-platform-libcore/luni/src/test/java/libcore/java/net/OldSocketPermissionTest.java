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

import java.net.SocketPermission;
import tests.support.Support_Configuration;

public class OldSocketPermissionTest extends junit.framework.TestCase {

    String starName = "*." + Support_Configuration.DomainAddress;

    String wwwName = Support_Configuration.HomeAddress;

    SocketPermission star_All = new SocketPermission(starName, "listen,accept,connect");

    SocketPermission www_All = new SocketPermission(wwwName,
            "connect,listen,accept");

    public void test_hashCode() {
        SocketPermission sp1 = new SocketPermission(
                Support_Configuration.InetTestIP, "resolve,connect");
        SocketPermission sp2 = new SocketPermission(
                Support_Configuration.InetTestIP, "resolve,connect");
        assertTrue("Same IP address should have equal hash codes",
                sp1.hashCode() == sp2.hashCode());

        assertTrue("Different names but returned equal hash codes",
                star_All.hashCode() != www_All.hashCode());
    }
}
