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

#include <Weave/Profiles/data-management/Current/WdmManagedNamespace.h>

#include <Weave/Support/CodeUtils.h>

#include "MockSinkTrait.h"


using namespace ::nl::Weave;
using namespace ::nl::Weave::TLV;
using namespace ::nl::Weave::Profiles::DataManagement;

using namespace ::nl::Weave::Profiles::Locale;
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


MockTraitUpdatableDataSink::MockTraitUpdatableDataSink(const TraitSchemaEngine * aEngine)
        : TraitUpdatableDataSink(aEngine)
{
}

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

WEAVE_ERROR LocaleSettingsTraitUpdatableDataSink::Mutate(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    static unsigned int whichLocale = 0;
    static const char * locales[] = { "en-US", "zh-TW", "ja-JP", "pl-PL", "zh-CN" };
    PropertyPathHandle pathHandle = kNullPropertyPathHandle;

    MOCK_strlcpy(mLocale, locales[whichLocale], sizeof(mLocale));
    whichLocale = (whichLocale + 1) % (sizeof(locales)/sizeof(locales[0]));


    pathHandle = LocaleSettingsTrait::kPropertyHandle_active_locale;


    WeaveLogDetail(DataManagement, "<set updated> in 0x%08x", pathHandle);

    return err;
}

WEAVE_ERROR LocaleSettingsTraitUpdatableDataSink::GetNextDictionaryItemKey(PropertyPathHandle aDictionaryHandle, uintptr_t &aContext, PropertyDictionaryKey &aKey)
{
    return WEAVE_END_OF_INPUT;
}
