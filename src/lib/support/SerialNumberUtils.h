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
 *      Utility functions for dealing with Nest style serial numbers.
 *
 */

#ifndef SERIALNUMBERUTILS_H_
#define SERIALNUMBERUTILS_H_

#include <Weave/Core/WeaveCore.h>

namespace nl {

extern void DateToManufacturingWeek(uint16_t year, uint8_t month, uint8_t day, uint16_t& mfgYear, uint8_t& mfgWeek);
extern void ManufacturingWeekToDate(uint16_t mfgYear, uint8_t mfgWeek, uint16_t& year, uint8_t& month, uint8_t& day);
extern WEAVE_ERROR ExtractManufacturingDateFromSerialNumber(const char *serialNum, uint16_t& year, uint8_t& month, uint8_t& day);
extern bool IsValidSerialNumber(const char *serialNum);

} // namespace nl

#endif /* SERIALNUMBERUTILS_H_ */
