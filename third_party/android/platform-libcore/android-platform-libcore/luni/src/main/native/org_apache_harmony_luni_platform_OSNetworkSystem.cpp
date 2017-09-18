/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "OSNetworkSystem"

#include "AsynchronousSocketCloseMonitor.h"
#include "JNIHelp.h"
#include "JniConstants.h"
#include "JniException.h"
#include "LocalArray.h"
#include "NetFd.h"
#include "NetworkUtilities.h"
#include "ScopedPrimitiveArray.h"
#include "jni.h"
#include "valueOf.h"

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>
#include <unistd.h>

// Temporary hack to build on systems that don't have up-to-date libc headers.
#ifndef IPV6_TCLASS
#ifdef __linux__
#define IPV6_TCLASS 67 // Linux
#else
#define IPV6_TCLASS -1 // BSD(-like); TODO: Something better than this!
#endif
#endif

/*
 * TODO: The multicast code is highly platform-dependent, and for now
 * we just punt on anything but Linux.
 */
#ifdef __linux__
#define ENABLE_MULTICAST
#endif

#define JAVASOCKOPT_IP_MULTICAST_IF 16
#define JAVASOCKOPT_IP_MULTICAST_IF2 31
#define JAVASOCKOPT_IP_MULTICAST_LOOP 18
#define JAVASOCKOPT_IP_TOS 3
#define JAVASOCKOPT_MCAST_JOIN_GROUP 19
#define JAVASOCKOPT_MCAST_LEAVE_GROUP 20
#define JAVASOCKOPT_MULTICAST_TTL 17
#define JAVASOCKOPT_SO_BROADCAST 32
#define JAVASOCKOPT_SO_KEEPALIVE 8
#define JAVASOCKOPT_SO_LINGER 128
#define JAVASOCKOPT_SO_OOBINLINE  4099
#define JAVASOCKOPT_SO_RCVBUF 4098
#define JAVASOCKOPT_SO_TIMEOUT  4102
#define JAVASOCKOPT_SO_REUSEADDR 4
#define JAVASOCKOPT_SO_SNDBUF 4097
#define JAVASOCKOPT_TCP_NODELAY 1

/* constants for OSNetworkSystem_selectImpl */
#define SOCKET_OP_NONE 0
#define SOCKET_OP_READ 1
#define SOCKET_OP_WRITE 2

static struct CachedFields {
    jfieldID iaddr_ipaddress;
    jfieldID integer_class_value;
    jfieldID boolean_class_value;
    jfieldID socketimpl_address;
    jfieldID socketimpl_port;
    jfieldID socketimpl_localport;
    jfieldID dpack_address;
    jfieldID dpack_port;
    jfieldID dpack_length;
} gCachedFields;

/**
 * Returns the port number in a sockaddr_storage structure.
 *
 * @param address the sockaddr_storage structure to get the port from
 *
 * @return the port number, or -1 if the address family is unknown.
 */
static int getSocketAddressPort(sockaddr_storage* ss) {
    switch (ss->ss_family) {
    case AF_INET:
        return ntohs(reinterpret_cast<sockaddr_in*>(ss)->sin_port);
    case AF_INET6:
        return ntohs(reinterpret_cast<sockaddr_in6*>(ss)->sin6_port);
    default:
        return -1;
    }
}

/**
 * Obtain the socket address family from an existing socket.
 *
 * @param socket the file descriptor of the socket to examine
 * @return an integer, the address family of the socket
 */
static int getSocketAddressFamily(int socket) {
    sockaddr_storage ss;
    socklen_t namelen = sizeof(ss);
    int ret = getsockname(socket, reinterpret_cast<sockaddr*>(&ss), &namelen);
    if (ret != 0) {
        return AF_UNSPEC;
    } else {
        return ss.ss_family;
    }
}

// Handles translating between IPv4 and IPv6 addresses so -- where possible --
// we can use either class of address with either an IPv4 or IPv6 socket.
class CompatibleSocketAddress {
public:
    // Constructs an address corresponding to 'ss' that's compatible with 'fd'.
    CompatibleSocketAddress(int fd, const sockaddr_storage& ss, bool mapUnspecified) {
        const int desiredFamily = getSocketAddressFamily(fd);
        if (ss.ss_family == AF_INET6) {
            if (desiredFamily == AF_INET6) {
                // Nothing to do.
                mCompatibleAddress = reinterpret_cast<const sockaddr*>(&ss);
            } else {
                sockaddr_in* sin = reinterpret_cast<sockaddr_in*>(&mTmp);
                const sockaddr_in6* sin6 = reinterpret_cast<const sockaddr_in6*>(&ss);
                memset(sin, 0, sizeof(*sin));
                sin->sin_family = AF_INET;
                sin->sin_port = sin6->sin6_port;
                if (IN6_IS_ADDR_V4COMPAT(&sin6->sin6_addr)) {
                    // We have an IPv6-mapped IPv4 address, but need plain old IPv4.
                    // Unmap the mapped address in ss into an IPv6 address in mTmp.
                    memcpy(&sin->sin_addr.s_addr, &sin6->sin6_addr.s6_addr[12], 4);
                    mCompatibleAddress = reinterpret_cast<const sockaddr*>(&mTmp);
                } else if (IN6_IS_ADDR_LOOPBACK(&sin6->sin6_addr)) {
                    // Translate the IPv6 loopback address to the IPv4 one.
                    sin->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
                    mCompatibleAddress = reinterpret_cast<const sockaddr*>(&mTmp);
                } else {
                    // We can't help you. We return what you gave us, and assume you'll
                    // get a sensible error when you use the address.
                    mCompatibleAddress = reinterpret_cast<const sockaddr*>(&ss);
                }
            }
        } else /* ss.ss_family == AF_INET */ {
            if (desiredFamily == AF_INET) {
                // Nothing to do.
                mCompatibleAddress = reinterpret_cast<const sockaddr*>(&ss);
            } else {
                // We have IPv4 and need IPv6.
                // Map the IPv4 address in ss into an IPv6 address in mTmp.
                const sockaddr_in* sin = reinterpret_cast<const sockaddr_in*>(&ss);
                sockaddr_in6* sin6 = reinterpret_cast<sockaddr_in6*>(&mTmp);
                memset(sin6, 0, sizeof(*sin6));
                sin6->sin6_family = AF_INET6;
                sin6->sin6_port = sin->sin_port;
                // TODO: mapUnspecified was introduced because kernels < 2.6.31 don't allow
                // you to bind to ::ffff:0.0.0.0. When we move to something >= 2.6.31, we
                // should make the code behave as if mapUnspecified were always true, and
                // remove the parameter.
                if (sin->sin_addr.s_addr != 0 || mapUnspecified) {
                    memset(&(sin6->sin6_addr.s6_addr[10]), 0xff, 2);
                }
                memcpy(&sin6->sin6_addr.s6_addr[12], &sin->sin_addr.s_addr, 4);
                mCompatibleAddress = reinterpret_cast<const sockaddr*>(&mTmp);
            }
        }
    }
    // Returns a pointer to an address compatible with the socket.
    const sockaddr* get() const {
        return mCompatibleAddress;
    }
private:
    const sockaddr* mCompatibleAddress;
    sockaddr_storage mTmp;
};

