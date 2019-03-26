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
 *      This file defines the Weave Software Update Profile Image
 *      Announce server and delegate interface.
 *
 */

#ifndef _WEAVE_IMAGE_ANNOUNCE_SERVER_H
#define _WEAVE_IMAGE_ANNOUNCE_SERVER_H

#include <Weave/Core/WeaveCore.h>
#include <Weave/Profiles/ProfileCommon.h>
#include "SoftwareUpdateProfile.h"

namespace nl {
namespace Weave {
namespace Profiles {
namespace SoftwareUpdate {
/// Interface for WeaveImageAnnounceServer delegate
/**
 * Delegates are notified when an image announcement is received.
 * It is their responsbility to free the exchange context and
 * initiate an Image Query request.
 */
class IWeaveImageAnnounceServerDelegate
{
public:
    /// Delegate function called on Image Announce
    /**
     * Called by WeaveImageAnnounceServer when image announcement is received.
     *
     * @param[in] ec context in which Image Announce was received.
     *        Probably still open on sender side, but this is not guaranteed.
     *        Must be closed by delegate.
     */
    virtual void OnImageAnnounce(ExchangeContext * ec) = 0;
};
/// Server that listens for Weave image announcements
/**
 * WeaveImageAnnounce server captures incoming image announcements and
 * notifies its delegate when one has been received.
 */
class WeaveImageAnnounceServer
{
public:
    /// Constructor
    WeaveImageAnnounceServer(void);
    /// Initializer
    /**
     * Initializer function where server registers to receive Image Announce messages.
     *
     * @param exchangeManager initialized WeaveExchangeManager with which server
     *        registers to receive Image Announce messages, must not be null
     * @param delegate delegate, may be null
     * @returns WEAVE_NO_ERROR on success, descriptive WEAVE_ERROR value otherwise
     */
    WEAVE_ERROR Init(WeaveExchangeManager * exchangeManager, IWeaveImageAnnounceServerDelegate * delegate);
    /// Delegate setter
    void SetDelegate(IWeaveImageAnnounceServerDelegate * delegate);

private:
    /// Handler for Weave image announcements
    static void HandleImageAnnounce(ExchangeContext * ec, const IPPacketInfo * packetInfo, const WeaveMessageInfo * msgInfo,
                                    uint32_t profileId, uint8_t msgType, PacketBuffer * imageAnnouncePayload);
    /// Delegate called on image announce
    IWeaveImageAnnounceServerDelegate * mDelegate;
};
} // namespace SoftwareUpdate
} // namespace Profiles
} // namespace Weave
} // namespace nl
#endif // _WEAVE_IMAGE_ANNOUNCE_SERVER_H
