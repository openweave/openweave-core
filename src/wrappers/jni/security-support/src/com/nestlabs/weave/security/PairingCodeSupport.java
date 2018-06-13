/*
 *
 *    Copyright (c) 2018 Nest Labs, Inc.
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

/** Utility methods for working with Weave pairing codes.
 */
public final class PairingCodeSupport {
    
    /** Set of characters that are valid in a Weave pairing code. */
    public static final String PAIRING_CODE_CHARS = "0123456789ABCDEFGHJKLMNPRSTUVWXY";

    /** Pairing code length for most Nest products. */
    public static final int STANDARD_PAIRING_CODE_LENGTH = 6; 

    /** Minimum length of a pairing code. 
     * 
     * NOTE: While this value defines the technical minimum size of a pairing code, the
     * security strength of a pairing code is *highly* dependent on its length, with
     * longer being better.  When choosing the length of a pairing code, product developers
     * must take into consideration the maximal rate of brute-force attack against the product,
     * and choose a length that prevents such attacks from completing in reasonable time.
     * In practice, a length of 5 random characters plus the check digit should be considered
     * the absolute minimum length given suitable rate limiting. 
     */
    public static final int MIN_PAIRING_CODE_LENGTH = 2; // 1 value character plus 1 check character
    
    /** Number of bits encoded in a single pairing code character. */
    public static final int BITS_PER_CHARACTER = 5;
    
    /** Determine whether a given pairing code is valid. 
     * 
     * Note that this function is case-insensitive.
     * 
     * @param pairingCode           The pairing code to be validated.
     * @return                      true if the pairing code consist of valid characters and
     *                              includes a correct check character. 
     */
    public static native boolean isValidPairingCode(String pairingCode);
    
    /** Normalize the characters in a pairing code string.
     * 
     * This method converts all alphabetic characters to upper-case, maps the illegal characters 'I', 'O',
     * 'Q' and 'Z' to '1', '0', '0' and '2', respectively, and removes all other non-pairing code characters
     * from the given string.
     * 
     * @param pairingCode           The pairing code to be normalized.
     * @return                      The normalized pairing code.
     */
    public static native String normalizePairingCode(String pairingCode);
    
    /** Compute the Verhoeff check character for a given input string.
     *  
     * Note that this function is case-insensitive.
     * 
     * @param basePairingCodeChars  The base characters of the pairing code, not including
     *                              a check character.
     * @return                      The computed Verhoeff check character. 
     */
    public static native char computeCheckChar(String basePairingCodeChars);

    /** Append a Verhoeff check character to a given input string.
     *  
     * Note that this function is case-insensitive.
     * 
     * @param basePairingCodeChars  The base characters of the pairing code, not including
     *                              a check character.
     * @return                      The characters from basePairingCodeChars with the Verhoeff
     *                              check character appended.
     */
    public static native String addCheckChar(String pairingCodeChars);

    /** Encode an integer value as a Weave pairing code.
     *
     * Generates a Weave pairing code string consisting of the supplied unsigned integer value, encoded as
     * a big-endian, base-32 numeral, plus a trailing Verhoeff check character. Negative values are encoded
     * in 2's compliment form.
     * 
     * The generated string has a fixed length specified by the pairingCodeLen parameter.  The string will
     * be padded on the left with zeros as necessary to meet this length.  If the value supplied exceeds what
     * can be encoded in the specified length an argument exception will be thrown. 
     * 
     * @param val                   The integer value to be encoded in the pairing code.
     * @param pairingCodeLen        The length of pairing code to be generated, including the 
     *                              check character.
     * @return                      A pairing code of the specified length which encodes the given value. 
     */
    public static native String intToPairingCode(long val, int pairingCodeLen);

    /** Decode a Weave pairing code as an integer value.
     *
     * Parses the initial characters of a Weave pairing code string (not including the check character) as
     * a big-endian, base-32 numeral and returns the resultant value as an integer.  The input string can
     * be any length >= 2 so long as the decoded integer fits within 64-bits; values exceeding this limit
     * result in an argument exception being thrown. Values in the range 2^63 to 2^64-1 are interpreted as
     * 2's complement negative numbers, resulting in a negative integer being returned.
     * 
     * This method does not verify the correctness of check character.
     * 
     * @param pairingCode           The pairing code to be decoded.
     * @return                      The decoded integer value. 
     */
    public static native long pairingCodeToInt(String pairingCode);

    /** Returns true if a supplied character is a valid Weave pairing code character.
     *
     * Note that this function is case-insensitive.
     * 
     * @param ch                    The character to be validated.
     * @return                      true if the character is a valid pairing code character
     *                              (independent of case).
     */
    public static native boolean isValidPairingCodeChar(char ch);
    
    /** Generate a random pairing code with a given length.
     * 
     * @param pairingCodeLen        The desired length of the generated pairing code, including
     *                              the check character.
     * @return                      A random pairing code.
     */
    public static native String generatePairingCode(int pairingCodeLen);

    static {
        WeaveSecuritySupport.forceLoad();
    }
}
