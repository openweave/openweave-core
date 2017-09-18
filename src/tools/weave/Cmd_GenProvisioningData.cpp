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
 *      that generates Weave device provisioning data.
 *
 */

#define __STDC_FORMAT_MACROS
#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#include <ctype.h>
#include <getopt.h>
#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "weave-tool.h"
#include <Weave/Support/verhoeff/Verhoeff.h>

using namespace nl::Weave::Profiles::Security;
using namespace nl::Weave::ASN1;

#define CMD_NAME "weave gen-provisioning-data"

static bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg);
static bool OutputProvisioningData(FILE *outFile, uint64_t devId, X509 *caCert, EVP_PKEY *caKey, const char *curveName,
                                   const struct tm& validFrom, uint32_t validDays,
                                   const char *sigType, const EVP_MD *sigHashAlgo, uint32_t pairingCodeLen);
static char *GeneratePairingCode(uint32_t pairingCodeLen);
static char *GeneratePermissions(uint64_t devId);

static OptionDef gCmdOptionDefs[] =
{
    { "dev-id",            kArgumentRequired, 'i' },
    { "count",             kArgumentRequired, 'c' },
    { "ca-cert",           kArgumentRequired, 'C' },
    { "ca-key",            kArgumentRequired, 'K' },
    { "out",               kArgumentRequired, 'o' },
    { "curve",             kArgumentRequired, 'u' },
    { "valid-from",        kArgumentRequired, 'V' },
    { "lifetime",          kArgumentRequired, 'l' },
    { "pairing-code-len",  kArgumentRequired, 'P' },
    { "sha1",              kNoArgument,       '1' },
    { "sha256",            kNoArgument,       '2' },
    { NULL }
};

static const char *const gCmdOptionHelp =
    "   -i, --dev-id <hex-digits>\n"
    "\n"
    "       The starting device id (in hex) for which provisioning data should be generated.\n"
    "\n"
    "   -c, --count <num>\n"
    "\n"
    "       The number of devices which the provisioning data should be generated.\n"
    "\n"
    "   -C, --ca-cert <file>\n"
    "\n"
    "       File containing CA certificate to be used to sign device certificates.\n"
    "       (File must be in PEM format).\n"
    "\n"
    "   -K, --ca-key <file>\n"
    "\n"
    "       File containing CA private key to be used to sign device certificates.\n"
    "       (File must be in PEM format).\n"
    "\n"
    "   -o, --out <file>\n"
    "\n"
    "       File into which the provisioning data will be written.  By default, data is\n"
    "       written to stdout.\n"
    "\n"
    "   -u, --curve <elliptic-curve-name>\n"
    "\n"
    "       The elliptic curve to use when generating the public/private keys.\n"
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
    "   -P, --pairing-code-len <num-chars>\n"
    "\n"
    "       The number of characters in the generated device pairing codes.\n"
    "       Default is 6.\n"
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
    "Usage: " CMD_NAME " [ <options...> ]\n",
    WEAVE_VERSION_STRING "\n" COPYRIGHT_STRING,
    "Generate manufacturing provisioning data for one or more devices."
);

static OptionSet *gCmdOptionSets[] =
{
    &gCmdOptions,
    &gHelpOptions,
    NULL
};

static uint64_t gDevId = 0;
static int32_t gDevCount = 0;
static const char *gCurveName = NULL;
static const char *gCACertFileName = NULL;
static const char *gCAKeyFileName = NULL;
static const char *gOutFileName = "-";
static int32_t gValidDays = 0;
static int32_t gPairingCodeLen = 6;
static const EVP_MD *gSigHashAlgo = NULL;
static const char *gSigType = NULL;
static struct tm gValidFrom;

