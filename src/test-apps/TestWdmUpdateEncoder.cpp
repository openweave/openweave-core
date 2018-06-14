/*
 *
 *    Copyright (c) 2018 Nest Labs, Inc.
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
 *      This file implements unit tests for the encoding of WDM UpdateRequest payloads.
 *
 */

#include "ToolCommon.h"

#include <nlbyteorder.h>
#include <nltest.h>

#include <Weave/Core/WeaveCore.h>

#include <Weave/Profiles/data-management/Current/WdmManagedNamespace.h>
#include <Weave/Profiles/data-management/DataManagement.h>

#include <nest/test/trait/TestATrait.h>
#include "MockSinkTraits.h"

#include <new>
#include <map>
#include <set>
#include <algorithm>
#include <set>
#include <string>
#include <iterator>

#define PRINT_TEST_NAME() printf("\n%s\n", __func__);

using namespace nl;
using namespace nl::Weave::TLV;
using namespace nl::Weave::Profiles::DataManagement;
using namespace Schema::Nest::Test::Trait;


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// System/Platform definitions
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current) {
namespace Platform {
    // For unit tests, a dummy critical section is sufficient.
    void CriticalSectionEnter()
    {
        return;
    }

    void CriticalSectionExit()
    {
        return;
    }
} // Platform

class WdmUpdateEncoderTest {
    public:
        WdmUpdateEncoderTest();
        ~WdmUpdateEncoderTest() { }

        // Tests
        void SetupTest();

        void TestInitCleanup(nlTestSuite *inSuite, void *inContext);
        void TestOneLeaf(nlTestSuite *inSuite, void *inContext);
        void TestRoot(nlTestSuite *inSuite, void *inContext);
        void TestWholeDictionary(nlTestSuite *inSuite, void *inContext);
        void TestTwoProperties(nlTestSuite *inSuite, void *inContext);
        void TestDictionaryElements(nlTestSuite *inSuite, void *inContext);
        void TestStructure(nlTestSuite *inSuite, void *inContext);
        void TestOverflowDictionary(nlTestSuite *inSuite, void *inContext);

    private:
        // The encoder
        UpdateEncoder mEncoder;
        UpdateEncoder::Context mContext;

        // These are here for convenience
        PacketBuffer *mBuf;
        TraitPath mTP;

        //
        // The state usually held by the SubscriptionClient
        //

        // The list of path to encode
        TraitPathStore mPathList;
        TraitPathStore::Record mStorage[10];

        // The Trait instances
        TestATraitUpdatableDataSink mTestATraitUpdatableDataSink0;

        // The catalog
        SingleResourceSinkTraitCatalog mSinkCatalog;
        SingleResourceSinkTraitCatalog::CatalogItem mSinkCatalogStore[9];

        // The set of TraitDataHandles assigned by the catalog
        // to the Trait instances
        enum
        {
            kTestATraitSink0Index = 0,
            kTestATraitSink1Index,
            kTestBTraitSinkIndex,
            kLocaleSettingsSinkIndex,
            kBoltLockSettingTraitSinkIndex,
            kApplicationKeysTraitSinkIndex,

            kLocaleCapabilitiesSourceIndex,
            kTestATraitSource0Index,
            kTestATraitSource1Index,
            kTestBTraitSourceIndex,
            kTestBLargeTraitSourceIndex,
            kMaxNumTraitHandles,
        };
        TraitDataHandle mTraitHandleSet[kMaxNumTraitHandles];

        // Test support functions
        void BasicTestBody(nlTestSuite *inSuite);
        void InitEncoderContext(nlTestSuite *inSuite);
        void VerifyDataList(nlTestSuite *inSuite, PacketBuffer *aBuf, size_t aItemToStartFrom = 0);

};

WdmUpdateEncoderTest::WdmUpdateEncoderTest() :
    mBuf(NULL),
    mSinkCatalog(ResourceIdentifier(ResourceIdentifier::SELF_NODE_ID),
            mSinkCatalogStore, sizeof(mSinkCatalogStore) / sizeof(mSinkCatalogStore[0]))
{
    mPathList.Init(mStorage, ArraySize(mStorage));

    mSinkCatalog.Add(0, &mTestATraitUpdatableDataSink0, mTraitHandleSet[kTestATraitSink0Index]);

    mTestATraitUpdatableDataSink0.SetUpdateEncoder(&mEncoder);

    mTestATraitUpdatableDataSink0.tai_map.clear();

    for (int32_t i = 0; i < 10; i++)
    {
        mTestATraitUpdatableDataSink0.tai_map[i] = i+100;
    }
}


void WdmUpdateEncoderTest::SetupTest()
{
    mPathList.Clear();
}


void WdmUpdateEncoderTest::InitEncoderContext(nlTestSuite *inSuite)
{
    if (NULL == mBuf)
    {
        mBuf = PacketBuffer::New(0);
        NL_TEST_ASSERT(inSuite, mBuf != NULL);
    }

    mBuf->SetDataLength(0);

    mContext.mBuf = mBuf;
    mContext.mMaxPayloadSize = mBuf->AvailableDataLength();
    mContext.mUpdateRequestIndex = 7;
    mContext.mExpiryTimeMicroSecond = 0;
    mContext.mItemInProgress = 0;
    mContext.mNextDictionaryElementPathHandle = kNullPropertyPathHandle;
    mContext.mInProgressUpdateList = &mPathList;
    mContext.mDataSinkCatalog = &mSinkCatalog;
}

void WdmUpdateEncoderTest::TestInitCleanup(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    PRINT_TEST_NAME();

    NL_TEST_ASSERT(inSuite, 0 == mPathList.GetNumItems());
}


void WdmUpdateEncoderTest::VerifyDataList(nlTestSuite *inSuite, PacketBuffer *aBuf, size_t aItemToStartFrom)
{
    WEAVE_ERROR err;
    nl::Weave::TLV::TLVReader reader;
    nl::Weave::TLV::TLVReader dataListReader;
    UpdateRequest::Parser parser;
    TraitPath tp;

    reader.Init(aBuf);
    reader.Next();
    parser.Init(reader);

    err = parser.CheckSchemaValidity();
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    DataList::Parser dataList;

    err = parser.GetDataList(&dataList);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    uint32_t updateRequestIndex = 0;
    err = parser.GetUpdateRequestIndex(&updateRequestIndex);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    NL_TEST_ASSERT(inSuite, updateRequestIndex == mContext.mUpdateRequestIndex);

    dataList.GetReader(&dataListReader);

    size_t firstItemNotEncoded = mContext.mItemInProgress;

    for (size_t i = aItemToStartFrom;
            i < firstItemNotEncoded;
            i = mPathList.GetNextValidItem(i))
    {
        mPathList.GetItemAt(i, tp);

        err = dataListReader.Next();
        NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

        DataElement::Parser element;
        err = element.Init(dataListReader);
        NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

        nl::Weave::TLV::TLVReader pathReader;

        err = element.GetReaderOnPath(&pathReader);
        NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

        TraitDataSink * dataSink;
        TraitDataHandle handle;
        PropertyPathHandle pathHandle;
        SchemaVersionRange versionRange;

        err = mSinkCatalog.AddressToHandle(pathReader, handle, versionRange);
        NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

        NL_TEST_ASSERT(inSuite, handle == tp.mTraitDataHandle);

        err = mSinkCatalog.Locate(handle, &dataSink);
        NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

        err = dataSink->GetSchemaEngine()->MapPathToHandle(pathReader, pathHandle);
        NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

        if (dataSink->GetSchemaEngine()->IsDictionary(tp.mPropertyPathHandle) &&
                false == mPathList.AreFlagsSet(i, SubscriptionClient::kFlag_ForceMerge))
        {
            // This dictionary should be encoded so that it gets completely replaced:
            // that is, the path points to its parent.
            tp.mPropertyPathHandle = dataSink->GetSchemaEngine()->GetParent(tp.mPropertyPathHandle);
        }

        NL_TEST_ASSERT(inSuite, pathHandle == tp.mPropertyPathHandle);
    }

    err = dataListReader.Next();
    NL_TEST_ASSERT(inSuite, err == WEAVE_END_OF_TLV);
}


void WdmUpdateEncoderTest::BasicTestBody(nlTestSuite *inSuite)
{
    WEAVE_ERROR err;

    InitEncoderContext(inSuite);

    err = mEncoder.EncodeRequest(&mContext);

    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    NL_TEST_ASSERT(inSuite, mPathList.GetPathStoreSize() == mContext.mItemInProgress);
    NL_TEST_ASSERT(inSuite, kNullPropertyPathHandle == mContext.mNextDictionaryElementPathHandle);

    VerifyDataList(inSuite, mBuf);
}


void WdmUpdateEncoderTest::TestOneLeaf(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    PRINT_TEST_NAME();

    mTP = {
        mTraitHandleSet[kTestATraitSink0Index],
        CreatePropertyPathHandle(TestATrait::kPropertyHandle_TaC)
    };

    err = mPathList.AddItem(mTP);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    BasicTestBody(inSuite);

    NL_TEST_ASSERT(inSuite, 1 == mPathList.GetNumItems());

}


void WdmUpdateEncoderTest::TestRoot(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    PRINT_TEST_NAME();

    mTP = {
        mTraitHandleSet[kTestATraitSink0Index],
        kRootPropertyPathHandle
    };

    err = mPathList.AddItem(mTP);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    BasicTestBody(inSuite);

    // TestAStruct has 2 dictionaries; one is empty; the non-empty one triggers
    // the addition of a private TraitPath.
    NL_TEST_ASSERT(inSuite, 2 == mPathList.GetNumItems());
}


void WdmUpdateEncoderTest::TestWholeDictionary(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    PRINT_TEST_NAME();

    mTP = {
        mTraitHandleSet[kTestATraitSink0Index],
        CreatePropertyPathHandle(TestATrait::kPropertyHandle_TaI)
    };

    err = mPathList.AddItem(mTP);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    BasicTestBody(inSuite);

    NL_TEST_ASSERT(inSuite, 1 == mPathList.GetNumItems());
}


void WdmUpdateEncoderTest::TestTwoProperties(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    PRINT_TEST_NAME();

    mTP = {
        mTraitHandleSet[kTestATraitSink0Index],
        CreatePropertyPathHandle(TestATrait::kPropertyHandle_TaA)
    };

    err = mPathList.AddItem(mTP);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    mTP.mPropertyPathHandle = CreatePropertyPathHandle(TestATrait::kPropertyHandle_TaB);
    err = mPathList.AddItem(mTP);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    BasicTestBody(inSuite);

    NL_TEST_ASSERT(inSuite, 2 == mPathList.GetNumItems());
}


void WdmUpdateEncoderTest::TestDictionaryElements(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    PRINT_TEST_NAME();

    for (uint32_t i = 0; i < 10; i++)
    {
        mTP = {
            mTraitHandleSet[kTestATraitSink0Index],
            CreatePropertyPathHandle(TestATrait::kPropertyHandle_TaI_Value, i)
        };

        err = mPathList.AddItem(mTP);
        NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    }

    BasicTestBody(inSuite);

    NL_TEST_ASSERT(inSuite, 10 == mPathList.GetNumItems());
}


void WdmUpdateEncoderTest::TestStructure(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    PRINT_TEST_NAME();

    mTP = {
        mTraitHandleSet[kTestATraitSink0Index],
        CreatePropertyPathHandle(TestATrait::kPropertyHandle_TaD)
    };

    err = mPathList.AddItem(mTP);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    BasicTestBody(inSuite);

    NL_TEST_ASSERT(inSuite, 1 == mPathList.GetNumItems());
}


void WdmUpdateEncoderTest::TestOverflowDictionary(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    PRINT_TEST_NAME();

    PacketBuffer::Free(mBuf);
    mBuf = NULL;

    mBuf = PacketBuffer::New(0);

    SetupTest();

    uint16_t totLen = mBuf->TotalLength();
    uint16_t available = mBuf->AvailableDataLength();
    printf("totLen empty: %" PRIu16 " bytes; available %" PRIu16 "\n", totLen, mBuf->AvailableDataLength());

    // encode the first item by itself

    mTP = {
        mTraitHandleSet[kTestATraitSink0Index],
        CreatePropertyPathHandle(TestATrait::kPropertyHandle_TaD)
    };

    err = mPathList.AddItem(mTP);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    BasicTestBody(inSuite);

    uint16_t encodedOneItemLen = mBuf->TotalLength();

    PacketBuffer::Free(mBuf);
    mBuf = NULL;

    // now encode the first item plus the dictionary

    SetupTest();

    mBuf = PacketBuffer::New(0);

    mTP = {
        mTraitHandleSet[kTestATraitSink0Index],
        CreatePropertyPathHandle(TestATrait::kPropertyHandle_TaD)
    };

    err = mPathList.AddItem(mTP);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    mTP.mPropertyPathHandle = CreatePropertyPathHandle(TestATrait::kPropertyHandle_TaI);

    err = mPathList.AddItem(mTP);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    BasicTestBody(inSuite);

    NL_TEST_ASSERT(inSuite, 2 == mPathList.GetNumItems());

    uint16_t encodedTwoItems = mBuf->TotalLength();
    printf("encoded with two items: %" PRIu16 " bytes; totLen: %" PRIu16 " available %" PRIu16 "\n", encodedTwoItems, mBuf->TotalLength(), mBuf->AvailableDataLength());

    uint16_t maxLen = mBuf->MaxDataLength();

    PacketBuffer::Free(mBuf);
    mBuf = NULL;

    // Repeat the test with all the payload lengths that fit the first DataElement but not the
    // full second one.

    for (uint16_t reserved = (available - encodedTwoItems +1); reserved <= (available - encodedOneItemLen); reserved++)
    {
        SetupTest();

        mTP = {
            mTraitHandleSet[kTestATraitSink0Index],
            CreatePropertyPathHandle(TestATrait::kPropertyHandle_TaD)
        };

        err = mPathList.AddItem(mTP);
        NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

        mTP.mPropertyPathHandle = CreatePropertyPathHandle(TestATrait::kPropertyHandle_TaI);

        err = mPathList.AddItem(mTP);
        NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

        if (mBuf != NULL)
        {
            PacketBuffer::Free(mBuf);
            mBuf = NULL;
        }

        mBuf = PacketBuffer::New(reserved);
        NL_TEST_ASSERT(inSuite, NULL != mBuf);

        InitEncoderContext(inSuite);
        printf("reserved %" PRIu16 " bytes; available %" PRIu16 "\n", reserved, mBuf->AvailableDataLength());

        err = mEncoder.EncodeRequest(&mContext);

        NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

        if (err != WEAVE_NO_ERROR)
        {
            continue;
        }

        VerifyDataList(inSuite, mBuf);

        if (kNullPropertyPathHandle == mContext.mNextDictionaryElementPathHandle)
        {
            NL_TEST_ASSERT(inSuite, 2 == mPathList.GetNumItems());
            // The dictionary was not encoded at all, and mItemInProgress points to the
            // dictionary (second item in the list).
            NL_TEST_ASSERT(inSuite, 1 == mContext.mItemInProgress);
        }
        else
        {
            // Dictionary overflowed
            // If the item that bounced is the very first one, the whole dictionary
            // should have bounced (it's a waste to send an empty dictionary here).
            if (GetPropertyDictionaryKey(mContext.mNextDictionaryElementPathHandle) == 0)
            {
                NL_TEST_ASSERT(inSuite, 2 == mPathList.GetNumItems());
            }
            else
            {
                NL_TEST_ASSERT(inSuite, 3 == mPathList.GetNumItems());
            }
            NL_TEST_ASSERT(inSuite, mContext.mItemInProgress == (mPathList.GetNumItems() -1));
        }

        // next payload

        // First re-assert that there is indeed more to encode.
        NL_TEST_ASSERT(inSuite, mContext.mItemInProgress < (mPathList.GetNumItems()));

        PacketBuffer::Free(mBuf);
        mBuf = NULL;

        mBuf = PacketBuffer::New(0);
        NL_TEST_ASSERT(inSuite, NULL != mBuf);

        mContext.mBuf = mBuf;
        mContext.mMaxPayloadSize = mBuf->AvailableDataLength();

        size_t itemToStartFrom = mContext.mItemInProgress;
        printf("second payload, starting from item %zu\n", itemToStartFrom);

        err = mEncoder.EncodeRequest(&mContext);

        NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

        if (err != WEAVE_NO_ERROR)
        {
            continue;
        }

        VerifyDataList(inSuite, mBuf, itemToStartFrom);
        NL_TEST_ASSERT(inSuite, kNullPropertyPathHandle == mContext.mNextDictionaryElementPathHandle);
        NL_TEST_ASSERT(inSuite, mPathList.GetPathStoreSize() == mContext.mItemInProgress);
    }
}

} // WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current)
}
}
} static SubscriptionEngine *gSubscriptionEngine;

