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
 *      Implements a mismatched schema data source and sink of TestCTrait
 *
 */

#ifndef MOCK_MISMATCHED_SCHEMA_H_
#define MOCK_MISMATCHED_SCHEMA_H_

// We want and assume the default managed namespace is Current and that is, explicitly, the managed namespace this code desires.
#include <Weave/Profiles/data-management/DataManagement.h>

#include "schema/nest/test/trait/TestCTrait.h"
#include "schema/nest/test/trait/TestMismatchedCTrait.h"

#include "schema/nest/test/trait/StructMismatchedCStructSchema.h"
#include "schema/nest/test/trait/StructCStructSchema.h"

using namespace ::nl::Weave::TLV;
using namespace ::nl::Weave::Profiles::DataManagement;
using namespace Schema::Nest::Test::Trait;

class TestMismatchedCTraitDataSource : public TraitDataSource
{
public:
    TestMismatchedCTraitDataSource(void);
    void SetValue(PropertyPathHandle aLeafHandle, uint32_t aValue);
    void Reset(void) { mVersion = 0; };

private:
    WEAVE_ERROR SetLeafData(PropertyPathHandle aLeafHandle, TLVReader &aReader);
    WEAVE_ERROR GetLeafData(PropertyPathHandle aLeafHandle, uint64_t aTagToWrite, TLVWriter &aWriter) __OVERRIDE;

    bool tc_a;
    TestCTrait::EnumC tc_b;
    TestMismatchedCTrait::StructMismatchedC tc_c;
    uint32_t tc_d;
    TestMismatchedCTrait::StructMismatchedC tc_e;
};

class TestCTraitDataSink : public TraitDataSink
{
public:
    TestCTraitDataSink(void);
    void Reset(void);

    WEAVE_ERROR SetLeafData(PropertyPathHandle aLeafHandle, TLVReader &aReader);
    WEAVE_ERROR OnEvent(uint16_t aType, void *aInEventParam);
    bool WasPathHandleSet(PropertyPathHandle aLeafHandle);
    bool WasAnyPathHandleSet(void);

private:

    bool mPathHandleSet[TestCTrait::kPropertyHandle_TcC_ScB];
};

// This data sink matches the schema of TestMismatchedCTrait and the
// corresponding data source. However, the application (SetLeafData)
// has not been updated. This tests the default 
class TestMismatchedCTraitDataSink : public TraitDataSink
{
public:
    TestMismatchedCTraitDataSink(void);
    void Reset(void);

    WEAVE_ERROR SetLeafData(PropertyPathHandle aLeafHandle, TLVReader &aReader);
    WEAVE_ERROR OnEvent(uint16_t aType, void *aInEventParam);
    bool WasPathHandleSet(PropertyPathHandle aLeafHandle);
    bool WasAnyPathHandleSet(void);

private:

    bool mPathHandleSet[TestMismatchedCTrait::kPropertyHandle_TcE_ScC];
};
#endif // MOCK_MISMATCHED_SCHEMA_H_
