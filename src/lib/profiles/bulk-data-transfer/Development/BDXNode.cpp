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
 *      Server.  It manages BDXTransfer objects and interfaces with the
 *      BdxProtocol class to facilitate transfers.
 */

#include <Weave/Support/CodeUtils.h>
#include <Weave/Support/WeaveFaultInjection.h>
#include <Weave/Support/logging/WeaveLogging.h>
#include <Weave/Profiles/common/CommonProfile.h>

#include <Weave/Profiles/bulk-data-transfer/Development/BDXNode.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(BDX, kWeaveManagedNamespaceDesignation_Development) {

using namespace ::nl::Weave::Profiles::Common;
using namespace ::nl::Weave::Logging;

#define BDX_RESPONSE_TIMEOUT_MS WEAVE_CONFIG_BDX_RESPONSE_TIMEOUT_SEC * 1000

/*
 * Here are the method definitions for the BdxNode class
 *
 * the no-arg constructor with defaulting for BDX clients
 */
BdxNode::BdxNode(void)
{
    mExchangeMgr            = NULL;
    mIsBdxTransferAllowed   = false;
    mInitialized            = false;
    mSendInitHandler        = NULL;
    mReceiveInitHandler     = NULL;
}

/**
 * @brief
 *  Put all transfers in a default state ready for use, store the
 *  WeaveExchangeManager and any other necessary Weave resources,
 *  and sets allowBdxTransferToRun(true).
 *
 * @param[in]   anExchangeMgr       An exchange manager to use for this bulk transfer operation.
 *
 * @retval      #WEAVE_NO_ERROR                 if successful
 * @retval      #WEAVE_ERROR_INCORRECT_STATE    if mExchangeMgr isn't null, already initialized
 */
WEAVE_ERROR BdxNode::Init(WeaveExchangeManager *anExchangeMgr)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // Error if already initialized.
    VerifyOrExit(mExchangeMgr == NULL, err = WEAVE_ERROR_INCORRECT_STATE);
    VerifyOrExit(anExchangeMgr != NULL, err = WEAVE_ERROR_INCORRECT_STATE);
    mExchangeMgr = anExchangeMgr;

    // Initialize all the BDXTransfers
    for (int i = 0; i < WEAVE_CONFIG_BDX_MAX_NUM_TRANSFERS; i++)
    {
        // Error if already initialized.
        mTransferPool[i].Reset();
    }

    mIsBdxTransferAllowed = true;
    mInitialized = true;

exit:
    return err;
}

/**
 * @brief
 *  Shuts down all transfers and releases any Weave resources
 *  (currently sets mExchangeMgr to NULL). Sets AllowBdxTransferToRun(false) and
 *  disconnects any current callbacks (for example, SendInitHandler).
 *
 * @note
 *   The BDX Protocol never calls this function as it is up to the platform
 *   to create and manage the BdxNode.
 *
 * @return  #WEAVE_NO_ERROR if successfully shut down, other error if AwaitBdxSend/ReceiveInit returns an error
 */
WEAVE_ERROR BdxNode::Shutdown(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    for (int i = 0; i < WEAVE_CONFIG_BDX_MAX_NUM_TRANSFERS; i++)
    {
        ShutdownTransfer(&mTransferPool[i]);
    }

    AllowBdxTransferToRun(false);

#if WEAVE_CONFIG_BDX_SERVER_SUPPORT
    // Unregister any callbacks BEFORE releasing mExchangeMgr
    err = AwaitBdxSendInit(NULL);
    VerifyOrExit(err == WEAVE_NO_ERROR || err == WEAVE_ERROR_INCORRECT_STATE ||
                 err == WEAVE_ERROR_NO_UNSOLICITED_MESSAGE_HANDLER,
                 WeaveLogDetail(BDX,
                               "Error removing existing sendinit callback in ShutdownServer: %d", err);
                );

    err = AwaitBdxReceiveInit(NULL);
    VerifyOrExit(err == WEAVE_NO_ERROR || err == WEAVE_ERROR_INCORRECT_STATE ||
                 err == WEAVE_ERROR_NO_UNSOLICITED_MESSAGE_HANDLER,
                 WeaveLogDetail(BDX,
                               "Error removing existing receiveinit callback in ShutdownServer: %d", err);
                );

exit:
#endif // WEAVE_CONFIG_BDX_SERVER_SUPPORT

    mExchangeMgr = NULL;

    return err;
}

/**
 * @brief
 *  Get and set up a new BDXTransfer from transfer pool if available,
 *  or set to NULL otherwise and return an error.
 *
 *  A BDXTransfer object obtained through this function is considered allocated
 *  by its mIsInitiated member being true. All other values will be the expected
 *  default values (see BDXTransfer::setDefaults()).
 *
 * @note
 *   You MUST shutdown this transfer in order to release it to the free pool!
 * @note
 *   You will not typically use this function and instead should use one of the
 *   other versions, which in turn defer to this function.  Only manually set up
 *   a BDXTransfer if you really know what you're doing and are prepared to do
 *   maintenance in the event of future changes to its structure!
 *
 * @param[in]   aXfer       A passed-by-reference pointer that will point to the
 *                              new BDXTransfer object if one is available, else NULL.
 *
 * @retval      #WEAVE_NO_ERROR                     If we successfully found a new BDXTransfer.
 * @retval      #WEAVE_ERROR_TOO_MANY_CONNECTIONS   If too many transfers are currently active and aXfer is NULL
 */
