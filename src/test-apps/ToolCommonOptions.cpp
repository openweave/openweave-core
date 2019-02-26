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
 *      Common command-line option handling code for test applications.
 *
 */


#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif
#define __STDC_FORMAT_MACROS

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

#include "ToolCommonOptions.h"
#include "TestPersistedStorageImplementation.h"
#include <Weave/WeaveVersion.h>
#include <Weave/Profiles/service-directory/ServiceDirectory.h>
#include <SystemLayer/SystemFaultInjection.h>
#include <Weave/Support/WeaveFaultInjection.h>
#include <InetLayer/InetFaultInjection.h>

using namespace nl::Inet;
using namespace nl::Weave;

// Global Variables

NetworkOptions gNetworkOptions;
WeaveNodeOptions gWeaveNodeOptions;
WeaveSecurityMode gWeaveSecurityMode;
WRMPOptions gWRMPOptions;
GroupKeyEncOptions gGroupKeyEncOptions;
GeneralSecurityOptions gGeneralSecurityOptions;
ServiceDirClientOptions gServiceDirClientOptions;
FaultInjectionOptions gFaultInjectionOptions;

NetworkOptions::NetworkOptions()
{
    static OptionDef optionDefs[] =
    {
        { "local-addr",   kArgumentRequired, 'a' },
        { "node-addr",    kArgumentRequired, kToolCommonOpt_NodeAddr }, /* alias for local-addr */
#if WEAVE_SYSTEM_CONFIG_USE_LWIP
        { "tap-device",   kArgumentRequired, kToolCommonOpt_TapDevice },
        { "ipv4-gateway", kArgumentRequired, kToolCommonOpt_IPv4GatewayAddr },
        { "dns-server",   kArgumentRequired, 'X' },
        { "debug-lwip",   kNoArgument,       kToolCommonOpt_DebugLwIP },
        { "event-delay",  kArgumentRequired, kToolCommonOpt_EventDelay },
#endif
        { NULL }
    };
    OptionDefs = optionDefs;

    HelpGroupName = "NETWORK OPTIONS";

    OptionHelp =
        "  -a, --local-addr, --node-addr <ip-addr>\n"
        "       Local address for the node.\n"
        "\n"
#if WEAVE_SYSTEM_CONFIG_USE_LWIP
        "  --tap-device <tap-dev-name>\n"
        "       TAP device name for LwIP hosted OS usage. Defaults to weave-dev-<node-id>.\n"
        "\n"
        "  --ipv4-gateway <ip-addr>\n"
        "       Address of default IPv4 gateway.\n"
        "\n"
        "  -X, --dns-server <ip-addr>\n"
        "       IPv4 address of local DNS server.\n"
        "\n"
        "  --debug-lwip\n"
        "       Enable LwIP debug messages.\n"
        "\n"
        "  --event-delay <int>\n"
        "       Delay event processing by specified number of iterations. Defaults to 0.\n"
        "\n"
#endif
        ;

    // Defaults.
    LocalIPv4Addr = nl::Inet::IPAddress::Any;
    LocalIPv6Addr = nl::Inet::IPAddress::Any;

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    TapDeviceName = NULL;
    LwIPDebugFlags = 0;
    EventDelay = 0;
    IPv4GatewayAddr = nl::Inet::IPAddress::Any;
    DNSServerAddr = nl::Inet::IPAddress::Any;
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP
}

bool NetworkOptions::HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg)
{
    nl::Inet::IPAddress localAddr;

    switch (id)
    {
    case 'a':
    case kToolCommonOpt_NodeAddr:
        if (!ParseIPAddress(arg, localAddr))
        {
            PrintArgError("%s: Invalid value specified for local IP address: %s\n", progName, arg);
            return false;
        }
#if INET_CONFIG_ENABLE_IPV4
        if (localAddr.IsIPv4())
        {
            LocalIPv4Addr = localAddr;
        }
        else
        {
            LocalIPv6Addr = localAddr;
        }
#else // INET_CONFIG_ENABLE_IPV4
        LocalIPv6Addr = localAddr;
#endif // INET_CONFIG_ENABLE_IPV4
        break;

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    case 'X':
        if (!ParseIPAddress(arg, DNSServerAddr))
        {
            PrintArgError("%s: Invalid value specified for DNS server address: %s\n", progName, arg);
            return false;
        }
        break;
    case kToolCommonOpt_TapDevice:
        TapDeviceName = arg;
        break;

    case kToolCommonOpt_IPv4GatewayAddr:
        if (!ParseIPAddress(arg, IPv4GatewayAddr) || !IPv4GatewayAddr.IsIPv4())
        {
            PrintArgError("%s: Invalid value specified for IPv4 gateway address: %s\n", progName, arg);
            return false;
        }
        break;

    case kToolCommonOpt_DebugLwIP:
#if defined(LWIP_DEBUG)
        gLwIP_DebugFlags = (LWIP_DBG_ON|LWIP_DBG_TRACE|LWIP_DBG_STATE|LWIP_DBG_FRESH|LWIP_DBG_HALT);
#endif
        break;
    case kToolCommonOpt_EventDelay:
        if (!ParseInt(arg, EventDelay))
        {
            PrintArgError("%s: Invalid value specified for event delay: %s\n", progName, arg);
            return false;
        }
        break;
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

    default:
        PrintArgError("%s: INTERNAL ERROR: Unhandled option: %s\n", progName, name);
        return false;
    }

    return true;
}

