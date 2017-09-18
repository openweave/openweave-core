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

package java.net;

import dalvik.system.BlockGuard;
import java.io.FileDescriptor;
import java.io.IOException;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.io.ObjectStreamException;
import java.io.ObjectStreamField;
import java.io.Serializable;
import java.security.AccessController;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.Enumeration;
import java.util.List;
import org.apache.harmony.luni.platform.INetworkSystem;
import org.apache.harmony.luni.platform.Platform;
import org.apache.harmony.luni.util.PriviAction;

/**
 * An Internet Protocol (IP) address. This can be either an IPv4 address or an IPv6 address, and
 * in practice you'll have an instance of either {@code Inet4Address} or {@code Inet6Address} (this
 * class cannot be instantiated directly). Most code does not need to distinguish between the two
 * families, and should use {@code InetAddress}.
 *
 * <p>An {@code InetAddress} may have a hostname (accessible via {@code getHostName}), but may not,
 * depending on how the {@code InetAddress} was created.
 *
 * <h4>IPv4 numeric address formats</h4>
 * <p>The {@code getAllByName} method accepts IPv4 addresses in the following forms:
 * <ul>
 * <li>{@code "1.2.3.4"} - 1.2.3.4
 * <li>{@code "1.2.3"} - 1.2.0.3
 * <li>{@code "1.2"} - 1.0.0.2
 * <li>{@code "16909060"} - 1.2.3.4
 * </ul>
 * <p>In the first three cases, each number is treated as an 8-bit value between 0 and 255.
 * In the fourth case, the single number is treated as a 32-bit value representing the entire
 * address.
 * <p>Note that each numeric part can be expressed in decimal (as above) or hex. For example,
 * {@code "0x01020304"} is equivalent to 1.2.3.4 and {@code "0xa.0xb.0xc.0xd"} is equivalent
 * to 10.11.12.13.
 *
 * <p>Typically, only the four-dot decimal form ({@code "1.2.3.4"}) is ever used. Any method that
 * <i>returns</i> a textual numeric address will use four-dot decimal form.
 *
 * <h4>IPv6 numeric address formats</h4>
 * <p>The {@code getAllByName} method accepts IPv6 addresses in the following forms (this text
 * comes from <a href="http://www.ietf.org/rfc/rfc2373.txt">RFC 2373</a>, which you should consult
 * for full details of IPv6 addressing):
 * <ul>
 * <li><p>The preferred form is {@code x:x:x:x:x:x:x:x}, where the 'x's are the
 * hexadecimal values of the eight 16-bit pieces of the address.
 * Note that it is not necessary to write the leading zeros in an
 * individual field, but there must be at least one numeral in every
 * field (except for the case described in the next bullet).
 * Examples:
 * <pre>
 *     FEDC:BA98:7654:3210:FEDC:BA98:7654:3210
 *     1080:0:0:0:8:800:200C:417A</pre>
 * </li>
 * <li>Due to some methods of allocating certain styles of IPv6
 * addresses, it will be common for addresses to contain long strings
 * of zero bits.  In order to make writing addresses containing zero
 * bits easier a special syntax is available to compress the zeros.
 * The use of "::" indicates multiple groups of 16-bits of zeros.
 * The "::" can only appear once in an address.  The "::" can also be
 * used to compress the leading and/or trailing zeros in an address.
 *
 * For example the following addresses:
 * <pre>
 *     1080:0:0:0:8:800:200C:417A  a unicast address
 *     FF01:0:0:0:0:0:0:101        a multicast address
 *     0:0:0:0:0:0:0:1             the loopback address
 *     0:0:0:0:0:0:0:0             the unspecified addresses</pre>
 * may be represented as:
 * <pre>
 *     1080::8:800:200C:417A       a unicast address
 *     FF01::101                   a multicast address
 *     ::1                         the loopback address
 *     ::                          the unspecified addresses</pre>
 * </li>
 * <li><p>An alternative form that is sometimes more convenient when dealing
 * with a mixed environment of IPv4 and IPv6 nodes is
 * {@code x:x:x:x:x:x:d.d.d.d}, where the 'x's are the hexadecimal values of
 * the six high-order 16-bit pieces of the address, and the 'd's are
 * the decimal values of the four low-order 8-bit pieces of the
 * address (standard IPv4 representation).  Examples:
 * <pre>
 *     0:0:0:0:0:0:13.1.68.3
 *     0:0:0:0:0:FFFF:129.144.52.38</pre>
 * or in compressed form:
 * <pre>
 *     ::13.1.68.3
 *     ::FFFF:129.144.52.38</pre>
 * </li>
 * </ul>
 * <p>Scopes are given using a trailing {@code %} followed by the scope id, as in
 * {@code 1080::8:800:200C:417A%2} or {@code 1080::8:800:200C:417A%en0}.
 * See <a href="https://www.ietf.org/rfc/rfc4007.txt">RFC 4007</a> for more on IPv6's scoped
 * address architecture.
 *
 * <h4>DNS caching</h4>
 * <p>On Android, addresses are cached for 600 seconds (10 minutes) by default. Failed lookups are
 * cached for 10 seconds. The underlying C library or OS may cache for longer, but you can control
 * the Java-level caching with the usual {@code "networkaddress.cache.ttl"} and
 * {@code "networkaddress.cache.negative.ttl"} system properties. These are parsed as integer
 * numbers of seconds, where the special value 0 means "don't cache" and -1 means "cache forever".
 *
 * <p>Note also that on Android &ndash; unlike the RI &ndash; the cache is not unbounded. The
 * current implementation caches around 512 entries, removed on a least-recently-used basis.
 * (Obviously, you should not rely on these details.)
 *
 * @see Inet4Address
 * @see Inet6Address
 */
