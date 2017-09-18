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
 *      This file defines class, which is used as a default dummy (empty)
 *      implementation of the group key store.
 *
 */

#ifndef WEAVEDUMMYGROUPKEYSTORE_H_
#define WEAVEDUMMYGROUPKEYSTORE_H_

#include "WeaveApplicationKeys.h"

namespace nl {
namespace Weave {
namespace Profiles {
namespace Security {
namespace AppKeys {

/**
 *  @class DummyKeyStore
 *
 *  @brief
 *    The definition of the dummy group key store.
 *
 */
class DummyGroupKeyStore : public GroupKeyStoreBase
{
public:
    DummyGroupKeyStore(void);

    // Manage application group key material storage.
    virtual WEAVE_ERROR RetrieveGroupKey(uint32_t keyId, WeaveGroupKey& key);
    virtual WEAVE_ERROR StoreGroupKey(const WeaveGroupKey& key);
    virtual WEAVE_ERROR DeleteGroupKey(uint32_t keyId);
    virtual WEAVE_ERROR DeleteGroupKeysOfAType(uint32_t keyType);
    virtual WEAVE_ERROR EnumerateGroupKeys(uint32_t keyType, uint32_t *keyIds, uint8_t keyIdsArraySize, uint8_t & keyCount);
    virtual WEAVE_ERROR Clear(void);

private:
    // Retrieve and Store LastUsedEpochKeyId value.
    virtual WEAVE_ERROR RetrieveLastUsedEpochKeyId(void);
    virtual WEAVE_ERROR StoreLastUsedEpochKeyId(void);

    // Get current platform UTC time in seconds.
    virtual WEAVE_ERROR GetCurrentUTCTime(uint32_t& utcTime);
};

} // namespace AppKeys
} // namespace Security
} // namespace Profiles
} // namespace Weave
} // namespace nl

#endif /* WEAVEDUMMYGROUPKEYSTORE_H_ */
