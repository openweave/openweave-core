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
 *      that generates a Weave general certificate with a string
 *      subject.
 *
 */

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif
#include <stdint.h>
#include <unistd.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <getopt.h>

#include "weave-tool.h"

using namespace nl::Weave::Profiles::Security;
using namespace nl::Weave::ASN1;

#define CMD_NAME "weave gen-general-cert"

static bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg);

static OptionDef gCmdOptionDefs[] =
{
    { "subject",    kArgumentRequired, 'S' },
    { "key",        kArgumentRequired, 'k' },
    { "ca-cert",    kArgumentRequired, 'C' },
    { "ca-key",     kArgumentRequired, 'K' },
    { "self",       kNoArgument,       's' },
    { "out",        kArgumentRequired, 'o' },
    { "valid-from", kArgumentRequired, 'V' },
    { "lifetime",   kArgumentRequired, 'l' },
    { "sha1",       kNoArgument,       '1' },
    { "sha256",     kNoArgument,       '2' },
    { NULL }
};

static const char *const gCmdOptionHelp =
    "   -S, --subject <string>\n"
    "\n"
    "       The subject of the new certificate.\n"
    "\n"
    "   -k, --key <file>\n"
    "\n"
    "       File containing the public and private keys for the new certificate.\n"
    "       (File must be in PEM format).\n"
    "\n"
    "   -C, --ca-cert <file>\n"
    "\n"
    "       File containing CA certificate to be used to sign the new certificate.\n"
    "       (File must be in PEM format).\n"
    "\n"
    "   -K, --ca-key <file>\n"
    "\n"
    "       File containing CA private key to be used to sign the new certificate.\n"
    "       (File must be in PEM format).\n"
    "\n"
    "   -o, --out <file>\n"
    "\n"
    "       File to contain the new certificate. (Will be written in Weave format).\n"
    "\n"
    "   -s, --self\n"
    "\n"
    "       Generate a self-signed certificate.\n"
    "\n"
    "   -V, --valid-from <YYYY>-<MM>-<DD> [ <HH>:<MM>:<SS> ]\n"
    "\n"
    "       The start date for the certificate's validity period.  If not specified,\n"
    "       the validity period starts on the current day.\n"
    "\n"
    "   -l, --lifetime <days>\n"
    "\n"
    "       The lifetime for the new certificate, in whole days.\n"
    "\n"
    "   -1, --sha1\n"
    "\n"
    "       Sign the certificate using a SHA-1 hash.\n"
    "\n"
    "   -2, --sha256\n"
    "\n"
    "       Sign the certificate using a SHA-256 hash.\n"
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
    "Usage: " CMD_NAME " <options...>\n",
    WEAVE_VERSION_STRING "\n" COPYRIGHT_STRING,
    "Generate a general Weave certificate with a string subject\n"
);

static OptionSet *gCmdOptionSets[] =
{
    &gCmdOptions,
    &gHelpOptions,
    NULL
};


static const char *gCertSubject = NULL;
static const char *gCACertFileName = NULL;
static const char *gCAKeyFileName = NULL;
static const char *gCertFileName = NULL;
static const char *gCertKeyFileName = NULL;
static bool gSelfSign = false;
static int32_t gValidDays = 0;
static const EVP_MD *gSigHashAlgo = NULL;
static struct tm gValidFrom;

