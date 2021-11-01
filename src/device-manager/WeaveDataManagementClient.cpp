/*
 *
 *    Copyright (c) 2020 Google LLC.
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
 *      WARNING: This is experimental feature for Weave Data Management Client
 */

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <time.h>
#include <string>
#include <chrono>

#include <Weave/Core/WeaveCore.h>

#if WEAVE_CONFIG_DATA_MANAGEMENT_CLIENT_EXPERIMENTAL
#include <Weave/Support/Base64.h>
#include <Weave/Support/CodeUtils.h>
#include <Weave/Core/WeaveEncoding.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Profiles/common/CommonProfile.h>
#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Profiles/data-management/Current/WdmManagedNamespace.h>
#include <Weave/Support/logging/WeaveLogging.h>
#include <Weave/Support/ErrorStr.h>
#include <Weave/Support/TimeUtils.h>
#include <Weave/DeviceManager/WeaveDataManagementClient.h>
#include <Weave/DeviceManager/TraitSchemaDirectory.h>
#include <Weave/Profiles/data-management/Current/GenericTraitCatalogImpl.h>
#include <Weave/Profiles/data-management/Current/GenericTraitCatalogImpl.ipp>

namespace nl {
namespace Weave {
namespace DeviceManager {
using namespace nl::Weave::Encoding;
using namespace nl::Weave::Profiles;
using namespace nl::Weave::Profiles::DataManagement;
using namespace ::nl::Weave;
using namespace ::nl::Weave::TLV;
using namespace Schema::Weave::Common;
using namespace nl::Weave::Profiles::DataManagement_Current::Event;
using std::to_string;

class GenericTraitUpdatableDataSink;
class WdmClient;

GenericTraitUpdatableDataSink::GenericTraitUpdatableDataSink(
    const nl::Weave::Profiles::DataManagement::TraitSchemaEngine * apEngine, WdmClient * apWdmClient) :
    TraitUpdatableDataSink(apEngine)
{
    mpAppState  = NULL;
    mOnError    = NULL;
    mpWdmClient = apWdmClient;
}

GenericTraitUpdatableDataSink::~GenericTraitUpdatableDataSink()
{
    Clear();
}

void GenericTraitUpdatableDataSink::Clear(void)
{
    SubscriptionClient * pSubscriptionClient = GetSubscriptionClient();
    if (pSubscriptionClient != NULL)
    {
        pSubscriptionClient->DiscardUpdates();
    }
    ClearVersion();

    for (auto itr = mPathTlvDataMap.begin(); itr != mPathTlvDataMap.end(); ++itr)
    {
        if (NULL != itr->second)
        {
            PacketBuffer::Free(itr->second);
            itr->second = NULL;
        }
    }

    mPathTlvDataMap.clear();
}

void GenericTraitUpdatableDataSink::UpdateTLVDataMap(PropertyPathHandle aPropertyPathHandle, PacketBuffer * apMsgBuf)
{
    auto it = mPathTlvDataMap.find(aPropertyPathHandle);
    if (it != mPathTlvDataMap.end() && NULL != it->second)
    {
        PacketBuffer::Free(it->second);
    }
    mPathTlvDataMap[aPropertyPathHandle] = apMsgBuf;
}

WEAVE_ERROR GenericTraitUpdatableDataSink::LocateTraitHandle(void * apContext,
                                                             const TraitCatalogBase<TraitDataSink> * const apCatalog,
                                                             TraitDataHandle & aHandle)
{
    GenericTraitUpdatableDataSink * pGenericTraitUpdatableDataSink = static_cast<GenericTraitUpdatableDataSink *>(apContext);
    return apCatalog->Locate(pGenericTraitUpdatableDataSink, aHandle);
}

WEAVE_ERROR GenericTraitUpdatableDataSink::RefreshData(void * appReqState, DMCompleteFunct onComplete, DMErrorFunct onError)
{
    ClearVersion();
    mpWdmClient->RefreshData(appReqState, this, onComplete, onError, LocateTraitHandle, false);
    return WEAVE_NO_ERROR;
}

WEAVE_ERROR GenericTraitUpdatableDataSink::SetData(const char * apPath, int64_t aValue, bool aIsConditional)
{
    return Set(apPath, aValue, aIsConditional);
}

WEAVE_ERROR GenericTraitUpdatableDataSink::SetData(const char * apPath, uint64_t aValue, bool aIsConditional)
{
    return Set(apPath, aValue, aIsConditional);
}

WEAVE_ERROR GenericTraitUpdatableDataSink::SetData(const char * apPath, double aValue, bool aIsConditional)
{
    return Set(apPath, aValue, aIsConditional);
}

WEAVE_ERROR GenericTraitUpdatableDataSink::SetBoolean(const char * apPath, bool aValue, bool aIsConditional)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    nl::Weave::TLV::TLVWriter writer;
    PropertyPathHandle propertyPathHandle = kNullPropertyPathHandle;
    PacketBuffer * pMsgBuf                = PacketBuffer::New();
    VerifyOrExit(NULL != pMsgBuf, err = WEAVE_ERROR_NO_MEMORY);

    VerifyOrExit(GetSubscriptionClient() != NULL, err = WEAVE_ERROR_INCORRECT_STATE);

    Lock(GetSubscriptionClient());

    err = GetSchemaEngine()->MapPathToHandle(apPath, propertyPathHandle);
    SuccessOrExit(err);

    writer.Init(pMsgBuf);

    err = writer.PutBoolean(AnonymousTag, aValue);
    SuccessOrExit(err);

    err = writer.Finalize();
    SuccessOrExit(err);

#if WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK
    err = DebugPrettyPrint(pMsgBuf);
    SuccessOrExit(err);
#endif // WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK

    UpdateTLVDataMap(propertyPathHandle, pMsgBuf);
    pMsgBuf = NULL;
    err     = SetUpdated(GetSubscriptionClient(), propertyPathHandle, aIsConditional);
    Unlock(GetSubscriptionClient());

    WeaveLogDetail(DataManagement, "<set updated> in 0x%08x", propertyPathHandle);

exit:
    WeaveLogFunctError(err);

    if (NULL != pMsgBuf)
    {
        PacketBuffer::Free(pMsgBuf);
        pMsgBuf = NULL;
    }
    return err;
}

WEAVE_ERROR GenericTraitUpdatableDataSink::SetString(const char * apPath, const char * aValue, bool aIsConditional)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    nl::Weave::TLV::TLVWriter writer;
    PropertyPathHandle propertyPathHandle = kNullPropertyPathHandle;
    PacketBuffer * pMsgBuf                = PacketBuffer::New();
    VerifyOrExit(NULL != pMsgBuf, err = WEAVE_ERROR_NO_MEMORY);

    VerifyOrExit(GetSubscriptionClient() != NULL, err = WEAVE_ERROR_INCORRECT_STATE);

    Lock(GetSubscriptionClient());

    err = GetSchemaEngine()->MapPathToHandle(apPath, propertyPathHandle);
    SuccessOrExit(err);

    writer.Init(pMsgBuf);

    err = writer.PutString(AnonymousTag, aValue);
    SuccessOrExit(err);

    err = writer.Finalize();
    SuccessOrExit(err);

#if WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK
    err = DebugPrettyPrint(pMsgBuf);
    SuccessOrExit(err);
#endif // WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK

    UpdateTLVDataMap(propertyPathHandle, pMsgBuf);
    pMsgBuf = NULL;
    err     = SetUpdated(GetSubscriptionClient(), propertyPathHandle, aIsConditional);
    Unlock(GetSubscriptionClient());

    WeaveLogDetail(DataManagement, "<set updated> in 0x%08x", propertyPathHandle);

exit:
    WeaveLogFunctError(err);

    if (NULL != pMsgBuf)
    {
        PacketBuffer::Free(pMsgBuf);
        pMsgBuf = NULL;
    }

