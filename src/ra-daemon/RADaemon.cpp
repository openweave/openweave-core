/*
 *
 *    Copyright (c) 2018 Google LLC.
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
 *      This file implements an InetLayer-portable object for an IPv6
 *      RFC4861-compliant Router Advertisement daemon.
 *
 */

#include <string.h>//memcpy

#include <InetLayer/InetLayer.h>

#if INET_CONFIG_ENABLE_RAW_ENDPOINT

#include <Weave/Core/WeaveEncoding.h>
#include "RADaemon.h"

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
#else
#include <errno.h>
#include <string.h>
#include <stdlib.h> //rand(), srand()
#include <arpa/inet.h>
#endif

//Width of the fuzzy factor
#define RAD_FUZZY_FACTOR                        (3U * 1000U)        //3 seconds

//Immediately after a link's prefix info is updated, freuently send RAs.
#define RAD_UNSOLICITED_STARTUP_PERIOD          (15U * 1000U)       //15 seconds
#define RAD_SHORT_UNSOLICITED_STARTUP_PERIOD    (RAD_UNSOLICITED_STARTUP_PERIOD - RAD_FUZZY_FACTOR)
//NOTE: the above is done only few times after a link's prefix info is updated.
#define RAD_MAX_UNSOLICITED_STARTUP_PERIODS     (4U)

//Every 100 secs send an RA (long after the last prefix info has been updated.)
#define RAD_UNSOLICITED_PERIOD                  (100U * 1000U)       //100 seconds
#define RAD_SHORT_UNSOLICITED_PERIOD            (RAD_UNSOLICITED_PERIOD - RAD_FUZZY_FACTOR)

//Try a previous failed attempt to send a periodic multicast RA.
#define RAD_UNSOLICITED_RETRY_PERIOD            (RAD_UNSOLICITED_STARTUP_PERIOD / 3)

//At most reply to 4 RSes per minute.
#define RAD_MAX_RSES_PER_TIME_FRAME             (4U)
#define RAD_MAX_RSES_PER_TIME_FRAME_PERIOD      (60U * 1000U)       //60 seconds

extern void DumpMemory(const uint8_t *mem, uint32_t len, const char *prefix, uint32_t rowWidth);

