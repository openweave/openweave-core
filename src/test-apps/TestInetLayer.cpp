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
 *      This file implements a process to effect a functional test for
 *      the InetLayer Internet Protocol stack abstraction interfaces.
 *
 */

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "ToolCommon.h"
#include <SystemLayer/SystemTimer.h>

using namespace nl::Inet;

#define TOOL_NAME "TestInetLayer"

static bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg);
static bool HandleNonOptionArgs(const char *progName, int argc, char *argv[]);
static void StartTest();
static void DriveSend();
static void HandleConnectionComplete(TCPEndPoint *ep, INET_ERROR conErr);
static void HandleConnectionReceived(TCPEndPoint *listeningEP, TCPEndPoint *conEP, const IPAddress &peerAddr,
        uint16_t peerPort);
static void HandleConnectionClosed(TCPEndPoint *ep, INET_ERROR err);
static void HandleDataSent(TCPEndPoint *ep, uint16_t len);
static void HandleDataReceived(TCPEndPoint *ep, PacketBuffer *data);
static void HandleAcceptError(TCPEndPoint *endPoint, INET_ERROR err);
static void HandleRawMessageReceived(RawEndPoint *endPoint, PacketBuffer *msg, IPAddress senderAddr);
static void HandleRawReceiveError(RawEndPoint *endPoint, INET_ERROR err, IPAddress senderAddr);
static void HandleUDPMessageReceived(UDPEndPoint *endPoint, PacketBuffer *msg, const IPPacketInfo *pktInfo);
static void HandleUDPReceiveError(UDPEndPoint *endPoint, INET_ERROR err, const IPPacketInfo *pktInfo);
static void HandleSendTimerComplete(System::Layer* aSystemLayer, void* aAppState, System::Error aError);
static PacketBuffer *MakeDataBuffer(int32_t len);

bool Listen = false;
const char *destHostName = NULL;
IPAddress DestAddr = IPAddress::Any;
bool IsTimeToSend = true;
int32_t SendInterval = 1000; // in ms
int32_t SendLength = 3200;
int32_t MaxSendLength = -1;
int32_t MinRcvLength = 0;
int32_t MaxRcvLength = -1;
int32_t TotalSendLength = 0;
int32_t TotalRcvLength = 0;
const char *IntfFilterName = NULL;
#if INET_CONFIG_ENABLE_IPV4
bool UseRaw4 = false;
#endif // INET_CONFIG_ENABLE_IPV4
bool UseRaw6 = false;
bool UseTCP = false;
TCPEndPoint *TCPEP = NULL;
TCPEndPoint *ListenEP = NULL;
UDPEndPoint *UDPEP = NULL;
RawEndPoint *Raw4EP = NULL;
RawEndPoint *Raw6EP = NULL;

static OptionDef gToolOptionDefs[] =
{
    { "intf-filter",        kArgumentRequired,  'F' },
    { "length",             kArgumentRequired,  'l' },
    { "max-receive",        kArgumentRequired,  'r' },
    { "max-send",           kArgumentRequired,  's' },
    { "interval",           kArgumentRequired,  'i' },
    { "listen",             kNoArgument,        'L' },
#if INET_CONFIG_ENABLE_IPV4
    { "raw4",               kNoArgument,        '4' },
#endif // INET_CONFIG_ENABLE_IPV4
    { "raw6",               kNoArgument,        '6' },
    { "tcp",                kNoArgument,        't' },
    { NULL }
};

static const char *gToolOptionHelp =
    "  -F, --intf-filter <interface-name>\n"
    "       Name of the interface to filter traffic from/to.\n"
    "\n"
    "  -l, --length <num>\n"
    "       Send specified number of bytes.\n"
    "\n"
    "  -r, --max-receive <num>\n"
    "       Maximum bytes to receive per connection.\n"
    "\n"
    "  -s, --max-send <num>\n"
    "       Maximum bytes to send per connection.\n"
    "\n"
    "  -i, --interval <ms>\n"
    "       Send data at the specified interval in milliseconds.\n"
    "\n"
    "  -L, --listen\n"
    "       Listen for incoming data.\n"
    "\n"
#if INET_CONFIG_ENABLE_IPV4
    "  -4, --raw4\n"
    "       Use Raw IPv4. Defaults to using UDP.\n"
    "\n"
#endif // INET_CONFIG_ENABLE_IPV4
    "  -6, --raw6\n"
    "       Use Raw IPv6. Defaults to using UDP.\n"
    "\n"
    "  -t, --tcp\n"
    "       Use TCP. Defaults to using UDP.\n"
    "\n";

