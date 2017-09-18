/*
 *
 *    Copyright (c) 2013-2017 Nest Labs, Inc.
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
 *  @file
 *    This file is the master "umbrella" include file for Weave Data
 *    Management.
 *
 *    This files defines the Weave data management profile. See the
 *    document "Nest Weave-Data Management Protocol" document for a
 *    complete(ish) description.
 *
 */

#ifndef _WEAVE_DATA_MANAGEMENT_LEGACY_H
#define _WEAVE_DATA_MANAGEMENT_LEGACY_H

// here's the stuff that's required to make this build.

#include <Weave/Profiles/data-management/Legacy/WdmManagedNamespace.h>

#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveMessageLayer.h>
#include <Weave/Profiles/ProfileCommon.h>
#include <Weave/Profiles/service-directory/ServiceDirectory.h>

// logging support

#include <Weave/Support/ErrorStr.h>
#include <Weave/Support/logging/WeaveLogging.h>

#include <Weave/Profiles/data-management/Binding.h>
#include <Weave/Profiles/data-management/ClientDataManager.h>
#include <Weave/Profiles/data-management/ClientNotifier.h>
#include <Weave/Profiles/data-management/DMConstants.h>
#include <Weave/Profiles/data-management/DMClient.h>
#include <Weave/Profiles/data-management/DMPublisher.h>
#include <Weave/Profiles/data-management/PublisherDataManager.h>
#include <Weave/Profiles/data-management/ProtocolEngine.h>
#include <Weave/Profiles/data-management/TopicIdentifier.h>

#endif // _WEAVE_DATA_MANAGEMENT_LEGACY_H
