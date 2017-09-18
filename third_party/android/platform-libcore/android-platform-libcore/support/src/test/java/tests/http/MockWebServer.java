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

package tests.http;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.MalformedURLException;
import java.net.Proxy;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.URL;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;
import java.util.Set;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.Callable;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.LinkedBlockingDeque;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.atomic.AtomicInteger;
import javax.net.ssl.SSLSocket;
import javax.net.ssl.SSLSocketFactory;

/**
 * A scriptable web server. Callers supply canned responses and the server
 * replays them upon request in sequence.
 */
public final class MockWebServer {

    static final String ASCII = "US-ASCII";

    private final BlockingQueue<RecordedRequest> requestQueue
            = new LinkedBlockingQueue<RecordedRequest>();
    private final BlockingQueue<MockResponse> responseQueue
            = new LinkedBlockingDeque<MockResponse>();
    private final Set<Socket> openClientSockets
            = Collections.newSetFromMap(new ConcurrentHashMap<Socket, Boolean>());
    private boolean singleResponse;
    private final AtomicInteger requestCount = new AtomicInteger();
    private int bodyLimit = Integer.MAX_VALUE;
    private ServerSocket serverSocket;
    private SSLSocketFactory sslSocketFactory;
    private ExecutorService executor;
    private boolean tunnelProxy;

    private int port = -1;

    public int getPort() {
        if (port == -1) {
            throw new IllegalStateException("Cannot retrieve port before calling play()");
        }
        return port;
    }

    public Proxy toProxyAddress() {
        return new Proxy(Proxy.Type.HTTP, new InetSocketAddress("localhost", getPort()));
    }

    /**
     * Returns a URL for connecting to this server.
     *
     * @param path the request path, such as "/".
     */
    public URL getUrl(String path) throws MalformedURLException, UnknownHostException {
        String host = InetAddress.getLocalHost().getHostName();
        return sslSocketFactory != null
                ? new URL("https://" + host + ":" + getPort() + path)
                : new URL("http://" + host + ":" + getPort() + path);
    }

    /**
     * Sets the number of bytes of the POST body to keep in memory to the given
     * limit.
     */
    public void setBodyLimit(int maxBodyLength) {
        this.bodyLimit = maxBodyLength;
    }

    /**
     * Serve requests with HTTPS rather than otherwise.
     *
     * @param tunnelProxy whether to expect the HTTP CONNECT method before
     *     negotiating TLS.
     */
    public void useHttps(SSLSocketFactory sslSocketFactory, boolean tunnelProxy) {
        this.sslSocketFactory = sslSocketFactory;
        this.tunnelProxy = tunnelProxy;
    }

    /**
     * Awaits the next HTTP request, removes it, and returns it. Callers should
     * use this to verify the request sent was as intended.
     */
    public RecordedRequest takeRequest() throws InterruptedException {
        return requestQueue.take();
    }

    /**
     * Returns the number of HTTP requests received thus far by this server.
     * This may exceed the number of HTTP connections when connection reuse is
     * in practice.
     */
    public int getRequestCount() {
        return requestCount.get();
    }

    public void enqueue(MockResponse response) {
        responseQueue.add(response);
    }

    /**
     * By default, this class processes requests coming in by adding them to a
     * queue and serves responses by removing them from another queue. This mode
     * is appropriate for correctness testing.
     *
     * <p>Serving a single response causes the server to be stateless: requests
     * are not enqueued, and responses are not dequeued. This mode is appropriate
     * for benchmarking.
     */
    public void setSingleResponse(boolean singleResponse) {
        this.singleResponse = singleResponse;
    }

