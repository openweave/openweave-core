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
#include <ctype.h>

#include "weave-tool.h"

using namespace nl::Weave::Profiles::Security;

struct CSVValue
{
    const char * Data;
    size_t Length;
};

#define CMD_NAME "weave convert-provisioning-data"

static bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg);
static bool HandleNonOptionArgs(const char *progName, int argc, char *argv[]);
static void TrimSpace(const char * & p, size_t & len);
static bool ParseCSVLine(const char * input, size_t inputLen, char sep, CSVValue * values, size_t maxValues, size_t & numValues);
static bool ReadCSVLine(FILE * inFile, char * buf, size_t bufSize, CSVValue * values, size_t maxValues, size_t & numValues);
static bool WriteCSVLine(FILE * outFile, CSVValue * values, size_t numValues);
static bool ConvertCertificate(const char * inCertB64, size_t inCertB64Len, CertFormat outCertFormat, char * & outCertB64, size_t & outCertB64Len);
static bool ConvertPrivateKey(const char * inKeyB64, size_t inKeyB64Len, KeyFormat keyFormat, char * & outKeyB64, size_t & outKeyB64Len);
static size_t FindColumnByPrefix(const CSVValue * colNames, size_t numCols, const char * prefix, size_t prefixLen);

enum
{
    kToolOpt_WeaveCert  = 1000,
    kToolOpt_DERCert    = 1001,
    kToolOpt_WeaveKey   = 1002,
    kToolOpt_DERKey     = 1003,
    kToolOpt_PKCS8Key   = 1004,
};

static OptionDef gCmdOptionDefs[] =
{
    { "weave",             kNoArgument,       'w'                   },
    { "der",               kNoArgument,       'x'                   },
    { "weave-cert",        kNoArgument,       kToolOpt_WeaveCert    },
    { "der-cert",          kNoArgument,       kToolOpt_DERCert      },
    { "weave-key",         kNoArgument,       kToolOpt_WeaveKey     },
    { "der-key",           kNoArgument,       kToolOpt_DERKey       },
    { "pkcs8-key",         kNoArgument,       kToolOpt_PKCS8Key     },
    { }
};

static const char *const gCmdOptionHelp =
    "   -w, --weave\n"
    "\n"
    "       Convert the certificate and private key to Weave TLV format.\n"
    "\n"
    "   -x, --der\n"
    "\n"
    "       Convert the certificate and private key to DER format. The certificate\n"
    "       is output in X.509 form, while the private key is output in SEC1/RFC-5915\n"
    "       form.\n"
    "\n"
    "   --weave-cert\n"
    "\n"
    "       Convert the certificate to Weave TLV format.\n"
    "\n"
    "   --der-cert\n"
    "\n"
    "       Convert the certificate to X.509 DER format.\n"
    "\n"
    "   --weave-key\n"
    "\n"
    "       Convert the private key to Weave TLV format.\n"
    "\n"
    "   --der-key\n"
    "\n"
    "       Convert the private key to SEC1/RFC-5915 DER format.\n"
    "\n"
    "   --pkcs8-key\n"
    "\n"
    "       Convert the private key to PKCS#8 DER format.\n"
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
    "Perform various conversions on a device provisioning data file.\n"
    "\n"
    "ARGUMENTS\n"
    "\n"
    "   <in-file>\n"
    "\n"
    "       A CSV file containing provisioning data to be converted, or - to read from\n"
    "       stdin.  The first line of this file must contain names for each of the CSV\n"
    "       columns.  Depending on which conversions are requested, the following columns\n"
    "       must be present:\n"
    "\n"
    "           Certificate        - Certificate in Weave TLV form, base-64 encoded\n"
    "           Certificate DER    - Certificate in X.509 DER form, base-64 encoded\n"
    "           Private Key        - Private key in Weave TLV form, base-64 encoded\n"
    "           Private Key DER    - Private key in SEC1/RFC-5915 DER form, base-64 encoded\n"
    "           Private Key PKCS8  - Private key in PKCS8 DER form, base-64 encoded\n"
    "\n"
    "       Any additional columns present in the input will be passed through to the output\n"
    "       without modification.\n"
    "\n"
    "   <out-file>\n"
    "\n"
    "       The CSV file to which the converted provisioning data should be written, or\n"
    "       - to write to stdout.\n"
    "\n"
);

static OptionSet *gCmdOptionSets[] =
{
    &gCmdOptions,
    &gHelpOptions,
    NULL
};

static const char *gInFileName;
static const char *gOutFileName;
static CertFormat gCertFormat = kCertFormat_Unknown;
static KeyFormat gKeyFormat = kKeyFormat_Unknown;

static const char gColumnName_Certificate[] = "Certificate";
static const size_t gColumnName_CertificateLen = sizeof(gColumnName_Certificate) - 1;

