/*
 *
 *    Copyright (c) 2019 Nest Labs, Inc.
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
 *          Utility functions for the Weave Addressing and Routing Module (WARM)
 *          for use on LwIP-based platforms.
 */

#ifndef WARM_SUPPORT_H
#define WARM_SUPPORT_H

#include <Warm/Warm.h>

namespace nl {
namespace Weave {
namespace DeviceLayer {
namespace Internal {

extern WEAVE_ERROR GetLwIPNetifForWarmInterfaceType(::nl::Weave::Warm::InterfaceType inInterfaceType, struct netif *& netif);
extern bool LwIPNetifSupportsMLD(struct netif * netif);
extern const char * WarmInterfaceTypeToStr(::nl::Weave::Warm::InterfaceType inInterfaceType);

} // namespace Internal
} // namespace DeviceLayer
} // namespace Weave
} // namespace nl

#endif // WARM_SUPPORT_H
