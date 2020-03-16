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
#include <Weave/Profiles/security/ApplicationKeysTraitDataSink.h>

#include "MockWdmNodeOptions.h"

#include <weave/trait/security/BoltLockSettingsTrait.h>
#include <weave/trait/locale/LocaleSettingsTrait.h>
#include <weave/trait/locale/LocaleCapabilitiesTrait.h>
#include <nest/test/trait/TestATrait.h>
#include <nest/test/trait/TestBTrait.h>
#include <nest/test/trait/TestCommon.h>

#include <map>

using namespace ::nl::Weave::Profiles::DataManagement_Current;

class MockTraitDataSink : public nl::Weave::Profiles::DataManagement::TraitDataSink
{
public:
    MockTraitDataSink(const nl::Weave::Profiles::DataManagement::TraitSchemaEngine * aEngine);
    void ResetDataSink(void) { ClearVersion(); };
};

#if WEAVE_CONFIG_ENABLE_WDM_UPDATE
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
    WEAVE_ERROR Mutate(SubscriptionClient * apSubClient,
                       bool aIsConditional,
                       MockWdmNodeOptions::WdmUpdateMutation aMutation);

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

namespace nl {
namespace Weave {
namespace Profiles {
namespace DataManagement_Current {
class    WdmUpdateEncoderTest;
class    WdmUpdateServerTest;
} // namespace DataManagement_Current
} // namespace Profiles
} // namespace Weave
} // namespace nl

class TestATraitUpdatableDataSink : public MockTraitUpdatableDataSink
{
public:
    enum
    {
        kNumMutations = 11
    };
    TestATraitUpdatableDataSink();

    WEAVE_ERROR OnEvent(uint16_t aType, void *aInParam) __OVERRIDE;

    WEAVE_ERROR Mutate(SubscriptionClient * apSubClient, bool aIsConditional, MockWdmNodeOptions::WdmUpdateMutation aMutation);
    uint32_t mTraitTestSet = 0;

private:
    friend class nl::Weave::Profiles::DataManagement_Current::WdmUpdateEncoderTest;
    friend class nl::Weave::Profiles::DataManagement_Current::WdmUpdateServerTest;
    void SetNullifiedPath(nl::Weave::Profiles::DataManagement::PropertyPathHandle aHandle, bool isNull);
    WEAVE_ERROR SetData(nl::Weave::Profiles::DataManagement::PropertyPathHandle aHandle, nl::Weave::TLV::TLVReader &aReader, bool aIsNull) __OVERRIDE;
    WEAVE_ERROR SetLeafData(nl::Weave::Profiles::DataManagement::PropertyPathHandle aLeafHandle, nl::Weave::TLV::TLVReader &aReader) __OVERRIDE;
    bool FindKey(uint32_t *aKeyList, uint32_t aListSize, nl::Weave::Profiles::DataManagement::PropertyPathHandle aHandle);
    WEAVE_ERROR GetData(nl::Weave::Profiles::DataManagement::PropertyPathHandle aHandle, uint64_t aTagToWrite, nl::Weave::TLV::TLVWriter &aWriter, bool &aIsNull, bool &aIsPresent) __OVERRIDE;
    WEAVE_ERROR GetLeafData(nl::Weave::Profiles::DataManagement::PropertyPathHandle aLeafHandle, uint64_t aTagToWrite, nl::Weave::TLV::TLVWriter &aWriter) __OVERRIDE;
    WEAVE_ERROR GetNextDictionaryItemKey(nl::Weave::Profiles::DataManagement::PropertyPathHandle aDictionaryHandle, uintptr_t &aContext, nl::Weave::Profiles::DataManagement::PropertyDictionaryKey &aKey) __OVERRIDE;

    int32_t taa;
    int32_t tab;
    uint32_t tac;
    Schema::Nest::Test::Trait::TestATrait::StructA tad;
    uint32_t tae[10];

    // weave.common.StringRef is implemented as a union
    char tag_string[20];
    uint16_t tag_ref;
    bool tag_use_ref;

    uint32_t tai_stageditem;
    std::map<uint16_t, uint32_t> tai_map;