WEAVE_ERROR BdxNode::AllocTransfer(BDXTransfer * &aXfer)
{
    WEAVE_ERROR err = WEAVE_ERROR_TOO_MANY_CONNECTIONS;

    WEAVE_FAULT_INJECT(FaultInjection::kFault_BDXAllocTransfer, ExitNow());

    for (int i = 0; i < WEAVE_CONFIG_BDX_MAX_NUM_TRANSFERS; i++)
    {
        aXfer = &mTransferPool[i];
        if (aXfer->mIsInitiated)
        {
            continue;
        }

        if (aXfer->mExchangeContext != NULL)
        {
            WeaveLogDetail(BDX, "Error! Xfer is not initiated but has mExchangeContext.");
            continue;
        }

        aXfer->mIsInitiated = true;
        err = WEAVE_NO_ERROR;
        ExitNow();
    }

    // All of the connections are in use
    aXfer = NULL;

exit:
    return err;
}

/**
 * @brief
 *  Get and set up a new BDXTransfer from transfer pool if available,
 *  or set to NULL otherwise and return an error.
 *
 * @note
 *  You MUST shutdown this transfer in order to release it to the free pool!
 * @note
 *  If a particular handler should not be used in the transfer you are setting up
 *  (e.g., a PutBlockHandler when Sending), set it to NULL.
 * @note
 *  See BDXTransferState.h for details about the handlers.
 *
 * @param[in]   aBinding        The Binding to the node we will initiate the transfer with.
 *                                  It's used to create an associated ExchangeContext for this transfer.
 * @param[in]   aBDXHandlers    A structure of BDX callback handlers to be called during the transfer
 * @param[in]   aFileDesignator The file designator for the file that will be transferred.
 * @param[in]   anAppState      An application-specific state object to be attached to the
 *                                  BDXTransfer for use by the user application and associated callbacks.
 * @param[in]   aXfer           A passed-by-reference pointer that will point to the
 *                                  new BDXTransfer object if one is available, else NULL.
 *
 *
 * @retval      #WEAVE_NO_ERROR                     If we successfully found a new BDXTransfer.
 * @retval      #WEAVE_ERROR_NO_MEMORY              If unable to create ExchangeContext
 * @retval      #WEAVE_ERROR_TOO_MANY_CONNECTIONS   If too many transfers are currently active and aXfer is NULL
 * @retval      #WEAVE_ERROR_INCORRECT_STATE        If aBinding is not prepared
 */
WEAVE_ERROR BdxNode::NewTransfer(Binding * aBinding, BDXHandlers aBDXHandlers,
                                 ReferencedString &aFileDesignator, void * anAppState,
                                 BDXTransfer * &aXfer)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    ExchangeContext * ec = NULL;

    VerifyOrExit(aBinding != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    err = aBinding->NewExchangeContext(ec);
    SuccessOrExit(err);

    // Note: ExchangeContext cleanup is handled within the call below
    err = NewTransfer(ec, aBDXHandlers, aFileDesignator, anAppState, aXfer);

exit:
    return err;
}

/**
 * @brief
 *  Get and set up a new BDXTransfer from transfer pool if available,
 *  or set to NULL otherwise and return an error.
 *
 * @note
 *  You MUST shutdown this transfer in order to release it to the free pool!
 * @note
 *  If a particular handler should not be used in the transfer you are setting up
 *  (e.g., a PutBlockHandler when Sending), set it to NULL.
 * @note
 *  See BDXTransferState.h for details about the handlers.
 *
 * @param[in]   aCon            The WeaveConnection to the node we will initiate the transfer with.
 *                                  It's used to create an associated ExchangeContext for this transfer.
 * @param[in]   aBDXHandlers    A structure of BDX callback handlers to be called during the transfer
 * @param[in]   aFileDesignator The file designator for the file that will be transferred.
 * @param[in]   anAppState      An application-specific state object to be attached to the
 *                                  BDXTransfer for use by the user application and associated callbacks.
 * @param[in]   aXfer           A passed-by-reference pointer that will point to the
 *                                  new BDXTransfer object if one is available, else NULL.
 *
 *
 * @retval      #WEAVE_NO_ERROR     If we successfully found a new BDXTransfer.
 * @retval      #WEAVE_ERROR_NO_MEMORY      If unable to create ExchangeContext
 * @retval      #WEAVE_ERROR_TOO_MANY_CONNECTIONS   If too many transfers are currently active and aXfer is NULL
 * @retval      #WEAVE_ERROR_INCORRECT_STATE        If mExchangeMgr has not been set yet or if aCon has not successfully connected
 */
WEAVE_ERROR BdxNode::NewTransfer(WeaveConnection * aCon, BDXHandlers aBDXHandlers,
                                 ReferencedString &aFileDesignator, void * anAppState,
                                 BDXTransfer * &aXfer)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    ExchangeContext *ec = NULL;

    VerifyOrExit(aCon != NULL, err = WEAVE_ERROR_INCORRECT_STATE);
    VerifyOrExit(aCon->State == WeaveConnection::kState_Connected, err = WEAVE_ERROR_INCORRECT_STATE);

    ec = mExchangeMgr->NewContext(aCon);
    VerifyOrExit(ec != NULL, err = WEAVE_ERROR_NO_MEMORY);

    // Note: ExchangeContext cleanup is handled within the call below
    err = NewTransfer(ec, aBDXHandlers, aFileDesignator, anAppState, aXfer);

exit:
    return err;
}

