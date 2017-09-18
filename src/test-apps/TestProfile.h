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
 *      This file defines a simple profile and Weave Data Management
 *      (WDM) profile database to go with it for testing purposes.
 *
 */

#ifndef _TEST_PROFILE_H
#define _TEST_PROFILE_H

// This code tests the legacy version of Weave Data Management
#define WEAVE_CONFIG_DATA_MANAGEMENT_NAMESPACE kWeaveManagedNamespace_Legacy

#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Profiles/data-management/ProfileDatabase.h>

enum
{
    /*
     * this is the "valid" test profile ID and it applies to the
     * simple profile defined below.
     */

    kWeaveProfile_Test               = 0x235A1234,

    kTag_IntegerItem                 = 1,
    kTestTopic                       = 0x235A000000004321ULL,

    /*
     * there is also a "bogus" profile ID that is used to cue specific
     * failure tests. the various tests are tied to profile instances.
     */

    kWeaveProfile_Fail               = 0x235A1235,

    kFailureInstance_CloseConnection = 0x235A123500000001ULL,
    kFailureInstance_NoResponse      = 0x235A123500000002ULL,

    kFailureMode_Invalid             = 0,
    kFailureMode_CloseConnection     = 1,
    kFailureMode_NoResponse          = 2,


    // miscellaneous items related to testing

    kTestBufferSize                  = 100,
    kUpdatePeriod                    = 10,

    /*
     * we define a response timeout of 2 seconds here. this is fine
     * for these tests but should be carefully considered in any
     * application where "real" communication is taking place. in
     * particular, if there's any chance that a response could get
     * dropped then either a timeout must be specified or some other
     * mechanism needs to be put in place to clear out the transaction
     * tables.
     */

    kDefaultDMResponseTimeout        = 2000 // 2 seconds
};

/*
 * this is a special method that looks up the requested failure mode
 * based on the the scheme outlined above where failures are called
 * for by invoking instances of a "bogus" profile on view requests.
 */

uint8_t LookupFailureMode(nl::Weave::Profiles::ReferencedTLVData &aPathList);

class TestProfileDB :
public nl::Weave::Profiles::DataManagement::ProfileDatabase
{
public:
    class TestData :
    public nl::Weave::Profiles::DataManagement::ProfileDatabase::ProfileData
    {
    public:
        TestData(void);

        ~TestData(void);

        WEAVE_ERROR StoreItem(const uint64_t &aTag, nl::Weave::TLV::TLVReader &aDataRdr);

        WEAVE_ERROR Retrieve(nl::Weave::TLV::TLVReader &aPathRdr, nl::Weave::TLV::TLVWriter &aDataWrtr);

        /*
         * this is a convenience method that allows the retrieval of the
         * whole thing without having to format a dummy path. and it can
         * be re-used to implement the above.
         */

        WEAVE_ERROR Retrieve(nl::Weave::TLV::TLVWriter &aDataWrtr);

        // data members

        uint32_t mIntegerItem;

        /*
         * we keep this around so we can revert a change
         */

        uint32_t mPreviousIntegerItem;

        /*
         * for testing purposes, it seems fine to have each test data
         * instance carry its own integer instance ID.
         */

        uint64_t mInstanceId;
    };

    WEAVE_ERROR LookupProfileData(nl::Weave::TLV::TLVReader &aPathReader, ProfileData **aResult);

    WEAVE_ERROR LookupProfileData(uint32_t aProfileId, nl::Weave::TLV::TLVReader *aPathReader, ProfileData **aResult);

    void ChangeProfileData(void);

    void RevertProfileData(void);

    TestData mTestData;
};

#endif // __TEST_PROFILE_H
