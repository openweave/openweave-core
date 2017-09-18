/*
 *
 *    Copyright (c) 2013-2017 Nest Labs, Inc.
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
 * Support methods for dealing with Weave certificates.
 */
public final class WeaveCertificateSupport {

    /**
     * Convert a certificate in Weave format to the equivalent X.509 certificate.
     * 
     * @param certBuf       A byte array containing the Weave certificate to be converted. 
     * @return              A byte array containing the resultant X.509 certificate.
     */
    public static byte[] weaveCertificateToX509(byte[] certBuf) 
            throws WeaveSecuritySupportException {
        return weaveCertificateToX509(certBuf, 0, certBuf.length);
    }

    /**
     * Convert a certificate in Weave format to the equivalent X.509 certificate.
     * 
     * @param certBuf       A byte array containing the Weave certificate to be converted.
     * @param offset        The offset into certBuf at which the certificate begins.
     * @param length        The length of the Weave certificate. 
     * @return              A byte array containing the resultant X.509 certificate.
     */
    public static native byte[] weaveCertificateToX509(byte[] certBuf, int offset, int length)
            throws WeaveSecuritySupportException;

    /**
     * Convert a certificate in X.509 format to the equivalent Weave certificate.
     * 
     * @param certBuf       A byte array containing the  X.509 certificate to be converted. 
     * @return              A byte array containing the resultant Weave certificate.
     */
    public static byte[] x509CertificateToWeave(byte[] certBuf)
            throws WeaveSecuritySupportException {
        return x509CertificateToWeave(certBuf, 0, certBuf.length);
    }

    /**
     * Convert a certificate in X.509 format to the equivalent Weave certificate.
     * 
     * @param certBuf       A byte array containing the  X.509 certificate to be converted.
     * @param offset        The offset into certBuf at which the certificate begins.
     * @param length        The length of the  X.509 certificate. 
     * @return              A byte array containing the resultant Weave certificate.
     */
    public static native byte[] x509CertificateToWeave(byte[] certBuf, int offset, int length)
            throws WeaveSecuritySupportException;
    
    static {
        WeaveSecuritySupport.forceLoad();
    }
}
