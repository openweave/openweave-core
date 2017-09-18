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
 *      Definition of TestGroupKeyStore class, which provides an implementation
 *      of the GroupKeyStoreBase interface for use in test applications.
 *
 */

#ifndef TESTPLATFORMGROUPKEYSTORE_H_
#define TESTPLATFORMGROUPKEYSTORE_H_

#include "ToolCommon.h"
#include <Weave/Profiles/security/WeaveApplicationKeys.h>

extern uint32_t sLastUsedEpochKeyId;
extern uint32_t sCurrentUTCTime;

extern const uint8_t sFabricSecret[];
extern const uint8_t sFabricSecretLen;

extern const uint8_t sFabricRootKey[];
extern const uint8_t sFabricRootKeyLen;

extern const uint8_t sClientRootKey[];
extern const uint8_t sClientRootKeyLen;

extern const uint8_t sServiceRootKey[];
extern const uint8_t sServiceRootKeyLen;

extern const uint32_t sInvalidRootKeyNumber;
extern const uint32_t sInvalidRootKeyId;

extern const uint32_t sEpochKey0_Number;
extern const uint32_t sEpochKey0_KeyId;
extern const uint32_t sEpochKey0_StartTime;
extern const uint8_t sEpochKey0_Key[];
extern const uint8_t sEpochKey0_KeyLen;

extern const uint32_t sEpochKey1_Number;
extern const uint32_t sEpochKey1_KeyId;
extern const uint32_t sEpochKey1_StartTime;
extern const uint8_t sEpochKey1_Key[];
extern const uint8_t sEpochKey1_KeyLen;

extern const uint32_t sEpochKey2_Number;
extern const uint32_t sEpochKey2_KeyId;
extern const uint32_t sEpochKey2_StartTime;
extern const uint8_t sEpochKey2_Key[];
extern const uint8_t sEpochKey2_KeyLen;

extern const uint32_t sEpochKey3_Number;
extern const uint32_t sEpochKey3_KeyId;
extern const uint32_t sEpochKey3_StartTime;
extern const uint8_t sEpochKey3_Key[];
extern const uint8_t sEpochKey3_KeyLen;

extern const uint32_t sEpochKey4_Number;
extern const uint32_t sEpochKey4_KeyId;
extern const uint32_t sEpochKey4_StartTime;
extern const uint8_t sEpochKey4_Key[];
extern const uint8_t sEpochKey4_KeyLen;

extern const uint32_t sEpochKey5_Number;
extern const uint32_t sEpochKey5_KeyId;
extern const uint32_t sEpochKey5_StartTime;
extern const uint8_t sEpochKey5_Key[];
extern const uint8_t sEpochKey5_KeyLen;

extern const uint32_t sAppGroupMasterKey0_Number;
extern const uint32_t sAppGroupMasterKey0_KeyId;
extern const uint32_t sAppGroupMasterKey0_GlobalId;
extern const uint8_t sAppGroupMasterKey0_Key[];
extern const uint8_t sAppGroupMasterKey0_KeyLen;

extern const uint32_t sAppGroupMasterKey4_Number;
extern const uint32_t sAppGroupMasterKey4_KeyId;
extern const uint32_t sAppGroupMasterKey4_GlobalId;
extern const uint8_t sAppGroupMasterKey4_Key[];
extern const uint8_t sAppGroupMasterKey4_KeyLen;

extern const uint32_t sAppGroupMasterKey10_Number;
extern const uint32_t sAppGroupMasterKey10_KeyId;
extern const uint32_t sAppGroupMasterKey10_GlobalId;
extern const uint8_t sAppGroupMasterKey10_Key[];
extern const uint8_t sAppGroupMasterKey10_KeyLen;

extern const uint32_t sAppGroupMasterKey54_Number;
extern const uint32_t sAppGroupMasterKey54_KeyId;
extern const uint32_t sAppGroupMasterKey54_GlobalId;
extern const uint8_t sAppGroupMasterKey54_Key[];
extern const uint8_t sAppGroupMasterKey54_KeyLen;

extern const uint32_t sAppGroupMasterKey7_Number;
extern const uint32_t sAppGroupMasterKey7_KeyId;
extern const uint32_t sAppGroupMasterKey7_GlobalId;
extern const uint8_t sAppGroupMasterKey7_Key[];
extern const uint8_t sAppGroupMasterKey7_KeyLen;

extern const uint32_t sIntermediateKeyId_FRK_E2;
extern const uint32_t sIntermediateKeyId_FRK_EC;
extern const uint8_t sIntermediateKey_FRK_E2[];
extern const uint8_t sIntermediateKeyLen_FRK_E2;

