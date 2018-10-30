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
 *      This file implements unit tests for the encoding and parsing of
 *      of WDM UpdateResponse payloads.
 *
 */

#include "ToolCommon.h"

#include <nlbyteorder.h>
#include <nlunit-test.h>

#include <Weave/Core/WeaveCore.h>

#include <Weave/Profiles/data-management/Current/WdmManagedNamespace.h>
#include <Weave/Profiles/data-management/DataManagement.h>

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
    static SubscriptionEngine *gSubscriptionEngine = NULL;
    return gSubscriptionEngine;
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

} // Platform

} // WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current)
} // Profiles
} // Weave
} // nl

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING && WEAVE_CONFIG_ENABLE_WDM_UPDATE
namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current) {

class WdmUpdateResponseTest {
    public:
        WdmUpdateResponseTest() { }
        ~WdmUpdateResponseTest() { }

        // Tests
        void SetupTest();
        void TearDownTest();

        void TestVersionList(nlTestSuite *inSuite, void *inContext);
        void TestStatusList(nlTestSuite *inSuite, void *inContext, bool aUseDeprecatedFormat);
        void TestStatusList(nlTestSuite *inSuite, void *inContext);
        void TestDeprecatedStatusList(nlTestSuite *inSuite, void *inContext);
        void TestUpdateResponse(nlTestSuite *inSuite, void *inContext);
        void TestCompactResponse(nlTestSuite *inSuite, void *inContext);

    private:
        // Objects under test
        VersionList::Builder mVersionListBuilder;
        VersionList::Parser mVersionListParser;
        StatusList::Builder mStatusListBuilder;
        StatusList::Parser mStatusListParser;
        UpdateResponse::Builder mUpdateResponseBuilder;
        UpdateResponse::Parser mUpdateResponseParser;


        // These are here for convenience
        uint8_t mBuf[1024];
        Weave::TLV::TLVWriter mWriter;
        Weave::TLV::TLVReader mReader;

        // Test support functions
        static WEAVE_ERROR WriteVersionList(VersionList::Builder &aBuilder);
        static WEAVE_ERROR WriteStatusList(StatusList::Builder &aBuilder);
        static void VerifyVersionList(nlTestSuite *inSuite, VersionList::Parser &aParser);
        static void VerifyStatusList(nlTestSuite *inSuite, StatusList::Parser &aParser);
};

void WdmUpdateResponseTest::SetupTest()
{
    memset(mBuf, 0, sizeof(mBuf));
}

void WdmUpdateResponseTest::TearDownTest()
{
}

WEAVE_ERROR WdmUpdateResponseTest::WriteVersionList(VersionList::Builder &aBuilder)
{
    uint64_t version;

    version = 1;
    aBuilder.AddVersion(version);
    version = 2;
    aBuilder.AddVersion(version);

    aBuilder.EndOfVersionList();

    return aBuilder.GetError();
}

WEAVE_ERROR WdmUpdateResponseTest::WriteStatusList(StatusList::Builder &aBuilder)
{
    aBuilder.AddStatus(0x1, 0x2);

    aBuilder.AddStatus(0x1, 0x3);

    aBuilder.EndOfStatusList();

    return aBuilder.GetError();
}

void WdmUpdateResponseTest::VerifyVersionList(nlTestSuite *inSuite, VersionList::Parser &aParser)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint64_t version;

    err = aParser.Next();
    NL_TEST_ASSERT(inSuite, WEAVE_NO_ERROR == err);

    err = aParser.GetVersion(&version);
    NL_TEST_ASSERT(inSuite, WEAVE_NO_ERROR == err);
    NL_TEST_ASSERT(inSuite, 1 == version);

    err = aParser.Next();
    NL_TEST_ASSERT(inSuite, WEAVE_NO_ERROR == err);
    err = aParser.GetVersion(&version);
    NL_TEST_ASSERT(inSuite, WEAVE_NO_ERROR == err);
    NL_TEST_ASSERT(inSuite, 2 == version);

    err = aParser.Next();
    NL_TEST_ASSERT(inSuite, err == WEAVE_END_OF_TLV);
}

