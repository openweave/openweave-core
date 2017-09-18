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

package nl.Weave.DeviceManager;

/**
 * Utility functions for working with Nest pairing codes.
 */
public class PairingCodeUtils
{
    /** Pairing code length for most Nest products. */
    public static final int STANDARD_PAIRING_CODE_LENGTH = 6;

    /** Pairing code length for Kryptonite. */
    public static final int KRYPTONITE_PAIRING_CODE_LENGTH = 9;

    /** Number of bits encoded in a single pairing code character. */
    public static final int BITS_PER_CHARACTER = 5;

    /** Verify the syntax and integrity of a Nest pairing code.
     * 
     * @param pairingCode          The pairing code.
     */
    public native static boolean isValidPairingCode(String pairingCode);

    /** Normalize the characters in a pairing code string.
     *
	 * This function converts all alphabetic characters to upper-case, maps the illegal characters 'I', 'O',
	 * 'Q' and 'Z' to '1', '0', '0' and '2', respectively, and removes all other non-pairing code characters
	 * from the given string.
     * 
     * @param pairingCode          The pairing code.
     */
    public native static String normalizePairingCode(String pairingCode);
    
    /**
     * Returns the device ID corresponding to a given Nevis pairing code.
     * 
     * If the supplied pairing code is not valid, of is not a Nevis pairing code the method returns 0. 
     *
     * @param pairingCode          The Nevis pairing code.
     */
    public native static long nevisPairingCodeToDeviceId(String pairingCode);

    /**
     * Returns the pairing code corresponding to a given Nevis device ID.
     * 
     * If the supplied device id is not a valid Nevis device id, the method returns null.
     *
     * @param deviceId             The Nevis device ID.
     */
    public native static String nevisDeviceIdToPairingCode(long deviceId);

    /**
     * Returns the device ID corresponding to a given Kryptonite pairing code.
     * 
     * If the supplied pairing code is not valid, of is not a Kryptonite pairing code the method returns 0. 
     *
     * @param pairingCode          The Kryptonite pairing code.
     */
    public native static long kryptonitePairingCodeToDeviceId(String pairingCode);

    /**
     * Returns the pairing code corresponding to a given Kryptonite device ID.
     * 
     * If the supplied device id is not a valid Krytponite device id, the method returns null.
     *
     * @param deviceId             The Kryptonite device ID.
     */
    public native static String kryptoniteDeviceIdToPairingCode(long deviceId);

    static {
        System.loadLibrary("WeaveDeviceManager");
    }
}