/**
 * Converts an InetAddress object and port number to a native address structure.
 */
static bool inetAddressToSocketAddress(JNIEnv* env, jobject inetAddress,
        int port, sockaddr_storage* ss) {
    // Get the byte array that stores the IP address bytes in the InetAddress.
    if (inetAddress == NULL) {
        jniThrowNullPointerException(env, NULL);
        return false;
    }
    jbyteArray addressBytes =
        reinterpret_cast<jbyteArray>(env->GetObjectField(inetAddress,
            gCachedFields.iaddr_ipaddress));

    return byteArrayToSocketAddress(env, NULL, addressBytes, port, ss);
}

// Converts a number of milliseconds to a timeval.
static timeval toTimeval(long ms) {
    timeval tv;
    tv.tv_sec = ms / 1000;
    tv.tv_usec = (ms - tv.tv_sec*1000) * 1000;
    return tv;
}

// Converts a timeval to a number of milliseconds.
static long toMs(const timeval& tv) {
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

/**
 * Query OS for timestamp.
 * Retrieve the current value of system clock and convert to milliseconds.
 *
 * @param[in] portLibrary The port library.
 *
 * @return 0 on failure, time value in milliseconds on success.
 * @deprecated Use @ref time_hires_clock and @ref time_hires_delta
 *
 * technically, this should return uint64_t since both timeval.tv_sec and
 * timeval.tv_usec are long
 */
static int time_msec_clock() {
    timeval tp;
    struct timezone tzp;
    gettimeofday(&tp, &tzp);
    return toMs(tp);
}

/**
 * Establish a connection to a peer with a timeout.  The member functions are called
 * repeatedly in order to carry out the connect and to allow other tasks to
 * proceed on certain platforms. The caller must first call ConnectHelper::start.
 * if the result is -EINPROGRESS it will then
 * call ConnectHelper::isConnected until either another error or 0 is returned to
 * indicate the connect is complete.  Each time the function should sleep for no
 * more than 'timeout' milliseconds.  If the connect succeeds or an error occurs,
 * the caller must always end the process by calling ConnectHelper::done.
 *
 * Member functions return 0 if no errors occur, otherwise -errno. TODO: use +errno.
 */
class ConnectHelper {
public:
    ConnectHelper(JNIEnv* env) : mEnv(env) {
    }

    int start(NetFd& fd, jobject inetAddr, jint port) {
        sockaddr_storage ss;
        if (!inetAddressToSocketAddress(mEnv, inetAddr, port, &ss)) {
            return -EINVAL; // Bogus, but clearly a failure, and we've already thrown.
        }

        // Set the socket to non-blocking and initiate a connection attempt...
        const CompatibleSocketAddress compatibleAddress(fd.get(), ss, true);
        if (!setBlocking(fd.get(), false) ||
                connect(fd.get(), compatibleAddress.get(), sizeof(sockaddr_storage)) == -1) {
            if (fd.isClosed()) {
                return -EINVAL; // Bogus, but clearly a failure, and we've already thrown.
            }
            if (errno != EINPROGRESS) {
                didFail(fd.get(), -errno);
            }
            return -errno;
        }
        // We connected straight away!
        didConnect(fd.get());
        return 0;
    }

    // Returns 0 if we're connected; -EINPROGRESS if we're still hopeful, -errno if we've failed.
    // 'timeout' the timeout in milliseconds. If timeout is negative, perform a blocking operation.
    int isConnected(int fd, int timeout) {
        timeval passedTimeout(toTimeval(timeout));

        // Initialize the fd sets for the select.
        fd_set readSet;
        fd_set writeSet;
        FD_ZERO(&readSet);
        FD_ZERO(&writeSet);
        FD_SET(fd, &readSet);
        FD_SET(fd, &writeSet);

        int nfds = fd + 1;
        timeval* tp = timeout >= 0 ? &passedTimeout : NULL;
        int rc = select(nfds, &readSet, &writeSet, NULL, tp);
        if (rc == -1) {
            if (errno == EINTR) {
                // We can't trivially retry a select with TEMP_FAILURE_RETRY, so punt and ask the
                // caller to try again.
                return -EINPROGRESS;
            }
            return -errno;
        }

        // If the fd is just in the write set, we're connected.
        if (FD_ISSET(fd, &writeSet) && !FD_ISSET(fd, &readSet)) {
            return 0;
        }

        // If the fd is in both the read and write set, there was an error.
        if (FD_ISSET(fd, &readSet) || FD_ISSET(fd, &writeSet)) {
            // Get the pending error.
            int error = 0;
            socklen_t errorLen = sizeof(error);
            if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &errorLen) == -1) {
                return -errno; // Couldn't get the real error, so report why not.
            }
            return -error;
        }

        // Timeout expired.
        return -EINPROGRESS;
    }

    void didConnect(int fd) {
        if (fd != -1) {
            setBlocking(fd, true);
        }
    }

    void didFail(int fd, int result) {
        if (fd != -1) {
            setBlocking(fd, true);
        }

        if (result == -ECONNRESET || result == -ECONNREFUSED || result == -EADDRNOTAVAIL ||
                result == -EADDRINUSE || result == -ENETUNREACH) {
            jniThrowConnectException(mEnv, -result);
        } else if (result == -EACCES) {
            jniThrowSecurityException(mEnv, -result);
        } else if (result == -ETIMEDOUT) {
            jniThrowSocketTimeoutException(mEnv, -result);
        } else {
            jniThrowSocketException(mEnv, -result);
        }
    }

private:
    JNIEnv* mEnv;
};

#ifdef ENABLE_MULTICAST
static void mcastJoinLeaveGroup(JNIEnv* env, int fd, jobject javaGroupRequest, bool join) {
    group_req groupRequest;

    // Get the IPv4 or IPv6 multicast address to join or leave.
    jfieldID fid = env->GetFieldID(JniConstants::multicastGroupRequestClass,
            "gr_group", "Ljava/net/InetAddress;");
    jobject group = env->GetObjectField(javaGroupRequest, fid);
    if (!inetAddressToSocketAddress(env, group, 0, &groupRequest.gr_group)) {
        return;
    }

    // Get the interface index to use (or 0 for "whatever").
    fid = env->GetFieldID(JniConstants::multicastGroupRequestClass, "gr_interface", "I");
    groupRequest.gr_interface = env->GetIntField(javaGroupRequest, fid);

    int level = groupRequest.gr_group.ss_family == AF_INET ? IPPROTO_IP : IPPROTO_IPV6;
    int option = join ? MCAST_JOIN_GROUP : MCAST_LEAVE_GROUP;
    int rc = setsockopt(fd, level, option, &groupRequest, sizeof(groupRequest));
    if (rc == -1) {
        jniThrowSocketException(env, errno);
        return;
    }
}
#endif // def ENABLE_MULTICAST