public class InetAddress implements Serializable {
    /** Our Java-side DNS cache. */
    private static final AddressCache addressCache = new AddressCache();

    private final static INetworkSystem NETIMPL = Platform.getNetworkSystem();

    private static final String ERRMSG_CONNECTION_REFUSED = "Connection refused";

    private static final long serialVersionUID = 3286316764910316507L;

    String hostName;

    private static class WaitReachable {
    }

    private transient Object waitReachable = new WaitReachable();

    private boolean reached;

    private int addrCount;

    int family = 0;

    byte[] ipaddress;

    /**
     * Constructs an {@code InetAddress}.
     *
     * Note: this constructor should not be used. Creating an InetAddress
     * without specifying whether it's an IPv4 or IPv6 address does not make
     * sense, because subsequent code cannot know which of of the subclasses'
     * methods need to be called to implement a given InetAddress method. The
     * proper way to create an InetAddress is to call new Inet4Address or
     * Inet6Address or to use one of the static methods that return
     * InetAddresses (e.g., getByAddress). That is why the API does not have
     * public constructors for any of these classes.
     */
    InetAddress() {}

    /**
     * Compares this {@code InetAddress} instance against the specified address
     * in {@code obj}. Two addresses are equal if their address byte arrays have
     * the same length and if the bytes in the arrays are equal.
     *
     * @param obj
     *            the object to be tested for equality.
     * @return {@code true} if both objects are equal, {@code false} otherwise.
     */
    @Override
    public boolean equals(Object obj) {
        if (!(obj instanceof InetAddress)) {
            return false;
        }
        return Arrays.equals(this.ipaddress, ((InetAddress) obj).ipaddress);
    }

    /**
     * Returns the IP address represented by this {@code InetAddress} instance
     * as a byte array. The elements are in network order (the highest order
     * address byte is in the zeroth element).
     *
     * @return the address in form of a byte array.
     */
    public byte[] getAddress() {
        return ipaddress.clone();
    }

    static final Comparator<byte[]> SHORTEST_FIRST = new Comparator<byte[]>() {
        public int compare(byte[] a1, byte[] a2) {
            return a1.length - a2.length;
        }
    };

    /**
     * Converts an array of byte arrays representing raw IP addresses of a host
     * to an array of InetAddress objects, sorting to respect the value of the
     * system property {@code "java.net.preferIPv6Addresses"}.
     *
     * @param rawAddresses the raw addresses to convert.
     * @param hostName the hostname corresponding to the IP address.
     * @return the corresponding InetAddresses, appropriately sorted.
     */
    static InetAddress[] bytesToInetAddresses(byte[][] rawAddresses, String hostName) {
        // If we prefer IPv4, ignore the RFC3484 ordering we get from getaddrinfo
        // and always put IPv4 addresses first. Arrays.sort() is stable, so the
        // internal ordering will not be changed.
        if (!preferIPv6Addresses()) {
            Arrays.sort(rawAddresses, SHORTEST_FIRST);
        }

        // Convert the byte arrays to InetAddresses.
        InetAddress[] returnedAddresses = new InetAddress[rawAddresses.length];
        for (int i = 0; i < rawAddresses.length; i++) {
            byte[] rawAddress = rawAddresses[i];
            if (rawAddress.length == 16) {
                returnedAddresses[i] = new Inet6Address(rawAddress, hostName);
            } else if (rawAddress.length == 4) {
                returnedAddresses[i] = new Inet4Address(rawAddress, hostName);
            } else {
              // Cannot happen, because the underlying code only returns
              // addresses that are 4 or 16 bytes long.
              throw new AssertionError("Impossible address length " +
                                       rawAddress.length);
            }
        }
        return returnedAddresses;
    }

