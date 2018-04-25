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

#include <internal/WeaveDeviceInternal.h>
#include <internal/BLEManager.h>
#include <internal/DeviceControlServer.h>
#include <internal/DeviceDescriptionServer.h>
#include <internal/NetworkProvisioningServer.h>
#include <internal/FabricProvisioningServer.h>
#include <internal/ServiceProvisioningServer.h>
#include <internal/EchoServer.h>

namespace nl {
namespace Weave {
namespace Device {

PlatformManager PlatformMgr;
ConfigurationManager ConfigurationMgr;
ConnectivityManager ConnectivityMgr;
TimeSyncManager TimeSyncMgr;

nl::Weave::System::Layer SystemLayer;
nl::Inet::InetLayer InetLayer;

nl::Weave::WeaveFabricState FabricState;
nl::Weave::WeaveMessageLayer MessageLayer;
nl::Weave::WeaveExchangeManager ExchangeMgr;
nl::Weave::WeaveSecurityManager SecurityMgr;

namespace Internal {

BLEManager BLEMgr;
EchoServer EchoSvr;
DeviceControlServer DeviceControlSvr;
DeviceDescriptionServer DeviceDescriptionSvr;
NetworkProvisioningServer NetworkProvisioningSvr;
FabricProvisioningServer FabricProvisioningSvr;
ServiceProvisioningServer ServiceProvisioningSvr;

const char * const TAG = "weave[DAL]";

} // namespace Internal
} // namespace Device
} // namespace Weave
} // namespace nl