    Schema::Nest::Test::Trait::TestATrait::StructA taj_stageditem;
    std::map<uint16_t, Schema::Nest::Test::Trait::TestATrait::StructA> taj_map;

    // byte array
    uint8_t tak[10];

    // day of week
    uint8_t tal;

    // implicit resourceid
    uint64_t tam_resourceid;
    // resource id and type
    uint8_t tan[10];
    uint16_t tan_type;

    uint32_t tao;

    int64_t tap; // milliseconds
    int64_t taq; // milliseconds
    uint32_t tar; // seconds
    uint32_t tas; // milliseconds

    uint32_t tat;
    int32_t tau;
    bool tav;
    char taw[20];
    // boxed float
    int16_t tax;

    bool nullified_path[Schema::Nest::Test::Trait::TestATrait::kPropertyHandle_TaJ_Value_SaB];
    uint32_t mTestCounter = 0;
};


class TestBTraitUpdatableDataSink : public MockTraitUpdatableDataSink
{
public:
    TestBTraitUpdatableDataSink();
    WEAVE_ERROR Mutate(SubscriptionClient * apSubClient, bool aIsConditional, MockWdmNodeOptions::WdmUpdateMutation aMutation);

private:
    void SetNullifiedPath(nl::Weave::Profiles::DataManagement::PropertyPathHandle aHandle, bool isNull);
    WEAVE_ERROR SetData(nl::Weave::Profiles::DataManagement::PropertyPathHandle aHandle, nl::Weave::TLV::TLVReader &aReader, bool aIsNull) __OVERRIDE;
    WEAVE_ERROR SetLeafData(nl::Weave::Profiles::DataManagement::PropertyPathHandle aLeafHandle, nl::Weave::TLV::TLVReader &aReader) __OVERRIDE;
    WEAVE_ERROR GetData(nl::Weave::Profiles::DataManagement::PropertyPathHandle aHandle, uint64_t aTagToWrite, nl::Weave::TLV::TLVWriter &aWriter, bool &aIsNull, bool &aIsPresent) __OVERRIDE;
    WEAVE_ERROR GetLeafData(nl::Weave::Profiles::DataManagement::PropertyPathHandle aLeafHandle, uint64_t aTagToWrite, nl::Weave::TLV::TLVWriter &aWriter) __OVERRIDE;
    WEAVE_ERROR GetNextDictionaryItemKey(nl::Weave::Profiles::DataManagement::PropertyPathHandle aDictionaryHandle, uintptr_t &aContext, nl::Weave::Profiles::DataManagement::PropertyDictionaryKey &aKey) __OVERRIDE;

    int32_t taa;
    int32_t tab;
    uint32_t tac;
    uint32_t tad_saa;
    bool tad_sab;
    uint32_t tae[500];
    char tai[10];
    int64_t tap;

    uint32_t tba;
    char tbb_sba[10];
    uint32_t tbb_sbb;
    uint32_t tbc_saa;
    bool tbc_sab;
    char tbc_seac[10];

    bool nullified_path[Schema::Nest::Test::Trait::TestBTrait::kPropertyHandle_TaJ_Value_SaB];
};

#endif // WEAVE_CONFIG_ENABLE_WDM_UPDATE

// This class is used to fetch data from sinks using existing data source APIs
class MockTraitDataSourceDelegate : public nl::Weave::Profiles::DataManagement::TraitSchemaEngine::IGetDataDelegate
{
public:
    WEAVE_ERROR GetData(nl::Weave::Profiles::DataManagement::PropertyPathHandle aHandle,
                        uint64_t aTagToWrite,
                        nl::Weave::TLV::TLVWriter &aWriter,
                        bool &aIsNull,
                        bool &aIsPresent) __OVERRIDE;
};

class LocaleSettingsTraitDataSink : public MockTraitDataSink,  public MockTraitDataSourceDelegate
{
public:
    LocaleSettingsTraitDataSink();

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

class LocaleCapabilitiesTraitDataSink : public MockTraitDataSink,  public MockTraitDataSourceDelegate
{
public:
    LocaleCapabilitiesTraitDataSink();

