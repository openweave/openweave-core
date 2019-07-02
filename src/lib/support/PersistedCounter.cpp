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
#include <Weave/Support/CodeUtils.h>
#include <Weave/Support/logging/WeaveLogging.h>
#include <Weave/Support/PersistedCounter.h>
#include <Weave/Support/platform/PersistedStorage.h>

#include <stdlib.h>
#include <string.h>

namespace nl {
namespace Weave {

PersistedCounter::PersistedCounter(void) :
    MonotonicallyIncreasingCounter(),
    mStartingCounterValue(0),
    mEpoch(0)
{
    memset(&mId, 0, sizeof(mId));
}

PersistedCounter::~PersistedCounter(void)
{
}

WEAVE_ERROR
PersistedCounter::Init(const nl::Weave::Platform::PersistedStorage::Key aId,
                       uint32_t aEpoch)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // Store the ID.
    mId = aId;

    // Check and store the epoch.
    VerifyOrExit(aEpoch > 0, err = WEAVE_ERROR_INVALID_INTEGER_VALUE);
    mEpoch = aEpoch;

    // Read our previously-stored starting value.
    err = ReadStartValue(mStartingCounterValue);
    SuccessOrExit(err);

#if WEAVE_CONFIG_PERSISTED_COUNTER_DEBUG_LOGGING
    WeaveLogDetail(EventLogging, "PersistedCounter::Init() aEpoch 0x%x mStartingCounterValue 0x%x", aEpoch, mStartingCounterValue);
#endif

    // Write out the counter value with which we'll start next time we
    // boot up.
    err = WriteStartValue(mStartingCounterValue + mEpoch);
    SuccessOrExit(err);

    // This will set the starting value, after which we're ready.
    err = MonotonicallyIncreasingCounter::Init(mStartingCounterValue);
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR
PersistedCounter::Advance(void)
{
    return IncrementCount();
}

WEAVE_ERROR
PersistedCounter::AdvanceEpochRelative(uint32_t aValue)
{
    WEAVE_ERROR ret;

    mStartingCounterValue = (aValue / mEpoch) * mEpoch;  // Start of enclosing epoch
    mCounterValue = mStartingCounterValue + mEpoch - 1;  // Move to end of enclosing epoch
    ret = IncrementCount(); // Force to next epoch
#if WEAVE_CONFIG_PERSISTED_COUNTER_DEBUG_LOGGING
    WeaveLogError(EventLogging, "Advanced counter to 0x%x (relative to 0x%x)", mCounterValue, aValue);
#endif

    return ret;
}

bool
PersistedCounter::GetNextValue(uint32_t &aValue)
{
    bool startNewEpoch = false;

    // Increment aValue.
    aValue++;

    // If we've exceeded the value with which we started by aEpoch or
    // more, we need to start a new epoch.
    if ((aValue - mStartingCounterValue) >= mEpoch)
    {
        aValue = mStartingCounterValue + mEpoch;
        startNewEpoch = true;
    }

    return startNewEpoch;
}

WEAVE_ERROR
PersistedCounter::IncrementCount(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // Get the incremented value.
    if (GetNextValue(mCounterValue))
    {
        // Started a new epoch, so write out the next one.
        err = WriteStartValue(mCounterValue + mEpoch);
        SuccessOrExit(err);

        mStartingCounterValue = mCounterValue;
    }

exit:
    return err;
}

WEAVE_ERROR
PersistedCounter::WriteStartValue(uint32_t aStartValue)
{
#if WEAVE_CONFIG_PERSISTED_COUNTER_DEBUG_LOGGING
    WeaveLogDetail(EventLogging, "PersistedCounter::WriteStartValue() aStartValue 0x%x", aStartValue);
#endif

    return nl::Weave::Platform::PersistedStorage::Write(mId, aStartValue);
}

WEAVE_ERROR
PersistedCounter::ReadStartValue(uint32_t &aStartValue)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    aStartValue = 0;

    err = nl::Weave::Platform::PersistedStorage::Read(mId, aStartValue);
    if (err == WEAVE_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND)
    {
        // No previously-stored value, no worries, the counter is initialized to zero.
        // Suppress the error.
        err = WEAVE_NO_ERROR;
    }
    else
    {
        SuccessOrExit(err);
    }

#if WEAVE_CONFIG_PERSISTED_COUNTER_DEBUG_LOGGING
    WeaveLogDetail(EventLogging, "PersistedCounter::ReadStartValue() aStartValue 0x%x", aStartValue);
#endif

exit:
    return err;
}

} // Weave
} // nl
