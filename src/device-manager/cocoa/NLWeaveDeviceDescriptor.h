/*
 *
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
 *      Objective-C representation of the DeviceDescription data communicated via device-description profile.
 *
 */

#import <Foundation/Foundation.h>

#import "NLWeaveDeviceManagerTypes.h"


@interface NLWeaveDeviceDescriptor : NSObject<NSCopying, NSCoding>

+ (instancetype)decodeDeviceDescriptor:(NSString *)descriptorStr;

@property (nonatomic, strong) NSNumber *DeviceId;                           	// Weave device id (0 = not present)
@property (nonatomic, strong) NSNumber *FabricId;                           	// Id of Weave fabric to which the device belongs (0 = not present)
@property (nonatomic) NSInteger DeviceFeatures;							// Indicates support for specific device features.
@property (nonatomic) NSUInteger VendorId;                         		// Device vendor code (0 = not present)
@property (nonatomic) NSInteger ProductId;                        		// Device product code (0 = not present)
@property (nonatomic) NSUInteger ProductRevision;                  		// Device product revision (0 = not present)

@property (nonatomic) NLManufacturingDate ManufacturingDate;

@property (nonatomic, strong) NSData *Primary802154MACAddress;      	// MAC address for primary 802.15.4 interface (big-endian, all zeros = not present)
@property (nonatomic, strong) NSData *PrimaryWiFiMACAddress;        	// MAC address for primary WiFi interface (big-endian, all zeros = not present)

@property (nonatomic, copy) NSString *SerialNumber;               	// Device serial number (nul terminated, 0 length = not present)
@property (nonatomic, copy) NSString *SoftwareVersion;            	// Active software version (nul terminated, 0 length = not present)
@property (nonatomic, copy) NSString *RendezvousWiFiESSID;        	// ESSID for pairing WiFi network (nul terminated, 0 length = not present)
@property (nonatomic, copy) NSString *PairingCode;                	// Device pairing code (nul terminated, 0 length = not present)

@property (nonatomic) NSUInteger PairingCompatibilityVersionMajor;		// Major device pairing software compatibility version
@property (nonatomic) NSUInteger PairingCompatibilityVersionMinor;		// Minor device pairing software compatibility version

- (BOOL)requiresLinePower;												// Indicates a device that requires line power.

@end
