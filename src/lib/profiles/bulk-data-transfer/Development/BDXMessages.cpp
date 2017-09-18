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

#include <Weave/Profiles/bulk-data-transfer/Development/BDXMessages.h>
#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveMessageLayer.h>
#include <Weave/Profiles/ProfileCommon.h>
#include <Weave/Support/CodeUtils.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(BDX, kWeaveManagedNamespaceDesignation_Development) {

using namespace ::nl::Weave::Profiles::StatusReporting;

#define VERSION_MASK 0x0F

/*
 * -- definitions for SendInit and its supporting classes --
 *
 * the no-arg constructor with defaults for the SendInit message.
 * note that the defaults here are set up for sleepy 802.15.4 devices.
 * in another context they should be changed on initialization.
 */
SendInit::SendInit()
    : mVersion(0)
    , mSenderDriveSupported(true)
    , mReceiverDriveSupported(false)
    , mAsynchronousModeSupported(false)
    , mDefiniteLength(true)
    , mStartOffsetPresent(false)
    , mWideRange(false)
    , mMaxBlockSize(32)
    , mStartOffset(0)
    , mLength(0)
    , mMetaDataWriteCallback(NULL)
    , mMetaDataAppState(NULL)
{
}

/**
 * @brief
 *  Initialize a "wide" SendInit
 *
 * @param[in]   aVersion        Version of BDX that we are using
 * @param[in]   aSenderDrive    True if the sender is driving
 * @param[in]   aReceiverDrive  True if the receiver is driving
 * @param[in]   aAsynchMode     True if the device supports asynchronous mode
 * @param[in]   aMaxBlockSize   Proposal for a maximum block size for this transfer
 * @param[in]   aStartOffset    Start offset in the file that we should start at
 * @param[in]   aLength         Length of the file to be transferred - 0 means
 *                              it has indefinite length
 * @param[in]   aFileDesignator A string that identifies the data to be transferred
 * @param[in]   aMetaData       (optional) Additional data in TLV format
 *
 * @return #WEAVE_NO_ERROR if successful
 */
WEAVE_ERROR SendInit::init(uint8_t aVersion, bool aSenderDrive, bool aReceiverDrive, bool aAsynchMode,
                           uint16_t aMaxBlockSize, uint64_t aStartOffset, uint64_t aLength,
                           ReferencedString &aFileDesignator, ReferencedTLVData *aMetaData = NULL)
{
    // Version is 8 bits maximum
    mVersion = aVersion;
    mWideRange = true;
    mSenderDriveSupported = aSenderDrive;
    mReceiverDriveSupported = aReceiverDrive;
    mAsynchronousModeSupported = aAsynchMode;
    mMaxBlockSize = aMaxBlockSize;

    mStartOffset = aStartOffset;
    if (mStartOffset > 0)
    {
        mStartOffsetPresent = true;
    }

    mLength = aLength;
    if (mLength == 0)
    {
        mDefiniteLength = false;
    }

    mFileDesignator = aFileDesignator;
    mMetaDataWriteCallback = NULL;
    mMetaDataAppState = NULL;
    if (aMetaData != NULL)
    {
        mMetaData = *aMetaData;
    }

    return WEAVE_NO_ERROR;
}

/**
 * @brief
 *  Initialize a "non-wide" SendInit (32 bit start offset, 32 bit length)
 *
 * @param[in]   aVersion        Version of BDX that we are using
 * @param[in]   aSenderDrive    True if the sender is driving
 * @param[in]   aReceiverDrive  True if the receiver is driving
 * @param[in]   aAsynchMode     True if the device supports asynchronous mode
 * @param[in]   aMaxBlockSize   Proposal for a maximum block size for this transfer
 * @param[in]   aStartOffset    Start offset in the file that we should start at
 * @param[in]   aLength         Length of the file to be transferred - 0 means
 *                              it has indefinite length
 * @param[in]   aFileDesignator A string that identifies the data to be transferred
 * @param[in]   aMetaData       (optional) Additional data in TLV format
 *
 * @return #WEAVE_NO_ERROR if successful
 */
WEAVE_ERROR SendInit::init(uint8_t aVersion, bool aSenderDrive, bool aReceiverDrive, bool aAsynchMode,
                           uint16_t aMaxBlockSize, uint32_t aStartOffset, uint32_t aLength,
                           ReferencedString &aFileDesignator, ReferencedTLVData *aMetaData = NULL)
{
    mWideRange = false;

    return init(aVersion, aSenderDrive, aReceiverDrive, aAsynchMode, aMaxBlockSize,
                (uint64_t)aStartOffset, (uint64_t)aLength, aFileDesignator, aMetaData);
}

