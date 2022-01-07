/*
 *
 *    Copyright (c) 2022 Nest Labs, Inc.
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
 *      that decodes and prints the contents of a Weave access token certificate.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <unistd.h>

#include "weave-tool.h"
#include <Weave/Support/ASN1OID.h>
#include <Weave/Profiles/security/WeaveSecurityDebug.h>
#include <Weave/Profiles/security/WeaveAccessToken.h>
#include <Weave/Support/Base64.h>

using namespace nl::Weave::ASN1;
using namespace nl::Weave::Profiles::Security;

#define CMD_NAME "weave print-cert"

static bool HandleNonOptionArgs(const char *progName, int argc, char *argv[]);
static bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg);

static OptionDef gCmdOptionDefs[] =
{
    { "base64", kNoArgument, 'b' },
    { }
};

static const char *const gCmdOptionHelp =
    "   -b, --base64\n"
    "\n"
    "       The file containing the TLV should be parsed as base64.\n"
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
    "Usage: " CMD_NAME " [<options...>] <cert-file>\n",
    WEAVE_VERSION_STRING "\n" COPYRIGHT_STRING,
    "Print a Weave Access Token certificate.\n"
    "\n"
    "ARGUMENTS\n"
    "\n"
    "  <access-token>\n"
    "\n"
    "       A file containing a Weave Access Token, in base-64 format\n"
    "\n"
);

static OptionSet *gCmdOptionSets[] =
{
    &gCmdOptions,
    &gHelpOptions,
    NULL
};

static const char *gCertFileName = NULL;
static bool gUseBase64Decoding = false;
enum {
    kNumCerts = 3,
    kCertBufSize = 1024,
};

bool Cmd_PrintAccessToken(int argc, char *argv[])
{
    bool res = true;
    WEAVE_ERROR err;
    WeaveCertificateData *certData;
    WeaveCertificateSet certSet;
    uint8_t * accessToken = NULL;
    uint32_t accessTokenLen = 0;
    TLVReader reader;

    if (argc == 1)
    {
        gHelpOptions.PrintBriefUsage(stderr);
        ExitNow(res = true);
    }

    if (!ParseArgs(CMD_NAME, argc, argv, gCmdOptionSets, HandleNonOptionArgs))
    {
        ExitNow(res = false);
    }

    if (!ReadFileIntoMem(gCertFileName, accessToken, accessTokenLen))
        ExitNow(res = false);

    if (gUseBase64Decoding)
    {
        uint32_t b64len = accessTokenLen;
        uint8_t * b64 = accessToken;
        accessToken = (uint8_t *)malloc(accessTokenLen);
        accessTokenLen = nl::Base64Decode((const char *)b64, b64len, accessToken);
        free(b64);
    }

    reader.Init(accessToken, accessTokenLen);
    certSet.Init(kNumCerts, kCertBufSize);
    err = LoadAccessTokenCerts(reader, certSet, 0, certData);

    if (err != WEAVE_NO_ERROR)
    {
        fprintf(stderr, "Error reading cert info: %s.\n", nl::ErrorStr(err));
        ExitNow(res = false);
    }

    err = DetermineCertType(*certData);
    if (err != WEAVE_NO_ERROR)
    {
        fprintf(stderr, "weave: %s.\n", nl::ErrorStr(err));
        ExitNow(res = false);
    }

    printf("Weave Access Token:\n");

    printf("Weave Certificate:\n");
    PrintCert(stdout, *certData, NULL, 2, true);

    printf("Access Token Private Key omitted\n");

    res = (err == WEAVE_NO_ERROR);

exit:
    if (accessToken != NULL)
        free(accessToken);
    return res;
}

bool HandleNonOptionArgs(const char *progName, int argc, char *argv[])
{
    if (argc == 0)
    {
        PrintArgError("%s: Please specify the name of the certificate to be printed.\n", progName);
        return false;
    }

    if (argc > 1)
    {
        PrintArgError("%s: Unexpected argument: %s\n", progName, argv[1]);
        return false;
    }

    gCertFileName = argv[0];

    return true;
}

bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg)
{
    switch (id)
    {
    case 'b':
        gUseBase64Decoding = true;
        break;

    default:
        PrintArgError("%s: INTERNAL ERROR: Unhandled option: %s\n", progName, name);
        return false;
    }

    return true;
}
