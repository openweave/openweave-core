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
 *      This file defines the mock trait data sources for use with the mock device framework.
 *
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
#include "MockSourceTraits.h"
#include "TestGroupKeyStore.h"
#include "ToolCommon.h"

#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Profiles/security/ApplicationKeysTrait.h>
#include <Weave/Profiles/security/ApplicationKeysStructSchema.h>
#include <schema/weave/common/DayOfWeekEnum.h>


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

LocaleSettingsTraitDataSource::LocaleSettingsTraitDataSource()
    : TraitDataSource(&LocaleSettingsTrait::TraitSchema)
{
    mVersion = 300;
    memset(mLocale, 0, sizeof(mLocale));
}

void LocaleSettingsTraitDataSource::Mutate()
{
    static unsigned int whichLocale = 0;
    static const char * locales[] = { "en-US", "zh-TW", "ja-JP", "pl-PL", "zh-CN" };

    Lock();

    MOCK_strlcpy(mLocale, locales[whichLocale], sizeof(mLocale));
    whichLocale = (whichLocale + 1) % (sizeof(locales)/sizeof(locales[0]));

    SetDirty(LocaleSettingsTrait::kPropertyHandle_active_locale);

    Unlock();
}

WEAVE_ERROR
LocaleSettingsTraitDataSource::SetLeafData(PropertyPathHandle aLeafHandle, TLVReader &aReader)
{
    return WEAVE_ERROR_UNSUPPORTED_WEAVE_FEATURE;
}

WEAVE_ERROR
LocaleSettingsTraitDataSource::GetLeafData(PropertyPathHandle aLeafHandle, uint64_t aTagToWrite, TLVWriter &aWriter)
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

LocaleCapabilitiesTraitDataSource::LocaleCapabilitiesTraitDataSource()
    : TraitDataSource(&LocaleCapabilitiesTrait::TraitSchema)
{
    mVersion = 400;

    mNumLocales = 3;
    mLocales[0] = "pl-PL";
    mLocales[1] = "ja-JP";
    mLocales[2] = "fr-FR";
}

void LocaleCapabilitiesTraitDataSource::Mutate()
{
    Lock();

    switch (GetVersion() % 3) {
        case 0:
            mNumLocales = 2;
            mLocales[0] = "en-US";
            mLocales[1] = "zh-TW";
            break;

        case 1:
            mNumLocales = 1;
            mLocales[0] = "zh-CN";
            break;

        case 2:
            mNumLocales = 3;
            mLocales[0] = "ja-JP";
            mLocales[1] = "pl-PL";
            mLocales[2] = "zh-CN";
            break;
    }

    SetDirty(LocaleCapabilitiesTrait::kPropertyHandle_available_locales);

    Unlock();
}

WEAVE_ERROR
LocaleCapabilitiesTraitDataSource::SetLeafData(PropertyPathHandle aLeafHandle, TLVReader &aReader)
{
    return WEAVE_ERROR_UNSUPPORTED_WEAVE_FEATURE;
}

WEAVE_ERROR
LocaleCapabilitiesTraitDataSource::GetLeafData(PropertyPathHandle aLeafHandle, uint64_t aTagToWrite, TLVWriter &aWriter)
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

BoltLockSettingTraitDataSource::BoltLockSettingTraitDataSource()
    : TraitDataSource(&BoltLockSettingTrait::TraitSchema)
{
    mVersion = 500;

    mAutoRelockOn = false;
    mAutoRelockDuration = 2;
}

void BoltLockSettingTraitDataSource::Mutate()
{
    Lock();

    if ((GetVersion() % 2) == 0) {
        SetDirty(BoltLockSettingTrait::kPropertyHandle_auto_relock_on);
        mAutoRelockOn = !mAutoRelockOn;
    }
    else {
        SetDirty(BoltLockSettingTrait::kPropertyHandle_auto_relock_duration);
        mAutoRelockDuration++;
    }

    Unlock();
}

WEAVE_ERROR
BoltLockSettingTraitDataSource::SetLeafData(PropertyPathHandle aLeafHandle, TLVReader &aReader)
{
    return WEAVE_ERROR_UNSUPPORTED_WEAVE_FEATURE;
}

WEAVE_ERROR
BoltLockSettingTraitDataSource::GetLeafData(PropertyPathHandle aLeafHandle, uint64_t aTagToWrite, TLVWriter &aWriter)
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

TestATraitDataSource::TestATraitDataSource()
    : TraitDataSource(&TestATrait::TraitSchema),
      mActiveCommand(NULL)
{
    uint8_t *tmp;
    mVersion = 100;
    mTestCounter = 0;
    taa = TestATrait::ENUM_A_VALUE_1;
    tab = TestCommonTrait::COMMON_ENUM_A_VALUE_1;
    tac = 3;
    tad.saA = 4;
    tad.saB = true;

    mTraitTestSet = 0;

    for (size_t i = 0; i < (sizeof(tae) / sizeof(tae[0])); i++) {
        tae[i] = i + 5;
    }

    tap = 1491859262000;

    tag_use_ref = false;
    tag_ref = 10;
    tam_resourceid = 0x18b4300000beefULL;
    tan_type = 1;

    tmp = &tan[0];
    nl::Weave::Encoding::LittleEndian::Write16(tmp, tan_type);
    nl::Weave::Encoding::LittleEndian::Write64(tmp, tam_resourceid);

    taq = -3000;
    tar = 3000;
    tas = 3000;

    tat = 1000;
    tau = -1000;
    tav = true;
    tax = 800;

    for (size_t i = 0; i < 4; i++) {
        tai_map[i] = 100 + i;
    }

    for (size_t i = 0; i < 4; i++) {
        taj_map[i] = { 300 + (uint32_t)i, true };
    }

    tal = DAY_OF_WEEK_SUNDAY;
}

