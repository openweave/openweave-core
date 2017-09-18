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

package org.apache.harmony.nio.internal;

import java.io.Closeable;
import java.io.FileDescriptor;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.channels.Pipe;
import java.nio.channels.spi.SelectorProvider;
import libcore.io.IoUtils;
import org.apache.harmony.luni.platform.FileDescriptorHandler;

/*
 * Implements {@link java.nio.channels.Pipe}.
 */
final class PipeImpl extends Pipe {
    private final PipeSinkChannel sink;
    private final PipeSourceChannel source;

    public PipeImpl() throws IOException {
        int[] fds = new int[2];
        IoUtils.pipe(fds);
        // Which fd is used for which channel is important. Unix pipes are only guaranteed to be
        // unidirectional, and indeed are only unidirectional on Linux. See IoUtils.pipe.
        this.sink = new PipeSinkChannel(fds[1]);
        this.source = new PipeSourceChannel(fds[0]);
    }

    @Override public SinkChannel sink() {
        return sink;
    }

    @Override public SourceChannel source() {
        return source;
    }

    /**
     * FileChannelImpl doesn't close its fd itself; it calls close on the object it's given.
     */
    private static class FdCloser implements Closeable {
        private final FileDescriptor fd;
        private FdCloser(FileDescriptor fd) {
            this.fd = fd;
        }
        public void close() throws IOException {
            IoUtils.close(fd);
        }
    }

    private class PipeSourceChannel extends Pipe.SourceChannel implements FileDescriptorHandler {
        private final FileDescriptor fd;
        private final FileChannelImpl channel;

        private PipeSourceChannel(int fd) throws IOException {
            super(SelectorProvider.provider());
            this.fd = IoUtils.newFileDescriptor(fd);
            this.channel = new ReadOnlyFileChannel(new FdCloser(this.fd), fd);
        }

        @Override protected void implCloseSelectableChannel() throws IOException {
            channel.close();
        }

        @Override protected void implConfigureBlocking(boolean blocking) throws IOException {
            IoUtils.setBlocking(getFD(), blocking);
        }

        public int read(ByteBuffer buffer) throws IOException {
            return channel.read(buffer);
        }

        public long read(ByteBuffer[] buffers) throws IOException {
            return channel.read(buffers);
        }

        public long read(ByteBuffer[] buffers, int offset, int length) throws IOException {
            return channel.read(buffers, offset, length);
        }

        public FileDescriptor getFD() {
            return fd;
        }
    }

    private class PipeSinkChannel extends Pipe.SinkChannel implements FileDescriptorHandler {
        private final FileDescriptor fd;
        private final FileChannelImpl channel;

        private PipeSinkChannel(int fd) throws IOException {
            super(SelectorProvider.provider());
            this.fd = IoUtils.newFileDescriptor(fd);
            this.channel = new WriteOnlyFileChannel(new FdCloser(this.fd), fd);
        }

        @Override protected void implCloseSelectableChannel() throws IOException {
            channel.close();
        }

        @Override protected void implConfigureBlocking(boolean blocking) throws IOException {
            IoUtils.setBlocking(getFD(), blocking);
        }

        public int write(ByteBuffer buffer) throws IOException {
            return channel.write(buffer);
        }

        public long write(ByteBuffer[] buffers) throws IOException {
            return channel.write(buffers);
        }

        public long write(ByteBuffer[] buffers, int offset, int length) throws IOException {
            return channel.write(buffers, offset, length);
        }

        public FileDescriptor getFD() {
            return fd;
        }
    }
}