/**
 * @brief
 *  Get and set up a new BDXTransfer from transfer pool if available,
 *  or set to NULL otherwise and return an error. The ExchangeContext (and possible
 *  underlying WeaveConnection) will have their AppState pointer set to
 *  the BDXTransfer.
 *
 * @note
 *  This is the version of NewTransfer that you should use if you want an encrypted
 *  exchange, which requires setting up the ExchangeContext manually.
 * @note
 *  You MUST shutdown this transfer in order to release it to the free pool!
 * @note
 *  If a particular handler should not be used in the transfer you are setting up
 *  (e.g., a PutBlockHandler when Sending), set it to NULL.
 * @note
 *  See BDXTransferState.h for details about the handlers.
 *
 * @param[in]   anEc            The ExchangeContext with the node we will initiate the transfer with.
 *                                  The server will handle closing it when the transfer is shutdown.
 * @param[in]   aBDXHandlers    A structure of BDX callback handlers to be called during the transfer
 * @param[in]   aFileDesignator The file designator for the file that will be transferred.
 * @param[in]   anAppState      An application-specific state object to be attached to the
 *                                  BDXTransfer for use by the user application and associated callbacks.
 * @param[in]   aXfer           A passed-by-reference pointer that will point to the
 *                                  new BDXTransfer object if one is available, else NULL.
 *
 *
 * @retval      #WEAVE_NO_ERROR                     If we successfully found a new BDXTransfer.
 * @retval      #WEAVE_ERROR_TOO_MANY_CONNECTIONS   If too many transfers are currently active (aXfer is NULL)
 * @retval      #WEAVE_ERROR_INCORRECT_STATE        If ec is NULL, closed, or otherwise invalid for a transfer
 */
WEAVE_ERROR BdxNode::NewTransfer(ExchangeContext *anEc, BDXHandlers aBDXHandlers,
                                 ReferencedString &aFileDesignator, void * anAppState,
                                 BDXTransfer * &aXfer)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    aXfer = NULL;

    err = InitTransfer(anEc, aXfer);
    SuccessOrExit(err);

    // set up the application-specific state
    aXfer->mFileDesignator = aFileDesignator;
    aXfer->mAppState = anAppState;

    // install application-specifc handlers
    aXfer->SetHandlers(aBDXHandlers);

exit:
    if (err != WEAVE_NO_ERROR)
    {
        if (aXfer != NULL)
        {
            aXfer->Shutdown();
        }
        else
        {
            // Transfer object uninitialized, so we do this manually
            if (anEc != NULL)
            {
                anEc->Close();
                anEc = NULL;
            }
        }
    }

    return err;
}

/**
 * @brief
 *  Get and set up a new BDXTransfer from transfer pool if available,
 *  or set to NULL otherwise and return an error.
 *
 * @note
 *  You likely will not be using this method directly as you'll want to
 *  configure the BDXTransfer's parameters at the same time.
 * @note
 *  You MUST shutdown this transfer in order to release it to the free pool!
 *
 * @param[in]   anEc    The ExchangeContext with the node we will initiate the transfer with.
 *                          The server will handle closing it when the transfer is shutdown.
 * @param[in]   aXfer   A passed-by-reference pointer that will point to the
 *                          new BDXTransfer object if one is available, else NULL.
 *
 * @retval      #WEAVE_NO_ERROR                     If we successfully found a new BDXTransfer.
 * @retval      #WEAVE_ERROR_TOO_MANY_CONNECTIONS   If too many transfers are currently active (aXfer is NULL)
 * @retval      #WEAVE_ERROR_INCORRECT_STATE        If ec is NULL, closed, or otherwise invalid for a transfer
 */
WEAVE_ERROR BdxNode::InitTransfer(ExchangeContext *anEc,
                                 BDXTransfer * &aXfer)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    //TODO: check if there is already an ongoing BDX Transfer on this ExchangeContext:
    //only one is allowed at a time according to the BDX specification
    VerifyOrExit(anEc != NULL, err = WEAVE_ERROR_INCORRECT_STATE);
    VerifyOrExit(!anEc->IsConnectionClosed(), err = WEAVE_ERROR_INCORRECT_STATE);

    err = AllocTransfer(aXfer);
    SuccessOrExit(err);

    // Hang new BDXTransfer on exchange context so we can access the xfer object
    // in the static handlers fired in response to incoming messages on the exchange
    anEc->AppState = aXfer;

    // Handle timeouts and other errors on this exchange
    if (anEc->ResponseTimeout == 0)
    {
        anEc->ResponseTimeout = BDX_RESPONSE_TIMEOUT_MS;
    }
    anEc->OnResponseTimeout = BdxProtocol::HandleResponseTimeout;
    anEc->OnConnectionClosed = BdxProtocol::HandleConnectionClosed;
    anEc->OnKeyError = BdxProtocol::HandleKeyError;

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    anEc->OnSendError = BdxProtocol::HandleSendError;
#endif // #if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING

    // Initialize xfer struct
    aXfer->mExchangeContext = anEc;

exit:
    return err;
}

/**
 * @brief
 *  Shutdown the given transfer object and return it to pool.
 *  This simply defers to BDXTransfer::Shutdown()
 *
 * @param[in]   aXfer       The BDXTransfer to shut down
 */
void BdxNode::ShutdownTransfer(BDXTransfer *aXfer)
{
    aXfer->Shutdown();
}

/**
 * @brief
 *  Use to enable/disable the BDX server without fully shutting it down and restarting.
 *
 * @param[in]   aEnable     Enable (true) or disable (false)
 */
void BdxNode::AllowBdxTransferToRun(bool aEnable)
{
    mIsBdxTransferAllowed = aEnable;
}

/**
 * @brief
 *  Returns true if the BDX server is allowed to start a transfer at this time, false otherwise.
 *
 * @return true if BDX transfer is allowed, false if not
 */
bool BdxNode::CanBdxTransferRun(void)
{
    return mIsBdxTransferAllowed;
}

/**
 * @brief
 *  Returns true if this BdxNode has already been initialized.
 *
 * @return true if this object has been initalized
 */
bool BdxNode::IsInitialized(void)
{
    return mInitialized;
}