void WdmUpdateResponseTest::VerifyStatusList(nlTestSuite *inSuite, StatusList::Parser &aParser)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint32_t profile;
    uint16_t statusCode;

    err = aParser.Next();
    NL_TEST_ASSERT(inSuite, WEAVE_NO_ERROR == err);

    err = aParser.GetProfileIDAndStatusCode(&profile, &statusCode);
    NL_TEST_ASSERT(inSuite, WEAVE_NO_ERROR == err);
    NL_TEST_ASSERT(inSuite, 1 == profile);
    NL_TEST_ASSERT(inSuite, 2 == statusCode);

    err = aParser.Next();
    NL_TEST_ASSERT(inSuite, WEAVE_NO_ERROR == err);

    err = aParser.GetProfileIDAndStatusCode(&profile, &statusCode);
    NL_TEST_ASSERT(inSuite, WEAVE_NO_ERROR == err);
    NL_TEST_ASSERT(inSuite, 1 == profile);
    NL_TEST_ASSERT(inSuite, 3 == statusCode);

    err = aParser.Next();
    NL_TEST_ASSERT(inSuite, err == WEAVE_END_OF_TLV);
}

void WdmUpdateResponseTest::TestVersionList(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    PRINT_TEST_NAME();

    mWriter.Init(mBuf, sizeof(mBuf));

    mVersionListBuilder.Init(&mWriter);

    err = WriteVersionList(mVersionListBuilder);

    NL_TEST_ASSERT(inSuite, WEAVE_NO_ERROR == err);

    uint32_t lenWritten = mWriter.GetLengthWritten();
    printf("lenWritten: %" PRIu32 "\n", lenWritten);

    mReader.Init(mBuf, lenWritten);
    err = mReader.Next();
    NL_TEST_ASSERT(inSuite, WEAVE_NO_ERROR == err);

    err = mVersionListParser.Init(mReader);
    NL_TEST_ASSERT(inSuite, WEAVE_NO_ERROR == err);

    err = mVersionListParser.CheckSchemaValidity();
    NL_TEST_ASSERT(inSuite, WEAVE_NO_ERROR == err);

    VerifyVersionList(inSuite, mVersionListParser);

    // Now test overflows

    for (size_t maxLen = 0; maxLen < lenWritten; maxLen++)
    {
        mWriter.Init(mBuf, maxLen);

        err = mVersionListBuilder.Init(&mWriter);

        if (WEAVE_NO_ERROR == err)
            err = WriteVersionList(mVersionListBuilder);

        NL_TEST_ASSERT(inSuite, WEAVE_ERROR_BUFFER_TOO_SMALL == err);

        mReader.Init(mBuf, maxLen);
        err = mReader.Next();

        if (WEAVE_NO_ERROR == err)
            err = mVersionListParser.Init(mReader);

        if (WEAVE_NO_ERROR == err)
            err = mVersionListParser.CheckSchemaValidity();

        // Note that CheckSchemaValidity succeeds if it can parse out the last StatusCode.
        // It does not care if the containers are not terminated properly at the end.
        NL_TEST_ASSERT(inSuite, WEAVE_END_OF_TLV == err || WEAVE_ERROR_WDM_MALFORMED_STATUS_ELEMENT == err || WEAVE_ERROR_TLV_UNDERRUN == err || WEAVE_NO_ERROR == err);
    }
}

