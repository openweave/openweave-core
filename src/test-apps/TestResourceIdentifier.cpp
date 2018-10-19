/*
 *
 *    Copyright (c) 2018 Google LLC.
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
 *      This file implements unit tests for the Weave TLV implementation.
 *
 */

#include "ToolCommon.h"

#include <nlbyteorder.h>
#include <nlunit-test.h>

#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Core/WeaveTLVDebug.hpp>
#include <Weave/Core/WeaveTLVUtilities.hpp>
#include <Weave/Core/WeaveTLVData.hpp>
#include <Weave/Core/WeaveCircularTLVBuffer.h>
#include <Weave/Support/RandUtils.h>
#include <Weave/Support/ErrorStr.h>

#include <Weave/Profiles/data-management/Current/WdmManagedNamespace.h>
#include <Weave/Profiles/data-management/DataManagement.h>

using namespace nl;
using namespace nl::Weave::TLV;
using namespace nl::Weave::Profiles::DataManagement;

static void CheckDefaultConstructor(nlTestSuite * inSuite, void * inContext);
static void CheckU64Constructor(nlTestSuite * inSuite, void * inContext);
static void CheckTypeU64Constructor(nlTestSuite * inSuite, void * inContext);

static void CheckByteArrayConstructor(nlTestSuite * inSuite, void * inContext);
static void CheckTLVSerDes(nlTestSuite * inSuite, void * inContext);
static void CheckStringSerDes(nlTestSuite * inSuite, void * inContext);
static void CheckTLVDecodingErrors(nlTestSuite * inSuite, void * inContext);
static void CheckEncoding(nlTestSuite * inSuite, ResourceIdentifier resource, const uint8_t * refBuffer, size_t refBufferLen,
                          uint8_t * buffer, size_t bufferLen);
static void CheckEncoding(nlTestSuite * inSuite, ResourceIdentifier resource, uint64_t tag, const uint8_t * refBuffer,
                          size_t refBufferLen, uint8_t * buffer, size_t bufferLen);
static void CheckDecoding(nlTestSuite * inSuite, const uint8_t * buffer, size_t bufferLen, ResourceIdentifier reference);
static void CheckDecoding(nlTestSuite * inSuite, const uint8_t * buffer, size_t bufferLen, uint64_t nodeid,
                          ResourceIdentifier reference);

static void CheckDefaultConstructor(nlTestSuite * inSuite, void * inContext)
{
    ResourceIdentifier resource;
    ResourceIdentifier resource1;
    ResourceIdentifier resource_self(ResourceIdentifier::SELF_NODE_ID);

    // Unspecified Node ID

    NL_TEST_ASSERT(inSuite, resource.GetResourceId() == kNodeIdNotSpecified);
    NL_TEST_ASSERT(inSuite, resource.GetResourceType() == ResourceIdentifier::RESOURCE_TYPE_RESERVED);
    // it is different than a self node ID
    NL_TEST_ASSERT(inSuite, resource != resource_self);
    // it is equal to other uninitialized resources
    NL_TEST_ASSERT(inSuite, resource == resource1);
}

static void CheckU64Constructor(nlTestSuite * inSuite, void * inContext)
{
    ResourceIdentifier resource(0x18b4300000000001ULL);
    ResourceIdentifier resource1(0x18b4300000000001ULL);
    ResourceIdentifier resource2(1ULL);
    ResourceIdentifier resource_self(ResourceIdentifier::SELF_NODE_ID);
    const char resource_type_string[] = "DEVICE";

    NL_TEST_ASSERT(inSuite, resource.GetResourceId() == 0x18b4300000000001ULL);
    NL_TEST_ASSERT(inSuite, resource.GetResourceType() == Schema::Weave::Common::RESOURCE_TYPE_DEVICE);
    NL_TEST_ASSERT(inSuite, strncmp(resource.ResourceTypeAsString(), resource_type_string, sizeof(resource_type_string)) == 0);

    // it is equal to another resource initialized in the same manner
    NL_TEST_ASSERT(inSuite, resource == resource1);

    // it is different than a self node ID
    NL_TEST_ASSERT(inSuite, resource != resource_self);
}

