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
 *      This file declares the basic Weave Bulk Data Transfer Server.
 *      Note that this class manages BDXTransfer objects, which contain state
 *      pertaining to a currently ongoing transfer, and any Weave-related
 *      resources necessary for establishing connections, etc.  The BdxProtocol
 *      class contains the actual protocol logic that handles manipulating these
 *      BDXTransfer objects but it does not contain any state itself.
 */

#ifndef _WEAVE_BDX_NODE_H
#define _WEAVE_BDX_NODE_H

#include <Weave/Profiles/bulk-data-transfer/Development/BDXManagedNamespace.hpp>
#include <Weave/Profiles/bulk-data-transfer/Development/BDXProtocol.h>
#include <Weave/Profiles/bulk-data-transfer/Development/BDXMessages.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(BDX, kWeaveManagedNamespaceDesignation_Development) {

/*
 * In order to make life easier for users of the bulk data transfer
 * protocol, we provide this server. The word "server" is chosen here
 * even though it can act both as a client or as a server.  While the client
 * is expected to only handle one ongoing transfer at a time, the server may
 * handle many.  Thus, configuring the number of BDXTransfer contained within
 * the server to 1 would make this into a 'client'.
 *
 * Note that the server defers to the BdxProtocol class for handling BDX
 * messages and the proper responses and maintenance of the BDXTransfer state.
 * Thus, this class is mostly responsible for managing these transfers and their
 * Weave-related resources.
 *
 * The main points of entry for users of the BDX Protocol are to call InitBdxSend(),
 * InitBdxReceive(), AwaitBdxReceiveInit(), or AwaitBdxSendInit() after creating and initializing the server
 * (init must be properly called before the server can be used)
 * or obtaining a reference to an existing one. Note that in order to use the first
 * two aforementioned methods, the user must first obtain and configure a BDXTransfer
 * object using NewTransfer().  This allows them the opportunity to configure the
 * callbacks and various parameters affecting the Transfer appropriately before
 * initializing it.
 */
//TODO: to lower the code footprint for a client application on a highly-constrained
// device, one might consider wrapping #ifdefs around AwaitBdxSendInit, AwaitBdxReceiveInit,
// HandleReceiveInit, and HandleSendInit to disable those features and entirely remove
// their code for a device not meant to support them.
class NL_DLL_EXPORT BdxNode
{
public:

    /** Default constructor that sets all members to NULL. Don't try to do anything
     * with the server until you've at least called init(). */
    BdxNode(void);

    WEAVE_ERROR Init(WeaveExchangeManager* anExchangeMgr);

    WEAVE_ERROR Shutdown(void);

    WEAVE_ERROR NewTransfer(Binding *aBinding, BDXHandlers aBDXHandlers,
                            ReferencedString &aFileDesignator, void * anAppState,
                            BDXTransfer * &aXfer);

    WEAVE_ERROR NewTransfer(WeaveConnection *aCon, BDXHandlers aBDXHandlers,
                            ReferencedString &aFileDesignator, void *anAppState,
                            BDXTransfer * &aXfer);

    static void ShutdownTransfer(BDXTransfer *aXfer);

    void AllowBdxTransferToRun(bool aEnable);

    bool CanBdxTransferRun(void);

    bool IsInitialized(void);

    WEAVE_ERROR InitBdxReceive(BDXTransfer &aXfer, bool aICanDrive, bool aUCanDrive,
                               bool aAsyncOk, ReferencedTLVData *aMetaData);

    WEAVE_ERROR InitBdxSend(BDXTransfer &aXfer, bool aICanDrive, bool aUCanDrive,
                            bool aAsyncOk, ReferencedTLVData *aMetaData);
    WEAVE_ERROR InitBdxSend(BDXTransfer &aXfer, bool aICanDrive, bool aUCanDrive,
                            bool aAsyncOk, SendInit::MetaDataTLVWriteCallback aMetaDataWriteCallback,
                            void *aMetaDataAppState);

    WEAVE_ERROR AwaitBdxReceiveInit(ReceiveInitHandler aReceiveInitHandler);
    WEAVE_ERROR AwaitBdxSendInit(SendInitHandler aSendInitHandler);

    static void HandleSendInit(ExchangeContext *anEc, const IPPacketInfo *aPktInfo,
                               const WeaveMessageInfo *aWeaveMsgInfo,
                               uint32_t aProfileId, uint8_t aMessageType,
                               PacketBuffer *aPacketBuffer);

    static void HandleReceiveInit(ExchangeContext *anEc, const IPPacketInfo *aPktInfo,
                                  const WeaveMessageInfo *aWeaveMsgInfo,
                                  uint32_t aProfileId, uint8_t aMessageType,
                                  PacketBuffer *aPacketBuffer);

private:
    WEAVE_ERROR NewTransfer(ExchangeContext *anEc, BDXHandlers aBDXHandlers,
                            ReferencedString &aFileDesignator, void *anAppState,
                            BDXTransfer * &aXfer);

    WEAVE_ERROR AllocTransfer(BDXTransfer * &aXfer);

    WEAVE_ERROR InitTransfer(ExchangeContext *anEc, BDXTransfer * &aXfer);

    WeaveExchangeManager *mExchangeMgr;

    bool mIsBdxTransferAllowed;              // True when server is allowed to start transfers
    bool mInitialized;

    BDXTransfer mTransferPool[WEAVE_CONFIG_BDX_MAX_NUM_TRANSFERS];

    // Application programmer-defined callbacks that take a Send/ReceiveInit message and a BDXTransfer,
    // determining whether they want to accept a transfer or not and setting up
    // appropriate application-specific resources.  See BdxProtocol.h for details.
    SendInitHandler mSendInitHandler;
    ReceiveInitHandler mReceiveInitHandler;

    static WEAVE_ERROR SendReject(ExchangeContext *anEc, uint8_t aVersion, uint16_t anErr, uint8_t aMsgType);
    static WEAVE_ERROR SendReceiveAccept(ExchangeContext *anEc, BDXTransfer *aXfer);
    static WEAVE_ERROR SendSendAccept(ExchangeContext *anEc, BDXTransfer *aXfer);
};

// Typedef to obscure the fact that Client and Server are the same code
typedef BdxNode BdxClient;
typedef BdxNode BdxServer;

} // namespace BulkDataTransfer
} // namespace Profiles
} // namespace Weave
} // namespace nl

#endif // _WEAVE_BDX_NODE_H
