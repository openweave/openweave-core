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
 *      This file implements a process to effect a functional test for
 *      the Weave Nest serial number support utilities.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

#include <Weave/Support/logging/WeaveLogging.h>
#include <Weave/Support/SerialNumberUtils.h>

using namespace nl;

void Abort()
{
    abort();
}

void TestAssert(bool assert, const char *msg)
{
    if (!assert)
    {
        printf("%s\n", msg);
        Abort();
    }
}

class MfgDateTestCase
{
public:
    uint16_t inYear;
    uint8_t inMonth;
    uint8_t inDay;
    uint16_t outMfgYear;
    uint8_t outMfgWeek;
    uint16_t outYear;
    uint8_t outMonth;
    uint8_t outDay;
};

MfgDateTestCase gMfgDateTestCases[] =
{
    // inYear, inMonth, inDay, outMfgYear, outMfgWeek, outYear, outMonth, outDay
    {    2011,       1,     3,       2011,          2,    2011,        1,      2  },
    {    2011,       7,    31,       2011,         32,    2011,        7,     31  },
    {    2011,       9,    14,       2011,         38,    2011,        9,     11  },
    {    2011,      12,    31,       2011,         53,    2011,       12,     25  },

    {    2012,       2,    26,       2012,          9,    2012,        2,     26  },
    {    2012,       2,    27,       2012,          9,    2012,        2,     26  },
    {    2012,       2,    28,       2012,          9,    2012,        2,     26  },
    {    2012,       2,    29,       2012,          9,    2012,        2,     26  },
    {    2012,       3,     1,       2012,          9,    2012,        2,     26  },
    {    2012,       3,     2,       2012,          9,    2012,        2,     26  },
    {    2012,       3,     3,       2012,          9,    2012,        2,     26  },
    {    2012,       3,     4,       2012,         10,    2012,        3,      4  },

    {    2013,       1,     2,       2013,          1,    2012,       12,     30  },
    {    2013,       1,     9,       2013,          2,    2013,        1,      6  },
    {    2013,       1,    16,       2013,          3,    2013,        1,     13  },
    {    2013,       1,    23,       2013,          4,    2013,        1,     20  },
    {    2013,       1,    30,       2013,          5,    2013,        1,     27  },
    {    2013,       2,     6,       2013,          6,    2013,        2,      3  },
    {    2013,       2,    13,       2013,          7,    2013,        2,     10  },
    {    2013,       2,    20,       2013,          8,    2013,        2,     17  },
    {    2013,       2,    27,       2013,          9,    2013,        2,     24  },
    {    2013,       3,     6,       2013,         10,    2013,        3,     03  },
    {    2013,       3,    13,       2013,         11,    2013,        3,     10  },
    {    2013,       3,    20,       2013,         12,    2013,        3,     17  },
    {    2013,       3,    27,       2013,         13,    2013,        3,     24  },
    {    2013,       4,     3,       2013,         14,    2013,        3,     31  },
    {    2013,       4,    10,       2013,         15,    2013,        4,      7  },
    {    2013,       4,    17,       2013,         16,    2013,        4,     14  },
    {    2013,       4,    24,       2013,         17,    2013,        4,     21  },
    {    2013,       5,     1,       2013,         18,    2013,        4,     28  },
    {    2013,       5,     8,       2013,         19,    2013,        5,      5  },
    {    2013,       5,    15,       2013,         20,    2013,        5,     12  },
    {    2013,       5,    22,       2013,         21,    2013,        5,     19  },
    {    2013,       5,    29,       2013,         22,    2013,        5,     26  },
    {    2013,       6,     5,       2013,         23,    2013,        6,      2  },
    {    2013,       6,    12,       2013,         24,    2013,        6,      9  },
    {    2013,       6,    19,       2013,         25,    2013,        6,     16  },
    {    2013,       6,    26,       2013,         26,    2013,        6,     23  },
    {    2013,       7,     3,       2013,         27,    2013,        6,     30  },
    {    2013,       7,    10,       2013,         28,    2013,        7,      7  },
    {    2013,       7,    17,       2013,         29,    2013,        7,     14  },
    {    2013,       7,    24,       2013,         30,    2013,        7,     21  },
    {    2013,       7,    31,       2013,         31,    2013,        7,     28  },
    {    2013,       8,     7,       2013,         32,    2013,        8,      4  },
    {    2013,       8,    14,       2013,         33,    2013,        8,     11  },
    {    2013,       8,    21,       2013,         34,    2013,        8,     18  },
    {    2013,       8,    28,       2013,         35,    2013,        8,     25  },
    {    2013,       9,     4,       2013,         36,    2013,        9,      1  },
    {    2013,       9,    11,       2013,         37,    2013,        9,      8  },
    {    2013,       9,    18,       2013,         38,    2013,        9,     15  },
    {    2013,       9,    25,       2013,         39,    2013,        9,     22  },
    {    2013,      10,     2,       2013,         40,    2013,        9,     29  },
    {    2013,      10,     9,       2013,         41,    2013,       10,      6  },
    {    2013,      10,    16,       2013,         42,    2013,       10,     13  },
    {    2013,      10,    23,       2013,         43,    2013,       10,     20  },
    {    2013,      10,    30,       2013,         44,    2013,       10,     27  },
    {    2013,      11,     6,       2013,         45,    2013,       11,      3  },
    {    2013,      11,    13,       2013,         46,    2013,       11,     10  },
    {    2013,      11,    20,       2013,         47,    2013,       11,     17  },
    {    2013,      11,    27,       2013,         48,    2013,       11,     24  },
    {    2013,      12,     4,       2013,         49,    2013,       12,      1  },
    {    2013,      12,    11,       2013,         50,    2013,       12,      8  },
    {    2013,      12,    18,       2013,         51,    2013,       12,     15  },
    {    2013,      12,    25,       2013,         52,    2013,       12,     22  },

    {    2015,       1,     1,       2015,          1,    2014,       12,     28  },
    {    2015,       2,     9,       2015,          7,    2015,        2,      8  },

    { 0 }
};

