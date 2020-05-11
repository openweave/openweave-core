/*
 *
 *    Copyright (c) 2020 Google LLC.
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
 *      This file provides defines non-public portion of NLWdmClient interface.
 *      This is WEAVE_CONFIG_DATA_MANAGEMENT_EXPERIMENTAL feature.
 *
 */

#include <Weave/Core/WeaveCore.h>
#import "NLWeaveDeviceManager.h"
#import "NLWdmClient.h"

@interface NLWdmClient ()
- (instancetype)init:(NSString *)name
          weaveWorkQueue:(dispatch_queue_t)weaveWorkQueue
        appCallbackQueue:(dispatch_queue_t)appCallbackQueue
             exchangeMgr:(nl::Weave::WeaveExchangeManager *)exchangeMgr
            messageLayer:(nl::Weave::WeaveMessageLayer *)messageLayer
    nlWeaveDeviceManager:(NLWeaveDeviceManager *)deviceMgr NS_DESIGNATED_INITIALIZER;

- (NSString *)statusReportToString:(NSUInteger)profileId statusCode:(NSInteger)statusCode;

- (void)removeDataSinkRef:(long long)traitInstancePtr;
- (NLGenericTraitUpdatableDataSink *)getDataSink:(long long)traitInstancePtr;

@end
