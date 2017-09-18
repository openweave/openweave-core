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
#include <Weave/Profiles/data-management/Current/WdmManagedNamespace.h>
#include <Weave/Profiles/data-management/DataManagement.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current) {

LoggingConfiguration :: LoggingConfiguration(void) :
    mGlobalImportance(WEAVE_CONFIG_EVENT_LOGGING_DEFAULT_IMPORTANCE),
    mImportanceExpiration(0),
    mMinimumLogUploadInterval(WEAVE_CONFIG_EVENT_LOGGING_MINIMUM_UPLOAD_SECONDS*1000),
    mMaximumLogUploadInterval(WEAVE_CONFIG_EVENT_LOGGING_MAXIMUM_UPLOAD_SECONDS*1000),
    mLoggingDestination(NULL),
    mDestNodeId(kAnyNodeId),
    mDestNodeIPAddress(nl::Inet::IPAddress::Any),
    mUploadThreshold(WEAVE_CONFIG_EVENT_LOGGING_XFER_THRESHOLD)
{

}

bool LoggingConfiguration :: SupportsPerProfileImportance(void) const
{
    return false;
}

ImportanceType LoggingConfiguration::GetProfileImportance(uint32_t profileId) const
{
    return mGlobalImportance;
}

LoggingConfiguration & LoggingConfiguration::GetInstance(void)
{
    static LoggingConfiguration sInstance;

    return sInstance;
}

uint64_t LoggingConfiguration::GetDestNodeId() const
{
    return mDestNodeId;
}

nl::Inet::IPAddress LoggingConfiguration::GetDestNodeIPAddress() const
{
    return mDestNodeIPAddress;
}


} // WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current)
} // Profiles
} // Weave
} // nl