    return err;
}

WEAVE_ERROR GenericTraitUpdatableDataSink::SetBytes(const char * apPath, const uint8_t * dataBuf, size_t dataLen,
                                                    bool aIsConditional)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    nl::Weave::TLV::TLVWriter writer;
    nl::Weave::TLV::TLVReader reader;
    PropertyPathHandle propertyPathHandle = kNullPropertyPathHandle;
    PacketBuffer * pMsgBuf                = PacketBuffer::New();
    VerifyOrExit(NULL != pMsgBuf, err = WEAVE_ERROR_NO_MEMORY);

    VerifyOrExit(GetSubscriptionClient() != NULL, err = WEAVE_ERROR_INCORRECT_STATE);

    Lock(GetSubscriptionClient());

    err = GetSchemaEngine()->MapPathToHandle(apPath, propertyPathHandle);
    SuccessOrExit(err);

    writer.Init(pMsgBuf);

    reader.Init(dataBuf, dataLen);
    err = reader.Next();
    SuccessOrExit(err);

    err = writer.CopyElement(AnonymousTag, reader);
    SuccessOrExit(err);

    err = writer.Finalize();
    SuccessOrExit(err);

#if WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK
    err = DebugPrettyPrint(pMsgBuf);
    SuccessOrExit(err);
#endif // WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK

    UpdateTLVDataMap(propertyPathHandle, pMsgBuf);
    pMsgBuf = NULL;
    err     = SetUpdated(GetSubscriptionClient(), propertyPathHandle, aIsConditional);
    Unlock(GetSubscriptionClient());

    WeaveLogDetail(DataManagement, "<set updated> in 0x%08x", propertyPathHandle);

exit:
    WeaveLogFunctError(err);

    if (NULL != pMsgBuf)
    {
        PacketBuffer::Free(pMsgBuf);
        pMsgBuf = NULL;
    }

    return err;
}

WEAVE_ERROR GenericTraitUpdatableDataSink::SetTLVBytes(const char * apPath, const uint8_t * dataBuf, size_t dataLen,
                                                       bool aIsConditional)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TLVReader reader;
    PropertyPathHandle propertyPathHandle                 = kNullPropertyPathHandle;
    TraitSchemaEngine::ISetDataDelegate * setDataDelegate = NULL;

    VerifyOrExit(GetSubscriptionClient() != NULL, err = WEAVE_ERROR_INCORRECT_STATE);
    Lock(GetSubscriptionClient());

    err = GetSchemaEngine()->MapPathToHandle(apPath, propertyPathHandle);
    SuccessOrExit(err);

    reader.Init(dataBuf, dataLen);
    err = reader.Next();
    SuccessOrExit(err);

    setDataDelegate = static_cast<TraitSchemaEngine::ISetDataDelegate *>(this);
    err             = GetSchemaEngine()->StoreData(propertyPathHandle, reader, setDataDelegate, NULL);
    SuccessOrExit(err);

    err = SetUpdated(GetSubscriptionClient(), propertyPathHandle, aIsConditional);

    Unlock(GetSubscriptionClient());
    WeaveLogDetail(DataManagement, "<set updated> in 0x%08x", propertyPathHandle);

exit:
    WeaveLogFunctError(err);

    return err;
}

WEAVE_ERROR GenericTraitUpdatableDataSink::SetNull(const char * apPath, bool aIsConditional)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    nl::Weave::TLV::TLVWriter writer;
    PropertyPathHandle propertyPathHandle = kNullPropertyPathHandle;
    PacketBuffer * pMsgBuf                = PacketBuffer::New();
    VerifyOrExit(NULL != pMsgBuf, err = WEAVE_ERROR_NO_MEMORY);

    VerifyOrExit(GetSubscriptionClient() != NULL, err = WEAVE_ERROR_INCORRECT_STATE);

    Lock(GetSubscriptionClient());

    err = GetSchemaEngine()->MapPathToHandle(apPath, propertyPathHandle);
    SuccessOrExit(err);

    writer.Init(pMsgBuf);

    err = writer.PutNull(AnonymousTag);
    SuccessOrExit(err);

    err = writer.Finalize();
    SuccessOrExit(err);

#if WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK
    err = DebugPrettyPrint(pMsgBuf);
    SuccessOrExit(err);
#endif // WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK

    UpdateTLVDataMap(propertyPathHandle, pMsgBuf);
    pMsgBuf = NULL;
    err     = SetUpdated(GetSubscriptionClient(), propertyPathHandle, aIsConditional);
    Unlock(GetSubscriptionClient());

    WeaveLogDetail(DataManagement, "<set updated> in 0x%08x", propertyPathHandle);

exit:
    WeaveLogFunctError(err);

    if (NULL != pMsgBuf)
    {
        PacketBuffer::Free(pMsgBuf);
        pMsgBuf = NULL;
    }

    return err;
}

WEAVE_ERROR GenericTraitUpdatableDataSink::SetStringArray(const char * apPath, const std::vector<std::string> & aValueVector,
                                                          bool aIsConditional)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    nl::Weave::TLV::TLVWriter writer;
    PropertyPathHandle propertyPathHandle = kNullPropertyPathHandle;
    TLVType OuterContainerType;
    PacketBuffer * pMsgBuf = PacketBuffer::New();
    VerifyOrExit(NULL != pMsgBuf, err = WEAVE_ERROR_NO_MEMORY);

    VerifyOrExit(GetSubscriptionClient() != NULL, err = WEAVE_ERROR_INCORRECT_STATE);

    Lock(GetSubscriptionClient());

    err = GetSchemaEngine()->MapPathToHandle(apPath, propertyPathHandle);
    SuccessOrExit(err);

    writer.Init(pMsgBuf);

    err = writer.StartContainer(AnonymousTag, kTLVType_Array, OuterContainerType);
    SuccessOrExit(err);

    for (uint8_t i = 0; i < aValueVector.size(); ++i)
    {
        err = writer.PutString(AnonymousTag, aValueVector[i].c_str());
        SuccessOrExit(err);
    }

    err = writer.EndContainer(OuterContainerType);
    SuccessOrExit(err);

    err = writer.Finalize();
    SuccessOrExit(err);

#if WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK
    err = DebugPrettyPrint(pMsgBuf);
    SuccessOrExit(err);
#endif // WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK

    UpdateTLVDataMap(propertyPathHandle, pMsgBuf);
    pMsgBuf = NULL;
    err     = SetUpdated(GetSubscriptionClient(), propertyPathHandle, aIsConditional);
    Unlock(GetSubscriptionClient());

    WeaveLogDetail(DataManagement, "<set updated> in 0x%08x", propertyPathHandle);

exit:
    WeaveLogFunctError(err);

    if (NULL != pMsgBuf)
    {
        PacketBuffer::Free(pMsgBuf);
        pMsgBuf = NULL;
    }

    return err;
}

template <class T>
WEAVE_ERROR GenericTraitUpdatableDataSink::Set(const char * apPath, T aValue, bool aIsConditional)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    nl::Weave::TLV::TLVWriter writer;
    PropertyPathHandle propertyPathHandle = kNullPropertyPathHandle;
    PacketBuffer * pMsgBuf                = PacketBuffer::New();
    VerifyOrExit(NULL != pMsgBuf, err = WEAVE_ERROR_NO_MEMORY);

    VerifyOrExit(GetSubscriptionClient() != NULL, err = WEAVE_ERROR_INCORRECT_STATE);

    Lock(GetSubscriptionClient());

    err = GetSchemaEngine()->MapPathToHandle(apPath, propertyPathHandle);
    SuccessOrExit(err);

    writer.Init(pMsgBuf);

    err = writer.Put(AnonymousTag, aValue);
    SuccessOrExit(err);

    err = writer.Finalize();
    SuccessOrExit(err);

