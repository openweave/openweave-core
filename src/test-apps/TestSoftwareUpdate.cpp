/*
 *
 *    Copyright (c) 2019 Google LLC.
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
 *      Unit tests for Weave network info serialization.
 *
 */

#include "ToolCommon.h"
#include <nlunit-test.h>

#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Profiles/ProfileCommon.h>
#include <Weave/Profiles/software-update/SoftwareUpdateProfile.h>

#include <SystemLayer/SystemPacketBuffer.h>

using namespace nl::Weave::TLV;
using namespace nl::Weave::Profiles::SoftwareUpdate;

void WeaveTestIntegrityTypeList(nlTestSuite * inSuite, void * inContext)
{
    WEAVE_ERROR err;
    // list is longer than the number of elements in the enum
    uint8_t list[]  = { kIntegrityType_SHA160, kIntegrityType_SHA256, kIntegrityType_SHA512, kIntegrityType_SHA160 };
    uint8_t list1[] = { kIntegrityType_SHA160, kIntegrityType_SHA160, kIntegrityType_SHA160, kIntegrityType_SHA160 };
    IntegrityTypeList testList;
    IntegrityTypeList parsedList;

    PacketBuffer * zeroSizeBuffer = PacketBuffer::New();
    zeroSizeBuffer->SetStart(zeroSizeBuffer->Start() + zeroSizeBuffer->MaxDataLength());

    PacketBuffer * lengthOnlyBuffer = PacketBuffer::New();
    lengthOnlyBuffer->SetStart(lengthOnlyBuffer->Start() + lengthOnlyBuffer->MaxDataLength() - 1);

    PacketBuffer * insufficientLengthBuffer = PacketBuffer::New();
    insufficientLengthBuffer->SetStart(insufficientLengthBuffer->Start() + insufficientLengthBuffer->MaxDataLength() - 2);

    PacketBuffer * largeBuffer = PacketBuffer::New();

    // init tests
    err = testList.init(0, NULL);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    err = testList.init(kIntegrityType_Last + 1, list);
    NL_TEST_ASSERT(inSuite, err == WEAVE_ERROR_INVALID_LIST_LENGTH);

    err = testList.init(kIntegrityType_Last, list);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    // pack test
    do
    {
        MessageIterator i(zeroSizeBuffer);
        err = testList.pack(i);
        NL_TEST_ASSERT(inSuite, err == WEAVE_ERROR_BUFFER_TOO_SMALL);
    } while (0);

    do
    {
        MessageIterator i(lengthOnlyBuffer);
        err = testList.pack(i);
        NL_TEST_ASSERT(inSuite, err == WEAVE_ERROR_BUFFER_TOO_SMALL);
    } while (0);

    do
    {
        MessageIterator i(insufficientLengthBuffer);
        err = testList.pack(i);
        NL_TEST_ASSERT(inSuite, err == WEAVE_ERROR_BUFFER_TOO_SMALL);
    } while (0);

    do
    {
        MessageIterator i(largeBuffer);
        err = testList.pack(i);
        NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    } while (0);

    // parse test

    do
    {
        MessageIterator i(zeroSizeBuffer);
        err = IntegrityTypeList::parse(i, parsedList);
        NL_TEST_ASSERT(inSuite, err == WEAVE_ERROR_BUFFER_TOO_SMALL);
    } while (0);

    do
    {
        MessageIterator i(lengthOnlyBuffer);
        uint8_t * len = lengthOnlyBuffer->Start();
        lengthOnlyBuffer->SetDataLength(1);
        *len = 0;
        err  = IntegrityTypeList::parse(i, parsedList);
        NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    } while (0);

    do
    {
        MessageIterator i(lengthOnlyBuffer);
        uint8_t * len = lengthOnlyBuffer->Start();
        lengthOnlyBuffer->SetDataLength(1);
        *len = 1;
        err  = IntegrityTypeList::parse(i, parsedList);
        NL_TEST_ASSERT(inSuite, err == WEAVE_ERROR_BUFFER_TOO_SMALL);
    } while (0);

    do
    {
        MessageIterator i(largeBuffer);
        err = IntegrityTypeList::parse(i, parsedList);
        NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    } while (0);

    do
    {
        MessageIterator i(largeBuffer);
        uint8_t * len = largeBuffer->Start();
        *len          = kIntegrityType_Last + 1;
        largeBuffer->SetDataLength(*len + 1);
        err = IntegrityTypeList::parse(i, parsedList);
        NL_TEST_ASSERT(inSuite, err == WEAVE_ERROR_INVALID_LIST_LENGTH);
    } while (0);

    // Equality tests

    testList.init(kIntegrityType_Last, list);
    parsedList.init(kIntegrityType_Last, list);
    NL_TEST_ASSERT(inSuite, testList == parsedList);

    testList.init(kIntegrityType_Last, list);
    parsedList.init(kIntegrityType_Last, list1);
    NL_TEST_ASSERT(inSuite, (testList == parsedList) == false);

    testList.init(0, NULL);
    parsedList.init(0, list);
    NL_TEST_ASSERT(inSuite, testList == parsedList);

    testList.init(kIntegrityType_Last, list);
    parsedList.init(kIntegrityType_Last - 1, list);
    NL_TEST_ASSERT(inSuite, (testList == parsedList) == false);

    PacketBuffer::Free(zeroSizeBuffer);
    PacketBuffer::Free(lengthOnlyBuffer);
    PacketBuffer::Free(insufficientLengthBuffer);
    PacketBuffer::Free(largeBuffer);
}

