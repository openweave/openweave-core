/*
 *
 *    Copyright (c) 2017 Nest Labs, Inc.
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
 *      This file implements a unit test suite for the Weave
 *      Profile Support interfaces.
 *
 */

#include <stdint.h>

#include <Weave/Support/ProfileStringSupport.hpp>

#include <nltest.h>

using namespace nl::Weave;

// Type Definitions

static const uint32_t kWeaveProfile_TestProfileStringSupport_1 = (0xFFF0 << 16 | 0xFEF0);
static const uint32_t kWeaveProfile_TestProfileStringSupport_2 = (0xFFF0 << 16 | 0xFEF1);
static const uint32_t kWeaveProfile_TestProfileStringSupport_3 = (0xFFF0 << 16 | 0xFEF2);

enum {
    kTestProfileStringSupport_1_MessageType_1 = 1,
    kTestProfileStringSupport_1_MessageType_2 = 2,

    kTestProfileStringSupport_2_MessageType_1 = 1,
    kTestProfileStringSupport_2_MessageType_2 = 2
};

// Forward Declarations

static const char * TestMessageStrFunct(uint32_t inProfileId, uint8_t inMsgType);
static const char * TestProfileStrFunct(uint32_t inProfileId);
static const char * TestStatusReportStrFunct(uint32_t inProfileId, uint16_t inStatusCode);

// Globals

static const struct Support::ProfileStringInfo sTestProfileStringInfo_1 = {
    kWeaveProfile_TestProfileStringSupport_1,
    TestMessageStrFunct,
    TestProfileStrFunct,
    TestStatusReportStrFunct
};

static struct Support::ProfileStringContext    sTestProfileStringContext_1  = {
    NULL,
    sTestProfileStringInfo_1
};

static const struct Support::ProfileStringInfo sTestProfileStringInfo_2 = {
    kWeaveProfile_TestProfileStringSupport_2,
    NULL,
    NULL,
    NULL
};

static struct Support::ProfileStringContext    sTestProfileStringContext_2  = {
    NULL,
    sTestProfileStringInfo_2
};

static const struct Support::ProfileStringInfo sTestProfileStringInfo_3 = {
    kWeaveProfile_TestProfileStringSupport_3,
    NULL,
    NULL,
    NULL
};

static struct Support::ProfileStringContext    sTestProfileStringContext_3  = {
    NULL,
    sTestProfileStringInfo_3
};

static struct Support::ProfileStringContext    sTestProfileStringContext_1_Alias  = {
    NULL,
    sTestProfileStringInfo_1
};

static struct Support::ProfileStringContext    sTestProfileStringContext_2_Alias  = {
    NULL,
    sTestProfileStringInfo_2
};

static struct Support::ProfileStringContext    sTestProfileStringContext_3_Alias  = {
    NULL,
    sTestProfileStringInfo_3
};

static const char * TestMessageStrFunct(uint32_t inProfileId, uint8_t inMsgType)
{
    return (NULL);
}

static const char * TestProfileStrFunct(uint32_t inProfileId)
{
    return (NULL);
}

static const char * TestStatusReportStrFunct(uint32_t inProfileId, uint16_t inStatusCode)
{
    return (NULL);
}

struct ProfileStringContextExtent
{
    Support::ProfileStringContext ** mObjects;
    size_t                           mCount;
};

struct ProfileStringRegisterParams
{
    ProfileStringContextExtent      mContexts;
    ProfileStringContextExtent      mAliases;
};

/**
 *  Test that unregister fails as expected with an empty registry.
 *
 */
