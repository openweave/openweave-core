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
 *      This file implements TestB Sink and Source for use in TestTDM.
 */

// Note that the choice of namespace alias must be made up front for each and every compile unit
// This is because many include paths could set the default alias to unintended target.
#include <Weave/Profiles/bulk-data-transfer/Development/BDXManagedNamespace.hpp>
#include <Weave/Profiles/data-management/Current/WdmManagedNamespace.h>

#include <Weave/Support/CodeUtils.h>
#include "MockTestBTrait.h"

using namespace ::nl::Weave::TLV;
using namespace ::nl::Weave::Profiles::DataManagement;

using namespace Schema::Nest::Test::Trait;

#define MAX_ARRAY_LEN 10

TestBTraitDataSource::TestBTraitDataSource(void)
    : TraitDataSource(&TestBTrait::TraitSchema),
    tag_use_ref(false)
{
}

void TestBTraitDataSource::Reset(void)
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

    tba = 200;
    tbb_sbb = 201;
    tag_use_ref = !tag_use_ref;
    tag_ref = 10;
    tam_resourceid = 0x18b4300000beefULL;

    tap = 1491859262000;

    taq = -3000;
    tar = 3000;
    tas = 3000;

    tbc_saa = 202;
    tbc_sab = false;

    memset(nullified_path, false, sizeof(nullified_path));
    memset(ephemeral_path, true, sizeof(ephemeral_path));
}


WEAVE_ERROR TestBTraitDataSource::GetData(PropertyPathHandle aHandle,
                                          uint64_t aTagToWrite,
                                          TLVWriter &aWriter,
                                          bool &aIsNull,
                                          bool &aIsPresent)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    switch (aHandle)
    {
        // only handle nullable path handles
        // if the handle is a leaf, we should call GetLeafData
        case TestBTrait::kPropertyHandle_TaD:
            aIsNull = nullified_path[aHandle - 1];
            aIsPresent = ephemeral_path[aHandle - 1];
            break;

        default:
            aIsNull = nullified_path[aHandle - 1];
            aIsPresent = ephemeral_path[aHandle - 1];
            break;
    }

    // TDM will handle writing null
    if ((!aIsNull) && (aIsPresent) && (mSchemaEngine->IsLeaf(aHandle)))
    {
        err = GetLeafData(aHandle, aTagToWrite, aWriter);
    }

    return err;
}

void TestBTraitDataSource::SetNullifiedPath(PropertyPathHandle aHandle, bool isNull)
{
    if (aHandle <= TestBTrait::kPropertyHandle_TaJ_Value_SaB)
    {
        if (nullified_path[aHandle - 1] != isNull)
        {
            nullified_path[aHandle - 1] = isNull;
            IncrementVersion();
        }
    }
}

