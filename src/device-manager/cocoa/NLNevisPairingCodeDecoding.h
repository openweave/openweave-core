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

#import <Foundation/Foundation.h>
#import <AvailabilityMacros.h>

@interface NLNevisPairingCodeDecoding : NSObject

/**
 * @brief
 *   Returns the device ID encoded in `pairingCode` if `pairingCode` is a valid pairing code;
 *   if invalid, returns 0.
 *
 *  @param[in]  pairingCode  The pairing code of the Nevis.
 */
+ (uint64_t)extractDeviceIDFromNevisPairingCode:(NSString *)pairingCode DEPRECATED_MSG_ATTRIBUTE("Use NLPairingCodeUtils instead.");

/**
 * @brief
 *   Encodes the given Nevis device ID into a 6-character pairing code, if `deviceId` is a valid
 *   Nevis tag number (i.e. it begins with 18B430040). If invalid, returns nil.
 *
 *  @param[in]  deviceId  The device ID of the Nevis.
 */
+ (NSString *)extractNevisPairingCodeFromDeviceID:(uint64_t)deviceId DEPRECATED_MSG_ATTRIBUTE("Use NLPairingCodeUtils instead.");

@end
