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

#if TARGET_OS_IPHONE
#import <UIKit/UIKit.h>
#import <os/log.h>
#endif

#include <Weave/Support/logging/WeaveLogging.h>

#if WEAVE_ERROR_LOGGING || WEAVE_PROGRESS_LOGGING || WEAVE_DETAIL_LOGGING

#define SYSTEM_VERSION_GREATER_THAN_OR_EQUAL_TO(v)                                                                                 \
    ([[[UIDevice currentDevice] systemVersion] compare:v options:NSNumericSearch] != NSOrderedAscending)

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

                // For versions >= iOS 10, NSLog is not using Apple System logger anymore
                // So for the logs to always show up in device console, we need to use os_log
                // with OS_LOG_DEFAULT which always gets logged in accordance with system's
                // standard behavior
#if TARGET_OS_IPHONE
                if (SYSTEM_VERSION_GREATER_THAN_OR_EQUAL_TO(@"10.0")) {
                    os_log(OS_LOG_DEFAULT, "%s", formattedMsg);
                } else {
                    NSLog(@"%s", formattedMsg);
                }
#else
                NSLog(@"%s", formattedMsg);
#endif
            }

            va_end(v);
        }

    } // namespace Logging

} // namespace Weave

} // namespace nl

#endif // WEAVE_ERROR_LOGGING || WEAVE_PROGRESS_LOGGING || WEAVE_DETAIL_LOGGING
