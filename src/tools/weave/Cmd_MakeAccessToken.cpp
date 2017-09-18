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
 *      that generates a Weave access token.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <unistd.h>

#include "weave-tool.h"
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Profiles/security/WeaveSecurity.h>

using namespace ::nl::Weave::TLV;
using namespace ::nl::Weave::Profiles;
using namespace ::nl::Weave::Profiles::Security;

#define CMD_NAME "weave make-access-token"

static bool HandleNonOptionArgs(const char *progName, int argc, char *argv[]);
static WEAVE_ERROR EncodeAccessToken(const uint8_t *cert, uint16_t certLen, const uint8_t *privKey, uint16_t privKeyLen,
                                     uint8_t *outBuf, uint32_t& outLen);

static HelpOptions gHelpOptions(
    CMD_NAME,
    "Usage: " CMD_NAME " [ <options...> ] <cert-file-name> <priv-key-file-name>\n",
    WEAVE_VERSION_STRING "\n" COPYRIGHT_STRING,
    "Make a Weave access token object."
    "\n"
    "ARGUMENTS\n"
    "\n"
    "  <cert-file-name>\n"
    "\n"
    "       File containing the Weave certificate to be included in the access token.\n"
    "\n"
    "  <priv-key-file-name>\n"
    "\n"
    "       File containing the Weave private key to be included in the access token.\n"
    "\n"
);

static OptionSet *gCmdOptionSets[] =
{
    &gHelpOptions,
    NULL
};

static const char *gCertFileName = NULL;
static const char *gPrivKeyFileName = NULL;

bool Cmd_MakeAccessToken(int argc, char *argv[])
{
    bool res = true;
    WEAVE_ERROR err;
    uint8_t *certBuf = NULL;
    uint32_t certLen;
    uint8_t *privKeyBuf = NULL;
    uint32_t privKeyLen;
    uint8_t *accessTokenBuf = NULL;
    uint32_t accessTokenLen;
    uint8_t *b64AccessTokenBuf = NULL;
    uint32_t b64AccessTokenLen;

    if (argc == 1)
    {
        gHelpOptions.PrintBriefUsage(stderr);
        ExitNow(res = true);
    }

    if (!ParseArgs(CMD_NAME, argc, argv, gCmdOptionSets, HandleNonOptionArgs))
    {
        ExitNow(res = true);
    }

    if (!ReadWeaveCert(gCertFileName, certBuf, certLen))
        ExitNow(res = false);

    if (!ReadWeavePrivateKey(gPrivKeyFileName, privKeyBuf, privKeyLen))
        ExitNow(res = false);

    accessTokenLen = certLen + privKeyLen + 64;
    accessTokenBuf = (uint8_t *)malloc(accessTokenLen);
    if (accessTokenBuf == NULL)
    {
        fprintf(stderr, "weave: Memory allocation failed.\n");
        ExitNow(res = false);
    }

    err = EncodeAccessToken(certBuf, certLen, privKeyBuf, privKeyLen, accessTokenBuf, accessTokenLen);
    if (err != WEAVE_NO_ERROR)
    {
        fprintf(stderr, "weave: Failed to encode Weave access token: %s\n", nl::ErrorStr(err));
        ExitNow(res = false);
    }

    b64AccessTokenBuf = Base64Encode(accessTokenBuf, accessTokenLen, NULL, 0, b64AccessTokenLen);
    if (b64AccessTokenBuf == NULL)
        ExitNow(res = false);

    fwrite(b64AccessTokenBuf, 1, b64AccessTokenLen, stdout);
    printf("\n");

exit:
    if (certBuf != NULL)
        free(certBuf);
    if (privKeyBuf != NULL)
        free(privKeyBuf);
    if (accessTokenBuf != NULL)
        free(accessTokenBuf);
    if (b64AccessTokenBuf != NULL)
        free(b64AccessTokenBuf);
    return res;
}

static bool HandleNonOptionArgs(const char *progName, int argc, char *argv[])
{
    if (argc == 0)
    {
        PrintArgError("%s: Please specify the certificate file name.\n", progName);
        return false;
    }

    if (argc == 1)
    {
        PrintArgError("%s: Please specify the private key file name.\n", progName);
        return false;
    }

    if (argc > 2)
    {
        PrintArgError("%s: Unexpected argument: %s\n", progName, argv[2]);
        return false;
    }

    gCertFileName = argv[0];
    gPrivKeyFileName = argv[1];

    return true;
}

static WEAVE_ERROR EncodeAccessToken(const uint8_t *cert, uint16_t certLen, const uint8_t *privKey, uint16_t privKeyLen,
                                     uint8_t *outBuf, uint32_t& outLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TLVWriter writer;

    writer.Init(outBuf, outLen);

    {
        TLVType containerType;

        err = writer.StartContainer(ProfileTag(kWeaveProfile_Security, kTag_WeaveAccessToken), kTLVType_Structure, containerType);
        SuccessOrExit(err);

        {
            TLVReader reader;

            reader.Init(cert, certLen);

            err = reader.Next();
            SuccessOrExit(err);

            err = writer.CopyContainer(ContextTag(kTag_AccessToken_Certificate), reader);
            SuccessOrExit(err);
        }

        {
            TLVReader reader;

            reader.Init(privKey, privKeyLen);

            err = reader.Next();
            SuccessOrExit(err);

            err = writer.CopyContainer(ContextTag(kTag_AccessToken_PrivateKey), reader);
            SuccessOrExit(err);
        }

        err = writer.EndContainer(containerType);
        SuccessOrExit(err);
    }

    err = writer.Finalize();
    SuccessOrExit(err);

    outLen = writer.GetLengthWritten();

exit:
    return err;
}
