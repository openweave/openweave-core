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

#import "NLNetworkInfo_Protected.h"

// using nl::Weave::Profiles::NetworkProvisioning;
using nl::Weave::Profiles::NetworkProvisioning::NetworkType;
using nl::Weave::Profiles::NetworkProvisioning::WiFiMode;
using nl::Weave::Profiles::NetworkProvisioning::WiFiRole;
using nl::Weave::Profiles::NetworkProvisioning::WiFiSecurityType;

@implementation NLNetworkInfo

+ (NLNetworkInfo *)createUsing:(NetworkInfo *)pNetworkInfo {
    return [[NLNetworkInfo alloc] initWith:pNetworkInfo];
}

- (id)initWithWiFiSSID:(NSString *)ssid wifiKey:(NSData *)wifiKey securityType:(NLWiFiSecurityType)securityType {
	if (self = [super init]) {
		_WiFiSSID = ssid;

        if (wifiKey)
            _WiFiKey = wifiKey;

		_WiFiSecurityType = securityType;

		_NetworkType = kNLNetworkType_WiFi;
		_WiFiMode = kNLWiFiMode_Managed;
		_WiFiRole = kNLWiFiRole_Station;

        _NetworkId = -1;
	}
	return self;
}

- (id)initWith:(nl::Weave::DeviceManager::NetworkInfo *)pNetworkInfo {
    if (self = [super init]) {
        _NetworkType = (NLNetworkType)pNetworkInfo->NetworkType;
        _NetworkId = pNetworkInfo->NetworkId;

        if (pNetworkInfo->WiFiSSID)
        {
            _WiFiSSID = [[NSString alloc] initWithCString:pNetworkInfo->WiFiSSID encoding:NSUTF8StringEncoding];
            _WiFiMode = (NLWiFiMode) pNetworkInfo->WiFiMode;
            _WiFiRole = (NLWiFiRole) pNetworkInfo->WiFiRole;
            _WiFiSecurityType = (NLWiFiSecurityType) pNetworkInfo->WiFiSecurityType;
        }

        if (pNetworkInfo->WiFiKey)
        {
            printf("pNetworkInfo->WiFiKey: %s\n", pNetworkInfo->WiFiKey);
            _WiFiKey = [NSData dataWithBytes:pNetworkInfo->WiFiKey length:pNetworkInfo->WiFiKeyLen];
        }

        if (pNetworkInfo->ThreadNetworkName)
            _ThreadNetworkName = [[NSString alloc] initWithCString:pNetworkInfo->ThreadNetworkName encoding:NSUTF8StringEncoding];

        if (pNetworkInfo->ThreadExtendedPANId)
        {
            _ThreadExtendedPANId = [[NSData alloc] initWithBytes:pNetworkInfo->ThreadExtendedPANId length:8];
        }

        if (pNetworkInfo->ThreadNetworkKey)
        {
            NSLog(@"Copying ThreadNetworkKey with length: %d", pNetworkInfo->ThreadNetworkKeyLen);
            _ThreadNetworkKey = [[NSData alloc] initWithBytes:pNetworkInfo->ThreadNetworkKey length:pNetworkInfo->ThreadNetworkKeyLen];
            NSLog(@"_ThreadNetworkKey: %@", _ThreadNetworkKey);
        }
        _ThreadPANId = pNetworkInfo->ThreadPANId;
        _ThreadChannel = pNetworkInfo->ThreadChannel;

        _WirelessSignalStrength = pNetworkInfo->WirelessSignalStrength;
    }
    return self;
}