void TestBTraitDataSource::SetPresentPath(PropertyPathHandle aHandle, bool isPresent)
{
    if (aHandle <= TestBTrait::kPropertyHandle_TaJ_Value_SaB)
    {
        if (ephemeral_path[aHandle - 1] != isPresent)
        {
            ephemeral_path[aHandle - 1] = isPresent;
            IncrementVersion();
        }
    }
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
            if (nullified_path[aLeafHandle - 1])
            {
                err = aWriter.PutNull(aTagToWrite);
                SuccessOrExit(err);
            }
            else if (ephemeral_path[aLeafHandle - 1] == false)
            {
                /* no op */
            }
            else
            {
                err = aWriter.Put(aTagToWrite, tad_saa);
                SuccessOrExit(err);
            }

            WeaveLogDetail(DataManagement, ">>  ta_d.sa_a = %u, null = %s", tad_saa, nullified_path[aLeafHandle - 1] ? "true" : "false");
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

        case TestBTrait::kPropertyHandle_TaG:
            if (tag_use_ref)
            {
                err = aWriter.Put(aTagToWrite, tag_ref);
                SuccessOrExit(err);

                WeaveLogDetail(DataManagement, ">>  ta_g.ref = %u", tag_ref);
            }
            else
            {
                err = aWriter.PutString(aTagToWrite, tag_string);
                SuccessOrExit(err);

                WeaveLogDetail(DataManagement, ">>  ta_g.string = %s", tag_string);
            }
            break;

        case TestBTrait::kPropertyHandle_TaL:
        {
            TLVType outerType;
            tal = 0;

            err = aWriter.StartContainer(aTagToWrite, kTLVType_Array, outerType);
            SuccessOrExit(err);

            for (size_t i = 0; i < 7; i++)
            {
                tal = (1 << i);
                err = aWriter.Put(AnonymousTag, tal);
                SuccessOrExit(err);

                WeaveLogDetail(DataManagement, ">> tal[%u] = %u", i, tal);
            }

            err = aWriter.EndContainer(outerType);
            break;
        }

        case TestBTrait::kPropertyHandle_TaM:
            err = aWriter.Put(aTagToWrite, tam_resourceid);
            SuccessOrExit(err);
            break;

        case TestBTrait::kPropertyHandle_TaN:
        {
            uint8_t *tmp = &tan[0];
            nl::Weave::Encoding::LittleEndian::Write16(tmp, 1);
            nl::Weave::Encoding::LittleEndian::Write64(tmp, tam_resourceid);
            err = aWriter.PutBytes(aTagToWrite, &tan[0], sizeof(tan));
            SuccessOrExit(err);
            break;
        }

        case TestATrait::kPropertyHandle_TaP:
            aWriter.Put(aTagToWrite, tap);
            SuccessOrExit(err);
            break;

        case TestBTrait::kPropertyHandle_TaQ:
            aWriter.Put(aTagToWrite, taq);
            SuccessOrExit(err);
            break;

        case TestBTrait::kPropertyHandle_TaR:
            aWriter.Put(aTagToWrite, tar);
            SuccessOrExit(err);
            break;

        case TestBTrait::kPropertyHandle_TaS:
            aWriter.Put(aTagToWrite, tas);
            SuccessOrExit(err);
            break;

        case TestBTrait::kPropertyHandle_TaW:
            aWriter.PutString(aTagToWrite, taw);
            SuccessOrExit(err);
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

            WeaveLogDetail(DataManagement, ">>  tb_c.sea_c = %u", tbc_seac);
            break;

        default:
            WeaveLogDetail(DataManagement, ">> %u  UNKNOWN!", aLeafHandle);
    }

exit:
    WeaveLogFunctError(err);
    return err;
}

WEAVE_ERROR TestBTraitDataSource::GetNextDictionaryItemKey(PropertyPathHandle aDictionaryHandle, uintptr_t &aContext, PropertyDictionaryKey &aKey)
{
    return WEAVE_END_OF_INPUT;
}

TestBTraitDataSink::TestBTraitDataSink()
    : TraitDataSink(&TestBTrait::TraitSchema)
{
}

void TestBTraitDataSink::Reset(void)
{
    ClearVersion();

    memset(set_path, 0, sizeof(set_path));
    memset(nullified_path, 0, sizeof(nullified_path));
}

void TestBTraitDataSink::SetDataCalled(PropertyPathHandle aHandle)
{
    if (aHandle <= TestBTrait::kPropertyHandle_TaJ_Value_SaB)
    {
        set_path[aHandle - 1] = true;
    }
}

void TestBTraitDataSink::SetPathHandleNull(PropertyPathHandle aHandle, bool aIsNull)
{
    if (aHandle <= TestBTrait::kPropertyHandle_TaJ_Value_SaB)
    {
        nullified_path[aHandle - 1] = aIsNull;
    }
}

bool TestBTraitDataSink::IsPathHandleSet(PropertyPathHandle aHandle)
{
    bool retval = false;
    if (aHandle <= TestBTrait::kPropertyHandle_TaJ_Value_SaB)
    {
        retval = set_path[aHandle - 1];
    }
    return retval;
}

