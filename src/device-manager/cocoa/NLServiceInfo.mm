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
 *      Objective-C representation of the data communicated via service-provisioning profile.
 *
 */

#import "NLServiceInfo.h"
#import "Base64Encoding.h"

@implementation NLServiceInfo

- (id)initWithServiceId:(NLServiceID)serviceId
              accountId:(NSString *)accountId
          serviceConfig:(NSString *)serviceConfig
           pairingToken:(NSString *)pairingToken
        pairingInitData:(NSString *)pairingInitData
{
	if (self = [super init]) {
		_serviceId = serviceId;
		_accountId = accountId;

		_serviceConfig = serviceConfig;

		_pairingToken = pairingToken;
		_pairingInitData = pairingInitData;

	}

	return self;
}

- (id)initWithServiceId:(NLServiceID)serviceId
          serviceConfig:(NSString *)serviceConfig
{
	return [self initWithServiceId:serviceId accountId:nil serviceConfig:serviceConfig pairingToken:nil pairingInitData:nil];
}

- (NSData *)decodeServiceConfig {
	Base64Encoding *base64coder = [Base64Encoding createBase64StringEncoding];
	return [base64coder decode:_serviceConfig];
}

@end
