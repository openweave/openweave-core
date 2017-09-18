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

package dalvik.system;

import java.io.FileDescriptor;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.net.DatagramPacket;
import java.net.InetAddress;
import java.net.SocketException;
import java.net.SocketImpl;
import java.net.SocketOptions;
import org.apache.harmony.luni.platform.IFileSystem;
import org.apache.harmony.luni.platform.INetworkSystem;

/**
 * Mechanism to let threads set restrictions on what code is allowed
 * to do in their thread.
 *
 * <p>This is meant for applications to prevent certain blocking
 * operations from running on their main event loop (or "UI") threads.
 *
 * <p>Note that this is all best-effort to catch most accidental mistakes
 * and isn't intended to be a perfect mechanism, nor provide any sort of
 * security.
 *
 * @hide
 */
public final class BlockGuard {

    public static final int DISALLOW_DISK_WRITE = 0x01;
    public static final int DISALLOW_DISK_READ = 0x02;
    public static final int DISALLOW_NETWORK = 0x04;
    public static final int PASS_RESTRICTIONS_VIA_RPC = 0x08;
    public static final int PENALTY_LOG = 0x10;
    public static final int PENALTY_DIALOG = 0x20;
    public static final int PENALTY_DEATH = 0x40;

    public interface Policy {
        /**
         * Called on disk writes.
         */
        void onWriteToDisk();

        /**
         * Called on disk writes.
         */
        void onReadFromDisk();

        /**
         * Called on network operations.
         */
        void onNetwork();

        /**
         * Returns the policy bitmask, for shipping over Binder calls
         * to remote threads/processes and reinstantiating the policy
         * there.  The bits in the mask are from the DISALLOW_* and
         * PENALTY_* constants.
         */
        int getPolicyMask();
    }

    public static class BlockGuardPolicyException extends RuntimeException {
        // bitmask of DISALLOW_*, PENALTY_*, etc flags
        private final int mPolicyState;
        private final int mPolicyViolated;

        public BlockGuardPolicyException(int policyState, int policyViolated) {
            mPolicyState = policyState;
            mPolicyViolated = policyViolated;
            fillInStackTrace();
        }

        public int getPolicy() {
            return mPolicyState;
        }

        public int getPolicyViolation() {
            return mPolicyViolated;
        }

        public String getMessage() {
            // Note: do not change this format casually.  It's
            // somewhat unfortunately Parceled and passed around
            // Binder calls and parsed back into an Exception by
            // Android's StrictMode.  This was the least invasive
            // option and avoided a gross mix of Java Serialization
            // combined with Parcels.
            return "policy=" + mPolicyState + " violation=" + mPolicyViolated;
        }
    }

    /**
     * The default, permissive policy that doesn't prevent any operations.
     */
    public static Policy LAX_POLICY = new Policy() {
            public void onWriteToDisk() {}
            public void onReadFromDisk() {}
            public void onNetwork() {}
            public int getPolicyMask() {
                return 0;
            }
        };

    private static ThreadLocal<Policy> threadPolicy = new ThreadLocal<Policy>() {
        @Override protected Policy initialValue() {
            return LAX_POLICY;
        }
    };

    /**
     * Get the current thread's policy.
     *
     * @return the current thread's policy.  Never returns null.
     *     Will return the LAX_POLICY instance if nothing else is set.
     */
    public static Policy getThreadPolicy() {
        return threadPolicy.get();
    }

    /**
     * Sets the current thread's block guard policy.
     *
     * @param policy policy to set.  May not be null.  Use the public LAX_POLICY
     *   if you want to unset the active policy.
     */
    public static void setThreadPolicy(Policy policy) {
        if (policy == null) {
            throw new NullPointerException("policy == null");
        }
        threadPolicy.set(policy);
    }

    private BlockGuard() {}

    /**
     * A filesystem wrapper that calls the policy check functions
     * on reads and writes.
     */
    public static class WrappedFileSystem implements IFileSystem {
        private final IFileSystem mFileSystem;

        public WrappedFileSystem(IFileSystem fileSystem) {
            mFileSystem = fileSystem;
        }

        public long read(int fileDescriptor, byte[] bytes, int offset, int length)
                throws IOException {
            BlockGuard.getThreadPolicy().onReadFromDisk();
            return mFileSystem.read(fileDescriptor, bytes, offset, length);
        }

        public long write(int fileDescriptor, byte[] bytes, int offset, int length)
                throws IOException {
            BlockGuard.getThreadPolicy().onWriteToDisk();
            return mFileSystem.write(fileDescriptor, bytes, offset, length);
        }