static const char gColumnName_CertificateDER[] = "Certificate DER";
static const size_t gColumnName_CertificateDERLen = sizeof(gColumnName_CertificateDER) - 1;

static const char gColumnName_PrivateKey[] = "Private Key";
static const size_t gColumnName_PrivateKeyLen = sizeof(gColumnName_PrivateKey) - 1;

static const char gColumnName_PrivateKeyDER[] = "Private Key DER";
static const size_t gColumnName_PrivateKeyDERLen = sizeof(gColumnName_PrivateKeyDER) - 1;

static const char gColumnName_PrivateKeyPKCS8[] = "Private Key PKCS8";
static const size_t gColumnName_PrivateKeyPKCS8Len = sizeof(gColumnName_PrivateKeyPKCS8) - 1;

bool Cmd_ConvertProvisioningData(int argc, char *argv[])
{
    enum
    {
        kMaxLineLength = 2048,
        kMaxCSVColumns = 20,
    };

    static char inBuf[kMaxLineLength];
    static CSVValue inValues[kMaxCSVColumns];

    bool res = true;
    FILE * inFile = NULL;
    FILE * outFile = NULL;
    char * convertedCert = NULL;
    size_t convertedCertLen = 0;
    char * convertedKey = NULL;
    size_t convertedKeyLen = 0;
    size_t numCols;
    size_t certCol, privKeyCol;
    bool outFileCreated = false;

    if (argc == 1)
    {
        gHelpOptions.PrintBriefUsage(stderr);
        ExitNow(res = true);
    }

    if (!ParseArgs(CMD_NAME, argc, argv, gCmdOptionSets, HandleNonOptionArgs))
    {
        ExitNow(res = false);
    }

    // Open the input file.
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

    // Open the output file.
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

    // Initialize OpenSSL.
    if (!InitOpenSSL())
        ExitNow(res = false);

    // Read the header line
    if (!ReadCSVLine(inFile, inBuf, sizeof(inBuf), inValues, kMaxCSVColumns, numCols))
    {
        ExitNow(res = false);
    }

    // Search for a column whose name begins with "Certificate".  Fail if requested to perform
    // certificate conversion and no Certificate column exists.
    certCol = FindColumnByPrefix(inValues, numCols, gColumnName_Certificate, gColumnName_CertificateLen);
    if (gCertFormat != kCertFormat_Unknown && certCol == numCols)
    {
        fprintf(stderr, "weave: No Certificate column in input data\n");
        ExitNow(res = false);
    }

    // Search for a column whose name begins with "Private Key".
    privKeyCol = FindColumnByPrefix(inValues, numCols, gColumnName_PrivateKey, gColumnName_PrivateKeyLen);
    if (gKeyFormat != kKeyFormat_Unknown && privKeyCol == numCols)
    {
        fprintf(stderr, "weave: No Private Key column in input data\n");
        ExitNow(res = false);
    }

    // Write the column names line to the output, substituting the appropriate name for the
    // columns being converted.
    switch (gCertFormat)
    {
    case kCertFormat_Unknown:
        break;
    case kCertFormat_Weave_Base64:
        inValues[certCol].Data = gColumnName_Certificate;
        inValues[certCol].Length = gColumnName_CertificateLen;
        break;
    case kCertFormat_X509_DER:
        inValues[certCol].Data = gColumnName_CertificateDER;
        inValues[certCol].Length = gColumnName_CertificateDERLen;
        break;
    default:
        fprintf(stderr, "INTERNAL ERROR: Invalid certificate format\n");
        ExitNow(res = false);
    }
    switch (gKeyFormat)
    {
    case kKeyFormat_Unknown:
        break;
    case kKeyFormat_Weave_Base64:
        inValues[privKeyCol].Data = gColumnName_PrivateKey;
        inValues[privKeyCol].Length = gColumnName_PrivateKeyLen;
        break;
    case kKeyFormat_DER:
        inValues[privKeyCol].Data = gColumnName_PrivateKeyDER;
        inValues[privKeyCol].Length = gColumnName_PrivateKeyDERLen;
        break;
    case kKeyFormat_DER_PKCS8:
        inValues[privKeyCol].Data = gColumnName_PrivateKeyPKCS8;
        inValues[privKeyCol].Length = gColumnName_PrivateKeyPKCS8Len;
        break;
    default:
        fprintf(stderr, "INTERNAL ERROR: Invalid key format\n");
        ExitNow(res = false);
    }
    if (!WriteCSVLine(outFile, inValues, numCols))
    {
        ExitNow(res = false);
    }

    // Read the remaining lines in the file, converting the appropriate values...
    while (true)
    {
        if (!ReadCSVLine(inFile, inBuf, sizeof(inBuf), inValues, kMaxCSVColumns, numCols))
        {
            ExitNow(res = false);
        }

        if (numCols == 0 && feof(inFile))
        {
            break;
        }

        if (gCertFormat != kCertFormat_Unknown && numCols > certCol)
        {
            if (!ConvertCertificate(inValues[certCol].Data, inValues[certCol].Length, gCertFormat, convertedCert, convertedCertLen))
            {
                ExitNow(res = false);
            }

            inValues[certCol].Data = convertedCert;
            inValues[certCol].Length = convertedCertLen;
        }

        if (gKeyFormat != kKeyFormat_Unknown && numCols > privKeyCol)
        {
            if (!ConvertPrivateKey(inValues[privKeyCol].Data, inValues[privKeyCol].Length, gKeyFormat, convertedKey, convertedKeyLen))
            {
                ExitNow(res = false);
            }

            inValues[privKeyCol].Data = convertedKey;
            inValues[privKeyCol].Length = convertedKeyLen;
        }

        if (!WriteCSVLine(outFile, inValues, numCols))
        {
            ExitNow(res = false);
        }

        if (convertedCert != NULL)
        {
            free(convertedCert);
            convertedCert = NULL;
        }

        if (convertedKey != NULL)
        {
            free(convertedKey);
            convertedKey = NULL;
        }
    }

exit:
    if (inFile != NULL && inFile != stdin)
        fclose(inFile);
    if (outFile != NULL && outFile != stdout)
        fclose(outFile);
    if (gOutFileName != NULL && outFileCreated && !res)
        unlink(gOutFileName);
    if (convertedCert != NULL)
        free(convertedCert);
    if (convertedKey != NULL)
        free(convertedKey);
    return res;
}