#if WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK
    err = DebugPrettyPrint(pMsgBuf);
    SuccessOrExit(err);
#endif // WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK

    UpdateTLVDataMap(propertyPathHandle, pMsgBuf);
    pMsgBuf = NULL;

    err = SetUpdated(GetSubscriptionClient(), propertyPathHandle, aIsConditional);
    Unlock(GetSubscriptionClient());

    WeaveLogDetail(DataManagement, "<set updated> in 0x%08x", propertyPathHandle);

exit:
    WeaveLogFunctError(err);

    if (NULL != pMsgBuf)
    {
        PacketBuffer::Free(pMsgBuf);
        pMsgBuf = NULL;
    }

    return err;
}

WEAVE_ERROR GenericTraitUpdatableDataSink::GetData(const char * apPath, int64_t & aValue)
{
    return Get(apPath, aValue);
}

WEAVE_ERROR GenericTraitUpdatableDataSink::GetData(const char * apPath, uint64_t & aValue)
{
    return Get(apPath, aValue);
}

WEAVE_ERROR GenericTraitUpdatableDataSink::GetData(const char * apPath, double & aValue)
{
    return Get(apPath, aValue);
}

WEAVE_ERROR GenericTraitUpdatableDataSink::GetBoolean(const char * apPath, bool & aValue)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    nl::Weave::TLV::TLVReader reader;
    PacketBuffer * pMsgBuf                = NULL;
    PropertyPathHandle propertyPathHandle = kNullPropertyPathHandle;
    std::map<PropertyPathHandle, PacketBuffer *>::iterator it;

    err = GetSchemaEngine()->MapPathToHandle(apPath, propertyPathHandle);
    SuccessOrExit(err);

    it = mPathTlvDataMap.find(propertyPathHandle);
    VerifyOrExit(it != mPathTlvDataMap.end(), err = WEAVE_ERROR_INVALID_TLV_TAG);

    pMsgBuf = mPathTlvDataMap[propertyPathHandle];

#if WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK
    err = DebugPrettyPrint(pMsgBuf);
    SuccessOrExit(err);
#endif // WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK

    reader.Init(pMsgBuf);
    err = reader.Next();
    SuccessOrExit(err);

    err = reader.Get(aValue);
    SuccessOrExit(err);

exit:
    WeaveLogFunctError(err);
    return err;
}

WEAVE_ERROR GenericTraitUpdatableDataSink::GetString(const char * apPath, BytesData * apBytesData)
{
    return GetBytes(apPath, apBytesData);
}

WEAVE_ERROR GenericTraitUpdatableDataSink::GetBytes(const char * apPath, BytesData * apBytesData)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    nl::Weave::TLV::TLVReader reader;
    PacketBuffer * pMsgBuf                = NULL;
    PropertyPathHandle propertyPathHandle = kNullPropertyPathHandle;
    std::map<PropertyPathHandle, PacketBuffer *>::iterator it;

    err = GetSchemaEngine()->MapPathToHandle(apPath, propertyPathHandle);
    SuccessOrExit(err);

    it = mPathTlvDataMap.find(propertyPathHandle);
    VerifyOrExit(it != mPathTlvDataMap.end(), err = WEAVE_ERROR_INVALID_TLV_TAG);

    pMsgBuf = mPathTlvDataMap[propertyPathHandle];

#if WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK
    err = DebugPrettyPrint(pMsgBuf);
    SuccessOrExit(err);
#endif // WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK

    reader.Init(pMsgBuf);
    err = reader.Next();
    SuccessOrExit(err);

    apBytesData->mDataLen = reader.GetLength();
    WeaveLogProgress(DataManagement, "GetBytes with length %d", apBytesData->mDataLen);
    if (apBytesData->mDataLen != 0)
    {
        err = reader.GetDataPtr(apBytesData->mpDataBuf);
        SuccessOrExit(err);
    }

exit:
    WeaveLogFunctError(err);
    return err;
}

WEAVE_ERROR GenericTraitUpdatableDataSink::GetTLVBytes(const char * apPath, BytesData * apBytesData)
{
    WEAVE_ERROR err                                       = WEAVE_NO_ERROR;
    TraitSchemaEngine::IGetDataDelegate * getDataDelegate = NULL;
    nl::Weave::TLV::TLVWriter writer;
    PropertyPathHandle propertyPathHandle = kNullPropertyPathHandle;

    PacketBuffer * pMsgBuf = PacketBuffer::New();
    VerifyOrExit(NULL != pMsgBuf, err = WEAVE_ERROR_NO_MEMORY);

    VerifyOrExit(NULL != apBytesData, err = WEAVE_ERROR_INVALID_ARGUMENT);

    err = GetSchemaEngine()->MapPathToHandle(apPath, propertyPathHandle);
    SuccessOrExit(err);

    writer.Init(pMsgBuf);

    getDataDelegate = static_cast<TraitSchemaEngine::IGetDataDelegate *>(this);
    err             = GetSchemaEngine()->RetrieveData(propertyPathHandle, AnonymousTag, writer, getDataDelegate);
    SuccessOrExit(err);

    err = writer.Finalize();
    SuccessOrExit(err);

#if WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK
    err = DebugPrettyPrint(pMsgBuf);
    SuccessOrExit(err);
#endif // WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK

    apBytesData->mpDataBuf = pMsgBuf->Start();
    apBytesData->mDataLen  = pMsgBuf->DataLength();
    apBytesData->mpMsgBuf  = pMsgBuf;
    pMsgBuf                = NULL;

exit:
    WeaveLogFunctError(err);

    if (NULL != pMsgBuf)
    {
        PacketBuffer::Free(pMsgBuf);
        pMsgBuf = NULL;
    }
    return err;
}

WEAVE_ERROR GenericTraitUpdatableDataSink::IsNull(const char * apPath, bool & aIsNull)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    nl::Weave::TLV::TLVReader reader;
    PacketBuffer * pMsgBuf                = NULL;
    PropertyPathHandle propertyPathHandle = kNullPropertyPathHandle;
    std::map<PropertyPathHandle, PacketBuffer *>::iterator it;

    err = GetSchemaEngine()->MapPathToHandle(apPath, propertyPathHandle);
    SuccessOrExit(err);

    it = mPathTlvDataMap.find(propertyPathHandle);
    VerifyOrExit(it != mPathTlvDataMap.end(), err = WEAVE_ERROR_INVALID_TLV_TAG);

    pMsgBuf = mPathTlvDataMap[propertyPathHandle];

#if WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK
    err = DebugPrettyPrint(pMsgBuf);
    SuccessOrExit(err);
#endif // WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK

    reader.Init(pMsgBuf);
    err = reader.Next();
    SuccessOrExit(err);

    if (reader.GetType() == kTLVType_Null)
    {
        aIsNull = true;
    }
    else
    {
        aIsNull = false;
    }

exit:
    WeaveLogFunctError(err);
    return err;
}

WEAVE_ERROR GenericTraitUpdatableDataSink::GetStringArray(const char * apPath, std::vector<std::string> & aValueVector)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    nl::Weave::TLV::TLVReader reader;
    PacketBuffer * pMsgBuf                = NULL;
    PropertyPathHandle propertyPathHandle = kNullPropertyPathHandle;
    TLVType OuterContainerType;
    std::map<PropertyPathHandle, PacketBuffer *>::iterator it;

    err = GetSchemaEngine()->MapPathToHandle(apPath, propertyPathHandle);
    SuccessOrExit(err);

    it = mPathTlvDataMap.find(propertyPathHandle);
    VerifyOrExit(it != mPathTlvDataMap.end(), err = WEAVE_ERROR_INVALID_TLV_TAG);

    pMsgBuf = mPathTlvDataMap[propertyPathHandle];