/**
 * @brief
 *  Initialize a "wide" SendInit
 *
 * @param[in]   aVersion        Version of BDX that we are using
 * @param[in]   aSenderDrive    True if the sender is driving
 * @param[in]   aReceiverDrive  True if the receiver is driving
 * @param[in]   aAsynchMode     True if the device supports asynchronous mode
 * @param[in]   aMaxBlockSize   Proposal for a maximum block size for this transfer
 * @param[in]   aStartOffset    Start offset in the file that we should start at
 * @param[in]   aLength         Length of the file to be transferred - 0 means
 *                              it has indefinite length
 * @param[in]   aFileDesignator A string that identifies the data to be transferred
 * @param[in]   aMetaDataWriteCallback (optional) A function to write out additional data in TLV format
 *
 * @return #WEAVE_NO_ERROR if successful
 */
WEAVE_ERROR SendInit::init(uint8_t aVersion, bool aSenderDrive, bool aReceiverDrive, bool aAsynchMode,
                           uint16_t aMaxBlockSize, uint64_t aStartOffset, uint64_t aLength,
                           ReferencedString &aFileDesignator, MetaDataTLVWriteCallback aMetaDataWriteCallback = NULL, void *aMetaDataAppState = NULL)
{
    // Version is 8 bits maximum
    mVersion = aVersion;
    mWideRange = true;
    mSenderDriveSupported = aSenderDrive;
    mReceiverDriveSupported = aReceiverDrive;
    mAsynchronousModeSupported = aAsynchMode;
    mMaxBlockSize = aMaxBlockSize;

    mStartOffset = aStartOffset;
    if (mStartOffset > 0)
    {
        mStartOffsetPresent = true;
    }

    mLength = aLength;
    if (mLength == 0)
    {
        mDefiniteLength = false;
    }

    mFileDesignator = aFileDesignator;

    mMetaDataWriteCallback = aMetaDataWriteCallback;
    mMetaDataAppState = aMetaDataAppState;

    return WEAVE_NO_ERROR;
}

/**
 * @brief
 *  Initialize a "non-wide" SendInit (32 bit start offset, 32 bit length)
 *
 * @param[in]   aVersion        Version of BDX that we are using
 * @param[in]   aSenderDrive    True if the sender is driving
 * @param[in]   aReceiverDrive  True if the receiver is driving
 * @param[in]   aAsynchMode     True if the device supports asynchronous mode
 * @param[in]   aMaxBlockSize   Proposal for a maximum block size for this transfer
 * @param[in]   aStartOffset    Start offset in the file that we should start at
 * @param[in]   aLength         Length of the file to be transferred - 0 means
 *                              it has indefinite length
 * @param[in]   aFileDesignator A string that identifies the data to be transferred
 * @param[in]   aMetaDataWriteCallback (optional) A function to write out additional data in TLV format
 *
 * @return #WEAVE_NO_ERROR if successful
 */
WEAVE_ERROR SendInit::init(uint8_t aVersion, bool aSenderDrive, bool aReceiverDrive, bool aAsynchMode,
                           uint16_t aMaxBlockSize, uint32_t aStartOffset, uint32_t aLength,
                           ReferencedString &aFileDesignator, MetaDataTLVWriteCallback aMetaDataWriteCallback = NULL, void *aMetaDataAppState = NULL)
{
    mWideRange = false;

    return init(aVersion, aSenderDrive, aReceiverDrive, aAsynchMode, aMaxBlockSize,
                (uint64_t)aStartOffset, (uint64_t)aLength, aFileDesignator, aMetaDataWriteCallback, aMetaDataAppState);
}

/**
 * @brief
 *  Pack a send init message into an PacketBuffer
 *
 * @param[out]  aBuffer         An PacketBuffer to pack the SendInit message in
 *
 * @retval  #WEAVE_NO_ERROR                 If successful
 * @retval  #WEAVE_ERROR_BUFFER_TOO_SMALL   If buffer is too small
 */