#if WEAVE_CONFIG_BDX_CLIENT_RECEIVE_SUPPORT
/**
 * @brief
 *  Initializes a BdxReceive transfer with the specified parameters and passes it
 *  to BdxProtocol::InitBdxReceive to send actual ReceiveInit message.
 *  You must first establish a BDXTransfer object via NewTransfer() and configure
 *  it appropriately.
 *
 * @param[in]   aXfer           The initiated and configured transfer state object to
 *                                  use for this transfer
 * @param[in]   aICanDrive      True if the initiator should propose that it can drive,
 *                                  false otherwise
 * @param[in]   aUCanDrive      True if the initiator should propose that the sender can drive,
 *                                  false otherwise
 * @param[in]   aAsyncOk        True if the initiator should propose using async transfer
 * @param[in]   aMetaData       (optional) TLV metadata
 *
 * @retval      #WEAVE_NO_ERROR                 If successful
 * @retval      #WEAVE_ERROR_INCORRECT_STATE    If BDXTransfer can't run right now
 * @retval      #WEAVE_ERROR_NO_MEMORY          BdxProtocol::InitBdxReceive failed to init
 */
WEAVE_ERROR BdxNode::InitBdxReceive(BDXTransfer &aXfer, bool aICanDrive, bool aUCanDrive,
                                    bool aAsyncOk, ReferencedTLVData *aMetaData)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(CanBdxTransferRun(),
                 err = WEAVE_ERROR_INCORRECT_STATE; WeaveLogDetail(BDX, "InitBdxReceive called but server cannot run currently"));

    aXfer.mAmInitiator = true;
    aXfer.mAmSender = false;

    // Arrange for messages in this exchange to go to our response handler.
    // NOTE: we may want to set different handlers for e.g., Receive, Send in the future
    aXfer.mExchangeContext->OnMessageReceived = BdxProtocol::HandleResponse;

    err = BdxProtocol::InitBdxReceive(aXfer, aICanDrive, aUCanDrive, aAsyncOk, aMetaData);

exit:
    return err;
}
#endif // WEAVE_CONFIG_BDX_CLIENT_RECEIVE_SUPPORT

#if WEAVE_CONFIG_BDX_CLIENT_SEND_SUPPORT
/**
 * @brief
 *  Initializes a BdxSend transfer with the specified parameters and passes it
 *  to BdxProtocol::InitBdxSend to send actual SendInit message.
 *  You must first establish a BDXTransfer object via NewTransfer() and configure
 *  it appropriately.
 *
 * @param[in]   aXfer           The initiated and configured transfer state object to
 *                                  use for this transfer
 * @param[in]   aICanDrive      True if the initiator should propose that it can drive,
 *                                  false otherwise
 * @param[in]   aUCanDrive      True if the initiator should propose that the sender can drive,
 *                                  false otherwise
 * @param[in]   aAsyncOk        True if the initiator should propose using async transfer
 * @param[in]   aMetaData       (optional) TLV metadata
 *
 * @retval      #WEAVE_NO_ERROR                 If successful
 * @retval      #WEAVE_ERROR_INCORRECT_STATE    If BDXTransfer can't run right now
 * @retval      #WEAVE_ERROR_NO_MEMORY          BdxProtocol::InitBdxSend failed to init
 */
WEAVE_ERROR BdxNode::InitBdxSend(BDXTransfer &aXfer, bool aICanDrive, bool aUCanDrive,
                                 bool aAsyncOk, ReferencedTLVData *aMetaData)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(CanBdxTransferRun(),
                 err = WEAVE_ERROR_INCORRECT_STATE; WeaveLogDetail(BDX, "InitBdxSend called but server cannot run currently"));

    aXfer.mAmInitiator = true;
    aXfer.mAmSender = true;

    // Arrange for messages in this exchange to go to our response handler.
    aXfer.mExchangeContext->OnMessageReceived = BdxProtocol::HandleResponse;

    err = BdxProtocol::InitBdxSend(aXfer, aICanDrive, aUCanDrive, aAsyncOk, aMetaData);

exit:
    return err;
}

/**
 * @brief
 *  Initializes a BdxSend transfer with the specified parameters and passes it
 *  to BdxProtocol::InitBdxSend to send actual SendInit message.
 *  You must first establish a BDXTransfer object via NewTransfer() and configure
 *  it appropriately.
 *
 * @param[in]   aXfer           The initiated and configured transfer state object to
 *                                  use for this transfer
 * @param[in]   aICanDrive      True if the initiator should propose that it can drive,
 *                                  false otherwise
 * @param[in]   aUCanDrive      True if the initiator should propose that the sender can drive,
 *                                  false otherwise
 * @param[in]   aAsyncOk        True if the initiator should propose using async transfer
 * @param[in]   aMetaDataWriteCallback (optional) Function to write TLV metadata
 * @param[in]   aMetaDataAppState      (optional) AppState for aMetaDataWriteCallback
 *
 * @retval      #WEAVE_NO_ERROR                 If successful
 * @retval      #WEAVE_ERROR_INCORRECT_STATE    If BDXTransfer can't run right now
 * @retval      #WEAVE_ERROR_NO_MEMORY          BdxProtocol::InitBdxSend failed to init
 */
WEAVE_ERROR BdxNode::InitBdxSend(BDXTransfer &aXfer, bool aICanDrive, bool aUCanDrive,
                                 bool aAsyncOk, SendInit::MetaDataTLVWriteCallback aMetaDataWriteCallback,
                                 void *aMetaDataAppState)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(CanBdxTransferRun(),
                 err = WEAVE_ERROR_INCORRECT_STATE; WeaveLogDetail(BDX, "InitBdxSend called but server cannot run currently"));

    aXfer.mAmInitiator = true;
    aXfer.mAmSender = true;

    // Arrange for messages in this exchange to go to our response handler.
    aXfer.mExchangeContext->OnMessageReceived = BdxProtocol::HandleResponse;

    err = BdxProtocol::InitBdxSend(aXfer, aICanDrive, aUCanDrive, aAsyncOk, aMetaDataWriteCallback, aMetaDataAppState);

exit:
    return err;
}
#endif // WEAVE_CONFIG_BDX_CLIENT_SEND_SUPPORT

