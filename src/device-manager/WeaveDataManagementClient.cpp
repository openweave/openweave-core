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

#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <time.h>

#include <Weave/Profiles/data-management/Current/WdmManagedNamespace.h>

#include <Weave/Core/WeaveCore.h>
#include <Weave/Support/Base64.h>
#include <Weave/Support/CodeUtils.h>
#include <Weave/Core/WeaveEncoding.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Profiles/common/CommonProfile.h>
#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Support/logging/WeaveLogging.h>
#include <Weave/Support/ErrorStr.h>
#include <Weave/Support/TimeUtils.h>

#include <Weave/Support/CodeUtils.h>
#include <Weave/DeviceManager/WeaveDataManagementClient.h>
#include <Weave/DeviceManager/TraitSchemaDirectory.h>

namespace nl {
namespace Weave {
namespace DeviceManager {

using namespace nl::Weave::Encoding;
using namespace nl::Weave::Profiles;
using namespace nl::Weave::Profiles::DataManagement;
using namespace ::nl::Weave;
using namespace ::nl::Weave::TLV;
using namespace Schema::Weave::Common;

class GenericTraitUpdatableDataSink;
class WdmClient;

GenericTraitUpdatableDataSink::GenericTraitUpdatableDataSink(const nl::Weave::Profiles::DataManagement::TraitSchemaEngine * apEngine, WdmClient *apWdmClient)
        : TraitUpdatableDataSink(apEngine)
{
    mpWdmClient = apWdmClient;
}

GenericTraitUpdatableDataSink::~GenericTraitUpdatableDataSink()
{
    Clear();
}

void GenericTraitUpdatableDataSink::Clear(void)
{
    SubscriptionClient *pSubscriptionClient = GetSubscriptionClient();
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

void GenericTraitUpdatableDataSink::UpdateTLVDataMap(PropertyPathHandle aPropertyPathHandle, PacketBuffer *apMsgBuf)
{
    if ((mPathTlvDataMap.find(aPropertyPathHandle) != mPathTlvDataMap.end()) && (NULL != mPathTlvDataMap[aPropertyPathHandle]))
    {
        PacketBuffer::Free(mPathTlvDataMap[aPropertyPathHandle]);
    }
    mPathTlvDataMap[aPropertyPathHandle] = apMsgBuf;
}

WEAVE_ERROR GenericTraitUpdatableDataSink::LocateTraitHandle(void* apContext, const TraitCatalogBase<TraitDataSink> * const apCatalog, TraitDataHandle & aHandle)
{
    GenericTraitUpdatableDataSink * pGenericTraitUpdatableDataSink = static_cast<GenericTraitUpdatableDataSink *>(apContext);
    return apCatalog->Locate(pGenericTraitUpdatableDataSink, aHandle);
}

WEAVE_ERROR GenericTraitUpdatableDataSink::RefreshData(void* appReqState, DMCompleteFunct onComplete, DMErrorFunct onError)
{
    ClearVersion();
    mpWdmClient->RefreshData(appReqState, this, onComplete, onError, LocateTraitHandle);
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
    PacketBuffer *pMsgBuf = PacketBuffer::New();
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
#endif //WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK

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

WEAVE_ERROR GenericTraitUpdatableDataSink::SetString(const char * apPath, const char * aValue, bool aIsConditional)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    nl::Weave::TLV::TLVWriter writer;
    PropertyPathHandle propertyPathHandle = kNullPropertyPathHandle;
    PacketBuffer *pMsgBuf = PacketBuffer::New();
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
#endif //WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK

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

WEAVE_ERROR GenericTraitUpdatableDataSink::SetNull(const char * apPath, bool aIsConditional)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    nl::Weave::TLV::TLVWriter writer;
    PropertyPathHandle propertyPathHandle = kNullPropertyPathHandle;
    PacketBuffer *pMsgBuf = PacketBuffer::New();
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
#endif //WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK

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

WEAVE_ERROR GenericTraitUpdatableDataSink::SetBytes(const char * apPath, const uint8_t * dataBuf, size_t dataLen, bool aIsConditional)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    nl::Weave::TLV::TLVWriter writer;
    nl::Weave::TLV::TLVReader reader;
    PropertyPathHandle propertyPathHandle = kNullPropertyPathHandle;
    PacketBuffer *pMsgBuf = PacketBuffer::New();
    VerifyOrExit(NULL != pMsgBuf, err = WEAVE_ERROR_NO_MEMORY);

    VerifyOrExit(GetSubscriptionClient() != NULL, err = WEAVE_ERROR_INCORRECT_STATE);

    Lock(GetSubscriptionClient());

    err = GetSchemaEngine()->MapPathToHandle(apPath, propertyPathHandle);
    SuccessOrExit(err);

    writer.Init(pMsgBuf);

    reader.Init(dataBuf, dataLen);
    reader.Next();
    writer.CopyElement(AnonymousTag, reader);

    err = writer.Finalize();
    SuccessOrExit(err);

#if WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK
    err = DebugPrettyPrint(pMsgBuf);
    SuccessOrExit(err);
#endif //WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK

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

WEAVE_ERROR GenericTraitUpdatableDataSink::SetTLVBytes(const char * apPath, const uint8_t * dataBuf, size_t dataLen, bool aIsConditional)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TLVReader reader;
    PropertyPathHandle propertyPathHandle = kNullPropertyPathHandle;
    TraitSchemaEngine::ISetDataDelegate *setDataDelegate = NULL;

    VerifyOrExit(GetSubscriptionClient() != NULL, err = WEAVE_ERROR_INCORRECT_STATE);
    Lock(GetSubscriptionClient());

    err = GetSchemaEngine()->MapPathToHandle(apPath, propertyPathHandle);
    SuccessOrExit(err);

    reader.Init(dataBuf, dataLen);
    err = reader.Next();
    SuccessOrExit(err);

    setDataDelegate = static_cast<TraitSchemaEngine::ISetDataDelegate *>(this);
    err = GetSchemaEngine()->StoreData(propertyPathHandle, reader, setDataDelegate, NULL);
    SuccessOrExit(err);

    err = SetUpdated(GetSubscriptionClient(), propertyPathHandle, aIsConditional);

    Unlock(GetSubscriptionClient());
    WeaveLogDetail(DataManagement, "<set updated> in 0x%08x", propertyPathHandle);

exit:
    WeaveLogFunctError(err);

    return err;
}

template <class T>
WEAVE_ERROR GenericTraitUpdatableDataSink::Set(const char * apPath, T aValue, bool aIsConditional)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    nl::Weave::TLV::TLVWriter writer;
    PropertyPathHandle propertyPathHandle = kNullPropertyPathHandle;
    PacketBuffer *pMsgBuf = PacketBuffer::New();
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
#endif //WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK

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

WEAVE_ERROR GenericTraitUpdatableDataSink::GetData(const char * apPath, int64_t& aValue)
{
    return Get(apPath, aValue);
}

WEAVE_ERROR GenericTraitUpdatableDataSink::GetData(const char * apPath, uint64_t& aValue)
{
    return Get(apPath, aValue);
}

WEAVE_ERROR GenericTraitUpdatableDataSink::GetData(const char * apPath, double& aValue)
{
    return Get(apPath, aValue);
}

WEAVE_ERROR GenericTraitUpdatableDataSink::GetBoolean(const char * apPath, bool& aValue)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    nl::Weave::TLV::TLVReader reader;
    PacketBuffer *pMsgBuf = NULL;
    PropertyPathHandle propertyPathHandle = kNullPropertyPathHandle;;
    std::map<PropertyPathHandle, PacketBuffer *>::iterator it;

    err = GetSchemaEngine()->MapPathToHandle(apPath, propertyPathHandle);
    SuccessOrExit(err);

    it = mPathTlvDataMap.find(propertyPathHandle);
    VerifyOrExit(it != mPathTlvDataMap.end(), err = WEAVE_ERROR_INCORRECT_STATE);

    pMsgBuf = mPathTlvDataMap[propertyPathHandle];

#if WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK
    err = DebugPrettyPrint(pMsgBuf);
    SuccessOrExit(err);
#endif //WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK

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
    PacketBuffer *pMsgBuf = NULL;
    PropertyPathHandle propertyPathHandle = kNullPropertyPathHandle;;
    std::map<PropertyPathHandle, PacketBuffer *>::iterator it;

    err = GetSchemaEngine()->MapPathToHandle(apPath, propertyPathHandle);
    SuccessOrExit(err);

    it = mPathTlvDataMap.find(propertyPathHandle);
    VerifyOrExit(it != mPathTlvDataMap.end(), err = WEAVE_ERROR_INCORRECT_STATE);

    pMsgBuf = mPathTlvDataMap[propertyPathHandle];

#if WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK
    err = DebugPrettyPrint(pMsgBuf);
    SuccessOrExit(err);
#endif //WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK

    reader.Init(pMsgBuf);
    err = reader.Next();
    SuccessOrExit(err);

    apBytesData->mDataLen = reader.GetLength();
    err = reader.GetDataPtr(apBytesData->mpDataBuf);
    SuccessOrExit(err);

exit:
    WeaveLogFunctError(err);
    return err;
}

WEAVE_ERROR GenericTraitUpdatableDataSink::GetTLVBytes(const char * apPath, BytesData * apBytesData)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TraitSchemaEngine::IGetDataDelegate *getDataDelegate = NULL;
    TLVType dummyContainerType;
    nl::Weave::TLV::TLVWriter writer;
    PropertyPathHandle propertyPathHandle = kNullPropertyPathHandle;

