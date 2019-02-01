/*
 *
 *    Copyright (c) 2018 Google LLC
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
 *      the InetLayer Internet Protocol stack abstraction interfaces
 *      for handling IP (v4 or v6) multicast on either bare IP (i.e.,
 *      "raw") or UDP endpoints.
 *
 */

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#include <signal.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <nlbyteorder.hpp>

#include <InetLayer/IPAddress.h>
#include <SystemLayer/SystemTimer.h>

#include "ToolCommon.h"

using namespace nl::Inet;

/* Preprocessor Macros */

#define kToolName                       "TestInetLayerMulticast"

#define kToolOptInterface               'I'
#define kToolOptNoLoopback              'L'
#define kToolOptIPv4Only                '4'
#define kToolOptIPv6Only                '6'
#define kToolOptGroup                   'g'
#define kToolOptInterval                'i'
#define kToolOptListen                  'l'
#define kToolOptRawIP                   'r'
#define kToolOptSendSize                's'
#define kToolOptUDPIP                   'u'

#define kToolOptBase                    1000
#define kToolOptExpectedGroupRxPackets  (kToolOptBase + 0)
#define kToolOptExpectedGroupTxPackets  (kToolOptBase + 1)

/* Type Definitions */

enum OptFlags
{
    kOptFlagUseIPv4    = 0x00000001,
    kOptFlagUseIPv6    = 0x00000002,

    kOptFlagUseRawIP   = 0x00000004,
    kOptFlagUseUDPIP   = 0x00000008,

    kOptFlagListen     = 0x00000010,

    kOptFlagNoLoopback = 0x00000020
};

enum kICMPTypeIndex
{
    kICMP_EchoRequestIndex = 0,
    kICMP_EchoReplyIndex   = 1
};

struct Stats
{
    uint32_t       mExpected;
    uint32_t       mActual;
};

struct TransferStats
{
    struct Stats   mReceive;
    struct Stats   mTransmit;
};

struct GroupAddress
{
    uint32_t       mGroup;
    TransferStats  mStats;
    IPAddress      mMulticastAddress;
};

template <size_t tCapacity>
struct GroupAddresses
{
    size_t         mSize;
    const size_t   mCapacity = tCapacity;
    GroupAddress   mAddresses[tCapacity];
};

template <size_t tCapacity>
struct TestState
{
    GroupAddresses<tCapacity> &  mGroupAddresses;
    bool                         mFailed;
    bool                         mSucceeded;
};

/* Function Declarations */

static void HandleSignal(int aSignal);
static bool HandleOption(const char *aProgram, OptionSet *aOptions, int aIdentifier, const char *aName, const char *aValue);
static bool HandleNonOptionArgs(const char *progName, int argc, char *argv[]);
static bool ParseGroupOpt(const char *aProgram, const char *aValue, bool aIPv6, uint32_t &aOutLastGroupIndex);
static bool ParseAndUpdateExpectedGroupPackets(const char *aProgram, const char *aValue, uint32_t aGroup, const char *aDescription, uint32_t &aOutExpected);

static void StartTest(void);
static void CleanupTest(void);
static void DriveSend(void);

static void HandleSendTimerComplete(System::Layer *aSystemLayer, void *aContext, System::Error aError);

/* Global Variables */

static const uint16_t    kUDPPort              = 4242;

static const uint8_t     kICMPv4_EchoRequest   = 8;
static const uint8_t     kICMPv4_EchoReply     = 0;

static const size_t      kICMPv6_FilterTypes   = 2;

static const uint8_t     kICMPv6_EchoRequest   = 128;
static const uint8_t     kICMPv6_EchoReply     = 129;

static const uint32_t    kOptFlagsDefault      = (kOptFlagUseIPv6 | kOptFlagUseRawIP);

static uint32_t          sOptFlags             = 0;

static const char *      sInterfaceName        = NULL;
static InterfaceId       sInterfaceId          = INET_NULL_INTERFACEID;

static RawEndPoint *     sRawIPEndPoint        = NULL;
static UDPEndPoint *     sUDPIPEndPoint        = NULL;

static GroupAddresses<4> sGroupAddresses;

static TestState<4>      sTestState            =
{
    sGroupAddresses,
    false,
    false
};

static uint32_t          sLastGroupIndex       = 0;

static const uint8_t     sICMPv4Types[2] =
{
    kICMPv4_EchoRequest,
    kICMPv4_EchoReply
};

static const uint8_t     sICMPv6Types[kICMPv6_FilterTypes] =
{
    kICMPv6_EchoRequest,
    kICMPv6_EchoReply
};

