/*
 *
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
 *      This file implements a derived unsolicited responder
 *      (i.e., server) for the Weave Software Update (SWU) profile used
 *      for the Weave mock device command line functional testing
 *      tool.
 *
 */

#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Support/CodeUtils.h>
#include <Weave/Core/WeaveEncoding.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include "MockSWUServer.h"
#include <Weave/Profiles/common/CommonProfile.h>


MockSoftwareUpdateServer::MockSoftwareUpdateServer()
{
    FabricState = NULL;
    ExchangeMgr = NULL;
    mCurServerOp = NULL;
    mRefImageQuery = NULL;
    mFileDesignator = NULL;
    mCurServerOpBuf = NULL;
}

WEAVE_ERROR MockSoftwareUpdateServer::Init(WeaveExchangeManager *exchangeMgr)
{
    FabricState = exchangeMgr->FabricState;
    ExchangeMgr = exchangeMgr;
    mCurServerOp = NULL;
    mCurServerOpBuf = NULL;
    mFileDesignator = NULL;

    // Register to receive unsolicited Service Provisioning messages from the exchange manager.
    WEAVE_ERROR err =
        ExchangeMgr->RegisterUnsolicitedMessageHandler(kWeaveProfile_SWU, HandleClientRequest, this);

    return err;
}

WEAVE_ERROR MockSoftwareUpdateServer::Shutdown()
{
    if (ExchangeMgr != NULL)
        ExchangeMgr->UnregisterUnsolicitedMessageHandler(kWeaveProfile_SWU);

    if (mCurServerOp != NULL)
    {
        mCurServerOp->Close();
        mCurServerOp = NULL;
    }

    if (mCurServerOpBuf != NULL)
    {
        PacketBuffer::Free(mCurServerOpBuf);
        mCurServerOpBuf = NULL;
    }

    FabricState = NULL;
    ExchangeMgr = NULL;
    mCurServerOp = NULL;

    return WEAVE_NO_ERROR;
}

void MockSoftwareUpdateServer::HandleClientRequest(ExchangeContext *ec, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo,
                                                    uint32_t profileId, uint8_t msgType, PacketBuffer *payload)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    MockSoftwareUpdateServer *server = (MockSoftwareUpdateServer *) ec->AppState;

    // Fail messages for the wrong profile. This shouldn't happen, but better safe than sorry.
    if (profileId != kWeaveProfile_SWU)
    {
        server->SendStatusReport(kWeaveProfile_Common, Common::kStatus_BadRequest, WEAVE_NO_ERROR);
        ec->Close();
        goto exit;
    }

    // Disallow simultaneous requests.
    if (server->mCurServerOp != NULL)
    {
        server->SendStatusReport(kWeaveProfile_Common, Common::kStatus_Busy, WEAVE_NO_ERROR);
        ec->Close();
        goto exit;
    }

    // Record that we have a request in process.
    server->mCurServerOp = ec;

    // Decode and dispatch the message.
    switch (msgType)
    {
        case kMsgType_ImageQuery:
            err = HandleImageQuery(ec, pktInfo, msgInfo, profileId, msgType, payload);
            SuccessOrExit(err);
            break;

        default:
            server->SendStatusReport(kWeaveProfile_Common, Common::kStatus_BadRequest);
            break;
    }

exit:

    if (payload != NULL)
        PacketBuffer::Free(payload);
}

