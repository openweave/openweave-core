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
 *      a server for the Weave Software Update (SWU) profile.
 *
 */

#define __STDC_FORMAT_MACROS
#define __STDC_LIMIT_MACROS

#include <inttypes.h>
#include <stdlib.h>

#include "ToolCommon.h"
#include <Weave/Core/WeaveSecurityMgr.h>
#include <Weave/Profiles/security/WeaveSecurity.h>
#include "MockSWUServer.h"

using namespace nl::Weave;
using namespace nl::Inet;

#define TOOL_NAME "weave-swu-server"

static bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg);
static bool ParseStringToUint8_List(char const *aString, char const *aDelim, uint8_t *aList, uint8_t *aListSize);
static void HandleConnectionReceived(WeaveMessageLayer *msgLayer, WeaveConnection *con);
static void HandleSecureSessionEstablished(WeaveSecurityManager *sm, WeaveConnection *con, void *reqState, uint16_t sessionKeyId, uint64_t peerNodeId, uint8_t encType);
static void HandleSecureSessionError(WeaveSecurityManager *sm, WeaveConnection *con, void *reqState, WEAVE_ERROR localErr, uint64_t peerNodeId, StatusReport *statusReport);
static void HandleConnectionClosed(WeaveConnection *con, WEAVE_ERROR conErr);
static void GenerateReferenceImageQuery(ImageQuery *aImageQuery);
static void HandleConnectionComplete(WeaveConnection *con, WEAVE_ERROR conErr);
static void StartServerConnection();

MockSoftwareUpdateServer MockSWUServer;

uint16_t ProductId              = 1;
uint16_t ProductRev             = 1;
uint16_t VendorId               = kWeaveVendor_NestLabs;
const char *gSoftwareVersion            = "1";
const char *gUpdateSchemeList   = "3";
const char *gIntegrityTypeList  = "0";
const char *gFileDesignator     = NULL;
bool gListening                 = false;
const char *gDestAddrStr        = NULL;
const char *gDestNodeIdStr      = NULL;
IPAddress gDestIPAddr;
WeaveConnection *Con            = NULL;
static uint64_t gDestNodeId     = 1;
bool gUseTCP                     = true;

enum
{
    kToolOpt_SoftwareVersion = 1000,
    kToolOpt_ProductId,
    kToolOpt_ProductRev,
    kToolOpt_VendorId,
    kToolOpt_IntegrityType,
    kToolOpt_UpdateScheme,
    kToolOpt_FileDesignator,
    kToolOpt_Listen,
    kToolOpt_DestAddr,
    kToolOpt_DestNodeId,
    kToolOpt_UseTCP,
    kToolOpt_UseUDP
};

static OptionDef gToolOptionDefs[] =
{
    { "sw-version",      kArgumentRequired, kToolOpt_SoftwareVersion  },
    { "product-id",      kArgumentRequired, kToolOpt_ProductId        },
    { "product-rev",     kArgumentRequired, kToolOpt_ProductRev       },
    { "vendor-id",       kArgumentRequired, kToolOpt_VendorId         },
    { "integrity-type",  kArgumentRequired, kToolOpt_IntegrityType    },
    { "update-scheme",   kArgumentRequired, kToolOpt_UpdateScheme     },
    { "file-designator", kArgumentRequired, kToolOpt_FileDesignator   },
    { "listen",          kNoArgument,       kToolOpt_Listen           },
    { "dest-addr",       kArgumentRequired, kToolOpt_DestAddr         },
    { "dest-node-id",    kArgumentRequired, kToolOpt_DestNodeId       },
    { "tcp",             kNoArgument,       kToolOpt_UseTCP           },
    { "udp",             kNoArgument,       kToolOpt_UseUDP           },
    { NULL }
};

