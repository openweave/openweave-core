/*
 *
 *    Copyright (c) 2018 Google LLC.
 *    Copyright (c) 2013-2017 Nest Labs, Inc.
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
 *      This file implements unit tests for the Weave TLV implementation.
 *
 */

#include "ToolCommon.h"

#include <nlbyteorder.h>
#include <nlunit-test.h>

#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Core/WeaveTLVDebug.hpp>
#include <Weave/Core/WeaveTLVUtilities.hpp>
#include <Weave/Core/WeaveTLVData.hpp>
#include <Weave/Core/WeaveCircularTLVBuffer.h>
#include <Weave/Support/RandUtils.h>

#include <Weave/Profiles/data-management/Current/WdmManagedNamespace.h>
#include <Weave/Profiles/data-management/DataManagement.h>

#include <nest/test/trait/TestHTrait.h>
#include <nest/test/trait/TestCTrait.h>
#include <nest/test/trait/TestMismatchedCTrait.h>

#include "MockMismatchedSchemaSinkAndSource.h"

#include "MockTestBTrait.h"

#include "MockPlatformClocks.h"

#include <new>
#include <map>
#include <set>
#include <algorithm>
#include <set>
#include <string>
#include <iterator>

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
#include <lwip/init.h>
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

using namespace nl;
using namespace nl::Weave::TLV;
using namespace nl::Weave::Profiles::DataManagement;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// System/Platform definitions
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace Private {

System::Error SetClock_RealTime(uint64_t newCurTime)
{
    return WEAVE_SYSTEM_NO_ERROR;
}

static System::Error GetClock_RealTime(uint64_t & curTime)
{
    curTime = 0x42; // arbitrary non-zero value.
    return WEAVE_SYSTEM_NO_ERROR;
}

} // namespace Private


namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current) {
namespace Platform {
    // for unit tests, the dummy critical section is sufficient.
    void CriticalSectionEnter()
    {
        return;
    }

    void CriticalSectionExit()
    {
        return;
    }
} // Platform
} // WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current)
}
}
}

static SubscriptionEngine *gSubscriptionEngine;

SubscriptionEngine * SubscriptionEngine::GetInstance()
{
    return gSubscriptionEngine;
}

static void CheckDataSourceEmptySchema(nlTestSuite *inSuite, void *inContext);
static void CheckDataSinkEmptySchema(nlTestSuite *inSuite, void *inContext);

static void TestTdmStatic_SingleLeafHandle(nlTestSuite *inSuite, void *inContext);
static void TestTdmStatic_SingleLevelMerge(nlTestSuite *inSuite, void *inContext);
static void TestTdmStatic_SingleLevelMergeDeep(nlTestSuite *inSuite, void *inContext);
static void TestTdmStatic_DirtyStruct(nlTestSuite *inSuite, void *inContext);
static void TestTdmStatic_DirtyLeafUnevenDepth(nlTestSuite *inSuite, void *inContext);
static void TestTdmStatic_MergeHandleSetOverflow(nlTestSuite *inSuite, void *inContext);
static void TestTdmStatic_MarkLeafHandleDirtyTwice(nlTestSuite *inSuite, void *inContext);

static void TestTdmStatic_TestNullableLeaf(nlTestSuite *inSuite, void *inContext);
static void TestTdmStatic_TestNullableStruct(nlTestSuite *inSuite, void *inContext);
static void TestTdmStatic_TestNonNullableLeaf(nlTestSuite *inSuite, void *inContext);

static void TestTdmStatic_TestEphemeralLeaf(nlTestSuite *inSuite, void *inContext);
static void TestTdmStatic_TestEphemeralStruct(nlTestSuite *inSuite, void *inContext);

static void TestTdmStatic_TestIsParent(nlTestSuite *inSuite, void *inContext);

static void TestTdmMismatched_PathInDataElement(nlTestSuite *inSuite, void *inContext);
static void TestTdmMismatched_TopLevelPOD(nlTestSuite *inSuite, void *inContext);
static void TestTdmMismatched_NestedStruct(nlTestSuite *inSuite, void *inContext);
static void TestTdmMismatched_TopLevelStruct(nlTestSuite *inSuite, void *inContext);
static void TestTdmMismatched_SetLeafDataMismatch(nlTestSuite *inSuite, void *inContext);

static void TestTdmDictionary_DictionaryEntryAddition(nlTestSuite *inSuite, void *inContext);
static void TestTdmDictionary_DictionaryEntriesAddition(nlTestSuite *inSuite, void *inContext);
static void TestTdmDictionary_ReplaceDictionary(nlTestSuite *inSuite, void *inContext);

static void TestTdmDictionary_DeleteSingle(nlTestSuite *inSuite, void *inContext);
static void TestTdmDictionary_DeleteMultiple(nlTestSuite *inSuite, void *inContext);
static void TestTdmDictionary_DeleteHandleSetOverflow(nlTestSuite *inSuite, void *inContext);
static void TestTdmDictionary_AddDeleteDifferent(nlTestSuite *inSuite, void *inContext);
static void TestTdmDictionary_DeleteAndMarkDirty(nlTestSuite *inSuite, void *inContext);
static void TestTdmDictionary_MarkDirtyAndDelete(nlTestSuite *inSuite, void *inContext);
static void TestTdmDictionary_DeleteAndMarkFarDirty(nlTestSuite *inSuite, void *inContext);
static void TestTdmDictionary_AddAndDeleteSimilar(nlTestSuite *inSuite, void *inContext);
static void TestTdmDictionary_ModifyAndDeleteSimilar(nlTestSuite *inSuite, void *inContext);
static void TestTdmDictionary_DeleteAndModifySimilar(nlTestSuite *inSuite, void *inContext);
static void TestTdmDictionary_DeleteAndModifyLeafSimilar(nlTestSuite *inSuite, void *inContext);
static void TestTdmDictionary_DeleteStoreOverflowAndItemAddition(nlTestSuite *inSuite, void *inContext);
static void TestTdmDictionary_DirtyStoreOverflowAndItemDeletion(nlTestSuite *inSuite, void *inContext);
static void TestTdmDictionary_DeleteEntryTwice(nlTestSuite *inSuite, void *inContext);
static void TestRandomizedDataVersions(nlTestSuite *inSuite, void *inContext);

static void TestTdmStatic_MultiInstance(nlTestSuite *inSuite, void *inContext);
static void CheckAllocateRightSizedBufferForNotifications(nlTestSuite *inSuite, void *inContext);

// Test Suite

/**
 *  Test Suite that lists all the test functions.
 */