WEAVE_ERROR MockSoftwareUpdateServer::GenerateImageDigest(const char *imagePath, uint8_t integrityType, uint8_t *digest)
{
    //Generate Image Digest
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint8_t block[512];
    FILE *file;
    int bytesRead = 0;
    nl::Weave::Platform::Security::SHA1 sha1;
    nl::Weave::Platform::Security::SHA256 sha256;

    file = fopen(imagePath, "r");
    if (file == NULL)
    {
        printf("Unable to open %s: %s\n", imagePath, strerror(errno));
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
    }

    switch (integrityType)
    {
    case kIntegrityType_SHA160:

        sha1.Begin();
        break;
    case kIntegrityType_SHA256:
        sha256.Begin();
        break;
    default:
        printf("Unsupported image integrity type: %u\n", integrityType);
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
        break;
    }

    while ((bytesRead = fread(block, 1, sizeof(block), file)) > 0)
    {
        if (integrityType == kIntegrityType_SHA160)
            sha1.AddData(block, bytesRead);
        else
            sha256.AddData(block, bytesRead);
    }

    if (bytesRead < 0)
    {
        printf("Error reading %s: %s\n", imagePath, strerror(errno));
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
    }

    if (integrityType == kIntegrityType_SHA160)
        sha1.Finish(digest);
    else
        sha256.Finish(digest);

exit:
    if (file != NULL)
        fclose(file);

    return err;
}

WEAVE_ERROR MockSoftwareUpdateServer::SendImageQueryResponse()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    ImageQueryResponse imageQueryResponse;
    IntegritySpec integritySpec;
    ReferencedString URI;
    uint8_t imageDigest[nl::Weave::Platform::Security::SHA256::kHashLength];
    uint8_t supported_update_scheme;
    int integrityType = -1;
    int i = 0;

    VerifyOrExit(mFileDesignator != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Use the first one
    supported_update_scheme = mRefImageQuery->updateSchemes.theList[0];

    // Scan the offered list of integrity types for SHA1 or SHA256.  Prefer SHA256 when offered.
    for (i = 0; i < mRefImageQuery->integrityTypes.theLength; i++)
    {
        switch (mRefImageQuery->integrityTypes.theList[i])
        {
        case kIntegrityType_SHA160:
            if (integrityType == -1)
                integrityType = kIntegrityType_SHA160;
            break;
        case kIntegrityType_SHA256:
            integrityType = kIntegrityType_SHA256;
            break;
        default:
            break;
        }
    }
    if (integrityType == -1)
    {
        printf("No supported integrity types in ImageQuery\n");
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
    }

    err = GenerateImageDigest(mFileDesignator, (uint8_t)integrityType, imageDigest);
    SuccessOrExit(err);

    err = integritySpec.init(mRefImageQuery->integrityTypes.theList[i], imageDigest);
    SuccessOrExit(err);

    err = URI.init((uint16_t)(strlen(mFileDesignator) + 1), (char *)mFileDesignator);
    SuccessOrExit(err);

    err = imageQueryResponse.init(URI, mRefImageQuery->version, integritySpec,
                                  supported_update_scheme, Normal, Unconditionally, false);
    SuccessOrExit(err);

    mCurServerOpBuf = PacketBuffer::New();
    err = imageQueryResponse.pack(mCurServerOpBuf);
    SuccessOrExit(err);

    // If we are given a URI, then we should download the image and calculate the checksum using the correct
    // integrity type otherwise use the local image provided in the argument and calc the checksum.

    err = mCurServerOp->SendMessage(kWeaveProfile_SWU, kMsgType_ImageQueryResponse, mCurServerOpBuf);
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR MockSoftwareUpdateServer::SendImageQueryStatus()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    mCurServerOpBuf = PacketBuffer::New();

    StatusReport statusReport;
    statusReport.init(kWeaveProfile_SWU, kStatus_NoUpdateAvailable);

    err = statusReport.pack(mCurServerOpBuf);
    SuccessOrExit(err);

    err = mCurServerOp->SendMessage(kWeaveProfile_Common, Common::kMsgType_StatusReport, mCurServerOpBuf);
    SuccessOrExit(err);

exit:
    return err;

}