static const char *gToolOptionHelp =
    " The following arguments are required : \n"
    "\n"
    " --vendor-id <num>\n"
    "       Unique vendor identifier of the sending device\n"
    "       Default is set to 0x235A -> NestLabs\n"
    "\n"
    " --sw-version <version>\n"
    "       Software version that will be compared against the version reported\n"
    "       through the image query. Default is 1.\n"
    "\n"
    " --product-id <num>\n"
    "       Product Id  is the vendor’s unique hardware product identity\n"
    "       of the sending device. Default is 1.\n"
    "\n"
    " --product-rev <num>\n"
    "       Vendor’s product’s hardware revision number of the sending device\n"
    "       Default is set to 1.\n"
    "\n"
    " --integrity-type <num>\n"
    "       Integrity type supported by the sending device\n"
    "       Default is set to 0 -> SHA512\n"
    "       0 -> SHA160 160-bit Secure Hash, aka SHA-1, required\n"
    "       1 -> SHA256 256-bit Secure Hash (SHA-2)\n"
    "       2 -> SHA512 512-bit, Secure Hash (SHA-2)\n"
    "\n"
    " --update-scheme <num>\n"
    "       Update schemes supported by the sending device\n"
    "       Default is set to 3 -> BDX\n"
    "       0 -> HTTP\n"
    "       1 -> HTTPS\n"
    "       2 -> SFTP\n"
    "       3 -> BDX Nest Weave download protocol\n"
     "\n"
    " --file-designator <string>\n"
    "       Path to the image file that is returned to the query\n"
    "       when an update is available. The path must be valid.\n"
    "\n"
    "  --tcp\n"
    "       Use TCP to send SWU Image Announce messages. This is the default.\n"
    "\n"
    "  --udp\n"
    "       Use UDP to send SWU Image Announce messages.\n"
    "\n"
    " --listen\n"
    "       Listen and respond to image request sent from another node.\n"
    "       Otherwise, Send Image Announce notification firstly.\n"
    "\n"
    "  --dest-addr <host>[:<port>]\n"
    "       Send an Image Announce notification to a specific address rather than one\n"
    "       derived from the destination node id.  <host> can be a hostname,\n"
    "       an IPv4 address or an IPv6 address.  If <port> is specified, image announce\n"
    "       will be sent to the specified port.\n"
    "\n"
    "  --dest-node-id\n"
    "       Send an Image Announce notification to a specific node id."
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
    "Usage: " TOOL_NAME " <options...>\n",
    WEAVE_VERSION_STRING "\n" WEAVE_TOOL_COPYRIGHT
);

static OptionSet *gToolOptionSets[] =
{
    &gToolOptions,
    &gNetworkOptions,
    &gWeaveNodeOptions,
    &gFaultInjectionOptions,
    &gHelpOptions,
    NULL
};

bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg)
{
    switch (id)
    {
    case kToolOpt_SoftwareVersion:
        gSoftwareVersion = arg;
        break;
    case kToolOpt_ProductId:
        if (!ParseInt(arg, ProductId))
        {
            PrintArgError("%s: Invalid value specified for product-id: %s\n", progName, arg);
            return false;
        }
        break;
    case kToolOpt_ProductRev:
        if (!ParseInt(arg, ProductRev))
        {
            PrintArgError("%s: Invalid value specified for product-rev: %s\n", progName, arg);
            return false;
        }
        break;
    case kToolOpt_VendorId:
        if (!ParseInt(arg, VendorId))
        {
            PrintArgError("%s: Invalid value specified for vendor-id: %s\n", progName, arg);
            return false;
        }
        break;
    case kToolOpt_IntegrityType:
        gIntegrityTypeList = arg;
        break;
    case kToolOpt_UpdateScheme:
        gUpdateSchemeList = arg;
        break;
    case kToolOpt_FileDesignator:
        gFileDesignator = arg;
        break;
    case kToolOpt_UseTCP:
        gUseTCP = true;
        break;
    case kToolOpt_UseUDP:
        gUseTCP = false;
        break;
    case kToolOpt_Listen:
        gListening = true;
        break;
    case kToolOpt_DestAddr:
        gDestAddrStr = arg;
        break;
    case kToolOpt_DestNodeId:
        gDestNodeIdStr = arg;
        if (!ParseNodeId(gDestNodeIdStr, gDestNodeId))
        {
            PrintArgError("%s: Invalid value specified for destination node-id: %s\n", progName, gDestNodeIdStr);
            return false;
        }
        break;
    default:
        PrintArgError("%s: INTERNAL ERROR: Unhandled option: %s\n", progName, name);
        return false;
    }

    return true;
}