    WEAVE_ERROR OnEvent(uint16_t aType, void *aInParam) __OVERRIDE;

private:
    WEAVE_ERROR SetLeafData(nl::Weave::Profiles::DataManagement::PropertyPathHandle aLeafHandle, nl::Weave::TLV::TLVReader &aReader) __OVERRIDE;
    WEAVE_ERROR GetLeafData(nl::Weave::Profiles::DataManagement::PropertyPathHandle aLeafHandle, uint64_t aTagToWrite, nl::Weave::TLV::TLVWriter &aWriter) __OVERRIDE;
    WEAVE_ERROR GetNextDictionaryItemKey(nl::Weave::Profiles::DataManagement::PropertyPathHandle aDictionaryHandle, uintptr_t &aContext, nl::Weave::Profiles::DataManagement::PropertyDictionaryKey &aKey) __OVERRIDE;

    enum
    {
        kMaxNumOfLocals = 10,
        kMaxNumOfCharsPerLocale = 24,
    };

    uint8_t mNumLocales;
    char mLocales[kMaxNumOfLocals][kMaxNumOfCharsPerLocale];
};

class BoltLockSettingTraitDataSink : public MockTraitDataSink, public MockTraitDataSourceDelegate
{
public:
    BoltLockSettingTraitDataSink();

private:
    WEAVE_ERROR SetLeafData(nl::Weave::Profiles::DataManagement::PropertyPathHandle aLeafHandle, nl::Weave::TLV::TLVReader &aReader) __OVERRIDE;
    WEAVE_ERROR GetLeafData(nl::Weave::Profiles::DataManagement::PropertyPathHandle aLeafHandle, uint64_t aTagToWrite, nl::Weave::TLV::TLVWriter &aWriter) __OVERRIDE;
    WEAVE_ERROR GetNextDictionaryItemKey(nl::Weave::Profiles::DataManagement::PropertyPathHandle aDictionaryHandle, uintptr_t &aContext, nl::Weave::Profiles::DataManagement::PropertyDictionaryKey &aKey) __OVERRIDE;

    bool mAutoRelockOn;
    uint32_t mAutoRelockDuration;
};

class TestATraitDataSink : public MockTraitDataSink, public MockTraitDataSourceDelegate
{
public:
    TestATraitDataSink();
    TestATraitDataSink(bool aAcceptsSublessNotifies);

    WEAVE_ERROR OnEvent(uint16_t aType, void *aInParam) __OVERRIDE;

private:
    void SetNullifiedPath(nl::Weave::Profiles::DataManagement::PropertyPathHandle aHandle, bool isNull);
    WEAVE_ERROR SetData(nl::Weave::Profiles::DataManagement::PropertyPathHandle aHandle, nl::Weave::TLV::TLVReader &aReader, bool aIsNull) __OVERRIDE;
    WEAVE_ERROR SetLeafData(nl::Weave::Profiles::DataManagement::PropertyPathHandle aLeafHandle, nl::Weave::TLV::TLVReader &aReader) __OVERRIDE;
    bool FindKey(uint32_t *aKeyList, uint32_t aListSize, nl::Weave::Profiles::DataManagement::PropertyPathHandle aHandle);
    WEAVE_ERROR GetData(nl::Weave::Profiles::DataManagement::PropertyPathHandle aHandle, uint64_t aTagToWrite, nl::Weave::TLV::TLVWriter &aWriter, bool &aIsNull, bool &aIsPresent) __OVERRIDE;
    WEAVE_ERROR GetLeafData(nl::Weave::Profiles::DataManagement::PropertyPathHandle aLeafHandle, uint64_t aTagToWrite, nl::Weave::TLV::TLVWriter &aWriter) __OVERRIDE;
    WEAVE_ERROR GetNextDictionaryItemKey(nl::Weave::Profiles::DataManagement::PropertyPathHandle aDictionaryHandle, uintptr_t &aContext, nl::Weave::Profiles::DataManagement::PropertyDictionaryKey &aKey) __OVERRIDE;

