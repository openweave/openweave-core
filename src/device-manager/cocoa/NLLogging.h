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

#if DEBUG
#define WDM_LOG_DEBUG NSLog
#define WDM_LOG_ERROR NSLog

#define WDM_LOG_METHOD_SIG()        ({ NSLog(@"[<%@: %p> %@]", NSStringFromClass([self class]), self, NSStringFromSelector(_cmd)); })
#else
// do nothing
#define WDM_LOG_DEBUG(...)
#define WDM_LOG_ERROR(...)
#define WDM_LOG_METHOD_SIG()        ({})

#endif