WEAVE_ERROR SendInit::pack(PacketBuffer *aBuffer)
{
    MessageIterator i(aBuffer);
    i.append();
    uint8_t rangeCtl = 0, ptcByte = 0;

    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // First four bits are the version of this message
    ptcByte |= mVersion & VERSION_MASK;
    // pack the transfer control byte
    if (mSenderDriveSupported) ptcByte |= kMode_SenderDrive;
    if (mReceiverDriveSupported) ptcByte |= kMode_ReceiverDrive;
    if (mAsynchronousModeSupported) ptcByte |= kMode_Asynchronous;

    err = i.writeByte(ptcByte);
    SuccessOrExit(err);

    // pack the range control byte
    if (mDefiniteLength) rangeCtl |= kRangeCtl_DefiniteLength;
    if (mStartOffsetPresent) rangeCtl |= kRangeCtl_StartOffsetPresent;
    if (mWideRange) rangeCtl |= kRangeCtl_WideRange;

    err = i.writeByte(rangeCtl);
    SuccessOrExit(err);
    err = i.write16(mMaxBlockSize);
    SuccessOrExit(err);

    if (mStartOffsetPresent)
    {
        if (mWideRange)
        {
            err = i.write64(mStartOffset);
        }
        else
        {
            err = i.write32((uint32_t)mStartOffset);
        }

        SuccessOrExit(err);
    }
    if (mDefiniteLength)
    {
        if (mWideRange)
        {
            err = i.write64(mLength);
        }
        else
        {
            err = i.write32((uint32_t)mLength);
        }

        SuccessOrExit(err);
    }

    mFileDesignator.pack(i);

    if (mMetaDataWriteCallback)
    {
        PacketBuffer *lPacketBuffer = i.GetBuffer();
        uint8_t *buf = lPacketBuffer->Start() + lPacketBuffer->DataLength();
        uint16_t bufLength =
            (lPacketBuffer->AvailableDataLength() < WEAVE_CONFIG_BDX_SEND_INIT_MAX_METADATA_BYTES)
            ? lPacketBuffer->AvailableDataLength() : WEAVE_CONFIG_BDX_SEND_INIT_MAX_METADATA_BYTES;
        uint16_t bytesWritten = 0;
        uint16_t prevDataLength = lPacketBuffer->DataLength();

        err = mMetaDataWriteCallback(buf, bufLength, bytesWritten, mMetaDataAppState);
        SuccessOrExit(err);

        // Adjust i.thePoint, in case we ever need to do something
        // else with i after this.
        i.thePoint += (lPacketBuffer->DataLength() - (prevDataLength - bytesWritten));

        // Adjust the length of aBuffer, which adjusting i.thePoint
        // doesn't accomplish.
        aBuffer->SetDataLength(prevDataLength + bytesWritten);
    }
    else
    {
        mMetaData.pack(i);
    }

exit:
    return err;
}

/**
 * @brief
 *  Returns the packed length of any metadata written out via mMetaDataWriteCallback,
 *  if we have one.
 *
 *  It should be noted that we make two assumptions about any metadata
 *  written out by our callback:
 *
 *  (1) The data is "constant" for the lifetime of the SendInit() to
 *      which it belongs, and won't change no matter when
 *      mMetaDataWriteCallback is called.
 *
 *  (2) The size of the data does not exceed WEAVE_CONFIG_BDX_SEND_INIT_MAX_METADATA_BYTES.
 *
 * @return length of any metadata written out via mMetaDataWriteCallback when packed.
 */
uint16_t SendInit::GetWrittenMetaDataCallbackLength(void)
{
    uint16_t length = 0;

    if (mMetaDataWriteCallback)
    {
        uint8_t buf[WEAVE_CONFIG_BDX_SEND_INIT_MAX_METADATA_BYTES];
        uint16_t bytesWritten = 0;

        mMetaDataWriteCallback(buf, sizeof(buf), bytesWritten, mMetaDataAppState);

        length = (uint16_t)bytesWritten;
    }

    return length;
}

/**
 * @brief
 *  Returns the packed length of this send init message
 *
 * @return length of the message when packed
 */
uint16_t SendInit::packedLength()
{
    // <xfer cctl>+<range ctl>+<max block>+<start offset (optional)>+<length (optional)>+<designator>+<metadata (optional)>
    uint16_t startOffsetLength = mStartOffsetPresent ? (mWideRange ? 8 : 4) : 0;
    uint16_t lengthLength = mDefiniteLength ? (mWideRange ? 8 : 4) : 0;
    uint16_t metaDataLength = 0;

    if (mMetaDataWriteCallback)
    {
        metaDataLength = GetWrittenMetaDataCallbackLength();
    }
    else
    {
        metaDataLength = mMetaData.packedLength();
    }

    return 1 + 1 + 2 + startOffsetLength + lengthLength + (2 + mFileDesignator.theLength) + metaDataLength;
}

/**
 * @brief
 *  Parse data from an PacketBuffer into a SendInit message format
 *
 * @param[in]   aBuffer     Pointer to an PacketBuffer which has the data we want to parse out
 * @param[out]  aRequest    Pointer to a SendInit object where we should store the results
 *
 * @retval  #WEAVE_NO_ERROR                 If successful
 * @retval  #WEAVE_ERROR_BUFFER_TOO_SMALL   If buffer is too small
 */
