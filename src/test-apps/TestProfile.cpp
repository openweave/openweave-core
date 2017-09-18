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

/**
 *    @file
 *      This file implements a simple profile and Weave Data
 *      Management (WDM) profile database to go with it for testing
 *      purposes.
 *
 */

#include <assert.h>

// This code tests the legacy version of Weave Data Management
#define WEAVE_CONFIG_DATA_MANAGEMENT_NAMESPACE kWeaveManagedNamespace_Legacy

#include "ToolCommon.h"
#include <Weave/Profiles/ProfileCommon.h>
#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Profiles/data-management/ProfileDatabase.h>
#include "TestProfile.h"

using namespace ::nl::Inet;
using namespace ::nl::Weave;
using namespace ::nl::Weave::TLV;
using namespace ::nl::Weave::Profiles;
using namespace ::nl::Weave::Profiles::Common;
using namespace ::nl::Weave::Profiles::DataManagement;
using namespace ::nl::Weave::Profiles::StatusReporting;

uint8_t LookupFailureMode(ReferencedTLVData &aPathList)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint8_t mode;
    TLVReader rdr;

    TLVType pathContainer;
    TLVType profileContainer;

    uint32_t profileId;
    uint64_t instanceId;

    err = OpenPathList(aPathList, rdr);
    SuccessOrExit(err);

    err = rdr.Next();
    SuccessOrExit(err);

    /*
     * we assume only a single list element here, which is to say
     * that if there's more than one element we ignore all but the
     * first.
     */

    err = rdr.EnterContainer(pathContainer);
    SuccessOrExit(err);

    /*
     * the first element of a path under WDM should be a structure
     * with 2 elements, one of which (the instance) is optional.
     */

    err = rdr.Next();
    SuccessOrExit(err);

    ValidateTLVType(kTLVType_Structure, rdr);
    SuccessOrExit(err);

    err = ValidateWDMTag(kTag_WDMPathProfile, rdr);
    SuccessOrExit(err);

    /*
     * parse the path profile and get the profile data object
     */

    err = rdr.EnterContainer(profileContainer);
    SuccessOrExit(err);

    /*
     * the first element here should be a profile ID
     */

    err = rdr.Next();
    SuccessOrExit(err);

    err = ValidateTLVType(kTLVType_UnsignedInteger, rdr);
    SuccessOrExit(err);

    err = ValidateWDMTag(kTag_WDMPathProfileId, rdr);
    SuccessOrExit(err);

    err = rdr.Get(profileId);
    SuccessOrExit(err);

    /*
     * and the second should be an instance
     */

    err = rdr.Next();
    SuccessOrExit(err);

    err = ValidateWDMTag(kTag_WDMPathProfileInstance, rdr);
    SuccessOrExit(err);

    err = rdr.Get(instanceId);
    SuccessOrExit(err);

    rdr.ExitContainer(profileContainer);
    rdr.ExitContainer(pathContainer);
    CloseList(rdr);

    /*
     *  OK. now we should have both the profile and instance ID in
     *  hand. figure out what we got.
     */

    VerifyOrExit(profileId == kWeaveProfile_Fail, mode = kFailureMode_Invalid);

    switch (instanceId)
    {
        case kFailureInstance_CloseConnection:
            mode = kFailureMode_CloseConnection;
            break;

        case kFailureInstance_NoResponse:
            mode = kFailureMode_NoResponse;
            break;

        default:
            mode = kFailureMode_Invalid;
            break;
    }

exit:
    if (err != WEAVE_NO_ERROR)
        mode = kFailureMode_Invalid;

    return mode;
}

TestProfileDB::TestData::TestData(void)
{
    mIntegerItem = 0;
    mPreviousIntegerItem = 0;
    mInstanceId = kInstanceIdNotSpecified;
}

TestProfileDB::TestData::~TestData(void)
{
    mIntegerItem = 0;
    mPreviousIntegerItem = 0;
    mInstanceId = kInstanceIdNotSpecified;
}

WEAVE_ERROR TestProfileDB::TestData::StoreItem(const uint64_t &aTag, TLVReader &aDataRdr)
{
    WEAVE_ERROR err;

    if (aTag == ContextTag(kTag_IntegerItem))
    {
        mPreviousIntegerItem = mIntegerItem;

        err = aDataRdr.Get(mIntegerItem);

        mVersion++;
    }

    else
        err = WEAVE_ERROR_INVALID_TLV_TAG;

    return err;
}

WEAVE_ERROR TestProfileDB::TestData::Retrieve(TLVReader &aPathRdr, TLVWriter &aDataWrtr)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint64_t tag;

    err = aPathRdr.Next();

    if (err == WEAVE_END_OF_TLV)
    {
        err = Retrieve(aDataWrtr);
        SuccessOrExit(err);
    }

    else
    {
        tag = aPathRdr.GetTag();

        /*
         * in this case, the path contained an additional tag accessing
         * a particular data item directly.
         */

        // write the path

        err = EncodePath(aDataWrtr,
                         ContextTag(kTag_WDMDataListElementPath),
                         kWeaveProfile_Test,
                         mInstanceId,
                         1,
                         tag);
        SuccessOrExit(err);

        // write the version

        err = aDataWrtr.Put(ContextTag(kTag_WDMDataListElementVersion), mVersion);
        SuccessOrExit(err);

        // and the data item

        err = aDataWrtr.Put(ContextTag(kTag_WDMDataListElementData), mIntegerItem);
    }

exit:
    return err;
}

WEAVE_ERROR TestProfileDB::TestData::Retrieve(TLVWriter &aDataWrtr)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TLVType structure;

    // write the path

    err = EncodePath(aDataWrtr,
                     ContextTag(kTag_WDMDataListElementPath),
                     kWeaveProfile_Test,
                     mInstanceId,
                     0);
    SuccessOrExit(err);

    // write the version

    err = aDataWrtr.Put(ContextTag(kTag_WDMDataListElementVersion), mVersion);
    SuccessOrExit(err);

    // and the whole structure

    err = aDataWrtr.StartContainer(ContextTag(kTag_WDMDataListElementData), kTLVType_Structure, structure);
    SuccessOrExit(err);

    err = aDataWrtr.Put(ContextTag(kTag_IntegerItem), mIntegerItem);
    SuccessOrExit(err);


    err = aDataWrtr.EndContainer(structure);
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR TestProfileDB::LookupProfileData(uint32_t aProfileId, TLVReader *aInstanceIdReader, ProfileData **aResult)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    ProfileData *data = static_cast<ProfileData *>(&mTestData);

    // we ignore the instance ID here

    if (aProfileId == kWeaveProfile_Test)
    {
        *aResult = data;
    }

    else
    {
        *aResult = NULL;

        err = WEAVE_ERROR_INVALID_PROFILE_ID;
    }

    return err;
}

void TestProfileDB::ChangeProfileData(void)
{
    mTestData.mPreviousIntegerItem = mTestData.mIntegerItem;

    mTestData.mIntegerItem = rand();

    mTestData.mVersion++;
}

void TestProfileDB::RevertProfileData(void)
{
    mTestData.mIntegerItem = mTestData.mPreviousIntegerItem;

    mTestData.mVersion--;
}
