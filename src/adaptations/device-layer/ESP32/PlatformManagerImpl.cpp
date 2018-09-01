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
#include <Weave/DeviceLayer/PlatformManager.h>
#include <Weave/DeviceLayer/internal/DeviceControlServer.h>
#include <Weave/DeviceLayer/internal/DeviceDescriptionServer.h>
#include <Weave/DeviceLayer/internal/NetworkProvisioningServer.h>
#include <Weave/DeviceLayer/internal/FabricProvisioningServer.h>
#include <Weave/DeviceLayer/internal/ServiceProvisioningServer.h>
#include <Weave/DeviceLayer/internal/ServiceDirectoryManager.h>
#include <Weave/DeviceLayer/internal/EchoServer.h>
#include <new>
#include <esp_timer.h>
#include <Weave/DeviceLayer/internal/BLEManager.h>

using namespace ::nl;
using namespace ::nl::Weave;
using namespace ::nl::Weave::DeviceLayer::Internal;

namespace nl {
namespace Weave {
namespace DeviceLayer {

namespace Internal {

extern WEAVE_ERROR InitCASEAuthDelegate();
extern WEAVE_ERROR InitEntropy();

} // namespace Internal

namespace {

struct RegisteredEventHandler
{
    RegisteredEventHandler * Next;
    PlatformManagerImpl::EventHandlerFunct Handler;
    intptr_t Arg;
};

SemaphoreHandle_t WeaveStackLock;
SemaphoreHandle_t LwIPCoreLock;
QueueHandle_t WeaveEventQueue;
bool WeaveTimerActive;
TimeOut_t NextTimerBaseTime;
TickType_t NextTimerDurationTicks;
RegisteredEventHandler * RegisteredEventHandlerList;
TaskHandle_t EventLoopTask;

} // unnamed namespace

PlatformManagerImpl PlatformManagerImpl::sInstance;

WEAVE_ERROR PlatformManagerImpl::InitLocks(void)
{
    WeaveStackLock = xSemaphoreCreateMutex();
    if (WeaveStackLock == NULL) {
        WeaveLogError(DeviceLayer, "Failed to create Weave stack lock");
        return WEAVE_ERROR_NO_MEMORY;
    }

    LwIPCoreLock = xSemaphoreCreateMutex();
    if (LwIPCoreLock == NULL) {
        WeaveLogError(DeviceLayer, "Failed to create LwIP core lock");
        return WEAVE_ERROR_NO_MEMORY;
    }

    return WEAVE_NO_ERROR;
}

WEAVE_ERROR PlatformManagerImpl::_InitWeaveStack(void)
{
    WEAVE_ERROR err;

    // Initialize the source used by Weave to get secure random data.
    err = InitEntropy();
    SuccessOrExit(err);

    // Initialize the master Weave event queue.
    err = InitWeaveEventQueue();
    SuccessOrExit(err);

    // Initialize the Configuration Manager object.
    err = ConfigurationMgr().Init();
    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogError(DeviceLayer, "Configuration Manager initialization failed: %s", ErrorStr(err));
    }
    SuccessOrExit(err);

    // Initialize the Weave system layer.
    new (&SystemLayer) System::Layer();
    err = SystemLayer.Init(NULL);
    if (err != WEAVE_SYSTEM_NO_ERROR)
    {
        WeaveLogError(DeviceLayer, "SystemLayer initialization failed: %s", ErrorStr(err));
    }
    SuccessOrExit(err);

    // Initialize the Weave Inet layer.
    new (&InetLayer) Inet::InetLayer();
    err = InetLayer.Init(SystemLayer, NULL);
    if (err != INET_NO_ERROR)
    {
        WeaveLogError(DeviceLayer, "InetLayer initialization failed: %s", ErrorStr(err));
    }
    SuccessOrExit(err);

    // Initialize the BLE manager.
#if WEAVE_DEVICE_CONFIG_ENABLE_WOBLE
    new (&BLEMgr) BLEManager();
    err = BLEMgr.Init();
    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogError(DeviceLayer, "BLEManager initialization failed: %s", ErrorStr(err));
    }
    SuccessOrExit(err);
#endif

    // Initialize the Weave fabric state object.
    new (&FabricState) WeaveFabricState();
    err = FabricState.Init();
    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogError(DeviceLayer, "FabricState initialization failed: %s", ErrorStr(err));
    }
    SuccessOrExit(err);

    FabricState.DefaultSubnet = kWeaveSubnetId_PrimaryWiFi;

#if WEAVE_CONFIG_SECURITY_TEST_MODE
    FabricState.LogKeys = true;
#endif

