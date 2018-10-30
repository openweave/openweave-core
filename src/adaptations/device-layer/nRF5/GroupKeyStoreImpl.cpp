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

/**
 *    @file
 *          Provides an implementation of the Weave GroupKeyStore interface
 *          for the ESP32 platform.
 */

#include <Weave/DeviceLayer/internal/WeaveDeviceLayerInternal.h>
#include <Weave/DeviceLayer/nRF5/GroupKeyStoreImpl.h>

using namespace ::nl;
using namespace ::nl::Weave;
using namespace ::nl::Weave::Profiles::Security::AppKeys;

namespace nl {
namespace Weave {
namespace DeviceLayer {
namespace Internal {

WEAVE_ERROR GroupKeyStoreImpl::RetrieveGroupKey(uint32_t keyId, WeaveGroupKey & key)
{
    // TODO: implement me
    return WEAVE_ERROR_KEY_NOT_FOUND;
}

WEAVE_ERROR GroupKeyStoreImpl::StoreGroupKey(const WeaveGroupKey & key)
{
    // TODO: implement me
    return WEAVE_NO_ERROR;
}

WEAVE_ERROR GroupKeyStoreImpl::DeleteGroupKey(uint32_t keyId)
{
    // TODO: implement me
    return WEAVE_NO_ERROR;
}

WEAVE_ERROR GroupKeyStoreImpl::DeleteGroupKeysOfAType(uint32_t keyType)
{
    // TODO: implement me
    return WEAVE_NO_ERROR;
}

WEAVE_ERROR GroupKeyStoreImpl::EnumerateGroupKeys(uint32_t keyType, uint32_t * keyIds,
        uint8_t keyIdsArraySize, uint8_t & keyCount)
{
    // TODO: implement me
    keyCount = 0;
    return WEAVE_NO_ERROR;
}

WEAVE_ERROR GroupKeyStoreImpl::Clear(void)
{
    // TODO: implement me
    return WEAVE_NO_ERROR;
}

WEAVE_ERROR GroupKeyStoreImpl::RetrieveLastUsedEpochKeyId(void)
{
    WEAVE_ERROR err;

    err = ReadConfigValue(kConfigKey_LastUsedEpochKeyId, LastUsedEpochKeyId);
    if (err == WEAVE_DEVICE_ERROR_CONFIG_NOT_FOUND)
    {
        LastUsedEpochKeyId = WeaveKeyId::kNone;
        err = WEAVE_NO_ERROR;
    }
    return err;
}

WEAVE_ERROR GroupKeyStoreImpl::StoreLastUsedEpochKeyId(void)
{
    return WriteConfigValue(kConfigKey_LastUsedEpochKeyId, LastUsedEpochKeyId);
}

WEAVE_ERROR GroupKeyStoreImpl::Init()
{
    // TODO: implement me
    return WEAVE_NO_ERROR;
}

} // namespace Internal
} // namespace DeviceLayer
} // namespace Weave
} // namespace nl

