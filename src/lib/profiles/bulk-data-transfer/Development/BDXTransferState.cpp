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
 *      This file contains definitons for the BDXTransfer object, which represents
 *      the state of an ongoing transfer and is managed by the BdxNode.
 */

#include <Weave/Support/logging/WeaveLogging.h>

#include <Weave/Profiles/bulk-data-transfer/Development/BDXTransferState.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(BDX, kWeaveManagedNamespaceDesignation_Development) {

using namespace nl::Weave::Logging;

/**
 * @brief
 *      Shuts down the current transfer, including closing any open ExchangeContext.
 *
 * Use this opportunity to free any resources associated with this transfer
 * and your application logic.
 *
 * @note
 *   The current BDX profile implementation only calls this method in the following circumstances:
 *   1) The whole server is shutdown
 *   2) An error occurs while handling a SendInit or ReceiveInit message
 *      that results in the transfer being unable to proceed
 *   3) The underlying connection is closed
 *   4) A SendReject or ReceiveReject message is received during transfer initiation
 *   5) A BlockEOF or BlockEOFAck is received, in which case
 *      mIsCompletedSuccessfully will be set to true
 *   6) The exchange timed out when waiting for a reply
 */
void BDXTransfer::Shutdown(void)
{
    if (mExchangeContext != NULL)
    {
        if (mIsCompletedSuccessfully)
        {
            mExchangeContext->Close();
        }
        else
        {
            mExchangeContext->Abort();
        }
    }

    Reset();
}

/**
 * @brief
 *      Sets all pointers to NULL, resets counters, etc. Called when shut down.
 */
void BDXTransfer::Reset(void)
{
    mExchangeContext                = NULL;
    mIsInitiated                    = false;
    mIsAccepted                     = false;
    mFirstQuery                     = true;
    mMaxBlockSize                   = DEFAULT_MAX_BLOCK_SIZE;
    mStartOffset                    = 0;
    mLength                         = 0;
    mBytesSent                      = 0;
    mBlockCounter                   = 0;
    mIsWideRange                    = false;
    mIsCompletedSuccessfully        = false;
    mAmInitiator                    = false;

    mHandlers.mSendAcceptHandler    = NULL;
    mHandlers.mReceiveAcceptHandler = NULL;
    mHandlers.mRejectHandler        = NULL;
    mHandlers.mGetBlockHandler      = NULL;
    mHandlers.mPutBlockHandler      = NULL;
    mHandlers.mXferErrorHandler     = NULL;
    mHandlers.mXferDoneHandler      = NULL;
    mHandlers.mErrorHandler         = NULL;
}

/**
 * @brief
 *      Returns true if this transfer is asynchronous, false otherwise.
 *
 * @note
 *   Asynchronous transfer is not currently implemented!
 *
 * @return true iff the transfer is asynchronous.
 */
bool BDXTransfer::IsAsync(void)
{
    return mTransferMode & kMode_Asynchronous;
}

/**
 * @brief
 *      Returns true if this entity (node) is the driver for this transfer, false otherwise.
 *
 * @return true iff this entity is the driver for this transfer
 */
bool BDXTransfer::IsDriver(void)
{
    return ((mAmSender && (mTransferMode & kMode_SenderDrive)) ||
            (!mAmSender && (mTransferMode & kMode_ReceiverDrive)));
}

/**
 * @brief
 *  This function sets the handlers on this BDXTransfer object.  You should always
 *  use this method rather than trying to set them manually as the underlying
 *  implementation of how the handler function pointers are stored is not a part
 *  of the public API.
 *
 * @note
 *  To disable a particular handler (e.g. ignore GetBlockHandler during a Receive
 *  Transfer), simply set it to NULL
 *
 * @param[in]   aHandlers       Structure of callback handlers to be called
 */
void BDXTransfer::SetHandlers(BDXHandlers aHandlers)
{
    mHandlers = aHandlers;
}

/**
 * @brief
 *  This function returns the default flags to be sent with a message
 *
 * @param[in]   aExpectResponse If we expect a response to this message
 *
 * @return The flags to be sent
 */
