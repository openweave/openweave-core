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
 *      This file contains definitions for the Weave Bulk Data Transfer
 *      Protocol.  It is set up as a collection of event handlers for processing incoming BDX
 *      Weave messages and responding appropriately. It does not maintain any state
 *      but instead operates on the state-carrying BDXTransfer object, which should
 *      be managed by the BdxNode.
 *
 */

#include <Weave/Support/CodeUtils.h>
#include <Weave/Support/logging/WeaveLogging.h>
#include <Weave/Core/WeaveEncoding.h>
#include <Weave/Core/WeaveServerBase.h>
#include <Weave/Profiles/bulk-data-transfer/Development/BDXProtocol.h>
#include <Weave/Support/WeaveFaultInjection.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(BDX, kWeaveManagedNamespaceDesignation_Development) {
namespace BdxProtocol {

using namespace ::nl::Weave::Profiles::StatusReporting;
using namespace nl::Weave::Logging;

#if WEAVE_CONFIG_BDX_CLIENT_RECEIVE_SUPPORT
/**
 * @brief
 *  Sends a ReceiveInit message for the given parameters and initiated BDXTransfer
 *
 * param[in]    aXfer           The transfer state object representing this new transfer
 * param[in]    aICanDrive      True if the initiator should propose that it drive,
 *                              false otherwise
 * param[in]    aUCanDrive      True if the initiator should propose that the sender drive,
 * param[in]    aAsyncOk        True if the initiator should propose using async transfer
 * param[in]    aMetaData       (optional) TLV metadata
 *
 * @retval      #WEAVE_ERROR_NO_MEMORY      If we could not get an PacketBuffer for sending the message
 * @retval      #WEAVE_NO_ERROR             If the message was successfully sent
 */
WEAVE_ERROR InitBdxReceive(BDXTransfer &aXfer, bool aICanDrive, bool aUCanDrive,
                           bool aAsyncOk, ReferencedTLVData *aMetaData)
{
    WEAVE_ERROR     err;
    ReceiveInit     msg;
    PacketBuffer*   buffer = PacketBuffer::New();
    uint16_t        flags;

    VerifyOrExit(buffer != NULL, err = WEAVE_ERROR_NO_MEMORY);

    if (aXfer.mIsWideRange)
    {
        err = msg.init(WEAVE_CONFIG_BDX_VERSION, aUCanDrive, aICanDrive, aAsyncOk, aXfer.mMaxBlockSize, aXfer.mStartOffset, aXfer.mLength, aXfer.mFileDesignator, aMetaData);
        SuccessOrExit(err);
    }
    else
    {
        err = msg.init(WEAVE_CONFIG_BDX_VERSION, aUCanDrive, aICanDrive, aAsyncOk, aXfer.mMaxBlockSize, (uint32_t) aXfer.mStartOffset, (uint32_t) aXfer.mLength, aXfer.mFileDesignator, aMetaData);
        SuccessOrExit(err);
    }

    err = msg.pack(buffer);
    SuccessOrExit(err);

    flags = aXfer.GetDefaultFlags(true);

    err = aXfer.mExchangeContext->SendMessage(kWeaveProfile_BDX, kMsgType_ReceiveInit, buffer, flags);
    SuccessOrExit(err);
    buffer = NULL;

exit:
    if (buffer != NULL)
    {
        PacketBuffer::Free(buffer);
    }

    return err;
}
#endif // WEAVE_CONFIG_BDX_CLIENT_RECEIVE_SUPPORT

#if WEAVE_CONFIG_BDX_CLIENT_SEND_SUPPORT
/**
 * @brief
 *  Sends a SendInit message for the given parameters and initiated BDXTransfer:
 *
 * @param[in]   aXfer       The transfer state object representing this new transfer
 * @param[in]   aICanDrive  True if the initiator should propose that it drive,
 *                          false otherwise
 * @param[in]   aUCanDrive  True if the initiator should propose that its counterpart drive,
 *                          false otherwise
 * @param[in]   aSynchOK    True if the initiator can do asynchronous mode, false otherwise
 * @param[in]   aMetaData   (optional) TLV metadata
 *
 * @retval      #WEAVE_ERROR_NO_MEMORY      If we could not get an PacketBuffer for sending the message
 * @retval      #WEAVE_NO_ERROR             If the message was successfully sent
 */
WEAVE_ERROR InitBdxSend(BDXTransfer &aXfer, bool aICanDrive, bool aUCanDrive,
                        bool aAsyncOk, ReferencedTLVData *aMetaData)
{
    WEAVE_ERROR     err;
    SendInit        msg;
    PacketBuffer*   buffer = PacketBuffer::New();
    uint16_t        flags;

    VerifyOrExit(buffer != NULL, err = WEAVE_ERROR_NO_MEMORY);

    if (aXfer.mIsWideRange)
    {
        err = msg.init(WEAVE_CONFIG_BDX_VERSION, aICanDrive, aUCanDrive, aAsyncOk, aXfer.mMaxBlockSize, aXfer.mStartOffset, aXfer.mLength, aXfer.mFileDesignator, aMetaData);
        SuccessOrExit(err);
    }
    else
    {
        err = msg.init(WEAVE_CONFIG_BDX_VERSION, aICanDrive, aUCanDrive, aAsyncOk, aXfer.mMaxBlockSize, (uint32_t)aXfer.mStartOffset, (uint32_t) aXfer.mLength, aXfer.mFileDesignator, aMetaData);
        SuccessOrExit(err);
    }

    err = msg.pack(buffer);
    SuccessOrExit(err);

    flags = aXfer.GetDefaultFlags(true);

    err = aXfer.mExchangeContext->SendMessage(kWeaveProfile_BDX, kMsgType_SendInit, buffer, flags);
    SuccessOrExit(err);
    buffer = NULL;

exit:
    if (buffer != NULL)
    {
        PacketBuffer::Free(buffer);
    }

    return err;
}

/**
 * @brief
 *  Sends a SendInit message for the given parameters and initiated BDXTransfer:
 *
 * @param[in]   aXfer       The transfer state object representing this new transfer
 * @param[in]   aICanDrive  True if the initiator should propose that it drive,
 *                          false otherwise
 * @param[in]   aUCanDrive  True if the initiator should propose that its counterpart drive,
 *                          false otherwise
 * @param[in]   aSynchOK    True if the initiator can do asynchronous mode, false otherwise
 * @param[in]   aMetaDataWriteCallback   (optional) Function to write TLV metadata
 * @param[in]   aMetaDataAppState        (optional) AppState for aMetaDataWriteCallback
 *
 * @retval      #WEAVE_ERROR_NO_MEMORY      If we could not get an PacketBuffer for sending the message
 * @retval      #WEAVE_NO_ERROR             If the message was successfully sent
 */
WEAVE_ERROR InitBdxSend(BDXTransfer &aXfer, bool aICanDrive, bool aUCanDrive,
                        bool aAsyncOk, SendInit::MetaDataTLVWriteCallback aMetaDataWriteCallback,
                        void *aMetaDataAppState)
{
    WEAVE_ERROR     err;
    SendInit        msg;
    PacketBuffer*   buffer = PacketBuffer::New();
    uint16_t        flags;

    VerifyOrExit(buffer != NULL, err = WEAVE_ERROR_NO_MEMORY);

    if (aXfer.mIsWideRange)
    {
        err = msg.init(WEAVE_CONFIG_BDX_VERSION, aICanDrive, aUCanDrive, aAsyncOk, aXfer.mMaxBlockSize, aXfer.mStartOffset, aXfer.mLength, aXfer.mFileDesignator, aMetaDataWriteCallback, aMetaDataAppState);
        SuccessOrExit(err);
    }
    else
    {
        err = msg.init(WEAVE_CONFIG_BDX_VERSION, aICanDrive, aUCanDrive, aAsyncOk, aXfer.mMaxBlockSize, (uint32_t)aXfer.mStartOffset, (uint32_t) aXfer.mLength, aXfer.mFileDesignator, aMetaDataWriteCallback, aMetaDataAppState);
        SuccessOrExit(err);
    }

    err = msg.pack(buffer);
    SuccessOrExit(err);

    flags = aXfer.GetDefaultFlags(true);

    err = aXfer.mExchangeContext->SendMessage(kWeaveProfile_BDX, kMsgType_SendInit, buffer, flags);
    SuccessOrExit(err);
    buffer = NULL;

exit:
    if (buffer != NULL)
    {
        PacketBuffer::Free(buffer);
    }

    return err;
}
#endif // WEAVE_CONFIG_BDX_CLIENT_SEND_SUPPORT

WEAVE_ERROR SendBadBlockCounterStatusReport(BDXTransfer & aXfer)
{
    SendStatusReport(aXfer.mExchangeContext, kWeaveProfile_BDX, kStatus_BadBlockCounter);

    return WEAVE_NO_ERROR;
}

#if WEAVE_CONFIG_BDX_V0_SUPPORT
/**
 * @brief
 *  This function sends a BlockQuery message for the given BDXTransfer.  The requested
 *  block number is equal to aXfer.mBlockCounter.
 *
 * @param[in]      aXfer        The BDXTransfer we're sending a BlockQuery for.
 *
 * @retval         #WEAVE_NO_ERROR          If we successfully sent the message.
 * @retval         #WEAVE_ERROR_NO_MEMORY   If no available PacketBuffers.
 */
WEAVE_ERROR SendBlockQuery(BDXTransfer &aXfer)
{
    WEAVE_ERROR     err     = WEAVE_NO_ERROR;
    PacketBuffer*   buffer  = PacketBuffer::NewWithAvailableSize(BlockQuery::kPayloadLen);
    BlockQuery      outMsg;
    uint16_t        flags;

    VerifyOrExit(buffer != NULL, err = WEAVE_ERROR_NO_MEMORY);

    err = outMsg.init(static_cast<uint8_t>(aXfer.mBlockCounter));
    SuccessOrExit(err);

    err = outMsg.pack(buffer);
    SuccessOrExit(err);

    flags = aXfer.GetDefaultFlags(true);

    err = aXfer.mExchangeContext->SendMessage(kWeaveProfile_BDX, kMsgType_BlockQuery, buffer, flags);
    buffer = NULL;

exit:
    if (buffer != NULL)
    {
        PacketBuffer::Free(buffer);
    }

    return err;
}
#endif // WEAVE_CONFIG_BDX_V0_SUPPORT

/**
 * @brief
 *  This function sends a BlockQueryV1 message for the given BDXTransfer.  The requested
 *  block number is equal to aXfer.mBlockCounter.
 *
 * @param[in]      aXfer        The BDXTransfer we're sending a BlockQuery for.
 *
 * @retval         #WEAVE_NO_ERROR          If we successfully sent the message.
 * @retval         #WEAVE_ERROR_NO_MEMORY   If no available PacketBuffers.
 */
WEAVE_ERROR SendBlockQueryV1(BDXTransfer &aXfer)
{
    WEAVE_ERROR     err     = WEAVE_NO_ERROR;
    PacketBuffer*   buffer  = PacketBuffer::NewWithAvailableSize(BlockQueryV1::kPayloadLen);
    BlockQueryV1    outMsg;
    uint16_t        flags;

    VerifyOrExit(buffer != NULL, err = WEAVE_ERROR_NO_MEMORY);

    SuccessOrExit(err = outMsg.init(aXfer.mBlockCounter));
    SuccessOrExit(err = outMsg.pack(buffer));

    flags = aXfer.GetDefaultFlags(true);

    err = aXfer.mExchangeContext->SendMessage(kWeaveProfile_BDX, kMsgType_BlockQueryV1, buffer, flags);
    buffer = NULL;

exit:
    if (buffer != NULL)
    {
        PacketBuffer::Free(buffer);
    }

    return err;
}

#if WEAVE_CONFIG_BDX_V0_SUPPORT
/**
 * @brief
 *  This function sends a BlockAck message for the given BDXTransfer.
 *  The acknowledged block number is equal to aXfer.mBlockCounter - 1
 *  as this function may only be called after the state transfer
 *  advanced to the next counter.
 *
 * @param[in]       aXfer       The BDXTransfer we're sending a BlockAck for.
 *
 * @retval          #WEAVE_NO_ERROR         If we successfully sent the message.
 * @retval          #WEAVE_ERROR_NO_MEMORY  If no available PacketBuffers.
 */
static WEAVE_ERROR SendBlockAck(BDXTransfer &aXfer)
{
    WEAVE_ERROR     err     = WEAVE_NO_ERROR;
    PacketBuffer*   buffer  = PacketBuffer::NewWithAvailableSize(BlockQuery::kPayloadLen);
    BlockAck        outMsg;
    uint16_t        flags;

    VerifyOrExit(buffer != NULL, err = WEAVE_ERROR_NO_MEMORY);

    SuccessOrExit(err = outMsg.init(static_cast<uint8_t>(aXfer.mBlockCounter - 1)));
    SuccessOrExit(err = outMsg.pack(buffer));

    flags = aXfer.GetDefaultFlags(false);

    err = aXfer.mExchangeContext->SendMessage(kWeaveProfile_BDX, kMsgType_BlockAck, buffer, flags);
    buffer = NULL;

exit:
    if (buffer != NULL)
    {
        PacketBuffer::Free(buffer);
    }

    return err;
}
#endif // WEAVE_CONFIG_BDX_V0_SUPPORT

/**
 * @brief
 *  This function sends a BlockAckV1 message for the given
 *  BDXTransfer.  The acknowledged block number is equal to
 *  aXfer.mBlockCounter - 1 as this function may only be called after
 *  the transfer state advanced to the next counter.
 *
 * @param[in]       aXfer       The BDXTransfer we're sending a BlockAck for.
 *
 * @retval          #WEAVE_NO_ERROR         If we successfully sent the message.
 * @retval          #WEAVE_ERROR_NO_MEMORY  If no available PacketBuffers.
 */
static WEAVE_ERROR SendBlockAckV1(BDXTransfer &aXfer)
{
    WEAVE_ERROR     err     = WEAVE_NO_ERROR;
    PacketBuffer*   buffer  = PacketBuffer::NewWithAvailableSize(BlockQueryV1::kPayloadLen);
    BlockAckV1      outMsg;
    uint16_t        flags;

    VerifyOrExit(buffer != NULL, err = WEAVE_ERROR_NO_MEMORY);

    SuccessOrExit(err = outMsg.init(aXfer.mBlockCounter - 1));
    SuccessOrExit(err = outMsg.pack(buffer));

    flags = aXfer.GetDefaultFlags(false);

    err = aXfer.mExchangeContext->SendMessage(kWeaveProfile_BDX, kMsgType_BlockAckV1, buffer, flags);
    buffer = NULL;

exit:
    if (buffer != NULL)
    {
        PacketBuffer::Free(buffer);
    }

    return err;
}

#if WEAVE_CONFIG_BDX_V0_SUPPORT
/**
 * @brief
 *  This function sends a BlockEOFAck message for the given BDXTransfer.  The acknowledged
 *  block number is equal to aXfer.mBlockCounter.
 *
 * @param[in]       aXfer       The BDXTransfer we're sending a BlockEOFAck for.
 *
 * @retval          #WEAVE_NO_ERROR         If we successfully sent the message.
 * @retval          #WEAVE_ERROR_NO_MEMORY  If no available PacketBuffers.
 */
static WEAVE_ERROR SendBlockEOFAck(BDXTransfer &aXfer)
{
    WEAVE_ERROR     err     = WEAVE_NO_ERROR;
    PacketBuffer*   buffer  = PacketBuffer::NewWithAvailableSize(BlockQuery::kPayloadLen);
    BlockEOFAck     outMsg;
    uint16_t        flags;

    VerifyOrExit(buffer != NULL, err = WEAVE_ERROR_NO_MEMORY);

    SuccessOrExit(err = outMsg.init(static_cast<uint8_t>(aXfer.mBlockCounter)));
    SuccessOrExit(err = outMsg.pack(buffer));

    flags = aXfer.GetDefaultFlags(false);

    err = aXfer.mExchangeContext->SendMessage(kWeaveProfile_BDX, kMsgType_BlockEOFAck, buffer, flags);
    buffer = NULL;

exit:
    if (buffer != NULL)
    {
        PacketBuffer::Free(buffer);
    }

    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogDetail(BDX, "SendBlockEOFAck failed.");
    }

