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
 *      This file defines the Fabric Provisioning Profile, used to
 *      manage membership to Weave Fabrics.
 *
 *      The Fabric Provisioning Profile facilitates client-server operations
 *      such that the client (the controlling device) can trigger specific
 *      functionality on the server (the device undergoing provisioning),
 *      to allow it to create, join, and leave Weave Fabrics.  This includes
 *      communicating Fabric configuration information such as identifiers,
 *      keys, security schemes, and related data.
 */

#ifndef FABRICPROVISIONING_H_
#define FABRICPROVISIONING_H_

#include <Weave/Support/NLDLLUtil.h>
#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveServerBase.h>
#include <Weave/Core/WeaveEncoding.h>
#include <Weave/Core/WeaveTLV.h>

/**
 *   @namespace nl::Weave::Profiles::FabricProvisioning
 *
 *   @brief
 *     This namespace includes all interfaces within Weave for the
 *     Weave Fabric Provisioning profile, the second of the three
 *     Weave provisioning profiles.
 */

namespace nl {
namespace Weave {
namespace Profiles {
namespace FabricProvisioning {

using nl::Inet::IPAddress;

/**
 * Fabric Provisioning Status Codes
 */
// clang-format off
enum
{
    kStatusCode_AlreadyMemberOfFabric = 1, /**< The recipient is already a member of a fabric. */
    kStatusCode_NotMemberOfFabric     = 2, /**< The recipient is not a member of a fabric. */
    kStatusCode_InvalidFabricConfig   = 3  /**< The specified fabric configuration was invalid. */
};

/**
 * Fabric Provisioning Message Types
 */
enum
{
    // Application/Device Messages
    kMsgType_CreateFabric             = 1,
    kMsgType_LeaveFabric              = 2,
    kMsgType_GetFabricConfig          = 3,
    kMsgType_GetFabricConfigComplete  = 4,
    kMsgType_JoinExistingFabric       = 5
};

/**
 * Fabric Provisioning Data Element Tags
 */
enum
{
    // ---- Top-level Data Elements ----
    kTag_FabricConfig                 = 1, /**< [ structure ] Contains provisioning information for an existing fabric.
                                                  @note @b IMPORTANT: As a convenience to readers, all elements in a FabricConfig
                                                  must be encoded in numeric tag order, at all levels. */

    // ---- Context-specific Tags for FabricConfig Structure ----
    kTag_FabricId                     = 1, /**< [ uint ] Fabric ID. */
    kTag_FabricKeys                   = 2, /**< [ array ] List of FabricKey structures. */

    // ---- Context-specific Tags for FabricKey Structure ----
    kTag_FabricKeyId                  = 1, /**< [ uint ] Weave key ID for fabric key. */
    kTag_EncryptionType               = 2, /**< [ uint ] Weave encryption type supported by the key. */
    kTag_DataKey                      = 3, /**< [ byte-string ] Data encryption key. */
    kTag_IntegrityKey                 = 4, /**< [ byte-string ] Data integrity key. */
    kTag_KeyScope                     = 5, /**< [ uint ] Enumerated value identifying the category of devices that can possess
                                                  the fabric key. */
    kTag_RotationScheme               = 6, /**< [ uint ] Enumerated value identifying the rotation scheme for the key. */
    kTag_RemainingLifeTime            = 7, /**< [ uint ] Remaining time (in seconds) until key expiration. Absent if lifetime
                                                  is indefinite or doesn't apply. */
    kTag_RemainingReservedTime        = 8  /**< [ uint ] Remaining time (in seconds) until key is eligible for use. Absent if key
                                                  can be used right away. */
};
// clang-format on

/**
 * Delegate class for implementing additional actions corresponding to Fabric Provisioning operations.
 */
class FabricProvisioningDelegate : public WeaveServerDelegateBase
{
public:
    /**
     * Indicates that the device has created a new Fabric.
     *
     * @retval #WEAVE_NO_ERROR On success.
     * @retval other           Other Weave or platform-specific error codes indicating that an error
     *                         occurred preventing the device from creating a fabric.
     */
    virtual WEAVE_ERROR HandleCreateFabric(void) = 0;

    /**
     * Indicates that the device has joined an existing Fabric.
     *
     * @retval #WEAVE_NO_ERROR On success.
     * @retval other           Other Weave or platform-specific error codes indicating that an error
     *                         occurred preventing the device from joining the fabric.
     */
    virtual WEAVE_ERROR HandleJoinExistingFabric(void) = 0;

    /**
     * Indicates that the device has left a Fabric.
     *
     * @retval #WEAVE_NO_ERROR On success.
     * @retval other           Other Weave or platform-specific error codes indicating that an error
     *                         occurred preventing the device from leaving the fabric.
     */
    virtual WEAVE_ERROR HandleLeaveFabric(void) = 0;

    /**
     * Indicates that the configuration of the current Weave Fabric has been
     * requested.
     *
     * @retval #WEAVE_NO_ERROR On success.
     * @retval other           Other Weave or platform-specific error codes indicating that an error
     *                         occurred preventing the device from returning the fabric config.
     */
    virtual WEAVE_ERROR HandleGetFabricConfig(void) = 0;

    /**
     * Enforce message-level access control for an incoming Fabric Provisioning request message.
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
 * Server class for implementing the Fabric Provisioning profile.
 */
// TODO: Additional documentation detail required (i.e. expected class usage, number in the system, instantiation requirements, lifetime).
class NL_DLL_EXPORT FabricProvisioningServer : public WeaveServerBase
{
public:
    FabricProvisioningServer(void);

    WEAVE_ERROR Init(WeaveExchangeManager *exchangeMgr);
    WEAVE_ERROR Shutdown(void);

    void SetDelegate(FabricProvisioningDelegate *delegate);

    virtual WEAVE_ERROR SendSuccessResponse(void);
    virtual WEAVE_ERROR SendStatusReport(uint32_t statusProfileId, uint16_t statusCode, WEAVE_ERROR sysError = WEAVE_NO_ERROR);

protected:
    FabricProvisioningDelegate *mDelegate;
    ExchangeContext *mCurClientOp;

private:
    static void HandleClientRequest(ExchangeContext *ec, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo, uint32_t profileId,
            uint8_t msgType, PacketBuffer *payload);

    FabricProvisioningServer(const FabricProvisioningServer&);   // not defined
};


} // namespace FabricProvisioning
} // namespace Profiles
} // namespace Weave
} // namespace nl

#endif /* FABRICPROVISIONING_H_ */