void TestATraitDataSource::Mutate()
{
    Lock();

    if (mTraitTestSet == 0)
    {
        static const uint8_t kNumTestCases = 8;
        if ((mTestCounter % kNumTestCases) == 6) {
            // enums
            SetDirty(TestATrait::kPropertyHandle_TaD_SaA);
            SetDirty(TestATrait::kPropertyHandle_TaD_SaB);
            SetDirty(TestATrait::kPropertyHandle_TaA);

            if (taa == TestATrait::ENUM_A_VALUE_1) {
                taa = TestATrait::ENUM_A_VALUE_2;
            }
            else {
                taa = TestATrait::ENUM_A_VALUE_1;
            }

            tad.saA++;
            tad.saB = !tad.saB;
        }
        else if ((mTestCounter % kNumTestCases) == 1) {
            // timestamp and duration
            SetDirty(TestATrait::kPropertyHandle_TaP);
            SetDirty(TestATrait::kPropertyHandle_TaC);
            SetDirty(TestATrait::kPropertyHandle_TaR);
            SetDirty(TestATrait::kPropertyHandle_TaS);

            tap++;
            tac++;
            tar++;
            tas++;
        }
        else if ((mTestCounter % kNumTestCases) == 2) {
            // resource id, implicit and otherwise
            uint8_t *tmp = &tan[0];
            SetDirty(TestATrait::kPropertyHandle_TaM);
            SetDirty(TestATrait::kPropertyHandle_TaN);

            tan_type = (tan_type + 1) % 8;
            nl::Weave::Encoding::LittleEndian::Write16(tmp, tan_type);
        }
        else if ((mTestCounter % kNumTestCases) == 3) {
            // string ref
            SetDirty(TestATrait::kPropertyHandle_TaG);

            tag_use_ref = !tag_use_ref;
        }
        else if ((mTestCounter % kNumTestCases) == 4)
        {
            // day of week
            SetDirty(TestATrait::kPropertyHandle_TaL);

            tal ^= DAY_OF_WEEK_FRIDAY;
        }
        else if ((mTestCounter % kNumTestCases) == 5)
        {
            // boxed types
            SetDirty(TestATrait::kPropertyHandle_TaT);
            SetDirty(TestATrait::kPropertyHandle_TaU);
            SetDirty(TestATrait::kPropertyHandle_TaV);
            SetDirty(TestATrait::kPropertyHandle_TaW);
            SetDirty(TestATrait::kPropertyHandle_TaX);

            tat++;
            tau--;
            tav = !tav;
            tax++;
            if (tax > 808)
            {
                tax = -808;
            }
        }
        else if ((mTestCounter % kNumTestCases) == 0)
        {
            // nullified fields
            SetNullifiedPath(TestATrait::kPropertyHandle_TaD, true);
            SetNullifiedPath(TestATrait::kPropertyHandle_TaP, true);
            SetNullifiedPath(TestATrait::kPropertyHandle_TaS, true);
            SetNullifiedPath(TestATrait::kPropertyHandle_TaT, true);
            SetNullifiedPath(TestATrait::kPropertyHandle_TaM, true);
            SetNullifiedPath(TestATrait::kPropertyHandle_TaN, true);
        }
        else if ((mTestCounter % kNumTestCases) == 7)
        {
            // nullified fields
            SetNullifiedPath(TestATrait::kPropertyHandle_TaD, false);
            SetNullifiedPath(TestATrait::kPropertyHandle_TaP, false);
            SetNullifiedPath(TestATrait::kPropertyHandle_TaS, false);
            SetNullifiedPath(TestATrait::kPropertyHandle_TaT, false);
            SetNullifiedPath(TestATrait::kPropertyHandle_TaM, false);
            SetNullifiedPath(TestATrait::kPropertyHandle_TaN, false);
        }
    }
    else
    {
        printf("Flipping %u\n", mTestCounter % 14);

        // For dictionary testing, we need to do a couple of different directed tests.
        //
        // 0. Generate a notify directed at the dictionary for modification of the dictionary as a whole (path = parent of dictionary). This results in a replace of the dictionary (2nd level replace)
        // 1. Generate a notify directed at the dictionary item for modification of a single existing item (path = dictionary item)
        // 2. Generate a notify directed deep within a dictionary item for modification of a single existing item (path = deep in dictionary item)
        // 3. Generate a notify directed at the dictionary item for modification of multiple items (path = dictionary, merge operation)
        // 4. Generate a notify indicating the addition of multiplle dictionary items (path = dictionary, merge)
        // 5. Generate a notify indicating the deletion of a dictionary item (path = dictionary, delete)
        // 6. Generate a notify indicating the deletion of multiple dictionary items (path = dictionary, delete)
        // 7. Generate a notify indicating the deletion of multiple dictionary items that overflows delete handle set (path = parent of dictionary, replace)
        // 8. Generate a notify indicating the deletion of an item + addition of another item (path = dictionary, add + delete)
        // 9. Generate a notify indicating deletion of an item followed by the re-addition of that item (path = dictionary, modify of item).
        // 10. Generate a notify indicating the modification of an item followed by a deletion of that item (path = dictionary, delete of item).
        // 11. Generate a notify indicating the deletion of a single item from two different dictionaries (path = root)
        // 12. Generate a notify indicating the deletion of addition of a single item into two different dictionaries (path = root)
        // 13. Wipe out both dictionaries (path = root)
        // The line numbers above correspond to the various 'if' statement branches below.
        //

        if ((mTestCounter % 14) == 0) {
            SetDirty(TestATrait::kPropertyHandle_TaJ);

            for (uint32_t i = 0; i < 4; i++) {
                taj_map[i] = { 300 + (uint32_t)i, false };
            }
        }
        else if ((mTestCounter % 14) == 1) {
            SetDirty(CreatePropertyPathHandle(TestATrait::kPropertyHandle_TaJ_Value, 1));
            taj_map[1].saA += 100;
        }
        else if ((mTestCounter % 14) == 2) {
            SetDirty(CreatePropertyPathHandle(TestATrait::kPropertyHandle_TaJ_Value_SaA, 1));
            taj_map[1].saA += 100;
        }
        else if ((mTestCounter % 14) == 3) {
            SetDirty(CreatePropertyPathHandle(TestATrait::kPropertyHandle_TaJ_Value, 2));
            SetDirty(CreatePropertyPathHandle(TestATrait::kPropertyHandle_TaJ_Value, 3));

            taj_map[2].saA += 100;
            taj_map[3].saA += 100;
        }
        else if ((mTestCounter % 14) == 4) {
            SetDirty(CreatePropertyPathHandle(TestATrait::kPropertyHandle_TaJ_Value, 4));
            SetDirty(CreatePropertyPathHandle(TestATrait::kPropertyHandle_TaJ_Value, 5));
            SetDirty(CreatePropertyPathHandle(TestATrait::kPropertyHandle_TaJ_Value, 6));
            SetDirty(CreatePropertyPathHandle(TestATrait::kPropertyHandle_TaJ_Value, 7));
            SetDirty(CreatePropertyPathHandle(TestATrait::kPropertyHandle_TaJ_Value, 8));
            SetDirty(CreatePropertyPathHandle(TestATrait::kPropertyHandle_TaJ_Value, 9));
            SetDirty(CreatePropertyPathHandle(TestATrait::kPropertyHandle_TaJ_Value, 10));

            taj_map[4] = { 304, false };
            taj_map[5] = { 305, false };
            taj_map[6] = { 306, false };
            taj_map[7] = { 307, false };
            taj_map[8] = { 308, false };
            taj_map[9] = { 309, false };
            taj_map[10] = { 310, false };
        }
        else if ((mTestCounter % 14) == 5) {
            taj_map.erase(10);
            DeleteKey(CreatePropertyPathHandle(TestATrait::kPropertyHandle_TaJ_Value, 10));
        }
        else if ((mTestCounter % 14) == 6) {
            taj_map.erase(9);
            taj_map.erase(8);
            DeleteKey(CreatePropertyPathHandle(TestATrait::kPropertyHandle_TaJ_Value, 9));
            DeleteKey(CreatePropertyPathHandle(TestATrait::kPropertyHandle_TaJ_Value, 8));
        }
        else if ((mTestCounter % 14) == 7) {
            taj_map.erase(7);
            taj_map.erase(6);
            taj_map.erase(5);
            taj_map.erase(4);
            taj_map.erase(3);

            DeleteKey(CreatePropertyPathHandle(TestATrait::kPropertyHandle_TaJ_Value, 7));
            DeleteKey(CreatePropertyPathHandle(TestATrait::kPropertyHandle_TaJ_Value, 6));
            DeleteKey(CreatePropertyPathHandle(TestATrait::kPropertyHandle_TaJ_Value, 5));
            DeleteKey(CreatePropertyPathHandle(TestATrait::kPropertyHandle_TaJ_Value, 4));
            DeleteKey(CreatePropertyPathHandle(TestATrait::kPropertyHandle_TaJ_Value, 3));
        }
        else if ((mTestCounter % 14) == 8) {
            taj_map.erase(2);
            DeleteKey(CreatePropertyPathHandle(TestATrait::kPropertyHandle_TaJ_Value, 2));

            taj_map[3] = { 303, false };
            SetDirty(CreatePropertyPathHandle(TestATrait::kPropertyHandle_TaJ_Value, 3));
        }
        else if ((mTestCounter % 14) == 9) {
            TestATrait::StructA tmp = taj_map[3];

            taj_map.erase(3);
            DeleteKey(CreatePropertyPathHandle(TestATrait::kPropertyHandle_TaJ_Value, 3));

            taj_map[3] = { tmp.saA + 100, tmp.saB };
            SetDirty(CreatePropertyPathHandle(TestATrait::kPropertyHandle_TaJ_Value, 3));
        }
        else if ((mTestCounter % 14) == 10) {
            taj_map[2].saA += 100;
            taj_map[2].saB = !taj_map[3].saB;
            SetDirty(CreatePropertyPathHandle(TestATrait::kPropertyHandle_TaJ_Value, 2));

            taj_map.erase(2);
            DeleteKey(CreatePropertyPathHandle(TestATrait::kPropertyHandle_TaJ_Value, 2));
        }
        else if ((mTestCounter % 14) == 11) {
            taj_map.erase(3);
            tai_map.erase(3);
            DeleteKey(CreatePropertyPathHandle(TestATrait::kPropertyHandle_TaJ_Value, 3));
            DeleteKey(CreatePropertyPathHandle(TestATrait::kPropertyHandle_TaI_Value, 3));
        }
        else if ((mTestCounter % 14) == 12) {
            taj_map[3] = { 303, false };
            tai_map[3] = 103;
            SetDirty(CreatePropertyPathHandle(TestATrait::kPropertyHandle_TaJ_Value, 3));
            SetDirty(CreatePropertyPathHandle(TestATrait::kPropertyHandle_TaI_Value, 3));
        }
        else if ((mTestCounter % 14) == 13) {
            taj_map.clear();
            tai_map.clear();

            for (int i = 0; i < 4; i++) {
                tai_map[i] = 100 + i;
            }

            SetDirty(TestATrait::kPropertyHandle_Root);
        }
    }

    mTestCounter++;
    Unlock();
}

