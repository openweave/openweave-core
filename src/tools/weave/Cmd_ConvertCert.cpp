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
 *      that converts a Weave certificate between TLV and X.509
 *      formats.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <unistd.h>

#include "weave-tool.h"

using namespace nl::Weave::Profiles::Security;

#define CMD_NAME "weave convert-cert"

static bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg);
static bool HandleNonOptionArgs(const char *progName, int argc, char *argv[]);

static OptionDef gCmdOptionDefs[] =
{
    { "x509",       kNoArgument, 'p' },
    { "x509-pem",   kNoArgument, 'p' }, // alias for --x509
    { "x509-der",   kNoArgument, 'x' },
    { "weave",      kNoArgument, 'w' },
    { "weave-b64",  kNoArgument, 'b' },
    { NULL }
};

static const char *const gCmdOptionHelp =
    "  -p, --x509, --x509-pem\n"
    "\n"
    "       Output an X.509 certificate in PEM format.\n"
    "\n"
    "  -x, --x509-der\n"
    "\n"
    "       Output an X.509 certificate in DER format.\n"
    "\n"
    "  -w, --weave\n"
    "\n"
    "       Output a Weave certificate in raw TLV format.\n"
    "\n"
    "  -b --weave-b64\n"
    "\n"
    "       Output a Weave certificate in base-64 format.\n"
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
    "Usage: " CMD_NAME " [ <options...> ] <in-file> <out-file>\n",
    WEAVE_VERSION_STRING "\n" COPYRIGHT_STRING,
    "Convert a certificate between Weave and X509 forms.\n"
    "\n"
    "ARGUMENTS\n"
    "\n"
    "  <in-file>\n"
    "\n"
    "       The input certificate file name, or - to read from stdin. The\n"
    "       format of the input certificate is auto-detected and can be any\n"
    "       of: X.509 PEM, X.509 DER, Weave base-64 or Weave raw TLV.\n"
    "\n"
    "  <out-file>\n"
    "\n"
    "       The output certificate file name, or - to write to stdout.\n"
    "\n"
);

static OptionSet *gCmdOptionSets[] =
{
    &gCmdOptions,
    &gHelpOptions,
    NULL
};

static const char *gInFileName = NULL;
static const char *gOutFileName = NULL;
static CertFormat gOutCertFormat = kCertFormat_Weave_Base64;