static const nlTest sTests[] = {
    NL_TEST_DEF("Test TraitDataSource + schema with no properties",  CheckDataSourceEmptySchema),
    NL_TEST_DEF("Test TraitDataSink + schema with no properties",    CheckDataSinkEmptySchema),

    // Tests the static schema portions of TDM
    NL_TEST_DEF("Test Tdm (Static schema): Single leaf handle", TestTdmStatic_SingleLeafHandle),

    NL_TEST_DEF("Test Tdm (Static schema): Single level merge of two leaf handles", TestTdmStatic_SingleLevelMerge),
    NL_TEST_DEF("Test Tdm (Static schema): Single level merge of two deeper leaf handles", TestTdmStatic_SingleLevelMergeDeep),
    NL_TEST_DEF("Test Tdm (Static schema): Dirty structure node containing leaf handles", TestTdmStatic_DirtyStruct),
    NL_TEST_DEF("Test Tdm (Static schema): Two dirty leaf handles at different depths", TestTdmStatic_DirtyLeafUnevenDepth),
    NL_TEST_DEF("Test Tdm (Static schema): Overflow of merge handles", TestTdmStatic_MergeHandleSetOverflow),
    NL_TEST_DEF("Test Tdm (Static schema): Mark same handle dirty twice", TestTdmStatic_MarkLeafHandleDirtyTwice),

    NL_TEST_DEF("Test Tdm (Static schema): Nullable leaf data", TestTdmStatic_TestNullableLeaf),
    NL_TEST_DEF("Test Tdm (Static schema): Nullable struct", TestTdmStatic_TestNullableStruct),
    NL_TEST_DEF("Test Tdm (Static schema): Non-Nullable leaf data", TestTdmStatic_TestNonNullableLeaf),

    NL_TEST_DEF("Test Tdm (Static schema): Ephemeral leaf data", TestTdmStatic_TestEphemeralLeaf),
    NL_TEST_DEF("Test Tdm (Static schema): Ephemeral struct", TestTdmStatic_TestEphemeralStruct),

    NL_TEST_DEF("Test Tdm (Static schema): IsParent", TestTdmStatic_TestIsParent),

    // Tests a mismatched schema on publisher and subscriber
    NL_TEST_DEF("Test Tdm (Mismatched schema): Path in DataElement is unmappable", TestTdmMismatched_PathInDataElement),
    NL_TEST_DEF("Test Tdm (Mismatched schema): Schema extended by top level POD", TestTdmMismatched_TopLevelPOD),
    NL_TEST_DEF("Test Tdm (Mismatched schema): Schema extended by nested struct", TestTdmMismatched_NestedStruct),
    NL_TEST_DEF("Test Tdm (Mismatched schema): Schema extended by top level struct", TestTdmMismatched_TopLevelStruct),
    NL_TEST_DEF("Test Tdm (Mismatched schema): App code doesn't match schema", TestTdmMismatched_SetLeafDataMismatch),

    // Tests the dictionary addition/modification portions of TDM
    NL_TEST_DEF("Test Tdm (Dictionary Addition/Modification): Addition of single dictionary entries", TestTdmDictionary_DictionaryEntryAddition),
    NL_TEST_DEF("Test Tdm (Dictionary Addition/Modification): Addition of two dictionary entries", TestTdmDictionary_DictionaryEntriesAddition),
    NL_TEST_DEF("Test Tdm (Dictionary Addition/Modification): Replace dictionary", TestTdmDictionary_ReplaceDictionary),

    // Tests the dictionary deletion portions of TDM
    NL_TEST_DEF("Test Tdm (Dictionary Deletion): Delete single dictionary entry", TestTdmDictionary_DeleteSingle),
    NL_TEST_DEF("Test Tdm (Dictionary Deletion): Delete two dictionary entries", TestTdmDictionary_DeleteMultiple),
    NL_TEST_DEF("Test Tdm (Dictionary Deletion): Overflow of delete handle set", TestTdmDictionary_DeleteHandleSetOverflow),
    NL_TEST_DEF("Test Tdm (Dictionary Deletion): Addition of one entry, deletion of another (within same dictionary)", TestTdmDictionary_AddDeleteDifferent),
    NL_TEST_DEF("Test Tdm (Dictionary Deletion): Delete dictionary entry, then mark dictionary dirty", TestTdmDictionary_DeleteAndMarkDirty),
    NL_TEST_DEF("Test Tdm (Dictionary Deletion): Mark dictionary dirty, then delete dictionary entry", TestTdmDictionary_MarkDirtyAndDelete),
    NL_TEST_DEF("Test Tdm (Dictionary Deletion): Delete entry, then mark another node that is not in a dictionary in the tree as dirty", TestTdmDictionary_DeleteAndMarkFarDirty),
    NL_TEST_DEF("Test Tdm (Dictionary Deletion): Add entry, then delete same entry", TestTdmDictionary_AddAndDeleteSimilar),
    NL_TEST_DEF("Test Tdm (Dictionary Deletion): Modify entry, then delete same entry", TestTdmDictionary_ModifyAndDeleteSimilar),
    NL_TEST_DEF("Test Tdm (Dictionary Deletion): Delete entry then add it back", TestTdmDictionary_DeleteAndModifySimilar),
    NL_TEST_DEF("Test Tdm (Dictionary Deletion): Delete entry then add it back but only mark leaf of dictionary entry dirty", TestTdmDictionary_DeleteAndModifyLeafSimilar),
    NL_TEST_DEF("Test Tdm (Dictionary Deletion): Test delete store overflow + item addition", TestTdmDictionary_DeleteStoreOverflowAndItemAddition),
    NL_TEST_DEF("Test Tdm (Dictionary Deletion): Test dirty store overflow + item deletion", TestTdmDictionary_DirtyStoreOverflowAndItemDeletion),
    NL_TEST_DEF("Test Tdm (Dictionary Deletion): Test delete same dictionary entry twice", TestTdmDictionary_DeleteEntryTwice),

    // Test randomized data versions
    NL_TEST_DEF("Test Tdm (Randomized Data Versions): Randomized Data Versions", TestRandomizedDataVersions),

    NL_TEST_DEF("Test Tdm (Multi Instance): Multi Instance", TestTdmStatic_MultiInstance),

    // Tests the allocation of buffer for building and sending Notifies and
    // Updates.
    NL_TEST_DEF("Test Allocate Right Sized Buffer", CheckAllocateRightSizedBufferForNotifications),


    NL_TEST_SENTINEL()
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Testing Empty Schema
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const TraitSchemaEngine::PropertyInfo gEmptyPropertyMap[] = {
};

const TraitSchemaEngine gEmptyTraitSchema = {
    {
        0x0,
        gEmptyPropertyMap,
        sizeof(gEmptyPropertyMap) / sizeof(gEmptyPropertyMap[0]),
        1,
#if (TDM_EXTENSION_SUPPORT) || (TDM_VERSIONING_SUPPORT)
        2,
#endif
#if (TDM_DICTIONARY_SUPPORT)
        NULL,
#endif
        NULL,
        NULL,
        NULL,
        NULL,
#if (TDM_EXTENSION_SUPPORT)
        NULL,
#endif
#if (TDM_VERSIONING_SUPPORT)
        NULL,
#endif
    }
};

class TestEmptyDataSource : public TraitDataSource {
public:
    TestEmptyDataSource(const TraitSchemaEngine *aSchema) : TraitDataSource(aSchema), mGetLeafDataCalled(false) { }

    // Making these public to allow tests to access them.
    using TraitDataSource::SetVersion;
    using TraitDataSource::IncrementVersion;

    // Throw an error if this ever gets called.
    WEAVE_ERROR GetLeafData(PropertyPathHandle aLeafHandle, uint64_t aTagToWrite, TLVWriter &aWriter) { mGetLeafDataCalled = true; return WEAVE_ERROR_INVALID_ARGUMENT; }

    bool mGetLeafDataCalled;
};

class TestEmptyDataSink : public TraitDataSink {
public:
    TestEmptyDataSink(const TraitSchemaEngine *aSchema);

    WEAVE_ERROR SetLeafData(PropertyPathHandle aLeafHandle, TLVReader &aReader) { mSetLeafDataCalled = true; return WEAVE_ERROR_INVALID_ARGUMENT; }
    WEAVE_ERROR OnEvent(uint16_t aType, void *aInEventParam);

    bool mSetLeafDataCalled;
    bool mEventDataElementBeginSignalled;
    bool mEventDataElementEndSignalled;
};

TestEmptyDataSink::TestEmptyDataSink(const TraitSchemaEngine *aSchema)
    : TraitDataSink(aSchema)
{
    mSetLeafDataCalled = false;
    mEventDataElementBeginSignalled = false;
    mEventDataElementEndSignalled = false;
}

WEAVE_ERROR TestEmptyDataSink::OnEvent(uint16_t aType, void *aInEventParam)
{
    if (aType == kEventDataElementBegin) {
        mEventDataElementBeginSignalled = true;
    }
    else if (aType == kEventDataElementEnd) {
        mEventDataElementEndSignalled = true;
    }

    return WEAVE_NO_ERROR;
}

static void CheckDataSourceEmptySchema(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err;
    TLVWriter writer;
    TLVType dummyContainerType;
    uint8_t buf[1024];
    TestEmptyDataSource dataSource(&gEmptyTraitSchema);
    DataElement::Parser parser;
    TLVReader reader;

    static const uint8_t Encoding[] =
    {
        nlWeaveTLV_STRUCTURE(nlWeaveTLV_TAG_ANONYMOUS),
            nlWeaveTLV_STRUCTURE(nlWeaveTLV_TAG_CONTEXT_SPECIFIC(ContextTag(DataElement::kCsTag_Data))),
            nlWeaveTLV_END_OF_CONTAINER,
        nlWeaveTLV_END_OF_CONTAINER,
    };

    writer.Init(buf, sizeof(buf));

    err = writer.StartContainer(AnonymousTag, kTLVType_Structure, dummyContainerType);
    SuccessOrExit(err);

    err = dataSource.ReadData(kRootPropertyPathHandle, ContextTag(DataElement::kCsTag_Data), writer);
    SuccessOrExit(err);

    // The 'GetLeafData' method shouldn't get called on the source given there is no properties in this trait.
    NL_TEST_ASSERT(inSuite, dataSource.mGetLeafDataCalled == false);

    err = writer.EndContainer(dummyContainerType);
    SuccessOrExit(err);

    err = writer.Finalize();
    reader.Init(buf, writer.GetLengthWritten());

    NL_TEST_ASSERT(inSuite, memcmp(Encoding, buf, sizeof(Encoding)) == 0);

exit:
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    return;
}

static void CheckDataSinkEmptySchema(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err;
    TestEmptyDataSink dataSink(&gEmptyTraitSchema);
    TLVReader reader;

    static const uint8_t Encoding[] =
    {
        nlWeaveTLV_STRUCTURE(nlWeaveTLV_TAG_ANONYMOUS),
            nlWeaveTLV_UINT64(nlWeaveTLV_TAG_CONTEXT_SPECIFIC(ContextTag(DataElement::kCsTag_Version)), 1),
            nlWeaveTLV_STRUCTURE(nlWeaveTLV_TAG_CONTEXT_SPECIFIC(ContextTag(DataElement::kCsTag_Data))),
            nlWeaveTLV_END_OF_CONTAINER,
        nlWeaveTLV_END_OF_CONTAINER,
    };

    reader.Init(Encoding, sizeof(Encoding));

    err = reader.Next();
    SuccessOrExit(err);

    err = dataSink.StoreDataElement(kRootPropertyPathHandle, reader, 0, NULL, NULL);
    SuccessOrExit(err);

    // The 'SetLeafData' method on the datasink shouldn't get called since there are no properties in this trait.
    // Additionally, we should still receive events indicating data element begin/end.
    NL_TEST_ASSERT(inSuite, dataSink.mSetLeafDataCalled == false);
    NL_TEST_ASSERT(inSuite, dataSink.mEventDataElementBeginSignalled == true);
    NL_TEST_ASSERT(inSuite, dataSink.mEventDataElementEndSignalled == true);

exit:
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    return;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Testing NotificationEngine + TraitData
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current) {

using namespace Schema::Nest::Test::Trait;

//
// This is a source that publishes values for the test_h_trait. This includes providing values for two separate dictionaries
// as well as values for the rest of the fields in the static part of the schema. I designed the test_h_trait to specifically
// have all the fields be of the same type (i.e uint32_t) to focus the testing not on the value types, but rather validating the
// fields extracted by the Notification Engine.
//
class TestTdmSource : public TraitDataSource {
public:
    TestTdmSource();
    void SetValue(PropertyPathHandle aPropertyPathHandle, uint32_t aValue);
    void Reset();

private:
    WEAVE_ERROR GetLeafData(PropertyPathHandle aLeafHandle, uint64_t aTagToWrite, TLVWriter &aWriter);
    WEAVE_ERROR GetNextDictionaryItemKey(PropertyPathHandle aDictionaryHandle, uintptr_t &aContext, PropertyDictionaryKey &aKey);

public:
    std::map <PropertyPathHandle, uint32_t> mValues;
    std::map <uint16_t, TestHTrait::StructDictionary> mDictlValues;
    std::map <uint16_t, TestHTrait::StructDictionary> mDictSaValues;

    uint32_t mBackingValue;
};

TestTdmSource::TestTdmSource()
    : TraitDataSource(&TestHTrait::TraitSchema)
{
    mBackingValue = 1;
}

void TestTdmSource::SetValue(PropertyPathHandle aPropertyPathHandle, uint32_t aValue)
{
    mValues[aPropertyPathHandle] = aValue;
    SetDirty(aPropertyPathHandle);
}

void TestTdmSource::Reset()
{
    mValues.clear();
    mDictlValues.clear();
    mDictSaValues.clear();
    mBackingValue = 1;
}

WEAVE_ERROR TestTdmSource::GetNextDictionaryItemKey(PropertyPathHandle aDictionaryHandle, uintptr_t &aContext, PropertyDictionaryKey &aKey)
{
    static std::map<uint16_t, TestHTrait::StructDictionary>::iterator it;
    std::map <uint16_t, TestHTrait::StructDictionary> *mapPtr = (aDictionaryHandle == TestHTrait::kPropertyHandle_L) ? &mDictlValues : &mDictSaValues;

    if (aContext == 0) {
        it = mapPtr->begin();
    }
    else {
        it++;
    }

    aContext = (uintptr_t)&it;
    if (it == mapPtr->end()) {
        return WEAVE_END_OF_INPUT;
    }
    else {
        aKey = it->first;
    }

    return WEAVE_NO_ERROR;
}

WEAVE_ERROR TestTdmSource::GetLeafData(PropertyPathHandle aLeafHandle, uint64_t aTagToWrite, TLVWriter &aWriter)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    PropertyPathHandle dictionaryItemHandle = kNullPropertyPathHandle;

    if (GetSchemaEngine()->IsInDictionary(aLeafHandle, dictionaryItemHandle)) {
        PropertyPathHandle dictionaryHandle = GetSchemaEngine()->GetParent(dictionaryItemHandle);
        PropertyDictionaryKey key = GetPropertyDictionaryKey(dictionaryItemHandle);

        if (dictionaryHandle == TestHTrait::kPropertyHandle_L) {
            if (mDictlValues.find(key) != mDictlValues.end()) {
                TestHTrait::StructDictionary item = mDictlValues[key];
                uint32_t val;

                switch (GetPropertySchemaHandle(aLeafHandle)) {
                    case TestHTrait::kPropertyHandle_L_Value_Da:
                        WeaveLogDetail(DataManagement, "[TestTdmSource::GetLeafData] >> l[%u].da = %u", key, item.da);
                        val = item.da;
                        break;

                    case TestHTrait::kPropertyHandle_L_Value_Db:
                        WeaveLogDetail(DataManagement, "[TestTdmSource::GetLeafData] >> l[%u].db = %u", key, item.da);
                        val = item.db;
                        break;

                    case TestHTrait::kPropertyHandle_L_Value_Dc:
                        WeaveLogDetail(DataManagement, "[TestTdmSource::GetLeafData] >> l[%u].dc = %u", key, item.da);
                        val = item.dc;
                        break;

                    default:
                        WeaveLogError(DataManagement, "Unknown handle passed in!");
                        return WEAVE_ERROR_INVALID_ARGUMENT;
                        break;
                }

                err = aWriter.Put(aTagToWrite, val);
                SuccessOrExit(err);
            }
            else {
                WeaveLogError(DataManagement, "Requested key %u for dictionary handle %u that doesn't exist!", key, dictionaryHandle);
                return WEAVE_ERROR_INVALID_ARGUMENT;
            }
        }

        if (dictionaryHandle == TestHTrait::kPropertyHandle_K_Sa) {
            if (mDictSaValues.find(key) != mDictSaValues.end()) {
                TestHTrait::StructDictionary item = mDictSaValues[key];
                uint32_t val;

                switch (GetPropertySchemaHandle(aLeafHandle)) {
                    case TestHTrait::kPropertyHandle_K_Sa_Value_Da:
                        WeaveLogDetail(DataManagement, "[TestTdmSource::GetLeafData] >> k.sa[%u].da = %u", key, item.da);
                        val = item.da;
                        break;

                    case TestHTrait::kPropertyHandle_K_Sa_Value_Db:
                        WeaveLogDetail(DataManagement, "[TestTdmSource::GetLeafData] >> k.sa[%u].db = %u", key, item.db);
                        val = item.db;
                        break;

                    case TestHTrait::kPropertyHandle_K_Sa_Value_Dc:
                        WeaveLogDetail(DataManagement, "[TestTdmSource::GetLeafData] >> k.sa[%u].dc = %u", key, item.dc);
                        val = item.dc;
                        break;

                    default:
                        WeaveLogError(DataManagement, "Unknown handle passed in!");
                        return WEAVE_ERROR_INVALID_ARGUMENT;
                        break;
                }

                err = aWriter.Put(aTagToWrite, val);
                SuccessOrExit(err);
            }
            else {
                WeaveLogError(DataManagement, "Requested key %u for dictionary handle %u that doesn't exist!", key, dictionaryHandle);
                return WEAVE_ERROR_INVALID_ARGUMENT;
            }
        }
    }
    else {
        if (mValues.find(aLeafHandle) != mValues.end()) {
            WeaveLogDetail(DataManagement, "[TestTdmSource::GetLeafData] >> handle:%u = %u", aLeafHandle, mValues[aLeafHandle]);
            err = aWriter.Put(aTagToWrite, mValues[aLeafHandle]);
            SuccessOrExit(err);
        }
        else {
            WeaveLogDetail(DataManagement, "[TestTdmSource::GetLeafData] >> *handle:%u = %u", aLeafHandle, mBackingValue);
            err = aWriter.Put(aTagToWrite, mBackingValue);
        }
    }

exit:
    return err;
}


//
// This is a very special sink that tracks all the replaces, deletions and modifications that are sent to it through the
// OnEvent and SetLeafData calls. This then allows for programmatic validation of the specific set of data that is expected for a set of modifications
// that are made on the source side.
//
class TestTdmSink : public TraitDataSink {
public:
    TestTdmSink();
    void Reset();
    void DumpChangeSets();

    bool ValidateChangeSets(std::map <PropertyPathHandle, uint32_t> aTargetModifiedSet, std::set <PropertyPathHandle> aTargetDeletedSet, std::set <PropertyPathHandle> aTargetReplacedSet);

private:
    WEAVE_ERROR OnEvent(uint16_t aType, void *aInParam);
    WEAVE_ERROR SetLeafData(PropertyPathHandle aLeafHandle, nl::Weave::TLV::TLVReader &aReader);
    WEAVE_ERROR GetLeafData(PropertyPathHandle aLeafHandle, uint64_t aTagToWrite, TLVWriter &aWriter);
    std::map <PropertyPathHandle, uint32_t> mModifiedHandles;
    std::set <PropertyPathHandle> mDeletedHandles;
    std::set <PropertyPathHandle> mReplacedDictionaries;
};

TestTdmSink::TestTdmSink()
    : TraitDataSink(&TestHTrait::TraitSchema)
{
}

void TestTdmSink::Reset()
{
    mModifiedHandles.clear();
    mDeletedHandles.clear();
    mReplacedDictionaries.clear();
    ClearVersion();
}

void TestTdmSink::DumpChangeSets()
{
    for (std::map <PropertyPathHandle, uint32_t>::iterator map_it = mModifiedHandles.begin(); map_it != mModifiedHandles.end(); ++map_it) {
        WeaveLogDetail(DataManagement, "[TestTdmSink::DumpChangeSets] <Modified> %u:%u = %u", GetPropertyDictionaryKey(map_it->first), GetPropertySchemaHandle(map_it->first), map_it->second);
    }

    for (std::set <PropertyPathHandle>::iterator set_it = mDeletedHandles.begin(); set_it != mDeletedHandles.end(); ++set_it) {
        WeaveLogDetail(DataManagement, "[TestTdmSink::DumpChangeSets] <Deleted> %u:%u", GetPropertyDictionaryKey(*set_it), GetPropertySchemaHandle(*set_it));
    }

    for (std::set <PropertyPathHandle>::iterator set_it = mReplacedDictionaries.begin(); set_it != mReplacedDictionaries.end(); ++set_it) {
        WeaveLogDetail(DataManagement, "[TestTdmSink::DumpChangeSets] <Replaced> %u:%u", GetPropertyDictionaryKey(*set_it), GetPropertySchemaHandle(*set_it));
    }
}

bool TestTdmSink::ValidateChangeSets(std::map <PropertyPathHandle, uint32_t> aTargetModifiedSet, std::set <PropertyPathHandle> aTargetDeletedSet, std::set <PropertyPathHandle> aTargetReplacedSet)
{
    std::map <PropertyPathHandle, uint32_t> modifiedDiff;
    std::set <PropertyPathHandle> deletedDiff;
    std::set <PropertyPathHandle> replacedDiff;
    bool match = true;

    std::set_symmetric_difference(mModifiedHandles.begin(), mModifiedHandles.end(), aTargetModifiedSet.begin(), aTargetModifiedSet.end(), inserter(modifiedDiff, modifiedDiff.begin()));
    std::set_symmetric_difference(mDeletedHandles.begin(), mDeletedHandles.end(), aTargetDeletedSet.begin(), aTargetDeletedSet.end(), inserter(deletedDiff, deletedDiff.begin()));
    std::set_symmetric_difference(mReplacedDictionaries.begin(), mReplacedDictionaries.end(), aTargetReplacedSet.begin(), aTargetReplacedSet.end(), inserter(replacedDiff, replacedDiff.begin()));

    for (std::map <PropertyPathHandle, uint32_t>::iterator map_it = modifiedDiff.begin(); map_it != modifiedDiff.end(); ++map_it) {
        WeaveLogDetail(DataManagement, "[TestTdmSink::ValidateChangeSets] <delta modified> %u:%u = %u", GetPropertyDictionaryKey(map_it->first), GetPropertySchemaHandle(map_it->first), map_it->second);
        match = false;
    }

    for (std::set <PropertyPathHandle>::iterator set_it = deletedDiff.begin(); set_it != deletedDiff.end(); ++set_it) {
        WeaveLogDetail(DataManagement, "[TestTdmSink::ValidateChangeSets] <delta deleted> %u:%u", GetPropertyDictionaryKey(*set_it), GetPropertySchemaHandle(*set_it));
        match = false;
    }

    for (std::set <PropertyPathHandle>::iterator set_it = replacedDiff.begin(); set_it != replacedDiff.end(); ++set_it) {
        WeaveLogDetail(DataManagement, "[TestTdmSink::ValidateChangeSets] <delta replaced> %u:%u", GetPropertyDictionaryKey(*set_it), GetPropertySchemaHandle(*set_it));
        match = false;
    }

    return match;
}

WEAVE_ERROR TestTdmSink::OnEvent(uint16_t aType, void *aInParam)
{
    InEventParam *inParam = static_cast<InEventParam *>(aInParam);

    switch (aType) {
        case kEventDictionaryItemDelete:
            WeaveLogDetail(DataManagement, "[TestTdmSink::OnEvent] Deleting %u:%u", GetPropertyDictionaryKey(inParam->mDictionaryItemDelete.mTargetHandle), GetPropertySchemaHandle(inParam->mDictionaryItemDelete.mTargetHandle));
            mDeletedHandles.insert(inParam->mDictionaryItemDelete.mTargetHandle);
            break;

        case kEventDictionaryItemModifyBegin:
            WeaveLogDetail(DataManagement, "[TestTdmSink::OnEvent] Adding/Modifying %u:%u", GetPropertyDictionaryKey(inParam->mDictionaryItemModifyBegin.mTargetHandle), GetPropertySchemaHandle(inParam->mDictionaryItemModifyBegin.mTargetHandle));
            break;

        case kEventDictionaryReplaceBegin:
            WeaveLogDetail(DataManagement, "[TestTdmSink::OnEvent] Replacing %u:%u", GetPropertyDictionaryKey(inParam->mDictionaryReplaceBegin.mTargetHandle), GetPropertySchemaHandle(inParam->mDictionaryReplaceBegin.mTargetHandle));
            mReplacedDictionaries.insert(inParam->mDictionaryReplaceBegin.mTargetHandle);
            break;
    }

    return WEAVE_NO_ERROR;
}

WEAVE_ERROR TestTdmSink::SetLeafData(PropertyPathHandle aHandle, TLVReader &aReader)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint16_t val;

    err = aReader.Get(val);
    SuccessOrExit(err);

    WeaveLogDetail(DataManagement, "[TestTdmSink::SetLeafData] << %u:%u = %u", GetPropertyDictionaryKey(aHandle), GetPropertySchemaHandle(aHandle), val);
    mModifiedHandles[aHandle] = val;

exit:
    return err;
}

