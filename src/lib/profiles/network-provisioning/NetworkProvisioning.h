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
 *      This file defines the Weave Network Provisioning Profile, used
 *      to configure network interfaces.
 *
 *      The Network Provisioning Profile facilitates client-server
 *      operations such that the client (the controlling device) can
 *      trigger specific network functionality on the server (the device
 *      undergoing network provisioning).  These operations revolve around
 *      the steps necessary to provision the server device's network
 *      interfaces (such as 802.15.4/Thread and 802.11/Wi-Fi) provisioned
 *      such that the device may participate in those networks.  This includes
 *      scanning and specifying network names and security credentials.
 *
 */

#ifndef NETWORKPROVISIONING_H_
#define NETWORKPROVISIONING_H_

#include <Weave/Support/NLDLLUtil.h>
#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveServerBase.h>
#include <Weave/Core/WeaveEncoding.h>
#include <Weave/Core/WeaveTLV.h>

/**
 *   @namespace nl::Weave::Profiles::NetworkProvisioning
 *
 *   @brief
 *     This namespace includes all interfaces within Weave for the
 *     Weave Network Provisioning profile, the first of the three
 *     Weave provisioning profiles.
 *
 *     The interfaces define status codes, message types, data element
 *     tags, other constants, a server object, and a delegate object.
 *
 *     The Nest Weave Network Provisioning Profile is focused on
 *     providing the data to get the network interfaces, such as
 *     802.15.4/Thread and 802.11/Wi-Fi, for a Weave device
 *     provisioned such that the device may participate in those
 *     networks.
 */

namespace nl {
namespace Weave {
namespace Profiles {
namespace NetworkProvisioning {

using nl::Weave::TLV::TLVReader;
using nl::Weave::TLV::TLVWriter;

/**
 *  Network Provisioning Status Codes.
 */
enum
{
    kStatusCode_UnknownNetwork                  = 1,            /**< A provisioned network with the specified network ID was not found. */
    kStatusCode_TooManyNetworks                 = 2,            /**< The maximum number of provisioned networks has been reached. */
    kStatusCode_InvalidNetworkConfiguration     = 3,            /**< The specified network configuration is invalid. */
    kStatusCode_UnsupportedNetworkType          = 4,            /**< The specified network type is unknown or unsupported. */
    kStatusCode_UnsupportedWiFiMode             = 5,            /**< The specified WiFi mode is unsupported. */
    kStatusCode_UnsupportedWiFiRole             = 6,            /**< The specified WiFi role is unsupported. */
    kStatusCode_UnsupportedWiFiSecurityType     = 7,            /**< The specified WiFi security type is unsupported. */
    kStatusCode_InvalidState                    = 8,            /**< The network provisioning operation could not be performed in the current state. */
    kStatusCode_TestNetworkFailed               = 9,            /**< The connectivity test of the specified network failed. */
                                                                // XXX Placeholder for more detailed errors to come
    kStatusCode_NetworkConnectFailed            = 10            /**< An attempt to connect to the specified network failed. */
};

/**
 * Network Provisioning Message Types.
 */
enum
{
    kMsgType_ScanNetworks                       = 1,
    kMsgType_NetworkScanComplete                = 2,
    kMsgType_AddNetwork                         = 3,
    kMsgType_AddNetworkComplete                 = 4,
    kMsgType_UpdateNetwork                      = 5,
    kMsgType_RemoveNetwork                      = 6,
    kMsgType_EnableNetwork                      = 7,
    kMsgType_DisableNetwork                     = 8,
    kMsgType_TestConnectivity                   = 9,
    kMsgType_SetRendezvousMode                  = 10,
    kMsgType_GetNetworks                        = 11,
    kMsgType_GetNetworksComplete                = 12,
    kMsgType_GetLastResult                      = 13
};

/**
 * @anchor NetworkProvisioningDataElementTags
 * Network Provisioning Data Element Tags.
 */
enum
{
    /**
     * Top-level Data Elements (profile-specific).
     */
    kTag_Networks                               = 1,	/**< An array of NetworkConfiguration structures. [ array ] */
    kTag_EnabledNetworks                        = 2,	/**< An array of NetworkIds identifying the networks currently enabled on the device. [ array ] */
    kTag_RendezvousMode                         = 3,	/**< A bit field indicating the currently active rendezvous mode. [ uint ] */

    /**
     * General-Purpose Data Elements (profile-specific).
     */
    kTag_NetworkInformation                     = 32,	/**< A structure containing information for a network. [ struct ] */
    kTag_NetworkId                              = 33,	/**< An integer uniquely identifying a provisioned network. [ uint, 32-bit max ] */
    kTag_NetworkType                            = 34,	/**< An unsigned integer value identifying the type of a network. [ uint, 8-bit max ] */
    kTag_WirelessSignalStrength                 = 35,   /**< An signed integer value giving the signal strength of a wireless network in dBm. [ int, 16-bit max ] */

