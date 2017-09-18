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
#ifndef _IP_PACKET_DECODER_H_
#define _IP_PACKET_DECODER_H_

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif // __STDC_FORMAT_MACROS

#define NL_IP6_ADDR_LEN_IN_BYTES  16
#define NL_IP_VERSION_6  6
#define NL_IP6_HDR_LEN  40
#define NL_UDP_HDR_LEN  8
#define NL_TCP_MIN_HDR_LEN  20
#define NL_PROTO_TYPE_TCP  6
#define NL_PROTO_TYPE_UDP  17
#define NL_PROTO_TYPE_ICMPV6  58

class DecodedIPPacket
{
public:

    // IP and TCP/UDP header fields
    uint8_t srcAddr[NL_IP6_ADDR_LEN_IN_BYTES];
    uint8_t destAddr[NL_IP6_ADDR_LEN_IN_BYTES];
    uint16_t srcPort;        // UDP or TCP source port number.
    uint16_t destPort;       // UDP or TCP destination port number.
    uint16_t checksum;       // UDP, TCP or ICMP checksum.
    uint16_t ipPktSize;      // The IP packet size in bytes.
    uint8_t ipProtoVersion;  // IP protocol version, viz, 4 or 6.
    uint8_t ipProtoType;     // The next header protocol type.

    // ICMPv6 header fields
    uint8_t icmpv6Type;      // ICMPv6 type.
    uint8_t icmpv6Code;      // ICMPv6 code.

    // Weave Message header fields
    uint64_t srcNodeId;      // Weave Source Node Identifier.
    uint64_t destNodeId;     // Weave Destination Node Identifier.
    uint32_t messageId;      // Weave Message Identifier.
    uint16_t msgHdrFlags;    // Weave Message header flag bits.
    uint8_t encryptionType;  // Weave Message encryption type.
    uint8_t keyId;           // Weave Message key id.

    // Weave Exchange header fields
    uint32_t profileId;      // Weave Message Profile Identifier.
    uint32_t ackMsgId;       // Acknowledgment Message Identifier.
    uint16_t msgType;        // Message type within specified Profile.
    uint16_t exchangeId;     // Weave Exchange Identifier.
    uint8_t  exchFlags;      // Bit flag indicators for the Weave Message.

    // Decode the IP packet.

    WEAVE_ERROR PacketHeaderDecode(const uint8_t *pkt, uint16_t pktLen);

    // Check whether a decoded packet contains a Weave message.

    bool DoesPacketHaveWeaveMessage(void) const;
private:

    uint64_t GetWeaveNodeIdFromAddr(const uint8_t *addr);

    WEAVE_ERROR ParseWeaveMessageHeader(const uint8_t *pkt, const uint8_t *pktStart, const uint8_t **payloadStart);
};

INET_ERROR LogPacket(const DecodedIPPacket &decodedPacket, bool isTunneled);

#endif //_IP_PACKET_DECODER_H_
