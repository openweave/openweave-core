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

#import "NLLogging.h"
#import "NLWeaveErrorCodes.h"
#import "NLWirelessRegConfig_Protected.h"

using nl::Weave::Profiles::NetworkProvisioning::WirelessOperatingLocation;
using nl::Weave::Profiles::NetworkProvisioning::WirelessRegDomain;

@implementation NLWirelessRegConfig

+ (NLWirelessRegConfig *)createUsing:(const WirelessRegConfig *)pWirelessRegConfig
{
    return [[NLWirelessRegConfig alloc] initWith:pWirelessRegConfig];
}

- (instancetype)initWithRegDomain:(NSString *)regDomain opLocation:(NLWirelessOperatingLocation)opLocation
{
    if (self = [self init]) {
        _RegDomain = regDomain;
        _OpLocation = opLocation;
        _SupportedRegDomains = [NSMutableArray arrayWithCapacity:0];
    }
    return self;
}

- (instancetype)initWithSupportedRegDomains:(NSMutableArray *)supportedRegDomains
                                 opLocation:(NLWirelessOperatingLocation)opLocation
                                  RegDomain:(NSString *)regDomain
{
    if (self = [self init]) {
        _RegDomain = regDomain;
        _OpLocation = opLocation;
        _SupportedRegDomains = supportedRegDomains;
    }
    return self;
}

- (instancetype)initWith:(const WirelessRegConfig *)pWirelessRegConfig
{
    if (self = [super init]) {
        _RegDomain = [[NSString alloc] initWithCString:pWirelessRegConfig->RegDomain.Code encoding:NSUTF8StringEncoding];
        _OpLocation = (NLWirelessOperatingLocation) pWirelessRegConfig->OpLocation;
        _SupportedRegDomains = [[NSMutableArray alloc] initWithCapacity:pWirelessRegConfig->NumSupportedRegDomains];

        WDM_LOG_DEBUG(@"pWirelessRegConfig->NumSupportedRegDomains = %u\n", pWirelessRegConfig->NumSupportedRegDomains);
        for (uint32_t i = 0; i < pWirelessRegConfig->NumSupportedRegDomains; i++) {
            NSMutableString * nlRegDomain = [NSMutableString string];
            for (NSUInteger j = 0; j < sizeof(WirelessRegDomain::Code); j++) {
                char ch = pWirelessRegConfig->SupportedRegDomains[i].Code[j];
                [nlRegDomain appendFormat:@"%c", ch];
            }
            [_SupportedRegDomains addObject:nlRegDomain];
        }
        NSLog(@"_SupportedRegDomains is: %@", _SupportedRegDomains);
    }
    return self;
}

#pragma mark - Protected methods

- (WirelessRegConfig)toWirelessRegConfig
{
    WirelessRegConfig wirelessRegConfig;

    memcpy(wirelessRegConfig.RegDomain.Code, [_RegDomain UTF8String], [_RegDomain length]);
    wirelessRegConfig.OpLocation = [self toWirelessOperatingLocation:_OpLocation];

    wirelessRegConfig.NumSupportedRegDomains = 0;
    // NOTE: The SupportRegulatoryDomains field is never sent *to* a device.  Thus we ignore the field value here.

    return wirelessRegConfig;
}

- (nl::Weave::Profiles::NetworkProvisioning::WirelessOperatingLocation)toWirelessOperatingLocation:
    (NLWirelessOperatingLocation)opLocation
{
    switch (opLocation) {
    case kNLWirelessOperatingLocation_Unknown:
        return nl::Weave::Profiles::NetworkProvisioning::kWirelessOperatingLocation_Unknown;
    case kNLWirelessOperatingLocation_Indoors:
        return nl::Weave::Profiles::NetworkProvisioning::kWirelessOperatingLocation_Indoors;
    case kNLWirelessOperatingLocation_Outdoors:
        return nl::Weave::Profiles::NetworkProvisioning::kWirelessOperatingLocation_Outdoors;
    default:
        return nl::Weave::Profiles::NetworkProvisioning::kWirelessOperatingLocation_NotSpecified;
    }
}

- (WEAVE_ERROR)getRegDomain:(NSString **)val
{
    *val = _RegDomain;
    return WEAVE_NO_ERROR;
}

- (WEAVE_ERROR)getOpLocation:(NLWirelessOperatingLocation *)val
{
    *val = _OpLocation;
    return WEAVE_NO_ERROR;
}

- (WEAVE_ERROR)getSupportedRegDomain:(NSMutableArray **)val
{
    *val = _SupportedRegDomains;
    return WEAVE_NO_ERROR;
}

@end
