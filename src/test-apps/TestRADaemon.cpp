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
 *      the Weave IPv6 Router Advertisement (RA) daemon.
 *
 */

#include "ToolCommon.h"

#include <InetLayer/IPPrefix.h>

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS
#include <arpa/inet.h>
#endif

#include <string.h>

#include <RADaemon.h>

#if WEAVE_SYSTEM_CONFIG_USE_LWIP

#include "lwip/netif.h"

#define TOOL_NAME "TestRADaemon"

static bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg);

bool Listen = false;

static OptionDef gToolOptionDefs[] =
{
    { "listen", kNoArgument, 'L' },
    { NULL }
};

static const char *gToolOptionHelp =
    "  -L, --listen\n"
    "       Listen for incoming data.\n"
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
    "Usage: " TOOL_NAME " [<options...>]\n"
    "       " TOOL_NAME " [<options...>] --listen\n",
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

int main(int argc, char *argv[])
{
    INET_ERROR err;

    IPPrefix ipPrefix;
    RADaemon radaemon;

    SetSIGUSR1Handler();

    if (!ParseArgsFromEnvVar(TOOL_NAME, TOOL_OPTIONS_ENV_VAR_NAME, gToolOptionSets, NULL, true) ||
        !ParseArgs(TOOL_NAME, argc, argv, gToolOptionSets))
    {
        exit(EXIT_FAILURE);
    }

    InitSystemLayer();
    InitNetwork();

    radaemon.Init(SystemLayer, Inet);

    struct netif *et0_intf = NULL;
    //Dump info about the various LwIP interfaces.
    for (netif *intf = netif_list; intf != NULL; intf=intf->next)
    {
        if ((intf->name[0] == 'e') && (intf->name[1] == 't') && (intf->num == 0))
        {
            et0_intf = intf;
        }
        for (int j = 0; j < LWIP_IPV6_NUM_ADDRESSES; ++j)
        {
            char ipv6_str[INET6_ADDRSTRLEN];
#if LWIP_VERSION_MAJOR > 1
            ip6addr_ntoa_r(ip_2_ip6(&intf->ip6_addr[j]), ipv6_str, sizeof(ipv6_str));
#else // LWIP_VERSION_MAJOR <= 1
            ip6addr_ntoa_r(&intf->ip6_addr[j], ipv6_str, sizeof(ipv6_str));
#endif // LWIP_VERSION_MAJOR <= 1
            printf("intf->name: %c%c%u (IPv6: %s)\n", intf->name[0], intf->name[1], intf->num, ipv6_str);
        }
    }

    //Set first prefix
    IPAddress::FromString("fd01:0001:0002:0003:0004:0005:0006:0001", ipPrefix.IPAddr);
    ipPrefix.Length = 64;
//  if ((err = radaemon.SetPrefixInfo("et0", LocalIPv6Addr, ipPrefix, 7200, 7200)) != INET_NO_ERROR)
  if ((err = radaemon.SetPrefixInfo(et0_intf, gNetworkOptions.LocalIPv6Addr, ipPrefix, 7200, 7200)) != INET_NO_ERROR)
    { printf("SetPrefixInfo (err: %d)\n", err); } else { printf("SetPrefixInfo (err: SUCCESS)\n"); }

    //Set second prefix
    IPAddress::FromString("fd02:0001:0002:0003:0004:0005:0006:0002", ipPrefix.IPAddr);
    ipPrefix.Length = 48;
//  if ((err = radaemon.SetPrefixInfo("et0", LocalIPv6Addr, ipPrefix, 7200, 7200)) != INET_NO_ERROR)
  if ((err = radaemon.SetPrefixInfo(et0_intf, gNetworkOptions.LocalIPv6Addr, ipPrefix, 7200, 7200)) != INET_NO_ERROR)
    { printf("SetPrefixInfo (err: %d)\n", err); } else { printf("SetPrefixInfo (err: SUCCESS)\n"); }

    //Set third prefix
    IPAddress::FromString("fd03:1234:ffff:ffff:ffff:ffff:ffff:ffff", ipPrefix.IPAddr);
    ipPrefix.Length = 97;
//  if ((err = radaemon.SetPrefixInfo("et0", LocalIPv6Addr, ipPrefix, 7200, 7200)) != INET_NO_ERROR)
  if ((err = radaemon.SetPrefixInfo(et0_intf, gNetworkOptions.LocalIPv6Addr, ipPrefix, 7200, 7200)) != INET_NO_ERROR)
    { printf("SetPrefixInfo (err: %d)\n", err); } else { printf("SetPrefixInfo (err: SUCCESS)\n"); }

    //Dump the current content of the LinkInfo table.
    printf("\nFirst dump of the table:\n");
    for (int j = 0; j < RAD_MAX_ADVERTISING_LINKS; ++j)
    {
        for (int k = 0; k < RAD_MAX_PREFIXES_PER_LINK; ++k)
        {
            printf("LinkInfo[%d].IPPrefixInfo[%d].IPPrefix: %08x%08x%08x%08x/%u\n", j, k,
                    radaemon.LinkInfo[j].IPPrefixInfo[k].IPPrefx.IPAddr.Addr[0], radaemon.LinkInfo[j].IPPrefixInfo[k].IPPrefx.IPAddr.Addr[1],
                    radaemon.LinkInfo[j].IPPrefixInfo[k].IPPrefx.IPAddr.Addr[2], radaemon.LinkInfo[j].IPPrefixInfo[k].IPPrefx.IPAddr.Addr[3],
                    radaemon.LinkInfo[j].IPPrefixInfo[k].IPPrefx.Length);
        }
    }

    //Delete second prefix
    IPAddress::FromString("fd02:0001:0002:0000:0000:0000:0000:0000", ipPrefix.IPAddr);
    ipPrefix.Length = 48;
//    radaemon.DelPrefixInfo("et0", ipPrefix);
    radaemon.DelPrefixInfo(et0_intf, ipPrefix);

    //Dump the current content of the LinkInfo table.
    printf("\nSecond dump of the table:\n");
    for (int j = 0; j < RAD_MAX_ADVERTISING_LINKS; ++j)
    {
        for (int k = 0; k < RAD_MAX_PREFIXES_PER_LINK; ++k)
        {
            printf("LinkInfo[%d].IPPrefixInfo[%d].IPPrefix: %08x%08x%08x%08x/%u\n", j, k,
                    radaemon.LinkInfo[j].IPPrefixInfo[k].IPPrefx.IPAddr.Addr[0], radaemon.LinkInfo[j].IPPrefixInfo[k].IPPrefx.IPAddr.Addr[1],
                    radaemon.LinkInfo[j].IPPrefixInfo[k].IPPrefx.IPAddr.Addr[2], radaemon.LinkInfo[j].IPPrefixInfo[k].IPPrefx.IPAddr.Addr[3],
                    radaemon.LinkInfo[j].IPPrefixInfo[k].IPPrefx.Length);
        }
    }

	while (!Done)
	{
		struct timeval sleepTime;
		sleepTime.tv_sec = 0;
		sleepTime.tv_usec = 10000;

		ServiceNetwork(sleepTime);
	}

	return EXIT_SUCCESS;
}

bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg)
{
    switch (id)
    {
    case 'L':
        Listen = true;
        break;
    default:
        PrintArgError("%s: INTERNAL ERROR: Unhandled option: %s\n", progName, name);
        return false;
    }

    return true;
}

#else // WEAVE_SYSTEM_CONFIG_USE_LWIP

int main(int argc, char *argv[])
{
    return EXIT_SUCCESS;
}

#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP
