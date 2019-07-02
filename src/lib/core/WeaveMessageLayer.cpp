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
 *      This file implements the WeaveMessageLayer class. It manages communication
 *      with other Weave nodes by employing one of several Inetlayer endpoints
 *      to establish a communication channel with other Weave nodes.
 *
 */

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#include <stdint.h>
#include <string.h>
#include <errno.h>

#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveMessageLayer.h>
#include <Weave/Core/WeaveExchangeMgr.h>
#include <Weave/Core/WeaveEncoding.h>
#include <Weave/Support/crypto/WeaveCrypto.h>
#include <Weave/Support/crypto/HashAlgos.h>
#include <Weave/Support/crypto/HMAC.h>
#include <Weave/Support/crypto/AESBlockCipher.h>
#include <Weave/Support/crypto/CTRMode.h>
#include <Weave/Support/logging/WeaveLogging.h>
#include <Weave/Support/ErrorStr.h>
#include <Weave/Support/CodeUtils.h>
#include <Weave/Support/WeaveFaultInjection.h>


namespace nl {
namespace Weave {

using namespace nl::Weave::Crypto;
using namespace nl::Weave::Encoding;

/**
 *  @def WEAVE_BIND_DETAIL_LOGGING
 *
 *  @brief
 *    Use Weave Bind detailed logging for Weave communication.
 *
 */
#ifndef WEAVE_BIND_DETAIL_LOGGING
#define WEAVE_BIND_DETAIL_LOGGING 1
#endif

/**
 *  @def WeaveBindLog(MSG, ...)
 *
 *  @brief
 *    Define WeaveBindLogic to be the same as WeaveLogProgress based on
 *    whether both #WEAVE_BIND_DETAIL_LOGGING and #WEAVE_DETAIL_LOGGING
 *    are set.
 *
 */
#if WEAVE_BIND_DETAIL_LOGGING && WEAVE_DETAIL_LOGGING
#define WeaveBindLog(MSG, ...) WeaveLogProgress(MessageLayer, MSG, ## __VA_ARGS__ )
#else
#define WeaveBindLog(MSG, ...)
#endif


enum
{
    kKeyIdLen = 2,
    kMinPayloadLen = 1
};

/**
 *  The Weave Message layer constructor.
 *
 *  @note
 *    The class must be initialized via WeaveMessageLayer::Init()
 *    prior to use.
 *
 */
WeaveMessageLayer::WeaveMessageLayer()
{
    State = kState_NotInitialized;
}

/**
 *  Initialize the Weave Message layer object.
 *
 *  @param[in]  context  A pointer to the InitContext object.
 *
 *  @retval  #WEAVE_NO_ERROR                     on successful initialization.
 *  @retval  #WEAVE_ERROR_INVALID_ARGUMENT       if the passed InitContext object is NULL.
 *  @retval  #WEAVE_ERROR_INCORRECT_STATE        if the state of the WeaveMessageLayer object is incorrect.
 *  @retval  other errors generated from the lower Inet layer during endpoint creation.
 *
 */
WEAVE_ERROR WeaveMessageLayer::Init(InitContext *context)
{
    WEAVE_ERROR res = WEAVE_NO_ERROR;

    if (context == NULL)
        return WEAVE_ERROR_INVALID_ARGUMENT;

    if (State != kState_NotInitialized)
        return WEAVE_ERROR_INCORRECT_STATE;

    SystemLayer = context->systemLayer;
    Inet = context->inet;
#if CONFIG_NETWORK_LAYER_BLE
    mBle = context->ble;
#endif

#if WEAVE_CONFIG_PROVIDE_OBSOLESCENT_INTERFACES
    if (SystemLayer == NULL)
    {
        SystemLayer = Inet->SystemLayer();
    }
#endif // WEAVE_CONFIG_PROVIDE_OBSOLESCENT_INTERFACES

    FabricState = context->fabricState;
    FabricState->MessageLayer = this;
    OnMessageReceived = NULL;
    OnReceiveError = NULL;
    OnConnectionReceived = NULL;
    OnUnsecuredConnectionReceived = NULL;
    OnUnsecuredConnectionCallbacksRemoved = NULL;
    OnAcceptError = NULL;
    OnMessageLayerActivityChange = NULL;
    memset(mConPool, 0, sizeof(mConPool));
    memset(mTunnelPool, 0, sizeof(mTunnelPool));
    AppState = NULL;
    ExchangeMgr = NULL;
    SecurityMgr = NULL;
    IsListening = context->listenTCP || context->listenUDP;
    IncomingConIdleTimeout = 0;

    //Internal and for Debug Only; When set, Message Layer drops message and returns.
    mDropMessage = false;
    mFlags = 0;
    if (context->listenTCP)
        mFlags |= kFlag_ListenTCP;
    if (context->listenUDP)
        mFlags |= kFlag_ListenUDP;
#if CONFIG_NETWORK_LAYER_BLE
    if (context->listenBLE)
        mFlags |= kFlag_ListenBLE;
#endif

#if WEAVE_CONFIG_ENABLE_TARGETED_LISTEN
#if INET_CONFIG_ENABLE_IPV4
    if (FabricState->ListenIPv6Addr != IPAddress::Any)
        mFlags |= kFlag_ListenIPv6;
    if (FabricState->ListenIPv4Addr != IPAddress::Any)
        mFlags |= kFlag_ListenIPv4;
    if ((mFlags & (kFlag_ListenIPv4 | kFlag_ListenIPv6)) == 0)
        mFlags |= kFlag_ListenIPv4 | kFlag_ListenIPv6;
#else // !INET_CONFIG_ENABLE_IPV4
    mFlags |= kFlag_ListenIPv6;
#endif // !INET_CONFIG_ENABLE_IPV4
#else // !WEAVE_CONFIG_ENABLE_TARGETED_LISTEN
    mFlags |= kFlag_ListenIPv6;
#if INET_CONFIG_ENABLE_IPV4
    mFlags |= kFlag_ListenIPv4;
#endif // !INET_CONFIG_ENABLE_IPV4
#endif // !WEAVE_CONFIG_ENABLE_TARGETED_LISTEN

    mIPv6TCPListen = NULL;
    mIPv6UDP = NULL;
#if WEAVE_CONFIG_ENABLE_TARGETED_LISTEN
    mIPv6UDPMulticastRcv = NULL;
#endif

#if INET_CONFIG_ENABLE_IPV4
    mIPv4TCPListen = NULL;
    mIPv4UDP = NULL;
#endif // INET_CONFIG_ENABLE_IPV4

#if WEAVE_CONFIG_ENABLE_UNSECURED_TCP_LISTEN
    mUnsecuredIPv6TCPListen = NULL;
#endif
    memset(mIPv6UDPLocalAddr, 0, sizeof(mIPv6UDPLocalAddr));
    memset(mInterfaces, 0, sizeof(mInterfaces));

    res = RefreshEndpoints();
    if (res != WEAVE_NO_ERROR)
        goto done;

done:
    if (res != WEAVE_NO_ERROR)
        Shutdown();
    else
        State = kState_Initialized;

    return res;
}

/**
 *  Shutdown the WeaveMessageLayer.
 *
 *  Close all open Inet layer endpoints, reset all
 *  higher layer callbacks, member variables and objects.
 *  A call to Shutdown() terminates the WeaveMessageLayer
 *  object.
 *
 */
WEAVE_ERROR WeaveMessageLayer::Shutdown()
{
    CloseEndpoints();

    State = kState_NotInitialized;
    IsListening = false;
    FabricState = NULL;
    OnMessageReceived = NULL;
    OnReceiveError = NULL;
    OnUnsecuredConnectionReceived = NULL;
    OnConnectionReceived = NULL;
    OnAcceptError = NULL;
    OnMessageLayerActivityChange = NULL;
    memset(mConPool, 0, sizeof(mConPool));
    memset(mTunnelPool, 0, sizeof(mTunnelPool));
    ExchangeMgr = NULL;
    AppState = NULL;
    mFlags = 0;

    return WEAVE_NO_ERROR;
}

#if WEAVE_CONFIG_ENABLE_TUNNELING
/**
 *  Send a tunneled IPv6 data message over UDP.
 *
 *  @param[in] msgInfo          A pointer to a WeaveMessageInfo object.
 *
 *  @param[in] destAddr         IPAddress of the UDP tunnel destination.
 *
 *  @param[in] msgBuf           A pointer to the PacketBuffer object holding the packet to send.
 *
 *  @retval  #WEAVE_NO_ERROR                    on successfully sending the message down to the network
 *                                              layer.
 *  @retval  #WEAVE_ERROR_INVALID_ADDRESS       if the destAddr is not specified or cannot be determined
 *                                              from destination node id.
 *  @retval  errors generated from the lower Inet layer UDP endpoint during sending.
 *
 */
WEAVE_ERROR WeaveMessageLayer::SendUDPTunneledMessage(const IPAddress &destAddr, WeaveMessageInfo *msgInfo, PacketBuffer *msgBuf)
{
    WEAVE_ERROR res = WEAVE_NO_ERROR;

    //Set message version to V2
    msgInfo->MessageVersion = kWeaveMessageVersion_V2;

    //Set the tunneling flag
    msgInfo->Flags |= kWeaveMessageFlag_TunneledData;

    res = SendMessage(destAddr, msgInfo, msgBuf);
    msgBuf = NULL;

    return res;
}
#endif // WEAVE_CONFIG_ENABLE_TUNNELING

/**
 *  Encode a Weave Message layer header into an PacketBuffer.
 *
 *  @param[in]    destAddr      The destination IP Address.
 *
 *  @param[in]    destPort      The destination port.
 *
 *  @param[in]    sendIntId     The interface on which to send the Weave message.
 *
 *  @param[in]    msgInfo       A pointer to a WeaveMessageInfo object.
 *
 *  @param[in]    payload       A pointer to the PacketBuffer object that would hold the Weave message.
 *
 *  @retval  #WEAVE_NO_ERROR                           on successful encoding of the Weave message.
 *  @retval  #WEAVE_ERROR_UNSUPPORTED_MESSAGE_VERSION  if the Weave Message version is not supported.
 *  @retval  #WEAVE_ERROR_INVALID_MESSAGE_LENGTH       if the payload length in the message buffer is zero.
 *  @retval  #WEAVE_ERROR_UNSUPPORTED_ENCRYPTION_TYPE  if the encryption type is not supported.
 *  @retval  #WEAVE_ERROR_MESSAGE_TOO_LONG             if the encoded message would be longer than the
 *                                                     requested maximum.
 *  @retval  #WEAVE_ERROR_BUFFER_TOO_SMALL             if there is not enough space before or after the
 *                                                     message payload.
 *  @retval  other errors generated by the fabric state object when fetching the session state.
 *
 */
WEAVE_ERROR WeaveMessageLayer::EncodeMessage(const IPAddress &destAddr, uint16_t destPort, InterfaceId sendIntId,
                                             WeaveMessageInfo *msgInfo, PacketBuffer *payload)
{
    WEAVE_ERROR res = WEAVE_NO_ERROR;

    // Set the source node identifier in the message header.
    if ((msgInfo->Flags & kWeaveMessageFlag_ReuseSourceId) == 0)
        msgInfo->SourceNodeId = FabricState->LocalNodeId;

    // Force inclusion of the source node identifier if the destination address is not a local fabric address.
    //
    // Technically it should be possible to omit the source node identifier in other situations beyond the
    // ones allowed for here.  However it is difficult to determine exactly what the source IP
    // address will be when sending a UDP packet, so we err on the side of correctness and only omit
    // the source identifier if we're part of a fabric and sending to another member of the same fabric.
    if (!FabricState->IsFabricAddress(destAddr))
        msgInfo->Flags |= kWeaveMessageFlag_SourceNodeId;

    // Force the destination node identifier to be included if it doesn't match the interface identifier in
    // the destination address.
    if (!destAddr.IsIPv6ULA() || IPv6InterfaceIdToWeaveNodeId(destAddr.InterfaceId()) != msgInfo->DestNodeId)
        msgInfo->Flags |= kWeaveMessageFlag_DestNodeId;

    // Encode the Weave message. NOTE that this results in the payload buffer containing the entire encoded message.
    res = EncodeMessage(msgInfo, payload, NULL, UINT16_MAX, 0);

    return res;
}

/**
 *  Send a Weave message using the underlying Inetlayer UDP endpoint after encoding it.
 *
 *  @note
 *    The destination port used is #WEAVE_PORT.
 *
 *  @param[in]    msgInfo       A pointer to a WeaveMessageInfo object containing information
 *                              about the message to be sent.
 *
 *  @param[in]    payload       A pointer to the PacketBuffer object holding the
 *                              encoded Weave message.
 *
 *  @retval  #WEAVE_NO_ERROR    on successfully sending the message down to the network layer.
 *  @retval  errors generated from the lower Inet layer UDP endpoint during sending.
 *
 */
WEAVE_ERROR WeaveMessageLayer::SendMessage(WeaveMessageInfo *msgInfo, PacketBuffer *payload)
{
    return SendMessage(IPAddress::Any, msgInfo, payload);
}

/**
 *  Send a Weave message using the underlying Inetlayer UDP endpoint after encoding it.
 *
 *  @note
 *    -The destination port used is #WEAVE_PORT.
 *
 *    -If the destination address has not been supplied, attempt to determine it from the node identifier in
 *     the message header. Fail if this can't be done.
 *
 *    -If the destination address is a fabric address for the local fabric, and the caller
 *     didn't specify the destination node id, extract it from the destination address.
 *
 *  @param[in]    destAddr      The destination IP Address.
 *
 *  @param[in]    msgInfo       A pointer to a WeaveMessageInfo object containing information
 *                              about the message to be sent.
 *
 *  @param[in]    payload       A pointer to the PacketBuffer object holding the
 *                              encoded Weave message.
 *
 *  @retval  #WEAVE_NO_ERROR    on successfully sending the message down to the network layer.
 *  @retval  errors generated from the lower Inet layer UDP endpoint during sending.
 *
 */
WEAVE_ERROR WeaveMessageLayer::SendMessage(const IPAddress &destAddr, WeaveMessageInfo *msgInfo,
                                           PacketBuffer *payload)
{
    return SendMessage(destAddr, WEAVE_PORT, INET_NULL_INTERFACEID, msgInfo, payload);
}

/**
 *  Send a Weave message using the underlying Inetlayer UDP endpoint after encoding it.
 *
 *  @note
 *    -If the destination address has not been supplied, attempt to determine it from the node identifier in
 *     the message header. Fail if this can't be done.
 *
 *    -If the destination address is a fabric address for the local fabric, and the caller
 *     didn't specify the destination node id, extract it from the destination address.
 *
 *  @param[in]    aDestAddr      The destination IP Address.
 *
 *  @param[in]    destPort      The destination port.
 *
 *  @param[in]    sendIntfId    The interface on which to send the Weave message.
 *
 *  @param[in]    msgInfo       A pointer to a WeaveMessageInfo object containing information
 *                              about the message to be sent.
 *
 *  @param[in]    payload       A pointer to the PacketBuffer object holding the
 *                              encoded Weave message.
 *
 *  @retval  #WEAVE_NO_ERROR                    on successfully sending the message down to the network
 *                                              layer.
 *  @retval  #WEAVE_ERROR_INVALID_ADDRESS       if the destAddr is not specified or cannot be determined
 *                                              from destination node id.
 *  @retval  errors generated from the lower Inet layer UDP endpoint during sending.
 *
 */
WEAVE_ERROR WeaveMessageLayer::SendMessage(const IPAddress &aDestAddr, uint16_t destPort, InterfaceId sendIntfId,
                                           WeaveMessageInfo *msgInfo, PacketBuffer *payload)
{
    WEAVE_ERROR res = WEAVE_NO_ERROR;
    IPAddress destAddr = aDestAddr;

    // Determine the message destination address based on the destination nodeId.
    res = SelectDestNodeIdAndAddress(msgInfo->DestNodeId, destAddr);
    SuccessOrExit(res);

    res = EncodeMessage(destAddr, destPort, sendIntfId, msgInfo, payload);
    SuccessOrExit(res);

    // on delay send, we do everything except actually send the
    // message.  As a result, the payload will contain the entire
    // state required for sending it a bit later
    if (msgInfo->Flags & kWeaveMessageFlag_DelaySend)
        return WEAVE_NO_ERROR;

    // Copy msg to a right-sized buffer if applicable
    payload = PacketBuffer::RightSize(payload);

    // Send the message using the appropriate UDP endpoint(s).
    return SendMessage(destAddr, destPort, sendIntfId, payload, msgInfo->Flags);

exit:
    if ((res != WEAVE_NO_ERROR) &&
        (payload != NULL) &&
        ((msgInfo->Flags & kWeaveMessageFlag_RetainBuffer) == 0))
    {
        PacketBuffer::Free(payload);
    }

    return res;
}

bool WeaveMessageLayer::IsIgnoredMulticastSendError(WEAVE_ERROR err)
{
    return err == WEAVE_NO_ERROR ||

            // Ignore routing errors. In general, these indicate that the underly interface
            // doesn't support multicast (e.g. the loopback interface on linux) or that the
            // inteface isn't fully configured (e.g. we're bound to an address on the interface
            // but the address hasn't finished DAD).
#if WEAVE_SYSTEM_CONFIG_USE_LWIP
           err == System::MapErrorLwIP(ERR_RTE)
#else
           err == System::MapErrorPOSIX(ENETUNREACH) || err == System::MapErrorPOSIX(EADDRNOTAVAIL)
#endif
           ;
}

/**
 *  Checks if error, while sending, is critical enough to report to the application.
 *
 *  @param[in]    err      The #WEAVE_ERROR being checked for criticality.
 *
 *  @return    true if the error is NOT critical; false otherwise.
 *
 */
bool WeaveMessageLayer::IsSendErrorNonCritical (WEAVE_ERROR err)
{
    return (err == INET_ERROR_NOT_IMPLEMENTED || err == INET_ERROR_OUTBOUND_MESSAGE_TRUNCATED ||
            err == INET_ERROR_MESSAGE_TOO_LONG || err == INET_ERROR_NO_MEMORY ||
            WEAVE_CONFIG_IsPlatformErrorNonCritical(err));
}

/**
 *  Send an encoded Weave message using the appropriate underlying Inetlayer UDPEndPoint (or EndPoints).
 *
 *  @param[in]    destAddr      The destination IP Address.
 *
 *  @param[in]    destPort      The destination port.
 *
 *  @param[in]    sendIntfId    The interface on which to send the Weave message.
 *
 *  @param[in]    payload       A pointer to the PacketBuffer object holding the encoded Weave message.
 *
 *  @param[in]    msgSendFlags  Send flags containing metadata about the message for the lower Inet layer.
 *
 *  @retval  #WEAVE_NO_ERROR    on successfully sending the message down to the network layer.
 *  @retval  errors generated from the lower Inet layer UDP endpoint during sending.
 *
 */
WEAVE_ERROR WeaveMessageLayer::SendMessage(const IPAddress &destAddr, uint16_t destPort, InterfaceId sendIntfId,
                                           PacketBuffer *payload, uint16_t msgSendFlags)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WEAVE_ERROR mcastSendErr = WEAVE_NO_ERROR;
    uint16_t udpSendFlags = (msgSendFlags & kWeaveMessageFlag_RetainBuffer) ? UDPEndPoint::kSendFlag_RetainBuffer : 0;
    UDPEndPoint* lUDP;
    bool lIsNotIPv6Multicast;

