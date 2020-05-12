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

#include <Weave/Profiles/data-management/Current/WdmManagedNamespace.h>
#include <Weave/Profiles/data-management/DataManagement.h>

#include <Weave/Profiles/data-management/WdmDictionary.h>

#include <nest/test/trait/TestHTrait.h>
#include <nest/test/trait/TestCTrait.h>
#include <nest/test/trait/TestMismatchedCTrait.h>

#include "MockMismatchedSchemaSinkAndSource.h"

#include "MockTestBTrait.h"

#include "MockPlatformClocks.h"

#include <new>
#include <map>
#include <set>
#include <algorithm>
#include <set>
#include <string>
#include <iterator>

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
#include <lwip/init.h>
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

using namespace nl;
using namespace nl::Weave::TLV;
using namespace nl::Weave::Profiles::DataManagement;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// System/Platform definitions
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace Private {

System::Error SetClock_RealTime(uint64_t newCurTime)
{
    return WEAVE_SYSTEM_NO_ERROR;
}

static System::Error GetClock_RealTime(uint64_t & curTime)
{
    curTime = 0x42; // arbitrary non-zero value.
    return WEAVE_SYSTEM_NO_ERROR;
}

} // namespace Private


namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current) {
namespace Platform {
    // for unit tests, the dummy critical section is sufficient.
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
}
}
}

static SubscriptionEngine *gSubscriptionEngine;

SubscriptionEngine * SubscriptionEngine::GetInstance()
{
    return gSubscriptionEngine;
}

static void TestTdmStatic_SingleLeafHandle(nlTestSuite *inSuite, void *inContext);

// Test Suite

/**
 *  Test Suite that lists all the test functions.
 */
static const nlTest sTests[] = {
    // Tests the static schema portions of TDM
    NL_TEST_DEF("Test Tdm (Static schema): Single leaf handle", TestTdmStatic_SingleLeafHandle),
    NL_TEST_SENTINEL()
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Testing NotificationEngine + TraitData
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current) {

using namespace Schema::Nest::Test::Trait;

class TestTdmSource : public TraitDataSource {
public:
    TestTdmSource();
    void Reset();

private:
    WEAVE_ERROR GetLeafData(PropertyPathHandle aLeafHandle, uint64_t aTagToWrite, TLVWriter &aWriter);
    WEAVE_ERROR GetNextDictionaryItemKey(PropertyPathHandle aDictionaryHandle, uintptr_t &aContext, PropertyDictionaryKey &aKey);

public:
    // 
    // The logical key here is the field 'da', which is of type uint32.
    //
    WdmDictionary<uint32_t, TestHTrait::StructDictionary> _dict;
};

TestTdmSource::TestTdmSource()
    : TraitDataSource(&TestHTrait::TraitSchema)
{
    // Using the Modify method to insert and modify.
    _dict.ModifyItem(1, [](auto &item) {
        item._logical_key = 10;
        item._data.da = 10;
        item._data.db = 1;
        item._data.dc = 2;
    });

    _dict.ModifyItem(2, [](auto &item) {
        item._logical_key = 20;
        item._data.da = 20;
        item._data.db = 3;
        item._data.dc = 4;
    });
}

void TestTdmSource::Reset()
{
}

WEAVE_ERROR TestTdmSource::GetNextDictionaryItemKey(PropertyPathHandle aDictionaryHandle, uintptr_t &aContext, PropertyDictionaryKey &aKey)
{
    static WdmDictionary<uint32_t, TestHTrait::StructDictionary>::DictKeyTableTypeIterator it;

    if (aContext == 0) {
        it = _dict.GetDictKeyTable().begin();
    }
    else {
        it++;
    }

    aContext = (uintptr_t)&it;

    if (it == _dict.GetDictKeyTable().end()) {
        return WEAVE_END_OF_INPUT;
    }
    else {
        aKey = it->_dict_key;
    }

    return WEAVE_NO_ERROR;
}

WEAVE_ERROR TestTdmSource::GetLeafData(PropertyPathHandle aLeafHandle, uint64_t aTagToWrite, TLVWriter &aWriter)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    PropertyPathHandle dictionaryItemHandle = kNullPropertyPathHandle;

    if (GetSchemaEngine()->IsInDictionary(aLeafHandle, dictionaryItemHandle)) {
        PropertyPathHandle dictionaryHandle = GetSchemaEngine()->GetParent(dictionaryItemHandle);
        PropertyDictionaryKey key = GetPropertyDictionaryKey(dictionaryItemHandle);

        if (dictionaryHandle == TestHTrait::kPropertyHandle_L) {
            auto it = _dict.GetDictKeyTable().find(key);

            if (it != _dict.GetDictKeyTable().end()) {
                TestHTrait::StructDictionary item = it->_data;
                uint32_t val;

                switch (GetPropertySchemaHandle(aLeafHandle)) {
                    case TestHTrait::kPropertyHandle_L_Value_Da:
                        WeaveLogDetail(DataManagement, "[TestTdmSource::GetLeafData] >> l[%u].da = %u", key, item.da);
                        val = item.da;
                        break;

                    case TestHTrait::kPropertyHandle_L_Value_Db:
                        WeaveLogDetail(DataManagement, "[TestTdmSource::GetLeafData] >> l[%u].db = %u", key, item.da);
                        val = item.db;
                        break;

                    case TestHTrait::kPropertyHandle_L_Value_Dc:
                        WeaveLogDetail(DataManagement, "[TestTdmSource::GetLeafData] >> l[%u].dc = %u", key, item.da);
                        val = item.dc;
                        break;

                    default:
                        WeaveLogError(DataManagement, "Unknown handle passed in!");
                        return WEAVE_ERROR_INVALID_ARGUMENT;
                        break;
                }

                err = aWriter.Put(aTagToWrite, val);
                SuccessOrExit(err);
            }
            else {
                WeaveLogError(DataManagement, "Requested key %u for dictionary handle %u that doesn't exist!", key, dictionaryHandle);
                return WEAVE_ERROR_INVALID_ARGUMENT;
            }
        }
    }

exit:
    return err;
}


