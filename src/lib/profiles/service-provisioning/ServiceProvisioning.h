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
 *      This file defines data types and objects for a Weave Service
 *      Provisioning profile unsolicited initiator (client) and
 *      responder (server).
 *
 */

#ifndef SERVICEPROVISIONING_H_
#define SERVICEPROVISIONING_H_

#include <Weave/Support/NLDLLUtil.h>
#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveServerBase.h>
#include <Weave/Core/WeaveEncoding.h>
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Profiles/security/WeaveCert.h>

/**
 *   @namespace nl::Weave::Profiles::ServiceProvisioning
 *
 *   @brief
 *     This namespace includes all interfaces within Weave for the
 *     Weave Service Provisioning profile, the third of the three
 *     Weave provisioning profiles.
 */

namespace nl {
namespace Weave {
namespace Profiles {
namespace ServiceProvisioning {

using nl::Inet::IPAddress;

/**
 *  Service Provisioning Status Codes
 */
enum
{
    kStatusCode_TooManyServices                 = 1,    /**< There are too many services registered on the device. */
    kStatusCode_ServiceAlreadyRegistered        = 2,    /**< The specified service is already registered on the device. */
    kStatusCode_InvalidServiceConfig            = 3,    /**< The specified service configuration is invalid. */
    kStatusCode_NoSuchService                   = 4,    /**< The specified id does not match a service registered on the device. */
    kStatusCode_PairingServerError              = 5,    /**< The device could not complete service pairing because it failed to talk to the pairing server. */
    kStatusCode_InvalidPairingToken             = 6,    /**< The device could not complete service pairing because it passed an invalid pairing token. */
    kStatusCode_PairingTokenOld                 = 7,    /**< The device could not complete service pairing because the pairing token it passed has expired. */
    kStatusCode_ServiceCommuncationError        = 8,    /**< The device could not complete service pairing because it encountered an error when communicating with the service. */
    kStatusCode_ServiceConfigTooLarge           = 9,    /**< The specified service configuration is too large. */
    kStatusCode_WrongFabric                     = 10,   /**< Device paired with a different fabric. */
    kStatusCode_TooManyFabrics                  = 11    /**< Too many fabrics in the structure. */

    // !!!!! IMPORTANT !!!!!  If you add new Service Provisioning status codes, you must coordinate this with the service team.
    // The service runs a separate implementation of the Weave protocol, so it does not automatically pick up undocumented or
    // uncommunicated changes to status codes in the devices' Weave stack.
};

/**
 *  Service Provisioning Message Types
 */
enum
{
    // Application/Device Messages
    kMsgType_RegisterServicePairAccount         = 1,
    kMsgType_UpdateService                      = 2,
    kMsgType_UnregisterService                  = 3,

    // Device/Service Messages
    kMsgType_UnpairDeviceFromAccount            = 101,
    kMsgType_PairDeviceToAccount                = 102,
    kMsgType_IFJServiceFabricJoin               = 103
};

/**
 * Service Provisioning Data Element Tags
 */
enum
{
    // ---- Top-level Data Elements ----
    kTag_ServiceConfig                          = 1,    /**< [ structure ] Describes a Weave Service. */
    kTag_ServiceEndPoint                        = 2,    /**< [ structure ] Describes a Weave Service EndPoint. */

    // ---- Context-specific Tags for ServiceConfig Structure ----
    kTag_ServiceConfig_CACerts                  = 1,    /**< [ array, length >= 1 ] List of trusted CA certificates for service.
                                                             Each element is a WeaveCertificate, as defined in the Security Profile. */
    kTag_ServiceConfig_DirectoryEndPoint        = 2,    /**< [ structure ] Contains contact information for the service's primary directory end point.
                                                             Contents are as defined below for ServiceEndPoint structure. */

    // ---- Context-specific Tags for ServiceEndPoint Structure ----
    kTag_ServiceEndPoint_Id                     = 1,    /**< [ uint, 8-64 bits ] Service end point id (an EUI-64) assigned to the service end point. */
    kTag_ServiceEndPoint_Addresses              = 2,    /**< [ array, length >= 1 ] List of addresses for the service end point.
                                                             Each element is a ServiceEndPointAddress structure, as defined below. */
    kTag_ServiceEndPoint_NodeId                 = 3,    /**< [ uint, 8-64 bits ] Weave node id of the node providing the service.
                                                             Mutually exclusive with Addresses list. */