static bool initCachedFields(JNIEnv* env) {
    memset(&gCachedFields, 0, sizeof(gCachedFields));
    struct CachedFields* c = &gCachedFields;

    struct fieldInfo {
        jfieldID* field;
        jclass clazz;
        const char* name;
        const char* type;
    } fields[] = {
        {&c->iaddr_ipaddress, JniConstants::inetAddressClass, "ipaddress", "[B"},
        {&c->integer_class_value, JniConstants::integerClass, "value", "I"},
        {&c->boolean_class_value, JniConstants::booleanClass, "value", "Z"},
        {&c->socketimpl_port, JniConstants::socketImplClass, "port", "I"},
        {&c->socketimpl_localport, JniConstants::socketImplClass, "localport", "I"},
        {&c->socketimpl_address, JniConstants::socketImplClass, "address", "Ljava/net/InetAddress;"},
        {&c->dpack_address, JniConstants::datagramPacketClass, "address", "Ljava/net/InetAddress;"},
        {&c->dpack_port, JniConstants::datagramPacketClass, "port", "I"},
        {&c->dpack_length, JniConstants::datagramPacketClass, "length", "I"}
    };
    for (unsigned i = 0; i < sizeof(fields) / sizeof(fields[0]); i++) {
        fieldInfo f = fields[i];
        *f.field = env->GetFieldID(f.clazz, f.name, f.type);
        if (*f.field == NULL) return false;
    }
    return true;
}

static void OSNetworkSystem_socket(JNIEnv* env, jobject, jobject fileDescriptor, jboolean stream) {
    if (fileDescriptor == NULL) {
        jniThrowNullPointerException(env, NULL);
        errno = EBADF;
        return;
    }

    // Try IPv6 but fall back to IPv4...
    int type = stream ? SOCK_STREAM : SOCK_DGRAM;
    int fd = socket(AF_INET6, type, 0);
    if (fd == -1 && errno == EAFNOSUPPORT) {
        fd = socket(AF_INET, type, 0);
    }
    if (fd == -1) {
        jniThrowSocketException(env, errno);
        return;
    } else {
        jniSetFileDescriptorOfFD(env, fileDescriptor, fd);
    }

#ifdef __linux__
    // The RFC (http://www.ietf.org/rfc/rfc3493.txt) says that IPV6_MULTICAST_HOPS defaults to 1.
    // The Linux kernel (at least up to 2.6.32) accidentally defaults to 64 (which would be correct
    // for the *unicast* hop limit). See http://www.spinics.net/lists/netdev/msg129022.html.
    // When that bug is fixed, we can remove this code. Until then, we manually set the hop
    // limit on IPv6 datagram sockets. (IPv4 is already correct.)
    if (type == SOCK_DGRAM && getSocketAddressFamily(fd) == AF_INET6) {
        int ttl = 1;
        setsockopt(fd, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &ttl, sizeof(int));
    }
#endif
}

static jint OSNetworkSystem_writeDirect(JNIEnv* env, jobject,
        jobject fileDescriptor, jint address, jint offset, jint count) {
    if (count <= 0) {
        return 0;
    }

    NetFd fd(env, fileDescriptor);
    if (fd.isClosed()) {
        return 0;
    }

    jbyte* src = reinterpret_cast<jbyte*>(static_cast<uintptr_t>(address + offset));

    ssize_t bytesSent;
    {
        int intFd = fd.get();
        AsynchronousSocketCloseMonitor monitor(intFd);
        bytesSent = NET_FAILURE_RETRY(fd, write(intFd, src, count));
    }
    if (env->ExceptionOccurred()) {
        return -1;
    }

    if (bytesSent == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // We were asked to write to a non-blocking socket, but were told
            // it would block, so report "no bytes written".
            return 0;
        } else {
            jniThrowSocketException(env, errno);
            return 0;
        }
    }
    return bytesSent;
}

static jint OSNetworkSystem_write(JNIEnv* env, jobject,
        jobject fileDescriptor, jbyteArray byteArray, jint offset, jint count) {
    ScopedByteArrayRW bytes(env, byteArray);
    if (bytes.get() == NULL) {
        return -1;
    }
    jint address = static_cast<jint>(reinterpret_cast<uintptr_t>(bytes.get()));
    int result = OSNetworkSystem_writeDirect(env, NULL, fileDescriptor, address, offset, count);
    return result;
}

static jboolean OSNetworkSystem_connectNonBlocking(JNIEnv* env, jobject, jobject fileDescriptor, jobject inetAddr, jint port) {
    NetFd fd(env, fileDescriptor);
    if (fd.isClosed()) {
        return JNI_FALSE;
    }

    ConnectHelper context(env);
    return context.start(fd, inetAddr, port) == 0;
}

static jboolean OSNetworkSystem_isConnected(JNIEnv* env, jobject, jobject fileDescriptor, jint timeout) {
    NetFd fd(env, fileDescriptor);
    if (fd.isClosed()) {
        return JNI_FALSE;
    }

    ConnectHelper context(env);
    int result = context.isConnected(fd.get(), timeout);
    if (result == 0) {
        context.didConnect(fd.get());
        return JNI_TRUE;
    } else if (result == -EINPROGRESS) {
        // Not yet connected, but not yet denied either... Try again later.
        return JNI_FALSE;
    } else {
        context.didFail(fd.get(), result);
        return JNI_FALSE;
    }
}

// TODO: move this into Java, using connectNonBlocking and isConnected!
static void OSNetworkSystem_connect(JNIEnv* env, jobject, jobject fileDescriptor,
        jobject inetAddr, jint port, jint timeout) {

    /* if a timeout was specified calculate the finish time value */
    bool hasTimeout = timeout > 0;
    int finishTime = 0;
    if (hasTimeout)  {
        finishTime = time_msec_clock() + (int) timeout;
    }

    NetFd fd(env, fileDescriptor);
    if (fd.isClosed()) {
        return;
    }

    ConnectHelper context(env);
    int result = context.start(fd, inetAddr, port);
    int remainingTimeout = timeout;
    while (result == -EINPROGRESS) {
        /*
         * ok now try and connect. Depending on the platform this may sleep
         * for up to passedTimeout milliseconds
         */
        result = context.isConnected(fd.get(), remainingTimeout);
        if (fd.isClosed()) {
            return;
        }
        if (result == 0) {
            context.didConnect(fd.get());
            return;
        } else if (result != -EINPROGRESS) {
            context.didFail(fd.get(), result);
            return;
        }

        /* check if the timeout has expired */
        if (hasTimeout) {
            remainingTimeout = finishTime - time_msec_clock();
            if (remainingTimeout <= 0) {
                context.didFail(fd.get(), -ETIMEDOUT);
                return;
            }
        } else {
            remainingTimeout = 100;
        }
    }
}

