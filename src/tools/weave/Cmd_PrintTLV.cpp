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
 *      This file implements the weave command of print-tlv.
 *
 */
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <Weave/Core/WeaveTLVDebug.hpp>
#include <Weave/Support/Base64.h>

#include "weave-tool.h"

using namespace nl::Weave::TLV;

#define CMD_NAME "weave print-tlv"

static bool HandleNonOptionArgs(const char *progName, int argc, char *argv[]);
static bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg);
static void _DumpWriter(const char *aFormat, ...);

static OptionDef gCmdOptionDefs[] =
{
    { "base64", kNoArgument, 'b' },
    { NULL }
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
    "Usage: " CMD_NAME " [<options...>] <tlv-file>\n",
    WEAVE_VERSION_STRING "\n" COPYRIGHT_STRING,
    "Print a Weave TLV encoding in human readable form.\n"
    "\n"
    "ARGUMENTS\n"
    "\n"
    "  <tlv-file>\n"
    "\n"
    "       A file containing an encoded Weave TLV element. The certificate\n"
    "       must be in raw TLV format or base-64 with -b option.\n"
    "\n"
);

static OptionSet *gCmdOptionSets[] =
{
    &gCmdOptions,
    &gHelpOptions,
    NULL
};

static const char *gFileName = NULL;
static bool gUseBase64Decoding = false;

bool Cmd_PrintTLV(int argc, char *argv[])
{
    bool res = true;
    uint8_t *map = NULL;
    uint8_t *raw = NULL;
    int fd;
    struct stat st;
    TLVReader reader;
    uint32_t len;

    if (argc == 1)
    {
        gHelpOptions.PrintBriefUsage(stderr);
        ExitNow(res = true);
    }

    if (!ParseArgs(CMD_NAME, argc, argv, gCmdOptionSets, HandleNonOptionArgs))
    {
        ExitNow(res = true);
    }

    fd = open(gFileName, O_RDONLY | O_CLOEXEC);
    if (fd < 0)
    {
        fprintf(stderr, "weave: Error reading %s\n", gFileName);
        ExitNow(res = false);
    }

    if (fstat(fd, &st) < 0)
    {
        close(fd);
        ExitNow(res = false);
    }

    map = static_cast<uint8_t *>(mmap(0, st.st_size, PROT_READ, MAP_SHARED, fd, 0));
    if (!map || map == MAP_FAILED)
    {
        close(fd);
        ExitNow(res = false);
    }

    if (gUseBase64Decoding)
    {
        raw = (uint8_t *)malloc(st.st_size);
        len = nl::Base64Decode((const char *)map, st.st_size, raw);
    }
    else
    {
        len = st.st_size;
        raw = map;
    }

    printf("TLV length is %d bytes\n", len);
    reader.Init(raw, len);
    nl::Weave::TLV::Debug::Dump(reader, _DumpWriter);

exit:
    if (raw != NULL && raw != map)
    {
        free(raw);
    }

    if (map != NULL)
    {
        munmap(map, st.st_size);
    }

    return res;
}

static bool HandleNonOptionArgs(const char *progName, int argc, char *argv[])
{
    if (argc == 0)
    {
        PrintArgError("%s: Please specify the name of a file to be printed.\n", progName);
        return false;
    }

    if (argc > 1)
    {
        PrintArgError("%s: Unexpected argument: %s\n", progName, argv[1]);
        return false;
    }

    gFileName = argv[0];

    return true;
}

static void _DumpWriter(const char *aFormat, ...)
{
    va_list args;

    va_start(args, aFormat);

    vprintf(aFormat, args);

    va_end(args);
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
