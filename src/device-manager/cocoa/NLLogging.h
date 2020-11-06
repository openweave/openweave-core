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
 *      Macros for different levels of logging.
 *
 */

#import <Foundation/Foundation.h>
#import "NLWeaveLogging.h"

NS_ASSUME_NONNULL_BEGIN

#if DEBUG

/** Name of the logging module for the Objective-C platform-specific component. */
static NSString * const kNLWeaveDeviceManagerCocoaModuleName = @"DM-Cocoa";

/** Macros for logging Weave messages of various log levels from platform-specific code. */
#define WDM_LOG_DEBUG(fmt, ...) _WDM_LOG(NLLogLevelDetail, fmt, ##__VA_ARGS__)
#define WDM_LOG_INFO(fmt, ...) _WDM_LOG(NLLogLevelProgress, fmt, ##__VA_ARGS__)
#define WDM_LOG_ERROR(fmt, ...) _WDM_LOG(NLLogLevelError, fmt, ##__VA_ARGS__)

/** Macro for logging a method signature (must be called from an Objective-C method). */
#define WDM_LOG_METHOD_SIG() WDM_LOG_INFO(@"<%@: %p>", NSStringFromClass([self class]), self)

/**
 * @def _WDM_LOG(logLevel, fmt, ...)
 *
 * @brief
 *   Helper macro for formatting Weave logs and delegating log handling to @c NLWeaveLogging.
 *
 *  @param logLevel An @c NLLogLevel specifying the level of the log message.
 *  @param fmt The log message format.
 *  @param ... A variable number of arguments to be added to the log message format.
 */
#define _WDM_LOG(logLevel, fmt, ...)                                                                                               \
    do {                                                                                                                           \
        NSString * formattedMessage = ([NSString                                                                                   \
            stringWithFormat:@"%s:%d %@", __PRETTY_FUNCTION__, __LINE__, [NSString stringWithFormat:fmt, ##__VA_ARGS__]]);         \
                                                                                                                                   \
        [NLWeaveLogging handleWeaveLogFromModule:NLLogModuleCocoa                                                                  \
                                      moduleName:kNLWeaveDeviceManagerCocoaModuleName                                              \
                                           level:logLevel                                                                          \
                                formattedMessage:formattedMessage];                                                                \
    } while (0);

#else // DEBUG

// Do nothing – strip logs from release builds.
#define WDM_LOG_DEBUG(...)
#define WDM_LOG_INFO(...)
#define WDM_LOG_ERROR(...)
#define WDM_LOG_METHOD_SIG() ({})

#endif // DEBUG

NS_ASSUME_NONNULL_END