#if WEAVE_CONFIG_BDX_SERVER_SUPPORT
/**
 * @brief
 *  Allow unsolicited ReceiveInit messages to be handled by the specified handlers on this server.
 *
 * @param[in]   aReceiveInitHandler A handler for ReceiveInit messages that configures a
 *                                      BDXTransfer object and determines whether to accept the
 *                                      transfer based on the parameters contained within the ReceiveInit message.
 *
 * @retval      #WEAVE_NO_ERROR                 If successful
 * @retval      #WEAVE_ERROR_INCORRECT_STATE    If mExchangeMgr hasn't been initialized
 * @retval      other                           A call to register/unregister a message handler failed
 */
WEAVE_ERROR BdxNode::AwaitBdxReceiveInit(ReceiveInitHandler aReceiveInitHandler)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    VerifyOrExit(mExchangeMgr != NULL, err = WEAVE_ERROR_INCORRECT_STATE);

    // Register callback if specified
    if (aReceiveInitHandler != NULL)
    {
        err = mExchangeMgr->RegisterUnsolicitedMessageHandler(kWeaveProfile_BDX, kMsgType_ReceiveInit, HandleReceiveInit, this);
        SuccessOrExit(err);

        mReceiveInitHandler = aReceiveInitHandler;
    }
    // Otherwise, unregister any currently set one
    else
    {
        err = mExchangeMgr->UnregisterUnsolicitedMessageHandler(kWeaveProfile_BDX, kMsgType_ReceiveInit);
    }

exit:
    return err;
}

/**
 * @brief
 *  Allow unsolicited SendInit messages to be handled by the specified handlers on this server.
 *
 * @param[in]   aSendInitHandler    A handler for SendInit messages that configures a
 *                                      BDXTransfer object and determines whether to accept the
 *                                      transfer based on the parameters contained within the SendInit message.
 *
 * @retval      #WEAVE_NO_ERROR                 If successful
 * @retval      #WEAVE_ERROR_INCORRECT_STATE    If mExchangeMgr hasn't been initialized
 * @retval      other                           A call to register/unregister a message handler failed
 */
WEAVE_ERROR BdxNode::AwaitBdxSendInit(SendInitHandler aSendInitHandler)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    VerifyOrExit(mExchangeMgr != NULL, err = WEAVE_ERROR_INCORRECT_STATE);

    if (aSendInitHandler != NULL)
    {
        err = mExchangeMgr->RegisterUnsolicitedMessageHandler(kWeaveProfile_BDX, kMsgType_SendInit, HandleSendInit, this);
        SuccessOrExit(err);

        mSendInitHandler = aSendInitHandler;
    }
    else
    {
        err = mExchangeMgr->UnregisterUnsolicitedMessageHandler(kWeaveProfile_BDX, kMsgType_SendInit);
    }

exit:
    return err;
}

/**
 * @brief
 *  Handler for ReceiveInit messages that parses the incoming message, grabs a
 *  BDXTransfer object (if one is available), passes the ReceiveInit object
 *  to the previously specified ReceiveInitHandler, which will set up
 *  the BDXTransfer object and determine if the ReceiveInit should be accepted,
 *  in which case the handler sends the appropriate response. If anything fails
 *  in this function, we send a reject to tell the initiator that we can't accept.
 *
 * @note
 *  statusCode tracks the BDXProfile Status Error code (to be transferred in case of failure),
 *  while err tracks our internal WEAVE_ERROR, which should never be transmitted.
 *
 * @param[in]   anEc                    ExchangeContext to be associated with this receive init
 * @param[in]   aPktInfo                Unused here, but needs to match function prototype
 * @param[in]   aWeaveMsgInfo           Weave Message Information for the message
 * @param[in]   aProfileId              ID of the profile under which the message is defined
 * @param[in]   aMessageType            The message type of that profile
 * @param[in]   aPayloadReceiveInit     The packed message itself
 */
