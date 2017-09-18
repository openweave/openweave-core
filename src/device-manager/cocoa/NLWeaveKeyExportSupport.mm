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
 *      This file defines a Wrapper for C++ utility functions for testing key export 
 *      functionality (needed for keyStore in mobileiOS tree).
 *
 */

#import "NLWeaveKeyExportSupport.h"

#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveKeyIds.h>
#include <Weave/Support/CodeUtils.h>
#include <Weave/Profiles/security/WeaveKeyExportClient.h>

using namespace nl::Weave::Profiles::Security::KeyExport;

NSString *const NLWeaveKeyExportSupportErrorDomain = @"NLWeaveKeyExportSupprtErrorDomain";

@implementation NLWeaveKeyExportSupport

static UInt32 const kMaxPubKeySize = (((WEAVE_CONFIG_MAX_EC_BITS + 7) / 8) + 1) * 2;
static UInt32 const kMaxECDSASigSize = kMaxPubKeySize;

+ (nullable NSData *) simulateDeviceKeyExport: (NSData *) keyExportReq
                                   deviceCert: (NSData *) deviceCert
                                devicePrivKey: (NSData *) devicePrivKey
                                trustRootCert: (NSData *) trustRootCert
                                   isReconfig: (BOOL *) isReconfigOut
                                        error: (NSError **) errOut {
    
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint16_t exportRespLen = 0;
    bool isReconfig = false;
    
    if (keyExportReq == nil || deviceCert == nil || devicePrivKey == nil ||
        trustRootCert == nil || isReconfigOut == nil) {
        if (errOut) {
            *errOut = [NSError errorWithDomain: NLWeaveKeyExportSupportErrorDomain
                       code: NLWeaveKeyExportSupportErrorDomainInvalidArgument
                       userInfo: nil];
        }
        
        return nil;
    }
    
    size_t exportRespBufSize =
    7                       // Key export response header size // TODO: adjust this
    + kMaxPubKeySize          // Ephemeral public key size
    + kMaxECDSASigSize        // Size of bare signature field
    + [deviceCert length]           // Size equal to at least the total size of the device certificate
    + 1024;                   // Space for additional signature fields plus encoding overhead
    
    
    NSMutableData * exportRespBuff = [[NSMutableData alloc] initWithLength:exportRespBufSize];
    
    err = SimulateDeviceKeyExport((unsigned char *) [deviceCert bytes],
                                  [deviceCert length],
                                  (unsigned char *) [devicePrivKey bytes],
                                  [devicePrivKey length],
                                  (unsigned char *) [trustRootCert bytes],
                                  [trustRootCert length],
                                  (unsigned char *) [keyExportReq bytes],
                                  [keyExportReq length],
                                  (unsigned char *) [exportRespBuff mutableBytes],
                                  [exportRespBuff length],
                                  exportRespLen,
                                  isReconfig);
    
    if (err != WEAVE_NO_ERROR) {
        if (errOut) {
            NSString *failureReason = [NSString stringWithFormat:NSLocalizedString(
                                                        @"SimulateDeviceKeyExport error: %d", @""), err];
            NSDictionary *userInfo = @{ NSLocalizedFailureReasonErrorKey: failureReason };
            *errOut = [NSError errorWithDomain:NLWeaveKeyExportSupportErrorDomain
                                          code:NLWeaveKeyExportSupportErrorDomainSimulateKeyExportFailure userInfo: userInfo];
        }
        
        return nil;
    }
    
    *isReconfigOut = isReconfig? true: false;
    
    [exportRespBuff setLength: exportRespLen];
    return exportRespBuff;
}

@end
