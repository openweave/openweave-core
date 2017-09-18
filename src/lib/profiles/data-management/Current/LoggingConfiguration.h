/*
 *
 *    Copyright (c) 2015-2017 Nest Labs, Inc.
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
 * @file
 *
 * @brief
 *   Configuration of the Weave Event Logging.
 *
 */

#ifndef _WEAVE_DATA_MANAGEMENT_EVENT_LOGGING_CONFIGURATION_CURRENT_H
#define _WEAVE_DATA_MANAGEMENT_EVENT_LOGGING_CONFIGURATION_CURRENT_H

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current) {

/**
 * @brief
 *   LoggingConfiguration encapsulates the configurable component
 *   of the Weave Event Logging subsystem.
 */
class LoggingConfiguration
{
public:

    LoggingConfiguration(void);

    ImportanceType GetProfileImportance(uint32_t profileId) const;

    bool SupportsPerProfileImportance(void) const;

    uint64_t GetDestNodeId() const;

    nl::Inet::IPAddress GetDestNodeIPAddress() const;

public:

    static LoggingConfiguration & GetInstance(void);

    ImportanceType       mGlobalImportance;
    timestamp_t          mImportanceExpiration;
    duration_t           mMinimumLogUploadInterval;
    duration_t           mMaximumLogUploadInterval;
    char *               mLoggingDestination; // should be changeable at runtime; need to drive the semantics of change.
    uint64_t             mDestNodeId;
    nl::Inet::IPAddress  mDestNodeIPAddress;
    uint32_t             mUploadThreshold;
    uint32_t             mLoggingVolume;

};

} // WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current)
} // Profiles
} // Weave
} // nl

#endif // _WEAVE_DATA_MANAGEMENT_EVENT_LOGGING_CONFIGURATION_CURRENT_H
