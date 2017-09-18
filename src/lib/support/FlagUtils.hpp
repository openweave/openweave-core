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
 *      This file defines functions for manipulating Boolean flags in
 *      a bitfield.
 *
 */

#ifndef NL_WEAVE_SUPPORT_FLAGUTILS_HPP
#define NL_WEAVE_SUPPORT_FLAGUTILS_HPP

#include <stdint.h>

namespace nl {

extern bool GetFlag(const uint8_t inFlags, const uint8_t inFlag);
extern void ClearFlag(uint8_t &outFlags, const uint8_t inFlag);
extern void SetFlag(uint8_t &outFlags, const uint8_t inFlag, const bool inValue);

}; // namespace nl

#endif // NL_WEAVE_SUPPORT_FLAGUTILS_HPP