static void CheckUnregisterEmpty(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR status;

    // Try the three actual profiles

    status = Support::UnregisterProfileStringInfo(sTestProfileStringContext_1);
    NL_TEST_ASSERT(inSuite, status == WEAVE_ERROR_PROFILE_STRING_CONTEXT_NOT_REGISTERED);

    status = Support::UnregisterProfileStringInfo(sTestProfileStringContext_2);
    NL_TEST_ASSERT(inSuite, status == WEAVE_ERROR_PROFILE_STRING_CONTEXT_NOT_REGISTERED);

    status = Support::UnregisterProfileStringInfo(sTestProfileStringContext_3);
    NL_TEST_ASSERT(inSuite, status == WEAVE_ERROR_PROFILE_STRING_CONTEXT_NOT_REGISTERED);

    // Try the three profile aliases

    status = Support::UnregisterProfileStringInfo(sTestProfileStringContext_1_Alias);
    NL_TEST_ASSERT(inSuite, status == WEAVE_ERROR_PROFILE_STRING_CONTEXT_NOT_REGISTERED);

    status = Support::UnregisterProfileStringInfo(sTestProfileStringContext_2_Alias);
    NL_TEST_ASSERT(inSuite, status == WEAVE_ERROR_PROFILE_STRING_CONTEXT_NOT_REGISTERED);

    status = Support::UnregisterProfileStringInfo(sTestProfileStringContext_3_Alias);
    NL_TEST_ASSERT(inSuite, status == WEAVE_ERROR_PROFILE_STRING_CONTEXT_NOT_REGISTERED);
}

/**
 *  Test that find fails as expected on an empty registry.
 *
 */
static void CheckFindEmpty(nlTestSuite *inSuite, void *inContext)
{
    const Support::ProfileStringInfo *result;

    // Try the three actual profiles

    result = Support::FindProfileStringInfo(kWeaveProfile_TestProfileStringSupport_1);
    NL_TEST_ASSERT(inSuite, result == NULL);

    result = Support::FindProfileStringInfo(kWeaveProfile_TestProfileStringSupport_2);
    NL_TEST_ASSERT(inSuite, result == NULL);

    result = Support::FindProfileStringInfo(kWeaveProfile_TestProfileStringSupport_3);
    NL_TEST_ASSERT(inSuite, result == NULL);
}

/**
 *  Test that registration works as expected on an empty registry.
 *
 */
static void CheckRegisterEmpty(nlTestSuite *inSuite, void *inContext, ProfileStringRegisterParams &inParams)
{
    WEAVE_ERROR status;
    size_t i;

    // Register the profiles in the requested order.

    for (i = 0; i < inParams.mContexts.mCount; i++)
    {

        status = Support::RegisterProfileStringInfo(*inParams.mContexts.mObjects[i]);
        NL_TEST_ASSERT(inSuite, status == WEAVE_NO_ERROR);
    }

    // Verify that registering them again fails.

    for (i = 0; i < inParams.mContexts.mCount; i++)
    {

        status = Support::RegisterProfileStringInfo(*inParams.mContexts.mObjects[i]);
        NL_TEST_ASSERT(inSuite, status == WEAVE_ERROR_PROFILE_STRING_CONTEXT_ALREADY_REGISTERED);
    }

    // Verify that registering their aliases also fails.

    for (i = 0; i < inParams.mAliases.mCount; i++)
    {
        status = Support::RegisterProfileStringInfo(*inParams.mAliases.mObjects[i]);
        NL_TEST_ASSERT(inSuite, status == WEAVE_ERROR_PROFILE_STRING_CONTEXT_ALREADY_REGISTERED);
    }

    // Verify that ordering is profile identifier ascending order

    NL_TEST_ASSERT(inSuite, sTestProfileStringContext_1.mNext == &sTestProfileStringContext_2);
    NL_TEST_ASSERT(inSuite, sTestProfileStringContext_2.mNext == &sTestProfileStringContext_3);
    NL_TEST_ASSERT(inSuite, sTestProfileStringContext_3.mNext == NULL);
}

/**
 *  Test that registration works as expected on an empty registry with
 *  profiles in the order 1-2-3.
 *
 */
static void CheckRegisterEmpty_123(nlTestSuite *inSuite, void *inContext)
{
    Support::ProfileStringContext * contexts[3] = {
        &sTestProfileStringContext_1,
        &sTestProfileStringContext_2,
        &sTestProfileStringContext_3
    };
    Support::ProfileStringContext * aliases[3] = {
        &sTestProfileStringContext_1_Alias,
        &sTestProfileStringContext_2_Alias,
        &sTestProfileStringContext_3_Alias
    };
    ProfileStringRegisterParams params = {
        { contexts, sizeof (contexts) / sizeof (contexts[0]) },
        { aliases,  sizeof (aliases) / sizeof (aliases[0]) },
    };

    CheckRegisterEmpty(inSuite, inContext, params);
}

/**
 *  Test that registration works as expected on an empty registry with
 *  profiles in the order 1-3-2.
 *
 */