    aXfer.mIsCompletedSuccessfully = true;
    aXfer.DispatchXferDoneHandler();

    return err;
}
#endif // WEAVE_CONFIG_BDX_V0_SUPPORT

/**
 * @brief
 *  This function sends a BlockEOFAckV1 message for the given BDXTransfer.  The acknowledged
 *  block number is equal to aXfer.mBlockCounter.
 *
 * @param[in]       aXfer       The BDXTransfer we're sending a BlockEOFAck for.
 *
 * @retval          #WEAVE_NO_ERROR         If we successfully sent the message.
 * @retval          #WEAVE_ERROR_NO_MEMORY  If no available PacketBuffers.
 */
static WEAVE_ERROR SendBlockEOFAckV1(BDXTransfer &aXfer)
{
    WEAVE_ERROR     err     = WEAVE_NO_ERROR;
    PacketBuffer*   buffer  = PacketBuffer::NewWithAvailableSize(BlockQueryV1::kPayloadLen);
    BlockEOFAckV1   outMsg;
    uint16_t        flags;

    VerifyOrExit(buffer != NULL, err = WEAVE_ERROR_NO_MEMORY);

    SuccessOrExit(err = outMsg.init(aXfer.mBlockCounter));
    SuccessOrExit(err = outMsg.pack(buffer));

    flags = aXfer.GetDefaultFlags(false);

    err = aXfer.mExchangeContext->SendMessage(kWeaveProfile_BDX, kMsgType_BlockEOFAckV1, buffer, flags);
    buffer = NULL;

exit:
    if (buffer != NULL)
    {
        PacketBuffer::Free(buffer);
    }

    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogDetail(BDX, "SendBlockEOFAckV1 failed.");
    }

    aXfer.mIsCompletedSuccessfully = true;
    aXfer.DispatchXferDoneHandler();

    return err;
}

