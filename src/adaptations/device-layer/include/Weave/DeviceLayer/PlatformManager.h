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

#ifndef PLATFORM_MANAGER_H
#define PLATFORM_MANAGER_H

#include <Weave/DeviceLayer/WeaveDeviceEvent.h>

namespace nl {
namespace Weave {
namespace DeviceLayer {

class ConnectivityManager;
class TraitManager;
struct WeaveDeviceEvent;
class ComfigurationManagerImpl;

namespace Internal {
class FabricProvisioningServer;
class ServiceProvisioningServer;
class BLEManager;
template<class ImplClass> class GenericConfigurationManagerImpl;
} // namespace Internal

class PlatformManager
{
public:

    WEAVE_ERROR InitLocks();
    WEAVE_ERROR InitWeaveStack();

    typedef void (*EventHandlerFunct)(const WeaveDeviceEvent * event, intptr_t arg);
    WEAVE_ERROR AddEventHandler(EventHandlerFunct handler, intptr_t arg = 0);
    void RemoveEventHandler(EventHandlerFunct handler, intptr_t arg = 0);

    void ScheduleWork(AsyncWorkFunct workFunct, intptr_t arg = 0);

    void RunEventLoop(void);
    WEAVE_ERROR StartEventLoopTask(void);

    void LockWeaveStack(void);
    bool TryLockWeaveStack(void);
    void UnlockWeaveStack(void);

    static esp_err_t HandleESPSystemEvent(void * ctx, system_event_t * event);

private:

    // NOTE: These members are for internal use by the following friends.

    friend class ConnectivityManager;
    class ConfigurationManagerImpl;
    template<class ImplClass> friend class Internal::GenericConfigurationManagerImpl;
    friend class TraitManager;
    friend class TimeSyncManager;
    friend class Internal::FabricProvisioningServer;
    friend class Internal::ServiceProvisioningServer;
    friend class Internal::BLEManager;
    friend nl::Weave::System::Error nl::Weave::System::Platform::Layer::DispatchEvent(nl::Weave::System::Layer & aLayer, void * aContext, const ::nl::Weave::DeviceLayer::WeaveDeviceEvent * aEvent);
    friend nl::Weave::System::Error nl::Weave::System::Platform::Layer::StartTimer(nl::Weave::System::Layer & aLayer, void * aContext, uint32_t aMilliseconds);

    void PostEvent(const WeaveDeviceEvent * event);

private:

    // NOTE: These members are private to the class and should not be used by friends.

    WEAVE_ERROR InitWeaveEventQueue(void);
    void DispatchEvent(const WeaveDeviceEvent * event);
    static void RunEventLoop(void * arg);
    static void HandleSessionEstablished(::nl::Weave::WeaveSecurityManager * sm, ::nl::Weave::WeaveConnection * con, void * reqState, uint16_t sessionKeyId, uint64_t peerNodeId, uint8_t encType);
};

} // namespace DeviceLayer
} // namespace Weave
} // namespace nl

#endif // PLATFORM_MANAGER_H