WEAVE_ERROR TestATraitDataSource::SetLeafData(PropertyPathHandle aLeafHandle, TLVReader &aReader)
{
    return WEAVE_ERROR_UNSUPPORTED_WEAVE_FEATURE;
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

WEAVE_ERROR TestATraitDataSource::GetNextDictionaryItemKey(PropertyPathHandle aDictionaryHandle, uintptr_t &aContext, PropertyDictionaryKey &aKey)
{
    static std::map<uint16_t, uint32_t>::iterator tai_iter;
    static std::map<uint16_t, TestATrait::StructA>::iterator taj_iter;

    if (aDictionaryHandle == TestATrait::kPropertyHandle_TaI) {
        return GetNextDictionaryItemKeyHelper(&tai_map, tai_iter, aContext, aKey);
    }
    else if (aDictionaryHandle == TestATrait::kPropertyHandle_TaJ) {
        return GetNextDictionaryItemKeyHelper(&taj_map, taj_iter, aContext, aKey);
    }
    else {
        return WEAVE_ERROR_INVALID_ARGUMENT;
    }
}

void TestATraitDataSource::SetNullifiedPath(PropertyPathHandle aHandle, bool isNull)
{
    if (aHandle <= TestATrait::kPropertyHandle_TaJ_Value_SaB)
    {
        if (nullified_path[aHandle - 1] != isNull)
        {
            nullified_path[aHandle - 1] = isNull;
            SetDirty(aHandle);
        }
    }
}

WEAVE_ERROR TestATraitDataSource::GetData(PropertyPathHandle aHandle,
                                          uint64_t aTagToWrite,
                                          TLVWriter &aWriter,
                                          bool &aIsNull,
                                          bool &aIsPresent)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (mSchemaEngine->IsNullable(aHandle))
    {
        aIsNull = nullified_path[GetPropertySchemaHandle(aHandle) - 1];
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

WEAVE_ERROR TestATraitDataSource::GetLeafData(PropertyPathHandle aLeafHandle, uint64_t aTagToWrite, TLVWriter &aWriter)
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
        {
            err = aWriter.Put(aTagToWrite, tal);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  ta_l = %x", tal);
            break;
        }

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
            WeaveLogDetail(DataManagement, ">>  UNKNOWN! %08x", aLeafHandle);
    }

exit:
    WeaveLogFunctError(err);
    return err;
}

