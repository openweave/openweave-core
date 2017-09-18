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
 *      This file implements Weave node state globals.
 *
 */

#include <Weave/Support/NLDLLUtil.h>

#include "WeaveMessageLayer.h"
#include "WeaveFabricState.h"
#include "WeaveExchangeMgr.h"
#include "WeaveSecurityMgr.h"

namespace nl {
namespace Weave {

NL_DLL_EXPORT WeaveFabricState     FabricState;
NL_DLL_EXPORT WeaveMessageLayer    MessageLayer;
NL_DLL_EXPORT WeaveExchangeManager ExchangeMgr;
NL_DLL_EXPORT WeaveSecurityManager SecurityMgr;

} // namespace nl
} // namespace Weave
