/*
 *
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
 *      This file implements NLWeaveDeviceManager interface
 *
 */

#import <Foundation/Foundation.h>
#import "NLWeaveStack.h"
#import "NLLogging.h"

#include <Weave/Core/WeaveError.h>

#include <Weave/Support/CodeUtils.h>

#include <WeaveDeviceManager.h>
#include <WeaveDataManagementClient.h>

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

static void onIdentifyDeviceComplete(nl::Weave::DeviceManager::WeaveDeviceManager * deviceMgr, void * appReqState,
    const nl::Weave::DeviceManager::DeviceDescription::WeaveDeviceDescriptor * devdesc);

@interface NLWeaveDeviceManager () {
    nl::Weave::DeviceManager::WeaveDeviceManager * _mWeaveCppDM;
    dispatch_queue_t _mWeaveWorkQueue;
    dispatch_queue_t _mAppCallbackQueue;

    // Note that these context variables are independent from context variables in the C++ Weave Device Manager,
    // for the C++ Weave Device Manager only takes one pointer as the app context, which is not enough to hold all
    // context information we need.
    WDMCompletionBlock _mCompletionHandler;
    WDMFailureBlock _mFailureHandler;
    NSString * _mRequestName;

    CBPeripheral * _blePeripheral;
}

- (NSString *)GetCurrentRequest;

@end

@implementation NLWeaveDeviceManager

@synthesize blePeripheral = _blePeripheral;
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
    // called at random queues (most probably from UI, when the app de-reference this device manager)
    dispatch_sync(_mWeaveWorkQueue, ^() {
        [self Shutdown_Internal];
    });
}

- (instancetype)init:(NSString *)name
      weaveWorkQueue:(dispatch_queue_t)weaveWorkQueue
    appCallbackQueue:(dispatch_queue_t)appCallbackQueue
         exchangeMgr:(nl::Weave::WeaveExchangeManager *)exchangeMgr
         securityMgr:(nl::Weave::WeaveSecurityManager *)securityMgr
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    WDM_LOG_METHOD_SIG();

    self = [super init];
    VerifyOrExit(nil != self, err = WEAVE_ERROR_NO_MEMORY);

    _mWeaveWorkQueue = weaveWorkQueue;
    _mAppCallbackQueue = appCallbackQueue;

    _name = name;

    _mWeaveCppDM = new nl::Weave::DeviceManager::WeaveDeviceManager();
    VerifyOrExit(NULL != _mWeaveCppDM, err = WEAVE_ERROR_NO_MEMORY);

    err = _mWeaveCppDM->Init(exchangeMgr, securityMgr);
    SuccessOrExit(err);

    _mWeaveCppDM->SetConnectTimeout(60000);

    // simple bridge shall not increase our reference count
    _mWeaveCppDM->AppState = (__bridge void *) self;

    [self MarkTransactionCompleted];