static void CheckTypeU64Constructor(nlTestSuite * inSuite, void * inContext)
{
    ResourceIdentifier resource(Schema::Weave::Common::RESOURCE_TYPE_DEVICE, 0x18b4300000000001ULL);
    ResourceIdentifier resource1(0x18b4300000000001ULL);
    ResourceIdentifier resource2(Schema::Weave::Common::RESOURCE_TYPE_USER, 0x18b4300000000001ULL);
    ResourceIdentifier resource_self(ResourceIdentifier::SELF_NODE_ID);

    NL_TEST_ASSERT(inSuite, resource.GetResourceId() == 0x18b4300000000001ULL);
    NL_TEST_ASSERT(inSuite, resource.GetResourceType() == Schema::Weave::Common::RESOURCE_TYPE_DEVICE);

    // it is equal to another resource initialized just by u64
    NL_TEST_ASSERT(inSuite, resource == resource1);

    // it is different than a self node ID
    NL_TEST_ASSERT(inSuite, resource != resource_self);
    // it is different from a resource of the same ID with a different type
    NL_TEST_ASSERT(inSuite, resource != resource2);
}

static void CheckByteArrayConstructor(nlTestSuite * inSuite, void * inContext)
{
    uint64_t id1        = 0x18b4300000000001ULL;
    const uint8_t id2[] = { 0x01, 0x00, 0x00, 0x00, 0x00, 0x30, 0xb4, 0x18 };
    ResourceIdentifier resource(0x18b4300000000001ULL);
    ResourceIdentifier resource1(Schema::Weave::Common::RESOURCE_TYPE_DEVICE, (uint8_t *) &id1, sizeof(id1));
    ResourceIdentifier resource2(Schema::Weave::Common::RESOURCE_TYPE_DEVICE, id2, sizeof(id2));
}

