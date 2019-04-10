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
 *      This file implements a process to effect a functional test for
 *      the Weave Abstract Syntax Notifcation One (ASN1) encode and
 *      decode interfaces.
 *
 */

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#include <stdio.h>
#include <stdint.h>
#include "ToolCommon.h"
#include <Weave/Support/ASN1.h>
#include <Weave/Support/ASN1Macros.h>

#define VerifyOrFail(TST, MSG) \
do { \
    if (!(TST)) \
    { \
        fprintf(stderr, "%s FAILED: ", __FUNCTION__); \
        fprintf(stderr, MSG); \
        exit(-1); \
    } \
} while (0)



using namespace nl::Weave::ASN1;

uint8_t EncodeTestResult[] =
{
    0x30, 0x81, 0xC1, 0x01, 0x01, 0x00, 0x01, 0x01, 0xFF, 0x31, 0x00, 0x03, 0x01, 0x00, 0x03, 0x02,
    0x07, 0x80, 0x03, 0x02, 0x06, 0xC0, 0x30, 0x16, 0x30, 0x0F, 0x30, 0x08, 0x03, 0x02, 0x03, 0xE8,
    0x03, 0x02, 0x02, 0xEC, 0x03, 0x03, 0x07, 0xE1, 0x80, 0x03, 0x03, 0x06, 0xE7, 0xC0, 0x02, 0x01,
    0x00, 0x02, 0x01, 0x01, 0x02, 0x01, 0xFF, 0x02, 0x04, 0x00, 0xFF, 0x00, 0xFF, 0x02, 0x04, 0xFF,
    0x00, 0xFF, 0x01, 0x02, 0x04, 0x7F, 0xFF, 0xFF, 0xFF, 0x02, 0x04, 0x80, 0x00, 0x00, 0x00, 0x02,
    0x08, 0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x02, 0x08, 0x80, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x06, 0x03, 0x55, 0x04, 0x0A, 0x04, 0x09, 0x01, 0x03, 0x05, 0x07, 0x10, 0x30,
    0x50, 0x70, 0x00, 0x04, 0x01, 0x01, 0x04, 0x00, 0x1B, 0x00, 0x13, 0x16, 0x53, 0x75, 0x64, 0x64,
    0x65, 0x6E, 0x20, 0x64, 0x65, 0x61, 0x74, 0x68, 0x20, 0x69, 0x6E, 0x20, 0x56, 0x65, 0x6E, 0x69,
    0x63, 0x65, 0x0C, 0x1A, 0x4F, 0x6E, 0x64, 0x20, 0x62, 0x72, 0x61, 0xCC, 0x8A, 0x64, 0x20, 0x64,
    0x6F, 0xCC, 0x88, 0x64, 0x20, 0x69, 0x20, 0x56, 0x65, 0x6E, 0x65, 0x64, 0x69, 0x67, 0x04, 0x14,
    0x30, 0x12, 0x06, 0x0A, 0x2B, 0x06, 0x01, 0x04, 0x01, 0x82, 0xC3, 0x2B, 0x01, 0x02, 0x03, 0x04,
    0x00, 0x02, 0x01, 0x2A
};

static uint8_t testOctetString[] = { 0x01, 0x03, 0x05, 0x07, 0x10, 0x30, 0x50, 0x70, 0x00 };
static const char *testPrintableString = "Sudden death in Venice";
static const char *testUTFString = "Ond bra\xCC\x8A""d do\xCC\x88""d i Venedig";

