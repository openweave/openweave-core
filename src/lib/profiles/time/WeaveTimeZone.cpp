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
 *      This file contains implementation of the TimeZoneUtcOffset class used in Time Services
 *      WEAVE_CONFIG_TIME must be defined if Time Services are needed
 *
 */

// __STDC_LIMIT_MACROS must be defined for UINT8_MAX to be defined for pre-C++11 clib
#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif // __STDC_LIMIT_MACROS

// __STDC_CONSTANT_MACROS must be defined for INT64_C and UINT64_C to be defined for pre-C++11 clib
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif // __STDC_CONSTANT_MACROS

// it is important for this first inclusion of stdint.h to have all the right switches turned ON
#include <stdint.h>

// __STDC_FORMAT_MACROS must be defined for PRIX64 to be defined for pre-C++11 clib
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif // __STDC_FORMAT_MACROS

// it is important for this first inclusion of inttypes.h to have all the right switches turned ON
#include <inttypes.h>

#include <Weave/Core/WeaveEncoding.h>
#include <Weave/Profiles/time/WeaveTime.h>
#include <Weave/Support/CodeUtils.h>
#include <Weave/Support/ErrorStr.h>
#include <Weave/Support/MathUtils.h>
#include <Weave/Support/logging/WeaveLogging.h>

#if WEAVE_CONFIG_TIME

using namespace nl::Weave::Profiles::Time;

WEAVE_ERROR TimeZoneUtcOffset::GetCurrentLocalTime(timesync_t * const aLocalTime, const timesync_t aUtcTime) const
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    bool found = false;

    if (1 == mSize)
    {
        // ignore the begin time check, as it shall be zero through decoding, anyway
        found = true;
        *aLocalTime = aUtcTime + timesync_t(mUtcOffsetRecord[0].mUtcOffset_sec) * 1000000;
    }
    else if (mSize >= 2)
    {
        for (int i = 0; i < mSize - 1; ++i)
        {
            if ((aUtcTime >= mUtcOffsetRecord[i].mBeginAt_usec) && (aUtcTime < mUtcOffsetRecord[i + 1].mBeginAt_usec))
            {
                // we found it!
                found = true;
                *aLocalTime = aUtcTime + timesync_t(mUtcOffsetRecord[i].mUtcOffset_sec) * 1000000;
                break;
            }
        }
    }
    else
    {
        // we don't have any records !
    }

    if (!found)
    {
        ExitNow(err= WEAVE_ERROR_KEY_NOT_FOUND);
    }

exit:
    WeaveLogFunctError(err);

    return err;
}

WEAVE_ERROR TimeZoneUtcOffset::Decode(const uint8_t * const aInputBuf, const uint32_t aDataSize)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint16_t status;
    int8_t numOfRecord;
    bool IsSubsequentOffsetAll32Bit;
    size_t minDataSizeNeeded;
    const uint8_t * cursor = aInputBuf;
    const uint8_t * const end_of_data = aInputBuf + aDataSize;

    mSize = 0;

    // status(2) + one record (4) == 6
    // status(2) + first record (12) + subsequent records == ....
    if ((end_of_data - cursor) < 6)
    {
        ExitNow(err = WEAVE_ERROR_INVALID_MESSAGE_LENGTH);
    }
    memcpy(&status, cursor, 2);
    cursor += 2;

    numOfRecord = int8_t(status & 0xF);
    IsSubsequentOffsetAll32Bit = (status & 0x10) ? true : false;
    if (0 != (status >> 5))
    {
        WeaveLogDetail(TimeService, "TimeZoneUtcOffset::Decode not all reserved bits are zero: 0x%X", status);
    }

    if (numOfRecord > WEAVE_CONFIG_TIME_NUM_UTC_OFFSET_RECORD)
    {
        WeaveLogDetail(TimeService, "TimeZoneUtcOffset::Decode received more offset records than we can store: %d",
            numOfRecord);
        numOfRecord = WEAVE_CONFIG_TIME_NUM_UTC_OFFSET_RECORD;
    }

    if (0 == numOfRecord)
    {
        // there must be at least one record
        ExitNow(err = WEAVE_ERROR_INVALID_LIST_LENGTH);
    }
    else if (1 == numOfRecord)
    {
        minDataSizeNeeded = 4;

        // note IsSubsequentOffsetAll32Bit shall be zero in this case, but we skipping verification here to save code space

        if (size_t(end_of_data - cursor) < minDataSizeNeeded)
        {
            ExitNow(err = WEAVE_ERROR_INVALID_MESSAGE_LENGTH);
        }

        // set the timestamp to 0, as it will be ignored at calculation when mSize == 1, anyway
        mUtcOffsetRecord[0].mBeginAt_usec = 0;

        memcpy(&mUtcOffsetRecord[0].mUtcOffset_sec, cursor, 4);
        cursor += 4;
    }
    else
    {
        if (IsSubsequentOffsetAll32Bit)
        {
            minDataSizeNeeded = 8 + 4 + (numOfRecord - 1) * 8;
        }
        else
        {
            minDataSizeNeeded = 8 + 4 + (numOfRecord - 1) * 6;
        }

        if (size_t(end_of_data - cursor) < minDataSizeNeeded)
        {
            ExitNow(err = WEAVE_ERROR_INVALID_MESSAGE_LENGTH);
        }

        memcpy(&mUtcOffsetRecord[0].mBeginAt_usec, cursor, 8);
        cursor += 8;

        memcpy(&mUtcOffsetRecord[0].mUtcOffset_sec, cursor, 4);
        cursor += 4;

        for (int i = 1; i < numOfRecord; ++i)
        {
            int32_t temp32;

            memcpy(&temp32, cursor, 4);
            cursor += 4;
            mUtcOffsetRecord[i].mBeginAt_usec = mUtcOffsetRecord[i - 1].mBeginAt_usec + timesync_t(temp32) * 1000000;

            if (IsSubsequentOffsetAll32Bit)
            {
                memcpy(&temp32, cursor, 4);
                cursor += 4;
            }
            else
            {
                int16_t temp16;
                memcpy(&temp16, cursor, 2);
                cursor += 2;
                // sign extension shall happen here
                temp32 = temp16;
            }
            mUtcOffsetRecord[i].mUtcOffset_sec = mUtcOffsetRecord[i - 1].mUtcOffset_sec + temp32;
        }
    }

    mSize = numOfRecord;

