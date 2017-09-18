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
 *      for the "development" (see [Managed Namespaces](@ref mns))
 *      Weave Bulk Data Transfer (BDX) profile used for functional
 *      testing of the implementation of core message handlers and
 *      protocol engine for that profile.
 *
 */

#include "ToolCommon.h"
#include "nlweavebdxclient.h"


namespace nl {
namespace Weave {
namespace Profiles {
namespace BulkDataTransfer {


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

WEAVE_ERROR BulkDataTransferClient::Init(WeaveExchangeManager *aExchangeMgr)
{
    // Error if already initialized.
    if (ExchangeMgr != NULL)
    {
        return WEAVE_ERROR_INCORRECT_STATE;
    }

    ExchangeMgr = aExchangeMgr;
    FabricState = aExchangeMgr->FabricState;
    mBlockCounter = 0;

    mDestFile = NULL;

    return WEAVE_NO_ERROR;
}

void BulkDataTransferClient::setCon(WeaveConnection *aCon)
{
    mCon = aCon;
}

WEAVE_ERROR BulkDataTransferClient::Shutdown()
{
    ExchangeMgr = NULL;
    FabricState = NULL;
    mBlockCounter = 0;

    return WEAVE_NO_ERROR;
}

/*
 * Public file upload/download API
 */

WEAVE_ERROR BulkDataTransferClient::SendFile(const char *DestFileName, WeaveConnection *aCon)
{
    printf("Sending file %s\n", DestFileName);

    mDestFile = fopen(DestFileName, "r");
    if (!mDestFile)
        return INET_MapOSError(errno);

    return SendSendInitRequest(aCon);
}

WEAVE_ERROR BulkDataTransferClient::SendFile(const char *DestFileName, uint64_t nodeId, IPAddress nodeAddr)
{
    printf("Sending file %s\n", DestFileName);

    mDestFile = fopen(DestFileName, "r");
    if (!mDestFile)
        return INET_MapOSError(errno);

    return SendSendInitRequest(nodeId, nodeAddr);
}

WEAVE_ERROR BulkDataTransferClient::ReceiveFile(const char *DestFileName, WeaveConnection *aCon)
{
    printf("Receiving file %s\n", DestFileName);

    mDestFile = fopen(DestFileName, "w");
    if (!mDestFile)
        return INET_MapOSError(errno);

    return SendSendInitRequest(aCon);
}

WEAVE_ERROR BulkDataTransferClient::ReceiveFile(const char *DestFileName, uint64_t nodeId, IPAddress nodeAddr)
{
    printf("Receiving file %s\n", DestFileName);

    mDestFile = fopen(DestFileName, "w");
    if (!mDestFile)
        return INET_MapOSError(errno);

    return SendSendInitRequest(nodeId, nodeAddr);
}

/*
 * SendInit request
 */

WEAVE_ERROR BulkDataTransferClient::SendSendInitRequest(WeaveConnection *aCon)
{
    // Discard any existing exchange context.
    // Effectively we can only have one BDX exchange with a single node at any one time.
    if (mExchangeCtx != NULL)
    {
        mExchangeCtx->Close();
        mExchangeCtx = NULL;
    }

    // Create a new exchange context.
    mExchangeCtx = ExchangeMgr->NewContext(aCon, this);
    if (mExchangeCtx == NULL)
        return WEAVE_ERROR_NO_MEMORY;

    return SendSendInitRequest();
}

WEAVE_ERROR BulkDataTransferClient::SendSendInitRequest(uint64_t nodeId, IPAddress nodeAddr)
{
    return SendSendInitRequest(nodeId, nodeAddr, WEAVE_PORT);
}

WEAVE_ERROR BulkDataTransferClient::SendSendInitRequest(uint64_t nodeId, IPAddress nodeAddr, uint16_t port)
{
    // Discard any existing exchange context.
    // Effectively we can only have one BDX exchange with a single node at any one time.
    if (mExchangeCtx != NULL)
    {
        mExchangeCtx->Close();
        mExchangeCtx = NULL;
    }

    if (nodeAddr == IPAddress::Any)
        nodeAddr = FabricState->SelectNodeAddress(nodeId);

    // Create a new exchange context.
    mExchangeCtx = ExchangeMgr->NewContext(nodeId, nodeAddr, this);
    if (mExchangeCtx == NULL)
        return WEAVE_ERROR_NO_MEMORY;

    return SendSendInitRequest();
}

#define BDX_MAX_BLOCK_SIZE 256
#define BDX_START_OFFSET 0

WEAVE_ERROR BulkDataTransferClient::SendSendInitRequest()
{
    if (!mExchangeCtx)
        return WEAVE_ERROR_INCORRECT_STATE;

    printf("0 SendSendInitRequest entering\n");

    // Configure the encryption and signature types to be used to send the request.
    mExchangeCtx->EncryptionType = EncryptionType;
    mExchangeCtx->KeyId = KeyId;

    // Arrange for messages in this exchange to go to our response handler.
    mExchangeCtx->OnMessageReceived = HandleSendInitResponse;

    /*
     * Build the SendInit request
     */

    ReferencedString aFileDesignator;
    //NOTE: normally this URI would have been agreed upon with the SWU protocol
    //      Ex.: the SWU server returned "bdx://nestlabs/share/heatlink/data/firmware/production/heatlink.elf",
    //      so the file name is extracted and is sent to the BDX server.
    char aFileDesignatorStr[] = "file:///var/log/heatlink.elf"; //FIXME: temporary hack for the purpose of testing
    aFileDesignator.init((uint16_t)(sizeof(aFileDesignatorStr)-1), aFileDesignatorStr);

    SendInit sendInit;
    bool asyncMode = false, senderDriven = true;
    sendInit.init(senderDriven, false /*ReceiverDrive*/, asyncMode, BDX_MAX_BLOCK_SIZE,
                        (uint32_t)BDX_START_OFFSET, (uint32_t)0 /*Length (zero means undefined length)*/, aFileDesignator, NULL /*MetaData*/);
    PacketBuffer* sendInitPayload = PacketBuffer::New();
    sendInit.pack(sendInitPayload);

    // Send a SendInit request message. Discard the exchange context if the send fails.
    WEAVE_ERROR err = mExchangeCtx->SendMessage(kWeaveProfile_BDX, kMsgType_SendInit, sendInitPayload);
    sendInitPayload = NULL;
    if (err != WEAVE_NO_ERROR)
    {
        printf("1 SendSendInitRequest\n");
        mExchangeCtx->Close();
        mExchangeCtx = NULL;
        mCon->Close();
        mCon = NULL;
    }

    printf("2 SendSendInitRequest exiting\n");

    return err;
}

void BulkDataTransferClient::HandleSendInitResponse(ExchangeContext *ec, const IPPacketInfo *packetInfo, const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *payload)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    printf("0 HandleSendInitResponse entering\n");

