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
 *      This file implements Weave Heartbeat Receiver.
 *
 */

#include "WeaveHeartbeat.h"
#include <Weave/Support/CodeUtils.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace Heartbeat {

WeaveHeartbeatReceiver::WeaveHeartbeatReceiver()
{
    FabricState = NULL;
    ExchangeMgr = NULL;
    OnHeartbeatReceived = NULL;
}

/**
 * Initialize the Weave Heartbeat Receiver and register to receive
 * Weave Heartbeat messages.
 *
 * @param[in] exchangeMgr   A pointer to the system Weave Exchange Manager.
 *
 * @retval #WEAVE_ERROR_INCORRECT_STATE                         If exchange manager is not null
 * @retval #WEAVE_ERROR_TOO_MANY_UNSOLICITED_MESSAGE_HANDLERS   If too many message handlers have
 *                                                              already been registered.
 * @retval #WEAVE_NO_ERROR                                      On success.
 */
WEAVE_ERROR WeaveHeartbeatReceiver::Init(WeaveExchangeManager *exchangeMgr)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(ExchangeMgr == NULL, err = WEAVE_ERROR_INCORRECT_STATE);

    VerifyOrExit(exchangeMgr != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    ExchangeMgr = exchangeMgr;
    FabricState = exchangeMgr->FabricState;
    OnHeartbeatReceived = NULL;

    err = ExchangeMgr->RegisterUnsolicitedMessageHandler(kWeaveProfile_Heartbeat, kHeartbeatMessageType_Heartbeat, HandleHeartbeat, this);

exit:
    return err;
}

/**
 * Shutdown the Weave Heartbeat Receiver and unregister the reception of
 * Weave Heartbeat messages.
 *
 * @retval #WEAVE_NO_ERROR  On success.
 */
WEAVE_ERROR WeaveHeartbeatReceiver::Shutdown()
{
    if (ExchangeMgr != NULL)
    {
        ExchangeMgr->UnregisterUnsolicitedMessageHandler(kWeaveProfile_Heartbeat, kHeartbeatMessageType_Heartbeat);
        ExchangeMgr = NULL;
    }

    FabricState = NULL;

    return WEAVE_NO_ERROR;
}


/**
 * Handle Weave Heartbeat messages when received.
 *
 * @param[in] ec            A pointer to the exchange context of the message.
 * @param[in] pktInfo       A pointer to the IP package info.
 * @param[in] msgHeader     A pointer to Weave message header.
 * @param[in] profileId     32-bit unsigned profile ID.
 * @param[in] msgType       8-bit unsigned message type.
 * @param[in] payload       A pointer to the PacketBuffer of the message payload.
 *
 * @retval #WEAVE_NO_ERROR  On success.
 */
void WeaveHeartbeatReceiver::HandleHeartbeat(ExchangeContext *ec, const IPPacketInfo *pktInfo,
        const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *payload)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveHeartbeatReceiver *receiver;
    uint8_t *p = NULL;
    uint8_t state = 0;

    VerifyOrExit(ec != NULL,        err = WEAVE_ERROR_INVALID_ARGUMENT);
    VerifyOrExit(payload != NULL,   err = WEAVE_ERROR_INVALID_ARGUMENT);

    receiver = static_cast<WeaveHeartbeatReceiver *>(ec->AppState);

    if (receiver->OnHeartbeatReceived != NULL)
    {
        p       = payload->Start();
        state   = nl::Weave::Encoding::Read8(p);

        receiver->OnHeartbeatReceived(msgInfo, state, err);
    }

exit:
    if (payload != NULL)
        PacketBuffer::Free(payload);

    if (ec != NULL)
    {
        ec->Close();
        ec = NULL;
    }
}

} // namespace Heartbeat
} // namespace Profiles
} // namespace Weave
} // namespace nl
