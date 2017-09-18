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
 *      the Weave Software Update (SWU) profile used for functional
 *      testing of the implementation of core message handlers for
 *      that profile.
 *
 */

#ifndef SOFTWARE_UPDATE_CLIENT_H_
#define SOFTWARE_UPDATE_CLIENT_H_

#include <Weave/Core/WeaveCore.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Profiles/common/CommonProfile.h>
#include <Weave/Profiles/software-update/SoftwareUpdateProfile.h>
#include <Weave/Profiles/status-report/StatusReportProfile.h>

using namespace ::nl::Weave::Profiles;
using namespace ::nl::Weave::Profiles::Common;
using namespace ::nl::Weave::Profiles::SoftwareUpdate;
using namespace ::nl::Weave::Profiles::StatusReporting;

namespace nl {
namespace Weave {
namespace Profiles {

using namespace ::nl::Inet;
using namespace ::nl::Weave;

class SoftwareUpdateClient
{
public:
	SoftwareUpdateClient();
	~SoftwareUpdateClient();

	WeaveExchangeManager *ExchangeMgr;		// [READ ONLY] Exchange manager object
	const WeaveFabricState *FabricState;	// [READ ONLY] Fabric state object
	uint8_t EncryptionType;                         // Encryption type to use during SWU
	uint16_t KeyId;                                 // Encryption key to use during SWU

	WEAVE_ERROR Init(WeaveExchangeManager *exchangeMgr);
	WEAVE_ERROR Shutdown();

    WEAVE_ERROR SendImageQueryRequest(WeaveConnection *con);
    WEAVE_ERROR SendImageQueryRequest(uint64_t nodeId, IPAddress nodeAddr);
    WEAVE_ERROR SendImageQueryRequest(uint64_t nodeId, IPAddress nodeAddr, uint16_t port);
    WEAVE_ERROR SendImageQueryRequest();

    void SetExchangeCtx(ExchangeContext *ec);			// Set the exchange context for the most recently started SWU exchange.
private:
	ExchangeContext *ExchangeCtx;			// The exchange context for the most recently started SWU exchange.

	static void HandleImageQueryResponse(ExchangeContext *ec, const IPPacketInfo *packetInfo, const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *payload);

	SoftwareUpdateClient(const SoftwareUpdateClient&);   // not defined
};

} // namespace Profiles
} // namespace Weave
} // namespace nl

#endif // SOFTWARE_UPDATE_CLIENT_H_