WeaveNodeOptions::WeaveNodeOptions()
{
    static OptionDef optionDefs[] =
    {
        { "fabric-id",            kArgumentRequired, 'f'                               },
        { "node-id",              kArgumentRequired, 'n'                               },
        { "subnet",               kArgumentRequired, 'N'                               },
        { "pairing-code",         kArgumentRequired, kToolCommonOpt_PairingCode        },
        { "persistent-cntr-file", kArgumentRequired, kToolCommonOpt_PersistentCntrFile },
        { NULL }
    };
    OptionDefs = optionDefs;

    HelpGroupName = "WEAVE NODE OPTIONS";

    OptionHelp =
        "  -f, --fabric-id <num>\n"
        "       Weave fabric id. Defaults to 1 unless --node-addr specified.\n"
        "\n"
        "  -n, --node-id <num>\n"
        "       Node id for local node. Defaults to 1 unless --node-addr specified.\n"
        "\n"
        "  -N, --subnet <num>\n"
        "       Subnet number for local node. Defaults to 1 unless --node-addr specified.\n"
        "\n"
        "  --pairing-code <string>\n"
        "       Pairing code string to use for PASE authentication.  Defaults to 'TEST'.\n"
        "\n"
        "  --persistent-cntr-file <counter-file>\n"
        "       File used to persist group message counter and event counters. Counters are stored in the following format:\n"
        "           CounterOneKey      (e.g. EncMsgCntr)\n"
        "           CounterOneValue    (e.g. 0x00000078)\n"
        "           CounterTwoKey      (e.g. ProductionEIDC)\n"
        "           CounterTwoValue    (e.g. 0x34FA78E4)\n"
        "       The intention was to store these data in a human interpreted format so\n"
        "       developers can manually modify this file. When this file is modified manually\n"
        "       developers should stick to this format - any other format will result in error.\n"
        "       If persistent-cntr-file option is not specified then by default counters are not persisted.\n"
        "\n"
        ;

    // Defaults.
    FabricId = nl::Weave::kFabricIdDefaultForTest;
    LocalNodeId = 1;
    SubnetId = 1;
    FabricIdSet = false;
    LocalNodeIdSet = false;
    SubnetIdSet = false;
    PairingCode = "TEST";
}

WeaveNodeOptions::~WeaveNodeOptions()
{
    if (sPersistentStoreFile != NULL)
        fclose(sPersistentStoreFile);
}

bool WeaveNodeOptions::HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg)
{
    switch (id)
    {
    case 'f':
        if (!ParseFabricId(arg, FabricId))
        {
            PrintArgError("%s: Invalid value specified for fabric id: %s\n", progName, arg);
            return false;
        }
        break;
    case 'n':
        if (!ParseNodeId(arg, LocalNodeId))
        {
            PrintArgError("%s: Invalid value specified for local node id: %s\n", progName, arg);
            return false;
        }
        LocalNodeIdSet = true;
        break;
    case 'N':
        if (!ParseSubnetId(arg, SubnetId))
        {
            PrintArgError("%s: Invalid value specified for local subnet: %s\n", progName, arg);
            return false;
        }
        break;
    case kToolCommonOpt_PairingCode:
        PairingCode = arg;
        break;
    case kToolCommonOpt_PersistentCntrFile:
        // If file already exists then open it for write/update.
        if ((sPersistentStoreFile = fopen(arg, "r")) == NULL)
        {
            sPersistentStoreFile = fopen(arg, "w+");
        }
        // If file doesn't exist then open it for read/update.
        else
        {
            fclose(sPersistentStoreFile);
            sPersistentStoreFile = fopen(arg, "r+");
        }

        if (sPersistentStoreFile == NULL)
        {
            printf("Unable to open %s\n%s\n", arg, strerror(errno));
            return false;
        }
        break;
    default:
        PrintArgError("%s: INTERNAL ERROR: Unhandled option: %s\n", progName, name);
        return false;
    }

    return true;
}

