/*
 *
 *    Copyright (c) 2020 Google LLC
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
 *      This file defines the @c NLWeaveLogWriter protocol for an external Weave log writer.
 *
 */

#import <Foundation/Foundation.h>
#import "NLWeaveLogging.h"

NS_ASSUME_NONNULL_BEGIN

/**
 * Defines an interface for an external log writer to register to receive Weave logs.
 *
 * External clients can implement this protocol to receive Weave log messages for custom handling –
 * the shared @c NLWeaveLogWriter can be set using +[NLWeaveLogging setSharedLogWriter:].
 */
@protocol NLWeaveLogWriter <NSObject>

/**
 * Method called with individual Weave log messages as they are logged.
 *
 * @param logModule The Weave module in which the log was created.
 * @param logLevel The logging level of the log.
 * @param logMessage The formatted log message.
 */
- (void)writeLogFromModule:(NLLogModule)logModule
                     level:(NLLogLevel)logLevel
                   message:(NSString *)logMessage NS_SWIFT_NAME(writeLog(from:level:message:));

@end

NS_ASSUME_NONNULL_END
