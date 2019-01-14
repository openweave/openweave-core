/*
 *
 *    Copyright (c) 2019 Google LLC
 *    Copyright (c) 2013-2017 Nest Labs, Inc.
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
 *      Objective-C representation of NetworkInfo payload communicated via network-provisioning profile.
 *
 */

#import <Foundation/Foundation.h>

#import "NLWeaveDeviceManagerTypes.h"

typedef int64_t NLNetworkID;

constexpr int64_t NLNetworkID_NotSpecified = -1LL;
constexpr int NLThreadPANId_NotSpecified = -1;
constexpr int NLThreadChannel_NotSpecified = -1;

@interface NLNetworkInfo : NSObject

@property (nonatomic) NLNetworkType NetworkType;                    // The type of network (WiFi, etc.)
@property (nonatomic) NLNetworkID NetworkId;                        // The network id assigned to the network by the device, -1 if not specified.

@property (nonatomic, strong) NSString *WiFiSSID;                   // The WiFi SSID, or NULL if not a WiFi network.
@property (nonatomic) NLWiFiMode WiFiMode;                          // The operating mode of the WiFi network.
@property (nonatomic) NLWiFiRole WiFiRole;                          // The role played by the device on the WiFi network.

@property (nonatomic) NLWiFiSecurityType WiFiSecurityType;          // The WiFi security type.

@property (nonatomic, strong) NSData *WiFiKey;                      // The WiFi key, or NULL if not specified.

@property (nonatomic, strong) NSString *ThreadNetworkName;          // The Thread network name, or NULL if not a Thread network.
@property (nonatomic, strong) NSData *ThreadExtendedPANId;          // The Thread extended PAN id, or NULL if not specified. Must be exactly 8 bytes in length.
@property (nonatomic, strong) NSData *ThreadNetworkKey;             // The Thread network key, or NULL if not specified.

@property (nonatomic) short WirelessSignalStrength;                 // The signal strength of the network, in dBm, or Short.MIN_VALUE if not available/applicable.

@property (nonatomic) int ThreadPANId;                              // The Thread PAN identifier (short) or NLThreadPANId_NotSpecified if not available/applicable.
@property (nonatomic) int ThreadChannel;                            // The Thread channel or NLThreadChannel_NotSpecified if not available/applicable.

- (id)initWithWiFiSSID:(NSString *)ssid wifiKey:(NSData *)wifiKey securityType:(NLWiFiSecurityType)securityType;
- (NSString *)WiFiSecurityTypeAsString;

@end
