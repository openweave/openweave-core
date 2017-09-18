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
 *      This file implements application keys trait data sink interfaces.
 *
 */

#include <Weave/Support/CodeUtils.h>
#include <Weave/Support/MathUtils.h>
#include <Weave/Support/crypto/WeaveCrypto.h>
#include "ApplicationKeysTraitDataSink.h"

namespace Schema {
namespace Weave {
namespace Trait {
namespace Auth {
namespace ApplicationKeysTrait {

using namespace nl::Weave;
using namespace nl::Weave::TLV;
using namespace nl::Weave::Crypto;
using namespace nl::Weave::Profiles::DataManagement;
using namespace nl::Weave::Profiles::Security::AppKeys;

/**
 * The constructor method.
 */
ApplicationKeysTraitDataSink::ApplicationKeysTraitDataSink(void)
    : TraitDataSink(&ApplicationKeysTrait::TraitSchema)
{
    GroupKeyStore = NULL;
}

/**
 * This method sets the platform specific key store object.
 *
 * @param[in] groupKeyStore    Pointer to the platform group key store object. It can be
 *                             NULL if no key store object is required.
 * @retval    None.
 *
 */
void ApplicationKeysTraitDataSink::SetGroupKeyStore(GroupKeyStoreBase *groupKeyStore)
{
    GroupKeyStore = groupKeyStore;
}

/**
 * This method is invoked to signal the occurrence of an event.
 *
 * @param[in] aType            Event type.
 *
 * @retval    #WEAVE_NO_ERROR.
 *
 */
WEAVE_ERROR ApplicationKeysTraitDataSink::OnEvent(uint16_t aType, void *aInEventParam)
{
    WeaveLogDetail(DataManagement, "ApplicationKeysTraitDataSink::OnEvent event: %u", aType);

    return WEAVE_NO_ERROR;
}

/**
 * This method reads in the data associated with the specified leaf handle.
 *
 * @param[in] aLeafHandle      Path handle to a leaf node.
 * @param[in] aReader          A reference to the TLVReader object that contains leaf data.
 *
 * @retval    #WEAVE_NO_ERROR   On success.
 * @retval    #WEAVE_ERROR_INVALID_ARGUMENT
 *                             If no pointer to the GroupKeyStore was set or if value of
 *                             one of the decoded TLV parameters has invalid value.
 * @retval    #WEAVE_ERROR_INVALID_TLV_TAG
 *                             If input property path handle (aLeafHandle) has invalid value.
 * @retval    #WEAVE_ERROR_WRONG_TLV_TYPE
 *                             If input TLV data has wrong type.
 * @retval    other            Error returned by TLV reader.
 *
 */
WEAVE_ERROR ApplicationKeysTraitDataSink::SetLeafData(PropertyPathHandle aLeafHandle, TLVReader &aReader)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint32_t keyType;
    uint8_t keyCount;
    uint8_t maxKeyCount;
    WeaveGroupKey groupKey;

    if (ApplicationKeysTrait::kPropertyHandle_EpochKeys == aLeafHandle)
    {
        keyType = WeaveKeyId::kType_AppEpochKey;
        maxKeyCount = WEAVE_CONFIG_MAX_APPLICATION_EPOCH_KEYS;
    }
    else if (ApplicationKeysTrait::kPropertyHandle_MasterKeys == aLeafHandle)
    {
        keyType = WeaveKeyId::kType_AppGroupMasterKey;
        maxKeyCount = WEAVE_CONFIG_MAX_APPLICATION_GROUPS;
    }
    else
    {
        WeaveLogDetail(DataManagement, "<< UNKNOWN!");
        ExitNow(err = WEAVE_ERROR_INVALID_TLV_TAG);
    }