static bool              sSendIntervalExpired  = true;
static uint32_t          sSendIntervalMs       = 1000;

static uint16_t          sSendSize             = 56;

static OptionDef         sToolOptionDefs[] =
{
    { "interface",                 kArgumentRequired,  kToolOptInterface              },
    { "interval",                  kArgumentRequired,  kToolOptInterval               },
#if INET_CONFIG_ENABLE_IPV4
    { "ipv4",                      kNoArgument,        kToolOptIPv4Only               },
#endif // INET_CONFIG_ENABLE_IPV4
    { "ipv6",                      kNoArgument,        kToolOptIPv6Only               },
    { "listen",                    kNoArgument,        kToolOptListen                 },
    { "group",                     kArgumentRequired,  kToolOptGroup                  },
    { "group-expected-rx-packets", kArgumentRequired,  kToolOptExpectedGroupRxPackets },
    { "group-expected-tx-packets", kArgumentRequired,  kToolOptExpectedGroupTxPackets },
    { "no-loopback",               kNoArgument,        kToolOptNoLoopback             },
    { "raw",                       kNoArgument,        kToolOptRawIP                  },
    { "send-size",                 kArgumentRequired,  kToolOptSendSize               },
    { "udp",                       kNoArgument,        kToolOptUDPIP                  },
    { NULL }
};

static const char *      sToolOptionHelp =
    "  -I, --interface <interface>\n"
    "       The network interface to bind to and from which to send and receive all multicast traffic.\n"
    "\n"
    "  -L, --no-loopback\n"
    "       Suppress the loopback of multicast packets.\n"
    "\n"
    "  -i, --interval <interval>\n"
    "       Wait interval milliseconds between sending each packet (default: 1000 ms).\n"
    "\n"
    "  -l, --listen\n"
    "       Act as a server (i.e., listen) for multicast packets rather than send them.\n"
    "\n"
#if INET_CONFIG_ENABLE_IPV4
    "  -4, --ipv4\n"
    "       Use IPv4 only.\n"
    "\n"
#endif // INET_CONFIG_ENABLE_IPV4
    "  -6, --ipv6\n"
    "       Use IPv6 only (default).\n"
    "\n"
    "  -g, --group <group>\n"
    "       Multicast group number to join.\n"
    "\n"
    "  --group-expected-rx-packets <packets>\n"
    "       Expect to receive this number of packets for the previously-specified multicast group.\n"
    "\n"
    "  --group-expected-tx-packets <packets>\n"
    "       Expect to send this number of packets for the previously-specified multicast group.\n"
    "\n"
    "  -s, --send-size <packetsize>\n"
    "       Send packetsize bytes of data (default: 56 bytes)\n"
    "\n"
    "  -r, --raw\n"
    "       Use raw IP (default).\n"
    "\n"
    "  -u, --udp\n"
    "       Use UDP over IP.\n"
    "\n";

static OptionSet         sToolOptions =
{
    HandleOption,
    sToolOptionDefs,
    "GENERAL OPTIONS",
    sToolOptionHelp
};

static HelpOptions       sHelpOptions(
    kToolName,
    "Usage: " kToolName " [ <options> ] [ -g <group> [ ... ] -I <interface> ]\n",
    WEAVE_VERSION_STRING "\n" WEAVE_TOOL_COPYRIGHT
);

static OptionSet *       sToolOptionSets[] =
{
    &sToolOptions,
    &gNetworkOptions,
    &gFaultInjectionOptions,
    &sHelpOptions,
    NULL
};

template <size_t tCapacity>
static bool IsTesting(TestState<tCapacity> &aTestState)
{
    bool lRetval;

    lRetval = (!aTestState.mFailed && !aTestState.mSucceeded);

    return (lRetval);
}

static void CheckSucceededOrFailed(const GroupAddress &aAddress, bool &aOutSucceeded, bool &aOutFailed)
{
    const TransferStats &lStats = aAddress.mStats;

#if DEBUG
    printf("Group %u: sent %u/%u, received %u/%u\n",
           aAddress.mGroup,
           lStats.mTransmit.mActual, lStats.mTransmit.mExpected,
           lStats.mReceive.mActual, lStats.mReceive.mExpected);
#endif

    if (((lStats.mTransmit.mExpected > 0) && (lStats.mTransmit.mActual > lStats.mTransmit.mExpected)) ||
        ((lStats.mReceive.mExpected > 0) && (lStats.mReceive.mActual > lStats.mReceive.mExpected)))
    {
        aOutFailed = true;
    }
    else if (((lStats.mTransmit.mExpected > 0) && (lStats.mTransmit.mActual < lStats.mTransmit.mExpected)) ||
             ((lStats.mReceive.mExpected > 0) && (lStats.mReceive.mActual < lStats.mReceive.mExpected)))
    {
        aOutSucceeded = false;
    }
}

