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
#include <Weave/Profiles/echo/WeaveEcho.h>
#include <Weave/Profiles/network-provisioning/NetworkProvisioning.h>
#include <Weave/Profiles/service-provisioning/ServiceProvisioning.h>
#include <Weave/Profiles/fabric-provisioning/FabricProvisioning.h>
#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Profiles/device-description/DeviceDescription.h>
#include <Weave/Profiles/device-control/DeviceControl.h>
#include <Weave/Profiles/vendor/nestlabs/device-description/NestProductIdentifiers.hpp>
#include <Weave/Profiles/security/WeaveSecurity.h>
#include <Weave/Profiles/security/WeaveAccessToken.h>
#include <Weave/Profiles/token-pairing/TokenPairing.h>
#include <Weave/DeviceManager/WeaveDeviceManager.h>
#include <Weave/Support/verhoeff/Verhoeff.h>
#include <Weave/Support/crypto/WeaveCrypto.h>
#include <Weave/Support/logging/WeaveLogging.h>
#include <Weave/Support/ErrorStr.h>
#include <Weave/Support/NestCerts.h>
#include <Weave/Support/TimeUtils.h>
#include <Weave/Profiles/vendor/nestlabs/dropcam-legacy-pairing/DropcamLegacyPairing.h>

#include <Weave/Support/CodeUtils.h>
#include <Weave/DeviceManager/WeaveDataManagementClient.h>
#include <Weave/DeviceManager/TraitSchemaDirectory.h>