void BdxNode::HandleReceiveInit(ExchangeContext *anEc, const IPPacketInfo *aPktInfo,
                                const WeaveMessageInfo *aWeaveMsgInfo,
                                uint32_t aProfileId, uint8_t aMessageType,
                                PacketBuffer *aPayloadReceiveInit)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint16_t statusCode = kStatus_Success;
    BdxNode *bdxApp = NULL;
    BDXTransfer *xfer = NULL;
    bool success = false;

    ReceiveInit receiveInit;
    // Set version early in case we exit before parsing. This allows us to send
    // the reject corresponding to the BDX version we're using.
    receiveInit.mVersion = WEAVE_CONFIG_BDX_VERSION;

    WeaveLogDetail(BDX, "HandleReceiveInit entering");

    VerifyOrExit(anEc != NULL, err = WEAVE_ERROR_INCORRECT_STATE; statusCode = kStatus_ServerBadState);

    VerifyOrExit(aProfileId == kWeaveProfile_BDX, err = WEAVE_ERROR_INVALID_PROFILE_ID; statusCode = kStatus_BadMessageContents);
    VerifyOrExit(aMessageType == kMsgType_ReceiveInit, err = WEAVE_ERROR_INVALID_MESSAGE_TYPE; statusCode = kStatus_BadMessageContents);

    // Parse init request and discard payload buffer
    err = ReceiveInit::parse(aPayloadReceiveInit, receiveInit);
    VerifyOrExit(err == WEAVE_NO_ERROR,
                 statusCode = kStatus_BadMessageContents;
                 WeaveLogDetail(BDX, "Error: HandleReceiveInit: Unable to parse Receive Init request: %d", err));
    PacketBuffer::Free(aPayloadReceiveInit);
    aPayloadReceiveInit = NULL;

    bdxApp = static_cast<BdxNode *>(anEc->AppState);

    VerifyOrExit(bdxApp->CanBdxTransferRun(),
                 err = WEAVE_ERROR_INCORRECT_STATE; statusCode = kStatus_ServerBadState);

    // Grab BDXTransfer object for this transfer
    err = bdxApp->InitTransfer(anEc, xfer);
    VerifyOrExit(err == WEAVE_NO_ERROR && xfer != NULL,
                 err = WEAVE_ERROR_NO_MEMORY; statusCode = kStatus_ServerBadState);

    // Configure xfer object
    xfer->mAmInitiator = false;
    xfer->mIsAccepted = false;
    xfer->mAmSender = true;
    xfer->mMaxBlockSize = receiveInit.mMaxBlockSize;
    xfer->mVersion = (receiveInit.mVersion > WEAVE_CONFIG_BDX_VERSION) ? WEAVE_CONFIG_BDX_VERSION : receiveInit.mVersion;

    // Verify we have a legitimate block size or reject
    VerifyOrExit(receiveInit.mMaxBlockSize > 0,
                 err = WEAVE_ERROR_INVALID_ARGUMENT; statusCode = kStatus_BadRequest);

    VerifyOrExit(receiveInit.mFileDesignator.theLength > 0,
                 err = WEAVE_ERROR_INVALID_ARGUMENT; statusCode = kStatus_BadRequest);

    // Fire application callback to validate request and setup transfer
    // Application should set the transfer mode and accept the transfer.
    VerifyOrExit(bdxApp->mReceiveInitHandler,
                 err = WEAVE_ERROR_NO_MESSAGE_HANDLER; statusCode = kStatus_ServerBadState);

    statusCode = bdxApp->mReceiveInitHandler(xfer, &receiveInit);
    VerifyOrExit(statusCode == kStatus_Success, err = WEAVE_ERROR_INCORRECT_STATE);

    // Validate the requested transfer mode
    VerifyOrExit(!(((xfer->mTransferMode == kMode_ReceiverDrive) && !receiveInit.mReceiverDriveSupported) ||
                   ((xfer->mTransferMode == kMode_SenderDrive) && !receiveInit.mSenderDriveSupported) ||
                   ((xfer->mTransferMode == kMode_Asynchronous))),
        //TODO: merge this up one line when async supported: && !receiveInit.mAsynchronousModeSupported)
                 err = WEAVE_ERROR_INVALID_TRANSFER_MODE; statusCode = kStatus_ServerBadState);

    // TODO: validate max block size?  anything else?
    WeaveLogDetail(BDX, "HandleReceiveInit validated request\n");

    err = SendReceiveAccept(anEc, xfer);
    VerifyOrExit(err == WEAVE_NO_ERROR, statusCode = kStatus_FailureToSend);

    WeaveLogDetail(BDX, "HandleReceiveInit exiting (success)\n");
    success = true;

exit:
    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogError(BDX, "HandleReceiveInit exiting (failure = %d)", err);
    }

    if (aPayloadReceiveInit)
    {
        PacketBuffer::Free(aPayloadReceiveInit);
    }

    if (!success)
    {
        if (statusCode != kStatus_BadMessageContents)
        {
            // Send a reject if we didn't succeed at handling the receive init and
            // it was not due to bad message contents.
            err = SendReject(anEc, receiveInit.mVersion, statusCode, kMsgType_ReceiveReject);
            if (err != WEAVE_NO_ERROR)
            {
                WeaveLogDetail(BDX, "Sending reject message failed : %d", err);
            }
        }

        if (xfer != NULL)
        {
            xfer->Shutdown();
        }
        else
        {
            // Transfer object uninitialized, so we do this manually
            if (anEc != NULL)
            {
                if (anEc->Con != NULL)
                {
                    anEc->Con->Close();
                    anEc->Con = NULL;
                }

                anEc->Close();
                anEc = NULL;
            }
        }
    }
}

/**
 * @brief
 *  Handler for SendInit messages that parses the incoming message, grabs a
 *  BDXTransfer object (if one is available), passes the SendInit object
 *  to the previously specified SendInitHandler, which will set up
 *  the BDXTransfer object and determine if the SendInit should be accepted,
 *  in which case the handler sends the appropriate response. If anything fails
 *  in this function, we send a reject to tell the initiator that we can't accept.
 *
 * @note
 *  statusCode tracks the BDXProfile Status Error code (to be transferred in case of failure),
 *  while err tracks our internal WEAVE_ERROR, which should never be transmitted.
 *
 * @param[in]   anEc                    ExchangeContext to be associated with this send init
 * @param[in]   aPktInfo                Unused here, but needs to match function prototype
 * @param[in]   aWeaveMsgInfo           Weave Message Information for the message
 * @param[in]   aProfileId              ID of the profile under which the message is defined
 * @param[in]   aMessageType            The message type of that profile
 * @param[in]   aPayload                The packed message itself
 */
    //TODO: pull in documentation from WeaveExchangeMgr.h::MessageReceiveFunct