static void OSNetworkSystem_bind(JNIEnv* env, jobject, jobject fileDescriptor,
        jobject inetAddress, jint port) {
    sockaddr_storage socketAddress;
    if (!inetAddressToSocketAddress(env, inetAddress, port, &socketAddress)) {
        return;
    }

    NetFd fd(env, fileDescriptor);
    if (fd.isClosed()) {
        return;
    }

    const CompatibleSocketAddress compatibleAddress(fd.get(), socketAddress, false);
    int rc = TEMP_FAILURE_RETRY(bind(fd.get(), compatibleAddress.get(), sizeof(sockaddr_storage)));
    if (rc == -1) {
        jniThrowBindException(env, errno);
    }
}

static void OSNetworkSystem_listen(JNIEnv* env, jobject, jobject fileDescriptor, jint backlog) {
    NetFd fd(env, fileDescriptor);
    if (fd.isClosed()) {
        return;
    }

    int rc = listen(fd.get(), backlog);
    if (rc == -1) {
        jniThrowSocketException(env, errno);
    }
}

static void OSNetworkSystem_accept(JNIEnv* env, jobject, jobject serverFileDescriptor,
        jobject newSocket, jobject clientFileDescriptor) {

    if (newSocket == NULL) {
        jniThrowNullPointerException(env, NULL);
        return;
    }

    NetFd serverFd(env, serverFileDescriptor);
    if (serverFd.isClosed()) {
        return;
    }

    sockaddr_storage ss;
    socklen_t addrLen = sizeof(ss);
    sockaddr* sa = reinterpret_cast<sockaddr*>(&ss);

    int clientFd;
    {
        int intFd = serverFd.get();
        AsynchronousSocketCloseMonitor monitor(intFd);
        clientFd = NET_FAILURE_RETRY(serverFd, accept(intFd, sa, &addrLen));
    }
    if (env->ExceptionOccurred()) {
        return;
    }
    if (clientFd == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            jniThrowSocketTimeoutException(env, errno);
        } else {
            jniThrowSocketException(env, errno);
        }
        return;
    }

    // Reset the inherited read timeout to the Java-specified default of 0.
    timeval timeout(toTimeval(0));
    int rc = setsockopt(clientFd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    if (rc == -1) {
        LOGE("couldn't reset SO_RCVTIMEO on accepted socket fd %i: %s", clientFd, strerror(errno));
        jniThrowSocketException(env, errno);
    }

    /*
     * For network sockets, put the peer address and port in instance variables.
     * We don't bother to do this for UNIX domain sockets, since most peers are
     * anonymous anyway.
     */
    if (ss.ss_family == AF_INET || ss.ss_family == AF_INET6) {
        // Remote address and port.
        jobject remoteAddress = socketAddressToInetAddress(env, &ss);
        if (remoteAddress == NULL) {
            close(clientFd);
            return;
        }
        int remotePort = getSocketAddressPort(&ss);
        env->SetObjectField(newSocket, gCachedFields.socketimpl_address, remoteAddress);
        env->SetIntField(newSocket, gCachedFields.socketimpl_port, remotePort);

        // Local port.
        memset(&ss, 0, addrLen);
        int rc = getsockname(clientFd, sa, &addrLen);
        if (rc == -1) {
            close(clientFd);
            jniThrowSocketException(env, errno);
            return;
        }
        int localPort = getSocketAddressPort(&ss);
        env->SetIntField(newSocket, gCachedFields.socketimpl_localport, localPort);
    }

    jniSetFileDescriptorOfFD(env, clientFileDescriptor, clientFd);
}

static void OSNetworkSystem_sendUrgentData(JNIEnv* env, jobject,
        jobject fileDescriptor, jbyte value) {
    NetFd fd(env, fileDescriptor);
    if (fd.isClosed()) {
        return;
    }

    int rc = send(fd.get(), &value, 1, MSG_OOB);
    if (rc == -1) {
        jniThrowSocketException(env, errno);
    }
}

static void OSNetworkSystem_disconnectDatagram(JNIEnv* env, jobject, jobject fileDescriptor) {
    NetFd fd(env, fileDescriptor);
    if (fd.isClosed()) {
        return;
    }

    // To disconnect a datagram socket, we connect to a bogus address with
    // the family AF_UNSPEC.
    sockaddr_storage ss;
    memset(&ss, 0, sizeof(ss));
    ss.ss_family = AF_UNSPEC;
    const sockaddr* sa = reinterpret_cast<const sockaddr*>(&ss);
    int rc = TEMP_FAILURE_RETRY(connect(fd.get(), sa, sizeof(ss)));
    if (rc == -1) {
        jniThrowSocketException(env, errno);
    }
}

static void OSNetworkSystem_setInetAddress(JNIEnv* env, jobject,
        jobject sender, jbyteArray address) {
    env->SetObjectField(sender, gCachedFields.iaddr_ipaddress, address);
}

// TODO: can we merge this with recvDirect?
static jint OSNetworkSystem_readDirect(JNIEnv* env, jobject, jobject fileDescriptor,
        jint address, jint count) {
    NetFd fd(env, fileDescriptor);
    if (fd.isClosed()) {
        return 0;
    }

    jbyte* dst = reinterpret_cast<jbyte*>(static_cast<uintptr_t>(address));
    ssize_t bytesReceived;
    {
        int intFd = fd.get();
        AsynchronousSocketCloseMonitor monitor(intFd);
        bytesReceived = NET_FAILURE_RETRY(fd, read(intFd, dst, count));
    }
    if (env->ExceptionOccurred()) {
        return -1;
    }
    if (bytesReceived == 0) {
        return -1;
    } else if (bytesReceived == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // We were asked to read a non-blocking socket with no data
            // available, so report "no bytes read".
            return 0;
        } else {
            jniThrowSocketException(env, errno);
            return 0;
        }
    } else {
        return bytesReceived;
    }
}

static jint OSNetworkSystem_read(JNIEnv* env, jclass, jobject fileDescriptor,
        jbyteArray byteArray, jint offset, jint count) {
    ScopedByteArrayRW bytes(env, byteArray);
    if (bytes.get() == NULL) {
        return -1;
    }
    jint address = static_cast<jint>(reinterpret_cast<uintptr_t>(bytes.get() + offset));
    return OSNetworkSystem_readDirect(env, NULL, fileDescriptor, address, count);
}

