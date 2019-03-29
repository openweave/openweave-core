/*
 *
 *    Copyright (c) 2019 Google LLC.
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
 *      Implementation for the Weave Device Layer EventLoggingManager object.
 *
 */

#include <Weave/DeviceLayer/internal/WeaveDeviceLayerInternal.h>
#include <Weave/DeviceLayer/EventLoggingManager.h>

using namespace nl::Weave::DeviceLayer;

namespace nl {
namespace Weave {
namespace DeviceLayer {
namespace Internal {

// For each priority level initialize event buffers and event Id counters.
#if WEAVE_DEVICE_CONFIG_EVENT_LOGGING_DEBUG_EVENTS
uint64_t gDebugEventBuffer[(WEAVE_DEVICE_CONFIG_EVENT_LOGGING_DEBUG_BUFFER_SIZE + 7) / 8];
PersistedCounter sDebugEventIdCounter;
#endif

#if WEAVE_DEVICE_CONFIG_EVENT_LOGGING_INFO_EVENTS
uint64_t gInfoEventBuffer[(WEAVE_DEVICE_CONFIG_EVENT_LOGGING_INFO_BUFFER_SIZE + 7) / 8];
PersistedCounter sInfoEventIdCounter;
#endif

uint64_t gProdEventBuffer[(WEAVE_DEVICE_CONFIG_EVENT_LOGGING_PROD_BUFFER_SIZE + 7) / 8];
PersistedCounter sProdEventIdCounter;

uint64_t gCritEventBuffer[(WEAVE_DEVICE_CONFIG_EVENT_LOGGING_CRIT_BUFFER_SIZE + 7) / 8];
PersistedCounter sCritEventIdCounter;

EventLoggingManager EventLoggingManager::sInstance;

WEAVE_ERROR EventLoggingManager::Init(void)
{
    nl::Weave::Platform::PersistedStorage::Key eidcStorageKeys[WEAVE_DEVICE_CONFIG_EVENT_LOGGING_NUM_BUFFERS];

    size_t eventBufferSizes[] = {
#if WEAVE_DEVICE_CONFIG_EVENT_LOGGING_DEBUG_EVENTS
        sizeof(gDebugEventBuffer),
#endif
#if WEAVE_DEVICE_CONFIG_EVENT_LOGGING_INFO_EVENTS
        sizeof(gInfoEventBuffer),
#endif
        sizeof(gProdEventBuffer),
        sizeof(gCritEventBuffer),
    };

    void * eventBuffers[] = {
#if WEAVE_DEVICE_CONFIG_EVENT_LOGGING_DEBUG_EVENTS
        static_cast<void *>(&gDebugEventBuffer[0]),
#endif
#if WEAVE_DEVICE_CONFIG_EVENT_LOGGING_INFO_EVENTS
        static_cast<void *>(&gInfoEventBuffer[0]),
#endif
        static_cast<void *>(&gProdEventBuffer[0]),
        static_cast<void *>(&gCritEventBuffer[0]),
    };

    // For each priority level initialize event id counter storage keys.
    ConfigurationMgr().GetEventIdCounterStorageKeys(eidcStorageKeys);

    const uint32_t eidcEpochs[] = {
#if WEAVE_DEVICE_CONFIG_EVENT_LOGGING_DEBUG_EVENTS
        WEAVE_DEVICE_CONFIG_EVENT_ID_COUNTER_EPOCH,
#endif
#if WEAVE_DEVICE_CONFIG_EVENT_LOGGING_INFO_EVENTS
        WEAVE_DEVICE_CONFIG_EVENT_ID_COUNTER_EPOCH,
#endif
        WEAVE_DEVICE_CONFIG_EVENT_ID_COUNTER_EPOCH,
        WEAVE_DEVICE_CONFIG_EVENT_ID_COUNTER_EPOCH,
    };

    PersistedCounter * eidcStorage[] = {
#if WEAVE_DEVICE_CONFIG_EVENT_LOGGING_DEBUG_EVENTS
        &sDebugEventIdCounter,
#endif
#if WEAVE_DEVICE_CONFIG_EVENT_LOGGING_INFO_EVENTS
        &sInfoEventIdCounter,
#endif
        &sProdEventIdCounter,
        &sCritEventIdCounter,
    };

    LoggingManagement::CreateLoggingManagement(
        &::nl::Weave::DeviceLayer::ExchangeMgr,
        WEAVE_DEVICE_CONFIG_EVENT_LOGGING_NUM_BUFFERS,
        &eventBufferSizes[0],
        &eventBuffers[0],
        eidcStorageKeys,
        eidcEpochs,
        eidcStorage);

    return WEAVE_NO_ERROR;
}

WEAVE_ERROR EventLoggingManager::Shutdown(void)
{
    LoggingManagement::DestroyLoggingManagement();
    return WEAVE_NO_ERROR;
}

} // namespace Internal
} // namespace DeviceLayer
} // namespace Weave
} // namespace nl


namespace nl {
namespace Weave {
namespace Profiles {
namespace DataManagement_Current {
namespace Platform {

void CriticalSectionEnter(void)
{
    return PlatformMgr().LockWeaveStack();
}

void CriticalSectionExit(void)
{
    return PlatformMgr().UnlockWeaveStack();
}

} // namespace Platform
} // namespace DataManagement_Current
} // namespace Profiles
} // namespace Weave
} // namespace nl
