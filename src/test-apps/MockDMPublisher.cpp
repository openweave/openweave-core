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
 *      This file implements a derived Weave Data Management (WDM)
 *      publisher for the Weave Data Management profile and protocol
 *      used for the Weave mock device command line functional testing
 *      tool.
 *
 */

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include <assert.h>

#include "MockDMPublisher.h"

using namespace ::nl::Inet;
using namespace ::nl::Weave;
using namespace ::nl::Weave::TLV;
using namespace ::nl::Weave::Profiles;
using namespace ::nl::Weave::Profiles::Common;
using namespace ::nl::Weave::Profiles::DataManagement;
using namespace ::nl::Weave::Profiles::StatusReporting;

WEAVE_ERROR MockDMPublisher::ViewIndication(ExchangeContext *aResponseCtx, ReferencedTLVData &aPathList)
{
    WEAVE_ERROR     err     = WEAVE_NO_ERROR;
    PacketBuffer*   buf     = PacketBuffer::New();

    StatusReport report;
    ReferencedTLVData dataList;

    // when a failure is requested, this is the mode;
    uint8_t mode = kFailureMode_Invalid;

    printf("processing: <view indication>\n");

    if (!buf)
    {
        err = WEAVE_ERROR_NO_MEMORY;
    }

    else
    {
        /*
         * so we've got a path list and we want to extract the relevant
         * data. just call retrieve after setting up a data list to store
         * the result in.
         */

        dataList.init(buf);
        err = mDatabase.Retrieve(aPathList, dataList);
    }

    if (err == WEAVE_NO_ERROR)
    {
        err = report.init(kWeaveProfile_Common, kStatus_Success);

        if (err == WEAVE_NO_ERROR)
            err = ViewResponse(aResponseCtx, report, &dataList);
    }

    else
    {
        /*
         * we're using view requests to test special failure cases,
         * e.g a closed connection in the middle of a transaction. the
         * cue for this is an invalid profile ID (see TestProfile.h)
         * and instance IDs that select different failure tests.
         */

        if (err == WEAVE_ERROR_INVALID_PROFILE_ID)
        {
            mode = LookupFailureMode(aPathList);

            switch (mode)
            {
                case kFailureMode_CloseConnection:

                    printf("<view indication> failure requested: closing connection\n");

                    aResponseCtx->Con->Close();

                    break;
                case kFailureMode_NoResponse:

                    printf("<view indication> failure requested: no response\n");

                    break;
                default:

                    printf("<view indication> failure requested: invalid request\n");

                    break;
            }

            err = WEAVE_NO_ERROR;
        }

        /*
         * if there's any other error here it was in the foregoing
         * retrieval of the data list. in any case, we should still be
         * able to send back status but we shouldn't include the data
         * list.
         */

        else
        {
            printf("<view indication> error: %s\n", ErrorStr(err));

            err = report.init(err);

            if (err == WEAVE_NO_ERROR)
                err = ViewResponse(aResponseCtx, report, NULL);

            if (err != WEAVE_NO_ERROR)
                printf("could not send view response: %s\n", ErrorStr(err));
        }
    }

    aResponseCtx->Close();
    PacketBuffer::Free(buf);

    return err;
}

WEAVE_ERROR MockDMPublisher::UpdateIndication(ExchangeContext *aResponseCtx, ReferencedTLVData &aDataList)
{

    WEAVE_ERROR err = WEAVE_NO_ERROR;

    StatusReport report;

    printf("processing: <update indication>\n");

    /*
     * store the data list and update the version
     */

    err = mDatabase.Store(aDataList);
    SuccessOrExit(err);

    mDatabase.mTestData.mVersion++;

exit:
    if (err == WEAVE_NO_ERROR)
    {
        err = report.init(kWeaveProfile_Common, kStatus_Success);
    }

    else
    {
        /*
         * if there's an error here it was in the foregoing storage
         * of the data list. send an internal error status.
         */

        printf("<update indication> error: %s\n", ErrorStr(err));

        err = report.init(err);
    }

    if (err == WEAVE_NO_ERROR)
        err = StatusResponse(aResponseCtx, report);

    aResponseCtx->Close();

    if (err != WEAVE_NO_ERROR)

        /*
         * if there's an error it can only be in formatting or
         * sending the response.
         */

        printf("could not send update response: %s\n", ErrorStr(err));

    return err;
}

void MockDMPublisher::IncompleteIndication(const uint64_t &aPeerNodeId, StatusReport &aReport)
{
    printf("processing: <incomplete indication>\n");
}

#if WEAVE_CONFIG_WDM_ALLOW_PUBLISHER_SUBSCRIPTION

