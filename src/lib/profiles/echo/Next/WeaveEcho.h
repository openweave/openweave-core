/*
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
 *      Definitions for the Weave Echo profile.
 *
 */

#ifndef WEAVE_ECHO_NEXT_H_
#define WEAVE_ECHO_NEXT_H_

#include <Weave/Support/NLDLLUtil.h>
#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveBinding.h>
#include <Weave/Core/WeaveServerBase.h>
#include <Weave/Profiles/WeaveProfiles.h>

namespace nl {
namespace Weave {
namespace Profiles {

namespace Echo_Next { }
namespace Echo = Echo_Next;

namespace Echo_Next {


/**
 * Weave Echo Profile Message Types
 */
enum
{
	kEchoMessageType_EchoRequest			        = 1,    //< Weave Echo Request message type
	kEchoMessageType_EchoResponse			        = 2     //< Weave Echo Response message type
};


} // namespace Echo_Next
} // namespace Profiles
} // namespace Weave
} // namespace nl


#endif // WEAVE_ECHO_NEXT_H_