bool TestBTraitDataSink::IsPathHandleNull(PropertyPathHandle aHandle)
{
    bool retval = false;
    if (aHandle <= TestBTrait::kPropertyHandle_TaJ_Value_SaB)
    {
        retval = nullified_path[aHandle - 1];
    }
    return retval;
}

WEAVE_ERROR
TestBTraitDataSink::SetData(PropertyPathHandle aHandle, TLVReader &aReader, bool aIsNull)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    SetDataCalled(aHandle);
    SetPathHandleNull(aHandle, aIsNull);

    if ((!aIsNull) && (mSchemaEngine->IsLeaf(aHandle)))
    {
        err = SetLeafData(aHandle, aReader);
    }
    return err;
}

WEAVE_ERROR
TestBTraitDataSink::SetLeafData(PropertyPathHandle aLeafHandle, TLVReader &aReader)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    SetDataCalled(aLeafHandle);

    switch (aLeafHandle) {
        //
        // TestATrait
        //

        case TestBTrait::kPropertyHandle_TaA:
            err = aReader.Get(taa);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, "<< ta_a = %u", taa);
            break;

        case TestBTrait::kPropertyHandle_TaB:
            err = aReader.Get(tab);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, "<< ta_b = %u", tab);
            break;

        case TestBTrait::kPropertyHandle_TaC:
            err = aReader.Get(tac);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, "<< ta_c = %u", tac);
            break;

        case TestBTrait::kPropertyHandle_TaD_SaA:
            err = aReader.Get(tad_saa);
            SuccessOrExit(err);
            SetPathHandleNull(aLeafHandle, false);
            // the parent of this field is also now non-null
            SetPathHandleNull(TestBTrait::kPropertyHandle_TaD, false);

            WeaveLogDetail(DataManagement, "<< ta_d.sa_a = %u", tad_saa);

            break;

        case TestBTrait::kPropertyHandle_TaD_SaB:
            err = aReader.Get(tad_sab);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, "<< ta_d.sa_b = %u", tad_sab);
            break;

        case TestBTrait::kPropertyHandle_TaE:
        {
            TLVType outerType;
            uint32_t i = 0;

            err = aReader.EnterContainer(outerType);
            SuccessOrExit(err);

            while (((err = aReader.Next()) == WEAVE_NO_ERROR) && (i < (sizeof(tae) / sizeof(tae[0]))))
            {
                err = aReader.Get(tae[i]);
                SuccessOrExit(err);

                WeaveLogDetail(DataManagement, "<< ta_e[%u] = %u", i, tae[i]);
                i++;
            }

            err = aReader.ExitContainer(outerType);
            break;
        }

        //
        // TestBTrait
        //

        case TestBTrait::kPropertyHandle_TbA:
            err = aReader.Get(tba);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, "<< tb_a = %u", tba);
            break;

        case TestBTrait::kPropertyHandle_TbB_SbA:
            err = aReader.GetString(tbb_sba, MAX_ARRAY_LEN);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, "<< tb_b.sb_a = %s", tbb_sba);
            break;

        case TestBTrait::kPropertyHandle_TbB_SbB:
            err = aReader.Get(tbb_sbb);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, "<< tb_b.sb_b = %u", tbb_sbb);
            break;

        case TestBTrait::kPropertyHandle_TbC_SaA:
            err = aReader.Get(tbc_saa);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, "<< tb_c.sa_a = %u", tbc_saa);
            break;

        case TestBTrait::kPropertyHandle_TbC_SaB:
            err = aReader.Get(tbc_sab);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, "<< tb_c.sa_b = %u", tbc_sab);
            break;

        case TestBTrait::kPropertyHandle_TbC_SeaC:
            err = aReader.GetString(tbc_seac, MAX_ARRAY_LEN);
            SuccessOrExit(err);

            WeaveLogDetail(DataManagement, "<< tb_c.sea_c = \"%s\"", tbc_seac);
            break;

        default:
            WeaveLogDetail(DataManagement, "<< UNKNOWN!");
    }

exit:
    return err;
}
