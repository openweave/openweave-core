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
 *      the Weave provisioning information verification hash
 *      generation interfaces.
 *
 */

#include <stdio.h>
#include <nltest.h>

#include <Weave/Profiles/security/WeaveProvHash.h>

using namespace nl::Weave::Profiles::Security;

static void MakeWeaveProvisioningHash_Test1(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err;
    char hashBuf[kWeaveProvisioningHashLength + 1];

    static uint64_t nodeId = 0x0123456789ABCDEFULL;
    static const char *weaveCert = "22222222222222222222222222222222222222222222";
    static const char *weavePrivKey = "44444444444444444444444444444444444444444444";
    static const char *pairingCode = "333333";

    static const char *expectedHash = "VWYmrGXhtCjLfveNxU9HN1RFDDBFkeKBDCUCbzoDJEs=";

    err = MakeWeaveProvisioningHash(nodeId,
                                    weaveCert, strlen(weaveCert),
                                    weavePrivKey, strlen(weavePrivKey),
                                    pairingCode, strlen(pairingCode),
                                    hashBuf, sizeof(hashBuf));

    // Verify MakeWeaveProvisioningHash() call succeeded.
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    // Verify that the hash returned by MakeWeaveProvisioningHash() has the correct length.
    NL_TEST_ASSERT(inSuite, strlen(hashBuf) == kWeaveProvisioningHashLength);

    // Verify that the hash returned by MakeWeaveProvisioningHash() has the correct value.
    NL_TEST_ASSERT(inSuite, strcmp(hashBuf, expectedHash) == 0);
}

void MakeDeviceCredentialHash_Test1(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err;
    char hashBuf[kDeviceCredentialHashLength + 1];

    static const char *sn = "02AA01AB2412001P";
    static const char *deviceId = "d.02AA01AB2412001P.TEST2";
    static const char *deviceSecret = "d.0TSIvbpCilGvgaTuNwunp_gJaWUGRPvKpPgSrripDhw";

    static const char *expectedHash = "RsMj0zDKIDjAqrQvlhCe4mp6KsMkSywliNtoAQzOOMA=";

    err = MakeDeviceCredentialHash(sn, strlen(sn),
                                   deviceId, strlen(deviceId),
                                   deviceSecret, strlen(deviceSecret),
                                   hashBuf, sizeof(hashBuf));

    // Verify MakeDeviceCredentialHash() call succeeded.
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    // Verify that the hash returned by MakeDeviceCredentialHash() has the correct length.
    NL_TEST_ASSERT(inSuite, strlen(hashBuf) == kDeviceCredentialHashLength);

    // Verify that the hash returned by MakeDeviceCredentialHash() has the correct value.
    NL_TEST_ASSERT(inSuite, strcmp(hashBuf, expectedHash) == 0);
}

int main(int argc, char *argv[])
{
    static const nlTest tests[] = {
        NL_TEST_DEF("MakeWeaveProvisioningHash",           MakeWeaveProvisioningHash_Test1),
        NL_TEST_DEF("MakeDeviceCredentialHash",            MakeDeviceCredentialHash_Test1),
        NL_TEST_SENTINEL()
    };

    static nlTestSuite testSuite = {
        "provisioning-hash",
        &tests[0]
    };

    nl_test_set_output_style(OUTPUT_CSV);

    nlTestRunner(&testSuite, NULL);

    return nlTestRunnerStats(&testSuite);
}