#if WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK
    err = DebugPrettyPrint(pMsgBuf);
    SuccessOrExit(err);
#endif // WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK

    reader.Init(pMsgBuf);
    err = reader.Next();
    SuccessOrExit(err);

    err = reader.EnterContainer(OuterContainerType);
    SuccessOrExit(err);

    while (WEAVE_NO_ERROR == (err = reader.Next()))
    {
        int length               = reader.GetLength();
        const uint8_t * pDataBuf = NULL;
        WeaveLogProgress(DataManagement, "GetStringArray with length %d", length);
        if (length != 0)
        {
            err = reader.GetDataPtr(pDataBuf);
            SuccessOrExit(err);
        }

        std::string val((char *) pDataBuf, length);
        aValueVector.push_back(val);
    }

    // if we have exhausted this container
    if (WEAVE_END_OF_TLV == err)
    {
        err = WEAVE_NO_ERROR;
    }

    err = reader.ExitContainer(OuterContainerType);
    SuccessOrExit(err);

exit:
    WeaveLogFunctError(err);
    return err;
}

template <class T>
WEAVE_ERROR GenericTraitUpdatableDataSink::Get(const char * apPath, T & aValue)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    nl::Weave::TLV::TLVReader reader;
    PacketBuffer * pMsgBuf                = NULL;
    PropertyPathHandle propertyPathHandle = kNullPropertyPathHandle;
    std::map<PropertyPathHandle, PacketBuffer *>::iterator it;

    err = GetSchemaEngine()->MapPathToHandle(apPath, propertyPathHandle);
    SuccessOrExit(err);

    it = mPathTlvDataMap.find(propertyPathHandle);
    VerifyOrExit(it != mPathTlvDataMap.end(), err = WEAVE_ERROR_INVALID_TLV_TAG);

    pMsgBuf = mPathTlvDataMap[propertyPathHandle];

#if WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK
    err = DebugPrettyPrint(pMsgBuf);
    SuccessOrExit(err);
#endif // WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK

    reader.Init(pMsgBuf);
    err = reader.Next();
    SuccessOrExit(err);

    err = reader.Get(aValue);
    SuccessOrExit(err);

exit:
    WeaveLogFunctError(err);
    return err;
}

WEAVE_ERROR GenericTraitUpdatableDataSink::DeleteData(const char * apPath)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    PropertyPathHandle propertyPathHandle = kNullPropertyPathHandle;
    std::map<PropertyPathHandle, PacketBuffer *>::iterator it;

    err = GetSchemaEngine()->MapPathToHandle(apPath, propertyPathHandle);
    SuccessOrExit(err);

    it = mPathTlvDataMap.find(propertyPathHandle);
    VerifyOrExit(it != mPathTlvDataMap.end(), err = WEAVE_ERROR_INVALID_TLV_TAG);

    if (NULL != it->second)
    {
        PacketBuffer::Free(it->second);
        it->second = NULL;
        WeaveLogProgress(DataManagement, "Deleted data in mPathTlvDataMap for path %s", apPath);
    }

    mPathTlvDataMap.erase(it);

    err = ClearUpdated(GetSubscriptionClient(), propertyPathHandle);

exit:
    WeaveLogFunctError(err);
    return err;
}

WEAVE_ERROR
GenericTraitUpdatableDataSink::SetLeafData(PropertyPathHandle aLeafHandle, TLVReader & aReader)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    nl::Weave::TLV::TLVWriter writer;
    PacketBuffer * pMsgBuf = PacketBuffer::New();
    VerifyOrExit(NULL != pMsgBuf, err = WEAVE_ERROR_NO_MEMORY);

    writer.Init(pMsgBuf);

    err = writer.CopyElement(AnonymousTag, aReader);
    SuccessOrExit(err);

    err = writer.Finalize();
    SuccessOrExit(err);

    UpdateTLVDataMap(aLeafHandle, pMsgBuf);
#if WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK
    err = DebugPrettyPrint(pMsgBuf);
    SuccessOrExit(err);
#endif // WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK
    pMsgBuf = NULL;

exit:
    WeaveLogFunctError(err);

    if (NULL != pMsgBuf)
    {
        PacketBuffer::Free(pMsgBuf);
        pMsgBuf = NULL;
    }

    return err;
}

WEAVE_ERROR
GenericTraitUpdatableDataSink::GetLeafData(PropertyPathHandle aLeafHandle, uint64_t aTagToWrite, TLVWriter & aWriter)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    nl::Weave::TLV::TLVReader reader;
    PacketBuffer * pMsgBuf = NULL;

    std::map<PropertyPathHandle, PacketBuffer *>::iterator it = mPathTlvDataMap.find(aLeafHandle);

    VerifyOrExit(it != mPathTlvDataMap.end(), err = WEAVE_ERROR_INVALID_TLV_TAG);

    pMsgBuf = mPathTlvDataMap[aLeafHandle];

#if WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK
    err = DebugPrettyPrint(pMsgBuf);
    SuccessOrExit(err);
#endif // WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK

    reader.Init(pMsgBuf);
    err = reader.Next();
    SuccessOrExit(err);
    err = aWriter.CopyElement(aTagToWrite, reader);
    SuccessOrExit(err);

exit:
    WeaveLogFunctError(err);
    return err;
}

WEAVE_ERROR GenericTraitUpdatableDataSink::GetNextDictionaryItemKey(PropertyPathHandle aDictionaryHandle, uintptr_t & aContext,
                                                                    PropertyDictionaryKey & aKey)
{
    return WEAVE_END_OF_INPUT;
}

#if WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK
void GenericTraitUpdatableDataSink::TLVPrettyPrinter(const char * aFormat, ...)
{
    va_list args;

    va_start(args, aFormat);

    vprintf(aFormat, args);

    va_end(args);
}

WEAVE_ERROR GenericTraitUpdatableDataSink::DebugPrettyPrint(PacketBuffer * apMsgBuf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    nl::Weave::TLV::TLVReader reader;
    reader.Init(apMsgBuf);
    err = reader.Next();
    SuccessOrExit(err);
    nl::Weave::TLV::Debug::Dump(reader, TLVPrettyPrinter);

exit:
    if (WEAVE_NO_ERROR != err)
    {
        WeaveLogProgress(DataManagement, "DebugPrettyPrint fails with err %d", err);
    }

    return err;
}
#endif // WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK

const nl::Weave::ExchangeContext::Timeout kResponseTimeoutMsec = 15000;

// mpWdmEventProcessor is unique_ptr, so we should use nullptr instead of NULL
WdmClient::WdmClient() :
    State(kState_NotInitialized), mpAppState(NULL), mOnError(NULL), mGetDataHandle(NULL), mpPublisherPathList(NULL),
    mpSubscriptionClient(NULL), mpMsgLayer(NULL), mpContext(NULL), mpAppReqState(NULL), mOpState(kOpState_Idle),
    mpWdmEventProcessor(nullptr), mEventStrBuffer(""), mEnableEventFetching(false)
{ }

void WdmClient::Close(void)
{
    if (mpSubscriptionClient != NULL)
    {
        mpSubscriptionClient->DiscardUpdates();
        mpSubscriptionClient->Free();
        mpSubscriptionClient = NULL;
    }

    mSinkCatalog.Iterate(ClearDataSink, this);

    mSinkCatalog.Clear();
    if (mpPublisherPathList != NULL)
    {
        delete[] mpPublisherPathList;
        mpPublisherPathList = NULL;
    }

    mpAppState    = NULL;
    mpContext     = NULL;
    mpMsgLayer    = NULL;
    mpAppReqState = NULL;
    mOnError      = NULL;

    mEventStrBuffer.clear();
    mpWdmEventProcessor.release();
    mEventFetchingTLE = false;

    State = kState_NotInitialized;
    ClearOpState();
}

