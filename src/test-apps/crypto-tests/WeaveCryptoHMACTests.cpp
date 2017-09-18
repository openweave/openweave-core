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
 *      This file implements interface to Weave Crypto HKDF Tests library.
 *
 */

#include <string.h>

#include <nltest.h>

#include <Weave/Support/crypto/HKDF.h>

#include "WeaveCryptoTests.h"

using namespace nl::Weave::Crypto;
using namespace nl::Weave::Platform::Security;

static void Check_HMACSHA1_Test1(nlTestSuite *inSuite, void *inContext)
{
    HMACSHA1 hmac;
    uint8_t digest[HMACSHA1::kDigestLength];

    static uint8_t Key[] = { 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b };
    static uint8_t Data[] = "Hi There";
    static uint8_t ExpectedDigest[] = { 0xb6, 0x17, 0x31, 0x86, 0x55, 0x05, 0x72, 0x64, 0xe2, 0x8b, 0xc0, 0xb6, 0xfb, 0x37, 0x8c, 0x8e, 0xf1, 0x46, 0xbe, 0x00 };

    hmac.Begin(Key, sizeof(Key));
    hmac.AddData(Data, sizeof(Data) - 1);
    hmac.Finish(digest);

    // Invalid digest returned by HMACSHA1::Finish()
    NL_TEST_ASSERT(inSuite, memcmp(digest, ExpectedDigest, HMACSHA1::kDigestLength) == 0);
}

static void Check_HMACSHA1_Test2(nlTestSuite *inSuite, void *inContext)
{
    HMACSHA1 hmac;
    uint8_t digest[HMACSHA1::kDigestLength];

    static uint8_t Key[] = { 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa };
    static uint8_t Data[] = {
        0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd,
        0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd,
        0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd,
        0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd,
        0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd
    };
    static uint8_t ExpectedDigest[] = { 0x12, 0x5d, 0x73, 0x42, 0xb9, 0xac, 0x11, 0xcd, 0x91, 0xa3, 0x9a, 0xf4, 0x8a, 0xa1, 0x7b, 0x4f, 0x63, 0xf1, 0x75, 0xd3 };

    hmac.Begin(Key, sizeof(Key));
    hmac.AddData(Data, sizeof(Data));
    hmac.Finish(digest);

    // Invalid digest returned by HMACSHA1::Finish()
    NL_TEST_ASSERT(inSuite, memcmp(digest, ExpectedDigest, HMACSHA1::kDigestLength) == 0);
}

static const nlTest sTests[] = {
    NL_TEST_DEF("HMACSHA1 Test1",          Check_HMACSHA1_Test1),
    NL_TEST_DEF("HMACSHA1 Test2",          Check_HMACSHA1_Test2),
    NL_TEST_SENTINEL()
};

int WeaveCryptoHMACTests(void)
{
    nlTestSuite theSuite = {
        "Weave Crypto HMACSHA1 Tests",
        &sTests[0]
    };

    nl_test_set_output_style(OUTPUT_CSV);

    nlTestRunner(&theSuite, NULL);

    return nlTestRunnerStats(&theSuite);
}