    // ---- Context-specific Tags for ServiceEndPointAddress Structure ----
    kTag_ServiceEndPointAddress_HostName        = 1,    /**< [ utf-8 string ] Host name or literal IP address. */
    kTag_ServiceEndPointAddress_Port            = 2     /**< [ uint, 1-63353 ] IP port number. Optional */
};


class NL_DLL_EXPORT RegisterServicePairAccountMessage
{
public:
    uint64_t ServiceId;
    const char *AccountId;
    uint16_t AccountIdLen;
    const uint8_t *ServiceConfig;
    uint16_t ServiceConfigLen;
    const uint8_t *PairingToken;
    uint16_t PairingTokenLen;
    const uint8_t *PairingInitData;
    uint16_t PairingInitDataLen;

    WEAVE_ERROR Encode(PacketBuffer *msgBuf);
    static WEAVE_ERROR Decode(PacketBuffer *msgBuf, RegisterServicePairAccountMessage& msg);
};

class NL_DLL_EXPORT UpdateServiceMessage
{
public:
    uint64_t ServiceId;
    const uint8_t *ServiceConfig;
    uint16_t ServiceConfigLen;

    WEAVE_ERROR Encode(PacketBuffer *msgBuf);
    static WEAVE_ERROR Decode(PacketBuffer *msgBuf, UpdateServiceMessage& msg);
};

class NL_DLL_EXPORT PairDeviceToAccountMessage
{
public:
    uint64_t ServiceId;
    uint64_t FabricId;
    const char *AccountId;
    uint16_t AccountIdLen;
    const uint8_t *PairingToken;
    uint16_t PairingTokenLen;
    const uint8_t *PairingInitData;
    uint16_t PairingInitDataLen;
    const uint8_t *DeviceInitData;
    uint16_t DeviceInitDataLen;

    WEAVE_ERROR Encode(PacketBuffer *msgBuf);
    static WEAVE_ERROR Decode(PacketBuffer *msgBuf, PairDeviceToAccountMessage& msg);
};

#if WEAVE_CONFIG_ENABLE_IFJ_SERVICE_FABRIC_JOIN
class NL_DLL_EXPORT IFJServiceFabricJoinMessage
{
public:
    uint64_t ServiceId;
    uint64_t FabricId;
    const uint8_t *DeviceInitData;
    uint16_t DeviceInitDataLen;

    WEAVE_ERROR Encode(PacketBuffer *msgBuf);
    static WEAVE_ERROR Decode(PacketBuffer *msgBuf, IFJServiceFabricJoinMessage& msg);
};
#endif // WEAVE_CONFIG_ENABLE_IFJ_SERVICE_FABRIC_JOIN

class ServiceProvisioningDelegate : public WeaveServerDelegateBase
{
public:
    virtual WEAVE_ERROR HandleRegisterServicePairAccount(RegisterServicePairAccountMessage& msg) = 0;
    virtual WEAVE_ERROR HandleUpdateService(UpdateServiceMessage& msg) = 0;
    virtual WEAVE_ERROR HandleUnregisterService(uint64_t serviceId) = 0;
    virtual void HandlePairDeviceToAccountResult(WEAVE_ERROR localErr, uint32_t serverStatusProfileId, uint16_t serverStatusCode) = 0;
#if WEAVE_CONFIG_ENABLE_IFJ_SERVICE_FABRIC_JOIN
    virtual void HandleIFJServiceFabricJoinResult(WEAVE_ERROR localErr, uint32_t serverStatusProfileId, uint16_t serverStatusCode) = 0;
#endif // WEAVE_CONFIG_ENABLE_IFJ_SERVICE_FABRIC_JOIN

    /**
     * Enforce message-level access control for an incoming Service Provisioning request message.
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

    /**
     * Called to determine if the device is currently paired to an account.
     */
    // TODO: make this pure virtual when product code provides appropriate implementations.
    virtual bool IsPairedToAccount() const;
};


/**
 * Simple server class for implementing the Service Provisioning profile.
 */
class NL_DLL_EXPORT ServiceProvisioningServer : public WeaveServerBase
{
public:
    ServiceProvisioningServer(void);

