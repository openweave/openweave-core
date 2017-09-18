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
 *      This file implements interfaces for the default group key store.
 *
 */

#include <Weave/Core/WeaveCore.h>
#include "WeaveDummyGroupKeyStore.h"

namespace nl {
namespace Weave {
namespace Profiles {
namespace Security {
namespace AppKeys {

DummyGroupKeyStore::DummyGroupKeyStore(void)
{
    Init();
}

WEAVE_ERROR DummyGroupKeyStore::RetrieveGroupKey(uint32_t keyId, WeaveGroupKey& key)
{
    return WEAVE_ERROR_UNSUPPORTED_WEAVE_FEATURE;
}

WEAVE_ERROR DummyGroupKeyStore::StoreGroupKey(const WeaveGroupKey& key)
{
    return WEAVE_ERROR_UNSUPPORTED_WEAVE_FEATURE;
}

WEAVE_ERROR DummyGroupKeyStore::DeleteGroupKey(uint32_t keyId)
{
    return WEAVE_ERROR_UNSUPPORTED_WEAVE_FEATURE;
}

WEAVE_ERROR DummyGroupKeyStore::DeleteGroupKeysOfAType(uint32_t keyType)
{
    return WEAVE_ERROR_UNSUPPORTED_WEAVE_FEATURE;
}

WEAVE_ERROR DummyGroupKeyStore::EnumerateGroupKeys(uint32_t keyType, uint32_t *keyIds, uint8_t keyIdsArraySize, uint8_t & keyCount)
{
    return WEAVE_ERROR_UNSUPPORTED_WEAVE_FEATURE;
}

WEAVE_ERROR DummyGroupKeyStore::Clear(void)
{
    return WEAVE_ERROR_UNSUPPORTED_WEAVE_FEATURE;
}

WEAVE_ERROR DummyGroupKeyStore::RetrieveLastUsedEpochKeyId(void)
{
    return WEAVE_ERROR_UNSUPPORTED_WEAVE_FEATURE;
}

WEAVE_ERROR DummyGroupKeyStore::StoreLastUsedEpochKeyId(void)
{
    return WEAVE_ERROR_UNSUPPORTED_WEAVE_FEATURE;
}

WEAVE_ERROR DummyGroupKeyStore::GetCurrentUTCTime(uint32_t& utcTime)
{
    return WEAVE_ERROR_UNSUPPORTED_WEAVE_FEATURE;
}

} // namespace AppKeys
} // namespace Security
} // namespace Profiles
} // namespace Weave
} // namespace nl
