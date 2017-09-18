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
 *      Utility functions for processing Nest serial numbers.
 *
 */

#include <Weave/Core/WeaveCore.h>
#include <Weave/Support/TimeUtils.h>
#include <Weave/Support/CodeUtils.h>

#include "SerialNumberUtils.h"

namespace nl {

enum {
    kDaysInDecember = 31,
    kDaysInWeek = 7
};


/**
 *  @def DateToManufacturingWeek
 *
 *  @brief
 *    Convert a calendar date to a Nest manufacturing week and year.
 *
 *  @param year
 *    Calendar year of manufacture.
 *
 *  @param month
 *    Month of manufacture in standard form (1=January ... 12=December).
 *
 *  @param day
 *    Day of manufacture (1=1st, 2=2nd, ...).
 *
 *  @param mfgYear
 *    [OUTPUT] Nest manufacturing year. Note that this may not be the same as the input year.
 *
 *  @param mfgWeek
 *    [OUTPUT] Nest manufacturing week (1=1st week of year, 2=2nd week of year, ...).
 *
 */
void DateToManufacturingWeek(uint16_t year, uint8_t month, uint8_t day, uint16_t& mfgYear, uint8_t& mfgWeek)
{
    // For years that do not end on a Saturday, the last few days of the year belong to week 1 of the
    // following year.
    if (month == 12 and day >= (kDaysInDecember + 1) - FirstWeekdayOfYear(year + 1))
    {
        mfgWeek = 1;
        mfgYear = year + 1;
    }

    // Otherwise...
    else
    {
        // convert the calendar date to an ordinal date.
        uint16_t dayOfYear;
        CalendarDateToOrdinalDate(year, month, day, dayOfYear);

        // Compute the manufacturing week from ordinal date.
        mfgWeek = (dayOfYear + FirstWeekdayOfYear(year) - 1) / kDaysInWeek + 1;
        mfgYear = year;
    }
}

/**
 *  @def ManufacturingWeekToDate
 *
 *  @brief
 *    Convert a manufacturing year and week to a calendar date corresponding to the start of the manufacturing week.
 *
 *  @param mfgYear
 *    Nest manufacturing year.
 *
 *  @param mfgWeek
 *    Nest manufacturing week (1=1st week of year, 2=2nd week of year, ...).
 *
 *  @param year
 *    Calendar year of manufacture.  Note that this may be different from mfgYear.
 *
 *  @param month
 *    Month of manufacture in standard form (1=January ... 12=December).
 *
 *  @param day
 *    Day of month corresponding to the first day of the manufacturing week, in standard form (1=1st, 2=2nd, ...).
 *
 */
void ManufacturingWeekToDate(uint16_t mfgYear, uint8_t mfgWeek, uint16_t& year, uint8_t& month, uint8_t& day)
{
    uint8_t firstWeekdayOfYear = FirstWeekdayOfYear(mfgYear);

    // Week 1 is special...
    if (mfgWeek == 1)
    {
        // If the year starts on a Sunday, then than week 1 starts on 1/1.
        if (firstWeekdayOfYear == 0)
        {
            year = mfgYear;
            month = 1;
            day = 1;
        }

        // Otherwise week 1 starts on the last Sunday of the previous year.
        else
        {
            year = mfgYear - 1;
            month = 12;
            day = (kDaysInDecember + 1) - firstWeekdayOfYear;
        }
    }

    // For all other weeks, compute the day of year from the week number and convert that to a calendar date.
    else
    {
        uint16_t dayOfYear = ((mfgWeek - 1) * kDaysInWeek) + 1 - firstWeekdayOfYear;
        OrdinalDateToCalendarDate(mfgYear, dayOfYear, month, day);
        year = mfgYear;
    }
}

#define CONVERT_DECIMAL2(PTR, VAL)                                      \
    do {                                                                \
        uint16_t v = (PTR)[0] - '0';                                    \
        VerifyOrExit(v <= 9, err = WEAVE_ERROR_INVALID_ARGUMENT);       \
        VAL = v * 10;                                                   \
        v = (PTR)[1] - '0';                                             \
        VerifyOrExit(v <= 9, err = WEAVE_ERROR_INVALID_ARGUMENT);       \
        VAL += v;                                                       \
    } while (0)

/**
 *  @def ExtractManufacturingDateFromSerialNumber
 *
 *  @brief
 *    Extracts the device manufacturing date from a Nest serial number.
 *
 *  @param serialNum
 *    The input serial number.
 *
 *  @param year
 *    [OUTPUT] Calendar year of manufacture.
 *
 *  @param month
 *    [OUTPUT] Month of manufacture in standard form (1=January ... 12=December).
 *
 *  @param day
 *    [OUTPUT] The day of manufacture, in standard form (1=1st, 2=2nd, ...).
 *
 *  @return
 *    WEAVE_NO_ERROR, or an error code if the input serial number could not be parsed.
 *
 *  @note
 *    Because Nest serial numbers have resolution to the week only, the returned date
 *    represents the start of the week in which the device was manufactured. This day
 *    is always a Sunday.
 */
WEAVE_ERROR ExtractManufacturingDateFromSerialNumber(const char *serialNum, uint16_t& year, uint8_t& month, uint8_t& day)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint16_t mfgWeek;
    uint16_t mfgYear;