    {
        WeaveMessageLayer::InitContext initContext;
        initContext.systemLayer = &SystemLayer;
        initContext.inet = &InetLayer;
        initContext.listenTCP = true;
        initContext.listenUDP = true;
#if WEAVE_DEVICE_CONFIG_ENABLE_WOBLE
        initContext.ble = BLEMgr.GetBleLayer();
        initContext.listenBLE = true;
#endif
        initContext.fabricState = &FabricState;

        // Initialize the Weave message layer.
        new (&MessageLayer) WeaveMessageLayer();
        err = MessageLayer.Init(&initContext);
        if (err != WEAVE_NO_ERROR) {
            WeaveLogError(DeviceLayer, "MessageLayer initialization failed: %s", ErrorStr(err));
        }
        SuccessOrExit(err);
    }

    // Initialize the Weave exchange manager.
    err = ExchangeMgr.Init(&MessageLayer);
    if (err != WEAVE_NO_ERROR) {
        WeaveLogError(DeviceLayer, "ExchangeMgr initialization failed: %s", ErrorStr(err));
    }
    SuccessOrExit(err);

    // Initialize the Weave security manager.
    new (&SecurityMgr) WeaveSecurityManager();
    err = SecurityMgr.Init(ExchangeMgr, SystemLayer);
    if (err != WEAVE_NO_ERROR) {
        WeaveLogError(DeviceLayer, "SecurityMgr initialization failed: %s", ErrorStr(err));
    }
    SuccessOrExit(err);

    SecurityMgr.OnSessionEstablished = HandleSessionEstablished;

    // Initialize the CASE auth delegate object.
    err = InitCASEAuthDelegate();
    SuccessOrExit(err);

#if WEAVE_CONFIG_SECURITY_TEST_MODE
    SecurityMgr.CASEUseKnownECDHKey = true;
#endif

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    // Initialize the service directory manager.
    err = InitServiceDirectoryManager();
    SuccessOrExit(err);
#endif

    // Perform dynamic configuration of the Weave stack based on stored settings.
    err = ConfigurationMgr().ConfigureWeaveStack();
    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogError(DeviceLayer, "ConfigureWeaveStack failed: %s", ErrorStr(err));
    }
    SuccessOrExit(err);

    // Initialize the Connectivity Manager object.
    err = ConnectivityMgr().Init();
    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogError(DeviceLayer, "Connectivity Manager initialization failed: %s", ErrorStr(err));
    }
    SuccessOrExit(err);

    // Initialize the Device Control server.
    err = DeviceControlSvr().Init();
    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogError(DeviceLayer, "Weave Device Control server initialization failed: %s", ErrorStr(err));
    }
    SuccessOrExit(err);

    // Initialize the Device Description server.
    err = DeviceDescriptionSvr().Init();
    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogError(DeviceLayer, "Weave Device Control server initialization failed: %s", ErrorStr(err));
    }
    SuccessOrExit(err);

    // Initialize the Network Provisioning server.
    err = NetworkProvisioningSvr().Init();
    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogError(DeviceLayer, "Weave Network Provisioning server initialization failed: %s", ErrorStr(err));
    }
    SuccessOrExit(err);

    // Initialize the Fabric Provisioning server.
    err = FabricProvisioningSvr().Init();
    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogError(DeviceLayer, "Weave Fabric Provisioning server initialization failed: %s", ErrorStr(err));
    }
    SuccessOrExit(err);

    // Initialize the Service Provisioning server.
    err = ServiceProvisioningSvr().Init();
    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogError(DeviceLayer, "Weave Service Provisioning server initialization failed: %s", ErrorStr(err));
    }
    SuccessOrExit(err);

    // Initialize the Echo server.
    err = EchoSvr().Init();
    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogError(DeviceLayer, "Weave Echo server initialization failed: %s", ErrorStr(err));
    }
    SuccessOrExit(err);

    // Initialize the Trait Manager object.
    err = TraitMgr().Init();
    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogError(DeviceLayer, "Trait Manager initialization failed: %s", ErrorStr(err));
    }
    SuccessOrExit(err);

    // Initialize the Time Sync Manager object.
    err = TimeSyncMgr().Init();
    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogError(DeviceLayer, "Time Sync Manager initialization failed: %s", ErrorStr(err));
    }
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR PlatformManagerImpl::_AddEventHandler(EventHandlerFunct handler, intptr_t arg)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    RegisteredEventHandler * eventHandler;

    // Do nothing if the event handler is already registered.
    for (eventHandler = RegisteredEventHandlerList; eventHandler != NULL; eventHandler = eventHandler->Next)
    {
        if (eventHandler->Handler == handler && eventHandler->Arg == arg)
        {
            ExitNow();
        }
    }

    eventHandler = (RegisteredEventHandler *)malloc(sizeof(RegisteredEventHandler));
    VerifyOrExit(eventHandler != NULL, err = WEAVE_ERROR_NO_MEMORY);

    eventHandler->Next = RegisteredEventHandlerList;
    eventHandler->Handler = handler;
    eventHandler->Arg = arg;

    RegisteredEventHandlerList = eventHandler;

