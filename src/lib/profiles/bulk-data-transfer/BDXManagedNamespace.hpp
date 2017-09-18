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
 *    @file
 *      This file is the umbrella header for the Current Generation Bulk
 *      Data Transfer profile.
 *
 */

#ifndef _WEAVE_BDX_CURRENT_HPP
#define _WEAVE_BDX_CURRENT_HPP

#include <Weave/Support/ManagedNamespace.hpp>

#if defined(WEAVE_CONFIG_BDX_NAMESPACE) \
    && (WEAVE_CONFIG_BDX_NAMESPACE != kWeaveManagedNamespace_Current) \
    && (WEAVE_CONFIG_BDX_NAMESPACE != kWeaveManagedNamespace_Development)
#error "WEAVE_CONFIG_BDX_NAMESPACE defined, but not as namespace kWeaveManagedNamespace_Current or kWeaveManagedNamespace_Development"
#endif

#ifndef WEAVE_CONFIG_BDX_NAMESPACE
#define WEAVE_CONFIG_BDX_NAMESPACE kWeaveManagedNamespace_Current
#endif

namespace nl {
namespace Weave {
namespace Profiles {

/**
 *   @namespace nl::Weave::Profiles::BDX_Current
 *
 *   @brief
 *     This namespace includes all interfaces within Weave for the
 *     Bulk Data Transfer (BDX) profile that are currently in production use.
 */
namespace WeaveMakeManagedNamespaceIdentifier(BDX, kWeaveManagedNamespaceDesignation_Current) { };

#if WEAVE_CONFIG_BDX_NAMESPACE == kWeaveManagedNamespace_Current
namespace BulkDataTransfer = WeaveMakeManagedNamespaceIdentifier(BDX, kWeaveManagedNamespaceDesignation_Current);
#endif

}; // namespace Profiles
}; // namespace Weave
}; // namespace nl

#endif // _WEAVE_BDX_CURRENT_HPP
