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
 *      that converts a Weave private key between TLV and PEM/DER
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

#define CMD_NAME "weave convert-key"

static bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg);
static bool HandleNonOptionArgs(const char *progName, int argc, char *argv[]);

static OptionDef gCmdOptionDefs[] =
{
    { "der",        kNoArgument, 'x' },
    { "pem",        kNoArgument, 'p' },
    { "weave",      kNoArgument, 'w' },
    { "weave-b64",  kNoArgument, 'b' },
    { NULL }
};

static const char *const gCmdOptionHelp =
    "  -p, --pem\n"
    "\n"
    "      Output the private key in PEM format.\n"
    "\n"
    "  -x, --der\n"
    "\n"
    "      Output the private key in DER format.\n"
    "\n"
    "  -w, --weave\n"
    "\n"
    "      Output the private key in Weave raw TLV format.\n"
    "      This is the default.\n"
    "\n"
    "  -b, --weave-b64\n"
    "\n"
    "      Output the private key in Weave base-64 format.\n"
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
    "Convert a private key between Weave and PEM/DER forms."
    "\n"
    "ARGUMENTS\n"
    "\n"
    "  <in-file>\n"
    "\n"
    "       The input private key file name, or - to read from stdin. The\n"
    "       format of the input key is auto-detected and can be any\n"
    "       of: PEM, DER, Weave base-64 or Weave raw TLV.\n"
    "\n"
    "  <out-file>\n"
    "\n"
    "       The output private key file name, or - to write to stdout.\n"
    "\n"
);

static OptionSet *gCmdOptionSets[] =
{
    &gCmdOptions,
    &gHelpOptions,
    NULL
};

const char *gInFileName;
const char *gOutFileName;
KeyFormat gOutFormat = kKeyFormat_Weave_Raw;

bool Cmd_ConvertKey(int argc, char *argv[])
{
    static uint8_t inKey[MAX_KEY_SIZE];

    bool res = true;
    FILE *inFile = NULL;
    KeyFormat inFormat;
    uint32_t inKeyLen = 0;
    FILE *outFile = NULL;
    uint8_t *outKey = NULL;
    uint32_t outKeyLen = 0;
    EVP_PKEY *key = NULL;
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

    inKeyLen = fread(inKey, 1, sizeof(inKey), inFile);
    if (ferror(inFile))
    {
        fprintf(stderr, "weave: Error reading %s\n%s\n", gInFileName, strerror(errno));
        ExitNow(res = false);
    }

    if (inKeyLen == sizeof(inKey))
    {
        fprintf(stderr, "weave: Input key too big\n");
        ExitNow(res = false);
    }

    if (!InitOpenSSL())
        ExitNow(res = false);

    inFormat = DetectKeyFormat(inKey, inKeyLen);

    if (inFormat == gOutFormat)
    {
        outKey = inKey;
        outKeyLen = inKeyLen;
    }

    else
    {
        if (!DecodePrivateKey(inKey, inKeyLen, inFormat, gInFileName, "Enter password for private key:", key))
            ExitNow(res = false);

        if (!EncodePrivateKey(key, gOutFormat, outKey, outKeyLen))
            ExitNow(res = false);
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

    if (fwrite(outKey, 1, outKeyLen, outFile) != outKeyLen || ferror(outFile))
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
    if (outKey != NULL && outKey != inKey)
        free(outKey);
    return res;
}

bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg)
{
    switch (id)
    {
    case 'p':
        gOutFormat = kKeyFormat_PEM;
        break;
    case 'x':
        gOutFormat = kKeyFormat_DER;
        break;
    case 'b':
        gOutFormat = kKeyFormat_Weave_Base64;
        break;
    case 'w':
        gOutFormat = kKeyFormat_Weave_Raw;
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
        PrintArgError("%s: Please specify the name of the input key file, or - for stdin.\n", progName);
        return false;
    }

    if (argc == 1)
    {
        PrintArgError("%s: Please specify the name of the output key file, or - for stdout\n", progName);
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