void WdmUpdateResponseTest::TestStatusList(nlTestSuite *inSuite, void *inContext, bool aUseDeprecatedFormat)
{
    WEAVE_ERROR err;

    PRINT_TEST_NAME();

    mWriter.Init(mBuf, sizeof(mBuf));

    err = mStatusListBuilder.Init(&mWriter);
    NL_TEST_ASSERT(inSuite, WEAVE_NO_ERROR == err);

    if (aUseDeprecatedFormat)
    {
        mStatusListBuilder.UseDeprecatedFormat();
    }

    err = WriteStatusList(mStatusListBuilder);

    NL_TEST_ASSERT(inSuite, WEAVE_NO_ERROR == err);

    uint32_t lenWritten = mWriter.GetLengthWritten();
    printf("lenWritten: %" PRIu32 "\n", lenWritten);

    mReader.Init(mBuf, lenWritten);
    err = mReader.Next();
    NL_TEST_ASSERT(inSuite, WEAVE_NO_ERROR == err);

    err = mStatusListParser.Init(mReader);
    NL_TEST_ASSERT(inSuite, WEAVE_NO_ERROR == err);

    err = mStatusListParser.CheckSchemaValidity();
    NL_TEST_ASSERT(inSuite, WEAVE_NO_ERROR == err);

    VerifyStatusList(inSuite, mStatusListParser);

    // Now test overflows

    for (size_t maxLen = 0; maxLen < lenWritten; maxLen++)
    {
        mWriter.Init(mBuf, maxLen);

        err = mStatusListBuilder.Init(&mWriter);

        if (aUseDeprecatedFormat)
        {
            mStatusListBuilder.UseDeprecatedFormat();
        }

        if (WEAVE_NO_ERROR == err)
            err = WriteStatusList(mStatusListBuilder);

        printf("maxLen = %zu, err = %d\n", maxLen, err);
        NL_TEST_ASSERT(inSuite, WEAVE_ERROR_BUFFER_TOO_SMALL == err);

        mReader.Init(mBuf, maxLen);
        err = mReader.Next();

        if (WEAVE_NO_ERROR == err)
            err = mStatusListParser.Init(mReader);

        if (WEAVE_NO_ERROR == err)
            err = mStatusListParser.CheckSchemaValidity();

        // Note that CheckSchemaValidity succeeds if it can parse out the last StatusCode.
        // It does not care if the containers are not terminated properly at the end.
        NL_TEST_ASSERT(inSuite, WEAVE_END_OF_TLV == err || WEAVE_ERROR_WDM_MALFORMED_STATUS_ELEMENT == err || WEAVE_ERROR_TLV_UNDERRUN == err || WEAVE_NO_ERROR == err);
    }
}

void WdmUpdateResponseTest::TestStatusList(nlTestSuite *inSuite, void *inContext)
{
    TestStatusList(inSuite, inContext, false);
}

void WdmUpdateResponseTest::TestDeprecatedStatusList(nlTestSuite *inSuite, void *inContext)
{
    TestStatusList(inSuite, inContext, true);
}

