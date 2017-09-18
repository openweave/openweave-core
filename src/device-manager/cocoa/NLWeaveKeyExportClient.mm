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
#import "NLWeaveKeyExportClient.h"
#import "NLWeaveKeyExportSupport.h"
#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveKeyIds.h>
#include <Weave/Support/CodeUtils.h>
#include <Weave/Profiles/security/WeaveKeyExportClient.h>

using namespace nl::Weave::Profiles::Security::KeyExport;

NSString *const NLWeaveKeyExportClientErrorDomain = @"NLWeaveKeyExportClientErrorDomain";

@interface NLWeaveKeyExportClient ()
{
    WeaveStandAloneKeyExportClient * _mKeyExportClientCpp;
}
@end

@implementation NLWeaveKeyExportClient

static UInt32 const kMaxPubKeySize = (((WEAVE_CONFIG_MAX_EC_BITS + 7) / 8) + 1) * 2;
static UInt32 const kMaxECDSASigSize = kMaxPubKeySize;

- (nullable NSData *) generateKeyExportRequest: (UInt32) keyId
                               responderNodeId: (UInt64) responderNodeId
                                   accessToken: (NSData *) accessToken
                                         error: (NSError **) errOut {
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint16_t exportReqLen = 0;
    
    if (accessToken == nil) {
        if (errOut) {
            *errOut = [NSError errorWithDomain: NLWeaveKeyExportClientErrorDomain
                                          code: NLWeaveKeyExportClientErrorDomainInvalidArgument
                                      userInfo: nil];
        }
        return nil;
    }
    
    size_t exportReqBufSize =
    7                       // Key export request header size
    + kMaxPubKeySize          // Ephemeral public key size
    + kMaxECDSASigSize        // Size of bare signature field
    +  [accessToken length]   // Size equal to at least the total size of the client certificates
    + 1024;                   // Space for additional signature fields plus encoding overhead
    
    if (exportReqBufSize > UINT16_MAX) {
        if (errOut) {
            *errOut = [NSError errorWithDomain: NLWeaveKeyExportClientErrorDomain
                                          code:NLWeaveKeyExportClientErrorDomainInvalidExportBufferSize
                                      userInfo:nil];
        }
        return nil;
    }
    
    NSMutableData * exportReqBuf = [[NSMutableData alloc] initWithLength:exportReqBufSize];
    
    err = _mKeyExportClientCpp->GenerateKeyExportRequest((uint32_t) keyId,
                                                         (uint64_t) responderNodeId,
                                                         (unsigned char *) [accessToken bytes],
                                                         [accessToken length],
                                                         (unsigned char *) [exportReqBuf mutableBytes],
                                                         exportReqBufSize,
                                                         exportReqLen);
    
    if (err != WEAVE_NO_ERROR) {
        if (errOut) {
            NSString *failureReason = [NSString stringWithFormat:NSLocalizedString(
                                            @"GenerateKeyExportRequest error: %d", @""), err];
            NSDictionary *userInfo = @{ NSLocalizedFailureReasonErrorKey: failureReason };
            *errOut = [NSError errorWithDomain:NLWeaveKeyExportClientErrorDomain
                                          code:NLWeaveKeyExportClientErrorDomainKeyExportRequestFailure userInfo: userInfo];
        }
        
        return nil;
    }
    
    [exportReqBuf setLength: exportReqLen];
    
    return exportReqBuf;
}

- (nullable NSData *) generateKeyExportRequest: (UInt32) keyId
                               responderNodeId: (UInt64) responderNodeId
                                    clientCert: (NSData *) clientCert
                                     clientKey: (NSData *) clientKey
                                         error: (NSError **) errOut {
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint16_t exportReqLen = 0;
    
    if (clientKey == nil || clientCert){
        if (errOut) {
            *errOut = [NSError errorWithDomain:NLWeaveKeyExportClientErrorDomain
                       code:NLWeaveKeyExportClientErrorDomainInvalidArgument
                       userInfo:nil];
        }
        return nil;
    }
    
    size_t exportReqBufSize =
    7                       // Key export request header size
    + kMaxPubKeySize          // Ephemeral public key size
    + kMaxECDSASigSize        // Size of bare signature field
    +  [clientCert length]   // Size equal to at least the total size of the client certificates
    + 1024;                   // Space for additional signature fields plus encoding overhead
    
    if (exportReqBufSize > UINT16_MAX) {
        if (errOut) {
            *errOut = [NSError errorWithDomain:NLWeaveKeyExportClientErrorDomain
                       code:NLWeaveKeyExportClientErrorDomainInvalidExportBufferSize
                       userInfo:nil];
        }
        return nil;
    }
    
    NSMutableData * exportReqBuf = [[NSMutableData alloc] initWithLength:exportReqBufSize];
    
    err = _mKeyExportClientCpp->GenerateKeyExportRequest((uint32_t) keyId,
                                     (uint64_t) responderNodeId,
                                     (unsigned char *) [clientCert bytes],
                                     [clientCert length],
                                     (unsigned char *) [clientKey bytes],
                                     [clientKey length],
                                      (unsigned char *) [exportReqBuf mutableBytes],
                                     exportReqBufSize,
                                     exportReqLen);
    
    if (err != WEAVE_NO_ERROR) {
        if (errOut) {
            NSString *failureReason = [NSString stringWithFormat:NSLocalizedString(
                                                                                   @"GenerateKeyExportRequest error: %d", @""), err];
            NSDictionary *userInfo = @{ NSLocalizedFailureReasonErrorKey: failureReason };
            *errOut = [NSError errorWithDomain:NLWeaveKeyExportClientErrorDomain
                                          code:NLWeaveKeyExportClientErrorDomainKeyExportRequestFailure userInfo: userInfo];
        }
        
        return nil;
    }
    
    [exportReqBuf setLength: exportReqLen];
    return exportReqBuf;
}

