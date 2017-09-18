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
 *      This file defines objects for a Weave Echo unsolicitied
 *      initaitor (client) and responder (server).
 *
 */

#ifndef WEAVE_ECHO_H_
#define WEAVE_ECHO_H_

#include <Weave/Support/NLDLLUtil.h>
#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveServerBase.h>
#include <Weave/Profiles/WeaveProfiles.h>

namespace nl {
namespace Weave {
namespace Profiles {

using namespace ::nl::Inet;
using namespace ::nl::Weave;

enum
{
	kEchoMessageType_EchoRequest			= 1,
	kEchoMessageType_EchoResponse			= 2
};

class NL_DLL_EXPORT WeaveEchoClient
{
public:
	WeaveEchoClient(void);

	const WeaveFabricState *FabricState;	// [READ ONLY] Fabric state object
	WeaveExchangeManager *ExchangeMgr;		// [READ ONLY] Exchange manager object
	uint8_t EncryptionType;                         // Encryption type to use when sending an Echo Request
	uint16_t KeyId;                                 // Encryption key to use when sending an Echo Request

	WEAVE_ERROR Init(WeaveExchangeManager *exchangeMgr);
	WEAVE_ERROR Shutdown(void);

	WEAVE_ERROR SendEchoRequest(WeaveConnection *con, PacketBuffer *payload);
    WEAVE_ERROR SendEchoRequest(uint64_t nodeId, IPAddress nodeAddr, PacketBuffer *payload);
	WEAVE_ERROR SendEchoRequest(uint64_t nodeId, IPAddress nodeAddr, uint16_t port, InterfaceId sendIntfId, PacketBuffer *payload);

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
	void SetRequestAck(bool requestAck);
	void SetWRMPACKDelay(uint16_t aWRMPACKDelay);
	void SetWRMPRetransInterval(uint32_t aRetransInterval);
	void SetWRMPRetransCount(uint8_t aRetransCount);
	typedef void (*EchoAckFunct)(void *msgCtxt);
	EchoAckFunct OnAckRcvdReceived;
#endif // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING

	typedef void (*EchoFunct)(uint64_t nodeId, IPAddress nodeAddr, PacketBuffer *payload);
	EchoFunct OnEchoResponseReceived;

private:
	ExchangeContext *ExchangeCtx;			// The exchange context for the most recently started Echo exchange.
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
	bool RequestAck;
        bool AckReceived;
        bool ResponseReceived;
	uint16_t WRMPACKDelay;
	uint32_t WRMPRetransInterval;
	uint8_t WRMPRetransCount;
	uint32_t appContext;
	static void HandleAckRcvd(ExchangeContext *ec, void *msgCtxt);
#endif // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
	static void HandleError(ExchangeContext *ec, WEAVE_ERROR err);
	static void HandleConnectionClosed(ExchangeContext *ec, WeaveConnection *con, WEAVE_ERROR conErr);
	static void HandleSendError(ExchangeContext *ec, WEAVE_ERROR sendErr, void *msgCtxt);
	static void HandleKeyError(ExchangeContext *ec, WEAVE_ERROR keyErr);

	WEAVE_ERROR SendEchoRequest(PacketBuffer *payload);
	static void HandleResponse(ExchangeContext *ec, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *payload);
	WeaveEchoClient(const WeaveEchoClient&);   // not defined
};


class NL_DLL_EXPORT WeaveEchoServer : public WeaveServerBase
{
public:
	WeaveEchoServer(void);

	WEAVE_ERROR Init(WeaveExchangeManager *exchangeMgr);
	WEAVE_ERROR Shutdown(void);

	typedef void (*EchoFunct)(uint64_t nodeId, IPAddress nodeAddr, PacketBuffer *payload);
	EchoFunct OnEchoRequestReceived;

private:
	static void HandleEchoRequest(ExchangeContext *ec, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *payload);

	WeaveEchoServer(const WeaveEchoServer&);   // not defined
};

} // namespace Profiles
} // namespace Weave
} // namespace nl


#endif // WEAVE_ECHO_H_
