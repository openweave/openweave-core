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
 *      The bulk data transfer protocol has a number applications
 *      including sensor data upload, in-band firmware download and
 *      log upload. As such, it needs to be flexible and accomodate a
 *      wide variety of use cases. The protocol defines a number of
 *      roles or role pairs:
 *
 *      - sender/receiver, the entities that send and receive the data
 *        of interest.
 *      - initiator/responder, the entities that initiate the exchange
 *        and respond to (by accepting or rejecting) the solicitation.
 *      - driver, the device that governs the rate of the exchange by,
 *        for example, requesting the next block of data.
 *
 *      Note that either the sender or the receiver may initiate the
 *      exchange and either may be the driver.
 *
 *      There is also a notion that the protocol may proceed
 *      synchronously or asynchronously. the latter case, the role of
 *      driver is suppressed.
 *
 *      The protocol has the following message types:
 *
 *      - SendInit, sent when the sender in a bulk data transfer
 *        wishes to initiate the exchange, e.g. if an embedded device
 *        wants to transfer some log data.
 *      - SendAccept,
 *      - SendReject,
 *      - ReceiveInit, sent when the receiver in a bulk data transfer
 *        wishes to initiate the exchange, e.g. if an embedded device
 *        wants to download an image.
 *      - ReceiveAccept
 *      - ReceiveReject
 *      - BlockQuery
 *      - BlockSend
 *      - BlockEOF
 *      - BlockAck
 *      - BlockEOFAck
 *      - TransferError
 *
 */

#ifndef _BULK_DATA_TRANSFER_PROFILE_H
#define _BULK_DATA_TRANSFER_PROFILE_H

#include <Weave/Profiles/bulk-data-transfer/BDXManagedNamespace.hpp>
#include <Weave/Support/NLDLLUtil.h>