    //Check if drop flag is set; If so, do not send message; return WEAVE_NO_ERROR;
    VerifyOrExit(!mDropMessage, err = WEAVE_NO_ERROR);

    // Drop the message and return. Free the buffer if it does not need to be
    // retained(e.g., for WRM retransmissions).
    WEAVE_FAULT_INJECT(FaultInjection::kFault_DropOutgoingUDPMsg,
            ExitNow(err = WEAVE_NO_ERROR);
            );

#if INET_CONFIG_ENABLE_IPV4
    if (destAddr.IsIPv4())
    {
        lUDP = mIPv4UDP;
        lIsNotIPv6Multicast = true;
    }
    else
    {
        lUDP = mIPv6UDP;
        lIsNotIPv6Multicast = !destAddr.IsMulticast();
    }
#else // !INET_CONFIG_ENABLE_IPV4
    lUDP = mIPv6UDP;
    lIsNotIPv6Multicast = !destAddr.IsMulticast();
#endif // !INET_CONFIG_ENABLE_IPV4

    VerifyOrExit(lUDP != NULL, err = WEAVE_ERROR_NO_ENDPOINT);

    // If sending to a unicast IPv6 destination or an IPv4 destination
    if (lIsNotIPv6Multicast)
    {
        // Send use the general-purpose IPv6 endpoint
        err = lUDP->SendTo(destAddr, WEAVE_PORT, sendIntfId, payload, udpSendFlags);
        payload = NULL;
    }

