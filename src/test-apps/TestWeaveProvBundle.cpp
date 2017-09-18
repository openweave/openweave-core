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
 *      the Weave device provisioning bundle decode and verification
 *      interfaces.
 *
 */

#include <stdio.h>
#include "ToolCommon.h"

#if WEAVE_CONFIG_ENABLE_PROVISIONING_BUNDLE_SUPPORT

#include <Weave/Profiles/security/WeaveProvBundle.h>

using namespace nl::Weave::Profiles::Security;

#define VerifyOrFail(TST, MSG) \
do { \
    if (!(TST)) \
    { \
        fprintf(stderr, "%s FAILED: ", __FUNCTION__); \
        fprintf(stderr, MSG); \
        exit(-1); \
    } \
} while (0)


static const char *TestProvBundle = "AQDDin0mj2eo2zGSJdJne+tri7q4TAJTzKgLAR9bZt9z1TrOMl8ueVgWb4UxI0Yvm7zKJVKyCRVdjHXIs2JpUkPG4boRyPXXYDEdTQcHawcCpOQ3WWufR8T4qeKWERiZGlkZR0ZwU3AJb+Ziz3aawBvVoHRSuZJIVcznQWANvB0CIYOaNmWuOjlg+/Ancx5+jXh4oGPATaxN0QtRAzJ1iwbJHvMNrO1jUejW4zBxwceijJoAK4mL1FkYXKJhjeohPAE/2bdjQsYKxFa2vyr69VSBxgTIVXPihwB3jPO4kTonbNFvhVK4cjZnrjNHdniMPqQfbBZynnQrVCCGqmC/lyOFqh3J5SuBeUkkxjfpHiWih2TSyBTwl9Kr18IZdt/hN65rUuD4dd0Utwe9hqvFbbFqoNctCE+MQDJE8cfOGy3eKWSBiZ3VAu4EOrbadM+TbIuCuS71SmlQyk51krjnwjYMpGXxqwy4eaudlPh/rIYfVlPYasgO2kZroFUnqHYGGWxAM9kl14DXDIpP1VYaVlOW2g3Z/5PzYVrcCLr12qr6uw==";
static const char *MasterKey = "d.h2aN-V0pbFXR6hn_odsjiHFAQKVnVU_eV8xXmWL_JKI";
static const uint64_t TestDeviceId = 0x18B430000002F659;
static const uint8_t TestCert[] =
{
    0xd5, 0x00, 0x00, 0x04, 0x00, 0x01, 0x00, 0x30, 0x01, 0x08, 0x20, 0x48,
    0x03, 0x06, 0x88, 0xd2, 0x62, 0x53, 0x24, 0x02, 0x04, 0x37, 0x03, 0x27,
    0x13, 0x02, 0x00, 0x00, 0xee, 0xee, 0x30, 0xb4, 0x18, 0x18, 0x26, 0x04,
    0xfa, 0x90, 0xc8, 0x1b, 0x26, 0x05, 0xfa, 0x3f, 0x11, 0x42, 0x37, 0x06,
    0x27, 0x11, 0x59, 0xf6, 0x02, 0x00, 0x00, 0x30, 0xb4, 0x18, 0x18, 0x24,
    0x07, 0x02, 0x26, 0x08, 0x15, 0x00, 0x5a, 0x23, 0x30, 0x0a, 0x31, 0x04,
    0xec, 0xab, 0x38, 0x43, 0x0b, 0xbb, 0x24, 0xeb, 0x23, 0x34, 0xde, 0xd3,
    0x67, 0x9e, 0x5e, 0x03, 0xde, 0xdd, 0xb4, 0xf2, 0x90, 0x14, 0xd1, 0xa4,
    0x07, 0x75, 0xb5, 0x29, 0x0a, 0xa9, 0x52, 0x4e, 0x10, 0xd0, 0x07, 0x4d,
    0x3b, 0x56, 0xad, 0x7a, 0x9c, 0x61, 0x07, 0xe4, 0x5e, 0xc2, 0x54, 0x53,
    0x35, 0x83, 0x29, 0x01, 0x18, 0x35, 0x82, 0x29, 0x01, 0x24, 0x02, 0x05,
    0x18, 0x35, 0x84, 0x29, 0x01, 0x36, 0x02, 0x04, 0x02, 0x04, 0x01, 0x18,
    0x18, 0x35, 0x81, 0x30, 0x02, 0x08, 0x46, 0x4d, 0x73, 0x0b, 0xb4, 0x06,
    0x5b, 0x6a, 0x18, 0x35, 0x80, 0x30, 0x02, 0x08, 0x44, 0xe3, 0x40, 0x38,
    0xa9, 0xd4, 0xb5, 0xa7, 0x18, 0x35, 0x0c, 0x30, 0x01, 0x19, 0x00, 0xd8,
    0x6f, 0x70, 0x1b, 0xad, 0x37, 0xd8, 0x9f, 0x3e, 0x69, 0x4b, 0x70, 0x40,
    0xf1, 0x2e, 0x64, 0x8f, 0x95, 0xba, 0xcb, 0x71, 0x73, 0x75, 0x75, 0x30,
    0x02, 0x19, 0x00, 0xab, 0x9f, 0x19, 0x53, 0x37, 0x1f, 0x87, 0xca, 0xef,
    0xca, 0xf5, 0x94, 0xb7, 0x41, 0x81, 0x50, 0xa5, 0xde, 0x26, 0xa1, 0x2e,
    0x82, 0x79, 0xf0, 0x18, 0x18
};