#if WEAVE_CONFIG_BDX_V0_SUPPORT
/**
 * @brief
 *  This function sends the next BlockSend retrieved by calling the BDXTransfer's
 *  GetBlockHandler.
 *
 * @param[in]       aXfer   The BDXTransfer whose GetBlockHandler is called to get the
 *                          next block before sending it using the associated ExchangeContext
 *
 * @retval          #WEAVE_ERROR_INCORRECT_STATE    If the GetBlockHandler is NULL
 *
 * @note
 *   This function defers to SendBlock once the next block has been grabbed
 */
WEAVE_ERROR SendNextBlock(BDXTransfer &aXfer)
{
    WEAVE_ERROR     err         = WEAVE_NO_ERROR;
    uint64_t        length;
    uint8_t*        data;
    bool            isLast;
    uint8_t         msgType;
    uint8_t         counter;
    PacketBuffer*   buffer      = PacketBuffer::New();
    uint16_t        flags;

    counter = static_cast<uint8_t>(aXfer.mBlockCounter);
    WeaveLogDetail(BDX, "Sending next block # %hd\n", counter);

    VerifyOrExit(buffer != NULL, err = WEAVE_ERROR_NO_MEMORY);

    // Before anything, we need to ensure that we have a registered GetBlockHandler
    // or else length, data, and isLast won't be properly initialized and we'll
    // be sending hard-to-debug garbage out.  See WEAV-524.
    VerifyOrExit(aXfer.mHandlers.mGetBlockHandler != NULL, err = WEAVE_ERROR_INCORRECT_STATE);

    // TODO: this should probably have an error code so we can properly handle/log issues detected by the user's callback
    // OR we could at least define some contract about when we will actually send a block
    // vs. returning some error (e.g. not send unless length > 0 && data != NULL).
    // pack the message, no additional abstraction for now.

    data = buffer->Start();

    WEAVE_FAULT_INJECT(FaultInjection::kFault_BDXBadBlockCounter, counter++);
    nl::Weave::Encoding::Write8(data, counter);

    data += sizeof(counter);
    length = buffer->AvailableDataLength() - sizeof(counter);

    aXfer.DispatchGetBlockHandler(&length, &data, &isLast);

    // Ensure that we can fit the buffer within the PacketBuffer, fail
    // if we cannot.

    VerifyOrExit((length + sizeof(counter)) <= buffer->AvailableDataLength(), err = WEAVE_ERROR_BUFFER_TOO_SMALL);

    // if the data pointer has changed, the callee used her own
    // buffer.  Copy the contents.

    if (data != (buffer->Start() + sizeof(counter)))
    {
        memcpy(buffer->Start() + sizeof(counter), data, length);
    }

    buffer->SetDataLength(length + sizeof(counter));

    if (isLast)
    {
        msgType = kMsgType_BlockEOF;
    }
    else
    {
        msgType = kMsgType_BlockSend;
    }

    // TODO: for async aXfer, don't expect response. For now, we always expect an ACK or
    // another BlockQuery
    flags = aXfer.GetDefaultFlags(true);

    err = aXfer.mExchangeContext->SendMessage(kWeaveProfile_BDX, msgType, buffer, flags);
    buffer = NULL;

exit:
    if (buffer != NULL)
    {
        PacketBuffer::Free(buffer);
    }

    return err;
}
#endif // WEAVE_CONFIG_BDX_V0_SUPPORT

