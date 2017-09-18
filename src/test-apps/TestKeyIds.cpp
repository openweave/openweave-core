/*
 *
 *    Copyright (c) 2016-2017 Nest Labs, Inc.
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
 *      This file implements unit tests for the Weave application keys library.
 *
 */

#include <stdio.h>
#include <nltest.h>
#include <string.h>

#include "ToolCommon.h"
#include "TestGroupKeyStore.h"
#include <Weave/Support/ErrorStr.h>
#include <Weave/Core/WeaveKeyIds.h>

#define DEBUG_PRINT_ENABLE 0

void KeyIds_Test1(nlTestSuite *inSuite, void *inContext)
{
    // Testing WeaveKeyId::GetType().
    NL_TEST_ASSERT(inSuite, WeaveKeyId::GetType(WeaveKeyId::kNone) == WeaveKeyId::kType_None);
    NL_TEST_ASSERT(inSuite, WeaveKeyId::GetType(WeaveKeyId::kFabricSecret) == WeaveKeyId::kType_General);
    NL_TEST_ASSERT(inSuite, WeaveKeyId::GetType(sTestDefaultTCPSessionKeyId) == WeaveKeyId::kType_Session);
    NL_TEST_ASSERT(inSuite, WeaveKeyId::GetType(sAppStaticKeyId_CRK_G10) == WeaveKeyId::kType_AppStaticKey);
    NL_TEST_ASSERT(inSuite, WeaveKeyId::GetType(sAppRotatingKeyId_SRK_E3_G54) == WeaveKeyId::kType_AppRotatingKey);
    NL_TEST_ASSERT(inSuite, WeaveKeyId::GetType(WeaveKeyId::kServiceRootKey) == WeaveKeyId::kType_AppRootKey);
    NL_TEST_ASSERT(inSuite, WeaveKeyId::GetType(sEpochKey0_KeyId) == WeaveKeyId::kType_AppEpochKey);
    NL_TEST_ASSERT(inSuite, WeaveKeyId::GetType(sAppGroupMasterKey0_KeyId) == WeaveKeyId::kType_AppGroupMasterKey);
    NL_TEST_ASSERT(inSuite, WeaveKeyId::GetType(sIntermediateKeyId_FRK_E2) == WeaveKeyId::kType_AppIntermediateKey);

    // Testing key type checking functions.
    NL_TEST_ASSERT(inSuite, WeaveKeyId::IsGeneralKey(WeaveKeyId::kFabricSecret));
    NL_TEST_ASSERT(inSuite, WeaveKeyId::IsSessionKey(sTestDefaultSessionKeyId));
    NL_TEST_ASSERT(inSuite, WeaveKeyId::IsAppStaticKey(sPasscodeEncStaticKeyId_CRK_G4));
    NL_TEST_ASSERT(inSuite, WeaveKeyId::IsAppRotatingKey(sPasscodeEncRotatingKeyId_CRK_E0_G4));
    NL_TEST_ASSERT(inSuite, WeaveKeyId::IsAppGroupKey(sPasscodeEncRotatingKeyId_CRK_E0_G4));
    NL_TEST_ASSERT(inSuite, WeaveKeyId::IsAppRootKey(sInvalidRootKeyId));
    NL_TEST_ASSERT(inSuite, WeaveKeyId::IsAppEpochKey(sEpochKey3_KeyId));
    NL_TEST_ASSERT(inSuite, WeaveKeyId::IsAppGroupMasterKey(sAppGroupMasterKey54_KeyId));

    NL_TEST_ASSERT(inSuite, !WeaveKeyId::IsGeneralKey(sTestDefaultSessionKeyId));
    NL_TEST_ASSERT(inSuite, !WeaveKeyId::IsSessionKey(sAppGroupMasterKey54_KeyId));
    NL_TEST_ASSERT(inSuite, !WeaveKeyId::IsAppStaticKey(sPasscodeEncRotatingKeyId_CRK_E0_G4));
    NL_TEST_ASSERT(inSuite, !WeaveKeyId::IsAppRotatingKey(sPasscodeEncStaticKeyId_CRK_G4));
    NL_TEST_ASSERT(inSuite, !WeaveKeyId::IsAppGroupKey(WeaveKeyId::kFabricSecret));
    NL_TEST_ASSERT(inSuite, !WeaveKeyId::IsAppRootKey(WeaveKeyId::kFabricSecret));
    NL_TEST_ASSERT(inSuite, !WeaveKeyId::IsAppEpochKey(sInvalidRootKeyId));
    NL_TEST_ASSERT(inSuite, !WeaveKeyId::IsAppGroupMasterKey(sEpochKey3_KeyId));

    // Testing get constituent key functions.
    NL_TEST_ASSERT(inSuite, WeaveKeyId::GetRootKeyId(sPasscodeEncRotatingKeyId_CRK_E0_G4) == WeaveKeyId::kClientRootKey);
    NL_TEST_ASSERT(inSuite, WeaveKeyId::GetEpochKeyId(sPasscodeEncRotatingKeyId_CRK_E0_G4) == sEpochKey0_KeyId);
    NL_TEST_ASSERT(inSuite, WeaveKeyId::GetAppGroupMasterKeyId(sPasscodeEncStaticKeyId_CRK_G4) == sAppGroupMasterKey4_KeyId);
    NL_TEST_ASSERT(inSuite, WeaveKeyId::GetRootKeyNumber(sInvalidRootKeyId) == sInvalidRootKeyNumber);
    NL_TEST_ASSERT(inSuite, WeaveKeyId::GetEpochKeyNumber(sAppRotatingKeyId_SRK_E3_G54) == sEpochKey3_Number);
    NL_TEST_ASSERT(inSuite, WeaveKeyId::GetAppGroupLocalNumber(sPasscodeEncStaticKeyId_CRK_G4) == sAppGroupMasterKey4_Number);

    // Testing make key functions.
    uint16_t shortKeyNumber = 0x02F6;
    uint16_t longKeyNumber = 0x8000 | shortKeyNumber;
    NL_TEST_ASSERT(inSuite, WeaveKeyId::MakeSessionKeyId(shortKeyNumber) == (WeaveKeyId::kType_Session | shortKeyNumber));
    NL_TEST_ASSERT(inSuite, WeaveKeyId::MakeGeneralKeyId(shortKeyNumber) == (WeaveKeyId::kType_General | shortKeyNumber));
    NL_TEST_ASSERT(inSuite, WeaveKeyId::MakeSessionKeyId(longKeyNumber) == (WeaveKeyId::kType_Session | shortKeyNumber));
    NL_TEST_ASSERT(inSuite, WeaveKeyId::MakeGeneralKeyId(longKeyNumber) == (WeaveKeyId::kType_General | shortKeyNumber));
    shortKeyNumber = 0x03;
    NL_TEST_ASSERT(inSuite, WeaveKeyId::MakeRootKeyId(shortKeyNumber) == (static_cast<uint32_t>(WeaveKeyId::kType_AppRootKey | (shortKeyNumber << 10))));
    NL_TEST_ASSERT(inSuite, WeaveKeyId::MakeEpochKeyId(shortKeyNumber) == (static_cast<uint32_t>(WeaveKeyId::kType_AppEpochKey | (shortKeyNumber << 7))));
    NL_TEST_ASSERT(inSuite, WeaveKeyId::MakeAppGroupMasterKeyId(shortKeyNumber) == (static_cast<uint32_t>(WeaveKeyId::kType_AppGroupMasterKey | (shortKeyNumber << 0))));

    // Testing property checking functions.
    NL_TEST_ASSERT(inSuite, WeaveKeyId::IncorporatesEpochKey(sPasscodeEncRotatingKeyId_CRK_E0_G4));
    NL_TEST_ASSERT(inSuite, WeaveKeyId::UsesCurrentEpochKey(sIntermediateKeyId_FRK_EC));
    NL_TEST_ASSERT(inSuite, WeaveKeyId::IncorporatesRootKey(sAppRotatingKeyId_SRK_E3_G54));
    NL_TEST_ASSERT(inSuite, WeaveKeyId::IncorporatesAppGroupMasterKey(sAppStaticKeyId_CRK_G10));

    NL_TEST_ASSERT(inSuite, !WeaveKeyId::IncorporatesEpochKey(sPasscodeEncStaticKeyId_CRK_G4));
    NL_TEST_ASSERT(inSuite, !WeaveKeyId::UsesCurrentEpochKey(sIntermediateKeyId_FRK_E2));
    NL_TEST_ASSERT(inSuite, !WeaveKeyId::IncorporatesRootKey(sTestDefaultSessionKeyId));
    NL_TEST_ASSERT(inSuite, !WeaveKeyId::IncorporatesAppGroupMasterKey(sIntermediateKeyId_FRK_E2));

    // Testing make key functions.
    uint32_t keyId;
    uint32_t keyId2;
    keyId = WeaveKeyId::MakeAppKeyId(WeaveKeyId::kType_AppRootKey, WeaveKeyId::kClientRootKey, WeaveKeyId::kNone, WeaveKeyId::kNone, false);
    NL_TEST_ASSERT(inSuite, keyId == WeaveKeyId::kClientRootKey);
    keyId = WeaveKeyId::MakeAppKeyId(WeaveKeyId::kType_AppEpochKey, WeaveKeyId::kNone, sEpochKey3_KeyId, WeaveKeyId::kNone, true);
    NL_TEST_ASSERT(inSuite, keyId == WeaveKeyId::ConvertToCurrentAppKeyId(sEpochKey3_KeyId));
    keyId = WeaveKeyId::MakeAppIntermediateKeyId(WeaveKeyId::kClientRootKey, sEpochKey3_KeyId, false);
    keyId2 = WeaveKeyId::kType_AppIntermediateKey | ((WeaveKeyId::kClientRootKey | sEpochKey3_KeyId) & 0xFFF);
    NL_TEST_ASSERT(inSuite, keyId == keyId2);
    keyId = WeaveKeyId::MakeAppRotatingKeyId(WeaveKeyId::kFabricRootKey, sEpochKey5_KeyId, sAppGroupMasterKey54_KeyId, false);
    keyId2 = WeaveKeyId::kType_AppRotatingKey | ((WeaveKeyId::kFabricRootKey | sEpochKey5_KeyId | sAppGroupMasterKey54_KeyId) & 0xFFF);
    NL_TEST_ASSERT(inSuite, keyId == keyId2);
    keyId = WeaveKeyId::MakeAppStaticKeyId(WeaveKeyId::kServiceRootKey, sAppGroupMasterKey4_KeyId);
    keyId2 = WeaveKeyId::kType_AppStaticKey | ((WeaveKeyId::kServiceRootKey | sAppGroupMasterKey4_KeyId) & 0xFFF);
    NL_TEST_ASSERT(inSuite, keyId == keyId2);

    NL_TEST_ASSERT(inSuite, WeaveKeyId::ConvertToCurrentAppKeyId(sIntermediateKeyId_FRK_E2) == sIntermediateKeyId_FRK_EC);
    NL_TEST_ASSERT(inSuite, WeaveKeyId::ConvertToStaticAppKeyId(sPasscodeEncRotatingKeyId_CRK_E0_G4) == sPasscodeEncStaticKeyId_CRK_G4);
    NL_TEST_ASSERT(inSuite, WeaveKeyId::UpdateEpochKeyId(sIntermediateKeyId_FRK_EC, sEpochKey2_KeyId) == sIntermediateKeyId_FRK_E2);

    // Testing WeaveKeyId::IsValidKeyId() function.
    NL_TEST_ASSERT(inSuite, !WeaveKeyId::IsValidKeyId(WeaveKeyId::kNone));
    NL_TEST_ASSERT(inSuite, WeaveKeyId::IsValidKeyId(WeaveKeyId::kFabricSecret));
    NL_TEST_ASSERT(inSuite, WeaveKeyId::IsValidKeyId(sTestDefaultSessionKeyId));
    NL_TEST_ASSERT(inSuite, WeaveKeyId::IsValidKeyId(sPasscodeEncStaticKeyId_CRK_G4));
    NL_TEST_ASSERT(inSuite, WeaveKeyId::IsValidKeyId(sPasscodeEncRotatingKeyId_CRK_E0_G4));
    NL_TEST_ASSERT(inSuite, WeaveKeyId::IsValidKeyId(WeaveKeyId::kClientRootKey));
    NL_TEST_ASSERT(inSuite, WeaveKeyId::IsValidKeyId(sIntermediateKeyId_FRK_E2));
    NL_TEST_ASSERT(inSuite, WeaveKeyId::IsValidKeyId(sIntermediateKeyId_FRK_EC));
    NL_TEST_ASSERT(inSuite, WeaveKeyId::IsValidKeyId(sEpochKey4_KeyId));
    NL_TEST_ASSERT(inSuite, WeaveKeyId::IsValidKeyId(WeaveKeyId::ConvertToCurrentAppKeyId(sEpochKey4_KeyId)));
    NL_TEST_ASSERT(inSuite, WeaveKeyId::IsValidKeyId(sAppGroupMasterKey54_KeyId));

    NL_TEST_ASSERT(inSuite, !WeaveKeyId::IsValidKeyId(sInvalidRootKeyId));
    NL_TEST_ASSERT(inSuite, !WeaveKeyId::IsValidKeyId(WeaveKeyId::MakeEpochKeyId(0x08)));
    NL_TEST_ASSERT(inSuite, !WeaveKeyId::IsValidKeyId(WeaveKeyId::MakeAppGroupMasterKeyId(0x80)));
    NL_TEST_ASSERT(inSuite, !WeaveKeyId::IsValidKeyId(sPasscodeEncStaticKeyId_CRK_G4 | 0x40000000));
    NL_TEST_ASSERT(inSuite, !WeaveKeyId::IsValidKeyId(WeaveKeyId::kType_AppGroupMasterKey | WeaveKeyId::kType_AppRotatingKey));

    // Testing WeaveKeyId::DescribeKey() function.
    const char *strKeyId1;
    const char *strKeyId2;
    strKeyId1 = WeaveKeyId::DescribeKey(WeaveKeyId::kNone);
    strKeyId2 = WeaveKeyId::DescribeKey(0xFFF);
    NL_TEST_ASSERT(inSuite, strcmp(strKeyId1, strKeyId2) == 0);
    strKeyId1 = WeaveKeyId::DescribeKey(WeaveKeyId::kFabricSecret);
    strKeyId2 = WeaveKeyId::DescribeKey(WeaveKeyId::kFabricRootKey);
    NL_TEST_ASSERT(inSuite, strcmp(strKeyId1, strKeyId2) != 0);
    strKeyId1 = WeaveKeyId::DescribeKey(sIntermediateKeyId_FRK_E2);
    NL_TEST_ASSERT(inSuite, strcmp(strKeyId1, "Application Intermediate Key") == 0);
    strKeyId1 = WeaveKeyId::DescribeKey(sInvalidRootKeyId);
    NL_TEST_ASSERT(inSuite, strcmp(strKeyId1, "Other Root Key") == 0);
    strKeyId1 = WeaveKeyId::DescribeKey(sAppGroupMasterKey54_KeyId);
    NL_TEST_ASSERT(inSuite, strcmp(strKeyId1, "Application Group Master Key") == 0);
}


int main(int argc, char *argv[])
{
    static const nlTest tests[] = {
        NL_TEST_DEF("KeyIds_Test1",                             KeyIds_Test1),
        NL_TEST_SENTINEL()
    };

    static nlTestSuite testSuite = {
        "key-identifiers",
        &tests[0]
    };

    nl_test_set_output_style(OUTPUT_CSV);

    nlTestRunner(&testSuite, NULL);

    return nlTestRunnerStats(&testSuite);
}