exit:
    id result = nil;
    if (WEAVE_NO_ERROR == err) {
        result = self;
    } else {
        WDM_LOG_ERROR(@"Error in init : (%d) %@\n", err, [NSString stringWithUTF8String:nl::ErrorStr(err)]);

        [self Shutdown_Internal];
    }
    return result;
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

    // there is no need to release Objective C objects, as we have ARC for them
    if (_mWeaveCppDM) {
        WDM_LOG_ERROR(@"Shutdown C++ Weave Device Manager");

        if (_blePeripheral) {
            // this is a hack to avoid further callback from BleLayer after closign
            [[[NLWeaveStack sharedStack] BleDelegate] forceBleDisconnect_Sync:_blePeripheral];
        }

        _mWeaveCppDM->Shutdown();

        delete _mWeaveCppDM;
        _mWeaveCppDM = NULL;
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
            if (_blePeripheral) {
                // this is a hack to avoid further callback from BleLayer after closign
                [[[NLWeaveStack sharedStack] BleDelegate] forceBleDisconnect_Sync:_blePeripheral];

                // release reference to the CBPeripheral
                // since device manager is the only one who holds a strong reference to this peripheral,
                // releasing it here cause immediate destruction and hence disconnection
                _blePeripheral = nil;
            }

            // Note that we're already in Weave work queue, which means all callbacks for the previous or current request
            // has either happened/completed or would be canceled by this call to Close. Therefore, it should be safe
            // to wipe out request context variables like _mRequestName and _mCompletionHandler.
            _mWeaveCppDM->Close();

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

- (BOOL)isConnected
{
    __block bool result = false;

    WDM_LOG_METHOD_SIG();

    // we use sync so the result is immediately available to the caller upon return
    dispatch_sync(_mWeaveWorkQueue, ^() {
        result = _mWeaveCppDM->IsConnected();
    });

    return result ? YES : NO;
}

- (BOOL)isValidPairingCode:(NSString *)pairingCode;
{
    __block bool result = false;

    WDM_LOG_METHOD_SIG();

    if ([pairingCode length] != 0) {
        // note we're not executing in any specific queue, as this subroutine is supposed to be 'static'
        result = nl::Weave::DeviceManager::WeaveDeviceManager::IsValidPairingCode([pairingCode UTF8String]);
    }

    return result ? YES : NO;
}

- (WEAVE_ERROR)GetDeviceId:(uint64_t *)deviceId
{
    __block WEAVE_ERROR err = WEAVE_NO_ERROR;
    __block uint64_t result = 0;

    WDM_LOG_METHOD_SIG();

    VerifyOrExit(NULL != _mWeaveCppDM, err = WEAVE_ERROR_INCORRECT_STATE);
    VerifyOrExit(NULL != deviceId, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // need this bracket to use Verify macros
    {
        // we use sync so the result is immediately available to the caller upon return
        dispatch_sync(_mWeaveWorkQueue, ^() {
            err = _mWeaveCppDM->GetDeviceId(result);

            if (err == WEAVE_ERROR_INCORRECT_STATE) {
                WDM_LOG_DEBUG(@"Got incorrect state error from GetDeviceId, ignore");

                err = WEAVE_NO_ERROR; // No exception, just return 0.
                result = 0;
            }
        });
    }

exit:
    if ((WEAVE_NO_ERROR == err) && (NULL != deviceId)) {
        *deviceId = result;
    }
    return err;
}

- (WEAVE_ERROR)GetDeviceMgrPtr:(long long*)deviceMgrPtr
{
    __block WEAVE_ERROR err = WEAVE_NO_ERROR;
    WDM_LOG_METHOD_SIG();
    *deviceMgrPtr = (long long)_mWeaveCppDM;

    return err;
}

- (WEAVE_ERROR)GetDeviceAddress:(NSMutableString *)strAddr;
{
    __block WEAVE_ERROR err = WEAVE_NO_ERROR;
    __block NSString * strResult = nil;

    WDM_LOG_METHOD_SIG();

    VerifyOrExit(NULL != strAddr, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // need this bracket to use Verify macros
    {
        // we use sync so the result is immediately available to the caller upon return
        dispatch_sync(_mWeaveWorkQueue, ^() {
            nl::Inet::IPAddress devAddr;
            err = _mWeaveCppDM->GetDeviceAddress(devAddr);
            if (WEAVE_NO_ERROR == err) {
                char devAddrStrBuf[32];
                strResult = [NSString stringWithUTF8String:devAddr.ToString(devAddrStrBuf, sizeof(devAddrStrBuf))];
            } else {
                strResult = @"";
            }
        });
    }

    [strAddr setString:strResult];

    if (err == WEAVE_ERROR_INCORRECT_STATE) {
        err = WEAVE_NO_ERROR; // No exception, just return null.
    }

exit:

    return err;
}

static void HandleSimpleOperationComplete(nl::Weave::DeviceManager::WeaveDeviceManager * deviceMgr, void * reqState)
{
    WDM_LOG_DEBUG(@"HandleSimpleOperationComplete");

    NLWeaveDeviceManager * dm = (__bridge NLWeaveDeviceManager *) reqState;
    // ignore the pointer to C++ device manager
    (void) deviceMgr;
    [dm DispatchAsyncCompletionBlock:nil];
}

static void onIdentifyDeviceComplete(nl::Weave::DeviceManager::WeaveDeviceManager * deviceMgr, void * appReqState,
    const nl::Weave::DeviceManager::DeviceDescription::WeaveDeviceDescriptor * devdesc)
{
    WDM_LOG_DEBUG(@"onIdentifyDeviceComplete");

    NLWeaveDeviceManager * dm = (__bridge NLWeaveDeviceManager *) appReqState;
    // ignore the pointer to C++ device manager
    (void) deviceMgr;
    [dm DispatchAsyncCompletionBlock:[NLWeaveDeviceDescriptor createUsing:*devdesc]];
}

static void onWeaveError(nl::Weave::DeviceManager::WeaveDeviceManager * deviceMgr, void * appReqState, WEAVE_ERROR code,
    nl::Weave::DeviceManager::DeviceStatus * devStatus)
{
    WDM_LOG_DEBUG(@"onWeaveError");

    NSError * error = nil;
    NSDictionary * userInfo = nil;

    NLWeaveDeviceManager * dm = (__bridge NLWeaveDeviceManager *) appReqState;
    // ignore the pointer to C++ device manager
    (void) deviceMgr;

    WDM_LOG_DEBUG(@"%@: Received error response to request %@, deviceMgrErr = %d, devStatus = %p\n", dm.name,
        [dm GetCurrentRequest], code, devStatus);

    NLWeaveRequestError requestError;
    if (devStatus != NULL && code == WEAVE_ERROR_STATUS_REPORT_RECEIVED) {
        NLProfileStatusError * statusError = [[NLProfileStatusError alloc]
            initWithProfileId:devStatus->StatusProfileId
                   statusCode:devStatus->StatusCode
                    errorCode:devStatus->SystemErrorCode
                 statusReport:[dm statusReportToString:devStatus->StatusProfileId statusCode:devStatus->StatusCode]];
        requestError = NLWeaveRequestError_ProfileStatusError;
        userInfo = @{ @"WeaveRequestErrorType" : @(requestError), @"errorInfo" : statusError };

        WDM_LOG_DEBUG(@"%@: status error: %@", dm.name, userInfo);
    } else {
        NLWeaveError * weaveError = [[NLWeaveError alloc] initWithWeaveError:code
                                                                      report:[NSString stringWithUTF8String:nl::ErrorStr(code)]];
        requestError = NLWeaveRequestError_WeaveError;
        userInfo = @{ @"WeaveRequestErrorType" : @(requestError), @"errorInfo" : weaveError };
    }

    error = [NSError errorWithDomain:@"com.nest.error" code:code userInfo:userInfo];

    [dm DispatchAsyncDefaultFailureBlock:error];
}

- (void)identifyDevice:(WDMCompletionBlock)completionHandler failure:(WDMFailureBlock)failureHandler
{
    WDM_LOG_METHOD_SIG();

    NSString * taskName = @"IdentifyDevice";

    // we use async for the results are sent back to caller via async means also
    dispatch_async(_mWeaveWorkQueue, ^() {
        if (nil == _mRequestName) {
            _mRequestName = taskName;
            _mCompletionHandler = completionHandler;
            _mFailureHandler = failureHandler;

            WEAVE_ERROR err = _mWeaveCppDM->IdentifyDevice((__bridge void *) self, onIdentifyDeviceComplete, onWeaveError);

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

- (void)rendezvousWithDevicePairingCode:(NSString *)pairingCode
                             completion:(WDMCompletionBlock)completionHandler
                                failure:(WDMFailureBlock)failureHandler
{
    // Note that NLIdentifyDeviceCriteria::create an empty criteria, which shall meet all devices
    // The prior implementation for this function, however, puts a limitation on the device mode
    NLIdentifyDeviceCriteria * criteria = [NLIdentifyDeviceCriteria create];
    criteria.TargetModes = NLTargetDeviceModeUserSelectedMode;

    [self rendezvousWithDevicePairingCode:pairingCode
                   identifyDeviceCriteria:criteria
                               completion:completionHandler
                                  failure:failureHandler];
}

- (void)rendezvousWithDevicePairingCode:(NSString *)pairingCode
                 identifyDeviceCriteria:(NLIdentifyDeviceCriteria *)identifyDeviceCriteria
                             completion:(WDMCompletionBlock)completionHandler
                                failure:(WDMFailureBlock)failureHandler
{
    WDM_LOG_METHOD_SIG();

    NSString * taskName = @"RendezvousDevice";

    // we use async for the results are sent back to caller via async means also
    dispatch_async(_mWeaveWorkQueue, ^() {
        if (nil == _mRequestName) {
            _mRequestName = taskName;
            _mCompletionHandler = completionHandler;
            _mFailureHandler = failureHandler;

            nl::Weave::Profiles::DeviceDescription::IdentifyDeviceCriteria deviceCriteria =
                [identifyDeviceCriteria toIdentifyDeviceCriteria];

            // note that RendezvousDevice interanlly makes a copy of the pairing code before return
            WEAVE_ERROR err = _mWeaveCppDM->RendezvousDevice(
                [pairingCode UTF8String], deviceCriteria, (__bridge void *) self, HandleSimpleOperationComplete, onWeaveError);

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

- (void)rendezvousWithDeviceAccessToken:(NSString *)accessToken
                 identifyDeviceCriteria:(NLIdentifyDeviceCriteria *)identifyDeviceCriteria
                             completion:(WDMCompletionBlock)completionHandler
                                failure:(WDMFailureBlock)failureHandler
{
    WDM_LOG_METHOD_SIG();

    NSString * taskName = @"RendezvousDevice";

    // we use async for the results are sent back to caller via async means also
    dispatch_async(_mWeaveWorkQueue, ^() {
        if (nil == _mRequestName) {
            _mRequestName = taskName;
            _mCompletionHandler = completionHandler;
            _mFailureHandler = failureHandler;

            nl::Weave::Profiles::DeviceDescription::IdentifyDeviceCriteria criteria =
                [identifyDeviceCriteria toIdentifyDeviceCriteria];

            Base64Encoding * base64coder = [Base64Encoding createBase64StringEncoding];
            NSData * accessTokenData = [base64coder decode:accessToken];

            // Note: accessTokenData is not supposed to be longer than uint32_t_max bytes
            // Weave, in general, uses unsigned 32-bit to represent data length
            uint32_t accessTokenLen = (uint32_t)[accessTokenData length];
            uint8_t * pAccessToken = (uint8_t *) [accessTokenData bytes];

            // note that RendezvousDevice interanlly makes a copy of the access token before return
            WEAVE_ERROR err = _mWeaveCppDM->RendezvousDevice(
                pAccessToken, accessTokenLen, criteria, (__bridge void *) self, HandleSimpleOperationComplete, onWeaveError);

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

- (void)passiveRendezvousWithCompletion:(WDMCompletionBlock)completionHandler failure:(WDMFailureBlock)failureHandler
{
    WDM_LOG_METHOD_SIG();

    NSString * taskName = @"PassiveRendezvous";

    // we use async for the results are sent back to caller via async means also
    dispatch_async(_mWeaveWorkQueue, ^() {
        if (nil == _mRequestName) {
            _mRequestName = taskName;
            _mCompletionHandler = completionHandler;
            _mFailureHandler = failureHandler;

            WEAVE_ERROR err
                = _mWeaveCppDM->PassiveRendezvousDevice((__bridge void *) self, HandleSimpleOperationComplete, onWeaveError);

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

- (void)passiveRendezvousWithDevicePairingCode:(NSString *)pairingCode
                                    completion:(WDMCompletionBlock)completionHandler
                                       failure:(WDMFailureBlock)failureHandler
{
    WDM_LOG_METHOD_SIG();

    NSString * taskName = @"PassiveRendezvous";

    // we use async for the results are sent back to caller via async means also
    dispatch_async(_mWeaveWorkQueue, ^() {
        if (nil == _mRequestName) {
            _mRequestName = taskName;
            _mCompletionHandler = completionHandler;
            _mFailureHandler = failureHandler;

            // note that PassiveRendezvousDevice interanlly makes a copy of the pairing code before return
            WEAVE_ERROR err = _mWeaveCppDM->PassiveRendezvousDevice(
                [pairingCode UTF8String], (__bridge void *) self, HandleSimpleOperationComplete, onWeaveError);

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

- (void)passiveRendezvousWithDeviceAccessToken:(NSString *)accessToken
                                    completion:(WDMCompletionBlock)completionHandler
                                       failure:(WDMFailureBlock)failureHandler
{
    WDM_LOG_METHOD_SIG();

    NSString * taskName = @"PassiveRendezvous";

    // we use async for the results are sent back to caller via async means also
    dispatch_async(_mWeaveWorkQueue, ^() {
        if (nil == _mRequestName) {
            _mRequestName = taskName;
            _mCompletionHandler = completionHandler;
            _mFailureHandler = failureHandler;

            Base64Encoding * base64coder = [Base64Encoding createBase64StringEncoding];
            NSData * accessTokenData = [base64coder decode:accessToken];

            // Note: accessTokenData is not supposed to be longer than uint32_t_max bytes
            // Weave, in general, uses unsigned 32-bit to represent data length
            uint32_t accessTokenLen = (uint32_t)[accessTokenData length];
            uint8_t * pAccessToken = (uint8_t *) [accessTokenData bytes];

            // note that PassiveRendezvousDevice interanlly makes a copy of the access token before return
            WEAVE_ERROR err = _mWeaveCppDM->PassiveRendezvousDevice(
                pAccessToken, accessTokenLen, (__bridge void *) self, HandleSimpleOperationComplete, onWeaveError);

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

- (void)remotePassiveRendezvousWithDevicePairingCode:(NSString *)pairingCode
                                           IPAddress:(NSString *)IPAddress
                                   rendezvousTimeout:(uint16_t)rendezvousTimeoutSec
                                   inactivityTimeout:(uint16_t)inactivityTimeoutSec
                                          completion:(WDMCompletionBlock)completionHandler
                                             failure:(WDMFailureBlock)failureHandler
{
    WDM_LOG_METHOD_SIG();

    NSString * taskName = @"RemotePassiveRendezvous";

    // we use async for the results are sent back to caller via async means also
    dispatch_async(_mWeaveWorkQueue, ^() {
        if (nil == _mRequestName) {
            _mRequestName = taskName;
            _mCompletionHandler = completionHandler;
            _mFailureHandler = failureHandler;

            nl::Inet::IPAddress rendezvousAddr;
            const char * rendezvousAddrStr = [IPAddress UTF8String];
            if (!nl::Inet::IPAddress::IPAddress::FromString(rendezvousAddrStr, rendezvousAddr)) {
                rendezvousAddr = nl::Inet::IPAddress::Any;
            }

            // note that RemotePassiveRendezvous interanlly makes a copy of the pairing code before return
            WEAVE_ERROR err = _mWeaveCppDM->RemotePassiveRendezvous(rendezvousAddr, [pairingCode UTF8String], rendezvousTimeoutSec,
                inactivityTimeoutSec, (__bridge void *) self, HandleSimpleOperationComplete, onWeaveError);

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

- (NSInteger)setRendezvousAddress:(NSString *)aRendezvousAddress
{
    WDM_LOG_METHOD_SIG();

    __block WEAVE_ERROR err = WEAVE_NO_ERROR;
    const char * rendezvousAddrStr = [aRendezvousAddress UTF8String];
    nl::Inet::IPAddress rendezvousAddr;

    if (!nl::Inet::IPAddress::FromString(rendezvousAddrStr, rendezvousAddr)) {
        ExitNow(err = WEAVE_ERROR_INVALID_ADDRESS);
    }

    // need this bracket to use Verify macros
    {
        // we use sync so the result is immediately available to the caller upon return
        dispatch_sync(_mWeaveWorkQueue, ^() {
            err = _mWeaveCppDM->SetWiFiRendezvousAddress(rendezvousAddr);
        });
    }

exit:
    return err;
}

static void onDeviceEnumerationResponse(nl::Weave::DeviceManager::WeaveDeviceManager * deviceMgr, void * appReqState,
    const nl::Weave::DeviceManager::DeviceDescription::WeaveDeviceDescriptor * devdesc, nl::Inet::IPAddress deviceAddr,
    nl::Inet::InterfaceId deviceIntf)
{
    WDM_LOG_DEBUG(@"onDeviceEnumerationResponse");

    WEAVE_ERROR err = WEAVE_NO_ERROR;
    NLWeaveDeviceManager * dm = (__bridge NLWeaveDeviceManager *) appReqState;
    NSString * deviceAddrStr;
    NSDictionary * deviceEnumerationResponsePlist;
    char devAddrCStr[INET6_ADDRSTRLEN + IF_NAMESIZE + 2]; // Include space for "<addr>%<interface name>" and NULL terminator

    // ignore the pointer to C++ device manager
    (void) deviceMgr;

    VerifyOrExit(deviceAddr.ToString(devAddrCStr, sizeof(devAddrCStr)) != NULL, err = WEAVE_ERROR_NO_MEMORY);

    // Add "%" separator character, with NULL terminator, per IETF RFCs 4001 and 4007
    VerifyOrExit(snprintf(&(devAddrCStr[strlen(devAddrCStr)]), 2, "%%") > 0, err = nl::Weave::System::MapErrorPOSIX(errno));

    // Concatenate zone index (aka interface name) to IP address, with NULL terminator, per IETF RFCs 4001 and 4007
    err = nl::Inet::GetInterfaceName(deviceIntf, &(devAddrCStr[strlen(devAddrCStr)]), IF_NAMESIZE + 1);
    SuccessOrExit(err);

    // create NSString from IPAddress, including IPv6 zone identifier
    deviceAddrStr = [NSString stringWithUTF8String:devAddrCStr];

    deviceEnumerationResponsePlist =
        @ { @"deviceAddress" : deviceAddrStr, @"deviceDescriptor" : [NLWeaveDeviceDescriptor createUsing:*devdesc] };

    [dm DispatchAsyncResponseBlock:deviceEnumerationResponsePlist];

exit:
    WDM_LOG_ERROR(@"Error in onDeviceEnumerationResponse: (%d) %@\n", err, [NSString stringWithUTF8String:nl::ErrorStr(err)]);
}

- (void)startDeviceEnumerationWithIdentifyDeviceCriteria:(NLIdentifyDeviceCriteria *)identifyDeviceCriteria
                                              completion:(WDMCompletionBlock)completionBlock
                                                 failure:(WDMFailureBlock)failureBlock
{
    WDM_LOG_METHOD_SIG();

    NSString * taskName = @"DeviceEnumeration";

    // we use async for the results that are sent back to caller via async means also
    dispatch_async(_mWeaveWorkQueue, ^() {
        if (nil == _mRequestName) {
            _mRequestName = taskName;
            _mCompletionHandler = completionBlock;
            _mFailureHandler = failureBlock;

            nl::Weave::Profiles::DeviceDescription::IdentifyDeviceCriteria deviceCriteria =
                [identifyDeviceCriteria toIdentifyDeviceCriteria];

            WEAVE_ERROR err = _mWeaveCppDM->StartDeviceEnumeration(
                (__bridge void *) self, deviceCriteria, onDeviceEnumerationResponse, onWeaveError);

            if (WEAVE_NO_ERROR != err) {
                [self DispatchAsyncDefaultFailureBlockWithCode:err];
            }
        } else {
            WDM_LOG_ERROR(@"%@: Attemp to %@ while we're still executing %@, ignore", _name, taskName, _mRequestName);

            // do not change _mRequestName, as we're rejecting this request
            [self DispatchAsyncFailureBlock:WEAVE_ERROR_INCORRECT_STATE taskName:taskName handler:failureBlock];
        }
    });
}

- (void)stopDeviceEnumeration
{
    WDM_LOG_METHOD_SIG();

    dispatch_sync(_mWeaveWorkQueue, ^() {
        _mWeaveCppDM->StopDeviceEnumeration();
    });

    [self MarkTransactionCompleted];
}

- (void)connectDevice:(uint64_t)deviceId
        deviceAddress:(NSString *)deviceAddress
           completion:(WDMCompletionBlock)completionHandler
              failure:(WDMFailureBlock)failureHandler
{
    WDM_LOG_METHOD_SIG();

    NSString * taskName = @"ConnectDevice";

    // we use async for the results are sent back to caller via async means also
    dispatch_async(_mWeaveWorkQueue, ^() {
        if (nil == _mRequestName) {
            _mRequestName = taskName;
            _mCompletionHandler = completionHandler;
            _mFailureHandler = failureHandler;

            nl::Inet::IPAddress deviceAddr;
            nl::Inet::IPAddress::FromString([deviceAddress UTF8String], deviceAddr);

            WEAVE_ERROR err = _mWeaveCppDM->ConnectDevice(
                deviceId, deviceAddr, (__bridge void *) self, HandleSimpleOperationComplete, onWeaveError);

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

- (void)reconnectDevice:(WDMCompletionBlock)completionHandler failure:(WDMFailureBlock)failureHandler
{
    WDM_LOG_METHOD_SIG();

    NSString * taskName = @"ReconnectDevice";

    // we use async for the results are sent back to caller via async means also
    dispatch_async(_mWeaveWorkQueue, ^() {
        if (nil == _mRequestName) {
            _mRequestName = taskName;
            _mCompletionHandler = completionHandler;
            _mFailureHandler = failureHandler;

            WEAVE_ERROR err = _mWeaveCppDM->ReconnectDevice((__bridge void *) self, HandleSimpleOperationComplete, onWeaveError);

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

- (void)connectBle:(CBPeripheral *)peripheral
        completion:(WDMCompletionBlock)completionHandler
           failure:(WDMFailureBlock)failureHandler
{
    WDM_LOG_METHOD_SIG();

    NSString * taskName = @"ConnectBle";

    // we use async for the results are sent back to caller via async means also
    dispatch_async(_mWeaveWorkQueue, ^() {
        if (nil == _mRequestName) {
            _mRequestName = taskName;
            _mCompletionHandler = completionHandler;
            _mFailureHandler = failureHandler;
            _blePeripheral = peripheral;

            // this will be called when the preparation completes one Weave work queue
            _BleConnectionPreparationCompleteHandler = ^(NLWeaveDeviceManager * dm, WEAVE_ERROR err) {
                if (WEAVE_NO_ERROR == err) {
                    err = _mWeaveCppDM->ConnectBle(
                        (__bridge void *) _blePeripheral, (__bridge void *) self, HandleSimpleOperationComplete, onWeaveError);
                }
                if (WEAVE_NO_ERROR != err) {
                    [dm DispatchAsyncDefaultFailureBlockWithCode:err];
                }
            };

            [[[NLWeaveStack sharedStack] BleDelegate] prepareNewBleConnection:self];
        } else {
            WDM_LOG_ERROR(@"%@: Attemp to %@ while we're still executing %@, ignore", _name, taskName, _mRequestName);

            // do not change _mRequestName, as we're rejecting this request
            [self DispatchAsyncFailureBlock:WEAVE_ERROR_INCORRECT_STATE taskName:taskName handler:failureHandler];
        }
    });
}

- (void)connectBleWithPairingCode:(CBPeripheral *)peripheral
                      pairingCode:(NSString *)pairingCode
                       completion:(WDMCompletionBlock)completionHandler
                          failure:(WDMFailureBlock)failureHandler
{
    WDM_LOG_METHOD_SIG();

    NSString * taskName = @"ConnectBle";

    // we use async for the results are sent back to caller via async means also
    dispatch_async(_mWeaveWorkQueue, ^() {
        if (nil == _mRequestName) {
            _mRequestName = taskName;
            _mCompletionHandler = completionHandler;
            _mFailureHandler = failureHandler;
            _blePeripheral = peripheral;

            // this will be called when the preparation completes one Weave work queue
            _BleConnectionPreparationCompleteHandler = ^(NLWeaveDeviceManager * dm, WEAVE_ERROR err) {
                if (WEAVE_NO_ERROR == err) {
                    const char * pairingCodeStr = [pairingCode UTF8String];

                    err = _mWeaveCppDM->ConnectBle((__bridge void *) _blePeripheral, pairingCodeStr, (__bridge void *) self,
                        HandleSimpleOperationComplete, onWeaveError);
                }
                if (WEAVE_NO_ERROR != err) {
                    [dm DispatchAsyncDefaultFailureBlockWithCode:err];
                }
            };

            [[[NLWeaveStack sharedStack] BleDelegate] prepareNewBleConnection:self];
        } else {
            WDM_LOG_ERROR(@"%@: Attemp to %@ while we're still executing %@, ignore", _name, taskName, _mRequestName);

            // do not change _mRequestName, as we're rejecting this request
            [self DispatchAsyncFailureBlock:WEAVE_ERROR_INCORRECT_STATE taskName:taskName handler:failureHandler];
        }
    });
}

static void onGetCameraAuthDataComplete(
    nl::Weave::DeviceManager::WeaveDeviceManager * deviceMgr, void * reqState, const char * macAddress, const char * signedPayload)
{
    WDM_LOG_DEBUG(@"onGetCameraAuthDataComplete");

    NLWeaveDeviceManager * dm = (__bridge NLWeaveDeviceManager *) reqState;
    // ignore the pointer to C++ device manager
    (void) deviceMgr;

    NSString * macAddressStr = [NSString stringWithUTF8String:macAddress];
    NSString * signedPayloadStr = [NSString stringWithUTF8String:signedPayload];

    WDM_LOG_DEBUG(@"Camera MAC = %s, payload = %s\n", macAddress, signedPayload);

    NSDictionary * cameraAuthDataPlist = @ { @"macAddress" : macAddressStr, @"signedPayload" : signedPayloadStr };

    [dm DispatchAsyncCompletionBlock:cameraAuthDataPlist];
}

- (void)getCameraAuthData:(NSString *)nonce completion:(WDMCompletionBlock)completionBlock failure:(WDMFailureBlock)failureBlock
{
    WDM_LOG_METHOD_SIG();

    NSString * taskName = @"GetCameraAuthData";

    // we use async for the results are sent back to caller via async means also
    dispatch_async(_mWeaveWorkQueue, ^() {
        if (nil == _mRequestName) {
            _mRequestName = taskName;
            _mCompletionHandler = completionBlock;
            _mFailureHandler = failureBlock;
            const char * nonceStr = [nonce UTF8String];

            WEAVE_ERROR err
                = _mWeaveCppDM->GetCameraAuthData(nonceStr, (__bridge void *) self, onGetCameraAuthDataComplete, onWeaveError);

            if (WEAVE_NO_ERROR != err) {
                [self DispatchAsyncDefaultFailureBlockWithCode:err];
            }
        } else {
            WDM_LOG_ERROR(@"%@: Attemp to %@ while we're still executing %@, ignore", _name, taskName, _mRequestName);

            // do not change _mRequestName, as we're rejecting this request
            [self DispatchAsyncFailureBlock:WEAVE_ERROR_INCORRECT_STATE taskName:taskName handler:failureBlock];
        }
    });
}

- (void)connectBleWithDeviceAccessToken:(CBPeripheral *)peripheral
                            accessToken:(NSString *)accessToken
                             completion:(WDMCompletionBlock)completionHandler
                                failure:(WDMFailureBlock)failureHandler
{
    WDM_LOG_METHOD_SIG();

    NSString * taskName = @"ConnectBle";

    // we use async for the results are sent back to caller via async means also
    dispatch_async(_mWeaveWorkQueue, ^() {
        if (nil == _mRequestName) {
            _mRequestName = taskName;
            _mCompletionHandler = completionHandler;
            _mFailureHandler = failureHandler;
            _blePeripheral = peripheral;

            // this will be called when the preparation completes one Weave work queue
            _BleConnectionPreparationCompleteHandler = ^(NLWeaveDeviceManager * dm, WEAVE_ERROR err) {
                if (WEAVE_NO_ERROR == err) {
                    Base64Encoding * base64coder = [Base64Encoding createBase64StringEncoding];
                    NSData * accessTokenData = [base64coder decode:accessToken];

                    // Note: accessTokenData is not supposed to be longer than uint32_t_max bytes
                    // Weave, in general, uses unsigned 32-bit to represent data length
                    uint32_t accessTokenLen = (uint32_t)[accessTokenData length];
                    uint8_t * pAccessToken = (uint8_t *) [accessTokenData bytes];

                    err = _mWeaveCppDM->ConnectBle((__bridge void *) _blePeripheral, pAccessToken, accessTokenLen,
                        (__bridge void *) self, HandleSimpleOperationComplete, onWeaveError);
                }

                if (WEAVE_NO_ERROR != err) {
                    [dm DispatchAsyncDefaultFailureBlockWithCode:err];
                }
            };

            [[[NLWeaveStack sharedStack] BleDelegate] prepareNewBleConnection:self];
        } else {
            WDM_LOG_ERROR(@"%@: Attemp to %@ while we're still executing %@, ignore", _name, taskName, _mRequestName);

            // do not change _mRequestName, as we're rejecting this request
            [self DispatchAsyncFailureBlock:WEAVE_ERROR_INCORRECT_STATE taskName:taskName handler:failureHandler];
        }
    });
}

static void onNetworkScanComplete(
    nl::Weave::DeviceManager::WeaveDeviceManager * deviceMgr, void * reqState, uint16_t netCount, const NetworkInfo * netInfoList)
{
    WDM_LOG_DEBUG(@"onNetworkScanComplete");

    NLWeaveDeviceManager * dm = (__bridge NLWeaveDeviceManager *) reqState;
    // ignore the pointer to C++ device manager
    (void) deviceMgr;

    NSMutableArray * networkInfoList = [NSMutableArray arrayWithCapacity:netCount];

    WDM_LOG_DEBUG(@"Count = %u\n", netCount);

    for (uint32_t i = 0; i < netCount; i++) {
        NetworkInfo * pNetworkInfoElement = (NetworkInfo *) &netInfoList[i];
        NLNetworkInfo * nlNetworkInfo = [NLNetworkInfo createUsing:pNetworkInfoElement];
        [networkInfoList addObject:nlNetworkInfo];
    }

    [dm DispatchAsyncCompletionBlock:networkInfoList];
}

- (void)scanNetworks:(NLNetworkType)networkType completion:(WDMCompletionBlock)completionBlock failure:(WDMFailureBlock)failureBlock
{
    WDM_LOG_METHOD_SIG();

    NSString * taskName = @"ScanNetworks";

    // we use async for the results are sent back to caller via async means also
    dispatch_async(_mWeaveWorkQueue, ^() {
        if (nil == _mRequestName) {
            _mRequestName = taskName;
            _mCompletionHandler = completionBlock;
            _mFailureHandler = failureBlock;

            bool isValidType = false;
            nl::Weave::Profiles::NetworkProvisioning::NetworkType type;

            switch (networkType) {
            case kNLNetworkType_NotSpecified:
                type = nl::Weave::Profiles::NetworkProvisioning::kNetworkType_NotSpecified;
                isValidType = true;
                break;
            case kNLNetworkType_WiFi:
                type = nl::Weave::Profiles::NetworkProvisioning::kNetworkType_WiFi;
                isValidType = true;
                break;
            default:
                WDM_LOG_ERROR(@"%@: invalid network type, abort", _name);
                type = nl::Weave::Profiles::NetworkProvisioning::kNetworkType_NotSpecified;
                isValidType = false;
            }

            if (isValidType) {
                WEAVE_ERROR err = _mWeaveCppDM->ScanNetworks(type, (__bridge void *) self, onNetworkScanComplete, onWeaveError);
                if (WEAVE_NO_ERROR != err) {
                    [self DispatchAsyncDefaultFailureBlockWithCode:err];
                }
            } else {
                [self DispatchAsyncDefaultFailureBlockWithCode:WEAVE_ERROR_INVALID_ARGUMENT];
            }
        } else {
            WDM_LOG_ERROR(@"%@: Attemp to %@ while we're still executing %@, ignore", _name, taskName, _mRequestName);

            // do not change _mRequestName, as we're rejecting this request
            [self DispatchAsyncFailureBlock:WEAVE_ERROR_INCORRECT_STATE taskName:taskName handler:failureBlock];
        }
    });
}

static void onAddNetworkComplete(nl::Weave::DeviceManager::WeaveDeviceManager * deviceMgr, void * reqState, uint32_t networkId)
{
    WDM_LOG_DEBUG(@"onAddNetworkComplete");

    NLWeaveDeviceManager * dm = (__bridge NLWeaveDeviceManager *) reqState;
    // ignore the pointer to C++ device manager
    (void) deviceMgr;

    WDM_LOG_DEBUG(@"networkId = %u\n", networkId);

    [dm DispatchAsyncCompletionBlock:@(networkId)];
}

- (void)addNetwork:(NLNetworkInfo *)nlNetworkInfo
        completion:(WDMCompletionBlock)completionBlock
           failure:(WDMFailureBlock)failureBlock
{
    WDM_LOG_METHOD_SIG();

    NSString * taskName = @"AddNetwork";

    // we use async for the results are sent back to caller via async means also
    dispatch_async(_mWeaveWorkQueue, ^() {
        if (nil == _mRequestName) {
            _mRequestName = taskName;
            _mCompletionHandler = completionBlock;
            _mFailureHandler = failureBlock;

            NetworkInfo networkInfo = [nlNetworkInfo toNetworkInfo];

            WEAVE_ERROR err = _mWeaveCppDM->AddNetwork(&networkInfo, (__bridge void *) self, onAddNetworkComplete, onWeaveError);
            if (WEAVE_NO_ERROR != err) {
                [self DispatchAsyncDefaultFailureBlockWithCode:err];
            }
        } else {
            WDM_LOG_ERROR(@"%@: Attemp to %@ while we're still executing %@, ignore", _name, taskName, _mRequestName);

            // do not change _mRequestName, as we're rejecting this request
            [self DispatchAsyncFailureBlock:WEAVE_ERROR_INCORRECT_STATE taskName:taskName handler:failureBlock];
        }
    });
}

- (void)updateNetwork:(NLNetworkInfo *)nlNetworkInfo
           completion:(WDMCompletionBlock)completionBlock
              failure:(WDMFailureBlock)failureBlock
{
    WDM_LOG_METHOD_SIG();

    NSString * taskName = @"updateNetwork";

    // we use async for the results are sent back to caller via async means also
    dispatch_async(_mWeaveWorkQueue, ^() {
        if (nil == _mRequestName) {
            _mRequestName = taskName;
            _mCompletionHandler = completionBlock;
            _mFailureHandler = failureBlock;

            NetworkInfo networkInfo = [nlNetworkInfo toNetworkInfo];

            WEAVE_ERROR err
                = _mWeaveCppDM->UpdateNetwork(&networkInfo, (__bridge void *) self, HandleSimpleOperationComplete, onWeaveError);
            if (WEAVE_NO_ERROR != err) {
                [self DispatchAsyncDefaultFailureBlockWithCode:err];
            }
        } else {
            WDM_LOG_ERROR(@"%@: Attemp to %@ while we're still executing %@, ignore", _name, taskName, _mRequestName);

            // do not change _mRequestName, as we're rejecting this request
            [self DispatchAsyncFailureBlock:WEAVE_ERROR_INCORRECT_STATE taskName:taskName handler:failureBlock];
        }
    });
}

- (void)removeNetwork:(NLNetworkID)networkId completion:(WDMCompletionBlock)completionBlock failure:(WDMFailureBlock)failureBlock
{
    WDM_LOG_METHOD_SIG();

    NSString * taskName = @"removeNetwork";

    // we use async for the results are sent back to caller via async means also
    dispatch_async(_mWeaveWorkQueue, ^() {
        if (nil == _mRequestName) {
            _mRequestName = taskName;
            _mCompletionHandler = completionBlock;
            _mFailureHandler = failureBlock;

            WEAVE_ERROR err = _mWeaveCppDM->RemoveNetwork(
                (uint32_t) networkId, (__bridge void *) self, HandleSimpleOperationComplete, onWeaveError);
            if (WEAVE_NO_ERROR != err) {
                [self DispatchAsyncDefaultFailureBlockWithCode:err];
            }
        } else {
            WDM_LOG_ERROR(@"%@: Attemp to %@ while we're still executing %@, ignore", _name, taskName, _mRequestName);

            // do not change _mRequestName, as we're rejecting this request
            [self DispatchAsyncFailureBlock:WEAVE_ERROR_INCORRECT_STATE taskName:taskName handler:failureBlock];
        }
    });
}

- (void)getNetworks:(uint8_t)flags completion:(WDMCompletionBlock)completionBlock failure:(WDMFailureBlock)failureBlock
{
    WDM_LOG_METHOD_SIG();

    NSString * taskName = @"getNetworks";

    // we use async for the results are sent back to caller via async means also
    dispatch_async(_mWeaveWorkQueue, ^() {
        if (nil == _mRequestName) {
            _mRequestName = taskName;
            _mCompletionHandler = completionBlock;
            _mFailureHandler = failureBlock;

            WEAVE_ERROR err = _mWeaveCppDM->GetNetworks(flags, (__bridge void *) self, onNetworkScanComplete, onWeaveError);
            if (WEAVE_NO_ERROR != err) {
                [self DispatchAsyncDefaultFailureBlockWithCode:err];
            }
        } else {
            WDM_LOG_ERROR(@"%@: Attemp to %@ while we're still executing %@, ignore", _name, taskName, _mRequestName);

            // do not change _mRequestName, as we're rejecting this request
            [self DispatchAsyncFailureBlock:WEAVE_ERROR_INCORRECT_STATE taskName:taskName handler:failureBlock];
        }
    });
}

- (void)enableNetwork:(NLNetworkID)networkId completion:(WDMCompletionBlock)completionBlock failure:(WDMFailureBlock)failureBlock
{
    WDM_LOG_METHOD_SIG();

    NSString * taskName = @"enableNetwork";

    // we use async for the results are sent back to caller via async means also
    dispatch_async(_mWeaveWorkQueue, ^() {
        if (nil == _mRequestName) {
            _mRequestName = taskName;
            _mCompletionHandler = completionBlock;
            _mFailureHandler = failureBlock;

            WEAVE_ERROR err = _mWeaveCppDM->EnableNetwork(
                (uint32_t) networkId, (__bridge void *) self, HandleSimpleOperationComplete, onWeaveError);
            if (WEAVE_NO_ERROR != err) {
                [self DispatchAsyncDefaultFailureBlockWithCode:err];
            }
        } else {
            WDM_LOG_ERROR(@"%@: Attemp to %@ while we're still executing %@, ignore", _name, taskName, _mRequestName);

            // do not change _mRequestName, as we're rejecting this request
            [self DispatchAsyncFailureBlock:WEAVE_ERROR_INCORRECT_STATE taskName:taskName handler:failureBlock];
        }
    });
}

- (void)disableNetwork:(NLNetworkID)networkId completion:(WDMCompletionBlock)completionBlock failure:(WDMFailureBlock)failureBlock
{
    WDM_LOG_METHOD_SIG();

    NSString * taskName = @"disableNetwork";

    // we use async for the results are sent back to caller via async means also
    dispatch_async(_mWeaveWorkQueue, ^() {
        if (nil == _mRequestName) {
            _mRequestName = taskName;
            _mCompletionHandler = completionBlock;
            _mFailureHandler = failureBlock;

            WEAVE_ERROR err = _mWeaveCppDM->DisableNetwork(
                (uint32_t) networkId, (__bridge void *) self, HandleSimpleOperationComplete, onWeaveError);
            if (WEAVE_NO_ERROR != err) {
                [self DispatchAsyncDefaultFailureBlockWithCode:err];
            }
        } else {
            WDM_LOG_ERROR(@"%@: Attemp to %@ while we're still executing %@, ignore", _name, taskName, _mRequestName);

            // do not change _mRequestName, as we're rejecting this request
            [self DispatchAsyncFailureBlock:WEAVE_ERROR_INCORRECT_STATE taskName:taskName handler:failureBlock];
        }
    });
}

- (void)testNetworkConnectivity:(NLNetworkID)networkId
                     completion:(WDMCompletionBlock)completionBlock
                        failure:(WDMFailureBlock)failureBlock
{
    WDM_LOG_METHOD_SIG();

    NSString * taskName = @"testNetworkConnectivity";

    // we use async for the results are sent back to caller via async means also
    dispatch_async(_mWeaveWorkQueue, ^() {
        if (nil == _mRequestName) {
            _mRequestName = taskName;
            _mCompletionHandler = completionBlock;
            _mFailureHandler = failureBlock;

            WEAVE_ERROR err = _mWeaveCppDM->TestNetworkConnectivity(
                (uint32_t) networkId, (__bridge void *) self, HandleSimpleOperationComplete, onWeaveError);
            if (WEAVE_NO_ERROR != err) {
                [self DispatchAsyncDefaultFailureBlockWithCode:err];
            }
        } else {
            WDM_LOG_ERROR(@"%@: Attemp to %@ while we're still executing %@, ignore", _name, taskName, _mRequestName);

            // do not change _mRequestName, as we're rejecting this request
            [self DispatchAsyncFailureBlock:WEAVE_ERROR_INCORRECT_STATE taskName:taskName handler:failureBlock];
        }
    });
}

static void onGetRendezvousModeComplete(
    nl::Weave::DeviceManager::WeaveDeviceManager * deviceMgr, void * reqState, uint16_t modeFlags)
{
    WDM_LOG_DEBUG(@"onAddNetworkComplete");

    NLWeaveDeviceManager * dm = (__bridge NLWeaveDeviceManager *) reqState;
    // ignore the pointer to C++ device manager
    (void) deviceMgr;

    WDM_LOG_DEBUG(@"networkId = %u\n", modeFlags);

    [dm DispatchAsyncCompletionBlock:@(modeFlags)];
}

- (void)getRendezvousMode:(WDMCompletionBlock)completionBlock failure:(WDMFailureBlock)failureBlock
{
    WDM_LOG_METHOD_SIG();

    NSString * taskName = @"getRendezvousMode";

    // we use async for the results are sent back to caller via async means also
    dispatch_async(_mWeaveWorkQueue, ^() {
        if (nil == _mRequestName) {
            _mRequestName = taskName;
            _mCompletionHandler = completionBlock;
            _mFailureHandler = failureBlock;

            WEAVE_ERROR err = _mWeaveCppDM->GetRendezvousMode((__bridge void *) self, onGetRendezvousModeComplete, onWeaveError);
            if (WEAVE_NO_ERROR != err) {
                [self DispatchAsyncDefaultFailureBlockWithCode:err];
            }
        } else {
            WDM_LOG_ERROR(@"%@: Attemp to %@ while we're still executing %@, ignore", _name, taskName, _mRequestName);

            // do not change _mRequestName, as we're rejecting this request
            [self DispatchAsyncFailureBlock:WEAVE_ERROR_INCORRECT_STATE taskName:taskName handler:failureBlock];
        }
    });
}

- (void)setRendezvousMode:(uint16_t)rendezvousFlags
               completion:(WDMCompletionBlock)completionBlock
                  failure:(WDMFailureBlock)failureBlock
{
    WDM_LOG_METHOD_SIG();

    NSString * taskName = @"SetRendezvousMode";

    // we use async for the results are sent back to caller via async means also
    dispatch_async(_mWeaveWorkQueue, ^() {
        if (nil == _mRequestName) {
            _mRequestName = taskName;
            _mCompletionHandler = completionBlock;
            _mFailureHandler = failureBlock;

            WEAVE_ERROR err = _mWeaveCppDM->SetRendezvousMode(
                (uint16_t) rendezvousFlags, (__bridge void *) self, HandleSimpleOperationComplete, onWeaveError);
            if (WEAVE_NO_ERROR != err) {
                [self DispatchAsyncDefaultFailureBlockWithCode:err];
            }
        } else {
            WDM_LOG_ERROR(@"%@: Attemp to %@ while we're still executing %@, ignore", _name, taskName, _mRequestName);

            // do not change _mRequestName, as we're rejecting this request
            [self DispatchAsyncFailureBlock:WEAVE_ERROR_INCORRECT_STATE taskName:taskName handler:failureBlock];
        }
    });
}

- (WEAVE_ERROR)setAutoReconnect:(BOOL)autoReconnect
{
    __block WEAVE_ERROR err = WEAVE_NO_ERROR;

    WDM_LOG_METHOD_SIG();

    VerifyOrExit(NULL != _mWeaveCppDM, err = WEAVE_ERROR_INCORRECT_STATE);

    // need this bracket to use Verify macros
    {
        // we use sync so the result is immediately available to the caller upon return
        dispatch_sync(_mWeaveWorkQueue, ^() {
            err = _mWeaveCppDM->SetAutoReconnect(autoReconnect ? true : false);
        });
    }

exit:
    return err;
}

- (void)createFabric:(WDMCompletionBlock)completionBlock failure:(WDMFailureBlock)failureBlock
{
    WDM_LOG_METHOD_SIG();

    NSString * taskName = @"CreateFabric";

    // we use async for the results are sent back to caller via async means also
    dispatch_async(_mWeaveWorkQueue, ^() {
        if (nil == _mRequestName) {
            _mRequestName = taskName;
            _mCompletionHandler = completionBlock;
            _mFailureHandler = failureBlock;

            WEAVE_ERROR err = _mWeaveCppDM->CreateFabric((__bridge void *) self, HandleSimpleOperationComplete, onWeaveError);
            if (WEAVE_NO_ERROR != err) {
                [self DispatchAsyncDefaultFailureBlockWithCode:err];
            }
        } else {
            WDM_LOG_ERROR(@"%@: Attemp to %@ while we're still executing %@, ignore", _name, taskName, _mRequestName);

            // do not change _mRequestName, as we're rejecting this request
            [self DispatchAsyncFailureBlock:WEAVE_ERROR_INCORRECT_STATE taskName:taskName handler:failureBlock];
        }
    });
}

- (void)leaveFabric:(WDMCompletionBlock)completionBlock failure:(WDMFailureBlock)failureBlock
{
    WDM_LOG_METHOD_SIG();

    NSString * taskName = @"LeaveFabric";

    // we use async for the results are sent back to caller via async means also
    dispatch_async(_mWeaveWorkQueue, ^() {
        if (nil == _mRequestName) {
            _mRequestName = taskName;
            _mCompletionHandler = completionBlock;
            _mFailureHandler = failureBlock;

            WEAVE_ERROR err = _mWeaveCppDM->LeaveFabric((__bridge void *) self, HandleSimpleOperationComplete, onWeaveError);
            if (WEAVE_NO_ERROR != err) {
                [self DispatchAsyncDefaultFailureBlockWithCode:err];
            }
        } else {
            WDM_LOG_ERROR(@"%@: Attemp to %@ while we're still executing %@, ignore", _name, taskName, _mRequestName);

            // do not change _mRequestName, as we're rejecting this request
            [self DispatchAsyncFailureBlock:WEAVE_ERROR_INCORRECT_STATE taskName:taskName handler:failureBlock];
        }
    });
}

static void onGetFabricConfigComplete(nl::Weave::DeviceManager::WeaveDeviceManager * deviceMgr, void * reqState,
    const uint8_t * fabricConfig, uint32_t fabricConfigLen)
{
    WDM_LOG_DEBUG(@"onGetFabricConfigComplete");

    NLWeaveDeviceManager * dm = (__bridge NLWeaveDeviceManager *) reqState;
    // ignore the pointer to C++ device manager
    (void) deviceMgr;

    [dm DispatchAsyncCompletionBlock:[NSData dataWithBytes:fabricConfig length:fabricConfigLen]];
}

- (void)getFabricConfig:(WDMCompletionBlock)completionBlock failure:(WDMFailureBlock)failureBlock
{
    WDM_LOG_METHOD_SIG();

    NSString * taskName = @"GetFabricConfig";

    // we use async for the results are sent back to caller via async means also
    dispatch_async(_mWeaveWorkQueue, ^() {
        if (nil == _mRequestName) {
            _mRequestName = taskName;
            _mCompletionHandler = completionBlock;
            _mFailureHandler = failureBlock;

            WEAVE_ERROR err = _mWeaveCppDM->GetFabricConfig((__bridge void *) self, onGetFabricConfigComplete, onWeaveError);
            if (WEAVE_NO_ERROR != err) {
                [self DispatchAsyncDefaultFailureBlockWithCode:err];
            }
        } else {
            WDM_LOG_ERROR(@"%@: Attemp to %@ while we're still executing %@, ignore", _name, taskName, _mRequestName);

            // do not change _mRequestName, as we're rejecting this reques
            [self DispatchAsyncFailureBlock:WEAVE_ERROR_INCORRECT_STATE taskName:taskName handler:failureBlock];
        }
    });
}

- (void)joinExistingFabric:(NSData *)fabricConfig
                completion:(WDMCompletionBlock)completionBlock
                   failure:(WDMFailureBlock)failureBlock
{
    WDM_LOG_METHOD_SIG();

    NSString * taskName = @"JoinExistingFabric";

    // we use async for the results are sent back to caller via async means also
    dispatch_async(_mWeaveWorkQueue, ^() {
        if (nil == _mRequestName) {
            _mRequestName = taskName;
            _mCompletionHandler = completionBlock;
            _mFailureHandler = failureBlock;

            uint16_t fabricConfigLen = fabricConfig ? (uint16_t)[fabricConfig length] : 0;
            uint8_t * fabricConfigBuf = (uint8_t *) [fabricConfig bytes];

            WEAVE_ERROR err = _mWeaveCppDM->JoinExistingFabric(
                fabricConfigBuf, fabricConfigLen, (__bridge void *) self, HandleSimpleOperationComplete, onWeaveError);
            if (WEAVE_NO_ERROR != err) {
                [self DispatchAsyncDefaultFailureBlockWithCode:err];
            }
        } else {
            WDM_LOG_ERROR(@"%@: Attemp to %@ while we're still executing %@, ignore", _name, taskName, _mRequestName);

            // do not change _mRequestName, as we're rejecting this request
            [self DispatchAsyncFailureBlock:WEAVE_ERROR_INCORRECT_STATE taskName:taskName handler:failureBlock];
        }
    });
}

- (void)registerServicePairAccount:(NLServiceInfo *)nlServiceInfo
                        completion:(WDMCompletionBlock)completionBlock
                           failure:(WDMFailureBlock)failureBlock
{
    WDM_LOG_METHOD_SIG();

    NSString * taskName = @"RegisterServicePairAccount";

    // we use async for the results are sent back to caller via async means also
    dispatch_async(_mWeaveWorkQueue, ^() {
        if (nil == _mRequestName) {
            _mRequestName = taskName;
            _mCompletionHandler = completionBlock;
            _mFailureHandler = failureBlock;

            const char * pAccountIdStr = [nlServiceInfo.accountId UTF8String];

            NSData * serviceConfigData = [nlServiceInfo decodeServiceConfig];
            uint16_t serviceConfigLen = (uint16_t)[serviceConfigData length];
            uint8_t * pServiceConfig = (uint8_t *) [serviceConfigData bytes];

            NSData * data = [NSData dataWithBytes:[nlServiceInfo.pairingToken UTF8String]
                                           length:[nlServiceInfo.pairingToken lengthOfBytesUsingEncoding:NSUTF8StringEncoding]];
            uint16_t pairingTokenLen = (uint16_t)[nlServiceInfo.pairingToken length];
            uint8_t * pPairingToken = (uint8_t *) [data bytes];

            data = [nlServiceInfo.pairingInitData dataUsingEncoding:NSUTF8StringEncoding];
            uint16_t pairingInitDataLen = (uint16_t) data.length;
            uint8_t * pPairingInitData = (uint8_t *) [data bytes];

            WEAVE_ERROR err = _mWeaveCppDM->RegisterServicePairAccount(nlServiceInfo.serviceId, pAccountIdStr, pServiceConfig,
                serviceConfigLen, pPairingToken, pairingTokenLen, pPairingInitData, pairingInitDataLen, (__bridge void *) self,
                HandleSimpleOperationComplete, onWeaveError);
            if (WEAVE_NO_ERROR != err) {
                [self DispatchAsyncDefaultFailureBlockWithCode:err];
            }
        } else {
            WDM_LOG_ERROR(@"%@: Attemp to %@ while we're still executing %@, ignore", _name, taskName, _mRequestName);

            // do not change _mRequestName, as we're rejecting this request
            [self DispatchAsyncFailureBlock:WEAVE_ERROR_INCORRECT_STATE taskName:taskName handler:failureBlock];
        }
    });
}

- (void)updateService:(NLServiceInfo *)nlServiceInfo
           completion:(WDMCompletionBlock)completionBlock
              failure:(WDMFailureBlock)failureBlock
{
    WDM_LOG_METHOD_SIG();

    NSString * taskName = @"UpdateService";

    // we use async for the results are sent back to caller via async means also
    dispatch_async(_mWeaveWorkQueue, ^() {
        if (nil == _mRequestName) {
            _mRequestName = taskName;
            _mCompletionHandler = completionBlock;
            _mFailureHandler = failureBlock;

            NSData * serviceConfigData = [nlServiceInfo decodeServiceConfig];
            uint16_t serviceConfigLen = [serviceConfigData length];
            uint8_t * pServiceConfig = (uint8_t *) [serviceConfigData bytes];

            WEAVE_ERROR err = _mWeaveCppDM->UpdateService(nlServiceInfo.serviceId, pServiceConfig, serviceConfigLen,
                (__bridge void *) self, HandleSimpleOperationComplete, onWeaveError);
            if (WEAVE_NO_ERROR != err) {
                [self DispatchAsyncDefaultFailureBlockWithCode:err];
            }
        } else {
            WDM_LOG_ERROR(@"%@: Attemp to %@ while we're still executing %@, ignore", _name, taskName, _mRequestName);

            // do not change _mRequestName, as we're rejecting this request
            [self DispatchAsyncFailureBlock:WEAVE_ERROR_INCORRECT_STATE taskName:taskName handler:failureBlock];
        }
    });
}

- (void)unregisterService:(uint64_t)serviceId completion:(WDMCompletionBlock)completionBlock failure:(WDMFailureBlock)failureBlock
{
    WDM_LOG_METHOD_SIG();

    NSString * taskName = @"UnregisterService";

    // we use async for the results are sent back to caller via async means also
    dispatch_async(_mWeaveWorkQueue, ^() {
        if (nil == _mRequestName) {
            _mRequestName = taskName;
            _mCompletionHandler = completionBlock;
            _mFailureHandler = failureBlock;

            WEAVE_ERROR err
                = _mWeaveCppDM->UnregisterService(serviceId, (__bridge void *) self, HandleSimpleOperationComplete, onWeaveError);
            if (WEAVE_NO_ERROR != err) {
                [self DispatchAsyncDefaultFailureBlockWithCode:err];
            }
        } else {
            WDM_LOG_ERROR(@"%@: Attemp to %@ while we're still executing %@, ignore", _name, taskName, _mRequestName);

            // do not change _mRequestName, as we're rejecting this request
            [self DispatchAsyncFailureBlock:WEAVE_ERROR_INCORRECT_STATE taskName:taskName handler:failureBlock];
        }
    });
}

- (void)getLastNetworkProvisioningResult:(WDMCompletionBlock)completionBlock failure:(WDMFailureBlock)failureBlock
{
    WDM_LOG_METHOD_SIG();

    NSString * taskName = @"GetLastNetworkProvisioningResult";

    // we use async for the results are sent back to caller via async means also
    dispatch_async(_mWeaveWorkQueue, ^() {
        if (nil == _mRequestName) {
            _mRequestName = taskName;
            _mCompletionHandler = completionBlock;
            _mFailureHandler = failureBlock;

            WEAVE_ERROR err = _mWeaveCppDM->GetLastNetworkProvisioningResult(
                (__bridge void *) self, HandleSimpleOperationComplete, onWeaveError);

            if (WEAVE_NO_ERROR != err) {
                [self DispatchAsyncDefaultFailureBlockWithCode:err];
            }
        } else {
            WDM_LOG_ERROR(@"%@: Attemp to %@ while we're still executing %@, ignore", _name, taskName, _mRequestName);

            // do not change _mRequestName, as we're rejecting this request
            [self DispatchAsyncFailureBlock:WEAVE_ERROR_INCORRECT_STATE taskName:taskName handler:failureBlock];
        }
    });
}

- (void)armFailSafe:(uint8_t)armMode
      failSafeToken:(uint32_t)failSafeToken
         completion:(WDMCompletionBlock)completionBlock
            failure:(WDMFailureBlock)failureBlock
{
    WDM_LOG_METHOD_SIG();

    NSString * taskName = @"ArmFailSafe";

    // we use async for the results are sent back to caller via async means also
    dispatch_async(_mWeaveWorkQueue, ^() {
        if (nil == _mRequestName) {
            _mRequestName = taskName;
            _mCompletionHandler = completionBlock;
            _mFailureHandler = failureBlock;

            WEAVE_ERROR err = _mWeaveCppDM->ArmFailSafe(
                armMode, failSafeToken, (__bridge void *) self, HandleSimpleOperationComplete, onWeaveError);

            if (WEAVE_NO_ERROR != err) {
                [self DispatchAsyncDefaultFailureBlockWithCode:err];
            }
        } else {
            WDM_LOG_ERROR(@"%@: Attemp to %@ while we're still executing %@, ignore", _name, taskName, _mRequestName);

            // do not change _mRequestName, as we're rejecting this request
            [self DispatchAsyncFailureBlock:WEAVE_ERROR_INCORRECT_STATE taskName:taskName handler:failureBlock];
        }
    });
}

- (void)disarmFailSafe:(WDMCompletionBlock)completionBlock failure:(WDMFailureBlock)failureBlock
{
    WDM_LOG_METHOD_SIG();

    NSString * taskName = @"DisarmFailSafe";

    // we use async for the results are sent back to caller via async means also
    dispatch_async(_mWeaveWorkQueue, ^() {
        if (nil == _mRequestName) {
            _mRequestName = taskName;
            _mCompletionHandler = completionBlock;
            _mFailureHandler = failureBlock;

            WEAVE_ERROR err = _mWeaveCppDM->DisarmFailSafe((__bridge void *) self, HandleSimpleOperationComplete, onWeaveError);

            if (WEAVE_NO_ERROR != err) {
                [self DispatchAsyncDefaultFailureBlockWithCode:err];
            }
        } else {
            WDM_LOG_ERROR(@"%@: Attemp to %@ while we're still executing %@, ignore", _name, taskName, _mRequestName);

            // do not change _mRequestName, as we're rejecting this request
            [self DispatchAsyncFailureBlock:WEAVE_ERROR_INCORRECT_STATE taskName:taskName handler:failureBlock];
        }
    });
}

- (void)resetConfig:(uint16_t)resetFlags completion:(WDMCompletionBlock)completionBlock failure:(WDMFailureBlock)failureBlock
{
    WDM_LOG_METHOD_SIG();

    NSString * taskName = @"ResetConfig";

    // we use async for the results are sent back to caller via async means also
    dispatch_async(_mWeaveWorkQueue, ^() {
        if (nil == _mRequestName) {
            _mRequestName = taskName;
            _mCompletionHandler = completionBlock;
            _mFailureHandler = failureBlock;

            WEAVE_ERROR err
                = _mWeaveCppDM->ResetConfig(resetFlags, (__bridge void *) self, HandleSimpleOperationComplete, onWeaveError);

            if (WEAVE_NO_ERROR != err) {
                [self DispatchAsyncDefaultFailureBlockWithCode:err];
            }
        } else {
            WDM_LOG_ERROR(@"%@: Attemp to %@ while we're still executing %@, ignore", _name, taskName, _mRequestName);

            // do not change _mRequestName, as we're rejecting this request
            [self DispatchAsyncFailureBlock:WEAVE_ERROR_INCORRECT_STATE taskName:taskName handler:failureBlock];
        }
    });
}

- (void)enableConnectionMonitor:(NSInteger)intervalMs
                        timeout:(NSInteger)timeoutMs
                     completion:(WDMCompletionBlock)completionBlock
                        failure:(WDMFailureBlock)failureBlock
{
    WDM_LOG_METHOD_SIG();

    NSString * taskName = @"EnableConnectionMonitor";

    // we use async for the results are sent back to caller via async means also
    dispatch_async(_mWeaveWorkQueue, ^() {
        if (nil == _mRequestName) {
            _mRequestName = taskName;
            _mCompletionHandler = completionBlock;
            _mFailureHandler = failureBlock;

            WEAVE_ERROR err = _mWeaveCppDM->EnableConnectionMonitor(
                intervalMs, timeoutMs, (__bridge void *) self, HandleSimpleOperationComplete, onWeaveError);

            if (WEAVE_NO_ERROR != err) {
                [self DispatchAsyncDefaultFailureBlockWithCode:err];
            }
        } else {
            WDM_LOG_ERROR(@"%@: Attemp to %@ while we're still executing %@, ignore", _name, taskName, _mRequestName);

            // do not change _mRequestName, as we're rejecting this request
            [self DispatchAsyncFailureBlock:WEAVE_ERROR_INCORRECT_STATE taskName:taskName handler:failureBlock];
        }
    });
}

- (void)disableConnectionMonitor:(WDMCompletionBlock)completionBlock failure:(WDMFailureBlock)failureBlock
{
    WDM_LOG_METHOD_SIG();

    NSString * taskName = @"DisableConnectionMonitor";

    // we use async for the results are sent back to caller via async means also
    dispatch_async(_mWeaveWorkQueue, ^() {
        if (nil == _mRequestName) {
            _mRequestName = taskName;
            _mCompletionHandler = completionBlock;
            _mFailureHandler = failureBlock;

            WEAVE_ERROR err
                = _mWeaveCppDM->DisableConnectionMonitor((__bridge void *) self, HandleSimpleOperationComplete, onWeaveError);

            if (WEAVE_NO_ERROR != err) {
                [self DispatchAsyncDefaultFailureBlockWithCode:err];
            }
        } else {
            WDM_LOG_ERROR(@"%@: Attemp to %@ while we're still executing %@, ignore", _name, taskName, _mRequestName);

            // do not change _mRequestName, as we're rejecting this request
            [self DispatchAsyncFailureBlock:WEAVE_ERROR_INCORRECT_STATE taskName:taskName handler:failureBlock];
        }
    });
}

- (void)startSystemTest:(uint32_t)profileId
                 testId:(uint32_t)testId
             completion:(WDMCompletionBlock)completionBlock
                failure:(WDMFailureBlock)failureBlock
{
    WDM_LOG_METHOD_SIG();

    NSString * taskName = @"StartSystemTest";

    // we use async for the results are sent back to caller via async means also
    dispatch_async(_mWeaveWorkQueue, ^() {
        if (nil == _mRequestName) {
            _mRequestName = taskName;
            _mCompletionHandler = completionBlock;
            _mFailureHandler = failureBlock;

            WEAVE_ERROR err = _mWeaveCppDM->StartSystemTest(
                (__bridge void *) self, profileId, testId, HandleSimpleOperationComplete, onWeaveError);

            if (WEAVE_NO_ERROR != err) {
                [self DispatchAsyncDefaultFailureBlockWithCode:err];
            }
        } else {
            WDM_LOG_ERROR(@"%@: Attemp to %@ while we're still executing %@, ignore", _name, taskName, _mRequestName);

            // do not change _mRequestName, as we're rejecting this request
            [self DispatchAsyncFailureBlock:WEAVE_ERROR_INCORRECT_STATE taskName:taskName handler:failureBlock];
        }
    });
}

- (void)ping:(WDMCompletionBlock)completionHandler failure:(WDMFailureBlock)failureHandler
{
    WDM_LOG_METHOD_SIG();

    NSString * taskName = @"Ping";

    // we use async for the results are sent back to caller via async means also
    dispatch_async(_mWeaveWorkQueue, ^() {
        if (nil == _mRequestName) {
            _mRequestName = taskName;
            _mCompletionHandler = completionHandler;
            _mFailureHandler = failureHandler;

            WEAVE_ERROR err = _mWeaveCppDM->Ping((__bridge void *) self, HandleSimpleOperationComplete, onWeaveError);

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

static void onPairTokenComplete(nl::Weave::DeviceManager::WeaveDeviceManager * deviceMgr, void * reqState,
    const uint8_t * pairingTokenBundle, uint32_t pairingTokenBundleLen)
{
    WDM_LOG_DEBUG(@"onPairTokenComplete");

    NLWeaveDeviceManager * dm = (__bridge NLWeaveDeviceManager *) reqState;
    // ignore the pointer to C++ device manager
    (void) deviceMgr;

    [dm DispatchAsyncCompletionBlock:[NSData dataWithBytes:pairingTokenBundle length:pairingTokenBundleLen]];
}

- (void)pairToken:(NSData *)pairingToken completion:(WDMCompletionBlock)completionBlock failure:(WDMFailureBlock)failureBlock
{
    WDM_LOG_METHOD_SIG();

    NSString * taskName = @"pairToken";

    // we use async for the results are sent back to caller via async means also
    dispatch_async(_mWeaveWorkQueue, ^() {
        if (nil == _mRequestName) {
            _mRequestName = taskName;
            _mCompletionHandler = completionBlock;
            _mFailureHandler = failureBlock;

            uint16_t pairingTokenLen = pairingToken ? (uint16_t)[pairingToken length] : 0;
            uint8_t * pairingTokenBuf = (uint8_t *) [pairingToken bytes];

            WEAVE_ERROR err = _mWeaveCppDM->PairToken(
                pairingTokenBuf, pairingTokenLen, (__bridge void *) self, onPairTokenComplete, onWeaveError);
            if (WEAVE_NO_ERROR != err) {
                [self DispatchAsyncDefaultFailureBlockWithCode:err];
            }
        } else {
            WDM_LOG_ERROR(@"%@: Attemp to %@ while we're still executing %@, ignore", _name, taskName, _mRequestName);

            // do not change _mRequestName, as we're rejecting this request
            [self DispatchAsyncFailureBlock:WEAVE_ERROR_INCORRECT_STATE taskName:taskName handler:failureBlock];
        }
    });
}

@end