    PacketBuffer *pMsgBuf = PacketBuffer::New();
    VerifyOrExit(NULL != pMsgBuf, err = WEAVE_ERROR_NO_MEMORY);

    VerifyOrExit(NULL != apBytesData, err = WEAVE_ERROR_INCORRECT_STATE);

    err = GetSchemaEngine()->MapPathToHandle(apPath, propertyPathHandle);
    SuccessOrExit(err);

    writer.Init(pMsgBuf);
    err = writer.StartContainer(AnonymousTag, kTLVType_Structure, dummyContainerType);
    SuccessOrExit(err);

    getDataDelegate = static_cast<TraitSchemaEngine::IGetDataDelegate *>(this);
    err = GetSchemaEngine()->RetrieveData(propertyPathHandle, ContextTag(DataElement::kCsTag_Data), writer, getDataDelegate);
    SuccessOrExit(err);

    err = writer.EndContainer(dummyContainerType);
    SuccessOrExit(err);

    err = writer.Finalize();
    SuccessOrExit(err);

#if WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK
    err = DebugPrettyPrint(pMsgBuf);
    SuccessOrExit(err);
#endif //WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK

    apBytesData->mpDataBuf = pMsgBuf->Start();
    apBytesData->mDataLen = pMsgBuf->DataLength();
    apBytesData->mpMsgBuf = pMsgBuf;
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

WEAVE_ERROR GenericTraitUpdatableDataSink::IsNull(const char * apPath, bool & aIsNull)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    nl::Weave::TLV::TLVReader reader;
    PacketBuffer *pMsgBuf = NULL;
    PropertyPathHandle propertyPathHandle = kNullPropertyPathHandle;;
    std::map<PropertyPathHandle, PacketBuffer *>::iterator it;

