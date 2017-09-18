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
 *      This file implements an unsolicited initiator (i.e., client)
 *      for the "legacy" (see [Managed Namespaces](@ref mns)) Weave
 *      Bulk Data Transfer (BDX) profile used for functional testing
 *      of the implementation of core message handlers and protocol
 *      engine for that profile.
 *
 */

#include "ToolCommon.h"
#include "nlweavebdxclient.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

namespace nl {
namespace Weave {
namespace Profiles {


using namespace nl::Weave::Profiles::BulkDataTransfer;

BulkDataTransferClient::BulkDataTransferClient()
{
    FabricState = NULL;
    ExchangeMgr = NULL;
    mBlockCounter = 0;
}

BulkDataTransferClient::~BulkDataTransferClient()
{
    FabricState = NULL;
    ExchangeMgr = NULL;
    mBlockCounter = 0;
}

WEAVE_ERROR BulkDataTransferClient::Init(WeaveExchangeManager *exchangeMgr, const char* DestFileName)
{
    // Error if already initialized.
    if (ExchangeMgr != NULL)
        return WEAVE_ERROR_INCORRECT_STATE;

    ExchangeMgr = exchangeMgr;
    FabricState = exchangeMgr->FabricState;
    mBlockCounter = 0;

    if (DestFileName)
    {
        DestFile = open(DestFileName, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
        if (DestFile < 0)
            return INET_MapOSError(errno);
    }

    return WEAVE_NO_ERROR;
}

void BulkDataTransferClient::setCon(WeaveConnection *con)
{
    mCon = con;
}

WEAVE_ERROR BulkDataTransferClient::Shutdown()
{
    ExchangeMgr = NULL;
    FabricState = NULL;
    mBlockCounter = 0;

    return WEAVE_NO_ERROR;
}


/*
 * ReceiveInit request
 */

WEAVE_ERROR BulkDataTransferClient::SendReceiveInitRequest(WeaveConnection *con)
{
    // Discard any existing exchange context.
    // Effectively we can only have one BDX exchange with a single node at any one time.
    if (ExchangeCtx != NULL)
    {
        ExchangeCtx->Close();
        ExchangeCtx = NULL;
    }

    // Create a new exchange context.
    ExchangeCtx = ExchangeMgr->NewContext(con, this);
    if (ExchangeCtx == NULL)
        return WEAVE_ERROR_NO_MEMORY;

    return SendReceiveInitRequest();
}

WEAVE_ERROR BulkDataTransferClient::SendReceiveInitRequest(uint64_t nodeId, IPAddress nodeAddr)
{
    return SendReceiveInitRequest(nodeId, nodeAddr, WEAVE_PORT);
}

WEAVE_ERROR BulkDataTransferClient::SendReceiveInitRequest(uint64_t nodeId, IPAddress nodeAddr, uint16_t port)
{
    // Discard any existing exchange context.
    // Effectively we can only have one BDX exchange with a single node at any one time.
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

    return SendReceiveInitRequest();
}

#define BDX_MAX_BLOCK_SIZE 256
#define BDX_START_OFFSET 0

WEAVE_ERROR BulkDataTransferClient::SendReceiveInitRequest()
{
    if (!ExchangeCtx)
        return WEAVE_ERROR_INCORRECT_STATE;

    printf("0 SendReceiveInitRequest entering\n");

    // Configure the encryption and signature types to be used to send the request.
    ExchangeCtx->EncryptionType = EncryptionType;
    ExchangeCtx->KeyId = KeyId;

    // Arrange for messages in this exchange to go to our response handler.
    ExchangeCtx->OnMessageReceived = HandleReceiveInitResponse;

    /*
     * Build the ReceiveInit request
     */

    ReferencedString aFileDesignator;
    //NOTE: normally this URI would have been agreed upon with the SWU protocol
    //      Ex.: the SWU server returned "bdx://nestlabs/share/heatlink/data/firmware/production/heatlink.elf",
    //      so the file name is extracted and is sent to the BDX server.
    char aFileDesignatorStr[] = "/var/log/heatlink.elf"; //FIXME: temporary hack for the purpose of testing
    aFileDesignator.init((uint16_t)(sizeof(aFileDesignatorStr)-1), aFileDesignatorStr);
    
    ReceiveInit receiveInit;
    receiveInit.init(false /*SenderDrive*/, true /*ReceiverDrive*/, false /*AsynchMode*/, BDX_MAX_BLOCK_SIZE,
                        (uint32_t)BDX_START_OFFSET, (uint32_t)0 /*Length (zero means undefined length)*/, aFileDesignator, NULL /*MetaData*/);
    PacketBuffer* receiveInitPayload = PacketBuffer::New();
    receiveInit.pack(receiveInitPayload);

    // Send a ReceiveInit request message. Discard the exchange context if the send fails.
    WEAVE_ERROR err = ExchangeCtx->SendMessage(kWeaveProfile_BDX, kMsgType_ReceiveInit, receiveInitPayload);
    receiveInitPayload = NULL;
    if (err != WEAVE_NO_ERROR)
    {
        printf("1 SendReceiveInitRequest\n");
        ExchangeCtx->Close();
        ExchangeCtx = NULL;
        mCon->Close();
        mCon = NULL;
    }

    printf("2 SendReceiveInitRequest exiting\n");

    return err;
}

void BulkDataTransferClient::HandleReceiveInitResponse(ExchangeContext *ec, const IPPacketInfo *packetInfo, const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *payload)
{
    printf("0 HandleReceiveInitResponse entering\n");

    BulkDataTransferClient *bdxApp = (BulkDataTransferClient *)ec->AppState;

    // Verify that the message is a ReceiveInit Response.
    if (profileId != kWeaveProfile_BDX || (msgType != kMsgType_ReceiveAccept && msgType != kMsgType_ReceiveReject))
    {
        printf("1 HandleReceiveInitResponse\n");
        // TODO: handle unexpected response
        return;
    }

    // Verify that the exchange context matches our current context. Bail if not.
    if (ec != bdxApp->ExchangeCtx)
    {
        printf("2 HandleReceiveInitResponse\n");
        return;
    }

    if (msgType == kMsgType_ReceiveAccept)
    {
        printf("3 HandleReceiveInitResponse\n");

        //Send BlockQuery request
        bdxApp->SendBlockQueryRequest();
    }
    else if (msgType == kMsgType_ReceiveReject)
    {
        printf("4 HandleReceiveInitResponse\n");

        bdxApp->mCon->Close();
        bdxApp->mCon = NULL;
    }

    // Free the payload buffer.
    PacketBuffer::Free(payload);

    printf("5 HandleReceiveInitResponse exiting\n");
}


/*
 * BlockQuery request
 */
WEAVE_ERROR BulkDataTransferClient::SendBlockQueryRequest()
{
    printf("0 SendBlockQueryRequest entering\n");

    if (!ExchangeCtx)
        return WEAVE_ERROR_INCORRECT_STATE;

    // Configure the encryption and signature types to be used to send the request.
    ExchangeCtx->EncryptionType = EncryptionType;
    ExchangeCtx->KeyId = KeyId;

    // Arrange for messages in this exchange to go to our response handler.
    ExchangeCtx->OnMessageReceived = HandleBlockQueryResponse;

    /*
     * Build the BlockQuery request
     */

    BlockQuery blockQuery;
    blockQuery.init(mBlockCounter++); //first block requested is zero (next will be one, next two, etc)
    PacketBuffer* blockQueryPayload = PacketBuffer::New();
    blockQuery.pack(blockQueryPayload);

    // Send a BlockQuery request message. Discard the exchange context if the send fails.
    WEAVE_ERROR err = ExchangeCtx->SendMessage(kWeaveProfile_BDX, kMsgType_BlockQuery, blockQueryPayload);
    blockQueryPayload = NULL;
    if (err != WEAVE_NO_ERROR)
    {
        printf("1 SendBlockQueryRequest\n");
        ExchangeCtx->Close();
        ExchangeCtx = NULL;
        mCon->Close();
        mCon = NULL;
    }

    printf("2 SendBlockQueryRequest exiting\n");

    return err;
}

void BulkDataTransferClient::HandleBlockQueryResponse(ExchangeContext *ec, const IPPacketInfo *packetInfo, const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *payload)
{
    printf("0 HandleBlockQueryResponse entering\n");

    BulkDataTransferClient *bdxApp = (BulkDataTransferClient *)ec->AppState;

    // Verify that the message is a ReceiveInit Response.
    if (profileId != kWeaveProfile_BDX || (msgType != kMsgType_BlockSend && msgType != kMsgType_BlockEOF))
    {
        printf("1 HandleBlockQueryResponse\n");

        // TODO: handle unexpected response
        return;
    }

    // Verify that the exchange context matches our current context. Bail if not.
    if (ec != bdxApp->ExchangeCtx)
    {
        printf("2 HandleBlockQueryResponse\n");

        return;
    }

    //dump received block/payload
//    DumpMemory(payload->Start(), payload->DataLength(), "==> ", 16);

    if (msgType == kMsgType_BlockSend)
    {
        printf("3 HandleBlockQueryResponse (BlockSend)\n");
        DumpMemory(payload->Start(), payload->DataLength(), "--> ", 16);

        if (bdxApp->DestFile > 0) {
            // Write bulk data to disk.
            int wtd = write(bdxApp->DestFile, payload->Start(), payload->DataLength());
            printf("Wrote %d bytes to disk.\n", wtd);
        }

        //Send another BlockQuery
        bdxApp->SendBlockQueryRequest();
    }
    else if (msgType == kMsgType_BlockEOF)
    {
        printf("4 HandleBlockQueryResponse (BlockEOF)\n");
        DumpMemory(payload->Start(), payload->DataLength(), "==> ", 16);

        if (bdxApp->DestFile > 0) {
            // Close destination file.
            close(bdxApp->DestFile);
        }

        //Send final BlockEOFAck
        bdxApp->SendBlockEOFAck();
    }

    // Free the payload buffer.
    PacketBuffer::Free(payload);

    printf("5 HandleBlockQueryResponse exiting\n");
}


/*
 * BlockEOFAck
 */
WEAVE_ERROR BulkDataTransferClient::SendBlockEOFAck()
{
    printf("0 SendBlockEOFAck entering\n");

    if (!ExchangeCtx)
        return WEAVE_ERROR_INCORRECT_STATE;

    // Configure the encryption and signature types to be used to send the request.
    ExchangeCtx->EncryptionType = EncryptionType;
    ExchangeCtx->KeyId = KeyId;

    // Arrange for messages in this exchange to go to our response handler.
    ExchangeCtx->OnMessageReceived = HandleBlockEOFAckResponse;

    /*
     * Build the BlockEOFAck
     */

    BlockEOFAck blockEOFAck;
    blockEOFAck.init(mBlockCounter-1); //final ack uses same block-counter of last block-query request
    PacketBuffer* blockEOFAckPayload = PacketBuffer::New();
    blockEOFAck.pack(blockEOFAckPayload);

    // Send an BlockEOFAck request message. Discard the exchange context if the send fails.
    WEAVE_ERROR err = ExchangeCtx->SendMessage(kWeaveProfile_BDX, kMsgType_BlockEOFAck, blockEOFAckPayload);
    blockEOFAckPayload = NULL;
//    if (err != WEAVE_NO_ERROR)
//    {
        ExchangeCtx->Close();
        ExchangeCtx = NULL;
        mCon->Close();
        mCon = NULL;
//    }

    printf("1 SendBlockEOFAck exiting\n");
    Done = true;

    return err;
}

void BulkDataTransferClient::HandleBlockEOFAckResponse(ExchangeContext *ec, const IPPacketInfo *packetInfo, const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *payload)
{
    printf("A response to BlockEOFAck is NOT expected!\n");
}

} // namespace Profiles
} // namespace Weave
} // namespace nl
