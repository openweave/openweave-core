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
 *      that validates a Weave certificate.
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

#define CMD_NAME "weave validate-cert"

static bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg);
static bool HandleNonOptionArgs(const char *progName, int argc, char *argv[]);

static OptionDef gCmdOptionDefs[] =
{
    { "cert",           kArgumentRequired,  'c' },
    { "trusted-cert",   kArgumentRequired,  't' },
    { "verbose",        kNoArgument,        'V' },
    { NULL }
};

static const char *const gCmdOptionHelp =
    "  -c, --cert <cert-file>\n"
    "\n"
    "       A file containing an untrusted Weave certificate to be used during\n"
    "       validation. The file must be in base-64 or TLV format.\n"
    "\n"
    "  -t, --trusted-cert <cert-file>\n"
    "\n"
    "       A file containing a trusted Weave certificate to be used during\n"
    "       validation. The file must be in base-64 or TLV format.\n"
    "\n"
    "  -V, --verbose\n"
    "\n"
    "       Display detailed validation results for each input certificate.\n"
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
    "Usage: " CMD_NAME " [ <options...> ] <target-cert-file>\n",
    WEAVE_VERSION_STRING "\n" COPYRIGHT_STRING,
    "Validate a chain of Weave certificates.\n"
    "\n"
    "ARGUMENTS\n"
    "\n"
    "  <target-cert-file>\n"
    "\n"
    "      A file containing the certificate to be validated. The certificate\n"
    "      must be a Weave certificate in either base-64 or TLV format.\n"
    "\n"
);

static OptionSet *gCmdOptionSets[] =
{
    &gCmdOptions,
    &gHelpOptions,
    NULL
};


enum { kMaxCerts = 64 };

static const char *gTargetCertFileName = NULL;
static const char *gCACertFileNames[kMaxCerts];
static bool gCACertIsTrusted[kMaxCerts];
static size_t gNumCertFileNames = 0;
static bool gVerbose = false;

bool Cmd_ValidateCert(int argc, char *argv[])
{
    bool res = true;
    WEAVE_ERROR err;
    WeaveCertificateSet certSet;
    WeaveCertificateData *certToBeValidated;
    WeaveCertificateData *validatedCert;
    uint8_t *certBufs[kMaxCerts];
    WEAVE_ERROR certValidationRes[kMaxCerts];
    ValidationContext context;

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

    for (size_t i = 0; i < gNumCertFileNames; i++)
    {
        if (!LoadWeaveCert(gCACertFileNames[i], gCACertIsTrusted[i], certSet, certBufs[certSet.CertCount]))
            ExitNow(res = false);
    }

    res = LoadWeaveCert(gTargetCertFileName, false, certSet, certBufs[certSet.CertCount]);
    if (!res)
        ExitNow(res = false);
    certToBeValidated = &certSet.Certs[certSet.CertCount - 1];

    memset(&context, 0, sizeof(context));
    context.EffectiveTime = SecondsSinceEpochToPackedCertTime(time(NULL));
    if (gVerbose)
    {
        context.CertValidationResults = certValidationRes;
        context.CertValidationResultsLen = kMaxCerts;
    }

    err = certSet.FindValidCert(certToBeValidated->SubjectDN, certToBeValidated->SubjectKeyId, context, validatedCert);
    if (err != WEAVE_NO_ERROR)
        printf("%s\n", nl::ErrorStr(err));

    if (gVerbose)
    {
        if (err == WEAVE_NO_ERROR)
            printf("Certificate validation completed successfully.\n");

        printf("\nValidation results:\n\n");

        PrintCertValidationResults(stdout, certSet, context, 2);
    }

    res = (err == WEAVE_NO_ERROR);

exit:
    certSet.Release();
    for (int i = 0; i < kMaxCerts; i++)
        if (certBufs[i] != NULL)
            free(certBufs[i]);
    return res;
}

bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg)
{
    switch (id)
    {
    case 'c':
    case 't':
        gCACertFileNames[gNumCertFileNames] = arg;
        gCACertIsTrusted[gNumCertFileNames++] = (id == 't');
        break;
    case 'V':
        gVerbose = true;
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
        PrintArgError("%s: Please specify the name of the certificate to be validated.\n", progName);
        return false;
    }

    if (argc > 1)
    {
        PrintArgError("%s: Unexpected argument: %s\n", progName, argv[1]);
        return false;
    }

    gTargetCertFileName = argv[0];

    return true;
}
