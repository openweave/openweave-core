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

package java.net;

/**
 * Used to pass an interface index and multicast address to native code.
 *
 * @hide PlainDatagramSocketImpl use only
 */
public final class MulticastGroupRequest {
    private final int gr_interface;
    private final InetAddress gr_group;

    public MulticastGroupRequest(InetAddress groupAddress, NetworkInterface networkInterface) {
        gr_group = groupAddress;
        gr_interface = (networkInterface != null) ? networkInterface.getIndex() : 0;
    }
}
