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
 *      This file implements a Wrapper for C++ implementation of pincode encryption/decryption functionality.
 *      for pin encryption.
 *
 */

#import "NLWeavePasscodeEncryptionSupport.h"
#include <Weave/Profiles/security/WeavePasscodes.h>

using namespace nl::Weave::Profiles::Security::Passcodes;

NSUInteger const NLWeavePasscode_Config1_TEST_ONLY = kPasscode_Config1_TEST_ONLY;
NSUInteger const NLWeavePasscode_Config2 = kPasscode_Config2;

UInt8 const NLWeavePasscodeEncKeyDiversifier[] = { 0x1A, 0x65, 0x5D, 0x96 };
UInt32 const NLWeavePasscodeEncKeyDiversifierSize = sizeof(NLWeavePasscodeEncKeyDiversifier);
UInt8 const NLWeavePasscodeFingerprintKeyDiversifier[] = { 0xD1, 0xA1, 0xD9, 0x6C };
UInt32 const NLWeavePasscodeFingerprintKeyDiversifierSize = sizeof(NLWeavePasscodeFingerprintKeyDiversifier);

UInt32 const NLWeavePasscodeEncryptionKeyLen = kPasscodeEncryptionKeyLen;
UInt32 const NLWeavePasscodeAuthenticationKeyLen = kPasscodeAuthenticationKeyLen;
UInt32 const NLWeavePasscodeFingerprintKeyLen = kPasscodeFingerprintKeyLen;

NSString *const NLPasscodeEncryptionSupportDomain = @"NLPasscodeEncryptionSupportDomain";

@implementation NLWeavePasscodeEncryptionSupport

#pragma mark Public Methods

+ (NSData *) encryptPasscode: (UInt8) config
                                keyId: (UInt32) keyId
                                nonce: (UInt32) nonce
                             passcode: (NSData *) passcode
                               encKey: (NSData *) encKey
                              authKey: (NSData *) authKey
                       fingerprintKey: (NSData *) fingerprintKey
                       error: (NSError **) errOut {

    if (config != NLWeavePasscode_Config1_TEST_ONLY) {
        if (![NLWeavePasscodeEncryptionSupport validateKeySize: encKey
                                                       authKey: authKey
                                                fingerprintKey: fingerprintKey
                                                         error: errOut]) {
            return nil;
        }
    }
    
    NSMutableData * encPasscodeBuff = [[NSMutableData alloc] initWithLength: kPasscodeMaxEncryptedLen];
    size_t encPasscodeLen =0;
    
    WEAVE_ERROR err = EncryptPasscode(config, keyId, nonce,
                                      (unsigned char *) [passcode bytes],
                                      [passcode length],
                                      (unsigned char *) [encKey bytes],
                                      (unsigned char *) [authKey bytes],
                                      (unsigned char *) [fingerprintKey bytes],
                                      (unsigned char *) [encPasscodeBuff mutableBytes],
                                      [encPasscodeBuff length],
                                      encPasscodeLen);
    if (err != WEAVE_NO_ERROR){
        if (errOut) {
            NSString *failureReason = [NSString stringWithFormat:NSLocalizedString(@"EncryptPasscode error: %d", @""), err];
            NSDictionary *userInfo = @{ NSLocalizedFailureReasonErrorKey: failureReason };
        
            *errOut = [NSError errorWithDomain: NLPasscodeEncryptionSupportDomain
                                      code: NLPasscodeEncryptionSupportDomainEncryptionFailure
                                  userInfo: userInfo];
        }
        return nil;
    }
    
    [encPasscodeBuff setLength: encPasscodeLen];
    return encPasscodeBuff;
}

+ (nullable NSData *) decryptPasscode: (NSData *) encPasscode
                               config: (UInt8) config
                               encKey: (NSData *) encKey
                              authKey: (NSData *) authKey
                       fingerprintKey: (NSData *) fingerprintKey
                                error: (NSError **) errOut {
    
    NSMutableData * passcodeBuff = [[NSMutableData alloc] initWithLength: kPasscodePaddedLen];
    size_t passcodeLen =0;
    
    if (config != NLWeavePasscode_Config1_TEST_ONLY) {
        if (![NLWeavePasscodeEncryptionSupport validateKeySize: encKey
                                                       authKey: authKey
                                                fingerprintKey: fingerprintKey
                                                         error: errOut]) {
            return nil;
        }
    }
    
    WEAVE_ERROR err = DecryptPasscode((unsigned char *) [encPasscode bytes],
                                      [encPasscode length],
                                      (unsigned char *) [encKey bytes],
                                      (unsigned char *) [authKey bytes],
                                      (unsigned char *) [fingerprintKey bytes],
                                      (unsigned char *) [passcodeBuff mutableBytes],
                                      [passcodeBuff length],
                                      passcodeLen);
    if (err != WEAVE_NO_ERROR){
        
        NSString *failureReason = [NSString stringWithFormat:NSLocalizedString(@"DecryptPasscode error: %d", @""), err];
        NSDictionary *userInfo = @{ NSLocalizedFailureReasonErrorKey: failureReason };
        
        *errOut = [NSError errorWithDomain: NLPasscodeEncryptionSupportDomain
                                      code: NLPasscodeEncryptionSupportDomainDecryptionFailure
                                  userInfo: userInfo];
        return nil;
    }
    
    [passcodeBuff setLength: passcodeLen];
    return passcodeBuff;
}

+ (BOOL) isSupportedPasscodeEncryptionConfig: (UInt8) config {
    return IsSupportedPasscodeEncryptionConfig(config);
}