void WeaveTestUpdateSchemeList(nlTestSuite * inSuite, void * inContext)
{
    WEAVE_ERROR err;
    // list is longer than the number of elements in the enum
    uint8_t list[]  = { kUpdateScheme_HTTP, kUpdateScheme_HTTPS, kUpdateScheme_SFTP, kUpdateScheme_BDX, kUpdateScheme_HTTP };
    uint8_t list1[] = { kUpdateScheme_HTTP, kUpdateScheme_HTTP, kUpdateScheme_HTTP, kUpdateScheme_HTTP };
    UpdateSchemeList testList;
    UpdateSchemeList parsedList;

    PacketBuffer * zeroSizeBuffer = PacketBuffer::New();
    zeroSizeBuffer->SetStart(zeroSizeBuffer->Start() + zeroSizeBuffer->MaxDataLength());

    PacketBuffer * lengthOnlyBuffer = PacketBuffer::New();
    lengthOnlyBuffer->SetStart(lengthOnlyBuffer->Start() + lengthOnlyBuffer->MaxDataLength() - 1);

    PacketBuffer * insufficientLengthBuffer = PacketBuffer::New();
    insufficientLengthBuffer->SetStart(insufficientLengthBuffer->Start() + insufficientLengthBuffer->MaxDataLength() - 2);

    PacketBuffer * largeBuffer = PacketBuffer::New();

    // init tests
    err = testList.init(0, NULL);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    err = testList.init(kUpdateScheme_Last + 1, list);
    NL_TEST_ASSERT(inSuite, err == WEAVE_ERROR_INVALID_LIST_LENGTH);

    err = testList.init(kUpdateScheme_Last, list);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    // pack test
    do
    {
        MessageIterator i(zeroSizeBuffer);
        err = testList.pack(i);
        NL_TEST_ASSERT(inSuite, err == WEAVE_ERROR_BUFFER_TOO_SMALL);
    } while (0);

    do
    {
        MessageIterator i(lengthOnlyBuffer);
        err = testList.pack(i);
        NL_TEST_ASSERT(inSuite, err == WEAVE_ERROR_BUFFER_TOO_SMALL);
    } while (0);

    do
    {
        MessageIterator i(insufficientLengthBuffer);
        err = testList.pack(i);
        NL_TEST_ASSERT(inSuite, err == WEAVE_ERROR_BUFFER_TOO_SMALL);
    } while (0);

    do
    {
        MessageIterator i(largeBuffer);
        err = testList.pack(i);
        NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    } while (0);

    // parse test

    do
    {
        MessageIterator i(zeroSizeBuffer);
        err = UpdateSchemeList::parse(i, parsedList);
        NL_TEST_ASSERT(inSuite, err == WEAVE_ERROR_BUFFER_TOO_SMALL);
    } while (0);

    do
    {
        MessageIterator i(lengthOnlyBuffer);
        uint8_t * len = lengthOnlyBuffer->Start();
        lengthOnlyBuffer->SetDataLength(1);
        *len = 0;
        err  = UpdateSchemeList::parse(i, parsedList);
        NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    } while (0);

    do
    {
        MessageIterator i(lengthOnlyBuffer);
        uint8_t * len = lengthOnlyBuffer->Start();
        lengthOnlyBuffer->SetDataLength(1);
        *len = 1;
        err  = UpdateSchemeList::parse(i, parsedList);
        NL_TEST_ASSERT(inSuite, err == WEAVE_ERROR_BUFFER_TOO_SMALL);
    } while (0);

    do
    {
        MessageIterator i(largeBuffer);
        err = UpdateSchemeList::parse(i, parsedList);
        NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    } while (0);

    do
    {
        MessageIterator i(largeBuffer);
        uint8_t * len = largeBuffer->Start();
        *len          = kUpdateScheme_Last + 1;
        largeBuffer->SetDataLength(*len + 1);
        err = UpdateSchemeList::parse(i, parsedList);
        NL_TEST_ASSERT(inSuite, err == WEAVE_ERROR_INVALID_LIST_LENGTH);
    } while (0);

    // Equality tests

    testList.init(kUpdateScheme_Last, list);
    parsedList.init(kUpdateScheme_Last, list);
    NL_TEST_ASSERT(inSuite, testList == parsedList);

    testList.init(kUpdateScheme_Last, list);
    parsedList.init(kUpdateScheme_Last, list1);
    NL_TEST_ASSERT(inSuite, (testList == parsedList) == false);

    testList.init(0, NULL);
    parsedList.init(0, list);
    NL_TEST_ASSERT(inSuite, testList == parsedList);

    testList.init(kUpdateScheme_Last, list);
    parsedList.init(kUpdateScheme_Last - 1, list);
    NL_TEST_ASSERT(inSuite, (testList == parsedList) == false);

    PacketBuffer::Free(zeroSizeBuffer);
    PacketBuffer::Free(lengthOnlyBuffer);
    PacketBuffer::Free(insufficientLengthBuffer);
    PacketBuffer::Free(largeBuffer);
}

