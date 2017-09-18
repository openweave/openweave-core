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
 *      Properties for filtering discovered devices.
 *
 */

#import "NLIdentifyDeviceCriteria.h"

#import "NLIdentifyDeviceCriteria_Protected.h"
#include <Weave/Profiles/device-description/DeviceDescription.h>

@implementation NLIdentifyDeviceCriteria

+ (NLIdentifyDeviceCriteria *)create {
    return [[NLIdentifyDeviceCriteria alloc] init];
}

- (id)init {
    if (self = [super init]) {
        _TargetFabricId = NLTargetFabricId_Any;
        _TargetModes = NLTargetDeviceModeAny;
        _TargetVendorId = kNLWeaveVendor_Any;   // any vendor id
        _TargetProductId = kNestWeaveProduct_NotSpecified;  // any product
        _TargetDeviceId = NLTargetDeviceId_AnyNodeId; //0xFFFF;   // any device
    }
    return self;
}

#pragma mark - Public methods

#pragma mark - Protected methods

- (IdentifyDeviceCriteria)toIdentifyDeviceCriteria {

	IdentifyDeviceCriteria identifyDeviceCriteria;
    //
    identifyDeviceCriteria.TargetFabricId = _TargetFabricId;
    identifyDeviceCriteria.TargetModes = _TargetModes;
    identifyDeviceCriteria.TargetVendorId = (uint16_t)_TargetVendorId;
    identifyDeviceCriteria.TargetProductId = (uint16_t)_TargetProductId;
    identifyDeviceCriteria.TargetDeviceId = _TargetDeviceId;

    return identifyDeviceCriteria;
}

#pragma mark - Private methods

- (NSString *)debugDescriptionWithIdentifyDeviceCriteria:(nl::Weave::Profiles::DeviceDescription::IdentifyDeviceCriteria *)pDeviceCriteria {
    NSString *descr = [NSString stringWithFormat:@"(%p), \n"
                                                         "\tTargetFabricId: %llu\n"
                                                         "\tTargetModes: %d\n"
                                                         "\tTargetVendorId: %d\n"
                                                         "\tTargetProductId: %d\n"
                                                         "\tTargetDeviceId: %llu\n",
                                             	self,
                                                pDeviceCriteria->TargetFabricId,
                                                pDeviceCriteria->TargetModes,
                                                pDeviceCriteria->TargetVendorId,
                                                pDeviceCriteria->TargetProductId,
                                                pDeviceCriteria->TargetDeviceId];

    return descr;
}

- (NSString *)debugDescription {
    NSString *descr = [NSString stringWithFormat:@"(%p), \n"
                                                         "\tTargetFabricId: %llu\n"
                                                         "\tTargetModes: %ld\n"
                                                         "\tTargetVendorId: %ld\n"
                                                         "\tTargetProductId: %ld\n"
                                                         "\tTargetDeviceId: %llu\n",
                                                 self,
                                                 _TargetFabricId,
                                                 (long)_TargetModes,
                                                 (long)_TargetVendorId,
                                                 (long)_TargetProductId,
                                                 _TargetDeviceId];

    return descr;
}

@end