class TestTdm {
public:
    TestTdm();

    int Setup();
    int Teardown();
    int Reset();
    int BuildAndProcessNotify();

    void TestTdmStatic_SingleLeafHandle(nlTestSuite *inSuite);
    void TestTdmStatic_SingleLevelMerge(nlTestSuite *inSuite);
    void TestTdmStatic_SingleLevelMergeDeep(nlTestSuite *inSuite);
    void TestTdmStatic_DirtyStruct(nlTestSuite *inSuite);
    void TestTdmStatic_DirtyLeafUnevenDepth(nlTestSuite *inSuite);
    void TestTdmStatic_MergeHandleSetOverflow(nlTestSuite *inSuite);
    void TestTdmStatic_MarkLeafHandleDirtyTwice(nlTestSuite *inSuite);

    void TestTdmStatic_TestNullableLeaf(nlTestSuite *inSuite);
    void TestTdmStatic_TestNullableStruct(nlTestSuite *inSuite);
    void TestTdmStatic_TestNonNullableLeaf(nlTestSuite *inSuite);

    void TestTdmStatic_TestEphemeralLeaf(nlTestSuite *inSuite);
    void TestTdmStatic_TestEphemeralStruct(nlTestSuite *inSuite);

    void TestTdmStatic_TestIsParent(nlTestSuite *inSuite);

    void TestTdmMismatched_PathInDataElement(nlTestSuite *inSuite);
    void TestTdmMismatched_TopLevelPOD(nlTestSuite *inSuite);
    void TestTdmMismatched_NestedStruct(nlTestSuite *inSuite);
    void TestTdmMismatched_TopLevelStruct(nlTestSuite *inSuite);
    void TestTdmMismatched_SetLeafDataMismatch(nlTestSuite *inSuite);

    void TestTdmDictionary_DictionaryEntryAddition(nlTestSuite *inSuite);
    void TestTdmDictionary_DictionaryEntriesAddition(nlTestSuite *inSuite);
    void TestTdmDictionary_ReplaceDictionary(nlTestSuite *inSuite);

    void TestTdmDictionary_DeleteSingle(nlTestSuite *inSuite);
    void TestTdmDictionary_DeleteMultiple(nlTestSuite *inSuite);
    void TestTdmDictionary_DeleteHandleSetOverflow(nlTestSuite *inSuite);
    void TestTdmDictionary_AddDeleteDifferent(nlTestSuite *inSuite);
    void TestTdmDictionary_DeleteAndMarkDirty(nlTestSuite *inSuite);
    void TestTdmDictionary_MarkDirtyAndDelete(nlTestSuite *inSuite);
    void TestTdmDictionary_DeleteAndMarkFarDirty(nlTestSuite *inSuite);
    void TestTdmDictionary_AddAndDeleteSimilar(nlTestSuite *inSuite);
    void TestTdmDictionary_ModifyAndDeleteSimilar(nlTestSuite *inSuite);
    void TestTdmDictionary_DeleteAndModifySimilar(nlTestSuite *inSuite);
    void TestTdmDictionary_DeleteAndModifyLeafSimilar(nlTestSuite *inSuite);
    void TestTdmDictionary_DeleteStoreOverflowAndItemAddition(nlTestSuite *inSuite);
    void TestTdmDictionary_DirtyStoreOverflowAndItemDeletion(nlTestSuite *inSuite);
    void TestTdmDictionary_DeleteEntryTwice(nlTestSuite *inSuite);

    void TestRandomizedDataVersions(nlTestSuite *inSuite);

    void TestTdmStatic_MultiInstance(nlTestSuite *inSuite);

    void CheckAllocateRightSizedBufferForNotifications(nlTestSuite *inSuite);

private:
    SubscriptionHandler *mSubHandler;
    SubscriptionClient *mSubClient;
    NotificationEngine *mNotificationEngine;