    BulkDataTransferClient *bdxApp = (BulkDataTransferClient *)ec->AppState;

    // Verify that the message is a SendInit Response.
    if (profileId != kWeaveProfile_BDX || (msgType != kMsgType_SendAccept && msgType != kMsgType_SendReject))
    {
        printf("1 HandleSendInitResponse\n");
        // TODO: handle unexpected response
        return;
    }

    // Verify that the exchange context matches our current context. Bail if not.
    if (ec != bdxApp->mExchangeCtx)
    {
        printf("2 HandleSendInitResponse\n");
        return;
    }

    if (msgType == kMsgType_SendAccept)
    {
        printf("3 HandleSendInitResponse\n");

        DumpMemory(payload->Start(), payload->DataLength(), "==> ", 16);

        // Parse info from response, set variables, and start sending blocks
        SendAccept sendAccept;
        err = SendAccept::parse(payload, sendAccept);
        VerifyOrExit(err == WEAVE_NO_ERROR, printf("Error parsing SendAccept payload!"));

        bdxApp->mTransferMode = sendAccept.mTransferMode;
        //TODO: validate the transfer mode, possibly looking back at what we requested?
        if (bdxApp->IsSenderDriveMode())
        {
            ec->OnMessageReceived = HandleBlockAck;
        }

        printf("Max block size negotiated: %d\n", sendAccept.mMaxBlockSize);
        bdxApp->mMaxBlockSize = sendAccept.mMaxBlockSize;
        PacketBuffer::Free(payload);
        payload = NULL;

        bdxApp->SendBlock();
    }
    else if (msgType == kMsgType_SendReject)
    {
        printf("4 HandleSendAcceptResponse rejected\n");

        bdxApp->mCon->Close();
        bdxApp->mCon = NULL;
    }

    // Free the payload buffer.
    PacketBuffer::Free(payload);

exit:
    if (err != WEAVE_NO_ERROR)
    {
        printf("HandleSendInitResponse exiting with error: %d\n", err);
    }
    else
    {
        printf("5 HandleSendInitResponse exiting\n");
    }