WEAVE_ERROR MockDMPublisher::SubscribeIndication(ExchangeContext *aResponseCtx, const TopicIdentifier &aTopicId)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    uint8_t dataBuf[kTestBufferSize];
    ReferencedTLVData dataList;
    ReferencedTLVData *pDataList = NULL;

    TLVWriter writer;

    StatusReport report;

    printf("processing: <subscribe indication>\n");

    /*
     * OK. if it's a topic ID we "know" then we respond with
     * success and AND we get together a data list to return
     * otherwise, we respond with non-success status
     */

    if (aTopicId == kTestTopic)
    {
        // make a nice success-oriented status report

        err = report.init(kWeaveProfile_Common, kStatus_Success);
        SuccessOrExit(err);

        // retrieve the current contents of the database

        err = dataList.init(0, kTestBufferSize, dataBuf);
        SuccessOrExit(err);

        err = StartDataList(dataList, writer);
        SuccessOrExit(err);

        err = StartDataListElement(writer);
        SuccessOrExit(err);

        err = mDatabase.mTestData.Retrieve(writer);
        SuccessOrExit(err);

        err = EndDataListElement(writer);
        SuccessOrExit(err);

        EndList(dataList, writer);

        pDataList = &dataList;
    }

    else
    {
        err = report.init(kWeaveProfile_Common, kStatus_UnknownTopic);
        SuccessOrExit(err);
    }

exit:
    if (err == WEAVE_NO_ERROR)
    {
        err = SubscribeResponse(aResponseCtx, report, aTopicId, pDataList);
    }

    else
    {
        /*
         * if there's an error here it was in the foregoing. send an
         * internal error status.
         */

        printf("<subscribe indication> error: %s\n", ErrorStr(err));

        err = report.init(err);

        if (err == WEAVE_NO_ERROR)
            err = StatusResponse(aResponseCtx, report);
    }

    aResponseCtx->Close();

    if (err != WEAVE_NO_ERROR)

        /*
         * if there's an error it can only be in formatting or
         * sending the response.
         */

        printf("could not send subscribe response: %s\n", ErrorStr(err));

    return err;
}

WEAVE_ERROR MockDMPublisher::SubscribeIndication(ExchangeContext *aResponseCtx, ReferencedTLVData &aPathList)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    StatusReport report;

    printf("processing: <subscribe indication>\n");

    /*
     * we're not currently supporting this form of subscription so
     * just send 'em a status report saying so.
     */

    printf("<subscribe indication> error: unsupported subscription type\n");

    err = report.init(kWeaveProfile_WDM, kStatus_UnsupportedSubscriptionMode);

    if (err == WEAVE_NO_ERROR)
        err = StatusResponse(aResponseCtx, report);

    aResponseCtx->Close();

    if (err != WEAVE_NO_ERROR)

        /*
         * if there's an error it can only be in formatting or
         * sending the response.
         */

        printf("could not send subscribe response: %s\n", ErrorStr(err));

    return err;
}

WEAVE_ERROR MockDMPublisher::UnsubscribeIndication(const uint64_t &aClientId, const TopicIdentifier &aTopicId, StatusReport &aReport)
{
    printf("processing: <unsubscribe indication 0x%" PRIx64 ", 0x%" PRIx64 ">\n", aClientId, aTopicId);

    return WEAVE_NO_ERROR;
}


WEAVE_ERROR MockDMPublisher::CancelSubscriptionIndication(ExchangeContext *aResponseCtx, const TopicIdentifier &aTopicId)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    printf("processing: <cancel subscription indication>\n");

    err = DMPublisher::CancelSubscriptionIndication(aResponseCtx, aTopicId);

    if (SubscriptionTableEmpty())
        printf("--- empty subscription table ---\n");

    return err;
}

WEAVE_ERROR MockDMPublisher::NotifyConfirm(const uint64_t &aResponderId, const TopicIdentifier &aTopicId, StatusReport &aStatus, uint16_t aTxnId)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    printf("processing: <notify confirm>\n");

    err = DMPublisher::NotifyConfirm(aResponderId, aTopicId, aStatus, aTxnId);

    if (err != WEAVE_NO_ERROR)
        EndSubscription(aTopicId, aResponderId);

    return err;
}

WEAVE_ERROR MockDMPublisher::Republish(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    uint8_t pathBuf[kTestBufferSize];
    TLVWriter writer;
    ReferencedTLVData pathList;

    uint8_t dataBuf[kTestBufferSize];
    ReferencedTLVData dataList;

    if (++mRepublicationCounter > kUpdatePeriod)
    {
        mRepublicationCounter = 0;

        if (!SubscriptionTableEmpty())
        {
            printf("change profile data and notify clients\n");

            mDatabase.ChangeProfileData();

            // extract the modified data using a path list

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

            err = dataList.init(0, kTestBufferSize, dataBuf);
            SuccessOrExit(err);

            err = mDatabase.Retrieve(pathList, dataList);
            SuccessOrExit(err);

            // and request a notification

            err = NotifyRequest(kTestTopic, dataList, 1, kDefaultDMResponseTimeout);
        }
    }

exit:
    if (err != WEAVE_NO_ERROR)
    {
        mDatabase.RevertProfileData();

        printf("error in change and notify: %s\n", ErrorStr(err));
    }

    return err;
}

#endif // WEAVE_CONFIG_WDM_ALLOW_PUBLISHER_SUBSCRIPTION