    // Otherwise we're sending to a multicast IPv6 destination...
    else
    {
        // Since we will be sending over multiple endpoints, ensure that the Inet layer code makes
        // a copy of the message when sending.  We'll take care of freeing the original when we're
        // done.
        udpSendFlags |= UDPEndPoint::kSendFlag_RetainBuffer;

        // If requested, send the multicast message over all interfaces using the appropriate IPv6
        // source link-local addresses for each interface...
        //
        // NOTE: In the case where we are configured to use a specific listening address (i.e.
        // FabricState->ListenIPv6Addr != IPAddress::Any) this code will actually end up sending
        // the message using the listening address as the source address. Since specifying a
        // listening address is primarily used for simulating multiple Weave nodes on a single
        // host, and there's no reasonable way for multiple nodes to share a single link-local
        // address, this limitation is deemed acceptable.
        //
        if ((IsBoundToLocalIPv6Address()) || (msgSendFlags & kWeaveMessageFlag_MulticastFromLinkLocal) != 0)
        {
            // Send the message over each local interface or the interface passed as argument
            // using the link-local address of the interface as the src address.
            if (sendIntfId == INET_NULL_INTERFACEID)
            {
                for (uint16_t i = 0; i < WEAVE_CONFIG_MAX_INTERFACES; i++)
                {
                    if (mInterfaces[i] != INET_NULL_INTERFACEID)
                    {
                        mcastSendErr = lUDP->SendTo(destAddr, WEAVE_PORT, mInterfaces[i], payload, udpSendFlags);
                        if (!IsIgnoredMulticastSendError(mcastSendErr))
                        {
                            err = mcastSendErr;
                        }
                    }
                }
            }
            else
            {
                mcastSendErr = lUDP->SendTo(destAddr, WEAVE_PORT, sendIntfId, payload, udpSendFlags);
                if (!IsIgnoredMulticastSendError(mcastSendErr))
                    err = mcastSendErr;

            }
        }

        // Otherwise, send the multicast message over all interfaces, generating a distinct message for each
        // bound address assigned to the interface...
        else
        {
            // Send the message over each interface or the interface passed as argument
            // using a ULA address as the src address.
            for (uint16_t i = 0; i < WEAVE_CONFIG_MAX_LOCAL_ADDR_UDP_ENDPOINTS; i++)
            {
                if (mIPv6UDPLocalAddr[i] != NULL)
                {
                    if (sendIntfId == INET_NULL_INTERFACEID)
                    {
                        mcastSendErr = mIPv6UDPLocalAddr[i]->SendTo(destAddr, WEAVE_PORT, sendIntfId, payload, udpSendFlags);
                        if (!IsIgnoredMulticastSendError(mcastSendErr))
                        {
                            err = mcastSendErr;
                        }

                    }
                    else
                    {
                        if (sendIntfId == mIPv6UDPLocalAddr[i]->GetBoundInterface())
                        {
                            mcastSendErr = mIPv6UDPLocalAddr[i]->SendTo(destAddr, WEAVE_PORT, sendIntfId, payload, udpSendFlags);
                            if (!IsIgnoredMulticastSendError(mcastSendErr))
                            {
                                err = mcastSendErr;
                            }
                            break;
                        }
                    }
                }
            }
        }
    }

exit:
    if (payload != NULL && (msgSendFlags & kWeaveMessageFlag_RetainBuffer) == 0)
        PacketBuffer::Free(payload);
    return err;
}

/**
 *  Resend an encoded Weave message using the underlying Inetlayer UDP endpoint.
 *
 *  @param[in]    msgInfo     A pointer to the WeaveMessageInfo object.
 *
 *  @param[in]    payload       A pointer to the PacketBuffer object holding the encoded Weave message.
 *
 *  @retval  #WEAVE_NO_ERROR    on successfully sending the message down to the network layer.
 *  @retval  errors generated from the lower Inet layer UDP endpoint during sending.
 *
 */
WEAVE_ERROR WeaveMessageLayer::ResendMessage(WeaveMessageInfo *msgInfo, PacketBuffer *payload)
{
    IPAddress destAddr = IPAddress::Any;
    return ResendMessage(destAddr, msgInfo, payload);
}

/**
 *  Resend an encoded Weave message using the underlying Inetlayer UDP endpoint.
 *
 *  @note
 *    The destination port used is #WEAVE_PORT.
 *
 *  @param[in]    destAddr      The destination IP Address.
 *
 *  @param[in]    msgInfo       A pointer to the WeaveMessageInfo object.
 *
 *  @param[in]    payload       A pointer to the PacketBuffer object holding the encoded Weave message.
 *
 *  @retval  #WEAVE_NO_ERROR    on successfully sending the message down to the network layer.
 *  @retval  errors generated from the lower Inet layer UDP endpoint during sending.
 *
 */
WEAVE_ERROR WeaveMessageLayer::ResendMessage(const IPAddress &destAddr, WeaveMessageInfo *msgInfo, PacketBuffer *payload)
{
    return ResendMessage(destAddr, WEAVE_PORT, msgInfo, payload);
}

/**
 *  Resend an encoded Weave message using the underlying Inetlayer UDP endpoint.
 *
 *  @param[in]    destAddr      The destination IP Address.
 *
 *  @param[in]    destPort      The destination port.
 *
 *  @param[in]    msgInfo       A pointer to the WeaveMessageInfo object.
 *
 *  @param[in]    payload       A pointer to the PacketBuffer object holding the encoded Weave message.
 *
 *  @retval  #WEAVE_NO_ERROR    on successfully sending the message down to the network layer.
 *  @retval  errors generated from the lower Inet layer UDP endpoint during sending.
 *
 */
WEAVE_ERROR WeaveMessageLayer::ResendMessage(const IPAddress &destAddr, uint16_t destPort, WeaveMessageInfo *msgInfo, PacketBuffer *payload)
{
    return ResendMessage(destAddr, WEAVE_PORT, INET_NULL_INTERFACEID, msgInfo, payload);
}

/**
 *  Resend an encoded Weave message using the underlying Inetlayer UDP endpoint.
 *
 *  @note
 *    -If the destination address has not been supplied, attempt to determine it from the node identifier in
 *     the message header. Fail if this can't be done.
 *
 *    -If the destination address is a fabric address for the local fabric, and the caller
 *     didn't specify the destination node id, extract it from the destination address.
 *
 *  @param[in]    aDestAddr      The destination IP Address.
 *
 *  @param[in]    destPort      The destination port.
 *
 *  @param[in]    interfaceId   The interface on which to send the Weave message.
 *
 *  @param[in]    msgInfo       A pointer to the WeaveMessageInfo object.
 *
 *  @param[in]    payload       A pointer to the PacketBuffer object holding the encoded Weave message.
 *
 *  @retval  #WEAVE_NO_ERROR    on successfully sending the message down to the network layer.
 *  @retval  errors generated from the lower Inet layer UDP endpoint during sending.
 *
 */
WEAVE_ERROR WeaveMessageLayer::ResendMessage(const IPAddress &aDestAddr, uint16_t destPort, InterfaceId interfaceId,
                                             WeaveMessageInfo *msgInfo, PacketBuffer *payload)
{
    WEAVE_ERROR res = WEAVE_NO_ERROR;
    IPAddress destAddr = aDestAddr;

    res = SelectDestNodeIdAndAddress(msgInfo->DestNodeId, destAddr);
    SuccessOrExit(res);

    return SendMessage(destAddr, destPort, interfaceId, payload, msgInfo->Flags);
exit:
    if ((res != WEAVE_NO_ERROR) &&
        (payload != NULL) &&
        ((msgInfo->Flags & kWeaveMessageFlag_RetainBuffer) == 0))
    {
        PacketBuffer::Free(payload);
    }
    return res;
}

/**
 *  Get the number of WeaveConnections in use and the size of the pool
 *
 *  @param[out]  aOutInUse  Reference to size_t, in which the number of
 *                         connections in use is stored.
 *
 */
void WeaveMessageLayer::GetConnectionPoolStats(nl::Weave::System::Stats::count_t &aOutInUse) const
{
    aOutInUse = 0;

    const WeaveConnection *con = (WeaveConnection *) mConPool;
    for (int i = 0; i < WEAVE_CONFIG_MAX_CONNECTIONS; i++, con++)
    {
        if (con->mRefCount != 0)
        {
            aOutInUse++;
        }
    }
}

/**
 *  Create a new WeaveConnection object from a pool.
 *
 *  @return  a pointer to the newly created WeaveConnection object if successful, otherwise
 *           NULL.
 *
 */
WeaveConnection *WeaveMessageLayer::NewConnection()
{
    WeaveConnection *con = (WeaveConnection *) mConPool;
    for (int i = 0; i < WEAVE_CONFIG_MAX_CONNECTIONS; i++, con++)
    {
        if (con->mRefCount == 0)
        {
            con->Init(this);
            return con;
        }
    }

    WeaveLogError(ExchangeManager, "New con FAILED");
    return NULL;
}

/**
 *  Create a new WeaveConnectionTunnel object from a pool.
 *
 *  @return  a pointer to the newly created WeaveConnectionTunnel object if successful,
 *           otherwise NULL.
 *
 */
WeaveConnectionTunnel *WeaveMessageLayer::NewConnectionTunnel()
{
    WeaveConnectionTunnel *tun = (WeaveConnectionTunnel *) mTunnelPool;
    for (int i = 0; i < WEAVE_CONFIG_MAX_TUNNELS; i++, tun++)
    {
        if (tun->IsInUse() == false)
        {
            tun->Init(this);
            return tun;
        }
    }

    WeaveLogError(ExchangeManager, "New tun FAILED");
    return NULL;
}

/**
 *  Create a WeaveConnectionTunnel by coupling together two specified WeaveConnections.
    On successful creation, the TCPEndPoints corresponding to the component WeaveConnection
    objects are handed over to the WeaveConnectionTunnel, otherwise the WeaveConnections are
    closed.
 *
 *  @param[out]    tunPtr                 A pointer to pointer of a WeaveConnectionTunnel object.
 *
 *  @param[in]     conOne                 A reference to the first WeaveConnection object.
 *
 *  @param[in]     conTwo                 A reference to the second WeaveConnection object.
 *
 *  @param[in]     inactivityTimeoutMS    The maximum time in milliseconds that the Weave
 *                                        connection tunnel could be idle.
 *
 *  @retval    #WEAVE_NO_ERROR            on successful creation of the WeaveConnectionTunnel.
 *  @retval    #WEAVE_ERROR_INCORRECT_STATE if the component WeaveConnection objects of the
 *                                          WeaveConnectionTunnel is not in the correct state.
 *  @retval    #WEAVE_ERROR_NO_MEMORY       if a new WeaveConnectionTunnel object cannot be created.
 *
 */
WEAVE_ERROR WeaveMessageLayer::CreateTunnel(WeaveConnectionTunnel **tunPtr, WeaveConnection &conOne,
        WeaveConnection &conTwo, uint32_t inactivityTimeoutMS)
{
    WeaveLogDetail(ExchangeManager, "Entering CreateTunnel");
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(conOne.State == WeaveConnection::kState_Connected && conTwo.State ==
            WeaveConnection::kState_Connected, err = WEAVE_ERROR_INCORRECT_STATE);

    *tunPtr = NewConnectionTunnel();
    VerifyOrExit(*tunPtr != NULL, err = WEAVE_ERROR_NO_MEMORY);

    // Form WeaveConnectionTunnel from former WeaveConnections' TCPEndPoints.
    err = (*tunPtr)->MakeTunnelConnected(conOne.mTcpEndPoint, conTwo.mTcpEndPoint);
    SuccessOrExit(err);

    WeaveLogProgress(ExchangeManager, "Created Weave tunnel from Cons (%04X, %04X) with EPs (%04X, %04X)",
            conOne.LogId(), conTwo.LogId(), conOne.mTcpEndPoint->LogId(), conTwo.mTcpEndPoint->LogId());

    if (inactivityTimeoutMS > 0)
    {
        // Set TCPEndPoint inactivity timeouts.
        conOne.mTcpEndPoint->SetIdleTimeout(inactivityTimeoutMS);
        conTwo.mTcpEndPoint->SetIdleTimeout(inactivityTimeoutMS);
    }

    // Remove TCPEndPoints from WeaveConnections now that we've handed the former to our new WeaveConnectionTunnel.
    conOne.mTcpEndPoint = NULL;
    conTwo.mTcpEndPoint = NULL;

exit:
    WeaveLogDetail(ExchangeManager, "Exiting CreateTunnel");

    // Close WeaveConnection args.
    conOne.Close(true);
    conTwo.Close(true);

    return err;
}

WEAVE_ERROR WeaveMessageLayer::SetUnsecuredConnectionListener(ConnectionReceiveFunct
        newOnUnsecuredConnectionReceived, CallbackRemovedFunct newOnUnsecuredConnectionCallbacksRemoved, bool force,
        void *listenerState)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    WeaveLogProgress(ExchangeManager, "Entered SetUnsecuredConnectionReceived, cb = %p, %p",
            newOnUnsecuredConnectionReceived, newOnUnsecuredConnectionCallbacksRemoved);

    if (IsUnsecuredListenEnabled() == false)
    {
        err = EnableUnsecuredListen();
        SuccessOrExit(err);
    }

    // New OnUnsecuredConnectionReceived cannot be null. To clear, use ClearOnUnsecuredConnectionReceived().
    VerifyOrExit(newOnUnsecuredConnectionReceived != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    if (OnUnsecuredConnectionReceived != NULL)
    {
        if (force == false)
        {
            err = WEAVE_ERROR_INCORRECT_STATE;
            ExitNow();
        }
        else if (OnUnsecuredConnectionCallbacksRemoved != NULL)
        {
            // Notify application that its previous OnUnsecuredConnectionReceived callback has been removed.
            OnUnsecuredConnectionCallbacksRemoved(UnsecuredConnectionReceivedAppState);
        }
    }

    OnUnsecuredConnectionReceived = newOnUnsecuredConnectionReceived;
    OnUnsecuredConnectionCallbacksRemoved = newOnUnsecuredConnectionCallbacksRemoved;
    UnsecuredConnectionReceivedAppState = listenerState;

exit:
    return err;
}

WEAVE_ERROR WeaveMessageLayer::ClearUnsecuredConnectionListener(ConnectionReceiveFunct
        oldOnUnsecuredConnectionReceived, CallbackRemovedFunct oldOnUnsecuredConnectionCallbacksRemoved)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    WeaveLogProgress(ExchangeManager, "Entered ClearUnsecuredConnectionListener, cbs = %p, %p",
            oldOnUnsecuredConnectionReceived, oldOnUnsecuredConnectionCallbacksRemoved);

    // Only clear callbacks and suppress OnUnsecuredConnectionCallbacksRemoved if caller can prove it owns current
    // callbacks. For proof of identification, we accept copies of callback function pointers.
    if (oldOnUnsecuredConnectionReceived != OnUnsecuredConnectionReceived || oldOnUnsecuredConnectionCallbacksRemoved
            != OnUnsecuredConnectionCallbacksRemoved)
    {
        if (oldOnUnsecuredConnectionReceived != OnUnsecuredConnectionReceived)
            WeaveLogError(ExchangeManager, "bad arg: OnUnsecuredConnectionReceived");
        else
            WeaveLogError(ExchangeManager, "bad arg: OnUnsecuredConnectionCallbacksRemoved");
        err = WEAVE_ERROR_INVALID_ARGUMENT;
        ExitNow();
    }

    if (IsUnsecuredListenEnabled() == true)
    {
        err = DisableUnsecuredListen();
        SuccessOrExit(err);
    }

    OnUnsecuredConnectionReceived = NULL;
    OnUnsecuredConnectionCallbacksRemoved = NULL;
    UnsecuredConnectionReceivedAppState = NULL;

exit:
    return err;
}

WEAVE_ERROR WeaveMessageLayer::SelectDestNodeIdAndAddress(uint64_t& destNodeId, IPAddress& destAddr)
{
    // If the destination address has not been supplied, attempt to determine it from the node id.
    // Fail if this can't be done.
    if (destAddr == IPAddress::Any)
    {
        destAddr = FabricState->SelectNodeAddress(destNodeId);
        if (destAddr == IPAddress::Any)
            return WEAVE_ERROR_INVALID_ADDRESS;
    }

    // If the destination address is a fabric address for the local fabric, and the caller
    // didn't specify the destination node id, extract it from the destination address.
    if (FabricState->IsFabricAddress(destAddr) && destNodeId == kNodeIdNotSpecified)
        destNodeId = IPv6InterfaceIdToWeaveNodeId(destAddr.InterfaceId());

    return WEAVE_NO_ERROR;
}

// Encode and return message header field value.
static uint16_t EncodeHeaderField(const WeaveMessageInfo *msgInfo)
{
    return ((((uint16_t)msgInfo->Flags) << kMsgHeaderField_FlagsShift) & kMsgHeaderField_FlagsMask) |
           ((((uint16_t)msgInfo->EncryptionType) << kMsgHeaderField_EncryptionTypeShift) & kMsgHeaderField_EncryptionTypeMask) |
           ((((uint16_t)msgInfo->MessageVersion) << kMsgHeaderField_MessageVersionShift) & kMsgHeaderField_MessageVersionMask);
}

// Decode message header field value.
static void DecodeHeaderField(const uint16_t headerField, WeaveMessageInfo *msgInfo)
{
    msgInfo->Flags = (uint16_t)((headerField & kMsgHeaderField_FlagsMask) >> kMsgHeaderField_FlagsShift);
    msgInfo->EncryptionType = (uint8_t)((headerField & kMsgHeaderField_EncryptionTypeMask) >> kMsgHeaderField_EncryptionTypeShift);
    msgInfo->MessageVersion = (uint8_t)((headerField & kMsgHeaderField_MessageVersionMask) >> kMsgHeaderField_MessageVersionShift);
}