static void CheckRegisterEmpty_132(nlTestSuite *inSuite, void *inContext)
{
    Support::ProfileStringContext * contexts[3] = {
        &sTestProfileStringContext_1,
        &sTestProfileStringContext_3,
        &sTestProfileStringContext_2
    };
    Support::ProfileStringContext * aliases[3] = {
        &sTestProfileStringContext_1_Alias,
        &sTestProfileStringContext_3_Alias,
        &sTestProfileStringContext_2_Alias
    };
    ProfileStringRegisterParams params = {
        { contexts, sizeof (contexts) / sizeof (contexts[0]) },
        { aliases,  sizeof (aliases) / sizeof (aliases[0]) },
    };

    CheckRegisterEmpty(inSuite, inContext, params);
}

/**
 *  Test that registration works as expected on an empty registry with
 *  profiles in the order 2-1-3.
 *
 */
static void CheckRegisterEmpty_213(nlTestSuite *inSuite, void *inContext)
{
    Support::ProfileStringContext * contexts[3] = {
        &sTestProfileStringContext_2,
        &sTestProfileStringContext_1,
        &sTestProfileStringContext_3
    };
    Support::ProfileStringContext * aliases[3] = {
        &sTestProfileStringContext_2_Alias,
        &sTestProfileStringContext_1_Alias,
        &sTestProfileStringContext_3_Alias
    };
    ProfileStringRegisterParams params = {
        { contexts, sizeof (contexts) / sizeof (contexts[0]) },
        { aliases,  sizeof (aliases) / sizeof (aliases[0]) },
    };

    CheckRegisterEmpty(inSuite, inContext, params);
}

/**
 *  Test that registration works as expected on an empty registry with
 *  profiles in the order 2-3-1.
 *
 */
static void CheckRegisterEmpty_231(nlTestSuite *inSuite, void *inContext)
{
    Support::ProfileStringContext * contexts[3] = {
        &sTestProfileStringContext_2,
        &sTestProfileStringContext_3,
        &sTestProfileStringContext_1
    };
    Support::ProfileStringContext * aliases[3] = {
        &sTestProfileStringContext_2_Alias,
        &sTestProfileStringContext_3_Alias,
        &sTestProfileStringContext_1_Alias
    };
    ProfileStringRegisterParams params = {
        { contexts, sizeof (contexts) / sizeof (contexts[0]) },
        { aliases,  sizeof (aliases) / sizeof (aliases[0]) },
    };

    CheckRegisterEmpty(inSuite, inContext, params);
}

/**
 *  Test that registration works as expected on an empty registry with
 *  profiles in the order 3-1-2.
 *
 */
static void CheckRegisterEmpty_312(nlTestSuite *inSuite, void *inContext)
{
    Support::ProfileStringContext * contexts[3] = {
        &sTestProfileStringContext_3,
        &sTestProfileStringContext_2,
        &sTestProfileStringContext_1
    };
    Support::ProfileStringContext * aliases[3] = {
        &sTestProfileStringContext_3_Alias,
        &sTestProfileStringContext_2_Alias,
        &sTestProfileStringContext_1_Alias
    };
    ProfileStringRegisterParams params = {
        { contexts, sizeof (contexts) / sizeof (contexts[0]) },
        { aliases,  sizeof (aliases) / sizeof (aliases[0]) },
    };

    CheckRegisterEmpty(inSuite, inContext, params);
}

/**
 *  Test that registration works as expected on an empty registry with
 *  profiles in the order 3-2-1.
 *
 */
static void CheckRegisterEmpty_321(nlTestSuite *inSuite, void *inContext)
{
    Support::ProfileStringContext * contexts[3] = {
        &sTestProfileStringContext_3,
        &sTestProfileStringContext_2,
        &sTestProfileStringContext_1
    };
    Support::ProfileStringContext * aliases[3] = {
        &sTestProfileStringContext_3_Alias,
        &sTestProfileStringContext_2_Alias,
        &sTestProfileStringContext_1_Alias
    };
    ProfileStringRegisterParams params = {
        { contexts, sizeof (contexts) / sizeof (contexts[0]) },
        { aliases,  sizeof (aliases) / sizeof (aliases[0]) },
    };

    CheckRegisterEmpty(inSuite, inContext, params);
}

/**
 *  Test that, assuming a prior registration, that find and unregister
 *  work as expected.
 *
 */
