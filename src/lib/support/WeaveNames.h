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
 *    @file
 *      This file declares functions for converting various ids into human-readable strings.
 *
 */

namespace nl {
namespace Weave {

const char *GetVendorName(uint16_t vendorId);
const char *GetProfileName(uint32_t profileId);
const char *GetMessageName(uint32_t profileId, uint8_t msgType);

} // namespace Weave
} // namespace nl
