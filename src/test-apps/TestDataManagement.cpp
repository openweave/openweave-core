/*
 *
 *    Copyright (c) 2014-2017 Nest Labs, Inc.
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
 *      This file implements an application that is intended to be a
 *      comprehensive tester for all aspects of Weave Data Management
 *      (WDM) that one can test in standalone mode. As such, it tests
 *      data structures, subsystems and networked behaviors. The
 *      latter requires a mock device acting as a WDM server and using
 *      the same test profile.
 *
 */

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include <assert.h>

// This code tests the legacy version of Weave Data Management
#define WEAVE_CONFIG_DATA_MANAGEMENT_NAMESPACE kWeaveManagedNamespace_Legacy

#include "ToolCommon.h"
#include <Weave/Profiles/ProfileCommon.h>
#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Profiles/data-management/ProfileDatabase.h>
#include "TestProfile.h"

using namespace ::nl::Inet;
using namespace ::nl::Weave;
using namespace ::nl::Weave::TLV;
using namespace ::nl::Weave::Profiles;
using namespace ::nl::Weave::Profiles::Common;
using namespace ::nl::Weave::Profiles::DataManagement;
using namespace ::nl::Weave::Profiles::StatusReporting;

#define TOOL_NAME "TestDataManagement"

static bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg);
static bool HandleNonOptionArgs(const char *progName, int argc, char *argv[]);
static void HandleConnectionReceived(WeaveMessageLayer *msgLayer, WeaveConnection *con);
static void HandleSecureSessionEstablished(WeaveSecurityManager *sm, WeaveConnection *con, void *reqState, uint16_t sessionKeyId, uint64_t peerNodeId, uint8_t encType);
static void HandleSecureSessionError(WeaveSecurityManager *sm, WeaveConnection *con, void *reqState, WEAVE_ERROR localErr, uint64_t peerNodeId, StatusReport *statusReport);
static void HandleConnectionClosed(WeaveConnection *con, WEAVE_ERROR conErr);
// static void HandleConnectionComplete(WeaveConnection *con, WEAVE_ERROR conErr);

uint64_t DestNodeId;
IPAddress DestAddr = IPAddress::Any; // TODO BUG: This isn't used anywhere, despite being settable from the command-line.
bool Debug = false;

/*
 * in some circumstances we may want to use multiple clients and then
 * we have to count them to know when we're done.
 */

uint8_t gClientCount = 1;
uint8_t gClientCounter = 0;
int gCyclingCnt = 32;

static OptionDef gToolOptionDefs[] =
{
    { "dest-addr",      kArgumentRequired,  'D' },
    { "cycling-cnt",    kArgumentRequired,  'c' },
#if WEAVE_CONFIG_SECURITY_TEST_MODE
    { "allow-dups",     kNoArgument,        'A' },
#endif
    { NULL }
};

static const char *gToolOptionHelp =
    "  -D, --dest-addr <dest-node-ip-addr>\n"
    "       Send weave messages to a specific IPv4/IPv6 address rather than one\n"
    "       derived the destination node id.\n"
    "\n"
    "  -c, --cycling-cnt <num>"
    "       The count of the cycling test\n"
    "\n"
#if WEAVE_CONFIG_SECURITY_TEST_MODE
    "  -A, --allow-dups\n"
    "       Allow reception of duplicate messages.\n"
    "\n"
#endif
    "";

static OptionSet gToolOptions =
{
    HandleOption,
    gToolOptionDefs,
    "GENERAL OPTIONS",
    gToolOptionHelp
};

static HelpOptions gHelpOptions(
    TOOL_NAME,
    "Usage: " TOOL_NAME " [<options...>] [<dest-node-id>]\n",
    WEAVE_VERSION_STRING "\n" WEAVE_TOOL_COPYRIGHT
);

static OptionSet *gToolOptionSets[] =
{
    &gToolOptions,
    &gNetworkOptions,
    &gWeaveNodeOptions,
    &gFaultInjectionOptions,
    &gHelpOptions,
    NULL
};

#define EnterTest(_name_, _errName_)                 \
  WEAVE_ERROR _errName_ = (                          \
    printf("\n\n"                                    \
           _name_                                    \
           "---\n"),                                 \
    WEAVE_NO_ERROR                                   \
  )

#define DriveTest(_done_)                            \
  do {                                               \
    while (!(_done_))                                \
    {                                                \
        struct timeval sleepTime;                    \
        sleepTime.tv_sec = 0;                        \
        sleepTime.tv_usec = 100000;                  \
        ServiceNetwork(sleepTime);                   \
    }                                                \
  } while (0)

#define ExitTest(_err_)                              \
  do {                                               \
    if ((_err_) == WEAVE_NO_ERROR)                   \
        printf("Success\n");                         \
    else                                             \
    {                                                \
        printf("error: %s\n", ErrorStr(_err_));      \
        exit(-1);                                    \
    }                                                \
  } while (0)