    err = GetSchemaEngine()->MapPathToHandle(apPath, propertyPathHandle);
    SuccessOrExit(err);

    it = mPathTlvDataMap.find(propertyPathHandle);
    VerifyOrExit(it != mPathTlvDataMap.end(), err = WEAVE_ERROR_INCORRECT_STATE);

    pMsgBuf = mPathTlvDataMap[propertyPathHandle];

#if WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK
    err = DebugPrettyPrint(pMsgBuf);
    SuccessOrExit(err);
#endif //WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK

    reader.Init(pMsgBuf);
    err = reader.Next();
    SuccessOrExit(err);

    if (reader.GetType() == kTLVElementType_Null)
    {
        aIsNull = true;
    }
    else {
        aIsNull = false;
    }

exit:
    WeaveLogFunctError(err);
    return err;
}

template <class T>
WEAVE_ERROR GenericTraitUpdatableDataSink::Get(const char * apPath, T &aValue)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    nl::Weave::TLV::TLVReader reader;
    PacketBuffer *pMsgBuf = NULL;
    PropertyPathHandle propertyPathHandle = kNullPropertyPathHandle;
    std::map<PropertyPathHandle, PacketBuffer *>::iterator it;

    err = GetSchemaEngine()->MapPathToHandle(apPath, propertyPathHandle);
    SuccessOrExit(err);