namespace nl {
namespace Weave {
namespace DeviceManager {

using namespace nl::Weave::Encoding;
using namespace nl::Weave::Profiles;
using namespace nl::Weave::Profiles::DataManagement;
using namespace nl::Weave::Profiles::DeviceDescription;
using namespace nl::Weave::Profiles::Security;
using namespace nl::Weave::Profiles::ServiceProvisioning;
using namespace ::nl::Weave;
using namespace ::nl::Weave::TLV;
using namespace Schema::Weave::Common;

class GenericTraitDataSink;
class WDMClient;

GenericTraitDataSink::GenericTraitDataSink(const nl::Weave::Profiles::DataManagement::TraitSchemaEngine * aEngine)
        : TraitUpdatableDataSink(aEngine)
{
}

GenericTraitDataSink::~GenericTraitDataSink()
{
    Close();
}

void GenericTraitDataSink::Close(void)
{
    for (auto itr = mPathTlvDataMap.begin(); itr != mPathTlvDataMap.end(); ++itr)
    {
        if (NULL != itr->second)
        {
            PacketBuffer::Free(itr->second);
            mPathTlvDataMap.erase(itr->first);
        }
    }
}

void GenericTraitDataSink::UpdateTLVDataMap(PropertyPathHandle aPropertyPathHandle, PacketBuffer *apMsgBuf)
{
    if ((mPathTlvDataMap.find(aPropertyPathHandle) != mPathTlvDataMap.end()) && (NULL != mPathTlvDataMap[aPropertyPathHandle]))
    {
        PacketBuffer::Free(mPathTlvDataMap[aPropertyPathHandle]);
    }
    mPathTlvDataMap[aPropertyPathHandle] = apMsgBuf;
}

WEAVE_ERROR GenericTraitDataSink::LocateTraitHandle(void* appReqState, const TraitCatalogBase<TraitDataSink> * const apCatalog, TraitDataHandle & aHandle)
{
    GenericTraitDataSink * pGenericTraitDataSink = static_cast<GenericTraitDataSink *>(appReqState);
    return apCatalog->Locate(pGenericTraitDataSink, aHandle);
}

WEAVE_ERROR GenericTraitDataSink::RefreshData(WDMClient* apWDMClient, WDMClientCompleteFunct onComplete, WDMClientErrorFunct onError)
{
    return apWDMClient->RefreshData(this, onComplete, onError, LocateTraitHandle);
}

WEAVE_ERROR GenericTraitDataSink::SetData(const char * apPath, int8_t aValue, bool aIsConditional)
{
    return Set(apPath, aValue, aIsConditional);
}

WEAVE_ERROR GenericTraitDataSink::SetData(const char * apPath, int16_t aValue, bool aIsConditional)
{
    return Set(apPath, aValue, aIsConditional);
}

WEAVE_ERROR GenericTraitDataSink::SetData(const char * apPath, int32_t aValue, bool aIsConditional)
{
    return Set(apPath, aValue, aIsConditional);
}

WEAVE_ERROR GenericTraitDataSink::SetData(const char * apPath, int64_t aValue, bool aIsConditional)
{
    return Set(apPath, aValue, aIsConditional);
}

WEAVE_ERROR GenericTraitDataSink::SetData(const char * apPath, uint8_t aValue, bool aIsConditional)
{
    return Set(apPath, aValue, aIsConditional);
}

WEAVE_ERROR GenericTraitDataSink::SetData(const char * apPath, uint16_t aValue, bool aIsConditional)
{
    return Set(apPath, aValue, aIsConditional);
}

WEAVE_ERROR GenericTraitDataSink::SetData(const char * apPath, uint32_t aValue, bool aIsConditional)
{
    return Set(apPath, aValue, aIsConditional);
}

WEAVE_ERROR GenericTraitDataSink::SetData(const char * apPath, uint64_t aValue, bool aIsConditional)
{
    return Set(apPath, aValue, aIsConditional);
}

WEAVE_ERROR GenericTraitDataSink::SetData(const char * apPath, double aValue, bool aIsConditional)
{
    return Set(apPath, aValue, aIsConditional);
}

WEAVE_ERROR GenericTraitDataSink::SetBoolean(const char * apPath, bool aValue, bool aIsConditional)
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

WEAVE_ERROR GenericTraitDataSink::SetString(const char * apPath, const char * aValue, bool aIsConditional)
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

WEAVE_ERROR GenericTraitDataSink::SetNull(const char * apPath, bool aIsConditional)
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

WEAVE_ERROR GenericTraitDataSink::SetLeafBytes(const char * apPath, const uint8_t * dataBuf, size_t dataLen, bool aIsConditional)
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

WEAVE_ERROR GenericTraitDataSink::SetBytes(const char * apPath, const uint8_t * dataBuf, size_t dataLen, bool aIsConditional)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TLVReader reader;
    PropertyPathHandle propertyPathHandle = kNullPropertyPathHandle;

    VerifyOrExit(GetSubscriptionClient() != NULL, err = WEAVE_ERROR_INCORRECT_STATE);
    Lock(GetSubscriptionClient());

    err = GetSchemaEngine()->MapPathToHandle(apPath, propertyPathHandle);
    SuccessOrExit(err);

    reader.Init(dataBuf, dataLen);
    err = reader.Next();
    SuccessOrExit(err);

    err = StoreDataElement(propertyPathHandle, reader, 0, NULL, NULL);
    SuccessOrExit(err);

    err = SetUpdated(GetSubscriptionClient(), propertyPathHandle, aIsConditional);
    Unlock(GetSubscriptionClient());
    WeaveLogDetail(DataManagement, "<set updated> in 0x%08x", propertyPathHandle);

exit:
    WeaveLogFunctError(err);

