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
 *      This file defines classes and interfaces for deriving and
 *      managing Weave constituent and application group keys.
 *
 */

#ifndef WEAVEAPPLICATIONKEYS_H_
#define WEAVEAPPLICATIONKEYS_H_

#include <Weave/Core/WeaveCore.h>

/**
 *   @namespace nl::Weave::Profiles::Security::AppKeys
 *
 *   @brief
 *     This namespace includes all interfaces within Weave for the Weave
 *     application keys library within the Weave security profile.
 */

namespace nl {
namespace Weave {
namespace Profiles {
namespace Security {
namespace AppKeys {

/**
 * @brief
 *   Key diversifier used for Weave fabric root key derivation. This value represents
 *   first 4 bytes of the SHA-1 HASH of "Fabric Root Key" phrase.
 */
extern const uint8_t kWeaveAppFabricRootKeyDiversifier[4];

/**
 * @brief
 *   Key diversifier used for Weave client root key derivation. This value represents
 *   first 4 bytes of the SHA-1 HASH of "Client Root Key" phrase.
 */
extern const uint8_t kWeaveAppClientRootKeyDiversifier[4];

/**
 * @brief
 *   Key diversifier used for Weave intermediate key derivation. This value represents
 *   first 4 bytes of the SHA-1 HASH of "Intermediate Key" phrase.
 */
extern const uint8_t kWeaveAppIntermediateKeyDiversifier[4];

/**
 * @brief
 *   Weave application keys protocol parameter definitions.
 */
enum
{
    // --- Key sizes.
    kWeaveAppGroupKeySize                               = 32,                    /**< Weave constituent group key size. */
    kWeaveAppRootKeySize                                = kWeaveAppGroupKeySize, /**< Weave application root key size. */
    kWeaveAppEpochKeySize                               = kWeaveAppGroupKeySize, /**< Weave application epoch key size. */
    kWeaveAppGroupMasterKeySize                         = kWeaveAppGroupKeySize, /**< Weave application group master key size. */
    kWeaveAppIntermediateKeySize                        = kWeaveAppGroupKeySize, /**< Weave application intermediate key size. */
    kWeaveFabricSecretSize                              = 36,                    /**< Weave fabric secret size. */

    // --- Key diversifiers sizes.
    /** Fabric root key diversifier size. */
    kWeaveAppFabricRootKeyDiversifierSize               = sizeof(kWeaveAppFabricRootKeyDiversifier),
    /** Client root key diversifier size. */
    kWeaveAppClientRootKeyDiversifierSize               = sizeof(kWeaveAppClientRootKeyDiversifier),
    /** Intermediate key diversifier size. */
    kWeaveAppIntermediateKeyDiversifierSize             = sizeof(kWeaveAppIntermediateKeyDiversifier),
};

/**
 *  @class WeaveGroupKey
 *
 *  @brief
 *    Contains information about Weave application group keys.
 *    Examples of keys that can be described by this class are: root key,
 *    epoch key, group master key, intermediate key, and fabric secret.
 *
 */
class WeaveGroupKey
{
public:
    enum
    {
        MaxKeySize                                      = kWeaveFabricSecretSize
    };
    uint32_t KeyId;                                      /**< The key ID. */
    uint8_t KeyLen;                                      /**< The key length. */
    uint8_t Key[MaxKeySize];                             /**< The secret key material. */
    union {
        uint32_t StartTime;                              /**< The epoch key start time. */
        uint32_t GlobalId;                               /**< The application group key global ID. */
    };
};

/**
 *  @class GroupKeyStoreBase
 *
 *  @brief
 *    The definition of the Weave group key store class. Functions in
 *    this class are called to manage application group keys.
 *
 */
class NL_DLL_EXPORT GroupKeyStoreBase
{
public:

    // Manage application group key material storage.
    virtual WEAVE_ERROR RetrieveGroupKey(uint32_t keyId, WeaveGroupKey& key) = 0;
    virtual WEAVE_ERROR StoreGroupKey(const WeaveGroupKey& key) = 0;
    virtual WEAVE_ERROR DeleteGroupKey(uint32_t keyId) = 0;
    virtual WEAVE_ERROR DeleteGroupKeysOfAType(uint32_t keyType) = 0;
    virtual WEAVE_ERROR EnumerateGroupKeys(uint32_t keyType, uint32_t *keyIds, uint8_t keyIdsArraySize, uint8_t & keyCount) = 0;
    virtual WEAVE_ERROR Clear(void) = 0;

    /**
     * Get current platform UTC time in seconds.
     *
     * @param[out]   utcTime             A reference to the time value.
     *
     * @retval #WEAVE_NO_ERROR           On success.
     * @retval #WEAVE_ERROR_UNSUPPORTED_CLOCK
     *                                   If platform does not support time functions.
     * @retval #WEAVE_ERROR_TIME_NOT_SYNCED_YET
     *                                   If platform does not have an accurate time yet.
     * @retval other                     Other Weave or platform error codes.
     */
    virtual WEAVE_ERROR GetCurrentUTCTime(uint32_t& utcTime) = 0;

    // Get current application key Id.
    WEAVE_ERROR GetCurrentAppKeyId(uint32_t keyId, uint32_t& curKeyId);

    // Get/Derive group key.
    WEAVE_ERROR GetGroupKey(uint32_t keyId, WeaveGroupKey& groupKey);

    // Derive application key.
    WEAVE_ERROR DeriveApplicationKey(uint32_t& appKeyId,
                                     const uint8_t *keySalt, uint8_t saltLen,
                                     const uint8_t *keyDiversifier, uint8_t diversifierLen,
                                     uint8_t *appKey, uint8_t keyBufSize, uint8_t keyLen,
                                     uint32_t& appGroupGlobalId);

protected:
    uint32_t LastUsedEpochKeyId;
    uint32_t NextEpochKeyStartTime;

    void Init(void);
    void OnEpochKeysChange(void);

    // Retrieve and Store LastUsedEpochKeyId value.
    virtual WEAVE_ERROR RetrieveLastUsedEpochKeyId(void) = 0;
    virtual WEAVE_ERROR StoreLastUsedEpochKeyId(void) = 0;

private:
    // Derive fabric/client root key.
    WEAVE_ERROR DeriveFabricOrClientRootKey(uint32_t rootKeyId, WeaveGroupKey& rootKey);

    // Derive intermediate key.
    WEAVE_ERROR DeriveIntermediateKey(uint32_t keyId, WeaveGroupKey& intermediateKey);
};


extern WEAVE_ERROR GetAppGroupMasterKeyId(uint32_t groupGlobalId, GroupKeyStoreBase *groupKeyStore, uint32_t& groupMasterKeyId);

} // namespace AppKeys
} // namespace Security
} // namespace Profiles
} // namespace Weave
} // namespace nl

#endif /* WEAVEAPPLICATIONKEYS_H_ */
