/*
 *
 *    Copyright (c) 2019 Google, LLC.
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
 *      This file implements NLWDMClient interface
 *
 */

#import <Foundation/Foundation.h>
#import "NLWeaveStack.h"
#import "NLLogging.h"

#include <Weave/Core/WeaveError.h>

#include <Weave/Support/CodeUtils.h>

#include <WeaveDeviceManager.h>

#include <net/if.h>

#import "NLWeaveBleDelegate.h"
#import "NLWeaveDeviceManager_Protected.h"
#import "NLWeaveDeviceDescriptor_Protected.h"
#import "NLNetworkInfo_Protected.h"
#import "NLProfileStatusError.h"
#import "NLWeaveError_Protected.h"
#include <Weave/Profiles/device-description/DeviceDescription.h>
#import "NLIdentifyDeviceCriteria_Protected.h"
#import "Base64Encoding.h"

#include <Weave/Core/WeaveCore.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Profiles/common/CommonProfile.h>
#include <Weave/Profiles/data-management/Current/WdmManagedNamespace.h>
#include <WeaveDataManagementClient.h>
#include <Weave/Support/NLDLLUtil.h>
#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Profiles/common/WeaveMessage.h>
#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Profiles/status-report/StatusReportProfile.h>
#include <SystemLayer/SystemPacketBuffer.h>
#include <Weave/Profiles/data-management/SubscriptionClient.h>

#import "NLWDMClient_Protected.h"
#import "NLGenericTraitUpdatableDataSink_Protected.h"

using namespace nl::Weave::Profiles;
using namespace nl::Weave::Profiles::DataManagement;

static void EngineEventCallback(void * const aAppState,
                                SubscriptionEngine::EventID aEvent,
                                const SubscriptionEngine::InEventParam & aInParam, SubscriptionEngine::OutEventParam & aOutParam);

static void BindingEventCallback (void * const apAppState, const nl::Weave::Binding::EventType aEvent,
                                  const nl::Weave::Binding::InEventParam & aInParam, nl::Weave::Binding::OutEventParam & aOutParam);

void EngineEventCallback(void * const aAppState,
                                SubscriptionEngine::EventID aEvent,
                                const SubscriptionEngine::InEventParam & aInParam, SubscriptionEngine::OutEventParam & aOutParam)
{
    switch (aEvent)
    {
        default:
            SubscriptionEngine::DefaultEventHandler(aEvent, aInParam, aOutParam);
            break;
    }
}


void BindingEventCallback (void * const apAppState, const nl::Weave::Binding::EventType aEvent,
                                  const nl::Weave::Binding::InEventParam & aInParam, nl::Weave::Binding::OutEventParam & aOutParam)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    nl::Weave::Binding * binding = aInParam.Source;

    WeaveLogDetail(DeviceManager, "%s: Event(%d)", __func__, aEvent);

    nl::Weave::DeviceManager::WeaveDeviceManager * const pDeviceMgr = reinterpret_cast<nl::Weave::DeviceManager::WeaveDeviceManager *>(apAppState);

    switch (aEvent)
    {
        case nl::Weave::Binding::kEvent_PrepareRequested:
            WeaveLogDetail(DeviceManager, "kEvent_PrepareRequested");
            err = pDeviceMgr->ConfigureBinding(binding);
            aOutParam.PrepareRequested.PrepareError = err;
            SuccessOrExit(err);
            break;

        case nl::Weave::Binding::kEvent_PrepareFailed:
            err = aInParam.PrepareFailed.Reason;
            WeaveLogDetail(DeviceManager, "kEvent_PrepareFailed: reason %d", err);
            break;

        case nl::Weave::Binding::kEvent_BindingFailed:
            err = aInParam.BindingFailed.Reason;
            WeaveLogDetail(DeviceManager, "kEvent_BindingFailed: reason %d", err);
            break;

        case nl::Weave::Binding::kEvent_BindingReady:
            WeaveLogDetail(DeviceManager, "kEvent_BindingReady");
            break;

        case nl::Weave::Binding::kEvent_DefaultCheck:
            WeaveLogDetail(DeviceManager, "kEvent_DefaultCheck");

        default:
            nl::Weave::Binding::DefaultEventHandler(apAppState, aEvent, aInParam, aOutParam);
    }

exit:
    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogDetail(DeviceManager, "error in BindingEventCallback");
    }
}

@interface NLWDMClient () {
    nl::Weave::DeviceManager::WDMClient * _mWeaveCppWDMClient;
    dispatch_queue_t _mWeaveWorkQueue;
    dispatch_queue_t _mAppCallbackQueue;