template <size_t tCapacity>
static void CheckSucceededOrFailed(TestState<tCapacity> &aTestState, bool &aOutSucceeded, bool &aOutFailed)
{
    for (size_t i = 0; i < aTestState.mGroupAddresses.mSize; i++)
    {
        const GroupAddress &lGroup = aTestState.mGroupAddresses.mAddresses[i];

        CheckSucceededOrFailed(lGroup, aOutSucceeded, aOutFailed);
    }

    if (aOutSucceeded || aOutFailed)
    {
        if (aOutSucceeded)
            aTestState.mSucceeded = true;

        if (aOutFailed)
            aTestState.mFailed = true;
    }
}

template <size_t tCapacity>
static bool WasSuccessful(TestState<tCapacity> &aTestState)
{
    bool lRetval = false;

    if (aTestState.mFailed)
        lRetval = false;
    else if (aTestState.mSucceeded)
        lRetval = true;

    return (lRetval);
}

static void HandleSignal(int aSignal)
{
    switch (aSignal)
    {

    case SIGUSR1:
        sTestState.mFailed = true;
        break;

    }
}

int main(int argc, char *argv[])
{
    bool          lSuccessful = true;
    INET_ERROR    lStatus;

    InitToolCommon();

    SetupFaultInjectionContext(argc, argv);

    SetSignalHandler(HandleSignal);

    if (argc == 1)
    {
        sHelpOptions.PrintBriefUsage(stderr);
        lSuccessful = false;
        goto done;
    }

    if (!ParseArgsFromEnvVar(kToolName, TOOL_OPTIONS_ENV_VAR_NAME, sToolOptionSets, NULL, true) ||
        !ParseArgs(kToolName, argc, argv, sToolOptionSets, HandleNonOptionArgs))
    {
        lSuccessful = false;
        goto done;
    }

    InitSystemLayer();

    InitNetwork();

    // At this point, we should have valid network interfaces,
    // including LwIP TUN/TAP shim interfaces. Validate the
    // -I/--interface argument, if present.

    if (sInterfaceName != NULL)
    {
        lStatus = InterfaceNameToId(sInterfaceName, sInterfaceId);
        if (lStatus != INET_NO_ERROR)
        {
            PrintArgError("%s: unknown network interface %s\n", kToolName, sInterfaceName);
            lSuccessful = false;
            goto shutdown;
        }
    }

    // If any multicast groups have been specified, ensure that a
    // network interface identifier has been specified and is valid.

    if ((sGroupAddresses.mSize > 0) && !IsInterfaceIdPresent(sInterfaceId))
    {
        PrintArgError("%s: a network interface is required when specifying one or more multicast groups\n", kToolName);
        lSuccessful = false;
        goto shutdown;
    }

    StartTest();

    while (IsTesting(sTestState))
    {
        struct timeval sleepTime;
        bool lSucceeded = true;
        bool lFailed = false;

        sleepTime.tv_sec = 0;
        sleepTime.tv_usec = 10000;

        ServiceNetwork(sleepTime);

        CheckSucceededOrFailed(sTestState, lSucceeded, lFailed);

#if DEBUG
        printf("%s %s number of expected packets\n",
               ((lSucceeded) ? "successfully" :
                ((lFailed) ? "failed to" :
                 "has not yet")),
               ((lSucceeded) ? ((sOptFlags & kOptFlagListen) ? "received" : "sent") :
                ((lFailed) ? ((sOptFlags & kOptFlagListen) ? "receive" : "send") :
                 (sOptFlags & kOptFlagListen) ? "received" : "sent"))
               );
#endif
    }

    sSendIntervalExpired = false;

    SystemLayer.CancelTimer(HandleSendTimerComplete, NULL);

    CleanupTest();

shutdown:
    ShutdownNetwork();
    ShutdownSystemLayer();

    lSuccessful = WasSuccessful(sTestState);

done:
    return (lSuccessful ? EXIT_SUCCESS : EXIT_FAILURE);
}

