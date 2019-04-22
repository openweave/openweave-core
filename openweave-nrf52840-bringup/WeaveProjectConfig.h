/*
 *
 *    Copyright (c) 2019 Google LLC.
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
 *          Example project configuration file for OpenWeave.
 *
 *          This is a place to put application or project-specific overrides
 *          to the default configuration values for general OpenWeave features.
 *
 */

#ifndef WEAVE_PROJECT_CONFIG_H
#define WEAVE_PROJECT_CONFIG_H

// Use the default device id 18B4300000000003 if one hasn't been provisioned in flash.
#define WEAVE_DEVICE_CONFIG_ENABLE_TEST_DEVICE_IDENTITY 3

// Use a default pairing code if one hasn't been provisioned in flash.
#define WEAVE_DEVICE_CONFIG_USE_TEST_PAIRING_CODE "NESTUS"

// For convenience, enable Weave Security Test Mode and disable the requirement for
// authentication in various protocols.
//
//    WARNING: These options make it possible to circumvent basic Weave security functionality,
//    including message encryption. Because of this they MUST NEVER BE ENABLED IN PRODUCTION BUILDS.
//
#define WEAVE_CONFIG_SECURITY_TEST_MODE 1
#define WEAVE_CONFIG_REQUIRE_AUTH 0

#endif // WEAVE_PROJECT_CONFIG_H