    /**
     * WiFi Data Elements (Profile-specific).
     */
    kTag_WiFiSSID                               = 64,	/**< A string containing a WiFi SSID. [ UTF-8 string ] */
    kTag_WiFiMode                               = 65,	/**< An integer identify the mode of operation of the WiFi network. [ uint, 8-bit max ] */
    kTag_WiFiRole                               = 66,	/**< An integer identify the role the device plays in the WiFi network. [ uint, 8-bit max ] */
    kTag_WiFiSecurityType                       = 67,	/**< An integer value identifying the type of security used by a WiFi network. [ uint, 8-bit max ] */
    kTag_WiFiPreSharedKey                       = 68,	/**< A byte string containing the WiFi password/pre-shared key. */

    /**
     * Thread Data Elements (profile-specific).
     */
    kTag_ThreadExtendedPANId                    = 80,   /**< The Thread extended PAN ID. [ byte string ] */
    kTag_ThreadNetworkName                      = 81,   /**< A UTF-8 string containing the name of the Thread network. [ UTF-8 string ] */
    kTag_ThreadNetworkKey                       = 82,   /**< The Thread master network key. [ bytes string ] */
    kTag_ThreadMeshPrefix                       = 83    /**< Thread mesh IPv6 /64 prefix (optional). [ bytes string, exactly 8 bytes ] */
};

/**
 * Network Types.
 */
enum NetworkType
{
    kNetworkType_NotSpecified                   = -1,

    kNetworkType_WiFi                           = 1,
    kNetworkType_Thread                         = 2
};

/**
 * WiFi Security Modes.
 */
enum WiFiSecurityType
{
    kWiFiSecurityType_NotSpecified              = -1,

    kWiFiSecurityType_None                      = 1,
    kWiFiSecurityType_WEP                       = 2,
    kWiFiSecurityType_WPAPersonal               = 3,
    kWiFiSecurityType_WPA2Personal              = 4,
    kWiFiSecurityType_WPA2MixedPersonal         = 5,
    kWiFiSecurityType_WPAEnterprise             = 6,
    kWiFiSecurityType_WPA2Enterprise            = 7,
    kWiFiSecurityType_WPA2MixedEnterprise       = 8
};

/**
 * WiFi Operating Modes.
 */
enum WiFiMode
{
    kWiFiMode_NotSpecified                      = -1,

    kWiFiMode_AdHoc                             = 1,
    kWiFiMode_Managed                           = 2
};

/**
 * Device WiFi Role.
 */
enum WiFiRole
{
    kWiFiRole_NotSpecified                      = -1,

    kWiFiRole_Station                           = 1,
    kWiFiRole_AccessPoint                       = 2
};

/**
 * Rendezvous Mode Flags.
 */
enum RendezvousModeFlags
{
    kRendezvousMode_EnableWiFiRendezvousNetwork = 0x0001,
    kRendezvousMode_EnableThreadRendezvous      = 0x0002
};

/**
 * Get Network Flags.
 */
enum GetNetworkFlags
{
    kGetNetwork_IncludeCredentials              = 0x01
};

/**
 * Delegate class for implementing Network Provisioning operations.
 */
class NetworkProvisioningDelegate : public WeaveServerDelegateBase
{
public:
    /**
     * Perform a network scan.
     *
     * @param[in] networkType   The technology (for example, WiFi or Thread) to scan.  @sa #NetworkType for valid types.
     *
     * @retval #WEAVE_NO_ERROR   On success.
     * @retval other            Other Weave or platform-specific error codes indicating that an error
     *                          occurred preventing the device from performing a network scan.
     */
    virtual WEAVE_ERROR HandleScanNetworks(uint8_t networkType) = 0;

    /**
     * Add a particular network.
     *
     * @param[in] networkInfoTLV    The network configuration encoded in TLV.  @sa @ref NetworkProvisioningDataElementTags for valid types.
     *
     * @retval #WEAVE_NO_ERROR  On success.
     * @retval other            Other Weave or platform-specific error codes indicating that an error
     *                          occurred preventing the device from adding the network.
     */
    virtual WEAVE_ERROR HandleAddNetwork(PacketBuffer *networkInfoTLV) = 0;

    /**
     * Update a network's configuration.
     *
     * @param[in] networkInfoTLV    The network configuration encoded in TLV.  @sa @ref NetworkProvisioningDataElementTags for valid types.
     *
     * @retval #WEAVE_NO_ERROR  On success.
     * @retval other            Other Weave or platform-specific error codes indicating that an error
     *                          occurred preventing the device from updating the network.
     */
    virtual WEAVE_ERROR HandleUpdateNetwork(PacketBuffer *networkInfoTLV) = 0;