class TestTdmSink : public TraitDataSink {
public:
    TestTdmSink();
    void Reset();
    void DumpChangeSets();

private:
    WEAVE_ERROR OnEvent(uint16_t aType, void *aInParam);
    WEAVE_ERROR SetLeafData(PropertyPathHandle aLeafHandle, nl::Weave::TLV::TLVReader &aReader);

public:
    //
    // Main canonical store of truth
    //
    WdmDictionary<uint32_t, TestHTrait::StructDictionary> _dict;

    //
    // Staged changes that are accrued and applied at the end
    // of a logical change
    //
    WdmDictionary<uint32_t, TestHTrait::StructDictionary> _stagedDict;

    //
    // Staged set of deleted path handles
    //
    std::set<PropertyPathHandle> _deletedDictItems;

    //
    // Staged item that gets appended to the staged dictionary on completion
    // of the modification.
    WdmDictionary<uint32_t, TestHTrait::StructDictionary>::Item _stagedDictItem;

    bool _isReplaceOperation;
};

TestTdmSink::TestTdmSink()
    : TraitDataSink(&TestHTrait::TraitSchema)
{
}

void TestTdmSink::Reset()
{
    ClearVersion();
    _stagedDict.GetDictKeyTable().clear();
    _deletedDictItems.clear();
}