exit:
    return err;
}

void PlatformManagerImpl::_RemoveEventHandler(EventHandlerFunct handler, intptr_t arg)
{
    RegisteredEventHandler ** eventHandlerIndirectPtr;

    for (eventHandlerIndirectPtr = &RegisteredEventHandlerList; *eventHandlerIndirectPtr != NULL; )
    {
        RegisteredEventHandler * eventHandler = (*eventHandlerIndirectPtr);

        if (eventHandler->Handler == handler && eventHandler->Arg == arg)
        {
            *eventHandlerIndirectPtr = eventHandler->Next;
            free(eventHandler);
        }
        else
        {
            eventHandlerIndirectPtr = &eventHandler->Next;
        }
    }
}

void PlatformManagerImpl::_ScheduleWork(AsyncWorkFunct workFunct, intptr_t arg)
{
    WeaveDeviceEvent event;
    event.Type = WeaveDeviceEvent::kEventType_CallWorkFunct;
    event.CallWorkFunct.WorkFunct = workFunct;
    event.CallWorkFunct.Arg = arg;

    PostEvent(&event);
}

void PlatformManagerImpl::_RunEventLoop(void)
{
    RunEventLoop(NULL);
}

WEAVE_ERROR PlatformManagerImpl::_StartEventLoopTask(void)
{
    BaseType_t res;

    res = xTaskCreate(RunEventLoop,
                WEAVE_DEVICE_CONFIG_WEAVE_TASK_NAME,
                WEAVE_DEVICE_CONFIG_WEAVE_TASK_STACK_SIZE,
                NULL,
                ESP_TASK_PRIO_MIN + WEAVE_DEVICE_CONFIG_WEAVE_TASK_PRIORITY,
                NULL);

    return (res == pdPASS) ? WEAVE_NO_ERROR : WEAVE_ERROR_NO_MEMORY;
}

void PlatformManagerImpl::_LockWeaveStack(void)
{
    xSemaphoreTake(WeaveStackLock, portMAX_DELAY);
}

bool PlatformManagerImpl::_TryLockWeaveStack(void)
{
    return xSemaphoreTake(WeaveStackLock, 0) == pdTRUE;
}

void PlatformManagerImpl::_UnlockWeaveStack(void)
{
    xSemaphoreGive(WeaveStackLock);
}

esp_err_t PlatformManagerImpl::HandleESPSystemEvent(void * ctx, system_event_t * espEvent)
{
    WeaveDeviceEvent event;
    event.Type = WeaveDeviceEvent::kEventType_ESPSystemEvent;
    event.ESPSystemEvent = *espEvent;

    sInstance.PostEvent(&event);

    return ESP_OK;
}

WEAVE_ERROR PlatformManagerImpl::InitWeaveEventQueue(void)
{
    WeaveEventQueue = xQueueCreate(WEAVE_DEVICE_CONFIG_MAX_EVENT_QUEUE_SIZE, sizeof(WeaveDeviceEvent));
    if (WeaveEventQueue == NULL)
    {
        WeaveLogError(DeviceLayer, "Failed to allocate Weave event queue");
        return WEAVE_ERROR_NO_MEMORY;
    }

    return WEAVE_NO_ERROR;
}

void PlatformManagerImpl::_PostEvent(const WeaveDeviceEvent * event)
{
    if (WeaveEventQueue != NULL)
    {
        if (!xQueueSend(WeaveEventQueue, event, 1))
        {
            WeaveLogError(DeviceLayer, "Failed to post event to Weave Platform event queue");
        }
    }
}

void PlatformManagerImpl::_DispatchEvent(const WeaveDeviceEvent * event)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

#if WEAVE_PROGRESS_LOGGING
    uint64_t startUS = System::Layer::GetClock_MonotonicHiRes();