    /**
     * Gets all IP addresses associated with the given {@code host} identified
     * by name or literal IP address. The IP address is resolved by the
     * configured name service. If the host name is empty or {@code null} an
     * {@code UnknownHostException} is thrown. If the host name is a literal IP
     * address string an array with the corresponding single {@code InetAddress}
     * is returned.
     *
     * @param host the hostname or literal IP string to be resolved.
     * @return the array of addresses associated with the specified host.
     * @throws UnknownHostException if the address lookup fails.
     */
    public static InetAddress[] getAllByName(String host) throws UnknownHostException {
        return getAllByNameImpl(host).clone();
    }

    /**
     * Returns the InetAddresses for {@code host}. The returned array is shared
     * and must be cloned before it is returned to application code.
     */
    static InetAddress[] getAllByNameImpl(String host) throws UnknownHostException {
        if (host == null || host.isEmpty()) {
            if (preferIPv6Addresses()) {
                return new InetAddress[] { Inet6Address.LOOPBACK, Inet4Address.LOOPBACK };
            } else {
                return new InetAddress[] { Inet4Address.LOOPBACK, Inet6Address.LOOPBACK };
            }
        }

        // Special-case "0" for legacy IPv4 applications.
        if (host.equals("0")) {
            return new InetAddress[] { Inet4Address.ANY };
        }

        try {
            byte[] hBytes = ipStringToByteArray(host);
            if (hBytes.length == 4) {
                return (new InetAddress[] { new Inet4Address(hBytes) });
            } else if (hBytes.length == 16) {
                return (new InetAddress[] { new Inet6Address(hBytes) });
            } else {
                throw new UnknownHostException(wrongAddressLength());
            }
        } catch (UnknownHostException e) {
        }

        SecurityManager security = System.getSecurityManager();
        if (security != null) {
            security.checkConnect(host, -1);
        }

        return lookupHostByName(host);
    }

    private static native String byteArrayToIpString(byte[] address);

    static native byte[] ipStringToByteArray(String address) throws UnknownHostException;

    private static String wrongAddressLength() {
        return "Invalid IP Address is neither 4 or 16 bytes";
    }

    static boolean preferIPv6Addresses() {
        String propertyName = "java.net.preferIPv6Addresses";
        String propertyValue = AccessController.doPrivileged(new PriviAction<String>(propertyName));
        return Boolean.parseBoolean(propertyValue);
    }

    /**
     * Returns the address of a host according to the given host string name
     * {@code host}. The host string may be either a machine name or a dotted
     * string IP address. If the latter, the {@code hostName} field is
     * determined upon demand. {@code host} can be {@code null} which means that
     * an address of the loopback interface is returned.
     *
     * @param host
     *            the hostName to be resolved to an address or {@code null}.
     * @return the {@code InetAddress} instance representing the host.
     * @throws UnknownHostException
     *             if the address lookup fails.
     */
    public static InetAddress getByName(String host) throws UnknownHostException {
        return getAllByNameImpl(host)[0];
    }

    /**
     * Gets the textual representation of this IP address.
     *
     * @return the textual representation of host's IP address.
     */
    public String getHostAddress() {
        return byteArrayToIpString(ipaddress);
    }

    /**
     * Gets the host name of this IP address. If the IP address could not be
     * resolved, the textual representation in a dotted-quad-notation is
     * returned.
     *
     * @return the corresponding string name of this IP address.
     */
    public String getHostName() {
        try {
            if (hostName == null) {
                int address = 0;
                if (ipaddress.length == 4) {
                    address = bytesToInt(ipaddress, 0);
                    if (address == 0) {
                        return hostName = byteArrayToIpString(ipaddress);
                    }
                }
                hostName = getHostByAddrImpl(ipaddress).hostName;
                if (hostName.equals("localhost") && ipaddress.length == 4
                        && address != 0x7f000001) {
                    return hostName = byteArrayToIpString(ipaddress);
                }
            }
        } catch (UnknownHostException e) {
            return hostName = byteArrayToIpString(ipaddress);
        }
        SecurityManager security = System.getSecurityManager();
        try {
            // Only check host names, not addresses
            if (security != null && isHostName(hostName)) {
                security.checkConnect(hostName, -1);
            }
        } catch (SecurityException e) {
            return byteArrayToIpString(ipaddress);
        }
        return hostName;
    }

