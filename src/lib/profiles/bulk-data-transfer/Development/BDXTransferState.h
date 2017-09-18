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
 *      This file declares a class that contains state related to an
 *      ongoing Weave Bulk Data Transfer. It is used by the
 *      BdxProtocol and BdxServer classes.
 *
 */

#ifndef _WEAVE_BDX_TRANSFER_STATE_H
#define _WEAVE_BDX_TRANSFER_STATE_H

#include <Weave/Profiles/bulk-data-transfer/Development/BDXManagedNamespace.hpp>
#include <Weave/Profiles/bulk-data-transfer/Development/BDXConstants.h>
#include <Weave/Profiles/bulk-data-transfer/Development/BDXMessages.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(BDX, kWeaveManagedNamespaceDesignation_Development) {

// The default size we set a transfer's maxBlockSize to
#define DEFAULT_MAX_BLOCK_SIZE 256

struct BDXTransfer; // forward declaration for inclusion in callbacks

// typedefs for handler types needed below

/**
 * @brief
 *  Callback invoked when receiving a SendInit message.
 *
 * Its job is to determine if you want to accept the SendInit and, if so, set
 * aXfer->mIsAccepted=true so that the protocol will send an accept message to
 * the initiator. The BDXTransfer object is initiated to default settings.
 * This is a good place to attach any application-specific state (open file
 * handles, etc.) to aXfer->mAppState. You should also attach the necessary
 * handlers for e.g. block handling to the BDXTransfer object at this point.
 * If an error code other than WEAVE_NO_ERROR is returned, the transfer is
 * assumed to be rejected and the protocol will handle sending a reject message
 * with the code.
 *
 * @param[in]   aXfer           Pointer to the BDXTransfer associated with this transfer
 * @param[in]   aSendInitMsg    Pointer to the SendInit message that we are processing
 */
typedef uint16_t (*SendInitHandler)(BDXTransfer *aXfer, SendInit *aSendInitMsg);

/**
 * @brief
 *  Callback invoked when receiving a ReceiveInit message.
 *
 * Its job is to determine if you want to accept the Receive and, if so, set
 * aXfer->mIsAccepted=true so that the protocol will send an accept message
 * to the initiator. The BDXTransfer object is initiated to default settings.
 * This is a good place to attach any application-specific state (open file
 * handles, etc.) to aXfer->mAppState. You should also attach the necessary
 * handlers for e.g. block handling to the BDXTransfer object at this point.
 * If an error code other than kStatus_Success is returned, the transfer is
 * assumed to be rejected and the protocol will handle sending a reject message
 * with the code.
 *
 * @param[in]   aXfer               Pointer to the BDXTransfer associated with this transfer
 * @param[in]   aReceiveInitMsg     Pointer to the ReceiveInit message that we are processing
 */
typedef uint16_t (*ReceiveInitHandler)(BDXTransfer *aXfer, ReceiveInit *aReceiveInitMsg);

/**
 * @brief
 *  Callback invoked when a previously sent SendInit is accepted by the destination.
 *
 * You may wish to use this opportunity to open files or allocate resources for
 * the transfer if you did not do so when initiating it.
 *
 * @param[in]   aXfer               Pointer to the BDXTransfer associated with this transfer
 * @param[in]   aSendAcceptMsg      Pointer to the SendAccept message that we are processing
 */
typedef WEAVE_ERROR (*SendAcceptHandler)(BDXTransfer *aXfer, SendAccept *aSendAcceptMsg);

/**
 * @brief
 *  Callback invoked when a previously sent ReceiveInit is accepted by the destination.
 *
 * You may wish to use this opportunity to open files or allocate resources for
 * the transfer if you did not do so when initiating it.
 *
 * @param[in]   aXfer               Pointer to the BDXTransfer associated with this transfer
 * @param[in]   aReceiveAcceptMsg   Pointer to the ReceiveAccept message that we are processing
 */
typedef WEAVE_ERROR (*ReceiveAcceptHandler)(BDXTransfer *aXfer, ReceiveAccept *aReceiveAcceptMsg);

/**
 * @brief
 *  Invoked if one of the previous Init messages was rejected by the destination.
 *
 * @note
 *  Use this handler to provide feedback to your application about how to adjust
 *  a future request to make it successful. It should also close the BDXTransfer here.
 *
 * @param[in]   aXfer               Pointer to the BDXTransfer associated with this transfer
 * @param[in]   aReport             Pointer to the StatusReport message rejection that we are processing
 */
typedef void (*RejectHandler)(BDXTransfer *aXfer, StatusReport *aReport);