/**
 *  Decode a Weave Message layer header from a received Weave message.
 *
 *  @param[in]    msgBuf        A pointer to the PacketBuffer object holding the Weave message.
 *
 *  @param[in]    msgInfo       A pointer to a WeaveMessageInfo object which will receive information
 *                              about the message.
 *
 *  @param[out]   payloadStart  A pointer to a pointer to the position in the message buffer after
 *                              decoding is complete.
 *
 *  @retval  #WEAVE_NO_ERROR    On successful decoding of the message header.
 *  @retval  #WEAVE_ERROR_INVALID_MESSAGE_LENGTH
 *                              If the message buffer passed is of invalid length.
 *  @retval  #WEAVE_ERROR_UNSUPPORTED_MESSAGE_VERSION
 *                              If the Weave Message header format version is not supported.
 *
 */
WEAVE_ERROR WeaveMessageLayer::DecodeHeader(PacketBuffer *msgBuf, WeaveMessageInfo *msgInfo, uint8_t **payloadStart)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint8_t *msgStart = msgBuf->Start();
    uint16_t msgLen = msgBuf->DataLength();
    uint8_t *msgEnd = msgStart + msgLen;
    uint8_t *p = msgStart;
    uint16_t headerField;

    if (msgLen < 6)
    {
        ExitNow(err = WEAVE_ERROR_INVALID_MESSAGE_LENGTH);
    }

    // Read and verify the header field.
    headerField = LittleEndian::Read16(p);
    VerifyOrExit((headerField & kMsgHeaderField_ReservedFlagsMask) == 0, err = WEAVE_ERROR_INVALID_MESSAGE_FLAG);

    // Decode the header field.
    DecodeHeaderField(headerField, msgInfo);

    // Error if the message version is unsupported.
    if (msgInfo->MessageVersion != kWeaveMessageVersion_V1 &&
        msgInfo->MessageVersion != kWeaveMessageVersion_V2)
    {
        ExitNow(err = WEAVE_ERROR_UNSUPPORTED_MESSAGE_VERSION);
    }

    // Decode the message id.
    msgInfo->MessageId = LittleEndian::Read32(p);

    // Decode the source node identifier if included in the message.
    if (msgInfo->Flags & kWeaveMessageFlag_SourceNodeId)
    {
        if ((p + 8) > msgEnd)
        {
            ExitNow(err = WEAVE_ERROR_INVALID_MESSAGE_LENGTH);
        }
        msgInfo->SourceNodeId = LittleEndian::Read64(p);
    }

    // Decode the destination node identifier if included in the message.
    if (msgInfo->Flags & kWeaveMessageFlag_DestNodeId)
    {
        if ((p + 8) > msgEnd)
        {
            ExitNow(err = WEAVE_ERROR_INVALID_MESSAGE_LENGTH);
        }
        msgInfo->DestNodeId = LittleEndian::Read64(p);
    }
    else
        // TODO: This is wrong. If not specified in the message, the destination node identifier must be
        // derived from destination IPv6 address to which the message was sent.  This is relatively
        // easy to determine for messages received over TCP (specifically by the inspecting the local
        // address of the connection). However it is much harder for UDP (no support in LwIP; requires
        // use of IP_PKTINFO socket option in sockets). For now we just assume the intended destination
        // is the local node.
        msgInfo->DestNodeId = FabricState->LocalNodeId;
    // Decode the encryption key identifier if present.
    if (msgInfo->EncryptionType != kWeaveEncryptionType_None)
    {
        if ((p + kKeyIdLen) > msgEnd)
        {
            ExitNow(err = WEAVE_ERROR_INVALID_MESSAGE_LENGTH);
        }
        msgInfo->KeyId = LittleEndian::Read16(p);
    }
    else
    {
        // Clear flag, which could have been accidentally set in the older version of code only for unencrypted messages.
        msgInfo->Flags &= ~kWeaveMessageFlag_MsgCounterSyncReq;

        msgInfo->KeyId = WeaveKeyId::kNone;
    }

    if (payloadStart != NULL)
    {
        *payloadStart = p;
    }

exit:
    return err;
}

WEAVE_ERROR WeaveMessageLayer::ReEncodeMessage(PacketBuffer *msgBuf)
{
    WeaveMessageInfo msgInfo;
    WEAVE_ERROR err;
    uint8_t *p;
    WeaveSessionState sessionState;
    uint16_t msgLen = msgBuf->DataLength();
    uint8_t *msgStart = msgBuf->Start();
    uint16_t encryptionLen;

    msgInfo.Clear();
    msgInfo.SourceNodeId = kNodeIdNotSpecified;

    err = DecodeHeader(msgBuf, &msgInfo, &p);
    if (err != WEAVE_NO_ERROR)
        return err;

    encryptionLen = msgLen - (p - msgStart);

    err = FabricState->GetSessionState(msgInfo.SourceNodeId, msgInfo.KeyId, msgInfo.EncryptionType, NULL, sessionState);
    if (err != WEAVE_NO_ERROR)
        return err;

    switch (msgInfo.EncryptionType)
    {
    case kWeaveEncryptionType_None:
        break;

    case kWeaveEncryptionType_AES128CTRSHA1:
        {
            // TODO: re-validate MIC to ensure that no part of the message has been altered since the time it was received.

            // Re-encrypt the payload.
            AES128CTRMode aes128CTR;
            aes128CTR.SetKey(sessionState.MsgEncKey->EncKey.AES128CTRSHA1.DataKey);
            aes128CTR.SetWeaveMessageCounter(msgInfo.SourceNodeId, msgInfo.MessageId);
            aes128CTR.EncryptData(p, encryptionLen, p);
        }
        break;
    default:
        return WEAVE_ERROR_UNSUPPORTED_ENCRYPTION_TYPE;
    }

    // signature remains untouched -- we have not modified it.

    return WEAVE_NO_ERROR;
}

/**
 *  Encode a WeaveMessageLayer header into an PacketBuffer.
 *
 *  @param[in]    msgInfo       A pointer to a WeaveMessageInfo object containing information
 *                              about the message to be encoded.
 *
 *  @param[in]    msgBuf        A pointer to the PacketBuffer object that would hold the Weave message.
 *
 *  @param[in]    con           A pointer to the WeaveConnection object.
 *
 *  @param[in]    maxLen        The maximum length of the encoded Weave message.
 *
 *  @param[in]    reserve       The reserved space before the payload to hold the Weave message header.
 *
 *  @retval  #WEAVE_NO_ERROR    on successful encoding of the message.
 *  @retval  #WEAVE_ERROR_UNSUPPORTED_MESSAGE_VERSION  if the Weave Message header format version is
 *                                                     not supported.
 *  @retval  #WEAVE_ERROR_INVALID_MESSAGE_LENGTH       if the payload length in the message buffer is zero.
 *  @retval  #WEAVE_ERROR_UNSUPPORTED_ENCRYPTION_TYPE  if the encryption type in the message header is not
 *                                                     supported.
 *  @retval  #WEAVE_ERROR_MESSAGE_TOO_LONG             if the encoded message would be longer than the
 *                                                     requested maximum.
 *  @retval  #WEAVE_ERROR_BUFFER_TOO_SMALL             if there is not enough space before or after the
 *                                                     message payload.
 *  @retval  other errors generated by the fabric state object when fetching the session state.
 *
 */
WEAVE_ERROR WeaveMessageLayer::EncodeMessage(WeaveMessageInfo *msgInfo, PacketBuffer *msgBuf, WeaveConnection *con,
        uint16_t maxLen, uint16_t reserve)
{
    WEAVE_ERROR err;
    uint8_t *p1;
    // Error if an unsupported message version requested.
    if (msgInfo->MessageVersion != kWeaveMessageVersion_V1 &&
        msgInfo->MessageVersion != kWeaveMessageVersion_V2)
        return WEAVE_ERROR_UNSUPPORTED_MESSAGE_VERSION;

    // Message already encoded, don't do anything
    if (msgInfo->Flags & kWeaveMessageFlag_MessageEncoded)
    {
        WeaveMessageInfo existingMsgInfo;
        existingMsgInfo.Clear();
        err = DecodeHeader(msgBuf, &existingMsgInfo, &p1);
        if (err != WEAVE_NO_ERROR)
        {
            return err;
        }
        msgInfo->DestNodeId = existingMsgInfo.DestNodeId;
        return WEAVE_NO_ERROR;
    }

    // Compute the number of bytes that will appear before and after the message payload
    // in the final encoded message.
    uint16_t headLen = 6;
    uint16_t tailLen = 0;
    uint16_t payloadLen = msgBuf->DataLength();
    if (msgInfo->Flags & kWeaveMessageFlag_SourceNodeId)
        headLen += 8;
    if (msgInfo->Flags & kWeaveMessageFlag_DestNodeId)
        headLen += 8;
    switch (msgInfo->EncryptionType)
    {
    case kWeaveEncryptionType_None:
        break;
    case kWeaveEncryptionType_AES128CTRSHA1:
        // Can only encrypt non-zero length payloads.
        if (payloadLen == 0)
            return WEAVE_ERROR_INVALID_MESSAGE_LENGTH;
        headLen += 2;
        tailLen += HMACSHA1::kDigestLength;
        break;
    default:
        return WEAVE_ERROR_UNSUPPORTED_ENCRYPTION_TYPE;
    }

    // Error if the encoded message would be longer than the requested maximum.
    if ((headLen + msgBuf->DataLength() + tailLen) > maxLen)
        return WEAVE_ERROR_MESSAGE_TOO_LONG;

    // Ensure there's enough room before the payload to hold the message header.
    // Return an error if there's not enough room in the buffer.
    if (!msgBuf->EnsureReservedSize(headLen + reserve))
        return WEAVE_ERROR_BUFFER_TOO_SMALL;

    // Error if not enough space after the message payload.
    if ((msgBuf->DataLength() + tailLen) > msgBuf->MaxDataLength())
        return WEAVE_ERROR_BUFFER_TOO_SMALL;

    uint8_t *payloadStart = msgBuf->Start();

    // Get the session state for the given destination node and encryption key.
    WeaveSessionState sessionState;

    if (msgInfo->DestNodeId == kAnyNodeId)
    {
        err = FabricState->GetSessionState(msgInfo->SourceNodeId, msgInfo->KeyId, msgInfo->EncryptionType, con, sessionState);
    }
    else
    {
        err = FabricState->GetSessionState(msgInfo->DestNodeId, msgInfo->KeyId, msgInfo->EncryptionType, con, sessionState);
    }
    if (err != WEAVE_NO_ERROR)
        return err;

    // Starting encoding at the appropriate point in the buffer before the payload data.
    uint8_t *p = payloadStart - headLen;

    // Allocate a new message identifier and write the message identifier field.
    if ((msgInfo->Flags & kWeaveMessageFlag_ReuseMessageId) == 0)
        msgInfo->MessageId = sessionState.NewMessageId();

#if WEAVE_CONFIG_USE_APP_GROUP_KEYS_FOR_MSG_ENC
    // Request message counter synchronization if peer group key counter is not synchronized.
    if (sessionState.MessageIdNotSynchronized() && WeaveKeyId::IsAppGroupKey(msgInfo->KeyId))
    {
        // Set the flag.
        msgInfo->Flags |= kWeaveMessageFlag_MsgCounterSyncReq;

        // Update fabric state.
        FabricState->OnMsgCounterSyncReqSent(msgInfo->MessageId);
    }
#endif

    // Adjust the buffer so that the start points to the start of the encoded message.
    msgBuf->SetStart(p);

    // Encode and verify the header field.
    uint16_t headerField = EncodeHeaderField(msgInfo);
    if ((headerField & kMsgHeaderField_ReservedFlagsMask) != 0)
        return WEAVE_ERROR_INVALID_ARGUMENT;

    // Write the header field.
    LittleEndian::Write16(p, headerField);

    if (msgInfo->DestNodeId == kAnyNodeId)
    {
        sessionState.IsDuplicateMessage(msgInfo->MessageId);
    }

    LittleEndian::Write32(p, msgInfo->MessageId);

    // If specified, encode the source node id.
    if (msgInfo->Flags & kWeaveMessageFlag_SourceNodeId)
    {
        LittleEndian::Write64(p, msgInfo->SourceNodeId);
    }

    // If specified, encode the destination node id.
    if (msgInfo->Flags & kWeaveMessageFlag_DestNodeId)
    {
        LittleEndian::Write64(p, msgInfo->DestNodeId);
    }

    switch (msgInfo->EncryptionType)
    {
    case kWeaveEncryptionType_None:
        // If no encryption requested, skip over the payload in the message buffer.
        p += payloadLen;
        break;

    case kWeaveEncryptionType_AES128CTRSHA1:
        // Encode the key id.
        LittleEndian::Write16(p, msgInfo->KeyId);

        // At this point we've completed encoding the head of the message (and therefore p == payloadStart),
        // so skip over the payload data.
        p += payloadLen;

        // Compute the integrity check value and store it immediately after the payload data.
        ComputeIntegrityCheck_AES128CTRSHA1(msgInfo, sessionState.MsgEncKey->EncKey.AES128CTRSHA1.IntegrityKey,
                                            payloadStart, payloadLen, p);
        p += HMACSHA1::kDigestLength;

        // Encrypt the message payload and the integrity check value that follows it, in place, in the message buffer.
        Encrypt_AES128CTRSHA1(msgInfo, sessionState.MsgEncKey->EncKey.AES128CTRSHA1.DataKey,
                              payloadStart, payloadLen + HMACSHA1::kDigestLength, payloadStart);

        break;
    }

    msgInfo->Flags |= kWeaveMessageFlag_MessageEncoded;
    // Update the buffer length to reflect the entire encoded message.
    msgBuf->SetDataLength(headLen + payloadLen + tailLen);

    // We update the cursor (p) out of good hygiene,
    // such that if the code is extended in the future such that the cursor is used,
    // it will be in the correct position for such code.
    IgnoreUnusedVariable(p);

    return WEAVE_NO_ERROR;
}

