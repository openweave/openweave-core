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
 *      Extern declarations for various Weave global objects.
 *
 */

#ifndef WEAVE_GLOBALS_H_
#define WEAVE_GLOBALS_H_

#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveConfig.h>

namespace nl {
namespace Weave {

extern WeaveFabricState FabricState;
extern WeaveMessageLayer MessageLayer;
extern WeaveExchangeManager ExchangeMgr;
extern WeaveSecurityManager SecurityMgr;

} // namespace nl
} // namespace Weave

#endif /* WEAVE_GLOBALS_H_ */