static void CheckTLVSerDes(nlTestSuite * inSuite, void * inContext)
{
    ResourceIdentifier resource(Schema::Weave::Common::RESOURCE_TYPE_DEVICE, 0x18b4300000000001ULL);
    ResourceIdentifier resource1(0x18b4300000000001ULL);
    ResourceIdentifier resource2(Schema::Weave::Common::RESOURCE_TYPE_USER, 0x18b4300000000001ULL);
    ResourceIdentifier resource3(0x1ULL);
    ResourceIdentifier resource_self(ResourceIdentifier::SELF_NODE_ID);
    ResourceIdentifier resource_unknown_type(0xC001, 0x18b4300000000001ULL);
    ResourceIdentifier resource_uninitialized;

    const uint8_t resource_tlv[] = { nlWeaveTLV_STRUCTURE(nlWeaveTLV_TAG_ANONYMOUS),
                                     nlWeaveTLV_BYTE_STRING_1ByteLength(nlWeaveTLV_TAG_CONTEXT_SPECIFIC(Path::kCsTag_ResourceID),
                                                                        // length:
                                                                        10,
                                                                        // type
                                                                        0x01, 0x00,
                                                                        // ID in LE order:
                                                                        0x01, 0x00, 0x00, 0x00, 0x00, 0x30, 0xb4, 0x18),
                                     nlWeaveTLV_END_OF_CONTAINER };

    const uint8_t resource_tag_tlv[] = { nlWeaveTLV_STRUCTURE(nlWeaveTLV_TAG_ANONYMOUS),
                                         nlWeaveTLV_BYTE_STRING_1ByteLength(nlWeaveTLV_TAG_CONTEXT_SPECIFIC(kTag_EventResourceID),
                                                                            // length:
                                                                            10,
                                                                            // type
                                                                            0x01, 0x00,
                                                                            // ID in LE order:
                                                                            0x01, 0x00, 0x00, 0x00, 0x00, 0x30, 0xb4, 0x18),
                                         nlWeaveTLV_END_OF_CONTAINER };

    const uint8_t resource1_tlv[] = { nlWeaveTLV_STRUCTURE(nlWeaveTLV_TAG_ANONYMOUS),
                                      nlWeaveTLV_UINT64(nlWeaveTLV_TAG_CONTEXT_SPECIFIC(Path::kCsTag_ResourceID),
                                                        0x18b4300000000001ULL),
                                      nlWeaveTLV_END_OF_CONTAINER };

    const uint8_t resource1_tag_tlv[] = {
        nlWeaveTLV_STRUCTURE(nlWeaveTLV_TAG_ANONYMOUS),
        nlWeaveTLV_UINT64(nlWeaveTLV_TAG_CONTEXT_SPECIFIC(kTag_EventResourceID), 0x18b4300000000001ULL), nlWeaveTLV_END_OF_CONTAINER
    };

    const uint8_t resource2_tlv[] = { nlWeaveTLV_STRUCTURE(nlWeaveTLV_TAG_ANONYMOUS),
                                      nlWeaveTLV_BYTE_STRING_1ByteLength(nlWeaveTLV_TAG_CONTEXT_SPECIFIC(Path::kCsTag_ResourceID),
                                                                         // length:
                                                                         10,
                                                                         // type
                                                                         0x02, 0x00,
                                                                         // ID in LE order:
                                                                         0x01, 0x00, 0x00, 0x00, 0x00, 0x30, 0xb4, 0x18),
                                      nlWeaveTLV_END_OF_CONTAINER };

    const uint8_t resource_self_tlv[] = { nlWeaveTLV_STRUCTURE(nlWeaveTLV_TAG_ANONYMOUS), nlWeaveTLV_END_OF_CONTAINER };

    const uint8_t resource3_tlv[] = { nlWeaveTLV_STRUCTURE(nlWeaveTLV_TAG_ANONYMOUS),
                                      nlWeaveTLV_UINT8(nlWeaveTLV_TAG_CONTEXT_SPECIFIC(Path::kCsTag_ResourceID), 0x1),
                                      nlWeaveTLV_END_OF_CONTAINER };

    const uint8_t resource_unknown_type_tlv[] = { nlWeaveTLV_STRUCTURE(nlWeaveTLV_TAG_ANONYMOUS),
                                                  nlWeaveTLV_BYTE_STRING_1ByteLength(
                                                      nlWeaveTLV_TAG_CONTEXT_SPECIFIC(Path::kCsTag_ResourceID),
                                                      // length:
                                                      10,
                                                      // type
                                                      0x01, 0xc0,
                                                      // ID in LE order:
                                                      0x01, 0x00, 0x00, 0x00, 0x00, 0x30, 0xb4, 0x18),
                                                  nlWeaveTLV_END_OF_CONTAINER };

    uint8_t buffer[1024];

    // ENCODING

    // resource gets encoded as resource1_tlv
    CheckEncoding(inSuite, resource, resource1_tlv, sizeof(resource1_tlv), buffer, sizeof(buffer));
    // resource1 gets encoded as resource1_tlv
    CheckEncoding(inSuite, resource1, resource1_tlv, sizeof(resource1_tlv), buffer, sizeof(buffer));
    // resource2 gets encoded as resource2
    CheckEncoding(inSuite, resource2, resource2_tlv, sizeof(resource2_tlv), buffer, sizeof(buffer));
    // resource_unknown_type gets encoded as resource_unknown_type_tlv
    CheckEncoding(inSuite, resource_unknown_type, resource_unknown_type_tlv, sizeof(resource_unknown_type_tlv), buffer,
                  sizeof(buffer));
    // resource_self gets encoded as resource_self_tlv
    CheckEncoding(inSuite, resource_self, resource_self_tlv, sizeof(resource_self_tlv), buffer, sizeof(buffer));

    // encoding with a different tag
    CheckEncoding(inSuite, resource, ContextTag(kTag_EventResourceID), resource1_tag_tlv, sizeof(resource1_tag_tlv), buffer,
                  sizeof(buffer));

    // DECODING
    CheckDecoding(inSuite, resource_tlv, sizeof(resource_tlv), resource);
    CheckDecoding(inSuite, resource_tag_tlv, sizeof(resource_tag_tlv), resource);
    CheckDecoding(inSuite, resource1_tlv, sizeof(resource1_tlv), resource1);
    CheckDecoding(inSuite, resource2_tlv, sizeof(resource2_tlv), resource2);
    CheckDecoding(inSuite, resource3_tlv, sizeof(resource3_tlv), resource3);
    CheckDecoding(inSuite, resource_unknown_type_tlv, sizeof(resource_unknown_type_tlv), resource_unknown_type);

    // mapping onto self
    CheckDecoding(inSuite, resource_tlv, sizeof(resource_tlv), 0x18b4300000000001ULL, resource_self);
    CheckDecoding(inSuite, resource_tag_tlv, sizeof(resource_tag_tlv), 0x18b4300000000001ULL, resource_self);
    CheckDecoding(inSuite, resource1_tlv, sizeof(resource1_tlv), 0x18b4300000000001ULL, resource_self);
    // not a device type, does not get remapped
    CheckDecoding(inSuite, resource2_tlv, sizeof(resource2_tlv), 0x18b4300000000001ULL, resource2);
    // remapping a short ID
    CheckDecoding(inSuite, resource3_tlv, sizeof(resource3_tlv), 0x1ULL, resource_self);
    // not a device type, does not get remapped
    CheckDecoding(inSuite, resource_unknown_type_tlv, sizeof(resource_unknown_type_tlv), 0x18b4300000000001ULL,
                  resource_unknown_type);
}

