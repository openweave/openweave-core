/*
 *
 *    Copyright (c) 2017 Nest Labs, Inc.
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
 *      Utility functions for working with Nest pairing codes.
 *
 */
 
#import <string.h>

#import "NLPairingCodeUtils.h"

#import <Weave/Support/pairing-code/PairingCodeUtils.h>

@implementation NLPairingCodeUtils

+ (BOOL)isValidPairingCode:(NSString *)pairingCode {
	const char *pairingCodeCS = [pairingCode UTF8String];
	return (nl::PairingCode::VerifyPairingCode(pairingCodeCS)) ? YES : NO;
}

+ (NSString *)normalizePairingCode:(NSString *)pairingCode {
	NSString *normalizedPairingCode = nil;
	char *pairingCodeCS = strdup([pairingCode UTF8String]);
	if (pairingCodeCS != NULL) {
		size_t pairingCodeLen = strlen(pairingCodeCS);
		nl::PairingCode::NormalizePairingCode(pairingCodeCS, pairingCodeLen);
		normalizedPairingCode = [NSString stringWithUTF8String:pairingCodeCS];
		free(pairingCodeCS);
	}
	return normalizedPairingCode;
}

+ (uint64_t)nevisPairingCodeToDeviceId:(NSString *)pairingCode {
	const char *pairingCodeCS = [pairingCode UTF8String];
	uint64_t deviceId;
	WEAVE_ERROR err = nl::PairingCode::NevisPairingCodeToDeviceId(pairingCodeCS, deviceId);
	return (err == WEAVE_NO_ERROR) ? deviceId : 0;
}

+ (NSString *)nevisDeviceIdToPairingCode:(uint64_t)deviceId {
	char pairingCodeBuf[nl::PairingCode::kStandardPairingCodeLength + 1];
	WEAVE_ERROR err = nl::PairingCode::NevisDeviceIdToPairingCode(deviceId, pairingCodeBuf, sizeof(pairingCodeBuf));
	if (err == WEAVE_NO_ERROR) {
        return [NSString stringWithUTF8String:pairingCodeBuf];
    } else {
        return nil;
    }
}

+ (uint64_t)kryptonitePairingCodeToDeviceId:(NSString *)pairingCode {
	const char *pairingCodeCS = [pairingCode UTF8String];
	uint64_t deviceId;
	WEAVE_ERROR err = nl::PairingCode::KryptonitePairingCodeToDeviceId(pairingCodeCS, deviceId);
	return (err == WEAVE_NO_ERROR) ? deviceId : 0;
}

+ (NSString *)kryptoniteDeviceIdToPairingCode:(uint64_t)deviceId {
	char pairingCodeBuf[nl::PairingCode::kKryptonitePairingCodeLength + 1];
	WEAVE_ERROR err = nl::PairingCode::KryptoniteDeviceIdToPairingCode(deviceId, pairingCodeBuf, sizeof(pairingCodeBuf));
	if (err == WEAVE_NO_ERROR) {
        return [NSString stringWithUTF8String:pairingCodeBuf];
    } else {
        return nil;
    }
}

@end
