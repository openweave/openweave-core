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
 *      This file contains implementation of the basic message codecs
 *      used in Time Services #WEAVE_CONFIG_TIME must be defined if
 *      Time Services are needed.
 *
 */

#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif

#include <Weave/Core/WeaveEncoding.h>
#include <Weave/Profiles/time/WeaveTime.h>
#include <Weave/Support/CodeUtils.h>
#include <Weave/Support/ErrorStr.h>
#include <Weave/Support/MathUtils.h>
#include <Weave/Support/logging/WeaveLogging.h>

#if WEAVE_CONFIG_TIME

using namespace nl::Weave::Encoding;

using namespace nl::Weave::Profiles::Time;

TimeChangeNotification::TimeChangeNotification(void)
{
}

WEAVE_ERROR TimeChangeNotification::Encode(PacketBuffer* const aMsg)
{
    // note that we assume the buffer is big enough for this message
    uint8_t *cursor = aMsg->Start();

    // write stuff into the message body
    // 16 reserved bits, all set to 0
    LittleEndian::Write16(cursor, 0);

    aMsg->SetDataLength(cursor - aMsg->Start());

    return WEAVE_NO_ERROR;
}

WEAVE_ERROR TimeChangeNotification::Decode(TimeChangeNotification * const aObject, PacketBuffer* const aMsg)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // we should have at least 16 reserved bits

    if (aMsg->DataLength() < 2)
    {
        ExitNow(err = WEAVE_ERROR_INVALID_MESSAGE_LENGTH);
    }

    {
        const uint8_t *cursor = aMsg->Start();
        const uint16_t status = LittleEndian::Read16(cursor);

        // non-zero reserved bits indicate the message has some extension that we are not aware of

        if (status != 0)
        {
            WeaveLogDetail(TimeService,
                "TimeSyncRequestAdvisory unknown extension, as reserved bits are not all 0s (0x%X)",
                status);
        }
    }

exit:
    WeaveLogFunctError(err);

    return err;
}

TimeSyncRequest::TimeSyncRequest(void) :
    mLikelihoodForResponse(kLikelihoodForResponse_Min)
{
}

void TimeSyncRequest::Init(const uint8_t aLikelihood, const bool aIsTimeCoordinator)
{
    mLikelihoodForResponse = aLikelihood;
    mIsTimeCoordinator = aIsTimeCoordinator;
}

WEAVE_ERROR TimeSyncRequest::Encode(PacketBuffer* const aMsg)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // validate the source object first

    // Note that we cannot put check on kLikelihoodForResponse_Min here, because it's currently set to 0.
    // The compiler would argue it's unnecessary
    if (mLikelihoodForResponse > kLikelihoodForResponse_Max)
    {
        ExitNow(err = WEAVE_ERROR_INCORRECT_STATE);
    }

    {
        // Compose the 16-bit status
        const uint16_t status = ((mLikelihoodForResponse & 0x1F) << 1) | (mIsTimeCoordinator ? 1 : 0);

        // write stuff into the message body
        // note that we assume the buffer is big enough for this message
        uint8_t *cursor = aMsg->Start();
        LittleEndian::Write16(cursor, status);

        // calculate the message length again
        aMsg->SetDataLength(cursor - aMsg->Start());
    }

exit:
    WeaveLogFunctError(err);

    return err;
}

WEAVE_ERROR TimeSyncRequest::Decode(TimeSyncRequest * const aObject, PacketBuffer* const aMsg)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // at least 2 bytes of status

    if (aMsg->DataLength() < 2)
    {
        ExitNow(err = WEAVE_ERROR_INVALID_MESSAGE_LENGTH);
    }

    {
        const uint8_t *cursor = aMsg->Start();
        const uint16_t status = LittleEndian::Read16(cursor);

        // lowest 1-bit
        aObject->mIsTimeCoordinator = status & 0x1;

        // next 5-bit
        aObject->mLikelihoodForResponse = (status >> 1) & 0x1F;

        // Note that we cannot put check on kLikelihoodForResponse_Min here, because it's currently set to 0.
        // The compiler would argue it's unnecessary
        if (aObject->mLikelihoodForResponse > kLikelihoodForResponse_Max)
        {
            ExitNow(err = WEAVE_ERROR_MESSAGE_INCOMPLETE);
        }

        // anything other than the low 6-bit is reserved
        // non-zero reserved bits indicate the message has some extension that we are not aware of
        if ((status >> 6) != 0)
        {
            WeaveLogDetail(TimeService, "TimeSyncRequest unknown extension, as reserved bits are not all 0s (0x%X)",
                status);
        }
    }