#endif // WEAVE_PROGRESS_LOGGING

    // If the event is a Weave System or Inet Layer event, deliver it to the SystemLayer event handler.
    if (event->Type == WeaveDeviceEvent::kEventType_WeaveSystemLayerEvent)
    {
        err = SystemLayer.HandleEvent(*event->WeaveSystemLayerEvent.Target, event->WeaveSystemLayerEvent.Type, event->WeaveSystemLayerEvent.Argument);
        if (err != WEAVE_SYSTEM_NO_ERROR)
        {
            WeaveLogError(DeviceLayer, "Error handling Weave System Layer event (type %d): %s", event->Type, nl::ErrorStr(err));
        }
    }

    // If the event is a "call work function" event, call the specified function.
    else if (event->Type == WeaveDeviceEvent::kEventType_CallWorkFunct)
    {
        event->CallWorkFunct.WorkFunct(event->CallWorkFunct.Arg);
    }

    // Otherwise deliver the event to all the platform components.  Each of these will decide
    // whether and how they want to react to the event.
    else
    {
        ConnectivityMgr().OnPlatformEvent(event);
        DeviceControlSvr().OnPlatformEvent(event);
        DeviceDescriptionSvr().OnPlatformEvent(event);
        NetworkProvisioningSvr().OnPlatformEvent(event);
        FabricProvisioningSvr().OnPlatformEvent(event);
        ServiceProvisioningSvr().OnPlatformEvent(event);
        TraitMgr().OnPlatformEvent(event);
        TimeSyncMgr().OnPlatformEvent(event);
#if WEAVE_DEVICE_CONFIG_ENABLE_WOBLE
        BLEMgr.OnPlatformEvent(event);
#endif
    }

    // If the event is not a device-layer internal event, deliver it to the application's registered
    // event handlers.
    if (!WeaveDeviceEvent::IsInternalEvent(event->Type))
    {
        for (RegisteredEventHandler * eventHandler = RegisteredEventHandlerList;
             eventHandler != NULL;
             eventHandler = eventHandler->Next)
        {
            eventHandler->Handler(event, eventHandler->Arg);
        }
    }

#if WEAVE_PROGRESS_LOGGING
    uint32_t delta = ((uint32_t)(System::Layer::GetClock_MonotonicHiRes() - startUS)) / 1000;
    if (delta > 100)
    {
        WeaveLogError(DeviceLayer, "Long dispatch time: %" PRId32 " ms", delta);
    }
#endif // WEAVE_PROGRESS_LOGGING
}

void PlatformManagerImpl::RunEventLoop(void * /* unused */)
{
    WEAVE_ERROR err;
    WeaveDeviceEvent event;

    VerifyOrDie(EventLoopTask == NULL);

    EventLoopTask = xTaskGetCurrentTaskHandle();

    while (true)
    {
        TickType_t waitTime;

        // If one or more Weave timers are active...
        if (WeaveTimerActive) {

            // Adjust the base time and remaining duration for the next scheduled timer based on the
            // amount of time that has elapsed since it was started.
            // IF the timer's expiration time has already arrived...
            if (xTaskCheckForTimeOut(&NextTimerBaseTime, &NextTimerDurationTicks) == pdTRUE) {

                // Reset the 'timer active' flag.  This will be set to true again by HandlePlatformTimer()
                // if there are further timers beyond the expired one that are still active.
                WeaveTimerActive = false;

                // Call into the system layer to dispatch the callback functions for all timers
                // that have expired.
                err = SystemLayer.HandlePlatformTimer();
                if (err != WEAVE_SYSTEM_NO_ERROR) {
                    WeaveLogError(DeviceLayer, "Error handling Weave timers: %s", ErrorStr(err));
                }

                // When processing the event queue below, do not wait if the queue is empty.  Instead
                // immediately loop around and process timers again
                waitTime = 0;
            }

            // If there is still time before the next timer expires, arrange to wait on the event queue
            // until that timer expires.
            else {
                waitTime = NextTimerDurationTicks;
            }
        }

        // Otherwise no Weave timers are active, so wait indefinitely for an event to arrive on the event
        // queue.
        else {
            waitTime = portMAX_DELAY;
        }

        // Unlock the Weave stack, allowing other threads to enter Weave while the event loop thread is sleeping.
        sInstance.UnlockWeaveStack();

        BaseType_t eventReceived = xQueueReceive(WeaveEventQueue, &event, waitTime);

        // Lock the Weave stack.
        sInstance.LockWeaveStack();

        // If an event was received, dispatch it.  Continue receiving events from the queue and
        // dispatching them until the queue is empty.
        while (eventReceived == pdTRUE) {

            sInstance.DispatchEvent(&event);

            eventReceived = xQueueReceive(WeaveEventQueue, &event, 0);
        }
    }
}