    /**
     * Remove a configured network.
     *
     * @param[in] networkId     The ID of the network to remove.
     *
     * @retval #WEAVE_NO_ERROR  On success.
     * @retval other            Other Weave or platform-specific error codes indicating that an error
     *                          occurred preventing the device from removing the network.
     */
    virtual WEAVE_ERROR HandleRemoveNetwork(uint32_t networkId) = 0;

    /**
     * Get the configured networks.
     *
     * @param[in] flags     Flags to filter the retrieved networks.
     *
     * @retval #WEAVE_NO_ERROR  On success.
     * @retval other            Other Weave or platform-specific error codes indicating that an error
     *                          occurred preventing the device from getting the configured networks.
     */
    virtual WEAVE_ERROR HandleGetNetworks(uint8_t flags) = 0;

    /**
     * Enable the specified network.
     *
     * @param[in] networkId     The ID of the network to enable.
     *
     * @retval #WEAVE_NO_ERROR  On success.
     * @retval other            Other Weave or platform-specific error codes indicating that an error
     *                          occurred preventing the device from enabling the network.
     */
    virtual WEAVE_ERROR HandleEnableNetwork(uint32_t networkId) = 0;

    /**
     * Disable the specified network.
     *
     * @param[in] networkId     The ID of the network to disable.
     *
     * @retval #WEAVE_NO_ERROR  On success.
     * @retval other            Other Weave or platform-specific error codes indicating that an error
     *                          occurred preventing the device from disabling the network.
     */
    virtual WEAVE_ERROR HandleDisableNetwork(uint32_t networkId) = 0;

    /**
     * Test the connectivity of the specified network.
     *
     * @param[in] networkId     The ID of the network to test the connectivity of.
     *
     * @retval #WEAVE_NO_ERROR  On success.
     * @retval other            Other Weave or platform-specific error codes indicating that an error
     *                          occurred preventing the device from testing connectivity.
     */
    virtual WEAVE_ERROR HandleTestConnectivity(uint32_t networkId) = 0;

    /**
     * Set the rendezvous mode.
     *
     * @param[in] rendezvousMode    The rendezvous mode to use.  @sa #RendezvousModeFlags for valid modes.
     *
     * @retval #WEAVE_NO_ERROR  On success.
     * @retval other            Other Weave or platform-specific error codes indicating that an error
     *                          occurred preventing the device from setting the rendezvous mode.
     */
    virtual WEAVE_ERROR HandleSetRendezvousMode(uint16_t rendezvousMode) = 0;

    /**
     * Enforce message-level access control for an incoming Network Provisioning request message.
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
 * Server class for implementing the Network Provisioning profile.
 */
// TODO: Additional documentation detail required (i.e. expected class usage, number in the system, instantiation requirements, lifetime).
class NL_DLL_EXPORT NetworkProvisioningServer : public WeaveServerBase
{
public:
    NetworkProvisioningServer(void);

    WEAVE_ERROR Init(WeaveExchangeManager *exchangeMgr);
    WEAVE_ERROR Shutdown(void);

    void SetDelegate(NetworkProvisioningDelegate *delegate);

    virtual WEAVE_ERROR SendNetworkScanComplete(uint8_t resultCount, PacketBuffer *scanResultsTLV);
    virtual WEAVE_ERROR SendAddNetworkComplete(uint32_t networkId);
    virtual WEAVE_ERROR SendGetNetworksComplete(uint8_t resultCount, PacketBuffer *resultsTLV);
    virtual WEAVE_ERROR SendSuccessResponse(void);
    virtual WEAVE_ERROR SendStatusReport(uint32_t statusProfileId, uint16_t statusCode, WEAVE_ERROR sysError = WEAVE_NO_ERROR);

protected:
    ExchangeContext *mCurOp;
    NetworkProvisioningDelegate *mDelegate;
    struct
    {
        uint32_t StatusProfileId;
        uint16_t StatusCode;
        WEAVE_ERROR SysError;
    } mLastOpResult;
    uint8_t mCurOpType;

private:
    static void HandleRequest(ExchangeContext *ec, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo, uint32_t profileId,
            uint8_t msgType, PacketBuffer *payload);
    WEAVE_ERROR SendCompleteWithNetworkList(uint8_t msgType, int8_t resultCount, PacketBuffer *resultTLV);

    NetworkProvisioningServer(const NetworkProvisioningServer&);   // not defined
};

} // namespace NetworkProvisioning
} // namespace Profiles
} // namespace Weave
} // namespace nl

#endif /* NETWORKPROVISIONING_H_ */