SubscriptionEngine * SubscriptionEngine::GetInstance()
{
    return gSubscriptionEngine;
}

WdmUpdateEncoderTest gWdmUpdateEncoderTest;



void WdmUpdateEncoderTest_InitCleanup(nlTestSuite *inSuite, void *inContext)
{
    gWdmUpdateEncoderTest.TestInitCleanup(inSuite, inContext);
}

void WdmUpdateEncoderTest_OneLeaf(nlTestSuite *inSuite, void *inContext)
{
    gWdmUpdateEncoderTest.TestOneLeaf(inSuite, inContext);
}

void WdmUpdateEncoderTest_Root(nlTestSuite *inSuite, void *inContext)
{
    gWdmUpdateEncoderTest.TestRoot(inSuite, inContext);
}

void WdmUpdateEncoderTest_WholeDictionary(nlTestSuite *inSuite, void *inContext)
{
    gWdmUpdateEncoderTest.TestWholeDictionary(inSuite, inContext);
}

void WdmUpdateEncoderTest_TwoProperties(nlTestSuite *inSuite, void *inContext)
{
    gWdmUpdateEncoderTest.TestTwoProperties(inSuite, inContext);
}

void WdmUpdateEncoderTest_DictionaryElements(nlTestSuite *inSuite, void *inContext)
{
    gWdmUpdateEncoderTest.TestDictionaryElements(inSuite, inContext);
}