WEAVE_ERROR WeaveMessageLayer::DecodeMessage(PacketBuffer *msgBuf, uint64_t sourceNodeId, WeaveConnection *con,
        WeaveMessageInfo *msgInfo, uint8_t **rPayload, uint16_t *rPayloadLen) // TODO: use references
{
    WEAVE_ERROR err;
    uint8_t *msgStart = msgBuf->Start();
    uint16_t msgLen = msgBuf->DataLength();
    uint8_t *msgEnd = msgStart + msgLen;
    uint8_t *p = msgStart;
    msgInfo->SourceNodeId = sourceNodeId;
    err = DecodeHeader(msgBuf, msgInfo, &p);
    sourceNodeId = msgInfo->SourceNodeId;

    if (err != WEAVE_NO_ERROR)
        return err;

    // Get the session state for the given source node and encryption key.
    WeaveSessionState sessionState;

    err = FabricState->GetSessionState(sourceNodeId, msgInfo->KeyId, msgInfo->EncryptionType, con, sessionState);
    if (err != WEAVE_NO_ERROR)
        return err;

    switch (msgInfo->EncryptionType)
    {
    case kWeaveEncryptionType_None:
        // Return the position and length of the payload within the message.
        *rPayloadLen = msgLen - (p - msgStart);
        *rPayload = p;

        // Skip over the payload.
        p += *rPayloadLen;
        break;

    case kWeaveEncryptionType_AES128CTRSHA1:
    {
        // Error if the message is short given the expected fields.
        if ((p + kMinPayloadLen + HMACSHA1::kDigestLength) > msgEnd)
            return WEAVE_ERROR_INVALID_MESSAGE_LENGTH;

        // Return the position and length of the payload within the message.
        uint16_t payloadLen = msgLen - ((p - msgStart) + HMACSHA1::kDigestLength);
        *rPayloadLen = payloadLen;
        *rPayload = p;

        // Decrypt the message payload and the integrity check value that follows it, in place, in the message buffer.
        Encrypt_AES128CTRSHA1(msgInfo, sessionState.MsgEncKey->EncKey.AES128CTRSHA1.DataKey,
                              p, payloadLen + HMACSHA1::kDigestLength, p);

        // Compute the expected integrity check value from the decrypted payload.
        uint8_t expectedIntegrityCheck[HMACSHA1::kDigestLength];
        ComputeIntegrityCheck_AES128CTRSHA1(msgInfo, sessionState.MsgEncKey->EncKey.AES128CTRSHA1.IntegrityKey,
                                            p, payloadLen, expectedIntegrityCheck);
        // Error if the expected integrity check doesn't match the integrity check in the message.
        if (!ConstantTimeCompare(p + payloadLen, expectedIntegrityCheck, HMACSHA1::kDigestLength))
            return WEAVE_ERROR_INTEGRITY_CHECK_FAILED;
        // Skip past the payload and the integrity check value.
        p += payloadLen + HMACSHA1::kDigestLength;

        break;
    }

    default:
        return WEAVE_ERROR_UNSUPPORTED_ENCRYPTION_TYPE;
    }

    // Set flag in the message header indicating that the message is a duplicate if:
    //  - A message with the same message identifier has already been received from that peer.
    //  - This is the first message from that peer encrypted with application keys.
    if (sessionState.IsDuplicateMessage(msgInfo->MessageId))
        msgInfo->Flags |= kWeaveMessageFlag_DuplicateMessage;

#if WEAVE_CONFIG_USE_APP_GROUP_KEYS_FOR_MSG_ENC
    // Set flag if peer group key message counter is not synchronized.
    if (sessionState.MessageIdNotSynchronized() && WeaveKeyId::IsAppGroupKey(msgInfo->KeyId))
        msgInfo->Flags |= kWeaveMessageFlag_PeerGroupMsgIdNotSynchronized;
#endif

    // Pass the peer authentication mode back to the application via the weave message header structure.
    msgInfo->PeerAuthMode = sessionState.AuthMode;

    return err;
}

WEAVE_ERROR WeaveMessageLayer::EncodeMessageWithLength(WeaveMessageInfo *msgInfo, PacketBuffer *msgBuf,
        WeaveConnection *con, uint16_t maxLen)
{
    // Encode the message, reserving 2 bytes for the length.
    WEAVE_ERROR err = EncodeMessage(msgInfo, msgBuf, con, maxLen - 2, 2);
    if (err != WEAVE_NO_ERROR)
        return err;

    // Prepend the message length to the beginning of the message.
    uint8_t * newMsgStart = msgBuf->Start() - 2;
    uint16_t msgLen = msgBuf->DataLength();
    msgBuf->SetStart(newMsgStart);
    LittleEndian::Put16(newMsgStart, msgLen);

    return WEAVE_NO_ERROR;
}

WEAVE_ERROR WeaveMessageLayer::DecodeMessageWithLength(PacketBuffer *msgBuf, uint64_t sourceNodeId, WeaveConnection *con,
        WeaveMessageInfo *msgInfo, uint8_t **rPayload, uint16_t *rPayloadLen, uint16_t *rFrameLen)
{
    uint8_t *dataStart = msgBuf->Start();
    uint16_t dataLen = msgBuf->DataLength();

    // Error if the message buffer doesn't contain the entire message length field.
    if (dataLen < 2)
    {
        *rFrameLen = 8; // Assume absolute minimum frame length.
        return WEAVE_ERROR_MESSAGE_INCOMPLETE;
    }

    // Read the message length.
    uint16_t msgLen = LittleEndian::Get16(dataStart);

    // The frame length is the length of the message plus the length of the length field.
    *rFrameLen = msgLen + 2;

    // Error if the message buffer doesn't contain the entire message, or is too
    // long to ever fit in the buffer.
    if (dataLen < *rFrameLen)
    {
        if (*rFrameLen > msgBuf->MaxDataLength() + msgBuf->ReservedSize())
            return WEAVE_ERROR_MESSAGE_TOO_LONG;
        return WEAVE_ERROR_MESSAGE_INCOMPLETE;
    }

    // Adjust the message buffer to point at the message, not including the message length field that precedes it,
    // and not including any data that may follow it.
    msgBuf->SetStart(dataStart + 2);
    msgBuf->SetDataLength(msgLen);

    // Decode the message.
    WEAVE_ERROR err = DecodeMessage(msgBuf, sourceNodeId, con, msgInfo, rPayload, rPayloadLen);

    // If successful, adjust the message buffer to point at any remaining data beyond the end of the message.
    // (This may in fact represent another message).
    if (err == WEAVE_NO_ERROR)
    {
        msgBuf->SetStart(dataStart + msgLen + 2);
        msgBuf->SetDataLength(dataLen - (msgLen + 2));
    }

    // Otherwise, reset the buffer to its original position/length.
    else
    {
        msgBuf->SetStart(dataStart);
        msgBuf->SetDataLength(dataLen);
    }

    return err;
}

void WeaveMessageLayer::HandleUDPMessage(UDPEndPoint *endPoint, PacketBuffer *msg, const IPPacketInfo *pktInfo)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveMessageLayer *msgLayer = (WeaveMessageLayer *) endPoint->AppState;
    WeaveMessageInfo msgInfo;
    uint64_t sourceNodeId;
    uint8_t *payload;
    uint16_t payloadLen;

    WEAVE_FAULT_INJECT(FaultInjection::kFault_DropIncomingUDPMsg,
                       PacketBuffer::Free(msg);
                       ExitNow(err = WEAVE_NO_ERROR));

    msgInfo.Clear();
    msgInfo.InPacketInfo = pktInfo;

    // If the message was sent to an IPv6 multicast address, verify that the sending address matches
    // one of the prefixes assigned to a local interface.  If not, ignore the message and report a
    // receive error to the application.
    //
    // Because the message was multicast, we will receive it regardless of what the sender's address is.
    // However, if we don't have a local address in the same prefix, it won't be possible for us to
    // respond. Furthermore, if we accept the message and then the sender retransmits it using a source
    // prefix that DOES match one of our address, the latter message will be discarded as a duplicate,
    // because we already accepted it when it was sent from the original address.
    //
    if (pktInfo->DestAddress.IsMulticast() && !msgLayer->Inet->MatchLocalIPv6Subnet(pktInfo->SrcAddress))
        err = WEAVE_ERROR_INVALID_ADDRESS;

    if (err == WEAVE_NO_ERROR)
    {
        // If the soruce address is a ULA, derive a node identifier from it.  Depending on what's in the
        // message header, this may in fact be the node identifier of the sending node.
        sourceNodeId = (pktInfo->SrcAddress.IsIPv6ULA()) ? IPv6InterfaceIdToWeaveNodeId(pktInfo->SrcAddress.InterfaceId()) : kNodeIdNotSpecified;

        // Attempt to decode the message.
        err = msgLayer->DecodeMessage(msg, sourceNodeId, NULL, &msgInfo, &payload, &payloadLen);

        if (err == WEAVE_NO_ERROR)
        {
            // Set the message buffer to point at the payload data.
            msg->SetStart(payload);
            msg->SetDataLength(payloadLen);
        }
    }

    // Verify that destination node identifier refers to the local node.
    if (err == WEAVE_NO_ERROR)
    {
        if (msgInfo.DestNodeId != msgLayer->FabricState->LocalNodeId && msgInfo.DestNodeId != kAnyNodeId)
            err = WEAVE_ERROR_INVALID_DESTINATION_NODE_ID;
    }

    // If an error occurred, discard the message and call the on receive error handler.
    SuccessOrExit(err);

    //Check if message carries tunneled data and needs to be sent to Tunnel Agent
    if (msgInfo.MessageVersion == kWeaveMessageVersion_V2)
    {
        if (msgInfo.Flags & kWeaveMessageFlag_TunneledData)
        {
#if WEAVE_CONFIG_ENABLE_TUNNELING
            // Policy for handling duplicate tunneled UDP message:
            //  - Eliminate duplicate tunneled encrypted messages to prevent replay of messages by
            //    a malicious man-in-the-middle.
            //  - Handle duplicate tunneled unencrypted message.
            // Dispatch the tunneled data message to the application if it is not a duplicate or unencrypted.
            if (!(msgInfo.Flags & kWeaveMessageFlag_DuplicateMessage) || msgInfo.KeyId == WeaveKeyId::kNone)
            {
                if (msgLayer->OnUDPTunneledMessageReceived)
                {
                    msgLayer->OnUDPTunneledMessageReceived(msgLayer, msg);
                }
                else
                {
                    ExitNow(err = WEAVE_ERROR_NO_MESSAGE_HANDLER);
                }
            }
#endif
        }
        else
        {
            // Call the supplied OnMessageReceived callback.
            if (msgLayer->OnMessageReceived != NULL)
            {
                msgLayer->OnMessageReceived(msgLayer, &msgInfo, msg);
            }
            else
            {
                ExitNow(err = WEAVE_ERROR_NO_MESSAGE_HANDLER);
            }
        }
    }
    else if (msgInfo.MessageVersion == kWeaveMessageVersion_V1)
    {
        // Call the supplied OnMessageReceived callback.
        if (msgLayer->OnMessageReceived != NULL)
            msgLayer->OnMessageReceived(msgLayer, &msgInfo, msg);
        else
        {
            ExitNow(err = WEAVE_ERROR_NO_MESSAGE_HANDLER);
        }
    }

exit:
    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogError(MessageLayer, "HandleUDPMessage Error %d", err);

        PacketBuffer::Free(msg);

        // Send key error response to the peer if required.
        // Key error response is sent only if the received message is not a multicast.
        if (!pktInfo->DestAddress.IsMulticast() && msgLayer->SecurityMgr->IsKeyError(err))
            msgLayer->SecurityMgr->SendKeyErrorMsg(&msgInfo, pktInfo, NULL, err);

        if (msgLayer->OnReceiveError != NULL)
            msgLayer->OnReceiveError(msgLayer, err, pktInfo);
    }

    return;
}

