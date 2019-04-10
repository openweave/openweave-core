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
 *      This file implements utility functions for OpenSSL, base-64
 *      encoding and decoding, date and time parsing, EUI64 parsing,
 *      OID translation, and file reading.
 *
 */

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif
#include <stdint.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <getopt.h>
#include <time.h>

#include "weave-tool.h"

#include <Weave/Support/Base64.h>

using namespace nl::Weave::Profiles::Security;
using namespace nl::Weave::ASN1;

int gNIDWeaveDeviceId;
int gNIDWeaveServiceEndpointId;
int gNIDWeaveCAId;
int gNIDWeaveSoftwarePublisherId;

// OBJ_length() and OBJ_get0_data() methods were introduced in OpenSSL
// version 1.1.0; for older versions of OpenSSL these methods are defined here.
#if (OPENSSL_VERSION_NUMBER < 0x10100000L)

static size_t OBJ_length(const ASN1_OBJECT *obj)
{
    if (obj == NULL)
        return 0;
    return obj->length;
}

static const unsigned char *OBJ_get0_data(const ASN1_OBJECT *obj)
{
    if (obj == NULL)
        return NULL;
    return obj->data;
}

#endif

bool InitOpenSSL()
{
    bool res = true;

    // OPENSSL_malloc_init() is a new method, which was first introduced
    // in OpenSSL version 1.1.0. Older versions of OpenSSL implement
    // equivalent method CRYPTO_malloc_init().
#if (OPENSSL_VERSION_NUMBER >= 0x10100000L)
    OPENSSL_malloc_init();
#else
    CRYPTO_malloc_init();
#endif

    ERR_load_crypto_strings();
    OpenSSL_add_all_algorithms();

    gNIDWeaveDeviceId = OBJ_create("1.3.6.1.4.1.41387.1.1", "WeaveDeviceId", "WeaveDeviceId");
    if (gNIDWeaveDeviceId == 0)
        ReportOpenSSLErrorAndExit("OBJ_create", res = false);

    gNIDWeaveServiceEndpointId = OBJ_create("1.3.6.1.4.1.41387.1.2", "WeaveServiceEndpointId", "WeaveServiceEndpointId");
    if (gNIDWeaveServiceEndpointId == 0)
        ReportOpenSSLErrorAndExit("OBJ_create", res = false);

    gNIDWeaveCAId = OBJ_create("1.3.6.1.4.1.41387.1.3", "WeaveCAId", "WeaveCAId");
    if (gNIDWeaveCAId == 0)
        ReportOpenSSLErrorAndExit("OBJ_create", res = false);

    gNIDWeaveSoftwarePublisherId = OBJ_create("1.3.6.1.4.1.41387.1.4", "WeaveSoftwarePublisherId", "WeaveSoftwarePublisherId");
    if (gNIDWeaveSoftwarePublisherId == 0)
        ReportOpenSSLErrorAndExit("OBJ_create", res = false);

    ASN1_STRING_TABLE_add(gNIDWeaveDeviceId, 16, 16, B_ASN1_UTF8STRING, 0);
    ASN1_STRING_TABLE_add(gNIDWeaveServiceEndpointId, 16, 16, B_ASN1_UTF8STRING, 0);
    ASN1_STRING_TABLE_add(gNIDWeaveCAId, 16, 16, B_ASN1_UTF8STRING, 0);
    ASN1_STRING_TABLE_add(gNIDWeaveSoftwarePublisherId, 16, 16, B_ASN1_UTF8STRING, 0);

exit:
    return res;
}

OID NIDToWeaveOID(int nid)
{
    const ASN1_OBJECT *encodedOID;

    encodedOID = OBJ_nid2obj(nid);
    if (encodedOID == NULL)
        return kOID_Unknown;

    return ParseObjectID(OBJ_get0_data(encodedOID), OBJ_length(encodedOID));
}

char *Base64Encode(const uint8_t *inData, uint32_t inDataLen)
{
    uint32_t outDataLen;
    return (char *)Base64Encode(inData, inDataLen, NULL, 0, outDataLen);
}

uint8_t *Base64Encode(const uint8_t *inData, uint32_t inDataLen, uint8_t *outBuf, uint32_t outBufSize, uint32_t& outDataLen)
{
    uint8_t *res = NULL;

    outDataLen = BASE64_ENCODED_LEN(inDataLen);

    if (outBuf != NULL && (outDataLen + 1) > outBufSize)
    {
        fprintf(stderr, "Buffer overflow\n");
        ExitNow(res = NULL);
    }

    res = (uint8_t *)malloc(outDataLen + 1);
    if (res == NULL)
    {
        fprintf(stderr, "Memory allocation error\n");
        ExitNow(res = NULL);
    }
    memset(res, 0xAB, outDataLen + 1);

    outDataLen = nl::Base64Encode32(inData, inDataLen, (char *)res);
    res[outDataLen] = 0;

    if (outBuf != NULL)
    {
        memcpy(outBuf, res, outDataLen + 1);
        free(res);
        res = outBuf;
    }

exit:
    return res;
}

