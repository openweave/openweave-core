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
 *      This file implements the Dropcam Legacy Pairing Profile, which provides
 *      operations and utilities used to pair both in-field and out-of-box
 *      Nest Cam devices with Weave via the camera cloud service.
 */

#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Support/CodeUtils.h>
#include <Weave/Core/WeaveEncoding.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include "DropcamLegacyPairing.h"
#include <Weave/Profiles/common/CommonProfile.h>
#include <Weave/Support/logging/WeaveLogging.h>
#include <Weave/Support/ProfileStringSupport.hpp>

/**
 *  @def REQUIRE_AUTH_DROPCAM_LEGACY_PAIRING
 *
 *  @brief
 *    Enable (1) or disable (0) support for client requests to the
 *    Dropcam Legacy Pairing server via an authenticated session. See
 *    also #WEAVE_CONFIG_REQUIRE_AUTH in WeaveConfig.h.
 *
 *    @note This preprocessor define shall be deasserted for development
 *          and testing purposes only. No Weave-enabled device shall
 *          be certified without this asserted.
 *
 */
#define REQUIRE_AUTH_DROPCAM_LEGACY_PAIRING    1

namespace nl {
namespace Weave {
namespace Profiles {
namespace Vendor {
namespace Nestlabs {
namespace DropcamLegacyPairing {

using namespace nl::Weave::Crypto;
using namespace nl::Weave::Encoding;
using namespace nl::Weave::TLV;

// Preprocessor Macros

#define kWeave_VendorNameString_Nest "Nest"
#define kWeave_ProfileNameString_DropcamLegacyPairing \
    kWeave_VendorNameString_Nest ":DropcamLegacyPairing"

// Forward Declarations

static const char *GetDropcamLegacyPairingMessageName(uint32_t inProfileId, uint8_t inMsgType);
static const char *GetDropcamLegacyPairingProfileName(uint32_t inProfileId);

static void _DropcamLegacyPairingProfileStringInit(void) __attribute__((constructor));
static void _DropcamLegacyPairingProfileStringDestroy(void) __attribute__((destructor));

// Globals

/**
 *  This structure provides storage for callbacks associated for
 *  returning human-readable support strings associated with the
 *  profile.
 */
static const Weave::Support::ProfileStringInfo sDropcamLegacyPairingProfileStringInfo = {
    kWeaveProfile_DropcamLegacyPairing,

    GetDropcamLegacyPairingMessageName,
    GetDropcamLegacyPairingProfileName,
    NULL
};

/**
 *  Context for registering and deregistering callbacks associated
 *  with for returning human-readable support strings associated with
 *  the profile.
 */
static Weave::Support::ProfileStringContext sDropcamLegacyPairingProfileStringContext = {
    NULL,
    sDropcamLegacyPairingProfileStringInfo
};

/**
 *  One time, yet reentrant, initializer for registering Weave Dropcam
 *  Legacy Pairing profile callbacks for returning human-readable
 *  support strings associated with the profile.
 */
static void _DropcamLegacyPairingProfileStringInit(void)
{
    (void)Weave::Support::RegisterProfileStringInfo(sDropcamLegacyPairingProfileStringContext);
}

/**
 *  One time, yet reentrant, deinitializer for unregistering Weave
 *  Dropcam Legacy Pairing profile callbacks for returning
 *  human-readable support strings associated with the profile.
 */
static void _DropcamLegacyPairingProfileStringDestroy(void)
{
    (void)Weave::Support::UnregisterProfileStringInfo(sDropcamLegacyPairingProfileStringContext);
}

/**
 *  @brief
 *    Callback function that returns a human-readable NULL-terminated
 *    C string describing the message type associated with this
 *    profile.
 *
 *  This callback, when registered, is invoked when a human-readable
 *  NULL-terminated C string is needed to describe the message type
 *  associated with this profile.
 *
 *  @param[in]  inProfileId  The profile identifier associated with the
 *                           specified message type.
 *
 *  @param[in]  inMsgType    The message type for which a human-readable
 *                           descriptive string is sought.
 *
 *  @return a pointer to the NULL-terminated C string if a match is
 *  found; otherwise, NULL.
 *
 */
static const char *GetDropcamLegacyPairingMessageName(uint32_t inProfileId, uint8_t inMsgType)
{
    const char *result = NULL;

    switch (inProfileId) {

    case kWeaveProfile_DropcamLegacyPairing:
        switch (inMsgType) {

        case DropcamLegacyPairing::kMsgType_CameraAuthDataRequest:
            result = "CameraAuthDataRequest";
            break;

        case DropcamLegacyPairing::kMsgType_CameraAuthDataResponse:
            result = "CameraAuthDataResponse";
            break;
        }
        break;
    }

    return (result);
}

/**
 *  @brief
 *    Callback function that returns a human-readable NULL-terminated
 *    C string describing the profile with this profile.
 *
 *  This callback, when registered, is invoked when a human-readable
 *  NULL-terminated C string is needed to describe this profile.
 *
 *  @param[in]  inProfileId  The profile identifier for which a human-readable
 *                           descriptive string is sought.
 *
 *  @return a pointer to the NULL-terminated C string if a match is
 *  found; otherwise, NULL.
 *
 */
static const char *GetDropcamLegacyPairingProfileName(uint32_t inProfileId)
{
    const char *result = NULL;

    switch (inProfileId)
    {
    case kWeaveProfile_DropcamLegacyPairing:
        result = kWeave_ProfileNameString_DropcamLegacyPairing;
        break;
    }

    return (result);
}

/**
 * Utility function to encode CameraAuthDataRequest message payload
 *
 * @param[in]    buf    A pointer to the Camera Auth Data Request message payload buffer.
 *
 * @param[in]    nonce  A pointer to the camera pairing nonce, formatted as a NULL-terminated UTF-8 string.
 *
 * @retval  #WEAVE_NO_ERROR                                     On success.
 * @retval other            Other Weave or platform-specific error codes indicating that an error
 *                          occurred preventing encoding of the message payload.
 */
WEAVE_ERROR EncodeCameraAuthDataRequest(PacketBuffer *buf, const char *nonce)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TLVWriter writer;

