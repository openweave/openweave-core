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
 *      This file contains implementations for the definitions
 *      contained and described in BulkDataTransfer.h
 *
 */

#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveMessageLayer.h>
#include <Weave/Profiles/ProfileCommon.h>
#include <Weave/Support/CodeUtils.h>
#include "BulkDataTransfer.h"

using namespace ::nl::Weave;
using namespace ::nl::Weave::Profiles;
using namespace ::nl::Weave::Profiles::StatusReporting;
using namespace ::nl::Weave::Profiles::WeaveMakeManagedNamespaceIdentifier(BDX, kWeaveManagedNamespaceDesignation_Current);
/*
 * -- definitions for SendInit and its supporting classes --
 *
 * the no-arg constructor with defaults for the SendInit message.
 * note that the defaults here are set up for sleepy 802.15.4 devices.
 * in another context they should be changed on initialization.
 */
SendInit::SendInit()
{
  senderDriveSupported=true;
  receiverDriveSupported=false;
  asynchronousModeSupported=false;
  definiteLength=true;
  startOffsetPresent=false;
  wideRange=false;
  theMaxBlockSize=32;
  theStartOffset=0;
  theLength=0;
}
/*
 * initialize a "wide" send inititiation
 * parameters:
 * - bool aSenderDrive, true if it's OK for the sender to drive, false otherwise
 * - bool aReceiverDrive, true if it's OK for the receiver to drive, false otherwise
 * - bool asynchMode, true if the device supports asynchronous mode, false otherwsie
 * - uint16_t aMaxBlockSize, proposes a maximum block size for the upcomign transfer
 * - uint64_t aStartOffset, a start-offset in the file at which to start. if the given
 * value is 0 then there's no offset.
 * - uint64_t aLength, the length of the file to be transferred. if the given value is 0
 * the tranfer is of indefinite length.
 * - ReferencedString &aFileDesignator, a string that identifies the data to be transferred
 * - ReferencedTLVData *aMetaData, (optional) additional data in TLV format.
 * return: error/status
 */
WEAVE_ERROR SendInit::init(bool aSenderDrive, bool aReceiverDrive, bool asynchMode,
                           uint16_t aMaxBlockSize, uint64_t aStartOffset, uint64_t aLength,
                           ReferencedString &aFileDesignator, ReferencedTLVData *aMetaData=NULL)
{
  wideRange=true;
  senderDriveSupported=aSenderDrive;
  receiverDriveSupported=aReceiverDrive;
  asynchronousModeSupported=asynchMode;
  theMaxBlockSize=aMaxBlockSize;
  theStartOffset=aStartOffset;
  if (theStartOffset==0) startOffsetPresent=false;
  theLength=aLength;
  if (theLength==0) definiteLength=false;
  theFileDesignator=aFileDesignator;
  if (aMetaData!=NULL) theMetaData=*aMetaData;
  return WEAVE_NO_ERROR;
}
/*
 * initialize a "non-wide" send inititiation
 * parameters: (same as above except)
 * - uint32_t aStartOffset, a 32-bit start offset
 * - uint32_t aLength, a 32-bit length
 * return error/status
 */
WEAVE_ERROR SendInit::init(bool aSenderDrive, bool aReceiverDrive, bool asynchMode,
			   uint16_t aMaxBlockSize, uint32_t aStartOffset, uint32_t aLength,
			   ReferencedString &aFileDesignator, ReferencedTLVData *aMetaData=NULL)
{
  init(aSenderDrive, aReceiverDrive, asynchMode, aMaxBlockSize,
       (uint64_t)aStartOffset, (uint64_t)aLength, aFileDesignator, aMetaData);
  wideRange=false;
  return WEAVE_NO_ERROR;
}
/*
 * parameter: PacketBuffer *aBuffer, a packet into which the pack the request
 * return: /error/status
 */
WEAVE_ERROR SendInit::pack(PacketBuffer *aBuffer)
{
  MessageIterator i(aBuffer);
  i.append();
  // pack the transfer control byte
  uint8_t xferCtl=0;
  if (senderDriveSupported) xferCtl|=kMode_SenderDrive;
  if (receiverDriveSupported) xferCtl|=kMode_ReceiverDrive;
  if (asynchronousModeSupported) xferCtl|=kMode_Asynchronous;
  TRY(i.writeByte(xferCtl));
  // pack the range control byte
  uint8_t rangeCtl=0;
  if (definiteLength) rangeCtl|=kRangeCtl_DefiniteLength;
  if (startOffsetPresent) rangeCtl|=kRangeCtl_StartOffsetPresent;
  if (wideRange) rangeCtl|=kRangeCtl_WideRange;
  TRY(i.writeByte(rangeCtl));
  TRY(i.write16(theMaxBlockSize));
  if (startOffsetPresent) {
    if (wideRange) {
      TRY(i.write64(theStartOffset));
    }
    else {
      TRY(i.write32((uint32_t)theStartOffset));
    }
  }
  if (definiteLength) {
    if (wideRange) {
      TRY(i.write64(theLength));
    }
    else {
      TRY(i.write32((uint32_t)theLength));
    }
  }
  theFileDesignator.pack(i);
  theMetaData.pack(i);
  return WEAVE_NO_ERROR;
}
/*
 * the packed length, the packed length
 */
uint16_t SendInit::packedLength()
{
  // <xfer cctl>+<range ctl>+<max block>+<start offset (optional)>+<length (optional)>+<designator>+<metadata (optional)>
  uint16_t startOffsetLength=startOffsetPresent?(wideRange?8:4):0;
  uint16_t lengthLength=definiteLength?(wideRange?8:4):0;
  return 1+1+2+startOffsetLength+lengthLength+(2+theFileDesignator.theLength)+theMetaData.packedLength();
}
/*
 * parameters:
 * - PacketBuffer *aBuffer, a pointer to a packet from which to parse the query
 * - SendInit &aRequest, a pointer to an object in which to put the result
 * return: error/status
 */