WEAVE_ERROR SendInit::parse(PacketBuffer *aBuffer, SendInit &aRequest)
{
    MessageIterator i(aBuffer);
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint8_t ptcByte;
    uint8_t rangeCtl;
    uint32_t tmpUint32Value = 0;

    // get the xfer ctl field and unpack it
    err = i.readByte(&ptcByte);
    SuccessOrExit(err);

    aRequest.mVersion = ptcByte & VERSION_MASK;
    aRequest.mSenderDriveSupported = ((ptcByte & kMode_SenderDrive) != 0);
    aRequest.mReceiverDriveSupported = ((ptcByte & kMode_ReceiverDrive) != 0);
    aRequest.mAsynchronousModeSupported = ((ptcByte & kMode_Asynchronous) != 0);

    // now the range ctl field and do the same
    err = i.readByte(&rangeCtl);
    SuccessOrExit(err);

    aRequest.mDefiniteLength = (rangeCtl & kRangeCtl_DefiniteLength) != 0;
    aRequest.mStartOffsetPresent = (rangeCtl & kRangeCtl_StartOffsetPresent) != 0;
    aRequest.mWideRange = (rangeCtl & kRangeCtl_WideRange) != 0;

    err = i.read16(&aRequest.mMaxBlockSize);
    SuccessOrExit(err);
    if (aRequest.mStartOffsetPresent)
    {
        if (aRequest.mWideRange)
        {
            err = i.read64(&aRequest.mStartOffset);
        }
        else
        {
            err = i.read32(&tmpUint32Value);
            aRequest.mStartOffset = tmpUint32Value;
        }

        SuccessOrExit(err);
    }

    if (aRequest.mDefiniteLength)
    {
        if (aRequest.mWideRange)
        {
            err = i.read64(&aRequest.mLength);
        }
        else
        {
            err = i.read32(&tmpUint32Value);
            aRequest.mLength = tmpUint32Value;
        }

        SuccessOrExit(err);
    }

    err = ReferencedString::parse(i, aRequest.mFileDesignator);
    SuccessOrExit(err);
    ReferencedTLVData::parse(i, aRequest.mMetaData);

exit:
    return err;
}

/**
 * @brief
 *  Equality comparison between SendInit messages
 *
 * @param[in]   another     Another SendInit message to compare this one to
 *
 * @return true iff they have all the same fields.
 */
bool SendInit::operator == (const SendInit &another) const
{
    return (mVersion == another.mVersion &&
            mSenderDriveSupported == another.mSenderDriveSupported &&
            mReceiverDriveSupported == another.mReceiverDriveSupported &&
            mAsynchronousModeSupported == another.mAsynchronousModeSupported &&
            mDefiniteLength == another.mDefiniteLength &&
            mStartOffsetPresent == another.mStartOffsetPresent &&
            mAsynchronousModeSupported == another.mAsynchronousModeSupported &&
            mMaxBlockSize == another.mMaxBlockSize &&
            mStartOffset == another.mStartOffset &&
            mFileDesignator == another.mFileDesignator &&
            mMetaData == another.mMetaData);
}

/*
 * -- definitions for SendAccept and its supporting classes --
 *
 * the no-arg constructor with defaults for the send accept message
 */
SendAccept::SendAccept()
    : mVersion(0)
    , mTransferMode(kMode_SenderDrive)
    , mMaxBlockSize(0)
{
}

/*
 * parameters:
 * - uint8_t aTransferMode, a transfer mode value. must be at MOST one of the
 * flag values for this field.
 * - uiunt16_t aMaxBlockSize, the maximum allowable block sixe for this exchange
 * - ReferencedTLVData *aMetaData, optional metadata
 * return: error/status
 */

/**
 * @brief
 *  Initialize a SendAccept message
 *
 * @param[in]   aVersion            Version of BDX that we are using
 * @param[in]   aTransferMode       Transfer mode that this transfer should have
 *                                  (Must be one of kMode_SenderDrive, kMode_ReceiverDrive, kMode_Asynchronous)
 * @param[in]   aMaxBlockSize       Maximum block size for this exchange
 * @param[in]   aMetaData           (optional) Optional TLV metadata
 *
 * @retval  #WEAVE_NO_ERROR                 If successful
 * @retval  #WEAVE_ERROR_BUFFER_TOO_SMALL   If buffer is too small
 */
WEAVE_ERROR SendAccept::init(uint8_t aVersion, uint8_t aTransferMode, uint16_t aMaxBlockSize, ReferencedTLVData *aMetaData = NULL)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    VerifyOrExit((aTransferMode & kMode_SenderDrive ||
                  aTransferMode & kMode_ReceiverDrive),
                 err = WEAVE_ERROR_INVALID_TRANSFER_MODE);

    mVersion = aVersion;
    mTransferMode = aTransferMode;
    mMaxBlockSize = aMaxBlockSize;

    if (aMetaData != NULL)
    {
        mMetaData = *aMetaData;
    }

exit:
    return err;
}

/**
 * @brief
 *  Pack a send accept message into an PacketBuffer
 *
 * @param[out]  aBuffer         An PacketBuffer to pack the SendAccept message in
 *
 * @retval  #WEAVE_NO_ERROR                 If successful
 * @retval  #WEAVE_ERROR_BUFFER_TOO_SMALL   If buffer is too small
 */
WEAVE_ERROR SendAccept::pack(PacketBuffer *aBuffer)
{
    MessageIterator i(aBuffer);
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    i.append();
    err = i.writeByte(mTransferMode | (mVersion & VERSION_MASK));
    SuccessOrExit(err);

    err = i.write16(mMaxBlockSize);
    SuccessOrExit(err);

    mMetaData.pack(i);

exit:
    return err;
}

/**
 * @brief
 *  Returns the packed length of this send accept message
 *
 * @return length of the message when packed
 */
uint16_t SendAccept::packedLength()
{
    // <transfer mode>+<max block size>+<meta data (optional)>
    return 1 + 2 + mMetaData.packedLength();
}