// TODO: can we merge this with readDirect?
static jint OSNetworkSystem_recvDirect(JNIEnv* env, jobject, jobject fileDescriptor, jobject packet,
        jint address, jint offset, jint length, jboolean peek, jboolean connected) {
    NetFd fd(env, fileDescriptor);
    if (fd.isClosed()) {
        return 0;
    }

    char* buf = reinterpret_cast<char*>(static_cast<uintptr_t>(address + offset));
    const int flags = peek ? MSG_PEEK : 0;
    sockaddr_storage ss;
    memset(&ss, 0, sizeof(ss));
    socklen_t sockAddrLen = sizeof(ss);
    sockaddr* from = connected ? NULL : reinterpret_cast<sockaddr*>(&ss);
    socklen_t* fromLength = connected ? NULL : &sockAddrLen;

    ssize_t bytesReceived;
    {
        int intFd = fd.get();
        AsynchronousSocketCloseMonitor monitor(intFd);
        bytesReceived = NET_FAILURE_RETRY(fd, recvfrom(intFd, buf, length, flags, from, fromLength));
    }
    if (env->ExceptionOccurred()) {
        return -1;
    }
    if (bytesReceived == -1) {
        if (connected && errno == ECONNREFUSED) {
            jniThrowException(env, "java/net/PortUnreachableException", "");
        } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
            jniThrowSocketTimeoutException(env, errno);
        } else {
            jniThrowSocketException(env, errno);
        }
        return 0;
    }

    if (packet != NULL) {
        env->SetIntField(packet, gCachedFields.dpack_length, bytesReceived);
        if (!connected) {
            jbyteArray addr = socketAddressToByteArray(env, &ss);
            if (addr == NULL) {
                return 0;
            }
            int port = getSocketAddressPort(&ss);
            jobject sender = byteArrayToInetAddress(env, addr);
            if (sender == NULL) {
                return 0;
            }
            env->SetObjectField(packet, gCachedFields.dpack_address, sender);
            env->SetIntField(packet, gCachedFields.dpack_port, port);
        }
    }
    return bytesReceived;
}

static jint OSNetworkSystem_recv(JNIEnv* env, jobject, jobject fd, jobject packet,
        jbyteArray javaBytes, jint offset, jint length, jboolean peek, jboolean connected) {
    ScopedByteArrayRW bytes(env, javaBytes);
    if (bytes.get() == NULL) {
        return -1;
    }
    uintptr_t address = reinterpret_cast<uintptr_t>(bytes.get());
    return OSNetworkSystem_recvDirect(env, NULL, fd, packet, address, offset, length, peek,
            connected);
}








static jint OSNetworkSystem_sendDirect(JNIEnv* env, jobject, jobject fileDescriptor, jint address, jint offset, jint length, jint port, jobject inetAddress) {
    NetFd fd(env, fileDescriptor);
    if (fd.isClosed()) {
        return -1;
    }

    sockaddr_storage receiver;
    if (inetAddress != NULL && !inetAddressToSocketAddress(env, inetAddress, port, &receiver)) {
        return -1;
    }

    int flags = 0;
    char* buf = reinterpret_cast<char*>(static_cast<uintptr_t>(address + offset));
    sockaddr* to = inetAddress ? reinterpret_cast<sockaddr*>(&receiver) : NULL;
    socklen_t toLength = inetAddress ? sizeof(receiver) : 0;

    ssize_t bytesSent;
    {
        int intFd = fd.get();
        AsynchronousSocketCloseMonitor monitor(intFd);
        bytesSent = NET_FAILURE_RETRY(fd, sendto(intFd, buf, length, flags, to, toLength));
    }
    if (env->ExceptionOccurred()) {
        return -1;
    }
    if (bytesSent == -1) {
        if (errno == ECONNRESET || errno == ECONNREFUSED) {
            return 0;
        } else {
            jniThrowSocketException(env, errno);
        }
    }
    return bytesSent;
}

static jint OSNetworkSystem_send(JNIEnv* env, jobject, jobject fd,
        jbyteArray data, jint offset, jint length,
        jint port, jobject inetAddress) {
    ScopedByteArrayRO bytes(env, data);
    if (bytes.get() == NULL) {
        return -1;
    }
    return OSNetworkSystem_sendDirect(env, NULL, fd,
            reinterpret_cast<uintptr_t>(bytes.get()), offset, length, port, inetAddress);
}








static bool isValidFd(int fd) {
    return fd >= 0 && fd < FD_SETSIZE;
}

static bool initFdSet(JNIEnv* env, jobjectArray fdArray, jint count, fd_set* fdSet, int* maxFd) {
    for (int i = 0; i < count; ++i) {
        jobject fileDescriptor = env->GetObjectArrayElement(fdArray, i);
        if (fileDescriptor == NULL) {
            return false;
        }

        const int fd = jniGetFDFromFileDescriptor(env, fileDescriptor);
        if (!isValidFd(fd)) {
            LOGE("selectImpl: ignoring invalid fd %i", fd);
            continue;
        }

        FD_SET(fd, fdSet);

        if (fd > *maxFd) {
            *maxFd = fd;
        }
    }
    return true;
}

/*
 * Note: fdSet has to be non-const because although on Linux FD_ISSET() is sane
 * and takes a const fd_set*, it takes fd_set* on Mac OS. POSIX is not on our
 * side here:
 *   http://www.opengroup.org/onlinepubs/000095399/functions/select.html
 */
static bool translateFdSet(JNIEnv* env, jobjectArray fdArray, jint count, fd_set& fdSet, jint* flagArray, size_t offset, jint op) {
    for (int i = 0; i < count; ++i) {
        jobject fileDescriptor = env->GetObjectArrayElement(fdArray, i);
        if (fileDescriptor == NULL) {
            return false;
        }

        const int fd = jniGetFDFromFileDescriptor(env, fileDescriptor);
        if (isValidFd(fd) && FD_ISSET(fd, &fdSet)) {
            flagArray[i + offset] = op;
        } else {
            flagArray[i + offset] = SOCKET_OP_NONE;
        }
    }
    return true;
}

static jboolean OSNetworkSystem_selectImpl(JNIEnv* env, jclass,
        jobjectArray readFDArray, jobjectArray writeFDArray, jint countReadC,
        jint countWriteC, jintArray outFlags, jlong timeoutMs) {

    // Initialize the fd_sets.
    int maxFd = -1;
    fd_set readFds;
    fd_set writeFds;
    FD_ZERO(&readFds);
    FD_ZERO(&writeFds);
    bool initialized = initFdSet(env, readFDArray, countReadC, &readFds, &maxFd) &&
                       initFdSet(env, writeFDArray, countWriteC, &writeFds, &maxFd);
    if (!initialized) {
        return -1;
    }

    // Initialize the timeout, if any.
    timeval tv;
    timeval* tvp = NULL;
    if (timeoutMs >= 0) {
        tv = toTimeval(timeoutMs);
        tvp = &tv;
    }

    // Perform the select.
    int result = select(maxFd + 1, &readFds, &writeFds, NULL, tvp);
    if (result == 0) {
        // Timeout.
        return JNI_FALSE;
    } else if (result == -1) {
        // Error.
        if (errno == EINTR) {
            return JNI_FALSE;
        } else {
            jniThrowSocketException(env, errno);
            return JNI_FALSE;
        }
    }

    // Translate the result into the int[] we're supposed to fill in.
    ScopedIntArrayRW flagArray(env, outFlags);
    if (flagArray.get() == NULL) {
        return JNI_FALSE;
    }
    return translateFdSet(env, readFDArray, countReadC, readFds, flagArray.get(), 0, SOCKET_OP_READ) &&
            translateFdSet(env, writeFDArray, countWriteC, writeFds, flagArray.get(), countReadC, SOCKET_OP_WRITE);
}