static WEAVE_ERROR ValidatePath(TLVReader &aReader,
                                const uint64_t &aTag,
                                uint32_t aProfileId,
                                const uint64_t &aInstanceId,
                                uint32_t aPathLen,
                                ...)
{
    WEAVE_ERROR err;

    va_list pathTags;

    TLVType pathContainer;
    TLVType profileContainer;

    uint32_t profileId;
    uint64_t instanceId = kInstanceIdNotSpecified;

    // This following line must be executed before the execution flow could possibly reach the exit label,
    // for we call va_end at there unconditionally.
    va_start(pathTags, aPathLen);

    VerifyOrExit((aReader.GetType() == kTLVType_Path), err = WEAVE_ERROR_WRONG_TLV_TYPE);

    err = ValidateWDMTag(aTag, aReader);
    SuccessOrExit(err);

    err = aReader.EnterContainer(pathContainer);
    SuccessOrExit(err);

    /*
     * the first element of a path under WDM should be a structure
     * with 2 elements, one of which (the instance) is optional.
     */

    err = aReader.Next();
    SuccessOrExit(err);

    VerifyOrExit(aReader.GetType() == kTLVType_Structure, err = WEAVE_ERROR_UNEXPECTED_TLV_ELEMENT);

    err = ValidateWDMTag(kTag_WDMPathProfile, aReader);
    SuccessOrExit(err);

    /*
     * check the path profile and instance
     */

    err = aReader.EnterContainer(profileContainer);
    SuccessOrExit(err);

    /*
     * the first element here should be a profile ID
     */

    err = aReader.Next();
    SuccessOrExit(err);

    err = ValidateWDMTag(kTag_WDMPathProfileId, aReader);
    SuccessOrExit(err);

    err = aReader.Get(profileId);
    SuccessOrExit(err);

    VerifyOrExit(profileId == aProfileId, err = WEAVE_ERROR_INVALID_PROFILE_ID);

    /*
     * and the second may be an instance
     */

    err = aReader.Next();

    if (err != WEAVE_END_OF_TLV)
    {
        err = ValidateWDMTag(kTag_WDMPathProfileInstance, aReader);
        SuccessOrExit(err);

        err = aReader.Get(instanceId);
        SuccessOrExit(err);
    }

    VerifyOrExit(instanceId == aInstanceId, err = WEAVE_ERROR_INCORRECT_STATE);

    err = aReader.ExitContainer(profileContainer);
    SuccessOrExit(err);

    // now, the residual path elements, if any

    for (uint32_t i = 0; i < aPathLen; i++)
    {
        err = aReader.Next();
        SuccessOrExit(err);

        if (err == WEAVE_NO_ERROR)
        {
            if (aReader.GetTag() != va_arg(pathTags, uint64_t))
            {
                err = WEAVE_ERROR_INVALID_TLV_TAG;

                break;
            }
        }

        else
        {
            err = WEAVE_ERROR_TLV_UNDERRUN;
            break;
        }
    }

    /*
     * again, don't hide the error that caused us to exit behind
     * the error you might get trying to exit the conmtainer.
     */

    if (err == WEAVE_NO_ERROR)
        err = aReader.ExitContainer(pathContainer);

    else
        aReader.ExitContainer(pathContainer);

exit:
    va_end(pathTags);

    return err;
}

/*
 * in order to use the "new improved" data management, we have to create
 * a sub-class of the WDM client and supply the relevant methods as follows.
 */

class DMTestClient :
    public DMClient
{
public:
    WEAVE_ERROR ViewConfirm(const uint64_t &aResponderId, StatusReport &aStatus, uint16_t aTxnId)
    {
        WEAVE_ERROR err = WEAVE_NO_ERROR;

        printf("processing: <view confirm - non-success status>\n");

        Done = true;

        return err;
    }

    WEAVE_ERROR ViewConfirm(const uint64_t &aResponderId, ReferencedTLVData &aDataList, uint16_t aTxnId)
    {
        WEAVE_ERROR err = WEAVE_NO_ERROR;

        printf("processing: <view confirm - success status>\n");

        err = mDatabase.Store(aDataList);

        if (err != WEAVE_NO_ERROR)
            printf("<view confirm> error: %x\n", err);

        Done = true;

        return err;
    }

    WEAVE_ERROR UpdateConfirm(const uint64_t &aResponderId, StatusReport &aStatus, uint16_t aTxnId)
    {
        WEAVE_ERROR err = WEAVE_NO_ERROR;

        if (aStatus.success())
            printf("processing: <update confirm - success!>\n");

        else
            printf("processing: <update confirm - non-success status>\n");

        Done = true;

        return err;
    }

    void IncompleteIndication(const uint64_t &aPeerNodeId, StatusReport &aReport)
    {
        printf("processing: <incomplete indication>\n");
    }

#if WEAVE_CONFIG_WDM_ALLOW_CLIENT_SUBSCRIPTION

    WEAVE_ERROR SubscribeConfirm(const uint64_t &aResponderId, StatusReport &aStatus, uint16_t aTxnId)
    {
        WEAVE_ERROR err = WEAVE_NO_ERROR;

        printf("processing: <subscribe confirm - non-success status>\n");

        gClientCounter++;

        if (gClientCounter == gClientCount)
            Done = true;

        return err;
    }

    WEAVE_ERROR SubscribeConfirm(const uint64_t &aResponderId,
                                 const TopicIdentifier &aTopicId,
                                 uint16_t aTxnId)
    {
        WEAVE_ERROR err = WEAVE_NO_ERROR;

        printf("processing: <subscribe confirm - success status, no data list>\n");

        Done = true;

        return err;
    }

    WEAVE_ERROR SubscribeConfirm(const uint64_t &aResponderId,
                                 const TopicIdentifier &aTopicId,
                                 ReferencedTLVData &aDataList,
                                 uint16_t aTxnId)
    {
        WEAVE_ERROR err = WEAVE_NO_ERROR;

        printf("processing: <subscribe confirm - success status + data list>\n");

        // and install the data

        err = mDatabase.Store(aDataList);
        SuccessOrExit(err);

        gClientCounter++;

        if (gClientCounter == gClientCount)
            Done = true;

    exit:
        if (err != WEAVE_NO_ERROR)
            printf("<subscribe confirm> error: %s\n", ErrorStr(err));

        return err;
    }

    WEAVE_ERROR UnsubscribeIndication(const uint64_t &aPublisherId, const TopicIdentifier &aTopicId, StatusReport &aReport)
    {
    printf("processing: <unsubscribe indication 0x%" PRIx64 ", 0x%" PRIx64 ">\n", aPublisherId, aTopicId);

    return WEAVE_NO_ERROR;
    }

    WEAVE_ERROR CancelSubscriptionConfirm(const uint64_t &aResponderId, const TopicIdentifier &aTopicId, StatusReport &aStatus, uint16_t aTxnId)
    {
        WEAVE_ERROR err = WEAVE_NO_ERROR;

        printf("processing: <cancel subscription confirm>\n");

        if (aStatus.success())
            printf("status == success\n");

        else
            printf("non-success status: <%x, %x>\n", aStatus.mProfileId, aStatus.mStatusCode);

        gClientCounter++;

        if (gClientCounter == gClientCount)
            Done = true;

        return err;
    }

    WEAVE_ERROR NotifyIndication(const TopicIdentifier &aTopicId, ReferencedTLVData &aDataList)
    {
        WEAVE_ERROR err = WEAVE_NO_ERROR;

        printf("processing: <notify indication>\n");

        err = mDatabase.Store(aDataList);

        gClientCounter++;

        if (gClientCounter == gClientCount)
            Done = true;

        return err;
    }

#endif // WEAVE_CONFIG_WDM_ALLOW_CLIENT_SUBSCRIPTION

    /*
     * the client comes with a profile database
     */

    TestProfileDB mDatabase;
};

