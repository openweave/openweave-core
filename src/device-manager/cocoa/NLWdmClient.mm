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
 *      This file implements NLWdmClient interface.
 *      This is WEAVE_CONFIG_DATA_MANAGEMENT_EXPERIMENTAL feature.
 *
 */

#import <Foundation/Foundation.h>
#import "NLWeaveStack.h"
#import "NLLogging.h"

#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveError.h>
#include <Weave/Support/CodeUtils.h>
#include <Weave/Support/NLDLLUtil.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Profiles/common/CommonProfile.h>
#include <Weave/Profiles/data-management/Current/WdmManagedNamespace.h>
#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Profiles/data-management/SubscriptionClient.h>
#include <WeaveDeviceManager.h>
#include <WeaveDataManagementClient.h>

#import "NLWeaveDeviceManager_Protected.h"
#import "NLProfileStatusError.h"
#import "NLWeaveError_Protected.h"
#import "NLWdmClient_Protected.h"
#import "NLGenericTraitUpdatableDataSink.h"
#import "NLGenericTraitUpdatableDataSink_Protected.h"
#import "NLResourceIdentifier.h"
#import "NLResourceIdentifier_Protected.h"
#import "NLWdmClientFlushUpdateError.h"
#import "NLWdmClientFlushUpdateDeviceStatusError.h"

using namespace nl::Weave::Profiles;
using namespace nl::Weave::Profiles::DataManagement;

#if WEAVE_CONFIG_DATA_MANAGEMENT_CLIENT_EXPERIMENTAL
static void bindingEventCallback(void * const apAppState, const nl::Weave::Binding::EventType aEvent,
    const nl::Weave::Binding::InEventParam & aInParam, nl::Weave::Binding::OutEventParam & aOutParam);

void bindingEventCallback(void * const apAppState, const nl::Weave::Binding::EventType aEvent,
    const nl::Weave::Binding::InEventParam & aInParam, nl::Weave::Binding::OutEventParam & aOutParam)
{
    WDM_LOG_DEBUG(@"%s: Event(%d)", __func__, aEvent);

    switch (aEvent) {
    case nl::Weave::Binding::kEvent_PrepareRequested:
        WDM_LOG_DEBUG(@"kEvent_PrepareRequested");
        break;

    case nl::Weave::Binding::kEvent_PrepareFailed:
        WDM_LOG_DEBUG(@"kEvent_PrepareFailed: reason %s", ::nl::ErrorStr(aInParam.PrepareFailed.Reason));
        break;

    case nl::Weave::Binding::kEvent_BindingFailed:
        WDM_LOG_DEBUG(@"kEvent_BindingFailed: reason %s", ::nl::ErrorStr(aInParam.PrepareFailed.Reason));
        break;

    case nl::Weave::Binding::kEvent_BindingReady:
        WDM_LOG_DEBUG(@"kEvent_BindingReady");
        break;

    case nl::Weave::Binding::kEvent_DefaultCheck:
        WDM_LOG_DEBUG(@"kEvent_DefaultCheck");
        // fall through
        OS_FALLTHROUGH;
    default:
        nl::Weave::Binding::DefaultEventHandler(apAppState, aEvent, aInParam, aOutParam);
    }
}

@interface NLWdmClient () {
    nl::Weave::DeviceManager::WdmClient * _mWeaveCppWdmClient;
    dispatch_queue_t _mWeaveWorkQueue;
    dispatch_queue_t _mAppCallbackQueue;

    WdmClientCompletionBlock _mCompletionHandler;
    WdmClientFailureBlock _mFailureHandler;
    NSString * _mRequestName;
    NSMutableDictionary * _mTraitMap;
}

- (NSString *)getCurrentRequest;
@end

