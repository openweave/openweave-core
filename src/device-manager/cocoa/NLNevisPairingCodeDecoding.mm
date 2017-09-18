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
 *      This file provides a means to decode Nevis pairing codes to get the device ID.
 *
 */
 
#import <string.h>

#import "NLNevisPairingCodeDecoding.h"

#import <Weave/Core/WeaveCore.h>
#import <Weave/Support/pairing-code/PairingCodeUtils.h>

@implementation NLNevisPairingCodeDecoding

+ (uint64_t)extractDeviceIDFromNevisPairingCode:(NSString *)pairingCode {
	const char *pairingCodeCS = [pairingCode UTF8String];
	uint64_t deviceId;
	WEAVE_ERROR err = nl::PairingCode::NevisPairingCodeToDeviceId(pairingCodeCS, deviceId);
	return (err == WEAVE_NO_ERROR) ? deviceId : 0;
}

+ (NSString *)extractNevisPairingCodeFromDeviceID:(uint64_t)deviceId {
	char pairingCodeBuf[nl::PairingCode::kStandardPairingCodeLength + 1];
	WEAVE_ERROR err = nl::PairingCode::NevisDeviceIdToPairingCode(deviceId, pairingCodeBuf, sizeof(pairingCodeBuf));
	if (err == WEAVE_NO_ERROR) {
        return [NSString stringWithUTF8String:pairingCodeBuf];
    } else {
        return nil;
    }
}

@end
