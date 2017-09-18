/*
 *
 *    Copyright (c) 2014-2017 Nest Labs, Inc.
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
 *      This file defines status used in the Locale Profile.
 *
 */

#ifndef WEAVE_PROFILES_LOCALE_STATUS_HPP
#define WEAVE_PROFILES_LOCALE_STATUS_HPP

namespace nl {

namespace Weave {

namespace Profiles {

namespace Locale {

// Locale Status Codes
//

enum {

    kStatus_LocaleNotSupportedOrUnknown = 0x0001, // The requested locale is not supported or is unknown to the device.
    kStatus_LocaleImproperlyFormatted   = 0x0002  // The requested locale is not a properly IETF BCP 47-formatted locale.

};

}; // namespace Locale

}; // namespace Profiles

}; // namespace Weave

}; // namespace nl

#endif // WEAVE_PROFILES_LOCALE_STATUS_HPP