    return err;
}

template <class T>
WEAVE_ERROR GenericTraitDataSink::Set(const char * apPath, T aValue, bool aIsConditional)
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

WEAVE_ERROR GenericTraitDataSink::GetData(const char * apPath, int8_t& aValue)
{
    return Get(apPath, aValue);
}

WEAVE_ERROR GenericTraitDataSink::GetData(const char * apPath, int16_t& aValue)
{
    return Get(apPath, aValue);
}

WEAVE_ERROR GenericTraitDataSink::GetData(const char * apPath, int32_t& aValue)
{
    return Get(apPath, aValue);
}

WEAVE_ERROR GenericTraitDataSink::GetData(const char * apPath, int64_t& aValue)
{
    return Get(apPath, aValue);
}

WEAVE_ERROR GenericTraitDataSink::GetData(const char * apPath, uint8_t& aValue)
{
    return Get(apPath, aValue);
}

WEAVE_ERROR GenericTraitDataSink::GetData(const char * apPath, uint16_t& aValue)
{
    return Get(apPath, aValue);
}

WEAVE_ERROR GenericTraitDataSink::GetData(const char * apPath, uint32_t& aValue)
{
    return Get(apPath, aValue);
}

WEAVE_ERROR GenericTraitDataSink::GetData(const char * apPath, uint64_t& aValue)
{
    return Get(apPath, aValue);
}

WEAVE_ERROR GenericTraitDataSink::GetData(const char * apPath, double& aValue)
{
    return Get(apPath, aValue);
}

WEAVE_ERROR GenericTraitDataSink::GetBoolean(const char * apPath, bool& aValue)
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

WEAVE_ERROR GenericTraitDataSink::GetString(const char * apPath, char * aValue)
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

    err = reader.GetString(aValue, sizeof(aValue));
    SuccessOrExit(err);

exit:
    WeaveLogFunctError(err);
    return err;
}

WEAVE_ERROR GenericTraitDataSink::GetLeafBytes(const char * apPath, ConstructBytesArrayFunct aCallback)
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

    aCallback(pMsgBuf->Start(), pMsgBuf->DataLength());

exit:
    WeaveLogFunctError(err);
    return err;
}

WEAVE_ERROR GenericTraitDataSink::GetBytes(const char * apPath, ConstructBytesArrayFunct aCallback)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TraitSchemaEngine::IGetDataDelegate *getDataDelegate;
    TLVType dummyContainerType;
    nl::Weave::TLV::TLVWriter writer;
    PropertyPathHandle propertyPathHandle = kNullPropertyPathHandle;

    PacketBuffer *pMsgBuf = PacketBuffer::New();
    VerifyOrExit(NULL != pMsgBuf, err = WEAVE_ERROR_NO_MEMORY);

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

    aCallback(pMsgBuf->Start(), pMsgBuf->DataLength());

exit:
    WeaveLogFunctError(err);

    if (NULL != pMsgBuf)
    {
        PacketBuffer::Free(pMsgBuf);
        pMsgBuf = NULL;
    }

    return err;
}

WEAVE_ERROR GenericTraitDataSink::IsNull(const char * apPath, bool & aIsNull)
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
WEAVE_ERROR GenericTraitDataSink::Get(const char * apPath, T &aValue)
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
GenericTraitDataSink::SetLeafData(PropertyPathHandle aLeafHandle, TLVReader &aReader)
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
GenericTraitDataSink::GetLeafData(PropertyPathHandle aLeafHandle, uint64_t aTagToWrite, TLVWriter &aWriter)
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

WEAVE_ERROR GenericTraitDataSink::GetNextDictionaryItemKey(PropertyPathHandle aDictionaryHandle, uintptr_t &aContext, PropertyDictionaryKey &aKey)
{
    return WEAVE_END_OF_INPUT;
}

#if WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK
void GenericTraitDataSink::TLVPrettyPrinter(const char *aFormat, ...)
{
    va_list args;

    va_start(args, aFormat);

    vprintf(aFormat, args);

    va_end(args);
}

WEAVE_ERROR GenericTraitDataSink::DebugPrettyPrint(PacketBuffer *apMsgBuf)
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

WDMClient::WDMClient() :
        mpPublisherPathList(NULL),
        mpSubscriptionClient(NULL),
        mpMsgLayer(NULL),
        mAppReqState(NULL)
{
    mOpState = kOpState_Idle;
}

WDMClient::~WDMClient(void)
{
    Close();
}

