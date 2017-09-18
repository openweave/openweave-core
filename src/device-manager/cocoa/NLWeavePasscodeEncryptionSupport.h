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
 *      This file defines a Wrapper for C++ implementation of pincode encryption/decryption functionality.
 *      for pin encryption.
 *
 */

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

extern NSString *const NLPasscodeEncryptionSupportDomain;

// Error codes for NLPasscodeEncryptionSupportDomain
typedef NS_ENUM (NSInteger, NLPasscodeEncryptionSupportDomainCode){
    NLPasscodeEncryptionSupportDomainSuccess = 0,
    NLPasscodeEncryptionSupportDomainEncryptionFailure = 1,
    NLPasscodeEncryptionSupportDomainDecryptionFailure = 2,
    NLPasscodeEncryptionSupportDomainInvalidData = 3,
    NLPasscodeEncryptionSupportDomainInvalidEncKeySize = 4,
    NLPasscodeEncryptionSupportDomainInvalidAuthKeySize = 5,
    NLPasscodeEncryptionSupportDomainInvalidFingerprintKeySize = 6,
};

/** Passcode encryption configuration 1 (TEST ONLY)
 *
 * Note: This encryption configuration is for testing only and provides no integrity or confidentiality.
 * Config 1 is only available in development builds.
 */
extern NSUInteger const NLWeavePasscode_Config1_TEST_ONLY;

/** Passcode encryption configuration 2
 */
extern NSUInteger const NLWeavePasscode_Config2;

/** Key diversifier used in the derivation of the passcode encryption and authentication keys.
 */
extern UInt8 const NLWeavePasscodeEncKeyDiversifier [];

/** Key diversifier used in the derivation of the passcode fingerprint key.
 */
extern UInt8 const NLWeavePasscodeFingerprintKeyDiversifier [];

extern UInt32 const NLWeavePasscodeEncKeyDiversifierSize;
extern UInt32 const NLWeavePasscodeFingerprintKeyDiversifierSize;

extern UInt32 const NLWeavePasscodeEncryptionKeyLen;
extern UInt32 const NLWeavePasscodeAuthenticationKeyLen;
extern UInt32 const NLWeavePasscodeFingerprintKeyLen;

/**
 * @class wrapper for C++ implementation of pincode encryption/decryption functionality.
 */
@interface NLWeavePasscodeEncryptionSupport : NSObject
/** Encrypt a passcode using the Nest Passcode Encryption scheme.
 */
+ (nullable NSData *) encryptPasscode: (UInt8) config
                       keyId: (UInt32) keyId
                       nonce: (UInt32) nonce
                    passcode: (NSData *) passcode
                      encKey: (NSData *) encKey
                     authKey: (NSData *) authKey
              fingerprintKey: (NSData *) fingerprintKey
                       error: (NSError **) errOut;

/** Decrypt a passcode that was encrypted using the Nest Passcode Encryption scheme.
 */
+ (nullable NSData *) decryptPasscode: (NSData *) encPasscode
                               config: (UInt8) config
                               encKey: (NSData *) encKey
                              authKey: (NSData *) authKey
                       fingerprintKey: (NSData *) fingerprintKey
                                error: (NSError **) errOut;

/** Determines if the specified Passcode encryption configuration is supported.
 */
+ (BOOL) isSupportedPasscodeEncryptionConfig: (UInt8) config;

/** Extract the configuration type from an encrypted Passcode.
 */
+ (BOOL) getEncryptedPasscodeConfig: (NSData *) encPasscode
                             config: (UInt8 *) configOut
                              error: (NSError **) errOut;

/** Extract the key id from an encrypted Passcode.
 */
+ (BOOL) getEncryptedPasscodeKeyId: (NSData *) encPasscode
                             keyId: (UInt32 *) keyIdOut
                             error: (NSError **) errOut;

/** Extract the nonce value from an encrypted Passcode.
 */
+ (BOOL) getEncryptedPasscodeNonce: (NSData *) encPasscode
                             nonce: (UInt32 *) nonceOut
                             error: (NSError **) errOut;

/** Extract the fingerprint from an encrypted Passcode.
 */
+ (nullable NSData *) getEncryptedPasscodeFingerprint: (NSData *) encPasscode error: (NSError **) errOut;


@end
NS_ASSUME_NONNULL_END
