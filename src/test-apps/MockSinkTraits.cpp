/*
 *
 *    Copyright (c) 2018 Google LLC.
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
 *      Sample mock trait data sinks that implement the simple and complex mock traits.
 */

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#include <stdint.h>
#include <inttypes.h>

// Note that the choice of namespace alias must be made up front for each and every compile unit
// This is because many include paths could set the default alias to unintended target.
#include <Weave/Profiles/bulk-data-transfer/Development/BDXManagedNamespace.hpp>
#include <Weave/Profiles/data-management/Current/WdmManagedNamespace.h>

#include <Weave/Support/CodeUtils.h>
#include <Weave/Profiles/security/ApplicationKeysTrait.h>
#include <Weave/Profiles/security/ApplicationKeysStructSchema.h>
#include "ToolCommon.h"
#include "MockSinkTraits.h"
#include <schema/weave/common/DayOfWeekEnum.h>

using namespace ::nl::Weave;
using namespace ::nl::Weave::TLV;
using namespace ::nl::Weave::Profiles::DataManagement;
using namespace ::nl::Weave::Profiles::Security::AppKeys;

using namespace Weave::Trait::Locale;
using namespace Weave::Trait::Security;
using namespace Schema::Weave::Trait::Auth;
using namespace Schema::Weave::Trait::Auth::ApplicationKeysTrait;
using namespace Schema::Nest::Test::Trait;
using namespace Schema::Weave::Common;

static size_t MOCK_strlcpy(char * dst, const char * src, size_t size);

static size_t MOCK_strlcpy(char * dst, const char * src, size_t size)
{
    // note that srtncpy doesn't NUL-terminate the destination if source is too long
    strncpy(dst, src, size - 1);
    // forcifully NUL-terminate the destination
    dst[size - 1] = '\0';
    // keep compatibility with common strlcpy definition
    return strlen(src);
}

MockTraitDataSink::MockTraitDataSink(const TraitSchemaEngine * aEngine)
    : TraitDataSink(aEngine)
{
}

#if WEAVE_CONFIG_ENABLE_WDM_UPDATE
MockTraitUpdatableDataSink::MockTraitUpdatableDataSink(const TraitSchemaEngine * aEngine)
        : TraitUpdatableDataSink(aEngine)
{
}
#endif // WEAVE_CONFIG_ENABLE_WDM_UPDATE

WEAVE_ERROR
MockTraitDataSourceDelegate::GetData(PropertyPathHandle aHandle,
                                     uint64_t aTagToWrite,
                                     nl::Weave::TLV::TLVWriter &aWriter,
                                     bool &aIsNull,
                                     bool &aIsPresent)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    aIsNull = false;
    aIsPresent = true;
    err = GetLeafData(aHandle, aTagToWrite, aWriter);

    return err;
}

LocaleSettingsTraitDataSink::LocaleSettingsTraitDataSink()
    : MockTraitDataSink(&LocaleSettingsTrait::TraitSchema)
{
    memset(mLocale, 0, sizeof(mLocale));
}

WEAVE_ERROR
LocaleSettingsTraitDataSink::SetLeafData(PropertyPathHandle aLeafHandle, TLVReader &aReader)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    switch (aLeafHandle) {
        case LocaleSettingsTrait::kPropertyHandle_active_locale:
            char next_locale[kMaxNumOfCharsPerLocale];
            err = aReader.GetString(next_locale, MAX_LOCALE_SIZE);
            SuccessOrExit(err);
            if (strncmp(next_locale, mLocale, MAX_LOCALE_SIZE) != 0)
            {
                WeaveLogDetail(DataManagement, "<<  active_locale is changed from \"%s\" to \"%s\"", mLocale, next_locale);
                memcpy(mLocale, next_locale, MAX_LOCALE_SIZE);
            }

            WeaveLogDetail(DataManagement, "<<  active_locale = \"%s\"", mLocale);
            break;

        default:
            WeaveLogDetail(DataManagement, "<<  UNKNOWN!");
            err = WEAVE_ERROR_TLV_TAG_NOT_FOUND;
    }

exit:
    return err;
}

WEAVE_ERROR
LocaleSettingsTraitDataSink::GetLeafData(PropertyPathHandle aLeafHandle, uint64_t aTagToWrite, TLVWriter &aWriter)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    switch (aLeafHandle) {
        case LocaleSettingsTrait::kPropertyHandle_active_locale:
            err = aWriter.PutString(aTagToWrite, mLocale);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  active_locale = \"%s\"", mLocale);
            break;

        default:
            WeaveLogDetail(DataManagement, ">>  UNKNOWN!");
            ExitNow(err = WEAVE_ERROR_TLV_TAG_NOT_FOUND);
    }

exit:
    WeaveLogFunctError(err);

    return err;
}

WEAVE_ERROR LocaleSettingsTraitDataSink::GetNextDictionaryItemKey(PropertyPathHandle aDictionaryHandle, uintptr_t &aContext, PropertyDictionaryKey &aKey)
{
    return WEAVE_END_OF_INPUT;
}

LocaleCapabilitiesTraitDataSink::LocaleCapabilitiesTraitDataSink()
    : MockTraitDataSink(&LocaleCapabilitiesTrait::TraitSchema),
      mNumLocales(0)
{
    memset(mLocales, 0, sizeof(mLocales));
}

WEAVE_ERROR
LocaleCapabilitiesTraitDataSink::OnEvent(uint16_t aType, void *aInParam)
{
    WeaveLogDetail(DataManagement, "LocaleCapabilitiesTraitDataSink::OnEvent event: %u", aType);
    return WEAVE_NO_ERROR;
}

WEAVE_ERROR
LocaleCapabilitiesTraitDataSink::SetLeafData(PropertyPathHandle aLeafHandle, TLVReader &aReader)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    char next_locale[kMaxNumOfCharsPerLocale];

    if (LocaleCapabilitiesTrait::kPropertyHandle_available_locales == aLeafHandle)
    {
        nl::Weave::TLV::TLVType OuterContainerType;

        // clear all locales
        mNumLocales = 0;

        VerifyOrExit(aReader.GetType() == nl::Weave::TLV::kTLVType_Array, err = WEAVE_ERROR_WRONG_TLV_TYPE);

        err = aReader.EnterContainer(OuterContainerType);
        SuccessOrExit(err);

        while (WEAVE_NO_ERROR == (err = aReader.Next()))
        {
            VerifyOrExit(nl::Weave::TLV::kTLVType_UTF8String == aReader.GetType(), err = WEAVE_ERROR_WRONG_TLV_TYPE);
            VerifyOrExit(nl::Weave::TLV::AnonymousTag == aReader.GetTag(), err = WEAVE_ERROR_INVALID_TLV_TAG);

            err = aReader.GetString(next_locale, kMaxNumOfCharsPerLocale);
            SuccessOrExit(err);
            if (strncmp(next_locale, mLocales[mNumLocales], MAX_LOCALE_SIZE) != 0)
            {
                 WeaveLogDetail(DataManagement,  "<<  locale[%u]  is changed from [%s] to [%s]", mNumLocales, mLocales[mNumLocales], next_locale);
                 memcpy(mLocales[mNumLocales], next_locale, MAX_LOCALE_SIZE);
            }

            WeaveLogDetail(DataManagement, "<<  locale[%u] = [%s]", mNumLocales, mLocales[mNumLocales]);

            ++mNumLocales;

            if (kMaxNumOfLocals == mNumLocales)
            {
                WeaveLogDetail(DataManagement, "Cannot handle more than %d locales, skip", kMaxNumOfLocals);
                break;
            }
        }

        // Note that ExitContainer internally skip all unread elements till the end of current container
        err = aReader.ExitContainer(OuterContainerType);
        SuccessOrExit(err);
    }
    else
    {
        WeaveLogDetail(DataManagement, "<<  UNKNOWN!");
        ExitNow(err = WEAVE_ERROR_INVALID_TLV_TAG);
    }

exit:
    return err;
}

WEAVE_ERROR
LocaleCapabilitiesTraitDataSink::GetLeafData(PropertyPathHandle aLeafHandle, uint64_t aTagToWrite, TLVWriter &aWriter)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (LocaleCapabilitiesTrait::kPropertyHandle_available_locales == aLeafHandle) {
        nl::Weave::TLV::TLVType OuterContainerType;

        err = aWriter.StartContainer(aTagToWrite, nl::Weave::TLV::kTLVType_Array, OuterContainerType);
        SuccessOrExit(err);

        for (uint8_t i = 0; i < mNumLocales; ++i) {
            err = aWriter.PutString(nl::Weave::TLV::AnonymousTag, mLocales[i]);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  locale[%u] = [%s]", i, mLocales[i]);
        }

        err = aWriter.EndContainer(OuterContainerType);
        SuccessOrExit(err);
    }
    else {
        WeaveLogDetail(DataManagement, ">>  UNKNOWN!");
        ExitNow(err = WEAVE_ERROR_INVALID_TLV_TAG);
    }

    exit:
    WeaveLogFunctError(err);

    return err;
}

WEAVE_ERROR LocaleCapabilitiesTraitDataSink::GetNextDictionaryItemKey(PropertyPathHandle aDictionaryHandle, uintptr_t &aContext, PropertyDictionaryKey &aKey)
{
    return WEAVE_END_OF_INPUT;
}

BoltLockSettingTraitDataSink::BoltLockSettingTraitDataSink()
    : MockTraitDataSink(&BoltLockSettingTrait::TraitSchema)
{
    mAutoRelockOn = false;
    mAutoRelockDuration = 0;
}

WEAVE_ERROR
BoltLockSettingTraitDataSink::SetLeafData(PropertyPathHandle aLeafHandle, TLVReader &aReader)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    switch (aLeafHandle) {
        case BoltLockSettingTrait::kPropertyHandle_auto_relock_on:
            bool next_autoRelockOn;
            err = aReader.Get(next_autoRelockOn);
            SuccessOrExit(err);
            if (next_autoRelockOn != mAutoRelockOn)
            {
                WeaveLogDetail(DataManagement, "<<  auto_relock_on is changed from %s to %s", mAutoRelockOn ? "true" : "false", next_autoRelockOn ? "true" : "false");
                mAutoRelockOn = next_autoRelockOn;
            }

            WeaveLogDetail(DataManagement, "<<  auto_relock_on = %s", mAutoRelockOn ? "true" : "false");
            break;

        case BoltLockSettingTrait::kPropertyHandle_auto_relock_duration:
            uint32_t aAutoRelockDuration;
            err = aReader.Get(aAutoRelockDuration);
            SuccessOrExit(err);
            if (aAutoRelockDuration != mAutoRelockDuration)
            {
                WeaveLogDetail(DataManagement, "<<  auto_relock_duration is changed from %u to %u", mAutoRelockDuration, aAutoRelockDuration);
                mAutoRelockDuration = aAutoRelockDuration;
            }

            WeaveLogDetail(DataManagement, "<<  auto_relock_duration = %u", mAutoRelockDuration);
            break;

        default:
            WeaveLogDetail(DataManagement, "<<  UNKNOWN!");
            err = WEAVE_ERROR_TLV_TAG_NOT_FOUND;
    }

exit:
    return err;
}

WEAVE_ERROR
BoltLockSettingTraitDataSink::GetLeafData(PropertyPathHandle aLeafHandle, uint64_t aTagToWrite, TLVWriter &aWriter)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    switch (aLeafHandle) {
        case BoltLockSettingTrait::kPropertyHandle_auto_relock_on:
            err = aWriter.PutBoolean(aTagToWrite, mAutoRelockOn);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  auto_relock_on = %s", mAutoRelockOn ? "true" : "false");
            break;

        case BoltLockSettingTrait::kPropertyHandle_auto_relock_duration:
            err = aWriter.Put(aTagToWrite, mAutoRelockDuration);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  auto_relock_duration = %u", mAutoRelockDuration);
            break;

        default:
            WeaveLogDetail(DataManagement, ">>  UNKNOWN!");
            ExitNow(err = WEAVE_ERROR_TLV_TAG_NOT_FOUND);
    }

    exit:
    WeaveLogFunctError(err);
    return err;
}

WEAVE_ERROR BoltLockSettingTraitDataSink::GetNextDictionaryItemKey(PropertyPathHandle aDictionaryHandle, uintptr_t &aContext, PropertyDictionaryKey &aKey)
{
    return WEAVE_END_OF_INPUT;
}