/**
 * @brief
 *     Get a block of data to be transmitted.
 *
 *  The caller provides the buffering space (buffer and length of the
 *  buffer, passed in by reference).  Callee (user application) SHOULD
 *  use the provided buffer, but for backward compatibility reasons,
 *  may return its own buffer.  Callee must not provide more than the
 *  `aLength` of bytes.  On return, `aLength` contains the actual
 *  number of bytes read into the buffer.
 *
 * @param[in] aXfer           The BDXTransfer associated with this ongoing
 *                            transfer
 * @param[inout] aLength      The length of data read and stored in this
 *                            block. On call to the function contains
 *                            the length of the of the buffer passed
 *                            in the `aDataBlock`.  On return, the
 *                            variable contains the length of data
 *                            actually read.
 * @param[inout] aDataBlock The pointer to the block of data. On
 *                            input, it contains the framework-
 *                            provided buffer; the callee may use that
 *                            space to fill the buffer, or provide its
 *                            own buffering space (for backward
 *                            compatibility applications). Applications
 *                            using provided buffer must not assume
 *                            any alignment.
 * @param[out] aLastBlock     True if the block should be sent as a
 *                            `BlockEOF` and the transfer completed,
 *                            false otherwise
 */
//TODO: define a contract for how to tell the caller not to send the block and/or
//that we encountered an error
typedef void (*GetBlockHandler)(BDXTransfer *aXfer,
                                uint64_t *aLength,
                                uint8_t **aDataBlock,
                                bool *aLastBlock);

/**
 * @brief
 *  Handle the block of data pointed to by aDataBlock of length aLength.
 *  Likely this will involve writing it to a file and closing said file if isLastBlock is true.
 *
 * @param[in]   aXfer       The BDXTransfer associated with this ongoing transfer
 * @param[in]   aLength     The length of data read and stored in the specified block
 * @param[in]   aDataBlock  The actual block of data
 * @param[in]   aLastBlock  True if the block was received as a BlockEOF and the
 *                          transfer completed, false otherwise. If true,
 *                          the programmer should probably finalize any file handles,
 *                          keeping in mind that the XferDoneHandler will be called
 *                          after this
 */
typedef void (*PutBlockHandler)(BDXTransfer *aXfer, uint64_t aLength,
                                uint8_t *aDataBlock, bool aLastBlock);

/**
 * @brief
 *  Handle TransferError messages received or sent by BDX.
 *
 * @note
 *  The BDX transfer is presumed to be potentially recoverable (possibly temporary e.g. out of
 *  PacketBuffers at the moment) and so the option of calling Shutdown() is left
 *  to the application programmer and the callbacks they define.
 *TODO: verify this and reconcile it with the language in the BDX document, which states:
 *  "[A TransferError] Can be sent at any time by either party to prematurely
 *  end the bulk data transfer."
 *
 * @note
 *   To determine if this TransferError was sent by this entity or its counterpart,
 *   inspect the aXfer->mAmInitiator member.
 *
 * @param[in]   aXfer           Pointer to the BDXTransfer associated with this transfer
 * @param[in]   aXferError      Pointer to the StatusReport message error that we are processing
 */
typedef void (*XferErrorHandler)(BDXTransfer *aXfer, StatusReport *aXferError);

/**
 * @brief
 *  Handle cases where the transfer is finished.
 *
 * @note
 *  To determine whether this transfer was aborted prematurely or completed
 *  successfully (that is, a BlockEOF or BlockEOFAck was received), inspect the
 *  aXfer->mIsCompletedSuccessfully member.
 *
 * @param[in]   aXfer           Pointer to the BDXTransfer associated with this transfer
 */
typedef void (*XferDoneHandler)(BDXTransfer *aXfer);

/**
 * @brief
 *  This handler is called any time a weave error is encountered that cannot
 *  directly be returned via error codes to user-application-defined control flow.
 *
 *  That is, if an error occurs within another handler whose signature has return
 *  type void (e.g. in response to an incoming Weave message or even dispatched
 *  by the protocol), this handler will be called so that the user can determine
 *  whether the transfer can be recovered and continue or if they should call
 *  Shutdown(). Note that it is possible for an error to occur before a BDXTransfer
 *  is initialized (e.g. already too many allocated transfer objects).  In such a case,
 *  said error will be logged by Weave and the protocol will handle cleaning up any
 *  necessary state that it allocated.
 *
 * @param[in]   aXfer           Pointer to the BDXTransfer associated with this transfer
 * @param[in]   anErrorCode     The error code that we need to process
 */

typedef void (*ErrorHandler)(BDXTransfer *aXfer, WEAVE_ERROR anErrorCode);

struct BDXHandlers
{
    SendAcceptHandler       mSendAcceptHandler;
    ReceiveAcceptHandler    mReceiveAcceptHandler;
    RejectHandler           mRejectHandler;
    GetBlockHandler         mGetBlockHandler;
    PutBlockHandler         mPutBlockHandler;
    XferErrorHandler        mXferErrorHandler;
    XferDoneHandler         mXferDoneHandler;
    ErrorHandler            mErrorHandler;
};

