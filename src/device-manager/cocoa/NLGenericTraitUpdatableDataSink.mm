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
 *      This file implements NLGenericTraitUpdatableDataSink interface
 *
 */

#import <Foundation/Foundation.h>
#import "NLWeaveStack.h"
#import "NLLogging.h"
#import "NLProfileStatusError.h"
#import "NLWeaveError_Protected.h"

#include <Weave/Core/WeaveError.h>
#include <Weave/Support/CodeUtils.h>
#include <WeaveDeviceManager.h>
#include <Weave/Core/WeaveCore.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Profiles/common/CommonProfile.h>
#include <Weave/Profiles/data-management/Current/WdmManagedNamespace.h>
#include <WeaveDataManagementClient.h>
#include <Weave/Support/NLDLLUtil.h>
#include <Weave/Core/WeaveCore.h>
#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Profiles/status-report/StatusReportProfile.h>
#include <SystemLayer/SystemPacketBuffer.h>
#include <Weave/Profiles/data-management/SubscriptionClient.h>

#import "NLWdmClient_Protected.h"
#import "NLGenericTraitUpdatableDataSink_Protected.h"
#include <iostream>
#include <vector>

using namespace nl::Weave::Profiles;
using namespace nl::Weave::Profiles::DataManagement;

@interface NLGenericTraitUpdatableDataSink () {
    nl::Weave::DeviceManager::GenericTraitUpdatableDataSink * _mWeaveCppGenericTraitUpdatableDataSink;
    NLWdmClient * _mNLWdmClient;
    dispatch_queue_t _mWeaveWorkQueue;
    dispatch_queue_t _mAppCallbackQueue;

    // Note that these context variables are independent from context variables in the C++ Weave Device Manager,
    // for the C++ Weave Device Manager only takes one pointer as the app context, which is not enough to hold all
    // context information we need.
    GenericTraitUpdatableDataSinkCompletionBlock _mCompletionHandler;
    GenericTraitUpdatableDataSinkFailureBlock _mFailureHandler;
    NSString * _mRequestName;
}

- (NSString *)GetCurrentRequest;
@end

@implementation NLGenericTraitUpdatableDataSink
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
    // called at random queues (most probably from UI, when the app de-reference this GenericTraitUpdatableDataSink)
    dispatch_sync(_mWeaveWorkQueue, ^() {
        [self shutdown_Internal];
    });
}

- (instancetype)init:(NSString *)name
      weaveWorkQueue:(dispatch_queue_t)weaveWorkQueue
    appCallbackQueue:(dispatch_queue_t)appCallbackQueue
    genericTraitUpdatableDataSinkPtr: (nl::Weave::DeviceManager::GenericTraitUpdatableDataSink *)dataSinkPtr
         nlWdmClient:(NLWdmClient *)nlWdmClient
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    WDM_LOG_METHOD_SIG();

    self = [super init];
    VerifyOrExit(nil != self, err = WEAVE_ERROR_NO_MEMORY);

    _mWeaveWorkQueue = weaveWorkQueue;
    _mAppCallbackQueue = appCallbackQueue;

    _name = name;
    _mWeaveCppGenericTraitUpdatableDataSink = dataSinkPtr;
    _mNLWdmClient = nlWdmClient;

    // simple bridge shall not increase our reference count
    _mWeaveCppGenericTraitUpdatableDataSink->mpAppState = (__bridge void *) self;

    [self markTransactionCompleted];

exit:
    id result = nil;
    if (WEAVE_NO_ERROR == err) {
        result = self;
    } else {
        WDM_LOG_ERROR(@"Error in init : (%d) %@\n", err, [NSString stringWithUTF8String:nl::ErrorStr(err)]);
        [self shutdown_Internal];
    }

    return result;
}

static void handleGenericUpdatableDataSinkComplete(void * dataSink, void * reqState)
{
    WDM_LOG_DEBUG(@"handleGenericUpdatableDataSinkComplete");

    NLGenericTraitUpdatableDataSink * sink = (__bridge NLGenericTraitUpdatableDataSink *) reqState;
    [sink DispatchAsyncCompletionBlock:nil];
}