        public long readv(int fileDescriptor, int[] addresses, int[] offsets,
                          int[] lengths, int size) throws IOException {
            BlockGuard.getThreadPolicy().onReadFromDisk();
            return mFileSystem.readv(fileDescriptor, addresses, offsets, lengths, size);
        }

        public long writev(int fileDescriptor, int[] addresses, int[] offsets,
                           int[] lengths, int size) throws IOException {
            BlockGuard.getThreadPolicy().onWriteToDisk();
            return mFileSystem.writev(fileDescriptor, addresses, offsets, lengths, size);
        }

        public long readDirect(int fileDescriptor, int address, int offset,
                               int length) throws IOException {
            BlockGuard.getThreadPolicy().onReadFromDisk();
            return mFileSystem.readDirect(fileDescriptor, address, offset, length);
        }

        public long writeDirect(int fileDescriptor, int address, int offset,
                                int length) throws IOException {
            BlockGuard.getThreadPolicy().onWriteToDisk();
            return mFileSystem.writeDirect(fileDescriptor, address, offset, length);
        }

        public boolean lock(int fileDescriptor, long start, long length, int type,
                            boolean waitFlag) throws IOException {
            return mFileSystem.lock(fileDescriptor, start, length, type, waitFlag);
        }

        public void unlock(int fileDescriptor, long start, long length)
                throws IOException {
            mFileSystem.unlock(fileDescriptor, start, length);
        }

        public long seek(int fileDescriptor, long offset, int whence)
                throws IOException {
            return mFileSystem.seek(fileDescriptor, offset, whence);
        }

        public void fsync(int fileDescriptor, boolean metadata) throws IOException {
            BlockGuard.getThreadPolicy().onWriteToDisk();
            mFileSystem.fsync(fileDescriptor, metadata);
        }

        public void truncate(int fileDescriptor, long size) throws IOException {
            BlockGuard.getThreadPolicy().onWriteToDisk();
            mFileSystem.truncate(fileDescriptor, size);
        }

        public int getAllocGranularity() throws IOException {
            return mFileSystem.getAllocGranularity();
        }

        public int open(String path, int mode) throws FileNotFoundException {
            BlockGuard.getThreadPolicy().onReadFromDisk();
            if (mode != 0) {  // 0 is read-only
                BlockGuard.getThreadPolicy().onWriteToDisk();
            }
            return mFileSystem.open(path, mode);
        }

        public long transfer(int fileHandler, FileDescriptor socketDescriptor,
                             long offset, long count) throws IOException {
            return mFileSystem.transfer(fileHandler, socketDescriptor, offset, count);
        }

        public int ioctlAvailable(FileDescriptor fileDescriptor) throws IOException {
            return mFileSystem.ioctlAvailable(fileDescriptor);
        }

        public long length(int fd) {
            return mFileSystem.length(fd);
        }
    }

    /**
     * A network wrapper that calls the policy check functions.
     */
    public static class WrappedNetworkSystem implements INetworkSystem {
        private final INetworkSystem mNetwork;

        public WrappedNetworkSystem(INetworkSystem network) {
            mNetwork = network;
        }

        public void accept(FileDescriptor serverFd, SocketImpl newSocket,
                FileDescriptor clientFd) throws IOException {
            BlockGuard.getThreadPolicy().onNetwork();
            mNetwork.accept(serverFd, newSocket, clientFd);
        }


        public void bind(FileDescriptor aFD, InetAddress inetAddress, int port)
                throws SocketException {
            mNetwork.bind(aFD, inetAddress, port);
        }

        public int read(FileDescriptor aFD, byte[] data, int offset, int count) throws IOException {
            BlockGuard.getThreadPolicy().onNetwork();
            return mNetwork.read(aFD, data, offset, count);
        }

        public int readDirect(FileDescriptor aFD, int address, int count) throws IOException {
            BlockGuard.getThreadPolicy().onNetwork();
            return mNetwork.readDirect(aFD, address, count);
        }

        public int write(FileDescriptor fd, byte[] data, int offset, int count)
                throws IOException {
            BlockGuard.getThreadPolicy().onNetwork();
            return mNetwork.write(fd, data, offset, count);
        }

        public int writeDirect(FileDescriptor fd, int address, int offset, int count)
                throws IOException {
            BlockGuard.getThreadPolicy().onNetwork();
            return mNetwork.writeDirect(fd, address, offset, count);
        }

        public boolean connectNonBlocking(FileDescriptor fd, InetAddress inetAddress, int port)
                throws IOException {
            BlockGuard.getThreadPolicy().onNetwork();
            return mNetwork.connectNonBlocking(fd, inetAddress, port);
        }

