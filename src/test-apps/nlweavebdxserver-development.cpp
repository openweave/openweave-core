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

#include "Weave/Support/logging/WeaveLogging.h"

#include "nlweavebdxserver.h"
#include <unistd.h>
#include <fcntl.h>

#if HAVE_CURL_CURL_H
#include <curl/curl.h>
#endif

#if HAVE_CURL_EASY_H
#include <curl/easy.h>
#endif

int DownloadFile(char *aFileDeisgnator);
size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t read_data(char *ptr, size_t size, size_t n, FILE *stream);

namespace nl {
namespace Weave {
namespace Profiles {

using namespace nl::Weave::Logging;

BulkDataTransferServer::BulkDataTransferServer()
    : mpExchangeMgr(NULL)
    , mpAppState(NULL)
    , OnBDXReceiveInitRequestReceived(NULL)
    , OnBDXSendInitRequestReceived(NULL)
    , OnBDXBlockQueryRequestReceived(NULL)
    , OnBDXBlockSendReceived(NULL)
    , OnBDXBlockEOFAckReceived(NULL)
    , OnBDXTransferFailed(NULL)
    , OnBDXTransferSucceeded(NULL)
    , mpDelegate(NULL)
    , mHostedFileName(NULL)
    , mBDXDownloadCanRun(false)
{
    memset(mTransferPool, 0, sizeof(mTransferPool));
}

BulkDataTransferServer::~BulkDataTransferServer()
{
    Shutdown();
}

void
BulkDataTransferServer::SetDelegate(BulkDataTransferServerDelegate *pDelegate)
{
    mpDelegate = pDelegate;
}

BulkDataTransferServerDelegate *
BulkDataTransferServer::GetDelegate()
{
    return mpDelegate;
}

void BulkDataTransferServer::AllowBDXServerToRun(bool aEnable)
{
    mBDXDownloadCanRun = aEnable;
}

bool BulkDataTransferServer::CanBDXServerRun()
{
    return mBDXDownloadCanRun;
}

WEAVE_ERROR BulkDataTransferServer::Init(WeaveExchangeManager *exchangeMgr, void *appState)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    //
    // Error if already initialized.
    VerifyOrExit(mpExchangeMgr == NULL, err = WEAVE_ERROR_INCORRECT_STATE);

    mpExchangeMgr = exchangeMgr;
    mpAppState = appState;
    mHostedFileName = NULL;

    // Initialize connection pool
    memset(mTransferPool, 0, sizeof(mTransferPool));
    for (int i = 0; i < MAX_NUM_BDX_TRANSFERS; i++)
    {
        //TODO: call a BDXTransfer.Init() function to set defaults? block counter, etc.
        mTransferPool[i].FD = NULL;
    }

    // Register to receive unsolicited ReceiveInitiation messages from the exchange manager.
    mpExchangeMgr->RegisterUnsolicitedMessageHandler(kWeaveProfile_BDX, kMsgType_ReceiveInit, HandleReceiveInitRequest, this);
    mpExchangeMgr->RegisterUnsolicitedMessageHandler(kWeaveProfile_BDX, kMsgType_SendInit, HandleSendInitRequest, this);

exit:
    return err;
}

WEAVE_ERROR BulkDataTransferServer::Shutdown()
{
    Log(kLogModule_BDX, kLogCategory_Detail, "0 BDX Shutdown entering\n");

    if (mpExchangeMgr != NULL)
    {
        // Shutdown actions to perform only if BDX server initialized:

        mpExchangeMgr->UnregisterUnsolicitedMessageHandler(kWeaveProfile_BDX, kMsgType_ReceiveInit);
        mpExchangeMgr->UnregisterUnsolicitedMessageHandler(kWeaveProfile_BDX, kMsgType_SendInit);
        mpExchangeMgr = NULL;

        // Explicitly shut down transfers to free any held Weave resources
        for (int i = 0; i < MAX_NUM_BDX_TRANSFERS; i++)
        {
            ShutdownTransfer(&mTransferPool[i]);
        }
    }

    // Shutdown actions to perform even if BDX server uninitialized:

    mpAppState = NULL;
    OnBDXReceiveInitRequestReceived = NULL;
    OnBDXSendInitRequestReceived = NULL;
    OnBDXBlockQueryRequestReceived = NULL;
    OnBDXBlockSendReceived = NULL;
    OnBDXBlockEOFAckReceived = NULL;
    OnBDXTransferFailed = NULL;
    OnBDXTransferSucceeded = NULL;

    Log(kLogModule_BDX, kLogCategory_Detail, "1 BDX Shutdown exiting\n");
    return WEAVE_NO_ERROR;
}

BulkDataTransferServer::BDXTransfer *BulkDataTransferServer::NewTransfer()
{
    for (int i = 0; i < MAX_NUM_BDX_TRANSFERS; i++)
    {
        //TODO: verify this is thread-safe??
        if (!mTransferPool[i].BdxApp)
        {
            mTransferPool[i].BdxApp = this;
            return &mTransferPool[i];
        }
    }

    return NULL;
}

void BulkDataTransferServer::ShutdownTransfer(BDXTransfer *xfer)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    VerifyOrExit(xfer->BdxApp != NULL, err = WEAVE_ERROR_INCORRECT_STATE);

