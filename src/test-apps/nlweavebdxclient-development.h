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
 *      This file defines an unsolicited initiator (i.e., client) for
 *      the "development" (see [Managed Namespaces](@ref mns)) Weave
 *      Bulk Data Transfer (BDX) profile used for functional testing
 *      of the implementation of core message handlers and protocol
 *      engine for that profile.
 *
 */

#ifndef BULK_DATA_TRANSFER_CLIENT_H_
#define BULK_DATA_TRANSFER_CLIENT_H_

#include <stdio.h>

#include <Weave/Core/WeaveCore.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Profiles/common/CommonProfile.h>
#include <Weave/Profiles/bulk-data-transfer/Development/BulkDataTransfer.h>
#include <Weave/Profiles/status-report/StatusReportProfile.h>

using namespace ::nl::Weave::Profiles;
using namespace ::nl::Weave::Profiles::Common;
using namespace ::nl::Weave::Profiles::BulkDataTransfer;
using namespace ::nl::Weave::Profiles::StatusReporting;

namespace nl {
namespace Weave {
namespace Profiles {

using namespace ::nl::Inet;
using namespace ::nl::Weave;

class BulkDataTransferClient
{
public:
	BulkDataTransferClient();
	~BulkDataTransferClient();

	WeaveExchangeManager *ExchangeMgr;		// [READ ONLY] Exchange manager object
	const WeaveFabricState *FabricState;	// [READ ONLY] Fabric state object
	uint8_t EncryptionType;                         // Encryption type to use when sending an Echo Request
	uint16_t KeyId;                                 // Encryption key to use when sending an Echo Request

	WEAVE_ERROR Init(WeaveExchangeManager *exchangeMgr);
    void setCon(WeaveConnection *con);
	WEAVE_ERROR Shutdown();

    WEAVE_ERROR SendFile(const char *DestFileName, WeaveConnection *con);
    WEAVE_ERROR SendFile(const char *DestFileName, uint64_t nodeId, IPAddress nodeAddr);
    WEAVE_ERROR ReceiveFile(const char *DestFileName, WeaveConnection *con);
    WEAVE_ERROR ReceiveFile(const char *DestFileName, uint64_t nodeId, IPAddress nodeAddr);

    WEAVE_ERROR SendReceiveInitRequest(WeaveConnection *con);
    WEAVE_ERROR SendReceiveInitRequest(uint64_t nodeId, IPAddress nodeAddr);
    WEAVE_ERROR SendReceiveInitRequest(uint64_t nodeId, IPAddress nodeAddr, uint16_t port);

    WEAVE_ERROR SendSendInitRequest(WeaveConnection *con);
    WEAVE_ERROR SendSendInitRequest(uint64_t nodeId, IPAddress nodeAddr);
    WEAVE_ERROR SendSendInitRequest(uint64_t nodeId, IPAddress nodeAddr, uint16_t port);

    WEAVE_ERROR SendBlockQueryRequest(WeaveConnection *con);
    WEAVE_ERROR SendBlockQueryRequest(uint64_t nodeId, IPAddress nodeAddr);
    WEAVE_ERROR SendBlockQueryRequest(uint64_t nodeId, IPAddress nodeAddr, uint16_t port);

    WEAVE_ERROR SendBlockEOFAck(WeaveConnection *con);
    WEAVE_ERROR SendBlockEOFAck(uint64_t nodeId, IPAddress nodeAddr);
    WEAVE_ERROR SendBlockEOFAck(uint64_t nodeId, IPAddress nodeAddr, uint16_t port);

private:
	ExchangeContext *   mExchangeCtx; // The exchange context for the most recently started Echo exchange.
    uint8_t             mBlockCounter;
    uint16_t            mMaxBlockSize; // For the currently open transfer
    WeaveConnection *   mCon;
    FILE *              mDestFile;
    uint8_t             mTransferMode;

	static WEAVE_ERROR HandleReceiveInitResponse(ExchangeContext *ec, const IPPacketInfo *packetInfo, const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *payload);
	static WEAVE_ERROR HandleSendInitResponse(ExchangeContext *ec, const IPPacketInfo *packetInfo, const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *payload);
    static WEAVE_ERROR HandleBlockAck(ExchangeContext *ec, const IPPacketInfo *packetInfo, const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *payload);
    static void HandleBlockEOFAck(ExchangeContext *ec, const IPPacketInfo *packetInfo, const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *payload);
    static void HandleBlockQueryResponse(ExchangeContext *ec, const IPPacketInfo *packetInfo, const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *payload);
    static void HandleBlockQueryRequest(ExchangeContext *ec, const IPPacketInfo *packetInfo, const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *payload);
    static void HandleBlockEOFAckResponse(ExchangeContext *ec, const IPPacketInfo *packetInfo, const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *payload);

    WEAVE_ERROR SendReceiveInitRequest();
    WEAVE_ERROR SendSendInitRequest();
    WEAVE_ERROR SendBlockQueryRequest();
    WEAVE_ERROR SendBlock();
    WEAVE_ERROR SendBlockEOFAck();

    bool IsAsyncMode();
    bool IsSenderDriveMode();
    bool IsReceiverDriveMode();

	BulkDataTransferClient(const BulkDataTransferClient&);   // not defined
};

} // namespace Profiles
} // namespace Weave
} // namespace nl

#endif //BULK_DATA_TRANSFER_CLIENT_H_
