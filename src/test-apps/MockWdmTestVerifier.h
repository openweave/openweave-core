/*
 *
 *    Copyright (c) 2017 Nest Labs, Inc.
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
#ifndef MOCK_WDM_TRAIT_VERIFIER_H
#define MOCK_WDM_TRAIT_VERIFIER_H

#include <inttypes.h>
#include <Weave/Core/WeaveCore.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Profiles/common/CommonProfile.h>
#include <Weave/Profiles/data-management/DataManagement.h>

uint16_t DumpPublisherTraitChecksum(nl::Weave::Profiles::DataManagement::TraitDataSource *inTraitDataSource);
uint16_t DumpClientTraitChecksum(const nl::Weave::Profiles::DataManagement::TraitSchemaEngine *inSchemaEngine, nl::Weave::Profiles::DataManagement::TraitSchemaEngine::IDataSourceDelegate *);

#endif //MOCK_WDM_TRAIT_VERIFIER_H
