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
 *      Sample mock trait data sinks that implement the simple and complex mock traits.
 *
 */

#ifndef MOCK_TRAIT_SINKS_H_
#define MOCK_TRAIT_SINKS_H_
#define MAX_ARRAY_LEN 10
#define MAX_ARRAY_SIZE sizeof(char) * MAX_ARRAY_LEN
#define MAX_LOCALE_SIZE sizeof(char) * 24

// We want and assume the default managed namespace is Current and that is, explicitly, the managed namespace this code desires.
#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Profiles/data-management/SubscriptionClient.h>

#include "LocaleSettingsTrait.h"

#include <map>

using namespace ::nl::Weave::Profiles::DataManagement_Current;

class MockTraitUpdatableDataSink : public nl::Weave::Profiles::DataManagement::TraitUpdatableDataSink
{
public:
    MockTraitUpdatableDataSink(const nl::Weave::Profiles::DataManagement::TraitSchemaEngine * aEngine);
    void ResetDataSink(void) { ClearVersion(); };
};

class LocaleSettingsTraitUpdatableDataSink : public MockTraitUpdatableDataSink
{
public:
    LocaleSettingsTraitUpdatableDataSink();
    WEAVE_ERROR Mutate(void);
private:
    WEAVE_ERROR SetLeafData(nl::Weave::Profiles::DataManagement::PropertyPathHandle aLeafHandle, nl::Weave::TLV::TLVReader &aReader) __OVERRIDE;
    WEAVE_ERROR GetLeafData(nl::Weave::Profiles::DataManagement::PropertyPathHandle aLeafHandle, uint64_t aTagToWrite, nl::Weave::TLV::TLVWriter &aWriter) __OVERRIDE;
    WEAVE_ERROR GetNextDictionaryItemKey(nl::Weave::Profiles::DataManagement::PropertyPathHandle aDictionaryHandle, uintptr_t &aContext, nl::Weave::Profiles::DataManagement::PropertyDictionaryKey &aKey) __OVERRIDE;

    enum
    {
        kMaxNumOfCharsPerLocale = 24
    };
    char mLocale[kMaxNumOfCharsPerLocale];
};

#endif // MOCK_TRAIT_SINKS_H_