static bool HandleOption(const char *aProgram, OptionSet *aOptions, int aIdentifier, const char *aName, const char *aValue)
{
    bool       retval = true;

    switch (aIdentifier)
    {

    case kToolOptInterval:
        if (!ParseInt(aValue, sSendIntervalMs) || sSendIntervalMs > UINT32_MAX)
        {
            PrintArgError("%s: invalid value specified for send interval: %s\n", aProgram, aValue);
            retval = false;
            goto done;
        }
        break;

    case kToolOptListen:
        sOptFlags |= kOptFlagListen;
        break;

    case kToolOptNoLoopback:
        sOptFlags |= kOptFlagNoLoopback;
        break;

#if INET_CONFIG_ENABLE_IPV4
    case kToolOptIPv4Only:
        if (sOptFlags & kOptFlagUseIPv6)
        {
            PrintArgError("%s: the use of --ipv4 is exclusive with --ipv6. Please select only one of the two options.\n", aProgram);
            retval = false;
            goto done;
        }
        sOptFlags |= kOptFlagUseIPv4;
        break;
#endif // INET_CONFIG_ENABLE_IPV4

    case kToolOptIPv6Only:
        if (sOptFlags & kOptFlagUseIPv4)
        {
            PrintArgError("%s: the use of --ipv6 is exclusive with --ipv4. Please select only one of the two options.\n", aProgram);
            retval = false;
            goto done;
        }
        sOptFlags |= kOptFlagUseIPv6;
        break;

    case kToolOptInterface:

        // NOTE: When using LwIP on a hosted OS, the interface will
        // not actually be available until AFTER InitNetwork,
        // consequently, we cannot do any meaningful validation
        // here. Simply save the value off and we will validate it
        // later.

        sInterfaceName = aValue;
        break;

    case kToolOptGroup:
        if (!ParseGroupOpt(aProgram, aValue, sOptFlags & kOptFlagUseIPv6, sLastGroupIndex))
        {
            retval = false;
            goto done;
        }
        break;

    case kToolOptExpectedGroupRxPackets:
        {
            GroupAddress &lGroupAddress = sGroupAddresses.mAddresses[sLastGroupIndex];

            if (!ParseAndUpdateExpectedGroupPackets(aProgram, aValue, lGroupAddress.mGroup, "received", lGroupAddress.mStats.mReceive.mExpected))
            {
                retval = false;
                goto done;
            }
        }
        break;

    case kToolOptExpectedGroupTxPackets:
        {
            GroupAddress &lGroupAddress = sGroupAddresses.mAddresses[sLastGroupIndex];

            if (!ParseAndUpdateExpectedGroupPackets(aProgram, aValue, lGroupAddress.mGroup, "sent", lGroupAddress.mStats.mTransmit.mExpected))
            {
                retval = false;
                goto done;
            }
        }
        break;

    case kToolOptRawIP:
        if (sOptFlags & kOptFlagUseUDPIP)
        {
            PrintArgError("%s: the use of --raw is exclusive with --udp. Please select only one of the two options.\n", aProgram);
            retval = false;
            goto done;
        }
        sOptFlags |= kOptFlagUseRawIP;
        break;

    case kToolOptSendSize:
        if (!ParseInt(aValue, sSendSize))
        {
            PrintArgError("%s: invalid value specified for send size: %s\n", aProgram, aValue);
            return false;
        }
        break;

    case kToolOptUDPIP:
        if (sOptFlags & kOptFlagUseRawIP)
        {
            PrintArgError("%s: the use of --udp is exclusive with --raw. Please select only one of the two options.\n", aProgram);
            retval = false;
            goto done;
        }
        sOptFlags |= kOptFlagUseUDPIP;
        break;

    default:
        PrintArgError("%s: INTERNAL ERROR: Unhandled option: %s\n", aProgram, aName);
        retval = false;
        break;

    }

 done:
    return (retval);
}

bool HandleNonOptionArgs(const char *aProgram, int argc, char *argv[])
{
    bool retval = true;

    if ((sOptFlags & (kOptFlagListen | kOptFlagNoLoopback)) == (kOptFlagListen | kOptFlagNoLoopback))
    {
        PrintArgError("%s: the listen option is exclusive with the loopback suppression option. Please select one or the other.\n", aProgram);
        retval = false;
        goto done;
    }

    // If there were any additional, non-parsed arguments, it's an error.

    if (argc > 0)
    {
        PrintArgError("%s: unexpected argument: %s\n", aProgram, argv[0]);
        retval = false;
        goto done;
    }

    // If no IP version or transport flags were specified, use the defaults.

    if (!(sOptFlags & (kOptFlagUseIPv4 | kOptFlagUseIPv6 | kOptFlagUseRawIP | kOptFlagUseUDPIP)))
    {
        sOptFlags |= kOptFlagsDefault;
    }

 done:
    return (retval);
}

