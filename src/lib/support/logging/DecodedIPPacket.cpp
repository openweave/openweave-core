/*
 *
 *    Copyright (c) 2014-2017 Nest Labs, Inc.
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
 *
 */

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif // __STDC_FORMAT_MACROS

#include <Weave/Core/WeaveCore.h>
#include <Weave/Support/logging/DecodedIPPacket.h>
#include <Weave/Core/WeaveEncoding.h>
#include <Weave/Support/CodeUtils.h>

#define NL_ICMPV6_MIN_PARSE_LEN  4  // Type, Code, and Checksum.
#define NL_NODE_ID_SIZE  8
#define NL_KEY_ID_SIZE  2
#define WEAVE_EXCH_HDR_LEN 8

using namespace nl::Weave::Encoding;
using namespace nl::Weave;

/**
 * Parse and decode the fields of the IP, UDP/TCP and Weave headers.
 *
 * @param[in]    p       A pointer to an IP packet.
 *
 * @param[in]    pktLen  The length of the IP packet.
 *
 * @return  INET_ERROR while parsing the packet or INET_NO_ERROR
 *          on Success.
 */
INET_ERROR DecodedIPPacket::PacketHeaderDecode(const uint8_t *p, uint16_t pktLen)
{
    INET_ERROR err = INET_NO_ERROR;
    const uint8_t *payload = NULL;
    const uint8_t *pktStart;

    // IP6 header fields
    uint32_t ip6VerTCFlags;
    uint16_t ip6PayloadLen;

    uint8_t versionFlags;

    // Store the start of the packet
    pktStart = p;

    // Make sure the length at least covers the version, TC, and flow label.
    VerifyOrExit(pktLen > 4, err = INET_ERROR_INVALID_IPV6_PKT);

    // Extract fields of the IPv6 header.
    ip6VerTCFlags = BigEndian::Read32(p);

    ipProtoVersion = (ip6VerTCFlags >> 28) & 0x0F;
    VerifyOrExit(ipProtoVersion == NL_IP_VERSION_6, err = INET_ERROR_WRONG_ADDRESS_TYPE);

    ip6PayloadLen = BigEndian::Read16(p);
    ipPktSize = NL_IP6_HDR_LEN + ip6PayloadLen;
    VerifyOrExit(ipPktSize == pktLen, err = INET_ERROR_INVALID_IPV6_PKT);

    ipProtoType = Read8(p);

    p += 1; // Move over to src address;

    memcpy(srcAddr, p, NL_IP6_ADDR_LEN_IN_BYTES);

    p += NL_IP6_ADDR_LEN_IN_BYTES; // Move over to dest address;

    memcpy(destAddr, p, NL_IP6_ADDR_LEN_IN_BYTES);

    p += NL_IP6_ADDR_LEN_IN_BYTES; // Move over to next header;

    // Parse the transport header

    if (ipProtoType == NL_PROTO_TYPE_UDP)
    {
        VerifyOrExit(ip6PayloadLen >= NL_UDP_HDR_LEN, err = INET_ERROR_INVALID_IPV6_PKT);

        // Parse UDP header fields.

        srcPort = BigEndian::Read16(p);

        destPort = BigEndian::Read16(p);

        p += 2; // Move over to UDP checksum;

        checksum = BigEndian::Read16(p);

    }
    else if (ipProtoType == NL_PROTO_TYPE_TCP)
    {
        VerifyOrExit(ip6PayloadLen >= NL_TCP_MIN_HDR_LEN, err = INET_ERROR_INVALID_IPV6_PKT);

        // Parse TCP header fields.

        srcPort = BigEndian::Read16(p);

        destPort = BigEndian::Read16(p);

        p += 12; // Move over to TCP checksum;

        checksum = BigEndian::Read16(p);

        p += 2; // Move over to TCP payload; Assumes no TCP options
    }
    else if (ipProtoType == NL_PROTO_TYPE_ICMPV6)
    {
        VerifyOrExit(ip6PayloadLen >= NL_ICMPV6_MIN_PARSE_LEN, err = INET_ERROR_INVALID_IPV6_PKT);

        icmpv6Type = *p++;

        icmpv6Code = *p++;

        checksum = BigEndian::Read16(p);
    }

    if (DoesPacketHaveWeaveMessage())
    {
        // Parse weave.

        err = ParseWeaveMessageHeader(p, pktStart, &payload);
        SuccessOrExit(err);

        if (encryptionType == nl::Weave::kWeaveEncryptionType_None)
        {
            // Parse Exchange fields

            // versionFlags(1) + messageType(1) + ExchangeId(2) + ProfileId(4)
            if ((pktStart + ipPktSize - payload) < WEAVE_EXCH_HDR_LEN)
                ExitNow(err = WEAVE_ERROR_INVALID_MESSAGE_LENGTH);

            // Read version and flags
            versionFlags = Read8(payload);

            if ((versionFlags >> 4) != nl::Weave::kWeaveExchangeVersion_V1)
                ExitNow(err = WEAVE_ERROR_UNSUPPORTED_EXCHANGE_VERSION);

            msgType = Read8(payload);
            exchangeId = LittleEndian::Read16(payload);
            profileId = LittleEndian::Read32(payload);
            exchFlags = versionFlags & 0xF;

            ackMsgId = 0;
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
            if ((exchFlags & nl::Weave::kWeaveExchangeFlag_AckId))
            {
                if ((payload + 4) > pktStart + ipPktSize)
                    ExitNow(err = WEAVE_ERROR_INVALID_MESSAGE_LENGTH);
                    ackMsgId = LittleEndian::Read32(payload);
            }
#endif
        }
    }
exit:
    return err;
}