void WdmClient::ClearDataSink(void * aTraitInstance, TraitDataHandle aHandle, void * aContext)
{
    GenericTraitUpdatableDataSink * pGenericTraitUpdatableDataSink = NULL;
    if (aTraitInstance != NULL)
    {
        pGenericTraitUpdatableDataSink = static_cast<GenericTraitUpdatableDataSink *>(aTraitInstance);
        delete pGenericTraitUpdatableDataSink;
        pGenericTraitUpdatableDataSink = NULL;
    }
}

namespace {
void WriteEscapedString(const char * apStr, size_t aLen, std::string & aBuf)
{
    // According to UTF8 encoding, all bytes from a multiple byte UTF8 sequence
    // will have 1 as most siginificant bit. So this function will output the
    // multi-byte characters without escape.
    constexpr char hex[] = "0123456789abcdef";
    for (size_t i = 0; i < aLen && apStr[i]; i++)
    {
        switch (apStr[i])
        {
        case 0 ... 31:
        case '"':
        case '\\':
        case '/':
            // escape it using JSON style
            aBuf += "\\u00";
            aBuf += hex[(apStr[i] >> 4) & 0xf];
            aBuf += hex[(apStr[i] & 0xf)];
            break;
        default:
            aBuf += apStr[i];
            break;
        }
    }
}

WEAVE_ERROR FormatEventData(TLVReader aInReader, std::string & aBuf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    switch (aInReader.GetType())
    {
    case nl::Weave::TLV::kTLVType_Structure:
    case nl::Weave::TLV::kTLVType_Array:
        break;
    case nl::Weave::TLV::kTLVType_SignedInteger: {
        int64_t value_s64;

        err = aInReader.Get(value_s64);
        SuccessOrExit(err);

        aBuf += to_string(value_s64);
        break;
    }

    case nl::Weave::TLV::kTLVType_UnsignedInteger: {
        uint64_t value_u64;

        err = aInReader.Get(value_u64);
        SuccessOrExit(err);

        aBuf += to_string(value_u64);
        break;
    }

    case nl::Weave::TLV::kTLVType_Boolean: {
        bool value_b;

        err = aInReader.Get(value_b);
        SuccessOrExit(err);

        aBuf += (value_b ? "true" : "false");
        break;
    }

    case nl::Weave::TLV::kTLVType_UTF8String: {
        char value_s[256];

        err = aInReader.GetString(value_s, sizeof(value_s));
        VerifyOrExit(err == WEAVE_NO_ERROR || err == WEAVE_ERROR_BUFFER_TOO_SMALL, );

        if (err == WEAVE_ERROR_BUFFER_TOO_SMALL)
        {
            aBuf += "\"(utf8 string too long)\"";
            err = WEAVE_NO_ERROR;
        }
        else
        {
            aBuf += '"';
            // String data needs escaped
            WriteEscapedString(value_s, min(sizeof(value_s), strlen(value_s)), aBuf);
            aBuf += '"';
        }
        break;
    }

    case nl::Weave::TLV::kTLVType_ByteString: {
        uint8_t value_b[256];
        uint32_t len, readerLen;

        readerLen = aInReader.GetLength();

        err = aInReader.GetBytes(value_b, sizeof(value_b));
        VerifyOrExit(err == WEAVE_NO_ERROR || err == WEAVE_ERROR_BUFFER_TOO_SMALL, );

        if (readerLen < sizeof(value_b))
        {
            len = readerLen;
        }
        else
        {
            len = sizeof(value_b);
        }

        if (err == WEAVE_ERROR_BUFFER_TOO_SMALL)
        {
            aBuf += "\"(byte string too long)\"";
        }
        else
        {
            aBuf += "[";
            for (size_t i = 0; i < len; i++)
            {
                aBuf += (i > 0 ? "," : "");
                aBuf += to_string(value_b[i]);
            }
            aBuf += "]";
        }
        break;
    }

    case nl::Weave::TLV::kTLVType_Null:
        aBuf += "null";
        break;

    default:
        aBuf += "\"<error data>\"";
        break;
    }

    if (aInReader.GetType() == nl::Weave::TLV::kTLVType_Structure || aInReader.GetType() == nl::Weave::TLV::kTLVType_Array)
    {
        bool insideStructure = (aInReader.GetType() == nl::Weave::TLV::kTLVType_Structure);
        aBuf += (insideStructure ? '{' : '[');
        const char terminating_char = (insideStructure ? '}' : ']');

        nl::Weave::TLV::TLVType type;
        bool isFirstChild = true;
        err               = aInReader.EnterContainer(type);
        SuccessOrExit(err);

        while ((err = aInReader.Next()) == WEAVE_NO_ERROR)
        {
            if (!isFirstChild)
                aBuf += ",";
            isFirstChild = false;
            if (insideStructure)
            {
                uint32_t tagNum = nl::Weave::TLV::TagNumFromTag(aInReader.GetTag());
                aBuf += "\"" + to_string(tagNum) + "\":";
            }
            err = FormatEventData(aInReader, aBuf);
            SuccessOrExit(err);
        }

        aBuf += terminating_char;

        err = aInReader.ExitContainer(type);
        SuccessOrExit(err);
    }

exit:
    return err;
}
} // namespace

void WdmClient::ClearDataSinkVersion(void * aTraitInstance, TraitDataHandle aHandle, void * aContext)
{
    GenericTraitUpdatableDataSink * pGenericTraitUpdatableDataSink = NULL;
    if (aTraitInstance != NULL)
    {
        pGenericTraitUpdatableDataSink = static_cast<GenericTraitUpdatableDataSink *>(aTraitInstance);
        pGenericTraitUpdatableDataSink->ClearVersion();
    }
}

