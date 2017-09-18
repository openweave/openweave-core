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

import java.io.FileDescriptor;
import java.io.IOException;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.SocketAddress;
import java.net.SocketImpl;
import java.net.SocketTimeoutException;
import java.nio.channels.ClosedChannelException;
import java.nio.channels.IllegalBlockingModeException;
import java.nio.channels.NotYetBoundException;
import java.nio.channels.ServerSocketChannel;
import java.nio.channels.SocketChannel;
import java.nio.channels.spi.SelectorProvider;
import org.apache.harmony.luni.net.PlainServerSocketImpl;
import org.apache.harmony.luni.platform.FileDescriptorHandler;
import org.apache.harmony.luni.platform.Platform;

/**
 * The default ServerSocketChannel.
 */
public final class ServerSocketChannelImpl
        extends ServerSocketChannel implements FileDescriptorHandler {

    private final FileDescriptor fd = new FileDescriptor();
    private final SocketImpl impl = new PlainServerSocketImpl(fd);
    private final ServerSocketAdapter socket = new ServerSocketAdapter(impl, this);

    private boolean isBound = false;

    private static class AcceptLock {}
    private final Object acceptLock = new AcceptLock();

    public ServerSocketChannelImpl(SelectorProvider sp) throws IOException {
        super(sp);
    }

    // for native call
    @SuppressWarnings("unused")
    private ServerSocketChannelImpl() throws IOException {
        this(SelectorProvider.provider());
    }

    @Override public ServerSocket socket() {
        return socket;
    }

    @Override public SocketChannel accept() throws IOException {
        if (!isOpen()) {
            throw new ClosedChannelException();
        }
        if (!isBound) {
            throw new NotYetBoundException();
        }

        // TODO: pass in the SelectorProvider used to create this ServerSocketChannelImpl?
        // Create an empty socket channel. This will be populated by ServerSocketAdapter.accept.
        SocketChannelImpl result = new SocketChannelImpl(SelectorProvider.provider(), false);
        Socket resultSocket = result.socket();

        try {
            begin();
            synchronized (acceptLock) {
                synchronized (blockingLock()) {
                    boolean isBlocking = isBlocking();
                    if (!isBlocking) {
                        int[] tryResult = new int[1];
                        boolean success = Platform.getNetworkSystem().select(
                                new FileDescriptor[] { fd },
                                new FileDescriptor[0], 1, 0, 0, tryResult);
                        if (!success || 0 == tryResult[0]) {
                            // no pending connections, returns immediately.
                            return null;
                        }
                    }
                    // do accept.
                    do {
                        try {
                            socket.accept(resultSocket, result);
                            // select successfully, break out immediately.
                            break;
                        } catch (SocketTimeoutException e) {
                            // continue to accept if the channel is in blocking mode.
                        }
                    } while (isBlocking);
                }
            }
        } finally {
            end(resultSocket.isConnected());
        }
        return result;
    }

    protected void implConfigureBlocking(boolean blockingMode) throws IOException {
        // Do nothing here. For real accept() operation in non-blocking mode,
        // it uses INetworkSystem.select. Whether a channel is blocking can be
        // decided by isBlocking() method.
    }

    synchronized protected void implCloseSelectableChannel() throws IOException {
        if (!socket.isClosed()) {
            socket.close();
        }
    }

    public FileDescriptor getFD() {
        return fd;
    }

    private static class ServerSocketAdapter extends ServerSocket {
        private final ServerSocketChannelImpl channelImpl;

        ServerSocketAdapter(SocketImpl impl, ServerSocketChannelImpl aChannelImpl) {
            super(impl);
            this.channelImpl = aChannelImpl;
        }

        @Override public void bind(SocketAddress localAddress, int backlog) throws IOException {
            super.bind(localAddress, backlog);
            channelImpl.isBound = true;
        }

        @Override public Socket accept() throws IOException {
            if (!channelImpl.isBound) {
                throw new IllegalBlockingModeException();
            }
            SocketChannel sc = channelImpl.accept();
            if (null == sc) {
                throw new IllegalBlockingModeException();
            }
            return sc.socket();
        }

        private Socket accept(Socket socket, SocketChannelImpl sockChannel) throws IOException {
            boolean connectOK = false;
            try {
                synchronized (this) {
                    super.implAccept(socket);
                    sockChannel.setConnected();
                    sockChannel.setBound(true);
                    sockChannel.finishAccept();
                }
                SecurityManager sm = System.getSecurityManager();
                if (sm != null) {
                    sm.checkAccept(socket.getInetAddress().getHostAddress(), socket.getPort());
                }
                connectOK = true;
            } finally {
                if (!connectOK) {
                    socket.close();
                }
            }
            return socket;
        }

        @Override public ServerSocketChannel getChannel() {
            return channelImpl;
        }

        @Override public boolean isBound() {
            return channelImpl.isBound;
        }

        @Override public void bind(SocketAddress localAddress) throws IOException {
            super.bind(localAddress);
            channelImpl.isBound = true;
        }

        @Override public void close() throws IOException {
            synchronized (channelImpl) {
                if (channelImpl.isOpen()) {
                    channelImpl.close();
                } else {
                    super.close();
                }
            }
        }
    }
}
