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

package org.apache.harmony.nio.internal;

// BEGIN android-note
// In this class the address length was changed from long to int.
// END android-note

import java.io.FileDescriptor;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.ConnectException;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.SocketAddress;
import java.net.SocketException;
import java.net.SocketUtils;
import java.net.UnknownHostException;
import java.nio.ByteBuffer;
import java.nio.channels.AlreadyConnectedException;
import java.nio.channels.ClosedChannelException;
import java.nio.channels.ConnectionPendingException;
import java.nio.channels.IllegalBlockingModeException;
import java.nio.channels.NoConnectionPendingException;
import java.nio.channels.NotYetConnectedException;
import java.nio.channels.SocketChannel;
import java.nio.channels.UnresolvedAddressException;
import java.nio.channels.UnsupportedAddressTypeException;
import java.nio.channels.spi.SelectorProvider;
import libcore.io.IoUtils;
import org.apache.harmony.luni.net.PlainSocketImpl;
import org.apache.harmony.luni.platform.FileDescriptorHandler;
import org.apache.harmony.luni.platform.INetworkSystem;
import org.apache.harmony.luni.platform.Platform;
import org.apache.harmony.nio.AddressUtil;

/*
 * The default implementation class of java.nio.channels.SocketChannel.
 */
class SocketChannelImpl extends SocketChannel implements FileDescriptorHandler {

    private static final int EOF = -1;

    // The singleton to do the native network operation.
    static final INetworkSystem networkSystem = Platform.getNetworkSystem();

    // Status un-init, not initialized.
    static final int SOCKET_STATUS_UNINIT = EOF;

    // Status before connect.
    static final int SOCKET_STATUS_UNCONNECTED = 0;

    // Status connection pending.
    static final int SOCKET_STATUS_PENDING = 1;

    // Status after connection success.
    static final int SOCKET_STATUS_CONNECTED = 2;

    // Status closed.
    static final int SOCKET_STATUS_CLOSED = 3;

    // The descriptor to interact with native code.
    FileDescriptor fd;

    // Our internal Socket.
    private SocketAdapter socket = null;

    // The address to be connected.
    InetSocketAddress connectAddress = null;

    // Local address of the this socket (package private for adapter).
    InetAddress localAddress = null;

    // Local port number.
    int localPort;

    // At first, uninitialized.
    int status = SOCKET_STATUS_UNINIT;

    // Whether the socket is bound.
    volatile boolean isBound = false;

    private static class ReadLock {}
    private final Object readLock = new ReadLock();

    private static class WriteLock {}
    private final Object writeLock = new WriteLock();

    /*
     * Constructor for creating a connected socket channel.
     */
    public SocketChannelImpl(SelectorProvider selectorProvider) throws IOException {
        this(selectorProvider, true);
    }

    /*
     * Constructor for creating an optionally connected socket channel.
     */
    public SocketChannelImpl(SelectorProvider selectorProvider, boolean connect) throws IOException {
        super(selectorProvider);
        fd = new FileDescriptor();
        status = SOCKET_STATUS_UNCONNECTED;
        if (connect) {
            networkSystem.socket(fd, true);
        }
    }

    /*
     * Getting the internal Socket If we have not the socket, we create a new
     * one.
     */
    @Override
    synchronized public Socket socket() {
        if (socket == null) {
            try {
                InetAddress addr = null;
                int port = 0;
                if (connectAddress != null) {
                    addr = connectAddress.getAddress();
                    port = connectAddress.getPort();
                }
                socket = new SocketAdapter(new PlainSocketImpl(fd, localPort, addr, port), this);
            } catch (SocketException e) {
                return null;
            }
        }
        return socket;
    }

    @Override
    synchronized public boolean isConnected() {
        return status == SOCKET_STATUS_CONNECTED;
    }

    /*
     * Status setting used by other class.
     */
    synchronized void setConnected() {
        status = SOCKET_STATUS_CONNECTED;
    }

    void setBound(boolean flag) {
        isBound = flag;
    }

