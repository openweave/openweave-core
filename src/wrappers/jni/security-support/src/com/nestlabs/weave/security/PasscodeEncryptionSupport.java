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

/** Utility methods for encrypting and decrypting passcode using the Nest Passcode Encryption scheme.
 */
public final class PasscodeEncryptionSupport {

    /** Encrypt a passcode using the Nest Passcode Encryption scheme.
     */
    public static native byte[] encryptPasscode(int config, int keyId, long nonce, String passcode, byte[] encKey, byte[] authKey, byte[] fingerprintKey)
        throws WeaveSecuritySupportException;
    
    /** Decrypt a passcode that was encrypted using the Nest Passcode Encryption scheme.
     */
    public static native String decryptPasscode(byte[] encryptedPasscode, byte[] encKey, byte[] authKey, byte[] fingerprintKey)
        throws WeaveSecuritySupportException;
    
    /** Determines if the specified Passcode encryption configuration is supported.
     */
    public static native boolean isSupportedPasscodeEncryptionConfig(int config);
    
    /** Extract the configuration type from an encrypted Passcode.
     */
    public static native int getEncryptedPasscodeConfig(byte[] encryptedPasscode)
        throws WeaveSecuritySupportException;
    
    /** Extract the key id from an encrypted Passcode.
     */
    public static native int getEncryptedPasscodeKeyId(byte[] encryptedPasscode)
        throws WeaveSecuritySupportException;
        
    /** Extract the nonce value from an encrypted Passcode.
     */
    public static native long getEncryptedPasscodeNonce(byte[] encryptedPasscode)
        throws WeaveSecuritySupportException;
        
    /** Extract the fingerprint from an encrypted Passcode.
     */
    public static native byte[] getEncryptedPasscodeFingerprint(byte[] encryptedPasscode)
        throws WeaveSecuritySupportException;

    /** Passcode encryption configuration 1 (TEST ONLY)
     * 
     * Note: This encryption configuration is for testing only and provides no integrity or confidentiality.
     * Config 1 is only available in development builds.
     */
    public static final int kPasscodeEncryptionConfig1_TEST_ONLY = 0x01;

    /** Passcode encryption configuration 2
     */
    public static final int kPasscodeEncryptionConfig2 = 0x02;
    
    /** Key diversifier used in the derivation of the passcode encryption and authentication keys.
     */
    public static final byte[] kPasscodeEncKeyDiversifier = new byte[] { (byte)0x1A, (byte)0x65, (byte)0x5D, (byte)0x96 };
    
    /** Key diversifier used in the derivation of the passcode fingerprint key.
     */
    public static final byte[] kPasscodeFingerprintKeyDiversifier = new byte[] { (byte)0xD1, (byte)0xA1, (byte)0xD9, (byte)0x6C };

    static {
        WeaveSecuritySupport.forceLoad();
    }
}