static OptionSet gToolOptions =
{
    HandleOption,
    gToolOptionDefs,
    "GENERAL OPTIONS",
    gToolOptionHelp
};

static HelpOptions gHelpOptions(
    TOOL_NAME,
    "Usage: " TOOL_NAME " <options> <src-node-addr> <dest-node-addr>\n"
    "       " TOOL_NAME " <options> <src-node-addr> --listen\n",
    WEAVE_VERSION_STRING "\n" WEAVE_TOOL_COPYRIGHT
);

static OptionSet *gToolOptionSets[] =
{
    &gToolOptions,
    &gNetworkOptions,
    &gFaultInjectionOptions,
    &gHelpOptions,
    NULL
};

#if INET_CONFIG_ENABLE_DNS_RESOLVER
static bool DNSResolutionComplete = false;
static void HandleDNSResolveComplete(void *appState, INET_ERROR err, uint8_t addrCount, IPAddress *addrArray);
#endif // INET_CONFIG_ENABLE_DNS_RESOLVER

int main(int argc, char *argv[])
{
    SetSignalHandler(DoneOnHandleSIGUSR1);

    if (argc == 1)
    {
        gHelpOptions.PrintBriefUsage(stderr);
        exit(EXIT_FAILURE);
    }

    if (!ParseArgsFromEnvVar(TOOL_NAME, TOOL_OPTIONS_ENV_VAR_NAME, gToolOptionSets, NULL, true) ||
        !ParseArgs(TOOL_NAME, argc, argv, gToolOptionSets, HandleNonOptionArgs))
    {
        exit(EXIT_FAILURE);
    }

    InitSystemLayer();
    InitNetwork();

    StartTest();

    while (!Done)
    {
        struct timeval sleepTime;
        sleepTime.tv_sec = 0;
        sleepTime.tv_usec = 10000;

        ServiceNetwork(sleepTime);
    }

    if (TCPEP)
        TCPEP->Shutdown();

    if (ListenEP)
        ListenEP->Shutdown();

    Inet.Shutdown();

    return EXIT_SUCCESS;
}

bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg)
{
    switch (id)
    {
#if INET_CONFIG_ENABLE_IPV4
    case '4':
        UseRaw4 = true;
        break;
#endif // INET_CONFIG_ENABLE_IPV4
    case '6':
        UseRaw6 = true;
        break;
    case 't':
        UseTCP = true;
        break;
    case 'L':
        Listen = true;
        break;
    case 'l':
        if (!ParseInt(arg, SendLength) || SendLength < 0 || SendLength > UINT16_MAX)
        {
            PrintArgError("%s: Invalid value specified for data length: %s\n", progName, arg);
            return false;
        }
        break;
    case 'r':
        if (!ParseInt(arg, MaxRcvLength) || MaxRcvLength < 0)
        {
            PrintArgError("%s: Invalid value specified for max receive: %s\n", progName, arg);
            return false;
        }
        break;
    case 's':
        if (!ParseInt(arg, MaxSendLength) || MaxSendLength < 0)
        {
            PrintArgError("%s: Invalid value specified for max send: %s\n", progName, arg);
            return false;
        }
        break;
    case 'i':
        if (!ParseInt(arg, SendInterval) || SendInterval < 0 || SendInterval > INT32_MAX)
        {
            PrintArgError("%s: Invalid value specified for send interval: %s\n", progName, arg);
            return false;
        }
        break;
    case 'F':
        IntfFilterName = arg;
        break;
    default:
        PrintArgError("%s: INTERNAL ERROR: Unhandled option: %s\n", progName, name);
        return false;
    }

    return true;
}

bool HandleNonOptionArgs(const char *progName, int argc, char *argv[])
{
    if (!Listen)
    {
        if (argc == 0)
        {
            PrintArgError("%s: Please specify a destination name or address\n", progName);
            return false;
        }

        destHostName = argv[0];

        argc--; argv++;
    }
    else
    {
        // If listening and a send length wasn't explicitly specified, then don't send anything.
        // NOTE: this only applies when using TCP.
        if (MaxSendLength == -1)
            MaxSendLength = 0;
    }

    if (argc > 0)
    {
        PrintArgError("%s: TestInetLayer: Unexpected argument: %s\n", progName, argv[0]);
        return false;
    }

    return true;
}

