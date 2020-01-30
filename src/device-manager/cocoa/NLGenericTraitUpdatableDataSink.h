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

// ----- Error Logging -----
- (NSString *)toErrorString:(WEAVE_ERROR)err;

- (void)clear;

- (void)refreshData:(GenericTraitUpdatableDataSinkCompletionBlock)completionHandler                failure:(GenericTraitUpdatableDataSinkFailureBlock)failureHandler;

- (WEAVE_ERROR)setSigned:(int64_t)val
                    path:(NSString *)path
             conditional:(BOOL) isConditional;
- (WEAVE_ERROR)setUnsigned:(uint64_t)val
                      path:(NSString *)path
               conditional:(BOOL) isConditional;
- (WEAVE_ERROR)setDouble:(double)val
                    path:(NSString *)path
              conditional:(BOOL) isConditional;
- (WEAVE_ERROR)setBoolean:(BOOL) val
                     path:(NSString *)path
              conditional:(BOOL) isConditional;
- (WEAVE_ERROR)setString:(NSString *) val
                    path:(NSString *) path
             conditional:(BOOL) isConditional;
- (WEAVE_ERROR)setNull:(NSString *)path
           conditional:(BOOL) isConditional;
- (WEAVE_ERROR)setBytes:(NSData *) val
                   path:(NSString *) path
            conditional:(BOOL) isConditional;
- (WEAVE_ERROR)setStringArray:(NSArray*)stringArray
                         path:(NSString *) path
                  conditional:(BOOL) isConditional;
- (WEAVE_ERROR)setSigned:(int64_t)val
                    path:(NSString *)path;
- (WEAVE_ERROR)setUnsigned:(uint64_t)val
                      path:(NSString *)path;
- (WEAVE_ERROR)setDouble:(double)val
                    path:(NSString *)path;
- (WEAVE_ERROR)setBoolean:(BOOL) val
                     path:(NSString *)path;
- (WEAVE_ERROR)setString:(NSString *) val
                    path:(NSString *) path;
- (WEAVE_ERROR)setNull:(NSString *)path;
- (WEAVE_ERROR)setBytes:(NSData *) val
                   path:(NSString *) path;
- (WEAVE_ERROR)setStringArray:(NSArray*)stringArray
                         path:(NSString *) path;
- (WEAVE_ERROR)getSigned:(int64_t *)val
                    path:(NSString *)path;
- (WEAVE_ERROR)getUnsigned:(uint64_t *)val
                      path:(NSString *)path;
- (WEAVE_ERROR)getDouble:(double *)val
                    path:(NSString *)path;
- (WEAVE_ERROR)getBoolean:(BOOL *)val
                     path:(NSString *)path;
- (NSString *)getString:(NSString *)path;
- (NSData *)getBytes:(NSString *)path;
- (NSArray *)getStringArray:(NSString *)path;
- (WEAVE_ERROR)getVersion:(uint64_t *)val;

@end