/**
 * @brief
 *  This function sends the next BlockSendV1 retrieved by calling the BDXTransfer's
 *  GetBlockHandler.
 *
 * @param[in]       aXfer   The BDXTransfer whose GetBlockHandler is called to get the
 *                          next block before sending it using the associated ExchangeContext
 *
 * @retval          #WEAVE_ERROR_INCORRECT_STATE    If the GetBlockHandler is NULL
 */
WEAVE_ERROR SendNextBlockV1(BDXTransfer &aXfer)
{
    WEAVE_ERROR     err         = WEAVE_NO_ERROR;
    uint64_t        length;
    uint8_t*        data;
    bool            isLast;
    uint8_t         msgType;
    PacketBuffer*   buffer      = PacketBuffer::New();
    uint16_t        flags;
    uint32_t        blockCounter;

    WeaveLogDetail(BDX, "Sending next block # %d\n", aXfer.mBlockCounter);

    VerifyOrExit(aXfer.mHandlers.mGetBlockHandler != NULL, err = WEAVE_ERROR_INCORRECT_STATE);

    VerifyOrExit(buffer != NULL, err = WEAVE_ERROR_NO_MEMORY);

    // pack the message, no additional abstraction for now.

    data = buffer->Start();

    blockCounter = aXfer.mBlockCounter;
    WEAVE_FAULT_INJECT(FaultInjection::kFault_BDXBadBlockCounter, blockCounter++);

    nl::Weave::Encoding::LittleEndian::Write32(data, blockCounter);

    length = buffer->AvailableDataLength() - sizeof(aXfer.mBlockCounter);

    if (length > aXfer.mMaxBlockSize)
    {
        length = aXfer.mMaxBlockSize;
    }

    aXfer.DispatchGetBlockHandler(&length, &data, &isLast);

    // Ensure that we can fit the buffer within the PacketBuffer, fail
    // if we cannot.

    VerifyOrExit((length + sizeof(aXfer.mBlockCounter)) <= buffer->AvailableDataLength(), err = WEAVE_ERROR_BUFFER_TOO_SMALL);

    // if the data pointer has changed, the callee used her own
    // buffer.  Copy the contents.

    if (data != (buffer->Start() + sizeof(aXfer.mBlockCounter)))
    {
        memcpy(buffer->Start() + sizeof(aXfer.mBlockCounter), data, length);
    }

    buffer->SetDataLength(length + sizeof(aXfer.mBlockCounter));

    if (isLast)
    {
        msgType = kMsgType_BlockEOFV1;
    }
    else
    {
        msgType = kMsgType_BlockSendV1;
    }

    // TODO: for async aXfer, don't expect response. For now, we always expect an ACK or
    // another BlockQuery
    flags = aXfer.GetDefaultFlags(true);

    err = aXfer.mExchangeContext->SendMessage(kWeaveProfile_BDX, msgType, buffer, flags);
    buffer = NULL;

exit:
    if (buffer != NULL)
    {
        PacketBuffer::Free(buffer);
    }

    return err;
}

/**
 * @brief
 *  The main handler for messages arriving on the BDX exchange.  It essentially
 *  acts as a router to extract the appropriate BDX header info and data,
 *  dispatching the appropriate handler to act on this object.
 *
 * @param[in]   anEc            The exchange context in case we need it
 * @param[in]   aPktInfo        Unused, but need to match the function prototype
 * @param[in]   aWeaveMsgInfo   Weave Message Information for the message
 * @param[in]   aProfileId      ID of the profile under which the message is defined
 * @param[in]   aMessageType    The message type of that profile
 * @param[in]   aPacketBuffer    The packed message itself
 */
void HandleResponse(ExchangeContext *anEc, const IPPacketInfo *aPktInfo, const WeaveMessageInfo *aWeaveMsgInfo,
                    uint32_t aProfileId, uint8_t aMessageType, PacketBuffer *aPacketBuffer)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;	// we'll use this later
    // extract the BDXTransfer object from the exchange context to access state
    BDXTransfer *xfer = static_cast<BDXTransfer *>(anEc->AppState);

    VerifyOrExit(aProfileId == kWeaveProfile_BDX || (aProfileId == kWeaveProfile_Common && aMessageType == Common::kMsgType_StatusReport), err = WEAVE_ERROR_INVALID_PROFILE_ID);
    VerifyOrExit(xfer->mIsInitiated, err = WEAVE_ERROR_INCORRECT_STATE);

    // (Re-)Initialize the next action to take
    xfer->mNext = NULL;

    if (xfer->mIsAccepted)
    {
        if (xfer->mAmSender)
        {
#if WEAVE_CONFIG_BDX_CLIENT_SEND_SUPPORT
            err = HandleResponseTransmit(*xfer, aProfileId, aMessageType, aPacketBuffer);
#endif // WEAVE_CONFIG_BDX_CLIENT_SEND_SUPPORT
        }
        else
        {
#if WEAVE_CONFIG_BDX_CLIENT_RECEIVE_SUPPORT
            err = HandleResponseReceive(*xfer, aProfileId, aMessageType, aPacketBuffer);
#endif // WEAVE_CONFIG_BDX_CLIENT_RECEIVE_SUPPORT
        }
    }
    else
    {
        err = HandleResponseNotAccepted(*xfer, aProfileId, aMessageType, aPacketBuffer);
    }

    PacketBuffer::Free(aPacketBuffer);
    aPacketBuffer = NULL;

    if (xfer->mNext)
    {
        err = xfer->mNext(*xfer);
        xfer->mNext = NULL;
    }

exit:
    if (aPacketBuffer != NULL)
        PacketBuffer::Free(aPacketBuffer);

    if (err != WEAVE_NO_ERROR)
    {
        xfer->DispatchErrorHandler(err);
    }
}

/**
 * @brief
 *  Handler for when the connection itself is closed. Calls the associated transfer's error handler
 *  and shuts down the transfer.
 *
 * @param[in]   anEc        Exchange context that detected a closed connection
 *                          We can find the associated BDXTransfer from this
 * @param[in]   aCon        The weave connection, unused in the actual function
 * @param[in]   aConErr     Error associated with the connection closing
 */
void HandleConnectionClosed(ExchangeContext *anEc, WeaveConnection *aCon, WEAVE_ERROR aConErr)
{
    BDXTransfer *xfer = static_cast<BDXTransfer *>(anEc->AppState);

    // If the other end closed the connection, pass WEAVE_ERROR_CONNECTION_CLOSED_UNEXPECTEDLY to the application if it didn't already have an error attached.
    if (aConErr == WEAVE_NO_ERROR)
    {
        aConErr = WEAVE_ERROR_CONNECTION_CLOSED_UNEXPECTEDLY;
    }

    // Forward the error to the app's error handler.
    xfer->DispatchErrorHandler(aConErr);
}