#define   NUM_ICMP6_FILTER_TYPES 2
uint8_t ICMP6Types[NUM_ICMP6_FILTER_TYPES] =
{ 128, 129 }; //Echo Request, Echo Reply
//#define     NUM_ICMP6_FILTER_TYPES 1
//uint8_t     ICMP6Types[NUM_ICMP6_FILTER_TYPES] = {255}; //Reserved for expansion of ICMPv6 informational messages

void StartTest()
{
    INET_ERROR err;

    IsTimeToSend = (MaxSendLength != 0);

    if (UseRaw6)
    {
        printf("UseRaw6, if: %s (WEAVE_SYSTEM_CONFIG_USE_LWIP: %d)\n", IntfFilterName, WEAVE_SYSTEM_CONFIG_USE_LWIP);
        err = Inet.NewRawEndPoint(kIPVersion_6, kIPProtocol_ICMPv6, &Raw6EP);
        FAIL_ERROR(err, "InetLayer::NewRawEndPoint (IPv6) failed");
        if (IntfFilterName != NULL)
        {
            InterfaceId intfId;
            err = InterfaceNameToId(IntfFilterName, intfId);
            FAIL_ERROR(err, "InterfaceNameToId failed");
            err = Raw6EP->BindInterface(intfId);
            FAIL_ERROR(err, "RawEndPoint::BindInterface (IPv6) failed");
        }
        Raw6EP->OnMessageReceived = HandleRawMessageReceived;
        Raw6EP->OnReceiveError = HandleRawReceiveError;
    }
#if INET_CONFIG_ENABLE_IPV4
    else if (UseRaw4)
    {
        err = Inet.NewRawEndPoint(kIPVersion_4, kIPProtocol_ICMPv4, &Raw4EP);
        FAIL_ERROR(err, "InetLayer::NewRawEndPoint (IPv4) failed");
        if (IntfFilterName != NULL)
        {
            InterfaceId intfId;
            err = InterfaceNameToId(IntfFilterName, intfId);
            FAIL_ERROR(err, "InterfaceNameToId failed");
            err = Raw4EP->BindInterface(intfId);
            FAIL_ERROR(err, "RawEndPoint::BindInterface (IPv4) failed");
        }
        Raw4EP->OnMessageReceived = HandleRawMessageReceived;
        Raw4EP->OnReceiveError = HandleRawReceiveError;
    }
#endif // INET_CONFIG_ENABLE_IPV4
    else if (!UseTCP)
    {
        err = Inet.NewUDPEndPoint(&UDPEP);
        FAIL_ERROR(err, "InetLayer::NewUDPEndPoint failed");
        if (IntfFilterName != NULL)
        {
            InterfaceId intfId;
            err = InterfaceNameToId(IntfFilterName, intfId);
            FAIL_ERROR(err, "InterfaceNameToId failed");
            err = UDPEP->BindInterface(kIPAddressType_IPv6, intfId);
            FAIL_ERROR(err, "RawEndPoint::BindInterface (IPv4) failed");
        }
        UDPEP->OnMessageReceived = HandleUDPMessageReceived;
        UDPEP->OnReceiveError = HandleUDPReceiveError;
    }

    if (Listen)
    {
        if (UseRaw6)
        {
            err = Raw6EP->Bind(kIPAddressType_IPv6, gNetworkOptions.LocalIPv6Addr);
            FAIL_ERROR(err, "RawEndPoint::Bind (IPv6) failed");

            if (NUM_ICMP6_FILTER_TYPES == 1)
                printf("NumICMP6Types: %d: [0]: %d\n", NUM_ICMP6_FILTER_TYPES, ICMP6Types[0]);
            if (NUM_ICMP6_FILTER_TYPES == 2)
                printf("NumICMP6Types: %d: [0]: %d, [1]: %d\n", NUM_ICMP6_FILTER_TYPES, ICMP6Types[0], ICMP6Types[1]);
            err = Raw6EP->SetICMPFilter(NUM_ICMP6_FILTER_TYPES, ICMP6Types);
            FAIL_ERROR(err, "RawEndPoint::SetICMPFilter (IPv6) failed");

            err = Raw6EP->Listen();
            FAIL_ERROR(err, "RawEndPoint::Listen (IPv6) failed");
        }
#if INET_CONFIG_ENABLE_IPV4
        else if (UseRaw4)
        {
            err = Raw4EP->Bind(kIPAddressType_IPv4, gNetworkOptions.LocalIPv4Addr);
            FAIL_ERROR(err, "RawEndPoint::Bind (IPv4) failed");

            err = Raw4EP->Listen();
            FAIL_ERROR(err, "RawEndPoint::Listen (IPv4) failed");
        }
#endif // INET_CONFIG_ENABLE_IPV4
        else if (UseTCP)
        {
            err = Inet.NewTCPEndPoint(&ListenEP);
            FAIL_ERROR(err, "InetLayer::NewTCPEndPoint failed");

            ListenEP->OnConnectionReceived = HandleConnectionReceived;
            ListenEP->OnAcceptError = HandleAcceptError;

            err = ListenEP->Bind(kIPAddressType_IPv6, IPAddress::Any, 4242, true);
            FAIL_ERROR(err, "TCPEndPoint::Bind failed");

            err = ListenEP->Listen(1);
            FAIL_ERROR(err, "TCPEndPoint::Listen failed");
        }
        else
        {
            err = UDPEP->Bind(kIPAddressType_IPv6, IPAddress::Any, 4242);
            FAIL_ERROR(err, "UDPEndPoint::Bind failed");

            err = UDPEP->Listen();
            FAIL_ERROR(err, "UDPEndPoint::Listen failed");
        }

        printf("Listening...\n");
    }

    DriveSend();
}