WEAVE_ERROR TestTdmSink::OnEvent(uint16_t aType, void *aInParam)
{
    InEventParam *inParam = static_cast<InEventParam *>(aInParam);

    switch (aType) {
        case kEventChangeBegin:
            _stagedDict.GetDictKeyTable().clear();
            _deletedDictItems.clear();
            _isReplaceOperation = false;
            break;

        case kEventDictionaryItemDelete:
            WeaveLogError(DataManagement, "[TestTdmSink::OnEvent] Deleting %u:%u", 
                          GetPropertyDictionaryKey(inParam->mDictionaryItemDelete.mTargetHandle), GetPropertySchemaHandle(inParam->mDictionaryItemDelete.mTargetHandle));


            if (GetPropertySchemaHandle(inParam->mDictionaryItemDelete.mTargetHandle) == TestHTrait::kPropertyHandle_L) {
                _deletedDictItems.insert(GetPropertyDictionaryKey(inParam->mDictionaryItemDelete.mTargetHandle));
            }

            break;

        case kEventDictionaryItemModifyBegin:
        {
            auto it = _dict.GetDictKeyTable().find(GetPropertyDictionaryKey(inParam->mDictionaryItemModifyBegin.mTargetHandle));

            WeaveLogError(DataManagement, "[TestTdmSink::OnEvent] Adding/Modifying %u:%u", GetPropertyDictionaryKey(inParam->mDictionaryItemModifyBegin.mTargetHandle), GetPropertySchemaHandle(inParam->mDictionaryItemModifyBegin.mTargetHandle));

            _stagedDictItem._dict_key = GetPropertyDictionaryKey(inParam->mDictionaryItemModifyBegin.mTargetHandle);
            if (it != _dict.GetDictKeyTable().end()) {
                _stagedDictItem = *it;
            }

            break;
        }

        case kEventDictionaryItemModifyEnd:
            _stagedDict.GetDictKeyTable().insert(_stagedDictItem);
            break;

        case kEventDictionaryReplaceBegin:
            WeaveLogError(DataManagement, "[TestTdmSink::OnEvent] Replacing %u:%u", GetPropertyDictionaryKey(inParam->mDictionaryReplaceBegin.mTargetHandle), GetPropertySchemaHandle(inParam->mDictionaryReplaceBegin.mTargetHandle));

            _isReplaceOperation = true;
            _stagedDict.GetDictKeyTable().clear();
            break;


        case kEventChangeEnd:
            WeaveLogError(DataManagement, "[TestTdmSink::OnEvent] Change End");
           
            //
            // Delete items
            // 
            for (auto it = _deletedDictItems.begin(); it != _deletedDictItems.end(); it++) {
                _dict.GetDictKeyTable().erase(*it);
            }

            _dict.ItemsAdded(_stagedDict, [](auto &i) {
                printf("A %u: %u\n", i->_logical_key, i->_data.db);
            }, true);

            //
            // Only do the negative intersection if we're doing a full replace on the dictionary. Otherwise, we'll
            // unintentionally remove elements.
            //
            if (_isReplaceOperation) {
                _dict.ItemsRemoved(_stagedDict, [](auto &i) {
                    printf("R %u: %u\n", i->_logical_key, i->_data.db);
                }, true);
            }

            _dict.ItemsModified(_stagedDict, [](auto &oldd, auto &newd) {
                printf("M %u: %u %u\n", oldd->_logical_key, oldd->_data.db, newd->_data.db);
            }, true);

            break;
    }

    return WEAVE_NO_ERROR;
}

WEAVE_ERROR TestTdmSink::SetLeafData(PropertyPathHandle aHandle, TLVReader &aReader)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    PropertySchemaHandle schemaHandle = GetPropertySchemaHandle(aHandle);

    WeaveLogError(DataManagement, "[TestTdmSink::SetLeafData] << %u:%u", GetPropertyDictionaryKey(aHandle), GetPropertySchemaHandle(aHandle));
    
    switch (schemaHandle) {
        case TestHTrait::kPropertyHandle_L_Value_Da:
        {
            err = aReader.Get(_stagedDictItem._data.da);
            SuccessOrExit(err);

            _stagedDictItem._logical_key = _stagedDictItem._data.da;
            break;
        }

        case TestHTrait::kPropertyHandle_L_Value_Db:
        {
            err = aReader.Get(_stagedDictItem._data.db);
            SuccessOrExit(err);
            break;
        }

        case TestHTrait::kPropertyHandle_L_Value_Dc:
        {
            err = aReader.Get(_stagedDictItem._data.dc);
            SuccessOrExit(err);
            break;
        }
    }

exit:
    return err;
}

class TestTdm {
public:
    TestTdm();

    int Setup();
    int Teardown();
    int Reset();
    int BuildAndProcessNotify();

    void TestTdmStatic_SingleLeafHandle(nlTestSuite *inSuite);

private:
    SubscriptionHandler *mSubHandler;
    SubscriptionClient *mSubClient;
    NotificationEngine *mNotificationEngine;

    SubscriptionEngine mSubscriptionEngine;
    WeaveExchangeManager mExchangeMgr;
    SingleResourceSourceTraitCatalog::CatalogItem mSourceCatalogStore[5];
    SingleResourceSourceTraitCatalog mSourceCatalog;
    SingleResourceSinkTraitCatalog::CatalogItem mSinkCatalogStore[5];
    SingleResourceSinkTraitCatalog mSinkCatalog;
    TestTdmSource mTestTdmSource;
    TestTdmSink mTestTdmSink;
    Binding *mClientBinding;
    uint32_t mTestCase;
    WEAVE_ERROR AllocateBuffer(uint32_t desiredSize, uint32_t minSize);
};

TestTdm::TestTdm()
    : mSourceCatalog(ResourceIdentifier(ResourceIdentifier::SELF_NODE_ID), mSourceCatalogStore, 5),
      mSinkCatalog(ResourceIdentifier(ResourceIdentifier::SELF_NODE_ID), mSinkCatalogStore, 5),
      mClientBinding(NULL)
{
    mTestCase = 0;
}

