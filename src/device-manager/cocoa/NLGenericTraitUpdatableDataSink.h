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

/**
 * convert weave error to string
 */
- (NSString *)toErrorString:(WEAVE_ERROR)err;

/**
 * clear trait data
 */
- (void)clear;

/**
 * Begins a sync of the trait data. The result of this operation can be observed through the CompletionHandler
 * and failureHandler
 */
- (void)refreshData:(GenericTraitUpdatableDataSinkCompletionBlock)completionHandler                failure:(GenericTraitUpdatableDataSinkFailureBlock)failureHandler;

/**
 * Assigns the provided value to the given path as a signed integer value.
 *
 * @param path the proto path to the property to modify
 * @param val the int64_t value to assign to the property
 * @param isConditional whether or not to allow overwriting any conflicting changes. If true, then if a later
 *     version of the trait has modified this property and does not equal to required version from update,
 *     this update will be dropped; otherwise, this value will overwrite the newer change
 */
- (WEAVE_ERROR)setSigned:(int64_t)val
                    path:(NSString *)path
             conditional:(BOOL) isConditional;

/**
 * Assigns the provided value to the given path as an unsigned integer value.
 *
 * @param path the proto path to the property to modify
 * @param val the uint64_t value to assign to the property
 * @param isConditional whether or not to allow overwriting any conflicting changes. If true, then if a later
 *     version of the trait has modified this property and does not equal to required version from update,
 *     this update will be dropped; otherwise, this value will overwrite the newer change
 */
- (WEAVE_ERROR)setUnsigned:(uint64_t)val
                      path:(NSString *)path
               conditional:(BOOL) isConditional;

/**
 * Assigns the provided value to the given path.
 *
 * @param path the proto path to the property to modify
 * @param val the double value to assign to the property
 * @param isConditional whether or not to allow overwriting any conflicting changes. If true, then if a later
 *     version of the trait has modified this property and does not equal to required version from update,
 *     this update will be dropped; otherwise, this value will overwrite the newer change
 */
- (WEAVE_ERROR)setDouble:(double)val
                    path:(NSString *)path
              conditional:(BOOL) isConditional;

/**
 * Assigns the provided value to the given path.
 *
 * @param path the proto path to the property to modify
 * @param val the boolean value to assign to the property
 * @param isConditional whether or not to allow overwriting any conflicting changes. If true, then if a later
 *     version of the trait has modified this property and does not equal to required version from update,
 *     this update will be dropped; otherwise, this value will overwrite the newer change
 */
- (WEAVE_ERROR)setBoolean:(BOOL) val
                     path:(NSString *)path
              conditional:(BOOL) isConditional;

/**
 * Assigns the provided value to the given path.
 *
 * @param path the proto path to the property to modify
 * @param val the string value to assign to the property
 * @param isConditional whether or not to allow overwriting any conflicting changes. If true, then if a later
 *     version of the trait has modified this property and does not equal to required version from update,
 *     this update will be dropped; otherwise, this value will overwrite the newer change
 */
- (WEAVE_ERROR)setString:(NSString *) val
                    path:(NSString *) path
             conditional:(BOOL) isConditional;

/**
 * Assigns Null to the given path.
 *
 * @param path the proto path to the property to modify
 * @param isConditional whether or not to allow overwriting any conflicting changes. If true, then if a later
 *     version of the trait has modified this property and does not equal to required version from update,
 *     this update will be dropped; otherwise, this value will overwrite the newer change
 */
- (WEAVE_ERROR)setNull:(NSString *)path
           conditional:(BOOL) isConditional;

/**
 * Assigns the provided value to the given path.
 *
 * @param path the proto path to the property to modify
 * @param val the bytes value to assign to the property
 * @param isConditional whether or not to allow overwriting any conflicting changes. If true, then if a later
 *     version of the trait has modified this property and does not equal to required version from update,
 *     this update will be dropped; otherwise, this value will overwrite the newer change
 */
- (WEAVE_ERROR)setBytes:(NSData *) val
                   path:(NSString *) path
            conditional:(BOOL) isConditional;