WEAVE_ERROR SendInit::parse(PacketBuffer *aBuffer, SendInit &aRequest)
{
  MessageIterator i(aBuffer);
  // get the xfer ctl field and unpack it
  uint8_t xferCtl;
  uint32_t tmpUint32;
  TRY(i.readByte(&xferCtl));
  aRequest.senderDriveSupported=((xferCtl&kMode_SenderDrive)!=0);
  aRequest.receiverDriveSupported=((xferCtl&kMode_ReceiverDrive)!=0);
  aRequest.asynchronousModeSupported=((xferCtl&kMode_Asynchronous)!=0);
  // now the range ctl field and do the same
  uint8_t rangeCtl;
  TRY(i.readByte(&rangeCtl));
  aRequest.definiteLength=(rangeCtl&kRangeCtl_DefiniteLength)!=0;
  aRequest.startOffsetPresent=(rangeCtl&kRangeCtl_StartOffsetPresent)!=0;
  aRequest.wideRange=(rangeCtl&kRangeCtl_WideRange)!=0;
  // now everything else
  TRY(i.read16(&aRequest.theMaxBlockSize));
  if (aRequest.startOffsetPresent) {
    if (aRequest.wideRange) {
      TRY(i.read64(&aRequest.theStartOffset));
    }
    else {
      TRY(i.read32(&tmpUint32));
      aRequest.theStartOffset = tmpUint32;
    }
  }
  if (aRequest.definiteLength) {
    if (aRequest.wideRange) {
      TRY(i.read64(&aRequest.theLength));
    }
    else {
        TRY(i.read32(&tmpUint32));
        aRequest.theLength = tmpUint32;
    }
  }
  TRY(ReferencedString::parse(i, aRequest.theFileDesignator));
  ReferencedTLVData::parse(i, aRequest.theMetaData);
  return WEAVE_NO_ERROR;
}
/*
 * check everything to see if two of these are equal
 */
bool SendInit::operator == (const SendInit &another) const
{
  return(senderDriveSupported==another.senderDriveSupported &&
         receiverDriveSupported==another.receiverDriveSupported &&
         asynchronousModeSupported==another.asynchronousModeSupported &&
         definiteLength==another.definiteLength &&
         startOffsetPresent==another.startOffsetPresent &&
         asynchronousModeSupported==another.asynchronousModeSupported &&
         theMaxBlockSize==another.theMaxBlockSize &&
         theStartOffset==another.theStartOffset &&
         theFileDesignator==another.theFileDesignator &&
         theMetaData==another.theMetaData);
}
/*
 * -- definitions for SendAccept and its supporting classes --
 *
 * the no-arg constructor with defaults for the send accept message
 */
SendAccept::SendAccept()
{
  theTransferMode=kMode_SenderDrive;
  theMaxBlockSize=0;
}
/*
 * parameters:
 * - uint8_t aTransferMode, a transfer mode value. must be at MOST one of the
 * flag values for this field.
 * - uiunt16_t aMaxBlockSize, the maximum allowable block sixe for this exchange
 * - ReferencedTLVData *aMetaData, optional metadata
 * - return error/status
 */
WEAVE_ERROR SendAccept::init(uint8_t aTransferMode, uint16_t aMaxBlockSize, ReferencedTLVData *aMetaData=NULL)
{
  if (aTransferMode!=kMode_SenderDrive &&
     aTransferMode!=kMode_ReceiverDrive &&
     aTransferMode!=kMode_Asynchronous)
    return WEAVE_ERROR_INVALID_TRANSFER_MODE;
  theTransferMode=aTransferMode;
  theMaxBlockSize=aMaxBlockSize;
  if (aMetaData!=NULL) theMetaData=*aMetaData;
  return WEAVE_NO_ERROR;
}
/*
 * parameter: PacketBuffer *aBuffer, a buffer in which to pack the response
 * return: error/status
 */
WEAVE_ERROR SendAccept::pack(PacketBuffer *aBuffer)
{
  MessageIterator i(aBuffer);
  i.append();
  TRY(i.writeByte(theTransferMode));
  TRY(i.write16(theMaxBlockSize));
  theMetaData.pack(i);
  return WEAVE_NO_ERROR;
}
/*
 * return the length of this message if you bothered to stuff it in a packet
 */
uint16_t SendAccept::packedLength()
{
  // <transfer mode>+<max block size>+<meta data (optional)>
  return 1+2+theMetaData.packedLength();
}
/*
 * parameters:
 * - PacketBuffer *aBuffer, a buffer from which to parse the response
 * - SendAccept &aResponse, a place to put the result
 * return: error/status
 */
WEAVE_ERROR SendAccept::parse(PacketBuffer *aBuffer, SendAccept &aResponse)
{
  MessageIterator i(aBuffer);
  TRY(i.readByte(&aResponse.theTransferMode));
  TRY(i.read16(&aResponse.theMaxBlockSize));
  ReferencedTLVData::parse(i, aResponse.theMetaData);
  return WEAVE_NO_ERROR;
}
/*
 * are they equal or not... that is the big question
 */
bool SendAccept::operator== (const SendAccept &another) const
{
  return(theTransferMode==another.theTransferMode &&
         theMaxBlockSize==another.theMaxBlockSize &&
         theMetaData==another.theMetaData);
}
/*
 * -- definitions for ReceiveInit and its supporting classes --
 *
 * the no-arg constructor with defaults for the ReceiveInit message.
 * note that the defaults here are set up for sleepy 802.15.4 devices.
 * in another context they should be changed on initialization.
 */
ReceiveInit::ReceiveInit()
{
  senderDriveSupported=false;
  receiverDriveSupported=true;
  asynchronousModeSupported=false;
  definiteLength=true;
  startOffsetPresent=false;
  wideRange=false;
  theMaxBlockSize=32;
  theStartOffset=0;
  theLength=0;
}
/*
 * -- definitions for ReceiveAccept and its supporting classes --
 *
 * the no-arg constructor with defaults for the send accept message
 */
