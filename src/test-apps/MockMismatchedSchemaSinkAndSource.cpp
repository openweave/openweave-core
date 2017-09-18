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
 *      Implements a mismatched schema of TestCTrait
 */

// Note that the choice of namespace alias must be made up front for each and every compile unit
// This is because many include paths could set the default alias to unintended target.
#include <Weave/Profiles/bulk-data-transfer/Development/BDXManagedNamespace.hpp>
#include <Weave/Profiles/data-management/Current/WdmManagedNamespace.h>

#include <Weave/Support/CodeUtils.h>
#include "MockMismatchedSchemaSinkAndSource.h"

using namespace Schema::Nest::Test::Trait;
using namespace ::nl::Weave::TLV;
using namespace ::nl::Weave::Profiles::DataManagement;

TestMismatchedCTraitDataSource :: TestMismatchedCTraitDataSource(void)
    : TraitDataSource(&TestMismatchedCTrait::TraitSchema)
{

}

void
TestMismatchedCTraitDataSource::SetValue(PropertyPathHandle aLeafHandle, uint32_t aValue)
{
    switch (aLeafHandle)
    {
        case TestMismatchedCTrait::kPropertyHandle_TcA:
            tc_a = static_cast<bool>(aValue);
            SetDirty(TestMismatchedCTrait::kPropertyHandle_TcA);
            WeaveLogDetail(DataManagement, "<<  tc_a = %s", tc_a ? "true" : "false");
            break;

        case TestMismatchedCTrait::kPropertyHandle_TcB:
            break;

        case TestMismatchedCTrait::kPropertyHandle_TcC_ScA:
            tc_c.scA = aValue;
            SetDirty(aLeafHandle);
            WeaveLogDetail(DataManagement, "<<  tc_c.scA = %u", tc_c.scA);
            break;

        case TestMismatchedCTrait::kPropertyHandle_TcC_ScB:
            tc_c.scB = static_cast<bool>(aValue);
            SetDirty(aLeafHandle);
            WeaveLogDetail(DataManagement, "<<  tc_c.scB = %s", tc_c.scB ? "true" : "false");
            break;

        case TestMismatchedCTrait::kPropertyHandle_TcC_ScC:
            tc_c.scC = aValue;
            SetDirty(aLeafHandle);
            WeaveLogDetail(DataManagement, "<<  tc_c.scC = %u", tc_c.scC);
            break;

        case TestMismatchedCTrait::kPropertyHandle_TcD:
            tc_d = aValue;
            SetDirty(aLeafHandle);
            WeaveLogDetail(DataManagement, "<<  tc_d = %u", tc_d);
            break;

        case TestMismatchedCTrait::kPropertyHandle_TcE_ScA:
            tc_e.scA = aValue;
            SetDirty(aLeafHandle);
            WeaveLogDetail(DataManagement, "<<  tc_e.scA = %u", tc_e.scA);
            break;

        case TestMismatchedCTrait::kPropertyHandle_TcE_ScB:
            tc_e.scB = static_cast<bool>(aValue);
            SetDirty(aLeafHandle);
            WeaveLogDetail(DataManagement, "<<  tc_e.scB = %s", tc_e.scB ? "true" : "false");
            break;

        case TestMismatchedCTrait::kPropertyHandle_TcE_ScC:
            tc_e.scC = aValue;
            SetDirty(aLeafHandle);
            WeaveLogDetail(DataManagement, "<<  tc_e.scC = %u", tc_e.scC);
            break;

        default:
            break;
    }
}

WEAVE_ERROR TestMismatchedCTraitDataSource::SetLeafData(
        PropertyPathHandle aLeafHandle, TLVReader &aReader)
{
    return WEAVE_ERROR_UNSUPPORTED_WEAVE_FEATURE;
}

WEAVE_ERROR TestMismatchedCTraitDataSource::GetLeafData(
        PropertyPathHandle aLeafHandle, uint64_t aTagToWrite, TLVWriter &aWriter)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    switch (aLeafHandle)
    {
        case TestMismatchedCTrait::kPropertyHandle_TcA:
            err = aWriter.Put(aTagToWrite, tc_a);
            SuccessOrExit(err);
            WeaveLogDetail(DataManagement, ">>  tc_a = %s", tc_a ? "true" : "false");
            break;

        case TestMismatchedCTrait::kPropertyHandle_TcB:
            break;

        case TestMismatchedCTrait::kPropertyHandle_TcC_ScA:
            err = aWriter.Put(aTagToWrite, tc_c.scA);
            SuccessOrExit(err);
            WeaveLogDetail(DataManagement, ">>  tc_c.scA = %u", tc_c.scA);
            break;

        case TestMismatchedCTrait::kPropertyHandle_TcC_ScB:
            err = aWriter.Put(aTagToWrite, tc_c.scB);
            SuccessOrExit(err);
            WeaveLogDetail(DataManagement, ">>  tc_c.scB = %s", tc_c.scB ? "true" : "false");
            break;

        case TestMismatchedCTrait::kPropertyHandle_TcC_ScC:
            err = aWriter.Put(aTagToWrite, tc_c.scC);
            SuccessOrExit(err);
            WeaveLogDetail(DataManagement, ">>  tc_c.scC = %u", tc_c.scC);
            break;

        case TestMismatchedCTrait::kPropertyHandle_TcD:
            err = aWriter.Put(aTagToWrite, tc_d);
            SuccessOrExit(err);
            WeaveLogDetail(DataManagement, ">>  tc_d = %u", tc_d);
            break;

        case TestMismatchedCTrait::kPropertyHandle_TcE_ScA:
            err = aWriter.Put(aTagToWrite, tc_e.scA);
            SuccessOrExit(err);
            WeaveLogDetail(DataManagement, ">>  tc_e.scA = %u", tc_e.scA);
            break;

        case TestMismatchedCTrait::kPropertyHandle_TcE_ScB:
            err = aWriter.Put(aTagToWrite, tc_e.scB);
            SuccessOrExit(err);
            WeaveLogDetail(DataManagement, ">>  tc_e.scB = %s", tc_e.scB ? "true" : "false");
            break;

        case TestMismatchedCTrait::kPropertyHandle_TcE_ScC:
            err = aWriter.Put(aTagToWrite, tc_e.scC);
            SuccessOrExit(err);
            WeaveLogDetail(DataManagement, ">>  tc_e.scC = %u", tc_e.scC);
            break;

        default:
            break;
    }