void TestATraitDataSource::HandleCommandOperationTimeout(nl::Weave::System::Layer* aSystemLayer, void *aAppState,
        nl::Weave::System::Error aErr)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TestATraitDataSource * const datasource = reinterpret_cast<TestATraitDataSource *>(aAppState);

    WeaveLogDetail(DataManagement, "Test trait A %s", __func__);

    VerifyOrExit (NULL != datasource->mActiveCommand, err = WEAVE_ERROR_INCORRECT_STATE);

    // send back response for command type 2
    {

        PacketBuffer *msgBuf = PacketBuffer::New();
        if (NULL == msgBuf)
        {
            // It's unlikely we'll have packet buffer to send out the status report, but let us try anyways
            err = datasource->mActiveCommand->SendError(nl::Weave::Profiles::kWeaveProfile_Common,
                    nl::Weave::Profiles::Common::kStatus_OutOfMemory, WEAVE_ERROR_NO_MEMORY);
            datasource->mActiveCommand = NULL;
            ExitNow ();
        }

        // Add response data here
        {
            nl::Weave::TLV::TLVWriter writer;
            nl::Weave::TLV::TLVType dummyType;

            writer.Init(msgBuf);

            err = writer.StartContainer(nl::Weave::TLV::AnonymousTag, nl::Weave::TLV::kTLVType_Structure, dummyType);
            SuccessOrExit(err);

            err = writer.Put(nl::Weave::TLV::ContextTag(1), (datasource->mCommandParam_1) + 1);
            SuccessOrExit(err);
            err = writer.PutBoolean(nl::Weave::TLV::ContextTag(2), !(datasource->mCommandParam_2));
            SuccessOrExit(err);

            err = writer.EndContainer(dummyType);
            SuccessOrExit(err);

            err = writer.Finalize();
            SuccessOrExit(err);
        }

        // send using WRM only if the peer has asked for it
        err = datasource->mActiveCommand->SendResponse(datasource->GetVersion(), msgBuf);
        SuccessOrExit(err);

        datasource->mActiveCommand = NULL;
    }

exit:
    WeaveLogFunctError(err);

    if (NULL != datasource->mActiveCommand)
    {
        datasource->mActiveCommand->Close();
        datasource->mActiveCommand = NULL;
    }
}