static jobject OSNetworkSystem_getSocketLocalAddress(JNIEnv* env,
        jobject, jobject fileDescriptor) {
    NetFd fd(env, fileDescriptor);
    if (fd.isClosed()) {
        return NULL;
    }

    sockaddr_storage ss;
    socklen_t ssLen = sizeof(ss);
    memset(&ss, 0, ssLen);
    int rc = getsockname(fd.get(), reinterpret_cast<sockaddr*>(&ss), &ssLen);
    if (rc == -1) {
        // TODO: the public API doesn't allow failure, so this whole method
        // represents a broken design. In practice, though, getsockname can't
        // fail unless we give it invalid arguments.
        LOGE("getsockname failed: %s (errno=%i)", strerror(errno), errno);
        return NULL;
    }
    return socketAddressToInetAddress(env, &ss);
}

static jint OSNetworkSystem_getSocketLocalPort(JNIEnv* env, jobject,
        jobject fileDescriptor) {
    NetFd fd(env, fileDescriptor);
    if (fd.isClosed()) {
        return 0;
    }

    sockaddr_storage ss;
    socklen_t ssLen = sizeof(ss);
    memset(&ss, 0, sizeof(ss));
    int rc = getsockname(fd.get(), reinterpret_cast<sockaddr*>(&ss), &ssLen);
    if (rc == -1) {
        // TODO: the public API doesn't allow failure, so this whole method
        // represents a broken design. In practice, though, getsockname can't
        // fail unless we give it invalid arguments.
        LOGE("getsockname failed: %s (errno=%i)", strerror(errno), errno);
        return 0;
    }
    return getSocketAddressPort(&ss);
}

template <typename T>
static bool getSocketOption(JNIEnv* env, const NetFd& fd, int level, int option, T* value) {
    socklen_t size = sizeof(*value);
    int rc = getsockopt(fd.get(), level, option, value, &size);
    if (rc == -1) {
        LOGE("getSocketOption(fd=%i, level=%i, option=%i) failed: %s (errno=%i)",
                fd.get(), level, option, strerror(errno), errno);
        jniThrowSocketException(env, errno);
        return false;
    }
    return true;
}

static jobject getSocketOption_Boolean(JNIEnv* env, const NetFd& fd, int level, int option) {
    int value;
    return getSocketOption(env, fd, level, option, &value) ? booleanValueOf(env, value) : NULL;
}

static jobject getSocketOption_Integer(JNIEnv* env, const NetFd& fd, int level, int option) {
    int value;
    return getSocketOption(env, fd, level, option, &value) ? integerValueOf(env, value) : NULL;
}

static jobject OSNetworkSystem_getSocketOption(JNIEnv* env, jobject, jobject fileDescriptor, jint option) {
    NetFd fd(env, fileDescriptor);
    if (fd.isClosed()) {
        return NULL;
    }

    int family = getSocketAddressFamily(fd.get());
    if (family != AF_INET && family != AF_INET6) {
        jniThrowSocketException(env, EAFNOSUPPORT);
        return NULL;
    }

    switch (option) {
    case JAVASOCKOPT_TCP_NODELAY:
        return getSocketOption_Boolean(env, fd, IPPROTO_TCP, TCP_NODELAY);
    case JAVASOCKOPT_SO_SNDBUF:
        return getSocketOption_Integer(env, fd, SOL_SOCKET, SO_SNDBUF);
    case JAVASOCKOPT_SO_RCVBUF:
        return getSocketOption_Integer(env, fd, SOL_SOCKET, SO_RCVBUF);
    case JAVASOCKOPT_SO_BROADCAST:
        return getSocketOption_Boolean(env, fd, SOL_SOCKET, SO_BROADCAST);
    case JAVASOCKOPT_SO_REUSEADDR:
        return getSocketOption_Boolean(env, fd, SOL_SOCKET, SO_REUSEADDR);
    case JAVASOCKOPT_SO_KEEPALIVE:
        return getSocketOption_Boolean(env, fd, SOL_SOCKET, SO_KEEPALIVE);
    case JAVASOCKOPT_SO_OOBINLINE:
        return getSocketOption_Boolean(env, fd, SOL_SOCKET, SO_OOBINLINE);
    case JAVASOCKOPT_IP_TOS:
        if (family == AF_INET) {
            return getSocketOption_Integer(env, fd, IPPROTO_IP, IP_TOS);
        } else {
            return getSocketOption_Integer(env, fd, IPPROTO_IPV6, IPV6_TCLASS);
        }
    case JAVASOCKOPT_SO_LINGER:
        {
            linger lingr;
            bool ok = getSocketOption(env, fd, SOL_SOCKET, SO_LINGER, &lingr);
            if (!ok) {
                return NULL; // We already threw.
            } else if (!lingr.l_onoff) {
                return booleanValueOf(env, false);
            } else {
                return integerValueOf(env, lingr.l_linger);
            }
        }
    case JAVASOCKOPT_SO_TIMEOUT:
        {
            timeval timeout;
            bool ok = getSocketOption(env, fd, SOL_SOCKET, SO_RCVTIMEO, &timeout);
            return ok ? integerValueOf(env, toMs(timeout)) : NULL;
        }
#ifdef ENABLE_MULTICAST
    case JAVASOCKOPT_IP_MULTICAST_IF:
        {
            // Although setsockopt(2) can take an ip_mreqn for IP_MULTICAST_IF, getsockopt(2)
            // always returns an in_addr.
            sockaddr_storage ss;
            memset(&ss, 0, sizeof(ss));
            ss.ss_family = AF_INET; // This call is IPv4-only.
            sockaddr_in* sa = reinterpret_cast<sockaddr_in*>(&ss);
            if (!getSocketOption(env, fd, IPPROTO_IP, IP_MULTICAST_IF, &sa->sin_addr)) {
                return NULL;
            }
            return socketAddressToInetAddress(env, &ss);
        }
    case JAVASOCKOPT_IP_MULTICAST_IF2:
        if (family == AF_INET) {
            // The caller's asking for an interface index, but that's not how IPv4 works.
            // Our Java should never get here, because we'll try IP_MULTICAST_IF first and
            // that will satisfy us.
            jniThrowSocketException(env, EAFNOSUPPORT);
        } else {
            return getSocketOption_Integer(env, fd, IPPROTO_IPV6, IPV6_MULTICAST_IF);
        }
    case JAVASOCKOPT_IP_MULTICAST_LOOP:
        if (family == AF_INET) {
            // Although IPv6 was cleaned up to use int, IPv4 multicast loopback uses a byte.
            u_char loopback;
            bool ok = getSocketOption(env, fd, IPPROTO_IP, IP_MULTICAST_LOOP, &loopback);
            return ok ? booleanValueOf(env, loopback) : NULL;
        } else {
            return getSocketOption_Boolean(env, fd, IPPROTO_IPV6, IPV6_MULTICAST_LOOP);
        }
    case JAVASOCKOPT_MULTICAST_TTL:
        if (family == AF_INET) {
            // Although IPv6 was cleaned up to use int, and IPv4 non-multicast TTL uses int,
            // IPv4 multicast TTL uses a byte.
            u_char ttl;
            bool ok = getSocketOption(env, fd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl);
            return ok ? integerValueOf(env, ttl) : NULL;
        } else {
            return getSocketOption_Integer(env, fd, IPPROTO_IPV6, IPV6_MULTICAST_HOPS);
        }
#else
    case JAVASOCKOPT_MULTICAST_TTL:
    case JAVASOCKOPT_IP_MULTICAST_IF:
    case JAVASOCKOPT_IP_MULTICAST_IF2:
    case JAVASOCKOPT_IP_MULTICAST_LOOP:
        jniThrowException(env, "java/lang/UnsupportedOperationException", NULL);
        return NULL;
#endif // def ENABLE_MULTICAST
    default:
        jniThrowSocketException(env, ENOPROTOOPT);
        return NULL;
    }
}