/*
 * and we need a test rig, which is basically a place to hang tests, all
 * of which are run from the client side.
 */

class DMClientTester
{
public:
    DMClientTester(uint8_t aTransport)
    {
        mTransport = aTransport;
    }

    /*
     * test cases
     */

    void Test_InitialState()
    {
        DMTestClient client;

        EnterTest("InitialState", err);

        assert(client.mDatabase.mTestData.mVersion == 0);
        assert(client.mDatabase.mTestData.mIntegerItem == 0);

        ExitTest(err);
    }

    void Test_DBStore()
    {
        DMTestClient client;

        EnterTest("DBStore", err);

        uint8_t buf[kTestBufferSize];
        TLVWriter writer;
        ReferencedTLVData dataList;

        // write a data list to store (including new version)

        writer.Init(buf, kTestBufferSize);

        err = StartDataList(writer);
        SuccessOrExit(err);

        err = StartDataListElement(writer);
        SuccessOrExit(err);

        err = EncodePath(writer,
                         ContextTag(kTag_WDMDataListElementPath),
                         kWeaveProfile_Test,
                         kInstanceIdNotSpecified,
                         1,
                         ContextTag(kTag_IntegerItem));
        SuccessOrExit(err);

        err = writer.Put(ContextTag(kTag_WDMDataListElementVersion), (uint64_t)1); // increment version
        SuccessOrExit(err);

        err = writer.Put(ContextTag(kTag_WDMDataListElementData), 1); // write 1
        SuccessOrExit(err);

        err = EndDataListElement(writer);
        SuccessOrExit(err);

        err = EndList(writer);
        SuccessOrExit(err);

        // now store it and confirm results

        err = dataList.init(writer.GetLengthWritten(), kTestBufferSize, buf);
        SuccessOrExit(err);

        err = client.mDatabase.Store(dataList);
        SuccessOrExit(err);

        assert(client.mDatabase.mTestData.mVersion == 1); // check for 1
        assert(client.mDatabase.mTestData.mIntegerItem == 1); // check for 1

    exit:
        ExitTest(err);
    }

    void Test_DBRetrieve()
    {
        DMTestClient client;

        EnterTest("DBRetrieve", err);

        uint8_t pathBuf[kTestBufferSize];
        TLVWriter writer;
        ReferencedTLVData pathList;

        uint8_t dataBuf[kTestBufferSize];
        ReferencedTLVData dataList;

        TLVReader dataListRdr;
        TLVReader pathRdr;
        uint64_t version;

        uint32_t item;

        // write a path list to extract a data item

        writer.Init(pathBuf, kTestBufferSize);

        err = StartPathList(writer);
        SuccessOrExit(err);

        err = EncodePath(writer, AnonymousTag, kWeaveProfile_Test, kInstanceIdNotSpecified, 1, ContextTag(kTag_IntegerItem));
        SuccessOrExit(err);

        err = EndList(writer);
        SuccessOrExit(err);

        err = writer.Finalize();
        SuccessOrExit(err);

        err = pathList.init(writer.GetLengthWritten(), kTestBufferSize, pathBuf);
        SuccessOrExit(err);

        // set up the data list to receive data

        err = dataList.init(0, kTestBufferSize, dataBuf);
        SuccessOrExit(err);

        // now do the retrieve

        err = client.mDatabase.Retrieve(pathList, dataList);
        SuccessOrExit(err);

        // validate the data list

        OpenDataList(dataList, dataListRdr);
        SuccessOrExit(err);

        err = dataListRdr.Next();
        SuccessOrExit(err);

        err = OpenDataListElement(dataListRdr, pathRdr, version);
        SuccessOrExit(err);

        err = ValidatePath(pathRdr,
                           ContextTag(kTag_WDMDataListElementPath),
                           kWeaveProfile_Test,
                           kInstanceIdNotSpecified,
                           1,
                           ContextTag(kTag_IntegerItem));
        SuccessOrExit(err);

        assert(version == client.mDatabase.mTestData.mVersion); // check version

        assert(CheckWDMTag(kTag_WDMDataListElementData, dataListRdr)); // check data tag

        assert(dataListRdr.GetType() == kTLVType_UnsignedInteger); // check data type

        err = dataListRdr.Get(item);
        SuccessOrExit(err);
        assert(item == client.mDatabase.mTestData.mIntegerItem); // check data

        err = CloseDataListElement(dataListRdr);
        SuccessOrExit(err);

        err = CloseList(dataListRdr);
        SuccessOrExit(err);

    exit:
        ExitTest(err);
    }

