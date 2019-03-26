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
 *      This file defines the Device Description profile, containing
 *      data semantics and methods to specify and query device specific
 *      characteristics that pertain to Weave.
 *
 *      The Device Description profile is used to query device
 *      specific characteristics of Weave nodes via a client-server
 *      interface.  This information is communicated via IdentifyRequest
 *      and IdentifyResponse message types, the former used to discover
 *      devices matching a filter, and the latter used to respond with a
 *      payload detailing some or all of the characteristics specific to that
 *      device.  Such characteristics include the device vendor and
 *      make / model, as well as network information including MAC addresses
 *      and connections.
 */

#ifndef DEVICEDESCRIPTION_H_
#define DEVICEDESCRIPTION_H_

#include <Weave/Support/NLDLLUtil.h>
#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveServerBase.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Core/WeaveTLV.h>

/**
 *   @namespace nl::Weave::Profiles::DeviceDescription
 *
 *   @brief
 *     This namespace includes all interfaces within Weave for the
 *     Weave Device Description profile.
 */

namespace nl {
namespace Weave {
namespace Profiles {
namespace DeviceDescription {


class IdentifyRequestMessage;
class IdentifyResponseMessage;

/**
 * Message Types for the Device Description Profile.
 */
enum
{
    kMessageType_IdentifyRequest                = 1,
    kMessageType_IdentifyResponse               = 2
};


/**
 * Data Element Tags for the Device Description Profile.
 *
 * @note For Feature Tags, the absence of a specific tag indicates that the device does not support the associated feature.
 */
enum
{
    // Top-level Tags
    kTag_WeaveDeviceDescriptor                  = 1,    /**< Structure containing information describing a Weave device. Top-level Tag */

    // Context-specific Tags for WeaveDeviceDescriptor Structure
    kTag_VendorId                               = 0,    /**< [ uint, range 1-65535 ] Code identifying product vendor. Context-specific Tag */
    kTag_ProductId                              = 1,    /**< [ uint, range 1-65535 ] Code identifying product. Context-specific Tag */
    kTag_ProductRevision                        = 2,    /**< [ uint, range 1-65535 ] Code identifying product revision. Context-specific Tag */
    kTag_ManufacturingDate                      = 3,    /**< [ uint, range 1-65535 ] Calendar date of manufacture in encoded form. Context-specific Tag */
    kTag_SerialNumber                           = 4,    /**< [ UTF-8 string, len 1-32 ] Device serial number. Context-specific Tag */
    kTag_Primary802154MACAddress                = 5,    /**< [ byte string, len = 8 ] MAC address for device's primary 802.15.4 interface. Context-specific Tag */
    kTag_PrimaryWiFiMACAddress                  = 6,    /**< [ byte string, len = 6 ] MAC address for device's primary WiFi interface. Context-specific Tag */
    kTag_RendezvousWiFiESSID                    = 7,    /**< [ UTF-8 string, len 1-32 ] ESSID for device's WiFi rendezvous network. Context-specific Tag.
                                                                @note: This tag is mutually exclusive with the RendezvousWiFiESSIDSuffix tag. */
    kTag_PairingCode                            = 8,    /**< [ UTF-8 string, len 6-16 ] The pairing code for the device. Context-specific Tag
                                                                @note @b IMPORTANT: For security reasons, the PairingCode field should *never*
                                                                be sent over the network. It is present in a WeaveDeviceDescriptor structure so
                                                                that is can encoded in a data label (e.g. QR-code) that is physically associated
                                                                with the device. */
    kTag_SoftwareVersion                        = 9,    /**< [ UTF-8 string, len 1-32 ] Version of software on the device. Context-specific Tag */
    kTag_DeviceId                               = 10,   /**< [ uint, 2^64 max ] Weave device ID. Context-specific Tag */
    kTag_FabricId                               = 11,   /**< [ uint, 2^64 max ] ID of Weave fabric to which the device belongs. Context-specific Tag */
    kTag_PairingCompatibilityVersionMajor       = 12,   /**< [ uint, range 1-65535 ] Pairing software compatibility major version. Context-specific Tag */
    kTag_PairingCompatibilityVersionMinor       = 13,   /**< [ uint, range 1-65535 ] Pairing software compatibility minor version. Context-specific Tag */
    kTag_RendezvousWiFiESSIDSuffix              = 14,   /**< [ UTF-8 string, len 1-32 ] ESSID suffix for device's WiFi rendezvous network. Context-specific Tag.
                                                                @note: This tag is mutually exclusive with the RendezvousWiFiESSID tag. */