/**
 * @brief
 *  Parse data from an PacketBuffer into a SendAccept message format
 *
 * @param[in]   aBuffer     Pointer to an PacketBuffer which has the data we want to parse out
 * @param[out]  aResponse   Pointer to a SendAccept object where we should store the results
 *
 * @retval  #WEAVE_NO_ERROR                 If successful
 * @retval  #WEAVE_ERROR_BUFFER_TOO_SMALL   If buffer is too small
 */
WEAVE_ERROR SendAccept::parse(PacketBuffer *aBuffer, SendAccept &aResponse)
{
    MessageIterator i(aBuffer);
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    uint8_t tcByte;

    err = i.readByte(&tcByte);
    SuccessOrExit(err);

    aResponse.mVersion = tcByte & VERSION_MASK ;
    aResponse.mTransferMode = tcByte & ~VERSION_MASK;

    err = i.read16(&aResponse.mMaxBlockSize);
    SuccessOrExit(err);

    ReferencedTLVData::parse(i, aResponse.mMetaData);

exit:
    return err;
}

/**
 * @brief
 *  Equality comparison between SendAccept messages
 *
 * @param[in]   another     Another SendAccept message to compare this one to
 *
 * @return true iff they have all the same fields.
 */
bool SendAccept::operator == (const SendAccept &another) const
{
    return (mVersion == another.mVersion &&
            mTransferMode == another.mTransferMode &&
            mMaxBlockSize == another.mMaxBlockSize &&
            mMetaData == another.mMetaData);
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
    mVersion = 0;
    mSenderDriveSupported = false;
    mReceiverDriveSupported = true;
    mAsynchronousModeSupported = false;
    mDefiniteLength = true;
    mStartOffsetPresent = false;
    mWideRange = false;
    mMaxBlockSize = 32;
    mStartOffset = 0;
    mLength = 0;
}

/*
 * -- definitions for ReceiveAccept and its supporting classes --
 *
 * the no-arg constructor with defaults for the send accept message
 */
ReceiveAccept::ReceiveAccept()
    : mDefiniteLength(true)
    , mWideRange(false)
    , mLength(0)
{
    mTransferMode = kMode_ReceiverDrive;
    mVersion = 0;
    mMaxBlockSize = 0;
}

/**
 * @brief
 *  Initialze a "wide" receive accept frame
 *
 * @param[in]   aVersion            Version of BDX that we are using
 * @param[in]   aTransferMode       Transfer mode to be used in the transfer
 * @param[in]   aMaxBlockSize       Maximum allowed block size for this transfer
 * @param[in]   aLength             Length of the file to be transferred, 0 if indefinite
 * @param[in]   aMetaData           (optional) TLV Metadata
 *
 * @return #WEAVE_NO_ERROR if successful
 */
WEAVE_ERROR ReceiveAccept::init(uint8_t aVersion, uint8_t aTransferMode, uint16_t aMaxBlockSize, uint64_t aLength, ReferencedTLVData *aMetaData = NULL)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    VerifyOrExit((aTransferMode & kMode_SenderDrive ||
                  aTransferMode & kMode_ReceiverDrive),
                 err = WEAVE_ERROR_INVALID_TRANSFER_MODE);

    mDefiniteLength = (aLength != 0);
    mWideRange = true;
    mVersion = aVersion;
    mTransferMode = aTransferMode;
    mMaxBlockSize = aMaxBlockSize;
    mLength = aLength;

    if (aMetaData != NULL)
    {
        mMetaData = *aMetaData;
    }

exit:
    return err;
}

/**
 * @brief
 *  Initialze a "non-wide" receive accept frame (32 bit length)
 *
 * @param[in]   aVersion            Version of BDX that we are using
 * @param[in]   aTransferMode       Transfer mode to be used in the transfer
 * @param[in]   aMaxBlockSize       Maximum allowed block size for this transfer
 * @param[in]   aLength             Length of the file to be transferred, 0 if indefinite
 * @param[in]   aMetaData           (optional) TLV Metadata
 *
 * @return #WEAVE_NO_ERROR if successful
 */
WEAVE_ERROR ReceiveAccept::init(uint8_t aVersion, uint8_t aTransferMode, uint16_t aMaxBlockSize, uint32_t aLength, ReferencedTLVData *aMetaData = NULL)
{
    mWideRange = false;

    return init(aVersion, aTransferMode, aMaxBlockSize,
                (uint64_t)aLength, aMetaData);
}

/**
 * @brief
 *  Pack a receive accept message into an PacketBuffer
 *
 * @param[out]  aBuffer         An PacketBuffer to pack the ReceiveAccept message in
 *
 * @retval  #WEAVE_NO_ERROR                 If successful
 * @retval  #WEAVE_ERROR_BUFFER_TOO_SMALL   If buffer is too small
 */
