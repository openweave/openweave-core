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
#include <Weave/Core/WeaveCore.h>

#define STRINGIFY(X) #X

#ifdef WEAVE_NON_PRODUCTION_MARKER

/** A sentinel symbol/string that signals code should be used in non-production contexts only.
 *
 * Release engineers are encouraged to add post-build checks for this symbol/string to protect against accidental
 * inclusion of non-production features in production builds.
 */
const char WEAVE_NON_PRODUCTION_MARKER[] = STRINGIFY(WEAVE_NON_PRODUCTION_MARKER);

#endif // WEAVE_NON_PRODUCTION_MARKER