    // Note that these context variables are independent from context variables in the C++ Weave Device Manager,
    // for the C++ Weave Device Manager only takes one pointer as the app context, which is not enough to hold all
    // context information we need.
    WDMCompletionBlock _mCompletionHandler;
    WDMFailureBlock _mFailureHandler;
    NSString * _mRequestName;
    NSMutableDictionary * _mTraitMap;
}

- (NSString *)GetCurrentRequest;
@end

@implementation NLWDMClient
@synthesize resultCallbackQueue = _mAppCallbackQueue;

/**
 @note
 This function can only be called by the ARC runtime
 */
- (void)dealloc
{
    // This method can only be called by ARC
    // Let's not rely on this unpredictable mechanism for de-initialization
    // application shall call ShutdownStack if it want to cleanly destroy everything before application termination
    WDM_LOG_METHOD_SIG();

    [self MarkTransactionCompleted];

    _mRequestName = @"dealloc-Shutdown";

    // we need to force the queue to be Weave work queue, as dealloc could be
    // called at random queues (most probably from UI, when the app de-reference this wdmClient)
    dispatch_sync(_mWeaveWorkQueue, ^() {
        [self Shutdown_Internal];
    });
}

- (instancetype)init:(NSString *)name
      weaveWorkQueue:(dispatch_queue_t)weaveWorkQueue
    appCallbackQueue:(dispatch_queue_t)appCallbackQueue
         exchangeMgr:(nl::Weave::WeaveExchangeManager *)exchangeMgr
         messageLayer:(nl::Weave::WeaveMessageLayer *)messageLayer
         nlWeaveDeviceManager:(NLWeaveDeviceManager *)deviceMgr
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    long long deviceMgrCppPtr = 0;
    nl::Weave::Binding * pBinding = NULL;
    WDM_LOG_METHOD_SIG();

    self = [super init];
    VerifyOrExit(nil != self, err = WEAVE_ERROR_NO_MEMORY);

    _mWeaveWorkQueue = weaveWorkQueue;
    _mAppCallbackQueue = appCallbackQueue;

    _name = name;

    WeaveLogProgress(DeviceManager, "NewWDMClient() called");

    [deviceMgr GetDeviceMgrPtr:&deviceMgrCppPtr];
    pBinding = exchangeMgr->NewBinding(BindingEventCallback, (nl::Weave::DeviceManager::WeaveDeviceManager*)deviceMgrCppPtr);
    VerifyOrExit(NULL != pBinding, err = WEAVE_ERROR_NO_MEMORY);

    if (pBinding->CanBePrepared())
    {
        err = pBinding->RequestPrepare();
        SuccessOrExit(err);
    }

    _mWeaveCppWDMClient = new nl::Weave::DeviceManager::WDMClient();
    VerifyOrExit(NULL != _mWeaveCppWDMClient, err = WEAVE_ERROR_NO_MEMORY);

    err = _mWeaveCppWDMClient->Init(messageLayer, pBinding);
    SuccessOrExit(err);

    // simple bridge shall not increase our reference count
    _mWeaveCppWDMClient->mpAppState = (__bridge void *) self;

    _mTraitMap = [[NSMutableDictionary alloc] init];
    [self MarkTransactionCompleted];

exit:
    id result = nil;
    if (WEAVE_NO_ERROR == err) {
        result = self;
    } else {
        WDM_LOG_ERROR(@"Error in init : (%d) %@\n", err, [NSString stringWithUTF8String:nl::ErrorStr(err)]);
        [self Shutdown_Internal];
        if (NULL != pBinding)
        {
            pBinding->Release();
        }
    }

    return result;
}

static void HandleWDMClientComplete(void * wdmClient, void * reqState)
{
    WDM_LOG_DEBUG(@"HandleWDMClientComplete");

    NLWDMClient * client = (__bridge NLWDMClient *) reqState;
    // ignore the pointer to C++ device manager
    (void) wdmClient;
    [client DispatchAsyncCompletionBlock:nil];
}

