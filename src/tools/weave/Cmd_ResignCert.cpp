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
 *      that re-signs a Weave certificate.
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

#define CMD_NAME "weave resign-cert"

static bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg);

static OptionDef gCmdOptionDefs[] =
{
    { "cert",       kArgumentRequired, 'c' },
    { "out",        kArgumentRequired, 'o' },
    { "ca-cert",    kArgumentRequired, 'C' },
    { "ca-key",     kArgumentRequired, 'K' },
    { "self",       kNoArgument,       's' },
    { "sha1",       kNoArgument,       '1' },
    { "sha256",     kNoArgument,       '2' },
    { NULL }
};

static const char *const gCmdOptionHelp =
    "  -c, --cert <file>\n"
    "\n"
    "       File containing the certificate to be re-signed.\n"
    "\n"
    "  -o, --out <file>\n"
    "\n"
    "       File to contain the re-signed certificate.\n"
    "\n"
    "  -C, --ca-cert <file>\n"
    "\n"
    "       File containing CA certificate to be used to re-sign the certificate\n"
    "       (in PEM format).\n"
    "\n"
    "  -K, --ca-key <file>\n"
    "\n"
    "       File containing CA private key to be used to re-sign the certificate\n"
    "       (in PEM format).\n"
    "\n"
    "  -s, --self\n"
    "\n"
    "       Generate a self-signed certificate.\n"
    "\n"
    "  -1, --sha1\n"
    "\n"
    "       Re-sign the certificate using a SHA-1 hash.\n"
    "\n"
    "  -2, --sha256\n"
    "\n"
    "       Re-sign the certificate using a SHA-256 hash.\n"
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
    "Usage: " CMD_NAME " [ <options...> ]\n",
    WEAVE_VERSION_STRING "\n" COPYRIGHT_STRING,
    "Resign a weave certificate using a new CA certificate/key."
);

static OptionSet *gCmdOptionSets[] =
{
    &gCmdOptions,
    &gHelpOptions,
    NULL
};

static const char *gInCertFileName = NULL;
static const char *gOutCertFileName = NULL;
static const char *gCACertFileName = NULL;
static const char *gCAKeyFileName = NULL;
static const EVP_MD *gSigHashAlgo = NULL;
static bool gSelfSign = false;

bool Cmd_ResignCert(int argc, char *argv[])
{
    bool res = true;
    FILE *outCertFile = NULL;
    X509 *caCert = NULL;
    X509 *cert = NULL;
    EVP_PKEY *caKey = NULL;
    CertFormat inCertFmt;
    bool outCertFileCreated = false;

    if (argc == 1)
    {
        gHelpOptions.PrintBriefUsage(stderr);
        ExitNow(res = true);
    }

    if (!ParseArgs(CMD_NAME, argc, argv, gCmdOptionSets))
    {
        ExitNow(res = true);
    }

    if (gInCertFileName == NULL)
    {
        fprintf(stderr, "Please specify certificate to be resigned using --cert option.\n");
        ExitNow(res = false);
    }

    if (gOutCertFileName == NULL)
    {
        fprintf(stderr, "Please specify the file name for the new certificate using the --out option.\n");
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

    if (gCAKeyFileName == NULL)
    {
        fprintf(stderr, "Please specify the CA key file name using the --ca-key option.\n");
        ExitNow(res = false);
    }

    if (gSigHashAlgo == NULL)
    {
        fprintf(stderr, "Please specify a signature hash algorithm using either the --sha1 or --sha256 options.\n");
        ExitNow(res = false);
    }

    if (access(gOutCertFileName, R_OK) == 0)
    {
        fprintf(stderr, "weave: ERROR: Output certificate file already exists (%s)\n"
                        "To replace the file, please remove it and re-run the command.\n",
                        gOutCertFileName);
        ExitNow(res = false);
    }

    outCertFile = fopen(gOutCertFileName, "w+");
    if (outCertFile == NULL)
    {
        fprintf(stderr, "weave: ERROR: Unable to create output certificate file (%s)\n"
                        "%s.\n",
                        gOutCertFileName,
                        strerror(errno));
        ExitNow(res = false);
    }
    outCertFileCreated = true;

    if (!InitOpenSSL())
        ExitNow(res = false);

    if (!ReadCert(gInCertFileName, cert, inCertFmt))
        ExitNow(res = false);

    if (!gSelfSign)
    {
        if (!ReadCertPEM(gCACertFileName, caCert))
            ExitNow(res = false);
    }
    else
        caCert = cert;

    if (!ReadPrivateKey(gCAKeyFileName, "Enter password for private key:", caKey))
        ExitNow(res = false);

    if (!ResignCert(cert, caCert, caKey, gSigHashAlgo))
        ExitNow(res = false);

    if (!WriteCert(cert, outCertFile, gOutCertFileName, inCertFmt))
        ExitNow(res = false);

    res = (fclose(outCertFile) != EOF);
    outCertFile = NULL;
    if (!res)
    {
        fprintf(stderr, "weave: ERROR: Unable to write output certificate file (%s)\n"
                        "%s.\n",
                        gOutCertFileName,
                        strerror(errno));
        ExitNow();
    }

exit:
    if (cert != NULL)
        X509_free(cert);
    if (caCert != NULL && caCert != cert)
        X509_free(caCert);
    if (caKey != NULL)
        EVP_PKEY_free(caKey);
    if (outCertFile != NULL)
        fclose(outCertFile);
    if (gOutCertFileName != NULL && outCertFileCreated && !res)
        unlink(gOutCertFileName);
    return res;
}

bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg)
{
    switch (id)
    {
    case 'c':
        gInCertFileName = arg;
        break;
    case 'o':
        gOutCertFileName = arg;
        break;
    case 'C':
        gCACertFileName = arg;
        break;
    case 'K':
        gCAKeyFileName = arg;
        break;
    case 's':
        gSelfSign = true;
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