// Create an IPv4 administratively-scoped multicast address

static IPAddress MakeIPv4Multicast(uint32_t aGroupIdentifier)
{
    IPAddress lAddress;

    lAddress.Addr[0] = 0;
    lAddress.Addr[1] = 0;
    lAddress.Addr[2] = nlByteOrderSwap32HostToBig(0xFFFF);
    lAddress.Addr[3] = nlByteOrderSwap32HostToBig((239 << 24) | (aGroupIdentifier & 0xFFFFFF));

    return (lAddress);
}

// Create an IPv6 site-scoped multicast address

static IPAddress MakeIPv6Multicast(uint32_t aGroupIdentifier)
{
    return (IPAddress::MakeIPv6Multicast(kIPv6MulticastScope_Site, aGroupIdentifier));
}

static void SetGroup(GroupAddress &aGroupAddress, uint32_t aGroupIdentifier, uint32_t aExpectedRx, uint32_t aExpectedTx)
{
    aGroupAddress.mGroup                     = aGroupIdentifier;
    aGroupAddress.mStats.mReceive.mExpected  = aExpectedRx;
    aGroupAddress.mStats.mReceive.mActual    = 0;
    aGroupAddress.mStats.mTransmit.mExpected = aExpectedTx;
    aGroupAddress.mStats.mTransmit.mActual   = 0;
}

static bool ParseGroupOpt(const char *aProgram, const char *aValue, bool aIPv6, uint32_t &aOutLastGroupIndex)
{
    uint32_t lGroupIdentifier;
    bool     lRetval = true;

    if (sGroupAddresses.mSize == sGroupAddresses.mCapacity)
    {
        PrintArgError("%s: the maximum number of allowed groups (%zu) have been specified\n", aProgram, sGroupAddresses.mCapacity);
        lRetval = false;
        goto done;
    }

    if (!ParseInt(aValue, lGroupIdentifier))
    {
        PrintArgError("%s: unrecognized group %s\n", aProgram, aValue);
        lRetval = false;
        goto done;
    }

    aOutLastGroupIndex = sGroupAddresses.mSize++;

    SetGroup(sGroupAddresses.mAddresses[aOutLastGroupIndex],
             lGroupIdentifier,
             lGroupIdentifier,
             lGroupIdentifier);

 done:
    return (lRetval);
}

static bool ParseAndUpdateExpectedGroupPackets(const char *aProgram, const char *aValue, uint32_t aGroup, const char *aDescription, uint32_t &aOutExpected)
{
    uint32_t  lExpectedGroupPackets;
    bool      lRetval = true;

    if (!ParseInt(aValue, lExpectedGroupPackets))
    {
        PrintArgError("%s: invalid value specified for expected group %u %s packets: %s\n", aProgram, aGroup, aDescription, aValue);
        lRetval = false;
        goto done;
    }

    aOutExpected = lExpectedGroupPackets;

 done:
    return (lRetval);
}

static PacketBuffer *MakeDataBuffer(uint16_t aSize)
{
    PacketBuffer *  lBuffer;
    uint8_t *       p;

    lBuffer = PacketBuffer::New();
    if (lBuffer == NULL)
        goto done;

    if (aSize > lBuffer->MaxDataLength())
        aSize = lBuffer->MaxDataLength();

    p = lBuffer->Start();

    for (uint16_t i = 0; i < aSize; i++)
    {
        p[i] = (uint8_t)(i & 0xFF);
    }

    lBuffer->SetDataLength(aSize);

 done:
    return (lBuffer);
}