exit:
    // We update the cursor out of good hygiene,
    // such that if the code is extended in the future such that the cursor is used,
    // it will be in the correct position for such code.
    IgnoreUnusedVariable(cursor);
    WeaveLogFunctError(err);

    return err;
}

WEAVE_ERROR TimeZoneUtcOffset::Encode(uint8_t * const aOutputBuf, uint32_t * const aDataSize)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    uint16_t status = 0;
    bool IsSubsequentOffsetAll32Bit = false;
    uint8_t * cursor = aOutputBuf;

    {
        const size_t buffer_size_needed = 2 + 8 + 4 + (mSize - 1) * 8;

        if (*aDataSize < buffer_size_needed)
        {
            *aDataSize = buffer_size_needed;
            ExitNow(err = WEAVE_ERROR_INVALID_MESSAGE_LENGTH);
        }
    }
    *aDataSize = 0;

    if ((mSize > 0xF) || (mSize > WEAVE_CONFIG_TIME_NUM_UTC_OFFSET_RECORD))
    {
        ExitNow(err = WEAVE_ERROR_INVALID_LIST_LENGTH);
    }
    status = mSize & 0xF;

    if (mSize < 1)
    {
        ExitNow(err = WEAVE_ERROR_INVALID_LIST_LENGTH);
    }
    else if (1 == mSize)
    {
        // this means we don't use DST, so there is only one record to check and encode
        // since there is only one and no subsequent record, the IsSubsequentOffsetAll32Bit is redundant and shall be 0
        memcpy(cursor, &status, 2);
        cursor += 2;
        memcpy(cursor, &mUtcOffsetRecord[0].mUtcOffset_sec, 4);
        cursor += 4;
    }
    else
    {
        // round 1: check if we need 4 bytes in the offsets
        for (int i = 1; i < mSize; ++i)
        {
            const int32_t diff_sec = mUtcOffsetRecord[i].mUtcOffset_sec - mUtcOffsetRecord[i - 1].mUtcOffset_sec;
            if ((diff_sec > INT16_MAX) || (diff_sec < INT16_MIN))
            {
                IsSubsequentOffsetAll32Bit = true;
            }
        }

        if (IsSubsequentOffsetAll32Bit)
        {
            status |= (1 << 4);
        }

        memcpy(cursor, &status, 2);
        cursor += 2;

        memcpy(cursor, &mUtcOffsetRecord[0].mBeginAt_usec, 8);
        cursor += 8;
        memcpy(cursor, &mUtcOffsetRecord[0].mUtcOffset_sec, 4);
        cursor += 4;

        // round 2: fill in subsequent records
        for (int i = 1; i < mSize; ++i)
        {
            int32_t temp32;

            const timesync_t diff_timestamp_sec =
                Platform::Divide((mUtcOffsetRecord[i].mBeginAt_usec - mUtcOffsetRecord[i - 1].mBeginAt_usec), 1000000);
            if (diff_timestamp_sec <= 0)
            {
                ExitNow(err = WEAVE_ERROR_INCORRECT_STATE);
            }
            else if (diff_timestamp_sec > INT32_MAX)
            {
                ExitNow(err = WEAVE_ERROR_INCORRECT_STATE);
            }
            {
                temp32 = int32_t(diff_timestamp_sec);
                memcpy(cursor, &temp32, 4);
                cursor += 4;
            }

            temp32 = mUtcOffsetRecord[i].mUtcOffset_sec - mUtcOffsetRecord[i - 1].mUtcOffset_sec;
            if (IsSubsequentOffsetAll32Bit)
            {
                memcpy(cursor, &temp32, 4);
                cursor += 4;
            }
            else
            {
                // sign shall be carried over
                const int temp16 = temp32;
                memcpy(cursor, &temp16, 2);
                cursor += 2;
            }
        }
    }

    *aDataSize = cursor - aOutputBuf;

exit:
    WeaveLogFunctError(err);

    return err;
}

#endif // WEAVE_CONFIG_TIME