/* Get the Weave Node Identifier from an IPv6 ULA address */
uint64_t DecodedIPPacket::GetWeaveNodeIdFromAddr(const uint8_t *addr)
{
    uint64_t nodeId = 0;

    if (addr[0] == 0xFD)
    {
        nodeId = ((static_cast<uint64_t>(addr[8] << 24 | addr[9] << 16 | addr[10] << 8 | addr[11]) << 32) |
                  static_cast<uint64_t>(addr[12] << 24 | addr[13] << 16 | addr[14] << 8 | addr[15]))
                  & ~(0x0200000000000000ULL);
    }

    return nodeId;
}

WEAVE_ERROR DecodedIPPacket::ParseWeaveMessageHeader(const uint8_t *p, const uint8_t *pktStart, const uint8_t **payloadStart)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint16_t msgLen = static_cast<uint16_t>((pktStart + ipPktSize) - p);
    const uint8_t *msgEnd = p + msgLen;
    uint16_t headerField;
    uint8_t msgVersion;

    if (msgLen < 6)
    {
        ExitNow(err = WEAVE_ERROR_INVALID_MESSAGE_LENGTH);
    }

    // Decode the header field.
    headerField = LittleEndian::Read16(p);

    msgVersion = static_cast<uint8_t>((headerField & nl::Weave::kMsgHeaderField_MessageVersionMask) >> nl::Weave::kMsgHeaderField_MessageVersionShift);

    msgHdrFlags = static_cast<uint16_t>((headerField & nl::Weave::kMsgHeaderField_FlagsMask) >> nl::Weave::kMsgHeaderField_FlagsShift);

    encryptionType = static_cast<uint8_t>((headerField & nl::Weave::kMsgHeaderField_EncryptionTypeMask) >> nl::Weave::kMsgHeaderField_EncryptionTypeShift);

    // Error if the message version is unsupported.
    if (msgVersion != nl::Weave::kWeaveMessageVersion_V1 &&
        msgVersion != nl::Weave::kWeaveMessageVersion_V2)
    {
        ExitNow(err = WEAVE_ERROR_UNSUPPORTED_MESSAGE_VERSION);
    }

    // Decode the message id.
    messageId = LittleEndian::Read32(p);

    srcNodeId = destNodeId = 0;

    // Decode the source node identifier if included in the message.
    if (msgHdrFlags & nl::Weave::kWeaveMessageFlag_SourceNodeId)
    {
        if ((p + NL_NODE_ID_SIZE) > msgEnd)
        {
            ExitNow(err = WEAVE_ERROR_INVALID_MESSAGE_LENGTH);
        }
        srcNodeId = LittleEndian::Read64(p);
    }
    else
    {
        srcNodeId = GetWeaveNodeIdFromAddr(srcAddr);
    }

    // Decode the destination node identifier if included in the message.
    if (msgHdrFlags & nl::Weave::kWeaveMessageFlag_DestNodeId)
    {
        if ((p + NL_NODE_ID_SIZE) > msgEnd)
        {
            ExitNow(err = WEAVE_ERROR_INVALID_MESSAGE_LENGTH);
        }
        destNodeId = LittleEndian::Read64(p);
    }
    else
    {
        destNodeId = GetWeaveNodeIdFromAddr(destAddr);
    }

    // Decode the encryption key identifier if present.
    if (encryptionType != nl::Weave::kWeaveEncryptionType_None)
    {
        if ((p + NL_KEY_ID_SIZE) > msgEnd)
        {
            ExitNow(err = WEAVE_ERROR_INVALID_MESSAGE_LENGTH);
        }
        keyId = LittleEndian::Read16(p);
    }
    else
        keyId = nl::Weave::WeaveKeyId::kNone;

    if (payloadStart != NULL)
    {
        *payloadStart = p;
    }

