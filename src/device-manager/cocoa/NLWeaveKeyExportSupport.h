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
 *      This file defines a Wrapper for C++ implementation of key export functionality
 *      to support pin encryption.
 *
 */

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

extern NSString *const NLWeaveKeyExportSupportErrorDomain;

// Error codes for NLWeaveKeyExportClientErrorDomain
typedef NS_ENUM (NSInteger, NLWeaveKeyExportSupportErrorDomainCode) {
     NLWeaveKeyExportSupportErrorDomainSimulateKeyExportFailure = 2,
     NLWeaveKeyExportSupportErrorDomainInvalidArgument = 3
};

/**
 *  Provides utility functions for testing Weave key export (for keystore in mobile-iOS tree)
 */
@interface NLWeaveKeyExportSupport : NSObject

+ (nullable NSData *) simulateDeviceKeyExport: (NSData *) keyExportReq
                                   deviceCert: (NSData *) responderNodeId
                                devicePrivKey: (NSData *) accessToken
                                trustRootCert: (NSData *) trustRootCert
                                   isReconfig: (BOOL *) isReconfig
                                        error: (NSError **) errOut;


@end
NS_ASSUME_NONNULL_END
