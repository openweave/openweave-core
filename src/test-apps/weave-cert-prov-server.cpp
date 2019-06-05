/*
 *
 *    Copyright (c) 2019 Google LLC.
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
 *      This file implements a command line tool, weave-cert-prov-server, for the
 *      Weave Certificate Provisioning Protocol (Security Profile).
 *
 *      The weave-cert-prov-server tool implements a facility for acting as a CA server
 *      (responder) for the certificate provisioning request, with a variety of options.
 *
 */

#define __STDC_FORMAT_MACROS
#define __STDC_LIMIT_MACROS

#include <inttypes.h>
#include <limits.h>
#include <signal.h>

#include "ToolCommon.h"
#include "CertProvOptions.h"
#include "MockCAService.h"
#include <Weave/WeaveVersion.h>
#include <Weave/Core/WeaveSecurityMgr.h>
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Profiles/security/WeaveSecurity.h>
#include <Weave/Profiles/security/WeaveCertProvisioning.h>
#include <Weave/Profiles/service-directory/ServiceDirectory.h>
#include <Weave/Support/crypto/WeaveCrypto.h>
#include <Weave/Support/TimeUtils.h>
#include <Weave/Support/WeaveFaultInjection.h>

using nl::StatusReportStr;
using namespace nl::Weave::TLV;
using namespace nl::Weave::Profiles::Security;
using namespace nl::Weave::Profiles::Security::CertProvisioning;

#define TOOL_NAME "weave-cert-prov-server"

static bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg);
static void HandleConnectionReceived(WeaveMessageLayer *msgLayer, WeaveConnection *con);

MockCAService  CertProvServer;

enum NameResolutionStateEnum
{
    kNameResolutionState_NotStarted,
    kNameResolutionState_InProgress,
    kNameResolutionState_Complete
} NameResolutionState = kNameResolutionState_NotStarted;

static OptionDef gToolOptionDefs[] =
{
    { "ca-cert",           kArgumentRequired, 'c' },
    { "ca-key",            kArgumentRequired, 'k' },
    { "send-ca-cert",      kNoArgument,       's' },
    { "do-not-rotate",     kNoArgument,       'r' },
    { NULL }
};

static const char *const gToolOptionHelp =
    "  -c, --ca-cert <cert-file>\n"
    "       File containing device operational CA certificate to be included along with the node's\n"
    "       operational certificate in the Get Certificat Response message. The file can contain\n"
    "       either raw TLV or base-64. If not specified the default test CA certificate is used.\n"
    "\n"
    "  -k, --ca-key <key-file>\n"
    "       File containing device operaional CA private key to be used to sign all leaf (node's)\n"
    "       operational certificates. The file can contain either raw TLV orbase-64. If not\n"
    "       specified the default test CA key is used.\n"
    "\n"
    "  -s, --send-ca-cert\n"
    "       Include device operational CA certificate in the Get Certificat Response message.\n"
    "       This option is set automatically when ca-cert is specified.\n"
    "\n"
    "  -r, --do-not-rotate\n"
    "       Do not issue new certificate to the Rotate Device Operational Certificate Request.\n"
    "       By default the GetCertificateResponse will be sent to this request.\n"
    "\n"
    ;

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
    WEAVE_VERSION_STRING "\n" WEAVE_TOOL_COPYRIGHT,
    "Receive and process get certificate request and send get certificate response messages.\n"
);

static OptionSet *gToolOptionSets[] =
{
    &gToolOptions,
    &gNetworkOptions,
    &gWeaveNodeOptions,
    &gWRMPOptions,
    &gDeviceDescOptions,
    &gHelpOptions,
    NULL
};

int main(int argc, char *argv[])
{
    WEAVE_ERROR err;

    {
        unsigned int seed;
        err = nl::Weave::Platform::Security::GetSecureRandomData((uint8_t *)&seed, sizeof(seed));
        FAIL_ERROR(err, "Random number generator seeding failed");
        srand(seed);
    }

    if (argc == 1)
    {
        gHelpOptions.PrintBriefUsage(stderr);
        exit(EXIT_FAILURE);
    }

    if (!ParseArgsFromEnvVar(TOOL_NAME, TOOL_OPTIONS_ENV_VAR_NAME, gToolOptionSets, NULL, true) ||
        !ParseArgs(TOOL_NAME, argc, argv, gToolOptionSets) ||
        !ResolveWeaveNetworkOptions(TOOL_NAME, gWeaveNodeOptions, gNetworkOptions))
    {
        exit(EXIT_FAILURE);
    }

    InitSystemLayer();
    InitNetwork();
    InitWeaveStack(true, true);
    MessageLayer.RefreshEndpoints();

    // Initialize the CertProvServer object.
    err = CertProvServer.Init(&ExchangeMgr);
    FAIL_ERROR(err, "MockCAService.Init failed");

    // Arrange to get called for various activities in the message layer.
    MessageLayer.OnConnectionReceived = HandleConnectionReceived;
    MessageLayer.OnReceiveError = HandleMessageReceiveError;
    MessageLayer.OnAcceptError = HandleAcceptConnectionError;

    PrintNodeConfig();

    while (!Done)
    {
        struct timeval sleepTime;
        sleepTime.tv_sec = 0;
        sleepTime.tv_usec = 100000;

        ServiceNetwork(sleepTime);
        fflush(stdout);
    }

    if (gSigusr1Received)
    {
        printf("Sigusr1Received\n");
        fflush(stdout);
    }

    CertProvServer.Shutdown();

    ShutdownWeaveStack();
    ShutdownNetwork();
    ShutdownSystemLayer();

    return EXIT_SUCCESS;
}

bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg)
{
    switch (id)
    {
    case 'c':
    {
        uint8_t * cert;
        uint16_t certLen;

        if (!ReadCertFile(arg, cert, certLen))
            return false;

        CertProvServer.SetCACert(cert, certLen);

        CertProvServer.IncludeRelatedCerts(true);

        break;
    }

    case 'k':
    {
        uint8_t * key;
        uint16_t keyLen;

        if (!ReadPrivateKeyFile(arg, key, keyLen))
            return false;

        CertProvServer.SetCAPrivateKey(key, keyLen);

        break;
    }

    case 's':
        CertProvServer.IncludeRelatedCerts(true);
        break;

    case 'r':
        CertProvServer.DoNotRotateCert(true);
        break;

    default:
        PrintArgError("%s: INTERNAL ERROR: Unhandled option: %s\n", progName, name);
        return false;
    }

    return true;
}

void HandleConnectionReceived(WeaveMessageLayer *msgLayer, WeaveConnection *con)
{
    char ipAddrStr[64];
    con->PeerAddr.ToString(ipAddrStr, sizeof(ipAddrStr));

    printf("Connection received from node %" PRIX64 " (%s)\n", con->PeerNodeId, ipAddrStr);
}