    // Feature Tags (Context-specific Tags in WeaveDeviceDescriptor that indicate presence of device features)
    // NOTE: The absence of a specific tag indicates that the device does not support the associated feature.
    kTag_DeviceFeature_HomeAlarmLinkCapable     = 100,  /**< [ boolean ] Indicates a Nest Protect that supports connection to a home alarm panel. Feature Tag */
    kTag_DeviceFeature_LinePowered              = 101   /**< [ boolean ] Indicates a device that requires line power. Feature Tag */
};


/**
 *  Contains descriptive information about a Weave device.
 */
class NL_DLL_EXPORT WeaveDeviceDescriptor
{
public:
    WeaveDeviceDescriptor(void);

    /**
     *  Defines the maximum length of some attributes.
     */
    enum
    {
        kMaxSerialNumberLength                  = 32,  /**< Maximum serial number length. */
        kMaxPairingCodeLength                   = 16,  /**< Maximum pairing code length. */
        kMaxRendezvousWiFiESSID                 = 32,  /**< Maximum WiFi ESSID for Rendezvous length. */
        kMaxSoftwareVersionLength               = 32   /**< Maximum software version length. */
    };

    /**
     *  Feature flags indicating specific device capabilities.
     */
    enum
    {
        kFeature_HomeAlarmLinkCapable           = 0x00000001,   /**< Indicates a Nest Protect that supports connection to a home alarm panel. */
        kFeature_LinePowered                    = 0x00000002    /**< Indicates a device that requires line power. */
    };

    /**
     * Flags field definitions.
     */
    enum
    {
        kFlag_IsRendezvousWiFiESSIDSuffix       = 0x01,         /**< Indicates that the RendezvousWiFiESSID value is a suffix string that
                                                                     appears at the end of the ESSID of the device's WiFi rendezvous network. */
    };

     // Device specific characteristics
    uint64_t DeviceId;                                          /**< Weave device ID (0 = not present) */
    uint64_t FabricId;                                          /**< ID of Weave fabric to which the device belongs (0 = not present) */
    uint32_t DeviceFeatures;                                    /**< Bit field indicating support for specific device features. */
    uint16_t VendorId;                                          /**< Device vendor code (0 = not present) */
    uint16_t ProductId;                                         /**< Device product code (0 = not present) */
    uint16_t ProductRevision;                                   /**< Device product revision (0 = not present) */
    struct {
        uint16_t Year;                                          /**< Year of device manufacture (valid range 2001 - 2099) */
        uint8_t Month;                                          /**< Month of device manufacture (1 = January) */
        uint8_t Day;                                            /**< Day of device manufacture (0 = not present) */
    } ManufacturingDate;
    uint8_t Primary802154MACAddress[8];                         /**< MAC address for primary 802.15.4 interface (big-endian, all zeros = not present) */
    uint8_t PrimaryWiFiMACAddress[6];                           /**< MAC address for primary WiFi interface (big-endian, all zeros = not present) */
    char SerialNumber[kMaxSerialNumberLength+1];                /**< Serial number of device (NUL terminated, 0 length = not present) */
    char SoftwareVersion[kMaxSoftwareVersionLength+1];          /**< Active software version (NUL terminated, 0 length = not present) */
    char RendezvousWiFiESSID[kMaxRendezvousWiFiESSID+1];        /**< ESSID for device WiFi rendezvous network (NUL terminated, 0 length = not present) */
    char PairingCode[kMaxPairingCodeLength+1];                  /**< Device pairing code (NUL terminated, 0 length = not present) */
    uint16_t PairingCompatibilityVersionMajor;                  /**< Major device pairing software compatibility version. */
    uint16_t PairingCompatibilityVersionMinor;                  /**< Minor device pairing software compatibility version. */
    uint8_t Flags;                                              /**< Bit field containing additional information about the device. */