void TestATraitDataSource::OnCustomCommand(nl::Weave::Profiles::DataManagement::Command * aCommand,
            const nl::Weave::WeaveMessageInfo * aMsgInfo,
            nl::Weave::PacketBuffer * aPayload,
            const uint64_t & aCommandType,
            const bool aIsExpiryTimeValid,
            const int64_t & aExpiryTimeMicroSecond,
            const bool aIsMustBeVersionValid,
            const uint64_t & aMustBeVersion,
            nl::Weave::TLV::TLVReader & aArgumentReader)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint32_t reportProfileId = nl::Weave::Profiles::kWeaveProfile_Common;
    uint16_t reportStatusCode = nl::Weave::Profiles::Common::kStatus_BadRequest;

    WeaveLogDetail(DataManagement, "Test trait A %s", __func__);

    // verify there is no active command already running
    // this is needed only if the implementation cannot handle more than one concurrent command per trait
    if (NULL != mActiveCommand)
    {
        // we already have one active command. Reject this new one directly
        reportProfileId = nl::Weave::Profiles::kWeaveProfile_Common;
        reportStatusCode = nl::Weave::Profiles::Common::kStatus_OutOfMemory;
        err = WEAVE_ERROR_NO_MEMORY;
        ExitNow ();
    }

    // verify if this command comes with a valid EC with the right peer node ID, key ID, and authenticator
    // more detailed example will be available when we have the security added

    // Note that the version check is passed to application layer because the application layer might want to know
    // someone is making a request.
    if (aIsMustBeVersionValid)
    {
        WeaveLogDetail(DataManagement, "Actual version is 0x%" PRIx64 ", while must-be version is: 0x%" PRIx64, GetVersion(), aMustBeVersion);

        if (aMustBeVersion != GetVersion())
        {
            reportProfileId = nl::Weave::Profiles::kWeaveProfile_WDM;
            reportStatusCode = kStatus_VersionMismatch;
            ExitNow ();
        }
    }

    WeaveLogDetail(DataManagement, "Command Type ID 0x%" PRIx64, aCommandType);

    // verify the command type and arguments are valid
    // command type 1: one shot signaling without custom data in response
    if (1 == aCommandType)
    {
        // Parse and validate the arguments according to schema definitions for this command
        {
            nl::Weave::TLV::TLVType OuterContainerType;
            err = aArgumentReader.EnterContainer(OuterContainerType);
            SuccessOrExit(err);

            while (WEAVE_NO_ERROR == (err = aArgumentReader.Next()))
            {
                // usually there is only context-specific tags in argument section
                VerifyOrExit(nl::Weave::TLV::IsContextTag(aArgumentReader.GetTag()), err = WEAVE_ERROR_INVALID_TLV_TAG);
                switch (nl::Weave::TLV::TagNumFromTag(aArgumentReader.GetTag()))
                {
                case kCmdParam_1:
                    err = aArgumentReader.Get(mCommandParam_1);
                    SuccessOrExit(err);
                    WeaveLogDetail(DataManagement, "Parameter 1: 0x%" PRIx32, mCommandParam_1);
                    break;

                case kCmdParam_2:
                    err = aArgumentReader.Get(mCommandParam_2);
                    SuccessOrExit(err);
                    WeaveLogDetail(DataManagement, "Parameter 2: %d", mCommandParam_2);
                    break;

                default:
                    // unrecognized arguments are allowed or not is a trait-specific question
                    ExitNow(err = WEAVE_ERROR_INVALID_TLV_TAG);
                }
            }

            if (WEAVE_END_OF_TLV == err)
            {
                // if any of the parameters are mandatory, we can verify at here
                err = WEAVE_NO_ERROR;
            }
            SuccessOrExit(err);
        }

        // Free the packet buffer after we parsed and processed/cached all arguments.
        // Doing this before allocating buffer for response might help reduce the max number of packet buffer needed
        PacketBuffer::Free(aPayload);
        aPayload = NULL;

        // Generate a success response right here
        {

            PacketBuffer * msgBuf = PacketBuffer::New();
            if (NULL == msgBuf)
            {
                // It's unlikely we'll have packet buffer to send out the status report, but let us try anyways
                reportProfileId = nl::Weave::Profiles::kWeaveProfile_Common;
                reportStatusCode = nl::Weave::Profiles::Common::kStatus_OutOfMemory;
                err = WEAVE_ERROR_NO_MEMORY;
                ExitNow ();
            }

            aCommand->SendResponse(GetVersion(), msgBuf);
            aCommand = NULL;
            msgBuf = NULL;
        }
    }
    // command type 2: with delayed, verifiable custom data in response
    else if (2 == aCommandType)
    {
        // Parse and validate the arguments according to schema definitions for this command
        {
            nl::Weave::TLV::TLVType OuterContainerType;
            err = aArgumentReader.EnterContainer(OuterContainerType);
            SuccessOrExit(err);

            while (WEAVE_NO_ERROR == (err = aArgumentReader.Next()))
            {
                // usually there is only context-specific tags in argument section
                VerifyOrExit(nl::Weave::TLV::IsContextTag(aArgumentReader.GetTag()), err = WEAVE_ERROR_INVALID_TLV_TAG);
                switch (nl::Weave::TLV::TagNumFromTag(aArgumentReader.GetTag()))
                {
                case kCmdParam_1:
                    err = aArgumentReader.Get(mCommandParam_1);
                    SuccessOrExit(err);
                    WeaveLogDetail(DataManagement, "Parameter 1: 0x%" PRIx32, mCommandParam_1);
                    break;

                case kCmdParam_2:
                    err = aArgumentReader.Get(mCommandParam_2);
                    SuccessOrExit(err);
                    WeaveLogDetail(DataManagement, "Parameter 2: %d", mCommandParam_2);
                    break;

                default:
                    // unrecognized arguments are allowed or not is a trait-specific question
                    ExitNow(err = WEAVE_ERROR_INVALID_TLV_TAG);
                }
            }

            if (WEAVE_END_OF_TLV == err)
            {
                // if any of the parameters are mandatory, we can verify at here
                err = WEAVE_NO_ERROR;
            }
            SuccessOrExit(err);
        }

        // Free the packet buffer after we parsed and processed/cached all arguments.
        // Doing this before allocating buffer for response might help reduce the max number of packet buffer needed
        PacketBuffer::Free(aPayload);
        aPayload = NULL;

        // send back response later
        // note this is just an example of some operation which would take 2 seconds to complete
        // if the requested operation can be finished without delay, a response can be sent from here
        err = aCommand->GetExchangeContext()->ExchangeMgr->MessageLayer->SystemLayer->StartTimer(2000, HandleCommandOperationTimeout, this);
        SuccessOrExit(err);

        err = aCommand->SendInProgress();
        SuccessOrExit(err);

        // transfer ownership
        mActiveCommand = aCommand;
        aCommand = NULL;
    }
    else
    {
        // unrecognized command type id
        // default error is bad request
        err = WEAVE_ERROR_NOT_IMPLEMENTED;
        ExitNow();
    }


exit:
    WeaveLogFunctError(err);

    if (NULL != aCommand)
    {
        aCommand->SendError(reportProfileId, reportStatusCode, err);
        aCommand = NULL;
    }

    if (aPayload)
    {
        PacketBuffer::Free(aPayload);
        aPayload = NULL;
    }
}

TestBTraitDataSource::TestBTraitDataSource()
    : TraitDataSource(&TestBTrait::TraitSchema)
{
    mVersion = 200;
    taa = TestATrait::ENUM_A_VALUE_1;
    tab = TestCommonTrait::COMMON_ENUM_A_VALUE_1;
    tac = 3;
    tad_saa = 4;
    tad_sab = true;

    for (size_t i = 0; i < (sizeof(tae) / sizeof(tae[0])); i++) {
        tae[i] = i + 5;
    }

    tap = 101;

    tba = 200;
    MOCK_strlcpy(tbb_sba, "testing", sizeof(tbb_sba));
    tbb_sbb = 201;

    tbc_saa = 202;
    tbc_sab = false;
    MOCK_strlcpy(tbc_seac, "hallo", sizeof(tbc_seac));
}

void TestBTraitDataSource::Mutate()
{
    Lock();

    if ((GetVersion() % 3) == 0) {
        SetDirty(TestBTrait::kPropertyHandle_TbB_SbB);
        SetDirty(TestBTrait::kPropertyHandle_TaA);

        if (taa == TestATrait::ENUM_A_VALUE_1) {
            taa = TestATrait::ENUM_A_VALUE_2;
        }
        else {
            taa = TestATrait::ENUM_A_VALUE_1;
        }

        tbb_sbb++;
    }
    else if ((GetVersion() % 3) == 1) {
        SetDirty(TestBTrait::kPropertyHandle_TaC);
        SetDirty(TestBTrait::kPropertyHandle_TaP);
        SetDirty(TestBTrait::kPropertyHandle_TbC_SaB);

        tap++;
        tac++;
        tbc_sab = !tbc_sab;
    }
    else if ((GetVersion() % 3) == 2) {
        SetDirty(TestBTrait::kPropertyHandle_TaP);

        tap++;
    }

    Unlock();
}