static void CheckTLVDecodingErrors(nlTestSuite * inSuite, void * inContext)
{
    const uint8_t resource_wrong_type_tlv[] = { nlWeaveTLV_STRUCTURE(nlWeaveTLV_TAG_ANONYMOUS),
                                                nlWeaveTLV_STRUCTURE(nlWeaveTLV_TAG_CONTEXT_SPECIFIC(Path::kCsTag_ResourceID)),
                                                nlWeaveTLV_UINT16(nlWeaveTLV_TAG_CONTEXT_SPECIFIC(1), 1),
                                                nlWeaveTLV_UINT64(nlWeaveTLV_TAG_CONTEXT_SPECIFIC(2), 0x18b4300000000001ULL),
                                                nlWeaveTLV_END_OF_CONTAINER,
                                                nlWeaveTLV_END_OF_CONTAINER };

    const uint8_t resource_wrong_byte_array_length_tlv[] = { nlWeaveTLV_STRUCTURE(nlWeaveTLV_TAG_ANONYMOUS),
                                                             nlWeaveTLV_BYTE_STRING_1ByteLength(
                                                                 nlWeaveTLV_TAG_CONTEXT_SPECIFIC(kTag_EventResourceID),
                                                                 // length:
                                                                 8,
                                                                 // ID in LE order:
                                                                 0x01, 0x00, 0x00, 0x00, 0x00, 0x30, 0xb4, 0x18),
                                                             nlWeaveTLV_END_OF_CONTAINER };
    nl::Weave::TLV::TLVReader reader;
    TLVType outer;
    WEAVE_ERROR err;
    ResourceIdentifier resource;

    // Decode a bogus encoding
    reader.Init(resource_wrong_type_tlv, sizeof(resource_wrong_type_tlv));
    reader.Next();
    reader.EnterContainer(outer);
    reader.Next();
    err = resource.FromTLV(reader);
    NL_TEST_ASSERT(inSuite, err == WEAVE_ERROR_WRONG_TLV_TYPE);

    // Decode a bogus encoding
    reader.Init(resource_wrong_byte_array_length_tlv, sizeof(resource_wrong_byte_array_length_tlv));
    reader.Next();
    reader.EnterContainer(outer);
    reader.Next();
    err = resource.FromTLV(reader);
    NL_TEST_ASSERT(inSuite, err == WEAVE_ERROR_WRONG_TLV_TYPE);
}

static void CheckDecoding(nlTestSuite * inSuite, const uint8_t * buffer, size_t bufferLen, ResourceIdentifier reference)
{
    nl::Weave::TLV::TLVReader reader;
    TLVType outer;
    WEAVE_ERROR err;
    ResourceIdentifier resource;
#if DEBUG_PRINT_ENABLE
    char buf[ResourceIdentifier::MAX_STRING_LENGTH];
#endif

    reader.Init(buffer, bufferLen);
    reader.Next();
    reader.EnterContainer(outer);
    reader.Next();
    err = resource.FromTLV(reader);
#if DEBUG_PRINT_ENABLE
    printf("ERR: %s\n", ErrorStr(err));
    printf("Buffer:\n");
    for (size_t i = 0; i < bufferLen; i++)
    {
        printf("%02x ", buffer[i]);
    }
    printf("\n");
    resource.ToString(buf, sizeof(buf));
    printf("ResourceId: %s\n", buf);
#endif
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    NL_TEST_ASSERT(inSuite, resource == reference);
}

