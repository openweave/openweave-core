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

#import "NLWeaveDeviceDescriptor.h"

#include <Weave/Profiles/device-description/DeviceDescription.h>

using nl::Weave::Profiles::DeviceDescription::WeaveDeviceDescriptor;

@interface NLWeaveDeviceDescriptor()

+ (NLWeaveDeviceDescriptor *)createUsing:(WeaveDeviceDescriptor)deviceDescriptor;

- (WeaveDeviceDescriptor)toWeaveDeviceDescriptor;
@end