static void DriveSendForGroup(GroupAddress &aGroupAddress)
{
    PacketBuffer *lBuffer;
    INET_ERROR    lStatus = INET_NO_ERROR;

    if (aGroupAddress.mStats.mTransmit.mActual < aGroupAddress.mStats.mTransmit.mExpected)
    {
        lBuffer = MakeDataBuffer(sSendSize);
        if (lBuffer == NULL)
        {
            printf("Failed to allocate PacketBuffer\n");
            goto done;
        }

        if ((sOptFlags & (kOptFlagUseRawIP | kOptFlagUseIPv6)) == (kOptFlagUseRawIP | kOptFlagUseIPv6))
        {
            uint8_t *p = lBuffer->Start();

            *p = sICMPv6Types[kICMP_EchoRequestIndex];

            lStatus = sRawIPEndPoint->SendTo(aGroupAddress.mMulticastAddress, lBuffer);
            FAIL_ERROR(lStatus, "RawEndPoint::SendTo (IPv6) failed");
        }
#if INET_CONFIG_ENABLE_IPV4
        else if ((sOptFlags & (kOptFlagUseRawIP | kOptFlagUseIPv4)) == (kOptFlagUseRawIP | kOptFlagUseIPv4))
        {
            uint8_t *p = lBuffer->Start();

            *p = sICMPv4Types[kICMP_EchoRequestIndex];

            lStatus = sRawIPEndPoint->SendTo(aGroupAddress.mMulticastAddress, lBuffer);
            FAIL_ERROR(lStatus, "RawEndPoint::SendTo (IPv4) failed");
        }
#endif // INET_CONFIG_ENABLE_IPV4
        else if ((sOptFlags & kOptFlagUseUDPIP) == kOptFlagUseUDPIP)
        {
            lStatus = sUDPIPEndPoint->SendTo(aGroupAddress.mMulticastAddress, kUDPPort, lBuffer);
            FAIL_ERROR(lStatus, "UDPEndPoint::SendTo failed");
        }

        if (lStatus == INET_NO_ERROR)
        {
            aGroupAddress.mStats.mTransmit.mActual++;

            printf("%u/%u transmitted for multicast group %u\n",
                   aGroupAddress.mStats.mTransmit.mActual,
                   aGroupAddress.mStats.mTransmit.mExpected,
                   aGroupAddress.mGroup);
        }
    }

 done:
    return;
}

static void DriveSend(void)
{
    if (!sSendIntervalExpired)
        goto done;

    sSendIntervalExpired = false;
    SystemLayer.StartTimer(sSendIntervalMs, HandleSendTimerComplete, NULL);

    // Iterate over each multicast group for which this node is a
    // member and send a packet.

    for (size_t i = 0; i < sGroupAddresses.mSize; i++)
    {
        GroupAddress &lGroupAddress = sGroupAddresses.mAddresses[i];

        DriveSendForGroup(lGroupAddress);
    }

 done:
    return;
}

static GroupAddress *FindGroupAddress(const IPAddress &aSourceAddress)
{
    GroupAddress *lResult = NULL;

    for (size_t i = 0; i < sGroupAddresses.mSize; i++)
    {
        GroupAddress &lGroupAddress = sGroupAddresses.mAddresses[i];

        if (lGroupAddress.mMulticastAddress == aSourceAddress)
        {
            lResult = &lGroupAddress;
            break;
        }
    }

    return (lResult);
}

static void HandleSendTimerComplete(System::Layer *aSystemLayer, void *aAppState, System::Error aError)
{
    FAIL_ERROR(aError, "Send timer completed with error");

    sSendIntervalExpired = true;

    DriveSend();
}

static void HandleRawMessageReceived(IPEndPointBasis *aEndPoint, PacketBuffer *aBuffer, const IPPacketInfo *aPacketInfo)
{
    char            lSourceAddressBuffer[INET6_ADDRSTRLEN];
    char            lDestinationAddressBuffer[INET6_ADDRSTRLEN];
    GroupAddress *  lGroupAddress;

    aPacketInfo->SrcAddress.ToString(lSourceAddressBuffer, sizeof (lSourceAddressBuffer));
    aPacketInfo->DestAddress.ToString(lDestinationAddressBuffer, sizeof (lDestinationAddressBuffer));

    printf("IP packet received from %s to %s (%zu bytes)\n",
           lSourceAddressBuffer,
           lDestinationAddressBuffer,
           (size_t)aBuffer->DataLength());

    lGroupAddress = FindGroupAddress(aPacketInfo->DestAddress);

    if (lGroupAddress != NULL)
    {
        lGroupAddress->mStats.mReceive.mActual++;

        printf("%u/%u received for multicast group %u\n",
               lGroupAddress->mStats.mReceive.mActual,
               lGroupAddress->mStats.mReceive.mExpected,
               lGroupAddress->mGroup);
    }

    PacketBuffer::Free(aBuffer);
}

static void HandleRawReceiveError(IPEndPointBasis *aEndPoint, INET_ERROR aError, const IPPacketInfo *aPacketInfo)
{
    char     lAddressBuffer[INET6_ADDRSTRLEN];

    if (aPacketInfo != NULL)
    {
        aPacketInfo->SrcAddress.ToString(lAddressBuffer, sizeof (lAddressBuffer));
    }
    else
    {
        strcpy(lAddressBuffer, "(unknown)");
    }

    printf("IP receive error from %s %s\n", lAddressBuffer, ErrorStr(aError));

    // sTestState.mFailed = true;
}