void WdmClient::ClientEventCallback(void * const aAppState, SubscriptionClient::EventID aEvent,
                                    const SubscriptionClient::InEventParam & aInParam,
                                    SubscriptionClient::OutEventParam & aOutParam)
{
    WEAVE_ERROR err              = WEAVE_NO_ERROR;
    WdmClient * const pWdmClient = reinterpret_cast<WdmClient *>(aAppState);
    DeviceStatus * deviceStatus  = NULL;
    OpState savedOpState         = pWdmClient->mOpState;
    WeaveLogDetail(DataManagement, "WdmClient ClientEventCallback: current op is, %d, event is %d", savedOpState, aEvent);

    switch (aEvent)
    {
    case SubscriptionClient::kEvent_OnExchangeStart:
        WeaveLogDetail(DataManagement, "Client->kEvent_OnExchangeStart");
        break;
    case SubscriptionClient::kEvent_OnSubscribeRequestPrepareNeeded:
        WeaveLogDetail(DataManagement, "Client->kEvent_OnSubscribeRequestPrepareNeeded");
        VerifyOrExit(kOpState_RefreshData == savedOpState, err = WEAVE_ERROR_INCORRECT_STATE);
        {
            uint16_t pathListLen  = 0;
            uint16_t traitListLen = 0;
            uint32_t eventListLen = 0;
            TraitDataHandle handle;
            bool needSubscribeSpecificTrait = false;

            if (pWdmClient->mGetDataHandle != NULL && pWdmClient->mpContext != NULL)
            {
                err = pWdmClient->mGetDataHandle(pWdmClient->mpContext, &pWdmClient->mSinkCatalog, handle);
                SuccessOrExit(err);
                traitListLen               = 1;
                needSubscribeSpecificTrait = true;
            }
            else
            {
                traitListLen = pWdmClient->mSinkCatalog.Size();
            }
            WeaveLogDetail(DataManagement, "prepare to subscribe %d trait data sink", traitListLen);

            if (!pWdmClient->mEnableEventFetching)
            {
                if (pWdmClient->mpPublisherPathList != NULL)
                {
                    delete[] pWdmClient->mpPublisherPathList;
                    pWdmClient->mpPublisherPathList = NULL;
                }

                if (traitListLen > 0)
                {
                    pWdmClient->mpPublisherPathList = new TraitPath[traitListLen];
                }

                if (needSubscribeSpecificTrait)
                {
                    pathListLen = 1;
                    err = pWdmClient->mSinkCatalog.PrepareSubscriptionSpecificPathList(pWdmClient->mpPublisherPathList, traitListLen,
                                                                                    handle);
                }
                else
                {
                    err = pWdmClient->mSinkCatalog.PrepareSubscriptionPathList(pWdmClient->mpPublisherPathList, traitListLen,
                                                                            pathListLen);
                }
                SuccessOrExit(err);
            }
            else
            {
                // When mEnableEventFetching is true, we will not subscribe to any traits
                pWdmClient->mpPublisherPathList = NULL;
                pathListLen = 0;

                err = pWdmClient->PrepareLastObservedEventList(eventListLen);
                SuccessOrExit(err);
            }

            // Reset the tle state
            pWdmClient->mEventFetchingTLE = false;

            aOutParam.mSubscribeRequestPrepareNeeded.mPathList      = pWdmClient->mpPublisherPathList;
            aOutParam.mSubscribeRequestPrepareNeeded.mPathListSize  = pathListLen;
            aOutParam.mSubscribeRequestPrepareNeeded.mNeedAllEvents = pWdmClient->mEnableEventFetching;
            // When aEnableEventFetching is false, the eventListLen will be 0
            aOutParam.mSubscribeRequestPrepareNeeded.mLastObservedEventList = pWdmClient->mLastObservedEventByImportanceForSending;
            aOutParam.mSubscribeRequestPrepareNeeded.mLastObservedEventListSize = eventListLen;
            aOutParam.mSubscribeRequestPrepareNeeded.mTimeoutSecMin             = 30;
            aOutParam.mSubscribeRequestPrepareNeeded.mTimeoutSecMax             = 120;
        }
        break;

    case SubscriptionClient::kEvent_OnSubscriptionEstablished:
        WeaveLogDetail(DataManagement, "Client->kEvent_OnSubscriptionEstablished");
        pWdmClient->mpSubscriptionClient->AbortSubscription();
        VerifyOrExit(kOpState_RefreshData == pWdmClient->mOpState, err = WEAVE_ERROR_INCORRECT_STATE);
        pWdmClient->mOnComplete.General(pWdmClient->mpContext, pWdmClient->mpAppReqState);
        pWdmClient->mpContext = NULL;
        pWdmClient->ClearOpState();
        break;
    case SubscriptionClient::kEvent_OnNotificationRequest:
        WeaveLogDetail(DataManagement, "Client->kEvent_OnNotificationRequest");
        VerifyOrExit(kOpState_RefreshData == pWdmClient->mOpState, err = WEAVE_ERROR_INCORRECT_STATE);
        break;
    case SubscriptionClient::kEvent_OnNotificationProcessed:
        WeaveLogDetail(DataManagement, "Client->kEvent_OnNotificationProcessed");
        VerifyOrExit((kOpState_RefreshData == pWdmClient->mOpState) || pWdmClient->mEventFetchingTLE, err = WEAVE_ERROR_INCORRECT_STATE);
        break;
    case SubscriptionClient::kEvent_OnSubscriptionTerminated:
        WeaveLogDetail(DataManagement, "Client->kEvent_OnSubscriptionTerminated. Reason: %u, peer = 0x%" PRIX64 "\n",
                       aInParam.mSubscriptionTerminated.mReason, aInParam.mSubscriptionTerminated.mClient->GetPeerNodeId());
        VerifyOrExit(kOpState_RefreshData == savedOpState, err = WEAVE_ERROR_INCORRECT_STATE);
        deviceStatus                  = new DeviceStatus();
        deviceStatus->StatusProfileId = aInParam.mSubscriptionTerminated.mStatusProfileId;
        deviceStatus->StatusCode      = aInParam.mSubscriptionTerminated.mStatusCode;
        deviceStatus->SystemErrorCode = aInParam.mSubscriptionTerminated.mReason;
        err                           = aInParam.mSubscriptionTerminated.mReason;
        break;
    case SubscriptionClient::kEvent_OnUpdateComplete:
        VerifyOrExit(kOpState_FlushUpdate == savedOpState, err = WEAVE_ERROR_INCORRECT_STATE);
        if ((aInParam.mUpdateComplete.mReason == WEAVE_NO_ERROR) &&
            (nl::Weave::Profiles::kWeaveProfile_Common == aInParam.mUpdateComplete.mStatusProfileId) &&
            (nl::Weave::Profiles::Common::kStatus_Success == aInParam.mUpdateComplete.mStatusCode))
        {
            WeaveLogDetail(DataManagement, "Update: path result: success, tdh %" PRIu16 ", ph %" PRIu16 ".",
                           aInParam.mUpdateComplete.mTraitDataHandle, aInParam.mUpdateComplete.mPropertyPathHandle);
        }
        else
        {

            WeaveLogDetail(DataManagement,
                           "Update: path failed: %s, %s, tdh %" PRIu16 ", ph %" PRIu16 ", %s, discard failed change",
                           ErrorStr(aInParam.mUpdateComplete.mReason),
                           nl::StatusReportStr(aInParam.mUpdateComplete.mStatusProfileId, aInParam.mUpdateComplete.mStatusCode),
                           aInParam.mUpdateComplete.mTraitDataHandle, aInParam.mUpdateComplete.mPropertyPathHandle,
                           aInParam.mUpdateComplete.mWillRetry ? "will retry" : "will not retry");

            err = pWdmClient->UpdateFailedPathResults(
                pWdmClient, aInParam.mUpdateComplete.mTraitDataHandle, aInParam.mUpdateComplete.mPropertyPathHandle,
                aInParam.mUpdateComplete.mReason, aInParam.mUpdateComplete.mStatusProfileId, aInParam.mUpdateComplete.mStatusCode);
        }

        break;
    case SubscriptionClient::kEvent_OnNoMorePendingUpdates:
        WeaveLogDetail(DataManagement, "Update: no more pending updates");
        VerifyOrExit(kOpState_FlushUpdate == savedOpState, err = WEAVE_ERROR_INCORRECT_STATE);
        pWdmClient->mpSubscriptionClient->DiscardUpdates();
        pWdmClient->mOnComplete.FlushUpdate(pWdmClient->mpContext, pWdmClient->mpAppReqState,
                                            pWdmClient->mFailedFlushPathStatus.size(), pWdmClient->mFailedFlushPathStatus.data());
        pWdmClient->mFailedFlushPathStatus.clear();
        pWdmClient->mFailedPaths.clear();
        pWdmClient->mpContext = NULL;
        pWdmClient->ClearOpState();
        break;

    case SubscriptionClient::kEvent_OnEventStreamReceived:
        WeaveLogDetail(DataManagement, "kEvent_OnEventStreamReceived");

        if (pWdmClient->mpWdmEventProcessor != nullptr)
        {
            // Remove ']' at end of buffer and append data then push ']' back to the buffer
            pWdmClient->mEventStrBuffer.pop_back();
            err = pWdmClient->mpWdmEventProcessor->ProcessEvents(*(aInParam.mEventStreamReceived.mReader),
                                                                *(pWdmClient->mpSubscriptionClient));
            pWdmClient->mEventStrBuffer += ']';

            SuccessOrExit(err);

            if (pWdmClient->mLimitEventFetchTimeout && std::chrono::system_clock::now() >= pWdmClient->mEventFetchTimeout)
            {
                pWdmClient->mEventFetchingTLE = true;
                pWdmClient->mpSubscriptionClient->AbortSubscription();
                VerifyOrExit(kOpState_RefreshData == pWdmClient->mOpState, err = WEAVE_ERROR_INCORRECT_STATE);
                pWdmClient->mOnComplete.General(pWdmClient->mpContext, pWdmClient->mpAppReqState);
                pWdmClient->mpContext = NULL;
                pWdmClient->ClearOpState();
            }
        }
        break;

    default:
        SubscriptionClient::DefaultEventHandler(aEvent, aInParam, aOutParam);
        break;
    }

exit:
    if (WEAVE_NO_ERROR != err)
    {
        WeaveLogError(DataManagement, "WdmClient ClientEventCallback failure: err = %d", err);

        if (pWdmClient->mOnError)
        {
            pWdmClient->mOnError(pWdmClient->mpContext, pWdmClient->mpAppReqState, err, deviceStatus);
            pWdmClient->mOnError = NULL;
        }

        if (kOpState_FlushUpdate == savedOpState)
        {
            pWdmClient->mpSubscriptionClient->DiscardUpdates();
            pWdmClient->mFailedFlushPathStatus.clear();
            pWdmClient->mFailedPaths.clear();
        }

        if (kOpState_RefreshData == savedOpState)
        {
            pWdmClient->mpSubscriptionClient->AbortSubscription();
            pWdmClient->mSinkCatalog.Iterate(ClearDataSinkVersion, pWdmClient);
        }

        pWdmClient->mpContext = NULL;
        pWdmClient->ClearOpState();
    }

    if (deviceStatus != NULL)
    {
        delete deviceStatus;
        deviceStatus = NULL;
    }

    return;
}

