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
 *      This file defines the Token Pairing profile, used to pair
 *      authentication tokens.
 *
 */

#ifndef TOKENPAIRING_H_
#define TOKENPAIRING_H_

#include <Weave/Support/NLDLLUtil.h>
#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveServerBase.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Core/WeaveTLV.h>

/**
 *   @namespace nl::Weave::Profiles::TokenPairing
 *
 *   @brief
 *     This namespace includes all interfaces within Weave for the
 *     Weave Authentication Token Pairing profile.
 */

namespace nl {
namespace Weave {
namespace Profiles {
namespace TokenPairing {


/**
 * Message Types for the Token Pairing Profile.
 */
enum
{
    kMsgType_PairTokenRequest                = 1,
    kMsgType_TokenCertificateResponse        = 2,
    kMsgType_TokenPairedResponse             = 3,
    kMsgType_UnpairTokenRequest              = 4
};


/**
 * Data Element Tags for the Token Pairing Profile.
 */
enum
{
    /**
     * Profile-specific Tags
     */
    kTag_TokenPairingBundle                     = 1,    /**< Structure containing an Auth Token Pairing Bundle. */

    /**
     * Context-specific Tags for TokenPairingBundle Structure
     */
    kTag_VendorId                               = 0,    /**< Code identifying product vendor. [ uint, range 1-65535 ] */
    kTag_ProductId                              = 1,    /**< Code identifying product. [ uint, range 1-65535 ] */
    kTag_ProductRevision                        = 2,    /**< Code identifying product revision. [ uint, range 1-65535 ] */
    kTag_SoftwareVersion                        = 3,    /**< Version of software on the device. [ UTF-8 string, len 1-32 ] */
    kTag_DeviceId                               = 4,    /**< Weave device ID. [ uint, 2^64 max ] */
    kTag_PairingToken                           = 5,    /**< Pairing token from the service [ byte string, len 1-128] */
    kTag_TakeIdentityRootKey                    = 6,    /**< TAKE IRK [ Byte String, len 1-16] */
    kTag_EphemeralIdIdentityKey                 = 7,    /**< Ephermeral ID Identity Key [ byte string, len 1-16 ] */
    kTag_TokenCurrentTimeCounterValueInSeconds  = 8,    /**< Token Current time counter (in seconds) [ unit, 2^32 max ] */
    kTag_EphemeralIdRotationPeriodScaler        = 9,    /**< Ephemeral ID rotation period scaler [ unit, 256 max ] */
    kTag_WeaveSignature                         = 10    /**< A Weave signature object (see profiles/security/WeaveSecurity.h) [ structure ] */
};


/**
 *  Contains descriptive information about an Auth Token Pairing Bundle.
 */
enum
{
    kTokenPairing_MaxPairingTokenLength             = 128,  /**< Maximum pairing token length. */
    kTokenPairing_MaxTakeIdentityRootKeyLength      = 16,   /**< Maximum TAKE IRK length. */
    kTokenPairing_MaxEphemeralIdIdentityKeyLength   = 16    /**< Maximum Ephermeral ID Identity Key length. */
};

class TokenPairingDelegate;

/**
 * Server object for responding to Token Pairing requests.
 */
class NL_DLL_EXPORT TokenPairingServer : public WeaveServerBase
{
public:
    TokenPairingServer(void);

    WEAVE_ERROR Init(WeaveExchangeManager *exchangeMgr);
    WEAVE_ERROR Shutdown(void);

    void SetDelegate(TokenPairingDelegate *delegate);

    virtual WEAVE_ERROR SendStatusReport(uint32_t statusProfileId, uint16_t statusCode, WEAVE_ERROR sysError = WEAVE_NO_ERROR);

    WEAVE_ERROR SendTokenCertificateResponse(PacketBuffer *certificate);
    WEAVE_ERROR SendTokenPairedResponse(PacketBuffer *tokenBundle);

protected:
    ExchangeContext *mCurClientOp;
    TokenPairingDelegate *mDelegate;
    bool mCertificateSent;

private:
    void CloseClientOp(void);
    static void HandleClientRequest(ExchangeContext *ec, const IPPacketInfo *pktInfo,
            const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *payload);

    TokenPairingServer(const TokenPairingServer&);   // not defined
};

// Abstract delegate class for implementing incoming Token Pairing operations on the server device.
class TokenPairingDelegate : public WeaveServerDelegateBase
{
public:
    virtual WEAVE_ERROR OnPairTokenRequest(TokenPairingServer *server, uint8_t *pairingToken, uint32_t pairTokenLength) = 0;
    virtual WEAVE_ERROR OnUnpairTokenRequest(TokenPairingServer *server) = 0;

    /**
     * Enforce message-level access control for an incoming Token Pairing request message.
     *
     * @param[in] ec            The ExchangeContext over which the message was received.
     * @param[in] msgProfileId  The profile id of the received message.
     * @param[in] msgType       The message type of the received message.
     * @param[in] msgInfo       A WeaveMessageInfo structure containing information about the received message.
     * @param[inout] result     An enumerated value describing the result of access control policy evaluation for
     *                          the received message. Upon entry to the method, the value represents the tentative
     *                          result at the current point in the evaluation process.  Upon return, the result
     *                          is expected to represent the final assessment of access control policy for the
     *                          message.
     */
    virtual void EnforceAccessControl(ExchangeContext *ec, uint32_t msgProfileId, uint8_t msgType,
                const WeaveMessageInfo *msgInfo, AccessControlResult& result);
};

} // namespace TokenPairing
} // namespace Profiles
} // namespace Weave
} // namespace nl

#endif // TOKENPAIRING_H_