    it = mPathTlvDataMap.find(propertyPathHandle);
    VerifyOrExit(it != mPathTlvDataMap.end(), err = WEAVE_ERROR_INCORRECT_STATE);

    pMsgBuf = mPathTlvDataMap[propertyPathHandle];

#if WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK
    err = DebugPrettyPrint(pMsgBuf);
    SuccessOrExit(err);
#endif //WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK

    reader.Init(pMsgBuf);
    err = reader.Next();
    SuccessOrExit(err);

    err = reader.Get(aValue);
    SuccessOrExit(err);

exit:
    WeaveLogFunctError(err);
    return err;
}

WEAVE_ERROR
GenericTraitUpdatableDataSink::SetLeafData(PropertyPathHandle aLeafHandle, TLVReader &aReader)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    nl::Weave::TLV::TLVWriter writer;
    PacketBuffer *pMsgBuf = PacketBuffer::New();
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
#endif //WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK
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
GenericTraitUpdatableDataSink::GetLeafData(PropertyPathHandle aLeafHandle, uint64_t aTagToWrite, TLVWriter &aWriter)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    nl::Weave::TLV::TLVReader reader;
    PacketBuffer *pMsgBuf = NULL;

    std::map<PropertyPathHandle, PacketBuffer *>::iterator it = mPathTlvDataMap.find(aLeafHandle);

    VerifyOrExit(it != mPathTlvDataMap.end(), err = WEAVE_ERROR_INCORRECT_STATE);

    pMsgBuf = mPathTlvDataMap[aLeafHandle];

#if WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK
    err = DebugPrettyPrint(pMsgBuf);
    SuccessOrExit(err);
#endif //WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK

    reader.Init(pMsgBuf);
    err = reader.Next();
    SuccessOrExit(err);
    err = aWriter.CopyElement(aTagToWrite, reader);
    SuccessOrExit(err);

exit:
    WeaveLogFunctError(err);
    return err;
}

WEAVE_ERROR GenericTraitUpdatableDataSink::GetNextDictionaryItemKey(PropertyPathHandle aDictionaryHandle, uintptr_t &aContext, PropertyDictionaryKey &aKey)
{
    return WEAVE_END_OF_INPUT;
}

#if WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK
void GenericTraitUpdatableDataSink::TLVPrettyPrinter(const char *aFormat, ...)
{
    va_list args;

    va_start(args, aFormat);

    vprintf(aFormat, args);

    va_end(args);
}

WEAVE_ERROR GenericTraitUpdatableDataSink::DebugPrettyPrint(PacketBuffer *apMsgBuf)
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
#endif //WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK

const nl::Weave::ExchangeContext::Timeout kResponseTimeoutMsec = 15000;