int TestTdm::Setup()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TraitDataHandle testTdmSourceHandle;
    TraitDataHandle testTdmSinkHandle;
    SubscriptionHandler::TraitInstanceInfo *traitInstance = NULL;

    gSubscriptionEngine = &mSubscriptionEngine;

    // Initialize SubEngine and set it up
    err = mSubscriptionEngine.Init(&ExchangeMgr, NULL, NULL);
    SuccessOrExit(err);

    err = mSubscriptionEngine.EnablePublisher(NULL, &mSourceCatalog);
    SuccessOrExit(err);

    // Get a sub handler and prime it to the right state
    err = mSubscriptionEngine.NewSubscriptionHandler(&mSubHandler);
    SuccessOrExit(err);

    mSubHandler->mBinding = ExchangeMgr.NewBinding();
    mSubHandler->mBinding->BeginConfiguration().Transport_UDP();

    mClientBinding = ExchangeMgr.NewBinding();

    err = mSubscriptionEngine.NewClient(&mSubClient, mClientBinding, NULL, NULL, &mSinkCatalog, 0);
    SuccessOrExit(err);

    mNotificationEngine = &mSubscriptionEngine.mNotificationEngine;

    mSourceCatalog.Add(0, &mTestTdmSource, testTdmSourceHandle);
    mSinkCatalog.Add(0, &mTestTdmSink, testTdmSinkHandle);

    traitInstance = mSubscriptionEngine.mTraitInfoPool;
    mSubHandler->mTraitInstanceList = traitInstance;
    mSubHandler->mNumTraitInstances++;
    ++(SubscriptionEngine::GetInstance()->mNumTraitInfosInPool);

    traitInstance->Init();
    traitInstance->mTraitDataHandle = testTdmSourceHandle;
    traitInstance->mRequestedVersion = 1;

exit:
    if (err != WEAVE_NO_ERROR) {
        WeaveLogError(DataManagement, "Error setting up test: %d", err);
    }

    return err;
}

void TestTdm::TestTdmStatic_SingleLeafHandle(nlTestSuite *inSuite)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    bool testPass = false;

    Reset();

    //
    // Replace
    //
    mTestTdmSource.Lock(); 
    mTestTdmSource.SetDirty(TestHTrait::kPropertyHandle_L);
    mTestTdmSource.Unlock();

    err = BuildAndProcessNotify();
    SuccessOrExit(err);

    printf("Equal: %d\n", mTestTdmSource._dict.IsEqual(mTestTdmSink._dict));

    //
    // Delete Item
    //
    mTestTdmSource.Lock(); 
    mTestTdmSource._dict.GetDictKeyTable().erase(0); 
    mTestTdmSource.DeleteKey(CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value, 0));
    mTestTdmSource.Unlock();

    err = BuildAndProcessNotify();
    SuccessOrExit(err);

    printf("Equal: %d\n", mTestTdmSource._dict.IsEqual(mTestTdmSink._dict));
   
    //
    // Add Item
    //
    mTestTdmSource.Lock();
    mTestTdmSource._dict.ModifyItem(10, [](auto &i) {
        i._logical_key = 300;
        i._data.da = 300;
        i._data.db = 30;
        i._data.dc = 30;
    });

    mTestTdmSource.Lock(); 
    mTestTdmSource.SetDirty(CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L_Value, 10));
    mTestTdmSource.Unlock();

    err = BuildAndProcessNotify();
    SuccessOrExit(err);

    printf("Equal: %d\n", mTestTdmSource._dict.IsEqual(mTestTdmSink._dict));

    //
    // Change dictionary keys, but keep logical keys + data stable.
    //
    mTestTdmSource.Lock();
    mTestTdmSource._dict.ModifyItem(10, [](auto &i) {
        i._dict_key = 100;
    });

    mTestTdmSource._dict.ModifyItem(1, [](auto &i) {
        i._dict_key = 1000;
    });
    
    mTestTdmSource.SetDirty(TestHTrait::kPropertyHandle_L);
    mTestTdmSource.Unlock();

    err = BuildAndProcessNotify();
    SuccessOrExit(err);

    printf("Equal: %d\n", mTestTdmSource._dict.IsEqual(mTestTdmSink._dict));
    
    
exit:
    NL_TEST_ASSERT(inSuite, testPass);
}

