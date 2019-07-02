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
 *      This file implements the command handler for the 'weave' tool
 *      that decodes and prints the contents of a Weave certificate.
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

using namespace nl::Weave::ASN1;
using namespace nl::Weave::Profiles::Security;

#define CMD_NAME "weave print-cert"

static bool HandleNonOptionArgs(const char *progName, int argc, char *argv[]);

static HelpOptions gHelpOptions(
    CMD_NAME,
    "Usage: " CMD_NAME " [<options...>] <cert-file>\n",
    WEAVE_VERSION_STRING "\n" COPYRIGHT_STRING,
    "Print a Weave certificate.\n"
    "\n"
    "ARGUMENTS\n"
    "\n"
    "  <cert-file>\n"
    "\n"
    "       A file containing a Weave certificate. The certificate must be in\n"
    "       base-64 or raw TLV format.\n"
    "\n"
);

static OptionSet *gCmdOptionSets[] =
{
    &gHelpOptions,
    NULL
};

static const char *gCertFileName = NULL;

bool Cmd_PrintCert(int argc, char *argv[])
{
    bool res = true;
    WEAVE_ERROR err;
    WeaveCertificateData certData;
    uint8_t * certBuf = NULL;
    uint32_t certLen;

    if (argc == 1)
    {
        gHelpOptions.PrintBriefUsage(stderr);
        ExitNow(res = true);
    }

    if (!ParseArgs(CMD_NAME, argc, argv, gCmdOptionSets, HandleNonOptionArgs))
    {
        ExitNow(res = false);
    }

    if (!ReadWeaveCert(gCertFileName, certBuf, certLen))
        ExitNow(res = false);

    err = DecodeWeaveCert(certBuf, certLen, certData);
    if (err != WEAVE_NO_ERROR)
    {
        fprintf(stderr, "weave: %s.\n", nl::ErrorStr(err));
        ExitNow(res = false);
    }

    err = DetermineCertType(certData);
    if (err != WEAVE_NO_ERROR)
    {
        fprintf(stderr, "weave: %s.\n", nl::ErrorStr(err));
        ExitNow(res = false);
    }

    printf("Weave Certificate:\n");
    PrintCert(stdout, certData, NULL, 2, true);

    res = (err == WEAVE_NO_ERROR);

exit:
    if (certBuf != NULL)
        free(certBuf);
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