        public boolean isConnected(FileDescriptor fd, int timeout) throws IOException {
            BlockGuard.getThreadPolicy().onNetwork();
            return mNetwork.isConnected(fd, timeout);
        }

        public int send(FileDescriptor fd, byte[] data, int offset, int length,
                int port, InetAddress inetAddress) throws IOException {
            // Note: no BlockGuard violation.  We permit datagrams
            // without hostname lookups.  (short, bounded amount of time)
            return mNetwork.send(fd, data, offset, length, port, inetAddress);
        }

        public int sendDirect(FileDescriptor fd, int address, int offset, int length,
                int port, InetAddress inetAddress) throws IOException {
            // Note: no BlockGuard violation.  We permit datagrams
            // without hostname lookups.  (short, bounded amount of time)
            return mNetwork.sendDirect(fd, address, offset, length, port, inetAddress);
        }

        public int recv(FileDescriptor fd, DatagramPacket packet, byte[] data, int offset,
                int length, boolean peek, boolean connected) throws IOException {
            BlockGuard.getThreadPolicy().onNetwork();
            return mNetwork.recv(fd, packet, data, offset, length, peek, connected);
        }

        public int recvDirect(FileDescriptor fd, DatagramPacket packet, int address, int offset,
                int length, boolean peek, boolean connected) throws IOException {
            BlockGuard.getThreadPolicy().onNetwork();
            return mNetwork.recvDirect(fd, packet, address, offset, length, peek, connected);
        }

        public void disconnectDatagram(FileDescriptor aFD) throws SocketException {
            mNetwork.disconnectDatagram(aFD);
        }

        public void socket(FileDescriptor aFD, boolean stream) throws SocketException {
            mNetwork.socket(aFD, stream);
        }

        public void shutdownInput(FileDescriptor descriptor) throws IOException {
            mNetwork.shutdownInput(descriptor);
        }

        public void shutdownOutput(FileDescriptor descriptor) throws IOException {
            mNetwork.shutdownOutput(descriptor);
        }

        public void sendUrgentData(FileDescriptor fd, byte value) {
            mNetwork.sendUrgentData(fd, value);
        }

        public void listen(FileDescriptor aFD, int backlog) throws SocketException {
            mNetwork.listen(aFD, backlog);
        }

        public void connect(FileDescriptor aFD, InetAddress inetAddress, int port,
                int timeout) throws SocketException {
            BlockGuard.getThreadPolicy().onNetwork();
            mNetwork.connect(aFD, inetAddress, port, timeout);
        }

        public InetAddress getSocketLocalAddress(FileDescriptor aFD) {
            return mNetwork.getSocketLocalAddress(aFD);
        }

        public boolean select(FileDescriptor[] readFDs, FileDescriptor[] writeFDs,
                int numReadable, int numWritable, long timeout, int[] flags)
                throws SocketException {
            BlockGuard.getThreadPolicy().onNetwork();
            return mNetwork.select(readFDs, writeFDs, numReadable, numWritable, timeout, flags);
        }

        public int getSocketLocalPort(FileDescriptor aFD) {
            return mNetwork.getSocketLocalPort(aFD);
        }

        public Object getSocketOption(FileDescriptor aFD, int opt)
                throws SocketException {
            return mNetwork.getSocketOption(aFD, opt);
        }

        public void setSocketOption(FileDescriptor aFD, int opt, Object optVal)
                throws SocketException {
            mNetwork.setSocketOption(aFD, opt, optVal);
        }

        public void close(FileDescriptor aFD) throws IOException {
            // We exclude sockets without SO_LINGER so that apps can close their network connections
            // in methods like onDestroy, which will run on the UI thread, without jumping through
            // extra hoops.
            if (isLingerSocket(aFD)) {
                BlockGuard.getThreadPolicy().onNetwork();
            }
            mNetwork.close(aFD);
        }

        public void setInetAddress(InetAddress sender, byte[] address) {
            mNetwork.setInetAddress(sender, address);
        }

        private boolean isLingerSocket(FileDescriptor fd) throws SocketException {
            try {
                Object lingerValue = mNetwork.getSocketOption(fd, SocketOptions.SO_LINGER);
                if (lingerValue instanceof Boolean) {
                    return (Boolean) lingerValue;
                } else if (lingerValue instanceof Integer) {
                    return ((Integer) lingerValue) != 0;
                }
                throw new AssertionError(lingerValue.getClass().getName());
            } catch (Exception ignored) {
                // We're called via Socket.close (which doesn't ask for us to be called), so we
                // must not throw here, because Socket.close must not throw if asked to close an
                // already-closed socket.
                return false;
            }
        }
    }
}