    void Test_DBRetrieveAndStore()
    {
        DMTestClient client;

        EnterTest("DBRetrieveAndStore", err);

        uint8_t pathBuf[kTestBufferSize];
        TLVWriter writer;
        ReferencedTLVData pathList;

        uint8_t dataBuf[kTestBufferSize];
        ReferencedTLVData dataList;

        // check inital state

        assert(client.mDatabase.mTestData.mVersion == 0);
        assert(client.mDatabase.mTestData.mIntegerItem == 0);

        // write a path list to extract the whole bucket

        writer.Init(pathBuf, kTestBufferSize);

        err = StartPathList(writer);
        SuccessOrExit(err);

        err = EncodePath(writer, AnonymousTag, kWeaveProfile_Test, kInstanceIdNotSpecified, 0);
        SuccessOrExit(err);

        err = EndList(writer);
        SuccessOrExit(err);

        err = writer.Finalize();
        SuccessOrExit(err);

        err = pathList.init(writer.GetLengthWritten(), kTestBufferSize, pathBuf);
        SuccessOrExit(err);

        // set up the data list to receive data

        err = dataList.init(0, kTestBufferSize, dataBuf);
        SuccessOrExit(err);

        // now do the retrieve

        err = client.mDatabase.Retrieve(pathList, dataList);
        SuccessOrExit(err);

        /*
         * OK, so here's what we do now. we've got a data list capturing the
         * original state. change the state and then verify that we can set it
         * back by applying the data list.
         */

        client.mDatabase.mTestData.mVersion = 1;
        client.mDatabase.mTestData.mIntegerItem = 1;

        err = client.mDatabase.Store(dataList);
        SuccessOrExit(err);

        assert(client.mDatabase.mTestData.mVersion == 0);
        assert(client.mDatabase.mTestData.mIntegerItem == 0);

    exit:
        ExitTest(err);
    }

    void Test_View(void)
    {
        DMTestClient client;

        uint8_t pathBuf[kTestBufferSize];

        TLVWriter writer;
        ReferencedTLVData pathList;

        EnterTest("DBView", err);

        // set up the client

        err = client.Init(&ExchangeMgr);
        SuccessOrExit(err);

        err = client.BindRequest(DestNodeId, mTransport);
        SuccessOrExit(err);

        // now, make a path list

        writer.Init(pathBuf, kTestBufferSize);

        err = StartPathList(writer);
        SuccessOrExit(err);

        // first path
        err = EncodePath(writer, AnonymousTag, kWeaveProfile_Test, kInstanceIdNotSpecified, 0);
        SuccessOrExit(err);

        // second path
        err = EncodePath(writer, AnonymousTag, kWeaveProfile_Test, kInstanceIdNotSpecified, 0);
        SuccessOrExit(err);

        err = EndList(writer);
        SuccessOrExit(err);

        err = writer.Finalize();
        SuccessOrExit(err);

        err = pathList.init(writer.GetLengthWritten(), kTestBufferSize, pathBuf);
        SuccessOrExit(err);

        /*
         * do this a bunch of times to test resource management
         */

        for (int i = 0; i < gCyclingCnt; i++)
        {
            err = client.ViewRequest(pathList, 1, kDefaultDMResponseTimeout);
            SuccessOrExit(err);

            // now drive
            Done = false;
            DriveTest(Done);
        }

        err = client.ViewRequest(DestNodeId, pathList, 1, kDefaultDMResponseTimeout);
        SuccessOrExit(err);

        // now drive
        Done = false;
        DriveTest(Done);

    exit:
        ExitTest(err);
    }