void PlatformManagerImpl::HandleSessionEstablished(WeaveSecurityManager * sm, WeaveConnection * con, void * reqState, uint16_t sessionKeyId, uint64_t peerNodeId, uint8_t encType)
{
    // Get the auth mode for the newly established session key.
    WeaveSessionKey * sessionKey;
    FabricState.GetSessionKey(sessionKeyId, peerNodeId, sessionKey);
    WeaveAuthMode authMode = (sessionKey != NULL) ? sessionKey->AuthMode : (WeaveAuthMode)kWeaveAuthMode_NotSpecified;

    // Post a SessionEstablished event for the new session.  If a PASE session is established
    // using the device's pairing code, presume that this is a commissioner and set the
    // IsCommissioner flag as a convenience to the application.
    WeaveDeviceEvent event;
    event.Type = WeaveDeviceEvent::kEventType_SessionEstablished;
    event.SessionEstablished.PeerNodeId = peerNodeId;
    event.SessionEstablished.SessionKeyId = sessionKeyId;
    event.SessionEstablished.EncType = encType;
    event.SessionEstablished.AuthMode = authMode;
    event.SessionEstablished.IsCommissioner = (authMode == kWeaveAuthMode_PASE_PairingCode);
    sInstance.PostEvent(&event);

    if (event.SessionEstablished.IsCommissioner)
    {
        WeaveLogProgress(DeviceLayer, "Commissioner session established");
    }
}

} // namespace DeviceLayer
} // namespace Weave
} // namespace nl


// ==================== LwIP Core Locking Functions ====================

extern "C" void lock_lwip_core()
{
    xSemaphoreTake(::nl::Weave::DeviceLayer::LwIPCoreLock, portMAX_DELAY);
}

extern "C" void unlock_lwip_core()
{
    xSemaphoreGive(::nl::Weave::DeviceLayer::LwIPCoreLock);
}


// ==================== Timer Support Functions ====================

namespace nl {
namespace Weave {
namespace System {
namespace Platform {
namespace Layer {

using namespace ::nl::Weave::DeviceLayer;
using namespace ::nl::Weave::DeviceLayer::Internal;

System::Error StartTimer(System::Layer & aLayer, void * aContext, uint32_t aMilliseconds)
{
    WeaveTimerActive = true;
    vTaskSetTimeOutState(&NextTimerBaseTime);
    NextTimerDurationTicks = pdMS_TO_TICKS(aMilliseconds);

    // If the platform timer is being updated by a thread other than the event loop thread,
    // trigger the event loop thread to recalculate its wait time by posting a no-op event
    // to the event queue.
    if (xTaskGetCurrentTaskHandle() != EventLoopTask)
    {
        WeaveDeviceEvent event;
        event.Type = WeaveDeviceEvent::kEventType_NoOp;
        PlatformMgr().PostEvent(&event);
    }

    return WEAVE_SYSTEM_NO_ERROR;
}

} // namespace Layer
} // namespace Platform
} // namespace System
} // namespace Weave
} // namespace nl


// ==================== System Layer Event Support Functions ====================

namespace nl {
namespace Weave {
namespace System {
namespace Platform {
namespace Layer {

using namespace ::nl::Weave::DeviceLayer;
using namespace ::nl::Weave::DeviceLayer::Internal;

System::Error PostEvent(System::Layer & aLayer, void * aContext, System::Object & aTarget, System::EventType aType, uintptr_t aArgument)
{
    Error err = WEAVE_SYSTEM_NO_ERROR;
    WeaveDeviceEvent event;

    event.Type = WeaveDeviceEvent::kEventType_WeaveSystemLayerEvent;
    event.WeaveSystemLayerEvent.Type = aType;
    event.WeaveSystemLayerEvent.Target = &aTarget;
    event.WeaveSystemLayerEvent.Argument = aArgument;

    if (!xQueueSend(WeaveEventQueue, &event, 1)) {
        WeaveLogError(DeviceLayer, "Failed to post event to Weave Platform event queue");
        err = WEAVE_ERROR_NO_MEMORY;
    }

    return err;
}

System::Error DispatchEvents(Layer & aLayer, void * aContext)
{
    PlatformMgr().RunEventLoop();
    return WEAVE_SYSTEM_NO_ERROR;
}

System::Error DispatchEvent(System::Layer & aLayer, void * aContext, const WeaveDeviceEvent * aEvent)
{
    PlatformMgr().DispatchEvent(aEvent);
    return WEAVE_NO_ERROR;
}

} // namespace Layer
} // namespace Platform
} // namespace System
} // namespace Weave
} // namespace nl
