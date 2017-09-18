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

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.util.ArrayList;
import java.util.List;

import static tests.http.MockWebServer.ASCII;

/**
 * A scripted response to be replayed by the mock web server.
 */
public class MockResponse {
    private static final String EMPTY_BODY_HEADER = "Content-Length: 0";
    private static final String CHUNKED_BODY_HEADER = "Transfer-encoding: chunked";
    private static final byte[] EMPTY_BODY = new byte[0];

    private String status = "HTTP/1.1 200 OK";
    private List<String> headers = new ArrayList<String>();
    private byte[] body = EMPTY_BODY;
    private boolean disconnectAtStart;
    private boolean disconnectAtEnd;

    public MockResponse() {
        headers.add(EMPTY_BODY_HEADER);
    }

    /**
     * Returns the HTTP response line, such as "HTTP/1.1 200 OK".
     */
    public String getStatus() {
        return status;
    }

    public MockResponse setResponseCode(int code) {
        this.status = "HTTP/1.1 " + code + " OK";
        return this;
    }

    public MockResponse setStatus(String status) {
        this.status = status;
        return this;
    }

    /**
     * Returns the HTTP headers, such as "Content-Length: 0".
     */
    public List<String> getHeaders() {
        return headers;
    }

    public MockResponse clearHeaders() {
        headers.clear();
        return this;
    }

    public MockResponse addHeader(String header) {
        headers.add(header);
        return this;
    }

    /**
     * Returns an input stream containing the raw HTTP payload.
     */
    public byte[] getBody() {
        return body;
    }

    public MockResponse setBody(byte[] body) {
        if (this.body == EMPTY_BODY) {
            headers.remove(EMPTY_BODY_HEADER);
        }
        this.headers.add("Content-Length: " + body.length);
        this.body = body;
        return this;
    }

    public MockResponse setBody(String body) {
        try {
            return setBody(body.getBytes(ASCII));
        } catch (UnsupportedEncodingException e) {
            throw new AssertionError();
        }
    }

    public MockResponse setChunkedBody(byte[] body, int maxChunkSize) throws IOException {
        headers.remove(EMPTY_BODY_HEADER);
        headers.add(CHUNKED_BODY_HEADER);

        ByteArrayOutputStream bytesOut = new ByteArrayOutputStream();
        int pos = 0;
        while (pos < body.length) {
            int chunkSize = Math.min(body.length - pos, maxChunkSize);
            bytesOut.write(Integer.toHexString(chunkSize).getBytes(ASCII));
            bytesOut.write("\r\n".getBytes(ASCII));
            bytesOut.write(body, pos, chunkSize);
            bytesOut.write("\r\n".getBytes(ASCII));
            pos += chunkSize;
        }
        bytesOut.write("0\r\n\r\n".getBytes(ASCII)); // last chunk + empty trailer + crlf
        this.body = bytesOut.toByteArray();
        return this;
    }

    public MockResponse setChunkedBody(String body, int maxChunkSize) throws IOException {
        return setChunkedBody(body.getBytes(ASCII), maxChunkSize);
    }

    /**
     * Request immediate close of connection without even reading the
     * request.
     * <p>
     * Use to simulate the real life case of losing connection
     * because of bugger SSL server close connection when it seems
     * something like a compression method or TLS extension it doesn't
     * understand, instead of simply ignoring it like it should.
     */
    public MockResponse setDisconnectAtStart(boolean disconnectAtStart) {
        this.disconnectAtStart = disconnectAtStart;
        return this;
    }

    public boolean getDisconnectAtStart() {
        return disconnectAtStart;
    }

    /**
     * Request close of connection after the response. This is the
     * default HTTP/1.0 behavior.
     */
    public MockResponse setDisconnectAtEnd(boolean disconnectAtEnd) {
        this.disconnectAtEnd = disconnectAtEnd;
        return this;
    }

    public boolean getDisconnectAtEnd() {
        return disconnectAtEnd;
    }

    @Override public String toString() {
        return status;
    }
}
