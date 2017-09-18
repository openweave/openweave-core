/*
 *
 *    Copyright (c) 2016-2017 Nest Labs, Inc.
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
 *      This file implements the Weave Data Management mock view server.
 *
 */

#define WEAVE_CONFIG_ENABLE_LOG_FILE_LINE_FUNC_ON_ERROR 1

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif // __STDC_FORMAT_MACROS

#if 0 // TODO: Need to fix this but not a high priority. See WEAV-1348
#include <Weave/Core/WeaveError.h>
#include <Weave/Core/WeaveSecurityMgr.h>
#include <Weave/Support/CodeUtils.h>
#include <Weave/Core/WeaveTLVDebug.hpp>
#include "MockWdmViewServer.h"
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Profiles/data-management/DataManagement.h>
#include "MockTraitSchemas.h"

using nl::Weave::System::PacketBuffer;

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING

using namespace nl::Weave::Profiles::DataManagement;

/**
 *  Log the specified message in the form of @a aFormat.
 *
 *  @param[in]     aFormat   A pointer to a NULL-terminated C string with
 *                           C Standard Library-style format specifiers
 *                           containing the log message to be formatted and
 *                           logged.
 *  @param[in]     ...       An argument list whose elements should correspond
 *                           to the format specifiers in @a aFormat.
 *
 */
void TLVPrettyPrinter(const char *aFormat, ...)
{
    va_list args;

    va_start(args, aFormat);

    // There is no proper Weave logging routine for us to use here
    vprintf(aFormat, args);

    va_end(args);
}

WEAVE_ERROR DebugPrettyPrint(nl::Weave::TLV::TLVReader & aReader)
{
    return nl::Weave::TLV::Debug::Dump(aReader, TLVPrettyPrinter);
}

class MockWdmViewServerImpl: public MockWdmViewServer
{
public:

    WEAVE_ERROR SimpleTest ();


    virtual WEAVE_ERROR Init (nl::Weave::WeaveExchangeManager *aExchangeMgr, const char * const aTestCaseId)
    {
        WEAVE_ERROR err = WEAVE_NO_ERROR;

        WeaveLogDetail(DataManagement, "Test Case ID: %s", aTestCaseId);

        //IgnoreUnusedVariable(aExchangeMgr);

        //SimpleTest ();

        err = aExchangeMgr->RegisterUnsolicitedMessageHandler(nl::Weave::Profiles::kWeaveProfile_WDM, kMsgType_ViewRequest, IncomingViewRequest, this);
        SuccessOrExit(err);

    exit:
        return err;
    }

private:

    static void IncomingViewRequest(nl::Weave::ExchangeContext *ec, const nl::Weave::IPPacketInfo *pktInfo,
        const nl::Weave::WeaveMessageInfo *msgInfo, uint32_t profileId,
                uint8_t msgType, PacketBuffer *payload);
};

static MockWdmViewServerImpl gWdmViewServer;

MockWdmViewServer * MockWdmViewServer::GetInstance ()
{
    return &gWdmViewServer;
}