void WeaveMessageLayer::HandleUDPReceiveError(UDPEndPoint *endPoint, INET_ERROR err, const IPPacketInfo *pktInfo)
{
    WeaveLogError(MessageLayer, "HandleUDPReceiveError Error %d", err);

    WeaveMessageLayer *msgLayer = (WeaveMessageLayer *) endPoint->AppState;
    if (msgLayer->OnReceiveError != NULL)
        msgLayer->OnReceiveError(msgLayer, err, pktInfo);
}

#if CONFIG_NETWORK_LAYER_BLE
void WeaveMessageLayer::HandleIncomingBleConnection(BLEEndPoint *bleEP)
{
    WeaveMessageLayer *msgLayer = (WeaveMessageLayer *) bleEP->mAppState;

    // Immediately close the connection if there's no callback registered.
    if (msgLayer->OnConnectionReceived == NULL && msgLayer->ExchangeMgr == NULL)
    {
        bleEP->Close();
        if (msgLayer->OnAcceptError != NULL)
            msgLayer->OnAcceptError(msgLayer, WEAVE_ERROR_NO_CONNECTION_HANDLER);
        return;
    }

    // Attempt to allocate a connection object. Fail if too many connections.
    WeaveConnection *con = msgLayer->NewConnection();
    if (con == NULL)
    {
        bleEP->Close();
        if (msgLayer->OnAcceptError != NULL)
            msgLayer->OnAcceptError(msgLayer, WEAVE_ERROR_TOO_MANY_CONNECTIONS);
        return;
    }

    // Setup the connection object.
    con->MakeConnectedBle(bleEP);

#if WEAVE_PROGRESS_LOGGING
    {
        WeaveLogProgress(MessageLayer, "WoBle con rcvd");
    }
#endif

    // Set the default idle timeout.
    con->SetIdleTimeout(msgLayer->IncomingConIdleTimeout);

    // If the exchange manager has been initialized, call its callback.
    if (msgLayer->ExchangeMgr != NULL)
        msgLayer->ExchangeMgr->HandleConnectionReceived(con);

    // Call the app's OnConnectionReceived callback.
    if (msgLayer->OnConnectionReceived != NULL)
        msgLayer->OnConnectionReceived(msgLayer, con);
}
#endif /* CONFIG_NETWORK_LAYER_BLE */

void WeaveMessageLayer::HandleIncomingTcpConnection(TCPEndPoint *listeningEP, TCPEndPoint *conEP, const IPAddress &peerAddr, uint16_t peerPort)
{
    INET_ERROR err;
    IPAddress localAddr;
    uint16_t localPort;
    WeaveMessageLayer *msgLayer = (WeaveMessageLayer *) listeningEP->AppState;

    // Immediately close the connection if there's no callback registered.
    if (msgLayer->OnConnectionReceived == NULL && msgLayer->ExchangeMgr == NULL)
    {
        conEP->Free();
        if (msgLayer->OnAcceptError != NULL)
            msgLayer->OnAcceptError(msgLayer, WEAVE_ERROR_NO_CONNECTION_HANDLER);
        return;
    }

    // Attempt to allocate a connection object. Fail if too many connections.
    WeaveConnection *con = msgLayer->NewConnection();
    if (con == NULL)
    {
        conEP->Free();
        if (msgLayer->OnAcceptError != NULL)
            msgLayer->OnAcceptError(msgLayer, WEAVE_ERROR_TOO_MANY_CONNECTIONS);
        return;
    }

    // Get the local address that was used for the connection.
    err = conEP->GetLocalInfo(&localAddr, &localPort);
    if (err != INET_NO_ERROR)
    {
        conEP->Free();
        if (msgLayer->OnAcceptError != NULL)
            msgLayer->OnAcceptError(msgLayer, err);
        return;
    }

    // Setup the connection object.
    con->MakeConnectedTcp(conEP, localAddr, peerAddr);

#if WEAVE_PROGRESS_LOGGING
    {
        char ipAddrStr[64];
        peerAddr.ToString(ipAddrStr, sizeof(ipAddrStr));
        WeaveLogProgress(MessageLayer, "Con %s %04" PRIX16 " %s %d", "rcvd", con->LogId(), ipAddrStr, (int)peerPort);
    }
#endif

    // Set the default idle timeout.
    con->SetIdleTimeout(msgLayer->IncomingConIdleTimeout);

    // If the exchange manager has been initialized, call its callback.
    if (msgLayer->ExchangeMgr != NULL)
        msgLayer->ExchangeMgr->HandleConnectionReceived(con);

    // Call the app's OnConnectionReceived callback.
    if (msgLayer->OnConnectionReceived != NULL)
        msgLayer->OnConnectionReceived(msgLayer, con);

    // If connection was received on unsecured port, call the app's OnUnsecuredConnectionReceived callback.
    if (msgLayer->OnUnsecuredConnectionReceived != NULL && conEP->GetLocalInfo(&localAddr, &localPort) ==
            WEAVE_NO_ERROR && localPort == WEAVE_UNSECURED_PORT)
        msgLayer->OnUnsecuredConnectionReceived(msgLayer, con);

}

void WeaveMessageLayer::HandleAcceptError(TCPEndPoint *ep, INET_ERROR err)
{
    WeaveMessageLayer *msgLayer = (WeaveMessageLayer *) ep->AppState;
    if (msgLayer->OnAcceptError != NULL)
        msgLayer->OnAcceptError(msgLayer, err);
}

/**
 *  Refresh the InetLayer endpoints based on the current state of the system's network interfaces.
 *
 *  @note
 *     This function is designed to be called multiple times. The first call will setup all the
 *     TCP / UDP endpoints needed for the messaging layer to communicate, based on the specified
 *     configuration (i.e. IPv4 listen enabled, IPv6 listen enabled, etc.). Subsequent calls will
 *     re-initialize the active endpoints based on the current state of the system's network
 *     interfaces.
 *
 *  @retval #WEAVE_NO_ERROR on successful refreshing of endpoints.
 *  @retval InetLayer errors based on calls to create TCP/UDP endpoints.
 *
 */
