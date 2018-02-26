/*
 *
 *    Copyright (c) 2017 Nest Labs, Inc.
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
 *      Namespace redirection header file for Weave Echo profile.
 *
 */

#ifndef WEAVE_ECHO_H_
#define WEAVE_ECHO_H_

#include <Weave/Support/ManagedNamespace.hpp>

// Select "Current" version by default.
#ifndef WEAVE_CONFIG_ECHO_NAMESPACE
#define WEAVE_CONFIG_ECHO_NAMESPACE kWeaveManagedNamespace_Current
#endif

// Include the appropriate header file based on the requested version, or fail if the requested version doesn't exist.
#if WEAVE_CONFIG_ECHO_NAMESPACE == kWeaveManagedNamespace_Current
#include <Weave/Profiles/echo/Current/WeaveEcho.h>
#elif WEAVE_CONFIG_ECHO_NAMESPACE == kWeaveManagedNamespace_Next
#include <Weave/Profiles/echo/Next/WeaveEcho.h>
#else
#error "Invalid WEAVE_CONFIG_ECHO_NAMESPACE selected"
#endif

#endif // WEAVE_ECHO_H_