void WDMClient::Close(void)
{
    //if (NULL == mpBinding)
    //{
    //    mpBinding->Release();
    //    mpBinding = NULL;
    //}

    mSinkCatalog.Clear();
    if (mpPublisherPathList != NULL)
    {
        delete[] mpPublisherPathList;
    }
    if (mpSubscriptionClient != NULL)
    {
        mpSubscriptionClient->Free();
        mpSubscriptionClient = NULL;
    }
    mpMsgLayer = NULL;
    mAppReqState = NULL;
    mOpState = kOpState_Idle;
}

void WDMClient::ClientEventCallback (void * const aAppState, SubscriptionClient::EventID aEvent,
                                     const SubscriptionClient::InEventParam & aInParam, SubscriptionClient::OutEventParam & aOutParam)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WDMClient * const pWDMDMClient = reinterpret_cast<WDMClient *>(aAppState);

    OpState savedOpState = pWDMDMClient->mOpState;
    WeaveLogDetail(DataManagement, "WDM ClientEventCallback: current op is, %d", savedOpState);

    switch (aEvent)
    {
        case SubscriptionClient::kEvent_OnExchangeStart:
            WeaveLogDetail(DataManagement, "Client->kEvent_OnExchangeStart");
            break;
        case SubscriptionClient::kEvent_OnSubscribeRequestPrepareNeeded:
            WeaveLogDetail(DataManagement, "Client->kEvent_OnSubscribeRequestPrepareNeeded");
            {
                uint16_t pathListLen = 0;
                uint16_t traitListLen = 0;
                TraitDataHandle handle;
                bool NeedSubscribeAll = true;

                if (pWDMDMClient->mGetDataHandle != NULL && pWDMDMClient->mAppReqState != NULL)
                {
                    traitListLen = 1;
                    err = pWDMDMClient->mGetDataHandle(pWDMDMClient->mAppReqState, &pWDMDMClient->mSinkCatalog, handle);
                    SuccessOrExit(err);
                    NeedSubscribeAll = false;
                }
                else
                {
                    NeedSubscribeAll = true;
                    traitListLen = pWDMDMClient->mSinkCatalog.Count();
                }

                if (pWDMDMClient->mpPublisherPathList == NULL)
                {
                    pWDMDMClient->mpPublisherPathList = new TraitPath[traitListLen];
                }
                else
                {
                    delete[] pWDMDMClient->mpPublisherPathList;
                    pWDMDMClient->mpPublisherPathList = new TraitPath[traitListLen];
                }

                pWDMDMClient->mSinkCatalog.PrepareSubscriptionPathList(pWDMDMClient->mpPublisherPathList,
                                                                       traitListLen,
                                                                       pathListLen, handle, NeedSubscribeAll);

                aOutParam.mSubscribeRequestPrepareNeeded.mPathList = pWDMDMClient->mpPublisherPathList;
                aOutParam.mSubscribeRequestPrepareNeeded.mPathListSize = pathListLen;
                aOutParam.mSubscribeRequestPrepareNeeded.mNeedAllEvents = false;
                aOutParam.mSubscribeRequestPrepareNeeded.mLastObservedEventList = NULL;
                aOutParam.mSubscribeRequestPrepareNeeded.mLastObservedEventListSize = 0;
                aOutParam.mSubscribeRequestPrepareNeeded.mTimeoutSecMin = 30;
                aOutParam.mSubscribeRequestPrepareNeeded.mTimeoutSecMax = 3600;
            }
            break;

        case SubscriptionClient::kEvent_OnSubscriptionEstablished:
            WeaveLogDetail(DataManagement, "Client->kEvent_OnSubscriptionEstablished");
            pWDMDMClient->mpSubscriptionClient->AbortSubscription();
            VerifyOrExit(kOpState_RefreshData == pWDMDMClient->mOpState, err = WEAVE_ERROR_INCORRECT_STATE);
            pWDMDMClient->ClearOpState();
            pWDMDMClient->mOnComplete.General(pWDMDMClient, pWDMDMClient->mAppReqState);
            break;
        case SubscriptionClient::kEvent_OnNotificationRequest:
            WeaveLogDetail(DataManagement, "Client->kEvent_OnNotificationRequest");
            break;
        case SubscriptionClient::kEvent_OnNotificationProcessed:
            WeaveLogDetail(DataManagement, "Client->kEvent_OnNotificationProcessed");
            break;
        case SubscriptionClient::kEvent_OnSubscriptionTerminated:
            WeaveLogDetail(DataManagement, "Client->kEvent_OnSubscriptionTerminated. Reason: %u, peer = 0x%" PRIX64 "\n",
            aInParam.mSubscriptionTerminated.mReason,
            aInParam.mSubscriptionTerminated.mClient->GetPeerNodeId());
            pWDMDMClient->mpSubscriptionClient->AbortSubscription();
            err = WEAVE_ERROR_INCORRECT_STATE;
            break;
        case SubscriptionClient::kEvent_OnUpdateComplete:
            if ((aInParam.mUpdateComplete.mReason == WEAVE_NO_ERROR) && (nl::Weave::Profiles::kWeaveProfile_Common == aInParam.mUpdateComplete.mStatusProfileId) && (nl::Weave::Profiles::Common::kStatus_Success == aInParam.mUpdateComplete.mStatusCode))
            {
                WeaveLogDetail(DataManagement, "Update: path result: success");
            }
            else
            {
                TraitDataSink *sink = NULL;
                WeaveLogDetail(DataManagement, "Update: path failed: %s, %s, tdh %" PRIu16", will %sretry",
                        ErrorStr(aInParam.mUpdateComplete.mReason),
                        nl::StatusReportStr(aInParam.mUpdateComplete.mStatusProfileId, aInParam.mUpdateComplete.mStatusCode),
                        aInParam.mUpdateComplete.mTraitDataHandle,
                        aInParam.mUpdateComplete.mWillRetry ? "" : "not ");
                WeaveLogDetail(DataManagement, "discard failed change");
                pWDMDMClient->mSinkCatalog.Locate(aInParam.mUpdateComplete.mTraitDataHandle, &sink);
                pWDMDMClient->mpSubscriptionClient->DiscardUpdates();
            }

            VerifyOrExit(kOpState_FlushUpdate == pWDMDMClient->mOpState, err = WEAVE_ERROR_INCORRECT_STATE);
            pWDMDMClient->ClearOpState();
            pWDMDMClient->mOnComplete.General(pWDMDMClient, pWDMDMClient->mAppReqState);

            break;
        case SubscriptionClient::kEvent_OnNoMorePendingUpdates:
            WeaveLogDetail(DataManagement, "Update: no more pending updates");
            break;

        default:
            SubscriptionClient::DefaultEventHandler(aEvent, aInParam, aOutParam);
            break;
    }

