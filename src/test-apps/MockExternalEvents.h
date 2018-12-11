/*
 *
 *    Copyright (c) 2016-2017 Nest Labs, Inc.
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
 *      This file declares mock external event generators
 *
 */
#ifndef MOCKEXTERNALEVENTS_H
#define MOCKEXTERNALEVENTS_H

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#include <Weave/Core/WeaveCore.h>

#if WEAVE_CONFIG_EVENT_LOGGING_EXTERNAL_EVENT_SUPPORT
WEAVE_ERROR LogMockExternalEvents(size_t aNumEvents, int numCallback);
WEAVE_ERROR LogMockDebugExternalEvents(size_t aNumEvents, int numCallback);
void ClearMockExternalEvents(int numCallback);
#endif

#endif /* MOCKEXTERNALEVENTS_H */