bool Cmd_GenProvisioningData(int argc, char *argv[])
{
    bool res = true;
    X509 *caCert = NULL;
    EVP_PKEY *caKey = NULL;
    FILE *outFile = NULL;
    bool outFileCreated = false;

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
        ExitNow(res = true);
    }

    if (!ParseArgs(CMD_NAME, argc, argv, gCmdOptionSets))
    {
        ExitNow(res = true);
    }

    if (gDevId == 0)
    {
        fprintf(stderr, "Please specify the starting device id using the --dev-id option.\n");
        ExitNow(res = false);
    }

    if (gDevCount == 0)
    {
        fprintf(stderr, "Please specify the number of devices device id using the --count option.\n");
        ExitNow(res = false);
    }

    if (gCACertFileName == NULL)
    {
        fprintf(stderr, "Please specify the CA certificate file name using the --ca-cert option.\n");
        ExitNow(res = false);
    }

    if (gCAKeyFileName == NULL)
    {
        fprintf(stderr, "Please specify the CA key file name using the --ca-key option.\n");
        ExitNow(res = false);
    }

    if (gCurveName == NULL)
    {
        fprintf(stderr, "Please specify the elliptic curve name using the --curve option.\n");
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

    if (!InitOpenSSL())
        ExitNow(res = false);

    if (!ReadCertPEM(gCACertFileName, caCert))
        ExitNow(res = false);

    if (!ReadPrivateKey(gCAKeyFileName, "Enter password for the CA key:", caKey))
        ExitNow(res = false);

    if (strcmp(gOutFileName, "-") != 0)
    {
        outFile = fopen(gOutFileName, "w+b");
        if (outFile == NULL)
        {
            fprintf(stderr, "weave: Unable to create %s\n%s\n", gOutFileName, strerror(errno));
            ExitNow(res = false);
        }
        outFileCreated = true;
    }
    else
        outFile = stdout;

    if (fprintf(outFile, "MAC, Certificate, Private Key, Permissions, Pairing Code, Certificate Type\n") < 0 ||
        ferror(outFile))
    {
        fprintf(stderr, "Error writing to output file: %s\n", strerror(errno));
        ExitNow(res = false);
    }

    for (int32_t i = 0; i < gDevCount; i++)
        if (!OutputProvisioningData(outFile, gDevId + i, caCert, caKey, gCurveName, gValidFrom, gValidDays, gSigType, gSigHashAlgo, gPairingCodeLen))
            ExitNow(res = false);

exit:
    if (caCert != NULL)
        X509_free(caCert);
    if (caKey != NULL)
        EVP_PKEY_free(caKey);
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
    case 'i':
        if (!ParseEUI64(arg, gDevId))
        {
            PrintArgError("%s: Invalid value specified for device id: %s\n", progName, arg);
            return false;
        }
        break;
    case 'c':
        if (!ParseInt(arg, gDevCount) || gDevCount <= 0)
        {
            PrintArgError("%s: Invalid value specified for device count: %s\n", progName, arg);
            return false;
        }
        break;
    case 'C':
        gCACertFileName = arg;
        break;
    case 'K':
        gCAKeyFileName = arg;
        break;
    case 'o':
        gOutFileName = arg;
        break;
    case 'u':
        gCurveName = arg;
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
    case 'P':
        if (!ParseInt(arg, gPairingCodeLen) || gPairingCodeLen <= 2)
        {
            PrintArgError("%s: Invalid value specified for pairing code length: %s\n", progName, arg);
            return false;
        }
        break;
    case '1':
        gSigHashAlgo = EVP_sha1();
        gSigType = "ECDSAWithSHA1";
        break;
    case '2':
        gSigHashAlgo = EVP_sha256();
        gSigType = "ECDSAWithSHA256";
        break;
    default:
        PrintArgError("%s: INTERNAL ERROR: Unhandled option: %s\n", progName, name);
        return false;
    }

    return true;
}

