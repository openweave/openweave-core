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
 *      This file defines NLWdmClient interface.
 *      This is WEAVE_CONFIG_DATA_MANAGEMENT_EXPERIMENTAL feature.
 *
 */

#import <Foundation/Foundation.h>
#import "NLWeaveErrorCodes.h"
#import "NLResourceIdentifier.h"
#import "NLGenericTraitUpdatableDataSink.h"

typedef void (^WdmClientCompletionBlock)(id owner, id data);
typedef void (^WdmClientFailureBlock)(id owner, NSError * error);

@interface NLWdmClient : NSObject
@property (copy, readonly) NSString * name;
@property (readonly) dispatch_queue_t resultCallbackQueue;
@property (weak) id owner;

/**
 *  @brief Disable default initializer inherited from NSObject
 */
- (instancetype)init NS_UNAVAILABLE;

/**
 *  @brief Forcefully release all resources and destroy all references.
 *
 *  There is no way to revive this WDMClient after this call.
 */
- (void)close:(WdmClientCompletionBlock)completionHandler;

// ----- Error Logging -----
- (NSString *)toErrorString:(WEAVE_ERROR)err;

/**
 * set Weave node ID in trait catalog in WdmClient
 *
 * @param nodeId Weave node ID
 *
 */
- (void)setNodeId:(uint64_t)nodeId;

/**
 * Create the new data newDataSink
 *
 * @param nlResourceIdentifier resource id is a globally-unique identifier for a Weave resource
 * @param profileId trait profile id
 * @param instanceId trait instance id
 * @param path trait path
 *
 */
- (NLGenericTraitUpdatableDataSink *)newDataSink:(NLResourceIdentifier *)nlResourceIdentifier
                                       profileId:(uint32_t)profileId
                                      instanceId:(uint64_t)instanceId
                                            path:(NSString *)path;

/**
 * Begins a flush of all trait data. The result of this operation can be observed through the CompletionHandler and
 * failureHandler, when operation completes, onWdmClientFlushUpdateComplete is called, application would receive statusResultsList,
 * if it is empty, it means success without failed path, if anything inside, the array member could be NLWdmClientFlushUpdateError(local client error)
 * or NLWdmClientFlushUpdateDeviceStatus(remote device status), application can use the path and dataSink from the above member to clear
 * particular data or skip the error if necessary. When operation fails, it usually means the operation cannot complete at all, for example
 * communication or protocol issue, onWdmClientError would be called.
 */
- (void)flushUpdate:(WdmClientCompletionBlock)completionHandler failure:(WdmClientFailureBlock)failureHandler;

/**
 * Begins a sync of all trait data. The result of this operation can be observed through the CompletionHandler and
 * failureHandler
 */
- (void)refreshData:(WdmClientCompletionBlock)completionHandler failure:(WdmClientFailureBlock)failureHandler;

@end
