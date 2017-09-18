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

/**
 *    @file
 *      Utility functions for working with Nest pairing codes.
 *
 */

#include <string.h>
#include <ctype.h>

#include <Weave/Support/CodeUtils.h>
#include <Weave/Support/verhoeff/Verhoeff.h>
#include <Weave/Support/pairing-code/PairingCodeUtils.h>

namespace nl {
namespace PairingCode {

enum {
    kCharacterValueMask = (1 << kBitsPerCharacter) - 1,
    kUInt64OverflowMask = (uint64_t)kCharacterValueMask << (64ULL - kBitsPerCharacter),
};


/** Verify a Nest pairing code against its check character.
 *
 * @param[in]   pairingCode             The pairing code string to be checked.  This string
 *                                      does not need to be NULL terminated.
 * @param[in]   pairingCodeLen          The length of the pairing code string, not including
 *                                      any NULL terminator character.  Must be >= 2.
 *
 * @retval #WEAVE_NO_ERROR              If the method succeeded.
 * @retval #WEAVE_ERROR_INVALID_ARGUMENT If pairingCodeLen is < 2, or the initial characters
 *                                      of the pairing code are not consistent with the value
 *                                      of the check character.
 */
WEAVE_ERROR VerifyPairingCode(const char *pairingCode, size_t pairingCodeLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // Enforce minimum length of 1 value character plus the check character.
    VerifyOrExit(pairingCodeLen >= kPairingCodeLenMin, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Verify the value portion of the pairing code against the trailing check character.
    if (!Verhoeff32::ValidateCheckChar(pairingCode[pairingCodeLen - 1], pairingCode, pairingCodeLen - 1))
        err = WEAVE_ERROR_INVALID_ARGUMENT;

exit:
    return err;
}

/** Normalize the characters in a pairing code string.
 *
 * This function converts all alphabetic characters to upper-case, maps the illegal characters 'I', 'O',
 * 'Q' and 'Z' to '1', '0', '0' and '2', respectively, and removes all other non-pairing code characters
 * from the given string.
 *
 * The input string is not required to be NULL terminated, however if it is the output will be NULL
 * terminated as well.
 *
 * @param[inout] pairingCode            On input, the pairing code string to be normalized. On output,
 *                                      The normalized string.  It is not necessary for the string to be
 *                                      NULL terminated.
 * @param[inout] pairingCodeLen         On input, the length of the pairing code string, not including
 *                                      any NULL terminator character.  On output, the length of the
 *                                      normalized string.
 *
 */
void NormalizePairingCode(char *pairingCode, size_t& pairingCodeLen)
{
    size_t newLen = 0;

    for (size_t i = 0; i < pairingCodeLen; i++)
    {
        char ch = pairingCode[i];

        ch = (char)toupper(ch);

        if (ch == 'I')
            ch = '1';
        else if (ch == 'O' || ch == 'Q')
            ch = '0';
        else if (ch == 'Z')
            ch = '2';

        if (Verhoeff32::CharToVal(ch) < 0)
        	continue;

        pairingCode[newLen] = ch;

        newLen++;
    }

    if (newLen < pairingCodeLen)
        pairingCode[newLen] = 0;

    pairingCodeLen = newLen;
}

/** Encode an integer value as a Nest pairing code.
 *
 * The function generates a Nest pairing code string consisting of a supplied unsigned integer
 * value, encoded as a big-endian, base-32 numeral, plus a trailing Verhoeff check character.
 * The generated string has a fixed length specified by the pairingCodeLen parameter. The
 * string is padded on the left with zeros as necessary to meet this length.
 *
 * @param[in]   val                     The value to be encoded.
 * @param[in]   pairingCodeLen          The desired length of the encoded pairing code string,
 *                                      including the trailing check character.  Must be >= 2.
 * @param[out]  outBuf                  A pointer to a character buffer that will receive the
 *                                      encoded pairing code, plus a null terminator character.
 *                                      The supplied buffer should be at least as big as
 *                                      pairingCodeLen + 1.
 *
 * @retval #WEAVE_NO_ERROR              If the method succeeded.
 * @retval #WEAVE_ERROR_INVALID_ARGUMENT If pairingCodeLen is < 2 or the supplied integer value
 *                                      cannot be encoded in the number of characters specified
 *                                      by pairingCodeLen, minus 1 for the check character.
 *
 */
WEAVE_ERROR IntToPairingCode(uint64_t val, uint8_t pairingCodeLen, char *outBuf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // Enforce minimum length of 1 value character plus check character.
    VerifyOrExit(pairingCodeLen >= kPairingCodeLenMin, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Encode value as big-endian base-32 integer numeral.
    for (int i = pairingCodeLen - 2; i >= 0; i--)
    {
        outBuf[i] = Verhoeff32::ValToChar(val & kCharacterValueMask);
        val >>= kBitsPerCharacter;
    }

    // Fail if value overflows the specified length.
    VerifyOrExit(val == 0, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Add the check character to the end of the pairing code string.
    outBuf[pairingCodeLen - 1] = Verhoeff32::ComputeCheckChar(outBuf, pairingCodeLen - 1);

    // Terminate the string.
    outBuf[pairingCodeLen] = '\0';

exit:
    return err;
}

/** Decode a Nest pairing code as an integer value.
 *
 * The function parses the initial characters of a Nest pairing code string as a big-endian,
 * base-32 numeral and returns the resultant value as an unsigned integer.  The input string
 * can be any length >= 2 so long as the decoded integer fits within a uint64_t.
 *
 * No attempt is made to verify the Verhoeff check character (see #VerifyPairingCode()).
 *
 * @param[in]   pairingCode             The pairing code string to be decoded.  This string
 *                                      does not need to be NULL terminated.
 * @param[in]   pairingCodeLen          The length of the pairing code string, not including
 *                                      any NULL terminator character.  Must be >= 2.
 * @param[out]  val                     The decoded integer value.
 *
 * @retval #WEAVE_NO_ERROR              If the method succeeded.
 * @retval #WEAVE_ERROR_INVALID_ARGUMENT If pairingCodeLen is < 2, or the supplied pairing
 *                                      code string contains an invalid character, or the
 *                                      integer value encoded in the pairing code exceeds
 *                                      the max value that can be stored in a uint64_t.
 *
 */
WEAVE_ERROR PairingCodeToInt(const char *pairingCode, size_t pairingCodeLen, uint64_t& val)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    val = 0;

    // Enforce minimum length of 1 value character plus the check character.
    VerifyOrExit(pairingCodeLen >= kPairingCodeLenMin, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Decode the initial characters of the pairing code (not including the trailing check character)
    // as a big-endian, base-32 numeral...
    for (size_t i = 0; i < pairingCodeLen - 1; i++)
    {
        // Convert the character to its equivalent integer value.
        int chVal = Verhoeff32::CharToVal(pairingCode[i]);

        // Fail if the character is invalid.
        VerifyOrExit(chVal >= 0, err = WEAVE_ERROR_INVALID_ARGUMENT);

        // Verify that the decoded value does not overflow what will fit in a uint64_t.
        VerifyOrExit((val & kUInt64OverflowMask) == 0, err = WEAVE_ERROR_INVALID_ARGUMENT);

        // Add the character value to the accumulated total.
        val = (val << kBitsPerCharacter) | (uint64_t)chVal;
    }

exit:
    return err;
}

/** Returns true if a supplied character is a valid Nest pairing code character.
 *
 * Note that this function is case-insensitive.
 *
 * @param[in]   ch                      The character to be tested.
 *
 * @returns                             True if a supplied character is a valid Nest pairing code character.
 */
bool IsValidPairingCodeChar(char ch)
{
    return Verhoeff32::CharToVal(ch) >= 0;
}

/** Convert a Nest pairing code character to an integer value in the range 0..31
 *
 * Note that this function is case-insensitive.
 *
 * @param[in]   ch                      The character to be converted.
 *
 * @returns                             An integer value corresponding to the specified pairing
 *                                      code character, or -1 if ch is not a valid character.
 */
int PairingCodeCharToInt(char ch)
{
    return Verhoeff32::CharToVal(ch);
}

/** Convert an integer value in the range 0..31 to its corresponding Nest pairing code character.
 *
 * Note that this function always produces upper-case characters.
 *
 * @param[in]   val                     The integer value to be converted.
 *
 * @returns                             The pairing code character that corresponds to the specified
 *                                      integer value, or 0 if the integer value is out of range.
 */
char IntToPairingCodeChar(int val)
{
    return Verhoeff32::ValToChar(val);
}



} // namespace PairingCode
} // namespace nl