ReceiveAccept::ReceiveAccept()
{
  theTransferMode=kMode_ReceiverDrive;
  definiteLength=true;
  wideRange=false;
  theMaxBlockSize=0;
  theLength=0;
}
/*
 * initialze a "wide" recevie accept frame
 * parameters:
 * - uint8_t aTransferMode, a transfer mode value. must be at MOST one of the
 * flag values for this field.
 * - uiunt16_t aMaxBlockSize, the maximum allowable block sixe for this exchange
 * - uint64_t aLength, the length of the file to be transferred (if known)
 * - ReferencedTLVData *aMetaData, optional metadata
 * - return error/status
 */
WEAVE_ERROR ReceiveAccept::init(uint8_t aTransferMode, uint16_t aMaxBlockSize, uint64_t aLength, ReferencedTLVData *aMetaData=NULL)
{
  definiteLength=(aLength!=0);
  wideRange=true;
  theLength=aLength;
  return SendAccept::init(aTransferMode, aMaxBlockSize, aMetaData);
}
/*
 * initialze a "narrow" recevie accept frame
 * parameters:
 * - uint8_t aTransferMode, a transfer mode value. must be at MOST one of the
 * flag values for this field.
 * - uiunt16_t aMaxBlockSize, the maximum allowable block sixe for this exchange
 * - uint
 * - uint32_t aLength, the length of the file to be transferred (if known)
 * - ReferencedTLVData *aMetaData, optional metadata
 * - return error/status
 */
WEAVE_ERROR ReceiveAccept::init(uint8_t aTransferMode, uint16_t aMaxBlockSize, uint32_t aLength, ReferencedTLVData *aMetaData=NULL)
{
  definiteLength=(aLength!=0);
  wideRange=false;
  theLength=(uint64_t)aLength;
  return SendAccept::init(aTransferMode, aMaxBlockSize, aMetaData);
}
/*
 * parameter: PacketBuffer *aBuffer, a buffer in which to pack the response
 * return: error/status
 */
WEAVE_ERROR ReceiveAccept::pack(PacketBuffer *aBuffer)
{
  MessageIterator i(aBuffer);
  i.append();
  TRY(i.writeByte(theTransferMode));
  // format and pack the range control field
  uint8_t rangeCtl=0;
  if (definiteLength) rangeCtl|=kRangeCtl_DefiniteLength;
  if (wideRange) rangeCtl|=kRangeCtl_WideRange;
  TRY(i.writeByte(rangeCtl));
  TRY(i.write16(theMaxBlockSize));
  // and the length, if any
  if (definiteLength) {
    if (wideRange) {
      TRY(i.write64(theLength));
    }
    else {
      TRY(i.write32((uint32_t)theLength));
    }
  }
  theMetaData.pack(i);
  return WEAVE_NO_ERROR;
}
/*
 * return the length of this message if you bothered to stuff it in a packet
 */
uint16_t ReceiveAccept::packedLength()
{
  // <transfer mode>+<range control>+<max block size>+<length (optional)>+<meta data (optional)>
  return 1+1+2+(definiteLength?(wideRange?8:4):0)+theMetaData.packedLength();
}
/*
 * parameters:
 * - PacketBuffer *aBuffer, a buffer from which to parse the response
 * - ReceiveAccept &aResponse, a place to put the result
 * return: error/status
 */
WEAVE_ERROR ReceiveAccept::parse(PacketBuffer *aBuffer, ReceiveAccept &aResponse)
{
  MessageIterator i(aBuffer);
  uint32_t tmpUint32Value;
  TRY(i.readByte(&aResponse.theTransferMode));
  // unpack the range control byte
  uint8_t rangeCtl;
  TRY(i.readByte(&rangeCtl));
  aResponse.definiteLength=((rangeCtl&kRangeCtl_DefiniteLength)!=0);
  aResponse.wideRange=((rangeCtl&kRangeCtl_WideRange)!=0);
  TRY(i.read16(&aResponse.theMaxBlockSize));
  if (aResponse.definiteLength) {
    if (aResponse.wideRange) {
      TRY(i.read64(&aResponse.theLength));
    }
    else {
      TRY(i.read32(&tmpUint32Value));
      aResponse.theLength = tmpUint32Value;
    }
  }
  ReferencedTLVData::parse(i, aResponse.theMetaData);
  return WEAVE_NO_ERROR;
}
/*
 * are they equal or not... that is the big question
 */
bool ReceiveAccept::operator== (const ReceiveAccept &another) const
{
  return(theTransferMode==another.theTransferMode &&
         definiteLength==another.definiteLength &&
         wideRange==another.wideRange &&
         theMaxBlockSize==another.theMaxBlockSize &&
         theLength==another.theLength &&
         theMetaData==another.theMetaData);
}
/*
 * -- definitions for BlockQuery and its supporting classes --
 *
 * the no-arg constructor with defaults for the block query message
 */
BlockQuery::BlockQuery()
{
  theBlockCounter=0;
}
/*
 * parameter: uint8_t aCounter, a block counter value
 * return: error/status
 */
WEAVE_ERROR BlockQuery::init(uint8_t aCounter)
{
  theBlockCounter=aCounter;
  return WEAVE_NO_ERROR;
}
/*
 * parameter: PacketBuffer *aBuffer, a buffer in which to pack the query
 * return: error/status
 */
WEAVE_ERROR BlockQuery::pack(PacketBuffer *aBuffer)
{
  MessageIterator i(aBuffer);
  i.append();
  TRY(i.writeByte(theBlockCounter));
  return WEAVE_NO_ERROR;
}
/*
 * return the length of this message if you bothered to stuff it in a packet
 */
uint16_t BlockQuery::packedLength()
{
  // just the counter
  return 1;
}
/*
 * parameters:
 * - PacketBuffer *aBuffer, a buffer from which to parse the query
 * - ReceiveAccept &aQuery, a place to put the result
 * return: error/status
 */
