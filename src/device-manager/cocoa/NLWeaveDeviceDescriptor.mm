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

#import "NLWeaveDeviceDescriptor_Protected.h"
#import <Weave/Profiles/device-description/DeviceDescription.h>
#import "NLLogging.h"
#include <Weave/Support/CodeUtils.h>

@implementation NLWeaveDeviceDescriptor

+ (instancetype)decodeDeviceDescriptor:(NSString *)descriptorStr
{
    WDM_LOG_METHOD_SIG();

    WEAVE_ERROR err;
    uint8_t *encodedBuf = NULL;
    uint32_t encodedLen;
    WeaveDeviceDescriptor deviceDesc;

    NLWeaveDeviceDescriptor *nlDeviceDescriptor = nil;

    encodedLen = [descriptorStr length];

    encodedBuf = (uint8_t *)malloc(encodedLen + 1);
    encodedBuf = (uint8_t *)strdup([descriptorStr UTF8String]);

    err = WeaveDeviceDescriptor::Decode(encodedBuf, encodedLen, deviceDesc);
    SuccessOrExit(err);

    nlDeviceDescriptor = [NLWeaveDeviceDescriptor createUsing:deviceDesc];

exit:
    if (encodedBuf != NULL)
        free((uint8_t *)encodedBuf);

    return nlDeviceDescriptor;
}

+ (NLWeaveDeviceDescriptor *)createUsing:(WeaveDeviceDescriptor)deviceDescriptor
{
    return [[NLWeaveDeviceDescriptor alloc] initWith:deviceDescriptor];
}

- (id)initWith:(nl::Weave::Profiles::DeviceDescription::WeaveDeviceDescriptor)deviceDescriptor
{
    if (self = [super init])
    {
        WDM_LOG_DEBUG(@"Creating new NLWeaveDeviceDescriptor using: ");
        WDM_LOG_DEBUG(@"WeaveDeviceDescriptor: %@", [self debugDescriptionWithDeviceDescriptor:&deviceDescriptor]);

        _DeviceId = [NSNumber numberWithUnsignedLongLong:deviceDescriptor.DeviceId];
        _FabricId = [NSNumber numberWithUnsignedLongLong:deviceDescriptor.FabricId];

        _DeviceFeatures = kNLDeviceFeatureNone;

        if ((deviceDescriptor.DeviceFeatures & WeaveDeviceDescriptor::kFeature_LinePowered) == WeaveDeviceDescriptor::kFeature_LinePowered)
        {
            _DeviceFeatures |= kNLDeviceFeature_LinePowered;
        }

        _VendorId = deviceDescriptor.VendorId;
        _ProductId = deviceDescriptor.ProductId;
        _ProductRevision = deviceDescriptor.ProductRevision;

        _ManufacturingDate.NLManufacturingDateMonth = deviceDescriptor.ManufacturingDate.Month;
        _ManufacturingDate.NLManufacturingDateDay = deviceDescriptor.ManufacturingDate.Day;
        _ManufacturingDate.NLManufacturingDateYear = deviceDescriptor.ManufacturingDate.Year;

        if (!WeaveDeviceDescriptor::IsZeroBytes(deviceDescriptor.Primary802154MACAddress, sizeof(deviceDescriptor.Primary802154MACAddress)))
        {
            NSUInteger length = sizeof(deviceDescriptor.Primary802154MACAddress) / sizeof(deviceDescriptor.Primary802154MACAddress[0]);
            _Primary802154MACAddress = [[NSData alloc] initWithBytes:deviceDescriptor.Primary802154MACAddress
                                                              length:length];
        }

        if (!WeaveDeviceDescriptor::IsZeroBytes(deviceDescriptor.PrimaryWiFiMACAddress, sizeof(deviceDescriptor.PrimaryWiFiMACAddress)))
        {
            NSUInteger length = sizeof(deviceDescriptor.PrimaryWiFiMACAddress) / sizeof(deviceDescriptor.PrimaryWiFiMACAddress[0]);
            _PrimaryWiFiMACAddress = [[NSData alloc] initWithBytes:deviceDescriptor.PrimaryWiFiMACAddress length:length];
        }

        _SerialNumber = [[NSString alloc] initWithUTF8String:deviceDescriptor.SerialNumber];

        _SoftwareVersion = [[NSString alloc] initWithUTF8String:deviceDescriptor.SoftwareVersion];

        _RendezvousWiFiESSID = [[NSString alloc] initWithUTF8String:deviceDescriptor.RendezvousWiFiESSID];

        _PairingCode = [[NSString alloc] initWithUTF8String:deviceDescriptor.PairingCode];

        _PairingCompatibilityVersionMajor = deviceDescriptor.PairingCompatibilityVersionMajor;
        _PairingCompatibilityVersionMinor = deviceDescriptor.PairingCompatibilityVersionMinor;
    }
    return self;
}