- (id)copyWithZone:(NSZone *)zone
{
    NLNetworkInfo *networkInfo = [[NLNetworkInfo allocWithZone:zone] init];

    networkInfo.NetworkType = self.NetworkType;
    networkInfo.NetworkId = self.NetworkId;

    networkInfo.WiFiSSID = [self.WiFiSSID copy];
    networkInfo.WiFiMode = self.WiFiMode;
    networkInfo.WiFiRole = self.WiFiRole;

    networkInfo.WiFiSecurityType = self.WiFiSecurityType;

    networkInfo.WiFiKey = [self.WiFiKey copy];

    networkInfo.ThreadNetworkName = [self.ThreadNetworkName copy];
    networkInfo.ThreadExtendedPANId = [self.ThreadExtendedPANId copy];
    networkInfo.ThreadNetworkKey = [self.ThreadNetworkKey copy];
    networkInfo.ThreadPANId = self.ThreadPANId;
    networkInfo.ThreadChannel = self.ThreadChannel;

    networkInfo.WirelessSignalStrength = self.WirelessSignalStrength;

    return networkInfo;
}

#pragma mark - Protected methods

- (NetworkInfo)toNetworkInfo {

    NetworkInfo networkInfo;

    networkInfo.NetworkType = [self toNetworkType:_NetworkType];
    printf("toNetworkInfo, _NetworkId, %lld\n", _NetworkId);
    networkInfo.NetworkId = _NetworkId;

    if (_WiFiSSID)
        networkInfo.WiFiSSID = strdup([_WiFiSSID UTF8String]);

    networkInfo.WiFiMode = [self toWifiMode:_WiFiMode];
    networkInfo.WiFiRole = [self toWifiRole:_WiFiRole];
    networkInfo.WiFiSecurityType = [self toWifiSecurityType:_WiFiSecurityType];

    if (_WiFiKey)
    {
        networkInfo.WiFiKeyLen = [_WiFiKey length];
        networkInfo.WiFiKey = (uint8_t *) malloc(networkInfo.WiFiKeyLen);
        assert(networkInfo.WiFiKey != NULL);
        memcpy(networkInfo.WiFiKey, [_WiFiKey bytes], networkInfo.WiFiKeyLen);

    }

    if (_ThreadNetworkName)
        networkInfo.ThreadNetworkName = strdup([_ThreadNetworkName UTF8String]);

    if (_ThreadExtendedPANId)
    {
        networkInfo.ThreadExtendedPANId = (uint8_t *) malloc(8);
        memcpy(networkInfo.ThreadExtendedPANId, [_ThreadExtendedPANId bytes], 8);
    }

    if (_ThreadNetworkKey)
    {
        networkInfo.ThreadNetworkKeyLen = [_ThreadNetworkKey length];
        networkInfo.ThreadNetworkKey = (uint8_t *) malloc(networkInfo.ThreadNetworkKeyLen);
        memcpy(networkInfo.ThreadNetworkKey, [_ThreadNetworkKey bytes], networkInfo.ThreadNetworkKeyLen);
    }

    networkInfo.ThreadPANId = _ThreadPANId;
    networkInfo.ThreadChannel = _ThreadChannel;
    networkInfo.WirelessSignalStrength = _WirelessSignalStrength;

    return networkInfo;
}

- (nl::Weave::Profiles::NetworkProvisioning::NetworkType)toNetworkType:(NLNetworkType)networkType
{
    switch (networkType) {
        case kNLNetworkType_WiFi:                       return nl::Weave::Profiles::NetworkProvisioning::kNetworkType_WiFi;
        case kNLNetworkType_Thread:                     return nl::Weave::Profiles::NetworkProvisioning::kNetworkType_Thread;
        default:                                        return nl::Weave::Profiles::NetworkProvisioning::kNetworkType_NotSpecified;
    }
}

- (nl::Weave::Profiles::NetworkProvisioning::WiFiMode)toWifiMode:(NLWiFiMode)mode
{
    switch (mode) {
        case kNLWiFiMode_AdHoc:                         return nl::Weave::Profiles::NetworkProvisioning::kWiFiMode_AdHoc;
        case kNLWiFiMode_Managed:                       return nl::Weave::Profiles::NetworkProvisioning::kWiFiMode_Managed;
        default:                                        return nl::Weave::Profiles::NetworkProvisioning::kWiFiMode_NotSpecified;
    }
}