void GenerateReferenceImageQuery(ImageQuery *aImageQuery)
{
    ProductSpec productSpec(VendorId,
                            ProductId,
                            ProductRev);

    ReferencedString versionString;
    versionString.init((uint8_t)strlen(gSoftwareVersion), (char *)gSoftwareVersion);

    IntegrityTypeList aTypeList;
    if (!ParseStringToUint8_List((char const *)gIntegrityTypeList, ",", aTypeList.theList, &aTypeList.theLength))
        exit(EXIT_FAILURE);

    UpdateSchemeList aSchemeList;
    if (!ParseStringToUint8_List((char const *)gUpdateSchemeList, ",", aSchemeList.theList, &aSchemeList.theLength))
        exit(EXIT_FAILURE);

    aImageQuery->init(productSpec, versionString, aTypeList, aSchemeList,
                    NULL /*package*/, NULL /*locale*/, 0 /*target node id*/, NULL /*metadata*/);
}

void StartServerConnection()
{
    printf("0 StartClientConnection entering (Con: %p)\n", Con);

    if (Con != NULL && Con->State == WeaveConnection::kState_Closed)
    {
        printf("  1 remove previous con (currently closed)\n");
        Con->Close();
        Con = NULL;
    }

    // Create a new connection unless there is already one in progress
    // (probably started via an ImageAnnounce notification.)
    if (Con == NULL)
    {
        printf("  2 no existing connection (probably no ImageAnnounce received)\n");
        Con = MessageLayer.NewConnection();
        if (Con == NULL)
        {
            printf("  3 WeaveConnection.Connect failed: no memory\n");
            return;
        }
        Con->OnConnectionComplete = HandleConnectionComplete;
        Con->OnConnectionClosed = HandleConnectionClosed;
        printf("  4 Con: %p\n", Con);

        printf("  5 (DestNodeId: %ld, DestIPAddrStr: %s)\n", (long)gDestNodeId, gDestAddrStr);
        IPAddress::FromString(gDestAddrStr, gDestIPAddr);
        WEAVE_ERROR err = Con->Connect(gDestNodeId, kWeaveAuthMode_Unauthenticated, gDestIPAddr);
        if (err != WEAVE_NO_ERROR)
        {
            printf("  6 WeaveConnection.Connect failed: %X (%s)\n", err, ErrorStr(err));
            Con->Close();
            Con = NULL;
            return;
        }
    }
    else
    {
        printf("  7 existing connection (probably ImageAnnounce received)\n");
        HandleConnectionComplete(Con, WEAVE_NO_ERROR);
    }

    printf("8 StartClientConnection exiting\n");
}