WeaveSecurityMode::WeaveSecurityMode()
{
    static OptionDef optionDefs[] =
    {
        { "no-security",   kNoArgument,       kToolCommonOpt_SecurityNone       },
        { "case",          kNoArgument,       kToolCommonOpt_SecurityCASE       },
        { "case-shared",   kNoArgument,       kToolCommonOpt_SecurityCASEShared },
        { "pase",          kNoArgument,       kToolCommonOpt_SecurityPASE       },
        { "group-enc",     kNoArgument,       kToolCommonOpt_SecurityGroupEnc   },
        { "take",          kNoArgument,       kToolCommonOpt_SecurityTAKE       },
        { NULL }
    };

    OptionDefs = optionDefs;

    HelpGroupName = "WEAVE SECURITY OPTIONS";

    OptionHelp =
            "  --no-security\n"
            "       Use no security session\n"
            "\n"
            "  --pase\n"
            "       Use PASE to create an authenticated session and encrypt messages using\n"
            "       the negotiated session key.\n"
            "\n"
            "  --case\n"
            "       Use CASE to create an authenticated session and encrypt messages using\n"
            "       the negotiated session key.\n"
            "\n"
            "  --case-shared\n"
            "       Use CASE to create an authenticated shared session to the Nest Core router\n"
            "       and encrypt messages using the negotiated session key.\n"
            "\n"
            "  --take\n"
            "       Use TAKE to create an authenticated session and encrypt messages using\n"
            "       the negotiated session key.\n"
            "\n"
#if WEAVE_CONFIG_USE_APP_GROUP_KEYS_FOR_MSG_ENC
            "  --group-enc\n"
            "       Use a group key to encrypt messages.\n"
            "       When group key encryption option is chosen the key id should be also specified.\n"
            "       Below are two examples how group key id can be specified:\n"
            "          --group-enc-key-id 0x00005536\n"
            "          --group-enc-key-type r --group-enc-root-key c --group-enc-epoch-key-num 2 --group-enc-app-key-num 54\n"
            "       Note that both examples describe the same rotating group key derived from client\n"
            "       root key, epoch key number 4 and app group master key number 54 (0x36).\n"
            "\n"
#endif // WEAVE_CONFIG_USE_APP_GROUP_KEYS_FOR_MSG_ENC
            ;

    // Defaults.
    SecurityMode = kNone;

}

bool WeaveSecurityMode::HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg)
{
    switch (id)
    {
        case kToolCommonOpt_SecurityNone:
            SecurityMode = kNone;
            break;
        case kToolCommonOpt_SecurityCASE:
            SecurityMode = kCASE;
            break;
        case kToolCommonOpt_SecurityCASEShared:
            SecurityMode = kCASEShared;
            break;
        case kToolCommonOpt_SecurityPASE:
            SecurityMode = kPASE;
            break;
        case kToolCommonOpt_SecurityGroupEnc:
            SecurityMode = kGroupEnc;
            break;
        case kToolCommonOpt_SecurityTAKE:
            SecurityMode = kTAKE;
            break;
        default:
            PrintArgError("%s: INTERNAL ERROR: Unhandled option: %s\n", progName, name);
            return false;
    }

    return true;
}


WRMPOptions::WRMPOptions()
{
    static OptionDef optionDefs[] =
    {
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
        { "wrmp-ack-delay",        kArgumentRequired, kToolCommonOpt_WRMPACKDelay        },
        { "wrmp-retrans-interval", kArgumentRequired, kToolCommonOpt_WRMPRetransInterval },
        { "wrmp-retrans-count",    kArgumentRequired, kToolCommonOpt_WRMPRetransCount    },
#endif // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
        { NULL }
    };
    OptionDefs = optionDefs;

    HelpGroupName = "WEAVE RELIABLE MESSAGING OPTIONS";

    OptionHelp =
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
        "  --wrmp-ack-delay <ms>\n"
        "       Set the WRMP maximum pending ACK delay (defaults to 200ms).\n"
        "\n"
        "  --wrmp-retrans-interval <ms>\n"
        "       Set the WRMP retransmission interval (defaults to 200ms).\n"
        "\n"
        "  --wrmp-retrans-count <int>\n"
        "       Set the WRMP retransmission count (defaults to 3).\n"
        "\n"
#endif // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
        "";

    // Defaults.
    ACKDelay = WEAVE_CONFIG_WRMP_DEFAULT_ACK_TIMEOUT;
    RetransInterval = WEAVE_CONFIG_WRMP_DEFAULT_ACTIVE_RETRANS_TIMEOUT;
    RetransCount = WEAVE_CONFIG_WRMP_DEFAULT_MAX_RETRANS;
}

nl::Weave::WRMPConfig WRMPOptions::GetWRMPConfig() const
{
    nl::Weave::WRMPConfig wrmpConfig;
    memset(&wrmpConfig, 0, sizeof(wrmpConfig));
    wrmpConfig.mInitialRetransTimeout = RetransInterval;
    wrmpConfig.mActiveRetransTimeout = RetransInterval;
    wrmpConfig.mAckPiggybackTimeout = ACKDelay;
    wrmpConfig.mMaxRetrans = RetransCount;
    return wrmpConfig;
}