    void Clear(void);

    static WEAVE_ERROR EncodeText(const WeaveDeviceDescriptor& desc, char *buf, uint32_t bufLen, uint32_t& outEncodedLen);
    static WEAVE_ERROR EncodeTLV(const WeaveDeviceDescriptor& desc, uint8_t *buf, uint32_t bufLen, uint32_t& outEncodedLen);
    static WEAVE_ERROR EncodeTLV(const WeaveDeviceDescriptor& desc, nl::Weave::TLV::TLVWriter& writer);
    static WEAVE_ERROR Decode(const uint8_t *data, uint32_t dataLen, WeaveDeviceDescriptor& outDesc);
    static WEAVE_ERROR DecodeText(const char *data, uint32_t dataLen, WeaveDeviceDescriptor& outDesc);
    static WEAVE_ERROR DecodeTLV(const uint8_t *data, uint32_t dataLen, WeaveDeviceDescriptor& outDesc);
    static WEAVE_ERROR DecodeTLV(nl::Weave::TLV::TLVReader& reader, WeaveDeviceDescriptor& outDesc);
    static bool IsZeroBytes(const uint8_t *buf, uint32_t len);

private:
    static WEAVE_ERROR EncodeManufacturingDate(uint16_t year, uint8_t month, uint8_t day, uint16_t& outEncodedDate);
    static WEAVE_ERROR DecodeManufacturingDate(uint16_t encodedDate, uint16_t& outYear, uint8_t& outMonth, uint8_t& outDay);
};


/**
 * Client object for issuing Device Description requests.
 */
class DeviceDescriptionClient
{
public:
    DeviceDescriptionClient(void);

    /**
     * Application defined state object.
     */
    void *AppState;

    const WeaveFabricState *FabricState;            /**< [READ ONLY] Fabric state object */
    WeaveExchangeManager *ExchangeMgr;              /**< [READ ONLY] Exchange manager object */

    WEAVE_ERROR Init(WeaveExchangeManager *exchangeMgr);
    WEAVE_ERROR Shutdown(void);

    WEAVE_ERROR SendIdentifyRequest(const IPAddress& nodeAddr, const IdentifyRequestMessage& msg);
    WEAVE_ERROR SendIdentifyRequest(const IdentifyRequestMessage& msg);
    WEAVE_ERROR CancelExchange(void);

    /**
     * This function is responsible for processing IdentityResponse messages.
     *
     *  @param[in]    appState     A pointer to the application defined state set when creating the
     *                             IdentityRequest Exchange Context.
     *  @param[in]    nodeId       The Weave node ID of the message source.
     *  @param[in]    nodeAddr     The IP address of the responding node.
     *  @param[in]    msg          A reference to the incoming IdentifyResponse message.
     *
     */
    typedef void (*HandleIdentifyResponseFunct)(void *appState, uint64_t nodeId, const IPAddress& nodeAddr, const IdentifyResponseMessage& msg);
    HandleIdentifyResponseFunct OnIdentifyResponseReceived;

private:
    ExchangeContext *ExchangeCtx;

    static void HandleResponse(ExchangeContext *ec, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *payload);

    DeviceDescriptionClient(const DeviceDescriptionClient&);   // not defined
};


/**
 * Server object for responding to Device Description requests.
 */
class NL_DLL_EXPORT DeviceDescriptionServer : public WeaveServerBase
{
public:
    DeviceDescriptionServer(void);

    void *AppState;                                 /**< Application defined state pointer to provide context for callbacks. */

    WEAVE_ERROR Init(WeaveExchangeManager *exchangeMgr);
    WEAVE_ERROR Shutdown(void);

    typedef void (*HandleIdentifyRequestFunct)(void *appState, uint64_t nodeId, const IPAddress& nodeAddr, const IdentifyRequestMessage& reqMsg, bool& sendResp, IdentifyResponseMessage& respMsg);