WEAVE_ERROR BlockQuery::parse(PacketBuffer *aBuffer, BlockQuery &aQuery)
{
  MessageIterator i(aBuffer);
  TRY(i.readByte(&aQuery.theBlockCounter));
  return WEAVE_NO_ERROR;
}
/*
 * liberte, equality, fraternite
 */
bool BlockQuery::operator== (const BlockQuery &another) const
{
  return theBlockCounter==another.theBlockCounter;
}
/*
 * -- definitions for BlockSend and its supporting classes --
 *
 * the no-arg constructor with defaults for the block send message
 */
BlockSend::BlockSend(void) :
    RetainedPacketBuffer()
{
  theBlockCounter=0;
  theLength=0;
  theData=NULL;
}
/*
 * parameters:
 * - uint8_t aCounter, a block counter value for this block
 * - uint64_t aLength, the length of the block
 * - uint8_t *aData, a pointer to the data, this may be in a message
 * buffer or elsewhere depending on whether it's a recevied or a sent message.
 * return: error/status
 */
WEAVE_ERROR BlockSend::init(uint8_t aCounter, uint64_t aLength, uint8_t *aData)
{
  theBlockCounter=aCounter;
  theLength=aLength;
  theData=aData;
  return WEAVE_NO_ERROR;
}
/*
 * parameter: PacketBuffer *aBuffer, a buffer in which to pack the response
 * return: error/status
 */
WEAVE_ERROR BlockSend::pack(PacketBuffer *aBuffer)
{
  MessageIterator i(aBuffer);
  i.append();
  TRY(i.writeByte(theBlockCounter));
  /*
   * note that we have a problem here. the 64-bit length field here implies
   * that we may want to have truly whopping block lengths but we have been,
   * up 'til now, assuming that it all fits in a single message buffer. for
   * the time being i'll just cast it but eventually we'll have to figure
   * this out.
   */
  uint8_t *p=theData;
  for (uint64_t j=0; j<theLength; j++) TRY(i.writeByte(*p++));
  return WEAVE_NO_ERROR;
}
/*
 * the length OTM
 */
uint16_t BlockSend::packedLength()
{
  // <block counter>+<length>
  return 1+theLength;
}
/*
 * parameters:
 * - PacketBuffer *aBuffer, a buffer from which to parse the response
 * - ReceiveAccept &aResponse, a place to put the result
 * return: error/status
 */
WEAVE_ERROR BlockSend::parse(PacketBuffer *aBuffer, BlockSend &aResponse)
{
  MessageIterator i(aBuffer);
  TRY(i.readByte(&aResponse.theBlockCounter));
  aResponse.theLength=(uint64_t)aBuffer->DataLength();
  aResponse.theData=aBuffer->Start();
  /*
   * we're holding onto this buffer and, while we might not want to
   * write anything after the data in it we want to:
   * - move the message iterator's insertion point past the data we've
   * just "parsed".
   * - set the private buffer pointer data member to point to it
   * - increment the reference count on the inet buffer.
   */
  i.thePoint+=aResponse.theLength;
  aResponse.Retain(aBuffer);
  return WEAVE_NO_ERROR;
}
/*
 * testing, testing
 */
bool BlockSend::operator== (const BlockSend &another) const
{
  return(theBlockCounter==another.theBlockCounter &&
         theLength==another.theLength &&
         theData==another.theData);
}
/*
 * here are the method definitions for the WeaveBdxClient class
 *
 * NOTE!!!! for now we're just defining enough stuff here to make
 * sensor data and log upload work.
 *
 * the no-arg constructor with defaulting for BDX clients
 */
WeaveBdxClient::WeaveBdxClient()
{
  isInitiated=false;
  isAccepted=false;
  isDone=false;
  amInitiator=false;
  amSender=false;
  amDriver=true;
  isAsynch=false;
  isWideRange=false;
  theMaxBlockSize=0;
  theStartOffset=0;
  theLength=0;
  theBlockCounter=0;
  theFabricState = NULL;
  theExchangeMgr = NULL;
  theConnection = NULL;
  theEncryptionType = kWeaveEncryptionType_None;
  theKeyId = WeaveKeyId::kNone;
  theSendInitHandler=NULL;
  theReceiveInitHandler=NULL;
  theSendAcceptHandler=NULL;
  theReceiveAcceptHandler=NULL;
  theRejectHandler=NULL;
  theGetBlockHandler=NULL;
  thePutBlockHandler=NULL;
  theXferErrorHandler=NULL;
  theXferDoneHandler=NULL;
  theErrorHandler=NULL;
  theExchangeCtx = NULL;
  theAppState = NULL;
}
/*
 * parameters:
 * - WeaveExchangeManager &anExchangeMgr, an exhcnage manager to use
 * for this bulk transfer operation.
 * - ReferencedString &aFileDesignator,
 * - uint16_t aMaxBlockSize,
 * - uint64_t aStartOffset,
 * - uint64_t aLength,
 * - bool aWideRange,
 return: error/status
 */
WEAVE_ERROR WeaveBdxClient::initClient(WeaveExchangeManager *anExchangeMgr,
                                       void *aAppState,
                                       ReferencedString &aFileDesignator,
                                       uint16_t aMaxBlockSize,
                                       uint64_t aStartOffset,
                                       uint64_t aLength,
                                       bool aWideRange)
{
  // Error if already initialized.
  if (theExchangeMgr!=NULL) return WEAVE_ERROR_INCORRECT_STATE;
  theExchangeMgr=anExchangeMgr;
  theFabricState=theExchangeMgr->FabricState;
  theConnection=NULL;
  theEncryptionType=kWeaveEncryptionType_None;
  theKeyId=WeaveKeyId::kNone;
  theExchangeCtx=NULL;
  // set up the application-specific state
  theFileDesignator=aFileDesignator;
  theMaxBlockSize=aMaxBlockSize;
  theStartOffset=aStartOffset;
  theLength=aLength;
  theBlockCounter=0;
  isWideRange=aWideRange;
  theAppState = aAppState;
  return WEAVE_NO_ERROR;
}

/*
 * return: error/status
 */