bool Cmd_GenGeneralCert(int argc, char *argv[])
{
    bool res = true;
    uint8_t *weaveCert = NULL;
    uint32_t weaveCertLen;
    X509 *caCert = NULL;
    X509 *newCert = NULL;
    EVP_PKEY *caKey = NULL;
    EVP_PKEY *newCertKey = NULL;
    FILE *newCertFile = NULL;
    bool certFileCreated = false;

    {
        time_t now = time(NULL);
        gValidFrom = *gmtime(&now);
        gValidFrom.tm_hour = 0;
        gValidFrom.tm_min = 0;
        gValidFrom.tm_sec = 0;
    }

    if (argc == 1)
    {
        gHelpOptions.PrintBriefUsage(stderr);
        ExitNow(res = false);
    }

    if (!ParseArgs(CMD_NAME, argc, argv, gCmdOptionSets))
    {
        ExitNow(res = false);
    }

    if (gCertSubject == NULL)
    {
        fprintf(stderr, "Please specify the subject for the new certificate using the --subject option.\n");
        ExitNow(res = false);
    }

    if (gCertKeyFileName == NULL)
    {
        fprintf(stderr, "Please use the --key option to specify the public/private key file for the\n"
                        "new certificate.\n");
        ExitNow(res = false);
    }

    if (gCACertFileName == NULL && !gSelfSign)
    {
        fprintf(stderr, "Please specify a CA certificate to be used to sign the new certificate (using\n"
                        "the --ca-cert option) or --self to generate a self-signed certificate.\n");
        ExitNow(res = false);
    }

    else if (gCACertFileName != NULL && gSelfSign)
    {
        fprintf(stderr, "Please specify only one of --ca-cert and --self.\n");
        ExitNow(res = false);
    }

    if (gCACertFileName != NULL && gCAKeyFileName == NULL)
    {
        fprintf(stderr, "Please use the the --ca-key option to specify the key file for the CA\n"
                        "certificate that will be used to sign the new certificate.\n");
        ExitNow(res = false);
    }

    if (gCertFileName == NULL)
    {
        fprintf(stderr, "Please specify the file name for the new certificate using the --out option.\n");
        ExitNow(res = false);
    }

    if (gValidDays == 0)
    {
        fprintf(stderr, "Please specify the lifetime for the new certificate (in days) using the --lifetime option.\n");
        ExitNow(res = false);
    }

    if (gSigHashAlgo == NULL)
    {
        fprintf(stderr, "Please specify a signature hash algorithm using either the --sha1 or --sha256 options.\n");
        ExitNow(res = false);
    }

    if (strcmp(gCertFileName, "-") != 0 && access(gCertFileName, R_OK) == 0)
    {
        fprintf(stderr, "weave: ERROR: Output file already exists (%s)\n"
                        "To replace the file, please remove it and re-run the command.\n",
                        gCertFileName);
        ExitNow(res = false);
    }

    if (!InitOpenSSL())
        ExitNow(res = false);

    if (strcmp(gCertFileName, "-") != 0)
    {
        newCertFile = fopen(gCertFileName, "w+");
        if (newCertFile == NULL)
        {
            fprintf(stderr, "weave: ERROR: Unable to create output file (%s)\n"
                            "%s.\n",
                            gCertFileName,
                            strerror(errno));
            ExitNow(res = false);
        }
        certFileCreated = true;
    }
    else
        newCertFile = stdout;

    if (!ReadPrivateKey(gCertKeyFileName, "Enter password for private key:", newCertKey))
        ExitNow(res = false);

    if (!gSelfSign)
    {
        if (!ReadCertPEM(gCACertFileName, caCert))
            ExitNow(res = false);

        if (!ReadPrivateKey(gCAKeyFileName, "Enter password for signing CA certificate key:", caKey))
            ExitNow(res = false);
    }
    else
        caKey = newCertKey;

    if (!MakeGeneralCert(gCertSubject, newCertKey, caCert, caKey, gValidFrom, gValidDays, gSigHashAlgo, newCert))
        ExitNow(res = false);

    if (!WeaveEncodeCert(newCert, weaveCert, weaveCertLen))
        ExitNow(res = false);

    if (fwrite(weaveCert, 1, weaveCertLen, newCertFile) != weaveCertLen)
    {
        fprintf(stderr, "weave: ERROR: Unable to write to %s\n%s\n", gCertFileName, strerror(ferror(newCertFile) ? errno : ENOSPC));
        ExitNow(res = false);
    }

    res = (newCertFile == stdout || fclose(newCertFile) != EOF);
    newCertFile = NULL;
    if (!res)
    {
        fprintf(stderr, "weave: ERROR: Unable to write certificate file (%s)\n"
                        "%s.\n",
                        gCertFileName,
                        strerror(errno));
        ExitNow();
    }

exit:
    if (newCert != NULL)
        X509_free(newCert);
    if (caCert != NULL)
        X509_free(caCert);
    if (caKey != NULL && caKey != newCertKey)
        EVP_PKEY_free(caKey);
    if (newCertKey != NULL)
        EVP_PKEY_free(newCertKey);
    if (newCertFile != NULL && newCertFile != stdout)
        fclose(newCertFile);
    if (gCertFileName != NULL && certFileCreated && !res)
        unlink(gCertFileName);
    return res;
}

bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg)
{
    switch (id)
    {
    case 'S':
        gCertSubject = arg;
        break;
    case 'C':
        gCACertFileName = arg;
        break;
    case 'K':
        gCAKeyFileName = arg;
        break;
    case 'o':
        gCertFileName = arg;
        break;
    case 'k':
        gCertKeyFileName = arg;
        break;
    case 's':
        gSelfSign = true;
        break;
    case 'V':
        if (!ParseDateTime(arg, gValidFrom))
        {
            PrintArgError("%s: Invalid value specified for certificate validity date: %s\n", progName, arg);
            return false;
        }
        break;
    case 'l':
        if (!ParseInt(arg, gValidDays) || gValidDays < 0)
        {
            PrintArgError("%s: Invalid value specified for certificate lifetime: %s\n", progName, arg);
            return false;
        }
        break;
    case '1':
        gSigHashAlgo = EVP_sha1();
        break;
    case '2':
        gSigHashAlgo = EVP_sha256();
        break;
    default:
        PrintArgError("%s: INTERNAL ERROR: Unhandled option: %s\n", progName, name);
        return false;
    }

    return true;
}