    SubscriptionEngine mSubscriptionEngine;
    WeaveExchangeManager mExchangeMgr;
    SingleResourceSourceTraitCatalog::CatalogItem mSourceCatalogStore[4];
    SingleResourceSourceTraitCatalog mSourceCatalog;
    SingleResourceSinkTraitCatalog::CatalogItem mSinkCatalogStore[4];
    SingleResourceSinkTraitCatalog mSinkCatalog;
    TestTdmSource mTestTdmSource;
    TestTdmSource mTestTdmSource1;
    TestTdmSink mTestTdmSink;
    TestTdmSink mTestTdmSink1;
    TestMismatchedCTraitDataSource mMismatchedTestCSource;
    TestCTraitDataSink mTestCSink;
    TestMismatchedCTraitDataSink mMismatchedTestCSink;

    TestBTraitDataSource mTestBSource;
    TestBTraitDataSink mTestBSink;

    Binding *mClientBinding;

    uint32_t mTestCase;

    WEAVE_ERROR AllocateBuffer(uint32_t desiredSize, uint32_t minSize);
};

TestTdm::TestTdm()
    : mSourceCatalog(ResourceIdentifier(ResourceIdentifier::SELF_NODE_ID), mSourceCatalogStore, 4),
      mSinkCatalog(ResourceIdentifier(ResourceIdentifier::SELF_NODE_ID), mSinkCatalogStore, 4),
      mClientBinding(NULL)
{
    mTestCase = 0;
}

int TestTdm::Setup()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TraitDataHandle testTdmSourceHandle;
    TraitDataHandle testTdmSourceHandle1;
    TraitDataHandle testTdmSinkHandle;
    TraitDataHandle testTdmSinkHandle1;
    TraitDataHandle testMismatchedCSourceHandle;
    TraitDataHandle testCSinkHandle;
    TraitDataHandle testBSourceHandle, testBSinkHandle;
    SubscriptionHandler::TraitInstanceInfo *traitInstance = NULL;

    gSubscriptionEngine = &mSubscriptionEngine;

    // Initialize SubEngine and set it up
    err = mSubscriptionEngine.Init(&ExchangeMgr, NULL, NULL);
    SuccessOrExit(err);

    err = mSubscriptionEngine.EnablePublisher(NULL, &mSourceCatalog);
    SuccessOrExit(err);

    // Get a sub handler and prime it to the right state
    err = mSubscriptionEngine.NewSubscriptionHandler(&mSubHandler);
    SuccessOrExit(err);

    mSubHandler->mBinding = ExchangeMgr.NewBinding();
    mSubHandler->mBinding->BeginConfiguration().Transport_UDP();

    mClientBinding = ExchangeMgr.NewBinding();

    err = mSubscriptionEngine.NewClient(&mSubClient, mClientBinding, NULL, NULL, &mSinkCatalog, 0);
    SuccessOrExit(err);

    mNotificationEngine = &mSubscriptionEngine.mNotificationEngine;

    mSourceCatalog.Add(0, &mTestTdmSource, testTdmSourceHandle);
    mSourceCatalog.Add(1, &mTestTdmSource1, testTdmSourceHandle1);

    mSourceCatalog.Add(2, &mMismatchedTestCSource, testMismatchedCSourceHandle);

    mSourceCatalog.Add(3, &mTestBSource, testBSourceHandle);

    mSinkCatalog.Add(0, &mTestTdmSink, testTdmSinkHandle);
    mSinkCatalog.Add(1, &mTestTdmSink1, testTdmSinkHandle1);

    mSinkCatalog.Add(2, &mTestCSink, testCSinkHandle);

    mSinkCatalog.Add(3, &mTestBSink, testBSinkHandle);

    traitInstance = mSubscriptionEngine.mTraitInfoPool;
    mSubHandler->mTraitInstanceList = traitInstance;
    mSubHandler->mNumTraitInstances++;
    ++(SubscriptionEngine::GetInstance()->mNumTraitInfosInPool);

    traitInstance->Init();
    traitInstance->mTraitDataHandle = testTdmSourceHandle;
    traitInstance->mRequestedVersion = 1;

    traitInstance = mSubscriptionEngine.mTraitInfoPool + 1;
    mSubHandler->mNumTraitInstances++;
    ++(SubscriptionEngine::GetInstance()->mNumTraitInfosInPool);

    traitInstance->Init();
    traitInstance->mTraitDataHandle = testTdmSourceHandle1;
    traitInstance->mRequestedVersion = 1;

    traitInstance = mSubscriptionEngine.mTraitInfoPool + 2;
    mSubHandler->mNumTraitInstances++;
    ++(SubscriptionEngine::GetInstance()->mNumTraitInfosInPool);

    traitInstance->Init();
    traitInstance->mTraitDataHandle = testMismatchedCSourceHandle;
    traitInstance->mRequestedVersion = 1;

    traitInstance = mSubscriptionEngine.mTraitInfoPool + 3;
    mSubHandler->mNumTraitInstances++;
    ++(SubscriptionEngine::GetInstance()->mNumTraitInfosInPool);

    traitInstance->Init();
    traitInstance->mTraitDataHandle = testBSourceHandle;
    traitInstance->mRequestedVersion = 1;

exit:
    if (err != WEAVE_NO_ERROR) {
        WeaveLogError(DataManagement, "Error setting up test: %d", err);
    }

    return err;
}

int TestTdm::Teardown()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (mClientBinding != NULL)
    {
        mClientBinding->Release();
        mClientBinding = NULL;
    }

    return err;
}

int TestTdm::Reset()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    mSubHandler->MoveToState(SubscriptionHandler::kState_SubscriptionEstablished_Idle);
    mTestTdmSink.Reset();
    mTestTdmSink1.Reset();

    mTestTdmSource.Reset();
    mTestTdmSource1.Reset();

    mMismatchedTestCSource.Reset();
    mTestCSink.Reset();

    mTestBSink.Reset();
    mTestBSource.Reset();

    mNotificationEngine->mGraphSolver.ClearDirty();

    return err;
}

int TestTdm::BuildAndProcessNotify()
{
    bool isSubscriptionClean;
    NotificationEngine::NotifyRequestBuilder notifyRequest;
    NotificationRequest::Parser notify;
    PacketBuffer *buf = NULL;
    TLVWriter writer;
    TLVReader reader;
    TLVType dummyType1, dummyType2;
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    bool neWriteInProgress = false;
    uint32_t maxNotificationSize = 0;
    uint32_t maxPayloadSize = 0;

    maxNotificationSize = mSubHandler->GetMaxNotificationSize();

    err = mSubHandler->mBinding->AllocateRightSizedBuffer(buf, maxNotificationSize, WDM_MIN_NOTIFICATION_SIZE, maxPayloadSize);
    SuccessOrExit(err);

    err = notifyRequest.Init(buf, &writer, mSubHandler, maxPayloadSize);
    SuccessOrExit(err);

    err = mNotificationEngine->BuildSingleNotifyRequestDataList(mSubHandler, notifyRequest, isSubscriptionClean, neWriteInProgress);
    SuccessOrExit(err);

    if (neWriteInProgress)
    {
        err = notifyRequest.MoveToState(NotificationEngine::kNotifyRequestBuilder_Idle);
        SuccessOrExit(err);

        reader.Init(buf);

        err = reader.Next();
        SuccessOrExit(err);

        notify.Init(reader);

        err = notify.CheckSchemaValidity();
        SuccessOrExit(err);

        // Enter the struct
        err = reader.EnterContainer(dummyType1);
        SuccessOrExit(err);

        // SubscriptionId
        err = reader.Next();
        SuccessOrExit(err);

        err = reader.Next();
        SuccessOrExit(err);

        VerifyOrExit(nl::Weave::TLV::kTLVType_Array == reader.GetType(), err = WEAVE_ERROR_WRONG_TLV_TYPE);

        err = reader.EnterContainer(dummyType2);
        SuccessOrExit(err);

        err = mSubClient->ProcessDataList(reader);
        SuccessOrExit(err);
    }
    else
    {
        WeaveLogDetail(DataManagement, "nothing has been written");
    }

exit:
    if (buf) {
        PacketBuffer::Free(buf);
    }

    return err;
}

void TestTdm::TestTdmStatic_MultiInstance(nlTestSuite *inSuite)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    bool testPass = false;

    Reset();

    mTestTdmSource.SetValue(TestHTrait::kPropertyHandle_A, 2);
    mTestTdmSource1.SetValue(TestHTrait::kPropertyHandle_B, 2);

    err = BuildAndProcessNotify();
    SuccessOrExit(err);

    testPass = mTestTdmSink.ValidateChangeSets( { { TestHTrait::kPropertyHandle_A, 2 } },
                                                { },
                                                { } );
    VerifyOrExit(testPass, );

    testPass = mTestTdmSink1.ValidateChangeSets( { { TestHTrait::kPropertyHandle_B, 2 } },
                                                { },
                                                { } );
    VerifyOrExit(testPass, );

exit:
    NL_TEST_ASSERT(inSuite, testPass);
}

void TestTdm::TestTdmStatic_SingleLeafHandle(nlTestSuite *inSuite)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    bool testPass = false;

    Reset();
    mTestTdmSource.SetValue(TestHTrait::kPropertyHandle_A, 2);

    err = BuildAndProcessNotify();
    SuccessOrExit(err);

    testPass = mTestTdmSink.ValidateChangeSets( { { TestHTrait::kPropertyHandle_A, 2 } },
                                                { },
                                                { } );

exit:
    NL_TEST_ASSERT(inSuite, testPass);
}

void TestTdm::TestTdmStatic_SingleLevelMerge(nlTestSuite *inSuite)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    bool testPass = false;

    Reset();
    mTestTdmSource.SetValue(TestHTrait::kPropertyHandle_B, 2);
    mTestTdmSource.SetValue(TestHTrait::kPropertyHandle_A, 2);

    err = BuildAndProcessNotify();
    SuccessOrExit(err);

    testPass = mTestTdmSink.ValidateChangeSets( { { TestHTrait::kPropertyHandle_B, 2 }, { TestHTrait::kPropertyHandle_A, 2 } },
                                                { },
                                                { });
exit:
    NL_TEST_ASSERT(inSuite, testPass);
}

void TestTdm::TestTdmStatic_SingleLevelMergeDeep(nlTestSuite *inSuite)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    bool testPass = false;

    Reset();

    mTestTdmSource.SetValue(TestHTrait::kPropertyHandle_K_Sb, 2);
    mTestTdmSource.SetValue(TestHTrait::kPropertyHandle_K_Sc, 2);

    err = BuildAndProcessNotify();
    SuccessOrExit(err);

    testPass = mTestTdmSink.ValidateChangeSets( { { TestHTrait::kPropertyHandle_K_Sb, 2 }, { TestHTrait::kPropertyHandle_K_Sc, 2 } },
                                                { },
                                                { });

exit:
    NL_TEST_ASSERT(inSuite, testPass);
}

void TestTdm::TestTdmStatic_DirtyStruct(nlTestSuite *inSuite)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    bool testPass = false;

    Reset();

    mTestTdmSource.SetDirty(TestHTrait::kPropertyHandle_K);

    err = BuildAndProcessNotify();
    SuccessOrExit(err);

    testPass = mTestTdmSink.ValidateChangeSets( { { TestHTrait::kPropertyHandle_K_Sb, 1 }, { TestHTrait::kPropertyHandle_K_Sc, 1 } },
                                                { },
                                                { TestHTrait::kPropertyHandle_K_Sa });

exit:
    NL_TEST_ASSERT(inSuite, testPass);
}

void TestTdm::TestTdmStatic_DirtyLeafUnevenDepth(nlTestSuite *inSuite)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    bool testPass = false;

    Reset();

    mTestTdmSource.SetDirty(TestHTrait::kPropertyHandle_A);
    mTestTdmSource.SetDirty(TestHTrait::kPropertyHandle_K_Sb);

    err = BuildAndProcessNotify();
    SuccessOrExit(err);

    testPass = mTestTdmSink.ValidateChangeSets( { { TestHTrait::kPropertyHandle_A, 1 }, { TestHTrait::kPropertyHandle_K_Sb, 1 },
                                                  { TestHTrait::kPropertyHandle_K_Sc, 1 } },
                                                { },
                                                { TestHTrait::kPropertyHandle_K_Sa } );

