/*
 *
 *    Copyright (c) 2020 Google LLC.
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
 *      This file implements unit tests for the Wdm Update Server.
 *
 */

#include "ToolCommon.h"

#include <nlbyteorder.h>
#include <nlunit-test.h>

#include <Weave/Core/WeaveCore.h>

#include <Weave/Profiles/data-management/Current/WdmManagedNamespace.h>
#include <Weave/Profiles/data-management/DataManagement.h>

#include <nest/test/trait/TestATrait.h>
#include "MockSinkTraits.h"
#include "MockSourceTraits.h"
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

SubscriptionEngine * SubscriptionEngine::GetInstance()
{
    static nl::Weave::Profiles::DataManagement::SubscriptionEngine gWdmSubscriptionEngine;
    return &gWdmSubscriptionEngine;
}

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

} // namespace Platform

} // namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current)
} // namespace Profiles
} // namespace Weave
} // namespace nl

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING && WEAVE_CONFIG_ENABLE_WDM_UPDATE
namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current) {

class WdmUpdateServerTest
{
public:
    WdmUpdateServerTest();
    ~WdmUpdateServerTest() { }

    // Tests
    void SetupTest();
    void TearDownTest();

    void TestInitCleanup(nlTestSuite * inSuite, void * inContext);
    void TestUpdateServerConditionalOneLeaf(nlTestSuite * inSuite, void * inContext);
    void TestUpdateServerUnconditionalOneLeaf(nlTestSuite * inSuite, void * inContext);
    void TestUpdateServerMixedConditionalOneLeaf(nlTestSuite * inSuite, void * inContext);
    void TestUpdateServerConditionalTwoProperties(nlTestSuite * inSuite, void * inContext);
    void TestUpdateServerUnconditionalTwoProperties(nlTestSuite * inSuite, void * inContext);
    void TestUpdateServerMixedConditionalTwoProperties(nlTestSuite * inSuite, void * inContext);

private:
    // The encoder
    UpdateEncoder mEncoder;
    UpdateEncoder::Context mContext;

    // These are here for convenience
    PacketBuffer * mBuf;
    TraitPath mTP;

    //
    // The state usually held by the SubscriptionClient
    //

    // The list of path to encode
    TraitPathStore mPathList;
    TraitPathStore::Record mStorage[10];

    // The Trait instances
    TestATraitUpdatableDataSink mTestATraitUpdatableDataSink0;
    TestATraitDataSource mTestATraitDataSource0;
    TestBTraitUpdatableDataSink mTestBTraitUpdatableDataSink;
    TestBTraitDataSource mTestBTraitDataSource;
    // The catalog
    SingleResourceSinkTraitCatalog mSinkCatalog;
    SingleResourceSinkTraitCatalog::CatalogItem mSinkCatalogStore[9];
    SingleResourceSourceTraitCatalog mSourceCatalog;
    SingleResourceSourceTraitCatalog::CatalogItem mSourceCatalogStore[9];
    // The set of TraitDataHandles assigned by the catalog
    // to the Trait instances
    enum
    {
        kTestATraitSink0Index = 0,
        kTestATraitSink1Index,
        kTestBTraitSinkIndex,

        kTestATraitSource0Index,
        kTestATraitSource1Index,
        kTestBTraitSourceIndex,
        kMaxNumTraitHandles,
    };
    TraitDataHandle mTraitHandleSet[kMaxNumTraitHandles];

    // Test support functions
    void UpdateServerBasicTestBody(nlTestSuite * inSuite);
    void InitEncoderContext(nlTestSuite * inSuite);
    static void TLVPrettyPrinter(const char * aFormat, ...);
    WEAVE_ERROR DebugPrettyPrint(PacketBuffer * apMsgBuf);

    WEAVE_ERROR VerifyUpdateRequest(nlTestSuite * inSuite, PacketBuffer * aPayload, size_t aItemToStartFrom = 0);
};

WdmUpdateServerTest::WdmUpdateServerTest() :
    mBuf(NULL), mSinkCatalog(ResourceIdentifier(ResourceIdentifier::SELF_NODE_ID), mSinkCatalogStore,
                             sizeof(mSinkCatalogStore) / sizeof(mSinkCatalogStore[0])),
    mSourceCatalog(ResourceIdentifier(ResourceIdentifier::SELF_NODE_ID), mSourceCatalogStore,
                   sizeof(mSourceCatalogStore) / sizeof(mSourceCatalogStore[0]))
{
    mPathList.Init(mStorage, ArraySize(mStorage));

    mSinkCatalog.Add(0, &mTestATraitUpdatableDataSink0, mTraitHandleSet[kTestATraitSink0Index]);
    mSourceCatalog.Add(0, &mTestATraitDataSource0, mTraitHandleSet[kTestBTraitSourceIndex]);

    mSinkCatalog.Add(0, &mTestBTraitUpdatableDataSink, mTraitHandleSet[kTestBTraitSinkIndex]);
    mSourceCatalog.Add(0, &mTestBTraitDataSource, mTraitHandleSet[kTestBTraitSourceIndex]);

    mTestATraitUpdatableDataSink0.SetUpdateEncoder(&mEncoder);
}

void WdmUpdateServerTest::SetupTest()
{
    mPathList.Clear();

    mTestATraitUpdatableDataSink0.tai_map.clear();

    for (int32_t i = 0; i < 10; i++)
    {
        mTestATraitUpdatableDataSink0.tai_map[i] = i + 100;
    }
}

void WdmUpdateServerTest::TearDownTest()
{
    if (mBuf != NULL)
    {
        PacketBuffer::Free(mBuf);
        mBuf = 0;
    }
}

void WdmUpdateServerTest::InitEncoderContext(nlTestSuite * inSuite)
{
    if (NULL == mBuf)
    {
        mBuf = PacketBuffer::New(0);
        NL_TEST_ASSERT(inSuite, mBuf != NULL);
    }

    mBuf->SetDataLength(0);

    mContext.mBuf                             = mBuf;
    mContext.mMaxPayloadSize                  = mBuf->AvailableDataLength();
    mContext.mUpdateRequestIndex              = 7;
    mContext.mExpiryTimeMicroSecond           = 0;
    mContext.mItemInProgress                  = 0;
    mContext.mNextDictionaryElementPathHandle = kNullPropertyPathHandle;
    mContext.mInProgressUpdateList            = &mPathList;
    mContext.mDataSinkCatalog                 = &mSinkCatalog;
}

void WdmUpdateServerTest::TestInitCleanup(nlTestSuite * inSuite, void * inContext)
{
    PRINT_TEST_NAME();

    NL_TEST_ASSERT(inSuite, 0 == mPathList.GetNumItems());
}

void WdmUpdateServerTest::TLVPrettyPrinter(const char * aFormat, ...)
{
    va_list args;

    va_start(args, aFormat);

    vprintf(aFormat, args);

    va_end(args);
}

WEAVE_ERROR WdmUpdateServerTest::DebugPrettyPrint(PacketBuffer * apMsgBuf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    nl::Weave::TLV::TLVReader reader;
    reader.Init(apMsgBuf);
    err = reader.Next();
    SuccessOrExit(err);
    nl::Weave::TLV::Debug::Dump(reader, TLVPrettyPrinter);

exit:
    if (WEAVE_NO_ERROR != err)
    {
        WeaveLogProgress(DataManagement, "DebugPrettyPrint fails with err %d", err);
    }

    return err;
}

WEAVE_ERROR WdmUpdateServerTest::VerifyUpdateRequest(nlTestSuite * inSuite, PacketBuffer * aPayload, size_t aItemToStartFrom)
{
    WEAVE_ERROR err     = WEAVE_NO_ERROR;
    PacketBuffer * pBuf = NULL;
    UpdateRequest::Parser update;
    Weave::TLV::TLVReader reader;
    bool existFailure                                                  = false;
    uint32_t numDataElements                                           = 0;
    uint32_t maxPayloadSize                                            = 0;
    SubscriptionEngine::StatusDataHandleElement * statusDataHandleList = NULL;
    uint8_t * pBufEndAddr                                              = NULL;
    SubscriptionEngine::UpdateRequestDataElementAccessControlDelegate acDelegate(NULL);

    err = SubscriptionEngine::GetInstance()->Init(&ExchangeMgr, this, NULL);
    SuccessOrExit(err);

    err = SubscriptionEngine::GetInstance()->EnablePublisher(NULL, &mSourceCatalog);
    SuccessOrExit(err);

    DebugPrettyPrint(aPayload);

    reader.Init(aPayload);

    err = reader.Next();
    SuccessOrExit(err);

    err = update.Init(reader);
    SuccessOrExit(err);

#if WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK
    err = update.CheckSchemaValidity();
    SuccessOrExit(err);
#endif // WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK

    {
        DataList::Parser dataList;
        err = update.GetDataList(&dataList);
        if (WEAVE_END_OF_TLV == err)
        {
            err = WEAVE_NO_ERROR;
        }
        SuccessOrExit(err);

        // re-initialize the reader to point to individual date element (reuse to save stack depth).
        dataList.GetReader(&reader);
    }

    err = SubscriptionEngine::AllocateRightSizedBuffer(pBuf, WDM_MAX_UPDATE_RESPONSE_SIZE, WDM_MIN_UPDATE_RESPONSE_SIZE,
                                                       maxPayloadSize);
    SuccessOrExit(err);

    statusDataHandleList =
        reinterpret_cast<SubscriptionEngine::StatusDataHandleElement *> WEAVE_SYSTEM_ALIGN_SIZE((size_t)(pBuf->Start()), 4);
    pBufEndAddr = pBuf->Start() + maxPayloadSize;
    err         = SubscriptionEngine::InitializeStatusDataHandleList(reader, statusDataHandleList, numDataElements, pBufEndAddr);
    SuccessOrExit(err);

    err = SubscriptionEngine::ProcessUpdateRequestDataList(reader, statusDataHandleList, &mSourceCatalog, acDelegate, existFailure,
                                                           numDataElements);
    SuccessOrExit(err);
exit:
    if (pBuf != NULL)
    {
        PacketBuffer::Free(pBuf);
        pBuf = NULL;
    }

    return err;
}

void WdmUpdateServerTest::UpdateServerBasicTestBody(nlTestSuite * inSuite)
{
    WEAVE_ERROR err;

    InitEncoderContext(inSuite);

    err = mEncoder.EncodeRequest(mContext);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    NL_TEST_ASSERT(inSuite, mPathList.GetPathStoreSize() == mContext.mItemInProgress);
    NL_TEST_ASSERT(inSuite, kNullPropertyPathHandle == mContext.mNextDictionaryElementPathHandle);

    err = VerifyUpdateRequest(inSuite, mBuf);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
}

void WdmUpdateServerTest::TestUpdateServerConditionalOneLeaf(nlTestSuite * inSuite, void * inContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    mTestATraitDataSource0.SetVersion(100);
    mTestATraitUpdatableDataSink0.SetUpdateRequiredVersion(100);
    mTestATraitUpdatableDataSink0.SetConditionalUpdate(true);

    PRINT_TEST_NAME();

    mTP = { mTraitHandleSet[kTestATraitSink0Index], CreatePropertyPathHandle(TestATrait::kPropertyHandle_TaC) };

    err = mPathList.AddItem(mTP);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    UpdateServerBasicTestBody(inSuite);

    NL_TEST_ASSERT(inSuite, mTestATraitDataSource0.GetVersion() == 101);
    mTestATraitUpdatableDataSink0.ClearUpdateRequiredVersion();
    mTestATraitUpdatableDataSink0.SetConditionalUpdate(false);
    NL_TEST_ASSERT(inSuite, 1 == mPathList.GetNumItems());
}

void WdmUpdateServerTest::TestUpdateServerUnconditionalOneLeaf(nlTestSuite * inSuite, void * inContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    mTestATraitDataSource0.SetVersion(100);

    PRINT_TEST_NAME();

    mTP = { mTraitHandleSet[kTestATraitSink0Index], CreatePropertyPathHandle(TestATrait::kPropertyHandle_TaC) };

    err = mPathList.AddItem(mTP);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    UpdateServerBasicTestBody(inSuite);
    NL_TEST_ASSERT(inSuite, mTestATraitDataSource0.GetVersion() == 101);

    NL_TEST_ASSERT(inSuite, 1 == mPathList.GetNumItems());
}

void WdmUpdateServerTest::TestUpdateServerMixedConditionalOneLeaf(nlTestSuite * inSuite, void * inContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    mTestATraitDataSource0.SetVersion(100);
    mTestATraitUpdatableDataSink0.SetUpdateRequiredVersion(100);
    mTestATraitUpdatableDataSink0.SetConditionalUpdate(true);

    mTestBTraitDataSource.SetVersion(200);

    PRINT_TEST_NAME();

    mTP = { mTraitHandleSet[kTestATraitSink0Index], CreatePropertyPathHandle(TestATrait::kPropertyHandle_TaC) };

    err = mPathList.AddItem(mTP);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    mTP = { mTraitHandleSet[kTestBTraitSinkIndex], CreatePropertyPathHandle(TestBTrait::kPropertyHandle_TaC) };

    err = mPathList.AddItem(mTP);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    UpdateServerBasicTestBody(inSuite);

    NL_TEST_ASSERT(inSuite, mTestATraitDataSource0.GetVersion() == 101);
    NL_TEST_ASSERT(inSuite, mTestBTraitDataSource.GetVersion() == 201);

    mTestATraitUpdatableDataSink0.ClearUpdateRequiredVersion();
    mTestATraitUpdatableDataSink0.SetConditionalUpdate(false);
    NL_TEST_ASSERT(inSuite, 2 == mPathList.GetNumItems());
}

void WdmUpdateServerTest::TestUpdateServerConditionalTwoProperties(nlTestSuite * inSuite, void * inContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    mTestATraitDataSource0.SetVersion(100);
    mTestATraitUpdatableDataSink0.SetUpdateRequiredVersion(100);
    mTestATraitUpdatableDataSink0.SetConditionalUpdate(true);

    PRINT_TEST_NAME();

    mTP = { mTraitHandleSet[kTestATraitSink0Index], CreatePropertyPathHandle(TestATrait::kPropertyHandle_TaA) };

    err = mPathList.AddItem(mTP);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    mTP.mPropertyPathHandle = CreatePropertyPathHandle(TestATrait::kPropertyHandle_TaB);
    err                     = mPathList.AddItem(mTP);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    UpdateServerBasicTestBody(inSuite);

    NL_TEST_ASSERT(inSuite, mTestATraitDataSource0.GetVersion() == 101);

    mTestATraitUpdatableDataSink0.ClearUpdateRequiredVersion();
    mTestATraitUpdatableDataSink0.SetConditionalUpdate(false);

    NL_TEST_ASSERT(inSuite, 2 == mPathList.GetNumItems());
}

void WdmUpdateServerTest::TestUpdateServerUnconditionalTwoProperties(nlTestSuite * inSuite, void * inContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    mTestATraitDataSource0.SetVersion(100);

    PRINT_TEST_NAME();

    mTP = { mTraitHandleSet[kTestATraitSink0Index], CreatePropertyPathHandle(TestATrait::kPropertyHandle_TaA) };

    err = mPathList.AddItem(mTP);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    mTP.mPropertyPathHandle = CreatePropertyPathHandle(TestATrait::kPropertyHandle_TaB);
    err                     = mPathList.AddItem(mTP);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    UpdateServerBasicTestBody(inSuite);

    NL_TEST_ASSERT(inSuite, mTestATraitDataSource0.GetVersion() == 101);

    NL_TEST_ASSERT(inSuite, 2 == mPathList.GetNumItems());
}

void WdmUpdateServerTest::TestUpdateServerMixedConditionalTwoProperties(nlTestSuite * inSuite, void * inContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    mTestATraitDataSource0.SetVersion(100);
    mTestATraitUpdatableDataSink0.SetUpdateRequiredVersion(100);
    mTestATraitUpdatableDataSink0.SetConditionalUpdate(true);

    PRINT_TEST_NAME();

    mTP = { mTraitHandleSet[kTestATraitSink0Index], CreatePropertyPathHandle(TestATrait::kPropertyHandle_TaA) };

    err = mPathList.AddItem(mTP);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    mTP.mPropertyPathHandle = CreatePropertyPathHandle(TestATrait::kPropertyHandle_TaB);
    err                     = mPathList.AddItem(mTP);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    mTP = { mTraitHandleSet[kTestBTraitSinkIndex], CreatePropertyPathHandle(TestBTrait::kPropertyHandle_TaC) };

    err = mPathList.AddItem(mTP);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    mTP.mPropertyPathHandle = CreatePropertyPathHandle(TestATrait::kPropertyHandle_TaD_SaA);
    err                     = mPathList.AddItem(mTP);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    UpdateServerBasicTestBody(inSuite);

    NL_TEST_ASSERT(inSuite, mTestATraitDataSource0.GetVersion() == 101);
    NL_TEST_ASSERT(inSuite, mTestBTraitDataSource.GetVersion() == 202);

    mTestATraitUpdatableDataSink0.ClearUpdateRequiredVersion();
    mTestATraitUpdatableDataSink0.SetConditionalUpdate(false);

    NL_TEST_ASSERT(inSuite, 4 == mPathList.GetNumItems());
}

} // namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current)
} // namespace Profiles
} // namespace Weave
} // namespace nl

