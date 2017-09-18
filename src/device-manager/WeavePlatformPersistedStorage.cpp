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

/**
 *    @file
 *      This file implements dummy version of persisted storage APIs for
 *      the Weave Python Device Manager.
 *
 */

#include <Weave/Support/platform/PersistedStorage.h>

namespace nl {
namespace Weave {
namespace Platform {
namespace PersistedStorage {

WEAVE_ERROR Read(const char *aKey, uint32_t &aValue)
{
    return WEAVE_NO_ERROR;
}

WEAVE_ERROR Write(const char *aKey, uint32_t aValue)
{
    return WEAVE_NO_ERROR;
}

} // PersistentStorage
} // Platform
} // Weave
} // nl