void WdmUpdateResponseTest::TestUpdateResponse(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    PRINT_TEST_NAME();

    mWriter.Init(mBuf, sizeof(mBuf));

    err = mUpdateResponseBuilder.Init(&mWriter);
    NL_TEST_ASSERT(inSuite, WEAVE_NO_ERROR == err);

    VersionList::Builder &lVLBuilder = mUpdateResponseBuilder.CreateVersionListBuilder();

    err = WriteVersionList(lVLBuilder);
    NL_TEST_ASSERT(inSuite, WEAVE_NO_ERROR == err);

    uint32_t lenWritten = mWriter.GetLengthWritten();
    printf("After VersionList, lenWritten: %" PRIu32 "\n", lenWritten);

    StatusList::Builder &lSLBuilder = mUpdateResponseBuilder.CreateStatusListBuilder();

    err = WriteStatusList(lSLBuilder);
    NL_TEST_ASSERT(inSuite, WEAVE_NO_ERROR == err);

    lenWritten = mWriter.GetLengthWritten();
    printf("After StatusList, lenWritten: %" PRIu32 "\n", lenWritten);

    mUpdateResponseBuilder.EndOfResponse();
    NL_TEST_ASSERT(inSuite, WEAVE_NO_ERROR == mUpdateResponseBuilder.GetError());

    lenWritten = mWriter.GetLengthWritten();
    printf("After whole response, lenWritten: %" PRIu32 "\n", lenWritten);

    mReader.Init(mBuf, lenWritten);
    err = mReader.Next();
    NL_TEST_ASSERT(inSuite, WEAVE_NO_ERROR == err);

    err = mUpdateResponseParser.Init(mReader);
    NL_TEST_ASSERT(inSuite, WEAVE_NO_ERROR == err);

    err = mUpdateResponseParser.CheckSchemaValidity();
    NL_TEST_ASSERT(inSuite, WEAVE_NO_ERROR == err);

    err = mUpdateResponseParser.GetStatusList(&mStatusListParser);
    NL_TEST_ASSERT(inSuite, WEAVE_NO_ERROR == err);

    err = mUpdateResponseParser.GetVersionList(&mVersionListParser);
    NL_TEST_ASSERT(inSuite, WEAVE_NO_ERROR == err);

    VerifyStatusList(inSuite, mStatusListParser);
    VerifyVersionList(inSuite, mVersionListParser);

    // Now test overflows

    for (size_t maxLen = 0; maxLen < lenWritten; maxLen++)
    {
        memset(mBuf, 0x42, sizeof(mBuf));

        // Check that the error progates through all calls.
        mWriter.Init(mBuf, maxLen);

        mUpdateResponseBuilder.Init(&mWriter);

        VersionList::Builder &lTmpVLBuilder = mUpdateResponseBuilder.CreateVersionListBuilder();

        WriteVersionList(lTmpVLBuilder);

        StatusList::Builder &lTmpSLBuilder = mUpdateResponseBuilder.CreateStatusListBuilder();

        WriteStatusList(lTmpSLBuilder);

        mUpdateResponseBuilder.EndOfResponse();

        err = mUpdateResponseBuilder.GetError();

        printf("maxLen = %zu, err = %d\n", maxLen, err);
        NL_TEST_ASSERT(inSuite, WEAVE_ERROR_BUFFER_TOO_SMALL == err);

        // Check the TLVWriter has not gone over the max length
        NL_TEST_ASSERT(inSuite, 0x42 == mBuf[maxLen]);

        mReader.Init(mBuf, maxLen);
        err = mReader.Next();

        if (WEAVE_NO_ERROR == err)
            err = mUpdateResponseParser.Init(mReader);

        if (WEAVE_NO_ERROR == err)
            err = mUpdateResponseParser.CheckSchemaValidity();

        // Note that CheckSchemaValidity succeeds if it can parse out the last StatusCode.
        // It does not care if the containers are not terminated properly at the end.
        NL_TEST_ASSERT(inSuite, WEAVE_END_OF_TLV == err || WEAVE_ERROR_WDM_MALFORMED_STATUS_ELEMENT == err || WEAVE_ERROR_TLV_UNDERRUN == err || WEAVE_NO_ERROR == err);
    }
}

/**
 * If the whole update is successful, the publisher can send an empty StatusList
 */
