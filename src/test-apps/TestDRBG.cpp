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

/**
 *    @file
 *      This file implements a process to effect a functional test for
 *      the Weave Deterministic Random Bit Generator (CTR-DRBG).
 *
 */

//#include <Weave/Core/WeaveConfig.h>
#include <stdio.h>
#include <string.h>

#include "ToolCommon.h"
#include "TestDRBG.h"
#include <Weave/Support/crypto/DRBG.h>

using namespace nl::Weave::Crypto;

#define DEBUG_PRINT_ENABLE 0

#define VerifyOrFail(TST, MSG) \
do { \
    if (!(TST)) \
    { \
        fprintf(stderr, "%s FAILED: ", __FUNCTION__); \
        fputs(MSG, stderr); \
        exit(-1); \
    } \
} while (0)

#define SuccessOrFail(ERR, MSG) \
do { \
    if ((ERR) != WEAVE_NO_ERROR) \
    { \
        fprintf(stderr, "%s FAILED: ", __FUNCTION__); \
        fputs(MSG, stderr); \
        fputs(ErrorStr(ERR), stderr); \
        fputs("\n", stderr); \
        exit(-1); \
    } \
} while (0)


static uint8_t *TestEntropy = NULL;
static size_t TestEntropyLen = 0;

// DefaultEntropy() function is compiled out to eliminate compiler warning.
// This function can be used later in case we need to do more testing of DRBG
// engine, where we collect entropy from real source.
#if WEAVE_CONFIG_TEST_DRBG_ENTROPY_COLLECTION
static int DefaultEntropy(uint8_t *entropy, size_t entropyLen)
{
    int randomVal;
    uint8_t *pRandomVal;
    size_t idx;

    if (entropyLen > 0 && !entropy)
        return 1;

    do
    {
        randomVal = rand();

        pRandomVal = (uint8_t *)&randomVal;

        for (idx = 0; (idx < sizeof(int)) && (entropyLen > 0); idx++, entropyLen--)
          *entropy++ = *pRandomVal++;
    }
    while (entropyLen > 0);

    return 0;
}
#endif // WEAVE_CONFIG_TEST_DRBG_ENTROPY_COLLECTION

static int TestEntropyFunct(uint8_t *entropy, size_t entropyLen)
{
    if (entropyLen > 0)
        if (!TestEntropy || !entropy || (TestEntropyLen < entropyLen))
            return 1;

    // Copy Test Entropy Data
    memcpy(entropy, TestEntropy, entropyLen);

    // Update parameters
    TestEntropy += entropyLen;
    TestEntropyLen -= entropyLen;

    return 0;
}

static uint8_t *TestRandomOutput = NULL;
static size_t TestRandomOutputLen = 0;

static int TestRandomOutputFunction(uint8_t *randomOutput, size_t randomOutputLen)
{
    int ret;

    if (randomOutputLen > 0)
        if (!randomOutput || !TestRandomOutput || (randomOutputLen > TestRandomOutputLen))
            return 1;

    // Compare Random data with expected output
    ret = memcmp(randomOutput, TestRandomOutput, randomOutputLen);

    // Update parameters
    TestRandomOutput += randomOutputLen;
    TestRandomOutputLen -= randomOutputLen;

    return ret;
}

static int TestDRBG(uint8_t iterationCount, bool usePR, bool useReseed,
                    uint8_t *entropyInput, size_t entropyInputLen,
                    uint8_t *personalizationString, size_t personalizationStringLen,
                    uint8_t *additionalInput, size_t additionalInputLen,
                    uint8_t *randomOutput, size_t randomOutputLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    int ret;
    AES128CTRDRBG ctrDRBG;
    uint8_t result[64];

#if DEBUG_PRINT_ENABLE
    printf("Starting CTR_DRBG AES-128 Test with the following features:\n");

    if (usePR)
        printf(" - Use Prediction Resistance\n");
    else
        printf(" - No Prediction Resistance\n");
#endif

    // Initialize Entropy Parameters
    TestEntropy = entropyInput;
    TestEntropyLen = entropyInputLen * iterationCount;
    if (usePR)
        TestEntropyLen *= 3;
    else if (useReseed)
        TestEntropyLen *= 2;

    // Initilize Result Check Parameters
    TestRandomOutput = randomOutput;
    TestRandomOutputLen = randomOutputLen * iterationCount;

    for (uint8_t i = 0; i < iterationCount; i++)
    {
#if DEBUG_PRINT_ENABLE
        printf("\tCOUNT = %u\n", i);
#endif

        // DRBG Instantiate Function
        err = ctrDRBG.Instantiate(TestEntropyFunct, entropyInputLen,
				  personalizationString, personalizationStringLen);
        SuccessOrExit(err);

        personalizationString += personalizationStringLen;

        if (useReseed)
        {
            // DRBG Reseed Function
            err = ctrDRBG.Reseed(additionalInput, additionalInputLen);
            SuccessOrExit(err);

            additionalInput += additionalInputLen;
        }

        // DRBG Generate Function (first call)
        err = ctrDRBG.Generate(result, randomOutputLen, additionalInput, additionalInputLen);
        SuccessOrExit(err);
        additionalInput += additionalInputLen;
        // DRBG Generate Function (second call)
        err = ctrDRBG.Generate(result, randomOutputLen, additionalInput, additionalInputLen);
        SuccessOrExit(err);
        additionalInput += additionalInputLen;

        // Check Result
        ret = TestRandomOutputFunction(result, randomOutputLen);
        VerifyOrExit(ret == 0, err = WEAVE_ERROR_DRBG_ENTROPY_SOURCE_FAILED);

        // DRBG Uninstantiate Function
        ctrDRBG.Uninstantiate();
    }

exit:
    return err;
}