void WeaveTestIntegritySpec(nlTestSuite * inSuite, void * inContext)
{
    WEAVE_ERROR err;
    uint8_t validHash[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 1, 15, 16, 17, 18, 19, 20 };

    IntegritySpec testIntegritySpec;
    IntegritySpec parsedIntegritySpec;

    PacketBuffer * zeroSizeBuffer = PacketBuffer::New();
    zeroSizeBuffer->SetStart(zeroSizeBuffer->Start() + zeroSizeBuffer->MaxDataLength());

    PacketBuffer * lengthOnlyBuffer = PacketBuffer::New();
    lengthOnlyBuffer->SetStart(lengthOnlyBuffer->Start() + lengthOnlyBuffer->MaxDataLength() - 2);

    PacketBuffer * insufficientLengthBuffer = PacketBuffer::New();
    insufficientLengthBuffer->SetStart(insufficientLengthBuffer->Start() + insufficientLengthBuffer->MaxDataLength() - 6);

    PacketBuffer * largeBuffer = PacketBuffer::New();

    // init tests
    err = testIntegritySpec.init(kIntegrityType_SHA160, validHash);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    err = testIntegritySpec.init(kIntegrityType_Last + 1, validHash);
    NL_TEST_ASSERT(inSuite, err == WEAVE_ERROR_INVALID_INTEGRITY_TYPE);

    // pack tests

    do
    {
        MessageIterator i(zeroSizeBuffer);
        err = testIntegritySpec.pack(i);
        NL_TEST_ASSERT(inSuite, err == WEAVE_ERROR_BUFFER_TOO_SMALL);
    } while (0);

    do
    {
        MessageIterator i(lengthOnlyBuffer);
        err = testIntegritySpec.pack(i);
        NL_TEST_ASSERT(inSuite, err == WEAVE_ERROR_BUFFER_TOO_SMALL);
    } while (0);

    do
    {
        MessageIterator i(insufficientLengthBuffer);
        err = testIntegritySpec.pack(i);
        NL_TEST_ASSERT(inSuite, err == WEAVE_ERROR_BUFFER_TOO_SMALL);
    } while (0);

    do
    {
        MessageIterator i(largeBuffer);
        err = testIntegritySpec.pack(i);
        NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    } while (0);

    // parse tests

    do
    {
        MessageIterator i(largeBuffer);
        err = IntegritySpec::parse(i, parsedIntegritySpec);
        NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    } while (0);

    do
    {
        uint16_t originalLength = largeBuffer->DataLength();
        for (int i = 0; i < originalLength; i++)
        {
            largeBuffer->SetDataLength(i);
            MessageIterator j(largeBuffer);
            err = IntegritySpec::parse(j, parsedIntegritySpec);
            NL_TEST_ASSERT(inSuite, err == WEAVE_ERROR_BUFFER_TOO_SMALL);
        }
        largeBuffer->SetDataLength(originalLength);
    } while (0);

    do
    {
        uint8_t * integrityType = largeBuffer->Start();
        *integrityType          = kIntegrityType_Last + 1;
        MessageIterator j(largeBuffer);
        err = IntegritySpec::parse(j, parsedIntegritySpec);
        NL_TEST_ASSERT(inSuite, err == WEAVE_ERROR_INVALID_INTEGRITY_TYPE);
    } while (0);

    // equality tests
    do
    {
        largeBuffer->SetDataLength(0);
        MessageIterator i(largeBuffer);
        MessageIterator j(largeBuffer);
        err = testIntegritySpec.pack(i);
        err = IntegritySpec::parse(j, parsedIntegritySpec);
        NL_TEST_ASSERT(inSuite, testIntegritySpec == parsedIntegritySpec);
    } while (0);

    do
    {
        largeBuffer->SetDataLength(0);
        MessageIterator i(largeBuffer);
        MessageIterator j(largeBuffer);
        err                     = testIntegritySpec.pack(i);
        uint8_t * integrityType = largeBuffer->Start();
        *integrityType          = kIntegrityType_SHA256;
        err                     = IntegritySpec::parse(j, parsedIntegritySpec);
        NL_TEST_ASSERT(inSuite, (testIntegritySpec == parsedIntegritySpec) == false);
    } while (0);

    do
    {
        largeBuffer->SetDataLength(0);
        MessageIterator i(largeBuffer);
        MessageIterator j(largeBuffer);
        err               = testIntegritySpec.pack(i);
        uint8_t * sigByte = largeBuffer->Start() + 10;
        *sigByte ^= 0xFF; // flip bits in a single byte in the signature
        err = IntegritySpec::parse(j, parsedIntegritySpec);
        NL_TEST_ASSERT(inSuite, (testIntegritySpec == parsedIntegritySpec) == false);
    } while (0);

    PacketBuffer::Free(zeroSizeBuffer);
    PacketBuffer::Free(lengthOnlyBuffer);
    PacketBuffer::Free(insufficientLengthBuffer);
    PacketBuffer::Free(largeBuffer);
}