    void Test_Update(void)
    {
        DMTestClient client;

        uint8_t pathBuf[kTestBufferSize];
        TLVWriter writer;
        ReferencedTLVData pathList;

        uint8_t dataBuf[kTestBufferSize];
        ReferencedTLVData dataList;

        EnterTest("DBUpdate", err);

        // set up the client

        err = client.Init(&ExchangeMgr);
        SuccessOrExit(err);

        err = client.BindRequest(DestNodeId, mTransport);
        SuccessOrExit(err);

        // now, make a path list

        writer.Init(pathBuf, kTestBufferSize);

        err = StartPathList(writer);
        SuccessOrExit(err);

        // first path
        err = EncodePath(writer, AnonymousTag, kWeaveProfile_Test, kInstanceIdNotSpecified, 1, ContextTag(kTag_IntegerItem));
        SuccessOrExit(err);

        // second path
        err = EncodePath(writer, AnonymousTag, kWeaveProfile_Test, kInstanceIdNotSpecified, 1, ContextTag(kTag_IntegerItem));
        SuccessOrExit(err);

        err = EndList(writer);
        SuccessOrExit(err);

        err = writer.Finalize();
        SuccessOrExit(err);

        err = pathList.init(writer.GetLengthWritten(), kTestBufferSize, pathBuf);
        SuccessOrExit(err);

        for (uint16_t i = 0; i < gCyclingCnt; i++)
        {
            uint64_t currentVersion = client.mDatabase.mTestData.mVersion;
            client.mDatabase.mTestData.mIntegerItem = i;

            // use path list to extract a data list

            err = dataList.init(0, kTestBufferSize, dataBuf);
            SuccessOrExit(err);

            err = client.mDatabase.Retrieve(pathList, dataList);
            SuccessOrExit(err);

            // send an update

#if WEAVE_CONFIG_WDM_ALLOW_CLIENT_LEGACY_MESSAGE_TYPES

            // it we're allowing old message types, send some

            if (i % 3 == 0)
            {
                err = client.UpdateRequest(dataList, 1, kDefaultDMResponseTimeout, true);
                SuccessOrExit(err);
            }
            else if (i % 3 == 1)
            {
                err = client.UpdateRequest(dataList, 1, kDefaultDMResponseTimeout);
                SuccessOrExit(err);
            }
            else if (i % 3 == 2)
            {
                err = client.UpdateRequest(DestNodeId, dataList, 1, kDefaultDMResponseTimeout);
                SuccessOrExit(err);
            }

#else

            err = client.UpdateRequest(dataList, 1, kDefaultDMResponseTimeout);
            SuccessOrExit(err);

#endif // WEAVE_CONFIG_WDM_ALLOW_CLIENT_LEGACY_MESSAGE_TYPES

            // now drive

            Done = false;
            DriveTest(Done);

            // OK, now send a view

            err = client.ViewRequest(pathList, 1, kDefaultDMResponseTimeout);
            SuccessOrExit(err);

            // and again...

            Done = false;
            DriveTest(Done);

            // now check the results

            assert(client.mDatabase.mTestData.mIntegerItem == i);
            assert(client.mDatabase.mTestData.mVersion > currentVersion);
        }

    exit:
        ExitTest(err);
    }

#if WEAVE_CONFIG_WDM_ALLOW_CLIENT_SUBSCRIPTION

    void Test_CancelNonSubscription(void)
    {
        DMTestClient client;

        gClientCount = 1;

        EnterTest("CancelNonSubscription", err);

        // set up the client with subscription enabled

        err = client.Init(&ExchangeMgr);
        SuccessOrExit(err);

        err = client.BindRequest(DestNodeId, mTransport);
        SuccessOrExit(err);

        for (uint16_t i = 0; i < gCyclingCnt; i++)
        {
            /*
             * insert the subscription so there'll be something there.
             */

            err = DMClient::sNotifier.InstallSubscription(kTopicIdNotSpecified, kTestTopic, DestNodeId, &client);
            SuccessOrExit(err);

            // now cancel

            err = client.CancelSubscriptionRequest(kTestTopic, 1, kDefaultDMResponseTimeout);
            SuccessOrExit(err);

            // drive

            Done = false;
            gClientCounter = 0;
            DriveTest(Done);

            assert(!client.HasSubscription(kTestTopic));
        }

    exit:
        ExitTest(err);
    }

    void Test_SubscribeToTopic(void)
    {
        DMTestClient client;

        uint8_t pathBuf[kTestBufferSize];
        TLVWriter writer;
        ReferencedTLVData pathList;

        gClientCount = 1;

        EnterTest("SubscribeToTopic", err);

        // set up the client with subscription enabled

        err = client.Init(&ExchangeMgr);
        SuccessOrExit(err);

        err = client.BindRequest(DestNodeId, mTransport);
        SuccessOrExit(err);

        writer.Init(pathBuf, kTestBufferSize);

        err = StartPathList(writer);
        SuccessOrExit(err);

        // first path
        err = EncodePath(writer, AnonymousTag, kWeaveProfile_Test, kInstanceIdNotSpecified, 1, ContextTag(kTag_IntegerItem));
        SuccessOrExit(err);

        err = EndList(writer);
        SuccessOrExit(err);

        err = writer.Finalize();
        SuccessOrExit(err);

        err = pathList.init(writer.GetLengthWritten(), kTestBufferSize, pathBuf);
        SuccessOrExit(err);

        err = client.SubscribeRequest(DestNodeId, pathList, 1, kDefaultDMResponseTimeout);
        SuccessOrExit(err);

        err = client.CancelSubscriptionRequest(kTestTopic, 1, kDefaultDMResponseTimeout);
        SuccessOrExit(err);

        Done = false;
        gClientCounter = 0;
        DriveTest(Done);

        assert(!client.HasSubscription(kTestTopic));

        err = client.SubscribeRequest(pathList, 1, kDefaultDMResponseTimeout);
        SuccessOrExit(err);

        err = client.CancelSubscriptionRequest(kTestTopic, 1, kDefaultDMResponseTimeout);
        SuccessOrExit(err);

        Done = false;
        gClientCounter = 0;
        DriveTest(Done);

        assert(!client.HasSubscription(kTestTopic));

        err = client.SubscribeRequest(DestNodeId, kTestTopic, 1, kDefaultDMResponseTimeout);
        SuccessOrExit(err);

        Done = false;
        gClientCounter = 0;
        DriveTest(Done);

        assert(client.HasSubscription(kTestTopic));

        err = client.CancelSubscriptionRequest(kTestTopic, 1, kDefaultDMResponseTimeout);
        SuccessOrExit(err);

        Done = false;
        gClientCounter = 0;
        DriveTest(Done);

        assert(!client.HasSubscription(kTestTopic));

        err = client.CancelTransactionRequest(1, err);
        SuccessOrExit(err);


        for (uint16_t i = 0; i < gCyclingCnt; i++)
        {
            err = client.SubscribeRequest(kTestTopic, 1, kDefaultDMResponseTimeout);
            SuccessOrExit(err);

            // now drive

            Done = false;
            gClientCounter = 0;
            DriveTest(Done);

            assert(client.HasSubscription(kTestTopic));

            // now wait for a notification

            Done = false;
            gClientCounter = 0;
            DriveTest(Done);

            // now cancel

            err = client.CancelSubscriptionRequest(kTestTopic, 1, kDefaultDMResponseTimeout);
            SuccessOrExit(err);

            // drive s'more

            Done = false;
            gClientCounter = 0;
            DriveTest(Done);

            assert(!client.HasSubscription(kTestTopic));

        }

    exit:
        ExitTest(err);
    }

