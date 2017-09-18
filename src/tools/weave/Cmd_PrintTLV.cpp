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

#include "weave-tool.h"

using namespace nl::Weave::TLV;

#define CMD_NAME "weave print-tlv"

static bool HandleNonOptionArgs(const char *progName, int argc, char *argv[]);
static void _DumpWriter(const char *aFormat, ...);

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
    "       A file containing an encoded Weave TLV element.\n"
    "\n"
);

static OptionSet *gCmdOptionSets[] =
{
    &gHelpOptions,
    NULL
};

static const char *gFileName = NULL;

bool Cmd_PrintTLV(int argc, char *argv[])
{
    bool res = true;
    uint8_t *map = NULL;
    int fd;
    struct stat st;
    TLVReader reader;

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

    reader.Init(map, st.st_size);
    nl::Weave::TLV::Debug::Dump(reader, _DumpWriter);

exit:
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