void WeaveTestProductSpec(nlTestSuite * inSuite, void * inContext)
{
    ProductSpec testDefaultSpec1;
    ProductSpec testDefaultSpec2;
    ProductSpec testSpec0(kWeaveVendor_Common, 2, 10);
    ProductSpec testSpec1(kWeaveVendor_Common, 2, 10);
    ProductSpec testSpec2(kWeaveVendor_Common, 2, 11);
    ProductSpec testSpec3(kWeaveVendor_Common, 1, 10);
    ProductSpec testSpec4(kWeaveVendor_NestLabs, 2, 10);

    NL_TEST_ASSERT(inSuite, testDefaultSpec2 == testDefaultSpec1);
    NL_TEST_ASSERT(inSuite, (testDefaultSpec2 == testSpec0) == false);
    NL_TEST_ASSERT(inSuite, testSpec0 == testSpec1);            // same data, diffent object
    NL_TEST_ASSERT(inSuite, (testSpec2 == testSpec1) == false); // productRev differs
    NL_TEST_ASSERT(inSuite, (testSpec3 == testSpec1) == false); // productId differs
    NL_TEST_ASSERT(inSuite, (testSpec4 == testSpec1) == false); // vendorId differs
}

void WeaveTestImageQuery(nlTestSuite * inSuite, void * inContext)
{
    WEAVE_ERROR err;

    // Components of a basic image query

    ProductSpec testSpec(kWeaveVendor_Common, 2, 10);
    uint8_t types[] = { kIntegrityType_SHA160 };
    IntegrityTypeList integrityTypeList;
    uint8_t schemes[] = { kUpdateScheme_HTTP, kUpdateScheme_BDX };
    UpdateSchemeList updateSchemeList;
    char packageString[] = "package!!";
    ReferencedString testPackage;
    char versionString[] = "v1.0";
    ReferencedString testVersion;
    char localeString[] = "en_AU.UTF-8";
    ReferencedString testLocale;
    uint8_t testTLVDataBytes[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    ReferencedTLVData testTLVData;
    uint64_t testNodeId            = 0x1122334455667788ULL;
    uint16_t noOptionsDataLength   = 0;
    uint16_t fullOptionsDataLength = 0;

    // initialize components of the ImageQuery
    err = integrityTypeList.init(1, types);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    err = updateSchemeList.init(2, schemes);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    err = testPackage.init(static_cast<uint8_t>(strlen(packageString)), packageString);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    err = testVersion.init(static_cast<uint8_t>(strlen(versionString)), versionString);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    err = testLocale.init(static_cast<uint8_t>(strlen(localeString)), localeString);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    err = testTLVData.init(10, 10, testTLVDataBytes);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    // Test create, pack, and parse for different options:
    // no options
    do
    {
        ImageQuery imageQuery;
        ImageQuery parsedQuery;
        PacketBuffer * buffer = PacketBuffer::New();

        err = imageQuery.init(testSpec, testVersion, integrityTypeList, updateSchemeList);
        NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

        err = imageQuery.pack(buffer);
        NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

        err = ImageQuery::parse(buffer, parsedQuery);
        NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
        NL_TEST_ASSERT(inSuite, parsedQuery == imageQuery);
        noOptionsDataLength = buffer->DataLength();

        PacketBuffer::Free(buffer);
    } while (0);

    // test locale option
    do
    {
        ImageQuery imageQuery;
        ImageQuery parsedQuery;
        PacketBuffer * buffer = PacketBuffer::New();

        err = imageQuery.init(testSpec, testVersion, integrityTypeList, updateSchemeList, NULL, &testLocale);
        NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

        err = imageQuery.pack(buffer);
        NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

        err = ImageQuery::parse(buffer, parsedQuery);
        NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
        NL_TEST_ASSERT(inSuite, parsedQuery == imageQuery);

        PacketBuffer::Free(buffer);
    } while (0);

    // test package option
    do
    {
        ImageQuery imageQuery;
        ImageQuery parsedQuery;
        PacketBuffer * buffer = PacketBuffer::New();

        err = imageQuery.init(testSpec, testVersion, integrityTypeList, updateSchemeList, &testPackage);
        NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

        err = imageQuery.pack(buffer);
        NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

        err = ImageQuery::parse(buffer, parsedQuery);
        NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
        NL_TEST_ASSERT(inSuite, parsedQuery == imageQuery);

        PacketBuffer::Free(buffer);
    } while (0);

    // test target node id
    do
    {
        ImageQuery imageQuery;
        ImageQuery parsedQuery;
        PacketBuffer * buffer = PacketBuffer::New();

        err = imageQuery.init(testSpec, testVersion, integrityTypeList, updateSchemeList, NULL, NULL, testNodeId);
        NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

        err = imageQuery.pack(buffer);
        NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

        err = ImageQuery::parse(buffer, parsedQuery);
        NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
        NL_TEST_ASSERT(inSuite, parsedQuery == imageQuery);

        PacketBuffer::Free(buffer);
    } while (0);

    // test TLV
    do
    {
        ImageQuery imageQuery;
        ImageQuery parsedQuery;
        PacketBuffer * buffer = PacketBuffer::New();

        err = imageQuery.init(testSpec, testVersion, integrityTypeList, updateSchemeList, NULL, NULL, 0, &testTLVData);
        NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

        err = imageQuery.pack(buffer);
        NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

        err = ImageQuery::parse(buffer, parsedQuery);
        NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
        NL_TEST_ASSERT(inSuite, parsedQuery == imageQuery);

        PacketBuffer::Free(buffer);
    } while (0);
    // test all options
    do
    {
        ImageQuery imageQuery;
        ImageQuery parsedQuery;
        PacketBuffer * buffer = PacketBuffer::New();

        err = imageQuery.init(testSpec, testVersion, integrityTypeList, updateSchemeList, &testPackage, &testLocale, testNodeId,
                              &testTLVData);
        NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

        err = imageQuery.pack(buffer);
        NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

        err = ImageQuery::parse(buffer, parsedQuery);
        NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
        NL_TEST_ASSERT(inSuite, parsedQuery == imageQuery);
        fullOptionsDataLength = buffer->DataLength();

        PacketBuffer::Free(buffer);
    } while (0);

    // test packing errors
    // no options
    do
    {
        ImageQuery imageQuery;
        PacketBuffer * buffer = PacketBuffer::New();
        uint8_t * end         = buffer->Start() + buffer->MaxDataLength();

        err = imageQuery.init(testSpec, testVersion, integrityTypeList, updateSchemeList);
        NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

        for (int i = 0; i < noOptionsDataLength; i++)
        {
            buffer->SetStart(end - i);
            buffer->SetDataLength(0);
            err = imageQuery.pack(buffer);
            NL_TEST_ASSERT(inSuite, err == WEAVE_ERROR_BUFFER_TOO_SMALL);
        }
        PacketBuffer::Free(buffer);
    } while (0);

    // full options
    do
    {
        ImageQuery imageQuery;
        PacketBuffer * buffer = PacketBuffer::New();
        uint8_t * end         = buffer->Start() + buffer->MaxDataLength();

        err = imageQuery.init(testSpec, testVersion, integrityTypeList, updateSchemeList, &testPackage, &testLocale, testNodeId,
                              &testTLVData);
        NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

        for (int i = 0; i < fullOptionsDataLength; i++)
        {
            buffer->SetStart(end - i);
            buffer->SetDataLength(0);
            err = imageQuery.pack(buffer);
            NL_TEST_ASSERT(inSuite, err == WEAVE_ERROR_BUFFER_TOO_SMALL);
        }
        PacketBuffer::Free(buffer);
    } while (0);

    // test parsing errors: short buffer
    // no options
    do
    {
        ImageQuery imageQuery;
        ImageQuery parsedQuery;
        PacketBuffer * buffer = PacketBuffer::New();

        err = imageQuery.init(testSpec, testVersion, integrityTypeList, updateSchemeList);
        NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

        err = imageQuery.pack(buffer);
        NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

        for (int i = 0; i < noOptionsDataLength; i++)
        {
            buffer->SetDataLength(i);
            err = ImageQuery::parse(buffer, parsedQuery);
            NL_TEST_ASSERT(inSuite, err == WEAVE_ERROR_BUFFER_TOO_SMALL);
        }
        PacketBuffer::Free(buffer);
    } while (0);

    // full options
    do
    {
        ImageQuery imageQuery;
        ImageQuery parsedQuery;
        PacketBuffer * buffer = PacketBuffer::New();

        err = imageQuery.init(testSpec, testVersion, integrityTypeList, updateSchemeList, &testPackage, &testLocale, testNodeId);
        NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

        err = imageQuery.pack(buffer);
        NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
        fullOptionsDataLength = buffer->DataLength();

        for (int i = 0; i < fullOptionsDataLength; i++)
        {
            buffer->SetDataLength(i);
            err = ImageQuery::parse(buffer, parsedQuery);
            NL_TEST_ASSERT(inSuite, err == WEAVE_ERROR_BUFFER_TOO_SMALL);
        }
        PacketBuffer::Free(buffer);
    } while (0);
}

void WeaveTestImageQueryResponse(nlTestSuite * inSuite, void * inContext)
{
    WEAVE_ERROR err;

    // Components of a basic image query response
    char uriString[] = "http://www.openweave.io";
    ReferencedString testUri;
    uint8_t testSHA160Hash[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 1, 15, 16, 17, 18, 19, 20 };
    IntegritySpec testIntegritySpec;
    ImageQueryResponse testResponse;
    ImageQueryResponse parsedResponse;
    char versionString[] = "v1.0";
    ReferencedString testVersion;

    err = testVersion.init(static_cast<uint8_t>(strlen(versionString)), versionString);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    err = testUri.init(static_cast<uint16_t>(strlen(uriString)), uriString);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    err = testIntegritySpec.init(kIntegrityType_SHA160, testSHA160Hash);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    err = testResponse.init(testUri, testVersion, testIntegritySpec, kUpdateScheme_HTTPS, Critical, IfLater, true);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    // pack and parse valid response
    do
    {
        PacketBuffer * buffer = PacketBuffer::New();

        err = testResponse.pack(buffer);
        NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

        err = ImageQueryResponse::parse(buffer, parsedResponse);
        NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
        NL_TEST_ASSERT(inSuite, parsedResponse == testResponse);

        PacketBuffer::Free(buffer);
    } while (0);

    // pack error handling
    do
    {
        PacketBuffer * buffer = PacketBuffer::New();
        uint8_t * end         = buffer->Start() + buffer->MaxDataLength();
        uint16_t packedLength;

        err = testResponse.pack(buffer);
        NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
        packedLength = buffer->DataLength();

        for (uint16_t i = 0; i < packedLength; i++)
        {
            buffer->SetStart(end - i);
            buffer->SetDataLength(0);
            err = testResponse.pack(buffer);
            NL_TEST_ASSERT(inSuite, err == WEAVE_ERROR_BUFFER_TOO_SMALL);
        }
        PacketBuffer::Free(buffer);
    } while (0);

    // parse incomplete packets
    do
    {
        PacketBuffer * buffer = PacketBuffer::New();
        uint16_t dataLen;
        err = testResponse.pack(buffer);
        NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

        dataLen = buffer->DataLength();
        for (int i = 0; i < dataLen; i++)
        {
            buffer->SetDataLength(i);
            err = ImageQueryResponse::parse(buffer, parsedResponse);
            NL_TEST_ASSERT(inSuite, err == WEAVE_ERROR_BUFFER_TOO_SMALL);
        }
        PacketBuffer::Free(buffer);
    } while (0);
}

/**
 *  Test Suite that lists all the test functions.
 */
static const nlTest sTests[] = { NL_TEST_DEF("Test IntergrityTypeList", WeaveTestIntegrityTypeList),
                                 NL_TEST_DEF("Test UpdateSchemeList", WeaveTestUpdateSchemeList),
                                 NL_TEST_DEF("Test IntegritySpec", WeaveTestIntegritySpec),
                                 NL_TEST_DEF("Test ProductSpec", WeaveTestProductSpec),
                                 NL_TEST_DEF("Test ImageQuery", WeaveTestImageQuery),
                                 NL_TEST_DEF("Test ImageQueryResponse", WeaveTestImageQueryResponse),
                                 NL_TEST_SENTINEL() };

/**
 *  Set up the test suite.
 */
static int TestSetup(void * inContext)
{
#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    tcpip_init(NULL, NULL);
#endif
    return (SUCCESS);
}

/**
 *  Tear down the test suite.
 */
static int TestTeardown(void * inContext)
{
    return (SUCCESS);
}

int main(int argc, char * argv[])
{
    nlTestSuite theSuite = { "network-info", &sTests[0], TestSetup, TestTeardown };

    // Generate machine-readable, comma-separated value (CSV) output.
    nl_test_set_output_style(OUTPUT_CSV);

    // Run test suit against one context
    nlTestRunner(&theSuite, NULL);

    return nlTestRunnerStats(&theSuite);
}
