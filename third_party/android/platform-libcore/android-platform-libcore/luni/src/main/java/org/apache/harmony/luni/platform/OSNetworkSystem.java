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

package org.apache.harmony.luni.platform;

import java.io.FileDescriptor;
import java.io.IOException;
import java.net.DatagramPacket;
import java.net.InetAddress;
import java.net.SocketException;
import java.net.SocketImpl;

/**
 * This wraps native code that implements the INetworkSystem interface.
 * Address length was changed from long to int for performance reasons.
 */
final class OSNetworkSystem implements INetworkSystem {
    private static final OSNetworkSystem singleton = new OSNetworkSystem();

    public static OSNetworkSystem getOSNetworkSystem() {
        return singleton;
    }

    private OSNetworkSystem() {
    }

    public native void accept(FileDescriptor serverFd, SocketImpl newSocket,
            FileDescriptor clientFd) throws IOException;

    public native void bind(FileDescriptor fd, InetAddress inetAddress, int port) throws SocketException;

    public native void connect(FileDescriptor fd, InetAddress inetAddress, int port, int timeout)
            throws SocketException;

    public native boolean connectNonBlocking(FileDescriptor fd, InetAddress inetAddress, int port)
            throws IOException;
    public native boolean isConnected(FileDescriptor fd, int timeout) throws IOException;

    public native void socket(FileDescriptor fd, boolean stream) throws SocketException;

    public native void disconnectDatagram(FileDescriptor fd) throws SocketException;

    public native InetAddress getSocketLocalAddress(FileDescriptor fd);

    public native int getSocketLocalPort(FileDescriptor fd);

    public native Object getSocketOption(FileDescriptor fd, int opt) throws SocketException;

    public native void listen(FileDescriptor fd, int backlog) throws SocketException;

    public native int read(FileDescriptor fd, byte[] data, int offset, int count)
            throws IOException;

    public native int readDirect(FileDescriptor fd, int address, int count) throws IOException;

    public native int recv(FileDescriptor fd, DatagramPacket packet,
            byte[] data, int offset, int length,
            boolean peek, boolean connected) throws IOException;

    public native int recvDirect(FileDescriptor fd, DatagramPacket packet,
            int address, int offset, int length,
            boolean peek, boolean connected) throws IOException;

    public boolean select(FileDescriptor[] readFDs, FileDescriptor[] writeFDs,
            int numReadable, int numWritable, long timeout, int[] flags)
            throws SocketException {
        if (numReadable < 0 || numWritable < 0) {
            throw new IllegalArgumentException();
        }

        int total = numReadable + numWritable;
        if (total == 0) {
            return true;
        }

        return selectImpl(readFDs, writeFDs, numReadable, numWritable, flags, timeout);
    }

    static native boolean selectImpl(FileDescriptor[] readfd,
            FileDescriptor[] writefd, int cread, int cwirte, int[] flags,
            long timeout);

    public native int send(FileDescriptor fd, byte[] data, int offset, int length,
            int port, InetAddress inetAddress) throws IOException;
    public native int sendDirect(FileDescriptor fd, int address, int offset, int length,
            int port, InetAddress inetAddress) throws IOException;

    public native void sendUrgentData(FileDescriptor fd, byte value);

    public native void setInetAddress(InetAddress sender, byte[] address);

    public native void setSocketOption(FileDescriptor fd, int opt, Object optVal)
            throws SocketException;

    public native void shutdownInput(FileDescriptor fd) throws IOException;

    public native void shutdownOutput(FileDescriptor fd) throws IOException;

    public native void close(FileDescriptor fd) throws IOException;

    public native int write(FileDescriptor fd, byte[] data, int offset, int count)
            throws IOException;

    public native int writeDirect(FileDescriptor fd, int address, int offset, int count)
            throws IOException;
}