/**
 * @brief
 *  Handler for when we timeout waiting for a response. Shuts down the transfer that timed out,
 *  and calls that transfer's error handler.
 *
 * @param[in]   anEc    Exchange context which we can find the BDXTransfer from
 */
void HandleResponseTimeout(ExchangeContext *anEc)
{
    BDXTransfer *xfer = static_cast<BDXTransfer *>(anEc->AppState);

    WeaveLogDetail(BDX, "Exchange timed out while waiting for reply");
    xfer->DispatchErrorHandler(WEAVE_ERROR_TIMEOUT);
}

/**
 * @brief
 *  Handler for when the key used to encrypt and authenticate Weave messages is no longer usable.
 *
 * @param[in]   anEc        Exchange context that detected a key error
 *                          We can find the associated BDXTransfer from this
 * @param[in]   aKeyErr     Error associated with the key no longer being usable
 */
void HandleKeyError(ExchangeContext *anEc, WEAVE_ERROR aKeyErr)
{
    BDXTransfer *xfer = static_cast<BDXTransfer *>(anEc->AppState);

    WeaveLogDetail(BDX, "Encryption and authentication key became unusable");
    xfer->DispatchErrorHandler(aKeyErr);
}

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
/**
 * @brief
 *  Handler for when the WRMP message we sent was not acknowledged.
 *
 * @param[in]   anEc        Exchange context that had an unacknowleged message
 *                          We can find the associated BDXTransfer from this
 * @param[in]   aSendErr    Error associated with the message send failure
 * @param[in]   aMsgCtxt    An arbitrary message context that was associated
 *                          with the unacknowledged message.
 */
void HandleSendError(ExchangeContext *anEc, WEAVE_ERROR aSendErr, void *aMsgCtxt)
{
    BDXTransfer *xfer = static_cast<BDXTransfer *>(anEc->AppState);

    WeaveLogDetail(BDX, "WMRP message was not acknowledged");
    xfer->DispatchErrorHandler(aSendErr);
}
#endif // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING

#if WEAVE_CONFIG_BDX_V0_SUPPORT
/**
 * @brief
 *  Sends a transfer error message with the associated profile id, status code, and exchange context.
 *
 * @param[in]   anEc            The exchange context where we should be sending the transfer error message
 * @param[in]   aProfileId      Profile ID
 * @param[in]   aStatusCode     Code associated with the transfer error
 */
void SendTransferError(ExchangeContext *anEc, uint32_t aProfileId, uint16_t aStatusCode)
{
    TransferError   transferError;
    PacketBuffer*   payloadTransferError    = PacketBuffer::New();
    WEAVE_ERROR     err                     = WEAVE_NO_ERROR;

    VerifyOrExit(payloadTransferError != NULL,
                 WeaveLogDetail(BDX,
                               "Error (out of PacketBuffers) in SendTransferError"));

    err = transferError.init(aProfileId, aStatusCode);
    VerifyOrExit(err == WEAVE_NO_ERROR,
                 WeaveLogDetail(BDX,
                               "Error initializing TransferError: %d", err));

    err = transferError.pack(payloadTransferError);
    VerifyOrExit(err == WEAVE_NO_ERROR,
                 WeaveLogDetail(BDX,
                               "Error packing TransferError: %d", err));

    //TODO: open question: should we dispatch the callback when sending an error or only when receiving?
    //dispatchXferErrorHandler(aProfileId, aStatusCode);

    err = anEc->SendMessage(kWeaveProfile_BDX, kMsgType_TransferError, payloadTransferError, GetBDXAckFlag(anEc));
    payloadTransferError = NULL;

    VerifyOrExit(err == WEAVE_NO_ERROR,
                 WeaveLogDetail(BDX,
                               "Error sending TransferError message: %d", err));

exit:
    if (payloadTransferError != NULL)
        PacketBuffer::Free(payloadTransferError);
}
#endif // WEAVE_CONFIG_BDX_V0_SUPPORT

/**
 * @brief
 *  Sends a status report message with the associated profile id, status code, and exchange context.
 *
 * @param[in]   anEc            The exchange context where we should be sending the status report message
 * @param[in]   aProfileId      Profile ID
 * @param[in]   aStatusCode     Code associated with the transfer error
 */
void SendStatusReport(ExchangeContext *anEc, uint32_t aProfileId, uint16_t aStatusCode)
{
    WEAVE_ERROR     err             = WEAVE_NO_ERROR;

    //TODO: open question: should we dispatch the callback when sending an error or only when receiving?
    //dispatchXferErrorHandler(aProfileId, aStatusCode);

    err = nl::Weave::WeaveServerBase::SendStatusReport(anEc, aProfileId, aStatusCode, WEAVE_NO_ERROR, GetBDXAckFlag(anEc));
    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogDetail(BDX, "Error sending StatusReport message: %d", err);
    }
}

#if WEAVE_CONFIG_BDX_CLIENT_SEND_SUPPORT
/*
 * the transfer has been started and i'm the sender.
 * If I'm driving, I should expect an ACK from my counterpart
 * and should send the next block.
 * Otherwise, I should expect a BlockQuery and should respond
 * with the right block.
 */