    /**
     * Gets the fully qualified domain name for the host associated with this IP
     * address. If a security manager is set, it is checked if the method caller
     * is allowed to get the hostname. Otherwise, the textual representation in
     * a dotted-quad-notation is returned.
     *
     * @return the fully qualified domain name of this IP address.
     */
    public String getCanonicalHostName() {
        String canonicalName;
        try {
            int address = 0;
            if (ipaddress.length == 4) {
                address = bytesToInt(ipaddress, 0);
                if (address == 0) {
                    return byteArrayToIpString(ipaddress);
                }
            }
            canonicalName = getHostByAddrImpl(ipaddress).hostName;
        } catch (UnknownHostException e) {
            return byteArrayToIpString(ipaddress);
        }
        SecurityManager security = System.getSecurityManager();
        try {
            // Only check host names, not addresses
            if (security != null && isHostName(canonicalName)) {
                security.checkConnect(canonicalName, -1);
            }
        } catch (SecurityException e) {
            return byteArrayToIpString(ipaddress);
        }
        return canonicalName;
    }

    /**
     * Returns an {@code InetAddress} for the local host if possible, or the
     * loopback address otherwise. This method works by getting the hostname,
     * performing a DNS lookup, and then taking the first returned address.
     * For devices with multiple network interfaces and/or multiple addresses
     * per interface, this does not necessarily return the {@code InetAddress}
     * you want.
     *
     * <p>Multiple interface/address configurations were relatively rare
     * when this API was designed, but multiple interfaces are the default for
     * modern mobile devices (with separate wifi and radio interfaces), and
     * the need to support both IPv4 and IPv6 has made multiple addresses
     * commonplace. New code should thus avoid this method except where it's
     * basically being used to get a loopback address or equivalent.
     *
     * <p>There are two main ways to get a more specific answer:
     * <ul>
     * <li>If you have a connected socket, you should probably use
     * {@link Socket#getLocalAddress} instead: that will give you the address
     * that's actually in use for that connection. (It's not possible to ask
     * the question "what local address would a connection to a given remote
     * address use?"; you have to actually make the connection and see.)</li>
     * <li>For other use cases, see {@link NetworkInterface}, which lets you
     * enumerate all available network interfaces and their addresses.</li>
     * </ul>
     *
     * <p>Note that if the host doesn't have a hostname set&nbsp;&ndash; as
     * Android devices typically don't&nbsp;&ndash; this method will
     * effectively return the loopback address, albeit by getting the name
     * {@code localhost} and then doing a lookup to translate that to
     * {@code 127.0.0.1}.
     *
     * @return an {@code InetAddress} representing the local host, or the
     * loopback address.
     * @throws UnknownHostException
     *             if the address lookup fails.
     */
    public static InetAddress getLocalHost() throws UnknownHostException {
        String host = gethostname();
        SecurityManager security = System.getSecurityManager();
        try {
            if (security != null) {
                security.checkConnect(host, -1);
            }
        } catch (SecurityException e) {
            return Inet4Address.LOOPBACK;
        }
        return lookupHostByName(host)[0];
    }
    private static native String gethostname();

    /**
     * Gets the hashcode of the represented IP address.
     *
     * @return the appropriate hashcode value.
     */
    @Override
    public int hashCode() {
        return Arrays.hashCode(ipaddress);
    }

    /*
     * Returns whether this address is an IP multicast address or not. This
     * implementation returns always {@code false}.
     *
     * @return {@code true} if this address is in the multicast group, {@code
     *         false} otherwise.
     */
    public boolean isMulticastAddress() {
        return false;
    }

    /**
     * Resolves a hostname to its IP addresses using a cache.
     *
     * @param host the hostname to resolve.
     * @return the IP addresses of the host.
     */
    private static InetAddress[] lookupHostByName(String host) throws UnknownHostException {
        BlockGuard.getThreadPolicy().onNetwork();
        // Do we have a result cached?
        InetAddress[] cachedResult = addressCache.get(host);
        if (cachedResult != null) {
            if (cachedResult.length > 0) {
                // A cached positive result.
                return cachedResult;
            } else {
                // A cached negative result.
                throw new UnknownHostException(host);
            }
        }
        try {
            InetAddress[] addresses = bytesToInetAddresses(getaddrinfo(host), host);
            addressCache.put(host, addresses);
            return addresses;
        } catch (UnknownHostException e) {
            addressCache.putUnknownHost(host);
            throw new UnknownHostException(host);
        }
    }
    private static native byte[][] getaddrinfo(String name) throws UnknownHostException;

