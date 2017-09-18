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

package org.apache.harmony.luni.net;

import java.io.FileDescriptor;
import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocketImpl;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.MulticastGroupRequest;
import java.net.NetworkInterface;
import java.net.SocketAddress;
import java.net.SocketException;
import java.net.UnknownHostException;
import org.apache.harmony.luni.platform.INetworkSystem;
import org.apache.harmony.luni.platform.Platform;

/**
 * The default, concrete instance of datagram sockets. This class does not
 * support security checks. Alternative types of DatagramSocketImpl's may be
 * used by setting the <code>impl.prefix</code> system property.
 */
public class PlainDatagramSocketImpl extends DatagramSocketImpl {

    private static final int MCAST_JOIN_GROUP = 19;
    private static final int MCAST_LEAVE_GROUP = 20;

    private static final int SO_BROADCAST = 32;

    static final int TCP_NODELAY = 4;

    final static int IP_MULTICAST_TTL = 17;

    private byte[] ipaddress = { 0, 0, 0, 0 };

    private INetworkSystem netImpl = Platform.getNetworkSystem();

    private volatile boolean isNativeConnected;

    private boolean streaming = true;

    private boolean shutdownInput;

    /**
     * used to keep address to which the socket was connected to at the native
     * level
     */
    private InetAddress connectedAddress;

    private int connectedPort = -1;

    public PlainDatagramSocketImpl(FileDescriptor fd, int localPort) {
        super();
        this.fd = fd;
        this.localPort = localPort;
    }

    public PlainDatagramSocketImpl() {
        super();
        fd = new FileDescriptor();
    }

    @Override
    public void bind(int port, InetAddress addr) throws SocketException {
        netImpl.bind(fd, addr, port);
        if (0 != port) {
            localPort = port;
        } else {
            localPort = netImpl.getSocketLocalPort(fd);
        }

        try {
            // Ignore failures
            setOption(SO_BROADCAST, Boolean.TRUE);
        } catch (IOException e) {
        }
    }

    @Override
    public void close() {
        synchronized (fd) {
            if (fd.valid()) {
                try {
                    netImpl.close(fd);
                } catch (IOException e) {
                }
                fd = new FileDescriptor();
            }
        }
    }

    @Override
    public void create() throws SocketException {
        netImpl.socket(fd, false);
    }

    @Override protected void finalize() throws Throwable {
        try {
            close();
        } finally {
            super.finalize();
        }
    }

    public Object getOption(int optID) throws SocketException {
        return netImpl.getSocketOption(fd, optID);
    }

    @Override
    public int getTimeToLive() throws IOException {
        return ((Integer) getOption(IP_MULTICAST_TTL)).intValue();
    }

    @Override
    public byte getTTL() throws IOException {
        return (byte) getTimeToLive();
    }

    @Override
    public void join(InetAddress addr) throws IOException {
        setOption(MCAST_JOIN_GROUP, new MulticastGroupRequest(addr, null));
    }

    @Override
    public void joinGroup(SocketAddress addr, NetworkInterface netInterface) throws IOException {
        if (addr instanceof InetSocketAddress) {
            InetAddress groupAddr = ((InetSocketAddress) addr).getAddress();
            setOption(MCAST_JOIN_GROUP, new MulticastGroupRequest(groupAddr, netInterface));
        }
    }

    @Override
    public void leave(InetAddress addr) throws IOException {
        setOption(MCAST_LEAVE_GROUP, new MulticastGroupRequest(addr, null));
    }

    @Override
    public void leaveGroup(SocketAddress addr, NetworkInterface netInterface) throws IOException {
        if (addr instanceof InetSocketAddress) {
            InetAddress groupAddr = ((InetSocketAddress) addr).getAddress();
            setOption(MCAST_LEAVE_GROUP, new MulticastGroupRequest(groupAddr, netInterface));
        }
    }

    @Override
    protected int peek(InetAddress sender) throws IOException {
        // We don't actually want the data: we just want the DatagramPacket's filled-in address.
        byte[] bytes = new byte[0];
        DatagramPacket packet = new DatagramPacket(bytes, bytes.length);
        int result = peekData(packet);
        netImpl.setInetAddress(sender, packet.getAddress().getAddress());
        return result;
    }

    private void doRecv(DatagramPacket pack, boolean peek) throws IOException {
        netImpl.recv(fd, pack, pack.getData(), pack.getOffset(), pack.getLength(), peek,
                isNativeConnected);
        if (isNativeConnected) {
            updatePacketRecvAddress(pack);
        }
    }

    @Override
    public void receive(DatagramPacket pack) throws IOException {
        doRecv(pack, false);
    }

    @Override
    public int peekData(DatagramPacket pack) throws IOException {
        doRecv(pack, true);
        return pack.getPort();
    }

    @Override
    public void send(DatagramPacket packet) throws IOException {
        int port = isNativeConnected ? 0 : packet.getPort();
        InetAddress address = isNativeConnected ? null : packet.getAddress();
        netImpl.send(fd, packet.getData(), packet.getOffset(), packet.getLength(), port, address);
    }

    public void setOption(int optID, Object val) throws SocketException {
        netImpl.setSocketOption(fd, optID, val);
    }

    @Override
    public void setTimeToLive(int ttl) throws IOException {
        setOption(IP_MULTICAST_TTL, Integer.valueOf(ttl));
    }

    @Override
    public void setTTL(byte ttl) throws IOException {
        setTimeToLive((int) ttl & 0xff); // Avoid sign extension.
    }

    @Override
    public void connect(InetAddress inetAddr, int port) throws SocketException {
        netImpl.connect(fd, inetAddr, port, 0);

        // if we get here then we are connected at the native level
        try {
            connectedAddress = InetAddress.getByAddress(inetAddr.getAddress());
        } catch (UnknownHostException e) {
            // this is never expected to happen as we should not have gotten
            // here if the address is not resolvable
            throw new SocketException("Host is unresolved: " + inetAddr.getHostName());
        }
        connectedPort = port;
        isNativeConnected = true;
    }

    @Override
    public void disconnect() {
        try {
            netImpl.disconnectDatagram(fd);
        } catch (Exception e) {
            // there is currently no way to return an error so just eat any
            // exception
        }
        connectedPort = -1;
        connectedAddress = null;
        isNativeConnected = false;
    }

    /**
     * Set the received address and port in the packet. We do this when the
     * Datagram socket is connected at the native level and the
     * recvConnnectedDatagramImpl does not update the packet with address from
     * which the packet was received
     *
     * @param packet
     *            the packet to be updated
     */
    private void updatePacketRecvAddress(DatagramPacket packet) {
        packet.setAddress(connectedAddress);
        packet.setPort(connectedPort);
    }
}