int TestTdm::Teardown()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (mClientBinding != NULL)
    {
        mClientBinding->Release();
        mClientBinding = NULL;
    }

    return err;
}

int TestTdm::Reset()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    mSubHandler->MoveToState(SubscriptionHandler::kState_SubscriptionEstablished_Idle);
    
    mTestTdmSink.Reset();
    mTestTdmSource.Reset();

    mNotificationEngine->mGraphSolver.ClearDirty();

    return err;
}

int TestTdm::BuildAndProcessNotify()
{
    bool isSubscriptionClean;
    NotificationEngine::NotifyRequestBuilder notifyRequest;
    NotificationRequest::Parser notify;
    PacketBuffer *buf = NULL;
    TLVWriter writer;
    TLVReader reader;
    TLVType dummyType1, dummyType2;
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    bool neWriteInProgress = false;
    uint32_t maxNotificationSize = 0;
    uint32_t maxPayloadSize = 0;

    maxNotificationSize = mSubHandler->GetMaxNotificationSize();

    err = mSubHandler->mBinding->AllocateRightSizedBuffer(buf, maxNotificationSize, WDM_MIN_NOTIFICATION_SIZE, maxPayloadSize);
    SuccessOrExit(err);

    err = notifyRequest.Init(buf, &writer, mSubHandler, maxPayloadSize);
    SuccessOrExit(err);

    err = mNotificationEngine->BuildSingleNotifyRequestDataList(mSubHandler, notifyRequest, isSubscriptionClean, neWriteInProgress);
    SuccessOrExit(err);

    if (neWriteInProgress)
    {
        err = notifyRequest.MoveToState(NotificationEngine::kNotifyRequestBuilder_Idle);
        SuccessOrExit(err);

        reader.Init(buf);

        err = reader.Next();
        SuccessOrExit(err);

        notify.Init(reader);

        err = notify.CheckSchemaValidity();
        SuccessOrExit(err);

        // Enter the struct
        err = reader.EnterContainer(dummyType1);
        SuccessOrExit(err);

        // SubscriptionId
        err = reader.Next();
        SuccessOrExit(err);

        err = reader.Next();
        SuccessOrExit(err);

        VerifyOrExit(nl::Weave::TLV::kTLVType_Array == reader.GetType(), err = WEAVE_ERROR_WRONG_TLV_TYPE);

        err = reader.EnterContainer(dummyType2);
        SuccessOrExit(err);

        err = mSubClient->ProcessDataList(reader);
        SuccessOrExit(err);
    }
    else
    {
        WeaveLogDetail(DataManagement, "nothing has been written");
    }

exit:
    if (buf) {
        PacketBuffer::Free(buf);
    }

    return err;
}

WEAVE_ERROR TestTdm::AllocateBuffer(uint32_t desiredSize, uint32_t minSize)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint32_t maxPayloadSize = 0;
    PacketBuffer *buf = NULL;


    err = mSubHandler->mBinding->AllocateRightSizedBuffer(buf, desiredSize, minSize, maxPayloadSize);
    SuccessOrExit(err);

exit:

    if (buf)
    {
        PacketBuffer::Free(buf);
    }

    return err;
}

} // WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current)
}
}
}

TestTdm *gTestTdm;

/**
 *  Set up the test suite.
 */
static int TestSetup(void *inContext)
{
    static TestTdm testTdm;
    gTestTdm = &testTdm;

    return testTdm.Setup();
}

/**
 *  Tear down the test suite.
 */
static int TestTeardown(void *inContext)
{
    return gTestTdm->Teardown();
}

static void TestTdmStatic_SingleLeafHandle(nlTestSuite *inSuite, void *inContext)
{
    gTestTdm->TestTdmStatic_SingleLeafHandle(inSuite);
}

/**
 *  Main
 */
int main(int argc, char *argv[])
{
#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    tcpip_init(NULL, NULL);
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

    MockPlatform::gMockPlatformClocks.GetClock_RealTime = Private::GetClock_RealTime;
    MockPlatform::gMockPlatformClocks.SetClock_RealTime = Private::SetClock_RealTime;

    nlTestSuite theSuite = {
        "weave-tdm",
        &sTests[0],
        TestSetup,
        TestTeardown
    };

    // Generate machine-readable, comma-separated value (CSV) output.
    nl_test_set_output_style(OUTPUT_CSV);

    // Run test suit against one context
    nlTestRunner(&theSuite, NULL);

    return nlTestRunnerStats(&theSuite);
}