void MockWdmViewServerImpl::IncomingViewRequest(nl::Weave::ExchangeContext *ec, const nl::Weave::IPPacketInfo *pktInfo,
    const nl::Weave::WeaveMessageInfo *msgInfo, uint32_t profileId,
            uint8_t msgType, PacketBuffer *payload)
{
    WEAVE_ERROR     err     = WEAVE_NO_ERROR;
    PacketBuffer*   msgBuf  = PacketBuffer::New();

    WeaveLogDetail(DataManagement, "Incoming View Request");

#if WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK
    {
        // Schema validation
        nl::Weave::TLV::TLVType dummyContainerType;
        nl::Weave::TLV::TLVReader ValidationReader;
        ValidationReader.Init(payload);
        ValidationReader.Next();

        err = ValidationReader.EnterContainer(dummyContainerType);
        SuccessOrExit(err);

        err = ValidationReader.Next();
        SuccessOrExit(err);

        PathList::Parser paths;
        paths.Init(ValidationReader);
        WeaveLogError(DataManagement, "CheckSchemaValidity: (0 means good) %" PRIu32, paths.CheckSchemaValidity());

        err = ValidationReader.ExitContainer(dummyContainerType);
        SuccessOrExit(err);
    }
#endif // WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK

    VerifyOrExit(NULL != msgBuf, err = WEAVE_ERROR_NO_MEMORY);
    {
        nl::Weave::TLV::TLVWriter writer;
        nl::Weave::TLV::TLVType dummyContainerType;
        writer.Init(msgBuf);

        err = writer.StartContainer(nl::Weave::TLV::AnonymousTag, nl::Weave::TLV::kTLVType_Structure, dummyContainerType);
        SuccessOrExit(err);

        DataList::Builder list;
        list.Init(&writer, ViewResponse::kCsTag_DataList);

        {
            DataElement::Builder & builder = list.CreateDataElementBuilder();

            builder.LastForCurrentChange(false);

            Path::Builder & pathBuilder = builder.CreatePathBuilder();
            pathBuilder.ProfileID(0x13)
                       .ResourceID(0xDEADBEEF)
                       .InstanceID(0)
                       .EndOfPath();

            if (WEAVE_NO_ERROR != pathBuilder.GetError())
            {
                WeaveLogError(DataManagement, "Path builder failed");
            }

            nl::Weave::TLV::TLVWriter * dataWriter = builder.GetWriter();
            {
                nl::Weave::TLV::TLVType type;

                dataWriter->StartContainer(nl::Weave::TLV::ContextTag(DataElement::kCsTag_Data), nl::Weave::TLV::kTLVType_Structure, type);

                {
                    dataWriter->PutString(nl::Weave::TLV::ContextTag(1), "en_gb");
                }

                dataWriter->EndContainer(type);
            }

            builder.EndOfDataElement();

            if (WEAVE_NO_ERROR != builder.GetError())
            {
                WeaveLogError(DataManagement, "DataElement builder failed");
            }
        }

        {
            DataElement::Builder & builder = list.CreateDataElementBuilder();

            builder.LastForCurrentChange(false);

            Path::Builder & pathBuilder = builder.CreatePathBuilder();
            pathBuilder.ProfileID(0x13)
                       .ResourceID(0xDEADBEEF)
                       .InstanceID(0)
                       .TagSection()
                       .AdditionalTag(nl::Weave::TLV::ContextTag(1))
                       .EndOfPath();

            if (WEAVE_NO_ERROR != pathBuilder.GetError())
            {
                WeaveLogError(DataManagement, "Path builder failed");
            }

            nl::Weave::TLV::TLVWriter * dataWriter = builder.GetWriter();
            dataWriter->PutString(nl::Weave::TLV::ContextTag(DataElement::kCsTag_Data), "en_us");

            builder.EndOfDataElement();

            if (WEAVE_NO_ERROR != builder.GetError())
            {
                WeaveLogError(DataManagement, "DataElement builder failed");
            }
        }

        {
            DataElement::Builder & builder = list.CreateDataElementBuilder();

            builder.LastForCurrentChange(false);

            Path::Builder & pathBuilder = builder.CreatePathBuilder();
            pathBuilder.ProfileID(0x14)
                       .ResourceID(0xDEADBEEF)
                       .InstanceID(0)
                       .EndOfPath();

            if (WEAVE_NO_ERROR != pathBuilder.GetError())
            {
                WeaveLogError(DataManagement, "Path builder failed");
            }

            nl::Weave::TLV::TLVWriter * dataWriter = builder.GetWriter();

            {
                nl::Weave::TLV::TLVType type1;
                dataWriter->StartContainer(nl::Weave::TLV::ContextTag(DataElement::kCsTag_Data), nl::Weave::TLV::kTLVType_Structure, type1);

                // root
                {
                    nl::Weave::TLV::TLVType type2;
                    dataWriter->StartContainer(nl::Weave::TLV::ContextTag(1), nl::Weave::TLV::kTLVType_Structure, type2);

                    // s1
                    {
                        nl::Weave::TLV::TLVType type3;
                        dataWriter->StartContainer(nl::Weave::TLV::ContextTag(1), nl::Weave::TLV::kTLVType_Structure, type3);

                        // s3
                        {
                            // a
                            uint32_t a = 100;
                            dataWriter->Put(nl::Weave::TLV::ContextTag(1), a);

                            // b
                            bool b = true;
                            dataWriter->PutBoolean(nl::Weave::TLV::ContextTag(2), b);

                            // c
                            char c[] = "hello";
                            dataWriter->PutString(nl::Weave::TLV::ContextTag(3), c);
                        }

                        dataWriter->EndContainer(type3);

                        // d
                        int32_t d = -20;
                        dataWriter->Put(nl::Weave::TLV::ContextTag(2), d);
                    }

                    dataWriter->EndContainer(type2);
                    dataWriter->StartContainer(nl::Weave::TLV::ContextTag(2), nl::Weave::TLV::kTLVType_Structure, type2);

                    // s2
                    {
                        // e
                        bool e = false;
                        dataWriter->PutBoolean(nl::Weave::TLV::ContextTag(1), e);

                        int8_t f = -4;
                        dataWriter->Put(nl::Weave::TLV::ContextTag(2), f);

                        int8_t g = -3;
                        dataWriter->Put(nl::Weave::TLV::ContextTag(3), g);
                     }

                    dataWriter->EndContainer(type2);

                    // h
                    int32_t h = -700;
                    dataWriter->Put(nl::Weave::TLV::ContextTag(3), h);
                }

                dataWriter->EndContainer(type1);
            }

            builder.EndOfDataElement();

            if (WEAVE_NO_ERROR != builder.GetError())
            {
                WeaveLogError(DataManagement, "DataElement builder failed");
            }
        }

        list.EndOfDataList();

        if (WEAVE_NO_ERROR != list.GetError())
        {
           WeaveLogError(DataManagement, "DataList builder failed");
        }

        err = writer.EndContainer(dummyContainerType);
        SuccessOrExit(err);

        err = writer.Finalize();
        SuccessOrExit(err);
    }

    err = ec->SendMessage(nl::Weave::Profiles::kWeaveProfile_WDM, kMsgType_ViewResponse, msgBuf, nl::Weave::ExchangeContext::kSendFlag_RequestAck);
    msgBuf = NULL;
    SuccessOrExit(err);

exit:
    WeaveLogFunctError(err);

    if (NULL != payload)
    {
        PacketBuffer::Free(payload);
        payload = NULL;
    }

    if (NULL != msgBuf)
    {
        PacketBuffer::Free(msgBuf);
        msgBuf = NULL;
    }

    if (NULL != ec)
    {
        ec->Close();
    }
}