WEAVE_ERROR WdmClient::Init(WeaveMessageLayer * apMsgLayer, Binding * apBinding)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    mpMsgLayer      = apMsgLayer;

    VerifyOrExit(State == kState_NotInitialized, err = WEAVE_ERROR_INCORRECT_STATE);

    if (mpSubscriptionClient == NULL)
    {
        err = SubscriptionEngine::GetInstance()->NewClient(
            &mpSubscriptionClient, apBinding, this, ClientEventCallback, &mSinkCatalog,
            kResponseTimeoutMsec * 2); // max num of msec between subscribe request and subscribe response
        SuccessOrExit(err);
    }

    mpSubscriptionClient->EnableResubscribe(NULL);

    State     = kState_Initialized;
    mpContext = NULL;
    ClearOpState();
    mFailedFlushPathStatus.clear();
    mFailedPaths.clear();

    mEventStrBuffer.clear();
    mLimitEventFetchTimeout = false;
    memset(mLastObservedEventByImportance, 0, sizeof mLastObservedEventByImportance);

    mEventFetchingTLE = false;

exit:
    return WEAVE_NO_ERROR;
}

void WdmClient::SetNodeId(uint64_t aNodeId)
{
    mSinkCatalog.SetNodeId(aNodeId);

    mpWdmEventProcessor.reset(new WdmEventProcessor(aNodeId, this));

    WeaveLogError(DataManagement, "mpWdmEventProcessor set to %p", mpWdmEventProcessor.get());
}

void WdmClient::SetEventFetchingTimeout(uint32_t aTimeoutSec)
{
    mLimitEventFetchTimeout = (aTimeoutSec != 0);
    mEventFetchTimeLimit    = std::chrono::seconds(aTimeoutSec);
}

WEAVE_ERROR WdmClient::UpdateFailedPathResults(WdmClient * const apWdmClient, TraitDataHandle mTraitDataHandle,
                                               PropertyPathHandle mPropertyPathHandle, uint32_t aReason, uint32_t aStatusProfileId,
                                               uint16_t aStatusCode)
{
    TraitDataSink * dataSink = NULL;
    WEAVE_ERROR err          = WEAVE_NO_ERROR;
    WdmClientFlushUpdateStatus updateStatus;
    std::string path;

    err = apWdmClient->mSinkCatalog.Locate(mTraitDataHandle, &dataSink);
    SuccessOrExit(err);

    err = dataSink->GetSchemaEngine()->MapHandleToPath(mPropertyPathHandle, path);
    SuccessOrExit(err);

    mFailedPaths.push_back(path);
    updateStatus.mpPath                     = mFailedPaths.back().c_str();
    updateStatus.mPathLen                   = mFailedPaths.back().length();
    updateStatus.mErrorCode                 = aReason;
    updateStatus.mDevStatus.SystemErrorCode = aReason;
    updateStatus.mDevStatus.StatusProfileId = aStatusProfileId;
    updateStatus.mDevStatus.StatusCode      = aStatusCode;
    updateStatus.mpDataSink                 = dataSink;
    mFailedFlushPathStatus.push_back(updateStatus);
    WeaveLogError(DataManagement, "Update: faild path is %s, length is %d", updateStatus.mpPath, updateStatus.mPathLen);

exit:
    if (WEAVE_NO_ERROR != err)
    {
        WeaveLogError(DataManagement, "Fail in UpdateFailedPathResults with err = %d", err);
    }
    return err;
}

WEAVE_ERROR WdmClient::NewDataSink(const ResourceIdentifier & aResourceId, uint32_t aProfileId, uint64_t aInstanceId,
                                   const char * apPath, GenericTraitUpdatableDataSink *& apGenericTraitUpdatableDataSink)
{
    WEAVE_ERROR err           = WEAVE_NO_ERROR;
    PropertyPathHandle handle = kNullPropertyPathHandle;

    const TraitSchemaEngine * pEngine = TraitSchemaDirectory::GetTraitSchemaEngine(aProfileId);
    VerifyOrExit(pEngine != NULL, err = WEAVE_ERROR_INVALID_PROFILE_ID);

    VerifyOrExit(mpSubscriptionClient != NULL, err = WEAVE_ERROR_INCORRECT_STATE);

    VerifyOrExit(WEAVE_NO_ERROR != GetDataSink(aResourceId, aProfileId, aInstanceId, apGenericTraitUpdatableDataSink),
                 WeaveLogDetail(DataManagement, "Trait exist"));

    apGenericTraitUpdatableDataSink = new GenericTraitUpdatableDataSink(pEngine, this);
    VerifyOrExit(apGenericTraitUpdatableDataSink != NULL, err = WEAVE_ERROR_NO_MEMORY);

    if (apPath == NULL)
    {
        handle = kRootPropertyPathHandle;
    }
    else
    {
        err = apGenericTraitUpdatableDataSink->GetSchemaEngine()->MapPathToHandle(apPath, handle);
        SuccessOrExit(err);
    }

    err = SubscribePublisherTrait(aResourceId, aInstanceId, handle, apGenericTraitUpdatableDataSink);
    SuccessOrExit(err);

    apGenericTraitUpdatableDataSink->SetSubscriptionClient(mpSubscriptionClient);

exit:
    WeaveLogFunctError(err);
    return err;
}

WEAVE_ERROR WdmClient::GetDataSink(const ResourceIdentifier & aResourceId, uint32_t aProfileId, uint64_t aInstanceId,
                                   GenericTraitUpdatableDataSink *& apGenericTraitUpdatableDataSink)
{
    WEAVE_ERROR err          = WEAVE_NO_ERROR;
    TraitDataSink * dataSink = NULL;

    err = mSinkCatalog.Locate(aProfileId, aInstanceId, aResourceId, &dataSink);
    SuccessOrExit(err);

    apGenericTraitUpdatableDataSink = static_cast<GenericTraitUpdatableDataSink *>(dataSink);

exit:
    return err;
}