WEAVE_ERROR WeaveMessageLayer::RefreshEndpoints()
{
    WEAVE_ERROR res = WEAVE_NO_ERROR;
#if INET_CONFIG_ENABLE_IPV4
    bool listenIPv4 = (mFlags & kFlag_ListenIPv4) != 0;
#endif // INET_CONFIG_ENABLE_IPV4
    bool listenIPv6 = (mFlags & kFlag_ListenIPv6) != 0;
    bool listenTCP = (mFlags & kFlag_ListenTCP) != 0;
    bool listenUDP = (mFlags & kFlag_ListenUDP) != 0;
#if CONFIG_NETWORK_LAYER_BLE
    bool listenBLE = (mFlags & kFlag_ListenBLE) != 0;
#endif

#if WEAVE_BIND_DETAIL_LOGGING && WEAVE_DETAIL_LOGGING
    char ipAddrStr[64];
    char intfStr[64];
#endif

#if INET_CONFIG_ENABLE_IPV4
    // Close and free the general-purpose IPv4 and IPv6 UDP endpoints.
    if (mIPv4UDP != NULL)
    {
        mIPv4UDP->Free();
        mIPv4UDP = NULL;
    }
#endif // INET_CONFIG_ENABLE_IPV4
    if (mIPv6UDP != NULL)
    {
        mIPv6UDP->Free();
        mIPv6UDP = NULL;
    }

    // Close and free all the currently open IPv6 interface endpoints. We will re-create them
    // below based on the current network interface config.
    for (int i = 0; i < WEAVE_CONFIG_MAX_LOCAL_ADDR_UDP_ENDPOINTS; i++)
        if (mIPv6UDPLocalAddr[i] != NULL)
        {
            if (mIPv6UDPLocalAddr[i] != mIPv6UDP)
                mIPv6UDPLocalAddr[i]->Free();
            mIPv6UDPLocalAddr[i] = NULL;
        }

    // Clear the list of interfaces.
    memset(mInterfaces, 0, sizeof(mInterfaces));

#if WEAVE_CONFIG_ENABLE_TARGETED_LISTEN

#if INET_CONFIG_ENABLE_IPV4
#define WEAVE_IPV4_LISTEN_ADDR (FabricState->ListenIPv4Addr)
#endif // INET_CONFIG_ENABLE_IPV4

#define WEAVE_IPV6_LISTEN_ADDR (FabricState->ListenIPv6Addr)
#define WEAVE_IPV6_LISTEN_INTF (mInterfaces[0])

    // If configured to use a specific IPv6 address, determine the interface associated
    // with that address.  Store it as the only interface in the interface list.
    if (IsBoundToLocalIPv6Address())
    {
        res = Inet->GetInterfaceFromAddr(WEAVE_IPV6_LISTEN_ADDR, mInterfaces[0]);
        if (res != WEAVE_NO_ERROR)
            goto exit;
    }

#else // !WEAVE_CONFIG_ENABLE_TARGETED_LISTEN

#if INET_CONFIG_ENABLE_IPV4
#define WEAVE_IPV4_LISTEN_ADDR (IPAddress::Any)
#endif // INET_CONFIG_ENABLE_IPV4

#define WEAVE_IPV6_LISTEN_ADDR (IPAddress::Any)
#define WEAVE_IPV6_LISTEN_INTF (INET_NULL_INTERFACEID)

#endif // !WEAVE_CONFIG_ENABLE_TARGETED_LISTEN

#if INET_CONFIG_ENABLE_IPV4
    // If needed, create a IPv4 TCP listening endpoint...
    if (listenTCP && listenIPv4)
    {
        if (mIPv4TCPListen == NULL)
        {
#if WEAVE_BIND_DETAIL_LOGGING && WEAVE_DETAIL_LOGGING
            WEAVE_IPV4_LISTEN_ADDR.ToString(ipAddrStr, sizeof(ipAddrStr));
            WeaveBindLog("Binding IPv4 TCP listen endpoint to [%s]:%d", ipAddrStr, WEAVE_PORT);
#endif

            res = Inet->NewTCPEndPoint(&mIPv4TCPListen);
            if (res != WEAVE_NO_ERROR)
                goto exit;

            // Bind the endpoint to the IPv4 listening address (if specified) and the Weave port.
            res = mIPv4TCPListen->Bind(kIPAddressType_IPv4, WEAVE_IPV4_LISTEN_ADDR, WEAVE_PORT, true);
            if (res != WEAVE_NO_ERROR)
                goto exit;

            WeaveBindLog("Listening on IPv4 TCP endpoint");

            // Listen for incoming TCP connections.
            mIPv4TCPListen->AppState = this;
            mIPv4TCPListen->OnConnectionReceived = HandleIncomingTcpConnection;
            mIPv4TCPListen->OnAcceptError = HandleAcceptError;
            res = mIPv4TCPListen->Listen(1);
            if (res != WEAVE_NO_ERROR)
                goto exit;
        }
    }
#endif // INET_CONFIG_ENABLE_IPV4

    // If needed, create a IPv6 TCP listening endpoint...
    if (listenTCP && listenIPv6)
    {
        if (mIPv6TCPListen == NULL)
        {
#if WEAVE_BIND_DETAIL_LOGGING && WEAVE_DETAIL_LOGGING
            WEAVE_IPV6_LISTEN_ADDR.ToString(ipAddrStr, sizeof(ipAddrStr));
            WeaveBindLog("Binding IPv6 TCP listen endpoint to [%s]:%d", ipAddrStr, WEAVE_PORT);
#endif

            res = Inet->NewTCPEndPoint(&mIPv6TCPListen);
            if (res != WEAVE_NO_ERROR)
                goto exit;

            // Bind the endpoint to the IPv6 listening address (if specified) and the Weave port.
            res = mIPv6TCPListen->Bind(kIPAddressType_IPv6, WEAVE_IPV6_LISTEN_ADDR, WEAVE_PORT, true);
            if (res != WEAVE_NO_ERROR)
                goto exit;

#if WEAVE_BIND_DETAIL_LOGGING && WEAVE_DETAIL_LOGGING
            WeaveBindLog("Listening on IPv6 TCP endpoint");
#endif

            // Listen for incoming TCP connections.
            mIPv6TCPListen->AppState = this;
            mIPv6TCPListen->OnConnectionReceived = HandleIncomingTcpConnection;
            mIPv6TCPListen->OnAcceptError = HandleAcceptError;
            res = mIPv6TCPListen->Listen(1);
            if (res != WEAVE_NO_ERROR)
                goto exit;
        }

    }

#if WEAVE_CONFIG_ENABLE_UNSECURED_TCP_LISTEN
    if (listenIPv6 && (mFlags & kFlag_ListenUnsecured) != 0)
    {
        if (mUnsecuredIPv6TCPListen == NULL)
        {
#if WEAVE_BIND_DETAIL_LOGGING && WEAVE_DETAIL_LOGGING
            WEAVE_IPV6_LISTEN_ADDR.ToString(ipAddrStr, sizeof(ipAddrStr));
            WeaveBindLog("Binding unsecured IPv6 TCP listen endpoint to [%s]:%d", ipAddrStr, WEAVE_UNSECURED_PORT);
#endif

            res = Inet->NewTCPEndPoint(&mUnsecuredIPv6TCPListen);
            if (res != WEAVE_NO_ERROR)
                goto exit;

            // Bind the endpoint to the IPv6 listening address (if specified) and the unsecured Weave port.
            res = mUnsecuredIPv6TCPListen->Bind(kIPAddressType_IPv6, WEAVE_IPV6_LISTEN_ADDR, WEAVE_UNSECURED_PORT, true);
            if (res != WEAVE_NO_ERROR)
                goto exit;

#if WEAVE_BIND_DETAIL_LOGGING && WEAVE_DETAIL_LOGGING
            WeaveBindLog("Listening on unsecured IPv6 TCP endpoint");
#endif

            // Listen for incoming TCP connections.
            mUnsecuredIPv6TCPListen->AppState = this;
            mUnsecuredIPv6TCPListen->OnConnectionReceived = HandleIncomingTcpConnection;
            mUnsecuredIPv6TCPListen->OnAcceptError = HandleAcceptError;
            res = mUnsecuredIPv6TCPListen->Listen(1);
            if (res != WEAVE_NO_ERROR)
                goto exit;
        }
    }

    else if (mUnsecuredIPv6TCPListen != NULL)
    {
        mUnsecuredIPv6TCPListen->Free();
        mUnsecuredIPv6TCPListen = NULL;
    }
#endif // WEAVE_CONFIG_ENABLE_UNSECURED_TCP_LISTEN

#if INET_CONFIG_ENABLE_IPV4
    // Create a general-purpose IPv4 UDP endpoint...
    if (mIPv4UDP == NULL)
    {
#if WEAVE_BIND_DETAIL_LOGGING && WEAVE_DETAIL_LOGGING
        WEAVE_IPV4_LISTEN_ADDR.ToString(ipAddrStr, sizeof(ipAddrStr));
        WeaveBindLog("Binding general purpose IPv4 UDP endpoint to [%s]:%d", ipAddrStr, WEAVE_PORT);
#endif // WEAVE_BIND_DETAIL_LOGGING && WEAVE_DETAIL_LOGGING

        res = Inet->NewUDPEndPoint(&mIPv4UDP);
        if (res != WEAVE_NO_ERROR)
            goto exit;

        // Bind the endpoint.  If a listening IPv4 address was specified bind to that,
        // otherwise bind to all addresses.
        res = mIPv4UDP->Bind(kIPAddressType_IPv4, WEAVE_IPV4_LISTEN_ADDR, WEAVE_PORT);
        if (res != WEAVE_NO_ERROR)
            goto exit;

        // Listen for incoming IPv4 UDP messages if so configured.
        if (listenUDP && listenIPv4)
        {
            WeaveBindLog("Listening on general purpose IPv4 UDP endpoint");

            mIPv4UDP->AppState = this;
            mIPv4UDP->OnMessageReceived = reinterpret_cast<IPEndPointBasis::OnMessageReceivedFunct>(HandleUDPMessage);
            mIPv4UDP->OnReceiveError = reinterpret_cast<IPEndPointBasis::OnReceiveErrorFunct>(HandleUDPReceiveError);
            res = mIPv4UDP->Listen();
            if (res != WEAVE_NO_ERROR)
                goto exit;
        }
    }
#endif // INET_CONFIG_ENABLE_IPV4

    // Create a general-purpose IPv6 UDP endpoint...
    if (mIPv6UDP == NULL)
    {
#if WEAVE_BIND_DETAIL_LOGGING && WEAVE_DETAIL_LOGGING
        GetInterfaceName(WEAVE_IPV6_LISTEN_INTF, intfStr, sizeof(intfStr));
        WEAVE_IPV6_LISTEN_ADDR.ToString(ipAddrStr, sizeof(ipAddrStr));
        WeaveBindLog("Binding general purpose IPv6 UDP endpoint to [%s]:%d (%s)", ipAddrStr, WEAVE_PORT, intfStr);
#endif // WEAVE_BIND_DETAIL_LOGGING && WEAVE_DETAIL_LOGGING

        res = Inet->NewUDPEndPoint(&mIPv6UDP);
        if (res != WEAVE_NO_ERROR)
            goto exit;

        // Bind the endpoint.  If a particular IPv6 address was specified, bind to that address and its
        // associated interface. Otherwise bind to all IPv6 addresses.
        res = mIPv6UDP->Bind(kIPAddressType_IPv6, WEAVE_IPV6_LISTEN_ADDR, WEAVE_PORT, WEAVE_IPV6_LISTEN_INTF);
        if (res != WEAVE_NO_ERROR)
            goto exit;

        // Listen for incoming IPv6 UDP messages if so configured.
        if (listenUDP && listenIPv6)
        {
            WeaveBindLog("Listening on general purpose IPv6 UDP endpoint");

            mIPv6UDP->AppState = this;
            mIPv6UDP->OnMessageReceived = reinterpret_cast<IPEndPointBasis::OnMessageReceivedFunct>(HandleUDPMessage);
            mIPv6UDP->OnReceiveError = reinterpret_cast<IPEndPointBasis::OnReceiveErrorFunct>(HandleUDPReceiveError);
            res = mIPv6UDP->Listen();
            if (res != WEAVE_NO_ERROR)
                goto exit;
        }
    }

#if WEAVE_CONFIG_ENABLE_TARGETED_LISTEN

    // If configured to use a specific IPv6 address...
    if (IsBoundToLocalIPv6Address())
    {
        // If IPv6 listening has been enabled, create a IPv6 UDP endpoint for receiving multicast messages.
        // Bind this interface to the link-local, all-nodes multicast address (ff02::1) and the interface
        // associated with the listening IPv6 address.
        if (listenIPv6 && mIPv6UDPMulticastRcv == NULL)
        {
            IPAddress ipv6LinkLocalAllNodes = IPAddress::MakeIPv6WellKnownMulticast(kIPv6MulticastScope_Link, kIPV6MulticastGroup_AllNodes);

#if WEAVE_BIND_DETAIL_LOGGING && WEAVE_DETAIL_LOGGING
            ipv6LinkLocalAllNodes.ToString(ipAddrStr, sizeof(ipAddrStr));
            GetInterfaceName(WEAVE_IPV6_LISTEN_INTF, intfStr, sizeof(intfStr));
            WeaveBindLog("Binding IPv6 multicast receive endpoint to [%s]:%d (%s)", ipAddrStr, WEAVE_PORT, intfStr);
#endif

            res = Inet->NewUDPEndPoint(&mIPv6UDPMulticastRcv);
            if (res != WEAVE_NO_ERROR)
                goto exit;

            // Bind the endpoint to the weave port on the IPv6 link-local all nodes multicast address.
            res = mIPv6UDPMulticastRcv->Bind(kIPAddressType_IPv6, ipv6LinkLocalAllNodes, WEAVE_PORT, WEAVE_IPV6_LISTEN_INTF);
            if (res != WEAVE_NO_ERROR)
                goto exit;

            WeaveBindLog("Listening on IPv6 multicast receive endpoint");

            // Enable reception of incoming messages.
            mIPv6UDPMulticastRcv->AppState = this;
            mIPv6UDPMulticastRcv->OnMessageReceived = reinterpret_cast<IPEndPointBasis::OnMessageReceivedFunct>(HandleUDPMessage);
            mIPv6UDPMulticastRcv->OnReceiveError = reinterpret_cast<IPEndPointBasis::OnReceiveErrorFunct>(HandleUDPReceiveError);
            res = mIPv6UDPMulticastRcv->Listen();
            if (res != WEAVE_NO_ERROR)
                goto exit;
        }
    }

    // Otherwise, the messaging layer is configured to use all available interfaces/addresses, so ...
    else

#endif // WEAVE_CONFIG_ENABLE_TARGETED_LISTEN

    {
        uint16_t epCount = 0;
        uint16_t i;

        // Scan the list of addresses assigned to the system's network interfaces.  For each address...
        for (InterfaceAddressIterator addrIter; addrIter.HasCurrent(); addrIter.Next())
        {
            InterfaceId curIntfId = addrIter.GetInterface();

#if WEAVE_BIND_DETAIL_LOGGING && WEAVE_DETAIL_LOGGING
            GetInterfaceName(curIntfId, intfStr, sizeof(intfStr));
#endif

            // Skip any interface that doesn't support multicast.
            if (!addrIter.SupportsMulticast())
                continue;

            // Add the interface to the interface list if it doesn't already exist.
            for (i = 0; i < WEAVE_CONFIG_MAX_INTERFACES; i++)
            {
                if (mInterfaces[i] == curIntfId)
                    break;
                if (mInterfaces[i] == INET_NULL_INTERFACEID)
                {
                    WeaveBindLog("Adding %s to interface table", intfStr);
                    mInterfaces[i] = curIntfId;
                    break;
                }
            }
            if (i == WEAVE_CONFIG_MAX_INTERFACES)
                WeaveLogError(MessageLayer, "Interface table full");

            // If we haven't exceeded the max ULA endpoints...
            if (epCount < WEAVE_CONFIG_MAX_LOCAL_ADDR_UDP_ENDPOINTS)
            {
                WEAVE_ERROR epErr;
                UDPEndPoint *& ep = mIPv6UDPLocalAddr[epCount];

                // Skip the address if it is not a ULA.
                IPAddress curAddr = addrIter.GetAddress();
                if (!curAddr.IsIPv6ULA())
                    continue;

                // Skip the address if we're a member of a fabric and the ULA is not a fabric address
                // (in particular, the global identifier in the ULA does not match the bottom 40 bits of the
                // fabric id).
                if (FabricState->FabricId != 0 && !FabricState->IsFabricAddress(curAddr))
                    continue;

#if WEAVE_BIND_DETAIL_LOGGING && WEAVE_DETAIL_LOGGING
                curAddr.ToString(ipAddrStr, sizeof(ipAddrStr));
                WeaveBindLog("Binding IPv6 UDP interface endpoint to [%s]:%d (%s)", ipAddrStr, WEAVE_PORT, intfStr);
#endif

                // Create an IPv6 UDP endpoint to be used for sending/receiving messages over the associated interface.
                res = Inet->NewUDPEndPoint(&ep);
                if (res != WEAVE_NO_ERROR)
                    goto exit;

                // Bind the endpoint to the identified address.  This ensures that messages sent over the endpoint
                // have the correct source address and port.
                epErr = ep->Bind(kIPAddressType_IPv6, curAddr, WEAVE_PORT, curIntfId);

                // Enable reception of incoming messages.
                WeaveBindLog("Listening on IPv6 UDP interface endpoint");
                if (epErr == WEAVE_NO_ERROR)
                {
                    ep->AppState = this;
                    ep->OnMessageReceived = reinterpret_cast<IPEndPointBasis::OnMessageReceivedFunct>(HandleUDPMessage);
                    ep->OnReceiveError = reinterpret_cast<IPEndPointBasis::OnReceiveErrorFunct>(HandleUDPReceiveError);
                    epErr = ep->Listen();
                }

                // If we successfully bound the endpoint, add it to the list. Otherwise, discard it and move on to
                // the next address.
                if (epErr == WEAVE_NO_ERROR)
                    epCount++;
                else
                {
                    ep->Free();
                    ep = NULL;
                }
            }
        }
    }

#if CONFIG_NETWORK_LAYER_BLE
    if (listenBLE)
    {
        if (mBle != NULL)
        {
            mBle->mAppState = this;
            mBle->OnWeaveBleConnectReceived = HandleIncomingBleConnection;
        }
        else
            WeaveLogError(ExchangeManager, "Cannot listen for BLE connections, null BleLayer");
    }
#endif

exit:
    if (res != WEAVE_NO_ERROR)
        WeaveBindLog("RefreshEndpoints failed: %ld", (long)res);
    return res;
}

void WeaveMessageLayer::Encrypt_AES128CTRSHA1(const WeaveMessageInfo *msgInfo, const uint8_t *key,
                                              const uint8_t *inData, uint16_t inLen, uint8_t *outBuf)
{
    AES128CTRMode aes128CTR;
    aes128CTR.SetKey(key);
    aes128CTR.SetWeaveMessageCounter(msgInfo->SourceNodeId, msgInfo->MessageId);
    aes128CTR.EncryptData(inData, inLen, outBuf);
}