    VerifyOrExit(strlen(serialNum) == 16, err = WEAVE_ERROR_INVALID_ARGUMENT);

    CONVERT_DECIMAL2(serialNum + 8, mfgWeek);
    CONVERT_DECIMAL2(serialNum + 10, mfgYear);
    mfgYear += 2000;

    ManufacturingWeekToDate(mfgYear, mfgWeek, year, month, day);

exit:
    return err;
}

static bool IsBase34NoIOChar(char ch)
{
    // return true iff the input character is in the range A-Z,0-9 excluding the characters I and O.
    return (ch >= 'A' && ch <= 'H') || (ch >= 'J' && ch <= 'N') || (ch >= 'P' && ch <= 'Z') || (ch >= '0' && ch <= '9');
}

/**
 *  @def IsValidSerialNumber
 *
 *  @brief
 *    Verify that the supplies string conforms to the Nest serial number syntax.
 *
 *  @param serialNum
 *    The string to be tested.
 *
 *  @return
 *    True if the string is a Nest serial number.
 *
 */
bool IsValidSerialNumber(const char *serialNum)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint16_t mfgWeek;
    uint16_t mfgYear;

    // Verify the length
    VerifyOrExit(strlen(serialNum) == 16, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Verify product number field
    VerifyOrExit(serialNum[0] >= '0' && serialNum[0] <= '9', err = WEAVE_ERROR_INVALID_ARGUMENT);
    VerifyOrExit(serialNum[1] >= '0' && serialNum[1] <= '9', err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Verify SKU/board field
    VerifyOrExit(serialNum[2] >= 'A' && serialNum[2] <= 'Z', err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Verify hardware revision and version level fields
    VerifyOrExit(serialNum[3] >= 'A' && serialNum[3] <= 'Z', err = WEAVE_ERROR_INVALID_ARGUMENT);
    VerifyOrExit(serialNum[4] >= '0' && serialNum[4] <= '9', err = WEAVE_ERROR_INVALID_ARGUMENT);
    VerifyOrExit(serialNum[5] >= '0' && serialNum[5] <= '9', err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Verify manufacturer/site field
    VerifyOrExit(serialNum[6] >= 'A' && serialNum[6] <= 'Z', err = WEAVE_ERROR_INVALID_ARGUMENT);
    VerifyOrExit(serialNum[7] >= 'A' && serialNum[7] <= 'Z', err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Verify week of manufacture field and value
    CONVERT_DECIMAL2(serialNum + 8, mfgWeek);
    VerifyOrExit(mfgWeek >= 1 && mfgWeek <= 53, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Verify year of manufacture field
    CONVERT_DECIMAL2(serialNum + 10, mfgYear);

    // Unit number field
    VerifyOrExit(IsBase34NoIOChar(serialNum[12]), err = WEAVE_ERROR_INVALID_ARGUMENT);
    VerifyOrExit(IsBase34NoIOChar(serialNum[13]), err = WEAVE_ERROR_INVALID_ARGUMENT);
    VerifyOrExit(IsBase34NoIOChar(serialNum[14]), err = WEAVE_ERROR_INVALID_ARGUMENT);
    VerifyOrExit(IsBase34NoIOChar(serialNum[15]), err = WEAVE_ERROR_INVALID_ARGUMENT);

exit:
    return err == WEAVE_NO_ERROR;
}

}