static void onWDMClientError(void * wdmClient, void * appReqState, WEAVE_ERROR code,
    nl::Weave::DeviceManager::DeviceStatus * devStatus)
{
    WDM_LOG_DEBUG(@"onWDMClientError");

    NSError * error = nil;
    NSDictionary * userInfo = nil;

    NLWDMClient * client = (__bridge NLWDMClient *) appReqState;
    // ignore the pointer to C++ device manager
    (void) wdmClient;

    WDM_LOG_DEBUG(@"%@: Received error response to request %@, wdmClientErr = %d, devStatus = %p\n", client.name,
        [client GetCurrentRequest], code, devStatus);

    NLWeaveRequestError requestError;
    if (devStatus != NULL && code == WEAVE_ERROR_STATUS_REPORT_RECEIVED) {
        NLProfileStatusError * statusError = [[NLProfileStatusError alloc]
            initWithProfileId:devStatus->StatusProfileId
                   statusCode:devStatus->StatusCode
                    errorCode:devStatus->SystemErrorCode
                 statusReport:[client statusReportToString:devStatus->StatusProfileId statusCode:devStatus->StatusCode]];
        requestError = NLWeaveRequestError_ProfileStatusError;
        userInfo = @{ @"WeaveRequestErrorType" : @(requestError), @"errorInfo" : statusError };

        WDM_LOG_DEBUG(@"%@: status error: %@", client.name, userInfo);
    } else {
        NLWeaveError * weaveError = [[NLWeaveError alloc] initWithWeaveError:code
                                                                      report:[NSString stringWithUTF8String:nl::ErrorStr(code)]];
        requestError = NLWeaveRequestError_WeaveError;
        userInfo = @{ @"WeaveRequestErrorType" : @(requestError), @"errorInfo" : weaveError };
    }

    error = [NSError errorWithDomain:@"com.nest.error" code:code userInfo:userInfo];

    [client DispatchAsyncDefaultFailureBlock:error];
}

- (void)MarkTransactionCompleted
{
    _mRequestName = nil;
    _mCompletionHandler = nil;
    _mFailureHandler = nil;
}

- (NSString *)GetCurrentRequest
{
    return _mRequestName;
}

- (void)DispatchAsyncFailureBlock:(WEAVE_ERROR)code taskName:(NSString *)taskName handler:(WDMFailureBlock)handler
{
    NSError * error =
        [NSError errorWithDomain:@"com.nest.error"
                            code:code
                        userInfo:[NSDictionary dictionaryWithObjectsAndKeys:[NSString stringWithUTF8String:nl::ErrorStr(code)],
                                               @"error", nil]];
    [self DispatchAsyncFailureBlockWithError:error taskName:taskName handler:handler];
}

- (void)DispatchAsyncFailureBlockWithError:(NSError *)error taskName:(NSString *)taskName handler:(WDMFailureBlock)handler
{
    if (NULL != handler) {
        // we use async because we don't need to wait for completion of this final completion report
        dispatch_async(_mAppCallbackQueue, ^() {
            WDM_LOG_DEBUG(@"%@: Calling failure handler for %@", _name, taskName);
            handler(_owner, error);
        });
    } else {
        WDM_LOG_DEBUG(@"%@: Skipping failure handler for %@", _name, taskName);
    }
}

- (void)DispatchAsyncDefaultFailureBlockWithCode:(WEAVE_ERROR)code
{
    NSError * error = [NSError errorWithDomain:@"com.nest.error" code:code userInfo:nil];
    [self DispatchAsyncDefaultFailureBlock:error];
}

- (void)DispatchAsyncDefaultFailureBlock:(NSError *)error
{
    NSString * taskName = _mRequestName;
    WDMFailureBlock failureHandler = _mFailureHandler;

    [self MarkTransactionCompleted];
    [self DispatchAsyncFailureBlockWithError:error taskName:taskName handler:failureHandler];
}

- (void)DispatchAsyncCompletionBlock:(id)data
{
    WDMCompletionBlock completionHandler = _mCompletionHandler;

    [self MarkTransactionCompleted];

    if (nil != completionHandler) {
        dispatch_async(_mAppCallbackQueue, ^() {
            completionHandler(_owner, data);
        });
    }
}

- (void)DispatchAsyncResponseBlock:(id)data
{
    WDMCompletionBlock completionHandler = _mCompletionHandler;

    if (nil != completionHandler) {
        dispatch_async(_mAppCallbackQueue, ^() {
            completionHandler(_owner, data);
        });
    }
}

- (NSString *)toErrorString:(WEAVE_ERROR)err
{
    WDM_LOG_METHOD_SIG();

    __block NSString * msg = nil;

    dispatch_sync(_mWeaveWorkQueue, ^() {
        msg = [NSString stringWithUTF8String:nl::ErrorStr(err)];
    });

    return msg;
}