template <typename T>
static void setSocketOption(JNIEnv* env, const NetFd& fd, int level, int option, T* value) {
    int rc = setsockopt(fd.get(), level, option, value, sizeof(*value));
    if (rc == -1) {
        LOGE("setSocketOption(fd=%i, level=%i, option=%i) failed: %s (errno=%i)",
                fd.get(), level, option, strerror(errno), errno);
        jniThrowSocketException(env, errno);
    }
}

static void OSNetworkSystem_setSocketOption(JNIEnv* env, jobject, jobject fileDescriptor, jint option, jobject optVal) {
    NetFd fd(env, fileDescriptor);
    if (fd.isClosed()) {
        return;
    }

    int intVal;
    bool wasBoolean = false;
    if (env->IsInstanceOf(optVal, JniConstants::integerClass)) {
        intVal = (int) env->GetIntField(optVal, gCachedFields.integer_class_value);
    } else if (env->IsInstanceOf(optVal, JniConstants::booleanClass)) {
        intVal = (int) env->GetBooleanField(optVal, gCachedFields.boolean_class_value);
        wasBoolean = true;
    } else if (env->IsInstanceOf(optVal, JniConstants::inetAddressClass)) {
        // We use optVal directly as an InetAddress for IP_MULTICAST_IF.
    } else if (env->IsInstanceOf(optVal, JniConstants::multicastGroupRequestClass)) {
        // We use optVal directly as a MulticastGroupRequest for MCAST_JOIN_GROUP/MCAST_LEAVE_GROUP.
    } else {
        jniThrowSocketException(env, EINVAL);
        return;
    }

    int family = getSocketAddressFamily(fd.get());
    if (family != AF_INET && family != AF_INET6) {
        jniThrowSocketException(env, EAFNOSUPPORT);
        return;
    }

    // Since we expect to have a AF_INET6 socket even if we're communicating via IPv4, we always
    // set the IPPROTO_IP options. As long as we fall back to creating IPv4 sockets if creating
    // an IPv6 socket fails, we need to make setting the IPPROTO_IPV6 options conditional.
    switch (option) {
    case JAVASOCKOPT_IP_TOS:
        setSocketOption(env, fd, IPPROTO_IP, IP_TOS, &intVal);
        if (family == AF_INET6) {
            setSocketOption(env, fd, IPPROTO_IPV6, IPV6_TCLASS, &intVal);
        }
        return;
    case JAVASOCKOPT_SO_BROADCAST:
        setSocketOption(env, fd, SOL_SOCKET, SO_BROADCAST, &intVal);
        return;
    case JAVASOCKOPT_SO_KEEPALIVE:
        setSocketOption(env, fd, SOL_SOCKET, SO_KEEPALIVE, &intVal);
        return;
    case JAVASOCKOPT_SO_LINGER:
        {
            linger l;
            l.l_onoff = !wasBoolean;
            l.l_linger = intVal <= 65535 ? intVal : 65535;
            setSocketOption(env, fd, SOL_SOCKET, SO_LINGER, &l);
            return;
        }
    case JAVASOCKOPT_SO_OOBINLINE:
        setSocketOption(env, fd, SOL_SOCKET, SO_OOBINLINE, &intVal);
        return;
    case JAVASOCKOPT_SO_RCVBUF:
        setSocketOption(env, fd, SOL_SOCKET, SO_RCVBUF, &intVal);
        return;
    case JAVASOCKOPT_SO_REUSEADDR:
        setSocketOption(env, fd, SOL_SOCKET, SO_REUSEADDR, &intVal);
        return;
    case JAVASOCKOPT_SO_SNDBUF:
        setSocketOption(env, fd, SOL_SOCKET, SO_SNDBUF, &intVal);
        return;
    case JAVASOCKOPT_SO_TIMEOUT:
        {
            timeval timeout(toTimeval(intVal));
            setSocketOption(env, fd, SOL_SOCKET, SO_RCVTIMEO, &timeout);
            return;
        }
    case JAVASOCKOPT_TCP_NODELAY:
        setSocketOption(env, fd, IPPROTO_TCP, TCP_NODELAY, &intVal);
        return;
#ifdef ENABLE_MULTICAST
    case JAVASOCKOPT_MCAST_JOIN_GROUP:
        mcastJoinLeaveGroup(env, fd.get(), optVal, true);
        return;
    case JAVASOCKOPT_MCAST_LEAVE_GROUP:
        mcastJoinLeaveGroup(env, fd.get(), optVal, false);
        return;
    case JAVASOCKOPT_IP_MULTICAST_IF:
        {
            sockaddr_storage sockVal;
            if (!env->IsInstanceOf(optVal, JniConstants::inetAddressClass) ||
                    !inetAddressToSocketAddress(env, optVal, 0, &sockVal)) {
                return;
            }
            // This call is IPv4 only. The socket may be IPv6, but the address
            // that identifies the interface to join must be an IPv4 address.
            if (sockVal.ss_family != AF_INET) {
                jniThrowSocketException(env, EAFNOSUPPORT);
                return;
            }
            ip_mreqn mcast_req;
            memset(&mcast_req, 0, sizeof(mcast_req));
            mcast_req.imr_address = reinterpret_cast<sockaddr_in*>(&sockVal)->sin_addr;
            setSocketOption(env, fd, IPPROTO_IP, IP_MULTICAST_IF, &mcast_req);
            return;
        }
    case JAVASOCKOPT_IP_MULTICAST_IF2:
        // TODO: is this right? should we unconditionally set the IPPROTO_IP state in case
        // we have an IPv6 socket communicating via IPv4?
        if (family == AF_INET) {
            // IP_MULTICAST_IF expects a pointer to an ip_mreqn struct.
            ip_mreqn multicastRequest;
            memset(&multicastRequest, 0, sizeof(multicastRequest));
            multicastRequest.imr_ifindex = intVal;
            setSocketOption(env, fd, IPPROTO_IP, IP_MULTICAST_IF, &multicastRequest);
        } else {
            // IPV6_MULTICAST_IF expects a pointer to an integer.
            setSocketOption(env, fd, IPPROTO_IPV6, IPV6_MULTICAST_IF, &intVal);
        }
        return;
    case JAVASOCKOPT_MULTICAST_TTL:
        {
            // Although IPv6 was cleaned up to use int, and IPv4 non-multicast TTL uses int,
            // IPv4 multicast TTL uses a byte.
            u_char ttl = intVal;
            setSocketOption(env, fd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl);
            if (family == AF_INET6) {
                setSocketOption(env, fd, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &intVal);
            }
            return;
        }
    case JAVASOCKOPT_IP_MULTICAST_LOOP:
        {
            // Although IPv6 was cleaned up to use int, IPv4 multicast loopback uses a byte.
            u_char loopback = intVal;
            setSocketOption(env, fd, IPPROTO_IP, IP_MULTICAST_LOOP, &loopback);
            if (family == AF_INET6) {
                setSocketOption(env, fd, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, &intVal);
            }
            return;
        }
#else
    case JAVASOCKOPT_MULTICAST_TTL:
    case JAVASOCKOPT_MCAST_JOIN_GROUP:
    case JAVASOCKOPT_MCAST_LEAVE_GROUP:
    case JAVASOCKOPT_IP_MULTICAST_IF:
    case JAVASOCKOPT_IP_MULTICAST_IF2:
    case JAVASOCKOPT_IP_MULTICAST_LOOP:
        jniThrowException(env, "java/lang/UnsupportedOperationException", NULL);
        return;
#endif // def ENABLE_MULTICAST
    default:
        jniThrowSocketException(env, ENOPROTOOPT);
    }
}