void WdmUpdateEncoderTest_Structure(nlTestSuite *inSuite, void *inContext)
{
    gWdmUpdateEncoderTest.TestStructure(inSuite, inContext);
}

void WdmUpdateEncoderTest_OverflowDictionary(nlTestSuite *inSuite, void *inContext)
{
    gWdmUpdateEncoderTest.TestOverflowDictionary(inSuite, inContext);
}
// Test Suite

/**
 *  Test Suite that lists all the test functions.
 */
static const nlTest sTests[] = {
    NL_TEST_DEF("Init and cleanup",  WdmUpdateEncoderTest_InitCleanup),
    NL_TEST_DEF("Encode one leaf",  WdmUpdateEncoderTest_OneLeaf),
    NL_TEST_DEF("Encode root",  WdmUpdateEncoderTest_Root),
    NL_TEST_DEF("Encode whole dictionary",  WdmUpdateEncoderTest_WholeDictionary),
    NL_TEST_DEF("Encode two properties",  WdmUpdateEncoderTest_TwoProperties),
    NL_TEST_DEF("Encode dictionary elements",  WdmUpdateEncoderTest_DictionaryElements),
    NL_TEST_DEF("Encode structure",  WdmUpdateEncoderTest_Structure),
    NL_TEST_DEF("Encode overflowing dictionary",  WdmUpdateEncoderTest_OverflowDictionary),

    NL_TEST_SENTINEL()
};

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current) {



} // WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current)
} // Profiles
} // Weave
} // nl


/**
 *  Set up the test suite.
 */
static int SuiteSetup(void *inContext)
{
    gSubscriptionEngine = NULL;

    return 0;
}

/**
 *  Tear down the test suite.
 */
static int SuiteTeardown(void *inContext)
{
    return 0;
}

/**
 *  Set up each test.
 */
static int TestSetup(void *inContext)
{
    gWdmUpdateEncoderTest.SetupTest();

    return 0;
}

/**
 *  Tear down each test.
 */
static int TestTeardown(void *inContext)
{
    return 0;
}

/**
 *  Main
 */
int main(int argc, char *argv[])
{
    nlTestSuite theSuite = {
        "weave-WdmUpdateEncoder",
        &sTests[0],
        SuiteSetup,
        SuiteTeardown,
        TestSetup,
        TestTeardown
    };

    // Generate machine-readable, comma-separated value (CSV) output.
    nl_test_set_output_style(OUTPUT_CSV);

    // Run test suit against one context
    nlTestRunner(&theSuite, NULL);

    return nlTestRunnerStats(&theSuite);
}
