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
 *      This file implements the Weave Software Update Profile Image
 *      Announce server class and its delegate interface.
 *
 *      This class encapsulates the logic to listen for Weave image
 *      announcements and notify a delegate class when it's time to
 *      send an image query request.
 */

#include "WeaveImageAnnounceServer.h"

namespace nl {
namespace Weave {
namespace Profiles {
namespace SoftwareUpdate {

using namespace ::nl::Weave;
using namespace ::nl::Weave::Profiles;

WeaveImageAnnounceServer::WeaveImageAnnounceServer()
{
    mDelegate = NULL;
}

WEAVE_ERROR WeaveImageAnnounceServer::Init(WeaveExchangeManager * exchangeManager, IWeaveImageAnnounceServerDelegate * delegate)
{
    mDelegate = delegate;

    if (!exchangeManager)
    {
        return WEAVE_ERROR_INCORRECT_STATE;
    }

    return exchangeManager->RegisterUnsolicitedMessageHandler(kWeaveProfile_SWU, kMsgType_ImageAnnounce, HandleImageAnnounce, this);
}

void WeaveImageAnnounceServer::HandleImageAnnounce(ExchangeContext * ec, const IPPacketInfo * packetInfo,
                                                   const WeaveMessageInfo * msgInfo, uint32_t profileId, uint8_t msgType,
                                                   PacketBuffer * imageAnnouncePayload)
{
    PacketBuffer::Free(imageAnnouncePayload);
    WeaveImageAnnounceServer * server = (WeaveImageAnnounceServer *) ec->AppState;

    if (server && server->mDelegate)
    {
        // Delegate responsible for closing exchange context and connection
        server->mDelegate->OnImageAnnounce(ec);
    }
    else
    {
        ec->Close();
        ec = NULL;
    }
}

} // namespace SoftwareUpdate
} // namespace Profiles
} // namespace Weave
} // namespace nl
