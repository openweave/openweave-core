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
 *      The constants for Bulk Data Transfer are in this file.
 *      Includes message types, transfer modes, range control, status codes.
 */

#ifndef _BULK_DATA_TRANSFER_CONSTANTS_H
#define _BULK_DATA_TRANSFER_CONSTANTS_H

#include <Weave/Profiles/bulk-data-transfer/Development/BDXManagedNamespace.hpp>
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

// the message type values
enum
{
    kMsgType_SendInit =                     0x01,
    kMsgType_SendAccept =                   0x02,
    kMsgType_SendReject =                   0x03,
    kMsgType_ReceiveInit =                  0x04,
    kMsgType_ReceiveAccept =                0x05,
    kMsgType_ReceiveReject =                0x06,
    kMsgType_BlockQuery =                   0x07,
    kMsgType_BlockSend =                    0x08,
    kMsgType_BlockEOF =                     0x09,
    kMsgType_BlockAck =                     0x0A,
    kMsgType_BlockEOFAck =                  0x0B,
    kMsgType_TransferError =                0x0F,
    kMsgType_BlockQueryV1 =                 0x10,
    kMsgType_BlockSendV1 =                  0x11,
    kMsgType_BlockEOFV1 =                   0x12,
    kMsgType_BlockAckV1 =                   0x13,
    kMsgType_BlockEOFAckV1 =                0x14,
};

/*
 * as described above there are 3 mutually exclusive transfer modes:
 * - sender drive
 * - receiver drive
 * - asynchronous
 * these are set up as bit values so they can be ORed together to
 * reflect device capabilities.
 */
enum
{
    kMode_SenderDrive =                     0x10,
    kMode_ReceiverDrive =                   0x20,
    kMode_Asynchronous =                    0x40,
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
enum
{
    kRangeCtl_DefiniteLength =              0x01,
    kRangeCtl_StartOffsetPresent =          0x02,
    kRangeCtl_WideRange =                   0x10,
};

/*
 * status/error codes for BDX
 */
enum
{
    kStatus_NoError =                       0x0000,
    kStatus_Overflow =                      0x0011,
    kStatus_LengthTooLarge =                0x0012,
    kStatus_LengthTooShort =                0x0013,
    kStatus_LengthMismatch =                0x0014,
    kStatus_LengthRequired =                0x0015,
    kStatus_BadMessageContents =            0x0016,
    kStatus_BadBlockCounter =               0x0017,
    kStatus_XferFailedUnknownErr =          0x001F,
    kStatus_ServerBadState =                0x0020,
    kStatus_FailureToSend =                 0x0021,
    kStatus_XferMethodNotSupported =        0x0050,
    kStatus_UnknownFile =                   0x0051,
    kStatus_StartOffsetNotSupported =       0x0052,
    kStatus_VersionNotSupported =           0x0053,
    kStatus_Unknown =                       0x005F,
};

} // namespace WeaveMakeManagedNamespaceIdentifier(BDX, kWeaveManagedNamespaceDesignation_Development)
} // namespace Profiles
} // namespace Weave
} // namespace nl

#endif // _BULK_DATA_TRANSFER_CONSTANTS_H