WEAVE_ERROR ReceiveAccept::pack(PacketBuffer *aBuffer)
{
    MessageIterator i(aBuffer);
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint8_t rangeCtl = 0;

    i.append();
    err = i.writeByte(mTransferMode | (mVersion & VERSION_MASK));
    SuccessOrExit(err);

    // format and pack the range control field
    if (mDefiniteLength) rangeCtl |= kRangeCtl_DefiniteLength;
    if (mWideRange) rangeCtl |= kRangeCtl_WideRange;

    err = i.writeByte(rangeCtl);
    SuccessOrExit(err);

    err = i.write16(mMaxBlockSize);
    SuccessOrExit(err);

    // and the length, if any
    if (mDefiniteLength)
    {
        if (mWideRange)
        {
            err = i.write64(mLength);
        }
        else
        {
            err = i.write32((uint32_t)mLength);
        }

        SuccessOrExit(err);
    }

    mMetaData.pack(i);

exit:
    return err;
}

/**
 * @brief
 *  Returns the packed length of this receive accept message
 *
 * @return length of the message when packed
 */
uint16_t ReceiveAccept::packedLength()
{
    // <transfer mode>+<range control>+<max block size>+<length (optional)>+<meta data (optional)>
    return 1 + 1 + 2 + (mDefiniteLength ? (mWideRange ? 8 : 4) : 0) + mMetaData.packedLength();
}

/**
 * @brief
 *  Parse data from an PacketBuffer into a ReceiveAccept message format
 *
 * @param[in]   aBuffer     Pointer to an PacketBuffer which has the data we want to parse out
 * @param[out]  aResponse   Pointer to a ReceiveAccept object where we should store the results
 *
 * @retval  #WEAVE_NO_ERROR                 If successful
 * @retval  #WEAVE_ERROR_BUFFER_TOO_SMALL   If buffer is too small
 */
WEAVE_ERROR ReceiveAccept::parse(PacketBuffer *aBuffer, ReceiveAccept &aResponse)
{
    MessageIterator i(aBuffer);
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint8_t tcByte;
    uint8_t rangeCtl;
    uint32_t tmpUint32Value;

    // unpack the transfer control byte
    err = i.readByte(&tcByte);
    SuccessOrExit(err);

    aResponse.mVersion = tcByte & VERSION_MASK ;
    aResponse.mTransferMode = tcByte & ~VERSION_MASK;

    // unpack the range control byte
    err = i.readByte(&rangeCtl);
    SuccessOrExit(err);

    aResponse.mDefiniteLength = ((rangeCtl & kRangeCtl_DefiniteLength) != 0);
    aResponse.mWideRange = ((rangeCtl & kRangeCtl_WideRange) != 0);

    err = i.read16(&aResponse.mMaxBlockSize);
    SuccessOrExit(err);

    if (aResponse.mDefiniteLength)
    {
        if (aResponse.mWideRange)
        {
            err = i.read64(&aResponse.mLength);
        }
        else
        {
            err = i.read32(&tmpUint32Value);
            aResponse.mLength = tmpUint32Value;
        }

        SuccessOrExit(err);
    }

    ReferencedTLVData::parse(i, aResponse.mMetaData);

exit:
    return err;
}

/**
 * @brief
 *  Equality comparison between ReceiveAccept messages
 *
 * @param[in]   another     Another ReceiveAccept message to compare this one to
 *
 * @return true iff they have all the same fields.
 */
bool ReceiveAccept::operator == (const ReceiveAccept &another) const
{
    return (mTransferMode == another.mTransferMode &&
            mDefiniteLength == another.mDefiniteLength &&
            mWideRange == another.mWideRange &&
            mMaxBlockSize == another.mMaxBlockSize &&
            mLength == another.mLength &&
            mMetaData == another.mMetaData);
}

/*
 * -- definitions for BlockQuery and its supporting classes --
 *
 * the no-arg constructor with defaults for the block query message
 */
BlockQuery::BlockQuery()
    : mBlockCounter(0)
{
}

/**
 * @brief
 *  Initialize a BlockQuery message
 *
 * @param[in]   aCounter        Block counter value to query for
 *
 * @return #WEAVE_NO_ERROR if successful
 */
WEAVE_ERROR BlockQuery::init(uint8_t aCounter)
{
    mBlockCounter = aCounter;
    return WEAVE_NO_ERROR;
}

/**
 * @brief
 *  Pack a block query message into an PacketBuffer
 *
 * @param[out]  aBuffer         An PacketBuffer to pack the BlockQuery message in
 *
 * @retval  #WEAVE_NO_ERROR                 If successful
 * @retval  #WEAVE_ERROR_BUFFER_TOO_SMALL   If buffer is too small
 */
WEAVE_ERROR BlockQuery::pack(PacketBuffer *aBuffer)
{
    MessageIterator i(aBuffer);
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    i.append();
    err = i.writeByte(mBlockCounter);
    return err;
}

/**
 * @brief
 *  Returns the packed length of this block query message
 *
 * @return length of the message when packed
 */
uint16_t BlockQuery::packedLength()
{
    // just the counter
    return 1;
}