static void handleGenericUpdatableDataSinkError(void * dataSink, void * appReqState, WEAVE_ERROR code,
    nl::Weave::DeviceManager::DeviceStatus * devStatus)
{
    WDM_LOG_DEBUG(@"handleGenericUpdatableDataSinkError");

    NSError * error = nil;
    NSDictionary * userInfo = nil;

    NLGenericTraitUpdatableDataSink * nlDataSink = (__bridge NLGenericTraitUpdatableDataSink *) appReqState;
    // ignore the pointer to C++ device manager
    (void) dataSink;

    WDM_LOG_DEBUG(@"%@: Received error response to request %@, wdmClientErr = %d, devStatus = %p\n", nlDataSink.name,
        [nlDataSink GetCurrentRequest], code, devStatus);

    NLWeaveRequestError requestError;
    if (devStatus != NULL && code == WEAVE_ERROR_STATUS_REPORT_RECEIVED) {
        NLProfileStatusError * statusError = [[NLProfileStatusError alloc]
            initWithProfileId:devStatus->StatusProfileId
                   statusCode:devStatus->StatusCode
                    errorCode:devStatus->SystemErrorCode
                 statusReport:[nlDataSink statusReportToString:devStatus->StatusProfileId statusCode:devStatus->StatusCode]];
        requestError = NLWeaveRequestError_ProfileStatusError;
        userInfo = @{ @"WeaveRequestErrorType" : @(requestError), @"errorInfo" : statusError };

        WDM_LOG_DEBUG(@"%@: status error: %@", nlDataSink.name, userInfo);
    } else {
        NLWeaveError * weaveError = [[NLWeaveError alloc] initWithWeaveError:code
                                                                      report:[NSString stringWithUTF8String:nl::ErrorStr(code)]];
        requestError = NLWeaveRequestError_WeaveError;
        userInfo = @{ @"WeaveRequestErrorType" : @(requestError), @"errorInfo" : weaveError };
    }

    error = [NSError errorWithDomain:@"com.nest.error" code:code userInfo:userInfo];

    [nlDataSink DispatchAsyncDefaultFailureBlock:error];
}

- (void)markTransactionCompleted
{
    _mRequestName = nil;
    _mCompletionHandler = nil;
    _mFailureHandler = nil;
}

- (NSString *)GetCurrentRequest
{
    return _mRequestName;
}

- (void)DispatchAsyncFailureBlock:(WEAVE_ERROR)code taskName:(NSString *)taskName handler:(GenericTraitUpdatableDataSinkFailureBlock)handler
{
    NSError * error =
        [NSError errorWithDomain:@"com.nest.error"
                            code:code
                        userInfo:[NSDictionary dictionaryWithObjectsAndKeys:[NSString stringWithUTF8String:nl::ErrorStr(code)],
                                               @"error", nil]];
    [self DispatchAsyncFailureBlockWithError:error taskName:taskName handler:handler];
}

- (void)DispatchAsyncFailureBlockWithError:(NSError *)error taskName:(NSString *)taskName handler:(GenericTraitUpdatableDataSinkFailureBlock)handler
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
    GenericTraitUpdatableDataSinkFailureBlock failureHandler = _mFailureHandler;

    [self markTransactionCompleted];
    [self DispatchAsyncFailureBlockWithError:error taskName:taskName handler:failureHandler];
}

- (void)DispatchAsyncCompletionBlock:(id)data
{
    GenericTraitUpdatableDataSinkCompletionBlock completionHandler = _mCompletionHandler;

    [self markTransactionCompleted];

    if (nil != completionHandler) {
        dispatch_async(_mAppCallbackQueue, ^() {
            completionHandler(_owner, data);
        });
    }
}