    @Override
    synchronized public boolean isConnectionPending() {
        return status == SOCKET_STATUS_PENDING;
    }

    @Override
    public boolean connect(SocketAddress socketAddress) throws IOException {
        // status must be open and unconnected
        checkUnconnected();

        // check the address
        InetSocketAddress inetSocketAddress = validateAddress(socketAddress);
        InetAddress normalAddr = inetSocketAddress.getAddress();

        // When connecting, map ANY address to Localhost
        if (normalAddr.isAnyLocalAddress()) {
            normalAddr = InetAddress.getLocalHost();
        }

        int port = inetSocketAddress.getPort();
        String hostName = normalAddr.getHostName();
        // security check
        SecurityManager sm = System.getSecurityManager();
        if (sm != null) {
            sm.checkConnect(hostName, port);
        }

        // connect result
        int result = EOF;
        boolean finished = false;

        try {
            if (isBlocking()) {
                begin();
                networkSystem.connect(fd, normalAddr, port, 0);
                finished = true; // Or we'd have thrown an exception.
            } else {
                finished = networkSystem.connectNonBlocking(fd, normalAddr, port);
                // set back to nonblocking to work around with a bug in portlib
                if (!isBlocking()) {
                    IoUtils.setBlocking(fd, false);
                }
            }
            isBound = finished;
        } catch (IOException e) {
            if (e instanceof ConnectException && !isBlocking()) {
                status = SOCKET_STATUS_PENDING;
            } else {
                if (isOpen()) {
                    close();
                    finished = true;
                }
                throw e;
            }
        } finally {
            if (isBlocking()) {
                end(finished);
            }
        }

        initLocalAddressAndPort();
        connectAddress = inetSocketAddress;
        if (socket != null) {
            socket.socketImpl().initRemoteAddressAndPort(connectAddress.getAddress(),
                    connectAddress.getPort());
        }

        synchronized (this) {
            if (isBlocking()) {
                status = (finished ? SOCKET_STATUS_CONNECTED : SOCKET_STATUS_UNCONNECTED);
            } else {
                status = SOCKET_STATUS_PENDING;
            }
        }
        return finished;
    }

    private void initLocalAddressAndPort() {
        localAddress = networkSystem.getSocketLocalAddress(fd);
        localPort = networkSystem.getSocketLocalPort(fd);
        if (socket != null) {
            socket.socketImpl().initLocalPort(localPort);
        }
    }

    @Override
    public boolean finishConnect() throws IOException {
        // status check
        synchronized (this) {
            if (!isOpen()) {
                throw new ClosedChannelException();
            }
            if (status == SOCKET_STATUS_CONNECTED) {
                return true;
            }
            if (status != SOCKET_STATUS_PENDING) {
                throw new NoConnectionPendingException();
            }
        }

        boolean finished = false;
        try {
            begin();
            final int WAIT_FOREVER = -1;
            final int POLL = 0;
            finished = networkSystem.isConnected(fd, isBlocking() ? WAIT_FOREVER : POLL);
            isBound = finished;
            initLocalAddressAndPort();
        } catch (ConnectException e) {
            if (isOpen()) {
                close();
                finished = true;
            }
            throw e;
        } finally {
            end(finished);
        }

        synchronized (this) {
            status = (finished ? SOCKET_STATUS_CONNECTED : status);
            isBound = finished;
            // TPE: Workaround for bug that turns socket back to blocking
            if (!isBlocking()) implConfigureBlocking(false);
        }
        return finished;
    }

    void finishAccept() {
        initLocalAddressAndPort();
    }

    @Override
    public int read(ByteBuffer target) throws IOException {
        FileChannelImpl.checkWritable(target);
        checkOpenConnected();
        if (!target.hasRemaining()) {
            return 0;
        }

        int readCount;
        if (target.isDirect() || target.hasArray()) {
            readCount = readImpl(target);
            if (readCount > 0) {
                target.position(target.position() + readCount);
            }
        } else {
            ByteBuffer readBuffer = null;
            byte[] readArray = null;
            readArray = new byte[target.remaining()];
            readBuffer = ByteBuffer.wrap(readArray);
            readCount = readImpl(readBuffer);
            if (readCount > 0) {
                target.put(readArray, 0, readCount);
            }
        }
        return readCount;
    }

