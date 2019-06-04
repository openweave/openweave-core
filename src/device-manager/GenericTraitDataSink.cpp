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
 *      Generic trait data sinks.
 */

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#include <stdint.h>
#include <inttypes.h>

// Note that the choice of namespace alias must be made up front for each and every compile unit
// This is because many include paths could set the default alias to unintended target.

#include <Weave/Profiles/data-management/Current/WdmManagedNamespace.h>
#include <Weave/Support/CodeUtils.h>
#include "GenericTraitDataSink.h"

using namespace ::nl::Weave;
using namespace ::nl::Weave::TLV;
using namespace ::nl::Weave::Profiles::DataManagement;
using namespace Schema::Weave::Common;

GenericTraitDataSink::GenericTraitDataSink(const nl::Weave::Profiles::DataManagement::TraitSchemaEngine * aEngine)
        : TraitUpdatableDataSink(aEngine)
{
}

GenericTraitDataSink::~GenericTraitDataSink()
{
    for (auto itr = mpathLeafTlvMap.begin(); itr != mpathLeafTlvMap.end(); ++itr)
    {
        if (NULL != itr->second)
        {
            PacketBuffer::Free(itr->second);
            mpathLeafTlvMap.erase(itr->first);
        }
    }
}

WEAVE_ERROR
GenericTraitDataSink::SetLeaf(PropertyPathHandle aLeafHandle, char *buf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    nl::Weave::TLV::TLVWriter writer;
    nl::Weave::TLV::TLVReader reader;
    PacketBuffer *msgBuf = PacketBuffer::New();
    VerifyOrExit(NULL != msgBuf, err = WEAVE_ERROR_NO_MEMORY);
    writer.Init(msgBuf);
    WeaveLogDetail(DataManagement, "test1");
    err = writer.PutString(AnonymousTag, buf);
    SuccessOrExit(err);
    err = writer.Finalize();
    SuccessOrExit(err);

    WeaveLogDetail(DataManagement, "test2: %s", buf);
    mpathLeafTlvMap[aLeafHandle] = msgBuf;

    SuccessOrExit(err);
    WeaveLogDetail(DataManagement, "test3");

    reader.Init(msgBuf);
    err = reader.Next();
    SuccessOrExit(err);
    WeaveLogDetail(DataManagement, "test4");
    DebugPrettyPrint(reader);

    msgBuf = NULL;
exit:
    if (NULL != msgBuf)
    {
        PacketBuffer::Free(msgBuf);
        msgBuf = NULL;
    }

    return err;
}

WEAVE_ERROR
GenericTraitDataSink::SetLeafData(PropertyPathHandle aLeafHandle, TLVReader &aReader)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    nl::Weave::TLV::TLVWriter writer;
    PacketBuffer *msgBuf = PacketBuffer::New();
    VerifyOrExit(NULL != msgBuf, err = WEAVE_ERROR_NO_MEMORY);
    writer.Init(msgBuf);
    err = writer.CopyElement(AnonymousTag, aReader);
    SuccessOrExit(err);
    err = writer.Finalize();
    SuccessOrExit(err);
    mpathLeafTlvMap[aLeafHandle] = msgBuf;
    msgBuf = NULL;
    SuccessOrExit(err);

exit:
    if (NULL != msgBuf)
    {
        PacketBuffer::Free(msgBuf);
        msgBuf = NULL;
    }
    return err;
}

WEAVE_ERROR
GenericTraitDataSink::GetLeafData(PropertyPathHandle aLeafHandle, char *buf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    nl::Weave::TLV::TLVReader reader;
    PacketBuffer *msgBuf = NULL;
    std::map<PropertyPathHandle, PacketBuffer *>::iterator it = mpathLeafTlvMap.find(aLeafHandle);
    WeaveLogDetail(DataManagement, "get1");
    VerifyOrExit(it != mpathLeafTlvMap.end(), err = WEAVE_ERROR_INCORRECT_STATE);

    WeaveLogDetail(DataManagement, "get2");
    msgBuf = mpathLeafTlvMap[aLeafHandle];
    mpathLeafTlvMap.erase(aLeafHandle);
    WeaveLogDetail(DataManagement, "get3");
    reader.Init(msgBuf);
    err = reader.Next();
    SuccessOrExit(err);
    WeaveLogDetail(DataManagement, "get4");
    err = reader.GetString(buf, sizeof(buf));
    SuccessOrExit(err);
    WeaveLogDetail(DataManagement, "get5");
exit:
    if (NULL != msgBuf)
    {
        PacketBuffer::Free(msgBuf);
        msgBuf = NULL;
    }
    WeaveLogFunctError(err);

    return err;
}

WEAVE_ERROR
GenericTraitDataSink::GetLeafData(PropertyPathHandle aLeafHandle, uint64_t aTagToWrite, TLVWriter &aWriter)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    nl::Weave::TLV::TLVReader reader;
    PacketBuffer *msgBuf = NULL;
    char next_locale[24];
    std::map<PropertyPathHandle, PacketBuffer *>::iterator it = mpathLeafTlvMap.find(aLeafHandle);
    WeaveLogDetail(DataManagement, "testget1");

    VerifyOrExit(it != mpathLeafTlvMap.end(), err = WEAVE_ERROR_INCORRECT_STATE);

    WeaveLogDetail(DataManagement, "testget2");
    msgBuf = mpathLeafTlvMap[aLeafHandle];
    //mpathLeafTlvMap.erase(aLeafHandle);
    WeaveLogDetail(DataManagement, "testget3");
    reader.Init(msgBuf);
    err = reader.Next();
    DebugPrettyPrint(reader);
    WeaveLogDetail(DataManagement, "testget4");
    SuccessOrExit(err);

    err = aWriter.CopyElement(aTagToWrite, reader);
    SuccessOrExit(err);
    WeaveLogDetail(DataManagement, "testget6");
exit:
    if (NULL != msgBuf)
    {
        PacketBuffer::Free(msgBuf);
        msgBuf = NULL;
    }
    WeaveLogFunctError(err);

    return err;
}

WEAVE_ERROR GenericTraitDataSink::GetNextDictionaryItemKey(PropertyPathHandle aDictionaryHandle, uintptr_t &aContext, PropertyDictionaryKey &aKey)
{
    return WEAVE_END_OF_INPUT;
}

void GenericTraitDataSink::TLVPrettyPrinter(const char *aFormat, ...)
{
    va_list args;

    va_start(args, aFormat);

    // There is no proper Weave logging routine for us to use here
    vprintf(aFormat, args);

    va_end(args);
}

WEAVE_ERROR GenericTraitDataSink::DebugPrettyPrint(nl::Weave::TLV::TLVReader & aReader)
{
    return nl::Weave::TLV::Debug::Dump(aReader, TLVPrettyPrinter);
}