WEAVE_ERROR TestBTraitDataSource::SetLeafData(PropertyPathHandle aLeafHandle, TLVReader &aReader)
{
    return WEAVE_ERROR_UNSUPPORTED_WEAVE_FEATURE;
}

WEAVE_ERROR TestBTraitDataSource::GetLeafData(PropertyPathHandle aLeafHandle, uint64_t aTagToWrite, TLVWriter &aWriter)
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

        case TestBTrait::kPropertyHandle_TaP:
            err = aWriter.Put(aTagToWrite, tap);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  ta_p = %" PRId64, tap);
            break;

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
            WeaveLogDetail(DataManagement, ">>  UNKNOWN! %08x", aLeafHandle);
    }

exit:
    WeaveLogFunctError(err);
    return err;
}

WEAVE_ERROR TestBTraitDataSource::GetNextDictionaryItemKey(PropertyPathHandle aDictionaryHandle, uintptr_t &aContext, PropertyDictionaryKey &aKey)
{
    return WEAVE_END_OF_INPUT;
}

TestBLargeTraitDataSource::TestBLargeTraitDataSource()
        : TraitDataSource(&TestBTrait::TraitSchema)
{
    mVersion = 200;
    taa = TestATrait::ENUM_A_VALUE_1;
    tab = TestCommonTrait::COMMON_ENUM_A_VALUE_1;
    tac = 3;
    tad_saa = 4;
    tad_sab = true;

    for (size_t i = 0; i < (sizeof(tae) / sizeof(tae[0])); i++) {
        tae[i] = i + 5;
    }

    tap = 0;

    tba = 200;
    MOCK_strlcpy(tbb_sba, "testing", sizeof(tbb_sba));
    tbb_sbb = 201;

    tbc_saa = 202;
    tbc_sab = false;
    MOCK_strlcpy(tbc_seac, "hallo", sizeof(tbc_seac));
}

void TestBLargeTraitDataSource::Mutate()
{
    Lock();

    if ((GetVersion() % 3) == 0) {
        SetDirty(TestBTrait::kPropertyHandle_TbB_SbB);
        SetDirty(TestBTrait::kPropertyHandle_TaA);

        if (taa == TestATrait::ENUM_A_VALUE_1) {
            taa = TestATrait::ENUM_A_VALUE_2;
        }
        else {
            taa = TestATrait::ENUM_A_VALUE_1;
        }

        tbb_sbb++;
    }
    else if ((GetVersion() % 3) == 1) {
        SetDirty(TestBTrait::kPropertyHandle_TaC);
        SetDirty(TestBTrait::kPropertyHandle_TaP);
        SetDirty(TestBTrait::kPropertyHandle_TbC_SaB);

        tap++;
        tac++;
        tbc_sab = !tbc_sab;
    }
    else if ((GetVersion() % 3) == 2) {
        SetDirty(TestBTrait::kPropertyHandle_TaP);

        tap++;
    }

    Unlock();
}

WEAVE_ERROR TestBLargeTraitDataSource::SetLeafData(PropertyPathHandle aLeafHandle, TLVReader &aReader)
{
    return WEAVE_ERROR_UNSUPPORTED_WEAVE_FEATURE;
}

WEAVE_ERROR TestBLargeTraitDataSource::GetLeafData(PropertyPathHandle aLeafHandle, uint64_t aTagToWrite, TLVWriter &aWriter)
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

        case TestBTrait::kPropertyHandle_TaP:
            err = aWriter.Put(aTagToWrite, tap);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, ">>  ta_p = %d", tap);
            break;

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

WEAVE_ERROR TestBLargeTraitDataSource::GetNextDictionaryItemKey(PropertyPathHandle aDictionaryHandle, uintptr_t &aContext, PropertyDictionaryKey &aKey)
{
    return WEAVE_END_OF_INPUT;
}

ApplicationKeysTraitDataSource::ApplicationKeysTraitDataSource(void)
    : TraitDataSource(&ApplicationKeysTrait::TraitSchema)
{
    mVersion = kInitialTraitVersionNumber;

    ClearEpochKeys();
    ClearGroupMasterKeys();

    AddEpochKey(sEpochKey1_Number, sEpochKey1_StartTime, sEpochKey1_Key, sEpochKey1_KeyLen);
    AddGroupMasterKey(sAppGroupMasterKey4_Number, sAppGroupMasterKey4_GlobalId, sAppGroupMasterKey4_Key, sAppGroupMasterKey4_KeyLen);
}

WEAVE_ERROR ApplicationKeysTraitDataSource::Mutate(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    bool mutateEpochKeys = true;
    bool mutateGroupMasterKeys = true;

    Lock();


    switch (GetVersion() % 3) {
        case 0:
            mutateEpochKeys = true;
            mutateGroupMasterKeys = false;
            break;

        case 1:
            mutateEpochKeys = false;
            mutateGroupMasterKeys = true;
            break;

        case 2:
            mutateEpochKeys = true;
            mutateGroupMasterKeys = true;
            break;
    }

    if (mutateEpochKeys)
    {
        err = MutateEpochKeys();
        SuccessOrExit(err);
    }

    if (mutateGroupMasterKeys)
    {
        err = MutateGroupMasterKeys();
        SuccessOrExit(err);
    }

exit:
    Unlock();

    return err;
}