static void CheckDecoding(nlTestSuite * inSuite, const uint8_t * buffer, size_t bufferLen, uint64_t nodeid,
                          ResourceIdentifier reference)
{
    nl::Weave::TLV::TLVReader reader;
    TLVType outer;
    WEAVE_ERROR err;
    ResourceIdentifier resource;
#if DEBUG_PRINT_ENABLE
    char buf[ResourceIdentifier::MAX_STRING_LENGTH];
#endif

    reader.Init(buffer, bufferLen);
    reader.Next();
    reader.EnterContainer(outer);
    reader.Next();
    err = resource.FromTLV(reader, nodeid);

#if DEBUG_PRINT_ENABLE
    printf("ERR: %s\n", ErrorStr(err));
    printf("Buffer:\n");
    for (size_t i = 0; i < bufferLen; i++)
    {
        printf("%02x ", buffer[i]);
    }
    printf("\n");
    resource.ToString(buf, sizeof(buf));
    printf("ResourceId: %s\n", buf);
#endif

    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    NL_TEST_ASSERT(inSuite, resource == reference);
}

static void CheckEncoding(nlTestSuite * inSuite, ResourceIdentifier resource, const uint8_t * refBuffer, size_t refBufferLen,
                          uint8_t * buffer, size_t bufferLen)
{
    nl::Weave::TLV::TLVWriter writer;
    WEAVE_ERROR err;
    TLVType outer;

    writer.Init(buffer, bufferLen);
    writer.StartContainer(AnonymousTag, kTLVType_Structure, outer);
    err = resource.ToTLV(writer);
    writer.EndContainer(outer);
    writer.Finalize();

#if DEBUG_PRINT_ENABLE
    printf("ERR: %s\n", ErrorStr(err));
    printf("Wrote: %" PRIu32 " bytes, Expected: %zu bytes\n", writer.GetLengthWritten(), refBufferLen);
    printf("Wrote:\n");
    for (size_t i = 0; i < refBufferLen; i++)
    {
        printf("%02x ", buffer[i]);
    }
    printf("\n");
    printf("Expected:\n");
    for (size_t i = 0; i < refBufferLen; i++)
    {
        printf("%02x ", refBuffer[i]);
    }
    printf("\n");
#endif
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    NL_TEST_ASSERT(inSuite, writer.GetLengthWritten() == refBufferLen);
    NL_TEST_ASSERT(inSuite, memcmp(buffer, refBuffer, refBufferLen) == 0);
}

static void CheckEncoding(nlTestSuite * inSuite, ResourceIdentifier resource, uint64_t tag, const uint8_t * refBuffer,
                          size_t refBufferLen, uint8_t * buffer, size_t bufferLen)
{
    nl::Weave::TLV::TLVWriter writer;
    WEAVE_ERROR err;
    TLVType outer;

    writer.Init(buffer, bufferLen);
    writer.StartContainer(AnonymousTag, kTLVType_Structure, outer);
    err = resource.ToTLV(writer, tag);
    writer.EndContainer(outer);
    writer.Finalize();

#if DEBUG_PRINT_ENABLE
    printf("ERR: %s\n", ErrorStr(err));
    printf("Wrote: %" PRIu32 ", Expected: %zu\n", writer.GetLengthWritten(), refBufferLen);
    printf("Wrote:\n");
    for (size_t i = 0; i < refBufferLen; i++)
    {
        printf("%02x ", buffer[i]);
    }
    printf("\n");
    printf("Expected:\n");
    for (size_t i = 0; i < refBufferLen; i++)
    {
        printf("%02x ", refBuffer[i]);
    }
    printf("\n");
#endif
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    NL_TEST_ASSERT(inSuite, writer.GetLengthWritten() == refBufferLen);
    NL_TEST_ASSERT(inSuite, memcmp(buffer, refBuffer, refBufferLen) == 0);
}