bool WRMPOptions::HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg)
{
    switch (id)
    {
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    case kToolCommonOpt_WRMPACKDelay:
        if (!ParseInt(arg, ACKDelay))
        {
            PrintArgError("%s: Invalid value specified for WRMP ACK delay: %s\n", progName, arg);
            return false;
        }
        break;
    case kToolCommonOpt_WRMPRetransInterval:
        if (!ParseInt(arg, RetransInterval))
        {
            PrintArgError("%s: Invalid value specified for WRMP retransmission interval: %s\n", progName, arg);
            return false;
        }
        break;
    case kToolCommonOpt_WRMPRetransCount:
        if (!ParseInt(arg, RetransCount))
        {
            PrintArgError("%s: Invalid value specified for WRMP retransmission count: %s\n", progName, arg);
            return false;
        }
        break;
#endif // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    default:
        PrintArgError("%s: INTERNAL ERROR: Unhandled option: %s\n", progName, name);
        return false;
    }

    return true;
}

GroupKeyEncOptions::GroupKeyEncOptions()
{
    static OptionDef optionDefs[] =
    {
#if WEAVE_CONFIG_USE_APP_GROUP_KEYS_FOR_MSG_ENC
        { "group-enc-key-id",           kArgumentRequired, kToolCommonOpt_GroupEncKeyId                },
        { "group-enc-key-type",         kArgumentRequired, kToolCommonOpt_GroupEncKeyType              },
        { "group-enc-root-key",         kArgumentRequired, kToolCommonOpt_GroupEncRootKey              },
        { "group-enc-epoch-key-num",    kArgumentRequired, kToolCommonOpt_GroupEncEpochKeyNum          },
        { "group-enc-app-key-num",      kArgumentRequired, kToolCommonOpt_GroupEncAppGroupMasterKeyNum },
#endif
        { NULL }
    };
    OptionDefs = optionDefs;

    HelpGroupName = "GROUP KEY MESSAGE ENCRYPTION OPTIONS";

    OptionHelp =
#if WEAVE_CONFIG_USE_APP_GROUP_KEYS_FOR_MSG_ENC
        "  --group-enc-key-id <int>\n"
        "       Key id of the group key that should be used to encrypt messages. This option\n"
        "       overrides any of the following options.\n"
        "\n"
        "  --group-enc-key-type <key-type>\n"
        "       Key type of the group key to be used for encrypting messages.\n"
        "       Valid values for <key-type> are:\n"
        "           r - rotating message encryption group key.\n"
        "           s - static message encryption group key.\n"
        "\n"
        "  --group-enc-root-key <root-type>\n"
        "       Root key type to be used to generate the group key id for encrypting messages.\n"
        "       Valid values for <root-type> are:\n"
        "           f - fabric root key.\n"
        "           c - client root key.\n"
        "           s - service root key.\n"
        "\n"
        "  --group-enc-epoch-key-num <int>\n"
        "       Epoch key number to be used to generate the group key id for encrypting messages.\n"
        "       when group key encyption option is chosen.\n"
        "\n"
        "  --group-enc-app-key-num <int>\n"
        "       Application group master key number to be used to generate the group key id for\n"
        "       encrypting messages.\n"
        "\n"
#endif // WEAVE_CONFIG_USE_APP_GROUP_KEYS_FOR_MSG_ENC
        "";

    // Defaults.
    EncKeyId = WeaveKeyId::kNone;
    EncKeyType = WeaveKeyId::kType_None;
    RootKeyId = WeaveKeyId::kNone;
    EpochKeyId = WeaveKeyId::kNone;
    AppGroupMasterKeyId = WeaveKeyId::kNone;
}

uint32_t GroupKeyEncOptions::GetEncKeyId() const
{
    if (EncKeyId != WeaveKeyId::kNone)
        return EncKeyId;
    if (EncKeyType == WeaveKeyId::kType_None)
        return WeaveKeyId::kNone;
    if (RootKeyId == WeaveKeyId::kNone || AppGroupMasterKeyId == WeaveKeyId::kNone)
        return WeaveKeyId::kNone;
    return WeaveKeyId::MakeAppKeyId(EncKeyType, RootKeyId,
                (EncKeyType == WeaveKeyId::kType_AppRotatingKey) ? EpochKeyId : WeaveKeyId::kNone,
                AppGroupMasterKeyId,
                (EncKeyType == WeaveKeyId::kType_AppRotatingKey && EpochKeyId == WeaveKeyId::kNone));
}