static const uint8_t TestPrivateKey[] =
{
    0xd5, 0x00, 0x00, 0x04, 0x00, 0x02, 0x00, 0x26, 0x01, 0x15, 0x00, 0x5a,
    0x23, 0x30, 0x02, 0x18, 0x5c, 0x23, 0xfa, 0xf1, 0x5d, 0xe1, 0x5a, 0xb9,
    0x03, 0xf0, 0xf1, 0x8f, 0xbc, 0xd1, 0xfb, 0xf7, 0xd8, 0xce, 0x16, 0x8a,
    0xd0, 0x48, 0x20, 0xf0, 0x30, 0x03, 0x31, 0x04, 0xec, 0xab, 0x38, 0x43,
    0x0b, 0xbb, 0x24, 0xeb, 0x23, 0x34, 0xde, 0xd3, 0x67, 0x9e, 0x5e, 0x03,
    0xde, 0xdd, 0xb4, 0xf2, 0x90, 0x14, 0xd1, 0xa4, 0x07, 0x75, 0xb5, 0x29,
    0x0a, 0xa9, 0x52, 0x4e, 0x10, 0xd0, 0x07, 0x4d, 0x3b, 0x56, 0xad, 0x7a,
    0x9c, 0x61, 0x07, 0xe4, 0x5e, 0xc2, 0x54, 0x53, 0x18
};

static const char *ExpectedPairingCode = "YKGRFR";



void WeaveProvBundle_DecodeTest()
{
    WEAVE_ERROR err;
    WeaveProvisioningBundle provBundle;

    uint8_t *provBundleCopy = (uint8_t *)strdup(TestProvBundle);

    err = WeaveProvisioningBundle::Decode(provBundleCopy, (uint32_t)strlen(TestProvBundle), MasterKey, (uint32_t)strlen(MasterKey), provBundle);
    VerifyOrFail(err == WEAVE_NO_ERROR, "WeaveProvisioningBundle::Decode returned error\n");

    VerifyOrFail(provBundle.WeaveDeviceId == TestDeviceId, "WeaveProvisioningBundle::Decode returned invalid device id\n");
    VerifyOrFail(provBundle.CertificateLen == sizeof(TestCert), "WeaveProvisioningBundle::Decode returned invalid certificate length\n");
    VerifyOrFail(memcmp(provBundle.Certificate, TestCert, sizeof(TestCert)) == 0, "WeaveProvisioningBundle::Decode returned invalid certificate\n");
    VerifyOrFail(provBundle.PrivateKeyLen == sizeof(TestPrivateKey), "WeaveProvisioningBundle::Decode returned invalid private key length\n");
    VerifyOrFail(memcmp(provBundle.PrivateKey, TestPrivateKey, sizeof(TestPrivateKey)) == 0, "WeaveProvisioningBundle::Decode returned invalid private key\n");
    VerifyOrFail(provBundle.PairingCodeLen == strlen(ExpectedPairingCode), "WeaveProvisioningBundle::Decode returned invalid pairing code length\n");
    VerifyOrFail(memcmp(provBundle.PairingCode, ExpectedPairingCode, strlen(ExpectedPairingCode)) == 0, "WeaveProvisioningBundle::Decode returned invalid pairing code\n");

    printf("DecodeTest succeeded\n");
}

void WeaveProvBundle_VerifyTest()
{
    WEAVE_ERROR err;
    WeaveProvisioningBundle provBundle;

    uint8_t *provBundleCopy = (uint8_t *)strdup(TestProvBundle);

    err = WeaveProvisioningBundle::Decode(provBundleCopy, (uint32_t)strlen(TestProvBundle), MasterKey, (uint32_t)strlen(MasterKey), provBundle);
    VerifyOrFail(err == WEAVE_NO_ERROR, "WeaveProvisioningBundle::Decode returned error\n");

    err = provBundle.Verify(TestDeviceId);
    VerifyOrFail(err == WEAVE_NO_ERROR, "WeaveProvisioningBundle::Verify returned error\n");

    printf("VerifyTest succeeded\n");
}

int main(int argc, char *argv[])
{
    WEAVE_ERROR err;

    err = nl::Weave::Platform::Security::InitSecureRandomDataSource(NULL, 64, NULL, 0);
    FAIL_ERROR(err, "InitSecureRandomDataSource() failed");

    WeaveProvBundle_DecodeTest();
    WeaveProvBundle_VerifyTest();
    printf("All tests succeeded\n");
    return 0;
}

#else // WEAVE_CONFIG_ENABLE_PROVISIONING_BUNDLE_SUPPORT

int main(int argc, char *argv[])
{
    printf("Weave provisioning bundle support disabled\n");
    return -1;
}

#endif // WEAVE_CONFIG_ENABLE_PROVISIONING_BUNDLE_SUPPORT