- (nl::Weave::Profiles::NetworkProvisioning::WiFiRole)toWifiRole:(NLWiFiRole)role
{
    switch (role) {
        case kNLWiFiRole_Station:                       return nl::Weave::Profiles::NetworkProvisioning::kWiFiRole_Station;
        case kNLWiFiRole_AccessPoint:                   return nl::Weave::Profiles::NetworkProvisioning::kWiFiRole_AccessPoint;
        default:                                        return nl::Weave::Profiles::NetworkProvisioning::kWiFiRole_NotSpecified;
    }
}

- (nl::Weave::Profiles::NetworkProvisioning::WiFiSecurityType)toWifiSecurityType:(NLWiFiSecurityType)securityType
{
    switch (securityType) {
        case kNLWiFiSecurityType_NotSpecified:          return nl::Weave::Profiles::NetworkProvisioning::kWiFiSecurityType_NotSpecified;
        case kNLWiFiSecurityType_None:                  return nl::Weave::Profiles::NetworkProvisioning::kWiFiSecurityType_None;
        case kNLWiFiSecurityType_WEP:                   return nl::Weave::Profiles::NetworkProvisioning::kWiFiSecurityType_WEP;
        case kNLWiFiSecurityType_WPAPersonal:           return nl::Weave::Profiles::NetworkProvisioning::kWiFiSecurityType_WPAPersonal;
        case kNLWiFiSecurityType_WPA2Personal:          return nl::Weave::Profiles::NetworkProvisioning::kWiFiSecurityType_WPA2Personal;
        case kNLWiFiSecurityType_WPA2MixedPersonal:     return nl::Weave::Profiles::NetworkProvisioning::kWiFiSecurityType_WPA2MixedPersonal;
        case kNLWiFiSecurityType_WPAEnterprise:         return nl::Weave::Profiles::NetworkProvisioning::kWiFiSecurityType_WPAEnterprise;
        case kNLWiFiSecurityType_WPA2Enterprise:        return nl::Weave::Profiles::NetworkProvisioning::kWiFiSecurityType_WPA2Enterprise;
        case kNLWiFiSecurityType_WPA2MixedEnterprise:   return nl::Weave::Profiles::NetworkProvisioning::kWiFiSecurityType_WPA2MixedEnterprise;
        default:                                        return nl::Weave::Profiles::NetworkProvisioning::kWiFiSecurityType_NotSpecified;
    }
}

#pragma mark - Public methods

- (NSString *)WiFiSecurityTypeAsString {
    switch (_WiFiSecurityType) {
        case kNLWiFiSecurityType_NotSpecified:
            return @"NotSpecified";
        case kNLWiFiSecurityType_None:
            return @"None";
        case kNLWiFiSecurityType_WEP:
            return @"WEP";
        case kNLWiFiSecurityType_WPAPersonal:
            return @"WPAPersonal";
        case kNLWiFiSecurityType_WPA2Personal:
            return @"WPA2Personal";
        case kNLWiFiSecurityType_WPA2MixedPersonal:
            return @"WPA2MixedPersonal";
        case kNLWiFiSecurityType_WPAEnterprise:
            return @"WPAEnterprise";
        case kNLWiFiSecurityType_WPA2Enterprise:
            return @"WPA2Enterprise";
        case kNLWiFiSecurityType_WPA2MixedEnterprise:
            return @"WPA2MixedEnterprise";
        default:
            return @"Unknown NLWiFiSecurityType";
    }
}

#pragma mark - Private methods