- (BOOL)requiresLinePower
{
    if ((_DeviceFeatures & kNLDeviceFeature_LinePowered) == kNLDeviceFeature_LinePowered)
    {
        return YES;
    }
    else
    {
        return NO;
    }
}

#pragma mark - NSCoding, NSCopying

- (id)initWithCoder:(NSCoder *)decoder {
    self = [super init];
    if (self != nil) {
        _DeviceId = [decoder decodeObjectForKey:NSStringFromSelector(@selector(DeviceId))];
        _FabricId = [decoder decodeObjectForKey:NSStringFromSelector(@selector(FabricId))];
        _DeviceFeatures = [decoder decodeIntegerForKey:NSStringFromSelector(@selector(DeviceFeatures))];
        _VendorId = [decoder decodeIntegerForKey:NSStringFromSelector(@selector(VendorId))];
        _ProductId = [decoder decodeIntegerForKey:NSStringFromSelector(@selector(ProductId))];
        _ProductRevision = [decoder decodeIntegerForKey:NSStringFromSelector(@selector(ProductRevision))];

        // Because encoding and decoding structs are a bit of a pain...
        _ManufacturingDate.NLManufacturingDateYear = [decoder decodeIntegerForKey:@"NLManufacturingDateYear"];
        _ManufacturingDate.NLManufacturingDateMonth = [decoder decodeIntegerForKey:@"NLManufacturingDateMonth"];
        _ManufacturingDate.NLManufacturingDateDay = [decoder decodeIntegerForKey:@"NLManufacturingDateDay"];

        _Primary802154MACAddress = [decoder decodeObjectForKey:NSStringFromSelector(@selector(Primary802154MACAddress))];
        _PrimaryWiFiMACAddress = [decoder decodeObjectForKey:NSStringFromSelector(@selector(PrimaryWiFiMACAddress))];

        _SerialNumber = [decoder decodeObjectForKey:NSStringFromSelector(@selector(SerialNumber))];
        _SoftwareVersion = [decoder decodeObjectForKey:NSStringFromSelector(@selector(SoftwareVersion))];
        _RendezvousWiFiESSID = [decoder decodeObjectForKey:NSStringFromSelector(@selector(RendezvousWiFiESSID))];
        _PairingCode = [decoder decodeObjectForKey:NSStringFromSelector(@selector(PairingCode))];

        _PairingCompatibilityVersionMajor = [decoder decodeIntegerForKey:NSStringFromSelector(@selector(PairingCompatibilityVersionMajor))];
        _PairingCompatibilityVersionMinor = [decoder decodeIntegerForKey:NSStringFromSelector(@selector(PairingCompatibilityVersionMinor))];
    }

    return self;
}