    @Override
    public long read(ByteBuffer[] targets, int offset, int length) throws IOException {
        if (!isIndexValid(targets, offset, length)) {
            throw new IndexOutOfBoundsException();
        }

        checkOpenConnected();
        int totalCount = FileChannelImpl.calculateTotalRemaining(targets, offset, length, true);
        if (totalCount == 0) {
            return 0;
        }
        byte[] readArray = new byte[totalCount];
        ByteBuffer readBuffer = ByteBuffer.wrap(readArray);
        int readCount;
        // read data to readBuffer, and then transfer data from readBuffer to
        // targets.
        readCount = readImpl(readBuffer);
        if (readCount > 0) {
            int left = readCount;
            int index = offset;
            // transfer data from readArray to targets
            while (left > 0) {
                int putLength = Math.min(targets[index].remaining(), left);
                targets[index].put(readArray, readCount - left, putLength);
                index++;
                left -= putLength;
            }
        }
        return readCount;
    }

    private boolean isIndexValid(ByteBuffer[] targets, int offset, int length) {
        return (length >= 0) && (offset >= 0)
                && ((long) length + (long) offset <= targets.length);
    }

    /**
     * Read from channel, and store the result in the target.
     *
     * @param target
     *            output parameter
     */
    private int readImpl(ByteBuffer target) throws IOException {
        synchronized (readLock) {
            int readCount = 0;
            try {
                if (isBlocking()) {
                    begin();
                }
                int offset = target.position();
                int length = target.remaining();
                if (target.isDirect()) {
                    // BEGIN android-changed
                    // changed address from long to int
                    int address = AddressUtil.getDirectBufferAddress(target);
                    readCount = networkSystem.readDirect(fd, address + offset, length);
                    // END android-changed
                } else {
                    // target is assured to have array.
                    byte[] array = target.array();
                    offset += target.arrayOffset();
                    readCount = networkSystem.read(fd, array, offset, length);
                }
                return readCount;
            } finally {
                if (isBlocking()) {
                    end(readCount > 0);
                }
            }
        }
    }

    @Override
    public int write(ByteBuffer source) throws IOException {
        if (null == source) {
            throw new NullPointerException();
        }
        checkOpenConnected();
        if (!source.hasRemaining()) {
            return 0;
        }
        return writeImpl(source);
    }

    @Override
    public long write(ByteBuffer[] sources, int offset, int length) throws IOException {
        if (!isIndexValid(sources, offset, length)) {
            throw new IndexOutOfBoundsException();
        }

        checkOpenConnected();
        int count = FileChannelImpl.calculateTotalRemaining(sources, offset, length, false);
        if (count == 0) {
            return 0;
        }
        ByteBuffer writeBuf = ByteBuffer.allocate(count);
        for (int val = offset; val < length + offset; val++) {
            ByteBuffer source = sources[val];
            int oldPosition = source.position();
            writeBuf.put(source);
            source.position(oldPosition);
        }
        writeBuf.flip();
        int result = writeImpl(writeBuf);
        int val = offset;
        int written = result;
        while (result > 0) {
            ByteBuffer source = sources[val];
            int gap = Math.min(result, source.remaining());
            source.position(source.position() + gap);
            val++;
            result -= gap;
        }
        return written;
    }

    /*
     * Write the source. return the count of bytes written.
     */
    private int writeImpl(ByteBuffer source) throws IOException {
        synchronized (writeLock) {
            if (!source.hasRemaining()) {
                return 0;
            }
            int writeCount = 0;
            try {
                int pos = source.position();
                int length = source.remaining();
                if (isBlocking()) {
                    begin();
                }
                if (source.isDirect()) {
                    int address = AddressUtil.getDirectBufferAddress(source);
                    writeCount = networkSystem.writeDirect(fd, address, pos, length);
                } else if (source.hasArray()) {
                    pos += source.arrayOffset();
                    writeCount = networkSystem.write(fd, source.array(), pos, length);
                } else {
                    byte[] array = new byte[length];
                    source.get(array);
                    writeCount = networkSystem.write(fd, array, 0, length);
                }
                source.position(pos + writeCount);
            } finally {
                if (isBlocking()) {
                    end(writeCount >= 0);
                }
            }
            return writeCount;
        }
    }

