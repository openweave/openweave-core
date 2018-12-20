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
 *      This file defines an InetLayer-portable object for an IPv6
 *      RFC4861-compliant Router Advertisement daemon.
 *
 */

#ifndef RADAEMON_H_
#define RADAEMON_H_

#include <InetLayer/InetLayer.h>

#if INET_CONFIG_ENABLE_RAW_ENDPOINT

#include <InetLayer/RawEndPoint.h>
#include <InetLayer/IPPrefix.h>
#include <Weave/Core/WeaveEncoding.h>

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
#else
#include <errno.h>
#endif

#define RAD_MAX_ADVERTISING_LINKS   2
#define RAD_MAX_PREFIXES_PER_LINK   4

#define IFNAMSIZE 16


namespace nl {
namespace Inet {

using ::nl::Weave::System::PacketBuffer;

#define FSM_NO_PREFIX       0  //This state must always be zero.
#define FSM_ADVERTISING     1

// RADaemon -- The object containing the per-link FSMs that periodically or on demand send Router Advertisements.
// NOTE: it is assumed that a single thread instanciates a single RADaemon object.
class RADaemon
{
private:
    struct  IPPrefixInformation
    {
        IPPrefix    IPPrefx;
        uint32_t    ValidLifetime;
        uint32_t    PreferredLifetime;
    };

    struct  LinkInformation
    {
        uint8_t                 FSMState;
        int8_t                  RSesDownCounter;
        uint16_t                NumRAsSentSoFar;
        InterfaceId             Link;
        IPAddress               LLAddr;
        RawEndPoint            *RawEP;          //Used to send all RAs
        RawEndPoint            *RawEPListen;    //Used to received RSes
        RADaemon               *Self;
        IPPrefixInformation     IPPrefixInfo[RAD_MAX_PREFIXES_PER_LINK];
    };

    static void MulticastRA(InetLayer* inet, void* appState, INET_ERROR err);
    static void HandleMessageReceived2(RawEndPoint *RawEPListen, PacketBuffer *msg, const IPPacketInfo *pktInfo);
    static void HandleReceiveError2(RawEndPoint *endPoint, INET_ERROR err, const IPPacketInfo *pktInfo);
    static void HandleMessageReceived(RawEndPoint *RawEPListen, PacketBuffer *msg, const IPPacketInfo *pktInfo);
    static void HandleReceiveError(RawEndPoint *endPoint, INET_ERROR err, const IPPacketInfo *pktInfo);
    static void BuildRA(PacketBuffer *RAPacket, LinkInformation *linkInfo, const IPAddress &destAddr);
    static uint16_t CalculateChecksum(uint16_t *startpos, uint16_t checklen);
    static void MulticastPeriodicRA(Weave::System::Layer* aSystemLayer, void* aAppState, Weave::System::Error aError);
    static void TrackRSes(Weave::System::Layer* aSystemLayer, void* aAppState, Weave::System::Error aError);
    static void UpdateLinkLocalAddr(LinkInformation *linkInfo, IPAddress *llAddr);
    static void McastAllPrefixes(LinkInformation *linkInfo);

public:
    Weave::System::Layer*   SystemLayer;
    InetLayer*              Inet;
    LinkInformation         LinkInfo[RAD_MAX_ADVERTISING_LINKS];

    static uint8_t ICMP6Types[];
    static uint8_t ICMP6TypesListen[];
    static uint8_t PeriodicRAsWorked;

    void        Init(Weave::System::Layer& aSystemLayer, InetLayer& aInetLayer);

#if INET_CONFIG_PROVIDE_OBSOLESCENT_INTERFACES
    void        Init(InetLayer* aInetLayer);
#endif

    INET_ERROR  SetPrefixInfo(InterfaceId link, IPAddress llAddr, IPPrefix ipPrefix, uint32_t validLifetime, uint32_t preferredLifetime);
    void        DelPrefixInfo(InterfaceId link, IPPrefix ipPrefix);
    void        DelLinkInfo(InterfaceId link);
};

#if INET_CONFIG_PROVIDE_OBSOLESCENT_INTERFACES
inline void RADaemon::Init(InetLayer* aInetLayer)
{
    this->Init(*aInetLayer->SystemLayer(), *aInetLayer);
}
#endif // INET_CONFIG_PROVIDE_OBSOLESCENT_INTERFACES

} /* namespace Inet */
} /* namespace nl */

#endif // INET_CONFIG_ENABLE_RAW_ENDPOINT

#endif /* RADAEMON_H_ */