exit:
    WeaveLogFunctError(err);

    return err;
}

TimeSyncResponse::TimeSyncResponse(void)
{
}

void TimeSyncResponse::Init(const TimeSyncRole aRole, const timesync_t aTimeOfRequest,
    const timesync_t aTimeOfResponse,
    const uint8_t aNumContributorInLastLocalSync, const uint16_t aTimeSinceLastSyncWithServer_min)
{
    mIsTimeCoordinator = (kTimeSyncRole_Coordinator == aRole) ? true : false;
    mTimeOfRequest = aTimeOfRequest;
    mTimeOfResponse = aTimeOfResponse;

    mNumContributorInLastLocalSync = aNumContributorInLastLocalSync;
    // clamp the number of contacts to kNumberOfContact_Max
    if (mNumContributorInLastLocalSync > kNumberOfContributor_Max)
    {
        mNumContributorInLastLocalSync = kNumberOfContributor_Max;
    }

    if (aTimeSinceLastSyncWithServer_min <= kTimeSinceLastSyncWithServer_Max)
    {
        mTimeSinceLastSyncWithServer_min = aTimeSinceLastSyncWithServer_min;
    }
    else
    {
        mTimeSinceLastSyncWithServer_min = kTimeSinceLastSyncWithServer_Invalid;
    }
}

WEAVE_ERROR TimeSyncResponse::Encode(PacketBuffer* const aMsg)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // Compose the 16-bit status
    const uint16_t status = uint16_t(((mNumContributorInLastLocalSync & 0x1F) << 1) | (mIsTimeCoordinator ? 1 : 0));
    const uint16_t freshness = uint16_t(mTimeSinceLastSyncWithServer_min & 0xFFF);

    // write stuff into the message body
    // note that we assume the buffer is big enough for this message
    uint8_t *cursor = aMsg->Start();

    LittleEndian::Write16(cursor, status);
    LittleEndian::Write64(cursor, mTimeOfRequest);
    LittleEndian::Write64(cursor, mTimeOfResponse);
    LittleEndian::Write16(cursor, freshness);

    // calculate the message length again
    aMsg->SetDataLength(cursor - aMsg->Start());

    WeaveLogFunctError(err);

    return err;
}

WEAVE_ERROR TimeSyncResponse::Decode(TimeSyncResponse * const aObject, PacketBuffer* const aMsg)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // at least 2 bytes of status plus 8 bytes of timestamp

    if (aMsg->DataLength() < 20)
    {
        ExitNow(err = WEAVE_ERROR_INVALID_MESSAGE_LENGTH);
    }

    {
        const uint8_t *cursor = aMsg->Start();
        const uint16_t status = LittleEndian::Read16(cursor);

        aObject->mTimeOfRequest = LittleEndian::Read64(cursor);
        aObject->mTimeOfResponse = LittleEndian::Read64(cursor);
        aObject->mTimeSinceLastSyncWithServer_min = (LittleEndian::Read16(cursor) & 0xFFF);

        if (aObject->mTimeOfRequest & MASK_INVALID_TIMESYNC)
        {
            ExitNow(err = WEAVE_ERROR_MESSAGE_INCOMPLETE);
        }

        if (aObject->mTimeOfResponse & MASK_INVALID_TIMESYNC)
        {
            ExitNow(err = WEAVE_ERROR_MESSAGE_INCOMPLETE);
        }

        // lowest 1-bit
        aObject->mIsTimeCoordinator = bool(status & 0x1);

        // next 5-bit
        aObject->mNumContributorInLastLocalSync = ((status >> 1) & 0x1F);

        // anything other than the low 6-bit is reserved
        // non-zero reserved bits indicate the message has some extension that we are not aware of
        if ((status >> 6) != 0)
        {
            WeaveLogDetail(TimeService, "TimeSyncResponse unknown extension, as reserved bits are not all 0s (0x%X)",
                status);
        }
    }

exit:
    WeaveLogFunctError(err);

    return err;
}

_TimeSyncNodeBase::_TimeSyncNodeBase(void) :
    FabricState(NULL),
    ExchangeMgr(NULL)
{

}

void _TimeSyncNodeBase::Init(WeaveFabricState * const aFabricState,
    WeaveExchangeManager * const aExchangeMgr)
{
    FabricState = aFabricState;
    ExchangeMgr = aExchangeMgr;
}

#endif // WEAVE_CONFIG_TIME
