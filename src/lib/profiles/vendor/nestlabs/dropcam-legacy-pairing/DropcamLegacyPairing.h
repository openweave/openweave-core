/*
 *
 *    Copyright (c) 2016 Nest Labs, Inc.
 *    All rights reserved.
 *
 *    This document is the property of Nest. It is considered
 *    confidential and proprietary information.
 *
 *    This document may not be reproduced or transmitted in any form,
 *    in whole or in part, without the express written permission of
 *    Nest.
 *
 */

/**
 *    @file
 *      This file defines the Dropcam Legacy Pairing Profile, which provides
 *      operations and utilities used to pair both in-field and out-of-box
 *      Nest Cam devices with Weave via the camera cloud service.
 */

#ifndef DROPCAMLEGACYPAIRING_H_
#define DROPCAMLEGACYPAIRING_H_

#include <Weave/Support/NLDLLUtil.h>
#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveServerBase.h>
#include <Weave/Core/WeaveEncoding.h>
#include <Weave/Core/WeaveTLV.h>

/**
 *   @namespace nl::Weave::Profiles::DropcamLegacyPairing
 *
 *   @brief
 *     This namespace includes all interfaces within Weave for the
 *     Weave Dropcam Legacy Pairing profile.
 */

namespace nl {
namespace Weave {
namespace Profiles {
namespace Vendor {
namespace Nestlabs {
namespace DropcamLegacyPairing {

using nl::Inet::IPAddress;

/**
 * Length in bytes of EUI-48 raw bytes representation
 */
const uint8_t EUI48_LEN = 6;

/**
 * Length in bytes of EUI-48 represented as string of 12 hex digits sans colon separators, plus NULL terminator
 */
const uint8_t EUI48_STR_LEN = 13;

/**
 * Length of camera secret in bytes
 */
const uint8_t CAMERA_SECRET_LEN = 32;

/**
 * Length in bytes of camera nonce string, excluding NULL terminator
 */
const uint8_t CAMERA_NONCE_LEN = 64;

/**
 * Number of camera auth data HMAC bytes appended to auth_data API parameter
 */
const uint8_t CAMERA_HMAC_LEN = 4;

/**
 * Length of binary camera auth data parameter before base64 string conversion
 */
const uint8_t CAMERA_AUTH_DATA_LEN = (EUI48_LEN + CAMERA_NONCE_LEN + CAMERA_HMAC_LEN);

/**
 * Constant for length in bytes of camera-generated pairing info HMAC, represented as raw bytes
 */
const uint8_t HMAC_BUF_LEN = 32;

/**
 * Dropcam Legacy Pairing Message Types
 */
enum
{
    kMsgType_CameraAuthDataRequest               = 1,    /**< Retrieve parameters for legacy Dropcam pairing web API call. */
    kMsgType_CameraAuthDataResponse              = 2     /**< Contains parameters for legacy Dropcam pairing web API call. */
};

/**
 * Utility functions to encode and decode Dropcam Legacy Pairing profile message payloads
 */
WEAVE_ERROR EncodeCameraAuthDataRequest(PacketBuffer *buf, const char *nonce);
WEAVE_ERROR DecodeCameraAuthDataResponse(PacketBuffer *buf, uint8_t (&macAddress)[EUI48_LEN], uint8_t (&hmac)[HMAC_BUF_LEN]);

/**
 * Delegate class for implementing incoming Dropcam Legacy Pairing operations on the server device.
 */
class DropcamLegacyPairingDelegate : public WeaveServerDelegateBase
{
public:
    /**
     * Retrieve camera's 32-byte secret, shared with the service and used to generate auth_data HMAC.
     *
     * @param[in]   secret      Reference to CAMERA_SECRET_LEN-byte buffer for camera secret
     *
     * @retval #WEAVE_NO_ERROR  On success.
     * @retval other            Other Weave or platform-specific error codes indicating that an error
     *                          occurred preventing generation of the Dropcam API parameters.
     */
    virtual WEAVE_ERROR GetCameraSecret(uint8_t (&secret)[CAMERA_SECRET_LEN]) = 0;

     /**
     * Retrieve camera's EUI-48 WiFi MAC address.
     *
     * @param[in]   hashString  Reference to buffer for the returned MAC address, represented as NULL-terminated string of
     *                          hex values without separators.
     *
     * @retval #WEAVE_NO_ERROR  On success.
     * @retval other            Other Weave or platform-specific error codes indicating that an error
     *                          occurred preventing generation of the Dropcam API parameters.
     */
    virtual WEAVE_ERROR GetCameraMACAddress(uint8_t (&macAddress)[EUI48_LEN]) = 0;

    /**
     * Enforce message-level access control for an incoming Dropcam Legacy Pairing request message.
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

/**
 * Server class for implementing the Dropcam Legacy Pairing profile.
 */
class NL_DLL_EXPORT DropcamLegacyPairingServer : public WeaveServerBase
{
public:
    DropcamLegacyPairingServer(void);

    WEAVE_ERROR Init(WeaveExchangeManager *exchangeMgr);
    WEAVE_ERROR Shutdown(void);

    void SetDelegate(DropcamLegacyPairingDelegate *delegate);

protected:
    DropcamLegacyPairingDelegate *mDelegate;

// ----- Weave callbacks -----

    // Message received
    static void HandleClientRequest(ExchangeContext *ec, const IPPacketInfo *pktInfo,
            const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *payload);
    WEAVE_ERROR HandleCameraAuthDataRequest(ExchangeContext *ec, PacketBuffer *(&msgBuf));

    DropcamLegacyPairingServer(const DropcamLegacyPairingServer&);   // not defined
};


} // namespace DropcamLegacyPairing
} // namespace Nestlabs
} // namespace Vendor
} // namespace Profiles
} // namespace Weave
} // namespace nl


#endif /* DROPCAMLEGACYPAIRING_H_ */