void WeaveMessageLayer::ComputeIntegrityCheck_AES128CTRSHA1(const WeaveMessageInfo *msgInfo, const uint8_t *key,
                                                            const uint8_t *inData, uint16_t inLen, uint8_t *outBuf)
{
    HMACSHA1 hmacSHA1;
    uint8_t encodedBuf[2 * sizeof(uint64_t) + sizeof(uint16_t) + sizeof(uint32_t)];
    uint8_t *p = encodedBuf;

    // Initialize HMAC Key.
    hmacSHA1.Begin(key, WeaveEncryptionKey_AES128CTRSHA1::IntegrityKeySize);

    // Encode the source and destination node identifiers in a little-endian format.
    Encoding::LittleEndian::Write64(p, msgInfo->SourceNodeId);
    Encoding::LittleEndian::Write64(p, msgInfo->DestNodeId);

    // Hash the message header field and the message Id for the message version V2.
    if (msgInfo->MessageVersion == kWeaveMessageVersion_V2)
    {
        // Encode message header field value.
        uint16_t headerField = EncodeHeaderField(msgInfo);

        // Mask destination and source node Id flags.
        headerField &= kMsgHeaderField_MessageHMACMask;

        // Encode the message header field and the message Id in a little-endian format.
        Encoding::LittleEndian::Write16(p, headerField);
        Encoding::LittleEndian::Write32(p, msgInfo->MessageId);
    }

    // Hash encoded message header fields.
    hmacSHA1.AddData(encodedBuf, p - encodedBuf);

    // Handle payload data.
    hmacSHA1.AddData(inData, inLen);

    // Generate the MAC.
    hmacSHA1.Finish(outBuf);
}

/**
 *  Close all open TCP and UDP endpoints. Then abort any
 *  open WeaveConnections and shutdown any open
 *  WeaveConnectionTunnel objects.
 *
 *  @note
 *    A call to CloseEndpoints() terminates all communication
 *    channels within the WeaveMessageLayer but does not terminate
 *    the WeaveMessageLayer object.
 *
 *  @sa Shutdown().
 *
 */
WEAVE_ERROR WeaveMessageLayer::CloseEndpoints()
{
    WeaveBindLog("Closing endpoints");

    if (mIPv6TCPListen != NULL)
    {
        mIPv6TCPListen->Free();
        mIPv6TCPListen = NULL;
    }

    if (mIPv6UDP != NULL)
    {
        mIPv6UDP->Free();
        mIPv6UDP = NULL;
    }

#if WEAVE_CONFIG_ENABLE_TARGETED_LISTEN

    if (mIPv6UDPMulticastRcv != NULL)
    {
        mIPv6UDPMulticastRcv->Free();
        mIPv6UDPMulticastRcv = NULL;
    }

#endif

#if WEAVE_CONFIG_ENABLE_UNSECURED_TCP_LISTEN

    if (mUnsecuredIPv6TCPListen != NULL)
    {
        mUnsecuredIPv6TCPListen->Free();
        mUnsecuredIPv6TCPListen = NULL;
    }

#endif

    for (int i = 0; i < WEAVE_CONFIG_MAX_LOCAL_ADDR_UDP_ENDPOINTS; i++)
    {
        if (mIPv6UDPLocalAddr[i] != NULL)
        {
            if (mIPv6UDPLocalAddr[i] != mIPv6UDP)
                mIPv6UDPLocalAddr[i]->Free();
            mIPv6UDPLocalAddr[i] = NULL;
        }
    }

#if INET_CONFIG_ENABLE_IPV4
    if (mIPv4TCPListen != NULL)
    {
        mIPv4TCPListen->Free();
        mIPv4TCPListen = NULL;
    }

    if (mIPv4UDP != NULL)
    {
        mIPv4UDP->Free();
        mIPv4UDP = NULL;
    }
#endif // INET_CONFIG_ENABLE_IPV4

    memset(mInterfaces, 0, sizeof(mInterfaces));

    // Abort any open connections.
    WeaveConnection *con = static_cast<WeaveConnection *>(mConPool);
    for (int i = 0; i < WEAVE_CONFIG_MAX_CONNECTIONS; i++, con++)
        if (con->mRefCount > 0)
            con->Abort();

    // Shut down any open tunnels.
    WeaveConnectionTunnel *tun = static_cast<WeaveConnectionTunnel *>(mTunnelPool);
    for (int i = 0; i < WEAVE_CONFIG_MAX_TUNNELS; i++, tun++)
    {
        if (tun->mMessageLayer != NULL)
        {
            // Suppress callback as we're shutting down the whole stack.
            tun->OnShutdown = NULL;
            tun->Shutdown();
        }
    }

    return WEAVE_NO_ERROR;
}

WEAVE_ERROR WeaveMessageLayer::EnableUnsecuredListen()
{
    // Enable reception of connections on the unsecured Weave port. This allows devices to establish
    // a connection while provisionally connected (i.e. without security) at the network layer.
    mFlags |= kFlag_ListenUnsecured;
    return RefreshEndpoints();
}

WEAVE_ERROR WeaveMessageLayer::DisableUnsecuredListen()
{
    mFlags &= ~kFlag_ListenUnsecured;
    return RefreshEndpoints();
}

bool WeaveMessageLayer::IsUnsecuredListenEnabled() const
{
    return mFlags & kFlag_ListenUnsecured;
}

/**
 *  Set an application handler that will get called every time the
 *  activity of the message layer changes.
 *  Specifically, application will be notified every time:
 *     - the number of opened exchanges changes.
 *     - the number of pending message counter synchronization requests
 *       changes from zero to at least one and back to zero.
 *  The handler is served as general signal indicating whether there
 *  are any ongoing Weave conversations or pending responses.
 *  The handler must be set after the WeaveMessageLayer has been initialized;
 *  shutting down the WeaveMessageLayer will clear out the current handler.
 *
 *  @param[in] messageLayerActivityChangeHandler A pointer to a function to
 *             be called whenever the message layer activity changes.
 *
 *  @retval None.
 */
void WeaveMessageLayer::SetSignalMessageLayerActivityChanged(MessageLayerActivityChangeHandlerFunct messageLayerActivityChangeHandler)
{
    OnMessageLayerActivityChange = messageLayerActivityChangeHandler;
}

/**
 *  This method is called every time the message layer activity changes.
 *  Specifically, it will be called every time:
 *     - the number of opened exchanges changes.
 *     - the number of pending message counter synchronization requests
 *       changes from zero to at least one and back to zero.
 *  New events can be added to this list in the future as needed.
 *
 *  @retval None.
 */
void WeaveMessageLayer::SignalMessageLayerActivityChanged(void)
{
    bool messageLayerIsActive;

    if (OnMessageLayerActivityChange)
    {
        messageLayerIsActive = (ExchangeMgr->mContextsInUse != 0)
#if WEAVE_CONFIG_USE_APP_GROUP_KEYS_FOR_MSG_ENC
                               || FabricState->IsMsgCounterSyncReqInProgress()
#endif
                               ;

        OnMessageLayerActivityChange(messageLayerIsActive);
    }
}

/**
 *  Get the max Weave payload size for a message configuration and supplied
 *  PacketBuffer.
 *
 *  The maximum payload size returned will not exceed the available space
 *  for a payload inside the supplied PacketBuffer.
 *
 *  If the message is UDP, the maximum payload size returned will not result in
 *  a Weave message that will not overflow the specified UDP MTU.
 *
 *  Finally, the maximum payload size returned will not result in a Weave
 *  message that will overflow the max Weave message size.
 *
 *  @param[in]    msgBuf        A pointer to the PacketBuffer to which the message
 *                              payload will be written.
 *
 *  @param[in]    isUDP         True if message is a UDP message.
 *
 *  @param[in]    udpMTU        The size of the UDP MTU. Ignored if isUDP is false.
 *
 *  @return the max Weave payload size.
 */
uint32_t WeaveMessageLayer::GetMaxWeavePayloadSize(const PacketBuffer *msgBuf, bool isUDP, uint32_t udpMTU)
{
    uint32_t maxWeaveMessageSize = isUDP ? udpMTU - INET_CONFIG_MAX_IP_AND_UDP_HEADER_SIZE : UINT16_MAX;
    uint32_t maxWeavePayloadSize = maxWeaveMessageSize - WEAVE_HEADER_RESERVE_SIZE - WEAVE_TRAILER_RESERVE_SIZE;
    uint32_t maxBufferablePayloadSize = msgBuf->AvailableDataLength() - WEAVE_TRAILER_RESERVE_SIZE;

    return maxBufferablePayloadSize < maxWeavePayloadSize
        ? maxBufferablePayloadSize
        : maxWeavePayloadSize;
}

/**
 * Constructs a string describing a peer node and its associated address / connection information.
 *
 * The generated string has the following format:
 *
 *     <node-id> ([<ip-address>]:<port>%<interface>, con <con-id>)
 *
 * @param[in] buf                       A pointer to a buffer into which the string should be written. The supplied
 *                                      buffer should be at least as big as kWeavePeerDescription_MaxLength. If a
 *                                      smaller buffer is given the string will be truncated to fit. The output
 *                                      will include a NUL termination character in all cases.
 * @param[in] bufSize                   The size of the buffer pointed at by buf.
 * @param[in] nodeId                    The node id to be printed.
 * @param[in] addr                      A pointer to an IP address to be printed; or NULL if no IP address should
 *                                      be printed.
 * @param[in] port                      An IP port number to be printed. No port number will be printed if addr
 *                                      is NULL.
 * @param[in] interfaceId               An InterfaceId identifying the interface to be printed. The output string
 *                                      will contain the name of the interface as known to the underlying network
 *                                      stack. No interface name will be printed if interfaceId is INET_NULL_INTERFACEID
 *                                      or if addr is NULL.
 * @param[in] con                       A pointer to a WeaveConnection object whose logging id should be printed;
 *                                      or NULL if no connection id should be printed.
 */
void WeaveMessageLayer::GetPeerDescription(char * buf, size_t bufSize, uint64_t nodeId,
    const IPAddress * addr, uint16_t port, InterfaceId interfaceId, const WeaveConnection * con)
{
    enum { kMaxInterfaceNameLength = 20 }; // Arbitrarily capped at 20 characters so long interface
                                           // names do not blow out the available space.

    uint32_t len;
    const char * sep = "";

    if (nodeId != kNodeIdNotSpecified)
    {
        len = snprintf(buf, bufSize, "%" PRIX64 " (", nodeId);
    }
    else
    {
        len = snprintf(buf, bufSize, "unknown (");
    }
    VerifyOrExit(len < bufSize, /* no-op */);

    if (addr != NULL)
    {
        buf[len++] = '[';
        VerifyOrExit(len < bufSize, /* no-op */);

        addr->ToString(buf + len, bufSize - len);
        len = strlen(buf);

        if (port > 0)
        {
            len += snprintf(buf + len, bufSize - len, "]:%" PRIu16, port);
        }
        else
        {
            len += snprintf(buf + len, bufSize - len, "]");
        }
        VerifyOrExit(len < bufSize, /* no-op */);

        if (interfaceId != INET_NULL_INTERFACEID)
        {
            char interfaceName[kMaxInterfaceNameLength+1];
            Inet::GetInterfaceName(interfaceId, interfaceName, sizeof(interfaceName));
            interfaceName[kMaxInterfaceNameLength] = 0;
            len += snprintf(buf + len, bufSize - len, "%%%s", interfaceName);
            VerifyOrExit(len < bufSize, /* no-op */);
        }

        sep = ", ";
    }

    if (con != NULL)
    {
        const char *conType;
        switch (con->NetworkType)
        {
        case WeaveConnection::kNetworkType_BLE:
            conType = "ble ";
            break;
        case WeaveConnection::kNetworkType_IP:
        default:
            conType = "";
            break;
        }

        len += snprintf(buf + len, bufSize - len, "%s%scon %04" PRIX16, sep, conType, con->LogId());
        VerifyOrExit(len < bufSize, /* no-op */);
    }

    snprintf(buf + len, bufSize - len, ")");

exit:
    if (bufSize > 0)
    {
        buf[bufSize - 1] = 0;
    }
    return;
}

/**
 * Constructs a string describing a peer node based on the information associated with a message received from the peer.
 *
 * @param[in] buf                       A pointer to a buffer into which the string should be written. The supplied
 *                                      buffer should be at least as big as kWeavePeerDescription_MaxLength. If a
 *                                      smaller buffer is given the string will be truncated to fit. The output
 *                                      will include a NUL termination character in all cases.
 * @param[in] bufSize                   The size of the buffer pointed at by buf.
 * @param[in] msgInfo                   A pointer to a WeaveMessageInfo structure containing information about the message.
 *
 */
void WeaveMessageLayer::GetPeerDescription(char * buf, size_t bufSize, const WeaveMessageInfo * msgInfo)
{
    GetPeerDescription(buf, bufSize, msgInfo->SourceNodeId,
        (msgInfo->InPacketInfo != NULL) ? &msgInfo->InPacketInfo->SrcAddress : NULL,
        (msgInfo->InPacketInfo != NULL) ? msgInfo->InPacketInfo->SrcPort : 0,
        (msgInfo->InPacketInfo != NULL) ? msgInfo->InPacketInfo->Interface : INET_NULL_INTERFACEID,
        msgInfo->InCon);
}

} // namespace nl
} // namespace Weave