WdmClient::WdmClient() :
        mpAppState(NULL),
        mpPublisherPathList(NULL),
        mpSubscriptionClient(NULL),
        mpMsgLayer(NULL),
        mpContext(NULL),
        mpAppReqState(NULL)
{
    State = kState_NotInitialized;
}

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

    mpAppState = NULL;
    mpContext = NULL;
    mpMsgLayer = NULL;
    mpAppReqState = NULL;
    mOnError = NULL;

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

void WdmClient::ClearDataSinkVersion(void * aTraitInstance, TraitDataHandle aHandle, void * aContext)
{
    GenericTraitUpdatableDataSink * pGenericTraitUpdatableDataSink = NULL;
    if (aTraitInstance != NULL)
    {
        pGenericTraitUpdatableDataSink = static_cast<GenericTraitUpdatableDataSink *>(aTraitInstance);
        pGenericTraitUpdatableDataSink->ClearVersion();
    }
}

void WdmClient::ClientEventCallback (void * const aAppState, SubscriptionClient::EventID aEvent,
                                     const SubscriptionClient::InEventParam & aInParam, SubscriptionClient::OutEventParam & aOutParam)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WdmClient * const pWdmClient = reinterpret_cast<WdmClient *>(aAppState);

    OpState savedOpState = pWdmClient->mOpState;
    WeaveLogDetail(DataManagement, "WDM ClientEventCallback: current op is, %d", savedOpState);

    switch (aEvent)
    {
        case SubscriptionClient::kEvent_OnExchangeStart:
            WeaveLogDetail(DataManagement, "Client->kEvent_OnExchangeStart");
            break;
        case SubscriptionClient::kEvent_OnSubscribeRequestPrepareNeeded:
            WeaveLogDetail(DataManagement, "Client->kEvent_OnSubscribeRequestPrepareNeeded");
            VerifyOrExit(kOpState_RefreshData == savedOpState, err = WEAVE_ERROR_INCORRECT_STATE);
            {
                uint16_t pathListLen = 0;
                uint16_t traitListLen = 0;
                TraitDataHandle handle;
                bool NeedSubscribeAll = true;

                if (pWdmClient->mGetDataHandle != NULL && pWdmClient->mpContext != NULL)
                {
                    traitListLen = 1;
                    err = pWdmClient->mGetDataHandle(pWdmClient->mpContext, &pWdmClient->mSinkCatalog, handle);
                    SuccessOrExit(err);
                    NeedSubscribeAll = false;
                }
                else
                {
                    NeedSubscribeAll = true;
                    traitListLen = pWdmClient->mSinkCatalog.Count();
                }

                if (traitListLen == 0)
                {
                    err = WEAVE_ERROR_INCORRECT_STATE;
                    WeaveLogDetail(DataManagement, "subscribe none trait data sink");
                    SuccessOrExit(err);
                }

                if (pWdmClient->mpPublisherPathList == NULL)
                {
                    delete[] pWdmClient->mpPublisherPathList;
                }

                pWdmClient->mpPublisherPathList = new TraitPath[traitListLen];

                pWdmClient->mSinkCatalog.PrepareSubscriptionPathList(pWdmClient->mpPublisherPathList,
                                                                       traitListLen,
                                                                       pathListLen, handle, NeedSubscribeAll);

                aOutParam.mSubscribeRequestPrepareNeeded.mPathList = pWdmClient->mpPublisherPathList;
                aOutParam.mSubscribeRequestPrepareNeeded.mPathListSize = pathListLen;
                aOutParam.mSubscribeRequestPrepareNeeded.mNeedAllEvents = false;
                aOutParam.mSubscribeRequestPrepareNeeded.mLastObservedEventList = NULL;
                aOutParam.mSubscribeRequestPrepareNeeded.mLastObservedEventListSize = 0;
                aOutParam.mSubscribeRequestPrepareNeeded.mTimeoutSecMin = 30;
                aOutParam.mSubscribeRequestPrepareNeeded.mTimeoutSecMax = 120;
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
            VerifyOrExit(kOpState_RefreshData == pWdmClient->mOpState, err = WEAVE_ERROR_INCORRECT_STATE);
            break;
        case SubscriptionClient::kEvent_OnSubscriptionTerminated:
            WeaveLogDetail(DataManagement, "Client->kEvent_OnSubscriptionTerminated. Reason: %u, peer = 0x%" PRIX64 "\n",
            aInParam.mSubscriptionTerminated.mReason,
            aInParam.mSubscriptionTerminated.mClient->GetPeerNodeId());
            pWdmClient->mpSubscriptionClient->AbortSubscription();
            err = WEAVE_ERROR_INCORRECT_STATE;
            break;
        case SubscriptionClient::kEvent_OnUpdateComplete:
            if ((aInParam.mUpdateComplete.mReason == WEAVE_NO_ERROR) && (nl::Weave::Profiles::kWeaveProfile_Common == aInParam.mUpdateComplete.mStatusProfileId) && (nl::Weave::Profiles::Common::kStatus_Success == aInParam.mUpdateComplete.mStatusCode))
            {
                WeaveLogDetail(DataManagement, "Update: path result: success");
            }
            else
            {
                WeaveLogDetail(DataManagement, "Update: path failed: %s, %s, tdh %" PRIu16", will %sretry, discard failed change",
                        ErrorStr(aInParam.mUpdateComplete.mReason),
                        nl::StatusReportStr(aInParam.mUpdateComplete.mStatusProfileId, aInParam.mUpdateComplete.mStatusCode),
                        aInParam.mUpdateComplete.mTraitDataHandle,
                        aInParam.mUpdateComplete.mWillRetry ? "" : "not ");
                pWdmClient->mpSubscriptionClient->DiscardUpdates();
            }

            break;
        case SubscriptionClient::kEvent_OnNoMorePendingUpdates:
            WeaveLogDetail(DataManagement, "Update: no more pending updates");
            VerifyOrExit(kOpState_FlushUpdate == savedOpState, err = WEAVE_ERROR_INCORRECT_STATE);
            pWdmClient->mOnComplete.General(pWdmClient->mpContext, pWdmClient->mpAppReqState);
            pWdmClient->mpContext = NULL;
            pWdmClient->ClearOpState();
            break;

        default:
            SubscriptionClient::DefaultEventHandler(aEvent, aInParam, aOutParam);
            break;
    }

exit:
    if (WEAVE_NO_ERROR != err)
    {
        WeaveLogError(DataManagement, "WDM ClientEventCallback failure: err = %d", err);
        pWdmClient->mOnError(pWdmClient->mpContext, pWdmClient->mpAppReqState, err, NULL);
        pWdmClient->mpContext = NULL;
        pWdmClient->ClearOpState();
    }

    return;
}

