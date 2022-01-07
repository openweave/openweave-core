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
 *      that decodes and prints the contents of a service config object.
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
#include <Weave/Profiles/service-provisioning/ServiceProvisioning.h>
#include <Weave/Profiles/service-directory/ServiceDirectory.h>
#include <Weave/Support/Base64.h>

using namespace nl::Weave::ASN1;
using namespace nl::Weave::Profiles::Security;
using namespace nl::Weave::TLV;
using namespace nl::Weave::Profiles::ServiceProvisioning;
using namespace nl::Weave::Profiles;

#define CMD_NAME "weave print-cert"

static WEAVE_ERROR PrintServiceHostname(TLVReader &reader);
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
    "Print a service config object.\n"
    "\n"
    "ARGUMENTS\n"
    "\n"
    "  <access-token>\n"
    "\n"
    "       A file containing a service config object either in binary (default) or in base-64 format\n"
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

bool Cmd_PrintServiceConfig(int argc, char *argv[])
{
    bool res = true;
    WEAVE_ERROR err;
    WeaveCertificateSet certSet;
    uint8_t * serviceConfig = NULL;
    uint32_t serviceConfigLen = 0;
    TLVReader reader;
    uint64_t serviceEndpoint;
    TLVType serviceConfigContainer, serviceEndpointContainer, serviceDirectoryContainer;

    if (argc == 1)
    {
        gHelpOptions.PrintBriefUsage(stderr);
        ExitNow(res = true);
    }

    if (!ParseArgs(CMD_NAME, argc, argv, gCmdOptionSets, HandleNonOptionArgs))
    {
        ExitNow(res = false);
    }

    if (!ReadFileIntoMem(gCertFileName, serviceConfig, serviceConfigLen))
        ExitNow(res = false);

    if (gUseBase64Decoding)
    {
        uint32_t b64len = serviceConfigLen;
        uint8_t * b64 = serviceConfig;
        serviceConfig = (uint8_t *)malloc(serviceConfigLen);
        serviceConfigLen = nl::Base64Decode((const char *)b64, b64len, serviceConfig);
        free(b64);
    }

    reader.Init(serviceConfig, serviceConfigLen);
    certSet.Init(kNumCerts, kCertBufSize);

    err = reader.Next(kTLVType_Structure, ProfileTag(kWeaveProfile_ServiceProvisioning, kTag_ServiceConfig));
    SuccessOrExit(err);

    err = reader.EnterContainer(serviceConfigContainer);
    SuccessOrExit(err);

    err = reader.Next(kTLVType_Array, ContextTag(kTag_ServiceConfig_CACerts));
    SuccessOrExit(err);

    err = certSet.LoadCerts(reader, 0);
    SuccessOrExit(err);

    printf("Weave Service Config:\n\n");
    printf("Trusted certificates:\n");
    for (int i = 0; i < certSet.CertCount; i++)
    {
        printf("Certificate %d\n", i + 1);
        PrintCert(stdout, certSet.Certs[i],NULL, 2, true);
    }

    err = reader.Next(kTLVType_Structure, ContextTag(kTag_ServiceEndPoint));
    SuccessOrExit(err);

    err = reader.EnterContainer(serviceEndpointContainer);
    SuccessOrExit(err);

    err = reader.Next(kTLVType_UnsignedInteger, ContextTag(kTag_ServiceEndPoint_Id));
    SuccessOrExit(err);

    err = reader.Get(serviceEndpoint);
    SuccessOrExit(err);

    printf("Service Endpoint ID: %016" PRIX64 "\n", serviceEndpoint);

    err = reader.Next(kTLVType_Array, ContextTag(kTag_ServiceEndPoint_Addresses));
    SuccessOrExit(err);

    err = reader.EnterContainer(serviceDirectoryContainer);
    SuccessOrExit(err);

    while (err == WEAVE_NO_ERROR)
    {
        err = PrintServiceHostname(reader);
    }
    if (err == WEAVE_END_OF_TLV)
    {
        err = WEAVE_NO_ERROR;
    }

exit:
    res = (err == WEAVE_NO_ERROR);

    if (serviceConfig != NULL)
        free(serviceConfig);
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

WEAVE_ERROR PrintServiceHostname(TLVReader &reader)
{
    WEAVE_ERROR err;
    TLVType container;
    uint32_t len;
    const uint8_t * strbuf = NULL;
    uint16_t port;
    err = reader.Next();
    SuccessOrExit(err);

    err = reader.EnterContainer(container);
    SuccessOrExit(err);

    while ((err = reader.Next()) == WEAVE_NO_ERROR)
    {
        uint64_t tag = reader.GetTag();
        uint8_t tagNum;
        if (!IsContextTag(tag))
        {
            continue;
        }

        tagNum = TagNumFromTag(tag);

        switch (tagNum)
        {
            case kTag_ServiceEndPointAddress_HostName:
                reader.GetDataPtr(strbuf);
                len = reader.GetLength();
                printf("Hostname: %-.*s ", (int)len, strbuf);
                break;
            case kTag_ServiceEndPointAddress_Port:
                reader.Get(port);
                printf("Port: %d ", port);
                break;

            default:
                printf("Unknown tag num %d", tagNum);
        }
    }
    printf("\n");
    reader.ExitContainer(container);
exit:
    return err;
}
