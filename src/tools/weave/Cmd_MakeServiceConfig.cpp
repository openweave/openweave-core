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
 *      This file implements the command handler for the 'weave' tool
 *      that generates Weave service configuration.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <unistd.h>

#include "weave-tool.h"
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Profiles/service-provisioning/ServiceProvisioning.h>
#include <Weave/Support/NestCerts.h>

using namespace ::nl::Weave::Profiles::Security;
using namespace ::nl::Weave::Profiles::ServiceProvisioning;
using namespace ::nl::NestCerts;

#define CMD_NAME "weave make-service-config"

static bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg);
static bool HandleNonOptionArgs(const char *progName, int argc, char *argv[]);

static OptionDef gCmdOptionDefs[] =
{
    { "cert",      kArgumentRequired, 'c' },
    { "prod-root", kNoArgument,       'p' },
    { "dev-root",  kNoArgument,       'd' },
    { NULL }
};

static const char *const gCmdOptionHelp =
    "  -c, --cert <file-name>\n"
    "\n"
    "      File containing Weave certificate to be included in the list of trusted\n"
    "      certificates.\n"
    "\n"
    "  -p, --prod-root\n"
    "\n"
    "      Include the Nest production root certificate in the list of trusted\n"
    "      certificates.\n"
    "\n"
    "  -d, --dev-root\n"
    "\n"
    "      Include the Nest development root certificate in the list of trusted\n"
    "      certificates.\n"
    "\n"
    ;

static OptionSet gCmdOptions =
{
    HandleOption,
    gCmdOptionDefs,
    "COMMAND OPTIONS",
    gCmdOptionHelp
};

static HelpOptions gHelpOptions(
    CMD_NAME,
    "Usage: " CMD_NAME " [<options...>] <dir-host-name> [<dir-port>]\n",
    WEAVE_VERSION_STRING "\n" COPYRIGHT_STRING,
    "Make a Weave service config object.\n"
    "\n"
    "ARGUMENTS\n"
    "\n"
    "  <dir-host-name>\n"
    "\n"
    "       Service directory hostname.\n"
    "\n"
    "  <dir-port>\n"
    "\n"
    "       Service directory port. Defaults to the Weave port.\n"
    "\n"
);

static OptionSet *gCmdOptionSets[] =
{
    &gCmdOptions,
    &gHelpOptions,
    NULL
};

enum { kMaxCerts = 64 };

static const char *gCertFileNames[kMaxCerts];
static size_t gNumCertFileNames = 0;
static const char *gDirHostName = NULL;
static int32_t gDirPort = WEAVE_PORT;
static bool gIncludeProdRootCert = false;
static bool gIncludeDevRootCert = false;

bool Cmd_MakeServiceConfig(int argc, char *argv[])
{
    bool res = true;
    WEAVE_ERROR err;
    WeaveCertificateSet certSet;
    uint8_t *certBufs[kMaxCerts];
    uint8_t *serviceConfigBuf = NULL;
    uint16_t serviceConfigLen = 63353;
    uint8_t *b64ServiceConfigBuf = NULL;
    uint32_t b64ServiceConfigLen;

    memset(certBufs, 0, sizeof(certBufs));

    if (argc == 1)
    {
        gHelpOptions.PrintBriefUsage(stderr);
        ExitNow(res = true);
    }

    if (!ParseArgs(CMD_NAME, argc, argv, gCmdOptionSets, HandleNonOptionArgs))
    {
        ExitNow(res = true);
    }

    err = certSet.Init(kMaxCerts, 2048);
    if (err != WEAVE_NO_ERROR)
    {
        fprintf(stderr, "weave: %s.\n", nl::ErrorStr(err));
        ExitNow(res = false);
    }

    if (!gIncludeProdRootCert && !gIncludeDevRootCert && gNumCertFileNames == 0)
    {
        fprintf(stderr, "weave: Please specify one or more trusted certificates\n");
        ExitNow(res = false);
    }

    if (gIncludeProdRootCert)
    {
        WeaveCertificateData *cert;
        err = certSet.LoadCert(nl::NestCerts::Production::Root::Cert, nl::NestCerts::Production::Root::CertLength, 0, cert);
        if (err != WEAVE_NO_ERROR)
        {
            fprintf(stderr, "weave: Error reading production root certificate: %s\n", nl::ErrorStr(err));
            ExitNow(res = false);
        }
    }

    if (gIncludeDevRootCert)
    {
        WeaveCertificateData *cert;
        err = certSet.LoadCert(nl::NestCerts::Development::Root::Cert, nl::NestCerts::Development::Root::CertLength, 0, cert);
        if (err != WEAVE_NO_ERROR)
        {
            fprintf(stderr, "weave: Error reading development root certificate: %s\n", nl::ErrorStr(err));
            ExitNow(res = false);
        }
    }

    for (size_t i = 0; i < gNumCertFileNames; i++)
    {
        if (!LoadWeaveCert(gCertFileNames[i], false, certSet, certBufs[certSet.CertCount]))
            ExitNow(res = false);
    }

    serviceConfigBuf = (uint8_t *)malloc(serviceConfigLen);
    if (serviceConfigBuf == NULL)
    {
        fprintf(stderr, "weave: Memory allocation failed\n");
        ExitNow(res = false);
    }

    err = EncodeServiceConfig(certSet, gDirHostName, (uint16_t)gDirPort, serviceConfigBuf, serviceConfigLen);
    if (err != WEAVE_NO_ERROR)
    {
        fprintf(stderr, "weave: Error encoding service config: %s\n", nl::ErrorStr(err));
        ExitNow(res = false);
    }

    b64ServiceConfigBuf = Base64Encode(serviceConfigBuf, serviceConfigLen, NULL, 0, b64ServiceConfigLen);
    if (b64ServiceConfigBuf == NULL)
        ExitNow(res = false);

    fwrite(b64ServiceConfigBuf, 1, b64ServiceConfigLen, stdout);
    printf("\n");

exit:
    certSet.Release();
    for (int i = 0; i < kMaxCerts; i++)
        if (certBufs[i] != NULL)
            free(certBufs[i]);
    if (serviceConfigBuf != NULL)
        free(serviceConfigBuf);
    if (b64ServiceConfigBuf != NULL)
        free(b64ServiceConfigBuf);
    return res;
}

bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg)
{
    switch (id)
    {
    case 'c':
        gCertFileNames[gNumCertFileNames++] = arg;
        break;
    case 'p':
        gIncludeProdRootCert = true;
        break;
    case 'd':
        gIncludeDevRootCert = true;
        break;
    default:
        PrintArgError("%s: INTERNAL ERROR: Unhandled option: %s\n", progName, name);
        return false;
    }

    return true;
}

bool HandleNonOptionArgs(const char *progName, int argc, char *argv[])
{
    if (argc == 0)
    {
        PrintArgError("%s: Please specify the service directory host name.\n", progName);
        return false;
    }

    if (argc > 2)
    {
        PrintArgError("%s: Unexpected argument: %s\n", progName, argv[2]);
        return false;
    }

    gDirHostName = argv[0];

    if (argc == 2)
    {
        if (!ParseInt(argv[1], gDirPort) || gDirPort < 1 || gDirPort > 65535)
        {
            PrintArgError("%s: Invalid value specified for service directory port: %s\n", progName, argv[1]);
            return false;
        }

    }

    return true;
}