static void doShutdown(JNIEnv* env, jobject fileDescriptor, int how) {
    NetFd fd(env, fileDescriptor);
    if (fd.isClosed()) {
        return;
    }
    int rc = shutdown(fd.get(), how);
    if (rc == -1) {
        jniThrowSocketException(env, errno);
    }
}

static void OSNetworkSystem_shutdownInput(JNIEnv* env, jobject, jobject fd) {
    doShutdown(env, fd, SHUT_RD);
}

static void OSNetworkSystem_shutdownOutput(JNIEnv* env, jobject, jobject fd) {
    doShutdown(env, fd, SHUT_WR);
}

static void OSNetworkSystem_close(JNIEnv* env, jobject, jobject fileDescriptor) {
    NetFd fd(env, fileDescriptor);
    if (fd.isClosed()) {
        return;
    }

    int oldFd = fd.get();
    jniSetFileDescriptorOfFD(env, fileDescriptor, -1);
    AsynchronousSocketCloseMonitor::signalBlockedThreads(oldFd);
    close(oldFd);
}

static JNINativeMethod gMethods[] = {
    NATIVE_METHOD(OSNetworkSystem, accept, "(Ljava/io/FileDescriptor;Ljava/net/SocketImpl;Ljava/io/FileDescriptor;)V"),
    NATIVE_METHOD(OSNetworkSystem, bind, "(Ljava/io/FileDescriptor;Ljava/net/InetAddress;I)V"),
    NATIVE_METHOD(OSNetworkSystem, close, "(Ljava/io/FileDescriptor;)V"),
    NATIVE_METHOD(OSNetworkSystem, connectNonBlocking, "(Ljava/io/FileDescriptor;Ljava/net/InetAddress;I)Z"),
    NATIVE_METHOD(OSNetworkSystem, connect, "(Ljava/io/FileDescriptor;Ljava/net/InetAddress;II)V"),
    NATIVE_METHOD(OSNetworkSystem, disconnectDatagram, "(Ljava/io/FileDescriptor;)V"),
    NATIVE_METHOD(OSNetworkSystem, getSocketLocalAddress, "(Ljava/io/FileDescriptor;)Ljava/net/InetAddress;"),
    NATIVE_METHOD(OSNetworkSystem, getSocketLocalPort, "(Ljava/io/FileDescriptor;)I"),
    NATIVE_METHOD(OSNetworkSystem, getSocketOption, "(Ljava/io/FileDescriptor;I)Ljava/lang/Object;"),
    NATIVE_METHOD(OSNetworkSystem, isConnected, "(Ljava/io/FileDescriptor;I)Z"),
    NATIVE_METHOD(OSNetworkSystem, listen, "(Ljava/io/FileDescriptor;I)V"),
    NATIVE_METHOD(OSNetworkSystem, read, "(Ljava/io/FileDescriptor;[BII)I"),
    NATIVE_METHOD(OSNetworkSystem, readDirect, "(Ljava/io/FileDescriptor;II)I"),
    NATIVE_METHOD(OSNetworkSystem, recv, "(Ljava/io/FileDescriptor;Ljava/net/DatagramPacket;[BIIZZ)I"),
    NATIVE_METHOD(OSNetworkSystem, recvDirect, "(Ljava/io/FileDescriptor;Ljava/net/DatagramPacket;IIIZZ)I"),
    NATIVE_METHOD(OSNetworkSystem, selectImpl, "([Ljava/io/FileDescriptor;[Ljava/io/FileDescriptor;II[IJ)Z"),
    NATIVE_METHOD(OSNetworkSystem, send, "(Ljava/io/FileDescriptor;[BIIILjava/net/InetAddress;)I"),
    NATIVE_METHOD(OSNetworkSystem, sendDirect, "(Ljava/io/FileDescriptor;IIIILjava/net/InetAddress;)I"),
    NATIVE_METHOD(OSNetworkSystem, sendUrgentData, "(Ljava/io/FileDescriptor;B)V"),
    NATIVE_METHOD(OSNetworkSystem, setInetAddress, "(Ljava/net/InetAddress;[B)V"),
    NATIVE_METHOD(OSNetworkSystem, setSocketOption, "(Ljava/io/FileDescriptor;ILjava/lang/Object;)V"),
    NATIVE_METHOD(OSNetworkSystem, shutdownInput, "(Ljava/io/FileDescriptor;)V"),
    NATIVE_METHOD(OSNetworkSystem, shutdownOutput, "(Ljava/io/FileDescriptor;)V"),
    NATIVE_METHOD(OSNetworkSystem, socket, "(Ljava/io/FileDescriptor;Z)V"),
    NATIVE_METHOD(OSNetworkSystem, write, "(Ljava/io/FileDescriptor;[BII)I"),
    NATIVE_METHOD(OSNetworkSystem, writeDirect, "(Ljava/io/FileDescriptor;III)I"),
};

int register_org_apache_harmony_luni_platform_OSNetworkSystem(JNIEnv* env) {
    AsynchronousSocketCloseMonitor::init();
    return initCachedFields(env) && jniRegisterNativeMethods(env,
            "org/apache/harmony/luni/platform/OSNetworkSystem", gMethods, NELEM(gMethods));
}