static void HandleUDPMessageReceived(IPEndPointBasis *aEndPoint, PacketBuffer *aBuffer, const IPPacketInfo *aPacketInfo)
{
    char            lSourceAddressBuffer[INET6_ADDRSTRLEN];
    char            lDestinationAddressBuffer[INET6_ADDRSTRLEN];
    GroupAddress *  lGroupAddress;

    aPacketInfo->SrcAddress.ToString(lSourceAddressBuffer, sizeof (lSourceAddressBuffer));
    aPacketInfo->DestAddress.ToString(lDestinationAddressBuffer, sizeof (lDestinationAddressBuffer));

    printf("UDP packet received from %s:%u to %s:%u (%zu bytes)\n",
           lSourceAddressBuffer, aPacketInfo->SrcPort,
           lDestinationAddressBuffer, aPacketInfo->DestPort,
           (size_t)aBuffer->DataLength());

    lGroupAddress = FindGroupAddress(aPacketInfo->DestAddress);

    if (lGroupAddress != NULL)
    {
        lGroupAddress->mStats.mReceive.mActual++;

        printf("%u/%u received for multicast group %u\n",
               lGroupAddress->mStats.mReceive.mActual,
               lGroupAddress->mStats.mReceive.mExpected,
               lGroupAddress->mGroup);
    }

    PacketBuffer::Free(aBuffer);
}

static void HandleUDPReceiveError(IPEndPointBasis *aEndPoint, INET_ERROR aError, const IPPacketInfo *aPacketInfo)
{
    char     lAddressBuffer[INET6_ADDRSTRLEN];
    uint16_t lSourcePort;

    if (aPacketInfo != NULL)
    {
        aPacketInfo->SrcAddress.ToString(lAddressBuffer, sizeof (lAddressBuffer));
        lSourcePort = aPacketInfo->SrcPort;
    }
    else
    {
        strcpy(lAddressBuffer, "(unknown)");
        lSourcePort = 0;
    }

    printf("UDP receive error from %s:%u: %s\n", lAddressBuffer, lSourcePort, ErrorStr(aError));

    // sTestState.mFailed = true;
}