int main(int argc, char *argv[])
{
    WEAVE_ERROR err;

    InitToolCommon();

    gWeaveNodeOptions.LocalNodeId = 0;

    SetSIGUSR1Handler();

    if (argc == 1)
    {
        gHelpOptions.PrintBriefUsage(stderr);
        exit(EXIT_FAILURE);
    }

    if (!ParseArgsFromEnvVar(TOOL_NAME, TOOL_OPTIONS_ENV_VAR_NAME, gToolOptionSets, NULL, true) ||
        !ParseArgs(TOOL_NAME, argc, argv, gToolOptionSets))
    {
        exit(EXIT_FAILURE);
    }

    if ((gListening && gDestNodeIdStr != NULL) || (!gListening && gDestNodeIdStr == NULL))
    {
        printf("Please specify either a node id or --listen\n");
        exit(EXIT_FAILURE);
    }

    if (gNetworkOptions.LocalIPv6Addr != IPAddress::Any)
    {
        if (!gNetworkOptions.LocalIPv6Addr.IsIPv6ULA())
        {
            printf("ERROR: Local address must be an IPv6 ULA\n");
            exit(EXIT_FAILURE);
        }

        if (gWeaveNodeOptions.LocalNodeId == 0)
            gWeaveNodeOptions.LocalNodeId = IPv6InterfaceIdToWeaveNodeId(gNetworkOptions.LocalIPv6Addr.InterfaceId());
        gWeaveNodeOptions.SubnetId = gNetworkOptions.LocalIPv6Addr.Subnet();
    }

    // Default LocalNodeId to 1 if not set explicitly, or by means of setting the node address.
    if (gWeaveNodeOptions.LocalNodeId == 0)
        gWeaveNodeOptions.LocalNodeId = 1;

    InitSystemLayer();

    InitNetwork();

    InitWeaveStack(true, true);

    PrintNodeConfig();

    // Arrange to get called for various activity in the message layer.
    MessageLayer.OnConnectionReceived = HandleConnectionReceived;
    MessageLayer.OnReceiveError = HandleMessageReceiveError;
    MessageLayer.OnAcceptError = HandleAcceptConnectionError;

    SecurityMgr.OnSessionEstablished = HandleSecureSessionEstablished;
    SecurityMgr.OnSessionError = HandleSecureSessionError;

    printf("\nUsing the following configuration:\n");
    printf("  Vendor Id: %d\n", VendorId);
    printf("  Product Id: %d\n", ProductId);
    printf("  Product Rev: %d\n", ProductRev);
    printf("  Software version: %s\n", gSoftwareVersion);
    printf("  Integrity Type: %s\n", gIntegrityTypeList);
    printf("  Update Scheme: %s\n", gUpdateSchemeList);
    printf("\n");

    ImageQuery imageQuery;
    GenerateReferenceImageQuery(&imageQuery);

    // Initialize the SWU-client application.
    err = MockSWUServer.Init(&ExchangeMgr);
    if (err != WEAVE_NO_ERROR)
    {
        printf("Software Update Server::Init failed: %s\n", ErrorStr(err));
        exit(EXIT_FAILURE);
    }

    MockSWUServer.SetReferenceImageQuery(&imageQuery);
    err = MockSWUServer.SetFileDesignator(gFileDesignator);
    if (err != WEAVE_NO_ERROR)
    {
        printf("Unable to open file: %s\n", gFileDesignator);
        printf("Make sure that the path exists and the file is valid\n");
        exit(EXIT_FAILURE);
    }

    if (gListening)
    {
        printf("Listening for Software Update requests...\n");
    }
    else
    {
        printf("Starting the TCP connection...\n");
        if (gUseTCP)
        {
            StartServerConnection();
        }
        else
        {
            err = MockSWUServer.SendImageAnnounce(gDestNodeId, gDestIPAddr);
            if (err != WEAVE_NO_ERROR)
            {
                printf("Software Update Server::SendImageAnnounce failed: %s\n", ErrorStr(err));
                return 0;
            }
        }
    }

    while (!Done)
    {
        struct timeval sleepTime;
        sleepTime.tv_sec = 0;
        sleepTime.tv_usec = 100000;

        ServiceNetwork(sleepTime);
    }

    MockSWUServer.Shutdown();
    ShutdownWeaveStack();
    ShutdownNetwork();
    ShutdownSystemLayer();

    return EXIT_SUCCESS;
}