uint16_t BDXTransfer::GetDefaultFlags(bool aExpectResponse)
{
    return ((aExpectResponse ? ExchangeContext::kSendFlag_ExpectResponse : 0) |
            (GetBDXAckFlag(mExchangeContext)));
}

/**
 * @brief
 *  If the receive accept handler has been set, call it.
 *
 * @param[in]   aReceiveAcceptMsg   ReceiveAccept message to be processed
 *
 * @return an error value
 */
WEAVE_ERROR BDXTransfer::DispatchReceiveAccept(ReceiveAccept *aReceiveAcceptMsg)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    if (mHandlers.mReceiveAcceptHandler)
    {
        err = mHandlers.mReceiveAcceptHandler(this, aReceiveAcceptMsg);
    }
    return err;
}

/**
 * @brief
 *  If the send accept handler has been set, call it.
 *
 * @param[in]   aSendAcceptMsg      SendAccept message to be processed
 *
 * @return an error value
 */
WEAVE_ERROR BDXTransfer::DispatchSendAccept(SendAccept *aSendAcceptMsg)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    if (mHandlers.mSendAcceptHandler)
    {
        err = mHandlers.mSendAcceptHandler(this, aSendAcceptMsg);
    }
    return err;
}

/**
 * @brief
 *  If the reject handler has been set, call it. If not set, also shut down the
 *  transfer as a default behavior.
 *
 * @param[in]   aReport             StatusReport message to be processed
 */
void BDXTransfer::DispatchRejectHandler(StatusReport *aReport)
{
    if (mHandlers.mRejectHandler)
    {
        mHandlers.mRejectHandler(this, aReport);
    }
    else
    {
        Shutdown();
    }
}


/**
 * @brief
 *  If the put block handler has been set, call it.
 *
 * @param[in]   aLength             Length of block
 * @param[in]   aDataBlock          Pointer to the data block
 * @param[in]   aLastBlock          True if this is the last block in the transfer
 */
void BDXTransfer::DispatchPutBlockHandler(uint64_t aLength,
                                          uint8_t *aDataBlock,
                                          bool aLastBlock)
{
    if (mHandlers.mPutBlockHandler)
    {
        mHandlers.mPutBlockHandler(this, aLength, aDataBlock, aLastBlock);
    }
}

/**
 * @brief
 *  If the get block handler has been set, call it.
 *
 * @param[in]   aLength             Length of block
 * @param[in]   aDataBlock          Pointer to the data block
 * @param[in]   aLastBlock          True if this is the last block in the transfer
 */
void BDXTransfer::DispatchGetBlockHandler(uint64_t *aLength,
                                          uint8_t **aDataBlock,
                                          bool *aLastBlock)
{
    if (mHandlers.mGetBlockHandler)
    {
        mHandlers.mGetBlockHandler(this, aLength, aDataBlock, aLastBlock);
    }
}

/**
 * @brief
 *  If the error handler has been set, call it. If not set, also shut down the
 *  transfer as a default behavior.
 *
 * @param[in]   anErrorCode         Error code to be processed
 */
void BDXTransfer::DispatchErrorHandler(WEAVE_ERROR anErrorCode)
{
    if (mHandlers.mErrorHandler)
    {
        mHandlers.mErrorHandler(this, anErrorCode);
    }
    else
    {
        Shutdown();
    }
}

/**
 * @brief
 *  If the transfer error handler has been set, call it. If not set, also shut down the
 *  transfer as a default behavior.
 *
 * @param[in]   aXferError          Status report of an error to be processed
 */
void BDXTransfer::DispatchXferErrorHandler(StatusReport *aXferError)
{
    if (mHandlers.mXferErrorHandler)
    {
        mHandlers.mXferErrorHandler(this, aXferError);
    }
    else
    {
        Shutdown();
    }
}

/**
 * @brief
 *  If the transfer done handler has been set, call it. If not set, also shut down the
 *  transfer as a default behavior.
 */
void BDXTransfer::DispatchXferDoneHandler(void)
{
    if (mHandlers.mXferDoneHandler)
    {
        mHandlers.mXferDoneHandler(this);
    }
    else
    {
        Shutdown();
    }
}

} // namespace BulkDataTransfer
} // namespace Profiles
} // namespace Weave
} // namespace nl