bool GroupKeyEncOptions::HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg)
{
    switch (id)
    {
#if WEAVE_CONFIG_USE_APP_GROUP_KEYS_FOR_MSG_ENC
    case kToolCommonOpt_GroupEncKeyId:
        if (!ParseInt(arg, EncKeyId, 0) || !WeaveKeyId::IsValidKeyId(EncKeyId) || !WeaveKeyId::IsAppGroupKey(EncKeyId))
        {
            PrintArgError("%s: Invalid value specified for the group encryption key id: %s\n", progName, arg);
            return false;
        }
        break;
    case kToolCommonOpt_GroupEncKeyType:
    {
        bool invalidKeyType;
        invalidKeyType = false;
        if (strlen(arg) != 1)
        {
            invalidKeyType = true;
        }
        else if (*arg == 'r')
        {
            EncKeyType = WeaveKeyId::kType_AppRotatingKey;
        }
        else if (*arg == 's')
        {
            EncKeyType = WeaveKeyId::kType_AppStaticKey;
        }
        else
        {
            invalidKeyType = true;
        }

        if (invalidKeyType)
        {
            PrintArgError("%s: Invalid value specified for the group encryption key type: %s\n", progName, arg);
            return false;
        }
        break;
    }
    case kToolCommonOpt_GroupEncRootKey:
    {
        bool invalidRootKey;
        invalidRootKey = false;
        if (strlen(arg) != 1)
        {
            invalidRootKey = true;
        }
        else if (*arg == 'f')
        {
            RootKeyId = WeaveKeyId::kFabricRootKey;
        }
        else if (*arg == 'c')
        {
            RootKeyId = WeaveKeyId::kClientRootKey;
        }
        else if (*arg == 's')
        {
            RootKeyId = WeaveKeyId::kServiceRootKey;
        }
        else
        {
            invalidRootKey = true;
        }

        if (invalidRootKey)
        {
            PrintArgError("%s: Invalid value specified for the root key: %s\n", progName, arg);
            return false;
        }
        break;
    }
    case kToolCommonOpt_GroupEncEpochKeyNum:
    {
        uint32_t epochKeyNum;
        if (!ParseInt(arg, epochKeyNum, 0) || epochKeyNum > 7)
        {
            PrintArgError("%s: Invalid value specified for the epoch key number: %s\n", progName, arg);
            return false;
        }
        EpochKeyId = WeaveKeyId::MakeEpochKeyId(epochKeyNum);
        break;
    }
    case kToolCommonOpt_GroupEncAppGroupMasterKeyNum:
    {
        uint32_t masterKeyNum;
        if (!ParseInt(arg, masterKeyNum, 0) || masterKeyNum > 127)
        {
            PrintArgError("%s: Invalid value specified for the group master key number: %s\n", progName, arg);
            return false;
        }
        AppGroupMasterKeyId = WeaveKeyId::MakeAppGroupMasterKeyId(masterKeyNum);
        break;
    }
#endif // WEAVE_CONFIG_USE_APP_GROUP_KEYS_FOR_MSG_ENC
    default:
        PrintArgError("%s: INTERNAL ERROR: Unhandled option: %s\n", progName, name);
        return false;
    }

    return true;
}

GeneralSecurityOptions::GeneralSecurityOptions()
{
    static OptionDef optionDefs[] =
    {
#if WEAVE_CONFIG_ENABLE_CASE_RESPONDER
        { "idle-session-timeout",           kArgumentRequired, kToolCommonOpt_GeneralSecurityIdleSessionTimeout                },
        { "session-establishment-timeout",  kArgumentRequired, kToolCommonOpt_GeneralSecuritySessionEstablishmentTimeout       },
#endif
        { NULL }
    };
    OptionDefs = optionDefs;

    HelpGroupName = "GENERAL SECURITY OPTIONS";

    OptionHelp =
        "  --idle-session-timeout <int>\n"
        "       The number of milliseconds after which an idle session will be removed.\n"
        "\n"
        "  --session-establishment-timeout <int>\n"
        "       The number of milliseconds after which an in-progress session establishment will timeout.\n"
        "\n"
        "";

    // Defaults.
    IdleSessionTimeout = WEAVE_CONFIG_DEFAULT_SECURITY_SESSION_IDLE_TIMEOUT;
    SessionEstablishmentTimeout = WEAVE_CONFIG_DEFAULT_SECURITY_SESSION_ESTABLISHMENT_TIMEOUT;
}

uint32_t GeneralSecurityOptions::GetIdleSessionTimeout() const
{
    return IdleSessionTimeout;
}

uint32_t GeneralSecurityOptions::GetSessionEstablishmentTimeout() const
{
    return SessionEstablishmentTimeout;
}

bool GeneralSecurityOptions::HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg)
{
    switch (id)
    {
    case kToolCommonOpt_GeneralSecurityIdleSessionTimeout:
        if ((!ParseInt(arg, IdleSessionTimeout)) || IdleSessionTimeout == 0)
        {
            PrintArgError("%s: Invalid value specified for the idle session timeout: %s\n", progName, arg);
            return false;
        }
        break;
    case kToolCommonOpt_GeneralSecuritySessionEstablishmentTimeout:
        if ((!ParseInt(arg, SessionEstablishmentTimeout)) || SessionEstablishmentTimeout == 0)
        {
            PrintArgError("%s: Invalid value specified for the session establishment timeout: %s\n", progName, arg);
            return false;
        }
        break;
    default:
        PrintArgError("%s: INTERNAL ERROR: Unhandled option: %s\n", progName, name);
        return false;
    }

    return true;
}