WEAVE_ERROR WeaveBdxClient::shutdownClient()
{
    return shutdownClient(WEAVE_NO_ERROR);
}

/*
 * return: error/status
 */
WEAVE_ERROR WeaveBdxClient::shutdownClient(WEAVE_ERROR aErr)
{
  if (theExchangeCtx!=NULL) {
    theExchangeCtx->Close();
    theExchangeCtx=NULL;
  }
  if (theConnection!=NULL) {
    if (WEAVE_NO_ERROR == aErr)
    {
      theConnection->Close();
    }
    else
    {
      theConnection->Abort();
    }
    theConnection=NULL;
  }
  theExchangeMgr=NULL;
  theFabricState=NULL;
  // clear application-specific state
  isInitiated=false;
  isAccepted=false;
  amInitiator=false;
  amSender=false;
  amDriver=false;
  isAsynch=false;
  isWideRange=false;
  theFileDesignator.Release();
  theMaxBlockSize=0;
  theStartOffset=0;
  theLength=0;
  theBlockCounter=0;
  return WEAVE_NO_ERROR;
}
/*
 * parameters:
 * - bool iCanDrive, true if the initiator should propose that it drive,
 * false otherwise
 * - ReceiveAcceptHandler anAcceptHandler, a handler for the case where the transfer
 * is accepted
 * - RejectHandler aRejectHandler, a hander for the case where the transfer is
 * rejected
 * - PutBlockHandler aBlockHandler, a handler that is supposed to save the block
 *   that was received
 * - XferErrorHandler anErrorHandler, a handler for fatal errors that occur in the
 * middle of the transfer
 * - XferDoneHandler aDoneHandler, a handler for when we're done transferring
 * - ErrorHandler anErrorHandler, a handler for fatal errors that occur in the
 * middle of the transfer
 * return: error/status
 */
WEAVE_ERROR WeaveBdxClient::initBdxReceive(bool iCanDrive,
                                           ReceiveAcceptHandler anAcceptHandler, RejectHandler aRejectHandler,
                                           PutBlockHandler aBlockHandler, XferErrorHandler aXferErrorHandler,
                                           XferDoneHandler aDoneHandler, ErrorHandler anErrorHandler)
{
  // Discard any existing exchange context. Effectively we can only have one transfer with
  // a single node at any one time.
  if (theExchangeCtx!=NULL) {
    theExchangeCtx->Close();
    theExchangeCtx = NULL;
  }
  // Create a new exchange context.
  if (theConnection!=NULL)
    theExchangeCtx=theExchangeMgr->NewContext(theConnection, this);
  else
    return WEAVE_ERROR_INCORRECT_STATE;
  // get out if this didn't work
  if (theExchangeCtx==NULL) return WEAVE_ERROR_NO_MEMORY; // TODO: better error code
  // Configure the encryption and signature types to be used to send the request.
  theExchangeCtx->EncryptionType=theEncryptionType;
  theExchangeCtx->KeyId=theKeyId;
  // set up the application-specific state
  isInitiated=true;
  isAccepted=false;
  amInitiator=true;
  amSender=false;
  // Arrange for messages in this exchange to go to our response handler.
  theExchangeCtx->OnMessageReceived=handleResponse;
  // Handle exchange failures due to the connection being closed.
  theExchangeCtx->OnConnectionClosed=handleConnectionClosed;
  // install application-specifc handlers
  theReceiveAcceptHandler=anAcceptHandler;
  theRejectHandler=aRejectHandler;
  thePutBlockHandler=aBlockHandler;
  theXferErrorHandler=aXferErrorHandler;
  theErrorHandler=anErrorHandler;
  theXferDoneHandler=aDoneHandler;
  WEAVE_ERROR err;
  ReceiveInit msg;
  // Send an BDX Request message. Discard the exchange context if the send fails.
  PacketBuffer *buffer=PacketBuffer::New();
  if (buffer==NULL) {
    err=WEAVE_ERROR_NO_MEMORY;
    goto cleanup;
  }
  if (isWideRange) {
    RESCUE(err, msg.init(!iCanDrive/*SenderDrive*/, iCanDrive/*ReceiverDrive*/, false/*aSynchOK*/, theMaxBlockSize, theStartOffset, theLength, theFileDesignator), cleanup);
  }
  else {
    RESCUE(err, msg.init(!iCanDrive/*SenderDrive*/, iCanDrive/*ReceiverDrive*/, false/*aSynchOK*/, theMaxBlockSize, (uint32_t)theStartOffset, (uint32_t)theLength, theFileDesignator), cleanup);
  }
  RESCUE(err, msg.pack(buffer), cleanup);

  err=theExchangeCtx->SendMessage(kWeaveProfile_BDX, kMsgType_ReceiveInit, buffer);
  buffer = NULL;
  if (err==WEAVE_NO_ERROR) return err;
cleanup:
  DispatchErrorHandler(err);
  if (buffer!=NULL) PacketBuffer::Free(buffer);
  shutdownClient(err);
  return err;
}

/*
 * parameters:
 * - bool iCanDrive, true if the initiator should propose that it drive,
 * false otherwise
 * - bool uCanDrive, true if the initiator should propose that its counterpart drive,
 * false otherwise
 * - bool aSynchOK, true if the initiator can do asynchronous mode, false otherwise
 * - SendAcceptHandler anAcceptHandler, a handler for the case where the transfer
 * is accepted
 * - RejectHandler aRejectHandler, a hander for the case where the transfer is
 * rejected
 * - GetBlockHandler aBlockHandler, a handler that is supposed to get "the next"
 * block to be transfered
 * - XferErrorHandler anErrorHandler, a handler for fatal errors that occur in the
 * middle of the transfer
 * - XferDoneHandler aDoneHandler, a handler for when we're done transferring
 * - ReferencedTLVData *aMetaData=NULL, (optional) metadata
 * return: error/status
 */