/**
 * @brief
 *  Parse data from an PacketBuffer into a BlockQuery message format
 *
 * @param[in]   aBuffer     Pointer to an PacketBuffer which has the data we want to parse out
 * @param[out]  aQuery      Pointer to a BlockQuery object where we should store the results
 *
 * @retval  #WEAVE_NO_ERROR                 If successful
 * @retval  #WEAVE_ERROR_BUFFER_TOO_SMALL   If buffer is too small
 */
WEAVE_ERROR BlockQuery::parse(PacketBuffer *aBuffer, BlockQuery &aQuery)
{
    MessageIterator i(aBuffer);
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    err = i.readByte(&aQuery.mBlockCounter);
    return err;
}

/**
 * @brief
 *  Equality comparison between BlockQuery messages
 *
 * @param[in]   another     Another BlockQuery message to compare this one to
 *
 * @return true iff they have all the same fields.
 */
bool BlockQuery::operator == (const BlockQuery &another) const
{
    return mBlockCounter == another.mBlockCounter;
}

/*
 * -- definitions for BlockSend and its supporting classes --
 *
 * the no-arg constructor with defaults for the block send message
 */
BlockSend::BlockSend()
    : mBlockCounter(0)
    , mLength(0)
    , mData(NULL)
{
}

/**
 * @brief
 *  Initialize a BlockSend message
 *
 * @param[in]   aCounter        Block counter value for this block
 * @param[in]   aLength         Length of the block
 * @param[in]   aData           Pointer to the data to be transferred
 *
 * @return #WEAVE_NO_ERROR if successful
 */
WEAVE_ERROR BlockSend::init(uint8_t aCounter, uint64_t aLength, uint8_t *aData)
{
    mBlockCounter = aCounter;
    mLength = aLength;
    mData = aData;
    return WEAVE_NO_ERROR;
}

/**
 * @brief
 *  Returns the packed length of this block send message
 *
 * @return length of the message when packed
 */
uint16_t BlockSend::packedLength()
{
    // <block counter>+<length>
    return sizeof(mBlockCounter) + mLength;
}

/**
 * @brief
 *  Parse data from an PacketBuffer into a BlockSend message format
 *
 * @param[in]   aBuffer     Pointer to an PacketBuffer which has the data we want to parse out
 * @param[out]  aResponse   Pointer to a BlockSend object where we should store the results
 *
 * @retval  #WEAVE_NO_ERROR                 If successful
 * @retval  #WEAVE_ERROR_BUFFER_TOO_SMALL   If buffer is too small
 */
WEAVE_ERROR BlockSend::parse(PacketBuffer *aBuffer, BlockSend &aResponse)
{
    MessageIterator i(aBuffer);
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(aBuffer->DataLength() >= sizeof(aResponse.mBlockCounter), err = WEAVE_ERROR_INVALID_MESSAGE_LENGTH);

    err = i.readByte(&aResponse.mBlockCounter);
    SuccessOrExit(err);

    aResponse.mLength = (uint64_t)aBuffer->DataLength() - sizeof(aResponse.mBlockCounter);
    if (aResponse.mLength == 0)
    {
        aResponse.mData = NULL;
    }
    else
    {
        aResponse.mData = aBuffer->Start() + sizeof(aResponse.mBlockCounter);
    }
    /*
     * we're holding onto this buffer and, while we might not want to
     * write anything after the data in it we want to:
     * - move the message iterator's insertion point past the data we've
     * just "parsed".
     * - set the private buffer pointer data member to point to it
     * - increment the reference count on the inet buffer.
     */
    i.thePoint += aResponse.mLength;
    aResponse.Retain(aBuffer);

exit:
    return err;
}

/**
 * @brief
 *  Equality comparison between BlockSend messages
 *
 * @param[in]   another     Another BlockSend message to compare this one to
 *
 * @return true iff they have all the same fields.
 */
bool BlockSend::operator == (const BlockSend &another) const
{
    return (mBlockCounter == another.mBlockCounter &&
            mLength == another.mLength &&
            // Use mLength as length since mLength == another.mLength at this point
            memcmp(mData, another.mData, mLength) == 0);
}


/*
 * -- definitions for BlockQueryV1 and its supporting classes --
 *
 * the no-arg constructor with defaults for the block query message
 */
BlockQueryV1::BlockQueryV1()
    : mBlockCounter(0)
{
}

/**
 * @brief
 *  Initialize a BlockQueryV1 message
 *
 * @param[in]   aCounter        Block counter value to query for
 *
 * @return #WEAVE_NO_ERROR if successful
 */
WEAVE_ERROR BlockQueryV1::init(uint32_t aCounter)
{
    mBlockCounter = aCounter;

    return WEAVE_NO_ERROR;
}

/**
 * @brief
 *  Pack a block query message into an PacketBuffer
 *
 * @param[out]  aBuffer         An PacketBuffer to pack the BlockQueryV1 message in
 *
 * @retval  #WEAVE_NO_ERROR                 If successful
 * @retval  #WEAVE_ERROR_BUFFER_TOO_SMALL   If buffer is too small
 */