    /**
     * Query the IP stack for the host address. The host is in address form.
     *
     * @param addr
     *            the host address to lookup.
     * @throws UnknownHostException
     *             if an error occurs during lookup.
     */
    static InetAddress getHostByAddrImpl(byte[] addr)
            throws UnknownHostException {
        BlockGuard.getThreadPolicy().onNetwork();
        if (addr.length == 4) {
            return new Inet4Address(addr, getnameinfo(addr));
        } else if (addr.length == 16) {
            return new Inet6Address(addr, getnameinfo(addr));
        } else {
            throw new UnknownHostException(wrongAddressLength());
        }
    }

    /**
     * Resolves an IP address to a hostname. Thread safe.
     */
    private static native String getnameinfo(byte[] addr);

    static String getHostNameInternal(String host, boolean isCheck) throws UnknownHostException {
        if (host == null || 0 == host.length()) {
            return Inet4Address.LOOPBACK.getHostAddress();
        }
        if (isHostName(host)) {
            if (isCheck) {
                SecurityManager sm = System.getSecurityManager();
                if (sm != null) {
                    sm.checkConnect(host, -1);
                }
            }
            return lookupHostByName(host)[0].getHostAddress();
        }
        return host;
    }

    /**
     * Returns a string containing a concise, human-readable description of this
     * IP address.
     *
     * @return the description, as host/address.
     */
    @Override
    public String toString() {
        return (hostName == null ? "" : hostName) + "/" + getHostAddress();
    }

    /**
     * Returns true if the string is a host name, false if it is an IP Address.
     */
    static boolean isHostName(String value) {
        try {
            ipStringToByteArray(value);
            return false;
        } catch (UnknownHostException e) {
            return true;
        }
    }

    /**
     * Returns whether this address is a loopback address or not. This
     * implementation returns always {@code false}. Valid IPv4 loopback
     * addresses are 127.d.d.d The only valid IPv6 loopback address is ::1.
     *
     * @return {@code true} if this instance represents a loopback address,
     *         {@code false} otherwise.
     */
    public boolean isLoopbackAddress() {
        return false;
    }

    /**
     * Returns whether this address is a link-local address or not. This
     * implementation returns always {@code false}.
     * <p>
     * Valid IPv6 link-local addresses are FE80::0 through to
     * FEBF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF.
     * <p>
     * There are no valid IPv4 link-local addresses.
     *
     * @return {@code true} if this instance represents a link-local address,
     *         {@code false} otherwise.
     */
    public boolean isLinkLocalAddress() {
        return false;
    }

    /**
     * Returns whether this address is a site-local address or not. This
     * implementation returns always {@code false}.
     * <p>
     * Valid IPv6 site-local addresses are FEC0::0 through to
     * FEFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF.
     * <p>
     * There are no valid IPv4 site-local addresses.
     *
     * @return {@code true} if this instance represents a site-local address,
     *         {@code false} otherwise.
     */
    public boolean isSiteLocalAddress() {
        return false;
    }

    /**
     * Returns whether this address is a global multicast address or not. This
     * implementation returns always {@code false}.
     * <p>
     * Valid IPv6 link-global multicast addresses are FFxE:/112 where x is a set
     * of flags, and the additional 112 bits make up the global multicast
     * address space.
     * <p>
     * Valid IPv4 global multicast addresses are between: 224.0.1.0 to
     * 238.255.255.255.
     *
     * @return {@code true} if this instance represents a global multicast
     *         address, {@code false} otherwise.
     */
    public boolean isMCGlobal() {
        return false;
    }

    /**
     * Returns whether this address is a node-local multicast address or not.
     * This implementation returns always {@code false}.
     * <p>
     * Valid IPv6 node-local multicast addresses are FFx1:/112 where x is a set
     * of flags, and the additional 112 bits make up the node-local multicast
     * address space.
     * <p>
     * There are no valid IPv4 node-local multicast addresses.
     *
     * @return {@code true} if this instance represents a node-local multicast
     *         address, {@code false} otherwise.
     */
    public boolean isMCNodeLocal() {
        return false;
    }

    /**
     * Returns whether this address is a link-local multicast address or not.
     * This implementation returns always {@code false}.
     * <p>
     * Valid IPv6 link-local multicast addresses are FFx2:/112 where x is a set
     * of flags, and the additional 112 bits make up the link-local multicast
     * address space.
     * <p>
     * Valid IPv4 link-local addresses are between: 224.0.0.0 to 224.0.0.255
     *
     * @return {@code true} if this instance represents a link-local multicast
     *         address, {@code false} otherwise.
     */
    public boolean isMCLinkLocal() {
        return false;
    }

