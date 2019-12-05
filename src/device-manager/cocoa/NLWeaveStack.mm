/*
 *
 *    Copyright (c) 2015-2018 Nest Labs, Inc.
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
 *      This file implements NLWeaveStack interface
 *
 */

#import <Foundation/Foundation.h>
#import "NLWeaveStack.h"
#import "NLLogging.h"
#import "NLWeaveBleDelegate_Protected.h"
#import "NLWeaveDeviceManager.h"
#import "NLWeaveDeviceManager_Protected.h"
#import "NLWdmClient_Protected.h"

#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveError.h>
#include <Weave/Support/CodeUtils.h>
#include <Weave/Profiles/data-management/Current/WdmManagedNamespace.h>
#include <Weave/Profiles/data-management/DataManagement.h>
#include <WeaveDataManagementClient.h>

using namespace nl::Weave::Profiles;
using namespace nl::Weave::Profiles::DataManagement;

@interface NLWeaveStack () {
    dispatch_queue_t _mWorkQueue;
    dispatch_queue_t _mSelectQueue;
    nl::Weave::System::Layer _mSystemLayer;
    nl::Inet::InetLayer _mInetLayer;
    nl::Weave::WeaveFabricState _mFabricState;
    nl::Weave::WeaveMessageLayer _mMessageLayer;
    nl::Weave::WeaveExchangeManager _mExchangeMgr;
    nl::Weave::WeaveSecurityManager _mSecurityMgr;
    NLWeaveDeviceManager * _mDeviceMgr;
    NLWdmClient * _mWdmClient;

    // for shutdown
    bool _mIsWaitingOnSelect;
    ShutdownCompletionBlock _mShutdownCompletionBlock;

#if CONFIG_NETWORK_LAYER_BLE
    nl::Ble::BleLayer _mBleLayer;
    NLWeaveBleDelegate * _mBleDelegate;
#endif // #if CONFIG_NETWORK_LAYER_BLE
    /*
    int selectRes;
    int MaxNumberedFdPlusOne;
    struct timeval sleepTime;
    fd_set readFDs, writeFDs, exceptFDs;
     */
}

- (WEAVE_ERROR)InitStack_internal:(NSString *)listenAddr bleDelegate:(NLWeaveBleDelegate *)bleDelegate;
- (void)ShutdownStack_Stage1;
- (void)ShutdownStack_Stage2;
- (void)TryProcessNetworkEvents;

- (instancetype)init NS_DESIGNATED_INITIALIZER;

@end

@implementation NLWeaveStack

@synthesize WorkQueue = _mWorkQueue;

@synthesize BleDelegate = _mBleDelegate;

#if CONFIG_NETWORK_LAYER_BLE
#endif //#if CONFIG_NETWORK_LAYER_BLE

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
}

/**
    @note
      This function can be called from any thread/queue at any time
 */
+ (instancetype)sharedStack
{
    static NLWeaveStack * mStack = nil;
    static dispatch_once_t onceToken;

    dispatch_once(&onceToken, ^{
        mStack = [[self alloc] init];
    });

    return mStack;
}

/**
    @note
      This function can only be called indirectly via sharedStack
 */
- (instancetype)init
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    WDM_LOG_METHOD_SIG();

    self = [super init];
    VerifyOrExit((self), err = WEAVE_ERROR_NO_MEMORY);

    _currentState = kWeaveStack_NotInitialized;

    // create a serial work queue for all direct Weave operations
    _mWorkQueue = dispatch_queue_create("com.nestlabs.queue.weave.work", DISPATCH_QUEUE_SERIAL);
    VerifyOrExit((_mWorkQueue), err = WEAVE_ERROR_NO_MEMORY);

    // create a serial work queue for all the select loop to wait on
    _mSelectQueue = dispatch_queue_create("com.nestlabs.queue.weave.select", DISPATCH_QUEUE_SERIAL);
    VerifyOrExit((_mSelectQueue), err = WEAVE_ERROR_NO_MEMORY);

    _mIsWaitingOnSelect = false;

    _currentState = kWeaveStack_QueueInitialized;

exit:
    id result = nil;
    if (WEAVE_NO_ERROR == err) {
        result = self;
    } else {
        if (self) {
            if (_mWorkQueue) {
                _mWorkQueue = nil;
            }

            if (_mSelectQueue) {
                _mSelectQueue = nil;
            }
        }

        // ErrorStr uses more than one global resources which cannot be safely accessed
        // from other threads without locking, so we just log the number here
        WDM_LOG_ERROR(@"Error in init : %d\n", err);
    }
    return result;
}