    WEAVE_ERROR Init(WeaveExchangeManager *exchangeMgr);
    WEAVE_ERROR Shutdown(void);

    void SetDelegate(ServiceProvisioningDelegate *delegate);
    ServiceProvisioningDelegate* GetDelegate(void) const;

    virtual WEAVE_ERROR SendSuccessResponse(void);
    virtual WEAVE_ERROR SendStatusReport(uint32_t statusProfileId, uint16_t statusCode, WEAVE_ERROR sysError = WEAVE_NO_ERROR);

    // TODO: [TT] Remove when Bindings support existing Weave Connections or Service Directory.
    WEAVE_ERROR SendPairDeviceToAccountRequest(WeaveConnection *serverCon, uint64_t serviceId, uint64_t fabricId,
                                               const char *accountId, uint16_t accountIdLen,
                                               const uint8_t *pairingToken, uint16_t pairingTokenLen,
                                               const uint8_t *pairingInitData, uint16_t pairingInitDataLen,
                                               const uint8_t *deviceInitData, uint16_t deviceInitDataLen);

    WEAVE_ERROR SendPairDeviceToAccountRequest(Binding *binding, uint64_t serviceId, uint64_t fabricId,
                                               const char *accountId, uint16_t accountIdLen,
                                               const uint8_t *pairingToken, uint16_t pairingTokenLen,
                                               const uint8_t *pairingInitData, uint16_t pairingInitDataLen,
                                               const uint8_t *deviceInitData, uint16_t deviceInitDataLen);
#if WEAVE_CONFIG_ENABLE_IFJ_SERVICE_FABRIC_JOIN

    WEAVE_ERROR SendIFJServiceFabricJoinRequest(Binding *binding, uint64_t serviceId, uint64_t fabricId,
                                                const uint8_t *deviceInitData, uint16_t deviceInitDataLen);
#endif // WEAVE_CONFIG_ENABLE_IFJ_SERVICE_FABRIC_JOIN

    static bool IsValidServiceConfig(const uint8_t *serviceConfig, uint16_t serviceConfigLen);

protected:
    enum
    {
        kServerOpState_Idle                             = 0,
        kServerOpState_PairDeviceToAccount              = 1,
        kServerOpState_IFJServiceFabricJoin             = 2
    };

    ServiceProvisioningDelegate *mDelegate;
    ExchangeContext *mCurClientOp;
    PacketBuffer *mCurClientOpBuf;
    union
    {
        RegisterServicePairAccountMessage RegisterServicePairAccount;
        UpdateServiceMessage UpdateService;
    } mCurClientOpMsg;
    ExchangeContext *mCurServerOp;
    uint8_t mServerOpState;

private:
    static void HandleClientRequest(ExchangeContext *ec, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo,
                                    uint32_t profileId, uint8_t msgType, PacketBuffer *payload);
    static void HandleServerResponse(ExchangeContext *ec, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo,
                                     uint32_t profileId, uint8_t msgType, PacketBuffer *payload);
    static void HandleServerResponseTimeout(ExchangeContext *ec);
    static void HandleServerConnectionClosed(ExchangeContext *ec, WeaveConnection *con, WEAVE_ERROR conErr);
    static void HandleServerKeyError(ExchangeContext *ec, WEAVE_ERROR keyErr);

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    static void HandleServerSendError(ExchangeContext *ec, WEAVE_ERROR err, void *msgCtxt);
#endif // #if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING

    void HandleServiceProvisioningOpResult(WEAVE_ERROR localErr, uint32_t serverStatusProfileId, uint16_t serverStatusCode);

    ServiceProvisioningServer(const ServiceProvisioningServer&);   // not defined
};


extern WEAVE_ERROR EncodeServiceConfig(nl::Weave::Profiles::Security::WeaveCertificateSet& certSet, const char *dirHostName, uint16_t dirPort, uint8_t *outBuf, uint16_t& outLen);


} // namespace ServiceProvisioning
} // namespace Profiles
} // namespace Weave
} // namespace nl

#endif /* SERVICEPROVISIONING_H_ */