    return err;
}

void BulkDataTransferClient::HandleBlockAck(ExchangeContext *ec, const IPPacketInfo *packetInfo, const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *payload)
{
    BlockAck ack;
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    printf("HandleBlock Ack entering\n");

    err = BlockAck::parse(payload, ack);
    VerifyOrExit(err == WEAVE_NO_ERROR, printf("Error parsing BlockAck: %d\n", err));
    VerifyOrExit(msgType == kMsgType_BlockAck,
                 err = WEAVE_ERROR_INVALID_MESSAGE_TYPE; printf("Error in HandleBlockAck: expected BlockAck but got %d\n", msgType);

    //TODO: verify that we got the right block number in this ACK

    BulkDataTransferClient *bdxApp = (BulkDataTransferClient *)ec->AppState;
    err = bdxApp->SendBlock();
    VerifyOrExit(err == WEAVE_NO_ERROR, printf("Error trying to SendBlock: %d\n", err));

exit:
    if (err != WEAVE_NO_ERROR)
    {
        printf("HandleBlockAck exiting with error: %d\n", err);
    }

    return;
}

void BulkDataTransferClient::HandleBlockEOFAck(ExchangeContext *ec, const IPPacketInfo *packetInfo, const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *payload)
{
    VerifyOrExit(msgType == kMsgType_BlocEOFkAck,
                 err = WEAVE_ERROR_INVALID_MESSAGE_TYPE; printf("Expected BlockEOFAck but got %d\n", msgType);

    // Close out the connections so the client can exit
    BulkDataTransferClient *bdxApp = (BulkDataTransferClient *)ec->AppState;
    bdxApp->mExchangeCtx->Close();
    bdxApp->mExchangeCtx = NULL;
    bdxApp->mCon->Close();
    bdxApp->mCon = NULL;
    //TODO: not use a global variable...
    Done = true;
    printf("BlockEOFAck Received. Transfer complete.  Exiting...\n");

exit:
    return err;
}

/*
 * ReceiveInit request
 */

WEAVE_ERROR BulkDataTransferClient::SendReceiveInitRequest(WeaveConnection *con)
{
    // Discard any existing exchange context.
    // Effectively we can only have one BDX exchange with a single node at any one time.
    if (mExchangeCtx != NULL)
    {
        mExchangeCtx->Close();
        mExchangeCtx = NULL;
    }

    // Create a new exchange context.
    mExchangeCtx = ExchangeMgr->NewContext(con, this);
    if (mExchangeCtx == NULL)
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
    if (mExchangeCtx != NULL)
    {
        mExchangeCtx->Close();
        mExchangeCtx = NULL;
    }

    if (nodeAddr == IPAddress::Any)
        nodeAddr = FabricState->SelectNodeAddress(nodeId);

    // Create a new exchange context.
    mExchangeCtx = ExchangeMgr->NewContext(nodeId, nodeAddr, this);
    if (mExchangeCtx == NULL)
        return WEAVE_ERROR_NO_MEMORY;

    return SendReceiveInitRequest();
}

WEAVE_ERROR BulkDataTransferClient::SendReceiveInitRequest()
{
    if (!mExchangeCtx)
        return WEAVE_ERROR_INCORRECT_STATE;

    printf("0 SendReceiveInitRequest entering\n");

    // Configure the encryption and signature types to be used to send the request.
    mExchangeCtx->EncryptionType = EncryptionType;
    mExchangeCtx->KeyId = KeyId;

    // Arrange for messages in this exchange to go to our response handler.
    mExchangeCtx->OnMessageReceived = HandleReceiveInitResponse;

    /*
     * Build the ReceiveInit request
     */

    ReferencedString aFileDesignator;
    //NOTE: normally this URI would have been agreed upon with the SWU protocol
    //      Ex.: the SWU server returned "bdx://nestlabs/share/heatlink/data/firmware/production/heatlink.elf",
    //      so the file name is extracted and is sent to the BDX server.
    char aFileDesignatorStr[] = "file:///var/log/heatlink.elf"; //FIXME: temporary hack for the purpose of testing
    aFileDesignator.init((uint16_t)(sizeof(aFileDesignatorStr)-1), aFileDesignatorStr);

    ReceiveInit receiveInit;
    receiveInit.init(false /*SenderDrive*/, true /*ReceiverDrive*/, false /*AsynchMode*/, BDX_MAX_BLOCK_SIZE,
                        (uint32_t)BDX_START_OFFSET, (uint32_t)0 /*Length (zero means undefined length)*/, aFileDesignator, NULL /*MetaData*/);
    PacketBuffer* receiveInitPayload = PacketBuffer::New();
    receiveInit.pack(receiveInitPayload);

    // Send a ReceiveInit request message. Discard the exchange context if the send fails.
    WEAVE_ERROR err = mExchangeCtx->SendMessage(kWeaveProfile_BDX, kMsgType_ReceiveInit, receiveInitPayload);
    receiveInitPayload = NULL;
    if (err != WEAVE_NO_ERROR)
    {
        printf("1 SendReceiveInitRequest\n");
        mExchangeCtx->Close();
        mExchangeCtx = NULL;
        mCon->Close();
        mCon = NULL;
    }

    printf("2 SendReceiveInitRequest exiting\n");

    return err;
}

WEAVE_ERROR BulkDataTransferClient::HandleReceiveInitResponse(ExchangeContext *ec, const IPPacketInfo *packetInfo, const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *payload)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    printf("0 HandleReceiveInitResponse entering\n");

    BulkDataTransferClient *bdxApp = (BulkDataTransferClient *)ec->AppState;

    // Verify that the message is a ReceiveInit Response.
    if (profileId != kWeaveProfile_BDX || (msgType != kMsgType_ReceiveAccept && msgType != kMsgType_ReceiveReject))
    {
        printf("1 HandleReceiveInitResponse\n");
        // TODO: handle unexpected response
        return err;
    }

    // Verify that the exchange context matches our current context. Bail if not.
    if (ec != bdxApp->mExchangeCtx)
    {
        printf("2 HandleReceiveInitResponse\n");
        return err;
    }

    if (msgType == kMsgType_ReceiveAccept)
    {
        printf("3 HandleReceiveInitResponse\n");

        ReceiveInit ri;
        ReceiveInit::parse(payload, ri);
        printf("Supposed maxBlockSize: %d\n", ri.mMaxBlockSize);
        DumpMemory(payload->Start(), payload->DataLength(), "--> ", 16);

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

    return err;
}

/*
 * BlockSend message
 */
WEAVE_ERROR BulkDataTransferClient::SendBlock()
{
    printf("0 SendBlock entering\n");

    if (!mExchangeCtx)
        return WEAVE_ERROR_INCORRECT_STATE;

    // Configure the encryption and signature types to be used to send the request.
    mExchangeCtx->EncryptionType = EncryptionType;
    mExchangeCtx->KeyId = KeyId;

    // Read the data from the file and build the BlockSend message
    printf("Reading data\n");
    uint8_t blockData[mMaxBlockSize];
    int bytesRead = fread(blockData, 1, mMaxBlockSize, mDestFile);
    printf("Data read: %d bytes of max size %d\n", bytesRead, mMaxBlockSize);

    BlockSend blockSend;
    blockSend.init(mBlockCounter++, bytesRead, blockData); //first block sent is zero (next will be one, next two, etc)
    PacketBuffer* blockSendPayload = PacketBuffer::New();
    blockSend.pack(blockSendPayload);

    // Send a BlockSend message. Make sure it's an EOF if this is the last block.
    // Discard the exchange context if the send fails.
    //
    // FIXME: we can reach EOF and have read exactly mMaxBlockSize,
    // but all the C API functions require us to try reading another byte
    // and receive the EOF character before being able to recognize that we're finished.
    // Therefore, we'll have to add complexity by pre-fetching each block and transmitting
    // the last one so that we can see if the pre-fetched block is of size 0,
    // indicating that the last fetched one should be sent as BlockEOF.
    bool atEOF = bytesRead < mMaxBlockSize;
    if (atEOF)
    {
        printf("Sending BlockEOF\n");
        mExchangeCtx->OnMessageReceived = HandleBlockEOFAck;
    }
    else
        printf("Sending Block...\n");

    WEAVE_ERROR err = mExchangeCtx->SendMessage(kWeaveProfile_BDX, atEOF ? kMsgType_BlockEOF : kMsgType_BlockSend, blockSendPayload);
    blockSendPayload = NULL;
    if (err != WEAVE_NO_ERROR)
    {
        printf("1 SendBlock\n");
        mExchangeCtx->Close();
        mExchangeCtx = NULL;
        mCon->Close();
        mCon = NULL;
    }

    printf("2 SendBlock exiting\n");

    return err;
}

/*
 * BlockQuery request
 */
WEAVE_ERROR BulkDataTransferClient::SendBlockQueryRequest()
{
    printf("0 SendBlockQueryRequest entering\n");

    if (!mExchangeCtx)
        return WEAVE_ERROR_INCORRECT_STATE;

    // Configure the encryption and signature types to be used to send the request.
    mExchangeCtx->EncryptionType = EncryptionType;
    mExchangeCtx->KeyId = KeyId;

    // Arrange for messages in this exchange to go to our response handler.
    mExchangeCtx->OnMessageReceived = HandleBlockQueryResponse;

    /*
     * Build the BlockQuery request
     */

    BlockQuery blockQuery;
    //FIXME: let's not increment the counter here so that we can e.g. resend the same block or test it easier
    blockQuery.init(mBlockCounter++); //first block requested is zero (next will be one, next two, etc)
    PacketBuffer* blockQueryPayload = PacketBuffer::New();
    blockQuery.pack(blockQueryPayload);

    // Send a BlockQuery request message. Discard the exchange context if the send fails.
    WEAVE_ERROR err = mExchangeCtx->SendMessage(kWeaveProfile_BDX, kMsgType_BlockQuery, blockQueryPayload);
    blockQueryPayload = NULL;
    if (err != WEAVE_NO_ERROR)
    {
        printf("1 SendBlockQueryRequest\n");
        mExchangeCtx->Close();
        mExchangeCtx = NULL;
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
    if (ec != bdxApp->mExchangeCtx)
    {
        printf("2 HandleBlockQueryResponse\n");

        return;
    }

    if (msgType == kMsgType_BlockSend)
    {
        printf("3 HandleBlockQueryResponse (BlockSend)\n");
        DumpMemory(payload->Start(), payload->DataLength(), "--> ", 16);

        if (bdxApp->mDestFile) {
            // Write bulk data to disk.
            // TODO: ensure this isn't writing the header info to disk as well
            int wtd = fwrite(payload->Start(), 1, payload->DataLength(), bdxApp->mDestFile);
            printf("Wrote %d bytes to disk.\n", wtd);
        }

        //Send another BlockQuery
        bdxApp->SendBlockQueryRequest();
    }
    else if (msgType == kMsgType_BlockEOF)
    {
        printf("4 HandleBlockQueryResponse (BlockEOF)\n");
        DumpMemory(payload->Start(), payload->DataLength(), "==> ", 16);

        //TODO: this isn't writing the payload to the file!!!!!
        if (bdxApp->mDestFile) {
            // Close destination file.
            if (fclose(bdxApp->mDestFile) == EOF)
            {
                printf("Error closing file! %d\n", ferror(bdxApp->mDestFile));
                exit(-1);
            }
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

    if (!mExchangeCtx)
        return WEAVE_ERROR_INCORRECT_STATE;

    // Configure the encryption and signature types to be used to send the request.
    mExchangeCtx->EncryptionType = EncryptionType;
    mExchangeCtx->KeyId = KeyId;

    // Arrange for messages in this exchange to go to our response handler.
    mExchangeCtx->OnMessageReceived = HandleBlockEOFAckResponse;

    /*
     * Build the BlockEOFAck
     */

    BlockEOFAck blockEOFAck;
    blockEOFAck.init(mBlockCounter - 1); //final ack uses same block-counter of last block-query request
    PacketBuffer* blockEOFAckPayload = PacketBuffer::New();
    blockEOFAck.pack(blockEOFAckPayload);

    // Send an BlockEOFAck request message. Discard the exchange context if the send fails.
    WEAVE_ERROR err = mExchangeCtx->SendMessage(kWeaveProfile_BDX, kMsgType_BlockEOFAck, blockEOFAckPayload);
    blockEOFAckPayload = NULL;
    if (err != WEAVE_NO_ERROR)
    {
        mExchangeCtx->Close();
        mExchangeCtx = NULL;
        mCon->Close();
        mCon = NULL;
    }

    printf("1 SendBlockEOFAck exiting\n");
    Done = true;

    return err;
}

void BulkDataTransferClient::HandleBlockEOFAckResponse(ExchangeContext *ec, const IPPacketInfo *packetInfo, const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *payload)
{
    printf("A response to BlockEOFAck is NOT expected!\n");
}

bool BulkDataTransferClient::IsAsyncMode()
{
    return mTransferMode & kMode_Asynchronous;
}

bool BulkDataTransferClient::IsSenderDriveMode()
{
    return mTransferMode & kMode_SenderDrive;
}

bool BulkDataTransferClient::IsReceiverDriveMode()
{
    return mTransferMode & kMode_ReceiverDrive;
}

} // namespace BulkDataTransfer
} // namespace Profiles
} // namespace Weave
} // namespace nl