    VerifyOrExit(GroupKeyStore != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    {
        TLVType OuterContainerType;

        keyCount = 0;

        VerifyOrExit(aReader.GetType() == kTLVType_Array, err = WEAVE_ERROR_WRONG_TLV_TYPE);

        err = aReader.EnterContainer(OuterContainerType);
        SuccessOrExit(err);

        // Delete all group keys of the specified type from the group key store.
        GroupKeyStore->DeleteGroupKeysOfAType(keyType);

        while ((err = aReader.Next(kTLVType_Structure, AnonymousTag)) == WEAVE_NO_ERROR)
        {
            TLVType containerType2;

            if (keyCount == maxKeyCount)
            {
                WeaveLogDetail(DataManagement, "Cannot handle more than %d %s, skip",
                               maxKeyCount, (keyType == WeaveKeyId::kType_AppEpochKey) ? "epoch keys" : "application groups");
                break;
            }

            err = aReader.EnterContainer(containerType2);
            SuccessOrExit(err);

            if (ApplicationKeysTrait::kPropertyHandle_EpochKeys == aLeafHandle)
            {
                int64_t startTimeMSec;
                uint64_t tlvTag;
                nl::Weave::TLV::TLVType tlvType;
                uint8_t epochKeyNumber;

                err = aReader.Next(kTLVType_UnsignedInteger, ContextTag(kTag_EpochKey_KeyId));
                SuccessOrExit(err);
                err = aReader.Get(epochKeyNumber);
                SuccessOrExit(err);
                groupKey.KeyId = WeaveKeyId::MakeEpochKeyId(epochKeyNumber);

                err = aReader.Next();
                SuccessOrExit(err);

                tlvTag = aReader.GetTag();
                VerifyOrExit(tlvTag == ContextTag(kTag_EpochKey_StartTime), err = WEAVE_ERROR_INVALID_TLV_TAG);

                tlvType = aReader.GetType();
                VerifyOrExit(tlvType == kTLVType_SignedInteger ||
                             tlvType == kTLVType_UnsignedInteger, err = WEAVE_ERROR_WRONG_TLV_TYPE);

                err = aReader.Get(startTimeMSec);
                SuccessOrExit(err);

                // Verify that the time value is in the expected range (in milliseconds).
                VerifyOrExit(startTimeMSec >= 0 && startTimeMSec <= (static_cast<int64_t>(UINT32_MAX) * 1000),
                             err = WEAVE_ERROR_INVALID_ARGUMENT);

                // Convert UTC time value with msec precision to seconds (rounded up).
                groupKey.StartTime = nl::Weave::Platform::DivideBy1000(startTimeMSec + 999);

                err = aReader.Next(kTLVType_ByteString, ContextTag(kTag_EpochKey_Key));
            }
            else
            {
                uint8_t appGroupLocalNumber;

                err = aReader.Next(kTLVType_UnsignedInteger, ContextTag(kTag_ApplicationGroup_GlobalId));
                SuccessOrExit(err);
                err = aReader.Get(groupKey.GlobalId);
                SuccessOrExit(err);

                err = aReader.Next(kTLVType_UnsignedInteger, ContextTag(kTag_ApplicationGroup_ShortId));
                SuccessOrExit(err);
                err = aReader.Get(appGroupLocalNumber);
                SuccessOrExit(err);
                groupKey.KeyId = WeaveKeyId::MakeAppGroupMasterKeyId(appGroupLocalNumber);

                err = aReader.Next(kTLVType_ByteString, ContextTag(kTag_ApplicationGroup_Key));
            }
            SuccessOrExit(err);
            groupKey.KeyLen = aReader.GetLength();
            VerifyOrExit(groupKey.KeyLen <= sizeof(groupKey.Key), err = WEAVE_ERROR_INVALID_ARGUMENT);
            err = aReader.GetBytes(groupKey.Key, groupKey.KeyLen);
            SuccessOrExit(err);

            err = GroupKeyStore->StoreGroupKey(groupKey);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, "<< groupKeyId = %08X", groupKey.KeyId);

            keyCount++;

            err = aReader.ExitContainer(containerType2);
            SuccessOrExit(err);
        }

        // Note that ExitContainer() internally skips all unread elements till the end of current container.
        err = aReader.ExitContainer(OuterContainerType);
        SuccessOrExit(err);
    }

exit:
    ClearSecretData(groupKey.Key, sizeof(groupKey.Key));

    return err;
}

} // ApplicationKeysTrait
} // Auth
} // Trait
} // Weave
} // Schema
