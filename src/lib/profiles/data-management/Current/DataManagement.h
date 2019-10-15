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
 *      This file is the umbrella header for the development Generation Weave
 *      Data Management (WDM) profile.
 *
 */

#ifndef _WEAVE_DATA_MANAGEMENT_CURRENT_H
#define _WEAVE_DATA_MANAGEMENT_CURRENT_H

/**
 *  Note that WDM Current can only work with features in BDX_Development
 */
#ifndef WEAVE_CONFIG_BDX_NAMESPACE
#define WEAVE_CONFIG_BDX_NAMESPACE kWeaveManagedNamespace_Development
#elif WEAVE_CONFIG_BDX_NAMESPACE != kWeaveManagedNamespace_Development
#define WEAVE_CONFIG_BDX_NAMESPACE kWeaveManagedNamespace_Development
#error "WEAVE_CONFIG_BDX_NAMESPACE defined, but not as namespace kWeaveManagedNamespace_Development"
#endif

#include <Weave/Profiles/bulk-data-transfer/Development/BDXManagedNamespace.hpp>

#include <Weave/Profiles/data-management/Current/WdmManagedNamespace.h>

#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Core/WeaveEncoding.h>
#include <Weave/Core/WeaveBinding.h>
#include <Weave/Core/WeaveServerBase.h>
#include <Weave/Core/WeaveTLVData.hpp>
#include <Weave/Core/WeaveMessageLayer.h>
#include <Weave/Core/WeaveTLVDebug.hpp>
#include <Weave/Core/WeaveTLVUtilities.hpp>
#include <Weave/Core/WeaveCircularTLVBuffer.h>

#include <Weave/Support/ErrorStr.h>
#include <Weave/Support/logging/WeaveLogging.h>
#include <Weave/Support/CodeUtils.h>
#include <Weave/Support/crypto/WeaveCrypto.h>

#include <Weave/Profiles/ProfileCommon.h>
#include <Weave/Profiles/security/WeaveSecurity.h>

#include <Weave/Profiles/data-management/MessageDef.h>
#include <Weave/Profiles/data-management/SubscriptionEngine.h>
#include <Weave/Profiles/data-management/Command.h>
#include <Weave/Profiles/data-management/SubscriptionHandler.h>
#include <Weave/Profiles/data-management/TraitData.h>
#include <Weave/Profiles/data-management/TraitCatalog.h>
#include <Weave/Profiles/data-management/GenericTraitCatalogImp.h>
#include <Weave/Profiles/data-management/NotificationEngine.h>
#include <Weave/Profiles/data-management/SubscriptionClient.h>
#include <Weave/Profiles/data-management/ViewClient.h>
#include <Weave/Profiles/data-management/UpdateClient.h>
#include <Weave/Profiles/data-management/EventLogging.h>
#include <Weave/Profiles/data-management/LoggingManagement.h>
#include <Weave/Profiles/data-management/EventLoggingTypes.h>
#include <Weave/Profiles/data-management/LoggingConfiguration.h>
#include <Weave/Profiles/data-management/EventProcessor.h>
#include <Weave/Profiles/data-management/LogBDXUpload.h>

#include <SystemLayer/SystemStats.h>

#endif // _WEAVE_DATA_MANAGEMENT_CURRENT_H