extern const uint32_t sAppStaticKeyId_CRK_G10;
extern const uint8_t sAppStaticKeyDiversifier_CRK_G10[];
extern const uint8_t sAppStaticKeyDiversifierLen_CRK_G10;
extern const uint8_t sAppStaticKey_CRK_G10[];
extern const uint8_t sAppStaticKeyLen_CRK_G10;

extern const uint32_t sAppRotatingKeyId_SRK_E3_G54;
extern const uint8_t sAppRotatingKeyDiversifier_SRK_E3_G54[];
extern const uint8_t sAppRotatingKeyDiversifierLen_SRK_E3_G54;
extern const uint8_t sAppRotatingKey_SRK_E3_G54[];
extern const uint8_t sAppRotatingKeyLen_SRK_E3_G54;


extern const uint32_t sPasscodeEncryptionKeyNonce;
extern const uint8_t sPasscodeEncryptionKeyNonceLen;
extern const uint8_t sPasscodeEncryptionKeyDiversifier[];
extern const uint8_t sPasscodeEncryptionKeyDiversifierLen;

extern const uint32_t sPasscodeEncRotatingKeyId_CRK_E0_G4;
extern const uint8_t sPasscodeEncRotatingKey_CRK_E0_G4[];
extern const uint8_t sPasscodeEncRotatingKeyLen_CRK_E0_G4;

extern const uint32_t sPasscodeEncStaticKeyId_CRK_G4;
extern const uint8_t sPasscodeEncStaticKey_CRK_G4[];
extern const uint8_t sPasscodeEncStaticKeyLen_CRK_G4;

extern const uint32_t sPasscodeFingerprintKeyId_CRK_G4;
extern const uint8_t sPasscodeFingerprintKey_CRK_G4[];
extern const uint8_t sPasscodeFingerprintKeyLen_CRK_G4;

class TestGroupKeyStore : public nl::Weave::Profiles::Security::AppKeys::GroupKeyStoreBase
{
public:
    TestGroupKeyStore(void);

    // Manage application group key material storage.
    virtual WEAVE_ERROR RetrieveGroupKey(uint32_t keyId, nl::Weave::Profiles::Security::AppKeys::WeaveGroupKey & key);
    virtual WEAVE_ERROR StoreGroupKey(const nl::Weave::Profiles::Security::AppKeys::WeaveGroupKey & key);
    virtual WEAVE_ERROR DeleteGroupKey(uint32_t keyId);
    virtual WEAVE_ERROR DeleteGroupKeysOfAType(uint32_t keyType);
    virtual WEAVE_ERROR EnumerateGroupKeys(uint32_t keyType, uint32_t *keyIds, uint8_t keyIdsArraySize, uint8_t & keyCount);
    virtual WEAVE_ERROR Clear(void);

    // Retrieve and Store LastUsedEpochKeyId value.
    virtual WEAVE_ERROR RetrieveLastUsedEpochKeyId(void);
    virtual WEAVE_ERROR StoreLastUsedEpochKeyId(void);

    // Get current platform UTC time in seconds.
    virtual WEAVE_ERROR GetCurrentUTCTime(uint32_t& utcTime);

    // Functions added for test purposes. Used to check values of the member variables.
    uint32_t TestValue_LastUsedEpochKeyId(void);
    uint32_t TestValue_NextEpochKeyStartTime(void);

private:
    enum
    {
        MaxGroupKeyCount                                = 1 + 1 + WEAVE_CONFIG_MAX_APPLICATION_EPOCH_KEYS + WEAVE_CONFIG_MAX_APPLICATION_GROUPS,

#if WEAVE_CONFIG_MAX_APPLICATION_EPOCH_KEYS > WEAVE_CONFIG_MAX_APPLICATION_GROUPS
        MaxGroupKeysOfATypeCount                        = WEAVE_CONFIG_MAX_APPLICATION_EPOCH_KEYS,
#elif WEAVE_CONFIG_MAX_APPLICATION_GROUPS > 1
        MaxGroupKeysOfATypeCount                        = WEAVE_CONFIG_MAX_APPLICATION_GROUPS,
#else
        // At lease one fabric secret key should be supported on any Weave platform.
        MaxGroupKeysOfATypeCount                        = 1,
#endif
    };

    nl::Weave::Profiles::Security::AppKeys::WeaveGroupKey Keys[MaxGroupKeyCount];
};

#endif /* TESTPLATFORMGROUPKEYSTORE_H_ */
