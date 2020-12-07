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
 *      This file implements a platform-specific Weave log interface.
 *
 */

#include <stdarg.h>
#include <stdio.h>

#import <Foundation/Foundation.h>
#import "NLWeaveLogWriter.h"
#import "NLWeaveLogging.h"

#if TARGET_OS_IPHONE
#import <UIKit/UIKit.h>
#import <os/log.h>
#endif

#include <Weave/Support/logging/WeaveLogging.h>

NS_ASSUME_NONNULL_BEGIN

#if WEAVE_ERROR_LOGGING || WEAVE_PROGRESS_LOGGING || WEAVE_DETAIL_LOGGING

#pragma mark - nl::Weave::Logging::Log

namespace nl {
namespace Weave {
    namespace Logging {

        /*
         *  void Log()
         *
         *  Description:
         *    This routine writes to the Foundation log stream, the
         *    specified Weave-related variable argument message for the
         *    specified Weave module and category.
         *
         *  Input(s):
         *    aModule   - An enumeration indicating the Weave module or
         *                subsystem the message is associated with.
         *    aCategory - An enumeration indicating the Weave category
         *                the message is associated with.
         *    aMsg      - A NULL-terminated C string containing the message,
         *                with C Standard I/O-style format specifiers, to log.
         *    ...       - A variable argument list, corresponding to format
         *                specifiers in the message.
         *
         *  Output(s):
         *    N/A
         *
         *  Returns:
         *    N/A
         *
         */
        void Log(uint8_t aModule, uint8_t aCategory, const char * aMsg, ...)
        {
            va_list v;

            va_start(v, aMsg);

            if (IsCategoryEnabled(aCategory)) {
                char formattedMsg[512];
                size_t prefixLen;

                char moduleName[nlWeaveLoggingModuleNameLen + 1];
                GetModuleName(moduleName, aModule);

                prefixLen = snprintf(formattedMsg, sizeof(formattedMsg), "WEAVE:%s: %s", moduleName,
                    (aCategory == kLogCategory_Error) ? "ERROR: " : "");
                if (prefixLen >= sizeof(formattedMsg))
                    prefixLen = sizeof(formattedMsg) - 1;

                vsnprintf(formattedMsg + prefixLen, sizeof(formattedMsg) - prefixLen, aMsg, v);

                // Pass the formatted log to @c NLWeaveLogging for handling.
                [NLWeaveLogging handleWeaveLogFromModule:(NLLogModule) aModule
                                              moduleName:[NSString stringWithUTF8String:moduleName]
                                                   level:(NLLogLevel) aCategory
                                        formattedMessage:[NSString stringWithUTF8String:formattedMsg]];
            }

            va_end(v);
        }

    } // namespace Logging
} // namespace Weave
} // namespace nl

#endif // WEAVE_ERROR_LOGGING || WEAVE_PROGRESS_LOGGING || WEAVE_DETAIL_LOGGING

#if TARGET_OS_IPHONE && DEBUG

/**
 * @def IS_OPERATING_SYSTEM_AT_LEAST_VERSION(version)
 *
 * @brief
 *   Compares the current version of operating system to the given version.
 *
 *  @param version An @c NSOperatingSystemVersion describing an iOS operating system version.
 *
 *  @return A boolean indicating whether the operating system version is at least the given version.
 */
#define IS_OPERATING_SYSTEM_AT_LEAST_VERSION(version)                                                                              \
    ([NSProcessInfo instancesRespondToSelector:@selector(isOperatingSystemAtLeastVersion:)] &&                                     \
        [[NSProcessInfo new] isOperatingSystemAtLeastVersion:version])

/** Minimum iOS version with OSLog support (iOS 10.0.0). */
static const NSOperatingSystemVersion kMinimumOSLogOperatingSystemVersion
    = { .majorVersion = 10, .minorVersion = 0, .patchVersion = 0 };

/** Root of the OSLog subsystem for Weave logs. */
static NSString * const kWeaveOSLogSubsystem = @"com.nest.weave";

/** Name of the subsystem for Weave logs from the shared C++ code. */
static NSString * const kWeaveCppComponent = @"cpp";

/** Name of the subsystem for Weave logs from the platform-specific Objective-C code. */
static NSString * const kWeaveCocoaComponent = @"cocoa";