static void CheckFindAndUnregister(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR status;
    const Support::ProfileStringInfo *result;

    // Verify that the three profiles that have been added are present.

    result = Support::FindProfileStringInfo(kWeaveProfile_TestProfileStringSupport_1);
    NL_TEST_ASSERT(inSuite, result != NULL);
    NL_TEST_ASSERT(inSuite, result == &sTestProfileStringInfo_1);

    result = Support::FindProfileStringInfo(kWeaveProfile_TestProfileStringSupport_2);
    NL_TEST_ASSERT(inSuite, result != NULL);
    NL_TEST_ASSERT(inSuite, result == &sTestProfileStringInfo_2);

    result = Support::FindProfileStringInfo(kWeaveProfile_TestProfileStringSupport_3);
    NL_TEST_ASSERT(inSuite, result != NULL);
    NL_TEST_ASSERT(inSuite, result == &sTestProfileStringInfo_3);

    // Remove one of the three profiles and cofirm, via find, that it is
    // gone.

    status = Support::UnregisterProfileStringInfo(sTestProfileStringContext_2);
    NL_TEST_ASSERT(inSuite, status == WEAVE_NO_ERROR);

    result = Support::FindProfileStringInfo(kWeaveProfile_TestProfileStringSupport_2);
    NL_TEST_ASSERT(inSuite, result == NULL);

    // Confirm, via unregister, that it is gone.

    status = Support::UnregisterProfileStringInfo(sTestProfileStringContext_2);
    NL_TEST_ASSERT(inSuite, status == WEAVE_ERROR_PROFILE_STRING_CONTEXT_NOT_REGISTERED);

    // Remove the second of the three profiles and cofirm, via find,
    // that it is gone.

    status = Support::UnregisterProfileStringInfo(sTestProfileStringContext_1);
    NL_TEST_ASSERT(inSuite, status == WEAVE_NO_ERROR);

    result = Support::FindProfileStringInfo(kWeaveProfile_TestProfileStringSupport_1);
    NL_TEST_ASSERT(inSuite, result == NULL);

    // Confirm, via unregister, that it is gone.

    status = Support::UnregisterProfileStringInfo(sTestProfileStringContext_1);
    NL_TEST_ASSERT(inSuite, status == WEAVE_ERROR_PROFILE_STRING_CONTEXT_NOT_REGISTERED);

    // Remove the third of the three profiles and cofirm, via find,
    // that it is gone.

    status = Support::UnregisterProfileStringInfo(sTestProfileStringContext_3);
    NL_TEST_ASSERT(inSuite, status == WEAVE_NO_ERROR);

    result = Support::FindProfileStringInfo(kWeaveProfile_TestProfileStringSupport_3);
    NL_TEST_ASSERT(inSuite, result == NULL);

    // Confirm, via unregister, that it is gone.

    status = Support::UnregisterProfileStringInfo(sTestProfileStringContext_3);
    NL_TEST_ASSERT(inSuite, status == WEAVE_ERROR_PROFILE_STRING_CONTEXT_NOT_REGISTERED);
}

/**
 *  Test that unregister works as expected when using an alias as the
 *  match target.
 *
 */
