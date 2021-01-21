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
 *      This file declares the mock source trait classes.
 *
 */

#ifndef MOCK_TRAIT_SOURCES_H_
#define MOCK_TRAIT_SOURCES_H_

// We want and assume the default managed namespace is Current and that is, explicitly, the managed namespace this code desires.
#include <Weave/Profiles/data-management/TraitData.h>
#include <Weave/Profiles/data-management/TraitCatalog.h>

#include <weave/trait/security/BoltLockSettingsTrait.h>
#include <weave/trait/locale/LocaleSettingsTrait.h>
#include <weave/trait/locale/LocaleCapabilitiesTrait.h>
#include <nest/test/trait/TestATrait.h>
#include <nest/test/trait/TestBTrait.h>
#include <nest/test/trait/TestCTrait.h>
#include <nest/test/trait/TestDTrait.h>
#include <nest/test/trait/TestCommon.h>
#include "TestGroupKeyStore.h"
#include <map>

#define MAX_LOCALE_SIZE sizeof(char) * 24

class LocaleSettingsTraitDataSource :
#if WDM_ENABLE_PUBLISHER_UPDATE_SERVER_SUPPORT
        public nl::Weave::Profiles::DataManagement::TraitUpdatableDataSource
#else
        public nl::Weave::Profiles::DataManagement::TraitDataSource
#endif
{
public:
    LocaleSettingsTraitDataSource();
    void Mutate();

private:
#if WDM_ENABLE_PUBLISHER_UPDATE_SERVER_SUPPORT
    WEAVE_ERROR SetLeafData(nl::Weave::Profiles::DataManagement::PropertyPathHandle aLeafHandle, nl::Weave::TLV::TLVReader &aReader) __OVERRIDE;
#endif // WDM_ENABLE_PUBLISHER_UPDATE_SERVER_SUPPORT
    WEAVE_ERROR GetLeafData(nl::Weave::Profiles::DataManagement::PropertyPathHandle aLeafHandle, uint64_t aTagToWrite, nl::Weave::TLV::TLVWriter &aWriter) __OVERRIDE;

    char mLocale[24];
};

class LocaleCapabilitiesTraitDataSource :
#if WDM_ENABLE_PUBLISHER_UPDATE_SERVER_SUPPORT
        public nl::Weave::Profiles::DataManagement::TraitUpdatableDataSource
#else
        public nl::Weave::Profiles::DataManagement::TraitDataSource
#endif
{
public:
    LocaleCapabilitiesTraitDataSource();
    void Mutate();

private:
#if WDM_ENABLE_PUBLISHER_UPDATE_SERVER_SUPPORT
    WEAVE_ERROR SetLeafData(nl::Weave::Profiles::DataManagement::PropertyPathHandle aLeafHandle, nl::Weave::TLV::TLVReader &aReader) __OVERRIDE;
#endif // WDM_ENABLE_PUBLISHER_UPDATE_SERVER_SUPPORT
    WEAVE_ERROR GetLeafData(nl::Weave::Profiles::DataManagement::PropertyPathHandle aLeafHandle, uint64_t aTagToWrite, nl::Weave::TLV::TLVWriter &aWriter) __OVERRIDE;

    enum {
        kMaxNumOfCharsPerLocale = 24,
        kMaxNumOfLocals = 10,
    };

    uint8_t mNumLocales;
    char mLocales[kMaxNumOfLocals][kMaxNumOfCharsPerLocale];
};

class BoltLockSettingTraitDataSource : public nl::Weave::Profiles::DataManagement::TraitDataSource
{
public:
    BoltLockSettingTraitDataSource();
    void Mutate();

private:
    WEAVE_ERROR GetLeafData(nl::Weave::Profiles::DataManagement::PropertyPathHandle aLeafHandle, uint64_t aTagToWrite, nl::Weave::TLV::TLVWriter &aWriter) __OVERRIDE;

    bool mAutoRelockOn;
    uint32_t mAutoRelockDuration;
};

class TestATraitDataSource :
#if WDM_ENABLE_PUBLISHER_UPDATE_SERVER_SUPPORT
        public nl::Weave::Profiles::DataManagement::TraitUpdatableDataSource
#else
        public nl::Weave::Profiles::DataManagement::TraitDataSource