/**
 * Assigns the provided value to the given path.
 *
 * @param path the proto path to the property to modify
 * @param val the string array value to assign to the property
 * @param isConditional whether or not to allow overwriting any conflicting changes. If true, then if a later
 *     version of the trait has modified this property and does not equal to required version from update,
 *     this update will be dropped; otherwise, this value will overwrite the newer change
 */
- (WEAVE_ERROR)setStringArray:(NSArray*)stringArray
                         path:(NSString *) path
                  conditional:(BOOL) isConditional;

/**
 * Assigns the provided value to the given path as a signed integer value with unconditional capability
 *
 * @param path the proto path to the property to modify
 * @param val the int64_t value to assign to the property
 */
- (WEAVE_ERROR)setSigned:(int64_t)val
                    path:(NSString *)path;

/**
 * Assigns the provided value to the given path as a signed integer value with unconditional capability
 *
 * @param path the proto path to the property to modify
 * @param val the uint64_t value to assign to the property
 */
- (WEAVE_ERROR)setUnsigned:(uint64_t)val
                      path:(NSString *)path;

/**
 * Assigns the provided value to the given path as a signed integer value with unconditional capability
 *
 * @param path the proto path to the property to modify
 * @param val the double value to assign to the property
 */
- (WEAVE_ERROR)setDouble:(double)val
                    path:(NSString *)path;

/**
 * Assigns the provided value to the given path as a signed integer value with unconditional capability
 *
 * @param path the proto path to the property to modify
 * @param val the boolean value to assign to the property
 */
- (WEAVE_ERROR)setBoolean:(BOOL) val
                     path:(NSString *)path;

/**
 * Assigns the provided value to the given path as a signed integer value with unconditional capability
 *
 * @param path the proto path to the property to modify
 * @param val the String value to assign to the property
 */
- (WEAVE_ERROR)setString:(NSString *) val
                    path:(NSString *) path;

/**
 * Assigns Null to the given path with unconditional capability
 *
 * @param path the proto path to the property to modify
 */
- (WEAVE_ERROR)setNull:(NSString *)path;

/**
 * Assigns the provided value to the given path with unconditional capability
 *
 * @param path the proto path to the property to modify
 * @param val the bytes value to assign to the property
 */
- (WEAVE_ERROR)setBytes:(NSData *) val
                   path:(NSString *) path;

/**
 * Assigns the provided value to the given path with unconditional capability
 *
 * @param path the proto path to the property to modify
 * @param val the string array to assign to the property
 */
- (WEAVE_ERROR)setStringArray:(NSArray*)stringArray
                         path:(NSString *) path;

/**
 * Get the int64_t value assigned to the property at the given path within this trait.
 */
- (WEAVE_ERROR)getSigned:(int64_t *)val
                    path:(NSString *)path;

/**
 * Get the uint64_t value assigned to the property at the given path within this trait.
 */
- (WEAVE_ERROR)getUnsigned:(uint64_t *)val
                      path:(NSString *)path;

/**
 * Get the double value assigned to the property at the given path within this trait.
 */
- (WEAVE_ERROR)getDouble:(double *)val
                    path:(NSString *)path;

/**
 * Get the boolean value assigned to the property at the given path within this trait.
 */
- (WEAVE_ERROR)getBoolean:(BOOL *)val
                     path:(NSString *)path;

/**
 * Get the string value assigned to the property at the given path within this trait.
 */
- (NSString *)getString:(NSString *)path;

/**
 * Get the bytes value assigned to the property at the given path within this trait.
 */
- (NSData *)getBytes:(NSString *)path;

/**
 * Check if null property at the given path within this trait.
 */
- (WEAVE_ERROR)isNull:(BOOL *)val
                     path:(NSString *)path;

/**
 * Get the string array value assigned to the property at the given path within this trait.
 */
- (NSArray *)getStringArray:(NSString *)path;

/** Returns the version of the trait represented by this data sink. */
- (WEAVE_ERROR)getVersion:(uint64_t *)val;

@end