// Current DRBG implementation doesn't support noDF option
#define WEAVE_CONFIG_DRBG_WITHOUT_DERIVATION_FUNCTION  0

// ============================================================
// Test Body
// ============================================================

int main(int argc, char *argv[])
{
    WEAVE_ERROR err;

    // ============================================================
    // [AES-128 no df]
    // [PredictionResistance = False]
    // [EntropyInputLen = 256]
    // [NonceLen = 0]
    // [PersonalizationStringLen = 0]
    // [AdditionalInputLen = 0]
    // [ReturnedBitsLen = 512]
    // ============================================================
#if WEAVE_CONFIG_DRBG_WITHOUT_DERIVATION_FUNCTION    // no DF
    err = TestDRBG(NIST_CTR_DRBG_AES128_NoPR_NoDF_Count,
                   NIST_CTR_DRBG_AES128_NoPR_NoDF_PredictionResistance,
                   NIST_CTR_DRBG_AES128_NoPR_NoDF_Reseed,
                   NIST_CTR_DRBG_AES128_NoPR_NoDF_EntropyInput,
                   NIST_CTR_DRBG_AES128_NoPR_NoDF_EntropyInputLen,
                   NIST_CTR_DRBG_AES128_NoPR_NoDF_PersonalizationString,
                   NIST_CTR_DRBG_AES128_NoPR_NoDF_PersonalizationStringLen,
                   NIST_CTR_DRBG_AES128_NoPR_NoDF_AdditionalInput,
                   NIST_CTR_DRBG_AES128_NoPR_NoDF_AdditionalInputLen,
                   NIST_CTR_DRBG_AES128_NoPR_NoDF_ReturnedBytes,
                   NIST_CTR_DRBG_AES128_NoPR_NoDF_ReturnedBytesLen);
    SuccessOrFail(err, "TestDRBG failed in NoPR & NoDF case\n");
#endif

    // ============================================================
    // [AES-128 use df]
    // [PredictionResistance = False]
    // [EntropyInputLen = 128]
    // [NonceLen = 64]
    // [PersonalizationStringLen = 128]
    // [AdditionalInputLen = 128]
    // [ReturnedBitsLen = 512]
    // ============================================================
#if (WEAVE_CONFIG_DRBG_RESEED_INTERVAL > 0)    // PredictionResistance = False
    err = TestDRBG(NIST_CTR_DRBG_AES128_NoPR_UseDF_Count,
                   NIST_CTR_DRBG_AES128_NoPR_UseDF_PredictionResistance,
                   NIST_CTR_DRBG_AES128_NoPR_UseDF_Reseed,
                   NIST_CTR_DRBG_AES128_NoPR_UseDF_EntropyInput,
                   NIST_CTR_DRBG_AES128_NoPR_UseDF_EntropyInputLen,
                   NIST_CTR_DRBG_AES128_NoPR_UseDF_PersonalizationString,
                   NIST_CTR_DRBG_AES128_NoPR_UseDF_PersonalizationStringLen,
                   NIST_CTR_DRBG_AES128_NoPR_UseDF_AdditionalInput,
                   NIST_CTR_DRBG_AES128_NoPR_UseDF_AdditionalInputLen,
                   NIST_CTR_DRBG_AES128_NoPR_UseDF_ReturnedBytes,
                   NIST_CTR_DRBG_AES128_NoPR_UseDF_ReturnedBytesLen);
    SuccessOrFail(err, "TestDRBG failed in NoPR & UseDF case\n");
#endif

    // ============================================================
    // [AES-128 no df]
    // [PredictionResistance = True]
    // [EntropyInputLen = 256]
    // [NonceLen = 0]
    // [PersonalizationStringLen = 256]
    // [AdditionalInputLen = 256]
    // [ReturnedBitsLen = 512]
    // ============================================================
#if WEAVE_CONFIG_DRBG_WITHOUT_DERIVATION_FUNCTION    // no DF
    err = TestDRBG(NIST_CTR_DRBG_AES128_UsePR_NoDF_Count,
                   NIST_CTR_DRBG_AES128_UsePR_NoDF_PredictionResistance,
                   NIST_CTR_DRBG_AES128_UsePR_NoDF_Reseed,
                   NIST_CTR_DRBG_AES128_UsePR_NoDF_EntropyInput,
                   NIST_CTR_DRBG_AES128_UsePR_NoDF_EntropyInputLen,
                   NIST_CTR_DRBG_AES128_UsePR_NoDF_PersonalizationString,
                   NIST_CTR_DRBG_AES128_UsePR_NoDF_PersonalizationStringLen,
                   NIST_CTR_DRBG_AES128_UsePR_NoDF_AdditionalInput,
                   NIST_CTR_DRBG_AES128_UsePR_NoDF_AdditionalInputLen,
                   NIST_CTR_DRBG_AES128_UsePR_NoDF_ReturnedBytes,
                   NIST_CTR_DRBG_AES128_UsePR_NoDF_ReturnedBytesLen);
    SuccessOrFail(err, "TestDRBG failed in UsePR & NoDF case\n");
#endif

    // ============================================================
    // [AES-128 use df]
    // [PredictionResistance = True]
    // [EntropyInputLen = 128]
    // [NonceLen = 64]
    // [PersonalizationStringLen = 0]
    // [AdditionalInputLen = 128]
    // [ReturnedBitsLen = 512]
    // ============================================================
#if (WEAVE_CONFIG_DRBG_RESEED_INTERVAL == 0)    // PredictionResistance = True
    err = TestDRBG(NIST_CTR_DRBG_AES128_UsePR_UseDF_Count,
                   NIST_CTR_DRBG_AES128_UsePR_UseDF_PredictionResistance,
                   NIST_CTR_DRBG_AES128_UsePR_UseDF_Reseed,
                   NIST_CTR_DRBG_AES128_UsePR_UseDF_EntropyInput,
                   NIST_CTR_DRBG_AES128_UsePR_UseDF_EntropyInputLen,
                   NIST_CTR_DRBG_AES128_UsePR_UseDF_PersonalizationString,
                   NIST_CTR_DRBG_AES128_UsePR_UseDF_PersonalizationStringLen,
                   NIST_CTR_DRBG_AES128_UsePR_UseDF_AdditionalInput,
                   NIST_CTR_DRBG_AES128_UsePR_UseDF_AdditionalInputLen,
                   NIST_CTR_DRBG_AES128_UsePR_UseDF_ReturnedBytes,
                   NIST_CTR_DRBG_AES128_UsePR_UseDF_ReturnedBytesLen);
    SuccessOrFail(err, "TestDRBG failed in UsePR & UseDF case\n");
#endif

    // ============================================================
    // [AES-128 no df]
    // [PredictionResistance = False]
    // [EntropyInputLen = 256]
    // [NonceLen = 0]
    // [PersonalizationStringLen = 0]
    // [AdditionalInputLen = 256]
    // [ReturnedBitsLen = 512]
    // ============================================================
#if WEAVE_CONFIG_DRBG_WITHOUT_DERIVATION_FUNCTION    // no DF
    err = TestDRBG(NIST_CTR_DRBG_AES128_NoReseed_NoDF_Count,
                   NIST_CTR_DRBG_AES128_NoReseed_NoDF_PredictionResistance,
                   NIST_CTR_DRBG_AES128_NoReseed_NoDF_Reseed,
                   NIST_CTR_DRBG_AES128_NoReseed_NoDF_EntropyInput,
                   NIST_CTR_DRBG_AES128_NoReseed_NoDF_EntropyInputLen,
                   NIST_CTR_DRBG_AES128_NoReseed_NoDF_PersonalizationString,
                   NIST_CTR_DRBG_AES128_NoReseed_NoDF_PersonalizationStringLen,
                   NIST_CTR_DRBG_AES128_NoReseed_NoDF_AdditionalInput,
                   NIST_CTR_DRBG_AES128_NoReseed_NoDF_AdditionalInputLen,
                   NIST_CTR_DRBG_AES128_NoReseed_NoDF_ReturnedBytes,
                   NIST_CTR_DRBG_AES128_NoReseed_NoDF_ReturnedBytesLen);
    SuccessOrFail(err, "TestDRBG failed in NoReseed & NoDF case\n");
#endif

    printf("All tests succeeded\n");
}