    void Test_MultipleClients(void)
    {
        DMTestClient client1;
        DMTestClient client2;

        gClientCount = 2;

        EnterTest("MultipleClients", err);

        // set up the client with subscription enabled

        err = client1.Init(&ExchangeMgr);
        SuccessOrExit(err);

        err = client2.Init(&ExchangeMgr);
        SuccessOrExit(err);

        err = client1.BindRequest(DestNodeId, mTransport);
        SuccessOrExit(err);
        err = client2.BindRequest(DestNodeId, mTransport);
        SuccessOrExit(err);

        for (uint16_t i = 0; i < gCyclingCnt; i++)
        {
            err = client1.SubscribeRequest(kTestTopic, 1, kDefaultDMResponseTimeout);
            SuccessOrExit(err);

            Done = false;
            gClientCounter = 0;
            DriveTest(Done);

            err = client2.SubscribeRequest(kTestTopic, 1, kDefaultDMResponseTimeout);
            SuccessOrExit(err);

            // now drive

            Done = false;
            gClientCounter = 0;
            DriveTest(Done);

            assert(client1.HasSubscription(kTestTopic));
            assert(client2.HasSubscription(kTestTopic));

            // now wait for a notification

            Done = false;
            gClientCounter = 0;
            DriveTest(Done);

            // now cancel

            err = client1.CancelSubscriptionRequest(kTestTopic, 1, kDefaultDMResponseTimeout);
            SuccessOrExit(err);

            err = client2.CancelSubscriptionRequest(kTestTopic, 1, kDefaultDMResponseTimeout);
            SuccessOrExit(err);

            // drive s'more

            Done = false;
            gClientCounter = 0;
            DriveTest(Done);

            assert(!client1.HasSubscription(kTestTopic));
            assert(!client2.HasSubscription(kTestTopic));
        }

    exit:
        ExitTest(err);
    }

#endif // WEAVE_CONFIG_WDM_ALLOW_CLIENT_SUBSCRIPTION

    void Test_CloseConnection(void)
    {
        DMTestClient client;

        uint8_t pathBuf[kTestBufferSize];
        TLVWriter writer;
        ReferencedTLVData pathList;

        EnterTest("CloseConnection", err);

        // set up the client

        err = client.Init(&ExchangeMgr);
        SuccessOrExit(err);

        err = client.BindRequest(DestNodeId, kTransport_TCP);
        SuccessOrExit(err);

        // now, make a "special" path list

        writer.Init(pathBuf, kTestBufferSize);

        err = StartPathList(writer);
        SuccessOrExit(err);

        err = EncodePath(writer, AnonymousTag, kWeaveProfile_Fail, kFailureInstance_CloseConnection, 0);
        SuccessOrExit(err);

        err = EndList(writer);
        SuccessOrExit(err);

        err = writer.Finalize();
        SuccessOrExit(err);

        err = pathList.init(writer.GetLengthWritten(), kTestBufferSize, pathBuf);
        SuccessOrExit(err);

        /*
         * do this a bunch of times to test resource management
         */

        for (int i = 0; i < gCyclingCnt; i++)
        {
            err = client.ViewRequest(pathList, 1, kDefaultDMResponseTimeout);
            SuccessOrExit(err);

            // now drive
            Done = false;
            DriveTest(Done);
        }

    exit:
        ExitTest(err);
    }