ServiceDirClientOptions::ServiceDirClientOptions()
{
    static OptionDef optionDefs[] =
    {
#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
        { "service-dir-server",             kArgumentRequired, kToolCommonOpt_ServiceDirServer },
        { "service-dir-url",                kArgumentRequired, kToolCommonOpt_ServiceDirServer }, // deprecated alias for service-dir-server
#if WEAVE_CONFIG_ENABLE_DNS_RESOLVER
        { "service-dir-dns-options",        kArgumentRequired, kToolCommonOpt_ServiceDirDNSOptions },
        { "service-dir-target-dns-options", kArgumentRequired, kToolCommonOpt_ServiceDirTargetDNSOptions },
#endif // WEAVE_CONFIG_ENABLE_DNS_RESOLVER
#endif // WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
        { NULL }
    };
    OptionDefs = optionDefs;

    HelpGroupName = "SERVICE DIRECTORY OPTIONS";

    OptionHelp =
#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
        "  --service-dir-server <host-name-or-ip-addr>[:<port>]\n"
        "       Use the specified server when making service directory requests.\n"
        "       (Deprecated alias: --service-dir-url)\n"
        "\n"
#if WEAVE_CONFIG_ENABLE_DNS_RESOLVER
        "  --service-dir-dns-options <dns-options>\n"
        "  --service-dir-target-dns-options <dns-options>\n"
        "       Use the specified DNS options when resolving hostnames during a\n"
        "       service connection attempt.  The first option controls the DNS\n"
        "       options used when connecting to the ServiceDirectory endpoint\n"
        "       itself.  The second option controls the DNS option used when\n"
        "       connecting to the endpoint that is ultimate target of the service\n"
        "       connection.  <dns-options> can be one of the following keywords:\n"
        "           Any (the default)\n"
        "              - Resolve IPv4 and/or IPv6 addresses in the native order\n"
        "                returned by the name server.\n"
        "           IPv4Only\n"
        "              - Resolve IPv4 addresses only.\n"
        "           IPv6Only\n"
        "              - Resolve IPv6 addresses only.\n"
        "           IPv4Preferred\n"
        "              - Resolve IPv4 and/or IPv6 addresses, with IPv4 addresses\n"
        "                given preference over IPv6.\n"
        "           IPv6Preferred\n"
        "              - Resolve IPv4 and/or IPv6 addresses, with IPv6 addresses\n"
        "                given preference over IPv4.\n"
        "\n"
#endif // WEAVE_CONFIG_ENABLE_DNS_RESOLVER
#endif // WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
        "";

    // Defaults.
    ServerHost = "frontdoor.integration.nestlabs.com";
    ServerPort = WEAVE_PORT;
#if WEAVE_CONFIG_ENABLE_DNS_RESOLVER
    DNSOptions_ServiceDirEndpoint = kDNSOption_AddrFamily_Any;
    DNSOptions_TargetEndpoint = kDNSOption_AddrFamily_Any;
#endif
}

bool ServiceDirClientOptions::HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg)
{
    WEAVE_ERROR err;
    const char *host;
    uint16_t hostLen;

    switch (id)
    {
#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    case kToolCommonOpt_ServiceDirServer:
        err = ParseHostAndPort(arg, strlen(arg), host, hostLen, ServerPort);
        if (err != WEAVE_NO_ERROR)
        {
            PrintArgError("%s: Invalid value specified for Service Directory server name: %s\n", progName, arg);
            return false;
        }
        ServerHost = strndup(host, hostLen);
        if (ServerPort == 0)
            ServerPort = WEAVE_PORT;
        break;
#if WEAVE_CONFIG_ENABLE_DNS_RESOLVER
    case kToolCommonOpt_ServiceDirDNSOptions:
        if (!ParseDNSOptions(progName, name, arg, DNSOptions_ServiceDirEndpoint))
        {
            return false;
        }
        break;
    case kToolCommonOpt_ServiceDirTargetDNSOptions:
        if (!ParseDNSOptions(progName, name, arg, DNSOptions_TargetEndpoint))
        {
            return false;
        }
        break;
#endif // WEAVE_CONFIG_ENABLE_DNS_RESOLVER
#endif // WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    default:
        PrintArgError("%s: INTERNAL ERROR: Unhandled option: %s\n", progName, name);
        return false;
    }

    return true;
}