static void CheckStringSerDes(nlTestSuite * inSuite, void * inContext)
{
    ResourceIdentifier resource(Schema::Weave::Common::RESOURCE_TYPE_DEVICE, 0x18b4300000000001ULL);
    ResourceIdentifier resource1(0x18b4300000000001ULL);
    ResourceIdentifier resource2(Schema::Weave::Common::RESOURCE_TYPE_USER, 0x18b4300000000001ULL);
    ResourceIdentifier resource_self(ResourceIdentifier::SELF_NODE_ID);
    ResourceIdentifier resource_unknown_type(0xC001, 0x18b4300000000001ULL);
    ResourceIdentifier resource_uninitialized;

    const char * resource_str               = "DEVICE_18B4300000000001";
    const char * resource2_str              = "USER_18B4300000000001";
    const char * resource_self_str          = "RESERVED_DEVICE_SELF";
    const char * resource_uninitialized_str = "RESERVED_NOT_SPECIFIED";
    const char * resource_unknown_type_str  = "(C001)_18B4300000000001";
    const char * resource_unknown_str       = "WIDGET_18B4300000000001";

    char resource_buf[ResourceIdentifier::MAX_STRING_LENGTH];
    WEAVE_ERROR err;

    // To string conversions
    resource.ToString(resource_buf, sizeof(resource_buf));
    NL_TEST_ASSERT(inSuite, strcmp(resource_buf, resource_str) == 0);

    resource1.ToString(resource_buf, sizeof(resource_buf));
    NL_TEST_ASSERT(inSuite, strcmp(resource_buf, resource_str) == 0);

    resource2.ToString(resource_buf, sizeof(resource_buf));
    NL_TEST_ASSERT(inSuite, strcmp(resource_buf, resource2_str) == 0);

    resource_self.ToString(resource_buf, sizeof(resource_buf));
    NL_TEST_ASSERT(inSuite, strcmp(resource_buf, resource_self_str) == 0);

    resource_uninitialized.ToString(resource_buf, sizeof(resource_buf));
    NL_TEST_ASSERT(inSuite, strcmp(resource_buf, resource_uninitialized_str) == 0);

    resource_unknown_type.ToString(resource_buf, sizeof(resource_buf));
    NL_TEST_ASSERT(inSuite, strcmp(resource_buf, resource_unknown_type_str) == 0);

    // From string conversions

    err = resource_uninitialized.FromString(resource_str, sizeof(resource_str));
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    NL_TEST_ASSERT(inSuite, resource_uninitialized == resource);
    NL_TEST_ASSERT(inSuite, resource_uninitialized != resource_self);

    // verify we map onto self node id
    err = resource_uninitialized.FromString(resource_str, sizeof(resource_str), 0x18b4300000000001ULL);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    NL_TEST_ASSERT(inSuite, resource_uninitialized != resource);
    NL_TEST_ASSERT(inSuite, resource_uninitialized == resource_self);

    err = resource_uninitialized.FromString(resource2_str, sizeof(resource2_str));
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    NL_TEST_ASSERT(inSuite, resource_uninitialized == resource2);

    // Verify errors

    err = resource_uninitialized.FromString(resource_uninitialized_str, sizeof(resource_uninitialized_str));
    NL_TEST_ASSERT(inSuite, err == WEAVE_ERROR_INVALID_ARGUMENT);

    err = resource_uninitialized.FromString(resource_self_str, sizeof(resource_self_str));
    NL_TEST_ASSERT(inSuite, err == WEAVE_ERROR_INVALID_ARGUMENT);

    err = resource_uninitialized.FromString(resource_unknown_type_str, sizeof(resource_unknown_type_str));
    NL_TEST_ASSERT(inSuite, err == WEAVE_ERROR_INVALID_ARGUMENT);

    err = resource_uninitialized.FromString(resource_unknown_str, sizeof(resource_unknown_str));
    NL_TEST_ASSERT(inSuite, err == WEAVE_ERROR_INVALID_ARGUMENT);
}

static int TestSetup(void * inContext)
{
    return (SUCCESS);
}

static int TestTeardown(void * inContext)
{

    return (SUCCESS);
}

static const nlTest sTests[] = {
    NL_TEST_DEF("Test ResourceIdentifier -- default constructor", CheckDefaultConstructor),
    NL_TEST_DEF("Test ResourceIdentifier -- u64 constructor", CheckU64Constructor),
    NL_TEST_DEF("Test ResourceIdentifier -- Type + u64 constructor", CheckTypeU64Constructor),
    NL_TEST_DEF("Test ResourceIdentifier -- Type + byte array constructor", CheckByteArrayConstructor),
    NL_TEST_DEF("Test ResourceIdentifier -- string conversions", CheckStringSerDes),
    NL_TEST_DEF("Test ResourceIdentifier -- TLV conversions", CheckTLVSerDes),
    NL_TEST_DEF("Test ResourceIdentifier -- erroneous TLV", CheckTLVDecodingErrors),
    NL_TEST_SENTINEL(),
};

/**
 *  Main
 */
int main(int argc, char * argv[])
{
    nlTestSuite theSuite = { "weave-resource-identifier", &sTests[0], TestSetup, TestTeardown };

    // Generate machine-readable, comma-separated value (CSV) output.
    nl_test_set_output_style(OUTPUT_CSV);

    // Run test suit against one context
    nlTestRunner(&theSuite, NULL);

    return nlTestRunnerStats(&theSuite);
}