/** This structure contains data members representing an active BDX transfer.
 * These objects are used by the BdxProtocol to maintain protocol state.
 * They are managed by the BdxServer, which handles creating and initializing
 * new transfers, including managing Connections and ExchangeContexts.
 *
 * @note:
 * The handlers attached to this object are specified by the end application
 * so that multiple transfers may be open simultaneously that each support different
 * application logic.
 */
struct BDXTransfer
{
    ExchangeContext *   mExchangeContext;
    void *              mAppState;

    // data members related to the transfer handling
    uint8_t             mTransferMode;
    // Version being used for this transfer
    uint8_t             mVersion;
    // TODO: perhaps compact these more efficiently as flags to save memory?
    bool                mIsInitiated;
    bool                mIsAccepted;
    bool                mIsCompletedSuccessfully; // true iff a BlockEOF or BlockEOFAck was received
    bool                mAmInitiator;
    bool                mAmSender;
    bool                mIsWideRange; // true is widths and offsets are 64 bits
    bool                mFirstQuery; // true if we haven't received our first query
    //TODO: bool mAckRcvd;  // may want to keep track of ACKs to support
                            // retransmission of old blocks in the future
    //TODO: int mSendFlags; // may want to configure SendFlags to be used in calls to
                            // SendMessage, e.g. to support WRMP

    /** file/block related data members
     * TODO: remove this? or should we just establish a contract of what
     * this string might look like and how it will be used?
     * Specifically, is it backed by an PacketBuffer?  If so, it probably
     * shouldn't stick around for the whole xfer as that takes up a pbuf
     */
    ReferencedString    mFileDesignator;
    uint16_t            mMaxBlockSize; // Max block size to be used during this transfer
    uint64_t            mStartOffset; // Offset to start at for transfer, typically 0
    uint64_t            mLength; // Expected length of the transfer, 0 if unkown
    uint64_t            mBytesSent; // How many bytes have been sent so far in this transfer
    /** The next block number we expect to receive a BlockQuery or BlockACK for
     * when sending (once the transfer has officially started).
     * When receiving, it is the next BlockSend we expect to receive
     * or the latest BlockQuery we sent (after the transfer has officially started
     * and the first query sent that is).
     */
    uint32_t            mBlockCounter;

    // application-supplied handlers
    //TODO: make these private when BdxProtocol doesn't inspect them directly
    //before calling DispatchGetBlockHandler().  We'll have to remove that check
    //anyway if we move to a delegate model.
    BDXHandlers mHandlers;

    WEAVE_ERROR (*mNext)(BDXTransfer &); // Next action to take after the processing of the response

    void Shutdown(void);

    void Reset(void);

    bool IsAsync(void);

    bool IsDriver(void);

    void SetHandlers(BDXHandlers aHandlers);

    uint16_t GetDefaultFlags(bool aExpectResponse);

    /**
     * Dispatchers simply check whether a handler has been set and then call it if so.
     * Therefore, these should be used as the public interface for calling callbacks,
     * which should never be touched directly by outside applications.  It is
     * possible that a future revision of BDXTransfer will use a delegate object
     * rather than storing individual pointers to each of the callbacks.
     */

    WEAVE_ERROR DispatchReceiveAccept(ReceiveAccept *aReceiveAcceptMsg);
    WEAVE_ERROR DispatchSendAccept(SendAccept *aSendAcceptMsg);
    void DispatchRejectHandler(StatusReport *aReport);
    void DispatchPutBlockHandler(uint64_t aLength,
                                 uint8_t *aDataBlock,
                                 bool aLastBlock);
    void DispatchGetBlockHandler(uint64_t *aLength,
                                 uint8_t **aDataBlock,
                                 bool *aLastBlock);
    void DispatchErrorHandler(WEAVE_ERROR anErrorCode);
    void DispatchXferErrorHandler(StatusReport *aXferError);
    void DispatchXferDoneHandler(void);
};

/**
 * @brief
 *  GetBDXAckFlag returns the appropriate flag for the RequestAck field
 *  depending on the exchange context's connection (no request ack for TCP),
 *  and based on compile time support for WRMP.
 *
 * @param[in]   anEc           The exchange context we should get the flag based on
 *
 * @return 0 or kSendFlag_RequestAck
 */
inline uint16_t GetBDXAckFlag(ExchangeContext *anEc)
{
    uint16_t flags = 0;

#if WEAVE_CONFIG_BDX_WRMP_SUPPORT
    if (anEc->Con == NULL)
    {
        flags = ExchangeContext::kSendFlag_RequestAck;
    }
#endif

    return flags;
}

} // namespace BulkDataTransfer
} // namespace Profiles
} // namespace Weave
} // namespace nl

#endif // _WEAVE_BDX_TRANSFER_STATE_H
