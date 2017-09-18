/*
 *
 *    Copyright (c) 2017 Nest Labs, Inc.
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
 *      This file implements the trait data sink and source for TestBTrait
 *
 */
#include <Weave/Profiles/data-management/DataManagement.h>

#include <Weave/Profiles/data-management/TraitData.h>
#include <Weave/Profiles/data-management/TraitCatalog.h>

#include <nest/test/trait/TestBTrait.h>
#include <nest/test/trait/TestATrait.h>
#include <nest/test/trait/TestCommonTrait.h>
#include <nest/test/trait/StructAStructSchema.h>
#include <nest/test/trait/StructBStructSchema.h>
#include <nest/test/trait/StructEAStructSchema.h>

class TestBTraitDataSource : public nl::Weave::Profiles::DataManagement::TraitDataSource
{
public:
    TestBTraitDataSource(void);
    void Reset(void);
    void SetNullifiedPath(nl::Weave::Profiles::DataManagement::PropertyPathHandle aHandle, bool isNull);
    void SetPresentPath(nl::Weave::Profiles::DataManagement::PropertyPathHandle aHandle, bool isPresent);

private:
    WEAVE_ERROR SetLeafData(nl::Weave::Profiles::DataManagement::PropertyPathHandle aLeafHandle, nl::Weave::TLV::TLVReader &aReader);
    WEAVE_ERROR GetData(nl::Weave::Profiles::DataManagement::PropertyPathHandle aHandle, uint64_t aTagToWrite, nl::Weave::TLV::TLVWriter &aWriter, bool &aIsNull, bool &aIsPresent) __OVERRIDE;
    WEAVE_ERROR GetLeafData(nl::Weave::Profiles::DataManagement::PropertyPathHandle aLeafHandle, uint64_t aTagToWrite, nl::Weave::TLV::TLVWriter &aWriter) __OVERRIDE;
    WEAVE_ERROR GetNextDictionaryItemKey(nl::Weave::Profiles::DataManagement::PropertyPathHandle aDictionaryHandle, uintptr_t &aContext, nl::Weave::Profiles::DataManagement::PropertyDictionaryKey &aKey) __OVERRIDE;

    Schema::Nest::Test::Trait::TestATrait::EnumA taa;
    Schema::Nest::Test::Trait::TestCommonTrait::CommonEnumA tab;
    uint32_t tac;
    Schema::Nest::Test::Trait::TestATrait::StructA tad;
    uint32_t tad_saa;
    bool tad_sab;
    uint32_t tae[10];

    // weave.common.StringRef is implemented as a union
    const char *tag_string = "stringreftest";
    uint16_t tag_ref;
    bool tag_use_ref;

    uint8_t tak[10];
    uint8_t tal;
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
    const char *taw = "boxedstring";
    // boxed float
    int16_t tax;

    uint32_t tba;
    Schema::Nest::Test::Trait::TestBTrait::StructB tbb;
    Schema::Nest::Test::Trait::TestBTrait::StructEA tbc;

    const char *tbb_sba = "testing";
    uint32_t tbb_sbb;
    uint32_t tbc_saa;
    bool tbc_sab;
    const char *tbc_seac = "hallo";

    bool nullified_path[Schema::Nest::Test::Trait::TestBTrait::kPropertyHandle_TaJ_Value_SaB];
    bool ephemeral_path[Schema::Nest::Test::Trait::TestBTrait::kPropertyHandle_TaJ_Value_SaB];
};

class TestBTraitDataSink : public nl::Weave::Profiles::DataManagement::TraitDataSink
{
public:
    TestBTraitDataSink();
    bool IsPathHandleSet(nl::Weave::Profiles::DataManagement::PropertyPathHandle aHandle);
    bool IsPathHandleNull(nl::Weave::Profiles::DataManagement::PropertyPathHandle aHandle);
    void Reset(void);

private:
    WEAVE_ERROR SetLeafData(nl::Weave::Profiles::DataManagement::PropertyPathHandle aLeafHandle, nl::Weave::TLV::TLVReader &aReader) __OVERRIDE;
    WEAVE_ERROR SetData(nl::Weave::Profiles::DataManagement::PropertyPathHandle aHandle, nl::Weave::TLV::TLVReader &aReader, bool aIsNull) __OVERRIDE;

    void SetDataCalled(nl::Weave::Profiles::DataManagement::PropertyPathHandle aHandle);
    void SetPathHandleNull(nl::Weave::Profiles::DataManagement::PropertyPathHandle aHandle, bool aIsNull);

    int32_t taa;
    int32_t tab;
    uint32_t tac;
    uint32_t tad_saa;
    bool tad_sab;
    uint32_t tae[10];
    char taf_strval[10];
    uint32_t taf_uintval;
    bool taf_boolval;
    int32_t tag_seconds;
    int32_t tag_nanos;
    char tah_literal[10];
    uint32_t tah_reference;
    char tai[10];

    uint32_t tba;
    char tbb_sba[10];
    uint32_t tbb_sbb;
    uint32_t tbc_saa;
    bool tbc_sab;
    char tbc_seac[10];

    bool set_path[Schema::Nest::Test::Trait::TestBTrait::kPropertyHandle_TaJ_Value_SaB];
    bool nullified_path[Schema::Nest::Test::Trait::TestBTrait::kPropertyHandle_TaJ_Value_SaB];
};

