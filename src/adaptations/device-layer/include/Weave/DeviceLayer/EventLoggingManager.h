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
 *      Defines the Weave Device Layer EventLoggingManager object.
 *
 */

#ifndef EVENT_LOGGING_MANAGER_H
#define EVENT_LOGGING_MANAGER_H

#include <Weave/Profiles/data-management/Current/DataManagement.h>

namespace nl {
namespace Weave {
namespace DeviceLayer {
namespace Internal {

/**
 * When an event is logged by Weave, the event is first serialized (on
 * the thread that's denoting the event) and stored in an event buffer
 * owned by a LoggingManagement.  Sometime later (on the Weave
 * thread), the event buffers are flushed and the events are offloaded
 * (via WDM) to all events subscribers.
 */
class EventLoggingManager final
{
    typedef ::nl::Weave::Profiles::DataManagement_Current::LoggingManagement LoggingManagement;

public:
    WEAVE_ERROR Init(void);
    WEAVE_ERROR Shutdown(void);

private:
    // ===== Members for internal use by the following friends.

    friend class ::nl::Weave::DeviceLayer::PlatformManagerImpl;
    friend EventLoggingManager & EventLoggingMgr(void);

    static EventLoggingManager sInstance;
};

/**
 * Returns a reference to the EventLoggingManager singleton object.
 */
inline EventLoggingManager & EventLoggingMgr(void)
{
    return EventLoggingManager::sInstance;
}

} // namespace Internal
} // namespace DeviceLayer
} // namespace Weave
} // namespace nl

#endif // EVENT_LOGGING_MANAGER_H
