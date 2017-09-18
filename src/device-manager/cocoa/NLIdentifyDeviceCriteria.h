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

#import <Foundation/Foundation.h>

#import "NLWeaveDeviceManagerTypes.h"

typedef uint64_t NLWeaveIdentifierType;

@interface NLIdentifyDeviceCriteria : NSObject

/** Specifies that only devices that are members of the specified Weave fabric should respond.
     * A value of 0 specifies that only devices that are not a member of a fabric should respond.
     * A value of -1 specifies that all  devices should respond regardless of fabric membership.
     */
@property (nonatomic) NLWeaveIdentifierType TargetFabricId;

/** Specifies that only devices that are currently in the specified modes should respond.
     * Values are taken from the TargetDeviceModes enum.
     */
@property (nonatomic) NLTargetDeviceModes TargetModes;

/** Specifies that only devices manufactured by the given vendor should respond.
     * A value of -1 specifies any vendor.
     */
@property (nonatomic) NSInteger TargetVendorId;

/** Specifies that only devices with the given product code should respond.
     * A value of -1 specifies any product.
     * If the TargetProductId field is specified, then the TargetVendorId must also be specified.
     */
@property (nonatomic) NSInteger TargetProductId;

/** Specifies that only the device with the specified Weave Node ID should respond.
     * A value of -1 specifies all devices should respond.
     *
     * NOTE: the value of the TargetDeviceId field is carried a Weave IdentifyRequest
     * in the Destination Node ID field of the Weave message header, and thus
     * does NOT appear in the payload of the message.
     */
@property (nonatomic) NLWeaveIdentifierType TargetDeviceId;

@end
