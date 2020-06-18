/*
 *
 *    Copyright (c) 2020 Google LLC
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
 *      Objective-C representation of wireless regulatory configuration information.
 *
 */

#import <Foundation/Foundation.h>

#import "NLWeaveDeviceManagerTypes.h"

@interface NLWirelessRegConfig : NSObject

@property (nonatomic, strong) NSString * RegDomain; // The region domain name
@property (nonatomic) NLWirelessOperatingLocation OpLocation; // The Wireless Operating location.
@property (nonatomic, strong) NSMutableArray * SupportedRegDomains; // The Supported Wireless reg domain.

- (id)initWithSupportedRegDomains:(NSMutableArray *)supportedRegDomains
                       opLocation:(NLWirelessOperatingLocation)opLocation
                        RegDomain:(NSString *)regDomain;

- (id)initWithRegDomain:(NSString *)regDomain opLocation:(NLWirelessOperatingLocation)opLocation;

- (WEAVE_ERROR)getRegDomain:(NSString **)val;

- (WEAVE_ERROR)getOpLocation:(NLWirelessOperatingLocation *)val;

- (WEAVE_ERROR)getSupportedRegDomain:(NSMutableArray **)val;

@end