WEAVE_ERROR HandleResponseTransmit(BDXTransfer &aXfer, uint32_t aProfileId, uint8_t aMessageType, PacketBuffer *aPacketBuffer)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint32_t rcvdCounter;

    if (aProfileId == kWeaveProfile_Common && aMessageType == Common::kMsgType_StatusReport)
    {
        StatusReport statusReport;

        err = StatusReport::parse(aPacketBuffer, statusReport);
        if (err == WEAVE_NO_ERROR)
        {
            aXfer.DispatchXferErrorHandler(&statusReport);
        }
    }
    else if (aProfileId == kWeaveProfile_BDX)
    {
        switch (aMessageType)
        {
#if WEAVE_CONFIG_BDX_V0_SUPPORT
            case kMsgType_BlockAck:
                /*
                 * Receiver MAY send an Ack before BlockQuery. We should
                 * free any resources associated with that block so we
                 * can stop resending it, but currently have no support
                 * for such activity.
                 * Similarly, we simply ignore Acks during async xfer.
                 */
                {
                    BlockAck ack;

                    // Don't set error here - simply ignore the message.
                    VerifyOrExit(aXfer.IsDriver() && !aXfer.IsAsync(), err = WEAVE_NO_ERROR);

                    err = BlockAck::parse(aPacketBuffer, ack);
                    VerifyOrExit(err == WEAVE_NO_ERROR, WeaveLogDetail(BDX, "BlockAck parse failed."));

                    rcvdCounter = ack.mBlockCounter;

                    if (rcvdCounter == static_cast<uint8_t>(aXfer.mBlockCounter))
                    {
                        // Update the counter and send the next block
                        aXfer.mBlockCounter++;
                        aXfer.mNext = SendNextBlock;
                    }
                    else if (rcvdCounter < static_cast<uint8_t>(aXfer.mBlockCounter))
                    {
                        // just ignore the packet
                        WeaveLogDetail(BDX, "Received BlockAck for old block: %hd\n", rcvdCounter);
                    }
                    else
                    {
                        // bad scene we've somehow fallen behind
                        WeaveLogDetail(BDX, "Received BlockAck for future block???: %d\n", rcvdCounter);
                    }
                }

                break;
#endif // WEAVE_CONFIG_BDX_V0_SUPPORT

            case kMsgType_BlockAckV1:
                {
                    BlockAckV1 ackV1;

                    VerifyOrExit(aXfer.IsDriver() && !aXfer.IsAsync(), err = WEAVE_NO_ERROR);

                    err = BlockAckV1::parse(aPacketBuffer, ackV1);
                    VerifyOrExit(err == WEAVE_NO_ERROR, WeaveLogDetail(BDX, "BlockAckV1 parse failed."));

                    rcvdCounter = ackV1.mBlockCounter;

                    if (rcvdCounter == aXfer.mBlockCounter)
                    {
                        // Update the counter and send the next block
                        aXfer.mBlockCounter++;
                        aXfer.mNext = SendNextBlockV1;
                    }
                    else
                    {
                        WeaveLogDetail(BDX, "Received bad block counter: %d, expected: %d", rcvdCounter, aXfer.mBlockCounter);

                        if (rcvdCounter > aXfer.mBlockCounter)
                        {
                            // Only send a status report if the block counter is later than expected, and ignore
                            // earlier than expected due to the possibility of duplicate messages
                            aXfer.mNext = SendBadBlockCounterStatusReport;
                        }
                    }
                }

                break;

#if WEAVE_CONFIG_BDX_V0_SUPPORT
            case kMsgType_BlockQuery:
                {
                    BlockQuery query;

                    VerifyOrExit(!aXfer.IsDriver() && !aXfer.IsAsync(), err = WEAVE_ERROR_INVALID_MESSAGE_TYPE);

                    err = BlockQuery::parse(aPacketBuffer, query);
                    VerifyOrExit(err == WEAVE_NO_ERROR, WeaveLogDetail(BDX, "BlockQuery parse failed."));

                    rcvdCounter = query.mBlockCounter;

                    // For the first query, look for counter = 0.
                    if (aXfer.mFirstQuery && rcvdCounter == 0)
                    {
                        aXfer.mFirstQuery = false;
                        aXfer.mNext = SendNextBlock;
                    }
                    // Afterwards, check that the received counter is the one after our block counter
                    else if (rcvdCounter == static_cast<uint8_t>(aXfer.mBlockCounter + 1))
                    {
                        // Increment block counter after verifying block query message + block counter if it isn't the first query
                        // This is because we need to stay on the same block counter if the receiver decides to send an ack
                        // Ex. Recv BlockQuery for #2, send Block #2, get ack back for #2. Need to have block counter on
                        // 2 to verify the ack, and only increment when we receive the next block query.
                        aXfer.mBlockCounter++;
                        aXfer.mNext = SendNextBlock;
                    }
                    else
                    {
                        // just ignore the packet
                        WeaveLogDetail(BDX, "Received bad block counter: %d, expected %d\n", rcvdCounter, aXfer.mBlockCounter + 1);
                    }
                }

                break;
#endif // WEAVE_CONFIG_BDX_V0_SUPPORT

            case kMsgType_BlockQueryV1:
                {
                    BlockQueryV1 queryV1;

                    VerifyOrExit(!aXfer.IsDriver() && !aXfer.IsAsync(), err = WEAVE_ERROR_INVALID_MESSAGE_TYPE);

                    err = BlockQueryV1::parse(aPacketBuffer, queryV1);
                    VerifyOrExit(err == WEAVE_NO_ERROR, WeaveLogDetail(BDX, "BlockQueryV1 parse failed."));

                    rcvdCounter = queryV1.mBlockCounter;

                    // For the first query, look for counter = 0.
                    if (aXfer.mFirstQuery && rcvdCounter == 0)
                    {
                        aXfer.mFirstQuery = false;
                        aXfer.mNext = SendNextBlockV1;
                    }
                    // Afterwards, check that the received counter is the one after our block counter
                    else if (rcvdCounter == aXfer.mBlockCounter + 1)
                    {
                        // Increment block counter after verifying block query message + block counter if it isn't the first query
                        // This is because we need to stay on the same block counter if the receiver decides to send an ack
                        // Ex. Recv BlockQuery for #2, send Block #2, get ack back for #2. Need to have block counter on
                        // 2 to verify the ack, and only increment when we receive the next block query.
                        aXfer.mBlockCounter++;
                        aXfer.mNext = SendNextBlockV1;
                    }
                    else
                    {
                        WeaveLogDetail(BDX, "Received bad block counter: %d, expected: %d", rcvdCounter, aXfer.mBlockCounter + 1);

                        if (rcvdCounter > aXfer.mBlockCounter + 1)
                        {
                            // Only send a status report if the block counter is later than expected, and ignore
                            // earlier than expected due to the possibility of duplicate messages
                            aXfer.mNext = SendBadBlockCounterStatusReport;
                        }
                    }
                }

                break;

#if WEAVE_CONFIG_BDX_V0_SUPPORT
            case kMsgType_BlockEOFAck:
                // TODO: should we bother parsing the message here just
                // to verify the block counter is correct? if we do parse it,
                // only set mIsCompletedSuccessfully to true on no error
                {
                    aXfer.mIsCompletedSuccessfully = true;
                    aXfer.DispatchXferDoneHandler();
                    aXfer.mFirstQuery = true;
                }

                break;
#endif // WEAVE_CONFIG_BDX_V0_SUPPORT

            case kMsgType_BlockEOFAckV1:
                {
                    BlockEOFAckV1 EOFAckV1;
                    err = BlockEOFAckV1::parse(aPacketBuffer, EOFAckV1);
                    VerifyOrExit(err == WEAVE_NO_ERROR, WeaveLogDetail(BDX, "BlockEOFAckV1 parse failed."));

                    rcvdCounter = EOFAckV1.mBlockCounter;

                    if (rcvdCounter == aXfer.mBlockCounter)
                    {
                        aXfer.mIsCompletedSuccessfully = true;
                        aXfer.DispatchXferDoneHandler();
                        aXfer.mFirstQuery = true;
                    }
                    else
                    {
                        WeaveLogDetail(BDX, "Received bad block counter: %d, expected: %d", rcvdCounter, aXfer.mBlockCounter);
                        aXfer.mNext = SendBadBlockCounterStatusReport;
                    }
                }

                break;

#if WEAVE_CONFIG_BDX_V0_SUPPORT
            case kMsgType_TransferError:
                {
                    TransferError inMsg;
                    TransferError::parse(aPacketBuffer, inMsg);
                    aXfer.DispatchXferErrorHandler(&inMsg);
                }

                break;
#endif // WEAVE_CONFIG_BDX_V0_SUPPORT

            default:
                {
                    aXfer.DispatchErrorHandler(WEAVE_ERROR_INVALID_MESSAGE_TYPE);
                }

                break;
        }
    }