    /*
     * Status check, open and "connected", when read and write.
     */
    synchronized private void checkOpenConnected() throws ClosedChannelException {
        if (!isOpen()) {
            throw new ClosedChannelException();
        }
        if (!isConnected()) {
            throw new NotYetConnectedException();
        }
    }

    /*
     * Status check, open and "unconnected", before connection.
     */
    synchronized private void checkUnconnected() throws IOException {
        if (!isOpen()) {
            throw new ClosedChannelException();
        }
        if (status == SOCKET_STATUS_CONNECTED) {
            throw new AlreadyConnectedException();
        }
        if (status == SOCKET_STATUS_PENDING) {
            throw new ConnectionPendingException();
        }
    }

    /*
     * Shared by this class and DatagramChannelImpl, to do the address transfer
     * and check.
     */
    static InetSocketAddress validateAddress(SocketAddress socketAddress) {
        if (null == socketAddress) {
            throw new IllegalArgumentException();
        }
        if (!(socketAddress instanceof InetSocketAddress)) {
            throw new UnsupportedAddressTypeException();
        }
        InetSocketAddress inetSocketAddress = (InetSocketAddress) socketAddress;
        if (inetSocketAddress.isUnresolved()) {
            throw new UnresolvedAddressException();
        }
        return inetSocketAddress;
    }

    /*
     * Get local address.
     */
    public InetAddress getLocalAddress() throws UnknownHostException {
        if (!isBound) {
            byte[] any_bytes = { 0, 0, 0, 0 };
            return InetAddress.getByAddress(any_bytes);
        }
        return localAddress;
    }

    /*
     * Do really closing action here.
     */
    @Override
    synchronized protected void implCloseSelectableChannel() throws IOException {
        if (SOCKET_STATUS_CLOSED != status) {
            status = SOCKET_STATUS_CLOSED;
            if (null != socket && !socket.isClosed()) {
                socket.close();
            } else {
                networkSystem.close(fd);
            }
        }
    }

    @Override
    protected void implConfigureBlocking(boolean blockMode) throws IOException {
        synchronized (blockingLock()) {
            IoUtils.setBlocking(fd, blockMode);
        }
    }

    /*
     * Get the fd.
     */
    public FileDescriptor getFD() {
        return fd;
    }

    /*
     * Adapter classes for internal socket.
     */
    private static class SocketAdapter extends Socket {
        private final SocketChannelImpl channel;
        private final PlainSocketImpl socketImpl;

        SocketAdapter(PlainSocketImpl socketImpl, SocketChannelImpl channel) throws SocketException {
            super(socketImpl);
            this.socketImpl = socketImpl;
            this.channel = channel;
            SocketUtils.setCreated(this);
        }

        PlainSocketImpl socketImpl() {
            return socketImpl;
        }

        @Override
        public SocketChannel getChannel() {
            return channel;
        }

        @Override
        public boolean isBound() {
            return channel.isBound;
        }

        @Override
        public boolean isConnected() {
            return channel.isConnected();
        }

        @Override
        public InetAddress getLocalAddress() {
            try {
                return channel.getLocalAddress();
            } catch (UnknownHostException e) {
                return null;
            }
        }

        @Override
        public void connect(SocketAddress remoteAddr, int timeout) throws IOException {
            if (!channel.isBlocking()) {
                throw new IllegalBlockingModeException();
            }
            if (isConnected()) {
                throw new AlreadyConnectedException();
            }
            super.connect(remoteAddr, timeout);
            channel.initLocalAddressAndPort();
            if (super.isConnected()) {
                channel.setConnected();
                channel.isBound = super.isBound();
            }
        }