/**
 *   @namespace nl::Weave::Profiles::BulkDataTransfer
 *
 *   @brief
 *     This namespace includes all interfaces within Weave for the
 *     Weave Bulk Data Transfer (BDX) profile.
 */

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(BDX, kWeaveManagedNamespaceDesignation_Current) {
  // the message type values
  enum {
    kMsgType_SendInit               = 0x01,
    kMsgType_SendAccept             = 0x02,
    kMsgType_SendReject             = 0x03,
    kMsgType_ReceiveInit            = 0x04,
    kMsgType_ReceiveAccept          = 0x05,
    kMsgType_ReceiveReject          = 0x06,
    kMsgType_BlockQuery             = 0x07,
    kMsgType_BlockSend              = 0x08,
    kMsgType_BlockAck               = 0x0A,
    kMsgType_BlockEOF               = 0x09,
    kMsgType_BlockEOFAck            = 0x0B,
    kMsgType_TransferError          = 0x0F,
  };
  /*
   * as described above there are 3 mutually exclusive transfer modes:
   * - sender drive
   * - receiver drive
   * - asynchronous
   * these are set up as bit values so they can be ORed together to
   * reflect device capabilities.
   */
  enum {
    kMode_SenderDrive               = 0x10,
    kMode_ReceiverDrive             = 0x20,
    kMode_Asynchronous              = 0x40,
  };
  /*
   * with respect to range control, there are several options:
   * - definite length, if set then the transfer has definite length
   * - start offset present, if set then the date to be transferred has
   * an initial offset.
   * - wide range, if set then the offset values during the file transfer will
   * be 8 bytes in length. otherwise they will be 4 bytes in length.
   * again, these are defined so as to be interpreted as bit fields.
   */
  enum {
    kRangeCtl_DefiniteLength        = 0x01,
    kRangeCtl_StartOffsetPresent    = 0x02,
    kRangeCtl_WideRange             = 0x10,
  };
  /*
   * status/error codes for BDX
   */
  enum {
    kStatus_Overflow                = 0x0011,
    kStatus_LengthTooShort          = 0x0013,
    kStatus_XferFailedUnknownErr    = 0x001F,
    kStatus_XferMethodNotSupported  = 0x0050,
    kStatus_UnknownFile             = 0x0051,
    kStatus_StartOffsetNotSupported = 0x0052,
    kStatus_Unknown                 = 0x005F,
  };
  /*
   * and now the message definitions.
   *
   * the SendInit message is used to start an exchange when the sender
   * is the initiator.
   */
  class NL_DLL_EXPORT SendInit {
  public:
    // constructor
    SendInit();
    // initializer
    WEAVE_ERROR init(bool, bool, bool, uint16_t, uint64_t, uint64_t, ReferencedString&, ReferencedTLVData*);
    WEAVE_ERROR init(bool, bool, bool, uint16_t, uint32_t, uint32_t, ReferencedString&, ReferencedTLVData*);
    // packing and parsing
    WEAVE_ERROR pack(PacketBuffer*);
    uint16_t packedLength();
    static WEAVE_ERROR parse(PacketBuffer*, SendInit&);
    // comparison
    bool operator == (const SendInit&) const;
    // data members
    // the transfer mode options
    bool senderDriveSupported;
    bool receiverDriveSupported;
    bool asynchronousModeSupported;
    // the range control options
    bool definiteLength;
    bool startOffsetPresent;
    bool wideRange;
    // block size and offset
    uint16_t theMaxBlockSize;
    uint64_t theStartOffset;
    uint64_t theLength;
    // the file designator
    ReferencedString theFileDesignator;
    // additional metadata
    ReferencedTLVData theMetaData;
  };
  /*
   * the SendAccept message is used to accept a proposed exchange when the
   * sender is the initiator.
   */
  class SendAccept {
  public:
    // constructor
    SendAccept();
    // initializer
    WEAVE_ERROR init(uint8_t, uint16_t, ReferencedTLVData*);
    // packing and parsing
    WEAVE_ERROR pack(PacketBuffer*);
    uint16_t packedLength();
    static WEAVE_ERROR parse(PacketBuffer*, SendAccept&);
    // comparison
    bool operator == (const SendAccept&) const;
    // data members
    uint8_t theTransferMode;
    uint16_t theMaxBlockSize;
    ReferencedTLVData theMetaData;
  };
  /*
   * the SendReject message is used to reject a proposed exchange when the
   * sender is the initiator.
   */
  class SendReject : public StatusReport { };
  /*
   * the ReceiveInit message is used to start an exchange when the receiver
   * is the initiator.
   */
  class NL_DLL_EXPORT ReceiveInit : public SendInit {
  public:
    // constructor
    ReceiveInit();
  };
  /*
   * the ReceiveAccept message is used to accept a proposed exchange when the
   * receiver is the initiator.
   */
  class ReceiveAccept : public SendAccept {
  public:
    // constructor
    ReceiveAccept();
    // initializer
    WEAVE_ERROR init(uint8_t, uint16_t, uint64_t, ReferencedTLVData*);
    WEAVE_ERROR init(uint8_t, uint16_t, uint32_t, ReferencedTLVData*);
    // packing and parsing
    WEAVE_ERROR pack(PacketBuffer*);
    uint16_t packedLength();
    static WEAVE_ERROR parse(PacketBuffer*, ReceiveAccept&);
    // comparison
    bool operator == (const ReceiveAccept&) const;
    // data members
    // the accepted range control options
    bool definiteLength;
    bool wideRange;
    uint64_t theLength;
    ReferencedTLVData theMetaData;
  };
  /*
   * the ReceiveReject message is used to reject a proposed exchange when the
   * sender is the initiator.
   */
  class ReceiveReject : public StatusReport { };
  /*
   * the BlockQuery message is used to request that a block of data
   * be transfered from sender to receiver.
   */
  class NL_DLL_EXPORT BlockQuery {
  public:
    // constructor
    BlockQuery();
    // initializer
    WEAVE_ERROR init(uint8_t);
    // packing and parsing
    WEAVE_ERROR pack(PacketBuffer*);
    uint16_t packedLength();
    static WEAVE_ERROR parse(PacketBuffer*, BlockQuery&);
    // comparison
    bool operator == (const BlockQuery&) const;
    // data members
    uint8_t theBlockCounter;
  };
  /*
   * the BlockSend message is used to transfer a block of data from sender
   * to receiver.
   */
  class BlockSend : public RetainedPacketBuffer
  {
  public:
    // constructor
    BlockSend();
    // initializer
    WEAVE_ERROR init(uint8_t, uint64_t, uint8_t*);
    // packing and parsing
    WEAVE_ERROR pack(PacketBuffer*);
    uint16_t packedLength();
    static WEAVE_ERROR parse(PacketBuffer*, BlockSend&);
    // comparison
    bool operator == (const BlockSend&) const;
    // data members
    uint8_t theBlockCounter;
    uint64_t theLength;
    uint8_t *theData;
  };
  /*
   * the BlockEOF message is used to transfer the last block of data from
   * sender to receiver.
   */
  class BlockEOF : public BlockSend { };
  /*
   * the BlockAck message is used to acknowledge a block of data
   */
  class BlockAck : public BlockQuery { };
  /*
   * the BlockEOFAck message is used to acknowledge the last block of data
   */
  class BlockEOFAck : public BlockQuery { };
  /*
   * the Error message is used to report an error and abort an exchange
   */
  class TransferError : public StatusReport { };
  /*
   * in order to make life easier for users of the bulk data transfer
   * protocol, we provide this client. the word "client" is somewhat
   * overloaded as a general rule so, to be clear, in this context it means,
   * "the enitity that is not the server". that is, a server may
   * may reasonably expected to handle transfers for multiple clients
   * while a client should expect, at least for present purposes, to
   * deal with a single server with a known address.
   *
   * philosophical considerations aside, a BDX client may be any possible
   * triple of sender/receiver, initiator/responder and driver/... uhh
   * passenger.
   */
  class NL_DLL_EXPORT WeaveBdxClient {
  public:
    // constructors
    WeaveBdxClient();
    // init/Shutdown
    WEAVE_ERROR initClient(WeaveExchangeManager*, void*, ReferencedString&, uint16_t, uint64_t, uint64_t, bool);
    WEAVE_ERROR shutdownClient();
    WEAVE_ERROR shutdownClient(WEAVE_ERROR aErr);
    // typedefs for handler types needed below
    typedef void (*SendInitHandler)(SendInit *aSendInitMsg);
    typedef void (*ReceiveInitHandler)(ReceiveInit *aReceiveInitMsg);
    typedef void (*SendAcceptHandler)(void *aAppState, SendAccept *aSendAcceptMsg);
    typedef void (*ReceiveAcceptHandler)(ReceiveAccept *aReceiveAcceptMsg);
    typedef void (*RejectHandler)(void *aAppState, StatusReport *aReport);
    typedef void (*GetBlockHandler)(void *aAppState,
                                    uint64_t *pLength,
                                    uint8_t **aDataBlock,
                                    bool *isLastBlock);
    typedef void (*PutBlockHandler)(uint64_t aLength, uint8_t *aDataBlock, bool isLastBlock);
    typedef void (*XferErrorHandler)(void *aAppState, StatusReport *aXferError);
    typedef void (*XferDoneHandler)(void *aAppState);
    typedef void (*ErrorHandler)(void *aAppState, WEAVE_ERROR anErrorCode);
    // protocol entry points
    WEAVE_ERROR initBdxReceive(bool iCanDrive,
                               ReceiveAcceptHandler anAcceptHandler, RejectHandler aRejectHandler,
                               PutBlockHandler aBlockHandler, XferErrorHandler aXferErrorHandler,
                               XferDoneHandler aDoneHandler, ErrorHandler anErrorHandler);
    WEAVE_ERROR initBdxSend(bool iCanDrive, bool uCanDrive, bool aSynchOK,
                            SendAcceptHandler anAcceptHandler, RejectHandler aRejectHandler,
                            GetBlockHandler aBlockHandler, XferErrorHandler aXferErrorHandler,
                            XferDoneHandler aDoneHandler, ErrorHandler anErrorHandler,
                            ReferencedTLVData *aMetaData=NULL);
    WEAVE_ERROR awaitBdxInit(); //TBD
  private:
    // the main handler for messages arriving on the exchange
    static void handleResponse(ExchangeContext*, const IPPacketInfo *, const WeaveMessageInfo*, uint32_t, uint8_t, PacketBuffer*);
    // handler for unexpected connection close indications
    static void handleConnectionClosed(ExchangeContext *anExchangeCtx, WeaveConnection *con, WEAVE_ERROR conErr);
  public:
    void DispatchReceiveAccept(ReceiveAccept *aReceiveAcceptMsg);
    void DispatchSendAccept(SendAccept *aSendAcceptMsg);
    void DispatchRejectHandler(StatusReport *aReport);
    void DispatchPutBlockHandler(uint64_t length,
                                 uint8_t *dataBlock,
                                 bool isLastBlock);
    void DispatchGetBlockHandler(uint64_t *pLength,
                                 uint8_t **aDataBlock,
                                 bool *isLastBlock);
    void DispatchXferErrorHandler(StatusReport *aXferError);
    void DispatchXferDoneHandler();
    void DispatchErrorHandler(WEAVE_ERROR anErrorCode);

  public:
    // data members
    bool isInitiated;        // true iff the transfer has been initiated
    bool isAccepted;         // true iff the transfer has been initiated and accepted
    bool isDone;             // true iff the transfer is done
    bool amInitiator;        // true iff this entity was the initiator
    bool amSender;           // true if this entity is the sender, false if it is the receiver
    bool amDriver;           // true if this entity is the driver
    bool isAsynch;           // true if transfer is anynchronous
    bool isWideRange;        // true is widths and offsets are 64 bits
    ReferencedString theFileDesignator;
    uint16_t theMaxBlockSize;
    uint64_t theStartOffset;
    uint64_t theLength;
    uint8_t theBlockCounter;
    const WeaveFabricState *theFabricState;  // [READ ONLY] Fabric state object
    WeaveExchangeManager *theExchangeMgr;    // [READ ONLY] Exchange manager object
    WeaveConnection *theConnection;          // Connection to use to talk to the BDX server; NULL means use UDP.
    uint8_t theEncryptionType;               // Encryption type to use when sending an BDX Request
    uint16_t theKeyId;                       // Encryption key to use when sending an BDX Request
    ExchangeContext *theExchangeCtx;         // The exchange context for the most recently started BDX exchange.
    // application-supplied handlers
    SendInitHandler theSendInitHandler;
    ReceiveInitHandler theReceiveInitHandler;
    SendAcceptHandler theSendAcceptHandler;
    ReceiveAcceptHandler theReceiveAcceptHandler;
    RejectHandler theRejectHandler;
    GetBlockHandler theGetBlockHandler;
    PutBlockHandler thePutBlockHandler;
    XferErrorHandler theXferErrorHandler;
    XferDoneHandler theXferDoneHandler;
    ErrorHandler theErrorHandler;
    void *theAppState;
  };
} // namespace BulkDataTransfer
} // namespace Profiles
} // namespace Weave
} // namespace nl
#endif // _BULK_DATA_TRANSFER_PROFILE_H