WEAVE_ERROR MockSoftwareUpdateServer::HandleImageQuery(ExchangeContext *ec, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo,
                                                    uint32_t profileId, uint8_t msgType, PacketBuffer *payload)
{
    ImageQuery ParsedImageQueryRequest;
    bool update_available = false;
    bool common_integrity_scheme_found = false;
    bool common_update_scheme_found = false;
    int index = 0;

    MockSoftwareUpdateServer *server = (MockSoftwareUpdateServer *) ec->AppState;

    //Parse image query ans print values
    WEAVE_ERROR err = ImageQuery::parse(payload, ParsedImageQueryRequest);
    if (WEAVE_NO_ERROR == err) {
        printf("\nReceievd Image Query Request\n");
        printf("    Vendor Id: %d\n", ParsedImageQueryRequest.productSpec.vendorId);
        printf("    Product Id: %d\n", ParsedImageQueryRequest.productSpec.productId);
        printf("    Product Rev: %d\n", ParsedImageQueryRequest.productSpec.productRev);
        printf("    Software version: %s\n", ParsedImageQueryRequest.version.printString());
        printf("    Integrity Type: %d\n", ParsedImageQueryRequest.integrityTypes.theList[0]);
        printf("    Update Scheme: %d\n", ParsedImageQueryRequest.updateSchemes.theList[0]);
        printf("\n");
    }

    // Condition to send a image query response:
    // 1. Product Spec matches
    // 2. If the server supports any of the client's intergrity schemes
    // 3. If the server supports any of the client's update schemes

    // Condition to send an Status Report with No Image Available
    // 1-2 from Image Query Response
    // 5. Client's and server version's are the same

    // Any other issues with Image Query such as an invalid field or entry:
    // Send down a status report with Error Message

    if ((ParsedImageQueryRequest.productSpec) == (server->mRefImageQuery->productSpec))
    {
        update_available = true;
    }
    VerifyOrExit(update_available, printf("Product Specs do not match\n"));

    for (index = 0; index < server->mRefImageQuery->integrityTypes.theLength; index++)
    {
        if (ParsedImageQueryRequest.integrityTypes.theList[0] == server->mRefImageQuery->integrityTypes.theList[index])
        {
            printf("Using integrity scheme :%d\n", ParsedImageQueryRequest.integrityTypes.theList[0]);
            common_integrity_scheme_found = true;
            break;
        }
    }

    if (!common_integrity_scheme_found)
    {
        update_available = false;
        VerifyOrExit(update_available, printf("Integrity Scheme requested by the client is not supported\n"));
    }

    for (index = 0; index < server->mRefImageQuery->updateSchemes.theLength; index++)
    {
        if (ParsedImageQueryRequest.updateSchemes.theList[0] == server->mRefImageQuery->updateSchemes.theList[index])
        {
            printf("Using Update Scheme :%d\n", ParsedImageQueryRequest.updateSchemes.theList[0]);
            common_update_scheme_found = true;
            break;
        }
    }

    if (!common_update_scheme_found)
    {
        update_available = false;
        VerifyOrExit(update_available, printf("Update Scheme requested by the client is not supported\n"));
    }

    if (ParsedImageQueryRequest.version == server->mRefImageQuery->version)
    {
        // Send down an image query status
        update_available = false;
    }
    else
    {
        // Send Down an Image Query response
        update_available = true;
    }

exit:
    if (update_available && common_update_scheme_found && common_integrity_scheme_found)
    {
        server->SendImageQueryResponse();
    }
    else
    {
        server->SendImageQueryStatus();
    }

    // Cleanup
    server->mCurServerOp = NULL;

    return WEAVE_NO_ERROR;
}

WEAVE_ERROR MockSoftwareUpdateServer::SendStatusReport(uint32_t statusProfileId, uint16_t statusCode, WEAVE_ERROR sysError)
{
    WEAVE_ERROR err;

    VerifyOrExit(mCurServerOp != NULL, err = WEAVE_ERROR_INCORRECT_STATE);

    printf("Sending StatusReport to the client -> Profile : %d   StatusCode : %d\n", statusProfileId, statusCode);
    err = WeaveServerBase::SendStatusReport(mCurServerOp, statusProfileId, statusCode, sysError);

exit:

    if (mCurServerOp != NULL)
    {
        mCurServerOp->Close();
        mCurServerOp = NULL;
    }

    if (mCurServerOpBuf != NULL)
    {
        PacketBuffer::Free(mCurServerOpBuf);
        mCurServerOpBuf = NULL;
    }

    return err;
}