void DriveSend()
{
    INET_ERROR err;

    if (!IsTimeToSend)
        return;

    if (TotalSendLength == MaxSendLength) {
        Inet.Shutdown();
        Done = true;
        return;
    }

#if INET_CONFIG_ENABLE_DNS_RESOLVER
    if (!DNSResolutionComplete && !Listen)
    {
        printf("Resolving destination name...\n");

        err = Inet.ResolveHostAddress(destHostName, 1, &DestAddr, HandleDNSResolveComplete, NULL);
        FAIL_ERROR(err, "InetLayer::ResolveHostAddress failed");

        return;
    }
#endif // INET_CONFIG_ENABLE_DNS_RESOLVER

    if (UseTCP)
    {
        if (TCPEP == NULL)
        {
            if (Listen)
                return;

            err = Inet.NewTCPEndPoint(&TCPEP);
            FAIL_ERROR(err, "InetLayer::NewTCPEndPoint failed");

            TCPEP->OnConnectComplete = HandleConnectionComplete;
            TCPEP->OnConnectionClosed = HandleConnectionClosed;
            TCPEP->OnDataSent = HandleDataSent;
            TCPEP->OnDataReceived = HandleDataReceived;

            err = TCPEP->Connect(DestAddr, 4242);
            FAIL_ERROR(err, "TCPEndPoint::Connect failed");
        }

        if (TCPEP->State != TCPEndPoint::kState_Connected && TCPEP->State != TCPEndPoint::kState_ReceiveShutdown)
            return;

        if (TCPEP->PendingSendLength() > 0)
            return;

        int32_t sendLen = SendLength;
        if (MaxSendLength != -1)
        {
            int32_t remainingLen = MaxSendLength - TotalSendLength;
            if (sendLen > remainingLen)
                sendLen = remainingLen;
        }

        IsTimeToSend = false;
        SystemLayer.StartTimer(SendInterval, HandleSendTimerComplete, NULL);

        PacketBuffer *buf = MakeDataBuffer(sendLen);
        if (buf == NULL)
        {
            printf("Failed to allocate PacketBuffer\n");
            return;
        }
        sendLen = buf->DataLength();

        err = TCPEP->Send(buf);
        if (err != INET_ERROR_CONNECTION_ABORTED)
            FAIL_ERROR(err, "TCPEndPoint::Send failed");

        TotalSendLength += sendLen;

        if (MaxSendLength != -1 && TotalSendLength == MaxSendLength)
        {
            printf("Closing connection\n");
            err = TCPEP->Close();
            FAIL_ERROR(err, "TCPEndPoint::Close failed");

            printf("Freeing end point\n");
            TCPEP->Free();
            TCPEP = NULL;
        }
    }

    else if (!Listen)
    {
        int32_t sendLen = SendLength;
        if (MaxSendLength != -1)
        {
            int32_t remainingLen = MaxSendLength - TotalSendLength;
            if (sendLen > remainingLen)
                sendLen = remainingLen;
        }

        IsTimeToSend = false;
        SystemLayer.StartTimer(SendInterval, HandleSendTimerComplete, NULL);

        PacketBuffer *buf = MakeDataBuffer(sendLen);
        if (buf == NULL)
        {
            printf("Failed to allocate PacketBuffer\n");
            return;
        }

        if (UseRaw6)
        {
            uint8_t *p = buf->Start();
            *p = ICMP6Types[0]; // change ICMPv6 type to be consistent with the filter
            err = Raw6EP->SendTo(DestAddr, buf);
            FAIL_ERROR(err, "RawEndPoint::SendTo (IPv6) failed");
        }
#if INET_CONFIG_ENABLE_IPV4
        else if (UseRaw4)
        {
            err = Raw4EP->SendTo(DestAddr, buf);
            FAIL_ERROR(err, "RawEndPoint::SendTo (IPv4) failed");
        }
#endif // INET_CONFIG_ENABLE_IPV4
        else if (!UseTCP)
        {
            err = UDPEP->SendTo(DestAddr, 4242, buf);
            FAIL_ERROR(err, "UDPEndPoint::SendTo failed");
        }

        TotalSendLength += sendLen;
    }
}