static void StartTest(void)
{
    IPAddressType      lIPAddressType = kIPAddressType_IPv6;
    IPProtocol         lIPProtocol    = kIPProtocol_ICMPv6;
    IPVersion          lIPVersion     = kIPVersion_6;
    IPEndPointBasis *  lEndPoint      = NULL;
    const bool         lUseLoopback   = ((sOptFlags & kOptFlagNoLoopback) == 0);
    INET_ERROR         lStatus;

#if INET_CONFIG_ENABLE_IPV4
    if (sOptFlags & kOptFlagUseIPv4)
    {
        lIPAddressType = kIPAddressType_IPv4;
        lIPVersion     = kIPVersion_4;
        lIPProtocol    = kIPProtocol_ICMPv4;
    }
#endif // INET_CONFIG_ENABLE_IPV4

    printf("Using %sIP%s, if: %s (w/%c LwIP)\n",
           ((sOptFlags & kOptFlagUseRawIP) ? "" : "UDP/"),
           ((sOptFlags & kOptFlagUseIPv4) ? "v4" : "v6"),
           ((sInterfaceName) ? sInterfaceName : "<none>"),
           (WEAVE_SYSTEM_CONFIG_USE_LWIP ? '\0' : 'o'));

    // Set up the endpoints for sending.

    if (sOptFlags & kOptFlagUseRawIP)
    {
        lStatus = Inet.NewRawEndPoint(lIPVersion, lIPProtocol, &sRawIPEndPoint);
        FAIL_ERROR(lStatus, "InetLayer::NewRawEndPoint failed");

        lEndPoint = sRawIPEndPoint;

        sRawIPEndPoint->OnMessageReceived = HandleRawMessageReceived;
        sRawIPEndPoint->OnReceiveError    = HandleRawReceiveError;

        lStatus = sRawIPEndPoint->Bind(lIPAddressType, IPAddress::Any);
        FAIL_ERROR(lStatus, "RawEndPoint::Bind failed");

        if (sOptFlags & kOptFlagUseIPv6)
        {
            lStatus = sRawIPEndPoint->SetICMPFilter(kICMPv6_FilterTypes, sICMPv6Types);
            FAIL_ERROR(lStatus, "RawEndPoint::SetICMPFilter (IPv6) failed");
        }

        if (IsInterfaceIdPresent(sInterfaceId))
        {
            lStatus = sRawIPEndPoint->BindInterface(lIPAddressType, sInterfaceId);
            FAIL_ERROR(lStatus, "RawEndPoint::BindInterface failed");
        }

        lStatus = sRawIPEndPoint->Listen();
        FAIL_ERROR(lStatus, "RawEndPoint::Listen failed");
    }
    else if (sOptFlags & kOptFlagUseUDPIP)
    {
        lStatus = Inet.NewUDPEndPoint(&sUDPIPEndPoint);
        FAIL_ERROR(lStatus, "InetLayer::NewUDPEndPoint failed");

        lEndPoint = sUDPIPEndPoint;

        sUDPIPEndPoint->OnMessageReceived = HandleUDPMessageReceived;
        sUDPIPEndPoint->OnReceiveError    = HandleUDPReceiveError;

        lStatus = sUDPIPEndPoint->Bind(lIPAddressType, IPAddress::Any, kUDPPort);
        FAIL_ERROR(lStatus, "UDPEndPoint::Bind failed");

        if (IsInterfaceIdPresent(sInterfaceId))
        {
            lStatus = sUDPIPEndPoint->BindInterface(lIPAddressType, sInterfaceId);
            FAIL_ERROR(lStatus, "UDPEndPoint::BindInterface failed");
        }

        lStatus = sUDPIPEndPoint->Listen();
        FAIL_ERROR(lStatus, "UDPEndPoint::Listen failed");
    }

    // If loopback suppression has been requested, attempt to disable
    // it; otherwise, attempt to enable it.

    lStatus = lEndPoint->SetMulticastLoopback(lIPVersion, lUseLoopback);
    FAIL_ERROR(lStatus, "SetMulticastLoopback failed");

    // Configure and join the multicast groups

    for (size_t i = 0; i < sGroupAddresses.mSize; i++)
    {
        char            lAddressBuffer[INET6_ADDRSTRLEN];
        GroupAddress &  lGroupAddress     = sGroupAddresses.mAddresses[i];
        IPAddress &     lMulticastAddress = lGroupAddress.mMulticastAddress;

        if ((lEndPoint != NULL) && IsInterfaceIdPresent(sInterfaceId))
        {
            if (sOptFlags & kOptFlagUseIPv4)
            {
                lMulticastAddress = MakeIPv4Multicast(lGroupAddress.mGroup);
            }
            else
            {
                lMulticastAddress = MakeIPv6Multicast(lGroupAddress.mGroup);
            }

            lMulticastAddress.ToString(lAddressBuffer, sizeof (lAddressBuffer));

            printf("Will join multicast group %s\n", lAddressBuffer);

            lStatus = lEndPoint->JoinMulticastGroup(sInterfaceId, lMulticastAddress);
            FAIL_ERROR(lStatus, "Could not join multicast group");
        }
    }

    if ((sOptFlags & kOptFlagListen))
        printf("Listening...\n");
    else
        DriveSend();
}

static void CleanupTest(void)
{
    IPEndPointBasis *  lEndPoint      = NULL;
    INET_ERROR         lStatus;

    if (sOptFlags & kOptFlagUseRawIP)
    {
        lEndPoint = sRawIPEndPoint;
    }
    else if (sOptFlags & kOptFlagUseUDPIP)
    {
        lEndPoint = sUDPIPEndPoint;
    }

    //  Leave the multicast groups

    for (size_t i = 0; i < sGroupAddresses.mSize; i++)
    {
        char            lAddressBuffer[INET6_ADDRSTRLEN];
        GroupAddress &  lGroupAddress     = sGroupAddresses.mAddresses[i];
        IPAddress &     lMulticastAddress = lGroupAddress.mMulticastAddress;

        if ((lEndPoint != NULL) && IsInterfaceIdPresent(sInterfaceId))
        {
            lMulticastAddress.ToString(lAddressBuffer, sizeof (lAddressBuffer));

            printf("Will leave multicast group %s\n", lAddressBuffer);

            lStatus = lEndPoint->LeaveMulticastGroup(sInterfaceId, lMulticastAddress);
            FAIL_ERROR(lStatus, "Could not leave multicast group");
        }
    }

    // Release the resources associated with the allocated end points.

    if (sRawIPEndPoint != NULL)
    {
        sRawIPEndPoint->Free();
    }

    if (sUDPIPEndPoint != NULL)
    {
        sUDPIPEndPoint->Free();
    }
}
