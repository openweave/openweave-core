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

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

#include <Weave/Support/CodeUtils.h>
#include <Weave/Support/logging/WeaveLogging.h>

#include "MockTimeSyncUtil.h"

#if WEAVE_CONFIG_TIME

using namespace nl::Weave::Profiles::Time;

void Abort()
{
    abort();
}

void TestError(WEAVE_ERROR err, const char *msg)
{
    if (err != WEAVE_NO_ERROR)
    {
        printf("%s: %s\n", msg, nl::ErrorStr(err));
        Abort();
    }
}

void TestAssert(bool assert, const char *msg)
{
    if (!assert)
    {
        printf("%s\n", msg);
        Abort();
    }
}

void TestCase1(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    WeaveLogProgress(TimeService, "------------------------------------------------------------");
    // test case 1, size == 4, PDT
    {
        uint8_t buffer[TimeZoneUtcOffset::BufferSizeForEncoding] = { 0 };
        TimeZoneUtcOffset utc_offset_enc;
        TimeZoneUtcOffset utc_offset_dec;
        uint32_t size = sizeof(buffer);
        utc_offset_enc.mSize = 4;
        utc_offset_enc.mUtcOffsetRecord[0].mBeginAt_usec = 1394355600000000LL; // 3/9/2014 2AM PST
        utc_offset_enc.mUtcOffsetRecord[0].mUtcOffset_sec = -3600 * 7;
        utc_offset_enc.mUtcOffsetRecord[1].mBeginAt_usec = 1414922400000000LL; // 11/2/2014 2AM PDT
        utc_offset_enc.mUtcOffsetRecord[1].mUtcOffset_sec = -3600 * 8;
        utc_offset_enc.mUtcOffsetRecord[2].mBeginAt_usec = 1425805200000000LL; // 3/8/2015 2AM PST
        utc_offset_enc.mUtcOffsetRecord[2].mUtcOffset_sec = -3600 * 7;
        utc_offset_enc.mUtcOffsetRecord[3].mBeginAt_usec = 1446372000000000LL; // 11/2/2015 2AM PDT
        utc_offset_enc.mUtcOffsetRecord[3].mUtcOffset_sec = -3600 * 8;

        WeaveLogProgress(TimeService, "TimeZone Unit Test Case 1: normal case");
        WeaveLogProgress(TimeService, "Encoding buffer size %u, Number of records: %u", size, utc_offset_enc.mSize);
        err = utc_offset_enc.Encode(buffer, &size);
        SuccessOrExit(err);
        WeaveLogProgress(TimeService, "Encoding buffer size used %u");
        err = utc_offset_dec.Decode(buffer, size);
        SuccessOrExit(err);
        WeaveLogProgress(TimeService, "Decoded number of records: %u", utc_offset_dec.mSize);

        for (int i = 0; i < utc_offset_dec.mSize; ++i)
        {
            WeaveLogProgress(TimeService, "[%d] timestamp usec: %ld, offset sec %d", i,
                utc_offset_dec.mUtcOffsetRecord[i].mBeginAt_usec, utc_offset_dec.mUtcOffsetRecord[i].mUtcOffset_sec);
        }

        {
            timesync_t utc_time = 1403303320000000LL; // Fri Jun 20 15:28:40 2014, PDT
            timesync_t local_time;
            time_t temp;

            temp = time_t(utc_time / 1000000);
            // we use 'local time' to convert the utc to local time
            WeaveLogProgress(TimeService, "Sample time: %s", asctime(localtime(&temp)));

            // this time we use GetCurrentLocalTime to convert utc to local time
            err = utc_offset_dec.GetCurrentLocalTime(&local_time, utc_time);
            SuccessOrExit(err);
            temp = time_t(local_time / 1000000);
            // since it's already converted, we use gmtime to process it (as there is no further tz/dst correction applied)
            WeaveLogProgress(TimeService, "Local time: %s", asctime(gmtime(&temp)));
        }

        WeaveLogProgress(TimeService, "TimeZone Unit Test Case 1: succeeded");
    }

    WeaveLogProgress(TimeService, "------------------------------------------------------------");

exit:
    TestError(err, "Test Case 1 Failed");
}

void TestCase2(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    WeaveLogProgress(TimeService, "------------------------------------------------------------");
    // test case 2, size == 4, very large offsets and changes
    // Samoa, timezone Pacific/Apia, in the middle of Pacific ocean, changed their timezone from UTC-11 to UTC+13 in 2012,
    // essentially advanced one full day
    {
        uint8_t buffer[TimeZoneUtcOffset::BufferSizeForEncoding] = { 0 };
        TimeZoneUtcOffset utc_offset_enc;
        TimeZoneUtcOffset utc_offset_dec;
        uint32_t size = sizeof(buffer);
        utc_offset_enc.mSize = 4;
        utc_offset_enc.mUtcOffsetRecord[0].mBeginAt_usec = 1394355600000000LL; //
        utc_offset_enc.mUtcOffsetRecord[0].mUtcOffset_sec = -3600 * 11;
        utc_offset_enc.mUtcOffsetRecord[1].mBeginAt_usec = 1414922400000000LL; //
        utc_offset_enc.mUtcOffsetRecord[1].mUtcOffset_sec = +3600 * 13;
        utc_offset_enc.mUtcOffsetRecord[2].mBeginAt_usec = 1425805200000000LL; //
        utc_offset_enc.mUtcOffsetRecord[2].mUtcOffset_sec = +3600 * 14;
        utc_offset_enc.mUtcOffsetRecord[3].mBeginAt_usec = 1446372000000000LL; //
        utc_offset_enc.mUtcOffsetRecord[3].mUtcOffset_sec = +3600 * 13;

        WeaveLogProgress(TimeService, "TimeZone Unit Test Case 2: huge offset changes");
        WeaveLogProgress(TimeService, "Encoding buffer size %u, Number of records: %u", size, utc_offset_enc.mSize);
        err = utc_offset_enc.Encode(buffer, &size);
        SuccessOrExit(err);
        WeaveLogProgress(TimeService, "Encoding buffer size used %u");
        err = utc_offset_dec.Decode(buffer, size);
        SuccessOrExit(err);
        WeaveLogProgress(TimeService, "Decoded number of records: %u", utc_offset_dec.mSize);

        for (int i = 0; i < utc_offset_dec.mSize; ++i)
        {
            WeaveLogProgress(TimeService, "[%d] timestamp usec: %ld, offset sec %d", i,
                utc_offset_dec.mUtcOffsetRecord[i].mBeginAt_usec, utc_offset_dec.mUtcOffsetRecord[i].mUtcOffset_sec);
        }

        WeaveLogProgress(TimeService, "TimeZone Unit Test Case 2: succeeded");
    }

    WeaveLogProgress(TimeService, "------------------------------------------------------------");

exit:
    TestError(err, "Test Case 2 Failed");
}

void TestCase3(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    WeaveLogProgress(TimeService, "------------------------------------------------------------");
    // test case 2, size == 4, very large offsets and changes
    // Samoa, timezone Pacific/Apia, in the middle of Pacific ocean, changed their timezone from UTC-11 to UTC+13 in 2012,
    // essentially advanced one full day
    {
        uint8_t buffer[TimeZoneUtcOffset::BufferSizeForEncoding] = { 0 };
        TimeZoneUtcOffset utc_offset_enc;
        TimeZoneUtcOffset utc_offset_dec;
        uint32_t size = sizeof(buffer);
        utc_offset_enc.mSize = 1;
        utc_offset_enc.mUtcOffsetRecord[0].mBeginAt_usec = 0; //
        utc_offset_enc.mUtcOffsetRecord[0].mUtcOffset_sec = +3600 * 7;

        WeaveLogProgress(TimeService, "TimeZone Unit Test Case 3: no DST case, single UTC offset");
        WeaveLogProgress(TimeService, "Encoding buffer size %u, Number of records: %u", size, utc_offset_enc.mSize);
        err = utc_offset_enc.Encode(buffer, &size);
        SuccessOrExit(err);
        WeaveLogProgress(TimeService, "Encoding buffer size used %u");
        err = utc_offset_dec.Decode(buffer, size);
        SuccessOrExit(err);
        WeaveLogProgress(TimeService, "Decoded number of records: %u", utc_offset_dec.mSize);

        for (int i = 0; i < utc_offset_dec.mSize; ++i)
        {
            WeaveLogProgress(TimeService, "[%d] timestamp usec: %ld, offset sec %d", i,
                utc_offset_dec.mUtcOffsetRecord[i].mBeginAt_usec, utc_offset_dec.mUtcOffsetRecord[i].mUtcOffset_sec);
        }

        {
            timesync_t utc_time = 1403303320000000LL; // Fri Jun 20 15:28:40 2014, PDT
            timesync_t local_time;
            time_t temp;

            temp = time_t(utc_time / 1000000);
            // we use 'asctime/gmtime' to convert the time to string, with UTC offset 0
            WeaveLogProgress(TimeService, "Sample time: %s", asctime(gmtime(&temp)));

            // this time we use GetCurrentLocalTime to convert utc to local time
            err = utc_offset_dec.GetCurrentLocalTime(&local_time, utc_time);
            SuccessOrExit(err);
            temp = time_t(local_time / 1000000);
            // fixed offset shall be applied here
            WeaveLogProgress(TimeService, "Local time: %s", asctime(gmtime(&temp)));
        }

        WeaveLogProgress(TimeService, "TimeZone Unit Test Case 3: succeeded");
    }

    WeaveLogProgress(TimeService, "------------------------------------------------------------");

exit:
    TestError(err, "Test Case 3 Failed");
}

int main()
{
    TestCase1();
    TestCase2();
    TestCase3();

    printf("All tests passed\n");
    return 0;
}

#else // WEAVE_CONFIG_TIME

int main()
{
    printf("Weave Time is NOT ENABLED. Test Skipped\n");
    return -1;
}

#endif // WEAVE_CONFIG_TIME
