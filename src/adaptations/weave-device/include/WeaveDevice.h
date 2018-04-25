/*
 *
 *    Copyright (c) 2018 Nest Labs, Inc.
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

#ifndef WEAVE_DEVICE_H
#define WEAVE_DEVICE_H

#include <Weave/Core/WeaveCore.h>
#include <WeaveDeviceConfig.h>
#include <BleLayer/BleLayer.h>
#include <PlatformManager.h>
#include <ConfigurationManager.h>
#include <ConnectivityManager.h>
#include <TimeSyncManager.h>
#include <WeaveDeviceError.h>

namespace nl {
namespace Weave {
namespace Device {

struct WeaveDeviceEvent;

extern PlatformManager PlatformMgr;
extern ConfigurationManager ConfigurationMgr;
extern ConnectivityManager ConnectivityMgr;
extern TimeSyncManager TimeSyncMgr;
extern nl::Weave::System::Layer SystemLayer;
extern nl::Inet::InetLayer InetLayer;
extern nl::Weave::WeaveFabricState FabricState;
extern nl::Weave::WeaveMessageLayer MessageLayer;
extern nl::Weave::WeaveExchangeManager ExchangeMgr;
extern nl::Weave::WeaveSecurityManager SecurityMgr;

} // namespace Device
} // namespace Weave
} // namespace nl

#endif // WEAVE_DEVICE_H