#if INET_CONFIG_ENABLE_DNS_RESOLVER
void HandleDNSResolveComplete(void *appState, INET_ERROR err, uint8_t addrCount, IPAddress *addrArray)
{
    DNSResolutionComplete = true;

    FAIL_ERROR(err, "DNS name resolution failed");

    if (addrCount > 0)
    {
        char destAddrStr[64];
        DestAddr.ToString(destAddrStr, sizeof(destAddrStr));
        printf("DNS name resolution complete: %s\n", destAddrStr);
    }
    else
        printf("DNS name resolution return no addresses\n");

    DriveSend();
}
#endif // INET_CONFIG_ENABLE_DNS_RESOLVER

void HandleConnectionComplete(TCPEndPoint *ep, INET_ERROR conErr)
{
    INET_ERROR err;
    IPAddress peerAddr;
    uint16_t peerPort;

    if (conErr == WEAVE_NO_ERROR)
    {
        err = ep->GetPeerInfo(&peerAddr, &peerPort);
        FAIL_ERROR(err, "TCPEndPoint::GetPeerInfo failed");

        char peerAddrStr[64];
        peerAddr.ToString(peerAddrStr, sizeof(peerAddrStr));

        printf("Connection established to %s:%ld\n", peerAddrStr, (long) peerPort);

        TotalSendLength = 0;
        TotalRcvLength = 0;

        if (TCPEP->PendingReceiveLength() == 0)
            TCPEP->PutBackReceivedData(NULL);
        TCPEP->DisableReceive();
        TCPEP->EnableKeepAlive(10, 100);
        TCPEP->DisableKeepAlive();
        TCPEP->EnableReceive();

        DriveSend();
    }
    else
    {
        printf("Connection FAILED: %s\n", ErrorStr(conErr));

        ep->Free();
        ep= NULL;

        IsTimeToSend = false;
        SystemLayer.CancelTimer(HandleSendTimerComplete, NULL);
        SystemLayer.StartTimer(SendInterval, HandleSendTimerComplete, NULL);
    }
}

void HandleConnectionReceived(TCPEndPoint *listeningEP, TCPEndPoint *conEP, const IPAddress &peerAddr, uint16_t peerPort)
{
    char peerAddrStr[64];
    peerAddr.ToString(peerAddrStr, sizeof(peerAddrStr));

    printf("Accepted connection from %s, port %ld\n", peerAddrStr, (long) peerPort);

    conEP->OnConnectComplete = HandleConnectionComplete;
    conEP->OnConnectionClosed = HandleConnectionClosed;
    conEP->OnDataSent = HandleDataSent;
    conEP->OnDataReceived = HandleDataReceived;

    TCPEP = conEP;

    TotalSendLength = 0;
    TotalRcvLength = 0;

    DriveSend();
}

void HandleConnectionClosed(TCPEndPoint *ep, INET_ERROR err)
{
    if (err == WEAVE_NO_ERROR)
        printf("Connection closed\n");
    else
        printf("Connection closed with error: %s\n", ErrorStr(err));

    printf("Freeing end point\n");
    ep->Free();

    if (ep == TCPEP)
    {
        TCPEP = NULL;
        DriveSend();
    }
}

void HandleDataSent(TCPEndPoint *ep, uint16_t len)
{
    printf("Data sent: %ld\n", (long) len);

    if (ep == TCPEP)
        DriveSend();
}