exit:
    return err;
}

TestCTraitDataSink::TestCTraitDataSink(void)
    : TraitDataSink(&TestCTrait::TraitSchema)
{

}

void
TestCTraitDataSink::Reset(void)
{
    ClearVersion();
    memset(mPathHandleSet, 0, sizeof(mPathHandleSet));
}

bool
TestCTraitDataSink::WasPathHandleSet(PropertyPathHandle aLeafHandle)
{
    bool retval = false;

    if (aLeafHandle <= TestCTrait::kPropertyHandle_TcC_ScB)
    {
        retval = mPathHandleSet[aLeafHandle - 1];
    }

    return retval;
}

bool
TestCTraitDataSink::WasAnyPathHandleSet(void)
{
    bool retval = false;

    for (uint32_t i = 0; i < TestCTrait::kPropertyHandle_TcC_ScB; i++)
    {
        retval |= mPathHandleSet[i];
    }
    return retval;
}

WEAVE_ERROR
TestCTraitDataSink::SetLeafData(
        PropertyPathHandle aLeafHandle, TLVReader &aReader)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveLogDetail(DataManagement, "leaf handle: %u", aLeafHandle);
    switch (aLeafHandle)
    {
        case TestCTrait::kPropertyHandle_TcA:
        case TestCTrait::kPropertyHandle_TcB:
        case TestCTrait::kPropertyHandle_TcC_ScA:
        case TestCTrait::kPropertyHandle_TcC_ScB:
            mPathHandleSet[aLeafHandle - 1] = true;
            break;

        default:
            err = WEAVE_ERROR_TLV_TAG_NOT_FOUND;
            break;
    }

    return err;
}

WEAVE_ERROR
TestCTraitDataSink::OnEvent(uint16_t aType, void *aInParam)
{
    return WEAVE_NO_ERROR;
}

TestMismatchedCTraitDataSink::TestMismatchedCTraitDataSink(void)
    : TraitDataSink(&TestMismatchedCTrait::TraitSchema)
{

}

void
TestMismatchedCTraitDataSink::Reset(void)
{
    ClearVersion();
    memset(mPathHandleSet, 0, sizeof(mPathHandleSet));
}

bool
TestMismatchedCTraitDataSink::WasPathHandleSet(PropertyPathHandle aLeafHandle)
{
    bool retval = false;

    if (aLeafHandle <= TestMismatchedCTrait::kPropertyHandle_TcE_ScC)
    {
        retval = mPathHandleSet[aLeafHandle - 1];
    }

    return retval;
}

bool
TestMismatchedCTraitDataSink::WasAnyPathHandleSet(void)
{
    bool retval = false;

    for (uint32_t i = 0; i < TestMismatchedCTrait::kPropertyHandle_TcE_ScC; i++)
    {
        retval |= mPathHandleSet[i];
    }
    return retval;
}

// This function is meant to approximate an application that hasn't been
// updated even though the backing schema is updated. as such, the leaf
// handles that are known match that of TestCTraitDataSink.
WEAVE_ERROR
TestMismatchedCTraitDataSink::SetLeafData(
        PropertyPathHandle aLeafHandle, TLVReader &aReader)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    switch (aLeafHandle)
    {
        case TestCTrait::kPropertyHandle_TcA:
        case TestCTrait::kPropertyHandle_TcB:
        case TestCTrait::kPropertyHandle_TcC_ScA:
        case TestCTrait::kPropertyHandle_TcC_ScB:
            mPathHandleSet[aLeafHandle - 1] = true;
            break;

        default:
            mPathHandleSet[aLeafHandle - 1] = true;
            err = HandleUnknownLeafHandle();
            break;
    }

    return err;
}

WEAVE_ERROR
TestMismatchedCTraitDataSink::OnEvent(uint16_t aType, void *aInParam)
{
    return WEAVE_NO_ERROR;
}
