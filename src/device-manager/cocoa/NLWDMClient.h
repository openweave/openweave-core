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
 *      This file defines NLWDMClient interface
 *
 */

#import <Foundation/Foundation.h>
#import "NLWeaveErrorCodes.h"

typedef void (^WDMClientCompletionBlock)(id owner, id data);
typedef void (^WDMClientFailureBlock)(id owner, NSError * error);

@interface NLWDMClient : NSObject
@property (copy, readonly) NSString * name;
@property (readonly) dispatch_queue_t resultCallbackQueue;
@property (weak) id owner;

/**
 *  @brief Disable default initializer inherited from NSObject
 */
- (instancetype)init NS_UNAVAILABLE;

/**
 *  @brief Close all connections gracifully.
 *
 *  The device manager would be ready for another connection after completion.
 */
- (void)Close:(GenericTraitUpdatableDataSinkCompletionBlock)completionHandler failure:(GenericTraitUpdatableDataSinkFailureBlock)failureHandler;

/**
 *  @brief Forcifully release all resources and destroy all references.
 *
 *  There is no way to revive this device manager after this call.
 */
- (void)Shutdown:(GenericTraitUpdatableDataSinkCompletionBlock)completionHandler;


// ----- Error Logging -----
- (NSString *)toErrorString:(WEAVE_ERROR)err;
//-(NSError *)toError:(WEAVE_ERROR)err;

- (NLGenericTraitUpdatableDataSink *)newDataSinkResourceType: (uint16_t)resourceType
                                                  ResourceId: (NSString *)resourceId
                                                   ProfileId: (uint32_t)profileId
                                                  InstanceId: (uint64_t)instanceId
                                                        Path: (NSString *)path;

- (void)flushUpdateCompletion:(GenericTraitUpdatableDataSinkCompletionBlock)completionHandler
                           failure:(WDMClientFailureBlock)failureHandler;

- (void)refreshDataCompletion:(WDMClientCompletionBlock)completionHandler failure:(WDMClientFailureBlock)failureHandler;

- (void) removeDataSinkRef:(long long)traitInstancePtr;

@end