@implementation NLWdmClient

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

    [self markTransactionCompleted];

    _mRequestName = @"dealloc-Shutdown";

    // we need to force the queue to be Weave work queue, as dealloc could be
    // called at random queues (most probably from UI, when the app de-reference this wdmClient)
    dispatch_sync(_mWeaveWorkQueue, ^() {
        [self shutdown_Internal];
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
    id result = nil;
    WDM_LOG_METHOD_SIG();

    self = [super init];
    VerifyOrExit(nil != self, err = WEAVE_ERROR_NO_MEMORY);

    _mWeaveWorkQueue = weaveWorkQueue;
    _mAppCallbackQueue = appCallbackQueue;

    _name = name;

    WDM_LOG_DEBUG(@"NewWdmClient() called");

    [deviceMgr GetDeviceMgrPtr:&deviceMgrCppPtr];
    pBinding = exchangeMgr->NewBinding(bindingEventCallback, (nl::Weave::DeviceManager::WeaveDeviceManager *) deviceMgrCppPtr);
    VerifyOrExit(NULL != pBinding, err = WEAVE_ERROR_NO_MEMORY);

    err = ((nl::Weave::DeviceManager::WeaveDeviceManager *) deviceMgrCppPtr)->ConfigureBinding(pBinding);
    SuccessOrExit(err);

    _mWeaveCppWdmClient = new nl::Weave::DeviceManager::WdmClient();
    VerifyOrExit(NULL != _mWeaveCppWdmClient, err = WEAVE_ERROR_NO_MEMORY);

    err = _mWeaveCppWdmClient->Init(messageLayer, pBinding);
    SuccessOrExit(err);

    _mWeaveCppWdmClient->mpAppState = (__bridge void *) self;

    _mTraitMap = [[NSMutableDictionary alloc] init];
    [self markTransactionCompleted];

exit:
    if (WEAVE_NO_ERROR == err) {
        result = self;
    } else {
        WDM_LOG_ERROR(@"Error in init : (%d) %@\n", err, [NSString stringWithUTF8String:nl::ErrorStr(err)]);
        [self shutdown_Internal];
        if (NULL != pBinding) {
            pBinding->Release();
        }
    }

    return result;
}

static void onWdmClientComplete(void * appState, void * reqState)
{
    WDM_LOG_DEBUG(@"onWdmClientComplete");

    NLWdmClient * client = (__bridge NLWdmClient *) reqState;
    [client dispatchAsyncCompletionBlock:nil];
}

static void onWdmClientError(
    void * appState, void * appReqState, WEAVE_ERROR code, nl::Weave::DeviceManager::DeviceStatus * devStatus)
{
    WDM_LOG_DEBUG(@"onWdmClientError");

    NSError * error = nil;
    NSDictionary * userInfo = nil;

    NLWdmClient * client = (__bridge NLWdmClient *) appReqState;

    WDM_LOG_DEBUG(@"%@: Received error response to request %@, wdmClientErr = %d, devStatus = %p\n", client.name,
        [client getCurrentRequest], code, devStatus);

    NLWeaveRequestError requestError;
    if (devStatus != NULL && code == WEAVE_ERROR_STATUS_REPORT_RECEIVED) {
        NLProfileStatusError * statusError = [[NLProfileStatusError alloc]
            initWithProfileId:devStatus->StatusProfileId
                   statusCode:devStatus->StatusCode
                    errorCode:devStatus->SystemErrorCode
                 statusReport:[client statusReportToString:devStatus->StatusProfileId statusCode:devStatus->StatusCode]];
        requestError = NLWeaveRequestError_ProfileStatusError;
        userInfo = @{@"WeaveRequestErrorType" : @(requestError), @"errorInfo" : statusError};

        WDM_LOG_DEBUG(@"%@: status error: %@", client.name, userInfo);
    } else {
        NLWeaveError * weaveError = [[NLWeaveError alloc] initWithWeaveError:code
                                                                      report:[NSString stringWithUTF8String:nl::ErrorStr(code)]];
        requestError = NLWeaveRequestError_WeaveError;
        userInfo = @{@"WeaveRequestErrorType" : @(requestError), @"errorInfo" : weaveError};
    }

    error = [NSError errorWithDomain:@"com.nest.error" code:code userInfo:userInfo];

    [client dispatchAsyncDefaultFailureBlock:error];
}

static void onWdmClientFlushUpdateComplete(
    void * appState, void * appReqState, uint16_t pathCount, nl::Weave::DeviceManager::WdmClientFlushUpdateStatus * statusResults)
{
    WDM_LOG_DEBUG(@"onWdmClientFlushUpdateComplete");

    NLWdmClient * client = (__bridge NLWdmClient *) appReqState;

    NSMutableArray * statusResultsList = [NSMutableArray arrayWithCapacity:pathCount];
    NLGenericTraitUpdatableDataSink * dataSink = nil;
    WDM_LOG_DEBUG(@"Failed path Counts = %u\n", pathCount);

    for (uint32_t i = 0; i < pathCount; i++) {
        dataSink = [client getDataSink:(long long) statusResults[i].mpDataSink];
        if (dataSink == nil) {
            WDM_LOG_DEBUG(@"unexpected, trait %d does not exist in traitMap", i);
        }
        if (statusResults[i].mErrorCode == WEAVE_ERROR_STATUS_REPORT_RECEIVED) {
            NLWdmClientFlushUpdateDeviceStatusError * statusError = [[NLWdmClientFlushUpdateDeviceStatusError alloc]
                initWithProfileId:statusResults[i].mDevStatus.StatusProfileId
                       statusCode:statusResults[i].mDevStatus.StatusCode
                        errorCode:statusResults[i].mDevStatus.SystemErrorCode
                     statusReport:[client statusReportToString:statusResults[i].mDevStatus.StatusProfileId
                                                    statusCode:statusResults[i].mDevStatus.StatusCode]
                             path:[NSString stringWithUTF8String:statusResults[i].mpPath]
                         dataSink:dataSink];

            [statusResultsList addObject:statusError];
        } else {
            NLWdmClientFlushUpdateError * weaveError = [[NLWdmClientFlushUpdateError alloc]
                initWithWeaveError:statusResults[i].mErrorCode
                            report:[NSString stringWithUTF8String:nl::ErrorStr(statusResults[i].mErrorCode)]
                              path:[NSString stringWithUTF8String:statusResults[i].mpPath]
                          dataSink:dataSink];
            [statusResultsList addObject:weaveError];
        }
    }

    NSLog(@"statusResultsList is: %@", statusResultsList);
    [client dispatchAsyncCompletionBlockWithResults:statusResultsList];
}

- (void)markTransactionCompleted
{
    _mRequestName = nil;
    _mCompletionHandler = nil;
    _mFailureHandler = nil;
}

- (NSString *)getCurrentRequest
{
    return _mRequestName;
}

- (void)dispatchAsyncFailureBlock:(WEAVE_ERROR)code taskName:(NSString *)taskName handler:(WdmClientFailureBlock)handler
{
    NSError * error =
        [NSError errorWithDomain:@"com.nest.error"
                            code:code
                        userInfo:[NSDictionary dictionaryWithObjectsAndKeys:[NSString stringWithUTF8String:nl::ErrorStr(code)],
                                               @"error", nil]];
    [self dispatchAsyncFailureBlockWithError:error taskName:taskName handler:handler];
}

- (void)dispatchAsyncFailureBlockWithError:(NSError *)error taskName:(NSString *)taskName handler:(WdmClientFailureBlock)handler
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

- (void)dispatchAsyncDefaultFailureBlockWithCode:(WEAVE_ERROR)code
{
    NSError * error = [NSError errorWithDomain:@"com.nest.error" code:code userInfo:nil];
    [self dispatchAsyncDefaultFailureBlock:error];
}

- (void)dispatchAsyncDefaultFailureBlock:(NSError *)error
{
    NSString * taskName = _mRequestName;
    WdmClientFailureBlock failureHandler = _mFailureHandler;

    [self markTransactionCompleted];
    [self dispatchAsyncFailureBlockWithError:error taskName:taskName handler:failureHandler];
}

- (void)dispatchAsyncCompletionBlock:(id)data
{
    WdmClientCompletionBlock completionHandler = _mCompletionHandler;

    [self markTransactionCompleted];

    if (nil != completionHandler) {
        dispatch_async(_mAppCallbackQueue, ^() {
            completionHandler(_owner, data);
        });
    }
}

- (void)dispatchAsyncResponseBlock:(id)data
{
    WdmClientCompletionBlock completionHandler = _mCompletionHandler;

    if (nil != completionHandler) {
        dispatch_async(_mAppCallbackQueue, ^() {
            completionHandler(_owner, data);
        });
    }
}

- (void)dispatchAsyncCompletionBlockWithResults:(id)error
{
    WdmClientCompletionBlock completionHandler = _mCompletionHandler;

    [self markTransactionCompleted];

    if (nil != completionHandler) {
        dispatch_async(_mAppCallbackQueue, ^() {
            completionHandler(_owner, error);
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

- (void)shutdown_Internal
{
    WDM_LOG_METHOD_SIG();

    NSArray * keys = [_mTraitMap allKeys];

    for (NSString * key in keys) {
        NLGenericTraitUpdatableDataSink * pDataSink = [_mTraitMap objectForKey:key];
        NSLog(@"key=%@, pDataSink=%@", key, pDataSink);
        if ((NSNull *) pDataSink != [NSNull null]) {
            [pDataSink close];
        }
    }
    [_mTraitMap removeAllObjects];
    _mTraitMap = nil;

    // there is no need to release Objective C objects, as we have ARC for them
    if (_mWeaveCppWdmClient) {
        WDM_LOG_ERROR(@"Shutdown C++ Weave WdmClient");

        _mWeaveCppWdmClient->Close();

        delete _mWeaveCppWdmClient;
        _mWeaveCppWdmClient = NULL;
    }

    [self dispatchAsyncCompletionBlock:nil];
}

- (void)close:(WdmClientCompletionBlock)completionHandler
{
    WDM_LOG_METHOD_SIG();

    dispatch_sync(_mWeaveWorkQueue, ^() {
        if (nil != _mRequestName) {
            WDM_LOG_ERROR(@"%@: Forcefully close while we're still executing %@, continue close", _name, _mRequestName);
        }

        [self markTransactionCompleted];

        _mCompletionHandler = [completionHandler copy];
        _mRequestName = @"close";
        [self shutdown_Internal];
    });
}

- (void)removeDataSinkRef:(long long)traitInstancePtr;
{
    NSString * address = [NSString stringWithFormat:@"%lld", traitInstancePtr];

    if ([_mTraitMap objectForKey:address] != nil) {
        _mTraitMap[address] = [NSNull null];
    }
}

- (NLGenericTraitUpdatableDataSink *)getDataSink:(long long)traitInstancePtr;
{
    NSString * address = [NSString stringWithFormat:@"%lld", traitInstancePtr];
    return (NLGenericTraitUpdatableDataSink *)[_mTraitMap objectForKey:address];
}

- (void)setNodeId:(uint64_t)nodeId;
{
    WDM_LOG_METHOD_SIG();

    // VerifyOrExit(NULL != _mWeaveCppWdmClient, err = WEAVE_ERROR_INCORRECT_STATE);

    dispatch_sync(_mWeaveWorkQueue, ^() {
        _mWeaveCppWdmClient->SetNodeId(nodeId);
    });
}

- (NLGenericTraitUpdatableDataSink *)newDataSink:(NLResourceIdentifier *)nlResourceIdentifier
                                       profileId:(uint32_t)profileId
                                      instanceId:(uint64_t)instanceId
                                            path:(NSString *)path;
{
    __block WEAVE_ERROR err = WEAVE_NO_ERROR;

    __block nl::Weave::DeviceManager::GenericTraitUpdatableDataSink * pDataSink = NULL;
    WDM_LOG_METHOD_SIG();

    // VerifyOrExit(NULL != _mWeaveCppWdmClient, err = WEAVE_ERROR_INCORRECT_STATE);

    dispatch_sync(_mWeaveWorkQueue, ^() {
        ResourceIdentifier resId = [nlResourceIdentifier toResourceIdentifier];
        err = _mWeaveCppWdmClient->NewDataSink(resId, profileId, instanceId, [path UTF8String], pDataSink);
    });

    if (err != WEAVE_NO_ERROR || pDataSink == NULL) {
        WDM_LOG_ERROR(@"pDataSink is not ready");
        return nil;
    }

    NSString * address = [NSString stringWithFormat:@"%lld", (long long) pDataSink];

    if ([_mTraitMap objectForKey:address] == nil) {
        WDM_LOG_DEBUG(@"creating new NLGenericTraitUpdatableDataSink with profild id %d", profileId);
        NLGenericTraitUpdatableDataSink * pTrait = [[NLGenericTraitUpdatableDataSink alloc] init:_name
                                                                                  weaveWorkQueue:_mWeaveWorkQueue
                                                                                appCallbackQueue:_mAppCallbackQueue
                                                                genericTraitUpdatableDataSinkPtr:pDataSink
                                                                                     nlWdmClient:self];
        [_mTraitMap setObject:pTrait forKey:address];
    }

    return [_mTraitMap objectForKey:address];
}

- (void)flushUpdate:(WdmClientCompletionBlock)completionHandler failure:(WdmClientFailureBlock)failureHandler
{
    WDM_LOG_METHOD_SIG();

    NSString * taskName = @"FlushUpdate";

    // we use async for the results are sent back to caller via async means also
    dispatch_async(_mWeaveWorkQueue, ^() {
        if (nil == _mRequestName) {
            _mRequestName = taskName;
            _mCompletionHandler = [completionHandler copy];
            _mFailureHandler = [failureHandler copy];

            WEAVE_ERROR err
                = _mWeaveCppWdmClient->FlushUpdate((__bridge void *) self, onWdmClientFlushUpdateComplete, onWdmClientError);

            if (WEAVE_NO_ERROR != err) {
                [self dispatchAsyncDefaultFailureBlockWithCode:err];
            }
        } else {
            WDM_LOG_ERROR(@"%@: Attemp to %@ while we're still executing %@, ignore", _name, taskName, _mRequestName);

            // do not change _mRequestName, as we're rejecting this request
            [self dispatchAsyncFailureBlock:WEAVE_ERROR_INCORRECT_STATE taskName:taskName handler:failureHandler];
        }
    });
}

- (void)refreshData:(WdmClientCompletionBlock)completionHandler failure:(WdmClientFailureBlock)failureHandler
{
    WDM_LOG_METHOD_SIG();

    NSString * taskName = @"RefreshData";

    // we use async for the results are sent back to caller via async means also
    dispatch_async(_mWeaveWorkQueue, ^() {
        if (nil == _mRequestName) {
            _mRequestName = taskName;
            _mCompletionHandler = [completionHandler copy];
            _mFailureHandler = [failureHandler copy];

            WEAVE_ERROR err = _mWeaveCppWdmClient->RefreshData((__bridge void *) self, onWdmClientComplete, onWdmClientError, NULL);

            if (WEAVE_NO_ERROR != err) {
                [self dispatchAsyncDefaultFailureBlockWithCode:err];
            }
        } else {
            WDM_LOG_ERROR(@"%@: Attemp to %@ while we're still executing %@, ignore", _name, taskName, _mRequestName);

            // do not change _mRequestName, as we're rejecting this request
            [self dispatchAsyncFailureBlock:WEAVE_ERROR_INCORRECT_STATE taskName:taskName handler:failureHandler];
        }
    });
}

@end
#endif // WEAVE_CONFIG_DATA_MANAGEMENT_CLIENT_EXPERIMENTAL
