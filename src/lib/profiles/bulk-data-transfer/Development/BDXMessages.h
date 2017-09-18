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
 *      Messages definitions for Bulk Data Transfer.
 */

#ifndef _BULK_DATA_TRANSFER_MESSAGES_H
#define _BULK_DATA_TRANSFER_MESSAGES_H

#include <Weave/Profiles/bulk-data-transfer/Development/BDXManagedNamespace.hpp>
#include <Weave/Profiles/bulk-data-transfer/Development/BDXConstants.h>
#include <Weave/Support/NLDLLUtil.h>
#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveMessageLayer.h>
#include <Weave/Profiles/ProfileCommon.h>

/**
 *   @namespace nl::Weave::Profiles::WeaveMakeManagedNamespaceIdentifier(BDX, kWeaveManagedNamespaceDesignation_Development) {
 *
 *   @brief
 *     This namespace includes all interfaces within Weave for the
 *     Weave Bulk Data Transfer (BDX) profile.
 */

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(BDX, kWeaveManagedNamespaceDesignation_Development) {

/**
 * @class SendInit
 *
 * @brief
 *   The SendInit message is used to start an exchange when the sender
 *   is the initiator.
 */
class NL_DLL_EXPORT SendInit
{
public:
    SendInit(void);

    WEAVE_ERROR init(uint8_t aVersion,
                     bool aSenderDrive,
                     bool aReceiverDrive,
                     bool aAsynchMode,
                     uint16_t aMaxBlockSize,
                     uint64_t aStartOffset,
                     uint64_t aLength,
                     ReferencedString &aFileDesignator,
                     ReferencedTLVData *aMetaData);

    WEAVE_ERROR init(uint8_t aVersion,
                     bool aSenderDrive,
                     bool aReceiverDrive,
                     bool aAsynchMode,
                     uint16_t aMaxBlockSize,
                     uint32_t aStartOffset,
                     uint32_t aLength,
                     ReferencedString &aFileDesignator,
                     ReferencedTLVData *aMetaData);

    /**
     * @brief
     *
     *   MetaDataTLVWriteCallback provides a means by which a client
     *   can supply a SendInit with any metadata they want.  The
     *   client is free to supply pre-encoded TLV (faster), encode
     *   on-the-fly (uses less memory), lazy-encode (a littl faster on
     *   startup), etc. as they see fit.
     *
     *   In all cases, it is assumed that the data produced by the
     *   callback is constant for a given SendInit, i.e. does not
     *   change no matter when it is called.  This is because the
     *   callback is also used to compute the length of any such
     *   written-out TLV, which could be requested at any time.
     *
     * @param[in]    aBuffer           The destination buffer, into which some TLV can be written
     * @param[in]    aBufferLength     The length (in bytes) of the destination buffer
     * @param[inout] aNumBytesWritten  The the number of bytes written to the destination buffer
     * @param[in]    aAppState         User-provided app state
     *
     * @retval       #WEAVE_ERROR      Any error encountered.
     */
    typedef WEAVE_ERROR (*MetaDataTLVWriteCallback)(uint8_t *aBuffer,
                                                    uint16_t aBufferLength,
                                                    uint16_t &aNumBytesWritten,
                                                    void *aAppState);

    WEAVE_ERROR init(uint8_t aVersion,
                     bool aSenderDrive,
                     bool aReceiverDrive,
                     bool aAsynchMode,
                     uint16_t aMaxBlockSize,
                     uint64_t aStartOffset,
                     uint64_t aLength,
                     ReferencedString &aFileDesignator,
                     MetaDataTLVWriteCallback aMetaDataWriteCallback,
                     void *aMetaDataAppState);

    WEAVE_ERROR init(uint8_t aVersion,
                     bool aSenderDrive,
                     bool aReceiverDrive,
                     bool aAsynchMode,
                     uint16_t aMaxBlockSize,
                     uint32_t aStartOffset,
                     uint32_t aLength,
                     ReferencedString &aFileDesignator,
                     MetaDataTLVWriteCallback aMetaDataWriteCallback,
                     void *aMetaDataAppState);

    WEAVE_ERROR pack(PacketBuffer *aBuffer);
    uint16_t packedLength(void);
    static WEAVE_ERROR parse(PacketBuffer *aBuffer, SendInit &aRequest);

private:
    uint16_t GetWrittenMetaDataCallbackLength(void);

public:
    bool operator == (const SendInit&) const;

    uint8_t mVersion;                   /**< Version of the BDX protocol we decided on. */
    // Transfer mode options
    bool mSenderDriveSupported;         /**< True if we can support sender drive. */
    bool mReceiverDriveSupported;       /**< True if we can support receiver drive. */
    bool mAsynchronousModeSupported;    /**< True if we can support async mode. */
    // Range control options
    bool mDefiniteLength;               /**< True if the length field is present. */
    bool mStartOffsetPresent;           /**< True if the start offset field is present. */
    bool mWideRange;                    /**< True if offset and length are 64 bits. */
    // Block size and offset
    uint16_t mMaxBlockSize;             /**< Proposed max block size to use in transfer. */
    uint64_t mStartOffset;              /**< Proposed start offset of data. */
    uint64_t mLength;                   /**< Proposed length of data in transfer, 0 for indefinite. */
    // File designator
    ReferencedString mFileDesignator;   /**< String containing pre-negotiated information. */
    // Additional metadata
    ReferencedTLVData mMetaData;        /**< Optional TLV Metadata. */
    MetaDataTLVWriteCallback mMetaDataWriteCallback; /**< Optional function to write out TLV Metadata. */
    void *mMetaDataAppState;            /**< Optional app state for TLV Metadata. */
};

/**
 * @class SendAccept
 *
 * @brief
 *   The SendAccept message is used to accept a proposed exchange when the
 *   sender is the initiator.
 */
class SendAccept
{
public:
    SendAccept(void);

    WEAVE_ERROR init(uint8_t aVersion, uint8_t aTransferMode, uint16_t aMaxBlockSize, ReferencedTLVData *aMetaData);

    WEAVE_ERROR pack(PacketBuffer *aBuffer);
    uint16_t packedLength(void);
    static WEAVE_ERROR parse(PacketBuffer *aBuffer, SendAccept &aResponse);

public:
    bool operator == (const SendAccept&) const;

    uint8_t mVersion;               /**< Version of the BDX protocol we decided on. */
    uint8_t mTransferMode;          /**< Transfer mode that we decided on. */
    uint16_t mMaxBlockSize;         /**< Maximum block size we decided on. */
    ReferencedTLVData mMetaData;    /**< Optional TLV Metadata. */
};

/**
 * @class SendReject
 *
 * @brief
 *   The SendReject message is used to reject a proposed exchange when the
 *   sender is the initiator.
 */
class SendReject : public StatusReport { };

/**
 * @class ReceiveInit
 *
 * @brief
 *   The ReceiveInit message is used to start an exchange when the receiver
 *   is the initiator.
 */
class NL_DLL_EXPORT ReceiveInit : public SendInit
{
public:
    ReceiveInit(void);
};

/**
 * @class ReceiveAccept
 *
 * @brief
 *   The ReceiveAccept message is used to accept a proposed exchange when the
 *   receiver is the initiator.
 */
class ReceiveAccept : public SendAccept
{
public:
    ReceiveAccept(void);

    WEAVE_ERROR init(uint8_t aVersion,
                     uint8_t aTransferMode,
                     uint16_t aMaxBlockSize,
                     uint64_t aLength,
                     ReferencedTLVData *aMetaData);
    WEAVE_ERROR init(uint8_t aVersion,
                     uint8_t aTransferMode,
                     uint16_t aMaxBlockSize,
                     uint32_t aLength,
                     ReferencedTLVData *aMetaData);

    WEAVE_ERROR pack(PacketBuffer *aBuffer);
    uint16_t packedLength(void);
    static WEAVE_ERROR parse(PacketBuffer *aBuffer, ReceiveAccept &aResponse);

public:
    bool operator == (const ReceiveAccept&) const;

    // Accepted range control options
    bool mDefiniteLength;           /**< True if a definite length was chosen. */
    bool mWideRange;                /**< True if our range and offset fields are 64 bits. */
    uint64_t mLength;               /**< Length of transfer we decided on. */
    ReferencedTLVData mMetaData;    /**< Optional TLV Metadata. */
};

/**
 * @class ReceiveReject
 *
 * @brief
 *   The ReceiveReject message is used to reject a proposed exchange when the
 *   sender is the initiator.
 */
class ReceiveReject : public StatusReport { };

/**
 * @class BlockQuery
 *
 * @brief
 *   The BlockQuery message is used to request that a block of data
 *   be transfered from sender to receiver.
 */
class NL_DLL_EXPORT BlockQuery
{
public:
    BlockQuery(void);

    WEAVE_ERROR init(uint8_t aCounter);

    WEAVE_ERROR pack(PacketBuffer *aBuffer);
    uint16_t packedLength(void);
    static WEAVE_ERROR parse(PacketBuffer *aBuffer, BlockQuery &aQuery);

    // BlockQuery payload length
    enum
    {
        kPayloadLen = 1,
    };

public:
    bool operator == (const BlockQuery&) const;

    uint8_t mBlockCounter;      /**< Counter of the block that we are asking for. */
};

/**
 * @class BlockSend
 *
 * @brief
 *   The BlockSend message is used to transfer a block of data from sender
 *   to receiver.
 */
class BlockSend : public RetainedPacketBuffer
{
public:
    BlockSend(void);

    WEAVE_ERROR init(uint8_t aCounter, uint64_t aLength, uint8_t *aData);

    uint16_t packedLength(void);
    static WEAVE_ERROR parse(PacketBuffer *aBuffer, BlockSend &aResponse);

public:
    bool operator == (const BlockSend&) const;

    uint8_t mBlockCounter;      /**< Counter of this block that is being sent. */
    uint64_t mLength;           /**< Length of data contained in this block. */
    uint8_t *mData;             /**< Pointer to the data to be received or transferred. */
};

/**
 * @class BlockEOF
 *
 * @brief
 *   The BlockEOF message is used to transfer the last block of data from
 *   sender to receiver.
 */
class BlockEOF : public BlockSend { };

/**
 * @class BlockAck
 *
 * @brief
 *   The BlockAck message is used to acknowledge a block of data
 */
class BlockAck : public BlockQuery { };

/**
 * @class BlockEOFAck
 *
 * @brief
 *   The BlockEOFAck message is used to acknowledge the last block of data
 */
class BlockEOFAck : public BlockQuery { };

/**
 * @class Transfer Error
 *
 * @brief
 *   The Error message is used to report an error and abort an exchange
 */
class TransferError : public StatusReport { };

/**
 * @class BlockQueryV1
 *
 * @brief
 *   The BlockQueryV1 message is used to request that a block of data
 *   be transfered from sender to receiver. It includes a 4 byte block counter.
 */
class NL_DLL_EXPORT BlockQueryV1
{
public:
    BlockQueryV1(void);

    WEAVE_ERROR init(uint32_t aCounter);

    WEAVE_ERROR pack(PacketBuffer *aBuffer);
    uint16_t packedLength(void);
    static WEAVE_ERROR parse(PacketBuffer *aBuffer, BlockQueryV1 &aQuery);

    // BlockQueryV1 payload length
    enum
    {
        kPayloadLen = 4,
    };

public:
    bool operator == (const BlockQueryV1&) const;

    uint32_t mBlockCounter;     /**< Counter of the block that we are asking for. */
};

/**
 * @class BlockSendV1
 *
 * @brief
 *   The BlockSendV1 message is used to transfer a block of data from sender
 *   to receiver. It has a 4 byte block counter.
 */
class BlockSendV1 : public RetainedPacketBuffer
{
public:
    BlockSendV1(void);

    WEAVE_ERROR init(uint32_t aCounter, uint64_t aLength, uint8_t *aData);

    uint16_t packedLength(void);
    static WEAVE_ERROR parse(PacketBuffer *aBuffer, BlockSendV1 &aResponse);

public:
    bool operator == (const BlockSendV1&) const;

    uint32_t mBlockCounter;     /**< Counter of this block that is being sent. */
    uint64_t mLength;           /**< Length of data contained in this block. */
    uint8_t *mData;             /**< Pointer to the data to be received or transferred. */
};

/**
 * @class BlockEOFV1
 *
 * @brief
 *   The BlockEOFV1 message is used to transfer the last block of data from
 *   sender to receiver. It has a 4 byte block counter.
 */
class BlockEOFV1 : public BlockSendV1 { };

/**
 * @class BlockAckV1
 *
 * @brief
 *   The BlockAckV1 message is used to acknowledge a block of data.
 *   It has a 4 byte block counter.
 */
class BlockAckV1 : public BlockQueryV1 { };

/**
 * @class BlockEOFAckV1
 *
 * @brief
 *   The BlockEOFAckV1 message is used to acknowledge the last block of data.
 *   It has a 4 byte block counter.
 */
class BlockEOFAckV1 : public BlockQueryV1 { };

} // namespace WeaveMakeManagedNamespaceIdentifier(BDX, kWeaveManagedNamespaceDesignation_Development)
} // namespace Profiles
} // namespace Weave
} // namespace nl

#endif // _BULK_DATA_TRANSFER_MESSAGES_H
