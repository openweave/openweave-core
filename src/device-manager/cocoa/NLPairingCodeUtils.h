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

#import <Foundation/Foundation.h>

@interface NLPairingCodeUtils : NSObject

/** Determine if a Nest pairing code is valid.
 *
 * @param[in]   pairingCode             The pairing code string to be checked.
 *
 * @returns                             TRUE if the supplied pairing code is valid.
 */
+ (BOOL)isValidPairingCode:(NSString *)pairingCode;

/** Normalize the characters in a pairing code string.
 *
 * This function converts all alphabetic characters to upper-case, maps the illegal characters 'I', 'O',
 * 'Q' and 'Z' to '1', '0', '0' and '2', respectively, and removes all other non-pairing code characters
 * from the given string.
 *
 * If the pairing code contains invalid characters, other than those listed above, the function returns
 * nil.
 *
 * @param[in] pairingCode               The pairing code string to be normalized.
 *
 * @returns                             The normalized pairing code.
 */
+ (NSString *)normalizePairingCode:(NSString *)pairingCode;

/** Returns the device ID encoded in Nevis pairing code.
 *
 * @param[in]  pairingCode                  A string containing a Nevis pairing code.
 *
 * @returns                                 A Nevis device id, or 0 if the supplied pairing code was invalid.
 */
+ (uint64_t)nevisPairingCodeToDeviceId:(NSString *)pairingCode;

/** Generates a Nevis pairing code string given a Nevis device id.
 *
 * @param[in]  deviceId                     A Nevis device id.
 *
 * @returns                                 A pairing code string, or nil if the supplied device id is out of range.
 *
 */
+ (NSString *)nevisDeviceIdToPairingCode:(uint64_t)deviceId;

/** Returns the device ID encoded in Kryptonite pairing code.
 *
 * @param[in]  pairingCode                  A string containing a Kryptonite pairing code.
 *
 * @returns                                 A Nevis device id, or 0 if the supplied pairing code was invalid.
 */
+ (uint64_t)kryptonitePairingCodeToDeviceId:(NSString *)pairingCode;

/** Generates a Kryptonite pairing code string given a Nevis device id.
 *
 * @param[in]  deviceId                     A Kryptonite device id.
 *
 * @returns                                 A pairing code string, or nil if the supplied device id is out of range.
 *
 */
+ (NSString *)kryptoniteDeviceIdToPairingCode:(uint64_t)deviceId;

@end
