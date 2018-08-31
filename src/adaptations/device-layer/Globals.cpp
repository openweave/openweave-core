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

#include <Weave/DeviceLayer/internal/WeaveDeviceLayerInternal.h>
#include <Weave/DeviceLayer/internal/BLEManager.h>
#include <Weave/DeviceLayer/internal/DeviceControlServer.h>
#include <Weave/DeviceLayer/internal/DeviceDescriptionServer.h>
#include <Weave/DeviceLayer/internal/NetworkProvisioningServer.h>
#include <Weave/DeviceLayer/internal/FabricProvisioningServer.h>
#include <Weave/DeviceLayer/internal/ServiceProvisioningServer.h>
#include <Weave/DeviceLayer/internal/EchoServer.h>

namespace nl {
namespace Weave {
namespace DeviceLayer {

PlatformManager PlatformMgr;
TraitManager TraitMgr;
TimeSyncManager TimeSyncMgr;

nl::Weave::System::Layer SystemLayer;
nl::Inet::InetLayer InetLayer;

nl::Weave::WeaveFabricState FabricState;
nl::Weave::WeaveMessageLayer MessageLayer;
nl::Weave::WeaveExchangeManager ExchangeMgr;
nl::Weave::WeaveSecurityManager SecurityMgr;

namespace Internal {

#if WEAVE_DEVICE_CONFIG_ENABLE_WOBLE
BLEManager BLEMgr;
#endif

EchoServer EchoSvr;
DeviceControlServer DeviceControlSvr;
DeviceDescriptionServer DeviceDescriptionSvr;
FabricProvisioningServer FabricProvisioningSvr;
ServiceProvisioningServer ServiceProvisioningSvr;

const char * const TAG = "weave[DL]";

} // namespace Internal
} // namespace DeviceLayer
} // namespace Weave
} // namespace nl