    int32_t taa;
    int32_t tab;
    uint32_t tac;
    Schema::Nest::Test::Trait::TestATrait::StructA tad;
    uint32_t tae[10];

    // weave.common.StringRef is implemented as a union
    char tag_string[20];
    uint16_t tag_ref;
    bool tag_use_ref;

    uint32_t tai_stageditem;
    std::map<uint16_t, uint32_t> tai_map;

    Schema::Nest::Test::Trait::TestATrait::StructA taj_stageditem;
    std::map<uint16_t, Schema::Nest::Test::Trait::TestATrait::StructA> taj_map;

    // byte array
    uint8_t tak[10];

    // day of week
    uint8_t tal;

    // implicit resourceid
    uint64_t tam_resourceid;
    // resource id and type
    uint8_t tan[10];

    uint32_t tao;

    int64_t tap; // milliseconds
    int64_t taq; // milliseconds
    uint32_t tar; // seconds
    uint32_t tas; // milliseconds

    uint32_t tat;
    int32_t tau;
    bool tav;
    char taw[20];
    // boxed float
    int16_t tax;

    bool nullified_path[Schema::Nest::Test::Trait::TestATrait::kPropertyHandle_TaJ_Value_SaB];
};

class TestBTraitDataSink : public MockTraitDataSink, public MockTraitDataSourceDelegate
{
public:
    TestBTraitDataSink();

private:
    void SetNullifiedPath(nl::Weave::Profiles::DataManagement::PropertyPathHandle aHandle, bool isNull);
    WEAVE_ERROR SetData(nl::Weave::Profiles::DataManagement::PropertyPathHandle aHandle, nl::Weave::TLV::TLVReader &aReader, bool aIsNull) __OVERRIDE;
    WEAVE_ERROR SetLeafData(nl::Weave::Profiles::DataManagement::PropertyPathHandle aLeafHandle, nl::Weave::TLV::TLVReader &aReader) __OVERRIDE;
    WEAVE_ERROR GetData(nl::Weave::Profiles::DataManagement::PropertyPathHandle aHandle, uint64_t aTagToWrite, nl::Weave::TLV::TLVWriter &aWriter, bool &aIsNull, bool &aIsPresent) __OVERRIDE;
    WEAVE_ERROR GetLeafData(nl::Weave::Profiles::DataManagement::PropertyPathHandle aLeafHandle, uint64_t aTagToWrite, nl::Weave::TLV::TLVWriter &aWriter) __OVERRIDE;
    WEAVE_ERROR GetNextDictionaryItemKey(nl::Weave::Profiles::DataManagement::PropertyPathHandle aDictionaryHandle, uintptr_t &aContext, nl::Weave::Profiles::DataManagement::PropertyDictionaryKey &aKey) __OVERRIDE;

    int32_t taa;
    int32_t tab;
    uint32_t tac;
    uint32_t tad_saa;
    bool tad_sab;
    uint32_t tae[10];
    char tai[10];
    int64_t tap;

    uint32_t tba;
    char tbb_sba[10];
    uint32_t tbb_sbb;
    uint32_t tbc_saa;
    bool tbc_sab;
    char tbc_seac[10];

    bool nullified_path[Schema::Nest::Test::Trait::TestBTrait::kPropertyHandle_TaJ_Value_SaB];
};

class TestApplicationKeysTraitDataSink : public Schema::Weave::Trait::Auth::ApplicationKeysTrait::ApplicationKeysTraitDataSink, public MockTraitDataSourceDelegate
{
public:
    TestApplicationKeysTraitDataSink(void);

private:
    WEAVE_ERROR GetLeafData(nl::Weave::Profiles::DataManagement::PropertyPathHandle aLeafHandle, uint64_t aTagToWrite, nl::Weave::TLV::TLVWriter &aWriter) __OVERRIDE;
    WEAVE_ERROR GetNextDictionaryItemKey(nl::Weave::Profiles::DataManagement::PropertyPathHandle aDictionaryHandle, uintptr_t &aContext, nl::Weave::Profiles::DataManagement::PropertyDictionaryKey &aKey) __OVERRIDE;
};

#endif // MOCK_TRAIT_SINKS_H_