void WdmUpdateResponseTest::TestCompactResponse(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    PRINT_TEST_NAME();

    mWriter.Init(mBuf, sizeof(mBuf));

    err = mUpdateResponseBuilder.Init(&mWriter);
    NL_TEST_ASSERT(inSuite, WEAVE_NO_ERROR == err);

    VersionList::Builder &lVLBuilder = mUpdateResponseBuilder.CreateVersionListBuilder();

    err = WriteVersionList(lVLBuilder);
    NL_TEST_ASSERT(inSuite, WEAVE_NO_ERROR == err);

    uint32_t lenWritten = mWriter.GetLengthWritten();
    printf("After VersionList, lenWritten: %" PRIu32 "\n", lenWritten);

    StatusList::Builder &lSLBuilder = mUpdateResponseBuilder.CreateStatusListBuilder();

    lSLBuilder.EndOfStatusList();

    err = lSLBuilder.GetError();
    NL_TEST_ASSERT(inSuite, WEAVE_NO_ERROR == err);

    lenWritten = mWriter.GetLengthWritten();
    printf("After StatusList, lenWritten: %" PRIu32 "\n", lenWritten);

    mUpdateResponseBuilder.EndOfResponse();
    NL_TEST_ASSERT(inSuite, WEAVE_NO_ERROR == mUpdateResponseBuilder.GetError());

    lenWritten = mWriter.GetLengthWritten();
    printf("After whole response, lenWritten: %" PRIu32 "\n", lenWritten);

    mReader.Init(mBuf, lenWritten);
    err = mReader.Next();
    NL_TEST_ASSERT(inSuite, WEAVE_NO_ERROR == err);

    err = mUpdateResponseParser.Init(mReader);
    NL_TEST_ASSERT(inSuite, WEAVE_NO_ERROR == err);

    err = mUpdateResponseParser.CheckSchemaValidity();
    NL_TEST_ASSERT(inSuite, WEAVE_NO_ERROR == err);

    err = mUpdateResponseParser.GetStatusList(&mStatusListParser);
    NL_TEST_ASSERT(inSuite, WEAVE_NO_ERROR == err);

    err = mUpdateResponseParser.GetVersionList(&mVersionListParser);
    NL_TEST_ASSERT(inSuite, WEAVE_NO_ERROR == err);

    VerifyVersionList(inSuite, mVersionListParser);

    uint32_t profile;
    uint16_t statusCode;
    err = mStatusListParser.Next();
    printf("Emtpy StatusList: Next err %d\n", err);
    NL_TEST_ASSERT(inSuite, WEAVE_END_OF_TLV == err);

    err = mStatusListParser.GetProfileIDAndStatusCode(&profile, &statusCode);
    printf("Emtpy StatusList: GetProfileIDAndStatusCode err %d\n", err);
    NL_TEST_ASSERT(inSuite, WEAVE_NO_ERROR != err);
}

} // WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current)
}
}
}


WdmUpdateResponseTest gWdmUpdateResponseTest;



void WdmUpdateResponseTest_VersionList(nlTestSuite *inSuite, void *inContext)
{
    gWdmUpdateResponseTest.TestVersionList(inSuite, inContext);
}

void WdmUpdateResponseTest_StatusList(nlTestSuite *inSuite, void *inContext)
{
    gWdmUpdateResponseTest.TestStatusList(inSuite, inContext);
}

void WdmUpdateResponseTest_DeprecatedStatusList(nlTestSuite *inSuite, void *inContext)
{
    gWdmUpdateResponseTest.TestDeprecatedStatusList(inSuite, inContext);
}

void WdmUpdateResponseTest_UpdateResponse(nlTestSuite *inSuite, void *inContext)
{
    gWdmUpdateResponseTest.TestUpdateResponse(inSuite, inContext);
}

void WdmUpdateResponseTest_CompactResponse(nlTestSuite *inSuite, void *inContext)
{
    gWdmUpdateResponseTest.TestCompactResponse(inSuite, inContext);
}
// Test Suite

/**
 *  Test Suite that lists all the test functions.
 */
static const nlTest sTests[] = {
    NL_TEST_DEF("VersionList",  WdmUpdateResponseTest_VersionList),
    NL_TEST_DEF("StatusList",  WdmUpdateResponseTest_StatusList),
    NL_TEST_DEF("DeprecatedStatusList",  WdmUpdateResponseTest_DeprecatedStatusList),
    NL_TEST_DEF("UpdateResponse",  WdmUpdateResponseTest_UpdateResponse),
    NL_TEST_DEF("Compact UpdateResponse",  WdmUpdateResponseTest_CompactResponse),

    NL_TEST_SENTINEL()
};

/**
 *  Set up the test suite.
 */
static int SuiteSetup(void *inContext)
{
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
    gWdmUpdateResponseTest.SetupTest();

    return 0;
}

/**
 *  Tear down each test.
 */
static int TestTeardown(void *inContext)
{
    gWdmUpdateResponseTest.TearDownTest();

    return 0;
}


/**
 *  Main
 */
int main(int argc, char *argv[])
{
#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    tcpip_init(NULL, NULL);
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

    nlTestSuite theSuite = {
        "weave-WdmUpdateResponse",
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

#else  // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING && WEAVE_CONFIG_ENABLE_WDM_UPDATE

int main(int argc, char *argv[])
{
    return 0;
}

#endif // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING && WEAVE_CONFIG_ENABLE_WDM_UPDATE