    /**
     * Returns whether this address is a site-local multicast address or not.
     * This implementation returns always {@code false}.
     * <p>
     * Valid IPv6 site-local multicast addresses are FFx5:/112 where x is a set
     * of flags, and the additional 112 bits make up the site-local multicast
     * address space.
     * <p>
     * Valid IPv4 site-local addresses are between: 239.252.0.0 to
     * 239.255.255.255
     *
     * @return {@code true} if this instance represents a site-local multicast
     *         address, {@code false} otherwise.
     */
    public boolean isMCSiteLocal() {
        return false;
    }

    /**
     * Returns whether this address is a organization-local multicast address or
     * not. This implementation returns always {@code false}.
     * <p>
     * Valid IPv6 organization-local multicast addresses are FFx8:/112 where x
     * is a set of flags, and the additional 112 bits make up the
     * organization-local multicast address space.
     * <p>
     * Valid IPv4 organization-local addresses are between: 239.192.0.0 to
     * 239.251.255.255
     *
     * @return {@code true} if this instance represents a organization-local
     *         multicast address, {@code false} otherwise.
     */
    public boolean isMCOrgLocal() {
        return false;
    }

    /**
     * Returns whether this is a wildcard address or not. This implementation
     * returns always {@code false}.
     *
     * @return {@code true} if this instance represents a wildcard address,
     *         {@code false} otherwise.
     */
    public boolean isAnyLocalAddress() {
        return false;
    }

    /**
     * Tries to reach this {@code InetAddress}. This method first tries to use
     * ICMP <i>(ICMP ECHO REQUEST)</i>. When first step fails, a TCP connection
     * on port 7 (Echo) of the remote host is established.
     *
     * @param timeout
     *            timeout in milliseconds before the test fails if no connection
     *            could be established.
     * @return {@code true} if this address is reachable, {@code false}
     *         otherwise.
     * @throws IOException
     *             if an error occurs during an I/O operation.
     * @throws IllegalArgumentException
     *             if timeout is less than zero.
     */
    public boolean isReachable(int timeout) throws IOException {
        return isReachable(null, 0, timeout);
    }

    /**
     * Tries to reach this {@code InetAddress}. This method first tries to use
     * ICMP <i>(ICMP ECHO REQUEST)</i>. When first step fails, a TCP connection
     * on port 7 (Echo) of the remote host is established.
     *
     * @param networkInterface
     *            the network interface on which to connection should be
     *            established.
     * @param ttl
     *            the maximum count of hops (time-to-live).
     * @param timeout
     *            timeout in milliseconds before the test fails if no connection
     *            could be established.
     * @return {@code true} if this address is reachable, {@code false}
     *         otherwise.
     * @throws IOException
     *             if an error occurs during an I/O operation.
     * @throws IllegalArgumentException
     *             if ttl or timeout is less than zero.
     */
    public boolean isReachable(NetworkInterface networkInterface, final int ttl,
            final int timeout) throws IOException {
        if (ttl < 0 || timeout < 0) {
            throw new IllegalArgumentException("ttl < 0 || timeout < 0");
        }
        if (networkInterface == null) {
            return isReachableByTCP(this, null, timeout);
        } else {
            return isReachableByMultiThread(networkInterface, ttl, timeout);
        }
    }

    /*
     * Uses multi-Thread to try if isReachable, returns true if any of threads
     * returns in time
     */
    private boolean isReachableByMultiThread(NetworkInterface netif,
            final int ttl, final int timeout)
            throws IOException {
        List<InetAddress> addresses = Collections.list(netif.getInetAddresses());
        if (addresses.isEmpty()) {
            return false;
        }
        reached = false;
        addrCount = addresses.size();
        boolean needWait = false;
        for (final InetAddress addr : addresses) {
            // loopback interface can only reach to local addresses
            if (addr.isLoopbackAddress()) {
                Enumeration<NetworkInterface> NetworkInterfaces = NetworkInterface
                        .getNetworkInterfaces();
                while (NetworkInterfaces.hasMoreElements()) {
                    NetworkInterface networkInterface = NetworkInterfaces
                            .nextElement();
                    Enumeration<InetAddress> localAddresses = networkInterface
                            .getInetAddresses();
                    while (localAddresses.hasMoreElements()) {
                        if (InetAddress.this.equals(localAddresses
                                .nextElement())) {
                            return true;
                        }
                    }
                }

                synchronized (waitReachable) {
                    addrCount--;

                    if (addrCount == 0) {
                        // if count equals zero, all thread
                        // expired,notifies main thread
                        waitReachable.notifyAll();
                    }
                }
                continue;
            }

            needWait = true;
            new Thread() {
                @Override public void run() {
                    /*
                     * Spec violation! This implementation doesn't attempt an
                     * ICMP; it skips right to TCP echo.
                     */
                    boolean threadReached = false;
                    try {
                        threadReached = isReachableByTCP(addr, InetAddress.this, timeout);
                    } catch (IOException e) {
                    }

                    synchronized (waitReachable) {
                        if (threadReached) {
                            // if thread reached this address, sets reached to
                            // true and notifies main thread
                            reached = true;
                            waitReachable.notifyAll();
                        } else {
                            addrCount--;
                            if (0 == addrCount) {
                                // if count equals zero, all thread
                                // expired,notifies main thread
                                waitReachable.notifyAll();
                            }
                        }
                    }
                }
            }.start();
        }

        if (needWait) {
            synchronized (waitReachable) {
                try {
                    while (!reached && (addrCount != 0)) {
                        // wait for notification
                        waitReachable.wait(1000);
                    }
                } catch (InterruptedException e) {
                    // do nothing
                }
                return reached;
            }
        }

        return false;
    }

