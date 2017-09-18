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
 *      This file implements an unsolicited initiator (i.e., client) for
 *      the Weave Software Update (SWU) profile used for functional
 *      testing of the implementation of core message handlers for
 *      that profile.
 *
 */

#include "ToolCommon.h"
#include "nlweaveswuclient.h"

namespace nl {
namespace Weave {
namespace Profiles {

using namespace nl::Weave::Profiles::SoftwareUpdate;

SoftwareUpdateClient::SoftwareUpdateClient()
{
    FabricState = NULL;
    ExchangeMgr = NULL;
}

SoftwareUpdateClient::~SoftwareUpdateClient()
{
    FabricState = NULL;
    ExchangeMgr = NULL;
}

WEAVE_ERROR SoftwareUpdateClient::Init(WeaveExchangeManager *exchangeMgr)
{
    // Error if already initialized.
    if (ExchangeMgr != NULL)
        return WEAVE_ERROR_INCORRECT_STATE;

    ExchangeMgr = exchangeMgr;
    FabricState = exchangeMgr->FabricState;

    return WEAVE_NO_ERROR;
}

WEAVE_ERROR SoftwareUpdateClient::Shutdown()
{
    ExchangeMgr = NULL;
    FabricState = NULL;

    return WEAVE_NO_ERROR;
}

WEAVE_ERROR SoftwareUpdateClient::SendImageQueryRequest(WeaveConnection *con)
{
    // Discard any existing exchange context.
    // Effectively we can only have one SWU exchange with a single node at any one time.
    if (ExchangeCtx != NULL)
    {
        ExchangeCtx->Close();
        ExchangeCtx = NULL;
    }

    // Create a new exchange context.
    ExchangeCtx = ExchangeMgr->NewContext(con, this);
    if (ExchangeCtx == NULL)
        return WEAVE_ERROR_NO_MEMORY;

    return SendImageQueryRequest();
}

WEAVE_ERROR SoftwareUpdateClient::SendImageQueryRequest(uint64_t nodeId, IPAddress nodeAddr)
{
    return SendImageQueryRequest(nodeId, nodeAddr, WEAVE_PORT);
}

WEAVE_ERROR SoftwareUpdateClient::SendImageQueryRequest(uint64_t nodeId, IPAddress nodeAddr, uint16_t port)
{
    // Discard any existing exchange context. Effectively we can only have one SWU exchange with
    // a single node at any one time.
    if (ExchangeCtx != NULL)
    {
        ExchangeCtx->Close();
        ExchangeCtx = NULL;
    }

    if (nodeAddr == IPAddress::Any)
        nodeAddr = FabricState->SelectNodeAddress(nodeId);

    // Create a new exchange context.
    ExchangeCtx = ExchangeMgr->NewContext(nodeId, nodeAddr, this);
    if (ExchangeCtx == NULL)
        return WEAVE_ERROR_NO_MEMORY;

    return SendImageQueryRequest();
}

WEAVE_ERROR SoftwareUpdateClient::SendImageQueryRequest()
{
    printf("0 SendImageQueryRequest entering\n");

    // Configure the encryption and signature types to be used to send the request.
    ExchangeCtx->EncryptionType = EncryptionType;
    ExchangeCtx->KeyId = KeyId;

    // Arrange for messages in this exchange to go to our response handler.
    ExchangeCtx->OnMessageReceived = HandleImageQueryResponse;

    /*
     * Build the ImageQuery request
     */

    ProductSpec aProductSpec(0x235A /*vendorId*/, 1 /*productId*/, 1 /*productRev*/);

    ReferencedString aVersion;
    char aVersionStr[] = "1.0d1";
    aVersion.init((uint8_t)(strlen(aVersionStr)), aVersionStr);

    IntegrityTypeList aTypeList;
    uint8_t updateTypeList[] = { kIntegrityType_SHA256 };
    aTypeList.init(sizeof(updateTypeList), updateTypeList);

    UpdateSchemeList aSchemeList;
    uint8_t updateSchemeList[] = { kUpdateScheme_BDX };
    aSchemeList.init(sizeof(updateSchemeList), updateSchemeList);

    ImageQuery imageQuery;
    imageQuery.init(aProductSpec, aVersion, aTypeList, aSchemeList,
                    NULL /*package*/, NULL /*locale*/, 0 /*target node id*/, NULL /*metadata*/);
    PacketBuffer* imageQueryPayload = PacketBuffer::New();
    imageQuery.pack(imageQueryPayload);

    // Send an ImageQuery Request message. Discard the exchange context if the send fails.
    WEAVE_ERROR err = ExchangeCtx->SendMessage(kWeaveProfile_SWU, kMsgType_ImageQuery, imageQueryPayload);
    imageQueryPayload = NULL;
    if (err != WEAVE_NO_ERROR)
    {
        printf("  1 ExchangeCtx->Sendmessage(ImageQuery) FAILED\n");
        ExchangeCtx->Close();
        ExchangeCtx = NULL;
    }

    printf("2 SendImageQueryRequest exiting\n");
    return err;
}

//ImageQuery response received
void SoftwareUpdateClient::HandleImageQueryResponse(ExchangeContext *ec, const IPPacketInfo *packetInfo, const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *payloadImageQueryResponse)
{
    printf("0 HandleImageQueryResponse entering\n");

    SoftwareUpdateClient *swuApp = (SoftwareUpdateClient *)ec->AppState;

    // Verify that the message is an ImageQuery Response.
    if (profileId != kWeaveProfile_SWU)
    {
        printf("  1 response is NOT a valid response\n");
        // TODO: handle unexpected response
        return;
    }
    else if (profileId == kWeaveProfile_SWU && msgType == kMsgType_ImageQueryStatus)
    {
        printf("Got an Image Query Status\n");

        StatusReport statusReport;
        StatusReport::parse(payloadImageQueryResponse, statusReport);

        printf("Status Report -> Profile: 0x%X, Status: 0x%X\n", statusReport.mProfileId, statusReport.mStatusCode);

        if (statusReport.mProfileId == kWeaveProfile_SWU && statusReport.mStatusCode == kStatus_NoUpdateAvailable)
        {
            printf("No Update Available\n");
        }

        return;
    }

    // Verify that the exchange context matches our current context. Bail if not.
    if (ec != swuApp->ExchangeCtx)
    {
        printf("  2 HandleImageQueryResponse exchange doesn't match\n");
        return;
    }

    //print the contents of the ImageQuery response
    ImageQueryResponse imageQueryResponse;
    printf("err: %d\n", imageQueryResponse.parse(payloadImageQueryResponse, imageQueryResponse));
    printf("====\n");
    DumpMemory(payloadImageQueryResponse->Start(), payloadImageQueryResponse->DataLength(), "==> ", 16);
    printf("====\n");
    printf("uri.theLength: %d\n", imageQueryResponse.uri.theLength);
    printf("uri.theString: %s\n", imageQueryResponse.uri.theString);
    printf("versionSpec.theLength: %d\n", imageQueryResponse.versionSpec.theLength);
    printf("versionSpec.theString: %s\n", imageQueryResponse.versionSpec.printString());
    printf("integritySpec.type: %d\n", imageQueryResponse.integritySpec.type);
    printf("updateScheme: %d\n", imageQueryResponse.updateScheme);
    printf("updatePriority: %d\n", imageQueryResponse.updatePriority);
    printf("updateCondition: %d\n", imageQueryResponse.updateCondition);
    printf("reportStatus: %d\n", imageQueryResponse.reportStatus);
    printf("====\n");

    // Free the payload buffer.
    PacketBuffer::Free(payloadImageQueryResponse);

    // Discard the exchange context.
    swuApp->ExchangeCtx->Close();
    swuApp->ExchangeCtx = NULL;

    printf("3 HandleImageQueryResponse exiting\n");
    Done = true;
}

// Set the exchange context for the most recently started SWU exchange.
void SoftwareUpdateClient::SetExchangeCtx(ExchangeContext *ec)
{
    ExchangeCtx = ec;
}

} // namespace Profiles
} // namespace Weave
} // namespace nl