exit:
    NL_TEST_ASSERT(inSuite, testPass);
}


void TestTdm::TestTdmStatic_MergeHandleSetOverflow(nlTestSuite *inSuite)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    bool testPass = false;

    Reset();

    // This should overflow the 4 merge handle set limitation, resulting in root being marked dirty.
    // **NOTE** If you increase this merge handle limit, then this test will have to be altered too!
    mTestTdmSource.SetDirty(TestHTrait::kPropertyHandle_A);
    mTestTdmSource.SetDirty(TestHTrait::kPropertyHandle_B);
    mTestTdmSource.SetDirty(TestHTrait::kPropertyHandle_C);
    mTestTdmSource.SetDirty(TestHTrait::kPropertyHandle_D);
    mTestTdmSource.SetDirty(TestHTrait::kPropertyHandle_E);
    mTestTdmSource.SetDirty(TestHTrait::kPropertyHandle_F);

    err = BuildAndProcessNotify();
    SuccessOrExit(err);

    testPass = mTestTdmSink.ValidateChangeSets( { { TestHTrait::kPropertyHandle_A, 1 }, { TestHTrait::kPropertyHandle_B, 1 },
                                                  { TestHTrait::kPropertyHandle_C, 1 }, { TestHTrait::kPropertyHandle_D, 1 },
                                                  { TestHTrait::kPropertyHandle_E, 1 }, { TestHTrait::kPropertyHandle_F, 1 },
                                                  { TestHTrait::kPropertyHandle_G, 1 }, { TestHTrait::kPropertyHandle_H, 1 },
                                                  { TestHTrait::kPropertyHandle_I, 1 }, { TestHTrait::kPropertyHandle_J, 1 },
                                                  { TestHTrait::kPropertyHandle_K_Sb, 1 }, { TestHTrait::kPropertyHandle_K_Sc, 1 } },
                                                { },
                                                { TestHTrait::kPropertyHandle_K_Sa, TestHTrait::kPropertyHandle_L });

exit:
    NL_TEST_ASSERT(inSuite, testPass);
}

void TestTdm::TestTdmStatic_MarkLeafHandleDirtyTwice(nlTestSuite *inSuite)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    bool testPass = false;

    Reset();

    mTestTdmSource.SetValue(TestHTrait::kPropertyHandle_A, 2);
    mTestTdmSource.SetValue(TestHTrait::kPropertyHandle_A, 2);

    err = BuildAndProcessNotify();
    SuccessOrExit(err);

    testPass = mTestTdmSink.ValidateChangeSets( { { TestHTrait::kPropertyHandle_A, 2 } },
                                                { },
                                                { } );

exit:
    NL_TEST_ASSERT(inSuite, testPass);
}

void TestTdm::TestTdmStatic_TestNullableLeaf(nlTestSuite *inSuite)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    Reset();

    mTestBSource.SetNullifiedPath(TestBTrait::kPropertyHandle_TaD_SaA, true);
    mTestBSource.SetDirty(TestBTrait::kPropertyHandle_Root);

    err = BuildAndProcessNotify();
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    NL_TEST_ASSERT(inSuite, mTestBSink.IsPathHandleNull(TestBTrait::kPropertyHandle_TaD_SaA));

    // set value and re-test
    mTestBSource.SetNullifiedPath(TestBTrait::kPropertyHandle_TaD_SaA, false);
    mTestBSource.SetDirty(TestBTrait::kPropertyHandle_Root);

    err = BuildAndProcessNotify();
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    NL_TEST_ASSERT(inSuite, mTestBSink.IsPathHandleNull(TestBTrait::kPropertyHandle_TaD_SaA) == false);
    NL_TEST_ASSERT(inSuite, mTestBSink.IsPathHandleSet(TestBTrait::kPropertyHandle_TaD_SaA));
}

void TestTdm::TestTdmStatic_TestNullableStruct(nlTestSuite *inSuite)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    bool anyChildrenSet = false;
    Reset();

    mTestBSource.SetNullifiedPath(TestBTrait::kPropertyHandle_TaD, true);
    mTestBSource.SetDirty(TestBTrait::kPropertyHandle_Root);

    err = BuildAndProcessNotify();
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    NL_TEST_ASSERT(inSuite, mTestBSink.IsPathHandleNull(TestBTrait::kPropertyHandle_TaD));
    NL_TEST_ASSERT(inSuite, mTestBSink.IsPathHandleSet(TestBTrait::kPropertyHandle_TaD));

    for (PropertyPathHandle i = TestBTrait::kPropertyHandle_TaD_SaA; i <= TestBTrait::kPropertyHandle_TaD_SaB; i++)
    {
        anyChildrenSet |= mTestBSink.IsPathHandleSet(i);
    }
    NL_TEST_ASSERT(inSuite, anyChildrenSet == false);
}

void TestTdm::TestTdmStatic_TestNonNullableLeaf(nlTestSuite *inSuite)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    Reset();

    mTestBSource.SetNullifiedPath(TestBTrait::kPropertyHandle_TbB_SbB, true);
    mTestBSource.SetDirty(TestBTrait::kPropertyHandle_Root);

    err = BuildAndProcessNotify();
    NL_TEST_ASSERT(inSuite, err == WEAVE_ERROR_WDM_SCHEMA_MISMATCH);
}

void TestTdm::TestTdmStatic_TestEphemeralLeaf(nlTestSuite *inSuite)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    Reset();

    mTestBSource.SetPresentPath(TestBTrait::kPropertyHandle_TaD_SaA, false);
    mTestBSource.SetDirty(TestBTrait::kPropertyHandle_Root);

    err = BuildAndProcessNotify();
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    NL_TEST_ASSERT(inSuite, mTestBSink.IsPathHandleSet(TestBTrait::kPropertyHandle_TaD_SaA) == false);
}

void TestTdm::TestTdmStatic_TestEphemeralStruct(nlTestSuite *inSuite)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    bool anyChildrenSet = false;
    Reset();

    mTestBSource.SetPresentPath(TestBTrait::kPropertyHandle_TaD, false);
    mTestBSource.SetDirty(TestBTrait::kPropertyHandle_Root);

    err = BuildAndProcessNotify();
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    NL_TEST_ASSERT(inSuite, mTestBSink.IsPathHandleSet(TestBTrait::kPropertyHandle_TaD) == false);
    for (PropertyPathHandle i = TestBTrait::kPropertyHandle_TaD_SaA; i <= TestBTrait::kPropertyHandle_TaD_SaB; i++)
    {
        anyChildrenSet |= mTestBSink.IsPathHandleSet(i);
    }
    NL_TEST_ASSERT(inSuite, anyChildrenSet == false);
}

void TestTdm::TestTdmStatic_TestIsParent(nlTestSuite *inSuite)
{
    const TraitSchemaEngine *se = mTestBSource.GetSchemaEngine();
    PropertyPathHandle leafInDictionary = CreatePropertyPathHandle(TestBTrait::kPropertyHandle_TaJ_Value_SaA, 3);

    // Remember that here "parent" really means "ancestor"

    // Check that:
    // 0. if any one of the argumets is kNullPropertyPathHandle, the result is false
    // 1. a property path is not its own parent
    // 2. root is a parent of any other property path
    // 3. the usual cases

    // kPropertyHandle_TaA is a leaf;
    // kPropertyHandle_TaD is a structure;
    // kPropertyHandle_TaD_SaA is a leaf inside TaD
    // kPropertyHandle_TaI and kPropertyHandle_TaJ are dictionaries

    NL_TEST_ASSERT(inSuite, false == se->IsParent(CreatePropertyPathHandle(TestBTrait::kPropertyHandle_TaD),
                                                  CreatePropertyPathHandle(TestBTrait::kPropertyHandle_TaD)));

    NL_TEST_ASSERT(inSuite, false == se->IsParent(CreatePropertyPathHandle(TestBTrait::kPropertyHandle_TaA),
                                                  CreatePropertyPathHandle(TestBTrait::kPropertyHandle_TaA)));

    NL_TEST_ASSERT(inSuite, false == se->IsParent(kRootPropertyPathHandle,
                                                  kRootPropertyPathHandle));

    NL_TEST_ASSERT(inSuite,          se->IsParent(CreatePropertyPathHandle(TestBTrait::kPropertyHandle_TaD_SaA),
                                                  CreatePropertyPathHandle(TestBTrait::kPropertyHandle_TaD)));

    NL_TEST_ASSERT(inSuite, false == se->IsParent(CreatePropertyPathHandle(TestBTrait::kPropertyHandle_TaD),
                                                  CreatePropertyPathHandle(TestBTrait::kPropertyHandle_TaD_SaA)));

    NL_TEST_ASSERT(inSuite, false == se->IsParent(kRootPropertyPathHandle,
                                                  CreatePropertyPathHandle(TestBTrait::kPropertyHandle_TaD)));

    NL_TEST_ASSERT(inSuite,          se->IsParent(CreatePropertyPathHandle(TestBTrait::kPropertyHandle_TaD),
                                                  kRootPropertyPathHandle));

    NL_TEST_ASSERT(inSuite, false == se->IsParent(CreatePropertyPathHandle(TestBTrait::kPropertyHandle_TaD_SaA),
                                                  CreatePropertyPathHandle(TestBTrait::kPropertyHandle_TaA)));

    NL_TEST_ASSERT(inSuite, false == se->IsParent(CreatePropertyPathHandle(kNullPropertyPathHandle),
                                                  CreatePropertyPathHandle(TestBTrait::kPropertyHandle_TaA)));

    NL_TEST_ASSERT(inSuite, false == se->IsParent(CreatePropertyPathHandle(TestBTrait::kPropertyHandle_TaA),
                                                  CreatePropertyPathHandle(kNullPropertyPathHandle)));

    NL_TEST_ASSERT(inSuite,          se->IsParent(se->GetDictionaryItemHandle(TestBTrait::kPropertyHandle_TaI, 3),
                                                  CreatePropertyPathHandle(TestBTrait::kPropertyHandle_TaI)));

    NL_TEST_ASSERT(inSuite, false == se->IsParent(CreatePropertyPathHandle(TestBTrait::kPropertyHandle_TaI),
                                                  se->GetDictionaryItemHandle(TestBTrait::kPropertyHandle_TaI, 3)));

    NL_TEST_ASSERT(inSuite,          se->IsParent(CreatePropertyPathHandle(TestBTrait::kPropertyHandle_TaJ_Value_SaA, 3),
                                                  CreatePropertyPathHandle(TestBTrait::kPropertyHandle_TaJ_Value, 3)));

    // the dictionary item structure with key 2 is not the parent of a leaf with key 3
    NL_TEST_ASSERT(inSuite, false == se->IsParent(CreatePropertyPathHandle(TestBTrait::kPropertyHandle_TaJ_Value_SaA, 3),
                                                  CreatePropertyPathHandle(TestBTrait::kPropertyHandle_TaJ_Value, 2)));

    // Note that these two are the same:
    NL_TEST_ASSERT(inSuite, CreatePropertyPathHandle(TestBTrait::kPropertyHandle_TaJ_Value, 2) == se->GetDictionaryItemHandle(TestBTrait::kPropertyHandle_TaJ, 2));


    // the dictionary item structure with key 0 is not the parent of a leaf with key 3
    NL_TEST_ASSERT(inSuite, false == se->IsParent(CreatePropertyPathHandle(TestBTrait::kPropertyHandle_TaJ_Value_SaA, 3),
                                                  CreatePropertyPathHandle(TestBTrait::kPropertyHandle_TaJ_Value)));

    NL_TEST_ASSERT(inSuite,          se->IsParent(CreatePropertyPathHandle(TestBTrait::kPropertyHandle_TaJ_Value_SaA, 3),
                                                  CreatePropertyPathHandle(TestBTrait::kPropertyHandle_TaJ)));

    NL_TEST_ASSERT(inSuite,          se->IsParent(CreatePropertyPathHandle(TestBTrait::kPropertyHandle_TaJ_Value_SaA, 3),
                                                  kRootPropertyPathHandle));

    NL_TEST_ASSERT(inSuite, false == se->IsParent(CreatePropertyPathHandle(TestBTrait::kPropertyHandle_TaI),
                                                  se->GetDictionaryItemHandle(TestBTrait::kPropertyHandle_TaI, 3)));

    NL_TEST_ASSERT(inSuite, false == se->IsParent(CreatePropertyPathHandle(TestBTrait::kPropertyHandle_TaJ_Value_SaA, 3),
                                                  CreatePropertyPathHandle(TestBTrait::kPropertyHandle_TaI)));

    // Check all ancestors of a leaf in a dictionary:
    for (PropertyPathHandle ancestor = se->GetParent(leafInDictionary); ancestor != kNullPropertyPathHandle; ancestor = se->GetParent(ancestor))
    {
        NL_TEST_ASSERT(inSuite, se->IsParent(leafInDictionary, ancestor));
        NL_TEST_ASSERT(inSuite, false == se->IsParent(ancestor, leafInDictionary));
    }
}