- (nullable NSData *) processKeyExportResponse: (UInt64) responderNodeId
                                    exportResp: (NSData *) exportResp
                                         error: (NSError **) errOut {
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    
    if (exportResp == nil) {
        if (errOut) {
            *errOut = [NSError  errorWithDomain: NLWeaveKeyExportClientErrorDomain
                                           code: NLWeaveKeyExportClientErrorDomainInvalidArgument
                                       userInfo: nil];
            }
        return nil;
    }
  
    
    // Since the exported key is contained within the export response, a buffer of the same size
    // is guaranteed to be sufficient.
    size_t exportedKeyBufLen = [exportResp length];
    NSMutableData * exportedKeyBuf = [[NSMutableData alloc] initWithLength:exportedKeyBufLen];
    
    uint16_t exportedKeyLen;
    uint32_t exportedKeyId;
    err = _mKeyExportClientCpp->ProcessKeyExportResponse(
                                                         (unsigned char *) [exportResp bytes],
                                                         [exportResp length],
                                                         (uint64_t) responderNodeId,
                                                         (unsigned char *) [exportedKeyBuf mutableBytes],
                                                         exportedKeyBufLen,
                                                         exportedKeyLen,
                                                         exportedKeyId);
    
    if (err != WEAVE_NO_ERROR) {
        if (errOut) {
            NSString *failureReason = [NSString stringWithFormat:NSLocalizedString(
                                                                                   @"ProcessKeyExportResponse error:: %d", @""), err];
            NSDictionary *userInfo = @{ NSLocalizedFailureReasonErrorKey: failureReason };
            *errOut = [NSError errorWithDomain:NLWeaveKeyExportClientErrorDomain
                                          code:NLWeaveKeyExportClientErrorDomainKeyExportResponseFailure userInfo: userInfo];
        }
        
        return nil;
    }
    
    [exportedKeyBuf setLength: exportedKeyLen];
    return exportedKeyBuf;
}

- (void) reset {
    _mKeyExportClientCpp->Reset();
}

/**
 @note
 This function can only be called by the ARC runtime
 */
- (void)dealloc
{
    _mKeyExportClientCpp->Reset();
    delete _mKeyExportClientCpp;
}

- (BOOL) processKeyExportReconfigure: (NSData *) reconfig
                            error: (NSError **) errOut {
    
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    
    if (reconfig == nil) {
        if (errOut) {
            *errOut = [NSError errorWithDomain: NLWeaveKeyExportClientErrorDomain
                                          code: NLWeaveKeyExportClientErrorDomainInvalidArgument
                                      userInfo: nil];
        }
        return false;
    }
    
    err = _mKeyExportClientCpp->ProcessKeyExportReconfigure((unsigned char *) [reconfig bytes], [reconfig length]);
    
    if (err != WEAVE_NO_ERROR) {
        if (errOut) {
            NSString *failureReason = [NSString stringWithFormat:NSLocalizedString(
                                                                                   @"ProcessKeyExportResponse error: %d", @""), err];
            NSDictionary *userInfo = @{ NSLocalizedFailureReasonErrorKey: failureReason };
            *errOut = [NSError errorWithDomain:NLWeaveKeyExportClientErrorDomain
                                          code:NLWeaveKeyExportClientErrorDomainProcessReconfiugreFailure userInfo: userInfo];
        }
        return false;
    }
    
    return true;
}

- (BOOL) allowNestDevelopmentDevices {
    
    return _mKeyExportClientCpp->AllowNestDevelopmentDevices();
}

- (void) setAllowNestDevelopmentDevices: (BOOL) nestDev {
    _mKeyExportClientCpp->AllowNestDevelopmentDevices(nestDev);
}

- (BOOL) allowSHA1DeviceCertificates {
    return _mKeyExportClientCpp->AllowSHA1DeviceCerts();
}

- (void) setAllowSHA1DeviceCertificates: (BOOL) nestDev {
    _mKeyExportClientCpp->AllowSHA1DeviceCerts(nestDev);
}

- (instancetype) init {
    self = [super init];
    
    if (self) {
        _mKeyExportClientCpp = new WeaveStandAloneKeyExportClient();
        _mKeyExportClientCpp->Init();
    }
    return self;
}

@end