WEAVE_ERROR WdmClient::FlushUpdate(void * apAppReqState, DMFlushUpdateCompleteFunct onComplete, DMErrorFunct onError)
{
    VerifyOrExit(mOpState == kOpState_Idle, WeaveLogError(DataManagement, "FlushUpdate with OpState %d", mOpState));

    mpAppReqState           = apAppReqState;
    mOnComplete.FlushUpdate = onComplete;
    mOnError                = onError;
    mOpState                = kOpState_FlushUpdate;
    mpContext               = this;

    mFailedFlushPathStatus.clear();
    mFailedPaths.clear();

    mpSubscriptionClient->FlushUpdate(true);

exit:
    return WEAVE_NO_ERROR;
}

WEAVE_ERROR WdmClient::RefreshData(void * apAppReqState, DMCompleteFunct onComplete, DMErrorFunct onError,
                                   GetDataHandleFunct getDataHandleCb, bool aFetchEvents)
{
    VerifyOrExit(mpSubscriptionClient != NULL, WeaveLogError(DataManagement, "mpSubscriptionClient is NULL"));

    mSinkCatalog.Iterate(ClearDataSinkVersion, this);

    RefreshData(apAppReqState, this, onComplete, onError, getDataHandleCb, aFetchEvents);

exit:
    return WEAVE_NO_ERROR;
}

WEAVE_ERROR WdmClient::RefreshData(void * apAppReqState, void * apContext, DMCompleteFunct onComplete, DMErrorFunct onError,
                                   GetDataHandleFunct getDataHandleCb, bool aFetchEvents)
{
    VerifyOrExit(mOpState == kOpState_Idle, WeaveLogError(DataManagement, "RefreshData with OpState %d", mOpState));

    mpAppReqState        = apAppReqState;
    mOnComplete.General  = onComplete;
    mOnError             = onError;
    mOpState             = kOpState_RefreshData;
    mGetDataHandle       = getDataHandleCb;
    mpContext            = apContext;
    mEnableEventFetching       = aFetchEvents;

    mpSubscriptionClient->InitiateSubscription();

exit:
    return WEAVE_NO_ERROR;
}

void WdmClient::ClearOpState()
{
    mOpState = kOpState_Idle;
}

WEAVE_ERROR WdmClient::SubscribePublisherTrait(const ResourceIdentifier & aResourceId, const uint64_t & aInstanceId,
                                               PropertyPathHandle aBasePathHandle, TraitDataSink * apDataSink)
{
    TraitDataHandle traitHandle;
    return mSinkCatalog.Add(aResourceId, aInstanceId, aBasePathHandle, apDataSink, traitHandle);
}

WEAVE_ERROR WdmClient::UnsubscribePublisherTrait(TraitDataSink * apDataSink)
{
    return mSinkCatalog.Remove(apDataSink);
}

WEAVE_ERROR WdmClient::GetEvents(BytesData * apBytesData)
{
    apBytesData->mpDataBuf = reinterpret_cast<const uint8_t *>(mEventStrBuffer.c_str());
    apBytesData->mDataLen  = mEventStrBuffer.size();
    apBytesData->mpMsgBuf  = NULL;

    return WEAVE_NO_ERROR;
}

WEAVE_ERROR WdmClient::ProcessEvent(nl::Weave::TLV::TLVReader inReader, const EventProcessor::EventHeader & inEventHeader)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (mEventStrBuffer.size() > 1)
    {
        // We will insert a bracket first, so the EventStrBuffer will be 1 for first event
        mEventStrBuffer += ",";
    }

    mEventStrBuffer += "{";

    mEventStrBuffer += "\"Source\":";
    mEventStrBuffer += to_string(inEventHeader.mSource);
    mEventStrBuffer += ",\"Importance\":";
    mEventStrBuffer += to_string(inEventHeader.mImportance);
    mEventStrBuffer += ",\"Id\":";
    mEventStrBuffer += to_string(inEventHeader.mId);

    mEventStrBuffer += ",\"RelatedImportance\":";
    mEventStrBuffer += to_string(inEventHeader.mRelatedImportance);
    mEventStrBuffer += ",\"RelatedId\":";
    mEventStrBuffer += to_string(inEventHeader.mRelatedId);
    mEventStrBuffer += ",\"UTCTimestamp\":";
    mEventStrBuffer += to_string(inEventHeader.mUTCTimestamp);
    mEventStrBuffer += ",\"SystemTimestamp\":";
    mEventStrBuffer += to_string(inEventHeader.mSystemTimestamp);
    mEventStrBuffer += ",\"ResourceId\":";
    mEventStrBuffer += to_string(inEventHeader.mResourceId);
    mEventStrBuffer += ",\"TraitProfileId\":";
    mEventStrBuffer += to_string(inEventHeader.mTraitProfileId);
    mEventStrBuffer += ",\"TraitInstanceId\":";
    mEventStrBuffer += to_string(inEventHeader.mTraitInstanceId);
    mEventStrBuffer += ",\"Type\":";
    mEventStrBuffer += to_string(inEventHeader.mType);

    mEventStrBuffer += ",\"DeltaUTCTime\":";
    mEventStrBuffer += to_string(inEventHeader.mDeltaUTCTime);
    mEventStrBuffer += ",\"DeltaSystemTime\":";
    mEventStrBuffer += to_string(inEventHeader.mDeltaSystemTime);

    mEventStrBuffer += ",\"PresenceMask\":";
    mEventStrBuffer += to_string(inEventHeader.mPresenceMask);
    mEventStrBuffer += ",\"DataSchemaVersionRange\": {\"MinVersion\":";
    mEventStrBuffer += to_string(inEventHeader.mDataSchemaVersionRange.mMinVersion);
    mEventStrBuffer += ",\"MaxVersion\":";
    mEventStrBuffer += to_string(inEventHeader.mDataSchemaVersionRange.mMaxVersion);
    mEventStrBuffer += "}";

    mEventStrBuffer += ",\"Data\":";
    err = FormatEventData(inReader, mEventStrBuffer);

    mEventStrBuffer += "}";

    mLastObservedEventByImportance[static_cast<int>(inEventHeader.mImportance) - static_cast<int>(kImportanceType_First)]
        .mSourceId = inEventHeader.mSource;
    mLastObservedEventByImportance[static_cast<int>(inEventHeader.mImportance) - static_cast<int>(kImportanceType_First)]
        .mImportance = inEventHeader.mImportance;
    mLastObservedEventByImportance[static_cast<int>(inEventHeader.mImportance) - static_cast<int>(kImportanceType_First)].mEventId =
        inEventHeader.mId;

    return err;
}

WEAVE_ERROR WdmClient::PrepareLastObservedEventList(uint32_t & aEventListLen)
{
    for (int i = 0; i < kImportanceType_Last - kImportanceType_First + 1; i++)
    {
        if (mLastObservedEventByImportance[i].mEventId)
        {
            mLastObservedEventByImportanceForSending[aEventListLen++] =
                mLastObservedEventByImportance[i];
        }
    }

    mEventFetchTimeout = std::chrono::system_clock::now() + mEventFetchTimeLimit;
    mEventStrBuffer    = "[]";

    return WEAVE_NO_ERROR;
}

WdmEventProcessor::WdmEventProcessor(uint64_t aNodeId, WdmClient * aWdmClient) : EventProcessor(aNodeId), mWdmClient(aWdmClient)
{
    // nothing to do
}

WEAVE_ERROR WdmEventProcessor::ProcessEvent(nl::Weave::TLV::TLVReader inReader,
                                            nl::Weave::Profiles::DataManagement::SubscriptionClient & inClient,
                                            const EventHeader & inEventHeader)
{
    return mWdmClient->ProcessEvent(inReader, inEventHeader);
}

WEAVE_ERROR WdmEventProcessor::GapDetected(const EventHeader & inEventHeader)
{
    // Do nothing
    return WEAVE_NO_ERROR;
};

} // namespace DeviceManager
} // namespace Weave
} // namespace nl
#endif // WEAVE_CONFIG_DATA_MANAGEMENT_CLIENT_EXPERIMENTAL