WEAVE_ERROR BlockQueryV1::pack(PacketBuffer *aBuffer)
{
    MessageIterator i(aBuffer);
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    i.append();
    err = i.write32(mBlockCounter);

    return err;
}

/**
 * @brief
 *  Returns the packed length of this block query message
 *
 * @return length of the message when packed
 */
uint16_t BlockQueryV1::packedLength()
{
    // just the counter
    return sizeof(mBlockCounter);
}

/**
 * @brief
 *  Parse data from an PacketBuffer into a BlockQueryV1 message format
 *
 * @param[in]   aBuffer     Pointer to an PacketBuffer which has the data we want to parse out
 * @param[out]  aQuery      Pointer to a BlockQueryV1 object where we should store the results
 *
 * @retval  #WEAVE_NO_ERROR                 If successful
 * @retval  #WEAVE_ERROR_BUFFER_TOO_SMALL   If buffer is too small
 */
WEAVE_ERROR BlockQueryV1::parse(PacketBuffer *aBuffer, BlockQueryV1 &aQuery)
{
    MessageIterator i(aBuffer);
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    err = i.read32(&aQuery.mBlockCounter);

    return err;
}

/**
 * @brief
 *  Equality comparison between BlockQueryV1 messages
 *
 * @param[in]   another     Another BlockQueryV1 message to compare this one to
 *
 * @return true iff they have all the same fields.
 */
bool BlockQueryV1::operator == (const BlockQueryV1 &another) const
{
    return mBlockCounter == another.mBlockCounter;
}

/*
 * -- definitions for BlockSendV1 and its supporting classes --
 *
 * the no-arg constructor with defaults for the block send message
 */
BlockSendV1::BlockSendV1()
    : mBlockCounter(0)
    , mLength(0)
    , mData(NULL)
{
}

/**
 * @brief
 *  Initialize a BlockSendV1 message
 *
 * @param[in]   aCounter        Block counter value for this block
 * @param[in]   aLength         Length of the block
 * @param[in]   aData           Pointer to the data to be transferred
 *
 * @return #WEAVE_NO_ERROR if successful
 */
WEAVE_ERROR BlockSendV1::init(uint32_t aCounter, uint64_t aLength, uint8_t *aData)
{
    mBlockCounter = aCounter;
    mLength = aLength;
    mData = aData;

    return WEAVE_NO_ERROR;
}

/**
 * @brief
 *  Returns the packed length of this block send message
 *
 * @return length of the message when packed
 */
uint16_t BlockSendV1::packedLength()
{
    // <block counter>+<length>
    return sizeof(mBlockCounter) + mLength;
}

/**
 * @brief
 *  Parse data from an PacketBuffer into a BlockSendV1 message format
 *
 * @param[in]   aBuffer     Pointer to an PacketBuffer which has the data we want to parse out
 * @param[out]  aResponse   Pointer to a BlockSendV1 object where we should store the results
 *
 * @retval  #WEAVE_NO_ERROR                 If successful
 * @retval  #WEAVE_ERROR_BUFFER_TOO_SMALL   If buffer is too small
 */
WEAVE_ERROR BlockSendV1::parse(PacketBuffer *aBuffer, BlockSendV1 &aResponse)
{
    MessageIterator i(aBuffer);
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(aBuffer->DataLength() >= sizeof(aResponse.mBlockCounter), err = WEAVE_ERROR_INVALID_MESSAGE_LENGTH);

    err = i.read32(&aResponse.mBlockCounter);
    SuccessOrExit(err);

    aResponse.mLength = (uint64_t)aBuffer->DataLength() - sizeof(aResponse.mBlockCounter);
    if (aResponse.mLength == 0)
    {
        aResponse.mData = NULL;
    }
    else
    {
        aResponse.mData = aBuffer->Start() + sizeof(aResponse.mBlockCounter);
    }
    /*
     * we're holding onto this buffer and, while we might not want to
     * write anything after the data in it we want to:
     * - move the message iterator's insertion point past the data we've
     * just "parsed".
     * - set the private buffer pointer data member to point to it
     * - increment the reference count on the inet buffer.
     */
    i.thePoint += aResponse.mLength + sizeof(aResponse.mBlockCounter);
    aResponse.Retain(aBuffer);

exit:
    return err;
}

/**
 * @brief
 *  Equality comparison between BlockSendV1 messages
 *
 * @param[in]   another     Another BlockSendV1 message to compare this one to
 *
 * @return true iff they have all the same fields.
 */
bool BlockSendV1::operator == (const BlockSendV1 &another) const
{
    return (mBlockCounter == another.mBlockCounter &&
            mLength == another.mLength &&
            // Use mLength as length since mLength == another.mLength at this point
            memcmp(mData, another.mData, mLength) == 0);
}

} // namespace BulkDataTransfer
} // namespace Profiles
} // namespace Weave
} // namespace nl