void HandleConnectionComplete(WeaveConnection *con, WEAVE_ERROR conErr)
{
    WEAVE_ERROR err;
    printf("0 HandleConnectionComplete entering\n");

    char ipAddrStr[64];
    con->PeerAddr.ToString(ipAddrStr, sizeof(ipAddrStr));

    if (conErr != WEAVE_NO_ERROR)
    {
        printf("  1 Connection FAILED to node %" PRIX64 " (%s): %s\n", con->PeerNodeId, ipAddrStr, ErrorStr(conErr));
        con->Close();
        Con = NULL;
        return;
    }

    printf("  2 Connection established to node %" PRIX64 " (%s)\n", con->PeerNodeId, ipAddrStr);

    if (Con != NULL)
    {
        printf("  3 Sending TCP Image Announce\n");
        err = MockSWUServer.SendImageAnnounce(Con);
        if (err != WEAVE_NO_ERROR)
        {
            printf("  4 Software Update Server::SendImageAnnounce failed: %s\n", ErrorStr(err));
            return;
        }
    }
    else
    {
        char buffer[64];
        printf("  5 (destIPAddr: %s (printed into a string))\n", gDestIPAddr.ToString(buffer, strlen(buffer)));
        err = MockSWUServer.SendImageAnnounce(gDestNodeId, gDestIPAddr);
        if (err != WEAVE_NO_ERROR)
        {
            printf("  6 Software Update Server::SendImageAnnounce failed: %s\n", ErrorStr(err));
            return;
        }
    }

    printf("7 HandleConnectionComplete exiting\n");
}

void HandleConnectionReceived(WeaveMessageLayer *msgLayer, WeaveConnection *con)
{
    char ipAddrStr[64];
    con->PeerAddr.ToString(ipAddrStr, sizeof(ipAddrStr));

    printf("Connection received from node %" PRIX64 " (%s)\n", con->PeerNodeId, ipAddrStr);

    con->OnConnectionClosed = HandleConnectionClosed;
}

void HandleSecureSessionEstablished(WeaveSecurityManager *sm, WeaveConnection *con, void *reqState, uint16_t sessionKeyId, uint64_t peerNodeId, uint8_t encType)
{
    char ipAddrStr[64];

    if (con)
        con->PeerAddr.ToString(ipAddrStr, sizeof(ipAddrStr));
    else
        gDestIPAddr.ToString(ipAddrStr, sizeof(ipAddrStr));

    printf("Secure session established with node %" PRIX64 " (%s)\n", peerNodeId, ipAddrStr);
}

void HandleSecureSessionError(WeaveSecurityManager *sm, WeaveConnection *con, void *reqState, WEAVE_ERROR localErr, uint64_t peerNodeId, StatusReport *statusReport)
{
    char ipAddrStr[64];

    if (con)
    {
        con->PeerAddr.ToString(ipAddrStr, sizeof(ipAddrStr));
        con->Close();
    }
    else
        gDestIPAddr.ToString(ipAddrStr, sizeof(ipAddrStr));

    if (localErr == WEAVE_ERROR_STATUS_REPORT_RECEIVED && statusReport != NULL)
        printf("FAILED to establish secure session with node %" PRIX64 " (%s): %s\n", peerNodeId, ipAddrStr, nl::StatusReportStr(statusReport->mProfileId, statusReport->mStatusCode));
    else
        printf("FAILED to establish secure session with node %" PRIX64 " (%s): %s\n", peerNodeId, ipAddrStr, ErrorStr(localErr));
}

void HandleConnectionClosed(WeaveConnection *con, WEAVE_ERROR conErr)
{
    char ipAddrStr[64];
    con->PeerAddr.ToString(ipAddrStr, sizeof(ipAddrStr));

    if (conErr == WEAVE_NO_ERROR)
        printf("Connection closed to node %" PRIX64 " (%s)\n", con->PeerNodeId, ipAddrStr);
    else
        printf("Connection ABORTED to node %" PRIX64 " (%s): %s\n", con->PeerNodeId, ipAddrStr, ErrorStr(conErr));

    con->Close();
}

bool ParseStringToUint8_List(char const *aString, char const *aDelim, uint8_t *aList, uint8_t *aListSize)
{
    char *token = NULL;
    int index = 0;

    if (aDelim == NULL || aString == NULL || aListSize == 0 || aList == NULL)
        return false;

    char *aDelimitedList = (char *) malloc (strlen(aString) + 1);
    strcpy(aDelimitedList, aString);

    token = strtok(aDelimitedList, aDelim);
    while (token != NULL) {
        aList[index] = strtoul(token, NULL, 10);
        token = strtok(NULL, aDelim);
        index++;
    }

    *aListSize = index;

    free(aDelimitedList);

    return true;
}
