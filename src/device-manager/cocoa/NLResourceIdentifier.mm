/*
 *
 *    Copyright (c) 2019 Google, LLC
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
 *      Objective-C representation of ResourceIdentifier.
 *
 */

#import "NLResourceIdentifier_Protected.h"

const int32_t RESOURCE_TYPE_RESERVED = 0;
const uint64_t SELF_NODE_ID = 0xfffffffffffffffe;

@implementation NLResourceIdentifier

- (instancetype)init
{
    return self = [self initWithType:RESOURCE_TYPE_RESERVED id:SELF_NODE_ID];
}

- (instancetype)initWithType:(int32_t)type
                          id:(uint64_t)idValue
{

    if (self = [super init]) {
        _ResourceType = type;
        _ResourceId = idValue;
    }

    return self;
}

#pragma mark - Protected methods

- (ResourceIdentifier)toResourceIdentifier
{
    return ResourceIdentifier(_ResourceType, _ResourceId);
}

@end