WEAVE_ERROR WeaveBdxClient::initBdxSend(bool iCanDrive, bool uCanDrive, bool aSynchOK,
                                        SendAcceptHandler anAcceptHandler, RejectHandler aRejectHandler,
                                        GetBlockHandler aBlockHandler, XferErrorHandler aXferErrorHandler,
                                        XferDoneHandler aDoneHandler, ErrorHandler anErrorHandler,
                                        ReferencedTLVData *aMetaData)
{
// Discard any existing exchange context. Effectively we can only have one transfer with
// a single node at any one time.
  if (theExchangeCtx!=NULL) {
    theExchangeCtx->Close();
    theExchangeCtx = NULL;
  }
  // Create a new exchange context.
  if (theConnection!=NULL)
    theExchangeCtx=theExchangeMgr->NewContext(theConnection, this);
  else
    return WEAVE_ERROR_INCORRECT_STATE;
  // get out if this didn't work
  if (theExchangeCtx==NULL) return WEAVE_ERROR_NO_MEMORY; // TODO: better error code
  // Configure the encryption and signature types to be used to send the request.
  theExchangeCtx->EncryptionType=theEncryptionType;
  theExchangeCtx->KeyId=theKeyId;
  // set up the application-specific state
  isInitiated=true;
  isAccepted=false;
  amInitiator=true;
  amSender=true;
  // Arrange for messages in this exchange to go to our response handler.
  theExchangeCtx->OnMessageReceived=handleResponse;
  // Handle exchange failures due to the connection being closed.
  theExchangeCtx->OnConnectionClosed=handleConnectionClosed;
  // install application-specifc handlers
  theSendAcceptHandler=anAcceptHandler;
  theRejectHandler=aRejectHandler;
  theGetBlockHandler=aBlockHandler;
  theXferErrorHandler=aXferErrorHandler;
  theErrorHandler=anErrorHandler;
  theXferDoneHandler=aDoneHandler;
  WEAVE_ERROR err;
  SendInit msg;
  // Send an BDX Request message. Discard the exchange context if the send fails.
  PacketBuffer *buffer=PacketBuffer::New();
  if (buffer==NULL) {
    err=WEAVE_ERROR_NO_MEMORY;
    goto cleanup;
  }
  if (isWideRange) {
    RESCUE(err, msg.init(iCanDrive, uCanDrive, aSynchOK, theMaxBlockSize, theStartOffset, theLength, theFileDesignator, aMetaData), cleanup);
  }
  else {
    RESCUE(err, msg.init(iCanDrive, uCanDrive, aSynchOK, theMaxBlockSize, (uint32_t)theStartOffset, (uint32_t)theLength, theFileDesignator, aMetaData), cleanup);
  }
  RESCUE(err, msg.pack(buffer), cleanup);

  err=theExchangeCtx->SendMessage(kWeaveProfile_BDX, kMsgType_SendInit, buffer);
  buffer = NULL;
  if (err==WEAVE_NO_ERROR) return err;
cleanup:
  DispatchErrorHandler(err);
  if (buffer!=NULL) PacketBuffer::Free(buffer);
  shutdownClient(err);
  return err;
}
/*
 *
 WEAVE_ERROR WeaveBdxClient::awaitBdxInit(uint64_t) {} //TBD
*/
/*
 *
WEAVE_ERROR WeaveBdxClient::awaitBdxInit(uint64_t, IPAddress) {} //TBD
*/
/*
 * here's a utility function for sending a block query
 * parameter: WeaveBdxClient bdxApp,
 * return: error/status
 */
WEAVE_ERROR sendBlockQuery(WeaveBdxClient *bdxApp)
{
  PacketBuffer *outBuffer=PacketBuffer::New();
  if (outBuffer==NULL) return WEAVE_ERROR_NO_MEMORY;
  BlockQuery outMsg;
  TRY(outMsg.init(bdxApp->theBlockCounter));
  TRY(outMsg.pack(outBuffer));
  TRY(bdxApp->theExchangeCtx->SendMessage(kWeaveProfile_BDX, kMsgType_BlockQuery, outBuffer));
  outBuffer = NULL;
  return WEAVE_NO_ERROR;
}
/*
 * here's a utility function for sending a block EOF ack
 * parameter: WeaveBdxClient bdxApp,
 * return: error/status
 */
WEAVE_ERROR sendBlockEOFAck(WeaveBdxClient *bdxApp)
{
  PacketBuffer *outBuffer=PacketBuffer::New();
  if (outBuffer==NULL) return WEAVE_ERROR_NO_MEMORY;
  BlockEOFAck outMsg;
  TRY(outMsg.init(bdxApp->theBlockCounter));
  TRY(outMsg.pack(outBuffer));
  TRY(bdxApp->theExchangeCtx->SendMessage(kWeaveProfile_BDX, kMsgType_BlockEOFAck, outBuffer));
  outBuffer = NULL;
  return WEAVE_NO_ERROR;
}
/*
 * here's a utility function for sending a block
 * parameter: WeaveBdxClient bdxApp,
 * return: error/status
 */
WEAVE_ERROR sendBlock(WeaveBdxClient *bdxApp)
{
  uint64_t length;
  uint8_t *data = NULL;
  bool isLast;
  uint8_t msgType;
  PacketBuffer *outBuffer = NULL;
  WEAVE_ERROR err = WEAVE_NO_ERROR;

  // This check ensures DispatchGetBlockHandler has a registered handler
  // to dispatch to, and hence data, length, and isLast are properly initialized.
  // Note that return value from the callback theGetBlockHandler would make this
  // code more robust, but it would require changes to the public API.
  VerifyOrExit(bdxApp->theGetBlockHandler != NULL, err = WEAVE_ERROR_INCORRECT_STATE);

  bdxApp->DispatchGetBlockHandler(&length, &data, &isLast);
  VerifyOrExit(data != NULL, err = WEAVE_ERROR_INCORRECT_STATE);

  outBuffer = PacketBuffer::New();
  VerifyOrExit(outBuffer != NULL, err = WEAVE_ERROR_NO_MEMORY);

  if (isLast) {
    bdxApp->isDone=true;
    BlockEOF outMsg;
    msgType=kMsgType_BlockEOF;
    TRY(outMsg.init(bdxApp->theBlockCounter, length, data));
    TRY(outMsg.pack(outBuffer));
  }
  else {
    BlockSend outMsg;
    msgType=kMsgType_BlockSend;
    TRY(outMsg.init(bdxApp->theBlockCounter, length, data));
    TRY(outMsg.pack(outBuffer));
  }
  TRY(bdxApp->theExchangeCtx->SendMessage(kWeaveProfile_BDX, msgType, outBuffer));

exit:
  return err;
}
/*
 * this is the main handle for BDX exchanges.
 * parameters:
 * - ExchangeContext *anExchangeCtx, the exhcnage context in case we need it
 * - WeaveMessageInfo *aWeaveMsgInfo, the message information for the message
 * - uint32_t aProfileId, the ID of the profile underwhihc the message is defined
 * - uint8_t aMessageType, the message type with that profile
 * - PacketBuffer *aPacketBuffer, the packed message itself
 */
