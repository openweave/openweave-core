/*
 *
 *    Copyright (c) 2015-2017 Nest Labs, Inc.
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
 *      This file provides defines NLWeaveStack interface
 *
 */

#import <CoreBluetooth/CoreBluetooth.h>

#import "NLWeaveErrorCodes.h"

@class NLWeaveBleDelegate;
@class NLWeaveDeviceManager;
@class NLWdmClient;

typedef NS_ENUM(NSInteger, EWeaveStackState) {
    kWeaveStack_NotInitialized = 0,
    kWeaveStack_QueueInitialized,
    kWeaveStack_Initializing,
    kWeaveStack_FullyInitialized,
    kWeaveStack_ShuttingDown,
};

typedef void (^ShutdownCompletionBlock)(WEAVE_ERROR result);

@interface NLWeaveStack : NSObject

@property (atomic) EWeaveStackState currentState;

@property (atomic, readonly) dispatch_queue_t WorkQueue;

@property (atomic, strong, readonly) NLWeaveBleDelegate * BleDelegate;

/**
 @note This is a singleton. There is no way to have more than one instances at any time.

 @return id of the shared Weave stack instance
 */
+ (instancetype)sharedStack;

- (WEAVE_ERROR)InitStack:(NSString *)listenAddr bleDelegate:(NLWeaveBleDelegate *)bleDelegate;

- (void)ShutdownStack:(ShutdownCompletionBlock)block;

- (NLWeaveDeviceManager *)createDeviceManager:(NSString *)name appCallbackQueue:(dispatch_queue_t)appCallbackQueue;

- (NLWdmClient *)createWdmClient:(NSString *)name appCallbackQueue:(dispatch_queue_t)appCallbackQueue;
@end