/**
    @note
      This function can only be called indirectly via InitStack, within the Weave workqueue.
 */
- (WEAVE_ERROR)InitStack_internal:(NSString *)listenAddr bleDelegate:(NLWeaveBleDelegate *)bleDelegate
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    nl::Weave::WeaveMessageLayer::InitContext initContext;

    VerifyOrExit((kWeaveStack_QueueInitialized == self.currentState), err = WEAVE_ERROR_INCORRECT_STATE);

    self.currentState = kWeaveStack_Initializing;

    _mIsWaitingOnSelect = false;

    // Initialize the System::Layer object
    err = _mSystemLayer.Init(NULL);
    SuccessOrExit(err);

    // Initialize the InetLayer object.
    err = _mInetLayer.Init(_mSystemLayer, NULL);
    SuccessOrExit(err);

    // Initialize the FabricState object.
    err = _mFabricState.Init();
    SuccessOrExit(err);

    // TODO: TEMPORARY HACK -- use a different default node id to avoid conflict with the mock device.
    _mFabricState.LocalNodeId = 2;

    // Configure the weave listening address, if one was provided
    {
        if (listenAddr != NULL) {
#if WEAVE_CONFIG_ENABLE_TARGETED_LISTEN
            const char * listenAddrStr = ([listenAddr length] != 0) ? [listenAddr UTF8String] : NULL;

            nl::Inet::IPAddress addr;
            if (nl::Inet::IPAddress::FromString(listenAddrStr, addr)) {
                _mFabricState.ListenIPv4Addr = addr;
            }
#else
            WDM_LOG_ERROR(@"Error in InitStack: targeted listening is not enabled");
            ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
#endif
        }
    }

#if CONFIG_NETWORK_LAYER_BLE
    if (nil != bleDelegate) {
        // retain the BLE Delegate provided by the application layer
        _mBleDelegate = bleDelegate;
        initContext.listenBLE = true;
    } else {
        _mBleDelegate = [[NLWeaveBleDelegate alloc] initDummyDelegate];
        initContext.listenBLE = false;
    }

    // Initialize the BleLayer
    err = _mBleLayer.Init([_mBleDelegate GetPlatformDelegate], [_mBleDelegate GetApplicationDelegate], &_mSystemLayer);
    SuccessOrExit(err);

    [_mBleDelegate SetBleLayer:&_mBleLayer];

    initContext.ble = &_mBleLayer;
#endif

    // Initialize the WeaveMessageLayer object.
    initContext.systemLayer = &_mSystemLayer;
    initContext.inet = &_mInetLayer;
    initContext.fabricState = &_mFabricState;
    initContext.listenTCP = false;
    initContext.listenUDP = true;
    err = _mMessageLayer.Init(&initContext);
    SuccessOrExit(err);

    // Initialize the Exchange Manager object.
    err = _mExchangeMgr.Init(&_mMessageLayer);
    SuccessOrExit(err);

    // Initialize the Security Manager object.
    err = _mSecurityMgr.Init(_mExchangeMgr, _mSystemLayer);
    SuccessOrExit(err);

    err = SubscriptionEngine::GetInstance()->Init(&_mExchangeMgr, NULL, NULL);
    SuccessOrExit(err);

    self.currentState = kWeaveStack_FullyInitialized;

    [self TryProcessNetworkEvents];

exit:
    if (WEAVE_NO_ERROR != err) {
        WDM_LOG_ERROR(@"Error in InitStack_internal : (%d) %@\n", err, [NSString stringWithUTF8String:nl::ErrorStr(err)]);
    }

    return err;
}

/**
    @note
      This function can be called from any thread/queue at any time
 */
- (WEAVE_ERROR)InitStack:(NSString *)listenAddr bleDelegate:(NLWeaveBleDelegate *)bleDelegate
{
    // initialize the stack
    WDM_LOG_METHOD_SIG();

    __block WEAVE_ERROR result = WEAVE_NO_ERROR;

    dispatch_sync(_mWorkQueue, ^(void) {
        result = [self InitStack_internal:listenAddr bleDelegate:bleDelegate];
    });

    if (WEAVE_NO_ERROR != result) {
        // ErrorStr uses more than one global resources which cannot be safely accessed
        // from other threads without locking, so we just log the number here
        WDM_LOG_ERROR(@"Error in InitStack : %d\n", result);
    }

    return result;
}