exit:

    return err;
}

/**
 * Check whether a decoded packet contains a Weave message.
 *
 * @note
 *   This method must be called after the packet has been decoded
 *   by the method PacketHeaderDecode().
 */
bool DecodedIPPacket::DoesPacketHaveWeaveMessage(void) const
{
    return ((ipProtoType == NL_PROTO_TYPE_UDP ||
            ipProtoType == NL_PROTO_TYPE_TCP)
            &&
            (srcPort == WEAVE_PORT ||
             srcPort == WEAVE_UNSECURED_PORT ||
             destPort == WEAVE_PORT ||
             destPort == WEAVE_UNSECURED_PORT));
}

/**
 * Log the decoded IP packet.
 *
 * @param[in]    decodedPacket     A const reference to a DecodedIPPacket object.
 *
 * @param[in]    isTunneled     A boolean indicating if the packet is encapsulated within a WeaveTunnel
 *                              so that it can be indicated in the log.
 *
 * return INET_ERROR
 */
INET_ERROR LogPacket(const DecodedIPPacket &decodedPacket, bool isTunneled)
{
    INET_ERROR err = INET_NO_ERROR;
    IPAddress ipSrc;
    IPAddress ipDest;
    char weaveFlags[25];
    const char *tunnelStr = (isTunneled ? "Tun:":"");
    const char *transport = (decodedPacket.ipProtoType == NL_PROTO_TYPE_UDP) ? "UDP" :
                             (decodedPacket.ipProtoType == NL_PROTO_TYPE_TCP) ? "TCP" : "";

    char srcIPAddrStr[INET6_ADDRSTRLEN], destIPAddrStr[INET6_ADDRSTRLEN];

    if (decodedPacket.ipProtoVersion == NL_IP_VERSION_6)
    {
        memcpy(&ipSrc.Addr[0], decodedPacket.srcAddr, NL_IP6_ADDR_LEN_IN_BYTES);
        memcpy(&ipDest.Addr[0], decodedPacket.destAddr, NL_IP6_ADDR_LEN_IN_BYTES);
    }
    else
    {
        //TODO : IPv4 address
    }

    ipSrc.ToString(srcIPAddrStr, sizeof(srcIPAddrStr));
    ipDest.ToString(destIPAddrStr, sizeof(destIPAddrStr));

    WeaveLogDetail(Inet, "%s IPv%d NxtHdr=%d PktSz=%d SrcAddr=%s DstAddr=%s",
                         tunnelStr, decodedPacket.ipProtoVersion, decodedPacket.ipProtoType, decodedPacket.ipPktSize,
                         srcIPAddrStr, destIPAddrStr);
    if (decodedPacket.ipProtoType == NL_PROTO_TYPE_UDP || decodedPacket.ipProtoType == NL_PROTO_TYPE_TCP)
    {
        WeaveLogDetail(Inet, "%s %s SrcPort=%d DstPort=%d ChkSum=%04" PRIX16,
                             tunnelStr, transport, decodedPacket.srcPort, decodedPacket.destPort, decodedPacket.checksum);
        if (decodedPacket.DoesPacketHaveWeaveMessage())
        {

            if (decodedPacket.encryptionType == nl::Weave::kWeaveEncryptionType_None)
            {
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
                snprintf(weaveFlags, sizeof(weaveFlags), "[E=%d FI=%d AR=%d CA=%d]", decodedPacket.encryptionType,
                         (decodedPacket.exchFlags & nl::Weave::kWeaveExchangeFlag_Initiator) ? 1 : 0,
                         (decodedPacket.exchFlags & nl::Weave::kWeaveExchangeFlag_NeedsAck) ? 1 : 0,
                         (decodedPacket.exchFlags & nl::Weave::kWeaveExchangeFlag_AckId) ? 1 : 0);

                WeaveLogDetail(Inet, "%s Weave Msg %08" PRIX32 ":%d Src=%016" PRIX64 " Dst=%016" PRIX64 " ExchId=%04" PRIX16 " MsgId=%08" PRIX32 " AckMsgId=%08" PRIX32 " KeyId=%d %s", tunnelStr, decodedPacket.profileId, decodedPacket.msgType, decodedPacket.srcNodeId, decodedPacket.destNodeId, decodedPacket.exchangeId, decodedPacket.messageId, decodedPacket.ackMsgId, decodedPacket.keyId, weaveFlags);
#else
                snprintf(weaveFlags, sizeof(weaveFlags), "[E=%d FI=%d]", decodedPacket.encryptionType,
                         (decodedPacket.exchFlags & nl::Weave::kWeaveExchangeFlag_Initiator) ? 1 : 0);

                WeaveLogDetail(Inet, "%s Weave Msg %08" PRIX32 ":%d Src=%016" PRIX64 " Dst=%016" PRIX64 " ExchId=%04" PRIX16 " MsgId=%08" PRIX32 " KeyId=%d %s", tunnelStr, decodedPacket.profileId, decodedPacket.msgType, decodedPacket.srcNodeId, decodedPacket.destNodeId, decodedPacket.exchangeId, decodedPacket.messageId, decodedPacket.keyId, weaveFlags);
#endif
            }
            else
            {  // Encrypted: Exclude exchange header fields.
                snprintf(weaveFlags, sizeof(weaveFlags), "[E=%d]", decodedPacket.encryptionType);

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
                WeaveLogDetail(Inet, "%s Weave Msg %08" PRIX32 ":%d Src=%016" PRIX64 " Dst=%016" PRIX64 " MsgId=%08" PRIX32 " AckMsgId=%08" PRIX32 " KeyId=%d %s", tunnelStr, decodedPacket.profileId, decodedPacket.msgType, decodedPacket.srcNodeId, decodedPacket.destNodeId, decodedPacket.messageId, decodedPacket.ackMsgId, decodedPacket.keyId, weaveFlags);
#else
                WeaveLogDetail(Inet, "%s Weave Msg %08" PRIX32 ":%d Src=%016" PRIX64 " Dst=%016" PRIX64 " MsgId=%08" PRIX32 " KeyId=%d %s", tunnelStr, decodedPacket.profileId, decodedPacket.msgType, decodedPacket.srcNodeId, decodedPacket.destNodeId, decodedPacket.messageId, decodedPacket.keyId, weaveFlags);
#endif
            }
        }
    }
    else if (decodedPacket.ipProtoType == NL_PROTO_TYPE_ICMPV6)
    {
        WeaveLogDetail(Inet, "%s ICMPv6 Type=%d Code=%d ChkSum=%04" PRIX16, tunnelStr, decodedPacket.icmpv6Type, decodedPacket.icmpv6Code,
                             decodedPacket.checksum);
    }
    else
    {
        ExitNow();
    }

exit:
    return err;
}
