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

#include "nlassert.h"
#include "nlweavebdxserver.h"
#include <unistd.h>
#include <fcntl.h>

namespace nl {
namespace Weave {
namespace Profiles {

BulkDataTransferServer::BulkDataTransferServer()
{
    ExchangeMgr = NULL;
    mpAppState = NULL;
    OnBDXReceiveInitRequestReceived = NULL;
    OnBDXBlockQueryRequestReceived = NULL;
    OnBDXBlockEOFAckReceived = NULL;
    OnBDXTransferFailed = NULL;
    OnBDXTransferSucceeded = NULL;
}

BulkDataTransferServer::~BulkDataTransferServer()
{
    Shutdown();
}

WEAVE_ERROR BulkDataTransferServer::Init(WeaveExchangeManager *exchangeMgr, void *appState,
        const char *hostedFileName, const char *receivedFileLocation)
{
    // Error if already initialized.
    if (ExchangeMgr != NULL)
        return WEAVE_ERROR_INCORRECT_STATE;

    ExchangeMgr = exchangeMgr;
    mpAppState = appState;
    mHostedFileName = hostedFileName;
    mReceivedFileLocation = receivedFileLocation;

    // Initialize connection pool
    memset(mTransferPool, 0, sizeof(mTransferPool));
    for (int i = 0; i < MAX_NUM_BDX_TRANSFERS; i++)
    {
        mTransferPool[i].FD = -1;
    }

    // Register to receive unsolicited ReceiveInitiation messages from the exchange manager.
    ExchangeMgr->RegisterUnsolicitedMessageHandler(kWeaveProfile_BDX, kMsgType_ReceiveInit, HandleReceiveInitRequest, this);
    ExchangeMgr->RegisterUnsolicitedMessageHandler(kWeaveProfile_BDX, kMsgType_SendInit, HandleSendInitRequest, this);

    return WEAVE_NO_ERROR;
}

WEAVE_ERROR BulkDataTransferServer::Shutdown()
{
    printf("0 BDX Shutdown entering\n");

    if (ExchangeMgr != NULL)
    {
        // Shutdown actions to perform only if BDX server initialized:

        ExchangeMgr->UnregisterUnsolicitedMessageHandler(kWeaveProfile_BDX, kMsgType_ReceiveInit);
        ExchangeMgr->UnregisterUnsolicitedMessageHandler(kWeaveProfile_BDX, kMsgType_SendInit);
        ExchangeMgr = NULL;

        // Explicitly shut down transfers to free any held Weave resources
        for (int i = 0; i < MAX_NUM_BDX_TRANSFERS; i++)
        {
            ShutdownTransfer(&mTransferPool[i], true);
        }
    }

    // Shutdown actions to perform even if BDX server uninitialized:

    mpAppState = NULL;
    OnBDXReceiveInitRequestReceived = NULL;
    OnBDXBlockQueryRequestReceived = NULL;
    OnBDXBlockEOFAckReceived = NULL;
    OnBDXTransferFailed = NULL;
    OnBDXTransferSucceeded = NULL;

    printf("1 BDX Shutdown exiting\n");
    return WEAVE_NO_ERROR;
}

BulkDataTransferServer::BDXTransfer *BulkDataTransferServer::NewTransfer()
{
    for (int i = 0; i < MAX_NUM_BDX_TRANSFERS; i++)
    {
        if (!mTransferPool[i].BdxApp)
        {
            mTransferPool[i].BdxApp = this;
            return &mTransferPool[i];
        }
    }

    return NULL;
}

void BulkDataTransferServer::ShutdownTransfer(BDXTransfer *xfer, bool closeCon)
{
    if (xfer->BdxApp == NULL)
    {
        // Suppress log spew if iterating through entire connection pool as part of Shutdown()
        return;
    }

    printf("0 BDX ShutdownTransfer entering\n");
    uint64_t peerNodeId = kNodeIdNotSpecified;
    IPAddress peerAddr = IPAddress::Any;

    // Get values to send application callback
    if (xfer->EC && xfer->EC->Con)
    {
        peerNodeId = xfer->EC->Con->PeerNodeId;
        peerAddr = xfer->EC->Con->PeerAddr;
    }

    // Fire application callback
    if (false == xfer->CompletedSuccessfully)
    {
        if (OnBDXTransferFailed)
        {
            OnBDXTransferFailed(peerNodeId, peerAddr, mpAppState);
        }
    }
    else
    {
        if (OnBDXTransferSucceeded)
        {
            OnBDXTransferSucceeded(peerNodeId, peerAddr, mpAppState);
        }
    }

    /* Reset and release transfer object. This needs to be done before the Weave connection
       is closed because closing a Weave connection will call EC->OnConnectionClosed which in turn will
       call our OnConnectionClosed handler which will then call ShutdownTranser() again.
       Because xfer->BdxApp is NULL the second time ShutdownTranser() is called it will exit right away */

    xfer->MaxBlockSize = 0;
    xfer->CompletedSuccessfully = false;
    xfer->BdxApp = NULL;

    // Release Weave resources
    if (xfer->EC)
    {
        printf("1 BDX ShutdownTransfer closing EC\n");

        if (closeCon && xfer->EC->Con)
        {
            printf("2 BDX ShutdownTransfer closing Con\n");
            xfer->EC->Con->Close();
            xfer->EC->Con = NULL;
        }

        xfer->EC->Close();
        xfer->EC = NULL;
    }

    // Free pbuf
    if (xfer->BlockBuffer)
    {
        printf("3 BDX ShutdownTransfer closing BlockBuffer\n");
        PacketBuffer::Free(xfer->BlockBuffer);
        xfer->BlockBuffer = NULL;
    }

    // Close file
    if (xfer->FD != -1)
    {
        printf("4 BDX ShutdownTransfer closing FD\n");
        close(xfer->FD);
        xfer->FD = -1;
    }

    printf("5 BDX ShutdownTransfer exiting");
}

void BulkDataTransferServer::HandleReceiveInitRequest(ExchangeContext *ec, const IPPacketInfo *packetInfo,
        const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *payloadReceiveInit)
{
    // We're guaranteed of the right message profile and type by the ExchangeMgr.
    printf("0 BDX HandleReceiveInitRequest entering\n");

    WEAVE_ERROR ret = WEAVE_NO_ERROR;
    const uint8_t BDX_SERVER_TRANSFER_MODE = 0x02U; //ASYNC==0, RDRIVE==1, SDRIVE==0
    BulkDataTransferServer *bdxApp = NULL;
    BDXTransfer *xfer = NULL;
    PacketBuffer *payload = NULL;
    char *fileDesignator = NULL;

    ReceiveAccept receiveAccept;
    ReceiveReject receiveReject;
    ReceiveInit   receiveInit;

    nlREQUIRE_ACTION(ec != NULL, handle_receive_init_request_failed,
                     printf("0.5 BDX HandleReceiveInitRequest failed, null EC\n"));
    bdxApp = (BulkDataTransferServer *) ec->AppState;

    // Parse init request and discard payload buffer
    ret = ReceiveInit::parse(payloadReceiveInit, receiveInit);
    nlREQUIRE(ret == WEAVE_NO_ERROR, handle_receive_init_request_failed);
    PacketBuffer::Free(payloadReceiveInit);
    payloadReceiveInit = NULL;

    // Grab BDXTransfer object for this transfer
    xfer = bdxApp->NewTransfer();

    if (!xfer)
    {
        printf("1 BDX HandleReceiveInitRequest (transfer alloc failed)\n");
        SendTransferError(ec, kWeaveProfile_Common, kStatus_OutOfMemory);
        goto handle_receive_init_request_failed;
    }

    // Hang new BDXTransfer on exchange context
    ec->AppState = xfer;

    // Initialize xfer struct (move to init function?)
    xfer->EC = ec;
    xfer->FD = -1; // memset(0) doesn't set us up for ShutdownTransfer()

    if (receiveInit.theMaxBlockSize <= 0) {
        printf("2 BDX HandleReceiveInitRequest (maxBlockSize <= 0)\n");

        // Send rejection status message
        receiveReject.init(kWeaveProfile_Common, kStatus_BadRequest);
        if ((payload = PacketBuffer::New()) == NULL)
        {
            printf("2.5 BDX HandleReceiveInitRequest (PacketBuffer alloc failed)\n");
            goto handle_receive_init_request_failed;
        }

        receiveReject.pack(payload);
        ret = ec->SendMessage(kWeaveProfile_Common, kMsgType_ReceiveReject, payload);
        if (ret != WEAVE_NO_ERROR)
        {
            printf("3 BDX HandleReceiveInitRequest err=%d\n", ret);
        }
        payload = NULL;
        goto handle_receive_init_request_failed;

    }
    xfer->MaxBlockSize  = receiveInit.theMaxBlockSize;

    if (receiveInit.theFileDesignator.theLength <= 0) {
        printf("4 BDX HandleReceiveInitRequest (bad FileDesignator)\n");
        SendTransferError(ec, kWeaveProfile_Common, kStatus_LengthTooShort);
        goto handle_receive_init_request_failed;
    }

    // Copy file name onto C-string
    // NOTE: the original string is not NUL terminated, but we know its length.
    fileDesignator = (char*)malloc(1 + receiveInit.theFileDesignator.theLength);
    memcpy(fileDesignator, receiveInit.theFileDesignator.theString, receiveInit.theFileDesignator.theLength);
    fileDesignator[receiveInit.theFileDesignator.theLength] = '\0';

    // TODO Validate requested file path with value from nlhlfirmware.plist
    // Future: delegate path security validation
    // nlclient will open() this path as root, so we must be conservative in our validation.
    if (0 != strcmp(fileDesignator, bdxApp->mHostedFileName))
    {
        printf("5 BDX HandleReceiveInitRequest (forbidden FileDesignator)\n");
        SendTransferError(ec, kWeaveProfile_Common, kStatus_UnknownFile); //TODO add 'forbidden' Weave status code
        free(fileDesignator);
        fileDesignator = NULL;
        goto handle_receive_init_request_failed;
    }

    // Open file to send
    xfer->FD = open(fileDesignator, O_RDONLY);
    free(fileDesignator);
    fileDesignator = NULL;

    if (xfer->FD == -1)
    {
        printf("6 BDX HandleReceiveInitRequest (open FAIL)\n");
        SendTransferError(ec, kWeaveProfile_Common, kStatus_InternalServerProblem);
        goto handle_receive_init_request_failed;
    }

    // Send a ReceiveAccept response back to the receiver.
    printf("7 BDX HandleReceiveInitRequest validated request\n");

    // Fire application callback once we've validated the request (TODO: call earlier? feels like semantic abuse)
    if (bdxApp->OnBDXReceiveInitRequestReceived)
    {
        bdxApp->OnBDXReceiveInitRequestReceived(ec->PeerNodeId, ec->PeerAddr, payloadReceiveInit, bdxApp->mpAppState);
    }

    // Set up response timeout and connection closed handler
    ec->Con->AppState = xfer;
    ec->OnConnectionClosed = HandleBDXConnectionClosed;
    ec->OnResponseTimeout = HandleResponseTimeout;
    ec->ResponseTimeout = BDX_RESPONSE_TIMEOUT_MS;

    // Set ourselves up to handle first BlockQueryRequest.
    ec->OnMessageReceived = HandleBlockQueryRequest;

    receiveAccept.init(BDX_SERVER_TRANSFER_MODE, receiveInit.theMaxBlockSize, receiveInit.theLength, NULL);
    if ((payload = PacketBuffer::New()) == NULL)
    {
        printf("7.5 BDX HandleReceiveInitRequest (PacketBuffer alloc failed)\n");
        goto handle_receive_init_request_failed;
    }

    receiveAccept.theMaxBlockSize = receiveInit.theMaxBlockSize;
    ret = receiveAccept.pack(payload);
    nlREQUIRE(ret == WEAVE_NO_ERROR, handle_receive_init_request_failed);

    ret = ec->SendMessage(kWeaveProfile_BDX, kMsgType_ReceiveAccept, payload, ExchangeContext::kSendFlag_ExpectResponse);
    payload = NULL;
    if (ret != WEAVE_NO_ERROR)
    {
        printf("8 BDX HandleReceiveInitRequest err=%d\n", ret);
        goto handle_receive_init_request_failed;
    }

    printf("9 BDX HandleReceiveInitRequest exiting (success)\n");
    return;

handle_receive_init_request_failed:
    printf("10 BDX HandleReceiveInitRequest exiting (failure)\n");

    if (payload)
    {
        PacketBuffer::Free(payload);
        payload = NULL;
    }

    if (payloadReceiveInit)
    {
        PacketBuffer::Free(payloadReceiveInit);
        payloadReceiveInit = NULL;
    }

    if (xfer)
    {
        bdxApp->ShutdownTransfer(xfer, true);
    }
    else
    {
        // Transfer object uninitialized, so we do this manually
        if (ec)
        {
            if (ec->Con)
            {
                ec->Con->Close();
                ec->Con = NULL;
            }

            ec->Close();
            ec = NULL;
        }
    }
}

void BulkDataTransferServer::HandleSendInitRequest(ExchangeContext *ec, const IPPacketInfo *packetInfo, const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *payload)
{
    printf("BDX HandleSendInitRequest entering\n");

    WEAVE_ERROR err = WEAVE_NO_ERROR;
    BulkDataTransferServer *bdxApp = NULL;
    BDXTransfer *xfer = NULL;

    SendInit sendInit;
    SendAccept sendAccept;
    SendReject sendReject;

    char *filename = NULL;
    uint8_t offset = 0;
    char *fileDesignator = NULL;
    PacketBuffer *SendInitResponsePayload = NULL;

    nlREQUIRE(ec, handle_send_init_request_failed);
    bdxApp = static_cast<BulkDataTransferServer *>(ec->AppState);

    xfer = bdxApp->NewTransfer();
    nlREQUIRE(xfer, handle_send_init_request_failed);

    xfer->EC = ec;
    xfer->FD = -1;
    xfer->CompletedSuccessfully = false;
    ec->AppState = (void*)xfer;

    err = SendInit::parse(payload, sendInit);
    nlREQUIRE(err == WEAVE_NO_ERROR, handle_send_init_request_failed);

    xfer->MaxBlockSize = sendInit.theMaxBlockSize;
    PacketBuffer::Free(payload);
    payload = NULL;

    /**
     * Allocate PacketBuffer
     */
    SendInitResponsePayload = PacketBuffer::New();
    nlREQUIRE_ACTION(SendInitResponsePayload != NULL, handle_send_init_request_failed,
                         printf("Error: BDX HandleSendInitRequest: PacketBuffer alloc failed\n"));

    // get the received file and location and name
    filename = strrchr(sendInit.theFileDesignator.theString, '/');
    if (filename == NULL)
    {
        filename = sendInit.theFileDesignator.theString;
    }
    else
    {
        filename++; //skip over '/'
    }

    fileDesignator = (char*)malloc(strlen(bdxApp->mReceivedFileLocation) + strlen(filename) + 2);
    nlREQUIRE(fileDesignator != NULL, handle_send_init_request_failed);

    memcpy(fileDesignator, bdxApp->mReceivedFileLocation, strlen(bdxApp->mReceivedFileLocation));
    if (bdxApp->mReceivedFileLocation[strlen(bdxApp->mReceivedFileLocation) - 1] != '/')
    {
        // if it doesn't end with '/', add one
        fileDesignator[strlen(bdxApp->mReceivedFileLocation)] = '/';
        offset++;
    }
    memcpy(fileDesignator + strlen(bdxApp->mReceivedFileLocation) + offset, filename, strlen(filename));
    fileDesignator[strlen(bdxApp->mReceivedFileLocation) + offset + strlen(filename)] = '\0';

    printf("File being saved to: %s\n", fileDesignator);
    xfer->FD = open(fileDesignator, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
    free(fileDesignator);

    if (xfer->FD == -1)
    {
        printf("Couldn't open file %s for writing...\n", filename);

        err = sendReject.init(kWeaveProfile_BDX, kStatus_UnknownFile);
        nlREQUIRE(err == WEAVE_NO_ERROR, handle_send_init_request_failed);

        err = sendReject.pack(SendInitResponsePayload);
        nlREQUIRE(err == WEAVE_NO_ERROR, handle_send_init_request_failed);

        err = ec->SendMessage(kWeaveProfile_BDX, kMsgType_SendReject, SendInitResponsePayload, ExchangeContext::kSendFlag_ExpectResponse);
        SendInitResponsePayload = NULL;
        goto handle_send_init_request_failed;
    }

    // Finish up configuring and then send the SendInitResponse message
    sendAccept.theMaxBlockSize = xfer->MaxBlockSize;
    err = sendAccept.init(kMode_SenderDrive, xfer->MaxBlockSize, NULL);
    nlREQUIRE(err == WEAVE_NO_ERROR, handle_send_init_request_failed);

    err = sendAccept.pack(SendInitResponsePayload);
    nlREQUIRE(err == WEAVE_NO_ERROR, handle_send_init_request_failed);

    ec->OnMessageReceived = HandleBlockSend;

    err = ec->SendMessage(kWeaveProfile_BDX, kMsgType_SendAccept, SendInitResponsePayload,
                          ExchangeContext::kSendFlag_ExpectResponse);
    SendInitResponsePayload = NULL;
    nlREQUIRE_ACTION(err == WEAVE_NO_ERROR, handle_send_init_request_failed,
                 printf("SendInitResponse error sending accept message: %d", err));

    return;

handle_send_init_request_failed:

    printf("BDX HandleSendInitRequest exiting (failure)\n");

    if (SendInitResponsePayload)
    {
        PacketBuffer::Free(SendInitResponsePayload);
        SendInitResponsePayload = NULL;
    }

    if (payload)
    {
        PacketBuffer::Free(payload);
        payload = NULL;
    }

    if (xfer)
    {
        bdxApp->ShutdownTransfer(xfer, true);
    }
    else
    {
        // Transfer object uninitialized, so we do this manually
        if (ec)
        {
            if (ec->Con)
            {
                ec->Con->Close();
                ec->Con = NULL;
            }

            ec->Close();
            ec = NULL;
        }
    }
}

void BulkDataTransferServer::HandleBlockSend(ExchangeContext *ec, const IPPacketInfo *packetInfo, const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *payload)
{
    printf("BDX HandleBlockSend entering\n");

    WEAVE_ERROR err = WEAVE_NO_ERROR;

    int len = 0;

    BlockSend blockSend;
    BDXTransfer *xfer = static_cast<BDXTransfer *>(ec->AppState);
    BulkDataTransferServer *bdxApp = xfer->BdxApp;

    // Parse message data to get the block counter later
    err = BlockSend::parse(payload, blockSend);
    //NOTE: we skip over the block counter so it doesn't appear in the file
    len = write(xfer->FD, blockSend.theData + sizeof(blockSend.theBlockCounter),
                     blockSend.theLength - sizeof(blockSend.theBlockCounter));
    nlREQUIRE_ACTION(len >= 0, handle_block_send_failed,
                 printf("Error: HandleBlockSend: Unable to read image into block\n"));

    PacketBuffer::Free(payload);
    payload = NULL;

    // Always need to ACK a BlockEOF
    if (msgType == kMsgType_BlockEOF)
    {
        printf("Sending BlockEOFAck");

        BlockEOFAck blockEOFAck;
        PacketBuffer* blockEOFAckPayload = PacketBuffer::New();
        nlREQUIRE_ACTION(blockEOFAckPayload, handle_block_send_failed,
                         err = WEAVE_ERROR_NO_MEMORY;
                         printf("Error: BDX HandleBlockSend: PacketBuffer alloc failed\n"));

        err = blockEOFAck.init(blockSend.theBlockCounter-1); //final ack uses same block-counter of last block-query request
        nlREQUIRE(err == WEAVE_NO_ERROR, handle_block_send_failed);

        err = blockEOFAck.pack(blockEOFAckPayload);
        nlREQUIRE(err == WEAVE_NO_ERROR, handle_block_send_failed);

        err = ec->SendMessage(kWeaveProfile_BDX, kMsgType_BlockEOFAck, blockEOFAckPayload);
        nlREQUIRE(err == WEAVE_NO_ERROR, handle_block_send_failed);

        blockEOFAckPayload = NULL;
        bdxApp->ShutdownTransfer(xfer, true);
    }

    // currently we only support synchronous mode, so send BlockAck
    else {
        printf("Sending BlockAck\n");

        BlockAck blockAck;
        PacketBuffer* blockAckPayload = PacketBuffer::New();
        nlREQUIRE_ACTION(blockAckPayload, handle_block_send_failed,
                         err = WEAVE_ERROR_NO_MEMORY;
                         printf("Error: BDX HandleBlockSend: PacketBuffer alloc failed\n"));

        err = blockAck.init(blockSend.theBlockCounter);
        nlREQUIRE(err == WEAVE_NO_ERROR, handle_block_send_failed);

        err = blockAck.pack(blockAckPayload);
        nlREQUIRE(err == WEAVE_NO_ERROR, handle_block_send_failed);

        err = ec->SendMessage(kWeaveProfile_BDX, kMsgType_BlockAck, blockAckPayload);
        nlREQUIRE(err == WEAVE_NO_ERROR, handle_block_send_failed);
        blockAckPayload = NULL;
    }

handle_block_send_failed:

    if (err != WEAVE_NO_ERROR)
    {
        bdxApp->ShutdownTransfer(xfer, true);
    }

    printf("HandleBlockSend exiting");
    return;
}

static int read_n(int fd, char *buf, int n)
{
    int nread, left = n;

    printf("0 read_n entering\n");

    while (left > 0)
    {
        printf("1 read_n (left: %d)\n", left);

        if ((nread = read(fd, buf, left)) > 0)
        {
            left -= nread;
            buf += nread;
            printf("2 read_n (nread: %d, left: %d)\n", nread, left);
        }
        else
        {
            printf("3 read_n (nread: 0, left: %d)\n", left);
            return n - left;
        }
    }

    printf("4 read_n (n: %d, left: %d) exiting\n", n, left);
    return n;
}

void BulkDataTransferServer::HandleBlockQueryRequest(ExchangeContext *ec, const IPPacketInfo *packetInfo,
        const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *payloadBlockQuery)
{
    printf("0 BDX HandleBlockQueryRequest entering\n");
    WEAVE_ERROR ret = WEAVE_NO_ERROR;
    char *block = NULL;
    int len = 0;

    BlockQuery blockQuery;
    BDXTransfer *xfer = (BDXTransfer* ) ec->AppState;
    BulkDataTransferServer *bdxApp = xfer->BdxApp;

    // Parse message data to get the block counter later
    BlockQuery::parse(payloadBlockQuery, blockQuery);

    PacketBuffer::Free(payloadBlockQuery);
    payloadBlockQuery = NULL;

    if (kWeaveProfile_BDX != profileId ||  kMsgType_BlockQuery != msgType)
    {
        printf("1 BDX HandleBlockQueryRequest bad msg type (%d, %d)\n", profileId, msgType);
        SendTransferError(ec, kWeaveProfile_Common, kStatus_BadRequest);
        goto handle_block_query_request_failed;
    }

    if ((xfer->BlockBuffer = PacketBuffer::New()) == NULL)
    {
        printf("2 BDX HandleBlockQueryRequest (PacketBuffer alloc failed)\n");
        SendTransferError(ec, kWeaveProfile_Common, kStatus_InternalServerProblem);
        goto handle_block_query_request_failed;
    }

    printf("3 BDX HandleBlockQueryRequest (xfer->FD: %d)\n", xfer->FD);

    block = (char *) xfer->BlockBuffer->Start();
    *block = blockQuery.theBlockCounter;
    len = read_n(xfer->FD, block + 1, xfer->MaxBlockSize);

    if (len == xfer->MaxBlockSize)
    {
        printf("4 BDX HandleBlockQueryRequest (len = %d)\n", len);
        xfer->BlockBuffer->SetDataLength((uint16_t) len + 1);

        // Prepare to handle next BlockQueryRequest.
        ec->OnMessageReceived = HandleBlockQueryRequest;

        // Send a BlockSend Response back to the sender.
        ret = ec->SendMessage(kWeaveProfile_BDX, kMsgType_BlockSend, xfer->BlockBuffer, ExchangeContext::kSendFlag_ExpectResponse);
        if (ret != WEAVE_NO_ERROR)
        {
            printf("5 BDX HandleBlockQueryRequest (SendMessage failed, err=%d)\n", ret);
            goto handle_block_query_request_failed;
        }
        xfer->BlockBuffer = NULL;
    }
    else if ((len >= 0) && (len < xfer-> MaxBlockSize))
    {
        printf("6 BDX HandleBlockQueryRequest (len == 0)\n");

        // At EOF, so send empty payload.
        xfer->BlockBuffer->SetDataLength((uint16_t) len + 1);

        // Prepare to handle BlockEOF ACK.
        ec->OnMessageReceived = HandleBlockEOFAck;

        // Send a BlockEOF Response back to the sender.
        ret = ec->SendMessage(kWeaveProfile_BDX, kMsgType_BlockEOF, xfer->BlockBuffer, ExchangeContext::kSendFlag_ExpectResponse);
        if (ret != WEAVE_NO_ERROR)
        {
            printf("7 BDX HandleBlockQueryRequest\n");
            goto handle_block_query_request_failed;
        }
        xfer->BlockBuffer = NULL;
    }
    else
    {
        printf("8 BDX HandleBlockQueryRequest read failed (len < 0)\n");
        // read() failed
        SendTransferError(ec, kWeaveProfile_Common, kStatus_InternalServerProblem);
        goto handle_block_query_request_failed;
    }

    printf("9 BDX HandleBlockQueryRequest exiting (success)\n");
    return;

handle_block_query_request_failed:
    printf("10 BDX HandleBlockQueryRequest exiting (failure)\n");
    bdxApp->ShutdownTransfer(xfer, true);
}

void BulkDataTransferServer::HandleBlockEOFAck(ExchangeContext *ec, const IPPacketInfo *packetInfo,
        const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *payload)
{
    printf("0 BDX HandleBlockEOFAck entering\n");
    BDXTransfer *xfer = (BDXTransfer* ) ec->AppState;
    BulkDataTransferServer *bdxApp = xfer->BdxApp;

    // Free unused query payload.
    PacketBuffer::Free(payload);
    payload = NULL;

    if (kWeaveProfile_BDX != profileId ||  kMsgType_BlockEOFAck != msgType)
    {
        printf("1 BDX HandleBlockEOFAck bad msg type (%d, %d)\n", profileId, msgType);
        SendTransferError(ec, kWeaveProfile_Common, kStatus_BadRequest);
    }
    else
    {
        // Set flag for connection closed handler
        xfer->CompletedSuccessfully = true;

        // Fire application callback
        if (bdxApp->OnBDXBlockEOFAckReceived)
        {
            bdxApp->OnBDXBlockEOFAckReceived(ec->PeerNodeId, ec->PeerAddr, payload, bdxApp->mpAppState);
        }
    }

    // Either way it's the end of the line
    bdxApp->ShutdownTransfer(xfer, true);

    printf("2 BDX HandleBlockEOFAck exiting\n");
}

void BulkDataTransferServer::HandleBDXConnectionClosed(ExchangeContext *ec, WeaveConnection *con, WEAVE_ERROR conErr)
{
    printf("0 BDX HandleBDXConnectionClosed entering (conErr = %d)\n", conErr);
    BDXTransfer *xfer = (BDXTransfer* ) ec->AppState;
    BulkDataTransferServer *bdxApp = xfer->BdxApp;
    bdxApp->ShutdownTransfer(xfer, false);
    printf("1 BDX HandleBDXConnectionClosed exiting\n");
}

void BulkDataTransferServer::HandleResponseTimeout(ExchangeContext *ec)
{
    printf("0 BDX HandleResponseTimeout entering\n");
    BDXTransfer *xfer = (BDXTransfer* ) ec->AppState;
    BulkDataTransferServer *bdxApp = xfer->BdxApp;
    bdxApp->ShutdownTransfer(xfer, true);
    printf("1 BDX HandleResponseTimeout exiting\n");
}

void BulkDataTransferServer::SendTransferError(ExchangeContext *ec, uint32_t aProfileId, uint16_t aStatusCode)
{
    TransferError transferError;
    transferError.init(aProfileId, aStatusCode);
    PacketBuffer* payloadTransferError = PacketBuffer::New();
    if (payloadTransferError == NULL)
    {
        printf("BDX SendTransferError (PacketBuffer alloc failed)\n");
        return;
    }

    transferError.pack(payloadTransferError);
    ec->SendMessage(kWeaveProfile_BDX, kMsgType_TransferError, payloadTransferError);
    payloadTransferError = NULL;
}

} // namespace Profiles
} // namespace Weave
} // namespace nl
