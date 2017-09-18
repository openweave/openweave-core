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
 *      This file implements a Wrapper for C++ implementation of key derivation functionality
 *      for pin encryption.
 *
 */

#import "NLHKDF.h"

#include <Weave/Support/NLDLLUtil.h>
#include <Weave/Core/WeaveCore.h>
#include <Weave/Support/crypto/HMAC.h>
#include <Weave/Support/crypto/HashAlgos.h>
#include <Weave/Support/crypto/HKDF.h>

NSString *const NLHKDFErrorDomain = @"NLHKDFErrorDomain";

@implementation NLHKDF

+ (NSData *) deriveKey: (NSString *) alg
                  salt: (NSData *) salt
          keyMaterial1: (NSData *) keyMaterial1
          keyMaterial2: (NSData *) keyMaterial2
                  info: (NSData *) info
       requestedKeyLen: (NSUInteger) requestedKeyLen
                 error: (NSError **) errOut {
    
    if (![alg isEqualToString:@"HKDFSHA1" ]) {
        if (errOut) {
            *errOut = [NSError errorWithDomain:NLHKDFErrorDomain
                                          code:NLHKDFErrorDomainAlgNotSupported userInfo:nil];
        }
        return nil;
    }
    
    NSMutableData * data = [[NSMutableData alloc] initWithLength:requestedKeyLen];
    
    WEAVE_ERROR err = nl::Weave::Crypto::HKDFSHA1::DeriveKey(
                                      (salt != nil) ? (unsigned char *) [salt bytes] : NULL,
                                      (salt != nil) ? [salt length] : 0,
                                        (unsigned char *) [keyMaterial1 bytes],
                                        [keyMaterial1 length],
                                        (unsigned char *) [keyMaterial2 bytes],
                                        [keyMaterial2 length],
                                        (unsigned char *) [info bytes],
                                        [info length],
                                        (unsigned char *) [data mutableBytes],
                                        requestedKeyLen,
                                        requestedKeyLen);
    
    if (err != WEAVE_NO_ERROR){
        if (err == WEAVE_ERROR_BUFFER_TOO_SMALL) {
            *errOut = [NSError errorWithDomain:NLHKDFErrorDomain
                                          code:NLHKDFErrorDomainKeyBuffSizeInsufficient userInfo:nil];
        } else if (err == WEAVE_ERROR_INVALID_ARGUMENT) {
            *errOut = [NSError errorWithDomain:NLHKDFErrorDomain
                                          code:NLHKDFErrorDomainKeyInvalidRequestedKeySize userInfo:nil];
        } else {
            NSString *failureReason = [NSString stringWithFormat:NSLocalizedString(@"DeriveKey error: %d", @""), err];
            NSDictionary *userInfo = @{ NSLocalizedFailureReasonErrorKey: failureReason };
            *errOut = [NSError errorWithDomain:NLHKDFErrorDomain
                                          code:NLHKDFErrorDomainDeriveKeyFailure userInfo: userInfo];
        }
        return nil;
    }
    
    return data;
}

@end
