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

#ifndef WEAVE_BDX_SERVER_H_
#define WEAVE_BDX_SERVER_H_

#include <Weave/Core/WeaveCore.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Profiles/common/CommonProfile.h>
#include <Weave/Profiles/bulk-data-transfer/BulkDataTransfer.h>
#include <Weave/Profiles/status-report/StatusReportProfile.h>

// 10 second timeout sometimes expires mid-transfer; 60 sec seems stable (environment: 2nd floor of 900)
#define BDX_RESPONSE_TIMEOUT_SEC 60 
#define BDX_RESPONSE_TIMEOUT_MS BDX_RESPONSE_TIMEOUT_SEC * 1000

#define MAX_NUM_BDX_TRANSFERS 12 // Purely arbitrary, resize to fit application

using namespace ::nl::Weave::Profiles;
using namespace ::nl::Weave::Profiles::Common;
using namespace ::nl::Weave::Profiles::BulkDataTransfer;
using namespace ::nl::Weave::Profiles::StatusReporting;

namespace nl {
namespace Weave {
namespace Profiles {

using namespace ::nl::Inet;
using namespace ::nl::Weave;

class BulkDataTransferServer
{
public:
	BulkDataTransferServer();
	~BulkDataTransferServer();

	WeaveExchangeManager *ExchangeMgr;		// [READ ONLY] Exchange manager object
    void *mpAppState; // passed to application callbacks

	WEAVE_ERROR Init(WeaveExchangeManager *exchangeMgr, void *appState, const char *hostedFileName, const char *receivedFileLocation);
	WEAVE_ERROR Shutdown();

    typedef void (*BDXFunct)(uint64_t nodeId, IPAddress nodeAddr, PacketBuffer *payload, void *appState);
    BDXFunct OnBDXReceiveInitRequestReceived;
    BDXFunct OnBDXSendInitRequestReceived;
    BDXFunct OnBDXBlockQueryRequestReceived;
    BDXFunct OnBDXBlockSendReceived; // also handles BlockEOF
    BDXFunct OnBDXBlockEOFAckReceived;
    typedef void (*BDXCompletedFunct)(uint64_t nodeId, IPAddress nodeAddr, void *appState);
    BDXCompletedFunct OnBDXTransferFailed;
    BDXCompletedFunct OnBDXTransferSucceeded;

private:
    const char *mHostedFileName;
    const char *mReceivedFileLocation;
    typedef struct
    {
        BulkDataTransferServer *BdxApp;
        ExchangeContext *EC;
        int FD;
        uint16_t MaxBlockSize;
        PacketBuffer *BlockBuffer;
        bool CompletedSuccessfully;
    } BDXTransfer;
    BDXTransfer mTransferPool[MAX_NUM_BDX_TRANSFERS];

    /// Get new BDXTransfer from transfer pool if available, or return NULL
    BDXTransfer *NewTransfer();
    /// Shutdown given transfer object and return it to pool
    void ShutdownTransfer(BDXTransfer *xfer, bool closeCon);

    static void HandleReceiveInitRequest(ExchangeContext *ec, const IPPacketInfo *packetInfo, 
            const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *payload);
    static void HandleSendInitRequest(ExchangeContext *ec, const IPPacketInfo *packetInfo,
            const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *payload);
    static void HandleBlockQueryRequest(ExchangeContext *ec, const IPPacketInfo *packetInfo, 
            const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *payload);
    static void HandleBlockSend(ExchangeContext *ec, const IPPacketInfo *packetInfo,
            const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *payload);
    static void HandleBlockEOFAck(ExchangeContext *ec, const IPPacketInfo *packetInfo, 
            const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *payload);
    static void HandleResponseTimeout(ExchangeContext *ec);
    static void HandleBDXConnectionClosed(ExchangeContext *ec, WeaveConnection *con, WEAVE_ERROR conErr);

    /// Send status message to receiver with specified profile ID and status code
    static void SendTransferError(ExchangeContext *ec, uint32_t aProfileId, uint16_t aStatusCode);

    BulkDataTransferServer(const BulkDataTransferServer&);   // not defined
};

} // namespace Profiles
} // namespace Weave
} // namespace nl


#endif // WEAVE_BDX_SERVER_H_