ASN1_ERROR DoEncode(ASN1Writer & writer)
{
    ASN1_ERROR err = ASN1_NO_ERROR;

    ASN1_START_SEQUENCE {
        ASN1_ENCODE_BOOLEAN(false);
        ASN1_ENCODE_BOOLEAN(true);
        ASN1_START_SET {
        } ASN1_END_SET;
        ASN1_ENCODE_BIT_STRING(0x0);
        ASN1_ENCODE_BIT_STRING(0x1);
        ASN1_ENCODE_BIT_STRING(0x3);
        ASN1_START_SEQUENCE {
            ASN1_START_SEQUENCE {
                ASN1_START_SEQUENCE {
                    ASN1_ENCODE_BIT_STRING(0x17);
                    ASN1_ENCODE_BIT_STRING(0x37);
                } ASN1_END_SEQUENCE;
                ASN1_ENCODE_BIT_STRING(0x187);
            } ASN1_END_SEQUENCE;
            ASN1_ENCODE_BIT_STRING(0x3E7);
        } ASN1_END_SEQUENCE;
        ASN1_ENCODE_INTEGER(0);
        ASN1_ENCODE_INTEGER(1);
        ASN1_ENCODE_INTEGER(-1);
        ASN1_ENCODE_INTEGER(0xFF00FF);
        ASN1_ENCODE_INTEGER(-0xFF00FF);
        ASN1_ENCODE_INTEGER(INT32_MAX);
        ASN1_ENCODE_INTEGER(INT32_MIN);
        ASN1_ENCODE_INTEGER(INT64_MAX);
        ASN1_ENCODE_INTEGER(INT64_MIN);
        ASN1_ENCODE_OBJECT_ID(kOID_AttributeType_OrganizationName);
        ASN1_ENCODE_OCTET_STRING(testOctetString, sizeof(testOctetString));
        ASN1_ENCODE_OCTET_STRING(testOctetString, 1);
        ASN1_ENCODE_OCTET_STRING(testOctetString, 0);
        ASN1_ENCODE_STRING(kASN1UniversalTag_GeneralString, "", 0);
        ASN1_ENCODE_STRING(kASN1UniversalTag_PrintableString, testPrintableString, strlen(testPrintableString));
        ASN1_ENCODE_STRING(kASN1UniversalTag_UTF8String, testUTFString, strlen(testUTFString));
        ASN1_START_OCTET_STRING_ENCAPSULATED {
            ASN1_START_SEQUENCE {
                ASN1_ENCODE_OBJECT_ID(kOID_AttributeType_WeaveServiceEndpointId);
                ASN1_START_BIT_STRING_ENCAPSULATED {
                    ASN1_ENCODE_INTEGER(42);
                } ASN1_END_ENCAPSULATED;
            } ASN1_END_SEQUENCE;
        } ASN1_END_ENCAPSULATED;
    } ASN1_END_SEQUENCE;

exit:
    return err;
}

void EncodeTest()
{
    ASN1_ERROR err = ASN1_NO_ERROR;
    uint8_t buf[2048];
    ASN1Writer writer;
    uint16_t encodedLen;

    writer.Init(buf, sizeof(buf));

    err = DoEncode(writer);
    SuccessOrExit(err);

    writer.Finalize();

    encodedLen = writer.GetLengthWritten();

#define DUMP_HEX 0
#if DUMP_HEX
    for (uint16_t i = 0; i < encodedLen; i++)
    {
        if (i != 0 && i % 16 == 0)
                printf("\n");
        printf("0x%02X, ", buf[i]);
    }
    printf("\n");
#endif

#define DUMP_RAW 0
#if DUMP_RAW
    fwrite(buf, 1, encodedLen, stdout);
#endif

    if (encodedLen != sizeof(EncodeTestResult))
    {
        fprintf(stderr, "EncodeTest FAILED: length mismatch\n");
        exit(-1);
    }

    for (uint32_t i = 0; i < encodedLen; i++)
        if (buf[i] != EncodeTestResult[i])
        {
            fprintf(stderr, "EncodeTest FAILED: output mismatch (offset %d, expected = %02X, actual = %02X)\n", i, EncodeTestResult[i], buf[i]);
            exit(-1);
        }


exit:
    if (err == ASN1_NO_ERROR)
        fprintf(stdout, "EncodeTest PASSED\n");
    else
    {
        fprintf(stderr, "EncodeTest FAILED: %s\n", ErrorStr(err));
        exit(-1);
    }
}

