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

/**
 *    @file
 *      This file defines application keys trait data sink interfaces.
 *
 */

#ifndef APPLICATION_KEYS_TRAIT_DATA_SINK_H_
#define APPLICATION_KEYS_TRAIT_DATA_SINK_H_

#include <Weave/Profiles/data-management/TraitData.h>
#include <Weave/Profiles/security/WeaveApplicationKeys.h>
#include <Weave/Profiles/security/ApplicationKeysTrait.h>
#include <Weave/Profiles/security/ApplicationKeysStructSchema.h>

namespace Schema {
namespace Weave {
namespace Trait {
namespace Auth {
namespace ApplicationKeysTrait {

/**
 *  @class ApplicationKeysTraitDataSink
 *
 *  @brief
 *    Contains interfaces for the Weave application keys trait data sink.
 *
 */
class ApplicationKeysTraitDataSink : public nl::Weave::Profiles::DataManagement::TraitDataSink
{
public:
    ApplicationKeysTraitDataSink(void);

    void SetGroupKeyStore(nl::Weave::Profiles::Security::AppKeys::GroupKeyStoreBase *groupKeyStore);

    WEAVE_ERROR OnEvent(uint16_t aType, void *aInEventParam) __OVERRIDE;

protected:
    nl::Weave::Profiles::Security::AppKeys::GroupKeyStoreBase *GroupKeyStore;

private:
    WEAVE_ERROR SetLeafData(nl::Weave::Profiles::DataManagement::PropertyPathHandle aLeafHandle, nl::Weave::TLV::TLVReader &aReader) __OVERRIDE;
};

} // ApplicationKeysTrait
} // Auth
} // Trait
} // Weave
} // Schema

#endif // APPLICATION_KEYS_TRAIT_DATA_SINK_H_