exit:
    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogError(BDX, "HandleResponseTransmit exit with error: %d", err);
    }

    return err;
}
#endif // WEAVE_CONFIG_BDX_CLIENT_SEND_SUPPORT

#if WEAVE_CONFIG_BDX_CLIENT_RECEIVE_SUPPORT
/*
 * otherwise, I'm the receiver. I should expect to get
 * a block here and then, If I'm driving, send out a
 * query for the next block otherwise just send an ACK.
 */
WEAVE_ERROR HandleResponseReceive(BDXTransfer &aXfer, uint32_t aProfileId, uint8_t aMessageType, PacketBuffer *aPacketBuffer)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint32_t rcvdCounter;

    if (aProfileId == kWeaveProfile_Common && aMessageType == Common::kMsgType_StatusReport)
    {
        StatusReport statusReport;

        err = StatusReport::parse(aPacketBuffer, statusReport);
        if (err == WEAVE_NO_ERROR)
        {
            aXfer.DispatchXferErrorHandler(&statusReport);
        }
    }
    else if (aProfileId == kWeaveProfile_BDX)
    {
        switch (aMessageType)
        {
#if WEAVE_CONFIG_BDX_V0_SUPPORT
            case kMsgType_BlockSend:
                {
                    BlockSend blockSend;

                    err = BlockSend::parse(aPacketBuffer, blockSend);
                    VerifyOrExit(err == WEAVE_NO_ERROR, WeaveLogDetail(BDX, "BlockSend parse failed."));

                    aXfer.DispatchPutBlockHandler(blockSend.mLength, blockSend.mData, false);

                    aXfer.mBlockCounter++;
                    // SendBlockAck will by design send out the ack for mBlockCounter - 1
                    aXfer.mNext = aXfer.IsDriver() ? SendBlockQuery : SendBlockAck;
                }

                break;
#endif // WEAVE_CONFIG_BDX_V0_SUPPORT

            case kMsgType_BlockSendV1:
                {
                    BlockSendV1 blockSendV1;
                    err = BlockSendV1::parse(aPacketBuffer, blockSendV1);
                    VerifyOrExit(err == WEAVE_NO_ERROR, WeaveLogDetail(BDX, "BlockSendV1 parse failed."));

                    rcvdCounter = blockSendV1.mBlockCounter;

                    if (rcvdCounter == aXfer.mBlockCounter)
                    {
                        aXfer.DispatchPutBlockHandler(blockSendV1.mLength, blockSendV1.mData, false);
                    }

                    if (rcvdCounter == aXfer.mBlockCounter)
                    {
                        aXfer.mBlockCounter++;
                        aXfer.mNext = aXfer.IsDriver() ? SendBlockQueryV1 : SendBlockAckV1;
                    }
                    else
                    {
                        WeaveLogDetail(BDX, "Received bad block counter: %d, expected: %d", rcvdCounter, aXfer.mBlockCounter);

                        if (rcvdCounter > aXfer.mBlockCounter)
                        {
                            // Only send a status report if the block counter is later than expected, and ignore
                            // earlier than expected due to the possibility of duplicate messages
                            aXfer.mNext = SendBadBlockCounterStatusReport;
                        }
                    }
                }

                break;

#if WEAVE_CONFIG_BDX_V0_SUPPORT
            case kMsgType_BlockEOF:
                // TODO: add a consistency check on the overall size of the file received
                // i.e. the sum of all received blocks should match the expected file size
                // this would require us to keep track of the number of bytes received
                // as we can imagine a case where a block in the middle wasn't full
                {
                    BlockEOF blockEOF;

                    // Only try to parse if we have a data length of > 0
                    if (aPacketBuffer->DataLength() != 0)
                    {
                        err = BlockEOF::parse(aPacketBuffer, blockEOF);
                        VerifyOrExit(err == WEAVE_NO_ERROR, WeaveLogDetail(BDX, "BlockEOF parse failed."));

                        aXfer.DispatchPutBlockHandler(blockEOF.mLength, blockEOF.mData, true);
                    }

                    // ACK the EOF and clean up transfer
                    aXfer.mNext = SendBlockEOFAck;
                }

                break;
#endif // WEAVE_CONFIG_BDX_V0_SUPPORT

            case kMsgType_BlockEOFV1:
                {
                    BlockEOFV1 blockEOFV1;
                    err = BlockEOFV1::parse(aPacketBuffer, blockEOFV1);
                    VerifyOrExit(err == WEAVE_NO_ERROR, WeaveLogDetail(BDX, "BlockEOFV1 parse failed."));

                    rcvdCounter = blockEOFV1.mBlockCounter;

                    if (rcvdCounter == aXfer.mBlockCounter)
                    {
                        aXfer.DispatchPutBlockHandler(blockEOFV1.mLength, blockEOFV1.mData, true);
                    }

                    if (rcvdCounter == aXfer.mBlockCounter)
                    {
                        aXfer.mNext = SendBlockEOFAckV1;
                    }
                    else
                    {
                        WeaveLogDetail(BDX, "Received bad block counter: %d, expected: %d", rcvdCounter, aXfer.mBlockCounter);
                        aXfer.mNext = SendBadBlockCounterStatusReport;
                    }
                }

                break;

#if WEAVE_CONFIG_BDX_V0_SUPPORT
            case kMsgType_TransferError:
                {
                    TransferError inMsg;

                    TransferError::parse(aPacketBuffer, inMsg);
                    aXfer.DispatchXferErrorHandler(&inMsg);
                }

                break;
#endif // WEAVE_CONFIG_BDX_V0_SUPPORT

            default:
                {
                    aXfer.DispatchErrorHandler(WEAVE_ERROR_INVALID_MESSAGE_TYPE);
                }

                break;
        }
    }

exit:
    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogError(BDX, "HandleResponseReceive exit with error: %d", err);
    }

    return err;
}
#endif // WEAVE_CONFIG_BDX_CLIENT_RECEIVE_SUPPORT