WEAVE_ERROR WdmClient::Init( WeaveMessageLayer * apMsgLayer, Binding * apBinding)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    mpMsgLayer = apMsgLayer;

    VerifyOrExit(State == kState_NotInitialized, err = WEAVE_ERROR_INCORRECT_STATE);

    if (mpSubscriptionClient == NULL)
    {
        err =  SubscriptionEngine::GetInstance()->NewClient(&mpSubscriptionClient,
                                                            apBinding,
                                                            this,
                                                            ClientEventCallback,
                                                            &mSinkCatalog,
                                                            kResponseTimeoutMsec * 2); // max num of msec between subscribe request and subscribe response
        SuccessOrExit(err);
    }

    mpSubscriptionClient->EnableResubscribe(NULL);

    State = kState_Initialized;
    mpContext = NULL;
    ClearOpState();

exit:
    return WEAVE_NO_ERROR;
}

WEAVE_ERROR WdmClient::NewDataSink(const ResourceIdentifier & aResourceId, uint32_t aProfileId, uint64_t aInstanceId, const char * apPath, GenericTraitUpdatableDataSink *& apGenericTraitUpdatableDataSink)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    PropertyPathHandle handle = kNullPropertyPathHandle;

    const TraitSchemaEngine * pEngine = TraitSchemaDirectory::GetTraitSchemaEngine(aProfileId);
    VerifyOrExit(pEngine != NULL, err = WEAVE_ERROR_INCORRECT_STATE);

    VerifyOrExit(mpSubscriptionClient != NULL, err = WEAVE_ERROR_INCORRECT_STATE);

    VerifyOrExit(WEAVE_NO_ERROR != GetDataSink(aResourceId, aProfileId, aInstanceId, apGenericTraitUpdatableDataSink), WeaveLogDetail(DataManagement, "Trait exist"));

    apGenericTraitUpdatableDataSink = new GenericTraitUpdatableDataSink(pEngine, this);
    VerifyOrExit(apGenericTraitUpdatableDataSink != NULL, err = WEAVE_ERROR_NO_MEMORY);

    if (apPath == NULL)
    {
        handle = kRootPropertyPathHandle;
    }
    else
    {
        apGenericTraitUpdatableDataSink->GetSchemaEngine()->MapPathToHandle(apPath, handle);
    }

    err = SubscribePublisherTrait(aResourceId, aInstanceId, handle, apGenericTraitUpdatableDataSink);
    SuccessOrExit(err);

    apGenericTraitUpdatableDataSink->SetSubscriptionClient(mpSubscriptionClient);

