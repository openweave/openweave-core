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
 *      Base-64 utility functions.
 *
 */

#import <Foundation/Foundation.h>

@interface Base64Encoding : NSObject {
}

@property (nonatomic) BOOL usePaddingDuringEncoding;

+ (id)createBase64StringEncoding;

- (NSString *)encode:(NSData *)inData;
- (NSData *)decode:(NSString *)string;

@end
