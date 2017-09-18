/*
 *
 *    Copyright (c) 2016-2017 Nest Labs, Inc.
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
 *      This file defines a Wrapper for C++ implementation of key derivation functionality
 *      for pin encryption.
 *
 */

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

extern NSString *const NLHKDFErrorDomain;

// Error codes for NLKeyStoreErrorDomain
typedef NS_ENUM (NSInteger, NLHKDFErrorDomainCode){
    NLHKDFErrorDomainAlgNotSupported = 2,
    NLHKDFErrorDomainKeyBuffSizeInsufficient = 3,
    NLHKDFErrorDomainKeyInvalidRequestedKeySize = 4,
    NLHKDFErrorDomainDeriveKeyFailure = 5
};

/**
 * @class Wrapper for C++ implementation of key derivation functionality
 */
@interface NLHKDF : NSObject

+ (nullable NSData *) deriveKey: (NSString *) alg
                  salt: (nullable NSData *) salt
          keyMaterial1: (NSData *) keyMaterial1
          keyMaterial2: (NSData *) keyMaterial2
                  info: (NSData *) info
       requestedKeyLen: (NSUInteger) requestedKeyLen
                 error: (NSError **) errOut;

@end
NS_ASSUME_NONNULL_END