/**
    @note
      This function can only be called indirectly via TryProcessNetworkEvents, within the Weave workqueue.
 */
- (void)ShutdownStack_Stage2
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    ShutdownCompletionBlock block = _mShutdownCompletionBlock;

    WDM_LOG_METHOD_SIG();

    _mShutdownCompletionBlock = nil;

    WDM_LOG_DEBUG(@"Shutdown Security Manager\n");
    _mSecurityMgr.Shutdown();

    WDM_LOG_DEBUG(@"Shutdown Exchange Manager\n");
    _mExchangeMgr.Shutdown();

    WDM_LOG_DEBUG(@"Shutdown Message Layer\n");
    _mMessageLayer.Shutdown();

#if CONFIG_NETWORK_LAYER_BLE
    WDM_LOG_DEBUG(@"Shutdown BLE Layer\n");
    _mBleLayer.Shutdown();
#endif

    WDM_LOG_DEBUG(@"Shutdown Fabric State\n");
    _mFabricState.Shutdown();

    WDM_LOG_DEBUG(@"Shutdown Inet Layer\n");
    _mInetLayer.Shutdown();

    WDM_LOG_DEBUG(@"Shutdown Weave System Layer\n");
    _mSystemLayer.Shutdown();

    WDM_LOG_DEBUG(@"Shutdown completed\n");

    self.currentState = kWeaveStack_QueueInitialized;

    if (WEAVE_NO_ERROR != err) {
        WDM_LOG_ERROR(@"Error in ShutdownStack_Stage2 : (%d) %@\n", err, [NSString stringWithUTF8String:nl::ErrorStr(err)]);
    }

    // inform application layer about the completion, if so desired
    if (block) {
        block(err);
    }
}

/**
    @note
      This function can only be called indirectly via ShutdownStack, within the Weave workqueue.
 */
- (void)ShutdownStack_Stage1
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    WDM_LOG_METHOD_SIG();

    switch (self.currentState) {
    case kWeaveStack_Initializing:
    case kWeaveStack_FullyInitialized:
        // continue shutting down
        break;
    case kWeaveStack_QueueInitialized:
        // do nothing and just return without error
        ExitNow();
    default:
        // abort shutting down
        ExitNow(err = WEAVE_ERROR_INCORRECT_STATE);
    }

    self.currentState = kWeaveStack_ShuttingDown;

    if (_mIsWaitingOnSelect) {
        // Inform the select queue that it's time to leave
        _mSystemLayer.WakeSelect();
    } else {
        // invoke stage 2 directly, as the select queue is not active (probably the initialization failed)
        [self ShutdownStack_Stage2];
    }

exit:
    if (WEAVE_NO_ERROR != err) {
        WDM_LOG_ERROR(@"Error in ShutdownStack_Stage1 : (%d) %@\n", err, [NSString stringWithUTF8String:nl::ErrorStr(err)]);

        _mShutdownCompletionBlock(err);
    }
}

/**
    @note
      This function can be called from any thread/queue at any time
 */
- (void)ShutdownStack:(ShutdownCompletionBlock)block;
{
    WEAVE_ERROR result = WEAVE_NO_ERROR;

    // release all C/C++ resources at here, if possible
    WDM_LOG_METHOD_SIG();

    // We're dispatching using sync on a serial queue, so it's safe to assume
    // everything dispatched earlier has been finished
    dispatch_sync(_mWorkQueue, ^(void) {
        _mShutdownCompletionBlock = block;
        [self ShutdownStack_Stage1];
    });

    if (WEAVE_NO_ERROR != result) {
        // ErrorStr uses more than one global resources which cannot be safely accessed
        // from other threads without locking, so we just log the number here
        WDM_LOG_ERROR(@"Error in ShutdownStack : %d\n", result);
    }
}

/**
    @note
      This function can only be called indirectly via initStack, within the Weave workqueue.
 */