WdmUpdateServerTest gWdmUpdateServerTest;

void WdmUpdateEncoderTest_InitCleanup(nlTestSuite * inSuite, void * inContext)
{
    gWdmUpdateServerTest.TestInitCleanup(inSuite, inContext);
}

void WdmUpdateServerTest_ConditionalOneLeaf(nlTestSuite * inSuite, void * inContext)
{
    gWdmUpdateServerTest.TestUpdateServerConditionalOneLeaf(inSuite, inContext);
}

void WdmUpdateServerTest_UnconditionalOneLeaf(nlTestSuite * inSuite, void * inContext)
{
    gWdmUpdateServerTest.TestUpdateServerUnconditionalOneLeaf(inSuite, inContext);
}

void WdmUpdateServerTest_MiexedConditionalOneLeaf(nlTestSuite * inSuite, void * inContext)
{
    gWdmUpdateServerTest.TestUpdateServerMixedConditionalOneLeaf(inSuite, inContext);
}

void WdmUpdateServerTest_ConditionalTwoProperties(nlTestSuite * inSuite, void * inContext)
{
    gWdmUpdateServerTest.TestUpdateServerConditionalTwoProperties(inSuite, inContext);
}

void WdmUpdateServerTest_UnconditionalTwoProperties(nlTestSuite * inSuite, void * inContext)
{
    gWdmUpdateServerTest.TestUpdateServerUnconditionalTwoProperties(inSuite, inContext);
}