    writer.Init(buf, buf->MaxDataLength());

    err = writer.PutString(AnonymousTag, nonce);
    SuccessOrExit(err);

    err = writer.Finalize();
    SuccessOrExit(err);

exit:
    return err;
}

/**
 * Utility function to decode CameraAuthDataResponse message payload
 *
 * @param[in]    buf    A pointer to the Camera Auth Data Response message payload buffer.
 *
 * @param[in]    macAddress    A byte array buffer for the camera's EUI-48 WiFi MAC address.
 *
 * @param[in]    hmac          A reference to the provided HMAC return buffer. HMAC returned as raw byte
 *                             array which may contain non-ASCII/Unicode characters.
 *
 * @retval  #WEAVE_NO_ERROR    On success.
 * @retval other            Other Weave or platform-specific error codes indicating that an error
 *                          occurred preventing decoding of the message payload.
 */
WEAVE_ERROR DecodeCameraAuthDataResponse(PacketBuffer *buf, uint8_t (&macAddress)[EUI48_LEN], uint8_t (&hmac)[HMAC_BUF_LEN])
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TLVReader reader;

    reader.Init(buf);

    err = reader.Next();
    SuccessOrExit(err);

    err = reader.GetBytes(macAddress, EUI48_LEN);
    SuccessOrExit(err);

    err = reader.Next();
    SuccessOrExit(err);