bool OutputProvisioningData(FILE *outFile, uint64_t devId, X509 *caCert, EVP_PKEY *caKey, const char *curveName,
                            const struct tm& validFrom, uint32_t validDays,
                            const char *sigType, const EVP_MD *sigHashAlgo, uint32_t pairingCodeLen)
{
    bool res = true;
    X509 *devCert = NULL;
    EVP_PKEY *devKey = NULL;
    uint8_t *weaveCert = NULL;
    uint32_t weaveCertLen;
    char *weaveCertB64 = NULL;
    uint8_t *weaveKey = NULL;
    uint32_t weaveKeyLen;
    char *weaveKeyB64 = NULL;
    char *pairingCode = NULL;
    char *perms = NULL;

    if (!MakeDeviceCert(devId, caCert, caKey, curveName, validFrom, validDays, sigHashAlgo, devCert, devKey))
        ExitNow(res = false);

    if (!WeaveEncodeCert(devCert, weaveCert, weaveCertLen))
        ExitNow(res = false);

    if (!WeaveEncodePrivateKey(devKey, weaveKey, weaveKeyLen))
        ExitNow(res = false);

    weaveCertB64 = Base64Encode(weaveCert, weaveCertLen);
    if (weaveCertB64 == NULL)
    {
        fprintf(stderr, "Memory allocation error\n");
        ExitNow(res = false);
    }

    weaveKeyB64 = Base64Encode(weaveKey, weaveKeyLen);
    if (weaveKeyB64 == NULL)
    {
        fprintf(stderr, "Memory allocation error\n");
        ExitNow(res = false);
    }

    perms = GeneratePermissions(devId);
    if (perms == NULL)
        ExitNow(res = false);

    pairingCode = GeneratePairingCode(pairingCodeLen);
    if (pairingCode == NULL)
        ExitNow(res = false);

    if (fprintf(outFile, "%016" PRIX64 ",%s,%s,%s,%s,%s\n", devId, weaveCertB64, weaveKeyB64, perms, pairingCode, sigType) < 0 ||
        ferror(outFile))
    {
        fprintf(stderr, "Error writing to output file: %s\n", strerror(errno));
        ExitNow(res = false);
    }

exit:
    if (weaveCertB64 != NULL)
        free(weaveCertB64);
    if (weaveKeyB64 != NULL)
        free(weaveKeyB64);
    if (devCert != NULL)
        X509_free(devCert);
    if (devKey != NULL)
        EVP_PKEY_free(devKey);
    if (weaveCert != NULL)
        free(weaveCert);
    if (weaveKey != NULL)
        free(weaveKey);
    if (pairingCode != NULL)
        free(pairingCode);
    if (perms != NULL)
        free(perms);
    return res;
}

char *GeneratePairingCode(uint32_t pairingCodeLen)
{
    char *pairingCode;

    pairingCode = (char *)malloc(pairingCodeLen + 1);
    if (pairingCode == NULL)
    {
        fprintf(stderr, "Memory allocation error\n");
        ExitNow(pairingCode = NULL);
    }

    // Generate random data for the pairing code, excluding the check digit at the end.
    if (!RAND_bytes((uint8_t *)pairingCode, pairingCodeLen - 1))
        ReportOpenSSLErrorAndExit("Failed to get random data", pairingCode = NULL);

    // Convert the random data to characters in the range 0-9, A-H, J-N, P, R-Y (base-32 alphanumeric, excluding I, O, Q and Z).
    for (uint32_t i = 0; i < pairingCodeLen - 1; i++)
    {
        uint8_t val = (uint8_t)pairingCode[i] / 8;
        pairingCode[i] = Verhoeff32::ValToChar(val);
    }

    // Compute the check digit.
    pairingCode[pairingCodeLen - 1] = Verhoeff32::ComputeCheckChar(pairingCode, pairingCodeLen - 1);

    // Terminate the string.
    pairingCode[pairingCodeLen] = 0;

exit:
    return pairingCode;
}

char *GeneratePermissions(uint64_t devId)
{
    // TODO: implement real permissions

    return strdup("__NONE__");
}