exit:
    if (WEAVE_NO_ERROR != err)
    {
        pWDMDMClient->ClearOpState();
        WeaveLogError(DataManagement, "WDM ClientEventCallback failure: err = %d", err);
        pWDMDMClient->mOnError(pWDMDMClient, pWDMDMClient->mAppReqState, err, NULL);
    }

    return;
}

WEAVE_ERROR WDMClient::Init( WeaveMessageLayer * apMsgLayer, Binding * apBinding)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    mpMsgLayer = apMsgLayer;

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

exit:
    return WEAVE_NO_ERROR;
}

WEAVE_ERROR WDMClient::NewDataSink(const ResourceIdentifier & aResourceId, uint32_t aProfileId, uint64_t aInstanceId, const char * apPath, GenericTraitDataSink *& apGenericTraitDataSink)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    PropertyPathHandle handle = kNullPropertyPathHandle;

    VerifyOrExit(WEAVE_NO_ERROR != GetDataSink(aResourceId, aProfileId, aInstanceId, apGenericTraitDataSink), WeaveLogDetail(DataManagement, "Trait (profile id: %" PRIu32", instance id): % exist", aProfileId, aInstanceId));

    apGenericTraitDataSink = new GenericTraitDataSink(TraitSchemaDirectory::GetTraitSchemaEngine(aProfileId));
    VerifyOrExit(apGenericTraitDataSink != NULL, err = WEAVE_ERROR_NO_MEMORY);

    if (apPath == NULL)
    {
        handle = kRootPropertyPathHandle;
    }
    else
    {
        apGenericTraitDataSink->GetSchemaEngine()->MapPathToHandle(apPath, handle);
    }

    err = SubscribePublisherTrait(aResourceId, aInstanceId, handle, apGenericTraitDataSink);
    SuccessOrExit(err);

    apGenericTraitDataSink->SetSubscriptionClient(mpSubscriptionClient);
    //WeaveLogProgress(DataManagement, "%s", __PRETTY_FUNCTION__);