void TestTdm::TestTdmMismatched_PathInDataElement(nlTestSuite *inSuite)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    Reset();

    // set tc_d (unknown to subscriber) - path in DataElement will be unrecognizable
    mMismatchedTestCSource.SetValue(TestMismatchedCTrait::kPropertyHandle_TcE_ScA, 10);

    err = BuildAndProcessNotify();
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    // no leaf data should be set
    NL_TEST_ASSERT(inSuite, mTestCSink.WasAnyPathHandleSet() == false);
}

void TestTdm::TestTdmMismatched_TopLevelPOD(nlTestSuite *inSuite)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    Reset();

    // set tc_a (known to subscriber) and tc_d (unknown to subscriber)
    mMismatchedTestCSource.SetValue(TestMismatchedCTrait::kPropertyHandle_TcA, 10);
    mMismatchedTestCSource.SetValue(TestMismatchedCTrait::kPropertyHandle_TcE_ScA, 10);

    err = BuildAndProcessNotify();
    // SetLeafData returns error for unrecognized paths
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    // check known path is successfully set
    NL_TEST_ASSERT(inSuite, mTestCSink.WasPathHandleSet(TestCTrait::kPropertyHandle_TcA));
}

void TestTdm::TestTdmMismatched_NestedStruct(nlTestSuite *inSuite)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    Reset();

    // set tc_c.ScA (known to subscriber) and tc_c.ScC (unknown to subscriber)
    mMismatchedTestCSource.SetValue(TestMismatchedCTrait::kPropertyHandle_TcC_ScA, 10);
    mMismatchedTestCSource.SetValue(TestMismatchedCTrait::kPropertyHandle_TcC_ScC, 10);

    err = BuildAndProcessNotify();
    // SetLeafData returns error for unrecognized paths
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    // check known path is successfully set
    NL_TEST_ASSERT(inSuite, mTestCSink.WasPathHandleSet(TestCTrait::kPropertyHandle_TcC_ScA));
}

void TestTdm::TestTdmMismatched_TopLevelStruct(nlTestSuite *inSuite)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    Reset();

    mMismatchedTestCSource.SetValue(TestMismatchedCTrait::kPropertyHandle_TcA, 10);
    mMismatchedTestCSource.SetValue(TestMismatchedCTrait::kPropertyHandle_TcE_ScA, 10);

    err = BuildAndProcessNotify();
    // SetLeafData returns error for unrecognized paths
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    // check known path is successfully set
    NL_TEST_ASSERT(inSuite, mTestCSink.WasPathHandleSet(TestCTrait::kPropertyHandle_TcA));
}

void TestTdm::TestTdmMismatched_SetLeafDataMismatch(nlTestSuite *inSuite)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TraitDataHandle mismatchedTestCSinkHandle;
    Reset();

    // sub out mTestCSink for mMismatchedTestCSink
    mSinkCatalog.Remove(2);
    mSinkCatalog.Add(2, &mMismatchedTestCSink, mismatchedTestCSinkHandle);

    mMismatchedTestCSource.SetValue(TestMismatchedCTrait::kPropertyHandle_TcA, 10);
    mMismatchedTestCSource.SetValue(TestMismatchedCTrait::kPropertyHandle_TcE_ScA, 10);

    err = BuildAndProcessNotify();
    // SetLeafData returns error for unrecognized paths
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    NL_TEST_ASSERT(inSuite, mMismatchedTestCSink.WasPathHandleSet(TestMismatchedCTrait::kPropertyHandle_TcE_ScA));
}

void TestTdm::TestTdmDictionary_DictionaryEntryAddition(nlTestSuite *inSuite)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    bool testPass = false;

    Reset();

    mTestTdmSource.mDictlValues[0] = { 1, 1, 1 };
    mTestTdmSource.SetDirty(CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value, 0));

    err = BuildAndProcessNotify();
    SuccessOrExit(err);

    testPass = mTestTdmSink.ValidateChangeSets( { { CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value_Da, 0), 1 },
                                                  { CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value_Db, 0), 1 },
                                                  { CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value_Dc, 0), 1 } },
                                                { },
                                                { });

exit:
    NL_TEST_ASSERT(inSuite, testPass);
}

void TestTdm::TestTdmDictionary_DictionaryEntriesAddition(nlTestSuite *inSuite)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    bool testPass = false;

    Reset();

    mTestTdmSource.mDictlValues[1] = { 1, 1, 1 };
    mTestTdmSource.mDictlValues[2] = { 1, 1, 1 };
    mTestTdmSource.SetDirty(CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value, 1));
    mTestTdmSource.SetDirty(CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value, 2));

    err = BuildAndProcessNotify();
    SuccessOrExit(err);

    testPass = mTestTdmSink.ValidateChangeSets( { { CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value_Da, 1), 1 },
                                                  { CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value_Db, 1), 1 },
                                                  { CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value_Dc, 1), 1 },
                                                  { CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value_Da, 2), 1 },
                                                  { CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value_Db, 2), 1 },
                                                  { CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value_Dc, 2), 1 } },
                                                { },
                                                { });

exit:
    NL_TEST_ASSERT(inSuite, testPass);
}

void TestTdm::TestTdmDictionary_ReplaceDictionary(nlTestSuite *inSuite)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    bool testPass = false;

    Reset();

    mTestTdmSource.mDictlValues[0] = { 1, 1, 1 };
    mTestTdmSource.mDictlValues[1] = { 1, 1, 1 };
    mTestTdmSource.mDictlValues[2] = { 1, 1, 1 };

    mTestTdmSource.SetDirty(TestHTrait::kPropertyHandle_L);

    err = BuildAndProcessNotify();
    SuccessOrExit(err);

    testPass = mTestTdmSink.ValidateChangeSets( { { CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value_Da, 0), 1 },
                                                  { CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value_Db, 0), 1 },
                                                  { CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value_Dc, 0), 1 },
                                                  { CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value_Da, 1), 1 },
                                                  { CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value_Db, 1), 1 },
                                                  { CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value_Dc, 1), 1 },
                                                  { CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value_Da, 2), 1 },
                                                  { CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value_Db, 2), 1 },
                                                  { CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value_Dc, 2), 1 } },
                                                { },
                                                { TestHTrait::kPropertyHandle_L });

exit:
    NL_TEST_ASSERT(inSuite, testPass);
}

void TestTdm::TestTdmDictionary_DeleteSingle(nlTestSuite *inSuite)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    bool testPass = false;

    Reset();

    mTestTdmSource.mDictlValues[0] = { 1, 1, 1 };
    mTestTdmSource.mDictlValues[1] = { 1, 1, 1 };
    mTestTdmSource.mDictlValues[2] = { 1, 1, 1 };
    mTestTdmSource.mDictlValues.erase(2);

    mTestTdmSource.DeleteKey(CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value, 2));

    err = BuildAndProcessNotify();
    SuccessOrExit(err);

    mTestTdmSink.DumpChangeSets();

    testPass = mTestTdmSink.ValidateChangeSets( { },
                                                { CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value, 2) },
                                                { });

exit:
    NL_TEST_ASSERT(inSuite, testPass);
}

void TestTdm::TestTdmDictionary_DeleteMultiple(nlTestSuite *inSuite)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    bool testPass = false;

    Reset();

    mTestTdmSource.mDictlValues[0] = { 1, 1, 1 };
    mTestTdmSource.mDictlValues[1] = { 1, 1, 1 };
    mTestTdmSource.mDictlValues[2] = { 1, 1, 1 };
    mTestTdmSource.mDictlValues.erase(2);
    mTestTdmSource.mDictlValues.erase(1);

    mTestTdmSource.DeleteKey(CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value, 1));
    mTestTdmSource.DeleteKey(CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value, 2));

    err = BuildAndProcessNotify();
    SuccessOrExit(err);

    testPass = mTestTdmSink.ValidateChangeSets( { },
                                                { CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value, 2), CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value, 1) },
                                                { });

exit:
    NL_TEST_ASSERT(inSuite, testPass);
}

void TestTdm::TestTdmDictionary_DeleteHandleSetOverflow(nlTestSuite *inSuite)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    bool testPass = false;

    Reset();

    // We overflow the delete handle set, which should result in a replace of the parent dictionary.
    // Thus, we should be getting a replace + all the elements in the dictionary
    mTestTdmSource.mDictlValues[0] = { 1, 1, 1 };
    mTestTdmSource.mDictlValues[1] = { 1, 1, 1 };
    mTestTdmSource.mDictlValues[2] = { 1, 1, 1 };
    mTestTdmSource.mDictlValues[3] = { 1, 1, 1 };
    mTestTdmSource.mDictlValues[4] = { 1, 1, 1 };
    mTestTdmSource.mDictlValues.erase(0);
    mTestTdmSource.mDictlValues.erase(1);
    mTestTdmSource.mDictlValues.erase(2);
    mTestTdmSource.mDictlValues.erase(3);
    mTestTdmSource.mDictlValues.erase(4);

    mTestTdmSource.DeleteKey(CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value, 0));
    mTestTdmSource.DeleteKey(CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value, 1));
    mTestTdmSource.DeleteKey(CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value, 2));
    mTestTdmSource.DeleteKey(CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value, 3));
    mTestTdmSource.DeleteKey(CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value, 4));

    err = BuildAndProcessNotify();
    SuccessOrExit(err);

    testPass = mTestTdmSink.ValidateChangeSets( { },
                                                { },
                                                { TestHTrait::kPropertyHandle_L });

exit:
    NL_TEST_ASSERT(inSuite, testPass);
}


void TestTdm::TestTdmDictionary_AddDeleteDifferent(nlTestSuite *inSuite)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    bool testPass = false;

    Reset();

    mTestTdmSource.mDictlValues[0] = { 1, 1, 1 };
    mTestTdmSource.mDictlValues[1] = { 1, 1, 1 };
    mTestTdmSource.mDictlValues.erase(0);

    mTestTdmSource.DeleteKey(CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value, 0));
    mTestTdmSource.SetDirty(CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value, 1));

    err = BuildAndProcessNotify();
    SuccessOrExit(err);


    testPass = mTestTdmSink.ValidateChangeSets( { { CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value_Da, 1), 1 },
                                                  { CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value_Db, 1), 1 },
                                                  { CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value_Dc, 1), 1 } },
                                                { CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value, 0) },
                                                { });

exit:
    NL_TEST_ASSERT(inSuite, testPass);
}