WEAVE_ERROR ApplicationKeysTraitDataSource::MutateEpochKeys(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    ClearEpochKeys();

    // Picked 6 pairs of epoch keys, which rotate in the following order:
    // {0, 1} --> {1, 2} --> {2, 3} --> {3, 4} --> {4, 5} --> {5, 0}
    switch (GetVersion() % 6) {
        case 0:
            err = AddEpochKey(sEpochKey0_Number, sEpochKey0_StartTime, sEpochKey0_Key, sEpochKey0_KeyLen);
            SuccessOrExit(err);

            err = AddEpochKey(sEpochKey1_Number, sEpochKey1_StartTime, sEpochKey1_Key, sEpochKey1_KeyLen);
            SuccessOrExit(err);
            break;

        case 1:
            err = AddEpochKey(sEpochKey1_Number, sEpochKey1_StartTime, sEpochKey1_Key, sEpochKey1_KeyLen);
            SuccessOrExit(err);

            err = AddEpochKey(sEpochKey2_Number, sEpochKey2_StartTime, sEpochKey2_Key, sEpochKey2_KeyLen);
            SuccessOrExit(err);
            break;

        case 2:
            err = AddEpochKey(sEpochKey2_Number, sEpochKey2_StartTime, sEpochKey2_Key, sEpochKey2_KeyLen);
            SuccessOrExit(err);

            err = AddEpochKey(sEpochKey3_Number, sEpochKey3_StartTime, sEpochKey3_Key, sEpochKey3_KeyLen);
            SuccessOrExit(err);
            break;

        case 3:
            err = AddEpochKey(sEpochKey3_Number, sEpochKey3_StartTime, sEpochKey3_Key, sEpochKey3_KeyLen);
            SuccessOrExit(err);

            err = AddEpochKey(sEpochKey4_Number, sEpochKey4_StartTime, sEpochKey4_Key, sEpochKey4_KeyLen);
            SuccessOrExit(err);
            break;

        case 4:
            err = AddEpochKey(sEpochKey4_Number, sEpochKey4_StartTime, sEpochKey4_Key, sEpochKey4_KeyLen);
            SuccessOrExit(err);

            err = AddEpochKey(sEpochKey5_Number, sEpochKey5_StartTime, sEpochKey5_Key, sEpochKey5_KeyLen);
            SuccessOrExit(err);
            break;

        case 5:
            err = AddEpochKey(sEpochKey5_Number, sEpochKey5_StartTime, sEpochKey5_Key, sEpochKey5_KeyLen);
            SuccessOrExit(err);

            err = AddEpochKey(sEpochKey0_Number, sEpochKey0_StartTime, sEpochKey0_Key, sEpochKey0_KeyLen);
            SuccessOrExit(err);
            break;
    }

exit:
    SetDirty(ApplicationKeysTrait::kPropertyHandle_EpochKeys);

    return err;
}

WEAVE_ERROR ApplicationKeysTraitDataSource::MutateGroupMasterKeys(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    ClearGroupMasterKeys();

    // Picked 5 (arbitrary) sets of group master keys to test key update fanctionality.
    switch (GetVersion() % 5) {
        case 0:
            err = AddGroupMasterKey(sAppGroupMasterKey10_Number, sAppGroupMasterKey10_GlobalId, sAppGroupMasterKey10_Key, sAppGroupMasterKey10_KeyLen);
            SuccessOrExit(err);

            err = AddGroupMasterKey(sAppGroupMasterKey7_Number, sAppGroupMasterKey7_GlobalId, sAppGroupMasterKey7_Key, sAppGroupMasterKey7_KeyLen);
            SuccessOrExit(err);
            break;

        case 1:
            err = AddGroupMasterKey(sAppGroupMasterKey0_Number, sAppGroupMasterKey0_GlobalId, sAppGroupMasterKey0_Key, sAppGroupMasterKey0_KeyLen);
            SuccessOrExit(err);
            break;

        case 2:
            err = AddGroupMasterKey(sAppGroupMasterKey4_Number, sAppGroupMasterKey4_GlobalId, sAppGroupMasterKey4_Key, sAppGroupMasterKey4_KeyLen);
            SuccessOrExit(err);

            err = AddGroupMasterKey(sAppGroupMasterKey7_Number, sAppGroupMasterKey7_GlobalId, sAppGroupMasterKey7_Key, sAppGroupMasterKey7_KeyLen);
            SuccessOrExit(err);

            err = AddGroupMasterKey(sAppGroupMasterKey54_Number, sAppGroupMasterKey54_GlobalId, sAppGroupMasterKey54_Key, sAppGroupMasterKey54_KeyLen);
            SuccessOrExit(err);
            break;

        case 3:
            err = AddGroupMasterKey(sAppGroupMasterKey10_Number, sAppGroupMasterKey10_GlobalId, sAppGroupMasterKey10_Key, sAppGroupMasterKey10_KeyLen);
            SuccessOrExit(err);

            err = AddGroupMasterKey(sAppGroupMasterKey4_Number, sAppGroupMasterKey4_GlobalId, sAppGroupMasterKey4_Key, sAppGroupMasterKey4_KeyLen);
            SuccessOrExit(err);

            err = AddGroupMasterKey(sAppGroupMasterKey7_Number, sAppGroupMasterKey7_GlobalId, sAppGroupMasterKey7_Key, sAppGroupMasterKey7_KeyLen);
            SuccessOrExit(err);

            err = AddGroupMasterKey(sAppGroupMasterKey54_Number, sAppGroupMasterKey54_GlobalId, sAppGroupMasterKey54_Key, sAppGroupMasterKey54_KeyLen);
            SuccessOrExit(err);
            break;

        case 4:
            break;
    }

exit:
    SetDirty(ApplicationKeysTrait::kPropertyHandle_MasterKeys);

    return err;
}

void ApplicationKeysTraitDataSource::ClearEpochKeys(void)
{
    memset(reinterpret_cast<uint8_t *>(&EpochKeys), 0, sizeof(EpochKeys));
}

void ApplicationKeysTraitDataSource::ClearGroupMasterKeys(void)
{
    memset(reinterpret_cast<uint8_t *>(&GroupMasterKeys), 0, sizeof(GroupMasterKeys));
}