- (void)TryProcessNetworkEvents
{
    __block struct timeval sleepTime;
    __block fd_set readFDs, writeFDs, exceptFDs;
    int MaxNumberedFdPlusOne = 0;
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // WDM_LOG_METHOD_SIG();

    VerifyOrExit(kWeaveStack_FullyInitialized == self.currentState, err = WEAVE_ERROR_INCORRECT_STATE);

    FD_ZERO(&readFDs);
    FD_ZERO(&writeFDs);
    FD_ZERO(&exceptFDs);

    sleepTime.tv_sec = 10;
    sleepTime.tv_usec = 0;

    // Collect the currently active file descriptors.
    _mSystemLayer.PrepareSelect(MaxNumberedFdPlusOne, &readFDs, &writeFDs, &exceptFDs, sleepTime);
    _mInetLayer.PrepareSelect(MaxNumberedFdPlusOne, &readFDs, &writeFDs, &exceptFDs, sleepTime);

    // WDM_LOG_DEBUG(@"Sleeping for %f sec.\n", sleepTime.tv_sec + (sleepTime.tv_usec / 1000000.0));

    _mIsWaitingOnSelect = true;

    // need this bracket to use the VerifyOrExit macro
    {
        dispatch_async(_mSelectQueue, ^(void) {
            // Wait for for I/O or for the next timer to expire.
            // Note that this is not a good practice to use with GCD, but it's
            int selectRes = select(MaxNumberedFdPlusOne, &readFDs, &writeFDs, &exceptFDs, &sleepTime);

            dispatch_async(_mWorkQueue, ^(void) {
                _mIsWaitingOnSelect = false;

                if (kWeaveStack_ShuttingDown == self.currentState) {
                    WDM_LOG_DEBUG(@"Select queue woke up but we're shutting down\n");

                    // continue on the 2nd stage of shutting down
                    [self ShutdownStack_Stage2];
                } else if (kWeaveStack_FullyInitialized == self.currentState) {
                    // Perform I/O and/or dispatch timers.
                    _mSystemLayer.HandleSelectResult(selectRes, &readFDs, &writeFDs, &exceptFDs);
                    _mInetLayer.HandleSelectResult(selectRes, &readFDs, &writeFDs, &exceptFDs);

                    // It's wierd that with iOS 9 SDK, we have to use disaptch_after here,
                    // instead of directly calling TryProcessNetworkEvents nor dispatch_async.
                    // If not, the memory usage skyrockets like there is memory leak
                    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 1 * NSEC_PER_MSEC), _mWorkQueue, ^(void) {
                        [self TryProcessNetworkEvents];
                    });
                } else {
                    WDM_LOG_ERROR(@"Select queue woke up in unexpected state\n");
                }
            });
        });
    }

exit:
    if (WEAVE_NO_ERROR != err) {
        // ErrorStr uses more than one global resources which cannot be safely accessed
        // from other threads without locking, so we just log the number here
        WDM_LOG_ERROR(@"Error in TryProcessNetworkEvents : %d\n", err);
    }
}

- (NLWeaveDeviceManager *)createDeviceManager:(NSString *)name appCallbackQueue:(dispatch_queue_t)appCallbackQueue
{
    WDM_LOG_METHOD_SIG();

    _mDeviceMgr = [[NLWeaveDeviceManager alloc] init:name
                                                     weaveWorkQueue:_mWorkQueue
                                                   appCallbackQueue:appCallbackQueue
                                                        exchangeMgr:&_mExchangeMgr
                                                        securityMgr:&_mSecurityMgr];
    if (nil == _mDeviceMgr) {
        WDM_LOG_ERROR(@"Cannot create new NLWeaveDeviceManager\n");
    }
    return _mDeviceMgr;
}

- (NLWdmClient *)createWdmClient:(NSString *)name appCallbackQueue:(dispatch_queue_t)appCallbackQueue
{
    WDM_LOG_METHOD_SIG();

    NLWdmClient * _mWdmClient = [[NLWdmClient alloc] init:name
                                         weaveWorkQueue:_mWorkQueue
                                         appCallbackQueue:appCallbackQueue
                                         exchangeMgr:&_mExchangeMgr
                                         messageLayer:&_mMessageLayer
                                         nlWeaveDeviceManager:_mDeviceMgr];
    if (nil == _mWdmClient) {
        WDM_LOG_ERROR(@"Cannot create new NLWdmClient\n");
    }
    return _mWdmClient;
}
@end

namespace nl {
namespace Weave {
namespace Platform {
namespace PersistedStorage {
WEAVE_ERROR Read(const char *aKey, uint32_t &aValue)
{
    return WEAVE_NO_ERROR;
}

WEAVE_ERROR Write(const char *aKey, uint32_t aValue)
{
    return WEAVE_NO_ERROR;
}

} // PersistentStorage
} // Platform
} // Weave
} // nl

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