    /**
     * Starts the server, serves all enqueued requests, and shuts the server
     * down.
     */
    public void play() throws IOException {
        executor = Executors.newCachedThreadPool();
        serverSocket = new ServerSocket(0);
        serverSocket.setReuseAddress(true);

        port = serverSocket.getLocalPort();
        executor.submit(namedCallable("MockWebServer-accept-" + port, new Callable<Void>() {
            public Void call() throws Exception {
                List<Throwable> failures = new ArrayList<Throwable>();
                try {
                    acceptConnections();
                } catch (Throwable e) {
                    failures.add(e);
                }

                /*
                 * This gnarly block of code will release all sockets and
                 * all thread, even if any close fails.
                 */
                try {
                    serverSocket.close();
                } catch (Throwable e) {
                    failures.add(e);
                }
                for (Iterator<Socket> s = openClientSockets.iterator(); s.hasNext(); ) {
                    try {
                        s.next().close();
                        s.remove();
                    } catch (Throwable e) {
                        failures.add(e);
                    }
                }
                try {
                    executor.shutdown();
                } catch (Throwable e) {
                    failures.add(e);
                }
                if (!failures.isEmpty()) {
                    Throwable thrown = failures.get(0);
                    if (thrown instanceof Exception) {
                        throw (Exception) thrown;
                    } else {
                        throw (Error) thrown;
                    }
                } else {
                    return null;
                }
            }

            public void acceptConnections() throws Exception {
                int count = 0;
                while (true) {
                    if (count > 0 && responseQueue.isEmpty()) {
                        return;
                    }

                    Socket socket = serverSocket.accept();
                    if (responseQueue.peek().getDisconnectAtStart()) {
                        responseQueue.take();
                        socket.close();
                        continue;
                    }
                    openClientSockets.add(socket);
                    serveConnection(socket);
                    count++;
                }
            }
        }));
    }

    public void shutdown() throws IOException {
        if (serverSocket != null) {
            serverSocket.close(); // should cause acceptConnections() to break out
        }
    }

    private void serveConnection(final Socket raw) {
        String name = "MockWebServer-" + raw.getRemoteSocketAddress();
        executor.submit(namedCallable(name, new Callable<Void>() {
            int sequenceNumber = 0;

            public Void call() throws Exception {
                Socket socket;
                if (sslSocketFactory != null) {
                    if (tunnelProxy) {
                        if (!processOneRequest(raw.getInputStream(), raw.getOutputStream())) {
                            throw new IllegalStateException("Tunnel without any CONNECT!");
                        }
                    }
                    socket = sslSocketFactory.createSocket(
                            raw, raw.getInetAddress().getHostAddress(), raw.getPort(), true);
                    ((SSLSocket) socket).setUseClientMode(false);
                } else {
                    socket = raw;
                }

                InputStream in = new BufferedInputStream(socket.getInputStream());
                OutputStream out = new BufferedOutputStream(socket.getOutputStream());

                if (!processOneRequest(in, out)) {
                    throw new IllegalStateException("Connection without any request!");
                }
                while (processOneRequest(in, out)) {}

                in.close();
                out.close();
                raw.close();
                openClientSockets.remove(raw);
                return null;
            }

            /**
             * Reads a request and writes its response. Returns true if a request
             * was processed.
             */
            private boolean processOneRequest(InputStream in, OutputStream out)
                    throws IOException, InterruptedException {
                RecordedRequest request = readRequest(in, sequenceNumber);
                if (request == null) {
                    return false;
                }
                MockResponse response = dispatch(request);
                writeResponse(out, response);
                if (response.getDisconnectAtEnd()) {
                    in.close();
                    out.close();
                }
                sequenceNumber++;
                return true;
            }
        }));
    }

