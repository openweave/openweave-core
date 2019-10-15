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
 *      This file implements methods for the Nest Weave Status Report
 *      profile.
 */

#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveMessageLayer.h>
#include <Weave/Profiles/ProfileCommon.h>
#include <Weave/Support/CodeUtils.h>

using namespace ::nl::Weave;
using namespace ::nl::Weave::TLV;
using namespace ::nl::Weave::Profiles;
using namespace ::nl::Weave::Profiles::Common;

namespace nl {
namespace Weave {
namespace Profiles {
namespace StatusReporting {

StatusReport::StatusReport(void)
{
    mProfileId = 0;
    mStatusCode = 0;
    mError = WEAVE_NO_ERROR;
}

StatusReport::~StatusReport(void)
{
    mProfileId = 0;
    mStatusCode = 0;
    mError = WEAVE_NO_ERROR;
}

WEAVE_ERROR StatusReport::init(uint32_t aProfileId, uint16_t aCode, ReferencedTLVData *aInfo)
{
    mProfileId = aProfileId;
    mStatusCode = aCode;
    mError = WEAVE_NO_ERROR;

    if (aInfo)
        mAdditionalInfo = *aInfo;

    return WEAVE_NO_ERROR;
}

WEAVE_ERROR StatusReport::init(WEAVE_ERROR aError)
{
    if (aError == WEAVE_NO_ERROR)
    {
        mProfileId = kWeaveProfile_Common;
        mStatusCode = kStatus_Success;
    }

    else
    {
        mProfileId = kWeaveProfile_Common;
        mStatusCode = kStatus_InternalError;
        mError = aError;
    }

    return WEAVE_NO_ERROR;
}

WEAVE_ERROR StatusReport::pack(PacketBuffer *aBuffer, uint32_t maxLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    MessageIterator i(aBuffer);
    TLVWriter writer;

    i.append();

    err = i.write32(mProfileId);
    SuccessOrExit(err);

    err = i.write16(mStatusCode);
    SuccessOrExit(err);

    /*
     * the assumption here is that EITHER there's
     * an error code that wants to be included as
     * metadata OR there's additional info passed
     * in at initialization time, which may include
     * an error, OR there's none of the above, in
     * which case the else clause her writes nothing
     */

    if (mError != WEAVE_NO_ERROR)
    {
        writer.Init(aBuffer);

        err = StartMetaData(writer);
        SuccessOrExit(err);

        err = AddErrorCode(writer, mError);
        SuccessOrExit(err);

        err = EndMetaData(writer);
        SuccessOrExit(err);

        /*
         * this is a bit of a hack. we basically set this
         * so that packedLength() below will return the right
         * number at least if we call it AFTER the thing has
         * been packed. it's not clear we ever use this.
         */

        mAdditionalInfo.theLength = writer.GetLengthWritten();
    }

    else
    {
        err = mAdditionalInfo.pack(i, maxLen - 6);
    }

exit:
    return err;
}

inline uint16_t StatusReport::packedLength(void)
{
    return sizeof(mProfileId)+sizeof(mStatusCode)+mAdditionalInfo.theLength;
}

WEAVE_ERROR StatusReport::parse(PacketBuffer *aBuffer, StatusReport &aDestination)
{
    WEAVE_ERROR err;
    MessageIterator i(aBuffer);

    err = i.read32(&aDestination.mProfileId);
    SuccessOrExit(err);

    err = i.read16(&aDestination.mStatusCode);
    SuccessOrExit(err);

    err = ReferencedTLVData::parse(i, aDestination.mAdditionalInfo);

exit:
    return err;
}

bool StatusReport::operator == (const StatusReport &another) const
{
    return ((mProfileId == another.mProfileId) && (mStatusCode == another.mStatusCode));
}

/*
 * the universal, gold standard for success is
 * <Nest Labs> : <Common Profile> : <Success>
 */

bool StatusReport::success(void)
{
    return(mProfileId == kWeaveProfile_Common && mStatusCode == kStatus_Success);
}

WEAVE_ERROR StatusReport::StartMetaData(nl::Weave::TLV::TLVWriter &aWriter)
{
    TLVType metaDataContainer;

    return aWriter.StartContainer(AnonymousTag, kTLVType_Structure, metaDataContainer);
}

WEAVE_ERROR StatusReport::EndMetaData(nl::Weave::TLV::TLVWriter &aWriter)
{
    WEAVE_ERROR err;
    TLVType metaDataContainer = kTLVType_Structure;

    err = aWriter.EndContainer(metaDataContainer);
    SuccessOrExit(err);

    err = aWriter.Finalize();

exit:
    return err;
}

WEAVE_ERROR StatusReport::AddErrorCode(nl::Weave::TLV::TLVWriter &aWriter, WEAVE_ERROR aError)
{
    return aWriter.Put(CommonTag(kTag_SystemErrorCode), aError);
}
} // namespace StatusReporting
} // namespace Profiles
} // namespace Weave
} // nl