bool HandleOption(const char * progName, OptionSet * optSet, int id, const char * name, const char * arg)
{
    switch (id)
    {
    case 'w':
        gCertFormat = kCertFormat_Weave_Base64;
        gKeyFormat = kKeyFormat_Weave_Base64;
        break;
    case 'x':
        gCertFormat = kCertFormat_X509_DER;
        gKeyFormat = kKeyFormat_DER;
        break;
    case kToolOpt_WeaveCert:
        gCertFormat = kCertFormat_Weave_Base64;
        break;
    case kToolOpt_DERCert:
        gCertFormat = kCertFormat_X509_DER;
        break;
    case kToolOpt_WeaveKey:
        gKeyFormat = kKeyFormat_Weave_Base64;
        break;
    case kToolOpt_DERKey:
        gKeyFormat = kKeyFormat_DER;
        break;
    case kToolOpt_PKCS8Key:
        gKeyFormat = kKeyFormat_DER_PKCS8;
        break;
    default:
        PrintArgError("%s: INTERNAL ERROR: Unhandled option: %s\n", progName, name);
        return false;
    }

    return true;
}

bool HandleNonOptionArgs(const char * progName, int argc, char * argv[])
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

void TrimSpace(const char * & p, size_t & len)
{
    while (len > 0 && isspace(*p))
    {
        p++;
        len--;
    }

    while (len > 0 && isspace(p[len - 1]))
    {
        len--;
    }
}

bool ParseCSVLine(const char * input, size_t inputLen, char sep, CSVValue * values, size_t maxValues, size_t & numValues)
{
    for (numValues = 0; inputLen > 0 && numValues < maxValues; numValues++)
    {
        CSVValue & value = values[numValues];

        value.Data = input;

        char * valueEnd = (char *)memchr(input, sep, inputLen);
        if (valueEnd != NULL)
        {
            value.Length = valueEnd - input;
            input = valueEnd + 1;
            inputLen -= value.Length + 1;
        }
        else
        {
            value.Length = inputLen;
            inputLen = 0;
        }

        TrimSpace(value.Data, value.Length);
    }

    if (inputLen != 0)
    {
        fprintf(stderr, "weave: Too many fields in input line\n");
        return false;
    }

    return true;
}

bool ReadCSVLine(FILE * inFile, char * buf, size_t bufSize, CSVValue * values, size_t maxValues, size_t & numValues)
{
    if (fgets(buf, bufSize, inFile) == NULL)
    {
        if (feof(inFile))
        {
            numValues = 0;
            return true;
        }
        fprintf(stderr, "weave: Error reading input file\n%s\n", strerror(errno));
        return false;
    }

    size_t lineLen = strlen(buf);

    if (lineLen == bufSize - 1 && buf[lineLen - 1] != '\n')
    {
        fprintf(stderr, "weave: Input line too long\n");
        return false;
    }

    return ParseCSVLine(buf, lineLen, ',', values, maxValues, numValues);
}