- (void)encodeWithCoder:(NSCoder *)coder {
    [coder encodeObject:self.DeviceId forKey:NSStringFromSelector(@selector(DeviceId))];
    [coder encodeObject:self.FabricId forKey:NSStringFromSelector(@selector(FabricId))];
    [coder encodeInteger:self.DeviceFeatures forKey:NSStringFromSelector(@selector(DeviceFeatures))];
    [coder encodeInteger:self.VendorId forKey:NSStringFromSelector(@selector(VendorId))];
    [coder encodeInteger:self.ProductId forKey:NSStringFromSelector(@selector(ProductId))];
    [coder encodeInteger:self.ProductRevision forKey:NSStringFromSelector(@selector(ProductRevision))];

    // Because encoding and decoding structs are a bit of a pain...
    [coder encodeInteger:self.ManufacturingDate.NLManufacturingDateYear forKey:@"NLManufacturingDateYear"];
    [coder encodeInteger:self.ManufacturingDate.NLManufacturingDateMonth forKey:@"NLManufacturingDateMonth"];
    [coder encodeInteger:self.ManufacturingDate.NLManufacturingDateDay forKey:@"NLManufacturingDateDay"];

    [coder encodeObject:self.Primary802154MACAddress forKey:NSStringFromSelector(@selector(Primary802154MACAddress))];
    [coder encodeObject:self.PrimaryWiFiMACAddress forKey:NSStringFromSelector(@selector(PrimaryWiFiMACAddress))];

    [coder encodeObject:self.SerialNumber forKey:NSStringFromSelector(@selector(SerialNumber))];
    [coder encodeObject:self.SoftwareVersion forKey:NSStringFromSelector(@selector(SoftwareVersion))];
    [coder encodeObject:self.RendezvousWiFiESSID forKey:NSStringFromSelector(@selector(RendezvousWiFiESSID))];
    [coder encodeObject:self.PairingCode forKey:NSStringFromSelector(@selector(PairingCode))];

    [coder encodeInteger:self.PairingCompatibilityVersionMajor forKey:NSStringFromSelector(@selector(PairingCompatibilityVersionMajor))];
    [coder encodeInteger:self.PairingCompatibilityVersionMinor forKey:NSStringFromSelector(@selector(PairingCompatibilityVersionMinor))];
}

- (id)copyWithZone:(NSZone *)zone {
    NLWeaveDeviceDescriptor *copy = [[[self class] allocWithZone:zone] init];

    copy.DeviceId = [self.DeviceId copy];
    copy.FabricId = [self.FabricId copy];
    copy.DeviceFeatures = self.DeviceFeatures;
    copy.VendorId = self.VendorId;
    copy.ProductId = self.ProductId;
    copy.ProductRevision = self.ProductRevision;

    copy.ManufacturingDate = self.ManufacturingDate;
    copy.Primary802154MACAddress = [self.Primary802154MACAddress copy];
    copy.PrimaryWiFiMACAddress = [self.PrimaryWiFiMACAddress copy];

    copy.SerialNumber = [self.SerialNumber copy];
    copy.SoftwareVersion = [self.SoftwareVersion copy];
    copy.RendezvousWiFiESSID = [self.RendezvousWiFiESSID copy];
    copy.PairingCode = [self.PairingCode copy];

    copy.PairingCompatibilityVersionMajor = self.PairingCompatibilityVersionMajor;
    copy.PairingCompatibilityVersionMinor = self.PairingCompatibilityVersionMinor;

    return copy;
}

#pragma mark - NSObject

- (NSUInteger)hash {
    return [self.DeviceId hash] ^ self.VendorId;
}

- (BOOL)isEqual:(id)object {
    if (self == object) {
        return YES;
    }

    if (![object isKindOfClass:[NLWeaveDeviceDescriptor class]]) {
        return NO;
    }

    return [self isEqualToNLWeaveDeviceDescriptor:(NLWeaveDeviceDescriptor *) object];
}