- (NSString *)statusReportToString:(NSUInteger)profileId statusCode:(NSInteger)statusCode
{
    WDM_LOG_METHOD_SIG();

    NSString * report = nil;

    const char * result = nl::StatusReportStr((uint32_t) profileId, statusCode);

    if (result) {
        report = [NSString stringWithUTF8String:result];
    }

    return report;
}

- (void)Shutdown_Internal
{
    WDM_LOG_METHOD_SIG();

    NSArray * keys =[_mTraitMap allKeys];
 
    for(NSString * key in keys)
    {
       NLGenericTraitUpdatableDataSink * pDataSink =[_mTraitMap objectForKey:key];
       NSLog(@"key=%@, pDataSink=%@",key, pDataSink);
       if ((NSNull *)pDataSink != [NSNull null])
       {
           [pDataSink Shutdown_Internal];
       }
    }
    [_mTraitMap removeAllObjects];
    _mTraitMap = nil;

    NSLog(@"_mTraitMap =%@",_mTraitMap);
    // there is no need to release Objective C objects, as we have ARC for them
    if (_mWeaveCppWDMClient) {
        WDM_LOG_ERROR(@"Shutdown C++ Weave WDMClient");

        _mWeaveCppWDMClient->Close();

        delete _mWeaveCppWDMClient;
        _mWeaveCppWDMClient = NULL;
    }

    [self DispatchAsyncCompletionBlock:nil];
}

- (void)Shutdown:(WDMCompletionBlock)completionHandler
{
    WDM_LOG_METHOD_SIG();

    dispatch_async(_mWeaveWorkQueue, ^() {
        // Note that conceptually we probably should not call shutdown while we're still closing the device manager and vice versa,
        // but both Shutdown and Close are implemented in a synchronous way so there is really no chance for them to be intertwined.
        if (nil != _mRequestName) {
            WDM_LOG_ERROR(@"%@: Forcefully shutdown while we're still executing %@, continue shutdown", _name, _mRequestName);
        }

        [self MarkTransactionCompleted];

        _mCompletionHandler = completionHandler;
        _mRequestName = @"Shutdown";
        [self Shutdown_Internal];
    });
}

- (void)Close:(WDMCompletionBlock)completionHandler failure:(WDMFailureBlock)failureHandler;
{
    WDM_LOG_METHOD_SIG();

    NSString * taskName = @"Close";

    dispatch_async(_mWeaveWorkQueue, ^() {
        bool IsOKay = true;
        if (nil != _mRequestName) {
            // Note that conceptually we probably should not call shutdown while we're still closing the device manager and vice
            // versa, but both Shutdown and Close are implemented in a synchronous way so there is really no chance for them to be
            // intertwined.
            if ([_mRequestName isEqualToString:@"Shutdown"]) {
                IsOKay = false;
            }
        }

        if (IsOKay) {
            // Note that we're already in Weave work queue, which means all callbacks for the previous or current request
            // has either happened/completed or would be canceled by this call to Close. Therefore, it should be safe
            // to wipe out request context variables like _mRequestName and _mCompletionHandler.
            _mWeaveCppWDMClient->Close();

            _mRequestName = taskName;
            _mCompletionHandler = completionHandler;
            [self DispatchAsyncCompletionBlock:nil];
        } else {
            WDM_LOG_ERROR(@"%@: Attemp to %@ while we're still executing %@, ignore", _name, taskName, _mRequestName);

            // do not change _mRequestName, as we're rejecting this request
            [self DispatchAsyncFailureBlock:WEAVE_ERROR_INCORRECT_STATE taskName:taskName handler:failureHandler];
        }
    });

}

- (void) removeDataSinkRef:(long long)traitInstancePtr;
{
    NSString *address = [NSString stringWithFormat:@"%lld", traitInstancePtr];

    if ([_mTraitMap objectForKey:address] != nil) 
    {
        _mTraitMap[address] = [NSNull null];
    }
}

