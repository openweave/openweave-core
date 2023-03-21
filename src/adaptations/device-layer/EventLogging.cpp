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
 *      Implementation for the Weave Device Layer Event Logging functions.
 *
 */

#include <Weave/DeviceLayer/internal/WeaveDeviceLayerInternal.h>
#include <Weave/DeviceLayer/internal/EventLogging.h>
#include <Weave/Profiles/data-management/Current/DataManagement.h>
#include <Weave/Support/MathUtils.h>

using namespace nl::Weave::DeviceLayer;
using namespace nl::Weave::Platform;

namespace nl {
namespace Weave {
namespace DeviceLayer {
namespace Internal {

/* BSS segment is limited on some device, provides an option to allocate large memory buffer on heap */
#ifdef WEAVE_DEVICE_CONFIG_EVENT_LOGGING_BUFFER_STATIC
uint64_t gCritEventBuffer[RoundUp(WEAVE_DEVICE_CONFIG_EVENT_LOGGING_CRIT_BUFFER_SIZE, sizeof(uint64_t)) / sizeof(uint64_t)];
uint64_t gProdEventBuffer[RoundUp(WEAVE_DEVICE_CONFIG_EVENT_LOGGING_PROD_BUFFER_SIZE, sizeof(uint64_t)) / sizeof(uint64_t)];
#if WEAVE_DEVICE_CONFIG_EVENT_LOGGING_INFO_BUFFER_SIZE
uint64_t gInfoEventBuffer[RoundUp(WEAVE_DEVICE_CONFIG_EVENT_LOGGING_INFO_BUFFER_SIZE, sizeof(uint64_t)) / sizeof(uint64_t)];
#endif
#if WEAVE_DEVICE_CONFIG_EVENT_LOGGING_DEBUG_BUFFER_SIZE
uint64_t gDebugEventBuffer[RoundUp(WEAVE_DEVICE_CONFIG_EVENT_LOGGING_DEBUG_BUFFER_SIZE, sizeof(uint64_t)) / sizeof(uint64_t)];
#endif
#endif

static nl::Weave::PersistedCounter sCritEventIdCounter;
static nl::Weave::PersistedCounter sProdEventIdCounter;
static nl::Weave::PersistedCounter sInfoEventIdCounter;
static nl::Weave::PersistedCounter sDebugEventIdCounter;

WEAVE_ERROR InitWeaveEventLogging(void)
{
    nl::Weave::Platform::PersistedStorage::Key critEventIdCounterStorageKey = WEAVE_DEVICE_CONFIG_PERSISTED_STORAGE_CRIT_EIDC_KEY;
    nl::Weave::Platform::PersistedStorage::Key prodEventIdCounterStorageKey = WEAVE_DEVICE_CONFIG_PERSISTED_STORAGE_PROD_EIDC_KEY;
#if WEAVE_DEVICE_CONFIG_EVENT_LOGGING_INFO_BUFFER_SIZE
    nl::Weave::Platform::PersistedStorage::Key infoEventIdCounterStorageKey = WEAVE_DEVICE_CONFIG_PERSISTED_STORAGE_INFO_EIDC_KEY;
#endif
#if WEAVE_DEVICE_CONFIG_EVENT_LOGGING_DEBUG_BUFFER_SIZE
    nl::Weave::Platform::PersistedStorage::Key debugEventIdCounterStorageKey = WEAVE_DEVICE_CONFIG_PERSISTED_STORAGE_DEBUG_EIDC_KEY;
#endif

#ifdef WEAVE_DEVICE_CONFIG_EVENT_LOGGING_BUFFER_STATIC
    nl::Weave::Profiles::DataManagement::LogStorageResources logStorageResources[] = {
        { static_cast<void *>(&gCritEventBuffer[0]), sizeof(gCritEventBuffer),
          &critEventIdCounterStorageKey,
          WEAVE_DEVICE_CONFIG_EVENT_ID_COUNTER_EPOCH,
          &sCritEventIdCounter,
          nl::Weave::Profiles::DataManagement::ImportanceType::ProductionCritical },
        { static_cast<void *>(&gProdEventBuffer[0]), sizeof(gProdEventBuffer),
          &prodEventIdCounterStorageKey,
          WEAVE_DEVICE_CONFIG_EVENT_ID_COUNTER_EPOCH,
          &sProdEventIdCounter,
          nl::Weave::Profiles::DataManagement::ImportanceType::Production },
#if WEAVE_DEVICE_CONFIG_EVENT_LOGGING_INFO_BUFFER_SIZE
        { static_cast<void *>(&gInfoEventBuffer[0]), sizeof(gInfoEventBuffer),
          &infoEventIdCounterStorageKey,
          WEAVE_DEVICE_CONFIG_EVENT_ID_COUNTER_EPOCH,
          &sInfoEventIdCounter,
          nl::Weave::Profiles::DataManagement::ImportanceType::Info },
#endif
#if WEAVE_DEVICE_CONFIG_EVENT_LOGGING_DEBUG_BUFFER_SIZE
        { static_cast<void *>(&gDebugEventBuffer[0]), sizeof(gDebugEventBuffer),
          &debugEventIdCounterStorageKey,
          WEAVE_DEVICE_CONFIG_EVENT_ID_COUNTER_EPOCH,
          &sDebugEventIdCounter,
          nl::Weave::Profiles::DataManagement::ImportanceType::Debug },
#endif
    };
#else
    nl::Weave::Profiles::DataManagement::LogStorageResources logStorageResources[] = {
        { static_cast<void *>(malloc(RoundUp(WEAVE_DEVICE_CONFIG_EVENT_LOGGING_CRIT_BUFFER_SIZE, sizeof(uint64_t)))),
          RoundUp(WEAVE_DEVICE_CONFIG_EVENT_LOGGING_CRIT_BUFFER_SIZE, sizeof(uint64_t)),
          &critEventIdCounterStorageKey,
          WEAVE_DEVICE_CONFIG_EVENT_ID_COUNTER_EPOCH,
          &sCritEventIdCounter,
          nl::Weave::Profiles::DataManagement::ImportanceType::ProductionCritical },
        { static_cast<void *>(malloc(RoundUp(WEAVE_DEVICE_CONFIG_EVENT_LOGGING_PROD_BUFFER_SIZE, sizeof(uint64_t)))),
          RoundUp(WEAVE_DEVICE_CONFIG_EVENT_LOGGING_PROD_BUFFER_SIZE, sizeof(uint64_t)),
          &prodEventIdCounterStorageKey,
          WEAVE_DEVICE_CONFIG_EVENT_ID_COUNTER_EPOCH,
          &sProdEventIdCounter,
          nl::Weave::Profiles::DataManagement::ImportanceType::Production },
#if WEAVE_DEVICE_CONFIG_EVENT_LOGGING_INFO_BUFFER_SIZE
        { static_cast<void *>(malloc(RoundUp(WEAVE_DEVICE_CONFIG_EVENT_LOGGING_INFO_BUFFER_SIZE, sizeof(uint64_t)))),
          RoundUp(WEAVE_DEVICE_CONFIG_EVENT_LOGGING_INFO_BUFFER_SIZE, sizeof(uint64_t)),
          &infoEventIdCounterStorageKey,
          WEAVE_DEVICE_CONFIG_EVENT_ID_COUNTER_EPOCH,
          &sInfoEventIdCounter,
          nl::Weave::Profiles::DataManagement::ImportanceType::Info },
#endif
#if WEAVE_DEVICE_CONFIG_EVENT_LOGGING_DEBUG_BUFFER_SIZE
        { static_cast<void *>(malloc(RoundUp(WEAVE_DEVICE_CONFIG_EVENT_LOGGING_DEBUG_BUFFER_SIZE, sizeof(uint64_t)))),
          RoundUp(WEAVE_DEVICE_CONFIG_EVENT_LOGGING_DEBUG_BUFFER_SIZE, sizeof(uint64_t)),
          &debugEventIdCounterStorageKey,
          WEAVE_DEVICE_CONFIG_EVENT_ID_COUNTER_EPOCH,
          &sDebugEventIdCounter,
          nl::Weave::Profiles::DataManagement::ImportanceType::Debug },
#endif
    };
#endif

    nl::Weave::Profiles::DataManagement::LoggingManagement::CreateLoggingManagement(
        &nl::Weave::DeviceLayer::ExchangeMgr,
        sizeof(logStorageResources) / sizeof(logStorageResources[0]),
        logStorageResources);

    return WEAVE_NO_ERROR;
}

} // namespace Internal
} // namespace DeviceLayer
} // namespace Weave
} // namespace nl