TestATraitDataSink::TestATraitDataSink()
    : MockTraitDataSink(&TestATrait::TraitSchema)
{
    taa = TestATrait::ENUM_A_VALUE_1;
    tab = TestCommon::COMMON_ENUM_A_VALUE_1;
    tac = 0;
    tad.saA = 0;
    tad.saB = false;

    for (size_t i = 0; i < (sizeof(tae) / sizeof(tae[0])); i++) {
        tae[i] = 0;
    }
    for (size_t i = 0; i < sizeof(nullified_path) / sizeof(nullified_path[0]); i++)
    {
        nullified_path[i] = false;
    }
}

TestATraitDataSink::TestATraitDataSink(bool aAcceptsSublessNotifies)
    : MockTraitDataSink(&TestATrait::TraitSchema)
{
    TestATraitDataSink();

#if WDM_ENABLE_SUBSCRIPTIONLESS_NOTIFICATION
   SetAcceptsSubscriptionlessNotifications(aAcceptsSublessNotifies);
#endif // WDM_ENABLE_SUBSCRIPTIONLESS_NOTIFICATION
}

WEAVE_ERROR
TestATraitDataSink::OnEvent(uint16_t aType, void *aInParam)
{
    InEventParam *inParam = static_cast<InEventParam *>(aInParam);
    PropertyPathHandle handle;
    PropertyDictionaryKey key;

    switch (aType) {
        case kEventDictionaryReplaceBegin:
        case kEventDictionaryReplaceEnd:
        case kEventDictionaryItemModifyBegin:
        case kEventDictionaryItemModifyEnd:
            WeaveLogDetail(DataManagement, "TestATraitDataSink::OnEvent event: %u (handle: %08x)", aType, *(PropertyPathHandle *)aInParam);
            break;

        default:
            WeaveLogDetail(DataManagement, "TestATraitDataSink::OnEvent event: %u", aType);
    }

    switch (aType) {
        case kEventDictionaryReplaceBegin:
            handle = inParam->mDictionaryItemModifyBegin.mTargetHandle;

            if (handle == TestATrait::kPropertyHandle_TaI) {
                WeaveLogDetail(DataManagement, "Clearing out dictionary tai...");
                tai_map.clear();
            }
            else if (handle == TestATrait::kPropertyHandle_TaJ) {
                WeaveLogDetail(DataManagement, "Clearing out dictionary taj...");
                taj_map.clear();
            }
            else {
                WeaveLogDetail(DataManagement, "Unknown dictionary!");
            }

            break;

        case kEventDictionaryItemDelete:
            handle = inParam->mDictionaryItemDelete.mTargetHandle;
            key = GetPropertyDictionaryKey(handle);

            if (GetPropertySchemaHandle(handle) == TestATrait::kPropertyHandle_TaI_Value) {
                WeaveLogDetail(DataManagement, "Deleting key %u from tai...", key);
                tai_map.erase(key);
            }
            else if (GetPropertySchemaHandle(handle) == TestATrait::kPropertyHandle_TaJ_Value) {
                WeaveLogDetail(DataManagement, "Deleting key %u from taj...", key);
                taj_map.erase(key);
            }

            break;

        case kEventDictionaryItemModifyBegin:
            handle = inParam->mDictionaryItemModifyBegin.mTargetHandle;
            key = GetPropertyDictionaryKey(handle);

            if (GetPropertySchemaHandle(handle) == TestATrait::kPropertyHandle_TaI_Value) {
                if (tai_map.find(key) == tai_map.end()) {
                    WeaveLogDetail(DataManagement, "Staging new key %u for tai...", key);
                    memset(&tai_stageditem, 0, sizeof(tai_stageditem));
                }
                else {
                    WeaveLogDetail(DataManagement, "Modifying key %u in tai...", key);
                    tai_stageditem = tai_map[key];
                }
            }
            else if (GetPropertySchemaHandle(handle) == TestATrait::kPropertyHandle_TaJ_Value) {
                if (taj_map.find(key) == taj_map.end()) {
                    WeaveLogDetail(DataManagement, "Staging new key %u for taj...", key);
                    memset(&taj_stageditem, 0, sizeof(taj_stageditem));
                }
                else {
                    WeaveLogDetail(DataManagement, "Modifying key %u in taj...", key);
                    taj_stageditem = taj_map[key];
                }
            }

            break;

        case kEventDictionaryItemModifyEnd:
            handle = inParam->mDictionaryItemModifyEnd.mTargetHandle;
            key = GetPropertyDictionaryKey(handle);

            if (GetPropertySchemaHandle(handle) == TestATrait::kPropertyHandle_TaI_Value) {
                if (tai_map.find(key) == tai_map.end()) {
                    WeaveLogDetail(DataManagement, "Adding key %u to tai...", key);
                }

                tai_map[key] = tai_stageditem;
            }
            else if (GetPropertySchemaHandle(handle) == TestATrait::kPropertyHandle_TaJ_Value) {
                if (taj_map.find(key) == taj_map.end()) {
                    WeaveLogDetail(DataManagement, "Adding key %u to taj...", key);
                }

                taj_map[key] = taj_stageditem;
            }

            break;
    }

    return WEAVE_NO_ERROR;
}

void TestATraitDataSink::SetNullifiedPath(PropertyPathHandle aHandle, bool isNull)
{
    if (aHandle <= TestATrait::kPropertyHandle_TaJ_Value_SaB)
    {
        if (nullified_path[aHandle - TraitSchemaEngine::kHandleTableOffset] != isNull)
        {
            nullified_path[aHandle - TraitSchemaEngine::kHandleTableOffset] = isNull;
        }
    }
}


WEAVE_ERROR
TestATraitDataSink::SetData(PropertyPathHandle aHandle, TLVReader &aReader, bool aIsNull)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (aIsNull && !mSchemaEngine->IsNullable(aHandle))
    {
        WeaveLogDetail(DataManagement, "<< Non-nullable handle %d received a NULL", aHandle);

#if TDM_DISABLE_STRICT_SCHEMA_COMPLIANCE == 0
        ExitNow(err = WEAVE_ERROR_INVALID_TLV_ELEMENT);
#endif
    }

    SetNullifiedPath(aHandle, aIsNull);

    if ((!aIsNull) && (mSchemaEngine->IsLeaf(aHandle)))
    {
        err = SetLeafData(aHandle, aReader);
        // set the parent handles to non-null
        while (aHandle != kRootPropertyPathHandle)
        {
            SetNullifiedPath(aHandle, aIsNull);
            aHandle = mSchemaEngine->GetParent(aHandle);
        }
    }

#if TDM_DISABLE_STRICT_SCHEMA_COMPLIANCE == 0
exit:
#endif
    return err;
}

WEAVE_ERROR
TestATraitDataSink::SetLeafData(PropertyPathHandle aLeafHandle, TLVReader &aReader)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    switch (GetPropertySchemaHandle(aLeafHandle)) {
        case TestATrait::kPropertyHandle_TaA:
            err = aReader.Get(taa);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, "<<  ta_a = %u", taa);
            break;

        case TestATrait::kPropertyHandle_TaB:
            err = aReader.Get(tab);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, "<<  ta_b = %u", tab);
            break;

        case TestATrait::kPropertyHandle_TaC:
            uint32_t next_tac;
            err = aReader.Get(next_tac);
            SuccessOrExit(err);
            if (next_tac != tac)
            {
                WeaveLogDetail(DataManagement, "<<  ta_c is changed from %u to %u", tac, next_tac);
                tac = next_tac;
            }

            WeaveLogDetail(DataManagement, "<<  ta_c = %u", tac);
            break;

        case TestATrait::kPropertyHandle_TaD_SaA:
            uint32_t next_tad_saa;
            err = aReader.Get(next_tad_saa);
            SuccessOrExit(err);
            if (next_tad_saa != tad.saA)
            {
                WeaveLogDetail(DataManagement, "<<  ta_d.sa_a is changed from %u to %u", tad.saA, next_tad_saa);
                tad.saA = next_tad_saa;
            }

            WeaveLogDetail(DataManagement, "<<  ta_d.sa_a = %u", tad.saA);
            break;

        case TestATrait::kPropertyHandle_TaD_SaB:
            bool next_tad_sab;
            err = aReader.Get(next_tad_sab);
            SuccessOrExit(err);
            if (next_tad_sab != tad.saB)
            {
                WeaveLogDetail(DataManagement, "<<  ta_d.sa_b is changed from %u to %u", tad.saB, next_tad_sab);
                tad.saB = next_tad_sab;
            }

            WeaveLogDetail(DataManagement, "<<  ta_d.sa_b = %u", tad.saB);
            break;

        case TestATrait::kPropertyHandle_TaE:
        {
            TLVType outerType;
            uint32_t i = 0;

            err = aReader.EnterContainer(outerType);
            SuccessOrExit(err);

            while (((err = aReader.Next()) == WEAVE_NO_ERROR) && (i < (sizeof(tae) / sizeof(tae[0])))) {
                uint32_t next_tae;
                err = aReader.Get(next_tae);
                SuccessOrExit(err);
                if (tae[i] != next_tae)
                {
                    WeaveLogDetail(DataManagement, "<<  ta_e[%u] is changed from %u to %u", i, tae[i], next_tae);
                    tae[i] = next_tae;
                }

                WeaveLogDetail(DataManagement, "<<  ta_e[%u] = %u", i, tae[i]);
                i++;
            }

            err = aReader.ExitContainer(outerType);
            break;
        }

        case TestATrait::kPropertyHandle_TaG:
        {
            if (aReader.GetType() == kTLVType_UTF8String)
            {
                err = aReader.GetString(tag_string, sizeof(tag_string));
                SuccessOrExit(err);

                tag_use_ref = false;

                WeaveLogDetail(DataManagement, "<<  ta_g string = %s", tag_string);
            }
            else
            {
                err = aReader.Get(tag_ref);
                SuccessOrExit(err);

                tag_use_ref = true;

                WeaveLogDetail(DataManagement, "<<  ta_g ref = %u", tag_ref);
            }
        }
            break;

        case TestATrait::kPropertyHandle_TaK:
            err = aReader.GetBytes(&tak[0], sizeof(tak));
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, "<<  ta_k %d bytes", sizeof(tak));
            break;

        case TestATrait::kPropertyHandle_TaL:
            err = aReader.Get(tal);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, "<<  ta_l = %x", tal);
            break;

        case TestATrait::kPropertyHandle_TaM:
            err = aReader.Get(tam_resourceid);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, "<<  ta_m = %" PRIx64 , tam_resourceid);
            break;

        case TestATrait::kPropertyHandle_TaN:
            err = aReader.GetBytes(&tan[0], sizeof(tan));
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, "<<  ta_n %d bytes", sizeof(tan));
            DumpMemory(&tan[0], sizeof(tan), "WEAVE:DMG: <<  ta_n ", 16);
            break;

        case TestATrait::kPropertyHandle_TaO:
            err = aReader.Get(tao);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, "<<  ta_o = %d", tao);
            break;

        case TestATrait::kPropertyHandle_TaP:
            int64_t next_tap;
            err = aReader.Get(next_tap);
            SuccessOrExit(err);

            if (next_tap != tap)
            {
                WeaveLogDetail(DataManagement, "<<  ta_p is changed from %d to %d", tap, next_tap);
                tap = next_tap;
            }
            WeaveLogDetail(DataManagement, "<<  ta_p = %d", tap);
            break;

        case TestATrait::kPropertyHandle_TaQ:
            err = aReader.Get(taq);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, "<<  ta_q %" PRId64 , taq);
            break;

        case TestATrait::kPropertyHandle_TaR:
            err = aReader.Get(tar);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, "<<  ta_r %u", tar);
            break;

        case TestATrait::kPropertyHandle_TaS:
            err = aReader.Get(tas);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, "<<  ta_s %u", tas);
            break;

        case TestATrait::kPropertyHandle_TaT:
            err = aReader.Get(tat);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, "<<  ta_t %u", tat);
            break;

        case TestATrait::kPropertyHandle_TaU:
            err = aReader.Get(tau);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, "<<  ta_u %d", tau);
            break;

        case TestATrait::kPropertyHandle_TaV:
            err = aReader.Get(tav);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, "<<  ta_v %u", tav);
            break;

        case TestATrait::kPropertyHandle_TaW:
            err = aReader.GetString(&taw[0], sizeof(taw));
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, "<<  ta_w %s", taw);
            break;

        case TestATrait::kPropertyHandle_TaX:
            err = aReader.Get(tax);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, "<<  ta_x %d", tax);
            break;

        case TestATrait::kPropertyHandle_TaI_Value:
            err = aReader.Get(tai_stageditem);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, "<<  tai[%u] = %u", GetPropertyDictionaryKey(aLeafHandle), tai_stageditem);
            break;

        case TestATrait::kPropertyHandle_TaJ_Value_SaA:
            err = aReader.Get(taj_stageditem.saA);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, "<<  taj[%u].sa_a = %u", GetPropertyDictionaryKey(aLeafHandle), taj_stageditem.saA);
            break;

        case TestATrait::kPropertyHandle_TaJ_Value_SaB:
            err = aReader.Get(taj_stageditem.saB);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, "<<  taj[%u].sa_b = %u", GetPropertyDictionaryKey(aLeafHandle), taj_stageditem.saB);
            break;

        default:
            WeaveLogDetail(DataManagement, "<<  UNKNOWN!");
    }