- (NLGenericTraitUpdatableDataSink *)newDataSinkResourceType: (uint16_t)resourceType
                                                ResourceId: (NSString *)resourceId
                                                 ProfileId: (uint32_t)profileId
                                                InstanceId: (uint64_t)instanceId
                                                      Path: (NSString *)path;
{
    __block WEAVE_ERROR err = WEAVE_NO_ERROR;
    __block uint64_t result = 0;

    __block nl::Weave::DeviceManager::GenericTraitUpdatableDataSink * pDataSink = NULL;
    WDM_LOG_METHOD_SIG();

    //VerifyOrExit(NULL != _mWeaveCppWDMClient, err = WEAVE_ERROR_INCORRECT_STATE);

    dispatch_sync(_mWeaveWorkQueue, ^() {
        Base64Encoding * base64coder = [Base64Encoding createBase64StringEncoding];
        NSData * resourceIdData = [base64coder decode:resourceId];

        if (nil == resourceId)
        {
            ResourceIdentifier resId = ResourceIdentifier(ResourceIdentifier::RESOURCE_TYPE_RESERVED, ResourceIdentifier::SELF_NODE_ID);
            err = _mWeaveCppWDMClient->NewDataSink(resId, profileId, instanceId, [path UTF8String], pDataSink);
        }
        else
        {
            uint32_t resourceIdLen = (uint32_t)[resourceIdData length];
            uint8_t * pResourceId = (uint8_t *) [resourceIdData bytes];
            ResourceIdentifier resId = ResourceIdentifier((uint16_t)resourceType, pResourceId, resourceIdLen);
            err = _mWeaveCppWDMClient->NewDataSink(resId, profileId, instanceId, [path UTF8String], pDataSink);
        }
    });
 
     if (err != WEAVE_NO_ERROR || pDataSink == NULL)
    {
        WDM_LOG_ERROR(@"pDataSink is not ready");
        return nil;
    }

    NSString *address = [NSString stringWithFormat:@"%lld", (long long)pDataSink];

    if ([_mTraitMap objectForKey:address] == nil) 
    {
        NLGenericTraitUpdatableDataSink * pTrait = [[NLGenericTraitUpdatableDataSink alloc] init:_name
                                                                                            weaveWorkQueue:_mWeaveWorkQueue
                                                                                            appCallbackQueue:_mAppCallbackQueue
                                                                                            genericTraitUpdatableDataSinkPtr: pDataSink
                                                                                            nlWDMClient:self];
        [_mTraitMap setObject:pTrait forKey:address];
    }

    return [_mTraitMap objectForKey:address];
}

- (void)flushUpdateCompletion:(WDMClientCompletionBlock)completionHandler
                           failure:(WDMClientFailureBlock)failureHandler
{
    WDM_LOG_METHOD_SIG();

    NSString * taskName = @"FlushUpdate";

    // we use async for the results are sent back to caller via async means also
    dispatch_async(_mWeaveWorkQueue, ^() {
        if (nil == _mRequestName) {
            _mRequestName = taskName;
            _mCompletionHandler = completionHandler;
            _mFailureHandler = failureHandler;

            WEAVE_ERROR err = _mWeaveCppWDMClient->FlushUpdate((__bridge void *) self, HandleWDMClientComplete, onWDMClientError);

            if (WEAVE_NO_ERROR != err) {
                [self DispatchAsyncDefaultFailureBlockWithCode:err];
            }
        } else {
            WDM_LOG_ERROR(@"%@: Attemp to %@ while we're still executing %@, ignore", _name, taskName, _mRequestName);

            // do not change _mRequestName, as we're rejecting this request
            [self DispatchAsyncFailureBlock:WEAVE_ERROR_INCORRECT_STATE taskName:taskName handler:failureHandler];
        }
    });
}

- (void)refreshDataCompletion:(WDMClientCompletionBlock)completionHandler failure:(WDMClientFailureBlock)failureHandler
{
    WDM_LOG_METHOD_SIG();

    NSString * taskName = @"RefreshData";

    // we use async for the results are sent back to caller via async means also
    dispatch_async(_mWeaveWorkQueue, ^() {
        if (nil == _mRequestName) {
            _mRequestName = taskName;
            _mCompletionHandler = completionHandler;
            _mFailureHandler = failureHandler;

            WEAVE_ERROR err = _mWeaveCppWDMClient->RefreshData((__bridge void *) self, HandleWDMClientComplete, onWDMClientError, NULL);

            if (WEAVE_NO_ERROR != err) {
                [self DispatchAsyncDefaultFailureBlockWithCode:err];
            }
        } else {
            WDM_LOG_ERROR(@"%@: Attemp to %@ while we're still executing %@, ignore", _name, taskName, _mRequestName);

            // do not change _mRequestName, as we're rejecting this request
            [self DispatchAsyncFailureBlock:WEAVE_ERROR_INCORRECT_STATE taskName:taskName handler:failureHandler];
        }
    });
}

@end

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current) {

SubscriptionEngine * SubscriptionEngine::GetInstance()
{
    static nl::Weave::Profiles::DataManagement::SubscriptionEngine sWdmSubscriptionEngine;
    return &sWdmSubscriptionEngine;
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