- (NSString *)debugDescriptionWithNetworkInfo:(nl::Weave::DeviceManager::NetworkInfo *)pNetworkInfo {
    NSString *descr = [NSString stringWithFormat:@"(%p), \n"
                                                         "\tNetworkType: %d\n"
                                                         "\tNetworkId: %lld\n"
                                                         "\tWiFiSSID: %s\n"
                                                         "\tWiFiMode: %d\n"
                                                         "\tWiFiRole: %d\n"
                                                         "\tWiFiSecurityType: %d\n"
                                                         "\tWiFiKey: %s\n"
                                                         "\tThreadNetworkName: %s\n"
                                                         "\tThreadPANId: %04x\n"
                                                         "\tThreadExtendedPANId: %s\n"
                                                         "\tThreadNetworkKeyLen: %d\n"
                                                         "\tThreadChannel: %d\n"
                                                         "\tWirelessSignalStrength: %d",
                                                 self,
                                                 pNetworkInfo->NetworkType,
                                                 pNetworkInfo->NetworkId,
                                                 pNetworkInfo->WiFiSSID,
                                                 pNetworkInfo->WiFiMode,
                                                 pNetworkInfo->WiFiRole,
                                                 pNetworkInfo->WiFiSecurityType,
                                                 pNetworkInfo->WiFiKey,
#if NLNETWORKINFO_DEBUG
                                                 pNetworkInfo->WiFiKey ? "*********" : "none",
#endif
                                                 pNetworkInfo->ThreadNetworkName,
                                                 pNetworkInfo->ThreadPANId,
                                                 pNetworkInfo->ThreadExtendedPANId,
                                                 pNetworkInfo->ThreadNetworkKeyLen,
                                                 pNetworkInfo->ThreadChannel,
                                                 pNetworkInfo->WirelessSignalStrength];

    return descr;
}

- (NSString *)debugDescription
{
    NSMutableString *descr = [[NSMutableString alloc] init];
    NSString *line = [NSString stringWithFormat:@"(%p), \n"
                                                         "\tNetworkType: %ld\n",
                                                         self,
                                                 (long)_NetworkType];

    [descr appendString:line];
    if (_NetworkId != NLNetworkID_NotSpecified)
    {
        line = [NSString stringWithFormat:@"\tNetworkId: %lld\n", _NetworkId];
        [descr appendString:line];
    } else {
        [descr appendString:@"NetworkId: -1 (Unspecified)"];
    }

    if (_WiFiSSID)
    {
        line = [NSString stringWithFormat:@"\tWiFiSSID: %@\n", _WiFiSSID];
        [descr appendString:line];
    }

    line = [NSString stringWithFormat:@"\tWiFiMode: %ld\n"
                                        "\tWiFiRole: %ld\n"
                                        "\tWiFiSecurityType: %ld\n"
                                        "\tWirelessSignalStrength: %ld\n",
                                                 (long)_WiFiMode,
                                                 (long)_WiFiRole,
                                                 (long)_WiFiSecurityType,
                                                 (long)_WirelessSignalStrength];
    [descr appendString:line];

    if (_WiFiKey)
    {
        line = [NSString stringWithFormat:@"\tWiFiKey: <exists>\n"];
        [descr appendString:line];
    }

    if (_ThreadNetworkName)
    {
        line = [NSString stringWithFormat:@"\tThreadNetworkName: %@\n", _ThreadNetworkName];
        [descr appendString:line];
    }

    if (_ThreadPANId != NLThreadPANId_NotSpecified)
    {
        line = [NSString stringWithFormat:@"\tThreadPANId: %04x\n", _ThreadPANId];
        [descr appendString:line];
    }

    if (_ThreadExtendedPANId)
    {
        line = [NSString stringWithFormat:@"\tThreadExtendedPANId: %@\n", _ThreadExtendedPANId];
        [descr appendString:line];
    }

    if (_ThreadNetworkKey)
    {
        line = [NSString stringWithFormat:@"\tThreadNetworkKey: %@\n", _ThreadNetworkKey];
        [descr appendString:line];
    }

    if (_ThreadChannel != NLThreadChannel_NotSpecified)
    {
        line = [NSString stringWithFormat:@"\tThreadChannel: %d\n", _ThreadChannel];
        [descr appendString:line];
    }

    return descr;
}

@end