void WeaveBdxClient::handleResponse(ExchangeContext *anExchangeCtx, const IPPacketInfo *pktInfo, const WeaveMessageInfo *aWeaveMsgInfo,
                                    uint32_t aProfileId, uint8_t aMessageType, PacketBuffer *aPacketBuffer)
{
  WEAVE_ERROR err=WEAVE_NO_ERROR;	// we'll use this later
  // extract the BDX client object from the exchange context
  WeaveBdxClient *bdxApp = (WeaveBdxClient *)anExchangeCtx->AppState;
  // Verify that the message is a BDX message and get out if it's not
  if (aProfileId!=kWeaveProfile_BDX)
    err=WEAVE_ERROR_INVALID_PROFILE_ID;
  else {
    if (bdxApp->isInitiated) {
      /*
       * here's the case where the transfer has been initiated.
       * on the initiator, this will always be true when we get
       * here. on a listener it won't. the two possibilites here
       * are that the transfer has also been accepted or it hasn't.
       */
      if (bdxApp->isAccepted) {
        /*
         * at this point the transfer has been accepted and we're
         * just moving the bits back and forth. what happens here
         * depends on whether we're sending or receving, driving
         * or not.
         */
        if (bdxApp->amSender) {
          /*
           * the transfer has been started and i'm the sender. this
           * means, basically, that i should expect an ACK or a
           * query depending on who's driving.
           */
          if (bdxApp->amDriver) {
            /*
             * i'm sending and driving. this means that it should expect
             * an ACK from my counterpart and should send the next block.
             * a transfer error is possible here as well.
             */
            switch (aMessageType) {
            case kMsgType_BlockAck:
              {
                BlockAck ack;
                BlockAck::parse(aPacketBuffer, ack);
                uint8_t rcvdCounter=ack.theBlockCounter;
                if (rcvdCounter==bdxApp->theBlockCounter) {
                  // everything's OK update the counter and send the next block
                  bdxApp->theBlockCounter++;
                  err=sendBlock(bdxApp);
                }
                else if (rcvdCounter<bdxApp->theBlockCounter) {
                  // just ignore the packet
                }
                else { // bad scene we've somehow fallen behind
                }
              }
              break;
            case kMsgType_BlockEOFAck:
              bdxApp->DispatchXferDoneHandler();
              break;
            case kMsgType_TransferError:
              {
                TransferError inMsg;
                TransferError::parse(aPacketBuffer, inMsg);
                bdxApp->DispatchXferErrorHandler(&inMsg);
              }
              break;
            default:
              bdxApp->DispatchErrorHandler(WEAVE_ERROR_INVALID_MESSAGE_TYPE);
            }
          }
          else { // not driving
            /*
             * i'm sending and my counterpart is driving. this means
             * i wait for a query and send the reqwuested block. there
             * is an optional ACK here and a transfer error is also a
             * possibiliity.
             */
            switch (aMessageType) {
            case kMsgType_BlockQuery:
              break;
            case kMsgType_BlockAck:
              break;
            case kMsgType_BlockEOFAck:
              break;
            case kMsgType_TransferError:
              break;
            default:
              break;
            }
          }
        }
        else { // receiving (not sending)
          /*
           * otherwise, i'm the receiver. i should expect to get
           * a block here and then, if i'm driving, send out a
           * query for the next block otherwise just send an ACK.
           */
          if (bdxApp->amDriver) {
            //send out a query for the next block
            switch (aMessageType) {
            case kMsgType_BlockSend:
              {
                  BlockSend blockSend;
                  BlockSend::parse(aPacketBuffer, blockSend);
                  bdxApp->DispatchPutBlockHandler(blockSend.theLength, blockSend.theData, false/*bool isLastBlock*/);
                  //update the block counter and send the next block query
                  bdxApp->theBlockCounter++;
                  err=sendBlockQuery(bdxApp);
              }
              break;
            case kMsgType_BlockEOF:
              {//TODO: add a consistency check on the overall size of the file received
               //i.e. the sum of all received blocks should match the expected file size
                BlockEOF blockEOF;
                BlockEOF::parse(aPacketBuffer, blockEOF);
                bdxApp->DispatchPutBlockHandler(blockEOF.theLength, blockEOF.theData, true/*bool isLastBlock*/);
                //maintain the same block counter used to send the previous block query
                //and send the final block EOF ack
                err=sendBlockEOFAck(bdxApp);
                bdxApp->DispatchXferDoneHandler();
              }
              break;
            case kMsgType_TransferError:
              {
                TransferError inMsg;
                TransferError::parse(aPacketBuffer, inMsg);
                bdxApp->DispatchXferErrorHandler(&inMsg);
              }
              break;
            default:
              bdxApp->DispatchErrorHandler(WEAVE_ERROR_INVALID_MESSAGE_TYPE);
            }
          }
          else { // not driving
            // TODO
          }
        }
      }
      else { // not accepted
        /*
         * here the tranfer hasn't been accepted yet, and we're waiting
         * either for an accept or a reject message.
         */
        switch (aMessageType) {
        case kMsgType_TransferError:
        {
            TransferError inMsg;
            TransferError::parse(aPacketBuffer, inMsg);
            bdxApp->DispatchXferErrorHandler(&inMsg);
            break;
        }
        case kMsgType_SendAccept:
          /*
           * the send is accepted. now we figure out who's driving and,
           * if necessary, kick it off.
           */
          {
            SendAccept inMsg;
            err=SendAccept::parse(aPacketBuffer, inMsg);
            if (err==WEAVE_NO_ERROR) {
              bdxApp->DispatchSendAccept(&inMsg);
              switch (inMsg.theTransferMode) {
              case kMode_SenderDrive:
                bdxApp->amDriver=true;
                bdxApp->isAccepted=true;
                bdxApp->theMaxBlockSize=inMsg.theMaxBlockSize;
                // try and send the first block
                err=sendBlock(bdxApp);
                break;
              case kMode_ReceiverDrive:
                // TODO
                break;
              case kMode_Asynchronous:
                // we don't currently support asynch mode
              default:
                err=WEAVE_ERROR_INVALID_TRANSFER_MODE;
              }
            }
          }
          break;
        case kMsgType_SendReject:
          /*
           * the send is rejected, callback the reject handler.
           */
          bdxApp->DispatchRejectHandler(NULL);
          break;
        case kMsgType_ReceiveAccept:
          /*
           * the receive is accepted. now we figure out who's driving and,
           * if necessary, kick it off.
           */
          {
            ReceiveAccept inMsg;
            err=ReceiveAccept::parse(aPacketBuffer, inMsg);
            if (err==WEAVE_NO_ERROR) {
              bdxApp->DispatchReceiveAccept(&inMsg);
              switch (inMsg.theTransferMode) {
              case kMode_ReceiverDrive:
                bdxApp->amDriver=true;
                bdxApp->isAccepted=true;
                bdxApp->theMaxBlockSize=inMsg.theMaxBlockSize;
                // try and send the first block
                bdxApp->theBlockCounter = 0;
                err=sendBlockQuery(bdxApp);
                break;
              case kMode_SenderDrive:
                // TODO
                break;
              case kMode_Asynchronous:
                // we don't currently support asynch mode
              default:
                err=WEAVE_ERROR_INVALID_TRANSFER_MODE;
              }
            }
          }
          break;
        case kMsgType_ReceiveReject:
          /*
           * the receive is rejected, callback the reject handler.
           */
          bdxApp->DispatchRejectHandler(NULL);
          break;
        default:
          err = WEAVE_ERROR_INVALID_MESSAGE_TYPE;
          break;
        }
      }
    }
    else { // not initiated
      /*
       * OK. so here we were listening and we get something,
       * which must be an initiation or else.
       */
      switch (aMessageType) {
      case kMsgType_SendInit:
        // TODO
        break;
      case kMsgType_ReceiveInit:
        // TODO
        break;
      default:
        // TODO
        break;
      }
    }
  }
  if (err!=WEAVE_NO_ERROR) {
    bdxApp->DispatchErrorHandler(err);
    bdxApp->shutdownClient(err);
  }
  PacketBuffer::Free(aPacketBuffer);
}