void MockSoftwareUpdateServer::SetReferenceImageQuery(ImageQuery *aRefImageQuery)
{
    mRefImageQuery = aRefImageQuery;

#if MOCK_SWU_SERVER_DEBUG
    printf("\nUsing the following configuration:\n");
    printf("  Vendor Id: %d\n", mRefImageQuery->productSpec.vendorId);
    printf("  Product Id: %d\n", mRefImageQuery->productSpec.productId);
    printf("  Product Rev: %d\n", mRefImageQuery->productSpec.productRev);
    printf("  Software version: %s\n", mRefImageQuery->version.printString());
    printf("  Integrity Type[s]: ");
    for (int i = 0 ; i < mRefImageQuery->integrityTypes.theLength; i++)
        {   printf("%d  ", mRefImageQuery->integrityTypes.theList[i]);  }
    printf("\n");
    printf("  Update Scheme[s]: ");
    for (int i = 0 ; i < mRefImageQuery->updateSchemes.theLength; i++)
        {   printf("%d  ", mRefImageQuery->updateSchemes.theList[i]);  }
    printf("\n");
#endif

}

WEAVE_ERROR MockSoftwareUpdateServer::SetFileDesignator(const char *aFileDesignator)
{
    FILE *file;

    if (!aFileDesignator)
    {
        printf("--file-designator not specified\n");
        exit(-1);
    }

    // Make sure we can open the image file
    file = fopen(aFileDesignator, "r");
    if (file == NULL) {
        return WEAVE_ERROR_INCORRECT_STATE;
    }
    else {
        fclose(file);
    }

    mFileDesignator = aFileDesignator;
    return WEAVE_NO_ERROR;
}

WEAVE_ERROR MockSoftwareUpdateServer::SendImageAnnounce(WeaveConnection *con)
{
    // Discard any existing exchange context.
    // Effectively we can only have one SWU exchange with a single node at any one time.
    if (mCurServerOp != NULL)
    {
        mCurServerOp->Close();
        mCurServerOp = NULL;
    }

    // Create a new exchange context.
    mCurServerOp = ExchangeMgr->NewContext(con, this);
    if (mCurServerOp == NULL)
        return WEAVE_ERROR_NO_MEMORY;

    return SendImageAnnounce();
}

WEAVE_ERROR MockSoftwareUpdateServer::SendImageAnnounce(uint64_t nodeId, IPAddress nodeAddr)
{
    return SendImageAnnounce(nodeId, nodeAddr, WEAVE_PORT);
}

WEAVE_ERROR MockSoftwareUpdateServer::SendImageAnnounce(uint64_t nodeId, IPAddress nodeAddr, uint16_t port)
{
    // Discard any existing exchange context. Effectively we can only have one SWU exchange with
    // a single node at any one time.
    if (mCurServerOp != NULL)
    {
        mCurServerOp->Close();
        mCurServerOp = NULL;
    }

    if (nodeAddr == IPAddress::Any)
        nodeAddr = FabricState->SelectNodeAddress(nodeId);

    // Create a new exchange context.
    mCurServerOp = ExchangeMgr->NewContext(nodeId, nodeAddr, this);
    if (mCurServerOp == NULL)
        return WEAVE_ERROR_NO_MEMORY;

    return SendImageAnnounce();
}

WEAVE_ERROR MockSoftwareUpdateServer::SendImageAnnounce()
{
    printf("0 SendImageAnnounce entering\n");

    PacketBuffer* lBuffer = PacketBuffer::New();

    // Send image announce message. Discard the exchange context if the send fails.
    WEAVE_ERROR err = mCurServerOp->SendMessage(kWeaveProfile_SWU, kMsgType_ImageAnnounce, lBuffer);
    lBuffer = NULL;

    mCurServerOp->Close();
    mCurServerOp = NULL;

    if (err != WEAVE_NO_ERROR)
    {
        printf(" 1 mCurServeOp->Sendmessage(Image announce) FAILED: %s\n", nl::ErrorStr(err));
        return err;
    }
    printf("2 Send Image Announce exiting\n");
    return err;
}
