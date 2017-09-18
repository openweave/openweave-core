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
 *      This file implements interfaces for deriving and retrieving
 *      Weave constituent and application group keys.
 *
 */

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveKeyIds.h>
#include "WeaveApplicationKeys.h"
#include <Weave/Core/WeaveEncoding.h>
#include <Weave/Support/crypto/WeaveCrypto.h>
#include <Weave/Support/crypto/HKDF.h>
#include <Weave/Support/CodeUtils.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace Security {
namespace AppKeys {

using namespace nl::Weave::Encoding;
using namespace nl::Weave::Crypto;

// Key diversifier used for Weave fabric root key derivation.
const uint8_t kWeaveAppFabricRootKeyDiversifier[] = { 0x21, 0xFA, 0x8F, 0x6A };

// Key diversifier used for Weave client root key derivation.
const uint8_t kWeaveAppClientRootKeyDiversifier[] = { 0x53, 0xE3, 0xFF, 0xE5 };

// Key diversifier used for Weave intermediate key derivation.
const uint8_t kWeaveAppIntermediateKeyDiversifier[] = { 0xBC, 0xAA, 0x95, 0xAD };

/**
 * Get application group master key ID given application group global ID.
 *
 * @param[in]   groupGlobalId    The application group global ID.
 * @param[in]   groupKeyStore    A pointer to the group key store object.
 * @param[out]  groupMasterKeyId The application group master key ID.
 *
 * @retval #WEAVE_NO_ERROR       On success.
 * @retval #WEAVE_ERROR_INVALID_ARGUMENT
 *                               If pointer to the group key store is not provided.
 * @retval #WEAVE_ERROR_UNSUPPORTED_WEAVE_FEATURE
 *                               If FabricState object wasn't initialized with fully
 *                               functional group key store.
 * @retval #WEAVE_ERROR_KEY_NOT_FOUND
 *                               If a group key with specified global ID is not found
 *                               in the platform key store.
 * @retval other                 Other platform-specific errors returned by the platform
 *                               key store APIs.
 *
 */
WEAVE_ERROR GetAppGroupMasterKeyId(uint32_t groupGlobalId, GroupKeyStoreBase *groupKeyStore, uint32_t& groupMasterKeyId)
{
    WEAVE_ERROR err;
    uint32_t groupMasterKeyIds[WEAVE_CONFIG_MAX_APPLICATION_GROUPS];
    uint8_t groupMasterKeyCount;
    WeaveGroupKey groupMasterKey;

    // Verify the group key store object is provided.
    VerifyOrExit(groupKeyStore != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Enumerate all application group master keys.
    err = groupKeyStore->EnumerateGroupKeys(WeaveKeyId::kType_AppGroupMasterKey, groupMasterKeyIds, sizeof(groupMasterKeyIds) / sizeof(uint32_t), groupMasterKeyCount);
    SuccessOrExit(err);

    for (int i = 0; i < groupMasterKeyCount; i++)
    {
        // Get application group master key.
        err = groupKeyStore->RetrieveGroupKey(groupMasterKeyIds[i], groupMasterKey);
        SuccessOrExit(err);

        // If group global Id matches.
        if (groupMasterKey.GlobalId == groupGlobalId)
        {
            groupMasterKeyId = groupMasterKey.KeyId;
            ExitNow();
        }
    }

    ExitNow(err = WEAVE_ERROR_KEY_NOT_FOUND);

exit:
    ClearSecretData(groupMasterKey.Key, groupMasterKey.MaxKeySize);

    return err;
}

/**
 * Initialize local group key store parameters.
 */
void GroupKeyStoreBase::Init(void)
{
    LastUsedEpochKeyId = WeaveKeyId::kNone;
    NextEpochKeyStartTime = UINT32_MAX;
}

/**
 * Returns current key ID.
 * Sets member variables associated with epoch keys to the default values when any change
 * (delete or store) happens to the set of application epoch keys. It is the responsibility
 * of the subclass that implements StoreGroupKey(), DeleteGroupKey(), and
 * DeleteGroupKeysOfAType() functions to call this method.
 */
void GroupKeyStoreBase::OnEpochKeysChange(void)
{
    LastUsedEpochKeyId = WeaveKeyId::kNone;
    NextEpochKeyStartTime = UINT32_MAX;
}

/**
 * Returns current key ID.
 * Finds current epoch key based on the current system time and the start time parameter
 * of each epoch key. If system doesn't have valid, accurate time then last-used epoch key
 * ID is returned.
 *
 * @param[in]    keyId           The application key ID.
 * @param[out]   curKeyId        The application current key ID.
 *
 * @retval #WEAVE_NO_ERROR       On success.
 * @retval #WEAVE_ERROR_INVALID_KEY_ID
 *                               If the input key ID had an invalid value.
 * @retval #WEAVE_ERROR_KEY_NOT_FOUND
 *                               If epoch keys are not found in the platform key store.
 * @retval other                 Other platform-specific errors returned by the platform
 *                               key store APIs.
 *
 */
WEAVE_ERROR GroupKeyStoreBase::GetCurrentAppKeyId(uint32_t keyId, uint32_t& curKeyId)
{
    WEAVE_ERROR err;
    uint32_t epochKeyIds[WEAVE_CONFIG_MAX_APPLICATION_EPOCH_KEYS];
    uint32_t epochKeyStartTimes[WEAVE_CONFIG_MAX_APPLICATION_EPOCH_KEYS];
    uint8_t epochKeyCount;
    uint32_t curUTCTime;
    int curEpochKeyIdIdx;
    WeaveGroupKey epochKey;

    // If current application key is not used to derive this application key.
    if (!WeaveKeyId::UsesCurrentEpochKey(keyId))
    {
        curKeyId = keyId;
        ExitNow(err = WEAVE_NO_ERROR);
    }

    // If platform key store state is idle (happens after platform reboot)
    // retrieve the last used epoch key Id from the store.
    if (LastUsedEpochKeyId == WeaveKeyId::kNone)
    {
        err = RetrieveLastUsedEpochKeyId();
        // If failed to retrieve value from the store assume that the value is still idle.
        if (err != WEAVE_NO_ERROR)
        {
            LastUsedEpochKeyId = WeaveKeyId::kNone;
            err = WEAVE_NO_ERROR;
        }
    }

    // Get current UTC time.
    // If platform doesn't support time functions or doesn't have an accurate time yet then assume that current
    // time is zero so the protocol below selects "oldest" epoch key (epoch key with smallest StartTime value).
    err = GetCurrentUTCTime(curUTCTime);
    if (err == WEAVE_ERROR_UNSUPPORTED_CLOCK || err == WEAVE_ERROR_TIME_NOT_SYNCED_YET)
    {
        curUTCTime = 0;
        err = WEAVE_NO_ERROR;
    }
    SuccessOrExit(err);

    // Update LastUsedEpochKeyId and NextEpochKeyStartTime values if needed.
    if (LastUsedEpochKeyId == WeaveKeyId::kNone || curUTCTime > NextEpochKeyStartTime)
    {
        // Enumerate all application epoch keys.
        err = EnumerateGroupKeys(WeaveKeyId::kType_AppEpochKey, epochKeyIds, sizeof(epochKeyIds) / sizeof(uint32_t), epochKeyCount);
        SuccessOrExit(err);

        VerifyOrExit(epochKeyCount > 0, err = WEAVE_ERROR_KEY_NOT_FOUND);

        // Get start time for each epoch key.
        for (int i = 0; i < epochKeyCount; i++)
        {
            err = RetrieveGroupKey(epochKeyIds[i], epochKey);
            SuccessOrExit(err);

            epochKeyStartTimes[i] = epochKey.StartTime;
        }

        // Search the list of epoch keys for the "current" epoch key.  The current epoch key is defined
        // as the newest epoch key (i.e. the key with the greatest start time) that has a start time that
        // is less than or equal to the current time.  In the case that the current time is unknown
        // (curUTCTime == 0) then the oldest epoch key (the key with the earliest start time) is selected.
        // Similarly, if the current time is known, but it falls before the start times of all keys, then
        // the oldest key is used.  If there is only one epoch key in the list then it is selected by default
        // (curEpochKeyIdIdx is initialized to 0).
        curEpochKeyIdIdx = 0;
        for (int i = 1; i < epochKeyCount; i++)
        {
            // Search unsorted list.
            if ((epochKeyStartTimes[i] > epochKeyStartTimes[curEpochKeyIdIdx] && epochKeyStartTimes[i] <= curUTCTime) ||
                (epochKeyStartTimes[i] < epochKeyStartTimes[curEpochKeyIdIdx] && epochKeyStartTimes[curEpochKeyIdIdx] > curUTCTime))
            {
                curEpochKeyIdIdx = i;
            }
        }

        // Search the list of keys again to find the next key (in order of start time), relative to the
        // current epoch key.  The start time of this key effectively marks the end-time of the current
        // key.  In the case where there is no next key (e.g. when the current key is the only key in the
        // list) use an end time of "indefinite" (UINT32_MAX), implying that the current key should remain
        // current until a new set of epoch keys is received.
        for (int i = 0; i < epochKeyCount; i++)
        {
            if ((epochKeyStartTimes[i] > epochKeyStartTimes[curEpochKeyIdIdx] && epochKeyStartTimes[i] < NextEpochKeyStartTime))
            {
                NextEpochKeyStartTime = epochKeyStartTimes[i];
            }
        }

        LastUsedEpochKeyId = epochKeyIds[curEpochKeyIdIdx];

        // Store GroupKeyStoreBase state.
        err = StoreLastUsedEpochKeyId();
        SuccessOrExit(err);
    }

    // Encode current epoch key id in the key id value.
    curKeyId = WeaveKeyId::UpdateEpochKeyId(keyId, LastUsedEpochKeyId);

exit:
    ClearSecretData(epochKey.Key, epochKey.MaxKeySize);

    return err;
}

/**
 * Get application group key.
 * This function derives or retrieves application group keys. Key types supported by
 * this function are: fabric secret, root key, epoch key, group master key, and intermediate key.
 *
 * @param[in]    keyId           The group key ID.
 * @param[out]   groupKey        A reference to the group key object.
 *
 * @retval #WEAVE_NO_ERROR       On success.
 * @retval #WEAVE_ERROR_INVALID_KEY_ID
 *                               If the requested key has invalid key ID.
 * @retval #WEAVE_ERROR_INVALID_ARGUMENT
 *                               If the platform key store returns invalid key parameters.
 * @retval other                 Other platform-specific errors returned by the platform
 *                               key store APIs.
 *
 */
WEAVE_ERROR GroupKeyStoreBase::GetGroupKey(uint32_t keyId, WeaveGroupKey& groupKey)
{
    WEAVE_ERROR err;
    uint32_t rootKeyId;
    uint8_t expectedKeyLen;

    // Get current key Id.
    err = GetCurrentAppKeyId(keyId, keyId);
    SuccessOrExit(err);

    switch (WeaveKeyId::GetType(keyId))
    {
    case WeaveKeyId::kType_AppRootKey:
        rootKeyId = WeaveKeyId::GetRootKeyId(keyId);

        // Derive fabric/client root key.
        if (rootKeyId == WeaveKeyId::kFabricRootKey || rootKeyId == WeaveKeyId::kClientRootKey)
        {
            err = DeriveFabricOrClientRootKey(rootKeyId, groupKey);
            break;
        }
        // Otherwise, fall through to retrieve service root key from the key store.
    case WeaveKeyId::kType_General:
    case WeaveKeyId::kType_AppEpochKey:
    case WeaveKeyId::kType_AppGroupMasterKey:
        // Retrieve key from the key store.
        err = RetrieveGroupKey(keyId, groupKey);
        break;

    case WeaveKeyId::kType_AppIntermediateKey:
        // Derive intermediate key.
        err = DeriveIntermediateKey(keyId, groupKey);
        break;

    default:
        ExitNow(err = WEAVE_ERROR_INVALID_KEY_ID);
    }
    SuccessOrExit(err);

    if (keyId == WeaveKeyId::kFabricSecret)
        expectedKeyLen = kWeaveFabricSecretSize;
    else
        expectedKeyLen = kWeaveAppGroupKeySize;

    // Verify correct key length and key id.
    VerifyOrExit(groupKey.KeyLen == expectedKeyLen &&
                 groupKey.KeyId == keyId, err = WEAVE_ERROR_INVALID_ARGUMENT);

exit:
    return err;
}

/**
 * Derive fabric/client root key.
 * Fabric and client root keys are derived from fabric secret, which is retrieved
 * from the platform key store.
 *
 * @param[in]   rootKeyId        The key ID associated with the requested root key.
 * @param[out]  rootKey          A reference to the key material object.
 *
 * @retval #WEAVE_NO_ERROR       On success.
 * @retval #WEAVE_ERROR_INVALID_KEY_ID
 *                               If the requested key has invalid key ID.
 * @retval #WEAVE_ERROR_INVALID_ARGUMENT
 *                               If the platform key store returns invalid key parameters.
 * @retval other                 Other platform-specific errors returned by the platform
 *                               key store APIs.
 *
 */
WEAVE_ERROR GroupKeyStoreBase::DeriveFabricOrClientRootKey(uint32_t rootKeyId, WeaveGroupKey& rootKey)
{
    WEAVE_ERROR err;
    WeaveGroupKey fabricSecret;
    const uint8_t *rootKeyDiversifier;

    // Set group root key id.
    rootKey.KeyId = rootKeyId;

    // Set root key diversifier value.
    if (rootKeyId == WeaveKeyId::kFabricRootKey)
        rootKeyDiversifier = kWeaveAppFabricRootKeyDiversifier;
    else
        rootKeyDiversifier = kWeaveAppClientRootKeyDiversifier;

    // Set key length.
    rootKey.KeyLen = kWeaveAppRootKeySize;

    // Get fabric secret.
    err = RetrieveGroupKey(WeaveKeyId::kFabricSecret, fabricSecret);
    SuccessOrExit(err);

    // Verify correct key size.
    VerifyOrExit(fabricSecret.KeyLen == kWeaveFabricSecretSize, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Derive fabric/client root key.
    err = HKDFSHA1::DeriveKey(NULL, 0,
                              fabricSecret.Key, fabricSecret.KeyLen,
                              NULL, 0,
                              rootKeyDiversifier, kWeaveAppFabricRootKeyDiversifierSize,
                              rootKey.Key, sizeof(rootKey.Key), kWeaveAppRootKeySize);
    SuccessOrExit(err);

exit:
    return err;
}

/**
 * Derive application intermediate key.
 * The intermediate key is derived from the root key and epoch key material specified
 * in the @a keyId input.
 *
 * @param[in]   keyId            The requested intermediate key ID.
 * @param[out]  intermediateKey  A reference to the key material object.
 *
 * @retval #WEAVE_NO_ERROR       On success.
 * @retval #WEAVE_ERROR_INVALID_KEY_ID
 *                               If the requested key has invalid key ID.
 * @retval #WEAVE_ERROR_INVALID_ARGUMENT
 *                               If the platform key store returns invalid key parameters.
 * @retval other                 Other platform-specific errors returned by the platform
 *                               key store APIs.
 *
 */
WEAVE_ERROR GroupKeyStoreBase::DeriveIntermediateKey(uint32_t keyId, WeaveGroupKey& intermediateKey)
{
    WEAVE_ERROR err;
    WeaveGroupKey rootKey;
    WeaveGroupKey epochKey;

    // Set group root key id.
    rootKey.KeyId = WeaveKeyId::GetRootKeyId(keyId);

    // Get/Derive root key.
    err = GetGroupKey(rootKey.KeyId, rootKey);
    SuccessOrExit(err);

    // Set epoch key id.
    epochKey.KeyId = WeaveKeyId::GetEpochKeyId(keyId);

    // Get epoch key.
    err = RetrieveGroupKey(epochKey.KeyId, epochKey);
    SuccessOrExit(err);

    // Verify correct key size.
    VerifyOrExit(epochKey.KeyLen == kWeaveAppEpochKeySize, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Set intermediate key length and Id.
    intermediateKey.KeyLen = kWeaveAppIntermediateKeySize;
    intermediateKey.KeyId = keyId;

    // Derive intermediate key.
    err = HKDFSHA1::DeriveKey(NULL, 0,
                              rootKey.Key, rootKey.KeyLen,
                              epochKey.Key, epochKey.KeyLen,
                              kWeaveAppIntermediateKeyDiversifier, kWeaveAppIntermediateKeyDiversifierSize,
                              intermediateKey.Key, sizeof(intermediateKey.Key), intermediateKey.KeyLen);
    SuccessOrExit(err);

exit:
    ClearSecretData(rootKey.Key, rootKey.MaxKeySize);
    ClearSecretData(epochKey.Key, epochKey.MaxKeySize);

    return err;
}

/**
 * Derives application key.
 * Three types of application keys are supported: current application key, rotating
 * application key and static application key. When current application key is requested
 * the function finds and uses the current epoch key based on the current system time and
 * the start time parameter of each epoch key.
 *
 * @param[inout] keyId              A reference to the requested key ID. When current application
 *                                  key is requested this field is updated to reflect the new
 *                                  type (rotating application key) and the actual epoch key ID
 *                                  that was used to generate application key.
 * @param[in]    keySalt            A pointer to a buffer with application key salt value.
 * @param[in]    saltLen            The length of the application key salt.
 * @param[in]    keyDiversifier     A pointer to a buffer with application key diversifier value.
 * @param[in]    diversifierLen     The length of the application key diversifier.
 * @param[out]   appKey             A pointer to a buffer where the derived key will be written.
 * @param[in]    keyBufSize         The length of the supplied key buffer.
 * @param[in]    keyLen             The length of the requested key material.
 * @param[out]   appGroupGlobalId   The application group global ID of the associated key.
 *
 * @retval #WEAVE_NO_ERROR          On success.
 * @retval #WEAVE_ERROR_BUFFER_TOO_SMALL
 *                                  If the provided key buffer size is not sufficient.
 * @retval #WEAVE_ERROR_INVALID_KEY_ID
 *                                  If the requested key has invalid key ID.
 * @retval #WEAVE_ERROR_INVALID_ARGUMENT
 *                                  If the platform key store returns invalid key parameters
 *                                  or key identifier has invalid value.
 * @retval other                    Other platform-specific errors returned by the platform
 *                                  key store APIs.
 *
 */
WEAVE_ERROR GroupKeyStoreBase::DeriveApplicationKey(uint32_t& keyId,
                                                    const uint8_t *keySalt, uint8_t saltLen,
                                                    const uint8_t *keyDiversifier, uint8_t diversifierLen,
                                                    uint8_t *appKey, uint8_t keyBufSize, uint8_t keyLen,
                                                    uint32_t& appGroupGlobalId)
{
    WEAVE_ERROR err;
    WeaveGroupKey intermediateKey;
    WeaveGroupKey groupMasterKey;
    uint32_t localKeyId;

    // Verify that key identifier has correct type.
    VerifyOrExit(WeaveKeyId::IsAppGroupKey(keyId), err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Get current key Id.
    err = GetCurrentAppKeyId(keyId, keyId);
    SuccessOrExit(err);

    // Set first requested key material, which can be of two types:
    //  - If keyId is an app static key then localKeyId is root key id.
    //  - If keyId is an app rotating key then localKeyId is intermediate key id.
    localKeyId = WeaveKeyId::GetRootKeyId(keyId);
    if (WeaveKeyId::IsAppRotatingKey(keyId))
    {
        uint32_t epochKeyId = WeaveKeyId::GetEpochKeyId(keyId);
        localKeyId = WeaveKeyId::MakeAppIntermediateKeyId(localKeyId, epochKeyId, false);
    }

    // Get root or intermediate key material.
    err = GetGroupKey(localKeyId, intermediateKey);
    SuccessOrExit(err);

    // Set application group master key id.
    localKeyId = WeaveKeyId::GetAppGroupMasterKeyId(keyId);

    // Retrieve application group master key.
    err = RetrieveGroupKey(localKeyId, groupMasterKey);
    SuccessOrExit(err);

    // Verify correct key size.
    VerifyOrExit(groupMasterKey.KeyLen == kWeaveAppGroupMasterKeySize, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Derive application key material.
    err = HKDFSHA1::DeriveKey(keySalt, saltLen,
                              intermediateKey.Key, intermediateKey.KeyLen,
                              groupMasterKey.Key, groupMasterKey.KeyLen,
                              keyDiversifier, diversifierLen,
                              appKey, keyBufSize, keyLen);
    SuccessOrExit(err);

    // Return the global id of the associated application group.
    appGroupGlobalId = groupMasterKey.GlobalId;

exit:
    ClearSecretData(intermediateKey.Key, intermediateKey.MaxKeySize);
    ClearSecretData(groupMasterKey.Key, groupMasterKey.MaxKeySize);

    return err;
}

} // namespace AppKeys
} // namespace Security
} // namespace Profiles
} // namespace Weave
} // namespace nl
