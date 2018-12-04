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

#ifndef WEAVE_DEVICE_GROUP_KEYSTORE_UNIT_TEST_H
#define WEAVE_DEVICE_GROUP_KEYSTORE_UNIT_TEST_H

#include <Weave/Core/WeaveCore.h>

namespace nl {
namespace Weave {
namespace DeviceLayer {
namespace Internal {

template<class GroupKeyStoreClass>
void RunGroupKeyStoreUnitTest(GroupKeyStoreClass * groupKeyStore)
{
    WEAVE_ERROR err;
    Profiles::Security::AppKeys::WeaveGroupKey keyIn, keyOut;

    // ===== Test 1: Store and retrieve root key

    // Store service root key
    keyIn.KeyId = WeaveKeyId::kServiceRootKey;
    keyIn.KeyLen = Profiles::Security::AppKeys::kWeaveAppRootKeySize;
    memset(keyIn.Key, 0x34, keyIn.KeyLen);
    err = groupKeyStore->StoreGroupKey(keyIn);
    VerifyOrDie(err == WEAVE_NO_ERROR);

    // Retrieve and validate service root key
    memset(&keyOut, 0, sizeof(keyOut));
    err = groupKeyStore->RetrieveGroupKey(WeaveKeyId::kServiceRootKey, keyOut);
    VerifyOrDie(err == WEAVE_NO_ERROR);
    VerifyOrDie(keyOut.KeyId == WeaveKeyId::kServiceRootKey);
    VerifyOrDie(keyOut.KeyLen == Profiles::Security::AppKeys::kWeaveAppRootKeySize);
    VerifyOrDie(memcmp(keyOut.Key, keyIn.Key, keyOut.KeyLen) == 0);

    // ===== Test 2: Store and retrieve fabric secret

    // Store fabric secret
    keyIn.KeyId = WeaveKeyId::kFabricSecret;
    keyIn.KeyLen = Profiles::Security::AppKeys::kWeaveFabricSecretSize;
    memset(keyIn.Key, 0xAB, keyIn.KeyLen);
    err = groupKeyStore->StoreGroupKey(keyIn);
    VerifyOrDie(err == WEAVE_NO_ERROR);

    // Retrieve and validate fabric secret
    memset(&keyOut, 0, sizeof(keyOut));
    err = groupKeyStore->RetrieveGroupKey(WeaveKeyId::kFabricSecret, keyOut);
    VerifyOrDie(err == WEAVE_NO_ERROR);
    VerifyOrDie(keyOut.KeyId == WeaveKeyId::kFabricSecret);
    VerifyOrDie(keyOut.KeyLen == Profiles::Security::AppKeys::kWeaveFabricSecretSize);
    VerifyOrDie(memcmp(keyOut.Key, keyIn.Key, keyOut.KeyLen) == 0);

    // ===== Test 3: Store and retrieve application master key

    // Store application master key
    keyIn.KeyId = WeaveKeyId::MakeAppGroupMasterKeyId(0x42);
    keyIn.KeyLen = Profiles::Security::AppKeys::kWeaveAppGroupMasterKeySize;
    memset(keyIn.Key, 0x42, keyIn.KeyLen);
    keyIn.GlobalId = 0x42424242;
    err = groupKeyStore->StoreGroupKey(keyIn);
    VerifyOrDie(err == WEAVE_NO_ERROR);

    // Retrieve and validate application master key
    memset(&keyOut, 0, sizeof(keyOut));
    err = groupKeyStore->RetrieveGroupKey(keyIn.KeyId, keyOut);
    VerifyOrDie(err == WEAVE_NO_ERROR);
    VerifyOrDie(keyOut.KeyId == keyIn.KeyId);
    VerifyOrDie(keyOut.KeyLen == Profiles::Security::AppKeys::kWeaveAppGroupMasterKeySize);
    VerifyOrDie(memcmp(keyOut.Key, keyIn.Key, keyOut.KeyLen) == 0);

    // ===== Test 4: Store and retrieve epoch keys

    // Store first epoch key
    keyIn.KeyId = WeaveKeyId::MakeEpochKeyId(2);
    keyIn.KeyLen = Profiles::Security::AppKeys::kWeaveAppEpochKeySize;
    memset(keyIn.Key, 0x73, keyIn.KeyLen);
    keyIn.StartTime = 0x74;
    err = groupKeyStore->StoreGroupKey(keyIn);
    VerifyOrDie(err == WEAVE_NO_ERROR);

    // Store second epoch key
    keyIn.KeyId = WeaveKeyId::MakeEpochKeyId(6);
    keyIn.KeyLen = Profiles::Security::AppKeys::kWeaveAppEpochKeySize;
    memset(keyIn.Key, 0x75, keyIn.KeyLen);
    keyIn.StartTime = 0x76;
    err = groupKeyStore->StoreGroupKey(keyIn);
    VerifyOrDie(err == WEAVE_NO_ERROR);

    // Retrieve and validate first epoch key
    memset(&keyOut, 0, sizeof(keyOut));
    err = groupKeyStore->RetrieveGroupKey(WeaveKeyId::MakeEpochKeyId(2), keyOut);
    VerifyOrDie(err == WEAVE_NO_ERROR);
    VerifyOrDie(keyOut.KeyId == WeaveKeyId::MakeEpochKeyId(2));
    VerifyOrDie(keyOut.KeyLen == Profiles::Security::AppKeys::kWeaveAppEpochKeySize);
    memset(keyIn.Key, 0x73, Profiles::Security::AppKeys::kWeaveAppEpochKeySize);
    VerifyOrDie(memcmp(keyOut.Key, keyIn.Key, Profiles::Security::AppKeys::kWeaveAppEpochKeySize) == 0);
    VerifyOrDie(keyOut.StartTime == 0x74);

    // Retrieve and validate second epoch key
    memset(&keyOut, 0, sizeof(keyOut));
    err = groupKeyStore->RetrieveGroupKey(WeaveKeyId::MakeEpochKeyId(6), keyOut);
    VerifyOrDie(err == WEAVE_NO_ERROR);
    VerifyOrDie(keyOut.KeyId == WeaveKeyId::MakeEpochKeyId(6));
    VerifyOrDie(keyOut.KeyLen == Profiles::Security::AppKeys::kWeaveAppEpochKeySize);
    memset(keyIn.Key, 0x75, Profiles::Security::AppKeys::kWeaveAppEpochKeySize);
    VerifyOrDie(memcmp(keyOut.Key, keyIn.Key, Profiles::Security::AppKeys::kWeaveAppEpochKeySize) == 0);
    VerifyOrDie(keyOut.StartTime == 0x76);

    // ===== Test 5: Enumerate epoch keys

    {
        enum { kKeyIdListSize = 32 };
        uint32_t keyIds[kKeyIdListSize];
        uint8_t keyCount, i;

        // Enumerate epoch keys
        err = groupKeyStore->EnumerateGroupKeys(WeaveKeyId::kType_AppEpochKey, keyIds, kKeyIdListSize, keyCount);
        VerifyOrDie(err == WEAVE_NO_ERROR);

        // Verify both epoch keys were returned.
        VerifyOrDie(keyCount >= 2);
        for (i = 0; i < keyCount && keyIds[i] != WeaveKeyId::MakeEpochKeyId(2); i++); VerifyOrDie(i < keyCount);
        for (i = 0; i < keyCount && keyIds[i] != WeaveKeyId::MakeEpochKeyId(6); i++); VerifyOrDie(i < keyCount);
    }

    // ===== Test 6: Enumerate all keys

    {
        enum { kKeyIdListSize = 32 };
        uint32_t keyIds[kKeyIdListSize];
        uint8_t keyCount, i;

        // Enumerate all keys
        err = groupKeyStore->EnumerateGroupKeys(WeaveKeyId::kType_None, keyIds, kKeyIdListSize, keyCount);
        VerifyOrDie(err == WEAVE_NO_ERROR);

        // Verify all keys were returned.
        VerifyOrDie(keyCount >= 5);
        for (i = 0; i < keyCount && keyIds[i] != WeaveKeyId::kServiceRootKey; i++); VerifyOrDie(i < keyCount);
        for (i = 0; i < keyCount && keyIds[i] != WeaveKeyId::kFabricSecret; i++); VerifyOrDie(i < keyCount);
        for (i = 0; i < keyCount && keyIds[i] != WeaveKeyId::MakeAppGroupMasterKeyId(0x42); i++); VerifyOrDie(i < keyCount);
        for (i = 0; i < keyCount && keyIds[i] != WeaveKeyId::MakeEpochKeyId(2); i++); VerifyOrDie(i < keyCount);
        for (i = 0; i < keyCount && keyIds[i] != WeaveKeyId::MakeEpochKeyId(6); i++); VerifyOrDie(i < keyCount);
    }

    // ===== Test 7: Overwrite the application master key

    // Update application master key
    keyIn.KeyId = WeaveKeyId::MakeAppGroupMasterKeyId(0x42);
    keyIn.KeyLen = Profiles::Security::AppKeys::kWeaveAppGroupMasterKeySize;
    memset(keyIn.Key, 0x24, keyIn.KeyLen);
    keyIn.GlobalId = 0x24242424;
    err = groupKeyStore->StoreGroupKey(keyIn);
    VerifyOrDie(err == WEAVE_NO_ERROR);

    // Retrieve and validate the update application master key
    memset(&keyOut, 0, sizeof(keyOut));
    err = groupKeyStore->RetrieveGroupKey(keyIn.KeyId, keyOut);
    VerifyOrDie(err == WEAVE_NO_ERROR);
    VerifyOrDie(keyOut.KeyId == keyIn.KeyId);
    VerifyOrDie(keyOut.KeyLen == Profiles::Security::AppKeys::kWeaveAppGroupMasterKeySize);
    VerifyOrDie(memcmp(keyOut.Key, keyIn.Key, keyOut.KeyLen) == 0);

    LogGroupKeys(groupKeyStore);

    // ===== Test 8: Clear all keys

    {
        enum { kKeyIdListSize = 32 };
        uint32_t keyIds[kKeyIdListSize];
        uint8_t keyCount;

        // Clear all keys
        err = groupKeyStore->Clear();
        VerifyOrDie(err == WEAVE_NO_ERROR);

        // Enumerate all keys
        err = groupKeyStore->EnumerateGroupKeys(WeaveKeyId::kType_None, keyIds, kKeyIdListSize, keyCount);
        VerifyOrDie(err == WEAVE_NO_ERROR);

        // Verify no keys were returned.
        VerifyOrDie(keyCount == 0);

        // Attempt to retrieve the fabric secret
        memset(&keyOut, 0, sizeof(keyOut));
        err = groupKeyStore->RetrieveGroupKey(WeaveKeyId::kFabricSecret, keyOut);
        VerifyOrDie(err == WEAVE_ERROR_KEY_NOT_FOUND);
    }

    LogGroupKeys(groupKeyStore);
}

} // namespace internal
} // namespace DeviceLayer
} // namespace Weave
} // namespace nl

#endif // WEAVE_DEVICE_GROUP_KEYSTORE_UNIT_TEST_H