- (void)DispatchAsyncResponseBlock:(id)data
{
    GenericTraitUpdatableDataSinkCompletionBlock completionHandler = _mCompletionHandler;

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

- (void)shutdown_Internal
{
    WDM_LOG_METHOD_SIG();

    if (_mNLWdmClient != nil)
    {
        [_mNLWdmClient removeDataSinkRef:(long long)_mWeaveCppGenericTraitUpdatableDataSink ];
        _mNLWdmClient = nil;
    }

    if (_mWeaveCppGenericTraitUpdatableDataSink != nil)
    {
        _mWeaveCppGenericTraitUpdatableDataSink->Clear();
    }
}

- (void)close
{
    WDM_LOG_METHOD_SIG();
    [self markTransactionCompleted];
    [self shutdown_Internal];
}

- (void)clear
{
    __block WEAVE_ERROR err = WEAVE_NO_ERROR;
    
    WDM_LOG_METHOD_SIG();
    
    VerifyOrExit(NULL !=_mWeaveCppGenericTraitUpdatableDataSink, err = WEAVE_ERROR_INCORRECT_STATE);
    
    // need this bracket to use Verify macros
    {
        // we use sync so the result is immediately available to the caller upon return
        dispatch_sync(_mWeaveWorkQueue, ^() {
            _mWeaveCppGenericTraitUpdatableDataSink->Clear();
        });
    }
exit:
    return;
}

- (void)refreshData:(GenericTraitUpdatableDataSinkCompletionBlock)completionHandler      failure:(GenericTraitUpdatableDataSinkFailureBlock)failureHandler
{
    WDM_LOG_METHOD_SIG();

    NSString * taskName = @"RefreshData";

    // we use async for the results are sent back to caller via async means also
    dispatch_async(_mWeaveWorkQueue, ^() {
        if (nil == _mRequestName) {
            _mRequestName = taskName;
            _mCompletionHandler = [completionHandler copy];
            _mFailureHandler = [failureHandler copy];

            WEAVE_ERROR err = _mWeaveCppGenericTraitUpdatableDataSink->RefreshData((__bridge void *) self, handleGenericUpdatableDataSinkComplete, handleGenericUpdatableDataSinkError);

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

- (WEAVE_ERROR)setSigned:(int64_t)val
                    path:(NSString *)path
             conditional:(BOOL) isConditional
{
    __block WEAVE_ERROR err = WEAVE_NO_ERROR;
    __block bool isConditionalBool = isConditional == YES ? true : false;

    WDM_LOG_METHOD_SIG();

    VerifyOrExit(NULL !=_mWeaveCppGenericTraitUpdatableDataSink, err = WEAVE_ERROR_INCORRECT_STATE);

    // need this bracket to use Verify macros
    {
        // we use sync so the result is immediately available to the caller upon return
        dispatch_sync(_mWeaveWorkQueue, ^() {
            err = _mWeaveCppGenericTraitUpdatableDataSink->SetData([path UTF8String], val, isConditionalBool);

            if (err == WEAVE_ERROR_INCORRECT_STATE) {
                WDM_LOG_DEBUG(@"Got incorrect state error from SetSigned, ignore");
                err = WEAVE_NO_ERROR; // No exception, just return 0.
            }
        });
    }

exit:
    return err;
}

- (WEAVE_ERROR)setUnsigned:(uint64_t)val
                      path:(NSString *)path
               conditional:(BOOL) isConditional
{
    __block WEAVE_ERROR err = WEAVE_NO_ERROR;
    __block bool isConditionalBool = isConditional == YES ? true : false;

    WDM_LOG_METHOD_SIG();

    VerifyOrExit(NULL !=_mWeaveCppGenericTraitUpdatableDataSink, err = WEAVE_ERROR_INCORRECT_STATE);

    // need this bracket to use Verify macros
    {
        // we use sync so the result is immediately available to the caller upon return
        dispatch_sync(_mWeaveWorkQueue, ^() {
            err = _mWeaveCppGenericTraitUpdatableDataSink->SetData([path UTF8String], val, isConditionalBool);

            if (err == WEAVE_ERROR_INCORRECT_STATE) {
                WDM_LOG_DEBUG(@"Got incorrect state error from SetUnsigned, ignore");

                err = WEAVE_NO_ERROR; // No exception, just return 0.
            }
        });
    }

exit:
    return err;
}

- (WEAVE_ERROR)setDouble:(double)val
                    path:(NSString *)path
             conditional:(BOOL) isConditional
{
    __block WEAVE_ERROR err = WEAVE_NO_ERROR;
    __block bool isConditionalBool = isConditional == YES ? true : false;

    WDM_LOG_METHOD_SIG();

    VerifyOrExit(NULL !=_mWeaveCppGenericTraitUpdatableDataSink, err = WEAVE_ERROR_INCORRECT_STATE);

    // need this bracket to use Verify macros
    {
        // we use sync so the result is immediately available to the caller upon return
        dispatch_sync(_mWeaveWorkQueue, ^() {
            err = _mWeaveCppGenericTraitUpdatableDataSink->SetData([path UTF8String], val, isConditionalBool);

            if (err == WEAVE_ERROR_INCORRECT_STATE) {
                WDM_LOG_DEBUG(@"Got incorrect state error from SetDouble, ignore");
                err = WEAVE_NO_ERROR; // No exception, just return 0.
            }
        });
    }

exit:
    return err;
}

- (WEAVE_ERROR)setBoolean:(BOOL) val
                     path:(NSString *)path
              conditional:(BOOL) isConditional
{
    __block WEAVE_ERROR err = WEAVE_NO_ERROR;
    __block bool isConditionalBool = isConditional == YES ? true : false;
    __block bool valBool= val == YES ? true : false;
    WDM_LOG_METHOD_SIG();

    VerifyOrExit(NULL !=_mWeaveCppGenericTraitUpdatableDataSink, err = WEAVE_ERROR_INCORRECT_STATE);

    // need this bracket to use Verify macros
    {
        // we use sync so the result is immediately available to the caller upon return
        dispatch_sync(_mWeaveWorkQueue, ^() {
            err = _mWeaveCppGenericTraitUpdatableDataSink->SetBoolean([path UTF8String], valBool, isConditionalBool);
            if (err == WEAVE_ERROR_INCORRECT_STATE) {
                WDM_LOG_DEBUG(@"Got incorrect state error from SetBoolean, ignore");
                err = WEAVE_NO_ERROR; // No exception, just return 0.
            }
        });
    }

exit:
    return err;
}

- (WEAVE_ERROR)setString:(NSString *) val
                    path:(NSString *) path
             conditional:(BOOL) isConditional
{
    __block WEAVE_ERROR err = WEAVE_NO_ERROR;
    __block bool isConditionalBool = (isConditional == YES ? true : false);
    WDM_LOG_METHOD_SIG();

    VerifyOrExit(NULL != _mWeaveCppGenericTraitUpdatableDataSink, err = WEAVE_ERROR_INCORRECT_STATE);

    // need this bracket to use Verify macros
    {
        // we use sync so the result is immediately available to the caller upon return
        dispatch_sync(_mWeaveWorkQueue, ^() {
            err = _mWeaveCppGenericTraitUpdatableDataSink->SetString([path UTF8String], [val UTF8String], isConditionalBool);

            if (err == WEAVE_ERROR_INCORRECT_STATE) {
                WDM_LOG_DEBUG(@"Got incorrect state error from SetData, ignore");
                err = WEAVE_NO_ERROR; // No exception, just return 0.
            }
        });
    }

exit:
    return err;
}

- (WEAVE_ERROR)setNull:(NSString *)path
           conditional:(BOOL) isConditional
{
    __block WEAVE_ERROR err = WEAVE_NO_ERROR;
    __block bool isConditionalBool = isConditional == YES ? true : false;

    WDM_LOG_METHOD_SIG();

    VerifyOrExit(NULL !=_mWeaveCppGenericTraitUpdatableDataSink, err = WEAVE_ERROR_INCORRECT_STATE);

    // need this bracket to use Verify macros
    {
        // we use sync so the result is immediately available to the caller upon return
        dispatch_sync(_mWeaveWorkQueue, ^() {
            err = _mWeaveCppGenericTraitUpdatableDataSink->SetNull([path UTF8String], isConditionalBool);

            if (err == WEAVE_ERROR_INCORRECT_STATE) {
                WDM_LOG_DEBUG(@"Got incorrect state error from SetData, ignore");
                err = WEAVE_NO_ERROR; // No exception, just return 0.
            }
        });
    }

exit:
    return err;
}

- (WEAVE_ERROR)setBytes:(NSData *) val
                   path:(NSString *) path
            conditional:(BOOL) isConditional
{
    __block WEAVE_ERROR err = WEAVE_NO_ERROR;
    __block bool isConditionalBool= isConditional == YES ? true : false;

    WDM_LOG_METHOD_SIG();

    VerifyOrExit(NULL !=_mWeaveCppGenericTraitUpdatableDataSink, err = WEAVE_ERROR_INCORRECT_STATE);

    // need this bracket to use Verify macros
    {
        // we use sync so the result is immediately available to the caller upon return
        dispatch_sync(_mWeaveWorkQueue, ^() {

            uint32_t valLen = (uint32_t)[val length];
            uint8_t * pVal = (uint8_t *) [val bytes];
            err = _mWeaveCppGenericTraitUpdatableDataSink->SetBytes([path UTF8String], pVal, valLen, isConditionalBool);

            if (err == WEAVE_ERROR_INCORRECT_STATE) {
                WDM_LOG_DEBUG(@"Got incorrect state error from SetBytes, ignore");
                err = WEAVE_NO_ERROR; // No exception, just return 0.
            }
        });
    }

exit:
    return err;
}

- (WEAVE_ERROR)setStringArray:(NSArray*)stringArray
                         path:(NSString *) path
                  conditional:(BOOL) isConditional
{
    __block WEAVE_ERROR err = WEAVE_NO_ERROR;
    __block bool isConditionalBool= isConditional == YES ? true : false;

    std::vector<std::string> stringVector;

    WDM_LOG_METHOD_SIG();

    VerifyOrExit(NULL !=_mWeaveCppGenericTraitUpdatableDataSink, err = WEAVE_ERROR_INCORRECT_STATE);

    for (id object in stringArray)
    {
        stringVector.push_back(std::string([object UTF8String]));
    }
    // need this bracket to use Verify macros
    {
        // we use sync so the result is immediately available to the caller upon return
        dispatch_sync(_mWeaveWorkQueue, ^() {
            err = _mWeaveCppGenericTraitUpdatableDataSink->SetStringArray([path UTF8String], stringVector, isConditionalBool);
            if (err == WEAVE_ERROR_INCORRECT_STATE) {
                WDM_LOG_DEBUG(@"Got incorrect state error from SetStringArray, ignore");
                err = WEAVE_NO_ERROR; // No exception, just return 0.
            }
        });
    }

exit:
    return err;
}

- (WEAVE_ERROR)setSigned:(int64_t)val
                    path:(NSString *)path
{
    return [self setSigned:val path:path conditional: NO];
}

- (WEAVE_ERROR)setUnsigned:(uint64_t)val
                      path:(NSString *)path

{
    return [self setUnsigned:val path:path conditional: NO];
}

- (WEAVE_ERROR)setDouble:(double)val
                    path:(NSString *)path

{
    return [self setDouble:val path:path conditional: NO];
}

- (WEAVE_ERROR)setBoolean:(BOOL) val
                     path:(NSString *)path
{
    return [self setBoolean:val path:path conditional: NO];
}

- (WEAVE_ERROR)setString:(NSString *) val
                    path:(NSString *) path
{
    return [self setString:val path:path conditional: NO];
}

- (WEAVE_ERROR)setNull:(NSString *)path
{
    return [self setNull:path conditional: NO];
}

- (WEAVE_ERROR)setBytes:(NSData *) val
                   path:(NSString *) path
{
    return [self setBytes:val path:path conditional: NO];
}

- (WEAVE_ERROR)setStringArray:(NSArray*)stringArray
                         path:(NSString *) path
{
    return [self setStringArray:stringArray path:path conditional: NO];
}

- (WEAVE_ERROR)getSigned:(int64_t *)val
                    path:(NSString *)path
{
    __block WEAVE_ERROR err = WEAVE_NO_ERROR;
    __block int64_t result = 0;

    WDM_LOG_METHOD_SIG();

    VerifyOrExit(NULL !=_mWeaveCppGenericTraitUpdatableDataSink, err = WEAVE_ERROR_INCORRECT_STATE);
    VerifyOrExit(NULL != val, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // need this bracket to use Verify macros
    {
        // we use sync so the result is immediately available to the caller upon return
        dispatch_sync(_mWeaveWorkQueue, ^() {
            err = _mWeaveCppGenericTraitUpdatableDataSink->GetData([path UTF8String], result);

            if (err == WEAVE_ERROR_INCORRECT_STATE) {
                WDM_LOG_DEBUG(@"Got incorrect state error from GetInt, ignore");

                err = WEAVE_NO_ERROR; // No exception, just return 0.
                result = 0;
            }
        });
    }

exit:
    if ((WEAVE_NO_ERROR == err) && (NULL != val)) {
        *val = result;
    }

    return err;
}

- (WEAVE_ERROR)getUnsigned:(uint64_t *)val
                      path:(NSString *)path
{
    __block WEAVE_ERROR err = WEAVE_NO_ERROR;
    __block uint64_t result = 0;

    WDM_LOG_METHOD_SIG();

    VerifyOrExit(NULL !=_mWeaveCppGenericTraitUpdatableDataSink, err = WEAVE_ERROR_INCORRECT_STATE);
    VerifyOrExit(NULL != val, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // need this bracket to use Verify macros
    {
        // we use sync so the result is immediately available to the caller upon return
        dispatch_sync(_mWeaveWorkQueue, ^() {
            err = _mWeaveCppGenericTraitUpdatableDataSink->GetData([path UTF8String], result);

            if (err == WEAVE_ERROR_INCORRECT_STATE) {
                WDM_LOG_DEBUG(@"Got incorrect state error from GetInt, ignore");

                err = WEAVE_NO_ERROR; // No exception, just return 0.
                result = 0;
            }
        });
    }

exit:
    if ((WEAVE_NO_ERROR == err) && (NULL != val)) {
        *val = result;
    }

    return err;
}

- (WEAVE_ERROR)getDouble:(double *)val
                    path:(NSString *)path
{
    __block WEAVE_ERROR err = WEAVE_NO_ERROR;
    __block double result = 0;

    WDM_LOG_METHOD_SIG();

    VerifyOrExit(NULL !=_mWeaveCppGenericTraitUpdatableDataSink, err = WEAVE_ERROR_INCORRECT_STATE);
    VerifyOrExit(NULL != val, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // need this bracket to use Verify macros
    {
        // we use sync so the result is immediately available to the caller upon return
        dispatch_sync(_mWeaveWorkQueue, ^() {
            err = _mWeaveCppGenericTraitUpdatableDataSink->GetData([path UTF8String], result);

            if (err == WEAVE_ERROR_INCORRECT_STATE) {
                WDM_LOG_DEBUG(@"Got incorrect state error from GetInt, ignore");

                err = WEAVE_NO_ERROR; // No exception, just return 0.
                result = 0;
            }
        });
    }

exit:
    if ((WEAVE_NO_ERROR == err) && (NULL != val)) {
        *val = result;
    }

    return err;
}

- (WEAVE_ERROR)getBoolean:(BOOL *)val
                     path:(NSString *)path
{
    __block WEAVE_ERROR err = WEAVE_NO_ERROR;
    __block bool resultConverted = false;

    WDM_LOG_METHOD_SIG();

    VerifyOrExit(NULL !=_mWeaveCppGenericTraitUpdatableDataSink, err = WEAVE_ERROR_INCORRECT_STATE);
    VerifyOrExit(NULL != val, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // need this bracket to use Verify macros
    {
        // we use sync so the result is immediately available to the caller upon return
        dispatch_sync(_mWeaveWorkQueue, ^() {
            err = _mWeaveCppGenericTraitUpdatableDataSink->GetBoolean([path UTF8String], resultConverted);

            if (err == WEAVE_ERROR_INCORRECT_STATE) {
                WDM_LOG_DEBUG(@"Got incorrect state error from GetInt, ignore");

                err = WEAVE_NO_ERROR; // No exception, just return 0.
            }
        });
    }

exit:
    if ((WEAVE_NO_ERROR == err) && (NULL != val)) {
        *val = (resultConverted == true ? YES: NO);
    }

    return err;
}

- (NSString *)getString:(NSString *)path
{
    __block WEAVE_ERROR err = WEAVE_NO_ERROR;
    __block  nl::Weave::DeviceManager::BytesData bytesData;
    NSString * result = nil;
    WDM_LOG_METHOD_SIG();

    VerifyOrExit(NULL !=_mWeaveCppGenericTraitUpdatableDataSink, err = WEAVE_ERROR_INCORRECT_STATE);

    // need this bracket to use Verify macros
    {
        // we use sync so the result is immediately available to the caller upon return
        dispatch_sync(_mWeaveWorkQueue, ^() {

            err = _mWeaveCppGenericTraitUpdatableDataSink->GetBytes([path UTF8String], &bytesData);

            if (err == WEAVE_ERROR_INCORRECT_STATE) {
                WDM_LOG_DEBUG(@"Got incorrect state error from GetString, ignore");

                err = WEAVE_NO_ERROR; // No exception, just return 0.
            }
        });
    }
    result = [[NSString alloc]initWithBytes:bytesData.mpDataBuf length:bytesData.mDataLen encoding:NSUTF8StringEncoding];

exit:
    return result;
}

- (NSData *)getBytes:(NSString *)path
{
    __block WEAVE_ERROR err = WEAVE_NO_ERROR;
    __block  nl::Weave::DeviceManager::BytesData bytesData;
    NSData * result = nil;
    WDM_LOG_METHOD_SIG();

    VerifyOrExit(NULL !=_mWeaveCppGenericTraitUpdatableDataSink, err = WEAVE_ERROR_INCORRECT_STATE);

    // need this bracket to use Verify macros
    {
        // we use sync so the result is immediately available to the caller upon return
        dispatch_sync(_mWeaveWorkQueue, ^() {

            err = _mWeaveCppGenericTraitUpdatableDataSink->GetBytes([path UTF8String], &bytesData);

            if (err == WEAVE_ERROR_INCORRECT_STATE) {
                WDM_LOG_DEBUG(@"Got incorrect state error from GetBytes, ignore");

                err = WEAVE_NO_ERROR; // No exception, just return 0.
            }
        });
    }

    result = [NSData dataWithBytes:bytesData.mpDataBuf length:bytesData.mDataLen];
exit:
    return result;
}

- (NSArray *)getStringArray:(NSString *)path
{
    __block WEAVE_ERROR err = WEAVE_NO_ERROR;
    __block std::vector<std::string> stringVector;
    NSMutableArray *arrayOut = [[NSMutableArray alloc] init];

    WDM_LOG_METHOD_SIG();

    VerifyOrExit(NULL !=_mWeaveCppGenericTraitUpdatableDataSink, err = WEAVE_ERROR_INCORRECT_STATE);

    // need this bracket to use Verify macros
    {
        // we use sync so the result is immediately available to the caller upon return
        dispatch_sync(_mWeaveWorkQueue, ^() {
            err = _mWeaveCppGenericTraitUpdatableDataSink->GetStringArray([path UTF8String], stringVector);

            if (err == WEAVE_ERROR_INCORRECT_STATE) {
                WDM_LOG_DEBUG(@"Got incorrect state error from GetStringArray, ignore");

                err = WEAVE_NO_ERROR; // No exception, just return 0.
            }
        });
    }

    for (auto iter = stringVector.begin(); iter < stringVector.end(); iter++)
    {
        [arrayOut addObject:[NSString stringWithCString:iter->c_str() encoding:[NSString defaultCStringEncoding]]];
    }

exit:
    return arrayOut;
}

- (WEAVE_ERROR)getVersion:(uint64_t *)val
{
    __block WEAVE_ERROR err = WEAVE_NO_ERROR;
    __block uint64_t result = 0;

    WDM_LOG_METHOD_SIG();

    VerifyOrExit(NULL !=_mWeaveCppGenericTraitUpdatableDataSink, err = WEAVE_ERROR_INCORRECT_STATE);
    VerifyOrExit(NULL != val, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // need this bracket to use Verify macros
    {
        // we use sync so the result is immediately available to the caller upon return
        dispatch_sync(_mWeaveWorkQueue, ^() {
            result = _mWeaveCppGenericTraitUpdatableDataSink->GetVersion();

            if (err == WEAVE_ERROR_INCORRECT_STATE) {
                WDM_LOG_DEBUG(@"Got incorrect state error from GetInt, ignore");

                err = WEAVE_NO_ERROR; // No exception, just return 0.
                result = 0;
            }
        });
    }

exit:
    if ((WEAVE_NO_ERROR == err) && (NULL != val)) {
        *val = result;
    }

    return err;
}

@end