void WeaveBdxClient::handleConnectionClosed(ExchangeContext *anExchangeCtx, WeaveConnection *con, WEAVE_ERROR conErr)
{
  WeaveBdxClient *bdxClient = (WeaveBdxClient *)anExchangeCtx->AppState;

  // If the other end closed the connection, pass WEAVE_ERROR_CONNECTION_CLOSED_UNEXPECTEDLY to the application.
  if (conErr == WEAVE_NO_ERROR) conErr = WEAVE_ERROR_CONNECTION_CLOSED_UNEXPECTEDLY;

  // Clear the client state.  Note this also closes the exchange context, meaning that anExchangeCtx is no longer valid.
  bdxClient->shutdownClient(conErr);

  // Forward the error to the app's error handler.
  if (bdxClient->theErrorHandler)
  {
    bdxClient->theErrorHandler(bdxClient->theAppState, conErr);
  }
}

void WeaveBdxClient::DispatchReceiveAccept(ReceiveAccept *aReceiveAcceptMsg)
{
  if (theReceiveAcceptHandler)
  {
    theReceiveAcceptHandler(aReceiveAcceptMsg);
  }
}

void WeaveBdxClient::DispatchSendAccept(SendAccept *aSendAcceptMsg)
{
  if (theSendAcceptHandler)
  {
    theSendAcceptHandler(theAppState, aSendAcceptMsg);
  }
}

void WeaveBdxClient::DispatchRejectHandler(StatusReport *aReport)
{
  if (theRejectHandler)
  {
      theRejectHandler(theAppState, aReport);
  }
}

void WeaveBdxClient::DispatchPutBlockHandler(uint64_t length,
                                             uint8_t *dataBlock,
                                             bool isLastBlock)
{
  if (thePutBlockHandler)
  {
      thePutBlockHandler(length, dataBlock, isLastBlock);
  }
}

void WeaveBdxClient::DispatchGetBlockHandler(uint64_t *pLength,
                                             uint8_t **aDataBlock,
                                             bool *isLastBlock)
{
  if (theGetBlockHandler)
  {
    theGetBlockHandler(theAppState, pLength, aDataBlock, isLastBlock);
  }
}

void WeaveBdxClient::DispatchXferErrorHandler(StatusReport *aXferError)
{
  if (theXferErrorHandler)
  {
    theXferErrorHandler(theAppState, aXferError);
  }
}

void WeaveBdxClient::DispatchXferDoneHandler()
{
  if (theXferDoneHandler)
  {
    theXferDoneHandler(theAppState);
  }
}

void WeaveBdxClient::DispatchErrorHandler(WEAVE_ERROR anErrorCode)
{
  if (theErrorHandler)
  {
    theErrorHandler(theAppState, anErrorCode);
  }
}