    /**
     * @param sequenceNumber the index of this request on this connection.
     */
    private RecordedRequest readRequest(InputStream in, int sequenceNumber) throws IOException {
        String request = readAsciiUntilCrlf(in);
        if (request.isEmpty()) {
            return null; // end of data; no more requests
        }

        List<String> headers = new ArrayList<String>();
        int contentLength = -1;
        boolean chunked = false;
        String header;
        while (!(header = readAsciiUntilCrlf(in)).isEmpty()) {
            headers.add(header);
            String lowercaseHeader = header.toLowerCase();
            if (contentLength == -1 && lowercaseHeader.startsWith("content-length:")) {
                contentLength = Integer.parseInt(header.substring(15).trim());
            }
            if (lowercaseHeader.startsWith("transfer-encoding:") &&
                    lowercaseHeader.substring(18).trim().equals("chunked")) {
                chunked = true;
            }
        }

        boolean hasBody = false;
        TruncatingOutputStream requestBody = new TruncatingOutputStream();
        List<Integer> chunkSizes = new ArrayList<Integer>();
        if (contentLength != -1) {
            hasBody = true;
            transfer(contentLength, in, requestBody);
        } else if (chunked) {
            hasBody = true;
            while (true) {
                int chunkSize = Integer.parseInt(readAsciiUntilCrlf(in).trim(), 16);
                if (chunkSize == 0) {
                    readEmptyLine(in);
                    break;
                }
                chunkSizes.add(chunkSize);
                transfer(chunkSize, in, requestBody);
                readEmptyLine(in);
            }
        }

        if (request.startsWith("GET ") || request.startsWith("CONNECT ")) {
            if (hasBody) {
                throw new IllegalArgumentException("GET requests should not have a body!");
            }
        } else if (request.startsWith("POST ")) {
            if (!hasBody) {
                throw new IllegalArgumentException("POST requests must have a body!");
            }
        } else {
            throw new UnsupportedOperationException("Unexpected method: " + request);
        }

        return new RecordedRequest(request, headers, chunkSizes,
                requestBody.numBytesReceived, requestBody.toByteArray(), sequenceNumber);
    }

    /**
     * Returns a response to satisfy {@code request}.
     */
    private MockResponse dispatch(RecordedRequest request) throws InterruptedException {
        if (responseQueue.isEmpty()) {
            throw new IllegalStateException("Unexpected request: " + request);
        }

        if (singleResponse) {
            return responseQueue.peek();
        } else {
            requestCount.incrementAndGet();
            requestQueue.add(request);
            return responseQueue.take();
        }
    }

    private void writeResponse(OutputStream out, MockResponse response) throws IOException {
        out.write((response.getStatus() + "\r\n").getBytes(ASCII));
        for (String header : response.getHeaders()) {
            out.write((header + "\r\n").getBytes(ASCII));
        }
        out.write(("\r\n").getBytes(ASCII));
        out.write(response.getBody());
        out.flush();
    }

    /**
     * Transfer bytes from {@code in} to {@code out} until either {@code length}
     * bytes have been transferred or {@code in} is exhausted.
     */
    private void transfer(int length, InputStream in, OutputStream out) throws IOException {
        byte[] buffer = new byte[1024];
        while (length > 0) {
            int count = in.read(buffer, 0, Math.min(buffer.length, length));
            if (count == -1) {
                return;
            }
            out.write(buffer, 0, count);
            length -= count;
        }
    }

    /**
     * Returns the text from {@code in} until the next "\r\n", or null if
     * {@code in} is exhausted.
     */
    private String readAsciiUntilCrlf(InputStream in) throws IOException {
        StringBuilder builder = new StringBuilder();
        while (true) {
            int c = in.read();
            if (c == '\n' && builder.length() > 0 && builder.charAt(builder.length() - 1) == '\r') {
                builder.deleteCharAt(builder.length() - 1);
                return builder.toString();
            } else if (c == -1) {
                return builder.toString();
            } else {
                builder.append((char) c);
            }
        }
    }

    private void readEmptyLine(InputStream in) throws IOException {
        String line = readAsciiUntilCrlf(in);
        if (!line.isEmpty()) {
            throw new IllegalStateException("Expected empty but was: " + line);
        }
    }

    /**
     * An output stream that drops data after bodyLimit bytes.
     */
    private class TruncatingOutputStream extends ByteArrayOutputStream {
        private int numBytesReceived = 0;
        @Override public void write(byte[] buffer, int offset, int len) {
            numBytesReceived += len;
            super.write(buffer, offset, Math.min(len, bodyLimit - count));
        }
        @Override public void write(int oneByte) {
            numBytesReceived++;
            if (count < bodyLimit) {
                super.write(oneByte);
            }
        }
    }

    private static <T> Callable<T> namedCallable(final String name, final Callable<T> callable) {
        return new Callable<T>() {
            public T call() throws Exception {
                String originalName = Thread.currentThread().getName();
                Thread.currentThread().setName(name);
                try {
                    return callable.call();
                } finally {
                    Thread.currentThread().setName(originalName);
                }
            }
        };
    }
}