exit:
    return err;
}

template <typename T>
WEAVE_ERROR GetNextDictionaryItemKeyHelper(std::map<uint16_t, T> *aMap, typename std::map<uint16_t, T>::iterator &it, uintptr_t &aContext, PropertyDictionaryKey &aKey)
{
    if (aContext == 0) {
        it = aMap->begin();
    }
    else {
        it++;
    }

    aContext = (uintptr_t)&it;
    if (it == aMap->end()) {
        return WEAVE_END_OF_INPUT;
    }
    else {
        aKey = it->first;
    }

    return WEAVE_NO_ERROR;
}

WEAVE_ERROR TestATraitDataSink::GetNextDictionaryItemKey(PropertyPathHandle aDictionaryHandle, uintptr_t &aContext, PropertyDictionaryKey &aKey)
{
    static std::map<uint16_t, uint32_t>::iterator tai_iter;
    static std::map<uint16_t, TestATrait::StructA>::iterator taj_iter;

    if (aDictionaryHandle == TestATrait::kPropertyHandle_TaI) {
        return GetNextDictionaryItemKeyHelper(&tai_map, tai_iter, aContext, aKey);
    }
    else if (aDictionaryHandle == TestATrait::kPropertyHandle_TaJ) {
        return GetNextDictionaryItemKeyHelper(&taj_map, taj_iter, aContext, aKey);
    }

    return WEAVE_ERROR_INVALID_ARGUMENT;
}

WEAVE_ERROR TestATraitDataSink::GetData(PropertyPathHandle aHandle,
                                          uint64_t aTagToWrite,
                                          TLVWriter &aWriter,
                                          bool &aIsNull,
                                          bool &aIsPresent)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (mSchemaEngine->IsNullable(aHandle))
    {
        aIsNull = nullified_path[GetPropertySchemaHandle(aHandle) - TraitSchemaEngine::kHandleTableOffset];
    }
    else
    {
        aIsNull = false;
    }

    aIsPresent = true;

    // TDM will handle writing null
    if ((!aIsNull) && (aIsPresent) && (mSchemaEngine->IsLeaf(aHandle)))
    {
        err = GetLeafData(aHandle, aTagToWrite, aWriter);
    }

    return err;
}



WEAVE_ERROR TestATraitDataSink::GetLeafData(PropertyPathHandle aLeafHandle, uint64_t aTagToWrite, TLVWriter &aWriter)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    switch (GetPropertySchemaHandle(aLeafHandle)) {
        case TestATrait::kPropertyHandle_TaA:
            err = aWriter.Put(aTagToWrite, taa);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  ta_a = %u", taa);
            break;

        case TestATrait::kPropertyHandle_TaB:
            err = aWriter.Put(aTagToWrite, tab);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  ta_b = %u", tab);
            break;

        case TestATrait::kPropertyHandle_TaC:
            err = aWriter.Put(aTagToWrite, tac);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  ta_c = %u", tac);
            break;

        case TestATrait::kPropertyHandle_TaD_SaA:
            err = aWriter.Put(aTagToWrite, tad.saA);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  ta_d.sa_a = %u", tad.saA);
            break;

        case TestATrait::kPropertyHandle_TaD_SaB:
            err = aWriter.PutBoolean(aTagToWrite, tad.saB);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  ta_d.sa_b = %s", tad.saB ? "true" : "false");
            break;

        case TestATrait::kPropertyHandle_TaE:
        {
            TLVType outerType;

            err = aWriter.StartContainer(aTagToWrite, kTLVType_Array, outerType);
            SuccessOrExit(err);

            for (size_t i = 0; i < (sizeof(tae) / sizeof(tae[0])); i++) {
                err = aWriter.Put(AnonymousTag, tae[i]);
                SuccessOrExit(err);

                WeaveLogDetail(DataManagement, ">>  ta_e[%u] = %u", i, tae[i]);
            }

            err = aWriter.EndContainer(outerType);
            break;
        }

        case TestATrait::kPropertyHandle_TaG:
        {
            if (tag_use_ref)
            {
                err = aWriter.Put(aTagToWrite, tag_ref);
                SuccessOrExit(err);

                WeaveLogDetail(DataManagement, ">>  ta_g ref = %u", tag_ref);
            }
            else
            {
                err = aWriter.PutString(aTagToWrite, tag_string);
                SuccessOrExit(err);

                WeaveLogDetail(DataManagement, ">>  ta_g string = %s", tag_string);
            }
        }
            break;

        case TestATrait::kPropertyHandle_TaH:
            // maybe TODO
            break;

        case TestATrait::kPropertyHandle_TaK:
            err = aWriter.PutBytes(aTagToWrite, tak, sizeof(tak));
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  ta_k %d bytes", sizeof(tak));
            break;

        case TestATrait::kPropertyHandle_TaL:
            err = aWriter.Put(aTagToWrite, tal);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  ta_l = %x", tal);
            break;

        case TestATrait::kPropertyHandle_TaM:
            err = aWriter.Put(aTagToWrite, tam_resourceid);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  ta_m = %" PRIx64 , tam_resourceid);
            break;

        case TestATrait::kPropertyHandle_TaN:
            err = aWriter.PutBytes(aTagToWrite, tan, sizeof(tan));
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  ta_n %d bytes", sizeof(tan));
            DumpMemory(&tan[0], sizeof(tan), "WEAVE:DMG: >>  ta_n ", 16);
            break;

        case TestATrait::kPropertyHandle_TaO:
            err = aWriter.Put(aTagToWrite, tao);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  ta_o %u", tao);
            break;

        case TestATrait::kPropertyHandle_TaP:
            err = aWriter.Put(aTagToWrite, tap);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  ta_p %" PRId64 , tap);
            break;

        case TestATrait::kPropertyHandle_TaQ:
            err = aWriter.Put(aTagToWrite, taq);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  ta_q %" PRId64 , taq);
            break;

        case TestATrait::kPropertyHandle_TaR:
            err = aWriter.Put(aTagToWrite, tar);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  ta_r %u", tar);
            break;

        case TestATrait::kPropertyHandle_TaS:
            err = aWriter.Put(aTagToWrite, tas);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  ta_s %u", tas);
            break;

        case TestATrait::kPropertyHandle_TaT:
            err = aWriter.Put(aTagToWrite, tat);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  ta_t %u", tat);
            break;

        case TestATrait::kPropertyHandle_TaU:
            err = aWriter.Put(aTagToWrite, tau);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  ta_u %d", tau);
            break;

        case TestATrait::kPropertyHandle_TaV:
            err = aWriter.PutBoolean(aTagToWrite, tav);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  ta_v %u", tav);
            break;

        case TestATrait::kPropertyHandle_TaW:
            err = aWriter.PutString(aTagToWrite, taw);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  ta_w %s", taw);
            break;

        case TestATrait::kPropertyHandle_TaX:
            err = aWriter.Put(aTagToWrite, tax);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  ta_x %d", tax);
            break;

        case TestATrait::kPropertyHandle_TaI_Value:
        {
            PropertyDictionaryKey key = GetPropertyDictionaryKey(aLeafHandle);

            err = aWriter.Put(aTagToWrite, tai_map[key]);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  ta_i[%u] = %u", key, tai_map[key]);
            break;
        }

        case TestATrait::kPropertyHandle_TaJ_Value_SaA:
        {
            PropertyDictionaryKey key = GetPropertyDictionaryKey(aLeafHandle);

            err = aWriter.Put(aTagToWrite, taj_map[key].saA);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  ta_j[%u].sa_a = %u", key, taj_map[key].saA);
            break;
        }


        case TestATrait::kPropertyHandle_TaJ_Value_SaB:
        {
            PropertyDictionaryKey key = GetPropertyDictionaryKey(aLeafHandle);

            err = aWriter.PutBoolean(aTagToWrite, taj_map[key].saB);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  ta_j[%u].sa_b = %u", key, taj_map[key].saB);
            break;
        }

        default:
            WeaveLogDetail(DataManagement, ">>  TestATrait UNKNOWN! %08x", aLeafHandle);
    }

exit:
    WeaveLogFunctError(err);
    return err;
}

WEAVE_ERROR TestBTraitDataSink::GetData(PropertyPathHandle aHandle,
                                          uint64_t aTagToWrite,
                                          TLVWriter &aWriter,
                                          bool &aIsNull,
                                          bool &aIsPresent)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (mSchemaEngine->IsNullable(aHandle))
    {
        aIsNull = nullified_path[GetPropertySchemaHandle(aHandle) - TraitSchemaEngine::kHandleTableOffset];
    }
    else
    {
        aIsNull = false;
    }

    aIsPresent = true;

    // TDM will handle writing null
    if ((!aIsNull) && (aIsPresent) && (mSchemaEngine->IsLeaf(aHandle)))
    {
        err = GetLeafData(aHandle, aTagToWrite, aWriter);
    }

    return err;
}

WEAVE_ERROR TestBTraitDataSink::GetLeafData(PropertyPathHandle aLeafHandle, uint64_t aTagToWrite, TLVWriter &aWriter)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    switch (aLeafHandle) {
        //
        // TestATrait
        //

        case TestBTrait::kPropertyHandle_TaA:
            err = aWriter.Put(aTagToWrite, taa);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  ta_a = %u", taa);
            break;

        case TestBTrait::kPropertyHandle_TaB:
            err = aWriter.Put(aTagToWrite, tab);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  ta_b = %u", tab);
            break;

        case TestBTrait::kPropertyHandle_TaC:
            err = aWriter.Put(aTagToWrite, tac);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  ta_c = %u", tac);
            break;

        case TestBTrait::kPropertyHandle_TaD_SaA:
            err = aWriter.Put(aTagToWrite, tad_saa);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  ta_d.sa_a = %u", tad_saa);
            break;

        case TestBTrait::kPropertyHandle_TaD_SaB:
            err = aWriter.PutBoolean(aTagToWrite, tad_sab);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  ta_d.sa_b = %s", tad_sab ? "true" : "false");
            break;

        case TestBTrait::kPropertyHandle_TaP:
            err = aWriter.Put(aTagToWrite, tap);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  ta_p = %d", tap);
            break;

        case TestBTrait::kPropertyHandle_TaE:
        {
            TLVType outerType;

            err = aWriter.StartContainer(aTagToWrite, kTLVType_Array, outerType);
            SuccessOrExit(err);

            for (size_t i = 0; i < (sizeof(tae) / sizeof(tae[0])); i++) {
                err = aWriter.Put(AnonymousTag, tae[i]);
                SuccessOrExit(err);

                WeaveLogDetail(DataManagement, ">>  ta_e[%u] = %u", i, tae[i]);
            }

            err = aWriter.EndContainer(outerType);
            break;
        }

            //
            // TestBTrait
            //

        case TestBTrait::kPropertyHandle_TbA:
            err = aWriter.Put(aTagToWrite, tba);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  tb_a = %u", tba);
            break;

        case TestBTrait::kPropertyHandle_TbB_SbA:
            err = aWriter.PutString(aTagToWrite, tbb_sba);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  tb_b.sb_a = \"%s\"", tbb_sba);
            break;

        case TestBTrait::kPropertyHandle_TbB_SbB:
            err = aWriter.Put(aTagToWrite, tbb_sbb);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  tb_b.sb_b = %u", tbb_sbb);
            break;

        case TestBTrait::kPropertyHandle_TbC_SaA:
            err = aWriter.Put(aTagToWrite, tbc_saa);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  tb_c.sa_a = %u", tbc_saa);
            break;

        case TestBTrait::kPropertyHandle_TbC_SaB:
            err = aWriter.PutBoolean(aTagToWrite, tbc_sab);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  tb_c.sa_b = %s", tbc_sab ? "true" : "false");
            break;

        case TestBTrait::kPropertyHandle_TbC_SeaC:
            err = aWriter.PutString(aTagToWrite, tbc_seac);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  tb_c.sea_c = %s", tbc_seac);
            break;

        default:
            WeaveLogDetail(DataManagement, ">>  UNKNOWN!");
    }

    exit:
    WeaveLogFunctError(err);
    return err;
}