void TestMfgDateConversion(void)
{
    for (MfgDateTestCase *testCase = gMfgDateTestCases; testCase->inYear != 0; testCase++)
    {
        uint16_t mfgYear;
        uint8_t mfgWeek;
        uint16_t year;
        uint8_t month;
        uint8_t day;

        DateToManufacturingWeek(testCase->inYear, testCase->inMonth, testCase->inDay, mfgYear, mfgWeek);
        TestAssert(mfgYear == testCase->outMfgYear, "Invalid mfgYear returned by DateToManufacturingWeek()");
        TestAssert(mfgWeek == testCase->outMfgWeek, "Invalid mfgWeek returned by DateToManufacturingWeek()");
        ManufacturingWeekToDate(mfgYear, mfgWeek, year, month, day);
        TestAssert(year == testCase->outYear, "Invalid year returned by ManufacturingWeekToDate()");
        TestAssert(month == testCase->outMonth, "Invalid month returned by ManufacturingWeekToDate()");
        TestAssert(day == testCase->outDay, "Invalid day returned by ManufacturingWeekToDate()");
    }
}

void TestMfgDateFromSerialNum(void)
{
    WEAVE_ERROR err;
    uint16_t year;
    uint8_t month;
    uint8_t day;

    for (MfgDateTestCase *testCase = gMfgDateTestCases; testCase->inYear != 0; testCase++)
    {
        char serialNum[18];

        snprintf(serialNum, sizeof (serialNum), "02AA01AB%02u%02u001P", testCase->outMfgWeek, testCase->outMfgYear % 100);

        err = ExtractManufacturingDateFromSerialNumber(serialNum, year, month, day);
        TestAssert(err == WEAVE_NO_ERROR, "Error returned by ExtractManufacturingDateFromSerialNumber()");
        TestAssert(year == testCase->outYear, "Invalid year returned by ExtractManufacturingDateFromSerialNumber()");
        TestAssert(month == testCase->outMonth, "Invalid month returned by ExtractManufacturingDateFromSerialNumber()");
        TestAssert(day == testCase->outDay, "Invalid day returned by ExtractManufacturingDateFromSerialNumber()");
    }

    err = ExtractManufacturingDateFromSerialNumber("02AA01AC25130CD", year, month, day);
    TestAssert(err == WEAVE_ERROR_INVALID_ARGUMENT, "Error not detected by ExtractManufacturingDateFromSerialNumber()");

    err = ExtractManufacturingDateFromSerialNumber("02AA01AC25130CD87", year, month, day);
    TestAssert(err == WEAVE_ERROR_INVALID_ARGUMENT, "Error not detected by ExtractManufacturingDateFromSerialNumber()");

    err = ExtractManufacturingDateFromSerialNumber("02AA01AC251A0CD8", year, month, day);
    TestAssert(err == WEAVE_ERROR_INVALID_ARGUMENT, "Error not detected by ExtractManufacturingDateFromSerialNumber()");

    err = ExtractManufacturingDateFromSerialNumber("02AA01AC,5130CD8", year, month, day);
    TestAssert(err == WEAVE_ERROR_INVALID_ARGUMENT, "Error not detected by ExtractManufacturingDateFromSerialNumber()");
}