namespace nl {
namespace Inet {

using namespace nl::Weave::Encoding;

#define RAD_IPV6_ADDR_LEN                       (16U)

#define RAD_ICMP6_TYPE_RS   133
#define RAD_ICMP6_TYPE_RA   134
uint8_t RADaemon::ICMP6Types[]          = { RAD_ICMP6_TYPE_RA };
uint8_t RADaemon::ICMP6TypesListen[]    = { RAD_ICMP6_TYPE_RS };
uint8_t RADaemon::PeriodicRAsWorked;

void RADaemon::Init(Weave::System::Layer& aSystemLayer, InetLayer& aInetLayer)
{
    SystemLayer         = &aSystemLayer;
    Inet                = &aInetLayer;
    PeriodicRAsWorked   = 0;
    memset(LinkInfo, 0, RAD_MAX_ADVERTISING_LINKS * sizeof(RADaemon::LinkInformation));
    for (int j = 0; j < RAD_MAX_ADVERTISING_LINKS; ++j)     { LinkInfo[j].Self = this; }
}

//Return a Standard Internet Checksum as described in RFC 1071.
//Code adapted from Section 4.0 "Implementation Examples", Subsection 4.1 "C".
//Verified correct behavior comparing to <http://ask.wireshark.org/questions/11061/icmp-checksum>
//And also comparing with <http://www.erg.abdn.ac.uk/~gorry/course/inet-pages/packet-dec2.html>
uint16_t RADaemon::CalculateChecksum(uint16_t *startpos, uint16_t checklen)
{
    uint32_t sum    = 0;
    uint16_t answer = 0;

    while (checklen > 1)
    {
        sum += *startpos++;
        checklen -= 2;
    }

    if (checklen == 1)
    {
        *(uint8_t *)(&answer) = *(uint8_t *)startpos;
        sum += answer;
    }

    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    answer = ~sum;

    return answer;
}

struct PrefixInfoOption
{
    uint8_t     Type;
    uint8_t     Length;
    uint8_t     PrefixLength;
    uint8_t     L_A_Reserved1;
    uint32_t    ValidLifetime;
    uint32_t    PreferredLifetime;
    uint32_t    Reserved2;
    uint8_t     Prefix[RAD_IPV6_ADDR_LEN];
};

struct RouterAdvertisementHeader
{
    uint8_t             Type;
    uint8_t             Code;
    uint16_t            Checksum;
    uint8_t             CurHopLimit;
    uint8_t             M_O_Reserved;
    uint16_t            RouterLifetime;
    uint32_t            ReacheableTime;
    uint32_t            RetransTimer;
    PrefixInfoOption    PrefixInfoOpt[RAD_MAX_PREFIXES_PER_LINK];
};

struct half
{
  uint64_t first;
  uint64_t second;
};

//ipv6_addr is IPAddr part of the IPPrefix.
//prefix is the length part of the IPPrefix.
//masked is like ipv6_addr but without the (128-prefix) less significant bits.
void mask_ipv6_address (const uint8_t *ipv6_addr, uint8_t prefix, uint8_t *masked)
{
  struct half halves;
  uint64_t mask = 0;
  unsigned char *p = masked;

  memset(masked, 0, RAD_IPV6_ADDR_LEN);
  memcpy(&halves, ipv6_addr, sizeof(halves));

  if (prefix <= 64)
  {
    mask = nl::Weave::Encoding::Swap64(nl::Weave::Encoding::Swap64(halves.first) & ((uint64_t) (~0) << (64 - prefix)));
    memcpy(masked, &mask, sizeof(uint64_t));
    return;
  }

  prefix -= 64;

  memcpy(masked, &(halves.first), sizeof(uint64_t));
  p += sizeof(uint64_t);
  mask = nl::Weave::Encoding::Swap64(nl::Weave::Encoding::Swap64(halves.second) & (uint64_t) (~0) << (64 - prefix));
  memcpy(p, &mask, sizeof(uint64_t));
}

struct PseudoHeader {
    uint16_t    PayloadLength;
    uint16_t    NextHeader;
    uint8_t     SrcAddr[16];
    uint8_t     DstAddr[16];
};

void RADaemon::BuildRA(PacketBuffer *RAPacket, RADaemon::LinkInformation *linkInfo, const IPAddress &destAddr)
{
    uint8_t     index = 0;

    RouterAdvertisementHeader *icmp6payload  = (RouterAdvertisementHeader*)RAPacket->Start();

    //Fill up the ICMPv6 header fields.
    icmp6payload->Type           = RAD_ICMP6_TYPE_RA;
    icmp6payload->Code           = 0;
    icmp6payload->Checksum       = 0;
    icmp6payload->CurHopLimit    = 0;
    icmp6payload->M_O_Reserved   = 0;
    icmp6payload->RouterLifetime = BigEndian::HostSwap16(0);
    icmp6payload->ReacheableTime = BigEndian::HostSwap32(0);
    icmp6payload->RetransTimer   = BigEndian::HostSwap32(0);

    //Fill up the prefix options, if any.
    for (int k = 0; k < RAD_MAX_PREFIXES_PER_LINK; ++k)
    {
        if (linkInfo->IPPrefixInfo[k].IPPrefx != IPPrefix::Zero)
        {
            PrefixInfoOption *prefixInfoOpt     = &icmp6payload->PrefixInfoOpt[index];
            prefixInfoOpt->Type                 = 3;
            prefixInfoOpt->Length               = 4;
            prefixInfoOpt->PrefixLength         = linkInfo->IPPrefixInfo[k].IPPrefx.Length;
            prefixInfoOpt->L_A_Reserved1        = 0xC0; //L == 1, A == 1
            prefixInfoOpt->ValidLifetime        = BigEndian::HostSwap32(linkInfo->IPPrefixInfo[k].ValidLifetime);
            prefixInfoOpt->PreferredLifetime    = BigEndian::HostSwap32(linkInfo->IPPrefixInfo[k].PreferredLifetime);
            prefixInfoOpt->Reserved2            = BigEndian::HostSwap32(0);
            memcpy(prefixInfoOpt->Prefix, linkInfo->IPPrefixInfo[k].IPPrefx.IPAddr.Addr, RAD_IPV6_ADDR_LEN);
            index++;
        }
    }//for k

    uint16_t reqSize = sizeof(RouterAdvertisementHeader) - (RAD_MAX_PREFIXES_PER_LINK - index) * sizeof(PrefixInfoOption);

    //Fill up IPv6 fields belonging to the pseudo header necessary to calculate the checksum.
    PseudoHeader *ip6payload = (PseudoHeader *)((uint8_t *)icmp6payload - sizeof(PseudoHeader));
    ip6payload->PayloadLength   = BigEndian::HostSwap16(reqSize);
    ip6payload->NextHeader      = BigEndian::HostSwap16(kIPProtocol_ICMPv6);
    memcpy(ip6payload->SrcAddr, &linkInfo->LLAddr, sizeof(ip6payload->SrcAddr));
    memcpy(ip6payload->DstAddr, &destAddr, sizeof(ip6payload->DstAddr));

    //NOTE: because the fields in the packets are already converted to BigEndian order,
    //      there is no need to convert the final result of the checksumming to such order.
    icmp6payload->Checksum = CalculateChecksum((uint16_t*)ip6payload, sizeof(PseudoHeader) + reqSize);

    //Tell the PacketBuffer about the length of the ICMP6 message.
    RAPacket->SetDataLength(reqSize);

//    Debugging
//    ::DumpMemory((const uint8_t*)ip6payload, sizeof(PseudoHeader), "", 16);
//    ::DumpMemory((const uint8_t*)icmp6payload, reqSize, "", 16);
}

void RADaemon::MulticastRA(InetLayer* inet, void* appState, INET_ERROR err)
{
    if ( (inet == NULL) || (appState == NULL) || (err != INET_NO_ERROR) )
    {
        return;
    }

    RADaemon::LinkInformation  *linkInfo    = (RADaemon::LinkInformation*)appState;

    IPAddress destAddr;
    IPAddress::FromString("FF02::1", destAddr);

    PacketBuffer *RAPacket = PacketBuffer::New();
    if (RAPacket == NULL)
        return;

    PeriodicRAsWorked = RAPacket ? 1: 0;
    if (PeriodicRAsWorked)
    {
        BuildRA(RAPacket, linkInfo, destAddr);
        linkInfo->RawEP->SendTo(destAddr, RAPacket);
    }
}

void RADaemon::MulticastPeriodicRA(Weave::System::Layer* aSystemLayer, void* aAppState, Weave::System::Error aError)
{
    RADaemon::LinkInformation*  lLinkInfo   = reinterpret_cast<RADaemon::LinkInformation*>(aAppState);
    INET_ERROR                  lError      = static_cast<INET_ERROR>(aError);
    uint32_t                    lTimeout    = RAD_UNSOLICITED_RETRY_PERIOD;
    uint32_t                    lFuzz       = 0;

    if (lError != INET_NO_ERROR)
    {
        return;
    }

    MulticastRA(lLinkInfo->Self->Inet, aAppState, lError);

    //Reschedule a new periodic mcast of RAs.
    if (PeriodicRAsWorked)
    {
        lFuzz       = rand() % (RAD_FUZZY_FACTOR * 2);
        lTimeout    = RAD_SHORT_UNSOLICITED_PERIOD;

        if (lLinkInfo->NumRAsSentSoFar++ < RAD_MAX_UNSOLICITED_STARTUP_PERIODS)
        {
            lTimeout = RAD_SHORT_UNSOLICITED_STARTUP_PERIOD;
        }
    }

    aSystemLayer->StartTimer(lTimeout + lFuzz, MulticastPeriodicRA, lLinkInfo);
}

void RADaemon::TrackRSes(Weave::System::Layer* aSystemLayer, void* aAppState, Weave::System::Error aError)
{
    RADaemon::LinkInformation*  lLinkInfo   = reinterpret_cast<RADaemon::LinkInformation*>(aAppState);
    INET_ERROR                  lError      = static_cast<INET_ERROR>(aError);

    if (lError != INET_NO_ERROR)
    {
        return;
    }

    lLinkInfo->RSesDownCounter = RAD_MAX_RSES_PER_TIME_FRAME;
    lLinkInfo->Self->SystemLayer->StartTimer(RAD_MAX_RSES_PER_TIME_FRAME_PERIOD, TrackRSes, lLinkInfo);
}

void RADaemon::UpdateLinkLocalAddr(RADaemon::LinkInformation *linkInfo, IPAddress *llAddr)
{
    //Multicast an RA with the current Link Local Address.
    MulticastRA(linkInfo->Self->Inet, linkInfo, INET_NO_ERROR);

    //Update the Link Local address of this link
    linkInfo->LLAddr = *llAddr;

    //Multicast an RA with the new Link Local Address.
    MulticastRA(linkInfo->Self->Inet, linkInfo, INET_NO_ERROR);

    linkInfo->NumRAsSentSoFar = 0;
}

void RADaemon::McastAllPrefixes(RADaemon::LinkInformation *linkInfo)
{
    //Multicast an RA.
    MulticastRA(linkInfo->Self->Inet, linkInfo, INET_NO_ERROR);

    linkInfo->NumRAsSentSoFar = 0;
}

void RADaemon::HandleReceiveError2(RawEndPoint *endPoint, INET_ERROR err, const IPPacketInfo *pktInfo)
{
}

void RADaemon::HandleReceiveError(RawEndPoint *endPoint, INET_ERROR err, const IPPacketInfo *pktInfo)
{
}

struct RSPacketHdr
{
    uint8_t     Type;
    uint8_t     Code;
    uint16_t    Checksum;
    uint32_t    Reserved;
};

struct RSOpt
{
    uint8_t     OptType;
    uint8_t     OptLen;
};

void RADaemon::HandleMessageReceived2(RawEndPoint *RawEPListen, PacketBuffer *msg, const IPPacketInfo *pktInfo)
{
}

void RADaemon::HandleMessageReceived(RawEndPoint *RawEPListen, PacketBuffer *msg, const IPPacketInfo *pktInfo)
{
    uint8_t             msgDataLen      = msg->DataLength();
    RSPacketHdr        *RSPacket        = (RSPacketHdr *)msg->Start();
    RSPacketHdr        *RSPacketEnd     = (RSPacketHdr *)((uint8_t *)RSPacket + msgDataLen);
    LinkInformation    *currLinkInfo    = NULL;
    PacketBuffer       *RAPacket        = NULL;

//    Debugging
//    char senderAddrStr[64];
//    pktInfo->SrcAddress.ToString(senderAddrStr, sizeof(senderAddrStr));
//    printf("Raw message received from %s (%ld bytes)\n", senderAddrStr, msgDataLen);
//    ::DumpMemory((const uint8_t *)RSPacket, msgDataLen, "  ", 16);

    //Error checks
    if (
            (RSPacket->Type != RAD_ICMP6_TYPE_RS) ||
            (RSPacket->Code != 0)
            //TODO: verify checksum
       )
    {
        goto finalize;
    }

    currLinkInfo = (RADaemon::LinkInformation *)RawEPListen->AppState;
    if (currLinkInfo->RSesDownCounter-- == 0)
    {
        currLinkInfo->RSesDownCounter = 0;
        goto finalize;
    }

    if (msgDataLen <= sizeof(RSPacketHdr))
    {
        //No options present
    }
    else
    {
        RSOpt *rsOpt = (RSOpt *)((uint8_t *)RSPacket + sizeof(RSPacketHdr));
        while (((uint8_t *)rsOpt < (uint8_t *)RSPacketEnd) && (rsOpt->OptType != 1))
        {
            rsOpt = (RSOpt *)((uint8_t *)rsOpt + rsOpt->OptLen);
        }
        if (((uint8_t *)rsOpt < (uint8_t *)RSPacketEnd) && (rsOpt->OptType == 1))
        {
//            For the time being the Mac addr is not used to decide how to send
//            the RA, but it is printed here, to prove that its parsing works.
//            printf("Source Link Layer Address Option: %x:%x:%x:%x:%x:%x\n",
//                    (&rsOpt->OptType+2)[0], (&rsOpt->OptType+2)[1], (&rsOpt->OptType+2)[2],
//                    (&rsOpt->OptType+2)[3], (&rsOpt->OptType+2)[4], (&rsOpt->OptType+2)[5]);
        }
        else
        {
            //No 'Source Link Layer Address' found.
        }
    }

    RAPacket = PacketBuffer::New();
    if (RAPacket == NULL)
        goto finalize;

    if (pktInfo == NULL)
        goto finalize;

    if (pktInfo->SrcAddress == IPAddress::Any)
    {
        uint32_t lTimeout = RAD_SHORT_UNSOLICITED_PERIOD;
        const uint32_t lFuzz = rand() % (RAD_FUZZY_FACTOR * 2);
        IPAddress mcastAddr;
        IPAddress::FromString("FF02::1", mcastAddr);
        RADaemon::BuildRA(RAPacket, currLinkInfo, mcastAddr);
        currLinkInfo->RawEP->SendTo(mcastAddr, RAPacket);

        //Since mcast has been used to reply to this RS, reschedule a new periodic mcast of RAs.
        RADaemon *self = currLinkInfo->Self;
        Weave::System::Layer* lSystemLayer = self->SystemLayer;

        if (currLinkInfo->NumRAsSentSoFar < RAD_MAX_UNSOLICITED_STARTUP_PERIODS)
        {
            lTimeout = RAD_SHORT_UNSOLICITED_STARTUP_PERIOD;
        }

        lSystemLayer->StartTimer(lTimeout + lFuzz, MulticastPeriodicRA, currLinkInfo);
    }
    else
    {
        RADaemon::BuildRA(RAPacket, currLinkInfo, pktInfo->SrcAddress);
        currLinkInfo->RawEP->SendTo(pktInfo->SrcAddress, RAPacket);
    }

finalize:
    PacketBuffer::Free(msg);
}

INET_ERROR  RADaemon::SetPrefixInfo(InterfaceId link, IPAddress llAddr, IPPrefix ipPrefix, uint32_t validLifetime,
    uint32_t preferredLifetime)
{
//    char buf[IFNAMSIZE];
    IPPrefixInformation            *currIPPrefixInfo;
    IPPrefixInformation            *freeIPPrefixInfo    = NULL;
    RADaemon::LinkInformation      *currLinkInfo;
    RADaemon::LinkInformation      *freeLinkInfo        = NULL;
    INET_ERROR                      err                 = INET_NO_ERROR;

    if ( !IsInterfaceIdPresent(link) || (ipPrefix == IPPrefix::Zero) )
    {
        return INET_ERROR_BAD_ARGS;
    }

    if (llAddr == IPAddress::Any)
    {
        err = Inet->GetLinkLocalAddr(link, &llAddr);
        if (err != INET_NO_ERROR)
        {
            return err;
        }
    }

    //Reset the bits in the ipPrefix that fall outside its length.
    IPAddress tmpPrefix;
    mask_ipv6_address((const uint8_t *)&ipPrefix.IPAddr, ipPrefix.Length, (uint8_t *)&tmpPrefix);
    ipPrefix.IPAddr = tmpPrefix;

    for (int j = 0; j < RAD_MAX_ADVERTISING_LINKS; ++j)
    {//Try updating an EXISTING entry
        currLinkInfo = &LinkInfo[j];
        if (currLinkInfo->FSMState != FSM_NO_PREFIX)
        {
            if ( link == currLinkInfo->Link )
            {
                if (currLinkInfo->LLAddr != llAddr)
                {
                    //RFC 4861, Section 6.2.8. 'Link Local Address Change':
                    // "If a router changes the link-local address for one of its interfaces,
                    //  it SHOULD inform hosts of this change.  The router SHOULD multicast a
                    //  few Router Advertisements from the old link-local address with the
                    //  Router Lifetime field set to zero and also multicast a few Router
                    //  Advertisements from the new link-local address."
                    UpdateLinkLocalAddr(currLinkInfo, &llAddr);
                }
                for (int k = 0; k < RAD_MAX_PREFIXES_PER_LINK; ++k)
                {//Look for the passed prefix
                    currIPPrefixInfo = &currLinkInfo->IPPrefixInfo[k];
                    if (currIPPrefixInfo->IPPrefx == ipPrefix)
                    {//Update existing prefix with latest info
                        currIPPrefixInfo->ValidLifetime     = validLifetime;
                        currIPPrefixInfo->PreferredLifetime = preferredLifetime;
                        //RFC 4861, Section 4.2 'Router Advertisement Message Format', Subsection 'Prefix Information':
                        // "A router SHOULD include all its on-link prefixes (except the link-local prefix) so
                        //  that multihomed hosts have complete prefix information about on-link destinations for the
                        //  links to which they attach."
                        McastAllPrefixes(currLinkInfo);
                        goto out;
                    }
                    else if (freeIPPrefixInfo == NULL && currIPPrefixInfo->IPPrefx == IPPrefix::Zero)
                    {//Keep track of the first free prefix, in case is needed later
                        freeIPPrefixInfo = currIPPrefixInfo;
                    }
                }//for k

                if (freeIPPrefixInfo == NULL)
                {//No free space to store passed prefix info
                    err = INET_ERROR_NO_MEMORY;
                }
                else
                {//Save passed prefix with its associated info
                    freeIPPrefixInfo->IPPrefx           = ipPrefix;
                    freeIPPrefixInfo->ValidLifetime     = validLifetime;
                    freeIPPrefixInfo->PreferredLifetime = preferredLifetime;
                    McastAllPrefixes(currLinkInfo);
                }
                goto out;
            }
        }
        else if (freeLinkInfo == NULL)
        {//Keep track of the first free entry, in case is needed later
            freeLinkInfo = currLinkInfo;
        }
    }//for j

    if (freeLinkInfo == NULL)
    {//No free space for passed interface
        err = INET_ERROR_NO_MEMORY;
        goto out;
    }

    //Use free space to store passed information.
    err = Inet->NewRawEndPoint(kIPVersion_6, kIPProtocol_ICMPv6, &freeLinkInfo->RawEP);
    if (err != INET_NO_ERROR)
    {
        goto out;
    }

    err = Inet->NewRawEndPoint(kIPVersion_6, kIPProtocol_ICMPv6, &freeLinkInfo->RawEPListen);
    if (err != INET_NO_ERROR)
    {
        goto release_first_rawep;
    }

    freeLinkInfo->RawEP->AppState                   = freeLinkInfo;
    freeLinkInfo->RawEP->OnMessageReceived          = reinterpret_cast<IPEndPointBasis::OnMessageReceivedFunct>(RADaemon::HandleMessageReceived2);
    freeLinkInfo->RawEP->OnReceiveError             = reinterpret_cast<IPEndPointBasis::OnReceiveErrorFunct>(RADaemon::HandleReceiveError2);
    freeLinkInfo->RawEPListen->AppState             = freeLinkInfo;
    freeLinkInfo->RawEPListen->OnMessageReceived    = reinterpret_cast<IPEndPointBasis::OnMessageReceivedFunct>(RADaemon::HandleMessageReceived);
    freeLinkInfo->RawEPListen->OnReceiveError       = reinterpret_cast<IPEndPointBasis::OnReceiveErrorFunct>(RADaemon::HandleReceiveError);

    err = freeLinkInfo->RawEP->BindIPv6LinkLocal(link, llAddr);
    if (err != INET_NO_ERROR)
    {
        goto release_both_raweps;
    }

    err = freeLinkInfo->RawEP->SetICMPFilter(sizeof(ICMP6Types) / sizeof(uint8_t), ICMP6Types);
    if (err != INET_NO_ERROR)
    {
        goto release_both_raweps;
    }

    err = freeLinkInfo->RawEPListen->BindIPv6LinkLocal(link, llAddr);
    if (err != INET_NO_ERROR)
    {
        goto release_both_raweps;
    }

    err = freeLinkInfo->RawEPListen->SetICMPFilter(sizeof(ICMP6TypesListen) / sizeof(uint8_t), ICMP6TypesListen);
    if (err != INET_NO_ERROR)
    {
        goto release_both_raweps;
    }

    err = freeLinkInfo->RawEPListen->Listen();
    if (err != INET_NO_ERROR)
    {
        goto release_both_raweps;
    }

    freeLinkInfo->FSMState                              = FSM_ADVERTISING;
    freeLinkInfo->Link                                  = link;
    freeLinkInfo->LLAddr                                = llAddr;
    freeLinkInfo->IPPrefixInfo[0].IPPrefx               = ipPrefix;
    freeLinkInfo->IPPrefixInfo[0].ValidLifetime         = validLifetime;
    freeLinkInfo->IPPrefixInfo[0].PreferredLifetime     = preferredLifetime;
    freeLinkInfo->NumRAsSentSoFar = 0;
    MulticastPeriodicRA(SystemLayer, freeLinkInfo, WEAVE_SYSTEM_NO_ERROR);
    TrackRSes(SystemLayer, freeLinkInfo, WEAVE_SYSTEM_NO_ERROR);
    goto out;

release_both_raweps:
    freeLinkInfo->RawEPListen->Free();
    freeLinkInfo->RawEPListen = NULL;

release_first_rawep:
    freeLinkInfo->RawEP->Free();
    freeLinkInfo->RawEP = NULL;

out:
    return err;
}

//Delete the prefix associated with the passed interface.
void RADaemon::DelPrefixInfo(InterfaceId link, IPPrefix ipPrefix)
{
    IPPrefixInformation        *currIPPrefixInfo;
    RADaemon::LinkInformation  *currLinkInfo;
    uint8_t                     num_free_prefixes   = 0;
    uint8_t                     prefix_removed      = 0;

    if ( !IsInterfaceIdPresent(link) || (ipPrefix == IPPrefix::Zero) )
    {
        return;
    }

    for (int j = 0; j < RAD_MAX_ADVERTISING_LINKS; ++j)
    {
        currLinkInfo = &LinkInfo[j];
        if ( link == currLinkInfo->Link )
        {
            for (int k = 0; k < RAD_MAX_PREFIXES_PER_LINK; ++k)
            {
                currIPPrefixInfo = &currLinkInfo->IPPrefixInfo[k];
                if (currIPPrefixInfo->IPPrefx == ipPrefix)
                {
                    memset(&currIPPrefixInfo->IPPrefx, 0, sizeof(IPPrefix));
                    prefix_removed = 1;
                    num_free_prefixes++;
                }
                else if (LinkInfo[j].IPPrefixInfo[k].IPPrefx == IPPrefix::Zero)
                {
                    num_free_prefixes++;
                }
            }//for k

            if (num_free_prefixes == RAD_MAX_PREFIXES_PER_LINK)
            {
                DelLinkInfo(link);
            }
            break;//from for j
        }
    }//for j

    if (prefix_removed == 1 && num_free_prefixes != RAD_MAX_PREFIXES_PER_LINK)
    {
        McastAllPrefixes(currLinkInfo);
    }
}

//Free all the information associated with the passed interface.
void RADaemon::DelLinkInfo(InterfaceId link)
{
    if ( !IsInterfaceIdPresent(link) )
    {
        return;
    }

    for (int j = 0; j < RAD_MAX_ADVERTISING_LINKS; ++j)
    {
        if ( link == LinkInfo[j].Link )
        {
            RADaemon::LinkInformation *currLinkInfo = &LinkInfo[j];
            currLinkInfo->RawEPListen->Free();
            currLinkInfo->RawEPListen = NULL;
            currLinkInfo->RawEP->Free();
            currLinkInfo->RawEP = NULL;
            SystemLayer->CancelTimer(MulticastPeriodicRA, currLinkInfo);
            SystemLayer->CancelTimer(TrackRSes, currLinkInfo);
            memset(currLinkInfo, 0, sizeof(RADaemon::LinkInformation));
            currLinkInfo->Self = this;
            break;
        }
    }
}


} /* namespace Inet */
} /* namespace nl */

#endif // INET_CONFIG_ENABLE_RAW_ENDPOINT