void BdxNode::HandleSendInit(ExchangeContext *anEc, const IPPacketInfo *aPktInfo,
                             const WeaveMessageInfo *aWeaveMsgInfo,
                             uint32_t aProfileId, uint8_t aMessageType,
                             PacketBuffer *aPayload)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint16_t statusCode = kStatus_Success;
    BdxNode * bdxApp;
    BDXTransfer *xfer = NULL;
    bool success = false;

    SendInit sendInit;
    // Set version early in case we exit before parsing. This allows us to send
    // the reject corresponding to the BDX version we're using.
    sendInit.mVersion = WEAVE_CONFIG_BDX_VERSION;

    WeaveLogDetail(BDX, "HandleSendInit entering");

    VerifyOrExit(anEc != NULL, err = WEAVE_ERROR_INCORRECT_STATE; statusCode = kStatus_ServerBadState);

    VerifyOrExit(aProfileId == kWeaveProfile_BDX, err = WEAVE_ERROR_INVALID_PROFILE_ID; statusCode = kStatus_BadMessageContents);
    VerifyOrExit(aMessageType == kMsgType_SendInit, err = WEAVE_ERROR_INVALID_MESSAGE_TYPE; statusCode = kStatus_BadMessageContents);

    // Parse init request and discard payload buffer
    err = SendInit::parse(aPayload, sendInit);
    VerifyOrExit(err == WEAVE_NO_ERROR,
                 statusCode = kStatus_BadMessageContents;
                 WeaveLogDetail(BDX, "Error: HandleSendInit: Unable to parse Send Init request: %d", err));
    PacketBuffer::Free(aPayload);
    aPayload = NULL;

    bdxApp = static_cast<BdxNode *>(anEc->AppState);

    VerifyOrExit(bdxApp->CanBdxTransferRun(),
                 err = WEAVE_ERROR_INCORRECT_STATE; statusCode = kStatus_ServerBadState);

    // Grab BDXTransfer object for this transfer
    err = bdxApp->InitTransfer(anEc, xfer);
    VerifyOrExit(err == WEAVE_NO_ERROR && xfer != NULL,
                 err = WEAVE_ERROR_NO_MEMORY; statusCode = kStatus_ServerBadState);

    // Configure xfer object
    xfer->mIsAccepted = false;
    xfer->mMaxBlockSize = sendInit.mMaxBlockSize;
    xfer->mAmInitiator = false;
    xfer->mAmSender = false;
    xfer->mVersion = (sendInit.mVersion > WEAVE_CONFIG_BDX_VERSION) ? WEAVE_CONFIG_BDX_VERSION : sendInit.mVersion;

    // Fire application callback to validate request and setup transfer
    // Application should set the transfer mode and accept the transfer.
    VerifyOrExit(bdxApp->mSendInitHandler,
                 err = WEAVE_ERROR_NO_MESSAGE_HANDLER; statusCode = kStatus_ServerBadState);

    statusCode = bdxApp->mSendInitHandler(xfer, &sendInit);
    VerifyOrExit(statusCode == kStatus_Success, err = WEAVE_ERROR_INCORRECT_STATE);

    // Validate the requested transfer mode
    VerifyOrExit(!(((xfer->mTransferMode == kMode_ReceiverDrive) && !sendInit.mReceiverDriveSupported) ||
                   ((xfer->mTransferMode == kMode_SenderDrive) && !sendInit.mSenderDriveSupported) ||
                   ((xfer->mTransferMode == kMode_Asynchronous))),
        //TODO: merge this up one line when async supported: && !sendInit.mAsynchronousModeSupported)
                 err = WEAVE_ERROR_INVALID_TRANSFER_MODE; statusCode = kStatus_ServerBadState);

    WeaveLogDetail(BDX, "HandleSendInit validated request\n");

    err = SendSendAccept(anEc, xfer);
    VerifyOrExit(err == WEAVE_NO_ERROR, statusCode = kStatus_FailureToSend);

    WeaveLogDetail(BDX, "HandleSendInit exiting (success)\n");
    success = true;

exit:
    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogError(BDX, "HandleSendInit exiting on failure: %s\n", ErrorStr(err));
    }

    if (aPayload)
    {
        PacketBuffer::Free(aPayload);
    }

    if (!success)
    {
        if (statusCode != kStatus_BadMessageContents)
        {
            // Send a reject if we didn't succeed at handling the receive init
            err = SendReject(anEc, sendInit.mVersion, statusCode, kMsgType_SendReject);
            if (err != WEAVE_NO_ERROR)
            {
                WeaveLogDetail(BDX, "Sending reject message failed : %d", err);
            }
        }

        if (xfer != NULL)
        {
            xfer->Shutdown();
        }
        else
        {
            // Transfer object uninitialized, so we do this manually
            if (anEc != NULL)
            {
                if (anEc->Con != NULL)
                {
                    anEc->Con->Close();
                    anEc->Con = NULL;
                }

                anEc->Close();
                anEc = NULL;
            }
        }
    }
}

/**
 * Function to send a reject message with the specified error.
 */
WEAVE_ERROR BdxNode::SendReject(ExchangeContext *anEc, uint8_t aVersion, uint16_t anErr, uint8_t aMsgType)
{
    WEAVE_ERROR     err             = WEAVE_NO_ERROR;
    StatusReport    rejectStatus;
    PacketBuffer*   responsePayload = NULL;
    uint16_t        flags           = 0;

    VerifyOrExit(aMsgType == kMsgType_SendReject || aMsgType == kMsgType_ReceiveReject,
                 err = WEAVE_ERROR_INVALID_ARGUMENT);

    WeaveLogDetail(BDX, "%s sending Reject due to error: %d", __FUNCTION__, anErr);

    err = rejectStatus.init(kWeaveProfile_BDX, anErr);
    VerifyOrExit(err == WEAVE_NO_ERROR,
                 WeaveLogDetail(BDX, "SendReject error calling Init on rejectStatus: %d", err));

    responsePayload = PacketBuffer::New();
    VerifyOrExit(responsePayload != NULL,
                 err = WEAVE_ERROR_NO_MEMORY; WeaveLogDetail(BDX, "SendReject couldn't grab PacketBuffer"));

    err = rejectStatus.pack(responsePayload);
    VerifyOrExit(err == WEAVE_NO_ERROR,
                 WeaveLogDetail(BDX, "SendReject couldn't pack rejectStatus: %d", err));

    flags = GetBDXAckFlag(anEc);
    if (aVersion == 0)
    {
        err = anEc->SendMessage(kWeaveProfile_BDX, aMsgType, responsePayload, flags);
    }
    else if (aVersion == 1)
    {
        err = anEc->SendMessage(kWeaveProfile_Common, Common::kMsgType_StatusReport, responsePayload, flags);
    }
    else
    {
        err = WEAVE_ERROR_UNSUPPORTED_MESSAGE_VERSION;
    }

    VerifyOrExit(err == WEAVE_NO_ERROR,
                 WeaveLogDetail(BDX, "SendReject error sending reject message: %d", err));

    responsePayload = NULL;

exit:
    if (responsePayload)
    {
        PacketBuffer::Free(responsePayload);
    }

    return err;
}