WEAVE_ERROR HandleResponseNotAccepted(BDXTransfer &aXfer, uint32_t aProfileId, uint8_t aMessageType, PacketBuffer *aPacketBuffer)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // V1 - Handle Status Report - if we received a status report and it hasn't been accepted,
    // we should treat the transfer as rejected.
    if (aProfileId == kWeaveProfile_Common && aMessageType == Common::kMsgType_StatusReport)
    {
        StatusReport statusReport;

        err = StatusReport::parse(aPacketBuffer, statusReport);
        if (err == WEAVE_NO_ERROR)
        {
            aXfer.DispatchRejectHandler(&statusReport);
        }
    }
    else if (aProfileId == kWeaveProfile_BDX)
    {
        /*
         * here the tranfer hasn't been accepted yet, and we're waiting
         * either for an accept or a reject message.
         */
        switch (aMessageType)
        {
#if WEAVE_CONFIG_BDX_V0_SUPPORT
            case kMsgType_TransferError:
                {
                    TransferError inMsg;

                    TransferError::parse(aPacketBuffer, inMsg);
                    aXfer.DispatchXferErrorHandler(&inMsg);
                }

                break;
#endif // WEAVE_CONFIG_BDX_V0_SUPPORT

#if WEAVE_CONFIG_BDX_CLIENT_SEND_SUPPORT
            case kMsgType_SendAccept:
                /*
                 * The send is accepted. now we figure out who's driving and,
                 * if necessary, kick it off.
                 */
                {
                    uint8_t xferMode;
                    SendAccept inMsg;

                    err = SendAccept::parse(aPacketBuffer, inMsg);
                    VerifyOrExit(err == WEAVE_NO_ERROR, WeaveLogDetail(BDX, "SendAccept parse failed."));
                    // This implies we're compatible with any version equal or below WEAVE_CONFIG_BDX_VERSION
                    VerifyOrExit(inMsg.mVersion <= WEAVE_CONFIG_BDX_VERSION,
                                 err = WEAVE_ERROR_UNSUPPORTED_MESSAGE_VERSION;
                                 WeaveLogDetail(BDX, "SendAccept returned an incompatible version: %d.", inMsg.mVersion));

                    aXfer.mIsAccepted = true;
                    aXfer.mMaxBlockSize = inMsg.mMaxBlockSize;
                    aXfer.mTransferMode = inMsg.mTransferMode;
                    aXfer.mVersion = inMsg.mVersion;
                    err = aXfer.DispatchSendAccept(&inMsg);
                    VerifyOrExit(err == WEAVE_NO_ERROR, WeaveLogDetail(BDX, "DispatchSendAccept failed."));

                    xferMode = inMsg.mTransferMode;

                    switch (xferMode)
                    {
                        case kMode_SenderDrive:
                            // Try and send the first block
                            VerifyOrExit(aXfer.mVersion < 2, err = WEAVE_ERROR_UNSUPPORTED_MESSAGE_VERSION);

#if WEAVE_CONFIG_BDX_V0_SUPPORT
                            aXfer.mNext = aXfer.mVersion == 1 ? SendNextBlockV1 : SendNextBlock;
#else
                            aXfer.mNext = aXfer.mVersion == 1 ? SendNextBlockV1 : NULL;
#endif // WEAVE_CONFIG_BDX_V0_SUPPORT
                            break;

                        case kMode_ReceiverDrive:
                            // Do nothing else as now we just wait for a BlockQuery
                            break;

                        case kMode_Asynchronous:
                            WeaveLogDetail(BDX, "Received request for Async transfer mode, but it's not implemented yet!");
                            err = WEAVE_ERROR_INVALID_TRANSFER_MODE;
                            break;

                        default:
                            err = WEAVE_ERROR_INVALID_TRANSFER_MODE;
                            break;
                    }
                }

                break;
#endif // WEAVE_CONFIG_BDX_CLIENT_SEND_SUPPORT

#if WEAVE_CONFIG_BDX_V0_SUPPORT
            case kMsgType_SendReject:
                {
                    SendReject sendReject;

                    err = SendReject::parse(aPacketBuffer, sendReject);
                    if (err == WEAVE_NO_ERROR)
                    {
                        aXfer.DispatchRejectHandler(&sendReject);
                    }
                }

                break;
#endif // WEAVE_CONFIG_BDX_V0_SUPPORT

#if WEAVE_CONFIG_BDX_CLIENT_RECEIVE_SUPPORT
            case kMsgType_ReceiveAccept:
                /*
                 * The receive is accepted. now we figure out who's driving and,
                 * if necessary, kick it off.
                 */
                {
                    uint8_t xferMode;
                    ReceiveAccept inMsg;

                    err = ReceiveAccept::parse(aPacketBuffer, inMsg);

                    VerifyOrExit(err == WEAVE_NO_ERROR, WeaveLogDetail(BDX, "ReceiveAccept parse failed."));
                    // This implies we're compatible with any version equal or below WEAVE_CONFIG_BDX_VERSION
                    VerifyOrExit(inMsg.mVersion <= WEAVE_CONFIG_BDX_VERSION,
                                 err = WEAVE_ERROR_UNSUPPORTED_MESSAGE_VERSION;
                                 WeaveLogDetail(BDX, "ReceiveAccept returned an incompatible version: %d.", inMsg.mVersion));

                    // Set up the aXfer object and call callback
                    aXfer.mIsAccepted = true;
                    aXfer.mMaxBlockSize = inMsg.mMaxBlockSize;
                    aXfer.mTransferMode = inMsg.mTransferMode;
                    aXfer.mVersion = inMsg.mVersion;
                    aXfer.mLength = inMsg.mLength;
                    err = aXfer.DispatchReceiveAccept(&inMsg);
                    VerifyOrExit(err == WEAVE_NO_ERROR, WeaveLogDetail(BDX, "DispatchReceiveAccept failed."));
                    xferMode = inMsg.mTransferMode;

                    switch (xferMode)
                    {
                        case kMode_SenderDrive:
                            WeaveLogDetail(BDX, "Receive accepted: am not driving, so waiting for first BlockSend");
                            // Do nothing else as now we just expect the first BlockSend
                            break;

                        case kMode_ReceiverDrive:
                            WeaveLogDetail(BDX, "Receive accepted: am driving, so sending first query");
                            VerifyOrExit(aXfer.mVersion < 2, err = WEAVE_ERROR_UNSUPPORTED_MESSAGE_VERSION);

#if WEAVE_CONFIG_BDX_V0_SUPPORT
                            aXfer.mNext = aXfer.mVersion == 1 ? SendBlockQueryV1 : SendBlockQuery;
#else
                            aXfer.mNext = aXfer.mVersion == 1 ? SendBlockQueryV1 : NULL;
#endif // WEAVE_CONFIG_BDX_V0_SUPPORT

                            break;

                        case kMode_Asynchronous:
                            WeaveLogDetail(BDX, "Received request for Async transfer mode, but it's not implemented yet!");
                            err = WEAVE_ERROR_INVALID_TRANSFER_MODE;
                            break;

                        default:
                            err = WEAVE_ERROR_INVALID_TRANSFER_MODE;
                            break;
                    }
                }

                break;
#endif // WEAVE_CONFIG_BDX_CLIENT_RECEIVE_SUPPORT

#if WEAVE_CONFIG_BDX_V0_SUPPORT
            case kMsgType_ReceiveReject:
                {
                    ReceiveReject receiveReject;
                    err = ReceiveReject::parse(aPacketBuffer, receiveReject);
                    if (err == WEAVE_NO_ERROR)
                    {
                        aXfer.DispatchRejectHandler(&receiveReject);
                    }
                }

                break;
#endif // WEAVE_CONFIG_BDX_V0_SUPPORT

            default:
                aXfer.DispatchErrorHandler(WEAVE_ERROR_INVALID_MESSAGE_TYPE);
                break;
        }
    }

exit:
    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogError(BDX, "HandleResponseNotAccepted exit with error: %d", err);
    }

    return err;
}

} // BdxProtocol
} // BulkDataTransfer
} // Profiles
} // Weave
} // nl