exit:
    return err;
}

WEAVE_ERROR WDMClient::GetDataSink(const ResourceIdentifier & aResourceId, uint32_t aProfileId, uint64_t aInstanceId, GenericTraitDataSink *& apGenericTraitDataSink)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TraitDataSink * dataSink = NULL;
    err = mSinkCatalog.Locate(aProfileId, aInstanceId, aResourceId, &dataSink);
    SuccessOrExit(err);
    apGenericTraitDataSink = static_cast<GenericTraitDataSink *>(dataSink);

exit:
    return err;
}

WEAVE_ERROR WDMClient::FlushUpdate(void* appReqState, WDMClientCompleteFunct onComplete, WDMClientErrorFunct onError)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(mOpState == kOpState_Idle, err = WEAVE_ERROR_INCORRECT_STATE);

    mAppReqState = appReqState;
    mOnComplete.General = onComplete;
    mOnError = onError;
    mOpState = kOpState_FlushUpdate;

    err = mpSubscriptionClient->FlushUpdate();
    SuccessOrExit(err);

exit:
    if (err != WEAVE_NO_ERROR)
    {
        ClearOpState();
    }
    return err;
}

WEAVE_ERROR WDMClient::RefreshData(void* appReqState, WDMClientCompleteFunct onComplete, WDMClientErrorFunct onError, GetDataHandleFunct getDataHandleCb)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    VerifyOrExit(mOpState == kOpState_Idle, err = WEAVE_ERROR_INCORRECT_STATE);

    mAppReqState = appReqState;
    mOnComplete.General = onComplete;
    mOnError = onError;
    mOpState = kOpState_RefreshData;
    mGetDataHandle = getDataHandleCb;

    mpSubscriptionClient->InitiateSubscription();

exit:
    if (err != WEAVE_NO_ERROR)
    {
        ClearOpState();
    }

    return err;
}

void WDMClient::ClearOpState()
{
    mOpState = kOpState_Idle;
}

WEAVE_ERROR WDMClient::SubscribePublisherTrait(const ResourceIdentifier & aResourceId, const uint64_t & aInstanceId,
                                               PropertyPathHandle aBasePathHandle, TraitDataSink * apDataSink)
{
    TraitDataHandle traitHandle;
    return mSinkCatalog.Add(aResourceId, aInstanceId, aBasePathHandle, apDataSink, traitHandle);
}

WEAVE_ERROR WDMClient::UnsubscribePublisherTrait(TraitDataSink * apDataSink)
{
    return mSinkCatalog.Remove(apDataSink);
}

} // namespace DeviceManager
} // namespace Weave
} // namespace nl

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current) {

SubscriptionEngine * SubscriptionEngine::GetInstance()
{
    static nl::Weave::Profiles::DataManagement::SubscriptionEngine gWdmSubscriptionEngine;
    return &gWdmSubscriptionEngine;
}

namespace Platform {
    void CriticalSectionEnter()
    {
        return;
    }

    void CriticalSectionExit()
    {
        return;
    }

} // Platform

} // WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current)
} // Profiles
} // Weave
} // nl