void TestTdm::TestTdmDictionary_DeleteAndMarkDirty(nlTestSuite *inSuite)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    bool testPass = false;

    Reset();

    mTestTdmSource.mDictlValues[0] = { 1, 1, 1 };
    mTestTdmSource.mDictlValues[1] = { 1, 1, 1 };
    mTestTdmSource.mDictlValues.erase(0);

    mTestTdmSource.DeleteKey(CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value, 0));
    mTestTdmSource.SetDirty(TestHTrait::kPropertyHandle_L);

    err = BuildAndProcessNotify();
    SuccessOrExit(err);

    testPass = mTestTdmSink.ValidateChangeSets( { { CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value_Da, 1), 1 },
                                                  { CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value_Db, 1), 1 },
                                                  { CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value_Dc, 1), 1 } },
                                                { },
                                                { TestHTrait::kPropertyHandle_L });

exit:
    NL_TEST_ASSERT(inSuite, testPass);
}

void TestTdm::TestTdmDictionary_MarkDirtyAndDelete(nlTestSuite *inSuite)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    bool testPass = false;

    Reset();

    mTestTdmSource.mDictlValues[0] = { 1, 1, 1 };
    mTestTdmSource.mDictlValues[1] = { 1, 1, 1 };

    mTestTdmSource.SetDirty(TestHTrait::kPropertyHandle_L);

    mTestTdmSource.mDictlValues.erase(0);
    mTestTdmSource.DeleteKey(CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value, 0));

    err = BuildAndProcessNotify();
    SuccessOrExit(err);

    testPass = mTestTdmSink.ValidateChangeSets( { { CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value_Da, 1), 1 },
                                                  { CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value_Db, 1), 1 },
                                                  { CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value_Dc, 1), 1 } },
                                                { },
                                                { TestHTrait::kPropertyHandle_L });

exit:
    NL_TEST_ASSERT(inSuite, testPass);
}

void TestTdm::TestTdmDictionary_DeleteAndMarkFarDirty(nlTestSuite *inSuite)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    bool testPass = false;

    Reset();

    mTestTdmSource.mDictSaValues[0] = { 1, 1, 1 };
    mTestTdmSource.mDictSaValues[1] = { 1, 1, 1 };

    mTestTdmSource.mDictSaValues.erase(0);
    mTestTdmSource.DeleteKey(CreatePropertyPathHandle(TestHTrait::kPropertyHandle_K_Sa_Value, 0));
    mTestTdmSource.SetDirty(TestHTrait::kPropertyHandle_K_Sb);

    err = BuildAndProcessNotify();
    SuccessOrExit(err);

    testPass = mTestTdmSink.ValidateChangeSets( { { TestHTrait::kPropertyHandle_K_Sb, 1 },
                                                  { CreatePropertyPathHandle(TestHTrait::kPropertyHandle_K_Sa_Value_Da, 1), 1 },
                                                  { CreatePropertyPathHandle(TestHTrait::kPropertyHandle_K_Sa_Value_Db, 1), 1 },
                                                  { CreatePropertyPathHandle(TestHTrait::kPropertyHandle_K_Sa_Value_Dc, 1), 1 } },
                                                { },
                                                { TestHTrait::kPropertyHandle_K_Sa });

exit:
    NL_TEST_ASSERT(inSuite, testPass);
}

void TestTdm::TestTdmDictionary_AddAndDeleteSimilar(nlTestSuite *inSuite)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    bool testPass = false;

    Reset();

    mTestTdmSource.mDictlValues[0] = { 1, 1, 1 };

    mTestTdmSource.mDictlValues[1] = { 1, 1, 1 };
    mTestTdmSource.SetDirty(CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value, 1));

    mTestTdmSource.mDictlValues.erase(1);
    mTestTdmSource.DeleteKey(CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value, 1));

    err = BuildAndProcessNotify();
    SuccessOrExit(err);

    testPass = mTestTdmSink.ValidateChangeSets( { },
                                                { CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value, 1) },
                                                { });

exit:
    NL_TEST_ASSERT(inSuite, testPass);
}


void TestTdm::TestTdmDictionary_ModifyAndDeleteSimilar(nlTestSuite *inSuite)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    bool testPass = false;

    Reset();

    mTestTdmSource.mDictlValues[0] = { 1, 1, 1 };
    mTestTdmSource.mDictlValues[1] = { 1, 1, 1 };

    mTestTdmSource.mDictlValues[1] = { 2, 2, 2 };
    mTestTdmSource.SetDirty(CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value, 1));

    mTestTdmSource.mDictlValues.erase(1);
    mTestTdmSource.DeleteKey(CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value, 1));

    err = BuildAndProcessNotify();
    SuccessOrExit(err);

    testPass = mTestTdmSink.ValidateChangeSets( { },
                                                { CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value, 1) },
                                                { });

exit:
    NL_TEST_ASSERT(inSuite, testPass);
}

void TestTdm::TestTdmDictionary_DeleteAndModifySimilar(nlTestSuite *inSuite)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    bool testPass = false;

    Reset();

    mTestTdmSource.mDictlValues[0] = { 1, 1, 1 };
    mTestTdmSource.mDictlValues[1] = { 1, 1, 1 };

    mTestTdmSource.mDictlValues.erase(1);
    mTestTdmSource.DeleteKey(CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value, 1));

    mTestTdmSource.mDictlValues[1] = { 2, 2, 2 };
    mTestTdmSource.SetDirty(CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value, 1));


    err = BuildAndProcessNotify();
    SuccessOrExit(err);

    testPass = mTestTdmSink.ValidateChangeSets( { { CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value_Da, 1), 2 },
                                                  { CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value_Db, 1), 2 },
                                                  { CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value_Dc, 1), 2 } },
                                                { },
                                                { });

exit:
    NL_TEST_ASSERT(inSuite, testPass);
}

void TestTdm::TestTdmDictionary_DeleteAndModifyLeafSimilar(nlTestSuite *inSuite)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    bool testPass = false;

    Reset();

    mTestTdmSource.mDictlValues[0] = { 1, 1, 1 };
    mTestTdmSource.mDictlValues[1] = { 1, 1, 1 };

    mTestTdmSource.mDictlValues.erase(1);
    mTestTdmSource.DeleteKey(CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value, 1));

    mTestTdmSource.mDictlValues[1] = { 2, 2, 2 };
    mTestTdmSource.SetDirty(CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value_Da, 1));

    err = BuildAndProcessNotify();
    SuccessOrExit(err);

    testPass = mTestTdmSink.ValidateChangeSets( { { CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value_Da, 1), 2 },
                                                  { CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value_Db, 1), 2 },
                                                  { CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value_Dc, 1), 2 } },
                                                { },
                                                { });

exit:
    NL_TEST_ASSERT(inSuite, testPass);
}

void TestTdm::TestTdmDictionary_DeleteStoreOverflowAndItemAddition(nlTestSuite *inSuite)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    bool testPass = false;

    Reset();

    mTestTdmSource1.mDictlValues[1] = { 1, 1, 1 };
    mTestTdmSource1.mDictlValues[2] = { 1, 1, 1 };
    mTestTdmSource1.mDictlValues[3] = { 1, 1, 1 };
    mTestTdmSource1.mDictlValues[4] = { 1, 1, 1 };
    mTestTdmSource1.mDictlValues[5] = { 1, 1, 1 };
    mTestTdmSource1.mDictlValues[6] = { 1, 1, 1 };
    mTestTdmSource1.mDictlValues[7] = { 1, 1, 1 };
    mTestTdmSource1.mDictlValues[8] = { 1, 1, 1 };
    mTestTdmSource1.mDictlValues[9] = { 1, 1, 1 };

    // we have to start by adding a handle from another trait to set up the interference.
    mTestTdmSource1.SetDirty(CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value, 1));
    mTestTdmSource1.SetDirty(CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value, 2));

    mTestTdmSource.mDictlValues[1] = { 1, 1, 1 };
    mTestTdmSource.mDictlValues[2] = { 1, 1, 1 };
    mTestTdmSource.mDictlValues[3] = { 1, 1, 1 };

    // then we add a couple of dictionary additions to the trait in question
    mTestTdmSource.SetDirty(CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value, 1));
    mTestTdmSource.SetDirty(CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value, 2));
    mTestTdmSource.SetDirty(CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value, 3));

    // then we fill it up past the store's capacity with the interference trait till it overflows,
    // resulting in the eviction of all of those entries associated with the interference trait.
    mTestTdmSource1.SetDirty(CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value, 3));
    mTestTdmSource1.SetDirty(CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value, 4));
    mTestTdmSource1.SetDirty(CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value, 5));
    mTestTdmSource1.SetDirty(CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value, 6));
    mTestTdmSource1.SetDirty(CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value, 7));
    mTestTdmSource1.SetDirty(CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value, 8));

    // Now finally, we put the delete in for the last item we added to the trait under test.
    mTestTdmSource.DeleteKey(CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value, 3));

    err = BuildAndProcessNotify();
    SuccessOrExit(err);

    mTestTdmSink1.DumpChangeSets();
    printf("\n");
    mTestTdmSink.DumpChangeSets();

    testPass = mTestTdmSink.ValidateChangeSets( { { CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value_Da, 1), 1 },
                                                  { CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value_Db, 1), 1 },
                                                  { CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value_Dc, 1), 1 },
                                                  { CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value_Da, 2), 1 },
                                                  { CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value_Db, 2), 1 },
                                                  { CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value_Dc, 2), 1 } },
                                                { CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value, 3) },
                                                { });

exit:
    NL_TEST_ASSERT(inSuite, testPass);
}

void TestTdm::TestTdmDictionary_DirtyStoreOverflowAndItemDeletion(nlTestSuite *inSuite)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    bool testPass = false;

    Reset();

    mTestTdmSource1.mDictlValues[1] = { 1, 1, 1 };
    mTestTdmSource1.mDictlValues[2] = { 1, 1, 1 };
    mTestTdmSource1.mDictlValues[3] = { 1, 1, 1 };
    mTestTdmSource1.mDictlValues[4] = { 1, 1, 1 };
    mTestTdmSource1.mDictlValues[5] = { 1, 1, 1 };
    mTestTdmSource1.mDictlValues[6] = { 1, 1, 1 };
    mTestTdmSource1.mDictlValues[7] = { 1, 1, 1 };
    mTestTdmSource1.mDictlValues[8] = { 1, 1, 1 };
    mTestTdmSource1.mDictlValues[9] = { 1, 1, 1 };

    mTestTdmSource1.mDictlValues.erase(1);
    mTestTdmSource1.mDictlValues.erase(2);

    // we have to start by adding a handle from another trait to set up the interference.
    mTestTdmSource1.DeleteKey(CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value, 1));
    mTestTdmSource1.DeleteKey(CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value, 2));

    mTestTdmSource.mDictlValues[1] = { 1, 1, 1 };
    mTestTdmSource.mDictlValues[2] = { 1, 1, 1 };
    mTestTdmSource.mDictlValues[3] = { 1, 1, 1 };
    mTestTdmSource.mDictlValues.erase(1);
    mTestTdmSource.mDictlValues.erase(2);
    mTestTdmSource.mDictlValues.erase(3);

    // then we add a couple of dictionary deletions to the trait in question
    mTestTdmSource.DeleteKey(CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value, 1));
    mTestTdmSource.DeleteKey(CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value, 2));
    mTestTdmSource.DeleteKey(CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value, 3));

    // then we fill it up past the store's capacity with the interference trait till it overflows,
    // resulting in the eviction of all of those entries associated with the interference trait.
    mTestTdmSource1.mDictlValues.erase(3);
    mTestTdmSource1.mDictlValues.erase(4);
    mTestTdmSource1.mDictlValues.erase(5);
    mTestTdmSource1.mDictlValues.erase(6);
    mTestTdmSource1.mDictlValues.erase(7);
    mTestTdmSource1.mDictlValues.erase(8);

    mTestTdmSource1.DeleteKey(CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value, 3));
    mTestTdmSource1.DeleteKey(CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value, 4));
    mTestTdmSource1.DeleteKey(CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value, 5));
    mTestTdmSource1.DeleteKey(CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value, 6));
    mTestTdmSource1.DeleteKey(CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value, 7));
    mTestTdmSource1.DeleteKey(CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value, 8));

    // Now finally, we put the addition in for the last item we added to.
    mTestTdmSource.mDictlValues[3] = { 1, 1, 1 };
    mTestTdmSource.SetDirty(CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value, 3));

    err = BuildAndProcessNotify();
    SuccessOrExit(err);

    mTestTdmSink1.DumpChangeSets();
    printf("\n");
    mTestTdmSink.DumpChangeSets();

    testPass = mTestTdmSink.ValidateChangeSets( { { CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value_Da, 3), 1 },
                                                  { CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value_Db, 3), 1 },
                                                  { CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value_Dc, 3), 1 } },
                                                { CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value, 1), CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value, 2) },
                                                { });