bool WriteCSVLine(FILE * outFile, CSVValue * values, size_t numValues)
{
    bool res = true;

    for (size_t i = 0; i < numValues; i++)
    {
        if (i != 0 && fputc(',', outFile) == EOF)
        {
            ExitNow(res = false);
        }

        if (fwrite(values[i].Data, 1, values[i].Length, outFile) != values[i].Length)
        {
            ExitNow(res = false);
        }
    }

    res = (fputc('\n', outFile) != EOF);

exit:
    if (!res)
    {
        fprintf(stderr, "weave: Error writing output file\n%s\n", strerror(ferror(outFile) ? errno : ENOSPC));
    }

    return res;
}

bool ConvertCertificate(const char * inCertB64, size_t inCertB64Len, CertFormat outCertFormat, char * & outCertB64, size_t & outCertB64Len)
{
    bool res = true;
    WEAVE_ERROR err;
    uint8_t * inCert = NULL;
    uint32_t inCertLen;
    uint8_t * outCert = NULL;
    uint32_t outCertLen;
    CertFormat inCertFormat;

    inCert = Base64Decode((const uint8_t *)inCertB64, (uint32_t)inCertB64Len, NULL, 0, inCertLen);
    if (inCert == NULL)
    {
        ExitNow(res = false);
    }

    inCertFormat = DetectCertFormat(inCert, inCertLen);

    if (inCertFormat != kCertFormat_Weave_Raw && inCertFormat != kCertFormat_X509_DER)
    {
        fprintf(stderr, "weave: Unrecognized certificate format: %*s\n", (int)inCertB64Len, inCertB64);
        ExitNow(res = false);
    }

    outCertLen = inCertLen * 100;
    outCert = (uint8_t *)malloc(outCertLen);
    if (outCert == NULL)
    {
        fprintf(stderr, "weave: Memory allocation failure\n");
        ExitNow(res = false);
    }

    if (inCertFormat == kCertFormat_Weave_Raw && outCertFormat == kCertFormat_X509_DER)
    {
        err = ConvertWeaveCertToX509Cert(inCert, inCertLen, outCert, outCertLen, outCertLen);
        if (err != WEAVE_NO_ERROR)
        {
            fprintf(stderr, "weave: Error converting certificate: %s\n", nl::ErrorStr(err));
            ExitNow(res = false);
        }
    }

    else if (inCertFormat == kCertFormat_X509_DER && outCertFormat == kCertFormat_Weave_Base64)
    {
        err = ConvertX509CertToWeaveCert(inCert, inCertLen, outCert, outCertLen, outCertLen);
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

    outCertB64 = Base64Encode(outCert, outCertLen);
    if (outCertB64 == NULL)
    {
        ExitNow(res = false);
    }

    outCertB64Len = strlen(outCertB64);

exit:
    if (inCert != NULL)
    {
        free(inCert);
    }
    if (outCert != NULL)
    {
        free(outCert);
    }
    return res;
}

bool ConvertPrivateKey(const char * inKeyB64, size_t inKeyB64Len, KeyFormat keyFormat, char * & outKeyB64, size_t & outKeyB64Len)
{
    bool res = true;
    uint8_t * inKey = NULL;
    uint32_t inKeyLen;
    EVP_PKEY *inKeyDecoded = NULL;
    uint8_t *outKey = NULL;
    uint32_t outKeyLen;

    inKey = Base64Decode((const uint8_t *)inKeyB64, (uint32_t)inKeyB64Len, NULL, 0, inKeyLen);
    if (inKey == NULL)
    {
        ExitNow(res = false);
    }

    if (!DecodePrivateKey(inKey, inKeyLen, kKeyFormat_Unknown, "", NULL, inKeyDecoded))
    {
        ExitNow(res = false);
    }

    // When Weave key format is requested, have EncodePrivateKey() encode to raw bytes, rather
    // than base64, since base64 encoding is handled below.
    if (keyFormat == kKeyFormat_Weave_Base64)
    {
        keyFormat = kKeyFormat_Weave_Raw;
    }

    if (!EncodePrivateKey(inKeyDecoded, keyFormat, outKey, outKeyLen))
    {
        ExitNow(res = false);
    }

    outKeyB64 = Base64Encode(outKey, outKeyLen);
    if (outKeyB64 == NULL)
    {
        ExitNow(res = false);
    }

    outKeyB64Len = strlen(outKeyB64);

exit:
    if (inKey != NULL)
    {
        free(inKey);
    }
    if (inKeyDecoded != NULL)
    {
        EVP_PKEY_free(inKeyDecoded);
    }
    if (outKey != NULL)
    {
        free(outKey);
    }
    return res;
}

size_t FindColumnByPrefix(const CSVValue * colNames, size_t numCols, const char * prefix, size_t prefixLen)
{
    size_t i;

    // Search for a column whose name matches prefix.
    for (i = 0; i < numCols; i++)
    {
        if (colNames[i].Length >= prefixLen &&
            memcmp(colNames[i].Data, prefix, prefixLen) == 0)
        {
            break;
        }
    }
    return i;
}
