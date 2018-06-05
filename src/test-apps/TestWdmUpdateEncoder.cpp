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
        void TestInitCleanup(nlTestSuite *inSuite, void *inContext);
        void TestOneLeaf(nlTestSuite *inSuite, void *inContext);

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
        void InitEncoderContext(nlTestSuite *inSuite);
        void VerifyDataList(nlTestSuite *inSuite, PacketBuffer *aBuf);

};

WdmUpdateEncoderTest::WdmUpdateEncoderTest() :
    mBuf(NULL),
    mSinkCatalog(ResourceIdentifier(ResourceIdentifier::SELF_NODE_ID), mSinkCatalogStore, sizeof(mSinkCatalogStore) / sizeof(mSinkCatalogStore[0]))
{
    mPathList.Init(mStorage, ArraySize(mStorage));

    mSinkCatalog.Add(0, &mTestATraitUpdatableDataSink0, mTraitHandleSet[kTestATraitSink0Index]);
}

void WdmUpdateEncoderTest::InitEncoderContext(nlTestSuite *inSuite)
{
    if (NULL == mBuf)
    {
        mBuf = PacketBuffer::New();
        NL_TEST_ASSERT(inSuite, mBuf != NULL);
    }

    mContext.mBuf = mBuf;
    mContext.mMaxPayloadSize = 1000;
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

    mPathList.Clear();

    NL_TEST_ASSERT(inSuite, mPathList.GetNumItems() == 0);
}

void WdmUpdateEncoderTest::VerifyDataList(nlTestSuite *inSuite, PacketBuffer *aBuf)
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

    for (size_t i = mPathList.GetFirstValidItem();
            i < mPathList.GetPathStoreSize();
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

        NL_TEST_ASSERT(inSuite, pathHandle == tp.mPropertyPathHandle);
    }
}

void WdmUpdateEncoderTest::TestOneLeaf(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    PRINT_TEST_NAME();

    mPathList.Clear();

    NL_TEST_ASSERT(inSuite, mPathList.GetNumItems() == 0);

    mTP = {
        mTraitHandleSet[kTestATraitSink0Index],
        CreatePropertyPathHandle(TestATrait::kPropertyHandle_TaC)
    };

    err = mPathList.AddItem(mTP);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    InitEncoderContext(inSuite);

    err = mEncoder.EncodeRequest(&mContext);

    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    NL_TEST_ASSERT(inSuite, mPathList.GetPathStoreSize() == mContext.mItemInProgress);
    NL_TEST_ASSERT(inSuite, 1 == mPathList.GetNumItems());
    NL_TEST_ASSERT(inSuite, kNullPropertyPathHandle == mContext.mNextDictionaryElementPathHandle);

    VerifyDataList(inSuite, mBuf);
}


} // WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current)
}
}
}

static SubscriptionEngine *gSubscriptionEngine;

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
// Test Suite

/**
 *  Test Suite that lists all the test functions.
 */
static const nlTest sTests[] = {
    NL_TEST_DEF("Init and cleanup",  WdmUpdateEncoderTest_InitCleanup),
    NL_TEST_DEF("Encode one leaf",  WdmUpdateEncoderTest_OneLeaf),

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
static int TestSetup(void *inContext)
{
    gSubscriptionEngine = NULL;

    return 0;
}

/**
 *  Tear down the test suite.
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
        TestSetup,
        TestTeardown
    };

    // Generate machine-readable, comma-separated value (CSV) output.
    nl_test_set_output_style(OUTPUT_CSV);

    // Run test suit against one context
    nlTestRunner(&theSuite, NULL);

    return nlTestRunnerStats(&theSuite);
}
