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
 *  Utility methods and definitions for working with Weave Application keys. 
 */
public final class ApplicationKeySupport {

    /** Diversifier value used to derive an intermediate key from a root key plus an epoch key.
     */
    final public static byte[] kAppIntermediateKeyDiversifier = new byte[] { (byte)0xBC, (byte)0xAA, (byte)0x95, (byte)0xAD };
    
    /** The size (in bytes) of a derived intermediate key.
     */
    final public static int kAppIntermediateKeySize = 32;
    
    public static byte[] deriveApplicationKey(int keyId, ConstituentKeySource keySource, 
            byte[] salt, byte[] diversifier, int keyLen) throws Exception {
        
        byte[] appKey = null;

        int keyType = WeaveKeyId.getKeyType(keyId);

        // Verify the key type being requested.
        if (keyType != WeaveKeyId.kKeyType_AppStaticKey && keyType != WeaveKeyId.kKeyType_AppRotatingKey) {
            throw new IllegalArgumentException("Invalid application key type specified");
        }

        // Raise an error if the caller has specified the "current epoch key" flag. The supplied key id is
        // for a rotating key, it must reference a specific epoch key.
        if (WeaveKeyId.usesCurrentEpochKey(keyId)) {
            throw new IllegalArgumentException("Invalid application key type specified");
        }

        // Search the key store for the requested root key. Fail with an exception if the key isn't found.
        final int rootKeyId = WeaveKeyId.getRootKeyId(keyId);
        final byte[] rootKey = keySource.getKey(rootKeyId);

        // Search the key store for the requested application group master key.
        // Fail with an exception if it isn't found.
        final int appGroupMasterKeyId = WeaveKeyId.getAppGroupMasterKeyId(keyId);
        final byte[] appGroupMasterKey = keySource.getKey(appGroupMasterKeyId);

        HKDF kdf = HKDF.getInstance("HKDFSHA1");

        try {
            final byte[] intermediateOrRootKey;

            // If deriving a rotating key (i.e. a key that incorporates an epoch
            // key)...
            if (keyType == WeaveKeyId.kKeyType_AppRotatingKey) {

                // Search the key store for the associated epoch key. Fail with an exception if it isn't found.
                final int epochKeyId = WeaveKeyId.getEpochKeyId(keyId);
                final byte[] epochKey = keySource.getKey(epochKeyId);

                // Derive an intermediate key from the root key and the selected
                // epoch key.
                kdf.beginExtractKey();
                kdf.addKeyMaterial(rootKey);
                kdf.addKeyMaterial(epochKey);
                kdf.finishExtractKey();
                intermediateOrRootKey = kdf.expandKey(kAppIntermediateKeySize, kAppIntermediateKeyDiversifier);
                kdf.reset();
            }

            // Otherwise we're deriving a static (non-rotating) key, so...
            else {

                // Arrange to derive the application key directly from the root key.
                intermediateOrRootKey = rootKey;
            }

            // Derive the requested application key from either the root key or
            // an intermediate key, plus the
            // specified application group key. Use the caller-specified salt
            // and diversifier values in the
            // key derivation.
            kdf.beginExtractKey(salt);
            kdf.addKeyMaterial(intermediateOrRootKey);
            kdf.addKeyMaterial(appGroupMasterKey);
            kdf.finishExtractKey();
            appKey = kdf.expandKey(keyLen, diversifier);
        } finally {
            kdf.reset();
        }

        return appKey;
    }
    
    public interface ConstituentKeySource {
        public byte[] getKey(int keyId) throws Exception;
    }
    
    static {
        WeaveSecuritySupport.forceLoad();
    }
}
