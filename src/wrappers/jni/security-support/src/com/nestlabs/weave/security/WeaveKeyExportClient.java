/*
 *
 *    Copyright (c) 2017 Nest Labs, Inc.
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */
package com.nestlabs.weave.security;

/**
 * Implements the client side of the Weave key export protocol for use in stand-alone (non-Weave
 *   messaging) contexts.
 */
public final class WeaveKeyExportClient {

    /**
     * Generate a key export request given a client certificate and private key.
     * 
     * @param keyId             The Weave key id of the key to be exported. 
     * @param responderNodeId   The Weave node id of the device to which the request will be forwarded; or
     *                          0 if the particular device id is unknown.
     * @param clientCert        A buffer containing a Weave certificate identifying the client making the request.
     *                          The certificate is expected to be encoded in Weave TLV format.
     * @param clientKey         A buffer containing the private key associated with the client certificate.
     *                          The private key is expected to be encoded in Weave TLV format.
     * @return                  A byte array containing the generated key export request.
     */
    public byte[] generateKeyExportRequest(int keyId, long responderNodeId, byte[] clientCert, byte[] clientKey) {
        return generateKeyExportRequest(getNativeClientPtr(), keyId, responderNodeId, clientCert, clientKey);
    }

    /**
     * Generate a key export request given an access token.
     * 
     * @param keyId             The Weave key id of the key to be exported. 
     * @param responderNodeId   The Weave node id of the device to which the request will be forwarded; or
     *                          0 if the particular device id is unknown.
     * @param accessToken       A buffer containing a Weave access token, in Weave TLV format.
     * @return                  A byte array containing the generated key export request.
     */
    public byte[] generateKeyExportRequest(int keyId, long responderNodeId, byte[] accessToken) {
        return generateKeyExportRequest(getNativeClientPtr(), keyId, responderNodeId, accessToken);
    }
    
    /**
     * Process the response to a previously-generated key export request. 
     * 
     * @param responderNodeId   The Weave node id of the device to which the request was forwarded; or
     *                          0 if the particular device id is unknown.
     * @param exportResp        A buffer containing a Weave key export response, as returned by the device.
     * @return                  A byte array containing exported key.
     */
    public byte[] processKeyExportResponse(long responderNodeId, byte[] exportResp) {
        return processKeyExportResponse(getNativeClientPtr(), responderNodeId, exportResp);
    }
    
    /**
     * Process a reconfigure message received in response to a previously-generated key export request. 
     * 
     * @param reconfig          A buffer containing a Weave key export reconfigure message, as returned
     *                          by the device.
     */
    public void processKeyExportReconfigure(byte[] reconfig) {
        processKeyExportReconfigure(getNativeClientPtr(), reconfig);
    }
    
    /**
     * Reset the key export client object, discarding any state associated with a pending key export request. 
     */
    public void reset() {
        resetNativeClient(getNativeClientPtr());
    }
    
    /**
     * True if key export responses from Nest development devices will be allowed.
     */
    public boolean allowNestDevelopmentDevices() {
        return allowNestDevelopmentDevices(getNativeClientPtr());
    }
    
    /**
     * Allow or disallow key export responses from Nest development devices.
     */
    public void setAllowNestDevelopmentDevices(boolean val) {
        setAllowNestDevelopmentDevices(getNativeClientPtr(), val);
    }
    
    /**
     * True if key export responses from devices with SHA1 certificates will be allowed.
     */
    public boolean allowSHA1DeviceCertificates() {
        return allowSHA1DeviceCertificates(getNativeClientPtr());
    }
    
    /**
     * Allow or disallow key export responses from devices with SHA1 certificates.
     */
    public void setAllowSHA1DeviceCertificates(boolean val) {
        setAllowSHA1DeviceCertificates(getNativeClientPtr(), val);
    }
    
    @Override
    public void finalize() throws Throwable {
        try {
            if (_nativeClientPtr != 0) {
                long tmpNativeClientPtr = _nativeClientPtr;
                _nativeClientPtr = 0;
                releaseNativeClient(tmpNativeClientPtr);
            }
        }
        catch (Throwable t) {
            throw t;
        }
        finally {
            super.finalize();
        }
    }

    private long _nativeClientPtr;
    
    private long getNativeClientPtr() {
        if (_nativeClientPtr == 0) {
            _nativeClientPtr = newNativeClient();
        }
        return _nativeClientPtr;
    }
    
    static {
        WeaveSecuritySupport.forceLoad();
    }

    private static native long newNativeClient();
    private static native void releaseNativeClient(long nativeClientPtr);
    private static native void resetNativeClient(long nativeClientPtr);
    private static native byte[] generateKeyExportRequest(long nativeClientPtr, int keyId, long responderNodeId, byte[] clientCert, byte[] clientKey);
    private static native byte[] generateKeyExportRequest(long nativeClientPtr, int keyId, long responderNodeId, byte[] accessToken);
    private static native byte[] processKeyExportResponse(long nativeClientPtr, long responderNodeId, byte[] exportResp);
    private static native void processKeyExportReconfigure(long nativeClientPtr, byte[] reconfig);
    private static native boolean allowNestDevelopmentDevices(long nativeClientPtr);
    private static native void setAllowNestDevelopmentDevices(long nativeClientPtr, boolean val);
    private static native boolean allowSHA1DeviceCertificates(long nativeClientPtr);
    private static native void setAllowSHA1DeviceCertificates(long nativeClientPtr, boolean val);
}
