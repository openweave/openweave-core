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
 *      Unit tests for pairing code utility functions.
 */

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <nltest.h>

#define NL_TEST_ASSERT_EXIT(inSuite, inCondition)     \
    do {                                              \
        (inSuite)->performedAssertions += 1;          \
                                                      \
        if (!(inCondition))                           \
        {                                             \
            printf("Failed assert: %s in %s:%u\n",    \
                   #inCondition, __FILE__, __LINE__); \
            (inSuite)->failedAssertions += 1;         \
            (inSuite)->flagError = true;              \
            goto exit;                                \
        }                                             \
    } while (0)

#include <Weave/Core/WeaveError.h>
#include <Weave/Support/pairing-code/PairingCodeUtils.h>

using namespace nl::PairingCode;

static inline bool IsValidPairingCodeChar(char ch)
{
    if (ch >= '0' && ch <= '9')
        return true;
    if (ch >= 'A' && ch <= 'H')
        return true;
    if (ch >= 'J' && ch <= 'N')
        return true;
    if (ch == 'P')
        return true;
    if (ch >= 'R' && ch <= 'Y')
        return true;
    return false;
}

static void CheckPairingCodeChars(nlTestSuite *inSuite, const char *pairingCode)
{
    for (; *pairingCode != 0; pairingCode++)
    {
        NL_TEST_ASSERT(inSuite, ::IsValidPairingCodeChar(*pairingCode));
    }
}

// Test Cases

static void Test_IntEncodeDecode(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    enum
    {
        kPairingCodeLength = 5,
        kNumPairingCodes = 1 << ((kPairingCodeLength - 1) * nl::PairingCode::kBitsPerCharacter),
    };
    char pairingCodeStr[kPairingCodeLength + 1];

    /// For all possible 5 character pairing codes...
    for (uint64_t i = 0; i < kNumPairingCodes; i++)
    {
        // Generate the current pairing code.
        err = IntToPairingCode(i, kPairingCodeLength, pairingCodeStr);
        NL_TEST_ASSERT_EXIT(inSuite, err == WEAVE_NO_ERROR);

        // Verify the correct length string was generated.
        NL_TEST_ASSERT_EXIT(inSuite, strlen(pairingCodeStr) == kPairingCodeLength);

        // Verify that the characters are correct.
        CheckPairingCodeChars(inSuite, pairingCodeStr);

        // Decode the pairing code back to an integer.
        uint64_t decodedI;
        err = PairingCodeToInt(pairingCodeStr, kPairingCodeLength, decodedI);
        NL_TEST_ASSERT_EXIT(inSuite, err == WEAVE_NO_ERROR);

        // Verify that the decoded integer value is correct.
        NL_TEST_ASSERT_EXIT(inSuite, i == decodedI);
    }

exit:
    return;
}

static char MutatePairingCodeChar(char ch)
{
    int mutation;

    // Generate a "random" mutation value in the range 1..31
    mutation = ((rand() >> 3) % 31) + 1;

    // Using the mutation value, permute the given character to another character in the pairing code character set.
    int chVal = PairingCodeCharToInt(ch);
    chVal = (chVal + mutation) % 32;
    ch = IntToPairingCodeChar(chVal);

    return ch;
}

static void Test_CheckCharacter(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    enum
    {
        kPairingCodeLength = 5,
        kNumPairingCodes = 1 << ((kPairingCodeLength - 1) * nl::PairingCode::kBitsPerCharacter),
    };
    char pairingCodeStr[kPairingCodeLength + 1];

    // For all possible 5 character pairing codes...
    for (uint64_t i = 0; i < kNumPairingCodes; i++)
    {
        // Create the pairing code.
        err = IntToPairingCode(i, kPairingCodeLength, pairingCodeStr);
        NL_TEST_ASSERT_EXIT(inSuite, err == WEAVE_NO_ERROR);

        // Verify the check character.
        err = VerifyPairingCode(pairingCodeStr, kPairingCodeLength);
        NL_TEST_ASSERT_EXIT(inSuite, err == WEAVE_NO_ERROR);

        // For each character in the current pairing code...
        for (size_t charIndex = 0; charIndex < kPairingCodeLength; charIndex++)
        {
            char ch = pairingCodeStr[charIndex];

            // Randomly mutate the character.
            pairingCodeStr[charIndex] = MutatePairingCodeChar(ch);

            // Confirm that VerifyPairingCode() detects the error.
            err = VerifyPairingCode(pairingCodeStr, kPairingCodeLength);
            NL_TEST_ASSERT_EXIT(inSuite, err == WEAVE_ERROR_INVALID_ARGUMENT);

            pairingCodeStr[charIndex] = ch;
        }
    }

exit:
    return;
}

static void Test_NevisPairingCode(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err;
    uint64_t deviceId;
    char pairingCodeBuf[kStandardPairingCodeLength + 1];

    err = NevisPairingCodeToDeviceId("004HLX", deviceId);
    NL_TEST_ASSERT_EXIT(inSuite, err == WEAVE_NO_ERROR);
    NL_TEST_ASSERT_EXIT(inSuite, deviceId == 0x18B4300400001234ull);

    err = NevisDeviceIdToPairingCode(0x18B4300400001234ull, pairingCodeBuf, sizeof(pairingCodeBuf));
    NL_TEST_ASSERT_EXIT(inSuite, err == WEAVE_NO_ERROR);
    NL_TEST_ASSERT_EXIT(inSuite, strcmp(pairingCodeBuf, "004HLX") == 0);

exit:
    return;
}

static void Test_KryptonitePairingCode(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err;
    uint64_t deviceId;
    char pairingCodeBuf[kKryptonitePairingCodeLength + 1];

    err = KryptonitePairingCodeToDeviceId("1XNDP3WW3", deviceId);
    NL_TEST_ASSERT_EXIT(inSuite, err == WEAVE_NO_ERROR);
    NL_TEST_ASSERT_EXIT(inSuite, deviceId == 0x18B430CFACDB8FBDull);

    err = KryptoniteDeviceIdToPairingCode(0x18B430CFACDB8FBDull, pairingCodeBuf, sizeof(pairingCodeBuf));
    NL_TEST_ASSERT_EXIT(inSuite, err == WEAVE_NO_ERROR);
    NL_TEST_ASSERT_EXIT(inSuite, strcmp(pairingCodeBuf, "1XNDP3WW3") == 0);

exit:
    return;
}

static void Test_Normalization(nlTestSuite *inSuite, void *inContext)
{
    char pairingCodeBuf[kStandardPairingCodeLength + 20];
    size_t pairingCodeLen;

    // 'I', 'O', 'Q' and 'Z' to '1', '0', '0' and '2'
    strcpy(pairingCodeBuf, "HZOWQI");
    pairingCodeLen = strlen(pairingCodeBuf);
    NormalizePairingCode(pairingCodeBuf, pairingCodeLen);
    NL_TEST_ASSERT_EXIT(inSuite, pairingCodeLen == kStandardPairingCodeLength);
    NL_TEST_ASSERT_EXIT(inSuite, strcmp(pairingCodeBuf, "H20W01") == 0);

    // Remove simple whitespace (' ', '\t', '\r', '\n') and punctuation ('-', '.').
    strcpy(pairingCodeBuf, "  H\r\n\nR-D-W6.7\t");
    pairingCodeLen = strlen(pairingCodeBuf);
    NormalizePairingCode(pairingCodeBuf, pairingCodeLen);
    NL_TEST_ASSERT_EXIT(inSuite, pairingCodeLen == kStandardPairingCodeLength);
    NL_TEST_ASSERT_EXIT(inSuite, strcmp(pairingCodeBuf, "HRDW67") == 0);

exit:
    return;
}



// Test Suite

/**
 *  Test Suite that lists all the test functions.
 */
static const nlTest sTests[] =
{
    NL_TEST_DEF("Integer encode / decode tests", Test_IntEncodeDecode),
    NL_TEST_DEF("Check character tests", Test_CheckCharacter),
    NL_TEST_DEF("Normalization tests", Test_Normalization),
    NL_TEST_DEF("Nevis pairing code tests", Test_NevisPairingCode),
    NL_TEST_DEF("Kryptonite pairing code tests", Test_KryptonitePairingCode),

    NL_TEST_SENTINEL()
};

static int TestSetup(void *inContext)
{
    // Nothing to do.
    return SUCCESS;
}

static int TestTeardown(void *inContext)
{
    // Nothing to do.
    return SUCCESS;
}

int main(int argc, char *argv[])
{
    nlTestSuite theSuite =
    {
        "pairing-code-utils",
        &sTests[0],
        TestSetup,
        TestTeardown
    };

    // Always use a repeatable sequence of "random" values to drive the test.
    srand(42);

    // Generate machine-readable, comma-separated value (CSV) output.
    nl_test_set_output_style(OUTPUT_CSV);

    // Run test suit against one context
    nlTestRunner(&theSuite, NULL);

    return nlTestRunnerStats(&theSuite);
}