        @Override
        public void bind(SocketAddress localAddr) throws IOException {
            if (channel.isConnected()) {
                throw new AlreadyConnectedException();
            }
            if (SocketChannelImpl.SOCKET_STATUS_PENDING == channel.status) {
                throw new ConnectionPendingException();
            }
            super.bind(localAddr);
            // keep here to see if need next version
            // channel.Address = getLocalSocketAddress();
            // channel.localport = getLocalPort();
            channel.isBound = true;
        }

        @Override
        public void close() throws IOException {
            synchronized (channel) {
                if (channel.isOpen()) {
                    channel.close();
                } else {
                    super.close();
                }
                channel.status = SocketChannelImpl.SOCKET_STATUS_CLOSED;
            }
        }

        @Override
        public OutputStream getOutputStream() throws IOException {
            checkOpenAndConnected();
            if (isOutputShutdown()) {
                throw new SocketException("Socket output is shutdown");
            }
            return new SocketChannelOutputStream(channel);
        }

        @Override
        public InputStream getInputStream() throws IOException {
            checkOpenAndConnected();
            if (isInputShutdown()) {
                throw new SocketException("Socket input is shutdown");
            }
            return new SocketChannelInputStream(channel);
        }

        private void checkOpenAndConnected() throws SocketException {
            if (!channel.isOpen()) {
                throw new SocketException("Socket is closed");
            }
            if (!channel.isConnected()) {
                throw new SocketException("Socket is not connected");
            }
        }
    }

    /*
     * This output stream delegates all operations to the associated channel.
     * Throws an IllegalBlockingModeException if the channel is in non-blocking
     * mode when performing write operations.
     */
    private static class SocketChannelOutputStream extends OutputStream {
        private final SocketChannel channel;

        public SocketChannelOutputStream(SocketChannel channel) {
            this.channel = channel;
        }

        /*
         * Closes this stream and channel.
         *
         * @exception IOException thrown if an error occurs during the close
         */
        @Override
        public void close() throws IOException {
            channel.close();
        }

        @Override
        public void write(byte[] buffer, int offset, int count) throws IOException {
            if (0 > offset || 0 > count || count + offset > buffer.length) {
                throw new IndexOutOfBoundsException();
            }
            ByteBuffer buf = ByteBuffer.wrap(buffer, offset, count);
            if (!channel.isBlocking()) {
                throw new IllegalBlockingModeException();
            }
            channel.write(buf);
        }

        @Override
        public void write(int oneByte) throws IOException {
            if (!channel.isBlocking()) {
                throw new IllegalBlockingModeException();
            }
            ByteBuffer buffer = ByteBuffer.allocate(1);
            buffer.put(0, (byte) (oneByte & 0xFF));
            channel.write(buffer);
        }
    }

    /*
     * This input stream delegates all operations to the associated channel.
     * Throws an IllegalBlockingModeException if the channel is in non-blocking
     * mode when performing read operations.
     */
    private static class SocketChannelInputStream extends InputStream {
        private final SocketChannel channel;

        public SocketChannelInputStream(SocketChannel channel) {
            this.channel = channel;
        }

        /*
         * Closes this stream and channel.
         */
        @Override
        public void close() throws IOException {
            channel.close();
        }

        @Override
        public int read() throws IOException {
            if (!channel.isBlocking()) {
                throw new IllegalBlockingModeException();
            }
            ByteBuffer buf = ByteBuffer.allocate(1);
            int result = channel.read(buf);
            // BEGIN android-changed: input was already consumed
            return (-1 == result) ? result : buf.get(0) & 0xFF;
            // END android-changed
        }

        @Override
        public int read(byte[] buffer, int offset, int count) throws IOException {
            if (0 > offset || 0 > count || count + offset > buffer.length) {
                throw new IndexOutOfBoundsException();
            }
            if (!channel.isBlocking()) {
                throw new IllegalBlockingModeException();
            }
            ByteBuffer buf = ByteBuffer.wrap(buffer, offset, count);
            return channel.read(buf);
        }
    }
}
