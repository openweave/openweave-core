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
package org.apache.harmony.luni.internal.net.www.protocol.https;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.ProtocolException;
import java.net.Proxy;
import java.net.URL;
import java.security.Permission;
import java.security.Principal;
import java.security.cert.Certificate;
import java.security.cert.CertificateException;
import java.util.List;
import java.util.Map;
import javax.net.ssl.HttpsURLConnection;
import javax.net.ssl.SSLHandshakeException;
import javax.net.ssl.SSLPeerUnverifiedException;
import javax.net.ssl.SSLSocket;
import org.apache.harmony.luni.internal.net.www.protocol.http.HttpConnection;
import org.apache.harmony.luni.internal.net.www.protocol.http.HttpURLConnectionImpl;

/**
 * HttpsURLConnection implementation.
 */
public class HttpsURLConnectionImpl extends HttpsURLConnection {

    /**
     * HttpsEngine that allows reuse of HttpURLConnectionImpl
     */
    private final HttpsEngine httpsEngine;

    /**
     * Local stash of HttpsEngine.connection.sslSocket for answering
     * queries such as getCipherSuite even after
     * httpsEngine.Connection has been recycled. It's presense is also
     * used to tell if the HttpsURLConnection is considered connected,
     * as opposed to the connected field of URLConnection or the a
     * non-null connect in HttpURLConnectionImpl
    */
    private SSLSocket sslSocket;

    protected HttpsURLConnectionImpl(URL url, int port) {
        super(url);
        httpsEngine = new HttpsEngine(url, port);
    }

    protected HttpsURLConnectionImpl(URL url, int port, Proxy proxy) {
        super(url);
        httpsEngine = new HttpsEngine(url, port, proxy);
    }

    private void checkConnected() {
        if (sslSocket == null) {
            throw new IllegalStateException("Connection has not yet been established");
        }
    }

    @Override
    public String getCipherSuite() {
        checkConnected();
        return sslSocket.getSession().getCipherSuite();
    }

    @Override
    public Certificate[] getLocalCertificates() {
        checkConnected();
        return sslSocket.getSession().getLocalCertificates();
    }

    @Override
    public Certificate[] getServerCertificates() throws SSLPeerUnverifiedException {
        checkConnected();
        return sslSocket.getSession().getPeerCertificates();
    }

    @Override
    public Principal getPeerPrincipal() throws SSLPeerUnverifiedException {
        checkConnected();
        return sslSocket.getSession().getPeerPrincipal();
    }

    @Override
    public Principal getLocalPrincipal() {
        checkConnected();
        return sslSocket.getSession().getLocalPrincipal();
    }

    @Override
    public void disconnect() {
        httpsEngine.disconnect();
    }

    @Override
    public InputStream getErrorStream() {
        return httpsEngine.getErrorStream();
    }

    @Override
    public String getRequestMethod() {
        return httpsEngine.getRequestMethod();
    }

    @Override
    public int getResponseCode() throws IOException {
        return httpsEngine.getResponseCode();
    }

    @Override
    public String getResponseMessage() throws IOException {
        return httpsEngine.getResponseMessage();
    }

    @Override
    public void setRequestMethod(String method) throws ProtocolException {
        httpsEngine.setRequestMethod(method);
    }

    @Override
    public boolean usingProxy() {
        return httpsEngine.usingProxy();
    }

    @Override
    public boolean getInstanceFollowRedirects() {
        return httpsEngine.getInstanceFollowRedirects();
    }

    @Override
    public void setInstanceFollowRedirects(boolean followRedirects) {
        httpsEngine.setInstanceFollowRedirects(followRedirects);
    }

    @Override
    public void connect() throws IOException {
        connected = true;
        httpsEngine.connect();
    }

    @Override
    public boolean getAllowUserInteraction() {
        return httpsEngine.getAllowUserInteraction();
    }

    @Override
    public Object getContent() throws IOException {
        return httpsEngine.getContent();
    }

    @SuppressWarnings("unchecked") // Spec does not generify
    @Override
    public Object getContent(Class[] types) throws IOException {
        return httpsEngine.getContent(types);
    }

    @Override
    public String getContentEncoding() {
        return httpsEngine.getContentEncoding();
    }

    @Override
    public int getContentLength() {
        return httpsEngine.getContentLength();
    }

    @Override
    public String getContentType() {
        return httpsEngine.getContentType();
    }

    @Override
    public long getDate() {
        return httpsEngine.getDate();
    }

    @Override
    public boolean getDefaultUseCaches() {
        return httpsEngine.getDefaultUseCaches();
    }

    @Override
    public boolean getDoInput() {
        return httpsEngine.getDoInput();
    }

    @Override
    public boolean getDoOutput() {
        return httpsEngine.getDoOutput();
    }

    @Override
    public long getExpiration() {
        return httpsEngine.getExpiration();
    }

    @Override
    public String getHeaderField(int pos) {
        return httpsEngine.getHeaderField(pos);
    }

    @Override
    public Map<String, List<String>> getHeaderFields() {
        return httpsEngine.getHeaderFields();
    }

    @Override
    public Map<String, List<String>> getRequestProperties() {
        return httpsEngine.getRequestProperties();
    }

    @Override
    public void addRequestProperty(String field, String newValue) {
        httpsEngine.addRequestProperty(field, newValue);
    }

    @Override
    public String getHeaderField(String key) {
        return httpsEngine.getHeaderField(key);
    }

    @Override
    public long getHeaderFieldDate(String field, long defaultValue) {
        return httpsEngine.getHeaderFieldDate(field, defaultValue);
    }