static void CheckUnregisterWithAlias(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR status;
    const Support::ProfileStringInfo *result;

    // Register the three profiles. Intentionally add (3) first and
    // then (2), to exercise sorted insert.

    status = Support::RegisterProfileStringInfo(sTestProfileStringContext_1);
    NL_TEST_ASSERT(inSuite, status == WEAVE_NO_ERROR);

    status = Support::RegisterProfileStringInfo(sTestProfileStringContext_3);
    NL_TEST_ASSERT(inSuite, status == WEAVE_NO_ERROR);

    status = Support::RegisterProfileStringInfo(sTestProfileStringContext_2);
    NL_TEST_ASSERT(inSuite, status == WEAVE_NO_ERROR);

    // Verify that the three profiles that have been added are present.

    result = Support::FindProfileStringInfo(kWeaveProfile_TestProfileStringSupport_1);
    NL_TEST_ASSERT(inSuite, result != NULL);
    NL_TEST_ASSERT(inSuite, result == &sTestProfileStringInfo_1);

    result = Support::FindProfileStringInfo(kWeaveProfile_TestProfileStringSupport_2);
    NL_TEST_ASSERT(inSuite, result != NULL);
    NL_TEST_ASSERT(inSuite, result == &sTestProfileStringInfo_2);

    result = Support::FindProfileStringInfo(kWeaveProfile_TestProfileStringSupport_3);
    NL_TEST_ASSERT(inSuite, result != NULL);
    NL_TEST_ASSERT(inSuite, result == &sTestProfileStringInfo_3);

    // Remove one of the three profiles and cofirm, via find, that it is
    // gone.

    status = Support::UnregisterProfileStringInfo(sTestProfileStringContext_2_Alias);
    NL_TEST_ASSERT(inSuite, status == WEAVE_NO_ERROR);

    result = Support::FindProfileStringInfo(kWeaveProfile_TestProfileStringSupport_2);
    NL_TEST_ASSERT(inSuite, result == NULL);

    // Confirm, via unregister, that it is gone.

    status = Support::UnregisterProfileStringInfo(sTestProfileStringContext_2_Alias);
    NL_TEST_ASSERT(inSuite, status == WEAVE_ERROR_PROFILE_STRING_CONTEXT_NOT_REGISTERED);

    // Remove the second of the three profiles and cofirm, via find,
    // that it is gone.

    status = Support::UnregisterProfileStringInfo(sTestProfileStringContext_1_Alias);
    NL_TEST_ASSERT(inSuite, status == WEAVE_NO_ERROR);

    result = Support::FindProfileStringInfo(kWeaveProfile_TestProfileStringSupport_1);
    NL_TEST_ASSERT(inSuite, result == NULL);

    // Confirm, via unregister, that it is gone.

    status = Support::UnregisterProfileStringInfo(sTestProfileStringContext_1_Alias);
    NL_TEST_ASSERT(inSuite, status == WEAVE_ERROR_PROFILE_STRING_CONTEXT_NOT_REGISTERED);

    // Remove the third of the three profiles and cofirm, via find,
    // that it is gone.

    status = Support::UnregisterProfileStringInfo(sTestProfileStringContext_3_Alias);
    NL_TEST_ASSERT(inSuite, status == WEAVE_NO_ERROR);

    result = Support::FindProfileStringInfo(kWeaveProfile_TestProfileStringSupport_3);
    NL_TEST_ASSERT(inSuite, result == NULL);

    // Confirm, via unregister, that it is gone.

    status = Support::UnregisterProfileStringInfo(sTestProfileStringContext_3_Alias);
    NL_TEST_ASSERT(inSuite, status == WEAVE_ERROR_PROFILE_STRING_CONTEXT_NOT_REGISTERED);
}

static const nlTest sTests[] = {
    NL_TEST_DEF("unregister (empty)",     CheckUnregisterEmpty),
    NL_TEST_DEF("find (empty)",           CheckFindEmpty),
    NL_TEST_DEF("register 1-2-3 (empty)", CheckRegisterEmpty_123),
    NL_TEST_DEF("find and unregister",    CheckFindAndUnregister),
    NL_TEST_DEF("register 1-3-2 (empty)", CheckRegisterEmpty_132),
    NL_TEST_DEF("find and unregister",    CheckFindAndUnregister),
    NL_TEST_DEF("register 2-1-3 (empty)", CheckRegisterEmpty_213),
    NL_TEST_DEF("find and unregister",    CheckFindAndUnregister),
    NL_TEST_DEF("register 2-3-1 (empty)", CheckRegisterEmpty_231),
    NL_TEST_DEF("find and unregister",    CheckFindAndUnregister),
    NL_TEST_DEF("register 3-1-2 (empty)", CheckRegisterEmpty_312),
    NL_TEST_DEF("find and unregister",    CheckFindAndUnregister),
    NL_TEST_DEF("register 3-2-1 (empty)", CheckRegisterEmpty_321),
    NL_TEST_DEF("find and unregister",    CheckFindAndUnregister),
    NL_TEST_DEF("unregister with alias",  CheckUnregisterWithAlias),
    NL_TEST_SENTINEL()
};

int main(void)
{
    nlTestSuite theSuite = {
        "weave-profile-string-support",
        &sTests[0]
    };

    nl_test_set_output_style(OUTPUT_CSV);

    nlTestRunner(&theSuite, NULL);

    return nlTestRunnerStats(&theSuite);
}