WEAVE_ERROR TestBTraitDataSink::GetNextDictionaryItemKey(PropertyPathHandle aDictionaryHandle, uintptr_t &aContext, PropertyDictionaryKey &aKey)
{
    return WEAVE_END_OF_INPUT;
}

TestBTraitDataSink::TestBTraitDataSink()
        : MockTraitDataSink(&TestBTrait::TraitSchema)
{
    taa = TestATrait::ENUM_A_VALUE_1;
    tab = TestCommon::COMMON_ENUM_A_VALUE_1;
    tac = 0;
    tad_saa = 0;
    tad_sab = false;

    for (size_t i = 0; i < (sizeof(tae) / sizeof(tae[0])); i++) {
        tae[i] = 0;
    }

    strcpy(tai, "");

    tba = 0;
    strcpy(tbb_sba, "");
    tbb_sbb = 0;

    tbc_saa = 0;
    tbc_sab = false;
    strcpy(tbc_seac, "");

    for (size_t i = 0; i < sizeof(nullified_path) / sizeof(nullified_path[0]); i++)
    {
        nullified_path[i] = false;
    }
}

void TestBTraitDataSink::SetNullifiedPath(PropertyPathHandle aHandle, bool isNull)
{
    if (aHandle <= TestBTrait::kPropertyHandle_TaJ_Value_SaB)
    {
        if (nullified_path[aHandle - TraitSchemaEngine::kHandleTableOffset] != isNull)
        {
            nullified_path[aHandle - TraitSchemaEngine::kHandleTableOffset] = isNull;
        }
    }
}


WEAVE_ERROR
TestBTraitDataSink::SetData(PropertyPathHandle aHandle, TLVReader &aReader, bool aIsNull)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (aIsNull && !mSchemaEngine->IsNullable(aHandle))
    {
        WeaveLogDetail(DataManagement, "<< Non-nullable handle %d received a NULL", aHandle);

#if TDM_DISABLE_STRICT_SCHEMA_COMPLIANCE == 0
        ExitNow(err = WEAVE_ERROR_INVALID_TLV_ELEMENT);
#endif
    }

    SetNullifiedPath(aHandle, aIsNull);

    if ((!aIsNull) && (mSchemaEngine->IsLeaf(aHandle)))
    {
        err = SetLeafData(aHandle, aReader);
        // set the parent handles to non-null
        while (aHandle != kRootPropertyPathHandle)
        {
            SetNullifiedPath(aHandle, aIsNull);
            aHandle = mSchemaEngine->GetParent(aHandle);
        }
    }

    return err;
}

WEAVE_ERROR
TestBTraitDataSink::SetLeafData(PropertyPathHandle aLeafHandle, TLVReader &aReader)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    switch (aLeafHandle) {
        //
        // TestATrait
        //

        case TestBTrait::kPropertyHandle_TaA:
            int32_t next_taa;
            err = aReader.Get(next_taa);
            SuccessOrExit(err);
            if (next_taa != taa)
            {
                WeaveLogDetail(DataManagement, "<<  ta_a is changed from %u to %u", taa, next_taa);
                taa = next_taa;
            }

            WeaveLogDetail(DataManagement, "<<  ta_a = %u", taa);
            break;

        case TestBTrait::kPropertyHandle_TaB:
            int32_t next_tab;
            err = aReader.Get(next_tab);
            SuccessOrExit(err);
            if (next_tab != tab)
            {
                WeaveLogDetail(DataManagement, "<<  ta_b is changed from %u to %u", tab, next_tab);
                tab = next_tab;
            }

            WeaveLogDetail(DataManagement, "<<  ta_b = %u", tab);
            break;

        case TestBTrait::kPropertyHandle_TaC:
            uint32_t next_tac;
            err = aReader.Get(next_tac);
            SuccessOrExit(err);
            if (next_tac != tac)
            {
                WeaveLogDetail(DataManagement, "<<  ta_c is changed from %u to %u", tac, next_tac);
                tac = next_tac;
            }

            WeaveLogDetail(DataManagement, "<<  ta_c = %u", tac);
            break;

        case TestBTrait::kPropertyHandle_TaD_SaA:
            uint32_t next_tad_saa;
            err = aReader.Get(next_tad_saa);
            SuccessOrExit(err);
            if (next_tad_saa != tad_saa)
            {
                WeaveLogDetail(DataManagement, "<<  ta_d.sa_a is changed from %u to %u", tad_saa, next_tad_saa);
                tad_saa = next_tad_saa;
            }

            WeaveLogDetail(DataManagement, "<<  ta_d.sa_a = %u", tad_saa);
            break;

        case TestBTrait::kPropertyHandle_TaD_SaB:
            bool next_tad_sab;
            err = aReader.Get(next_tad_sab);
            SuccessOrExit(err);
            if (next_tad_sab != tad_sab)
            {
                WeaveLogDetail(DataManagement, "<<  ta_d.sa_b is changed from %u to %u", tad_sab, next_tad_sab);
                tad_sab = next_tad_sab;
            }

            WeaveLogDetail(DataManagement, "<<  ta_d.sa_b = %u", tad_sab);
            break;

        case TestBTrait::kPropertyHandle_TaE:
        {
            TLVType outerType;
            uint32_t i = 0;

            err = aReader.EnterContainer(outerType);
            SuccessOrExit(err);

            while (((err = aReader.Next()) == WEAVE_NO_ERROR) && (i < (sizeof(tae) / sizeof(tae[0])))) {
                uint32_t next_tae;
                err = aReader.Get(next_tae);
                SuccessOrExit(err);
                if (tae[i] != next_tae)
                {
                    WeaveLogDetail(DataManagement, "<<  ta_e[%u] is changed from %u to %u", i, tae[i], next_tae);
                    tae[i] = next_tae;
                }

                WeaveLogDetail(DataManagement, "<<  ta_e[%u] = %u", i, tae[i]);
                i++;
            }

            err = aReader.ExitContainer(outerType);
            break;
        }

        case TestBTrait::kPropertyHandle_TaP:
            err = aReader.Get(tap);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, "<<  ta_p = %" PRId64, tap);
            break;

            //
            // TestBTrait
            //

        case TestBTrait::kPropertyHandle_TbA:
            uint32_t next_tba;
            err = aReader.Get(next_tba);
            SuccessOrExit(err);
            if (tba != next_tba)
            {
                WeaveLogDetail(DataManagement, "<<  tb_a is changed from %u to %u", tba, next_tba);
                tba = next_tba;
            }

            WeaveLogDetail(DataManagement, "<<  tb_a = %u", tba);
            break;

        case TestBTrait::kPropertyHandle_TbB_SbA:
            char next_tbb_sba[MAX_ARRAY_LEN];
            err = aReader.GetString(next_tbb_sba, MAX_ARRAY_SIZE);
            SuccessOrExit(err);
            if (strncmp(tbb_sba, next_tbb_sba, MAX_ARRAY_SIZE))
            {
                WeaveLogDetail(DataManagement, "<<  tb_b.sb_a is changed from %s to %s", tbb_sba, next_tbb_sba);
                memcpy(tbb_sba, next_tbb_sba, MAX_ARRAY_SIZE);
            }

            WeaveLogDetail(DataManagement, "<<  tb_b.sb_a = %s", tbb_sba);
            break;

        case TestBTrait::kPropertyHandle_TbB_SbB:
            uint32_t next_tbb_sbb;
            err = aReader.Get(next_tbb_sbb);
            SuccessOrExit(err);
            if (tbb_sbb != next_tbb_sbb)
            {
                WeaveLogDetail(DataManagement, "<<  tb_b.sb_b is changed from %u to %u", tbb_sbb, next_tbb_sbb);
                tbb_sbb = next_tbb_sbb;
            }

            WeaveLogDetail(DataManagement, "<<  tb_b.sb_b = %u", tbb_sbb);
            break;

        case TestBTrait::kPropertyHandle_TbC_SaA:
            uint32_t next_tbc_saa;
            err = aReader.Get(next_tbc_saa);
            SuccessOrExit(err);
            if (tbc_saa != next_tbc_saa)
            {
                WeaveLogDetail(DataManagement, "<<  tb_c.sa_a is changed from %u to %u", tbc_saa, next_tbc_saa);
                tbc_saa = next_tbc_saa;
            }

            WeaveLogDetail(DataManagement, "<<  tb_c.sa_a = %u", tbc_saa);
            break;

        case TestBTrait::kPropertyHandle_TbC_SaB:
            bool next_tbc_sab;
            err = aReader.Get(next_tbc_sab);
            SuccessOrExit(err);
            if (tbc_sab != next_tbc_sab)
            {
                WeaveLogDetail(DataManagement, "<<  tb_c.sa_b is changed from %u to %u", tbc_sab, next_tbc_sab);
                tbc_sab = next_tbc_sab;
            }

            WeaveLogDetail(DataManagement, "<<  tb_c.sa_b = %u", tbc_sab);
            break;

        case TestBTrait::kPropertyHandle_TbC_SeaC:
            char next_tbc_seac[MAX_ARRAY_LEN];
            err = aReader.GetString(next_tbc_seac, MAX_ARRAY_SIZE);
            SuccessOrExit(err);
            if (strncmp(tbc_seac, next_tbc_seac, MAX_ARRAY_SIZE))
            {
                WeaveLogDetail(DataManagement, "<<  tb_c.sea_c is changed from \"%s\" to \"%s\"", tbc_seac, next_tbc_seac);
                memcpy(tbc_seac, next_tbc_seac, MAX_ARRAY_SIZE);
            }

            WeaveLogDetail(DataManagement, "<<  tb_c.sea_c = \"%s\"", tbc_seac);
            break;

        default:
            WeaveLogDetail(DataManagement, "<<  TestBTrait UNKNOWN! %08x", aLeafHandle);
    }

    exit:
    return err;
}

TestApplicationKeysTraitDataSink::TestApplicationKeysTraitDataSink(void)
{
}

enum
{
#if WEAVE_CONFIG_MAX_APPLICATION_EPOCH_KEYS > WEAVE_CONFIG_MAX_APPLICATION_GROUPS
    kMaxGroupKeysOfATypeCount                        = WEAVE_CONFIG_MAX_APPLICATION_EPOCH_KEYS,
#else
    kMaxGroupKeysOfATypeCount                        = WEAVE_CONFIG_MAX_APPLICATION_GROUPS,
#endif
};

WEAVE_ERROR TestApplicationKeysTraitDataSink::GetLeafData(PropertyPathHandle aLeafHandle, uint64_t aTagToWrite, TLVWriter &aWriter)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TLVType outerContainerType;
    uint32_t keyType;
    uint32_t keyIds[kMaxGroupKeysOfATypeCount];
    uint8_t keyCount;
    WeaveGroupKey groupKey;

    VerifyOrExit(GroupKeyStore != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    if (ApplicationKeysTrait::kPropertyHandle_EpochKeys == aLeafHandle)
    {
        keyType = WeaveKeyId::kType_AppEpochKey;
    }
    else if (ApplicationKeysTrait::kPropertyHandle_MasterKeys == aLeafHandle)
    {
        keyType = WeaveKeyId::kType_AppGroupMasterKey;
    }
    else
    {
        ExitNow(err = WEAVE_ERROR_INVALID_TLV_TAG);
    }

    err = aWriter.StartContainer(aTagToWrite, kTLVType_Array, outerContainerType);
    SuccessOrExit(err);

    err = GroupKeyStore->EnumerateGroupKeys(keyType, keyIds, sizeof(keyIds) / sizeof(keyIds[0]), keyCount);
    SuccessOrExit(err);

    for (int i = 0; i < keyCount; ++i)
    {
        err = GroupKeyStore->GetGroupKey(keyIds[i], groupKey);
        SuccessOrExit(err);

        TLVType containerType2;

        err = aWriter.StartContainer(AnonymousTag, kTLVType_Structure, containerType2);
        SuccessOrExit(err);

        if (keyType == WeaveKeyId::kType_AppEpochKey)
        {
            const uint32_t epochKeyNumber = WeaveKeyId::GetEpochKeyNumber(groupKey.KeyId);
            err = aWriter.Put(ContextTag(kTag_EpochKey_KeyId), epochKeyNumber);
            SuccessOrExit(err);

            err = aWriter.Put(ContextTag(kTag_EpochKey_StartTime), (static_cast<int64_t>(groupKey.StartTime) * 1000));
            SuccessOrExit(err);

            err = aWriter.PutBytes(ContextTag(kTag_EpochKey_Key), groupKey.Key, groupKey.KeyLen);
            SuccessOrExit(err);

            err = aWriter.EndContainer(containerType2);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">> GroupEpochKeyId = %08" PRIX32, groupKey.KeyId);
        }
        // If master group key.
        else
        {
            err = aWriter.Put(ContextTag(kTag_ApplicationGroup_GlobalId), groupKey.GlobalId);
            SuccessOrExit(err);

            const uint32_t appGroupLocalNumber = WeaveKeyId::GetAppGroupLocalNumber(groupKey.KeyId);
            err = aWriter.Put(ContextTag(kTag_ApplicationGroup_ShortId), appGroupLocalNumber);
            SuccessOrExit(err);

            err = aWriter.PutBytes(ContextTag(kTag_ApplicationGroup_Key), groupKey.Key, groupKey.KeyLen);
            SuccessOrExit(err);

            err = aWriter.EndContainer(containerType2);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">> GroupMasterKeyId = %08" PRIX32, groupKey.KeyId);
        }
    }

    err = aWriter.EndContainer(outerContainerType);
    SuccessOrExit(err);