    /**
     * This function is responsible for processing IdentityRequest messages.
     *
     *  @param[in]    appState     A pointer to the application defined state set when registering
     *                             to receive messages of this type.
     *  @param[in]    nodeId       The Weave node ID of the message source.
     *  @param[in]    nodeAddr     The IP address of the message source.
     *  @param[in]    reqMsg       A reference to the incoming IdentifyRequest message.
     *  @param[out]   sendResp     A reference to a boolean that should be set to true if a response message should be sent to the initiator.
     *  @param[out]   respMsg      A reference to the IdentifyResponse message to be sent to the initiator.
     *
     */
    HandleIdentifyRequestFunct OnIdentifyRequestReceived;

private:
    static void HandleRequest(ExchangeContext *ec, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *payload);

    DeviceDescriptionServer(const DeviceDescriptionServer&);   // not defined
};


/**
 * Special target fabric IDs.
 */
enum TargetFabricIds
{
    kTargetFabricId_NotInFabric = kFabricIdNotSpecified,        /**< Specifies that only devices that are __not__ a member of a fabric should respond. */
    kTargetFabricId_AnyFabric   = kReservedFabricIdStart,       /**< Specifies that only devices that __are_ a member of a fabric should respond. */
    kTargetFabricId_Any         = kMaxFabricId,                 /**< Specifies that all devices should respond regardless of fabric membership. */
};

extern bool MatchTargetFabricId(uint64_t fabricId, uint64_t targetFabricId);

/**
 * Bit field (32-bits max) identifying which devices should respond
 * to a LocateRequest Message based on their current mode.
 *
 * Note that the modes defined here are intended to be general such that they can be
 * applied to a variety of device types.
 */
enum TargetDeviceModes
{
    kTargetDeviceMode_Any               = 0x00000000,           /**< Locate all devices regardless of mode. */

    kTargetDeviceMode_UserSelectedMode  = 0x00000001            /**< Locate all devices in 'user-selected' mode -- that is, where the device has
                                                                     been directly identified by a user by pressing a button (or equivalent). */
};

/**
 * Represents criteria use to select devices in the IdentifyDevice protocol.
 */
class NL_DLL_EXPORT IdentifyDeviceCriteria
{
public:
    /**
     * Specifies that only devices that are members of the specified Weave fabric
     * should respond. Value can be an actual fabric ID, or one of the
     * #TargetFabricIds enum values.
     */
    uint64_t TargetFabricId;

    /**
     * Specifies that only devices that are currently in the specified modes
     * should respond. Values are taken from the #TargetDeviceModes enum.
     */
    uint32_t TargetModes;

    /**
     * Specifies that only devices manufactured by the specified vendor should
     * respond to the identify request. A value of 0xFFFF specifies any vendor.
     */
    uint16_t TargetVendorId;

    /**
     * Specifies that only devices with the specified product ID should respond.
     * A value of 0xFFFF specifies any product.
     * If the TargetProductId field is specified, then the TargetVendorId must
     * also be specified.
     */
    uint16_t TargetProductId;

    /**
     * Specifies that only the device with the specified Weave Node ID should respond.
     * A value of kAnyNodeId specifies any device.
     *
     * @note The value of the TargetDeviceId field is carried a Weave IdentifyRequest
     *  in the Destination node ID field of the Weave message header, and thus
     *  does __not__ appear in the payload of the message.
     */
    uint64_t TargetDeviceId;

    IdentifyDeviceCriteria(void);

    void Reset(void);
};


/**
 * Parsed form of an IdentifyRequest Message.
 */
class NL_DLL_EXPORT IdentifyRequestMessage : public IdentifyDeviceCriteria
{
public:
    WEAVE_ERROR Encode(PacketBuffer *msgBuf) const;
    static WEAVE_ERROR Decode(PacketBuffer *msgBuf, uint64_t msgDestNodeId, IdentifyRequestMessage& msg);
};


/**
 * Parsed form of an IdentifyResponse Message.
 */
class NL_DLL_EXPORT IdentifyResponseMessage
{
public:
    /**
     * A device descriptor describing the responding device.
     */
    WeaveDeviceDescriptor DeviceDesc;

    WEAVE_ERROR Encode(PacketBuffer *msgBuf);
    static WEAVE_ERROR Decode(PacketBuffer *msgBuf, IdentifyResponseMessage& msg);
};


} // namespace DeviceDescription
} // namespace Profiles
} // namespace Weave
} // namespace nl

#endif // DEVICEDESCRIPTION_H_