/**
 * Function to send a receive accept for the given transfer
 */
WEAVE_ERROR BdxNode::SendReceiveAccept(ExchangeContext *anEc, BDXTransfer *aXfer)
{
    WEAVE_ERROR     err             = WEAVE_NO_ERROR;
    ReceiveAccept   receiveAccept;
    PacketBuffer*   payload         = NULL;
    uint16_t        flags;

    // Send a ReceiveAccept response back to the receiver.
    err = receiveAccept.init(aXfer->mVersion, aXfer->mTransferMode, aXfer->mMaxBlockSize, aXfer->mLength, NULL);
    VerifyOrExit(err == WEAVE_NO_ERROR,
                 WeaveLogDetail(BDX, "SendReceiveAccept error calling Init on receiveAccept: %d", err));

    payload = PacketBuffer::New();
    VerifyOrExit(payload != NULL,
                 err = WEAVE_ERROR_NO_MEMORY;
                 WeaveLogDetail(BDX, "SendReceiveAccept error grabbing PacketBuffer"));

    err = receiveAccept.pack(payload);
    VerifyOrExit(err == WEAVE_NO_ERROR,
                 WeaveLogDetail(BDX, "SendReceiveAccept error packing receiveAccept : %d\n", err));

    // Set ourselves up to handle first BlockQueryRequest.
    anEc->OnMessageReceived = BdxProtocol::HandleResponse;

    // Expect a response if we are not the driver
    flags = aXfer->GetDefaultFlags(!aXfer->IsDriver());

    err = anEc->SendMessage(kWeaveProfile_BDX, kMsgType_ReceiveAccept, payload, flags);
    payload = NULL;
    VerifyOrExit(err == WEAVE_NO_ERROR, WeaveLogDetail(BDX, "SendReceiveAccept error sending accept message: %d", err));

    // Send first block if we're driving
    if (aXfer->IsDriver())
    {
        WeaveLogDetail(BDX, "ReceiveAccept sent: Am driving so sending first block");
        if (aXfer->mVersion == 1)
        {
            err = BdxProtocol::SendNextBlockV1(*aXfer);
        }
#if WEAVE_CONFIG_BDX_V0_SUPPORT
        else if (aXfer->mVersion == 0)
        {
            err = BdxProtocol::SendNextBlock(*aXfer);
        }
#endif // WEAVE_CONFIG_BDX_V0_SUPPORT
        else
        {
            err = WEAVE_ERROR_UNSUPPORTED_MESSAGE_VERSION;
        }

        VerifyOrExit(err == WEAVE_NO_ERROR,
                     WeaveLogDetail(BDX, "Error sending first block in SendReceiveAccept: %d", err));
    }

exit:
    if (payload)
    {
        PacketBuffer::Free(payload);
    }

    return err;
}

/**
 * Function to send a send accept for the given transfer
 */
WEAVE_ERROR BdxNode::SendSendAccept(ExchangeContext *anEc, BDXTransfer *aXfer)
{
    WEAVE_ERROR     err         = WEAVE_NO_ERROR;
    SendAccept      sendAccept;
    PacketBuffer*   payload     = NULL;
    uint16_t        flags;

    // Send a ReceiveAccept response back to the receiver.
    err = sendAccept.init(aXfer->mVersion, aXfer->mTransferMode, aXfer->mMaxBlockSize, NULL);
    VerifyOrExit(err == WEAVE_NO_ERROR,
                 WeaveLogDetail(BDX, "SendSendAccept error calling Init on sendAccept: %d", err));

    payload = PacketBuffer::New();
    VerifyOrExit(payload != NULL,
                 err = WEAVE_ERROR_NO_MEMORY;
                 WeaveLogDetail(BDX, "SendSendAccept error grabbing PacketBuffer"));

    err = sendAccept.pack(payload);
    VerifyOrExit(err == WEAVE_NO_ERROR,
                 WeaveLogDetail(BDX, "SendSendAccept error packing sendAccept : %d\n", err));

    // Set ourselves up to handle first BlockQueryRequest.
    anEc->OnMessageReceived = BdxProtocol::HandleResponse;

    // Expect a response if we are not the driver
    flags = aXfer->GetDefaultFlags(!aXfer->IsDriver());

    err = anEc->SendMessage(kWeaveProfile_BDX, kMsgType_SendAccept, payload, flags);
    payload = NULL;
    VerifyOrExit(err == WEAVE_NO_ERROR,
                 WeaveLogDetail(BDX, "SendSendAccept error sending accept message: %d", err));

    // Send a block query if we're driving
    if (aXfer->IsDriver())
    {
        WeaveLogDetail(BDX, "SendAccept sent: Am driving so sending first block query");
        if (aXfer->mVersion == 1)
        {
            err = BdxProtocol::SendBlockQueryV1(*aXfer);
        }
#if WEAVE_CONFIG_BDX_V0_SUPPORT
        else if (aXfer->mVersion == 0)
        {
            err = BdxProtocol::SendBlockQuery(*aXfer);
        }
#endif // WEAVE_CONFIG_BDX_V0_SUPPORT
        else
        {
            err = WEAVE_ERROR_UNSUPPORTED_MESSAGE_VERSION;
        }

        VerifyOrExit(err == WEAVE_NO_ERROR,
                     WeaveLogDetail(BDX, "Error sending first block query in SendSendAccept: %d", err));
    }

exit:
    if (payload)
    {
        PacketBuffer::Free(payload);
    }

    return err;
}
#endif // WEAVE_CONFIG_BDX_SERVER_SUPPORT

} // namespace BulkDataTransfer
} // namespace Profiles
} // namespace Weave
} // namespace nl