WEAVE_ERROR ServiceDirClientOptions::GetRootDirectoryEntry(uint8_t *buf, uint16_t bufSize)
{
    using namespace nl::Weave::Encoding;
    using namespace nl::Weave::Profiles::ServiceDirectory;

    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint8_t serverHostLen = (uint8_t)strlen(ServerHost);

    VerifyOrExit(bufSize >= (1 + 8 + 1 + 1 + serverHostLen + 2), err = WEAVE_ERROR_BUFFER_TOO_SMALL);

    Write8(buf, 0x41);                                       // Entry Type = Host/Port List, Host/Port List = 1
    LittleEndian::Write64(buf, kServiceEndpoint_Directory);  // Service Endpoint Id = Directory Service
    Write8(buf, 0x80);                                       // Host ID Type = Fully Qualified, Suffix Index Present = false, Port Id Present = true
    Write8(buf, serverHostLen);
    memcpy(buf, ServerHost, serverHostLen); buf += serverHostLen;
    LittleEndian::Write16(buf, ServerPort);

exit:
    return err;
}

WEAVE_ERROR GetRootServiceDirectoryEntry(uint8_t *buf, uint16_t bufSize)
{
    return gServiceDirClientOptions.GetRootDirectoryEntry(buf, bufSize);
}

void ServiceDirClientOptions::OverrideConnectArguments(ServiceConnectBeginArgs & args)
{
#if WEAVE_CONFIG_ENABLE_DNS_RESOLVER

    // If specific DNS options have been set for the connection to the
    // ServiceDirectory endpoint, override the default DNS options.
    if (args.ServiceEndpoint == kServiceEndpoint_Directory)
    {
        if (DNSOptions_ServiceDirEndpoint != kDNSOption_AddrFamily_Any)
        {
            args.DNSOptions = DNSOptions_ServiceDirEndpoint;
        }
    }

    // Otherwise, if specific DNS options have been set for all other service connections
    // override the default DNS options.
    else
    {
        if (DNSOptions_TargetEndpoint != kDNSOption_AddrFamily_Any)
        {
            args.DNSOptions = DNSOptions_TargetEndpoint;
        }
    }

#endif // WEAVE_CONFIG_ENABLE_DNS_RESOLVER
}

void OverrideServiceConnectArguments(::nl::Weave::Profiles::ServiceDirectory::ServiceConnectBeginArgs & args)
{
    gServiceDirClientOptions.OverrideConnectArguments(args);
}

FaultInjectionOptions::FaultInjectionOptions()
{
    static OptionDef optionDefs[] =
    {
#if WEAVE_CONFIG_TEST || WEAVE_SYSTEM_CONFIG_TEST || INET_CONFIG_TEST
        { "faults",                kArgumentRequired, kToolCommonOpt_FaultInjection      },
        { "iterations",            kArgumentRequired, kToolCommonOpt_FaultTestIterations },
        { "debug-resource-usage",  kNoArgument,       kToolCommonOpt_DebugResourceUsage  },
        { "print-fault-counters",  kNoArgument,       kToolCommonOpt_PrintFaultCounters  },
        { "extra-cleanup-time",    kArgumentRequired, kToolCommonOpt_ExtraCleanupTime },
#endif
        { NULL }
    };
    OptionDefs = optionDefs;

    HelpGroupName = "FAULT INJECTION OPTIONS";

    OptionHelp =
#if WEAVE_CONFIG_TEST || WEAVE_SYSTEM_CONFIG_TEST || INET_CONFIG_TEST
        "  --faults <fault-string>\n"
        "       Inject specified fault(s) into the operation of the tool at runtime.\n"
        "\n"
        "  --iterations <int>\n"
        "       Execute the program operation the given number of times\n"
        "\n"
        "  --debug-resource-usage\n"
        "       Print all stats counters before exiting.\n"
        "\n"
        "  --print-fault-counters\n"
        "       Print the fault-injection counters before exiting.\n"
        "\n"
        "  --extra-cleanup-time\n"
        "       Allow extra time before asserting resource leaks; this is useful when\n"
        "       running fault-injection tests to let the system free stale ExchangeContext\n"
        "       instances after WRMP has exhausted all retransmission; a failed WRMP transmission\n"
        "       should fail a normal happy-sequence test, but not necessarily a fault-injection test.\n"
        "       The value is in milliseconds; a common value is 10000.\n"
        "\n"
#endif
        "";

    // Defaults
    TestIterations = 1;
    DebugResourceUsage = false;
    PrintFaultCounters = false;
    ExtraCleanupTimeMsec = 0;
}

