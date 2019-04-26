/*
 *
 *    Copyright (c) 2018 Nest Labs, Inc.
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

#ifndef WEAVE_DEVICE_ERROR_H
#define WEAVE_DEVICE_ERROR_H


#define WEAVE_DEVICE_ERROR_MIN 11000000
#define WEAVE_DEVICE_ERROR_MAX 11000999
#define _WEAVE_DEVICE_ERROR(e) (WEAVE_DEVICE_ERROR_MIN + (e))

/**
 *  @def WEAVE_DEVICE_ERROR_CONFIG_VALUE_NOT_FOUND
 *
 *  @brief
 *    The requested configuration value was not found.
 *
 */
#define WEAVE_DEVICE_ERROR_CONFIG_NOT_FOUND                   _WEAVE_DEVICE_ERROR(1)

/**
 *  @def WEAVE_DEVICE_ERROR_NOT_SERVICE_PROVISIONED
 *
 *  @brief
 *    The device has not been service provisioned.
 *
 */
#define WEAVE_DEVICE_ERROR_NOT_SERVICE_PROVISIONED            _WEAVE_DEVICE_ERROR(2)


#endif // WEAVE_DEVICE_ERROR_H
