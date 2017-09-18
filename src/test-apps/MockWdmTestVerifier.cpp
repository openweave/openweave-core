/*
 *
 *    Copyright (c) 2017 Nest Labs, Inc.
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
 *      This file implements the Checksumming and related utilities
 *      for verifying the state of a trait.
 *
 */

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif // __STDC_FORMAT_MACROS

#include <inttypes.h>

#include "MockWdmTestVerifier.h"

#include <Weave/Core/WeaveTLVDebug.hpp>
#include <Weave/Support/CodeUtils.h>

using namespace nl::Weave;
using namespace nl::Weave::TLV;
using namespace nl::Weave::Profiles::DataManagement;

static void SimpleDumpWriter(const char *aFormat, ...)
{
    va_list args;

    va_start(args, aFormat);

    vprintf(aFormat, args);

    va_end(args);
}

//Return a Standard Internet Checksum as described in RFC 1071.
//Code adapted from Section 4.0 "Implementation Examples", Subsection 4.1 "C".
//Verified correct behavior comparing to <http://ask.wireshark.org/questions/11061/icmp-checksum>
//And also comparing with <http://www.erg.abdn.ac.uk/~gorry/course/inet-pages/packet-dec2.html>
static uint16_t CalculateChecksum(uint8_t *startpos, uint8_t checklen)
{
    uint32_t sum   = 0;
    uint16_t n     = 0;
    uint16_t answer= 0;

    while (checklen > 0)
    {
        n = (uint16_t) (*startpos++);
        checklen--;
        if (checklen > 0)
        {
            n += ((uint16_t) (*startpos++))<<8;
            checklen --;
        }
        sum += n;
    }

    while (sum > 0xffff)
    {
        sum = (sum >> 16) + (sum & 0xffff);
    }
    answer = ~sum;

    return answer;
}


uint16_t ChecksumTLV(uint8_t *inBuffer, uint32_t inLen, const char *inChecksumType)
{
    nl::Weave::TLV::TLVReader reader;
    uint16_t checksum = -1;
    reader.Init(inBuffer, inLen);

    nl::Weave::TLV::Debug::Dump(reader, SimpleDumpWriter);

    checksum = CalculateChecksum(inBuffer, inLen);

    WeaveLogDetail(DataManagement, "%s trait Checksum is %04X\n", inChecksumType, checksum);

    return checksum;
}


uint16_t DumpPublisherTraitChecksum(TraitDataSource *inTraitDataSource)
{
    WEAVE_ERROR err;
    uint8_t buffer[2048] = { 0 };
    TLVType dummyContainerType;
    nl::Weave::TLV::TLVWriter writer;
    uint32_t encodedLen = 0;
    uint16_t checksum = -1;
    writer.Init(buffer, sizeof(buffer));
    err = writer.StartContainer(AnonymousTag, kTLVType_Structure, dummyContainerType);
    SuccessOrExit(err);

    err = inTraitDataSource->ReadData(kRootPropertyPathHandle, ContextTag(DataElement::kCsTag_Data), writer);
    SuccessOrExit(err);

    err = writer.EndContainer(dummyContainerType);
    SuccessOrExit(err);

    err = writer.Finalize();
    SuccessOrExit(err);

    encodedLen = writer.GetLengthWritten();
    checksum = ChecksumTLV(buffer, encodedLen, "Publisher");

exit:
    WeaveLogFunctError(err);

    return checksum;
}

uint16_t DumpClientTraitChecksum(const TraitSchemaEngine *inSchemaEngine, TraitSchemaEngine::IDataSourceDelegate *inTraitDataSource)
{
    WEAVE_ERROR err;
    uint8_t buffer[2048] = { 0 };
    TLVType dummyContainerType;
    nl::Weave::TLV::TLVWriter writer;
    uint32_t encodedLen = 0;
    uint16_t checksum = -1;

    writer.Init(buffer, sizeof(buffer));
    err = writer.StartContainer(AnonymousTag, kTLVType_Structure, dummyContainerType);
    SuccessOrExit(err);

    err = inSchemaEngine->RetrieveData(kRootPropertyPathHandle, ContextTag(DataElement::kCsTag_Data), writer, inTraitDataSource);
    SuccessOrExit(err);

    err = writer.EndContainer(dummyContainerType);
    SuccessOrExit(err);

    err = writer.Finalize();
    SuccessOrExit(err);

    encodedLen = writer.GetLengthWritten();
    checksum = ChecksumTLV(buffer, encodedLen, "Client");

exit:
    WeaveLogFunctError(err);

    return checksum;
}