void DecodeTest()
{
    ASN1_ERROR err = ASN1_NO_ERROR;
    ASN1Reader reader;
    bool boolVal;
    uint32_t bitStringVal;
    int64_t intVal;
    OID oidVal;

    reader.Init(EncodeTestResult, sizeof(EncodeTestResult));

    ASN1_PARSE_ENTER_SEQUENCE {
        ASN1_PARSE_BOOLEAN(boolVal); VerifyOrFail(boolVal == false, "Expected boolean false\n");
        ASN1_PARSE_BOOLEAN(boolVal); VerifyOrFail(boolVal == true, "Expected boolean true\n");
        ASN1_PARSE_ENTER_SET {
        } ASN1_EXIT_SET;
        ASN1_PARSE_BIT_STRING(bitStringVal); VerifyOrFail(bitStringVal == 0, "Expected bit string 0\n");
        ASN1_PARSE_BIT_STRING(bitStringVal); VerifyOrFail(bitStringVal == 1, "Expected bit string 1\n");
        ASN1_PARSE_BIT_STRING(bitStringVal); VerifyOrFail(bitStringVal == 3, "Expected bit string 3\n");
        ASN1_PARSE_ENTER_SEQUENCE {
            ASN1_PARSE_ENTER_SEQUENCE {
                ASN1_PARSE_ENTER_SEQUENCE {
                    ASN1_PARSE_BIT_STRING(bitStringVal); VerifyOrFail(bitStringVal == 0x17, "Expected bit string 0x17\n");
                    ASN1_PARSE_BIT_STRING(bitStringVal); VerifyOrFail(bitStringVal == 0x37, "Expected bit string 0x37\n");
                } ASN1_EXIT_SEQUENCE;
                ASN1_PARSE_BIT_STRING(bitStringVal); VerifyOrFail(bitStringVal == 0x187, "Expected bit string 0x187\n");
            } ASN1_EXIT_SEQUENCE;
            ASN1_PARSE_BIT_STRING(bitStringVal); VerifyOrFail(bitStringVal == 0x3E7, "Expected bit string 0x3E7\n");
        } ASN1_EXIT_SEQUENCE;
        ASN1_PARSE_INTEGER(intVal); VerifyOrFail(intVal == 0, "Expected integer 0\n");
        ASN1_PARSE_INTEGER(intVal); VerifyOrFail(intVal == 1, "Expected integer 1\n");
        ASN1_PARSE_INTEGER(intVal); VerifyOrFail(intVal == -1, "Expected integer -1\n");
        ASN1_PARSE_INTEGER(intVal); VerifyOrFail(intVal == 0xFF00FF, "Expected integer 0xFF00FF\n");
        ASN1_PARSE_INTEGER(intVal); VerifyOrFail(intVal == -0xFF00FF, "Expected integer -0xFF00FF\n");
        ASN1_PARSE_INTEGER(intVal); VerifyOrFail(intVal == INT32_MAX, "Expected integer INT32_MAX\n");
        ASN1_PARSE_INTEGER(intVal); VerifyOrFail(intVal == INT32_MIN, "Expected integer INT32_MIN\n");
        ASN1_PARSE_INTEGER(intVal); VerifyOrFail(intVal == INT64_MAX, "Expected integer INT64_MAX\n");
        ASN1_PARSE_INTEGER(intVal); VerifyOrFail(intVal == INT64_MIN, "Expected integer INT64_MIN\n");
        ASN1_PARSE_OBJECT_ID(oidVal); VerifyOrFail(oidVal == kOID_AttributeType_OrganizationName, "Expected object id OrganizationName\n");
        ASN1_PARSE_ELEMENT(kASN1TagClass_Universal, kASN1UniversalTag_OctetString);
        VerifyOrFail(reader.ValueLen == sizeof(testOctetString), "Expected octet string length = sizeof(testOctetString)\n");
        VerifyOrFail(memcmp(reader.Value, testOctetString, sizeof(testOctetString)) == 0, "Invalid octet string value");
        ASN1_PARSE_ELEMENT(kASN1TagClass_Universal, kASN1UniversalTag_OctetString);
        VerifyOrFail(reader.ValueLen == 1, "Expected octet string length = 1\n");
        VerifyOrFail(reader.Value[0] == testOctetString[0], "Invalid octet string value");
        ASN1_PARSE_ELEMENT(kASN1TagClass_Universal, kASN1UniversalTag_OctetString);
        VerifyOrFail(reader.ValueLen == 0, "Expected octet string length = 0\n");
        ASN1_PARSE_ELEMENT(kASN1TagClass_Universal, kASN1UniversalTag_GeneralString);
        VerifyOrFail(reader.ValueLen == 0, "Expected general string length = 0\n");
        ASN1_PARSE_ELEMENT(kASN1TagClass_Universal, kASN1UniversalTag_PrintableString);
        VerifyOrFail(reader.ValueLen == strlen(testPrintableString), "Expected printable string length = sizeof(testPrintableString)\n");
        VerifyOrFail(memcmp(reader.Value, testPrintableString, strlen(testPrintableString)) == 0, "Invalid printable string value");
        ASN1_PARSE_ELEMENT(kASN1TagClass_Universal, kASN1UniversalTag_UTF8String);
        VerifyOrFail(reader.ValueLen == strlen(testUTFString), "Expected utf8 string length = sizeof(testUTFString)\n");
        VerifyOrFail(memcmp(reader.Value, testUTFString, strlen(testUTFString)) == 0, "Invalid utf8 string value");
        ASN1_PARSE_ENTER_ENCAPSULATED(kASN1TagClass_Universal, kASN1UniversalTag_OctetString) {
            ASN1_PARSE_ENTER_SEQUENCE {
                ASN1_PARSE_OBJECT_ID(oidVal); VerifyOrFail(oidVal == kOID_AttributeType_WeaveServiceEndpointId, "Expected object id WeaveServiceEndpointId\n");
                ASN1_PARSE_ENTER_ENCAPSULATED(kASN1TagClass_Universal, kASN1UniversalTag_BitString) {
                    ASN1_PARSE_INTEGER(intVal); VerifyOrFail(intVal == 42, "Expected integer 42\n");
                } ASN1_EXIT_ENCAPSULATED;
            } ASN1_EXIT_SEQUENCE;
        } ASN1_EXIT_ENCAPSULATED;
    } ASN1_EXIT_SEQUENCE;

exit:
    if (err == ASN1_NO_ERROR)
        fprintf(stdout, "DecodeTest PASSED\n");
    else
    {
        fprintf(stderr, "DecodeTest FAILED: %s\n", ErrorStr(err));
        exit(-1);
    }
}

void NullWriterTest()
{
    ASN1_ERROR err = ASN1_NO_ERROR;
    ASN1Writer writer;
    uint16_t encodedLen;

    writer.InitNullWriter();

    err = DoEncode(writer);
    SuccessOrExit(err);

    writer.Finalize();

    encodedLen = writer.GetLengthWritten();

    if (encodedLen != 0)
    {
        fprintf(stderr, "NullWriterTest FAILED: Unexpected value from GetLengthWritten()\n");
        exit(-1);
    }

exit:
    if (err == ASN1_NO_ERROR)
        fprintf(stdout, "NullWriterTest PASSED\n");
    else
    {
        fprintf(stderr, "NullWriterTest FAILED: %s\n", ErrorStr(err));
        exit(-1);
    }
}


int main(int argc, char *argv[])
{
    EncodeTest();
    DecodeTest();
    NullWriterTest();
}