WEAVE_ERROR ApplicationKeysTraitDataSource::AddEpochKey(uint8_t epochKeyNumber, uint32_t startTime, const uint8_t *key, uint8_t keyLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint32_t keyId = WeaveKeyId::MakeEpochKeyId(epochKeyNumber);
    int keyInd = WEAVE_CONFIG_MAX_APPLICATION_EPOCH_KEYS;

    VerifyOrExit(keyLen <= sizeof(EpochKeys[0].Key), err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Find key entry that stores group epoch key with the same Id (then override the value) or find the first unused key entry.
    for (int i = 0; i < WEAVE_CONFIG_MAX_APPLICATION_EPOCH_KEYS; ++i)
    {
        if (EpochKeys[i].KeyId == keyId)
        {
            keyInd = i;
            break;
        }
        if (keyInd == WEAVE_CONFIG_MAX_APPLICATION_EPOCH_KEYS && EpochKeys[i].KeyId == WeaveKeyId::kNone)
        {
            keyInd = i;
        }
    }

    VerifyOrExit(keyInd < WEAVE_CONFIG_MAX_APPLICATION_EPOCH_KEYS, err = WEAVE_ERROR_NO_MEMORY);

    EpochKeys[keyInd].KeyId = keyId;
    EpochKeys[keyInd].KeyLen = keyLen;
    EpochKeys[keyInd].StartTime = startTime;
    memcpy(EpochKeys[keyInd].Key, key, keyLen);

exit:
    return err;
}

WEAVE_ERROR ApplicationKeysTraitDataSource::AddGroupMasterKey(uint8_t appGroupLocalNumber, uint32_t globalId, const uint8_t *key, uint8_t keyLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint32_t keyId = WeaveKeyId::MakeAppGroupMasterKeyId(appGroupLocalNumber);
    int keyInd = WEAVE_CONFIG_MAX_APPLICATION_GROUPS;

    VerifyOrExit(keyLen <= sizeof(GroupMasterKeys[0].Key), err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Find key entry that stores group master key with the same Id (then override the value) or find the first unused key entry.
    for (int i = 0; i < WEAVE_CONFIG_MAX_APPLICATION_GROUPS; ++i)
    {
        if (GroupMasterKeys[i].KeyId == keyId)
        {
            keyInd = i;
            break;
        }
        if (keyInd == WEAVE_CONFIG_MAX_APPLICATION_GROUPS && GroupMasterKeys[i].KeyId == WeaveKeyId::kNone)
        {
            keyInd = i;
        }
    }

    VerifyOrExit(keyInd < WEAVE_CONFIG_MAX_APPLICATION_GROUPS, err = WEAVE_ERROR_NO_MEMORY);

    GroupMasterKeys[keyInd].KeyId = keyId;
    GroupMasterKeys[keyInd].KeyLen = keyLen;
    GroupMasterKeys[keyInd].GlobalId = globalId;
    memcpy(GroupMasterKeys[keyInd].Key, key, keyLen);

exit:
    return err;
}

WEAVE_ERROR ApplicationKeysTraitDataSource::GetNextDictionaryItemKey(PropertyPathHandle aDictionaryHandle, uintptr_t &aContext, PropertyDictionaryKey &aKey)
{
    return WEAVE_END_OF_INPUT;
}

WEAVE_ERROR ApplicationKeysTraitDataSource::SetLeafData(PropertyPathHandle aLeafHandle, TLVReader &aReader)
{
    return WEAVE_ERROR_UNSUPPORTED_WEAVE_FEATURE;
}

WEAVE_ERROR ApplicationKeysTraitDataSource::GetLeafData(PropertyPathHandle aLeafHandle, uint64_t aTagToWrite, TLVWriter &aWriter)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TLVType outerContainerType;

    err = aWriter.StartContainer(aTagToWrite, kTLVType_Array, outerContainerType);
    SuccessOrExit(err);

    if (ApplicationKeysTrait::kPropertyHandle_EpochKeys == aLeafHandle)
    {
        for (int i = 0; i < WEAVE_CONFIG_MAX_APPLICATION_EPOCH_KEYS; ++i)
        {
            if (EpochKeys[i].KeyId != WeaveKeyId::kNone)
            {
                TLVType containerType2;

                err = aWriter.StartContainer(AnonymousTag, kTLVType_Structure, containerType2);
                SuccessOrExit(err);

                const uint32_t epochKeyNumber = WeaveKeyId::GetEpochKeyNumber(EpochKeys[i].KeyId);
                err = aWriter.Put(ContextTag(kTag_EpochKey_KeyId), epochKeyNumber);
                SuccessOrExit(err);

                err = aWriter.Put(ContextTag(kTag_EpochKey_StartTime), (static_cast<int64_t>(EpochKeys[i].StartTime) * 1000));
                SuccessOrExit(err);

                err = aWriter.PutBytes(ContextTag(kTag_EpochKey_Key), EpochKeys[i].Key, EpochKeys[i].KeyLen);
                SuccessOrExit(err);

                err = aWriter.EndContainer(containerType2);
                SuccessOrExit(err);

                WeaveLogDetail(DataManagement, ">>  GroupEpochKeyId = %08" PRIX32, EpochKeys[i].KeyId);
            }
        }
    }
    else if (ApplicationKeysTrait::kPropertyHandle_MasterKeys == aLeafHandle)
    {
        for (int i = 0; i < WEAVE_CONFIG_MAX_APPLICATION_GROUPS; ++i)
        {
            if (GroupMasterKeys[i].KeyId != WeaveKeyId::kNone)
            {
                TLVType containerType2;

                err = aWriter.StartContainer(AnonymousTag, kTLVType_Structure, containerType2);
                SuccessOrExit(err);

                err = aWriter.Put(ContextTag(kTag_ApplicationGroup_GlobalId), GroupMasterKeys[i].GlobalId);
                SuccessOrExit(err);

                const uint32_t appGroupLocalNumber = WeaveKeyId::GetAppGroupLocalNumber(GroupMasterKeys[i].KeyId);
                err = aWriter.Put(ContextTag(kTag_ApplicationGroup_ShortId), appGroupLocalNumber);
                SuccessOrExit(err);

                err = aWriter.PutBytes(ContextTag(kTag_ApplicationGroup_Key), GroupMasterKeys[i].Key, GroupMasterKeys[i].KeyLen);
                SuccessOrExit(err);

                err = aWriter.EndContainer(containerType2);
                SuccessOrExit(err);

                WeaveLogDetail(DataManagement, ">>  GroupMasterKeyId = %08" PRIX32, GroupMasterKeys[i].KeyId);
            }
        }
    }
    else
    {
        WeaveLogDetail(DataManagement, "<<  UNKNOWN!");
        ExitNow(err = WEAVE_ERROR_INVALID_TLV_TAG);
    }

    err = aWriter.EndContainer(outerContainerType);
    SuccessOrExit(err);

exit:
    WeaveLogFunctError(err);

    return err;
}