bool FaultInjectionOptions::HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg)
{
    using namespace nl::FaultInjection;

    GetManagerFn faultMgrFnTable[] =
    {
        nl::Weave::FaultInjection::GetManager,
        nl::Inet::FaultInjection::GetManager,
        nl::Weave::System::FaultInjection::GetManager
    };
    size_t faultMgrFnTableLen = sizeof(faultMgrFnTable) / sizeof(faultMgrFnTable[0]);

    switch (id)
    {
#if WEAVE_CONFIG_TEST || WEAVE_SYSTEM_CONFIG_TEST || INET_CONFIG_TEST
    case kToolCommonOpt_FaultInjection:
    {
        char *mutableArg = strdup(arg);
        bool parseRes = ParseFaultInjectionStr(mutableArg, faultMgrFnTable, faultMgrFnTableLen);
        free(mutableArg);
        if (!parseRes)
        {
            PrintArgError("%s: Invalid string specified for fault injection option: %s\n", progName, arg);
            return false;
        }
        break;
    }
    case kToolCommonOpt_FaultTestIterations:
        if ((!ParseInt(arg, TestIterations)) || (TestIterations == 0))
        {
            PrintArgError("%s: Invalid value specified for the number of iterations to execute: %s\n", progName, arg);
            return false;
        }
        break;
    case kToolCommonOpt_DebugResourceUsage:
        DebugResourceUsage = true;
        break;
    case kToolCommonOpt_PrintFaultCounters:
        PrintFaultCounters = true;
        break;
    case kToolCommonOpt_ExtraCleanupTime:
        if ((!ParseInt(arg, ExtraCleanupTimeMsec)) || (ExtraCleanupTimeMsec == 0))
        {
            PrintArgError("%s: Invalid value specified for the extra time to wait before checking for leaks: %s\n", progName, arg);
            return false;
        }
        break;
#endif // WEAVE_CONFIG_TEST || WEAVE_SYSTEM_CONFIG_TEST || INET_CONFIG_TEST
    default:
        PrintArgError("%s: INTERNAL ERROR: Unhandled option: %s\n", progName, name);
        return false;
    }

    return true;
}


#if WEAVE_CONFIG_ENABLE_DNS_RESOLVER

static bool GetToken(const char * & inStr, const char * & token, size_t & tokenLen, const char * sepChars)
{
    token = inStr;

    // Skip leading whitespace
    while (*token != 0 && isspace(*token))
    {
        token++;
    }

    // Check no more tokens.
    if (*token == 0)
    {
        return false;
    }

    // Find end of token and advance inStr beyond end of token.
    const char * tokenEnd = strpbrk(token, sepChars);
    if (tokenEnd != NULL)
    {
        tokenLen = tokenEnd - inStr;
        inStr = tokenEnd + 1;
    }
    else
    {
        tokenLen = strlen(token);
        inStr += tokenLen;
    }

    // Trim trailing whitespace.
    while (tokenLen > 0 && isspace(token[tokenLen - 1]))
    {
        tokenLen--;
    }

    return true;
}

static bool MatchToken(const char * token, size_t tokenLen, const char * expectedStr)
{
    return strlen(expectedStr) == tokenLen && strncasecmp(expectedStr, token, tokenLen) == 0;
}

/**
 * Parse a string representation of the nl::Inet::DNSOptions enumeration.
 */
bool ParseDNSOptions(const char * progName, const char *argName, const char * arg, uint8_t & dnsOptions)
{
    const char * token;
    size_t tokenLen;

    dnsOptions = kDNSOption_AddrFamily_Any;

    while (GetToken(arg, token, tokenLen, ",|:"))
    {
        if (MatchToken(token, tokenLen, "Any"))
        {
            dnsOptions = (dnsOptions & ~kDNSOption_AddrFamily_Mask) | kDNSOption_AddrFamily_Any;
        }
        else if (MatchToken(token, tokenLen, "IPv4Only"))
        {
#if INET_CONFIG_ENABLE_IPV4
            dnsOptions = (dnsOptions & ~kDNSOption_AddrFamily_Mask) | kDNSOption_AddrFamily_IPv4Only;
#else
            PrintArgError("%s: DNSOption IPv4Only not supported\n", progName);
            return false;
#endif
        }
        else if (MatchToken(token, tokenLen, "IPv4Preferred"))
        {
#if INET_CONFIG_ENABLE_IPV4
            dnsOptions = (dnsOptions & ~kDNSOption_AddrFamily_Mask) | kDNSOption_AddrFamily_IPv4Preferred;
#else
            PrintArgError("%s: DNSOption IPv4Preferred not supported\n", progName);
            return false;
#endif
        }
        else if (MatchToken(token, tokenLen, "IPv6Only"))
        {
            dnsOptions = (dnsOptions & ~kDNSOption_AddrFamily_Mask) | kDNSOption_AddrFamily_IPv6Only;
        }
        else if (MatchToken(token, tokenLen, "IPv6Preferred"))
        {
            dnsOptions = (dnsOptions & ~kDNSOption_AddrFamily_Mask) | kDNSOption_AddrFamily_IPv6Preferred;
        }
        else
        {
            PrintArgError("%s: Unrecognized value specified for %s: %*s\n", progName, argName, tokenLen, token);
            return false;
        }
    }

    return true;
}

#endif // WEAVE_CONFIG_ENABLE_DNS_RESOLVER