    err = reader.GetBytes(hmac, HMAC_BUF_LEN);
    SuccessOrExit(err);

exit:
    return err;
}

/**
 * Null-initialize the Dropcam Legacy Pairing Server. Must call Init() prior to use.
 */
DropcamLegacyPairingServer::DropcamLegacyPairingServer(void)
{
    FabricState = NULL;
    ExchangeMgr = NULL;
    mDelegate = NULL;
}

/**
 * Initialize the Dropcam Legacy Pairing Server state and register to receive
 * Dropcam Legacy Pairing messages.
 *
 * @param[in]    exchangeMgr    A pointer to the Weave Exchange Manager.
 *
 * @retval  #WEAVE_ERROR_TOO_MANY_UNSOLICITED_MESSAGE_HANDLERS  When too many unsolicited message
 *                                                              handlers are registered.
 * @retval  #WEAVE_NO_ERROR                                     On success.
 */
WEAVE_ERROR DropcamLegacyPairingServer::Init(WeaveExchangeManager *exchangeMgr)
{
    FabricState = exchangeMgr->FabricState;
    ExchangeMgr = exchangeMgr;

    // Register to receive unsolicited Dropcam Legacy Pairing messages from the exchange manager.
    WEAVE_ERROR err =
        ExchangeMgr->RegisterUnsolicitedMessageHandler(kWeaveProfile_DropcamLegacyPairing, HandleClientRequest, this);

    return err;
}

/**
 * Shutdown the Dropcam Legacy Pairing Server.
 *
 * @retval #WEAVE_NO_ERROR unconditionally.
 */
WEAVE_ERROR DropcamLegacyPairingServer::Shutdown(void)
{
    if (ExchangeMgr != NULL)
        ExchangeMgr->UnregisterUnsolicitedMessageHandler(kWeaveProfile_DropcamLegacyPairing);

    FabricState = NULL;
    ExchangeMgr = NULL;

    return WEAVE_NO_ERROR;
}

/**
 * Set the delegate to process Dropcam Legacy Pairing Server events.
 *
 * @param[in]   delegate    A pointer to the Dropcam Legacy Pairing Delegate.
 */
void DropcamLegacyPairingServer::SetDelegate(DropcamLegacyPairingDelegate *delegate)
{
    mDelegate = delegate;
}

void DropcamLegacyPairingServer::HandleClientRequest(ExchangeContext *ec, const IPPacketInfo *pktInfo,
    const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *msgBuf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    DropcamLegacyPairingServer *server = reinterpret_cast<DropcamLegacyPairingServer *>(ec->AppState);

    // Fail messages for the wrong profile. This shouldn't happen, but better safe than sorry.
    if (profileId != kWeaveProfile_DropcamLegacyPairing)
    {
        WeaveServerBase::SendStatusReport(ec, kWeaveProfile_Common, Common::kStatus_BadRequest, WEAVE_NO_ERROR);
        ec->Close();
        ExitNow();
    }

    // Call on the delegate to enforce message-level access control.  If policy dictates the message should NOT
    // be processed, then simply end the exchange and return.  If an error response was warranted, the appropriate
    // response will have been sent within EnforceAccessControl().
    if (!server->EnforceAccessControl(ec, profileId, msgType, msgInfo, server->mDelegate))
    {
        ec->Close();
        ExitNow();
    }

    // Decode and dispatch the message.
    switch (msgType)
    {
    case kMsgType_CameraAuthDataRequest:
        err = server->HandleCameraAuthDataRequest(ec, msgBuf);
        SuccessOrExit(err);
        break;

    default:
        WeaveServerBase::SendStatusReport(ec, kWeaveProfile_Common, Common::kStatus_BadRequest, WEAVE_NO_ERROR);
        ExitNow();
        break;
    }

exit:
    if (msgBuf != NULL)
    {
        PacketBuffer::Free(msgBuf);
    }

    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogError(DropcamLegacyPairing, "Error handling DropcamLegacyPairing client request, err = %d\n", err);
        WeaveServerBase::SendStatusReport(ec, kWeaveProfile_Common, Common::kStatus_InternalError, err);
    }