    void Test_NoResponse(void)
    {
        DMTestClient client;

        uint8_t pathBuf[kTestBufferSize];
        TLVWriter writer;
        ReferencedTLVData pathList;

        EnterTest("NoResponse", err);

        // set up the client

        err = client.Init(&ExchangeMgr);
        SuccessOrExit(err);

        err = client.BindRequest(DestNodeId, mTransport);
        SuccessOrExit(err);

        // now, make a "special" path list

        writer.Init(pathBuf, kTestBufferSize);

        err = StartPathList(writer);
        SuccessOrExit(err);

        err = EncodePath(writer, AnonymousTag, kWeaveProfile_Fail, kFailureInstance_NoResponse, 0);
        SuccessOrExit(err);

        err = EndList(writer);
        SuccessOrExit(err);

        err = writer.Finalize();
        SuccessOrExit(err);

        err = pathList.init(writer.GetLengthWritten(), kTestBufferSize, pathBuf);
        SuccessOrExit(err);

        /*
         * do this a bunch of times to test resource management
         */

        for (int i = 0; i < gCyclingCnt; i++)
        {
            err = client.ViewRequest(pathList, 1, kDefaultDMResponseTimeout);
            SuccessOrExit(err);

            // now drive
            Done = false;
            DriveTest(Done);
        }

    exit:
        ExitTest(err);
    }

    static void Run(void)
    {
        printf("Running WDM client tests---\n");

        /*
         * check the profile database operaiton
         */

        {
            DMClientTester tester(kTransport_UDP);

            tester.Test_InitialState();
        }

        {
            DMClientTester tester(kTransport_UDP);

            tester.Test_DBStore();
        }

        {
            DMClientTester tester(kTransport_UDP);

            tester.Test_DBRetrieve();
        }

        {
            DMClientTester tester(kTransport_UDP);

            tester.Test_DBRetrieveAndStore();
        }

        /*
         * then the client notifier stuff
         */

        /*
         * and the server subscription table
         */

        /*
         * OK, now test the stuff that actually requires communication
         */

        // test view

        {
            DMClientTester tester(kTransport_UDP);

            tester.Test_View();
        }
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
        {
            DMClientTester tester(kTransport_WRMP);

            tester.Test_View();
        }
#endif
        // and using TCP

        {
            DMClientTester tester(kTransport_TCP);

            tester.Test_View();
        }

        // test update

        {
            DMClientTester tester(kTransport_UDP);

            tester.Test_Update();
        }
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
        {
            DMClientTester tester(kTransport_WRMP);

            tester.Test_Update();
        }
#endif
        {
            DMClientTester tester(kTransport_TCP);

            tester.Test_Update();
        }

#if WEAVE_CONFIG_WDM_ALLOW_CLIENT_SUBSCRIPTION

        // cancel a non-existent subscription

        {
            DMClientTester tester(kTransport_UDP);

            tester.Test_CancelNonSubscription();
        }
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
        {
            DMClientTester tester(kTransport_WRMP);

            tester.Test_CancelNonSubscription();
        }
#endif
        {
            DMClientTester tester(kTransport_TCP);

            tester.Test_CancelNonSubscription();
        }

        // now try establishing a topic subscription

        {
            DMClientTester tester(kTransport_UDP);

            tester.Test_SubscribeToTopic();
        }

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
        {
            DMClientTester tester(kTransport_WRMP);

            tester.Test_SubscribeToTopic();
        }
#endif
        {
            DMClientTester tester(kTransport_TCP);

            tester.Test_SubscribeToTopic();
        }

        // do the same with multiple clients

        {
            DMClientTester tester(kTransport_UDP);

            tester.Test_MultipleClients();
        }
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
        {
            DMClientTester tester(kTransport_WRMP);

            tester.Test_MultipleClients();
        }
#endif
        {
            DMClientTester tester(kTransport_TCP);

            tester.Test_MultipleClients();
        }

#endif // WEAVE_CONFIG_WDM_ALLOW_CLIENT_SUBSCRIPTION

        // these are negative tests

        {
            DMClientTester tester(kTransport_TCP);

            tester.Test_CloseConnection();
        }

        {
            DMClientTester tester(kTransport_UDP);

            tester.Test_NoResponse();
        }

        {
            DMClientTester tester(kTransport_TCP);

            tester.Test_NoResponse();
        }
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
        {
            DMClientTester tester(kTransport_WRMP);

            tester.Test_NoResponse();
        }
#endif
    }

    // we need to keep the transport around since the DME one is protected

    uint8_t mTransport;
};

int main(int argc, char *argv[])
{
    gWeaveNodeOptions.FabricId = 0;
    gWeaveNodeOptions.LocalNodeId = 0;

    if (argc == 1)
    {
        gHelpOptions.PrintBriefUsage(stderr);
        exit(EXIT_FAILURE);
    }

    if (!ParseArgsFromEnvVar(TOOL_NAME, TOOL_OPTIONS_ENV_VAR_NAME, gToolOptionSets, NULL, true) ||
        !ParseArgs(TOOL_NAME, argc, argv, gToolOptionSets, HandleNonOptionArgs))
    {
        exit(EXIT_FAILURE);
    }

    if (gNetworkOptions.LocalIPv6Addr != IPAddress::Any)
    {
        if (!gNetworkOptions.LocalIPv6Addr.IsIPv6ULA())
        {
            printf("ERROR: Local address must be an IPv6 ULA\n");
            exit(EXIT_FAILURE);
        }

        gWeaveNodeOptions.FabricId = gNetworkOptions.LocalIPv6Addr.GlobalId();
        gWeaveNodeOptions.LocalNodeId = IPv6InterfaceIdToWeaveNodeId(gNetworkOptions.LocalIPv6Addr.InterfaceId());
        gWeaveNodeOptions.SubnetId = gNetworkOptions.LocalIPv6Addr.Subnet();
    }

    // Default LocalNodeId to 1 if not set explicitly, or by means of setting the node address.
    if (gWeaveNodeOptions.LocalNodeId == 0)
        gWeaveNodeOptions.LocalNodeId = 1;

    InitSystemLayer();

    InitNetwork();

    InitWeaveStack(false, true);

    // Arrange to get called for various activity in the message layer.
    MessageLayer.OnConnectionReceived = HandleConnectionReceived;
    MessageLayer.OnReceiveError = HandleMessageReceiveError;
    MessageLayer.OnAcceptError = HandleAcceptConnectionError;

    SecurityMgr.OnSessionEstablished = HandleSecureSessionEstablished;
    SecurityMgr.OnSessionError = HandleSecureSessionError;

    if (DestAddr == IPAddress::Any)
        DestAddr = FabricState.SelectNodeAddress(DestNodeId);

    PrintNodeConfig();

    DMClientTester::Run();

    printf("WDM Test is Completed!\n");

    ShutdownWeaveStack();
    ShutdownNetwork();
    ShutdownSystemLayer();

    return EXIT_SUCCESS;
}

bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg)
{
    switch (id)
    {
    case 'D':
        if (!ParseIPAddress(arg, DestAddr))
        {
            PrintArgError("%s: Invalid value specified for destination IP address: %s\n", progName, arg);
            return false;
        }
        break;
    case 'c':
        if (!ParseInt(arg, gCyclingCnt))
        {
            PrintArgError("%s: Invalid value specified for cycling count: %s\n", progName, arg);
            return false;
        }
        break;
    default:
        PrintArgError("%s: INTERNAL ERROR: Unhandled option: %s\n", progName, name);
        return false;
    }

    return true;
}

bool HandleNonOptionArgs(const char *progName, int argc, char *argv[])
{
    if (argc > 0)
    {
        if (argc > 1)
        {
            PrintArgError("%s: Unexpected argument: %s\n", progName, argv[1]);
            return false;
        }

        if (!ParseNodeId(argv[0], DestNodeId))
        {
            PrintArgError("%s: Invalid value specified for destination node-id: %s\n", progName, argv[0]);
            return false;
        }
    }

    return true;
}

void HandleConnectionReceived(WeaveMessageLayer *msgLayer, WeaveConnection *con)
{
    char ipAddrStr[64];
    con->PeerAddr.ToString(ipAddrStr, sizeof(ipAddrStr));

    printf("Connection received from node 0x%" PRIx64 " (%s)\n", con->PeerNodeId, ipAddrStr);

    con->OnConnectionClosed = HandleConnectionClosed;
}

void HandleSecureSessionEstablished(WeaveSecurityManager *sm, WeaveConnection *con, void *reqState, uint16_t sessionKeyId, uint64_t peerNodeId, uint8_t encType)
{
    char ipAddrStr[64] = "";

    if (con != NULL)
        con->PeerAddr.ToString(ipAddrStr, sizeof(ipAddrStr));

    printf("Secure session established with node 0x%" PRIx64 " (%s)\n", peerNodeId, ipAddrStr);
}

void HandleSecureSessionError(WeaveSecurityManager *sm, WeaveConnection *con, void *reqState, WEAVE_ERROR localErr, uint64_t peerNodeId, StatusReport *statusReport)
{
    char ipAddrStr[64] = "";

    if (con != NULL)
    {
        con->PeerAddr.ToString(ipAddrStr, sizeof(ipAddrStr));
        con->Close();
    }

    if (localErr == WEAVE_ERROR_STATUS_REPORT_RECEIVED && statusReport != NULL)
        printf("FAILED to establish secure session with node 0x%" PRIx64 " (%s): %s\n", peerNodeId, ipAddrStr, nl::StatusReportStr(statusReport->mProfileId, statusReport->mStatusCode));
    else
        printf("FAILED to establish secure session with node 0x%" PRIx64 " (%s): %s\n", peerNodeId, ipAddrStr, ErrorStr(localErr));
}

void HandleConnectionClosed(WeaveConnection *con, WEAVE_ERROR conErr)
{
    char ipAddrStr[64];
    // Note: by contract con MUST NOT be NULL, so it is used without the check.
    con->PeerAddr.ToString(ipAddrStr, sizeof(ipAddrStr));

    if (conErr == WEAVE_NO_ERROR)
        printf("Connection closed to node 0x%" PRIx64 " (%s)\n", con->PeerNodeId, ipAddrStr);
    else
        printf("Connection ABORTED to node 0x%" PRIx64 " (%s): %s\n", con->PeerNodeId, ipAddrStr, ErrorStr(conErr));

    con->Close();
}

/*
 *
 * we may have need of this some day but for now we're not using it and it's
 * generating warnings to that effect.
 *
void HandleConnectionComplete(WeaveConnection *con, WEAVE_ERROR err)
{
    char ipAddrStr[64];
    con->PeerAddr.ToString(ipAddrStr, sizeof(ipAddrStr));

    if (err == WEAVE_NO_ERROR)
    {
        printf("Connection established to %s\n", ipAddrStr);

        ExchangeMgr.AllowUnsolicitedMessages(con);
    }
    else
    {
        printf("Failed to establish connection to %s: %s\n", ipAddrStr, ErrorStr(err));
        con->Close();
    }
}
*
*/
