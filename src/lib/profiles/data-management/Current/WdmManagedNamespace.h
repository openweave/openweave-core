/*
 *
 *    Copyright (c) 2016-2017 Nest Labs, Inc.
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
#ifndef _WEAVE_DATA_MANAGEMENT_MANAGEDNAMESPACE_CURRENT_H
#define _WEAVE_DATA_MANAGEMENT_MANAGEDNAMESPACE_CURRENT_H

#include <Weave/Support/ManagedNamespace.hpp>

#if defined(WEAVE_CONFIG_DATA_MANAGEMENT_NAMESPACE) && WEAVE_CONFIG_DATA_MANAGEMENT_NAMESPACE != kWeaveManagedNamespace_Current
#error Compiling Weave Data Management current-designated managed namespace file with WEAVE_CONFIG_DATA_MANAGEMENT_NAMESPACE defined != kWeaveManagedNamespace_Current
#endif

#ifndef WEAVE_CONFIG_DATA_MANAGEMENT_NAMESPACE
#define WEAVE_CONFIG_DATA_MANAGEMENT_NAMESPACE kWeaveManagedNamespace_Current
#endif

namespace nl {
namespace Weave {
namespace Profiles {

/**
 *   @namespace nl::Weave::Profiles::DataManagement_Current
 *
 *   @brief
 *     This namespace includes all interfaces within Weave for the
 *     Weave Data Management (WDM) profile that are currently supported
 *     for production use.
 */
namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current) { };

namespace DataManagement = WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current);

}; // namespace Profiles
}; // namespace Weave
}; // namespace nl

#endif // _WEAVE_DATA_MANAGEMENT_MANAGEDNAMESPACE_CURRENT_H