void WdmUpdateServerTest_MixedConditionalTwoProperties(nlTestSuite * inSuite, void * inContext)
{
    gWdmUpdateServerTest.TestUpdateServerMixedConditionalTwoProperties(inSuite, inContext);
}

// Test Suite

/**
 *  Test Suite that lists all the test functions.
 */
static const nlTest sTests[] = { NL_TEST_DEF("Init and cleanup", WdmUpdateEncoderTest_InitCleanup),
                                 NL_TEST_DEF("Decode conditional one leaf", WdmUpdateServerTest_ConditionalOneLeaf),
                                 NL_TEST_DEF("Decode unconditional one leaf", WdmUpdateServerTest_UnconditionalOneLeaf),
                                 NL_TEST_DEF("Decode mixedconditional one leaf", WdmUpdateServerTest_MiexedConditionalOneLeaf),
                                 NL_TEST_DEF("Decode conditional two properties", WdmUpdateServerTest_ConditionalTwoProperties),
                                 NL_TEST_DEF("Decode unconditional two properties", WdmUpdateServerTest_UnconditionalTwoProperties),
                                 NL_TEST_DEF("Decode mixedconditional two properties",
                                             WdmUpdateServerTest_MixedConditionalTwoProperties),
                                 NL_TEST_SENTINEL() };