- (BOOL)isEqualToNLWeaveDeviceDescriptor:(NLWeaveDeviceDescriptor *)descriptor {
    BOOL equalDeviceId = (self.DeviceId == nil && descriptor.DeviceId == nil) || [self.DeviceId isEqual:descriptor.DeviceId];
    BOOL equalFabricId = (self.FabricId == nil && descriptor.FabricId == nil) || [self.FabricId isEqual:descriptor.FabricId];

    BOOL equalDeviceFeatures = self.DeviceFeatures == descriptor.DeviceFeatures;
    BOOL equalVendorId = self.VendorId == descriptor.VendorId;
    BOOL equalProductId = self.ProductId == descriptor.ProductId;
    BOOL equalProductRevision = self.ProductRevision == descriptor.ProductRevision;

    BOOL equalManufacturingDate = (self.ManufacturingDate.NLManufacturingDateYear == descriptor.ManufacturingDate.NLManufacturingDateYear) &&
                                    (self.ManufacturingDate.NLManufacturingDateMonth == descriptor.ManufacturingDate.NLManufacturingDateMonth) &&
                                    (self.ManufacturingDate.NLManufacturingDateDay == descriptor.ManufacturingDate.NLManufacturingDateDay);

    BOOL equalPrimary802154MACAddress = (self.Primary802154MACAddress == nil && descriptor.Primary802154MACAddress == nil) || [self.Primary802154MACAddress isEqual:descriptor.Primary802154MACAddress];
    BOOL equalPrimaryWiFiMACAddress = (self.PrimaryWiFiMACAddress == nil && descriptor.PrimaryWiFiMACAddress == nil) || [self.PrimaryWiFiMACAddress isEqual:descriptor.PrimaryWiFiMACAddress];

    BOOL equalSerialNumber = (self.SerialNumber == nil && descriptor.SerialNumber == nil) || [self.SerialNumber isEqualToString:descriptor.SerialNumber];
    BOOL equalSoftwareVersion = (self.SoftwareVersion == nil && descriptor.SoftwareVersion == nil) || [self.SoftwareVersion isEqualToString:descriptor.SoftwareVersion];
    BOOL equalRendezvousWiFiESSID = (self.RendezvousWiFiESSID == nil && descriptor.RendezvousWiFiESSID == nil) || [self.RendezvousWiFiESSID isEqualToString:descriptor.RendezvousWiFiESSID];
    BOOL equalPairingCode = (self.PairingCode == nil && descriptor.PairingCode == nil) || [self.PairingCode isEqualToString:descriptor.PairingCode];


    BOOL equalPairingCompatibilityVersionMajor = self.PairingCompatibilityVersionMajor == descriptor.PairingCompatibilityVersionMajor;
    BOOL equalPairingCompatibilityVersionMinor = self.PairingCompatibilityVersionMinor == descriptor.PairingCompatibilityVersionMinor;


    return equalDeviceId && equalFabricId &&
            equalDeviceFeatures && equalVendorId && equalProductId && equalProductRevision &&
            equalManufacturingDate && equalPrimary802154MACAddress && equalPrimaryWiFiMACAddress &&
            equalSerialNumber && equalSoftwareVersion && equalRendezvousWiFiESSID && equalPairingCode &&
            equalPairingCompatibilityVersionMajor && equalPairingCompatibilityVersionMinor;
}

#pragma mark - Protected methods

- (WeaveDeviceDescriptor)toWeaveDeviceDescriptor
{
    WeaveDeviceDescriptor deviceDescriptor;
    return deviceDescriptor;
}

#pragma mark - Private methods

