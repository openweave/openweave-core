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
 *      This file defines NLGenericUpdatableDataSink interface
 *
 */

#import <Foundation/Foundation.h>
#import "NLWeaveErrorCodes.h"

typedef void (^GenericTraitUpdatableDataSinkCompletionBlock)(id owner, id data);
typedef void (^GenericTraitUpdatableDataSinkFailureBlock)(id owner, NSError * error);

@interface NLGenericTraitUpdatableDataSink : NSObject
@property (copy, readonly) NSString * name;
@property (readonly) dispatch_queue_t resultCallbackQueue;
@property (weak) id owner;

/**
 *  @brief Disable default initializer inherited from NSObject
 */
- (instancetype)init NS_UNAVAILABLE;

/**
 *  @brief Forcifully release all resources and destroy all references.
 *
 */
- (void)Close:(GenericTraitUpdatableDataSinkCompletionBlock)completionHandler;

// ----- Error Logging -----
- (NSString *)toErrorString:(WEAVE_ERROR)err;

- (void)refreshDataCompletion:(GenericTraitUpdatableDataSinkCompletionBlock)completionHandler failure:(GenericTraitUpdatableDataSinkFailureBlock)failureHandler;

- (WEAVE_ERROR)SetSignedOnPath:(NSString *)path
                    Value:(int64_t)val
               Conditionality:(BOOL) isConditional;
- (WEAVE_ERROR)SetUnsignedOnPath:(NSString *)path
                         Value:(uint64_t)val
                Conditionality:(BOOL) isConditional;
- (WEAVE_ERROR)SetDoubleOnPath:(NSString *)path
                       Value:(double)val
              Conditionality:(BOOL) isConditional;
- (WEAVE_ERROR)SetBooleanOnPath:(NSString *)path
                        Value:(BOOL) val
               Conditionality:(BOOL) isConditional;
- (WEAVE_ERROR)SetStringOnPath:(NSString *) path
                       Value:(NSString *) val
               Conditionality:(BOOL) isConditional;
- (WEAVE_ERROR)SetNULLOnPath:(NSString *)path
               Conditionality:(BOOL) isConditional;
- (WEAVE_ERROR)SetBytesOnPath:(NSString *) path
                       Value:(NSData *) val
               Conditionality:(BOOL) isConditional;

- (WEAVE_ERROR)SetSignedOnPath:(NSString *)path
                    Value:(int64_t)val;
- (WEAVE_ERROR)SetUnsignedOnPath:(NSString *)path
                         Value:(uint64_t)val;
- (WEAVE_ERROR)SetDoubleOnPath:(NSString *)path
                       Value:(double)val;
- (WEAVE_ERROR)SetBooleanOnPath:(NSString *)path
                        Value:(BOOL) val;
- (WEAVE_ERROR)SetStringOnPath:(NSString *) path
                       Value:(NSString *) val;
- (WEAVE_ERROR)SetNULLOnPath:(NSString *)path;
- (WEAVE_ERROR)SetBytesOnPath:(NSString *) path
                       Value:(NSData *) val;

- (WEAVE_ERROR)GetInt:(int64_t *)val
               OnPath:(NSString *)path;
- (WEAVE_ERROR)GetUnsigned:(uint64_t *)val
                    OnPath:(NSString *)path;
- (WEAVE_ERROR)GetDouble:(double *)val
                    OnPath:(NSString *)path;
- (WEAVE_ERROR)GetBoolean:(BOOL *)val
                    OnPath:(NSString *)path;
- (NSString *)GetStringOnPath:(NSString *)path;
- (NSData *)GetBytesOnPath:(NSString *)path;
- (WEAVE_ERROR)GetVersion:(uint64_t *)val;

@end