uint8_t *Base64Decode(const uint8_t *inData, uint32_t inDataLen, uint8_t *outBuf, uint32_t outBufSize, uint32_t& outDataLen)
{
    uint8_t *res = NULL;

    outDataLen = BASE64_MAX_DECODED_LEN(inDataLen);

    if (outBuf != NULL)
    {
        if (outDataLen > outBufSize)
        {
            fprintf(stderr, "Buffer overflow\n");
            ExitNow();
        }
        res = outBuf;
    }
    else
    {
        res = (uint8_t *)malloc(outDataLen);
        if (res == NULL)
        {
            fprintf(stderr, "Memory allocation error\n");
            ExitNow();
        }
    }

    outDataLen = nl::Base64Decode32((const char *)inData, inDataLen, res);
    if (outDataLen == UINT32_MAX)
    {
        fprintf(stderr, "Base-64 decode error\n");
        if (res != outBuf)
            free(res);
        ExitNow(res = NULL);
    }

exit:
    return res;
}

bool IsBase64String(const char * str, uint32_t strLen)
{
    for (; strLen > 0; strLen--, str++)
    {
        if (!isalnum(*str) && *str != '+' && *str != '/' && *str != '=' && !isspace(*str))
            return false;
    }
    return true;
}

bool ContainsPEMMarker(const char *marker, const uint8_t *data, uint32_t dataLen)
{
    size_t markerLen = strlen(marker);

    if (dataLen > markerLen)
        for (uint32_t i = 0; i <= dataLen - markerLen; i++)
            if (strncmp((char *)data + i, marker, markerLen) == 0)
                return true;
    return false;
}

bool ParseEUI64(const char *str, uint64_t& nodeId)
{
    char *parseEnd;

    errno = 0;
    nodeId = strtoull(str, &parseEnd, 16);
    return parseEnd > str && *parseEnd == 0 && (nodeId != ULLONG_MAX || errno == 0);
}

bool ParseDateTime(const char *str, struct tm& date)
{
    const char *p;

    memset(&date, 0, sizeof(date));

    if ((p = strptime(str, "%Y-%m-%d %H:%M:%S", &date)) == NULL &&
        (p = strptime(str, "%Y/%m/%d %H:%M:%S", &date)) == NULL &&
        (p = strptime(str, "%Y%m%d%H%M%SZ", &date)) == NULL  &&
        (p = strptime(str, "%Y-%m-%d", &date)) == NULL &&
        (p = strptime(str, "%Y/%m/%d", &date)) == NULL &&
        (p = strptime(str, "%Y%m%d", &date)) == NULL)
    {
        return false;
    }

    if (*p != 0)
    {
        return false;
    }

    return true;
}

bool ReadFileIntoMem(const char *fileName, uint8_t *& data, uint32_t &dataLen)
{
    bool res = true;
    FILE *file = NULL;
    long int fileLen;
    size_t readRes;

    data = NULL;
    dataLen = 0;

    file = fopen(fileName, "r");
    if (file == NULL)
    {
        fprintf(stderr, "weave: Unable to open %s: %s\n", fileName, strerror(errno));
        ExitNow(res = false);
    }

    fseek(file, 0, SEEK_END);
    fileLen = ftell(file);
    fseek(file, 0, SEEK_SET);
    if (fileLen < 0 || ferror(file))
    {
        fprintf(stderr, "weave: Error reading %s: %s\n", fileName, strerror(errno));
        ExitNow(res = false);
    }

    dataLen = (uint32_t)fileLen;
    data = (uint8_t *)malloc((size_t)dataLen);
    if (data == NULL)
    {
        fprintf(stderr, "weave: Error reading %s: out of memory\n", fileName);
        ExitNow(res = false);
    }

    readRes = fread(data, 1, (size_t)dataLen, file);
    if (readRes < (size_t)dataLen || ferror(file))
    {
        fprintf(stderr, "weave: Error reading %s: %s\n", fileName, strerror(errno));
        ExitNow(res = false);
    }

exit:
    if (file != NULL)
        fclose(file);
    if (!res && data != NULL)
    {
        free(data);
        data = NULL;
    }
    return res;
}