    Log(kLogModule_BDX, kLogCategory_Detail, "0 BDX ShutdownTransfer entering\n");
    uint64_t peerNodeId = kNodeIdNotSpecified;
    IPAddress peerAddr = IPAddress::Any;

    // Get values to send application callback
    if (xfer->EC && xfer->EC->Con)
    {
        peerNodeId = xfer->EC->Con->PeerNodeId;
        peerAddr = xfer->EC->Con->PeerAddr;
    }

    // Fire application callback
    if (!xfer->CompletedSuccessfully)
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

    // Release Weave resources
    if (xfer->EC)
    {
        Log(kLogModule_BDX, kLogCategory_Detail, "1 BDX ShutdownTransfer closing EC\n");

        if (xfer->EC->Con)
        {
            Log(kLogModule_BDX, kLogCategory_Detail, "2 BDX ShutdownTransfer closing Con\n");
            xfer->EC->Con->Close();
            xfer->EC->Con = NULL;
        }

        xfer->EC->Close();
        xfer->EC = NULL;
    }

    // Free pbuf
    if (xfer->BlockBuffer)
    {
        Log(kLogModule_BDX, kLogCategory_Detail, "3 BDX ShutdownTransfer closing BlockBuffer\n");
        PacketBuffer::Free(xfer->BlockBuffer);
        xfer->BlockBuffer = NULL;
    }

    // Close file
    if (xfer->FD)
    {
        Log(kLogModule_BDX, kLogCategory_Detail, "4 BDX ShutdownTransfer closing FD\n");
        if (fclose(xfer->FD) == EOF) {
            Log(kLogModule_BDX, kLogCategory_Error, "4.5 BDX ShutdownTransfer error closing file!\n");
        }

        xfer->FD = NULL;
    }

