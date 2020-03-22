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
 *      Generic trait data sinks which use map to store trait data inside, and provide a list of set/get public API for
 *      application use
 */

#ifndef WEAVE_DATA_MANAGEMENT_CLIENT_H_
#define WEAVE_DATA_MANAGEMENT_CLIENT_H_

#include <Weave/Core/WeaveCore.h>
#if WEAVE_CONFIG_DATA_MANAGEMENT_CLIENT_EXPERIMENTAL
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Support/NLDLLUtil.h>
#include <Weave/Profiles/common/WeaveMessage.h>
#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Profiles/data-management/Current/WdmManagedNamespace.h>
#include <Weave/Profiles/status-report/StatusReportProfile.h>
#include <SystemLayer/SystemPacketBuffer.h>
#include <Weave/Profiles/data-management/SubscriptionClient.h>
#include <Weave/Profiles/data-management/Current/GenericTraitCatalogImpl.h>
#include <map>
#include <vector>
#include "WeaveDeviceManager.h"

namespace nl {
namespace Weave {
namespace DeviceManager {
using namespace nl::Weave::Encoding;
using namespace nl::Weave::Profiles;
using namespace nl::Weave::Profiles::DataManagement;
using namespace nl::Weave::Profiles::Security;
using namespace ::nl::Weave::Profiles::DataManagement_Current;

class NL_DLL_EXPORT BytesData
{
public:
    BytesData() : mpDataBuf(NULL), mDataLen(0), mpMsgBuf(NULL) { }
    void Clear()
    {
        if (mpMsgBuf != NULL)
        {
            PacketBuffer::Free(mpMsgBuf);
        }
        mpDataBuf = NULL;
        mDataLen  = 0;
    }

    const uint8_t * mpDataBuf;
    uint32_t mDataLen;
    PacketBuffer * mpMsgBuf;
};

class GenericTraitUpdatableDataSink;
class WdmClient;

class WdmClientFlushUpdateStatus
{
public:
    uint32_t mErrorCode;
    DeviceStatus mDevStatus;
    const char * mpPath;
    uint32_t mPathLen;
    TraitDataSink * mpDataSink;
};

extern "C" {
typedef void (*DMCompleteFunct)(void * appState, void * appReqState);
typedef void (*DMErrorFunct)(void * appState, void * appReqState, WEAVE_ERROR err, DeviceStatus * devStatus);
typedef void (*DMFlushUpdateCompleteFunct)(void * appState, void * appReqState, uint16_t pathCount,
                                           WdmClientFlushUpdateStatus * statusResults);
typedef WEAVE_ERROR (*GetDataHandleFunct)(void * apContext, const TraitCatalogBase<TraitDataSink> * const apCatalog,
                                          TraitDataHandle & aHandle);
};

class NL_DLL_EXPORT GenericTraitUpdatableDataSink : public nl::Weave::Profiles::DataManagement::TraitUpdatableDataSink
{
    friend class WdmClient;
    using nl::Weave::Profiles::DataManagement_Current::TraitDataSink::SetData;
    using nl::Weave::Profiles::DataManagement_Current::TraitUpdatableDataSink::GetData;

private:
    GenericTraitUpdatableDataSink(const nl::Weave::Profiles::DataManagement::TraitSchemaEngine * aEngine, WdmClient * apWdmClient);
    ~GenericTraitUpdatableDataSink(void);

public:
    void Clear(void);

    WEAVE_ERROR RefreshData(void * appReqState, DMCompleteFunct onComplete, DMErrorFunct onError);

    WEAVE_ERROR SetData(const char * apPath, int64_t aValue, bool aIsConditional = false);
    WEAVE_ERROR SetData(const char * apPath, uint64_t aValue, bool aIsConditional = false);
    WEAVE_ERROR SetData(const char * apPath, double aValue, bool aIsConditional = false);
    WEAVE_ERROR SetBoolean(const char * apPath, bool aValue, bool aIsConditional = false);
    WEAVE_ERROR SetString(const char * apPath, const char * aValue, bool aIsConditional = false);
    WEAVE_ERROR SetBytes(const char * apPath, const uint8_t * dataBuf, size_t dataLen, bool aIsConditional = false);
    WEAVE_ERROR SetTLVBytes(const char * apPath, const uint8_t * dataBuf, size_t dataLen, bool aIsConditional = false);
    WEAVE_ERROR SetNull(const char * apPath, bool aIsConditional = false);

    WEAVE_ERROR SetStringArray(const char * apPath, const std::vector<std::string> & aValueVector, bool aIsConditional = false);

    WEAVE_ERROR GetData(const char * apPath, int64_t & aValue);
    WEAVE_ERROR GetData(const char * apPath, uint64_t & aValue);
    WEAVE_ERROR GetData(const char * apPath, double & aValue);

    WEAVE_ERROR GetBoolean(const char * apPath, bool & aValue);
    WEAVE_ERROR GetString(const char * apPath, BytesData * apBytesData);
    WEAVE_ERROR GetBytes(const char * apPath, BytesData * apBytesData);
    WEAVE_ERROR GetTLVBytes(const char * apPath, BytesData * apBytesData);
    WEAVE_ERROR IsNull(const char * apPath, bool & aIsNull);
    WEAVE_ERROR GetStringArray(const char * apPath, std::vector<std::string> & aValueVector);

    WEAVE_ERROR DeleteData(const char * apPath);