exit:

    WeaveLogFunctError(err);

    return err;
}

WEAVE_ERROR TestApplicationKeysTraitDataSink::GetNextDictionaryItemKey(PropertyPathHandle aDictionaryHandle, uintptr_t &aContext, PropertyDictionaryKey &aKey)
{
    return WEAVE_END_OF_INPUT;
}

#if WEAVE_CONFIG_ENABLE_WDM_UPDATE
LocaleSettingsTraitUpdatableDataSink::LocaleSettingsTraitUpdatableDataSink()
        : MockTraitUpdatableDataSink(&LocaleSettingsTrait::TraitSchema)
{
    memset(mLocale, 0, sizeof(mLocale));
}

WEAVE_ERROR
LocaleSettingsTraitUpdatableDataSink::SetLeafData(PropertyPathHandle aLeafHandle, TLVReader &aReader)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    switch (aLeafHandle) {
        case LocaleSettingsTrait::kPropertyHandle_active_locale:
            char next_locale[kMaxNumOfCharsPerLocale];
            err = aReader.GetString(next_locale, MAX_LOCALE_SIZE);
            SuccessOrExit(err);
            if (strncmp(next_locale, mLocale, MAX_LOCALE_SIZE) != 0)
            {
                WeaveLogDetail(DataManagement, "<<  active_locale is changed from \"%s\" to \"%s\"", mLocale, next_locale);
                memcpy(mLocale, next_locale, MAX_LOCALE_SIZE);
            }

            WeaveLogDetail(DataManagement, "<<  active_locale = \"%s\"", mLocale);
            break;

        default:
            WeaveLogDetail(DataManagement, "<<  UNKNOWN!");
            err = WEAVE_ERROR_TLV_TAG_NOT_FOUND;
    }

    exit:
    return err;
}

WEAVE_ERROR
LocaleSettingsTraitUpdatableDataSink::GetLeafData(PropertyPathHandle aLeafHandle, uint64_t aTagToWrite, TLVWriter &aWriter)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    switch (aLeafHandle) {
        case LocaleSettingsTrait::kPropertyHandle_active_locale:
            err = aWriter.PutString(aTagToWrite, mLocale);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  active_locale = \"%s\"", mLocale);
            break;

        default:
            WeaveLogDetail(DataManagement, ">>  UNKNOWN!");
            ExitNow(err = WEAVE_ERROR_TLV_TAG_NOT_FOUND);
    }

    exit:
    WeaveLogFunctError(err);

    return err;
}

WEAVE_ERROR LocaleSettingsTraitUpdatableDataSink::Mutate(SubscriptionClient * apSubClient,
                                                         bool aIsConditional,
                                                         MockWdmNodeOptions::WdmUpdateMutation aMutation)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    static unsigned int whichLocale = 0;
    static const char * locales[] = { "en-US", "zh-TW", "ja-JP", "pl-PL", "zh-CN" };
    bool isLocked = false;
    PropertyPathHandle pathHandle = kNullPropertyPathHandle;

    err = Lock(apSubClient);
    SuccessOrExit(err);

    isLocked = true;

    MOCK_strlcpy(mLocale, locales[whichLocale], sizeof(mLocale));
    whichLocale = (whichLocale + 1) % (sizeof(locales)/sizeof(locales[0]));

    // This trait instance only supports the OneLeaf and Root mutations.

    switch (aMutation)
    {
    case MockWdmNodeOptions::kMutation_Root:
        pathHandle = LocaleSettingsTrait::kPropertyHandle_Root;
        break;

    case MockWdmNodeOptions::kMutation_OneLeaf:
    default:
        aMutation = MockWdmNodeOptions::kMutation_OneLeaf;
        pathHandle = LocaleSettingsTrait::kPropertyHandle_active_locale;
        break;
    }

    WeaveLogDetail(DataManagement, "<set updated> in 0x%08x", pathHandle);

    err = SetUpdated(apSubClient, pathHandle, aIsConditional);
    SuccessOrExit(err);

exit:
    WeaveLogDetail(DataManagement, "LocaleSettingsTrait mutated %s with error %d", MockWdmNodeOptions::GetMutationStrings()[aMutation], err);

    if (isLocked)
    {
        Unlock(apSubClient);
    }

    return err;
}

WEAVE_ERROR LocaleSettingsTraitUpdatableDataSink::GetNextDictionaryItemKey(PropertyPathHandle aDictionaryHandle, uintptr_t &aContext, PropertyDictionaryKey &aKey)
{
    return WEAVE_END_OF_INPUT;
}

WEAVE_ERROR TestATraitUpdatableDataSink::Mutate(SubscriptionClient * apSubClient,
                                                bool aIsConditional,
                                                MockWdmNodeOptions::WdmUpdateMutation aMutation)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    bool isLocked = false;

    err = Lock(apSubClient);
    SuccessOrExit(err);

    isLocked = true;

    WeaveLogDetail(DataManagement,
            "TestATraitUpdatableDataSink: mTraitTestSet: %" PRIu32 ", mTestCounter: %" PRIu32 "",
            mTraitTestSet, mTestCounter);

    switch (aMutation)
    {
    case MockWdmNodeOptions::kMutation_OneLeaf:
        err = SetUpdated(apSubClient, TestATrait::kPropertyHandle_TaP, aIsConditional);
        SuccessOrExit(err);

        tap++;

        break;

    case MockWdmNodeOptions::kMutation_SameLevelLeaves:
        err = SetUpdated(apSubClient, TestATrait::kPropertyHandle_TaP, aIsConditional);
        SuccessOrExit(err);

        err = SetUpdated(apSubClient, TestATrait::kPropertyHandle_TaC, aIsConditional);
        SuccessOrExit(err);

        err = SetUpdated(apSubClient, TestATrait::kPropertyHandle_TaR, aIsConditional);
        SuccessOrExit(err);

        tap++;
        tac++;
        tar++;

        break;

    case MockWdmNodeOptions::kMutation_DiffLevelLeaves:
        err = SetUpdated(apSubClient, TestATrait::kPropertyHandle_TaD_SaB, aIsConditional);
        SuccessOrExit(err);

        err = SetUpdated(apSubClient, TestATrait::kPropertyHandle_TaA, aIsConditional);
        SuccessOrExit(err);

        if (taa == TestATrait::ENUM_A_VALUE_1) {
            taa = TestATrait::ENUM_A_VALUE_2;
        }
        else {
            taa = TestATrait::ENUM_A_VALUE_1;
        }

        tad.saB = !tad.saB;

        break;

    case MockWdmNodeOptions::kMutation_WholeDictionary:
    {
        uint32_t seed = 0;

        err = SetUpdated(apSubClient, TestATrait::kPropertyHandle_TaI, aIsConditional);
        SuccessOrExit(err);

        if (tai_map[0] == seed)
        {
            // Just make sure we change the data
            seed = 100;
        }

        tai_map.clear();
        for (uint16_t i = 0; i < 10; i++) {
            tai_map[i] = { (uint32_t)i + seed };
        }

        break;
    }

    case MockWdmNodeOptions::kMutation_WholeLargeDictionary:
    {
        uint32_t seed = 0;

        err = SetUpdated(apSubClient, TestATrait::kPropertyHandle_TaI, aIsConditional);
        SuccessOrExit(err);

        if (tai_map[0] == seed)
        {
            // Just make sure we change the data
            seed = 100;
        }

        tai_map.clear();
        for (uint16_t i = 0; i < 800; i++) {
            tai_map[i] = { (uint32_t)i + 1 };
        }

        break;
    }

    case MockWdmNodeOptions::kMutation_FewDictionaryItems:
        err = SetUpdated(apSubClient, CreatePropertyPathHandle(TestATrait::kPropertyHandle_TaI_Value, 4), aIsConditional);
        SuccessOrExit(err);

        err = SetUpdated(apSubClient, CreatePropertyPathHandle(TestATrait::kPropertyHandle_TaI_Value, 5), aIsConditional);
        SuccessOrExit(err);

        err = SetUpdated(apSubClient, CreatePropertyPathHandle(TestATrait::kPropertyHandle_TaI_Value, 6), aIsConditional);
        SuccessOrExit(err);

        err = SetUpdated(apSubClient, CreatePropertyPathHandle(TestATrait::kPropertyHandle_TaI_Value, 7), aIsConditional);
        SuccessOrExit(err);

        tai_map[4] = { 4 };
        tai_map[5] = { 5 };
        tai_map[6] = { 6 };
        tai_map[7] = { 7 };

        break;

    case MockWdmNodeOptions::kMutation_ManyDictionaryItems:
        for (size_t i = 0; i < 60; i++)
        {
            err = SetUpdated(apSubClient, CreatePropertyPathHandle(TestATrait::kPropertyHandle_TaI_Value, i), aIsConditional);
            SuccessOrExit(err);

            tai_map[i] = { static_cast<uint32_t>(i) };
        }

        break;

    case MockWdmNodeOptions::kMutation_WholeDictionaryAndLeaf:
        err = SetUpdated(apSubClient, TestATrait::kPropertyHandle_TaI, aIsConditional);
        SuccessOrExit(err);

        err = SetUpdated(apSubClient, TestATrait::kPropertyHandle_TaP, aIsConditional);
        SuccessOrExit(err);

        tai_map.clear();
        for (uint16_t i = 0; i < 10; i++) {
            tai_map[i] = { static_cast<uint32_t>(i) + 1 };
        }

        tap++;

        break;

    case MockWdmNodeOptions::kMutation_OneStructure:

        err = SetUpdated(apSubClient, TestATrait::kPropertyHandle_TaD, aIsConditional);
        SuccessOrExit(err);

        tad.saA = mTestCounter;
        tad.saB = !tad.saB;

        break;

    case MockWdmNodeOptions::kMutation_OneLeafOneStructure:

        err = SetUpdated(apSubClient, TestATrait::kPropertyHandle_TaP, aIsConditional);
        SuccessOrExit(err);

        err = SetUpdated(apSubClient, TestATrait::kPropertyHandle_TaD, aIsConditional);
        SuccessOrExit(err);

        tap++;

        tad.saA = mTestCounter;
        tad.saB = !tad.saB;

        break;

    case MockWdmNodeOptions::kMutation_Root:
        {

        Schema::Nest::Test::Trait::TestATrait::StructA tmpTajItem;

        // Root and some more subpaths
        err = SetUpdated(apSubClient, TestATrait::kPropertyHandle_TaD_SaA, aIsConditional);
        SuccessOrExit(err);

        err = SetUpdated(apSubClient, TestATrait::kPropertyHandle_TaA, aIsConditional);
        SuccessOrExit(err);

        err = SetUpdated(apSubClient, TestATrait::kPropertyHandle_TaD, aIsConditional);
        SuccessOrExit(err);

        err = SetUpdated(apSubClient, TestATrait::kPropertyHandle_TaI, aIsConditional);
        SuccessOrExit(err);

        err = SetUpdated(apSubClient, TestATrait::kPropertyHandle_Root, aIsConditional);
        SuccessOrExit(err);

        err = SetUpdated(apSubClient, TestATrait::kPropertyHandle_TaD, aIsConditional);
        SuccessOrExit(err);

        tai_map.clear();
        // test_weave_wdm_next_service_update_10_cond_Root_multi_traits.py depends on
        // the number of items added the to the dictionaries here
        for (uint16_t i = 0; i < 85; i++) {
            tai_map[i] = { ((uint32_t)i + 1)*10 + 7 };
        }

        taj_map.clear();
        tmpTajItem.saB = tad.saB;
        tmpTajItem.saA = 0;
        if (tmpTajItem.saB)
        {
            tmpTajItem.saA++;
        }
        for (uint16_t i = 0; i < 10; i++) {
            tmpTajItem.saA++;
            taj_map[i] = tmpTajItem;
        }

        if (taa == TestATrait::ENUM_A_VALUE_1) {
            taa = TestATrait::ENUM_A_VALUE_2;
        }
        else {
            taa = TestATrait::ENUM_A_VALUE_1;
        }

        tad.saB = !tad.saB;

        break;
        }

    case MockWdmNodeOptions::kMutation_RootWithLargeDictionary:

        err = SetUpdated(apSubClient, TestATrait::kPropertyHandle_Root, aIsConditional);
        SuccessOrExit(err);

        tai_map.clear();
        for (uint16_t i = 0; i < 800; i++) {
            tai_map[i] = { ((uint32_t)i + 1)*10 + 3 };
        }

        tad.saB = !tad.saB;

        break;

    default:
        WeaveDie();
        break;
    }

    mTestCounter++;