    private boolean isReachableByTCP(InetAddress destination, InetAddress source, int timeout)
            throws IOException {
        FileDescriptor fd = new FileDescriptor();
        boolean reached = false;
        NETIMPL.socket(fd, true);
        try {
            if (null != source) {
                NETIMPL.bind(fd, source, 0);
            }
            NETIMPL.connect(fd, destination, 7, timeout);
            reached = true;
        } catch (IOException e) {
            if (ERRMSG_CONNECTION_REFUSED.equals(e.getMessage())) {
                // Connection refused means the IP is reachable
                reached = true;
            }
        }

        NETIMPL.close(fd);

        return reached;
    }

    /**
     * Returns the {@code InetAddress} corresponding to the array of bytes. In
     * the case of an IPv4 address there must be exactly 4 bytes and for IPv6
     * exactly 16 bytes. If not, an {@code UnknownHostException} is thrown.
     * <p>
     * The IP address is not validated by a name service.
     * <p>
     * The high order byte is {@code ipAddress[0]}.
     *
     * @param ipAddress
     *            is either a 4 (IPv4) or 16 (IPv6) byte long array.
     * @return an {@code InetAddress} instance representing the given IP address
     *         {@code ipAddress}.
     * @throws UnknownHostException
     *             if the given byte array has no valid length.
     */
    public static InetAddress getByAddress(byte[] ipAddress)
            throws UnknownHostException {
        // simply call the method by the same name specifying the default scope
        // id of 0
        return getByAddressInternal(null, ipAddress, 0);
    }

    /**
     * Returns the {@code InetAddress} corresponding to the array of bytes. In
     * the case of an IPv4 address there must be exactly 4 bytes and for IPv6
     * exactly 16 bytes. If not, an {@code UnknownHostException} is thrown. The
     * IP address is not validated by a name service. The high order byte is
     * {@code ipAddress[0]}.
     *
     * @param ipAddress
     *            either a 4 (IPv4) or 16 (IPv6) byte array.
     * @param scope_id
     *            the scope id for an IPV6 scoped address. If not a scoped
     *            address just pass in 0.
     * @return the InetAddress
     * @throws UnknownHostException
     */
    static InetAddress getByAddress(byte[] ipAddress, int scope_id)
            throws UnknownHostException {
        return getByAddressInternal(null, ipAddress, scope_id);
    }

    private static boolean isIPv4MappedAddress(byte[] ipAddress) {
        // Check if the address matches ::FFFF:d.d.d.d
        // The first 10 bytes are 0. The next to are -1 (FF).
        // The last 4 bytes are varied.
        if (ipAddress == null || ipAddress.length != 16) {
            return false;
        }
        for (int i = 0; i < 10; i++) {
            if (ipAddress[i] != 0) {
                return false;
            }
        }
        if (ipAddress[10] != -1 || ipAddress[11] != -1) {
            return false;
        }
        return true;
    }

    private static byte[] ipv4MappedToIPv4(byte[] mappedAddress) {
        byte[] ipv4Address = new byte[4];
        for(int i = 0; i < 4; i++) {
            ipv4Address[i] = mappedAddress[12 + i];
        }
        return ipv4Address;
    }

    /**
     * Returns the {@code InetAddress} corresponding to the array of bytes, and
     * the given hostname. In the case of an IPv4 address there must be exactly
     * 4 bytes and for IPv6 exactly 16 bytes. If not, an {@code
     * UnknownHostException} will be thrown.
     * <p>
     * The host name and IP address are not validated.
     * <p>
     * The hostname either be a machine alias or a valid IPv6 or IPv4 address
     * format.
     * <p>
     * The high order byte is {@code ipAddress[0]}.
     *
     * @param hostName
     *            the string representation of hostname or IP address.
     * @param ipAddress
     *            either a 4 (IPv4) or 16 (IPv6) byte long array.
     * @return an {@code InetAddress} instance representing the given IP address
     *         and hostname.
     * @throws UnknownHostException
     *             if the given byte array has no valid length.
     */
    public static InetAddress getByAddress(String hostName, byte[] ipAddress)
            throws UnknownHostException {
        // just call the method by the same name passing in a default scope id
        // of 0
        return getByAddressInternal(hostName, ipAddress, 0);
    }