    // Reset and release transfer object
    xfer->mMaxBlockSize = 0;
    xfer->CompletedSuccessfully = false;
    xfer->BdxApp = NULL;

exit:
    if (err != WEAVE_NO_ERROR)
    {
        Log(kLogModule_BDX, kLogCategory_Detail, "BDX ShutdownTransfer exiting with error: %d", err);
    }
    else
    {
        Log(kLogModule_BDX, kLogCategory_Detail, "5 BDX ShutdownTransfer exiting");
    }
}

void BulkDataTransferServer::HandleReceiveInitRequest(ExchangeContext *ec, const IPPacketInfo *packetInfo,
                                                      const WeaveMessageInfo *msgInfo, uint32_t profileId,
                                                      uint8_t msgType, PacketBuffer *payloadReceiveInit)
{
    // We're guaranteed of the right message profile and type by the mpExchangeMgr.
    Log(kLogModule_BDX, kLogCategory_Detail, "0 BDX HandleReceiveInitRequest entering\n");

    WEAVE_ERROR ret = WEAVE_NO_ERROR;
    const uint8_t BDX_SERVER_TRANSFER_MODE = kMode_ReceiverDrive;
    BulkDataTransferServer *bdxApp = NULL;
    BDXTransfer *xfer = NULL;
    PacketBuffer *payload = NULL;
    char *fileDesignator = NULL;
    int retval = 0;

    ReceiveAccept receiveAccept;
    ReceiveReject receiveReject;
    ReceiveInit receiveInit;

    VerifyOrExit(ec != NULL,
                 Log(kLogModule_BDX, kLogCategory_Error, "0.5 BDX HandleReceiveInitRequest failed, null EC\n"));
    bdxApp = static_cast<BulkDataTransferServer *>(ec->AppState);

    VerifyOrExit(bdxApp->CanBDXServerRun(),
                 Log(kLogModule_BDX, kLogCategory_Error, "0.5 BDX HandleReceiveInitRequest failed, can't run!\n"));

    // Parse init request and discard payload buffer
    ret = ReceiveInit::parse(payloadReceiveInit, receiveInit);
    VerifyOrExit(ret == WEAVE_NO_ERROR,
                 Log(kLogModule_BDX, kLogCategory_Error, "0.5 BDX HandleReceiveInitRequest failed, error parsing\n"));
    PacketBuffer::Free(payloadReceiveInit);
    payloadReceiveInit = NULL;

    // Grab BDXTransfer object for this transfer
    xfer = bdxApp->NewTransfer();

    VerifyOrExit(xfer, Log(kLogModule_BDX,
                           kLogCategory_Error,
                           "1 BDX HandleReceiveInitRequest (transfer alloc failed)\n");
                       SendTransferError(ec, kWeaveProfile_Common, kStatus_OutOfMemory));

    // Hang new BDXTransfer on exchange context
    ec->AppState = xfer;

    // Initialize xfer struct (move to init function?)
    xfer->EC = ec;
    xfer->FD = NULL; // memset(0) doesn't set us up for ShutdownTransfer()

    if (receiveInit.mMaxBlockSize <= 0) {
        Log(kLogModule_BDX, kLogCategory_Error, "2 BDX HandleReceiveInitRequest (maxBlockSize <= 0)\n");

        // Send rejection status message
        receiveReject.init(kWeaveProfile_Common, kStatus_BadRequest);
        payload = PacketBuffer::New();
        receiveReject.pack(payload);
        ret = ec->SendMessage(kWeaveProfile_Common, kMsgType_ReceiveReject, payload);
        if (ret != WEAVE_NO_ERROR)
        {
            Log(kLogModule_BDX, kLogCategory_Error, "3 BDX HandleReceiveInitRequest err=%d\n", ret);
        }

        payload = NULL;
        goto exit;
    }
    xfer->mMaxBlockSize  = receiveInit.mMaxBlockSize;

    if (receiveInit.mFileDesignator.theLength <= 0) {
        Log(kLogModule_BDX, kLogCategory_Error, "4 BDX HandleReceiveInitRequest (bad FileDesignator)\n");
        SendTransferError(ec, kWeaveProfile_Common, kStatus_LengthTooShort);
        goto exit;
    }

    // // Copy file name onto C-string
    // // NOTE: the original string is not NUL terminated, but we know its length.
    fileDesignator = (char*)malloc(receiveInit.mFileDesignator.theLength + 1);
    memcpy(fileDesignator, receiveInit.mFileDesignator.theString, receiveInit.mFileDesignator.theLength);
    fileDesignator[receiveInit.mFileDesignator.theLength] = '\0';

#if BUILD_FEATURE_IMAGE_CACHE
    // // TODO Validate requested file path with value from nlhlfirmware.plist
    // // Future: delegate path security validation
    // // nlclient will open() this path as root, so we must be conservative in our validation.
    // if (0 != strcmp(fileDesignator, bdxApp->mHostedFileName))
    // {
    //     Log(kLogModule_BDX, kLogCategory_Error,  "5 BDX HandleReceiveInitRequest (forbidden FileDesignator)\n");
    //     SendTransferError(ec, kWeaveProfile_Common, kStatus_UnknownFile); //TODO add 'forbidden' Weave status code
    //     free(fileDesignator);
    //     fileDesignator = NULL;
    //     goto exit;
    // }
#else
    Log(kLogModule_BDX, kLogCategory_Detail, "BDX: Download URI : %s\n", fileDesignator);

    /**
     * Download file
     */
    retval = DownloadFile(fileDesignator);

    if (retval != CURLE_OK && fileDesignator != NULL)
    {
        Log(kLogModule_BDX, kLogCategory_Error, "BDX: Unable to download the file :%d\n", retval);
        free(fileDesignator);
        fileDesignator = NULL;

        receiveReject.init(kWeaveProfile_BDX, kStatus_UnknownFile);
        payload = PacketBuffer::New();
        receiveReject.pack(payload);
        ret = ec->SendMessage(kWeaveProfile_BDX, kMsgType_ReceiveReject, payload);
        if (ret != WEAVE_NO_ERROR)
        {
            Log(kLogModule_BDX, kLogCategory_Error, "8 BDX HandleReceiveInitRequest err=%d\n", ret);
        }

        payload = NULL;

        goto exit;
    }
#endif
    // Open file to send
    xfer->FD = fopen(fileDesignator, "r");
    if (xfer->FD == NULL)
    {
        SendTransferError(ec, kWeaveProfile_Common, kStatus_InternalServerProblem);
        goto exit;
    }

    Log(kLogModule_BDX, kLogCategory_Detail, "7 BDX HandleReceiveInitRequest validated request\n");

    // Fire application callback once we've validated the request (TODO: call earlier? feels like semantic abuse)
    if (bdxApp->OnBDXReceiveInitRequestReceived)
    {
        bdxApp->OnBDXReceiveInitRequestReceived(ec->PeerNodeId, ec->PeerAddr, payloadReceiveInit, bdxApp->mpAppState);
    }

    // Set up response timeout and connection closed handler
    ec->Con->AppState = xfer;
    ec->Con->OnConnectionClosed = HandleBDXConnectionClosed;
    ec->OnResponseTimeout = HandleResponseTimeout;
    ec->ResponseTimeout = BDX_RESPONSE_TIMEOUT_MS;

    // Set ourselves up to handle first BlockQueryRequest.
    ec->OnMessageReceived = HandleBlockQueryRequest;

    // Send a ReceiveAccept response back to the receiver.
    ret = receiveAccept.init(BDX_SERVER_TRANSFER_MODE, receiveInit.mMaxBlockSize, receiveInit.mLength, NULL);
    VerifyOrExit(ret == WEAVE_NO_ERROR, Log(kLogModule_BDX, kLogCategory_Progress, "7.5 BDX HandleReceiveInitRequest error initializing ReceiveAccept\n"));

    payload = PacketBuffer::New();
    ret = receiveAccept.pack(payload);
    VerifyOrExit(ret == WEAVE_NO_ERROR,
                 Log(kLogModule_BDX, kLogCategory_Progress, "7.5 BDX HandleReceiveInitRequest packing err=%d\n", ret));

    ret = ec->SendMessage(kWeaveProfile_BDX, kMsgType_ReceiveAccept, payload, ExchangeContext::kSendFlag_ExpectResponse);
    payload = NULL;
    if (ret != WEAVE_NO_ERROR)
    {
        Log(kLogModule_BDX, kLogCategory_Error, "8 BDX HandleReceiveInitRequest err=%d\n", ret);
        goto exit;
    }

    free(fileDesignator);
    fileDesignator = NULL;

    Log(kLogModule_BDX, kLogCategory_Detail, "9 BDX HandleReceiveInitRequest exiting (success)\n");
    return;

exit:
    Log(kLogModule_BDX, kLogCategory_Error, "10 BDX HandleReceiveInitRequest exiting (failure on code %d)\n", ret);

    if (fileDesignator)
    {
        free(fileDesignator);
        fileDesignator = NULL;
    }

    if (payloadReceiveInit)
    {
        PacketBuffer::Free(payloadReceiveInit);
    }

    if (payload)
    {
        PacketBuffer::Free(payload);
    }

    if (xfer)
    {
        bdxApp->ShutdownTransfer(xfer);
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
    Log(kLogModule_BDX, kLogCategory_Detail, "0 BDX HandleSendInitRequest entering\n");

    WEAVE_ERROR err = WEAVE_NO_ERROR;
    BulkDataTransferServer *bdxApp = NULL;
    BDXTransfer *xfer = NULL;

    SendInit sendInit;
    SendAccept sendAccept;
    SendReject sendReject;

    char *filename = NULL;
    PacketBuffer *SendInitResponsePayload;

    VerifyOrExit(ec != NULL,
                 Log(kLogModule_BDX, kLogCategory_Error, "HandleSendInitRequest failed: NULL EC!"));

    bdxApp = static_cast<BulkDataTransferServer *>(ec->AppState);

    xfer = bdxApp->NewTransfer();
    //TODO: move this other initialization logic to NewTransfer()?
    //xfer->mBlockCounter = 0;
    xfer->EC = ec;
    xfer->CompletedSuccessfully = false;
    // Hang the Transfer handle on the EC now instead of the whole app
    ec->AppState = (void*)xfer;

    VerifyOrExit(profileId == kWeaveProfile_BDX,
                 Log(kLogModule_BDX, kLogCategory_Error, "HandleSendInit failed: incorrect ProfileId"));
    VerifyOrExit(msgType == kMsgType_SendInit,
                 Log(kLogModule_BDX, kLogCategory_Error, "HandleSendInit failed: Incorrect msgType"));

    //TODO?
    //if (ec->Con == NULL)
        //xfer->mSendFlags = ExchangeContext::kSendFlag_ExpectResponse | ExchangeContext::kSendFlag_UseWRMP;
    //else
        //xfer->mSendFlags = ExchangeContext::kSendFlag_ExpectResponse;

    /**
     * Parse Send Init. Request
     */
    err = SendInit::parse(payload, sendInit);
    xfer->mMaxBlockSize = sendInit.mMaxBlockSize;
    VerifyOrExit(err == WEAVE_NO_ERROR,
                 Log(kLogModule_BDX, kLogCategory_Error, "Error: HandleSendInit: Unable to parse Send Init. request: %d", err));
    PacketBuffer::Free(payload);
    payload = NULL;

    /**
     * Allocate PacketBuffer
     */
    SendInitResponsePayload = PacketBuffer::New();
    VerifyOrExit(SendInitResponsePayload != NULL,
                 Log(kLogModule_BDX, kLogCategory_Progress, "Error: HandleSendInit: Unable to allocate PacketBuffer: %d", err));

    /**
     * Extract file name and open it for writing
     */
    VerifyOrExit(sendInit.mFileDesignator.theLength > 0,
                 Log(kLogModule_BDX, kLogCategory_Progress, "Error: HandleSendInit: No file name provided"));

    // build up the filename, which will be stored under /tmp, then open the file
    filename = (char*) malloc (sendInit.mFileDesignator.theLength + 1 + strlen(TEMP_FILE_LOCATION));
    strcpy(filename, TEMP_FILE_LOCATION);
    strncat(filename, sendInit.mFileDesignator.theString, sendInit.mFileDesignator.theLength);
    Log(kLogModule_BDX, kLogCategory_Detail, "Opening file %s for writing...", filename);

    xfer->FD = fopen(filename, "w");

    if (xfer->FD == NULL)  // Unable to open file for writing.  Already exists? TODO
    {
        Log(kLogModule_BDX, kLogCategory_Error, "Couldn't open file %s for writing...", filename);
        fclose(xfer->FD);
        free(filename);
        filename = NULL;

        sendReject.init(kWeaveProfile_BDX, kStatus_UnknownFile);
        sendReject.pack(SendInitResponsePayload);

        err = ec->SendMessage(kWeaveProfile_BDX, kMsgType_SendReject, SendInitResponsePayload, ExchangeContext::kSendFlag_ExpectResponse);
        SendInitResponsePayload = NULL;
        VerifyOrExit(err == WEAVE_NO_ERROR,
                     Log(kLogModule_BDX, kLogCategory_Error, "Error: HandleSendInit: Failed to send reject message: %d", err));
        VerifyOrExit(false,
                     Log(kLogModule_BDX, kLogCategory_Progress, "Send Init. Request rejected"));
    }

    free(filename);
    filename = NULL;

    // Determine transfer mode (TODO: non-hard coded?)
    xfer->mTransferMode = kMode_SenderDrive;
    VerifyOrExit(sendInit.mSenderDriveSupported,
                 Log(kLogModule_BDX, kLogCategory_Progress, "SendInitResponse error: SenderDrive mode not supported on client!"));

    // Finish up configuring and then send the SendInitResponse message
    sendAccept.init(xfer->mTransferMode, xfer->mMaxBlockSize, NULL);
    sendAccept.pack(SendInitResponsePayload);

    ec->OnMessageReceived = HandleBlockSend;

    err = ec->SendMessage(kWeaveProfile_BDX, kMsgType_SendAccept, SendInitResponsePayload,
                          ExchangeContext::kSendFlag_ExpectResponse);
    SendInitResponsePayload = NULL;
    VerifyOrExit(err == WEAVE_NO_ERROR,
                 Log(kLogModule_BDX, kLogCategory_Progress, "SendInitResponse error sending accept message: %d", err));

    return;

exit:

   Log(kLogModule_BDX, kLogCategory_Error, "10 BDX HandleSendInitRequest exiting (failure)\n");

    if (xfer)
    {
        xfer->BdxApp->ShutdownTransfer(xfer);
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

void BulkDataTransferServer::HandleBlockQueryRequest(ExchangeContext *ec, const IPPacketInfo *packetInfo,
        const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *payloadBlockQuery)
{
    Log(kLogModule_BDX, kLogCategory_Detail, "0 BDX HandleBlockQueryRequest entering\n");
    WEAVE_ERROR ret = WEAVE_NO_ERROR;
    char *block = NULL;
    int len;

    BDXTransfer *xfer = (BDXTransfer* ) ec->AppState;
    BulkDataTransferServer *bdxApp = xfer->BdxApp;

    // Free unused query payload.
    PacketBuffer::Free(payloadBlockQuery);
    payloadBlockQuery = NULL;

    if (kWeaveProfile_BDX != profileId ||  kMsgType_BlockQuery != msgType)
    {
        Log(kLogModule_BDX, kLogCategory_Error, "1 BDX HandleBlockQueryRequest bad msg type (%d, %d)\n", profileId, msgType);
        SendTransferError(ec, kWeaveProfile_Common, kStatus_BadRequest);
        goto exit;
    }

    //FIXME: should this be freed?  Or is setting to NULL good enough?
    if ((xfer->BlockBuffer = PacketBuffer::New()) == NULL)
    {
        Log(kLogModule_BDX, kLogCategory_Error, "2 BDX HandleBlockQueryRequest (PacketBuffer alloc failed)\n");
        SendTransferError(ec, kWeaveProfile_Common, kStatus_InternalServerProblem);
        goto exit;
    }

    Log(kLogModule_BDX, kLogCategory_Detail, "3 BDX HandleBlockQueryRequest");

    block = (char *) xfer->BlockBuffer->Start();
    len = read_data(block, 1, xfer->mMaxBlockSize, xfer->FD);
    xfer->BlockBuffer->SetDataLength((uint16_t) len);

    // EOF case first
    // TODO: we don't actually pack the payload using BlockSend/EOF objects
    // as we currently don't transmit block number
    if (len < xfer->mMaxBlockSize && len >= 0)
    {
        Log(kLogModule_BDX, kLogCategory_Detail, "6 BDX HandleBlockQueryRequest (BlockEOF, len = %d)\n", len);

        // Prepare to handle BlockEOF ACK.
        ec->OnMessageReceived = HandleBlockEOFAck;

        // Send a BlockEOF Response back to the sender.
        ret = ec->SendMessage(kWeaveProfile_BDX, kMsgType_BlockEOF, xfer->BlockBuffer, ExchangeContext::kSendFlag_ExpectResponse);
        if (ret != WEAVE_NO_ERROR)
        {
            Log(kLogModule_BDX, kLogCategory_Error, "7 BDX HandleBlockQueryRequest\n");
            goto exit;
        }

        xfer->BlockBuffer = NULL;
    }
    else if (len > 0)
    {
        Log(kLogModule_BDX, kLogCategory_Error, "4 BDX HandleBlockQueryRequest (len = %d)\n", len);

        // Prepare to handle next BlockQueryRequest.
        ec->OnMessageReceived = HandleBlockQueryRequest;

        // Send a BlockSend Response back to the sender.
        ret = ec->SendMessage(kWeaveProfile_BDX, kMsgType_BlockSend, xfer->BlockBuffer, ExchangeContext::kSendFlag_ExpectResponse);
        if (ret != WEAVE_NO_ERROR)
        {
            Log(kLogModule_BDX, kLogCategory_Detail, "5 BDX HandleBlockQueryRequest (SendMessage failed, err=%d)\n", ret);
            goto exit;
        }

        xfer->BlockBuffer = NULL;
    }
    else
    {
        Log(kLogModule_BDX, kLogCategory_Error, "8 BDX HandleBlockQueryRequest read failed (len < 0)\n");
        // read() failed
        SendTransferError(ec, kWeaveProfile_Common, kStatus_InternalServerProblem);
        goto exit;
    }

    Log(kLogModule_BDX, kLogCategory_Detail, "9 BDX HandleBlockQueryRequest exiting (success)\n");
    return;

exit:
    Log(kLogModule_BDX, kLogCategory_Error, "10 BDX HandleBlockQueryRequest exiting (failure)\n");
    bdxApp->ShutdownTransfer(xfer);
}

void BulkDataTransferServer::HandleBlockSend(ExchangeContext *ec, const IPPacketInfo *packetInfo, const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *payload)
{
    Log(kLogModule_BDX, kLogCategory_Detail, "0 BDX HandleBlockSend entering\n");

    WEAVE_ERROR err = WEAVE_NO_ERROR;
    const char *errMsg = NULL;

    int len = 0;

    BlockSend blockSend;
    BDXTransfer *xfer = static_cast<BDXTransfer *>(ec->AppState);

    VerifyOrExit(ec->Con != NULL,
                 Log(kLogModule_BDX, kLogCategory_Error, "Error: HandleBlockSend: Connection is NULL!"));

    VerifyOrExit(profileId == kWeaveProfile_BDX,
                 Log(kLogModule_BDX, kLogCategory_Error, "Error: HandleBlockSend: Incorrect ProfileId"));

    VerifyOrExit(msgType == kMsgType_BlockSend or msgType == kMsgType_BlockEOF,
                 Log(kLogModule_BDX, kLogCategory_Error, "Error: HandleBlockSend: Incorrect MsgType"));

    //DumpMemory(payload->Start(), payload->DataLength(), "--> ", 16);

    // Parse message data to get the block counter later
    err = BlockSend::parse(payload, blockSend);
    VerifyOrExit(err == WEAVE_NO_ERROR,
                 Log(kLogModule_BDX, kLogCategory_Error, "Error: HandleBlockSend: Error parsing BlockSend"));

    //block = (uint8_t*) malloc (xfer->mMaxBlockSize);

    VerifyOrExit(xfer->FD != NULL,
                 Log(kLogModule_BDX, kLogCategory_Error, "Error: HandleBlockSend: File handle is NULL!"));
    //NOTE: we skip over the block counter so it doesn't appear in the file
    len = write_data(blockSend.mData + sizeof(blockSend.mBlockCounter), 1,
                     blockSend.mLength - sizeof(blockSend.mBlockCounter), xfer->FD);
    VerifyOrExit(len >= 0,
                 Log(kLogModule_BDX, kLogCategory_Error, "Error: HandleBlockSend: Unable to read image into block"));

    PacketBuffer::Free(payload);
    payload = NULL;

    // Always need to ACK a BlockEOF
    if (msgType == kMsgType_BlockEOF)
    {
        Log(kLogModule_BDX, kLogCategory_Detail, "Sending BlockEOFAck");

        BlockEOFAck blockEOFAck;
        //TODO: deal with the block counter not being sent optionally
        blockEOFAck.init(blockSend.mBlockCounter-1); //final ack uses same block-counter of last block-query request
        PacketBuffer* blockEOFAckPayload = PacketBuffer::New();
        blockEOFAck.pack(blockEOFAckPayload);

        err = ec->SendMessage(kWeaveProfile_BDX, kMsgType_BlockEOFAck, blockEOFAckPayload);
        VerifyOrExit(err == WEAVE_NO_ERROR,
                     Log(kLogModule_BDX, kLogCategory_Error, "Error: HandleBlockSend: Failed to send message: err=%d", err));
        blockEOFAckPayload = NULL;

        xfer->BdxApp->ShutdownTransfer(xfer);
    }

    // currently we only support synchronous mode, so send BlockAck
    else {
        Log(kLogModule_BDX, kLogCategory_Detail, "Sending BlockAck");

        BlockAck blockAck;
        blockAck.init(blockSend.mBlockCounter);
        PacketBuffer* blockAckPayload = PacketBuffer::New();
        blockAck.pack(blockAckPayload);

        err = ec->SendMessage(kWeaveProfile_BDX, kMsgType_BlockAck, blockAckPayload);
        VerifyOrExit(err == WEAVE_NO_ERROR,
                     Log(kLogModule_BDX, kLogCategory_Error, "Error: HandleBlockSend: Failed to send message: err=%d", err));
        blockAckPayload = NULL;
    }

exit:

    if (err != WEAVE_NO_ERROR)
    {
        Log(kLogModule_BDX, kLogCategory_Progress, errMsg);
        xfer->BdxApp->ShutdownTransfer(xfer);
    }

    Log(kLogModule_BDX, kLogCategory_Detail, "HandleBlockSend exiting");
    return;
}

void BulkDataTransferServer::HandleBlockEOFAck(ExchangeContext *ec, const IPPacketInfo *packetInfo,
        const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *payload)
{
    Log(kLogModule_BDX, kLogCategory_Detail, "0 BDX HandleBlockEOFAck entering\n");
    BDXTransfer *xfer = (BDXTransfer *) ec->AppState;
    BulkDataTransferServer *bdxApp = xfer->BdxApp;

    // Free unused query payload.
    PacketBuffer::Free(payload);
    payload = NULL;

    if (kWeaveProfile_BDX != profileId ||  kMsgType_BlockEOFAck != msgType)
    {
        Log(kLogModule_BDX, kLogCategory_Error, "1 BDX HandleBlockEOFAck bad msg type (%d, %d)\n", profileId, msgType);
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
    bdxApp->ShutdownTransfer(xfer);

    Log(kLogModule_BDX, kLogCategory_Detail, "2 BDX HandleBlockEOFAck exiting\n");
}

void BulkDataTransferServer::HandleBDXConnectionClosed(WeaveConnection *con, WEAVE_ERROR conErr)
{
    Log(kLogModule_BDX, kLogCategory_Detail, "0 BDX HandleBDXConnectionClosed entering (conErr = %d)\n", conErr);
    BDXTransfer *xfer = (BDXTransfer *) con->AppState;
    BulkDataTransferServer *bdxApp = xfer->BdxApp;
    bdxApp->ShutdownTransfer(xfer);
    Log(kLogModule_BDX, kLogCategory_Detail, "1 BDX HandleBDXConnectionClosed exiting\n");
}

void BulkDataTransferServer::HandleResponseTimeout(ExchangeContext *ec)
{
    Log(kLogModule_BDX, kLogCategory_Detail, "0 BDX HandleResponseTimeout entering\n");
    BDXTransfer *xfer = (BDXTransfer *) ec->AppState;
    BulkDataTransferServer *bdxApp = xfer->BdxApp;
    bdxApp->ShutdownTransfer(xfer);
    Log(kLogModule_BDX, kLogCategory_Detail, "1 BDX HandleResponseTimeout exiting\n");
}

void BulkDataTransferServer::SendTransferError(ExchangeContext *ec, uint32_t aProfileId, uint16_t aStatusCode)
{
    TransferError transferError;
    transferError.init(aProfileId, aStatusCode);
    PacketBuffer* payloadTransferError = PacketBuffer::New();
    transferError.pack(payloadTransferError);
    ec->SendMessage(kWeaveProfile_BDX, kMsgType_TransferError, payloadTransferError);
    payloadTransferError = NULL;
}

} // namespace Profiles
} // namespace Weave
} // namespace nl

#if !defined(BUILD_FEATURE_IMAGE_CACHE)
size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}

// Unclear as to why we need the while loop in here.  Perhaps for
// handling streams that are being added to as we consume from them?
size_t read_data(char *ptr, size_t size, size_t n, FILE *stream)
{
    int nread, left = n;

    Log(kLogModule_BDX, kLogCategory_Detail, "0 read_n entering\n");

    while (left > 0)
    {
        Log(kLogModule_BDX, kLogCategory_Detail, "1 read_n (left: %d)\n", left);

        if ((nread = fread(ptr, size, n, stream)) > 0)
        {
            left -= nread;
            ptr += nread;
            Log(kLogModule_BDX, kLogCategory_Detail, "2 read_n (nread: %d, left: %d)\n", nread, left);
        }
        else
        {
            Log(kLogModule_BDX, kLogCategory_Detail, "3 read_n (nread: 0, left: %d)\n", left);
            return n - left;
        }
    }

    Log(kLogModule_BDX, kLogCategory_Detail, "4 read_n (n: %d, left: %d) exiting\n", n, left);
    return n;
}


int DownloadFile(char *aFileDesignator)
{
    CURL *curl;
    FILE *fp;
    CURLcode res = CURLE_FAILED_INIT;
    char *pch = NULL, *file_name = NULL;
    char *download_url = (char *) malloc(1 + strlen(aFileDesignator));

    strcpy(download_url, aFileDesignator);

    // Extract the file name out of the download URL
    pch = strtok(aFileDesignator,"/");
    while (pch != NULL)
    {
        file_name = pch;
        pch = strtok(NULL, "/");
    }

    char outfilename[FILENAME_MAX] = TEMP_FILE_LOCATION;
    strcat(outfilename, file_name);

    if (access(outfilename, F_OK) != -1)
    {
        strcpy(aFileDesignator, outfilename);
        return 0;
    }
    else
    {
        curl = curl_easy_init();
        if (curl)
        {
            fp = fopen(outfilename,"wb");
            Log(kLogModule_BDX, kLogCategory_Error, "BDX: Downloading Image : |%s|\n", download_url);
            curl_easy_setopt(curl, CURLOPT_URL, download_url);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
            res = curl_easy_perform(curl);

            /* always cleanup */
            curl_easy_cleanup(curl);
            fclose(fp);

            if (res != CURLE_OK)
            {
                remove(outfilename);
                aFileDesignator = NULL;
            }
            else
            {
                /*TODO: Cleanup old Pinna image files : SUN-918 */
                strcpy(aFileDesignator, outfilename);
            }
        }
        else
        {
            Log(kLogModule_BDX, kLogCategory_Error, "BDX: Failed to initialize curl\n");
            aFileDesignator = NULL;
        }
    }

    free(download_url);
    download_url = NULL;

    return res;
}

#endif