exit:
    return err;
}

WEAVE_ERROR WdmClient::GetDataSink(const ResourceIdentifier & aResourceId, uint32_t aProfileId, uint64_t aInstanceId, GenericTraitUpdatableDataSink *& apGenericTraitUpdatableDataSink)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TraitDataSink * dataSink = NULL;
    err = mSinkCatalog.Locate(aProfileId, aInstanceId, aResourceId, &dataSink);
    SuccessOrExit(err);
    apGenericTraitUpdatableDataSink = static_cast<GenericTraitUpdatableDataSink *>(dataSink);

    exit:
    return err;
}

WEAVE_ERROR WdmClient::FlushUpdate(void* apAppReqState, DMCompleteFunct onComplete, DMErrorFunct onError)
{
    VerifyOrExit(mOpState == kOpState_Idle, WeaveLogError(DataManagement, "FlushUpdate with OpState %d", mOpState));

    mpAppReqState = apAppReqState;
    mOnComplete.General = onComplete;
    mOnError = onError;
    mOpState = kOpState_FlushUpdate;
    mpContext = this;
    mpSubscriptionClient->FlushUpdate(true);

exit:
    return WEAVE_NO_ERROR;
}

WEAVE_ERROR WdmClient::RefreshData(void* apAppReqState, DMCompleteFunct onComplete, DMErrorFunct onError, GetDataHandleFunct getDataHandleCb)
{
    VerifyOrExit(mpSubscriptionClient != NULL, WeaveLogError(DataManagement, "mpSubscriptionClient is NULL"));

    mSinkCatalog.Iterate(ClearDataSinkVersion, this);

    RefreshData(apAppReqState, this, onComplete, onError, getDataHandleCb);

exit:
    return WEAVE_NO_ERROR;
}

WEAVE_ERROR WdmClient::RefreshData(void* apAppReqState, void* apContext, DMCompleteFunct onComplete, DMErrorFunct onError, GetDataHandleFunct getDataHandleCb)
{
    VerifyOrExit(mOpState == kOpState_Idle, WeaveLogError(DataManagement, "RefreshData with OpState %d", mOpState));

    mpAppReqState = apAppReqState;
    mOnComplete.General = onComplete;
    mOnError = onError;
    mOpState = kOpState_RefreshData;
    mGetDataHandle = getDataHandleCb;
    mpContext = apContext;

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

} // namespace DeviceManager
} // namespace Weave
} // namespace nl