#endif
{
public:
    TestATraitDataSource();
    void Mutate();

    uint32_t mTraitTestSet;

private:
#if WDM_ENABLE_PUBLISHER_UPDATE_SERVER_SUPPORT
    WEAVE_ERROR SetLeafData(nl::Weave::Profiles::DataManagement::PropertyPathHandle aLeafHandle, nl::Weave::TLV::TLVReader &aReader) __OVERRIDE;
#endif // WDM_ENABLE_PUBLISHER_UPDATE_SERVER_SUPPORT

    void SetNullifiedPath(nl::Weave::Profiles::DataManagement::PropertyPathHandle aHandle, bool isNull);

    WEAVE_ERROR GetData(nl::Weave::Profiles::DataManagement::PropertyPathHandle aHandle, uint64_t aTagToWrite, nl::Weave::TLV::TLVWriter &aWriter, bool &aIsNull, bool &aIsPresent) __OVERRIDE;
    WEAVE_ERROR GetLeafData(nl::Weave::Profiles::DataManagement::PropertyPathHandle aLeafHandle, uint64_t aTagToWrite, nl::Weave::TLV::TLVWriter &aWriter) __OVERRIDE;
    WEAVE_ERROR GetNextDictionaryItemKey(nl::Weave::Profiles::DataManagement::PropertyPathHandle aDictionaryHandle, uintptr_t &aContext, nl::Weave::Profiles::DataManagement::PropertyDictionaryKey &aKey) __OVERRIDE;

    virtual void OnCustomCommand(nl::Weave::Profiles::DataManagement::Command * aCommand,
                const nl::Weave::WeaveMessageInfo * aMsgInfo,
                nl::Weave::PacketBuffer * aPayload,
                const uint64_t & aCommandType,
                const bool aIsExpiryTimeValid,
                const int64_t & aExpiryTimeMicroSecond,
                const bool aIsMustBeVersionValid,
                const uint64_t & aMustBeVersion,
                nl::Weave::TLV::TLVReader & aArgumentReader) __OVERRIDE;

    static void HandleCommandOperationTimeout(nl::Weave::System::Layer* aSystemLayer, void *aAppState,
            nl::Weave::System::Error aErr);

    enum
    {
        kCmdType_1      = 1,

        kCmdParam_1     = 1,
        kCmdParam_2     = 2,
    };
    uint32_t mCommandParam_1;
    bool mCommandParam_2;
    nl::Weave::Profiles::DataManagement::Command * mActiveCommand;

    int32_t taa;
    int32_t tab;
    uint32_t tac;
    Schema::Nest::Test::Trait::TestATrait::StructA tad;
    uint32_t tae[10];

    // weave.common.StringRef is implemented as a union
    char *tag_string = "stringreftest";
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
    char *taw = "boxedstring";
    // boxed float
    int16_t tax;

    bool nullified_path[Schema::Nest::Test::Trait::TestATrait::kPropertyHandle_TaJ_Value_SaB];

    uint32_t mTestCounter;
};

class TestBTraitDataSource:
#if WDM_ENABLE_PUBLISHER_UPDATE_SERVER_SUPPORT
    public nl::Weave::Profiles::DataManagement::TraitUpdatableDataSource
#else
    public nl::Weave::Profiles::DataManagement::TraitDataSource
#endif
{
public:
    TestBTraitDataSource();
    void Mutate();

private:
#if WDM_ENABLE_PUBLISHER_UPDATE_SERVER_SUPPORT
    WEAVE_ERROR SetLeafData(nl::Weave::Profiles::DataManagement::PropertyPathHandle aLeafHandle, nl::Weave::TLV::TLVReader &aReader) __OVERRIDE;
#endif // WDM_ENABLE_PUBLISHER_UPDATE_SERVER_SUPPORT
    WEAVE_ERROR GetLeafData(nl::Weave::Profiles::DataManagement::PropertyPathHandle aLeafHandle, uint64_t aTagToWrite, nl::Weave::TLV::TLVWriter &aWriter) __OVERRIDE;
    WEAVE_ERROR GetNextDictionaryItemKey(nl::Weave::Profiles::DataManagement::PropertyPathHandle aDictionaryHandle, uintptr_t &aContext, nl::Weave::Profiles::DataManagement::PropertyDictionaryKey &aKey) __OVERRIDE;

    int32_t taa;
    int32_t tab;
    uint32_t tac;
    uint32_t tad_saa;
    bool tad_sab;
    uint32_t tae[10];
    int64_t tap;
    uint32_t tba;
    char tbb_sba[10];
    uint32_t tbb_sbb;
    uint32_t tbc_saa;
    bool tbc_sab;
    char tbc_seac[10];
};