void TestSerialNumValidation(void)
{
    static const char *GoodSNs[] =
    {
        "01AA02RA09140021",
        "01AA02RA2014000C",
        "01AA02RA2014002J",
        "01AA02RA20140042",
        "01AA02RA20140051",
        "01AA02RA2014006F",
        "01AA02RA201400F2",
        "01AA02RA201400FL",
        "01AA02RA201400HM",
        "02AA01AB391203BM",
        "02AA01AC0714060Y",
        "02AA01AB401203K8",
        "02AA01AC40130425",
        "02AA01AC071405AB",
        "02AA01AC071407LX",
        "02AA01AC071405WR",
        "02AA01AC071405A8",
        "02AA01AB371205S0",
        "02AA01AB4112091K",
        "02AA01AB381206N0",
        "01AA02RA20140048",
        "02AA01RC221400TQ",
        "02AA01RC221400GL",
        "02AA01RC2214010E",
        "02AA01AC35130FZ4",
        "02AA01AC35130FZ0",
        "02AA01AC4013045L",
        "02AA01RC221400R7",
        "02AA01AC401303XC",
        "01AA02AB0712005J",
        "01AA01RA26120091",
        "02AA01AB04130DLC",
        "02AA01AC211400SH",
        "02AA01AC211406P1",
        "02AA01AC211405L9",
        "05BA01AC0313003G",
        "05BA01AC231300AB",
        "05CA01AC291300AG",
        NULL
    };
    static const char *BadSNs[] =
    {
        "02AA01AC25130CD",    // too short
        "02AA01AC25130CD87",  // too long
        "02AA01AC75230CD8",   // invalid week
        "Z2AA01AC25130CD8",   // invalid character pos 1
        "0ZAA01AC25130CD8",   // invalid character pos 2
        "020A01AC25130CD8",   // invalid character pos 3
        "02A201AC25130CD8",   // invalid character pos 4
        "02AAZ1AC25130CD8",   // invalid character pos 5
        "02AA0ZAC25130CD8",   // invalid character pos 6
        "02AA019C25130CD8",   // invalid character pos 7
        "02AA01A925130CD8",   // invalid character pos 8
        "02AA01AAZ5130CD8",   // invalid character pos 9
        "02AA01AA2Z130CD8",   // invalid character pos 10
        "02AA01AA25Z30CD8",   // invalid character pos 11
        "02AA01AA251Z0CD8",   // invalid character pos 12
        "02AA01AA2513ICD8",   // invalid character pos 13
        "02AA01AA25130OD8",   // invalid character pos 14
        "02AA01AA25130C*8",   // invalid character pos 15
        "02AA01AA25130CD)",   // invalid character pos 16
        NULL
    };

    for (const char **sn = GoodSNs; *sn != NULL; sn++)
    {
        bool isValid = IsValidSerialNumber(*sn);
        if (!isValid)
            printf("%s\n", *sn);
        TestAssert(isValid, "IsValidSerialNumber() returned false for valid serial number");
    }

    for (const char **sn = BadSNs; *sn != NULL; sn++)
    {
        bool isValid = IsValidSerialNumber(*sn);
        if (isValid)
            printf("%s\n", *sn);
        TestAssert(!isValid, "IsValidSerialNumber() returned true for invalid serial number");
    }
}

int main()
{
    TestMfgDateConversion();
    TestMfgDateFromSerialNum();
    TestSerialNumValidation();
    printf("All tests passed\n");
    return 0;
}