exit:

    WeaveLogDetail(DataManagement, "TestATrait mutated %s with error %d", MockWdmNodeOptions::GetMutationStrings()[aMutation], err);

    if (isLocked)
    {
        Unlock(apSubClient);
    }

    return err;
}

TestATraitUpdatableDataSink::TestATraitUpdatableDataSink()
        : MockTraitUpdatableDataSink(&TestATrait::TraitSchema)
{
    taa = TestATrait::ENUM_A_VALUE_1;
    tab = TestCommon::COMMON_ENUM_A_VALUE_1;
    tac = 0;
    tad.saA = 0;
    tad.saB = false;

    for (size_t i = 0; i < (sizeof(tae) / sizeof(tae[0])); i++) {
        tae[i] = 0;
    }
    for (size_t i = 0; i < sizeof(nullified_path) / sizeof(nullified_path[0]); i++)
    {
        nullified_path[i] = false;
    }
}

WEAVE_ERROR
TestATraitUpdatableDataSink::OnEvent(uint16_t aType, void *aInParam)
{
    InEventParam *inParam = static_cast<InEventParam *>(aInParam);
    PropertyPathHandle handle;
    PropertyDictionaryKey key;

    switch (aType) {
        case kEventDictionaryReplaceBegin:
        case kEventDictionaryReplaceEnd:
        case kEventDictionaryItemModifyBegin:
        case kEventDictionaryItemModifyEnd:
            WeaveLogDetail(DataManagement, "TestATraitUpdatableDataSink::OnEvent event: %u (handle: %08x)", aType, *(PropertyPathHandle *)aInParam);
            break;

        default:
            WeaveLogDetail(DataManagement, "TestATraitUpdatableDataSink::OnEvent event: %u", aType);
    }

    switch (aType) {
        case kEventDictionaryReplaceBegin:
            handle = inParam->mDictionaryItemModifyBegin.mTargetHandle;

            if (handle == TestATrait::kPropertyHandle_TaI) {
                WeaveLogDetail(DataManagement, "Clearing out dictionary tai...");
                tai_map.clear();
            }
            else if (handle == TestATrait::kPropertyHandle_TaJ) {
                WeaveLogDetail(DataManagement, "Clearing out dictionary taj...");
                taj_map.clear();
            }
            else {
                WeaveLogDetail(DataManagement, "Unknown dictionary!");
            }

            break;

        case kEventDictionaryItemDelete:
            handle = inParam->mDictionaryItemDelete.mTargetHandle;
            key = GetPropertyDictionaryKey(handle);

            if (GetPropertySchemaHandle(handle) == TestATrait::kPropertyHandle_TaI_Value) {
                WeaveLogDetail(DataManagement, "Deleting key %u from tai...", key);
                tai_map.erase(key);
            }
            else if (GetPropertySchemaHandle(handle) == TestATrait::kPropertyHandle_TaJ_Value) {
                WeaveLogDetail(DataManagement, "Deleting key %u from taj...", key);
                taj_map.erase(key);
            }

            break;

        case kEventDictionaryItemModifyBegin:
            handle = inParam->mDictionaryItemModifyBegin.mTargetHandle;
            key = GetPropertyDictionaryKey(handle);

            if (GetPropertySchemaHandle(handle) == TestATrait::kPropertyHandle_TaI_Value) {
                if (tai_map.find(key) == tai_map.end()) {
                    WeaveLogDetail(DataManagement, "Staging new key %u for tai...", key);
                    memset(&tai_stageditem, 0, sizeof(tai_stageditem));
                }
                else {
                    WeaveLogDetail(DataManagement, "Modifying key %u in tai...", key);
                    tai_stageditem = tai_map[key];
                }
            }
            else if (GetPropertySchemaHandle(handle) == TestATrait::kPropertyHandle_TaJ_Value) {
                if (taj_map.find(key) == taj_map.end()) {
                    WeaveLogDetail(DataManagement, "Staging new key %u for taj...", key);
                    memset(&taj_stageditem, 0, sizeof(taj_stageditem));
                }
                else {
                    WeaveLogDetail(DataManagement, "Modifying key %u in taj...", key);
                    taj_stageditem = taj_map[key];
                }
            }

            break;

        case kEventDictionaryItemModifyEnd:
            handle = inParam->mDictionaryItemModifyEnd.mTargetHandle;
            key = GetPropertyDictionaryKey(handle);

            if (GetPropertySchemaHandle(handle) == TestATrait::kPropertyHandle_TaI_Value) {
                if (tai_map.find(key) == tai_map.end()) {
                    WeaveLogDetail(DataManagement, "Adding key %u to tai...", key);
                }

                tai_map[key] = tai_stageditem;
            }
            else if (GetPropertySchemaHandle(handle) == TestATrait::kPropertyHandle_TaJ_Value) {
                if (taj_map.find(key) == taj_map.end()) {
                    WeaveLogDetail(DataManagement, "Adding key %u to taj...", key);
                }

                taj_map[key] = taj_stageditem;
            }

            break;
    }

    return WEAVE_NO_ERROR;
}

void TestATraitUpdatableDataSink::SetNullifiedPath(PropertyPathHandle aHandle, bool isNull)
{
    if (aHandle <= TestATrait::kPropertyHandle_TaJ_Value_SaB)
    {
        if (nullified_path[aHandle - TraitSchemaEngine::kHandleTableOffset] != isNull)
        {
            nullified_path[aHandle - TraitSchemaEngine::kHandleTableOffset] = isNull;
        }
    }
}


WEAVE_ERROR
TestATraitUpdatableDataSink::SetData(PropertyPathHandle aHandle, TLVReader &aReader, bool aIsNull)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (aIsNull && !mSchemaEngine->IsNullable(aHandle))
    {
        WeaveLogDetail(DataManagement, "<< Non-nullable handle %d received a NULL", aHandle);

#if TDM_DISABLE_STRICT_SCHEMA_COMPLIANCE == 0
        ExitNow(err = WEAVE_ERROR_INVALID_TLV_ELEMENT);
#endif
    }

    SetNullifiedPath(aHandle, aIsNull);

    if ((!aIsNull) && (mSchemaEngine->IsLeaf(aHandle)))
    {
        err = SetLeafData(aHandle, aReader);
        // set the parent handles to non-null
        while (aHandle != kRootPropertyPathHandle)
        {
            SetNullifiedPath(aHandle, aIsNull);
            aHandle = mSchemaEngine->GetParent(aHandle);
        }
    }

#if TDM_DISABLE_STRICT_SCHEMA_COMPLIANCE == 0
exit:
#endif
    return err;
}

WEAVE_ERROR
TestATraitUpdatableDataSink::SetLeafData(PropertyPathHandle aLeafHandle, TLVReader &aReader)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    switch (GetPropertySchemaHandle(aLeafHandle)) {
        case TestATrait::kPropertyHandle_TaA:
            err = aReader.Get(taa);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, "<<  ta_a = %u", taa);
            break;

        case TestATrait::kPropertyHandle_TaB:
            err = aReader.Get(tab);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, "<<  ta_b = %u", tab);
            break;

        case TestATrait::kPropertyHandle_TaC:
            uint32_t next_tac;
            err = aReader.Get(next_tac);
            SuccessOrExit(err);
            if (next_tac != tac)
            {
                WeaveLogDetail(DataManagement, "<<  ta_c is changed from %u to %u", tac, next_tac);
                tac = next_tac;
            }

            WeaveLogDetail(DataManagement, "<<  ta_c = %u", tac);
            break;

        case TestATrait::kPropertyHandle_TaD_SaA:
            uint32_t next_tad_saa;
            err = aReader.Get(next_tad_saa);
            SuccessOrExit(err);
            if (next_tad_saa != tad.saA)
            {
                WeaveLogDetail(DataManagement, "<<  ta_d.sa_a is changed from %u to %u", tad.saA, next_tad_saa);
                tad.saA = next_tad_saa;
            }

            WeaveLogDetail(DataManagement, "<<  ta_d.sa_a = %u", tad.saA);
            break;

        case TestATrait::kPropertyHandle_TaD_SaB:
            bool next_tad_sab;
            err = aReader.Get(next_tad_sab);
            SuccessOrExit(err);
            if (next_tad_sab != tad.saB)
            {
                WeaveLogDetail(DataManagement, "<<  ta_d.sa_b is changed from %u to %u", tad.saB, next_tad_sab);
                tad.saB = next_tad_sab;
            }

            WeaveLogDetail(DataManagement, "<<  ta_d.sa_b = %u", tad.saB);
            break;

        case TestATrait::kPropertyHandle_TaE:
        {
            TLVType outerType;
            uint32_t i = 0;

            err = aReader.EnterContainer(outerType);
            SuccessOrExit(err);

            while (((err = aReader.Next()) == WEAVE_NO_ERROR) && (i < (sizeof(tae) / sizeof(tae[0])))) {
                uint32_t next_tae;
                err = aReader.Get(next_tae);
                SuccessOrExit(err);
                if (tae[i] != next_tae)
                {
                    WeaveLogDetail(DataManagement, "<<  ta_e[%u] is changed from %u to %u", i, tae[i], next_tae);
                    tae[i] = next_tae;
                }

                WeaveLogDetail(DataManagement, "<<  ta_e[%u] = %u", i, tae[i]);
                i++;
            }

            err = aReader.ExitContainer(outerType);
            break;
        }

        case TestATrait::kPropertyHandle_TaG:
        {
            if (aReader.GetType() == kTLVType_UTF8String)
            {
                err = aReader.GetString(tag_string, sizeof(tag_string));
                SuccessOrExit(err);

                tag_use_ref = false;

                WeaveLogDetail(DataManagement, "<<  ta_g string = %s", tag_string);
            }
            else
            {
                err = aReader.Get(tag_ref);
                SuccessOrExit(err);

                tag_use_ref = true;

                WeaveLogDetail(DataManagement, "<<  ta_g ref = %u", tag_ref);
            }
        }
            break;

        case TestATrait::kPropertyHandle_TaK:
            err = aReader.GetBytes(&tak[0], sizeof(tak));
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, "<<  ta_k %d bytes", sizeof(tak));
            break;

        case TestATrait::kPropertyHandle_TaL:
            err = aReader.Get(tal);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, "<<  ta_l = %x", tal);
            break;

        case TestATrait::kPropertyHandle_TaM:
            err = aReader.Get(tam_resourceid);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, "<<  ta_m = %" PRIx64 , tam_resourceid);
            break;

        case TestATrait::kPropertyHandle_TaN:
            err = aReader.GetBytes(&tan[0], sizeof(tan));
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, "<<  ta_n %d bytes", sizeof(tan));
            DumpMemory(&tan[0], sizeof(tan), "WEAVE:DMG: <<  ta_n ", 16);
            break;

        case TestATrait::kPropertyHandle_TaO:
            err = aReader.Get(tao);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, "<<  ta_o = %d", tao);
            break;

        case TestATrait::kPropertyHandle_TaP:
            int64_t next_tap;
            err = aReader.Get(next_tap);
            SuccessOrExit(err);

            if (next_tap != tap)
            {
                WeaveLogDetail(DataManagement, "<<  ta_p is changed from %d to %d", tap, next_tap);
                tap = next_tap;
            }
            WeaveLogDetail(DataManagement, "<<  ta_p = %d", tap);
            break;

        case TestATrait::kPropertyHandle_TaQ:
            err = aReader.Get(taq);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, "<<  ta_q %" PRId64 , taq);
            break;

        case TestATrait::kPropertyHandle_TaR:
            err = aReader.Get(tar);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, "<<  ta_r %u", tar);
            break;

        case TestATrait::kPropertyHandle_TaS:
            err = aReader.Get(tas);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, "<<  ta_s %u", tas);
            break;

        case TestATrait::kPropertyHandle_TaT:
            err = aReader.Get(tat);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, "<<  ta_t %u", tat);
            break;

        case TestATrait::kPropertyHandle_TaU:
            err = aReader.Get(tau);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, "<<  ta_u %d", tau);
            break;

        case TestATrait::kPropertyHandle_TaV:
            err = aReader.Get(tav);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, "<<  ta_v %u", tav);
            break;

        case TestATrait::kPropertyHandle_TaW:
            err = aReader.GetString(&taw[0], sizeof(taw));
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, "<<  ta_w %s", taw);
            break;

        case TestATrait::kPropertyHandle_TaX:
            err = aReader.Get(tax);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, "<<  ta_x %d", tax);
            break;

        case TestATrait::kPropertyHandle_TaI_Value:
            err = aReader.Get(tai_stageditem);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, "<<  tai[%u] = %u", GetPropertyDictionaryKey(aLeafHandle), tai_stageditem);
            break;

        case TestATrait::kPropertyHandle_TaJ_Value_SaA:
            err = aReader.Get(taj_stageditem.saA);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, "<<  taj[%u].sa_a = %u", GetPropertyDictionaryKey(aLeafHandle), taj_stageditem.saA);
            break;

        case TestATrait::kPropertyHandle_TaJ_Value_SaB:
            err = aReader.Get(taj_stageditem.saB);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, "<<  taj[%u].sa_b = %u", GetPropertyDictionaryKey(aLeafHandle), taj_stageditem.saB);
            break;

        default:
            WeaveLogDetail(DataManagement, "<<  UNKNOWN!");
    }

    exit:
    return err;
}