void HandleDataReceived(TCPEndPoint *ep, PacketBuffer *data)
{
    INET_ERROR err;

    if (data->TotalLength() < MinRcvLength && ep->State == TCPEndPoint::kState_Connected)
    {
        err = ep->PutBackReceivedData(data);
        FAIL_ERROR(err, "TCPEndPoint::PutBackReceivedData failed");
        return;
    }

    for (PacketBuffer *buf = data; buf; buf = buf->Next())
    {
        printf("Data received (%ld bytes)\n", (long) buf->DataLength());
        uint8_t *p = buf->Start();
        for (uint16_t i = 0; i < buf->DataLength(); i++, p++, TotalRcvLength++)
            if (*p != (uint8_t) TotalRcvLength)
            {
                printf("Bad data value, offset %d\n", (int) i);
                exit(-1);
            }
        printf("Total received data length: %d bytes\n", TotalRcvLength);
    }

    err = ep->AckReceive(data->TotalLength());
    FAIL_ERROR(err, "TCPEndPoint::AckReceive failed");

    PacketBuffer::Free(data);

    if (MaxRcvLength != -1 && TotalRcvLength >= MaxRcvLength)
    {
        printf("Closing connection\n");
        err = ep->Close();
        FAIL_ERROR(err, "TCPEndPoint::Close failed");

        printf("Freeing end point\n");
        ep->Free();
        if (ep == TCPEP)
            TCPEP = NULL;

        TotalRcvLength  = 0;
    }
}

void HandleAcceptError(TCPEndPoint *endPoint, INET_ERROR err)
{
    printf("Accept error: %s\n", ErrorStr(err));
}

void HandleRawMessageReceived(RawEndPoint *endPoint, PacketBuffer *msg, IPAddress senderAddr)
{
    char senderAddrStr[64];
    senderAddr.ToString(senderAddrStr, sizeof(senderAddrStr));

    printf("Raw message received from %s (%ld bytes)\n", senderAddrStr, (long) msg->DataLength());
    TotalRcvLength += msg->DataLength();
    printf("Total received data length: %d bytes\n", TotalRcvLength);

    PacketBuffer::Free(msg);
}

void HandleRawReceiveError(RawEndPoint *endPoint, INET_ERROR err, IPAddress senderAddr)
{
    char senderAddrStr[64];
    senderAddr.ToString(senderAddrStr, sizeof(senderAddrStr));

    printf("Raw receive error (%s): %s\n", senderAddrStr, ErrorStr(err));
}

void HandleUDPMessageReceived(UDPEndPoint *endPoint, PacketBuffer *msg, const IPPacketInfo *pktInfo)
{
    char senderAddrStr[64];
    pktInfo->SrcAddress.ToString(senderAddrStr, sizeof(senderAddrStr));

    for (PacketBuffer *buf = msg; buf; buf = buf->Next())
    {
        printf("UDP message received from %s, port %ld (%ld bytes)\n", senderAddrStr, (long) pktInfo->SrcPort,
                (long) msg->DataLength());

        uint8_t *p = buf->Start();
        for (uint16_t i = 0; i < buf->DataLength(); i++, p++, TotalRcvLength++)
            if (*p != (uint8_t) TotalRcvLength)
            {
                printf("Bad data value, offset %d\n", (int) i);
                exit(-1);
            }
        printf("Total received data length: %d bytes\n", TotalRcvLength);
    }

    PacketBuffer::Free(msg);
}

void HandleUDPReceiveError(UDPEndPoint *endPoint, INET_ERROR err, const IPPacketInfo *pktInfo)
{
    char senderAddrStr[64];
    uint16_t senderPort;
    if (pktInfo != NULL)
    {
        pktInfo->SrcAddress.ToString(senderAddrStr, sizeof(senderAddrStr));
        senderPort = pktInfo->SrcPort;
    }
    else
    {
        strcpy(senderAddrStr, "(unknown)");
        senderPort = 0;
    }

    printf("UDP receive error (%s, port %ld): %s\n", senderAddrStr, (long) senderPort, ErrorStr(err));
}

void HandleSendTimerComplete(System::Layer* aSystemLayer, void* aAppState, System::Error aError)
{
    FAIL_ERROR(aError, "Send timer completed with error");

    IsTimeToSend = true;

    DriveSend();
}

PacketBuffer *MakeDataBuffer(int32_t desiredLen)
{
    PacketBuffer *buf;

    buf = PacketBuffer::New();
    if (buf == NULL)
        return NULL;

    if (desiredLen > buf->MaxDataLength())
        desiredLen = buf->MaxDataLength();

    char *p = (char *) buf->Start();
    for (uint16_t i = 0; i < desiredLen; i++, p++)
        *p = (uint8_t) (TotalSendLength + i);

    buf->SetDataLength(desiredLen);

    return buf;
}