    @Override
    public int getHeaderFieldInt(String field, int defaultValue) {
        return httpsEngine.getHeaderFieldInt(field, defaultValue);
    }

    @Override
    public String getHeaderFieldKey(int posn) {
        return httpsEngine.getHeaderFieldKey(posn);
    }

    @Override
    public long getIfModifiedSince() {
        return httpsEngine.getIfModifiedSince();
    }

    @Override
    public InputStream getInputStream() throws IOException {
        return httpsEngine.getInputStream();
    }

    @Override
    public long getLastModified() {
        return httpsEngine.getLastModified();
    }

    @Override
    public OutputStream getOutputStream() throws IOException {
        return httpsEngine.getOutputStream();
    }

    @Override
    public Permission getPermission() throws IOException {
        return httpsEngine.getPermission();
    }

    @Override
    public String getRequestProperty(String field) {
        return httpsEngine.getRequestProperty(field);
    }

    @Override
    public URL getURL() {
        return httpsEngine.getURL();
    }

    @Override
    public boolean getUseCaches() {
        return httpsEngine.getUseCaches();
    }

    @Override
    public void setAllowUserInteraction(boolean newValue) {
        httpsEngine.setAllowUserInteraction(newValue);
    }

    @Override
    public void setDefaultUseCaches(boolean newValue) {
        httpsEngine.setDefaultUseCaches(newValue);
    }

    @Override
    public void setDoInput(boolean newValue) {
        httpsEngine.setDoInput(newValue);
    }

    @Override
    public void setDoOutput(boolean newValue) {
        httpsEngine.setDoOutput(newValue);
    }

    @Override
    public void setIfModifiedSince(long newValue) {
        httpsEngine.setIfModifiedSince(newValue);
    }

    @Override
    public void setRequestProperty(String field, String newValue) {
        httpsEngine.setRequestProperty(field, newValue);
    }

    @Override
    public void setUseCaches(boolean newValue) {
        httpsEngine.setUseCaches(newValue);
    }

    @Override
    public void setConnectTimeout(int timeout) {
        httpsEngine.setConnectTimeout(timeout);
    }

    @Override
    public int getConnectTimeout() {
        return httpsEngine.getConnectTimeout();
    }

    @Override
    public void setReadTimeout(int timeout) {
        httpsEngine.setReadTimeout(timeout);
    }

    @Override
    public int getReadTimeout() {
        return httpsEngine.getReadTimeout();
    }

    @Override
    public String toString() {
        return httpsEngine.toString();
    }

    private final class HttpsEngine extends HttpURLConnectionImpl {

        protected HttpsEngine(URL url, int port) {
            super(url, port);
        }

        protected HttpsEngine(URL url, int port, Proxy proxy) {
            super(url, port, proxy);
        }

        @Override public void makeConnection() throws IOException {
            /*
             * Short-circuit a reentrant call. The first step in doing SSL with
             * an HTTP proxy requires calling retrieveResponse() which calls
             * back into makeConnection(). We can return immediately because the
             * unencrypted connection is already valid.
             */
            if (method == CONNECT) {
                return;
            }

            boolean connectionReused;
            // first try an SSL connection with compression and
            // various TLS extensions enabled, if it fails (and its
            // not unheard of that it will) fallback to a more
            // barebones connections
            try {
                connectionReused = makeSslConnection(true);
            } catch (IOException e) {
                // If the problem was a CertificateException from the X509TrustManager,
                // do not retry, we didn't have an abrupt server initiated exception.
                if (e instanceof SSLHandshakeException
                        && e.getCause() instanceof CertificateException) {
                    throw e;
                }
                releaseSocket(false);
                connectionReused = makeSslConnection(false);
            }

            if (!connectionReused) {
                sslSocket = connection.verifySecureSocketHostname(getHostnameVerifier());
            }
            setUpTransportIO(connection);
        }

        /**
         * Attempt to make an https connection. Returns true if a
         * connection was reused, false otherwise.
         *
         * @param tlsTolerant If true, assume server can handle common
         * TLS extensions and SSL deflate compression. If false, use
         * an SSL3 only fallback mode without compression.
         */
        private boolean makeSslConnection(boolean tlsTolerant) throws IOException {

            super.makeConnection();

            // if super.makeConnection returned a connection from the
            // pool, sslSocket needs to be initialized here. If it is
            // a new connection, it will be initialized by
            // getSecureSocket below.
            sslSocket = connection.getSecureSocketIfConnected();

            // we already have an SSL connection,
            if (sslSocket != null) {
                return true;
            }

            // make SSL Tunnel
            if (requiresTunnel()) {
                String originalMethod = method;
                method = CONNECT;
                intermediateResponse = true;
                try {
                    retrieveResponse();
                    discardIntermediateResponse();
                } finally {
                    method = originalMethod;
                    intermediateResponse = false;
                }
            }

            connection.setupSecureSocket(getSSLSocketFactory(), tlsTolerant);
            return false;
        }

        @Override protected void setUpTransportIO(HttpConnection connection) throws IOException {
            if (!requiresTunnel() && sslSocket == null) {
                return; // don't initialize streams that won't be used
            }
            super.setUpTransportIO(connection);
        }

        @Override protected boolean requiresTunnel() {
            return usingProxy();
        }

        @Override protected String requestString() {
            if (!usingProxy()) {
                return super.requestString();

            } else if (method == CONNECT) {
                // SSL tunnels require host:port for the origin server
                return url.getHost() + ":" + url.getEffectivePort();

            } else {
                // we has made SSL Tunneling, return /requested.data
                String file = url.getFile();
                if (file == null || file.length() == 0) {
                    file = "/";
                }
                return file;
            }
        }
    }
}