WEAVE_ERROR TestATraitUpdatableDataSink::GetNextDictionaryItemKey(PropertyPathHandle aDictionaryHandle, uintptr_t &aContext, PropertyDictionaryKey &aKey)
{
    static std::map<uint16_t, uint32_t>::iterator tai_iter;
    static std::map<uint16_t, TestATrait::StructA>::iterator taj_iter;

    if (aDictionaryHandle == TestATrait::kPropertyHandle_TaI) {
        return GetNextDictionaryItemKeyHelper(&tai_map, tai_iter, aContext, aKey);
    }
    else if (aDictionaryHandle == TestATrait::kPropertyHandle_TaJ) {
        return GetNextDictionaryItemKeyHelper(&taj_map, taj_iter, aContext, aKey);
    }

    return WEAVE_ERROR_INVALID_ARGUMENT;
}

WEAVE_ERROR TestATraitUpdatableDataSink::GetData(PropertyPathHandle aHandle,
                                        uint64_t aTagToWrite,
                                        TLVWriter &aWriter,
                                        bool &aIsNull,
                                        bool &aIsPresent)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (mSchemaEngine->IsNullable(aHandle))
    {
        aIsNull = nullified_path[GetPropertySchemaHandle(aHandle) - TraitSchemaEngine::kHandleTableOffset];
    }
    else
    {
        aIsNull = false;
    }

    aIsPresent = true;

    // TDM will handle writing null
    if ((!aIsNull) && (aIsPresent) && (mSchemaEngine->IsLeaf(aHandle)))
    {
        err = GetLeafData(aHandle, aTagToWrite, aWriter);
    }

    return err;
}



WEAVE_ERROR TestATraitUpdatableDataSink::GetLeafData(PropertyPathHandle aLeafHandle, uint64_t aTagToWrite, TLVWriter &aWriter)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    switch (GetPropertySchemaHandle(aLeafHandle)) {
        case TestATrait::kPropertyHandle_TaA:
            err = aWriter.Put(aTagToWrite, taa);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  ta_a = %u", taa);
            break;

        case TestATrait::kPropertyHandle_TaB:
            err = aWriter.Put(aTagToWrite, tab);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  ta_b = %u", tab);
            break;

        case TestATrait::kPropertyHandle_TaC:
            err = aWriter.Put(aTagToWrite, tac);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  ta_c = %u", tac);
            break;

        case TestATrait::kPropertyHandle_TaD_SaA:
            err = aWriter.Put(aTagToWrite, tad.saA);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  ta_d.sa_a = %u", tad.saA);
            break;

        case TestATrait::kPropertyHandle_TaD_SaB:
            err = aWriter.PutBoolean(aTagToWrite, tad.saB);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  ta_d.sa_b = %s", tad.saB ? "true" : "false");
            break;

        case TestATrait::kPropertyHandle_TaE:
        {
            TLVType outerType;

            err = aWriter.StartContainer(aTagToWrite, kTLVType_Array, outerType);
            SuccessOrExit(err);

            for (size_t i = 0; i < (sizeof(tae) / sizeof(tae[0])); i++) {
                err = aWriter.Put(AnonymousTag, tae[i]);
                SuccessOrExit(err);

                WeaveLogDetail(DataManagement, ">>  ta_e[%u] = %u", i, tae[i]);
            }

            err = aWriter.EndContainer(outerType);
            break;
        }

        case TestATrait::kPropertyHandle_TaG:
        {
            if (tag_use_ref)
            {
                err = aWriter.Put(aTagToWrite, tag_ref);
                SuccessOrExit(err);

                WeaveLogDetail(DataManagement, ">>  ta_g ref = %u", tag_ref);
            }
            else
            {
                err = aWriter.PutString(aTagToWrite, tag_string);
                SuccessOrExit(err);

                WeaveLogDetail(DataManagement, ">>  ta_g string = %s", tag_string);
            }
        }
            break;

        case TestATrait::kPropertyHandle_TaH:
            // maybe TODO
            break;

        case TestATrait::kPropertyHandle_TaK:
            err = aWriter.PutBytes(aTagToWrite, tak, sizeof(tak));
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  ta_k %d bytes", sizeof(tak));
            break;

        case TestATrait::kPropertyHandle_TaL:
            err = aWriter.Put(aTagToWrite, tal);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  ta_l = %x", tal);
            break;

        case TestATrait::kPropertyHandle_TaM:
            err = aWriter.Put(aTagToWrite, tam_resourceid);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  ta_m = %" PRIx64 , tam_resourceid);
            break;

        case TestATrait::kPropertyHandle_TaN:
            err = aWriter.PutBytes(aTagToWrite, tan, sizeof(tan));
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  ta_n %d bytes", sizeof(tan));
            DumpMemory(&tan[0], sizeof(tan), "WEAVE:DMG: >>  ta_n ", 16);
            break;

        case TestATrait::kPropertyHandle_TaO:
            err = aWriter.Put(aTagToWrite, tao);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  ta_o %u", tao);
            break;

        case TestATrait::kPropertyHandle_TaP:
            err = aWriter.Put(aTagToWrite, tap);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  ta_p %" PRId64 , tap);
            break;

        case TestATrait::kPropertyHandle_TaQ:
            err = aWriter.Put(aTagToWrite, taq);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  ta_q %" PRId64 , taq);
            break;

        case TestATrait::kPropertyHandle_TaR:
            err = aWriter.Put(aTagToWrite, tar);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  ta_r %u", tar);
            break;

        case TestATrait::kPropertyHandle_TaS:
            err = aWriter.Put(aTagToWrite, tas);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  ta_s %u", tas);
            break;

        case TestATrait::kPropertyHandle_TaT:
            err = aWriter.Put(aTagToWrite, tat);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  ta_t %u", tat);
            break;

        case TestATrait::kPropertyHandle_TaU:
            err = aWriter.Put(aTagToWrite, tau);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  ta_u %d", tau);
            break;

        case TestATrait::kPropertyHandle_TaV:
            err = aWriter.PutBoolean(aTagToWrite, tav);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  ta_v %u", tav);
            break;

        case TestATrait::kPropertyHandle_TaW:
            err = aWriter.PutString(aTagToWrite, taw);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  ta_w %s", taw);
            break;

        case TestATrait::kPropertyHandle_TaX:
            err = aWriter.Put(aTagToWrite, tax);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  ta_x %d", tax);
            break;

        case TestATrait::kPropertyHandle_TaI_Value:
        {
            PropertyDictionaryKey key = GetPropertyDictionaryKey(aLeafHandle);

            err = aWriter.Put(aTagToWrite, tai_map[key]);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  ta_i[%u] = %u", key, tai_map[key]);
            break;
        }

        case TestATrait::kPropertyHandle_TaJ_Value_SaA:
        {
            PropertyDictionaryKey key = GetPropertyDictionaryKey(aLeafHandle);

            err = aWriter.Put(aTagToWrite, taj_map[key].saA);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  ta_j[%u].sa_a = %u", key, taj_map[key].saA);
            break;
        }


        case TestATrait::kPropertyHandle_TaJ_Value_SaB:
        {
            PropertyDictionaryKey key = GetPropertyDictionaryKey(aLeafHandle);

            err = aWriter.PutBoolean(aTagToWrite, taj_map[key].saB);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  ta_j[%u].sa_b = %u", key, taj_map[key].saB);
            break;
        }

        default:
            WeaveLogDetail(DataManagement, ">>  TestATrait UNKNOWN! %08x", aLeafHandle);
    }

    exit:
    WeaveLogFunctError(err);
    return err;
}

TestBTraitUpdatableDataSink::TestBTraitUpdatableDataSink()
    : MockTraitUpdatableDataSink(&TestBTrait::TraitSchema)
{
    taa = TestATrait::ENUM_A_VALUE_1;
    tab = TestCommon::COMMON_ENUM_A_VALUE_1;
    tac = 0;
    tad_saa = 0;
    tad_sab = false;

    for (size_t i = 0; i < (sizeof(tae) / sizeof(tae[0])); i++) {
        tae[i] = 0;
    }

    strcpy(tai, "");

    tba = 0;
    strcpy(tbb_sba, "");
    tbb_sbb = 0;

    tbc_saa = 0;
    tbc_sab = false;
    strcpy(tbc_seac, "");

    for (size_t i = 0; i < sizeof(nullified_path) / sizeof(nullified_path[0]); i++)
    {
        nullified_path[i] = false;
    }
}

WEAVE_ERROR TestBTraitUpdatableDataSink::Mutate(SubscriptionClient * apSubClient,
                                                bool aIsConditional,
                                                MockWdmNodeOptions::WdmUpdateMutation aMutation)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    bool isLocked = false;
    static int testNum = 0;
    const int numTests = 3;

    err = Lock(apSubClient);
    SuccessOrExit(err);

    isLocked = true;

    switch (aMutation)
    {
    case MockWdmNodeOptions::kMutation_OneLeafOneStructure:

        err = SetUpdated(apSubClient, TestBTrait::kPropertyHandle_TbB_SbB, aIsConditional);
        SuccessOrExit(err);

        err = SetUpdated(apSubClient, TestBTrait::kPropertyHandle_TaA, aIsConditional);
        SuccessOrExit(err);

        if (taa == TestATrait::ENUM_A_VALUE_1) {
            taa = TestATrait::ENUM_A_VALUE_2;
        }
        else {
            taa = TestATrait::ENUM_A_VALUE_1;
        }

        tbb_sbb++;

        break;

    case MockWdmNodeOptions::kMutation_DiffLevelLeaves:

        err = SetUpdated(apSubClient, TestBTrait::kPropertyHandle_TaC, aIsConditional);
        SuccessOrExit(err);

        err = SetUpdated(apSubClient, TestBTrait::kPropertyHandle_TaP, aIsConditional);
        SuccessOrExit(err);

        err = SetUpdated(apSubClient, TestBTrait::kPropertyHandle_TbC_SaB, aIsConditional);
        SuccessOrExit(err);

        tap++;
        tac++;
        tbc_sab = !tbc_sab;

        break;

    case MockWdmNodeOptions::kMutation_OneLeaf:
    default:
        aMutation = MockWdmNodeOptions::kMutation_OneLeaf;

        err = SetUpdated(apSubClient, TestBTrait::kPropertyHandle_TaP, aIsConditional);
        SuccessOrExit(err);

        tap++;
        break;
    }

    testNum = (testNum +1) % numTests;

exit:
    WeaveLogDetail(DataManagement, "TestBTrait mutated %s with error %d", MockWdmNodeOptions::GetMutationStrings()[aMutation], err);

    if (isLocked)
    {
        Unlock(apSubClient);
    }

    return err;
}

void TestBTraitUpdatableDataSink::SetNullifiedPath(PropertyPathHandle aHandle, bool isNull)
{
    if (aHandle <= TestBTrait::kPropertyHandle_TaJ_Value_SaB)
    {
        if (nullified_path[aHandle - TraitSchemaEngine::kHandleTableOffset] != isNull)
        {
            nullified_path[aHandle - TraitSchemaEngine::kHandleTableOffset] = isNull;
        }
    }
}


