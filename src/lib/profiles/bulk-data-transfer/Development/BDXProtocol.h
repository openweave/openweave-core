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
 *      This file declares the basic Weave Bulk Data Transfer Protocol.
 *      This class contains the actual protocol logic that handles manipulating these
 *      BDXTransfer objects but it does not contain any state itself.
 *
 *      Note that many of the message handlers are actually implemented
 *      outside of this file so that they may be customized, dynamically loaded,
 *      etc. and are then dispatched in response to the appropriate message by
 *      this class.  In this manner, the logic of the Weave BDX protocol is
 *      contained within these definitions, but the actual implementation of how
 *      to read/write files, negotiating who drives, the block size, etc. is
 *      contained within separate definitions so that application programmers
 *      can implement their own versions.
 *
 */

#ifndef _WEAVE_BDX_PROTOCOL_H
#define _WEAVE_BDX_PROTOCOL_H

#include <Weave/Profiles/bulk-data-transfer/Development/BDXManagedNamespace.hpp>
#include <Weave/Profiles/bulk-data-transfer/Development/BDXTransferState.h>
#include <Weave/Profiles/bulk-data-transfer/Development/BDXMessages.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(BDX, kWeaveManagedNamespaceDesignation_Development) {
namespace BdxProtocol {

// The following two handlers are protocol entry points

WEAVE_ERROR InitBdxReceive(BDXTransfer &aXfer, bool aICanDrive, bool aUCanDrive,
                           bool aAsyncOk, ReferencedTLVData *aMetaData);

WEAVE_ERROR InitBdxSend(BDXTransfer &aXfer, bool aICanDrive, bool aUCanDrive,
                        bool aAsyncOk, ReferencedTLVData *aMetaData);
WEAVE_ERROR InitBdxSend(BDXTransfer &aXfer, bool aICanDrive, bool aUCanDrive,
                        bool aAsyncOk, SendInit::MetaDataTLVWriteCallback aMetaDataWriteCallback, void *aMetaDataAppState);

// Helper functions for handling Weave BDX sends

WEAVE_ERROR SendBlockQuery(BDXTransfer &aXfer);

WEAVE_ERROR SendBlockQueryV1(BDXTransfer &aXfer);

WEAVE_ERROR SendNextBlock(BDXTransfer &aXfer);

WEAVE_ERROR SendNextBlockV1(BDXTransfer &aXfer);

// The following handlers are stateless callbacks meant to be passed to the
// ExchangeContext in order to handle incoming BDX messages.
// They handle the actual BDX protocol interaction and defer to the previously
// defined callbacks for application-specific logic.

void HandleResponse(ExchangeContext *anEc, const IPPacketInfo *aPktInfo, const WeaveMessageInfo *aWeaveMsgInfo,
                    uint32_t aProfileId, uint8_t aMessageType, PacketBuffer *aPacketBuffer);

void HandleConnectionClosed(ExchangeContext *anEc, WeaveConnection *aCon, WEAVE_ERROR aConErr);

void HandleResponseTimeout(ExchangeContext *anEc);

void HandleKeyError(ExchangeContext *anEc, WEAVE_ERROR aKeyErr);

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
void HandleSendError(ExchangeContext *anEc, WEAVE_ERROR aSendErr, void *aMsgCtxt);
#endif // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING

void SendTransferError(ExchangeContext *anEc, uint32_t aProfileId,  uint16_t aStatusCode);

void SendStatusReport(ExchangeContext *anEc, uint32_t aProfileId,  uint16_t aStatusCode);

/* Private functions only to be called from this group of functions*/
WEAVE_ERROR HandleResponseTransmit(BDXTransfer &aXfer, uint32_t aProfileId,
                                   uint8_t aMessageType, PacketBuffer *aPacketBuffer);

WEAVE_ERROR HandleResponseReceive(BDXTransfer &aXfer, uint32_t aProfileId,
                                  uint8_t aMessageType, PacketBuffer *aPacketBuffer);

WEAVE_ERROR HandleResponseNotAccepted(BDXTransfer &aXfer, uint32_t aProfileId,
                                      uint8_t aMessageType, PacketBuffer *aPacketBuffer);

} // namespace BdxProtocol
} // namespace BulkDataTransfer
} // namespace Profiles
} // namespace Weave
} // namespace nl

#endif // _WEAVE_BDX_PROTOCOL_H