exit:
    NL_TEST_ASSERT(inSuite, testPass);
}

void TestTdm::TestTdmDictionary_DeleteEntryTwice(nlTestSuite *inSuite)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    bool testPass = false;

    Reset();

    mTestTdmSource.mDictlValues[0] = { 1, 1, 1 };
    mTestTdmSource.mDictlValues[1] = { 1, 1, 1 };
    mTestTdmSource.mDictlValues[2] = { 1, 1, 1 };
    mTestTdmSource.mDictlValues.erase(2);

    mTestTdmSource.DeleteKey(CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value, 2));
    mTestTdmSource.DeleteKey(CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value, 2));

    err = BuildAndProcessNotify();
    SuccessOrExit(err);

    mTestTdmSink.DumpChangeSets();

    testPass = mTestTdmSink.ValidateChangeSets( { },
                                                { CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value, 2) },
                                                { });


exit:
    NL_TEST_ASSERT(inSuite, testPass);
}

void TestTdm::TestRandomizedDataVersions(nlTestSuite *inSuite)
{
    TestEmptyDataSource dataSource1(&gEmptyTraitSchema);
    TestEmptyDataSource dataSource2(&gEmptyTraitSchema);
    TestEmptyDataSource dataSource3(&gEmptyTraitSchema);
    uint64_t version;

    // Case 1 - retrieve version right after construction - it should not be 0.
    version = dataSource1.GetVersion();
    NL_TEST_ASSERT(inSuite, version != 0);

    // Case 2 - increment the version first, then retrieve it to ensure it is not 1
    dataSource2.IncrementVersion();
    version = dataSource2.GetVersion();
    NL_TEST_ASSERT(inSuite, version != 1);

    // Case 3 - set the version to something other than 0 post construction, then check it.
    dataSource3.SetVersion(10);
    version = dataSource3.GetVersion();
    NL_TEST_ASSERT(inSuite, version == 10);
}

WEAVE_ERROR TestTdm::AllocateBuffer(uint32_t desiredSize, uint32_t minSize)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint32_t maxPayloadSize = 0;
    PacketBuffer *buf = NULL;


    err = mSubHandler->mBinding->AllocateRightSizedBuffer(buf, desiredSize, minSize, maxPayloadSize);
    SuccessOrExit(err);

exit:

    if (buf)
    {
        PacketBuffer::Free(buf);
    }

    return err;
}

void TestTdm::CheckAllocateRightSizedBufferForNotifications(nlTestSuite *inSuite)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint32_t fakeMax = UINT16_MAX;

    err = AllocateBuffer(WDM_MAX_NOTIFICATION_SIZE, WDM_MIN_NOTIFICATION_SIZE);

    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    err = AllocateBuffer(fakeMax, fakeMax);

    NL_TEST_ASSERT(inSuite, err != WEAVE_NO_ERROR);
}

} // WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current)
}
}
}

TestTdm *gTestTdm;

/**
 *  Set up the test suite.
 */
static int TestSetup(void *inContext)
{
    static TestTdm testTdm;
    gTestTdm = &testTdm;

    return testTdm.Setup();
}

/**
 *  Tear down the test suite.
 */
static int TestTeardown(void *inContext)
{
    return gTestTdm->Teardown();
}

static void TestTdmStatic_SingleLeafHandle(nlTestSuite *inSuite, void *inContext)
{
    gTestTdm->TestTdmStatic_SingleLeafHandle(inSuite);
}

static void TestTdmStatic_SingleLevelMerge(nlTestSuite *inSuite, void *inContext)
{
    gTestTdm->TestTdmStatic_SingleLevelMerge(inSuite);
}

static void TestTdmStatic_SingleLevelMergeDeep(nlTestSuite *inSuite, void *inContext)
{
    gTestTdm->TestTdmStatic_SingleLevelMergeDeep(inSuite);
}

static void TestTdmStatic_DirtyStruct(nlTestSuite *inSuite, void *inContext)
{
    gTestTdm->TestTdmStatic_DirtyStruct(inSuite);
}

static void TestTdmStatic_DirtyLeafUnevenDepth(nlTestSuite *inSuite, void *inContext)
{
    gTestTdm->TestTdmStatic_DirtyLeafUnevenDepth(inSuite);
}

static void TestTdmStatic_MergeHandleSetOverflow(nlTestSuite *inSuite, void *inContext)
{
    gTestTdm->TestTdmStatic_MergeHandleSetOverflow(inSuite);
}

static void TestTdmStatic_MarkLeafHandleDirtyTwice(nlTestSuite *inSuite, void *inContext)
{
    gTestTdm->TestTdmStatic_MarkLeafHandleDirtyTwice(inSuite);
}

static void TestTdmStatic_TestNullableStruct(nlTestSuite *inSuite, void *inContext)
{
    gTestTdm->TestTdmStatic_TestNullableStruct(inSuite);
}

static void TestTdmStatic_TestNullableLeaf(nlTestSuite *inSuite, void *inContext)
{
    gTestTdm->TestTdmStatic_TestNullableLeaf(inSuite);
}

static void TestTdmStatic_TestNonNullableLeaf(nlTestSuite *inSuite, void *inContext)
{
    gTestTdm->TestTdmStatic_TestNonNullableLeaf(inSuite);
}

static void TestTdmStatic_TestEphemeralStruct(nlTestSuite *inSuite, void *inContext)
{
    gTestTdm->TestTdmStatic_TestEphemeralStruct(inSuite);
}

static void TestTdmStatic_TestIsParent(nlTestSuite *inSuite, void *inContext)
{
    gTestTdm->TestTdmStatic_TestIsParent(inSuite);
}

static void TestTdmStatic_TestEphemeralLeaf(nlTestSuite *inSuite, void *inContext)
{
    gTestTdm->TestTdmStatic_TestEphemeralLeaf(inSuite);
}

static void TestTdmMismatched_PathInDataElement(nlTestSuite *inSuite, void *inContext)
{
    gTestTdm->TestTdmMismatched_PathInDataElement(inSuite);
}

static void TestTdmMismatched_TopLevelPOD(nlTestSuite *inSuite, void *inContext)
{
    gTestTdm->TestTdmMismatched_TopLevelPOD(inSuite);
}

static void TestTdmMismatched_NestedStruct(nlTestSuite *inSuite, void *inContext)
{
    gTestTdm->TestTdmMismatched_NestedStruct(inSuite);
}

static void TestTdmMismatched_TopLevelStruct(nlTestSuite *inSuite, void *inContext)
{
    gTestTdm->TestTdmMismatched_TopLevelStruct(inSuite);
}

static void TestTdmMismatched_SetLeafDataMismatch(nlTestSuite *inSuite, void *inContext)
{
    gTestTdm->TestTdmMismatched_SetLeafDataMismatch(inSuite);
}
static void TestTdmDictionary_DictionaryEntryAddition(nlTestSuite *inSuite, void *inContext)
{
    gTestTdm->TestTdmDictionary_DictionaryEntryAddition(inSuite);
}

static void TestTdmDictionary_DictionaryEntriesAddition(nlTestSuite *inSuite, void *inContext)
{
    gTestTdm->TestTdmDictionary_DictionaryEntriesAddition(inSuite);
}

static void TestTdmDictionary_ReplaceDictionary(nlTestSuite *inSuite, void *inContext)
{
    gTestTdm->TestTdmDictionary_ReplaceDictionary(inSuite);
}

static void TestTdmDictionary_DeleteSingle(nlTestSuite *inSuite, void *inContext)
{
    gTestTdm->TestTdmDictionary_DeleteSingle(inSuite);
}

static void TestTdmDictionary_DeleteMultiple(nlTestSuite *inSuite, void *inContext)
{
    gTestTdm->TestTdmDictionary_DeleteMultiple(inSuite);
}

static void TestTdmDictionary_DeleteHandleSetOverflow(nlTestSuite *inSuite, void *inContext)
{
    gTestTdm->TestTdmDictionary_DeleteHandleSetOverflow(inSuite);
}

static void TestTdmDictionary_AddDeleteDifferent(nlTestSuite *inSuite, void *inContext)
{
    gTestTdm->TestTdmDictionary_AddDeleteDifferent(inSuite);
}

static void TestTdmDictionary_DeleteAndMarkDirty(nlTestSuite *inSuite, void *inContext)
{
    gTestTdm->TestTdmDictionary_DeleteAndMarkDirty(inSuite);
}

static void TestTdmDictionary_MarkDirtyAndDelete(nlTestSuite *inSuite, void *inContext)
{
    gTestTdm->TestTdmDictionary_MarkDirtyAndDelete(inSuite);
}

static void TestTdmDictionary_DeleteAndMarkFarDirty(nlTestSuite *inSuite, void *inContext)
{
    gTestTdm->TestTdmDictionary_DeleteAndMarkFarDirty(inSuite);
}

static void TestTdmDictionary_AddAndDeleteSimilar(nlTestSuite *inSuite, void *inContext)
{
    gTestTdm->TestTdmDictionary_AddAndDeleteSimilar(inSuite);
}

static void TestTdmDictionary_ModifyAndDeleteSimilar(nlTestSuite *inSuite, void *inContext)
{
    gTestTdm->TestTdmDictionary_ModifyAndDeleteSimilar(inSuite);
}

static void TestTdmDictionary_DeleteAndModifySimilar(nlTestSuite *inSuite, void *inContext)
{
    gTestTdm->TestTdmDictionary_DeleteAndModifySimilar(inSuite);
}

static void TestTdmDictionary_DeleteAndModifyLeafSimilar(nlTestSuite *inSuite, void *inContext)
{
    gTestTdm->TestTdmDictionary_DeleteAndModifyLeafSimilar(inSuite);
}

static void TestTdmDictionary_DeleteStoreOverflowAndItemAddition(nlTestSuite *inSuite, void *inContext)
{
    gTestTdm->TestTdmDictionary_DeleteStoreOverflowAndItemAddition(inSuite);
}

static void TestTdmDictionary_DirtyStoreOverflowAndItemDeletion(nlTestSuite *inSuite, void *inContext)
{
    gTestTdm->TestTdmDictionary_DirtyStoreOverflowAndItemDeletion(inSuite);
}

static void TestTdmDictionary_DeleteEntryTwice(nlTestSuite *inSuite, void *inContext)
{
    gTestTdm->TestTdmDictionary_DeleteEntryTwice(inSuite);
}

static void  TestRandomizedDataVersions(nlTestSuite *inSuite, void *inContext)
{
    gTestTdm->TestRandomizedDataVersions(inSuite);
}

static void TestTdmStatic_MultiInstance(nlTestSuite *inSuite, void *inContext)
{
    gTestTdm->TestTdmStatic_MultiInstance(inSuite);
}

static void CheckAllocateRightSizedBufferForNotifications(nlTestSuite *inSuite, void *inContext)
{
    gTestTdm->CheckAllocateRightSizedBufferForNotifications(inSuite);
}

/**
 *  Main
 */
int main(int argc, char *argv[])
{
#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    tcpip_init(NULL, NULL);
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

    MockPlatform::gMockPlatformClocks.GetClock_RealTime = Private::GetClock_RealTime;
    MockPlatform::gMockPlatformClocks.SetClock_RealTime = Private::SetClock_RealTime;

    nlTestSuite theSuite = {
        "weave-tdm",
        &sTests[0],
        TestSetup,
        TestTeardown
    };

    // Generate machine-readable, comma-separated value (CSV) output.
    nl_test_set_output_style(OUTPUT_CSV);

    // Run test suit against one context
    nlTestRunner(&theSuite, NULL);

    return nlTestRunnerStats(&theSuite);
}
