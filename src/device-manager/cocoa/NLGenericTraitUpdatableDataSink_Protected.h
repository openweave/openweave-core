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
 *      This file provides defines non-public portion of NLWeaveDeviceManager interface
 *
 */

#include <Weave/Core/WeaveCore.h>
#include <WeaveDataManagementClient.h>
#import "NLGenericTraitUpdatableDataSink.h"

@interface NLGenericTraitUpdatableDataSink ()
- (instancetype)init:(NSString *)name
      weaveWorkQueue:(dispatch_queue_t)weaveWorkQueue
    appCallbackQueue:(dispatch_queue_t)appCallbackQueue
    genericTraitUpdatableDataSinkPtr: (nl::Weave::DeviceManager::GenericTraitUpdatableDataSink *)dataSinkPtr
         nlWdmClient:(NLWdmClient *)nlWdmClient NS_DESIGNATED_INITIALIZER;
- (NSString *)statusReportToString:(NSUInteger)profileId statusCode:(NSInteger)statusCode;

/**
 *  @brief Forcifully release all resources and destroy all references.
 *
 */
- (void)close;

@end
