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
 * Java implementation of the HKDF key derivation function, as defined in RFC-5869.
 */
public class HKDF {

    public void beginExtractKey() {
        beginExtractKey(null);
    }

    public void beginExtractKey(byte[] salt) {
        if (mState != State.INIT) {
            throw new IllegalStateException();
        }
        // NOTE: The check for length==0 works-around a bug in Java's HMAC implementation
        // which disallows zero-length keys, even though this is allowed by the HMAC spec.
        if (salt == null || salt.length == 0) {
            salt = new byte[] { 0 }; 
        }
        initMac(salt);
        mState = State.EXTRACT;
    }
    
    public void addKeyMaterial(byte[] keyMaterial) {
        if (mState != State.EXTRACT) {
            throw new IllegalStateException();
        }
        mMac.update(keyMaterial, 0, keyMaterial.length);
    }
    
    public void finishExtractKey() {
        if (mState != State.EXTRACT) {
            throw new IllegalStateException();
        }
        mPseudoRandomKey = mMac.doFinal();
        mState = State.EXPAND;
    }
    
    public byte[] expandKey(int requestedKeyLen, byte[] info) {
        if (mState != State.EXPAND) {
            throw new IllegalStateException();
        }
        final int macLen = mMac.getMacLength();
        if (requestedKeyLen < 1 || requestedKeyLen > 255 * macLen) {
            throw new IllegalArgumentException("requestedKeyLen too large");
        }
        byte[] generatedKey = new byte[requestedKeyLen];
        for (int generatedKeyLen = 0; generatedKeyLen < requestedKeyLen; generatedKeyLen += macLen) {
            initMac(mPseudoRandomKey);
            if (generatedKeyLen > 0) {
                mMac.update(generatedKey, generatedKeyLen - macLen, macLen);
            }
            if (info != null) {
                mMac.update(info);
            }
            mMac.update((byte) ((generatedKeyLen / macLen) + 1));
            byte[] newKeyData = mMac.doFinal();
            System.arraycopy(newKeyData, 0, generatedKey, generatedKeyLen, Math.min(requestedKeyLen - generatedKeyLen, macLen));
        }
        return generatedKey;
    }

    public void reset() {
        mMac.reset();
        mPseudoRandomKey = null;
        mState = State.INIT;
    }

    public byte[] pseudoRandomKey() {
        return mPseudoRandomKey;
    }

    public static HKDF getInstance(String algorithm) throws java.security.NoSuchAlgorithmException {
        if (algorithm.equalsIgnoreCase("HKDFSHA1"))
            return new HKDF(javax.crypto.Mac.getInstance("HmacSHA1"));
        if (algorithm.equalsIgnoreCase("HKDFSHA256"))
            return new HKDF(javax.crypto.Mac.getInstance("HmacSHA256"));
        throw new java.security.NoSuchAlgorithmException("Unknown HKDF algorithm: " + algorithm);
    }

    // ===== Private Members =====

    private enum State {
        INIT,
        EXTRACT,
        EXPAND,
    };
    
    private State mState;
    private javax.crypto.Mac mMac;
    private byte[] mPseudoRandomKey;
    
    private HKDF(javax.crypto.Mac mac) {
        mState = State.INIT;
        mMac = mac;
    }
    
    private void initMac(byte[] key) {
        try {
            mMac.init(new javax.crypto.spec.SecretKeySpec(key, mMac.getAlgorithm()));
        }
        catch (java.security.InvalidKeyException ex) {
            throw new IllegalArgumentException("Invalid MAC algorithm supplied for HKDF");
        }
    }
}