WEAVE_ERROR
TestBTraitUpdatableDataSink::SetData(PropertyPathHandle aHandle, TLVReader &aReader, bool aIsNull)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (aIsNull && !mSchemaEngine->IsNullable(aHandle))
    {
        WeaveLogDetail(DataManagement, "<< Non-nullable handle %d received a NULL", aHandle);

#if TDM_DISABLE_STRICT_SCHEMA_COMPLIANCE == 0
        ExitNow(err = WEAVE_ERROR_INVALID_TLV_ELEMENT);
#endif
    }

    SetNullifiedPath(aHandle, aIsNull);

    if ((!aIsNull) && (mSchemaEngine->IsLeaf(aHandle)))
    {
        err = SetLeafData(aHandle, aReader);
        // set the parent handles to non-null
        while (aHandle != kRootPropertyPathHandle)
        {
            SetNullifiedPath(aHandle, aIsNull);
            aHandle = mSchemaEngine->GetParent(aHandle);
        }
    }

    return err;
}

WEAVE_ERROR
TestBTraitUpdatableDataSink::SetLeafData(PropertyPathHandle aLeafHandle, TLVReader &aReader)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    switch (aLeafHandle) {
        //
        // TestATrait
        //

        case TestBTrait::kPropertyHandle_TaA:
            int32_t next_taa;
            err = aReader.Get(next_taa);
            SuccessOrExit(err);
            if (next_taa != taa)
            {
                WeaveLogDetail(DataManagement, "<<  ta_a is changed from %u to %u", taa, next_taa);
                taa = next_taa;
            }

            WeaveLogDetail(DataManagement, "<<  ta_a = %u", taa);
            break;

        case TestBTrait::kPropertyHandle_TaB:
            int32_t next_tab;
            err = aReader.Get(next_tab);
            SuccessOrExit(err);
            if (next_tab != tab)
            {
                WeaveLogDetail(DataManagement, "<<  ta_b is changed from %u to %u", tab, next_tab);
                tab = next_tab;
            }

            WeaveLogDetail(DataManagement, "<<  ta_b = %u", tab);
            break;

        case TestBTrait::kPropertyHandle_TaC:
            uint32_t next_tac;
            err = aReader.Get(next_tac);
            SuccessOrExit(err);
            if (next_tac != tac)
            {
                WeaveLogDetail(DataManagement, "<<  ta_c is changed from %u to %u", tac, next_tac);
                tac = next_tac;
            }

            WeaveLogDetail(DataManagement, "<<  ta_c = %u", tac);
            break;

        case TestBTrait::kPropertyHandle_TaD_SaA:
            uint32_t next_tad_saa;
            err = aReader.Get(next_tad_saa);
            SuccessOrExit(err);
            if (next_tad_saa != tad_saa)
            {
                WeaveLogDetail(DataManagement, "<<  ta_d.sa_a is changed from %u to %u", tad_saa, next_tad_saa);
                tad_saa = next_tad_saa;
            }

            WeaveLogDetail(DataManagement, "<<  ta_d.sa_a = %u", tad_saa);
            break;

        case TestBTrait::kPropertyHandle_TaD_SaB:
            bool next_tad_sab;
            err = aReader.Get(next_tad_sab);
            SuccessOrExit(err);
            if (next_tad_sab != tad_sab)
            {
                WeaveLogDetail(DataManagement, "<<  ta_d.sa_b is changed from %u to %u", tad_sab, next_tad_sab);
                tad_sab = next_tad_sab;
            }

            WeaveLogDetail(DataManagement, "<<  ta_d.sa_b = %u", tad_sab);
            break;

        case TestBTrait::kPropertyHandle_TaE:
        {
            TLVType outerType;
            uint32_t i = 0;

            err = aReader.EnterContainer(outerType);
            SuccessOrExit(err);

            while (((err = aReader.Next()) == WEAVE_NO_ERROR) && (i < (sizeof(tae) / sizeof(tae[0])))) {
                uint32_t next_tae;
                err = aReader.Get(next_tae);
                SuccessOrExit(err);
                if (tae[i] != next_tae)
                {
                    WeaveLogDetail(DataManagement, "<<  ta_e[%u] is changed from %u to %u", i, tae[i], next_tae);
                    tae[i] = next_tae;
                }

                WeaveLogDetail(DataManagement, "<<  ta_e[%u] = %u", i, tae[i]);
                i++;
            }

            err = aReader.ExitContainer(outerType);
            break;
        }

        case TestBTrait::kPropertyHandle_TaP:
            err = aReader.Get(tap);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, "<<  ta_p = %" PRId64, tap);
            break;

        //
        // TestBTrait
        //

        case TestBTrait::kPropertyHandle_TbA:
            uint32_t next_tba;
            err = aReader.Get(next_tba);
            SuccessOrExit(err);
            if (tba != next_tba)
            {
                WeaveLogDetail(DataManagement, "<<  tb_a is changed from %u to %u", tba, next_tba);
                tba = next_tba;
            }

            WeaveLogDetail(DataManagement, "<<  tb_a = %u", tba);
            break;

        case TestBTrait::kPropertyHandle_TbB_SbA:
            char next_tbb_sba[MAX_ARRAY_LEN];
            err = aReader.GetString(next_tbb_sba, MAX_ARRAY_SIZE);
            SuccessOrExit(err);
            if (strncmp(tbb_sba, next_tbb_sba, MAX_ARRAY_SIZE))
            {
                WeaveLogDetail(DataManagement, "<<  tb_b.sb_a is changed from %s to %s", tbb_sba, next_tbb_sba);
                memcpy(tbb_sba, next_tbb_sba, MAX_ARRAY_SIZE);
            }

            WeaveLogDetail(DataManagement, "<<  tb_b.sb_a = %s", tbb_sba);
            break;

        case TestBTrait::kPropertyHandle_TbB_SbB:
            uint32_t next_tbb_sbb;
            err = aReader.Get(next_tbb_sbb);
            SuccessOrExit(err);
            if (tbb_sbb != next_tbb_sbb)
            {
                WeaveLogDetail(DataManagement, "<<  tb_b.sb_b is changed from %u to %u", tbb_sbb, next_tbb_sbb);
                tbb_sbb = next_tbb_sbb;
            }

            WeaveLogDetail(DataManagement, "<<  tb_b.sb_b = %u", tbb_sbb);
            break;

        case TestBTrait::kPropertyHandle_TbC_SaA:
            uint32_t next_tbc_saa;
            err = aReader.Get(next_tbc_saa);
            SuccessOrExit(err);
            if (tbc_saa != next_tbc_saa)
            {
                WeaveLogDetail(DataManagement, "<<  tb_c.sa_a is changed from %u to %u", tbc_saa, next_tbc_saa);
                tbc_saa = next_tbc_saa;
            }

            WeaveLogDetail(DataManagement, "<<  tb_c.sa_a = %u", tbc_saa);
            break;

        case TestBTrait::kPropertyHandle_TbC_SaB:
            bool next_tbc_sab;
            err = aReader.Get(next_tbc_sab);
            SuccessOrExit(err);
            if (tbc_sab != next_tbc_sab)
            {
                WeaveLogDetail(DataManagement, "<<  tb_c.sa_b is changed from %u to %u", tbc_sab, next_tbc_sab);
                tbc_sab = next_tbc_sab;
            }

            WeaveLogDetail(DataManagement, "<<  tb_c.sa_b = %u", tbc_sab);
            break;

        case TestBTrait::kPropertyHandle_TbC_SeaC:
            char next_tbc_seac[MAX_ARRAY_LEN];
            err = aReader.GetString(next_tbc_seac, MAX_ARRAY_SIZE);
            SuccessOrExit(err);
            if (strncmp(tbc_seac, next_tbc_seac, MAX_ARRAY_SIZE))
            {
                WeaveLogDetail(DataManagement, "<<  tb_c.sea_c is changed from \"%s\" to \"%s\"", tbc_seac, next_tbc_seac);
                memcpy(tbc_seac, next_tbc_seac, MAX_ARRAY_SIZE);
            }

            WeaveLogDetail(DataManagement, "<<  tb_c.sea_c = \"%s\"", tbc_seac);
            break;

        default:
            WeaveLogDetail(DataManagement, "<<  TestBTrait UNKNOWN! %08x", aLeafHandle);
    }

exit:
    return err;
}


WEAVE_ERROR TestBTraitUpdatableDataSink::GetData(PropertyPathHandle aHandle,
                                        uint64_t aTagToWrite,
                                        TLVWriter &aWriter,
                                        bool &aIsNull,
                                        bool &aIsPresent)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (mSchemaEngine->IsNullable(aHandle))
    {
        aIsNull = nullified_path[GetPropertySchemaHandle(aHandle) - TraitSchemaEngine::kHandleTableOffset];
    }
    else
    {
        aIsNull = false;
    }

    aIsPresent = true;

    // TDM will handle writing null
    if ((!aIsNull) && (aIsPresent) && (mSchemaEngine->IsLeaf(aHandle)))
    {
        err = GetLeafData(aHandle, aTagToWrite, aWriter);
    }

    return err;
}

WEAVE_ERROR TestBTraitUpdatableDataSink::GetLeafData(PropertyPathHandle aLeafHandle, uint64_t aTagToWrite, TLVWriter &aWriter)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    switch (aLeafHandle) {
        //
        // TestATrait
        //

        case TestBTrait::kPropertyHandle_TaA:
            err = aWriter.Put(aTagToWrite, taa);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  ta_a = %u", taa);
            break;

        case TestBTrait::kPropertyHandle_TaB:
            err = aWriter.Put(aTagToWrite, tab);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  ta_b = %u", tab);
            break;

        case TestBTrait::kPropertyHandle_TaC:
            err = aWriter.Put(aTagToWrite, tac);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  ta_c = %u", tac);
            break;

        case TestBTrait::kPropertyHandle_TaD_SaA:
            err = aWriter.Put(aTagToWrite, tad_saa);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  ta_d.sa_a = %u", tad_saa);
            break;

        case TestBTrait::kPropertyHandle_TaD_SaB:
            err = aWriter.PutBoolean(aTagToWrite, tad_sab);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  ta_d.sa_b = %s", tad_sab ? "true" : "false");
            break;

        case TestBTrait::kPropertyHandle_TaP:
            err = aWriter.Put(aTagToWrite, tap);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  ta_p = %d", tap);
            break;

        case TestBTrait::kPropertyHandle_TaE:
        {
            TLVType outerType;

            err = aWriter.StartContainer(aTagToWrite, kTLVType_Array, outerType);
            SuccessOrExit(err);

            for (size_t i = 0; i < (sizeof(tae) / sizeof(tae[0])); i++) {
                err = aWriter.Put(AnonymousTag, tae[i]);
                SuccessOrExit(err);

                WeaveLogDetail(DataManagement, ">>  ta_e[%u] = %u", i, tae[i]);
            }

            err = aWriter.EndContainer(outerType);
            break;
        }

            //
            // TestBTrait
            //

        case TestBTrait::kPropertyHandle_TbA:
            err = aWriter.Put(aTagToWrite, tba);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  tb_a = %u", tba);
            break;

        case TestBTrait::kPropertyHandle_TbB_SbA:
            err = aWriter.PutString(aTagToWrite, tbb_sba);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  tb_b.sb_a = \"%s\"", tbb_sba);
            break;

        case TestBTrait::kPropertyHandle_TbB_SbB:
            err = aWriter.Put(aTagToWrite, tbb_sbb);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  tb_b.sb_b = %u", tbb_sbb);
            break;

        case TestBTrait::kPropertyHandle_TbC_SaA:
            err = aWriter.Put(aTagToWrite, tbc_saa);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  tb_c.sa_a = %u", tbc_saa);
            break;

        case TestBTrait::kPropertyHandle_TbC_SaB:
            err = aWriter.PutBoolean(aTagToWrite, tbc_sab);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  tb_c.sa_b = %s", tbc_sab ? "true" : "false");
            break;

        case TestBTrait::kPropertyHandle_TbC_SeaC:
            err = aWriter.PutString(aTagToWrite, tbc_seac);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  tb_c.sea_c = %s", tbc_seac);
            break;

        default:
            WeaveLogDetail(DataManagement, ">>  UNKNOWN!");
    }

    exit:
    WeaveLogFunctError(err);
    return err;
}

WEAVE_ERROR TestBTraitUpdatableDataSink::GetNextDictionaryItemKey(PropertyPathHandle aDictionaryHandle, uintptr_t &aContext, PropertyDictionaryKey &aKey)
{
    return WEAVE_END_OF_INPUT;
}

#endif // WEAVE_CONFIG_ENABLE_WDM_UPDATE