    void * mpAppState;

private:
    union
    {
        DMCompleteFunct General;
    } mOnComplete;
    DMErrorFunct mOnError;

    template <class T>
    WEAVE_ERROR Set(const char * apPath, T aValue, bool aIsConditional = false);
    template <class T>
    WEAVE_ERROR Get(const char * apPath, T & aValue);

    WEAVE_ERROR SetLeafData(nl::Weave::Profiles::DataManagement::PropertyPathHandle aLeafHandle,
                            nl::Weave::TLV::TLVReader & aReader) __OVERRIDE;
    WEAVE_ERROR GetLeafData(nl::Weave::Profiles::DataManagement::PropertyPathHandle aLeafHandle, uint64_t aTagToWrite,
                            nl::Weave::TLV::TLVWriter & aWriter) __OVERRIDE;
    WEAVE_ERROR GetNextDictionaryItemKey(nl::Weave::Profiles::DataManagement::PropertyPathHandle aDictionaryHandle,
                                         uintptr_t & aContext,
                                         nl::Weave::Profiles::DataManagement::PropertyDictionaryKey & aKey) __OVERRIDE;

    void UpdateTLVDataMap(PropertyPathHandle aPropertyPathHandle, PacketBuffer * apMsgBuf);
    static WEAVE_ERROR LocateTraitHandle(void * apContext, const TraitCatalogBase<TraitDataSink> * const apCatalog,
                                         TraitDataHandle & aHandle);

#if WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK
    static void TLVPrettyPrinter(const char * aFormat, ...);
    static WEAVE_ERROR DebugPrettyPrint(PacketBuffer * apMsgBuf);
#endif // WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK

    std::map<PropertyPathHandle, PacketBuffer *> mPathTlvDataMap;
    WdmClient * mpWdmClient;
};

class NL_DLL_EXPORT WdmClient
{
    friend class GenericTraitUpdatableDataSink;

public:
    enum
    {
        kState_NotInitialized = 0,
        kState_Initialized    = 1
    } State; // [READ-ONLY] Current state

    WdmClient(void);

    WEAVE_ERROR Init(WeaveMessageLayer * apMsgLayer, Binding * apBinding);

    void Close(void);

    void SetNodeId(uint64_t aNodeId);

    WEAVE_ERROR NewDataSink(const ResourceIdentifier & aResourceId, uint32_t aProfileId, uint64_t aInstanceId, const char * apPath,
                            GenericTraitUpdatableDataSink *& apGenericTraitUpdatableDataSink);

    WEAVE_ERROR FlushUpdate(void * apAppReqState, DMFlushUpdateCompleteFunct onComplete, DMErrorFunct onError);

    WEAVE_ERROR RefreshData(void * apAppReqState, DMCompleteFunct onComplete, DMErrorFunct onError,
                            GetDataHandleFunct getDataHandleCb);

    void * mpAppState;

private:
    enum OpState
    {
        kOpState_Idle        = 0,
        kOpState_FlushUpdate = 1,
        kOpState_RefreshData = 2,
    };

    void ClearOpState();

    union
    {
        DMCompleteFunct General;
        DMFlushUpdateCompleteFunct FlushUpdate;
    } mOnComplete;
    DMErrorFunct mOnError;
    GetDataHandleFunct mGetDataHandle;

    static void ClearDataSink(void * aTraitInstance, TraitDataHandle aHandle, void * aContext);
    static void ClearDataSinkVersion(void * aTraitInstance, TraitDataHandle aHandle, void * aContext);
    static void ClientEventCallback(void * const aAppState, SubscriptionClient::EventID aEvent,
                                    const SubscriptionClient::InEventParam & aInParam,
                                    SubscriptionClient::OutEventParam & aOutParam);

    WEAVE_ERROR RefreshData(void * apAppReqState, void * apContext, DMCompleteFunct onComplete, DMErrorFunct onError,
                            GetDataHandleFunct getDataHandleCb);
    WEAVE_ERROR GetDataSink(const ResourceIdentifier & aResourceId, uint32_t aProfileId, uint64_t aInstanceId,
                            GenericTraitUpdatableDataSink *& apGenericTraitUpdatableDataSink);
    WEAVE_ERROR SubscribePublisherTrait(const ResourceIdentifier & aResourceId, const uint64_t & aInstanceId,
                                        PropertyPathHandle aBasePathHandle, TraitDataSink * apDataSink);

    WEAVE_ERROR UnsubscribePublisherTrait(TraitDataSink * apDataSink);

    WEAVE_ERROR UpdateFailedPathResults(WdmClient * const apWdmClient, TraitDataHandle mTraitDataHandle,
                                        PropertyPathHandle mPropertyPathHandle, uint32_t aReason, uint32_t aStatusProfileId,
                                        uint16_t aStatusCode);

    GenericTraitSinkCatalog mSinkCatalog;
    TraitPath * mpPublisherPathList;

    SubscriptionClient * mpSubscriptionClient;
    WeaveMessageLayer * mpMsgLayer;
    void * mpContext;
    void * mpAppReqState;
    OpState mOpState;
    std::vector<std::string> mFailedPaths;
    std::vector<WdmClientFlushUpdateStatus> mFailedFlushPathStatus;
};
} // namespace DeviceManager
} // namespace Weave
} // namespace nl
#endif // WEAVE_CONFIG_DATA_MANAGEMENT_CLIENT_EXPERIMENTAL
#endif // WEAVE_DATA_MANAGEMENT_CLIENT_H_