WEAVE_ERROR MockWdmViewServerImpl::SimpleTest ()
{
    WeaveLogError(DataManagement, "Simple Test");

    WeaveLogError(DataManagement, "####################### Path Test #######################");
    {
        uint8_t buffer[1024] = { 0 };
        nl::Weave::TLV::TLVWriter writer;
        writer.Init(buffer, sizeof(buffer));

        Path::Builder builder;
        builder.Init(&writer);
        builder.ProfileID(0xABCD1234)
            .ResourceID(0x12345678)
            .InstanceID(0xdeadbeef)
            .EndOfPath();

        if (WEAVE_NO_ERROR != builder.GetError())
        {
            WeaveLogError(DataManagement, "Path builder failed");
        }

        uint32_t encodedLen = writer.GetLengthWritten();
        nl::Weave::TLV::TLVReader reader;
        reader.Init(buffer, encodedLen);
        reader.Next();
        Path::Parser path;
        path.Init(reader);
#if WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK
        WeaveLogError(DataManagement, "CheckSchemaValidity: (0 means good) %" PRIu32, path.CheckSchemaValidity());
#endif // WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK
        uint64_t temp64 = 0;
        path.GetResourceID(&temp64);
        WeaveLogError(DataManagement, "GetResourceID: 0x%X", temp64);
        uint32_t temp32 = 0;
        path.GetProfileID(&temp32);
        WeaveLogError(DataManagement, "GetProfileID: 0x%X", temp32);
        temp64 = 0;
        path.GetInstanceID(&temp64);
        WeaveLogError(DataManagement, "GetInstanceID: 0x%X", temp64);
    }

    WeaveLogError(DataManagement, "####################### DataElement Test #######################");
    {
        uint8_t buffer[1024] = { 0 };
        nl::Weave::TLV::TLVWriter writer;
        writer.Init(buffer, sizeof(buffer));

        DataElement::Builder builder;
        builder.Init(&writer);

        builder.LastForCurrentChange(true);

        Path::Builder & pathBuilder = builder.CreatePathBuilder();
        pathBuilder.ProfileID(0xABCD1234)
                   .ResourceID(0x12345678)
                   .InstanceID(0xdeadbeef)
                   .TagSection()
                   .AdditionalTag(nl::Weave::TLV::ProfileTag(0x12345678, 4096))
                   .AdditionalTag(nl::Weave::TLV::ContextTag(5))
                   .EndOfPath();

        if (WEAVE_NO_ERROR != pathBuilder.GetError())
        {
            WeaveLogError(DataManagement, "Path builder failed");
        }

        nl::Weave::TLV::TLVWriter * dataWriter = builder.GetWriter();
        //dataWriter->Put(nl::Weave::TLV::ContextTag(DataElement::kCsTag_Data), 4.23f);
        nl::Weave::TLV::TLVType tempContainerType;
        // TODO: check for error
        dataWriter->StartContainer(nl::Weave::TLV::ContextTag(DataElement::kCsTag_Data), nl::Weave::TLV::kTLVType_Structure, tempContainerType);
        dataWriter->ImplicitProfileId = 0xABCD1234;
        // TODO: check for error
        dataWriter->Put(nl::Weave::TLV::ProfileTag(0xABCD1234, 8), 4.23f);
        dataWriter->ImplicitProfileId = nl::Weave::TLV::kProfileIdNotSpecified;
        // TODO: check for error
        dataWriter->EndContainer(tempContainerType);

        builder.EndOfDataElement();

        if (WEAVE_NO_ERROR != builder.GetError())
        {
            WeaveLogError(DataManagement, "DataElement builder failed");
        }

        uint32_t encodedLen = writer.GetLengthWritten();
        nl::Weave::TLV::TLVReader reader;
        reader.Init(buffer, encodedLen);
        reader.Next();

#if WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK
        DataElement::Parser data;
        data.Init(reader);
        WeaveLogError(DataManagement, "CheckSchemaValidity: (0 means good) %" PRIu32, data.CheckSchemaValidity());
#endif // WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK
    }

    WeaveLogError(DataManagement, "####################### PathList Test #######################");
    {
        uint8_t buffer[1024] = { 0 };
        nl::Weave::TLV::TLVWriter writer;
        writer.Init(buffer, sizeof(buffer));

        PathList::Builder list;
        list.Init(&writer, ViewRequest::kCsTag_PathList);

        for (unsigned int i = 0; i < 5; ++i)
        {
            Path::Builder & path = list.CreatePathBuilder();
            path.ProfileID(0xABCD1234)
                .ResourceID(0x12345678)
                .InstanceID(i)
                .TagSection();

            for (unsigned int j = 0; j < i; ++j)
            {
                path.AdditionalTag(nl::Weave::TLV::ContextTag(j));
            }

            path.EndOfPath();

            if (WEAVE_NO_ERROR != path.GetError())
            {
                WeaveLogError(DataManagement, "Path builder failed");
            }
        }
        list.EndOfPathList();

        if (WEAVE_NO_ERROR != list.GetError())
        {
            WeaveLogError(DataManagement, "PathList builder failed");
        }

        uint32_t encodedLen = writer.GetLengthWritten();
        nl::Weave::TLV::TLVReader reader;
        reader.Init(buffer, encodedLen);
        reader.Next();

#if WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK
        PathList::Parser paths;
        paths.Init(reader);
        WeaveLogError(DataManagement, "CheckSchemaValidity: (0 means good) %" PRIu32, paths.CheckSchemaValidity());
#endif // WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK
    }

    WeaveLogError(DataManagement, "####################### DataList Test #######################");
    {
        uint8_t buffer[1024] = { 0 };
        nl::Weave::TLV::TLVWriter writer;
        writer.Init(buffer, sizeof(buffer));

        DataList::Builder list;
        list.Init(&writer, ViewResponse::kCsTag_DataList);

        for (unsigned int i = 0; i < 3; ++i)
        {
            DataElement::Builder & builder = list.CreateDataElementBuilder();

            builder.LastForCurrentChange(true);

            Path::Builder & pathBuilder = builder.CreatePathBuilder();
            pathBuilder.ProfileID(0xABCD1234)
                       .ResourceID(0x12345678)
                       .InstanceID(i)
                       .TagSection()
                       .AdditionalTag(nl::Weave::TLV::ProfileTag(0x12345678, 4096))
                       .AdditionalTag(nl::Weave::TLV::ContextTag(i))
                       .EndOfPath();

            if (WEAVE_NO_ERROR != pathBuilder.GetError())
            {
                WeaveLogError(DataManagement, "Path builder failed");
            }

            nl::Weave::TLV::TLVWriter * dataWriter = builder.GetWriter();
            dataWriter->Put(nl::Weave::TLV::ContextTag(DataElement::kCsTag_Data), 100 + i);

            builder.EndOfDataElement();

            if (WEAVE_NO_ERROR != builder.GetError())
            {
                WeaveLogError(DataManagement, "DataElement builder failed");
            }
        }
        list.EndOfDataList();

        if (WEAVE_NO_ERROR != list.GetError())
        {
           WeaveLogError(DataManagement, "DataList builder failed");
        }

        uint32_t encodedLen = writer.GetLengthWritten();
        nl::Weave::TLV::TLVReader reader;
        reader.Init(buffer, encodedLen);
        reader.Next();

#if WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK
        DataList::Parser data;
        data.Init(reader);
        WeaveLogError(DataManagement, "CheckSchemaValidity: (0 means good) %" PRIu32, data.CheckSchemaValidity());
#endif // WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK
    }

    WeaveLogError(DataManagement, "Simple Test Completed");

    return WEAVE_NO_ERROR;
}

#endif // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
#endif // 0
