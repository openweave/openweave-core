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

#include <Weave/Profiles/bulk-data-transfer/Development/BulkDataTransfer.h>
#include <Weave/Profiles/common/CommonProfile.h>
#include <Weave/Profiles/WeaveProfiles.h>

// 10 second timeout sometimes expires mid-transfer; 60 sec seems stable (environment: 2nd floor of 900)
#define BDX_RESPONSE_TIMEOUT_SEC 60
#define BDX_RESPONSE_TIMEOUT_MS BDX_RESPONSE_TIMEOUT_SEC * 1000

#define MAX_NUM_BDX_TRANSFERS 12 // Purely arbitrary, resize to fit application

#define TEMP_FILE_LOCATION "/tmp/"

using namespace ::nl::Weave::Profiles;
using namespace ::nl::Weave::Profiles::Common;
using namespace ::nl::Weave::Profiles::BulkDataTransfer;
using namespace ::nl::Weave::Profiles::StatusReporting;

namespace nl {
namespace Weave {
namespace Profiles {

using namespace ::nl::Inet;
using namespace ::nl::Weave;

class BulkDataTransferServerDelegate
{
    //TODO: actually implement
public:
    virtual bool AllowBDXServerToRun();
};

class BulkDataTransferServer
{
public:
	BulkDataTransferServer();
	~BulkDataTransferServer();

	WeaveExchangeManager *mpExchangeMgr; // [READ ONLY] Exchange manager object
    void *mpAppState; // passed to application callbacks, set in Init(), currently unused

	WEAVE_ERROR Init(WeaveExchangeManager *exchangeMgr, void *appState);
	WEAVE_ERROR Shutdown();

    /** Callback functions that will be fired, if set, during the appropriate event. */
    typedef void (*BDXFunct)(uint64_t nodeId, IPAddress nodeAddr, PacketBuffer *payload, void *appState);
    BDXFunct OnBDXReceiveInitRequestReceived;
    BDXFunct OnBDXSendInitRequestReceived;
    BDXFunct OnBDXBlockQueryRequestReceived;
    BDXFunct OnBDXBlockSendReceived; // also handles BlockEOF
    BDXFunct OnBDXBlockEOFAckReceived;
    typedef void (*BDXCompletedFunct)(uint64_t nodeId, IPAddress nodeAddr, void *appState);
    BDXCompletedFunct OnBDXTransferFailed;
    BDXCompletedFunct OnBDXTransferSucceeded;

    void AllowBDXServerToRun(bool aEnable);
    bool CanBDXServerRun();

    void SetDelegate(BulkDataTransferServerDelegate *pDelegate);
    BulkDataTransferServerDelegate *GetDelegate();

private:
    BulkDataTransferServerDelegate *mpDelegate;

    //TODO: move this to the transfer?  shouldn't a server be able to serve multiple files?
    //currently it isn't even being used
    char *mHostedFileName;
    typedef struct
    {
        BulkDataTransferServer *BdxApp;
        ExchangeContext *EC;
        FILE * FD;
        //TODO when we actually use it: uint16_t mBlockCounter;
        uint8_t mTransferMode;
        //TODO: bool mAckRcvd; ???
        uint16_t mMaxBlockSize;
        PacketBuffer *BlockBuffer;
        bool CompletedSuccessfully;
        //TODO: int mSendFlags; ????
    } BDXTransfer;

    bool mBDXDownloadCanRun;

    BDXTransfer mTransferPool[MAX_NUM_BDX_TRANSFERS];

    /// Get new BDXTransfer from transfer pool if available, or return NULL
    BDXTransfer *NewTransfer();
    /// Shutdown given transfer object and return it to pool
    void ShutdownTransfer(BDXTransfer *xfer);

    // These static functions will actually be set as the handlers for the various messages.
    // Note that some handlers will actually set the next handler to be used based on the
    // expected response, so carefully error check that you receive an appropriate message type.
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
    static void HandleBDXConnectionClosed(WeaveConnection *con, WEAVE_ERROR conErr);
    //TODO:
    //    static void HandleDDRcvd(uint64_t nodeId, uint32_t pauseTime);
    //    static void HandleThrottleRcvd(uint64_t nodeId, uint32_t pauseTime);
    //    static void HandleAckRcvd(void *ctxt);


    /// Send status message to receiver with specified profile ID and status code
    static void SendTransferError(ExchangeContext *ec, uint32_t aProfileId, uint16_t aStatusCode);

    BulkDataTransferServer(const BulkDataTransferServer&);   // not defined
};

} // namespace Profiles
} // namespace Weave
} // namespace nl


#endif // WEAVE_BDX_SERVER_H_