    /**
     * Returns the {@code InetAddress} corresponding to the array of bytes, and
     * the given hostname. In the case of an IPv4 address there must be exactly
     * 4 bytes and for IPv6 exactly 16 bytes. If not, an {@code
     * UnknownHostException} is thrown. The host name and IP address are not
     * validated. The hostname either be a machine alias or a valid IPv6 or IPv4
     * address format. The high order byte is {@code ipAddress[0]}.
     *
     * @param hostName
     *            string representation of hostname or IP address.
     * @param ipAddress
     *            either a 4 (IPv4) or 16 (IPv6) byte array.
     * @param scope_id
     *            the scope id for a scoped address. If not a scoped address
     *            just pass in 0.
     * @return the InetAddress
     * @throws UnknownHostException
     */
    static InetAddress getByAddressInternal(String hostName, byte[] ipAddress,
            int scope_id) throws UnknownHostException {
        if (ipAddress == null) {
            throw new UnknownHostException("ipAddress == null");
        }
        switch (ipAddress.length) {
            case 4:
                return new Inet4Address(ipAddress.clone());
            case 16:
                // First check to see if the address is an IPv6-mapped
                // IPv4 address. If it is, then we can make it a IPv4
                // address, otherwise, we'll create an IPv6 address.
                if (isIPv4MappedAddress(ipAddress)) {
                    return new Inet4Address(ipv4MappedToIPv4(ipAddress));
                } else {
                    return new Inet6Address(ipAddress.clone(), scope_id);
                }
            default:
                throw new UnknownHostException(
                        "Invalid IP Address is neither 4 or 16 bytes: " + hostName);
        }
    }

    /**
     * Takes the integer and chops it into 4 bytes, putting it into the byte
     * array starting with the high order byte at the index start. This method
     * makes no checks on the validity of the parameters.
     */
    static void intToBytes(int value, byte[] bytes, int start) {
        // Shift the int so the current byte is right-most
        // Use a byte mask of 255 to single out the last byte.
        bytes[start] = (byte) ((value >> 24) & 255);
        bytes[start + 1] = (byte) ((value >> 16) & 255);
        bytes[start + 2] = (byte) ((value >> 8) & 255);
        bytes[start + 3] = (byte) (value & 255);
    }

    /**
     * Takes the byte array and creates an integer out of four bytes starting at
     * start as the high-order byte. This method makes no checks on the validity
     * of the parameters.
     */
    static int bytesToInt(byte[] bytes, int start) {
        // First mask the byte with 255, as when a negative
        // signed byte converts to an integer, it has bits
        // on in the first 3 bytes, we are only concerned
        // about the right-most 8 bits.
        // Then shift the rightmost byte to align with its
        // position in the integer.
        int value = ((bytes[start + 3] & 255))
                | ((bytes[start + 2] & 255) << 8)
                | ((bytes[start + 1] & 255) << 16)
                | ((bytes[start] & 255) << 24);
        return value;
    }

    private static final ObjectStreamField[] serialPersistentFields = {
            new ObjectStreamField("address", Integer.TYPE),
            new ObjectStreamField("family", Integer.TYPE),
            new ObjectStreamField("hostName", String.class) };

    private void writeObject(ObjectOutputStream stream) throws IOException {
        ObjectOutputStream.PutField fields = stream.putFields();
        if (ipaddress == null) {
            fields.put("address", 0);
        } else {
            fields.put("address", bytesToInt(ipaddress, 0));
        }
        fields.put("family", family);
        fields.put("hostName", hostName);

        stream.writeFields();
    }

    private void readObject(ObjectInputStream stream) throws IOException,
            ClassNotFoundException {
        ObjectInputStream.GetField fields = stream.readFields();
        int addr = fields.get("address", 0);
        ipaddress = new byte[4];
        intToBytes(addr, ipaddress, 0);
        hostName = (String) fields.get("hostName", null);
        family = fields.get("family", 2);
    }

    /*
     * The spec requires that if we encounter a generic InetAddress in
     * serialized form then we should interpret it as an Inet4 address.
     */
    private Object readResolve() throws ObjectStreamException {
        return new Inet4Address(ipaddress, hostName);
    }
}
