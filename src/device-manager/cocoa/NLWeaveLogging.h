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
 *      This file implements a platform-specific Weave log interface.
 *
 */

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@protocol NLWeaveLogWriter;

/**
 * Weave logging modules – for indicating which component created a log.
 *
 * @note
 *   Must align with the order of nl::Weave::Logging::LogModule values.
 */
typedef NS_ENUM(NSInteger, NLLogModule) {
    NLLogModuleNotSpecified = 0,

    NLLogModuleInet,
    NLLogModuleBle,
    NLLogModuleMessageLayer,
    NLLogModuleSecurityManager,
    NLLogModuleExchangeManager,
    NLLogModuleTLV,
    NLLogModuleASN1,
    NLLogModuleCrypto,
    NLLogModuleDeviceManager,
    NLLogModuleAlarm,
    NLLogModuleBDX,
    NLLogModuleDataManagement,
    NLLogModuleDeviceControl,
    NLLogModuleDeviceDescription,
    NLLogModuleEcho,
    NLLogModuleFabricProvisioning,
    NLLogModuleNetworkProvisioning,
    NLLogModuleServiceDirectory,
    NLLogModuleServiceProvisioning,
    NLLogModuleSoftwareUpdate,
    NLLogModuleTokenPairing,
    NLLogModuleHeatLink,
    NLLogModuleTimeService,
    NLLogModuleWeaveTunnel,
    NLLogModuleHeartbeat,
    NLLogModuleWeaveSystemLayer,
    NLLogModuleDropcamLegacyPairing,
    NLLogModuleEventLogging,
    NLLogModuleSupport,

    // Module for logs originating from the Objective-C @c NLLogging macros.
    //
    // Must NOT overlap with the values mapped from nl::Weave::Logging::LogModule.
    NLLogModuleCocoa = 100,
};

/**
 * Logging levels – for indicating the relative importance of a log message.
 *
 * @note
 *   Must align with the order of nl::Weave::Logging::LogCategory values.
 */
typedef NS_ENUM(NSInteger, NLLogLevel) {
    NLLogLevelNone = 0,
    NLLogLevelError,
    NLLogLevelProgress,
    NLLogLevelDetail,
    NLLogLevelRetain,
};

/**
 * Platform-specific component for managing Weave log messages.
 *
 * Exposes an interface for external clients to configure the shared @c NLWeaveLogWriter – the log
 * writer will receive Weave logs from both the shared C++ source code and the platform-specific
 * Objective-C source code. This allows clients to connect Weave logs to their own logging system.
 */
@interface NLWeaveLogging : NSObject

#pragma mark Logging Configuration

/**
 * Sets the shared @c NLWeaveLogWriter to start receiving Weave logs.
 *
 * @note
 *   This should be called prior to any Weave operations to ensure the log writer has been
 *   configured before any logs are written. The log writer will only receive messages logged after
 *   it has been set as the shared writer.
 */
+ (void)setSharedLogWriter:(nullable id<NLWeaveLogWriter>)logWriter;

#pragma mark Log Methods

/**
 * Internal handler method for logging a message to the console and notifying the shared log writer.
 *
 * @param logModule The logging module to which the log belongs.
 * @param logModuleName The name of the logging module.
 * @param logLevel The level of the log message.
 * @param formattedLogMessage The formatted log message.
 */
+ (void)handleWeaveLogFromModule:(NLLogModule)logModule
                      moduleName:(NSString *)logModuleName
                           level:(NLLogLevel)logLevel
                formattedMessage:(NSString *)formattedLogMessage;

@end

NS_ASSUME_NONNULL_END