- (NSString *)debugDescriptionWithDeviceDescriptor:(nl::Weave::Profiles::DeviceDescription::WeaveDeviceDescriptor *)pDeviceDescriptor
{
    BOOL linePowered = (pDeviceDescriptor->DeviceFeatures & WeaveDeviceDescriptor::kFeature_LinePowered) == WeaveDeviceDescriptor::kFeature_LinePowered;

    NSUInteger lenV6 = sizeof(pDeviceDescriptor->Primary802154MACAddress) / sizeof(pDeviceDescriptor->Primary802154MACAddress[0]);
    NSUInteger lenV4 = sizeof(pDeviceDescriptor->PrimaryWiFiMACAddress) / sizeof(pDeviceDescriptor->PrimaryWiFiMACAddress[0]);
    NSString *descr = [NSString stringWithFormat:@"(%p), \n"
                                                         "\tDeviceId: %llu\n"
                                                         "\tFabricId: %qX\n"
                                                         "\tDeviceFeatures: %d\n"
                                                         "\tVendorId: %lu\n"
                                                         "\tProductId: %ld\n"
                                                         "\tProductRevision: %lu\n"
                                                         "\tManufacturingDate: %ld/%ld/%ld\n"
                                                         "\tPrimary802154MACAddress len: %lu\n"
                                                         "\tPrimaryWiFiMACAddress len: %lu\n"
                                                         "\tSerialNumber: %s\n"
                                                         "\tSoftwareVersion: %s\n"
                                                         "\tRendezvousWiFiESSID: %s\n"
                                                         "\tPairingCode: %s\n"
                                                         "\tPairingCompatibilityVersionMajor: %d\n"
                                                         "\tPairingCompatibilityVersionMinor: %d\n"
                                                         "\tRequires Line Power: %d\n",
                                                 self,
                                                 pDeviceDescriptor->DeviceId,
                                                 pDeviceDescriptor->FabricId,
                                                 pDeviceDescriptor->DeviceFeatures,
                                                 (unsigned long)pDeviceDescriptor->VendorId,
                                                 (long)pDeviceDescriptor->ProductId,
                                                 (unsigned long)pDeviceDescriptor->ProductRevision,
                                                 (long)pDeviceDescriptor->ManufacturingDate.Month,
                                                 (long)pDeviceDescriptor->ManufacturingDate.Day,
                                                 (long)pDeviceDescriptor->ManufacturingDate.Year,
                                                 (unsigned long)lenV6,
                                                 (unsigned long)lenV4,
                                                 pDeviceDescriptor->SerialNumber,
                                                 pDeviceDescriptor->SoftwareVersion,
                                                 pDeviceDescriptor->RendezvousWiFiESSID,
                                                 pDeviceDescriptor->PairingCode,
                                                 pDeviceDescriptor->PairingCompatibilityVersionMajor,
                                                 pDeviceDescriptor->PairingCompatibilityVersionMinor,
                                                 linePowered];

    return descr;
}

- (NSString *)debugDescription
{
    NSString *descr = [NSString stringWithFormat:@"(%p), \n"
                                                         "\tDeviceId: %llu\n"
                                                         "\tFabricId: %qX\n"
                                                         "\tDeviceFeatures: %ld\n"
                                                         "\tVendorId: %lu\n"
                                                         "\tProductId: %ld\n"
                                                         "\tProductRevision: %lu\n"
                                                         "\tManufacturingDate: %ld/%ld/%ld\n"
                                                         "\tPrimary802154MACAddress: %@\n"
                                                         "\tPrimaryWiFiMACAddress: %@\n"
                                                         "\tSerialNumber: %@\n"
                                                         "\tSoftwareVersion: %@\n"
                                                         "\tRendezvousWiFiESSID: %@\n"
                                                         "\tPairingCode: %@\n"
                                                         "\tPairingCompatibilityVersionMajor: %ld\n"
                                                         "\tPairingCompatibilityVersionMinor: %ld\n"
                                                         "\tRequires Line Power: %d\n",

                                                 self,
                                                 [_DeviceId longLongValue],
                                                 [_FabricId longLongValue],
                                                 (long)_DeviceFeatures,
                                                 (unsigned long)_VendorId,
                                                 (long)_ProductId,
                                                 (unsigned long)_ProductRevision,
                                                 (long)_ManufacturingDate.NLManufacturingDateMonth,
                                                 (long)_ManufacturingDate.NLManufacturingDateDay,
                                                 (long)_ManufacturingDate.NLManufacturingDateYear,
                                                 _Primary802154MACAddress,
                                                 _PrimaryWiFiMACAddress,
                                                 _SerialNumber,
                                                 _SoftwareVersion,
                                                 _RendezvousWiFiESSID,
                                                 _PairingCode,
                                                 (long)_PairingCompatibilityVersionMajor,
                                                 (long)_PairingCompatibilityVersionMinor,
                                                 [self requiresLinePower]];
    return descr;
}

@end