/** Root of the OSLog category for Weave logs. */
static NSString * const kWeaveOSLogCategory = @"WEAVE";

#pragma mark - @c NLOSLogStore

/** Stores a dictionary mapping each @c NLLogModule to an OSLog logging component. */
@interface NLOSLogStore : NSObject

/** Returns the @c os_log_t for the given @c NLLogModule, creating a new one if needed. */
+ (os_log_t)logForModule:(NLLogModule)logModule name:(NSString *)moduleName;

@end

@implementation NLOSLogStore

/** Map of @c NLLogModule values to their OS Log object. */
static NSMutableDictionary<NSNumber *, os_log_t> * gModuleMap;

#pragma mark Initialization

+ (void)initialize
{
    if (self != [NLOSLogStore class]) {
        return;
    }

    @synchronized(self) {
        gModuleMap = [[NSMutableDictionary alloc] init];
    }
}

#pragma mark Public

+ (os_log_t)logForModule:(NLLogModule)logModule name:(NSString *)logModuleName
{
    @synchronized(self) {
        if (!gModuleMap[@(logModule)]) {
            gModuleMap[@(logModule)] = [self createLogForModule:logModule name:logModuleName];
        }

        return gModuleMap[@(logModule)];
    }
}

#pragma mark Private

+ (os_log_t)createLogForModule:(NLLogModule)logModule name:(NSString *)name
{
    NSString * component = (logModule == NLLogModuleCocoa ? kWeaveCocoaComponent : kWeaveCppComponent);
    NSString * subsystem = [NSString stringWithFormat:@"%@.%@", kWeaveOSLogSubsystem, component];
    NSString * category = [NSString stringWithFormat:@"%@.%@", kWeaveOSLogCategory, name];
    return os_log_create(subsystem.UTF8String, category.UTF8String);
}

@end

#endif // TARGET_OS_IPHONE && DEBUG

#pragma mark - @c NLWeaveLogging

@implementation NLWeaveLogging

/** The shared external log writer that is subscribed to receive Weave logs. */
static __nullable id<NLWeaveLogWriter> gSharedWriter = nil;

#pragma mark - Public

+ (void)setSharedLogWriter:(nullable id<NLWeaveLogWriter>)logWriter
{
    @synchronized(self) {
        gSharedWriter = logWriter;
    }
}

+ (void)handleWeaveLogFromModule:(NLLogModule)logModule
                      moduleName:(NSString *)logModuleName
                           level:(NLLogLevel)logLevel
                formattedMessage:(NSString *)formattedLogMessage
{
    if (!formattedLogMessage) {
        return;
    }

    // 1. Write the log to the system console (in debug builds).
#if DEBUG
    [self writeConsoleLog:formattedLogMessage logModule:logModule logModuleName:logModuleName logLevel:logLevel];
#endif // DEBUG

    // 2. Notify the shared log writer.
    id<NLWeaveLogWriter> writer = gSharedWriter;
    [writer writeLogFromModule:logModule level:logLevel message:formattedLogMessage];
}

#pragma mark - Private

#if DEBUG

/** Writes a log message to the system console. */
+ (void)writeConsoleLog:(NSString *)formattedLogMessage
              logModule:(NLLogModule)logModule
          logModuleName:(NSString *)logModuleName
               logLevel:(NLLogLevel)logLevel
{
#if TARGET_OS_IPHONE
    // For iOS 10+, OSLog replaces Apple System Logger (and NSLog).
    if (IS_OPERATING_SYSTEM_AT_LEAST_VERSION(kMinimumOSLogOperatingSystemVersion)) {
        os_log_t log = [NLOSLogStore logForModule:logModule name:logModuleName];
        os_log_type_t type = (logLevel == NLLogLevelError ? OS_LOG_TYPE_ERROR : OS_LOG_TYPE_DEFAULT);
        os_log_with_type(log, type, "%{public}s", formattedLogMessage.UTF8String);
        return;
    }
#endif // TARGET_OS_IPHONE

    // Fallback to NSLog for both iOS <9 and non-iPhone targets.
    NSLog(@"%@", formattedLogMessage);
}

#endif // DEBUG

@end

NS_ASSUME_NONNULL_END