bool Cmd_ConvertCert(int argc, char *argv[])
{
    static uint8_t inCert[MAX_CERT_SIZE];
    static uint8_t outCert[MAX_CERT_SIZE];

    bool res = true;
    WEAVE_ERROR err;
    FILE *inFile = NULL;
    FILE *outFile = NULL;
    uint32_t inCertLen = 0;
    CertFormat inCertFormat;
    uint32_t outCertLen = 0;
    bool outFileCreated = false;

    if (argc == 1)
    {
        gHelpOptions.PrintBriefUsage(stderr);
        ExitNow(res = true);
    }

    if (!ParseArgs(CMD_NAME, argc, argv, gCmdOptionSets, HandleNonOptionArgs))
    {
        ExitNow(res = true);
    }

    if (strcmp(gInFileName, "-") != 0)
    {
        inFile = fopen(gInFileName, "rb");
        if (inFile == NULL)
        {
            fprintf(stderr, "weave: Unable to open %s\n%s\n", gInFileName, strerror(errno));
            ExitNow(res = false);
        }
    }
    else
        inFile = stdin;

    inCertLen = fread(inCert, 1, sizeof(inCert), inFile);
    if (ferror(inFile))
    {
        fprintf(stderr, "weave: Error reading %s\n%s\n", gInFileName, strerror(errno));
        ExitNow(res = false);
    }

    if (inCertLen == sizeof(inCert))
    {
        fprintf(stderr, "weave: Input certificate too big\n");
        ExitNow(res = false);
    }

    if (!InitOpenSSL())
        ExitNow(res = false);

    inCertFormat = DetectCertFormat(inCert, inCertLen);

    if (inCertFormat == gOutCertFormat)
    {
        memcpy(outCert, inCert, inCertLen);
        outCertLen = inCertLen;
    }

    else
    {
        if (inCertFormat == kCertFormat_X509_PEM)
        {
            if (!X509PEMToDER(inCert, inCertLen))
                ExitNow(res = false);
            inCertFormat = kCertFormat_X509_DER;
        }
        else if (inCertFormat == kCertFormat_Weave_Base64)
        {
            if (!Base64Decode(inCert, inCertLen, inCert, sizeof(inCert), inCertLen))
                ExitNow(res = false);
            inCertFormat = kCertFormat_Weave_Raw;
        }

        if (inCertFormat == kCertFormat_X509_DER && (gOutCertFormat == kCertFormat_Weave_Raw || gOutCertFormat == kCertFormat_Weave_Base64))
        {
            err = ConvertX509CertToWeaveCert(inCert, inCertLen, outCert, sizeof(outCert), outCertLen);
            if (err != WEAVE_NO_ERROR)
            {
                fprintf(stderr, "weave: Error converting certificate: %s\n", nl::ErrorStr(err));
                ExitNow(res = false);
            }
        }
        else if (inCertFormat == kCertFormat_Weave_Raw && (gOutCertFormat == kCertFormat_X509_DER || gOutCertFormat == kCertFormat_X509_PEM))
        {
            err = ConvertWeaveCertToX509Cert(inCert, inCertLen, outCert, sizeof(outCert), outCertLen);
            if (err != WEAVE_NO_ERROR)
            {
                fprintf(stderr, "weave: Error converting certificate: %s\n", nl::ErrorStr(err));
                ExitNow(res = false);
            }
        }
        else
        {
            memcpy(outCert, inCert, inCertLen);
            outCertLen = inCertLen;
        }

        if (gOutCertFormat == kCertFormat_X509_PEM)
        {
            if (!X509DERToPEM(outCert, outCertLen, sizeof(outCert)))
                ExitNow(res = false);
        }
        else if (gOutCertFormat == kCertFormat_Weave_Base64)
        {
            if (!Base64Encode(outCert, outCertLen, outCert, sizeof(outCert), outCertLen))
                ExitNow(res = false);
        }
    }

    if (strcmp(gOutFileName, "-") != 0)
    {
        outFile = fopen(gOutFileName, "w+b");
        if (outFile == NULL)
        {
            fprintf(stderr, "weave: ERROR: Unable to create %s\n%s\n", gOutFileName, strerror(errno));
            ExitNow(res = false);
        }
        outFileCreated = true;
    }
    else
        outFile = stdout;

    if (fwrite(outCert, 1, outCertLen, outFile) != outCertLen)
    {
        fprintf(stderr, "weave: ERROR: Unable to write to %s\n%s\n", gOutFileName, strerror(ferror(outFile) ? errno : ENOSPC));
        ExitNow(res = false);
    }

exit:
    if (inFile != NULL && inFile != stdin)
        fclose(inFile);
    if (outFile != NULL && outFile != stdout)
        fclose(outFile);
    if (gOutFileName != NULL && outFileCreated && !res)
        unlink(gOutFileName);
    return res;
}

bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg)
{
    switch (id)
    {
    case 'p':
        gOutCertFormat = kCertFormat_X509_PEM;
        break;
    case 'x':
        gOutCertFormat = kCertFormat_X509_DER;
        break;
    case 'b':
        gOutCertFormat = kCertFormat_Weave_Base64;
        break;
    case 'w':
        gOutCertFormat = kCertFormat_Weave_Raw;
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
        PrintArgError("%s: Please specify the name of the input certificate file, or - for stdin.\n", progName);
        return false;
    }

    if (argc == 1)
    {
        PrintArgError("%s: Please specify the name of the output certificate file, or - for stdout\n", progName);
        return false;
    }

    if (argc > 2)
    {
        PrintArgError("%s: Unexpected argument: %s\n", progName, argv[2]);
        return false;
    }

    gInFileName = argv[0];
    gOutFileName = argv[1];

    return true;
}