class TestBLargeTraitDataSource : public nl::Weave::Profiles::DataManagement::TraitDataSource
{
public:
    TestBLargeTraitDataSource();
    void Mutate();

private:
    WEAVE_ERROR GetLeafData(nl::Weave::Profiles::DataManagement::PropertyPathHandle aLeafHandle, uint64_t aTagToWrite, nl::Weave::TLV::TLVWriter &aWriter) __OVERRIDE;
    WEAVE_ERROR GetNextDictionaryItemKey(nl::Weave::Profiles::DataManagement::PropertyPathHandle aDictionaryHandle, uintptr_t &aContext, nl::Weave::Profiles::DataManagement::PropertyDictionaryKey &aKey) __OVERRIDE;

    Schema::Nest::Test::Trait::TestATrait::EnumA taa;
    Schema::Nest::Test::Trait::TestCommon::CommonEnumA tab;
    uint32_t tac;
    uint32_t tad_saa;
    bool tad_sab;
    uint32_t tae[500];
    int64_t tap;

    uint32_t tba;
    char tbb_sba[10];
    uint32_t tbb_sbb;
    uint32_t tbc_saa;
    bool tbc_sab;
    char tbc_seac[10];
};

class ApplicationKeysTraitDataSource : public nl::Weave::Profiles::DataManagement::TraitDataSource
{
public:
    ApplicationKeysTraitDataSource(void);
    WEAVE_ERROR Mutate(void);

private:
    enum
    {
        kInitialTraitVersionNumber                      = 100,
    };

    WEAVE_ERROR GetLeafData(nl::Weave::Profiles::DataManagement::PropertyPathHandle aLeafHandle, uint64_t aTagToWrite, nl::Weave::TLV::TLVWriter &aWriter) __OVERRIDE;
    WEAVE_ERROR GetNextDictionaryItemKey(nl::Weave::Profiles::DataManagement::PropertyPathHandle aDictionaryHandle, uintptr_t &aContext, nl::Weave::Profiles::DataManagement::PropertyDictionaryKey &aKey) __OVERRIDE;

    void ClearEpochKeys(void);
    void ClearGroupMasterKeys(void);
    WEAVE_ERROR MutateEpochKeys(void);
    WEAVE_ERROR MutateGroupMasterKeys(void);
    WEAVE_ERROR AddEpochKey(uint8_t epochKeyNumber, uint32_t startTime, const uint8_t *key, uint8_t keyLen);
    WEAVE_ERROR AddGroupMasterKey(uint8_t appGroupLocalNumber, uint32_t globalId, const uint8_t *key, uint8_t keyLen);

    nl::Weave::Profiles::Security::AppKeys::WeaveGroupKey EpochKeys[WEAVE_CONFIG_MAX_APPLICATION_EPOCH_KEYS];
    nl::Weave::Profiles::Security::AppKeys::WeaveGroupKey GroupMasterKeys[WEAVE_CONFIG_MAX_APPLICATION_GROUPS];
};

class TestCTraitDataSource :
#if WDM_ENABLE_PUBLISHER_UPDATE_SERVER_SUPPORT
        public nl::Weave::Profiles::DataManagement::TraitUpdatableDataSource
#else
        public nl::Weave::Profiles::DataManagement::TraitDataSource
#endif
{
public:
    TestCTraitDataSource();
    void Mutate();
    static void TLVPrettyPrinter(const char *aFormat, ...);
private:
#if WDM_ENABLE_PUBLISHER_UPDATE_SERVER_SUPPORT
    WEAVE_ERROR SetLeafData(nl::Weave::Profiles::DataManagement::PropertyPathHandle aLeafHandle, nl::Weave::TLV::TLVReader &aReader) __OVERRIDE;
#endif // WDM_ENABLE_PUBLISHER_UPDATE_SERVER_SUPPORT

    WEAVE_ERROR GetLeafData(nl::Weave::Profiles::DataManagement::PropertyPathHandle aLeafHandle, uint64_t aTagToWrite, nl::Weave::TLV::TLVWriter &aWriter) __OVERRIDE;
    bool taa;
    int32_t tab;
    Schema::Nest::Test::Trait::TestCTrait::StructC tac;
    uint32_t tad;
};

#endif // MOCK_TRAIT_SOURCES_H_