+ (BOOL) getEncryptedPasscodeConfig: (NSData *) encPasscode
                              config: (UInt8 *) configOut
                               error: (NSError **) errOut {
    UInt8 config =0;
    WEAVE_ERROR err = GetEncryptedPasscodeConfig((unsigned char *) [encPasscode bytes],
                                                [encPasscode length], config);
    if (err != WEAVE_NO_ERROR){
        NSString *failureReason =
        [NSString stringWithFormat:NSLocalizedString(@"retrieving encrypted config error: %d", @""), err];
        NSDictionary *userInfo = @{ NSLocalizedFailureReasonErrorKey: failureReason };
        
        *errOut = [NSError errorWithDomain: NLPasscodeEncryptionSupportDomain
                                      code: NLPasscodeEncryptionSupportDomainInvalidData
                                  userInfo: userInfo];
        return FALSE;
    }
    *configOut = config;
    
    return TRUE;
}
+ (BOOL) getEncryptedPasscodeKeyId: (NSData *) encPasscode
                             keyId: (UInt32 *) keyIdOut
                             error: (NSError **) errOut {
    UInt32 keyId =0;
    WEAVE_ERROR err = GetEncryptedPasscodeKeyId((unsigned char *) [encPasscode bytes],
                                                  [encPasscode length], (uint32_t &) keyId);
    if (err != WEAVE_NO_ERROR) {
        NSString *failureReason =
        [NSString stringWithFormat:NSLocalizedString(@"retrieving encrypted config error: %d", @""), err];
        NSDictionary *userInfo = @{ NSLocalizedFailureReasonErrorKey: failureReason };
        
        *errOut = [NSError errorWithDomain: NLPasscodeEncryptionSupportDomain
                                      code: NLPasscodeEncryptionSupportDomainInvalidData
                                  userInfo: userInfo];
        return FALSE;
    }
    *keyIdOut = keyId;
    
    return TRUE;
}

+ (BOOL) getEncryptedPasscodeNonce: (NSData *) encPasscode
                             nonce: (UInt32 *) nonceOut
                             error: (NSError **) errOut {
    UInt32 nonce = 0;
    WEAVE_ERROR err = GetEncryptedPasscodeNonce((unsigned char *) [encPasscode bytes],
                                                [encPasscode length], (uint32_t &) nonce);
    if (err != WEAVE_NO_ERROR) {
        NSString *failureReason =
        [NSString stringWithFormat:NSLocalizedString(@"retrieving encrypted passcode nonce: %d", @""), err];
        NSDictionary *userInfo = @{ NSLocalizedFailureReasonErrorKey: failureReason };
        
        *errOut = [NSError errorWithDomain: NLPasscodeEncryptionSupportDomain
                                      code: NLPasscodeEncryptionSupportDomainInvalidData
                                  userInfo: userInfo];
        return FALSE;
    }
    *nonceOut = nonce;
    
    return TRUE;
    
}

+ (nullable NSData *) getEncryptedPasscodeFingerprint: (NSData *) encPasscode
                                                error: (NSError **) errOut {
    
    NSMutableData * fingerprint = [[NSMutableData alloc] initWithLength: kPasscodeFingerprintLen];
    size_t fingerprintLen =0;
    
    WEAVE_ERROR err = GetEncryptedPasscodeFingerprint((unsigned char *) [encPasscode bytes],
                                                      [encPasscode length],
                                                      (unsigned char *) [fingerprint mutableBytes],
                                                      [fingerprint length],
                                                      fingerprintLen);
    if (err != WEAVE_NO_ERROR){
        
        NSString *failureReason = [NSString stringWithFormat:NSLocalizedString(@"get fingerprint error: %d", @""), err];
        NSDictionary *userInfo = @{ NSLocalizedFailureReasonErrorKey: failureReason };
        
        *errOut = [NSError errorWithDomain: NLPasscodeEncryptionSupportDomain
                                      code: NLPasscodeEncryptionSupportDomainInvalidData
                                  userInfo: userInfo];
        return nil;
    }
    
    [fingerprint setLength: fingerprintLen];
    return fingerprint;
}

#pragma mark Private Methods

+ (BOOL) validateKeySize: (NSData *) encKey
                 authKey: (NSData *) authKey
          fingerprintKey: (NSData *) fingerprintKey
                   error: (NSError **) errOut {
    
    if ([encKey length] != kPasscodeEncryptionKeyLen) {
        if (errOut) {
            *errOut = [NSError errorWithDomain: NLPasscodeEncryptionSupportDomain
                                          code: NLPasscodeEncryptionSupportDomainInvalidEncKeySize
                                      userInfo: nil];
        }
        return FALSE;
    }
    
    if ([authKey length] != kPasscodeAuthenticationKeyLen) {
        if (errOut) {
            *errOut = [NSError errorWithDomain: NLPasscodeEncryptionSupportDomain
                                          code: NLPasscodeEncryptionSupportDomainInvalidAuthKeySize
                                      userInfo: nil];
        }
        return FALSE;
    }
    
    if ([fingerprintKey length] != kPasscodeFingerprintKeyLen) {
        
        if (errOut) {
            *errOut = [NSError errorWithDomain: NLPasscodeEncryptionSupportDomain
                                          code: NLPasscodeEncryptionSupportDomainInvalidFingerprintKeySize
                                      userInfo: nil];
        }
        return FALSE;
    }
    
    return TRUE;
}

@end