/**
 *  Set up the test suite.
 */
static int SuiteSetup(void * inContext)
{
    return 0;
}

/**
 *  Tear down the test suite.
 */
static int SuiteTeardown(void * inContext)
{
    return 0;
}

/**
 *  Set up each test.
 */
static int TestSetup(void * inContext)
{
    gWdmUpdateServerTest.SetupTest();

    return 0;
}

/**
 *  Tear down each test.
 */
static int TestTeardown(void * inContext)
{
    gWdmUpdateServerTest.TearDownTest();

    return 0;
}

/**
 *  Main
 */
int main(int argc, char * argv[])
{
#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    tcpip_init(NULL, NULL);
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

    nlTestSuite theSuite = { "weave-WdmUpdateServer", &sTests[0], SuiteSetup, SuiteTeardown, TestSetup, TestTeardown };

    // Generate machine-readable, comma-separated value (CSV) output.
    nl_test_set_output_style(OUTPUT_CSV);

    // Run test suit against one context
    nlTestRunner(&theSuite, NULL);

    return nlTestRunnerStats(&theSuite);
}

#else // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING && WEAVE_CONFIG_ENABLE_WDM_UPDATE

int main(int argc, char * argv[])
{
    return 0;
}

#endif // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING && WEAVE_CONFIG_ENABLE_WDM_UPDATE