	if (NULL != ec)
    {
        ec->Close();
    }
}

WEAVE_ERROR DropcamLegacyPairingServer::HandleCameraAuthDataRequest(ExchangeContext *ec, PacketBuffer *(&msgBuf))
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    HMACSHA256 hmacObj;
    const uint8_t authDataMessageLen = EUI48_LEN + CAMERA_NONCE_LEN;
    uint8_t authDataMessage[authDataMessageLen];
    uint8_t hmac[HMAC_BUF_LEN];
    uint8_t macAddress[EUI48_LEN];
    uint8_t secret[CAMERA_SECRET_LEN];
    const uint8_t *noncePtr;
    TLVReader reader;
    TLVWriter writer;

    VerifyOrExit(NULL != mDelegate, err = WEAVE_ERROR_INCORRECT_STATE);

    memset(hmac, 0, HMAC_BUF_LEN);

    // Decode request
    reader.Init(msgBuf);

    err = reader.Next();
    SuccessOrExit(err);

    VerifyOrExit(reader.GetType() == kTLVType_UTF8String, err = WEAVE_ERROR_UNEXPECTED_TLV_ELEMENT);
    err = reader.GetDataPtr(noncePtr);
    SuccessOrExit(err);

    // Get camera MAC address
    err = mDelegate->GetCameraMACAddress(macAddress);
    SuccessOrExit(err);

    // Get camera secret
    err = mDelegate->GetCameraSecret(secret);
    SuccessOrExit(err);

    // Calculate HMAC of camera secret and auth_data message
    memcpy(authDataMessage, macAddress, EUI48_LEN);
    memcpy(&authDataMessage[EUI48_LEN], noncePtr, CAMERA_NONCE_LEN);

    hmacObj.Begin(secret, CAMERA_SECRET_LEN);
    hmacObj.AddData(authDataMessage, authDataMessageLen);
    hmacObj.Finish(hmac);

    // Re-use request buffer
    PacketBuffer::Free(msgBuf);
    msgBuf = PacketBuffer::New();
    VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    // Encode response
    writer.Init(msgBuf, msgBuf->MaxDataLength());

    err = writer.PutBytes(AnonymousTag, macAddress, EUI48_LEN);
    SuccessOrExit(err);

    err = writer.PutBytes(AnonymousTag, hmac, HMAC_BUF_LEN);
    SuccessOrExit(err);

    err = writer.Finalize();
    SuccessOrExit(err);

    // Send MAC address and pairing data HMAC to client
    err = ec->SendMessage(kWeaveProfile_DropcamLegacyPairing, kMsgType_CameraAuthDataResponse, msgBuf, 0);
    SuccessOrExit(err);
    msgBuf = NULL;

exit:
    return err;
}

void DropcamLegacyPairingDelegate::EnforceAccessControl(ExchangeContext *ec, uint32_t msgProfileId, uint8_t msgType,
        const WeaveMessageInfo *msgInfo, AccessControlResult& result)
{
    // If the result has not already been determined by a subclass...
    if (result == kAccessControlResult_NotDetermined)
    {
        switch (msgType)
        {
        case kMsgType_CameraAuthDataRequest:
#if WEAVE_CONFIG_REQUIRE_AUTH_DROPCAM_LEGACY_PAIRING
            if (msgInfo->PeerAuthMode == kWeaveAuthMode_PASE_PairingCode)
            {
                result = kAccessControlResult_Accepted;
            }
#else // WEAVE_CONFIG_REQUIRE_AUTH_DROPCAM_LEGACY_PAIRING
            result = kAccessControlResult_Accepted;
#endif // WEAVE_CONFIG_REQUIRE_AUTH_DROPCAM_LEGACY_PAIRING
            break;

        default:
            WeaveServerBase::SendStatusReport(ec, kWeaveProfile_Common, Common::kStatus_UnsupportedMessage, WEAVE_NO_ERROR);
            result = kAccessControlResult_Rejected_RespSent;
            break;
        }
    }

    // Call up to the base class.
    WeaveServerDelegateBase::EnforceAccessControl(ec, msgProfileId, msgType, msgInfo, result);
}

} // namespace DropcamLegacyPairing
} // namespace Nestlabs
} // namespace Vendor
} // namespace Profiles
} // namespace Weave
} // namespace nl
