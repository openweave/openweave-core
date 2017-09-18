/*
 *
 *    Copyright (c) 2015-2017 Nest Labs, Inc.
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
 *      code utilities templates and macros.
 *
 */

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#include <stdint.h>

#include <nltest.h>

static void TestWeaveDie(void);

#define WeaveDie() TestWeaveDie()

#include <Weave/Support/CodeUtils.h>

struct TestVerifyOrDieContext
{
    bool mShouldDie;
    bool mDidDie;
};

static struct TestVerifyOrDieContext sContext;

static void CheckMin(nlTestSuite *inSuite, void *inContext)
{
    int8_t   s8result;
    int16_t  s16result;
    int32_t  s32result;
    int64_t  s64result;
    uint8_t  u8result;
    uint16_t u16result;
    uint32_t u32result;
    uint64_t u64result;

    // Test 8-bit signed integers

    s8result = nl::Weave::min(INT8_MIN, INT8_MAX);
    NL_TEST_ASSERT(inSuite, s8result == INT8_MIN);

    s8result = nl::Weave::min(INT8_MAX, INT8_MIN);
    NL_TEST_ASSERT(inSuite, s8result == INT8_MIN);

    s8result = nl::Weave::min(INT8_MIN, 0);
    NL_TEST_ASSERT(inSuite, s8result == INT8_MIN);

    s8result = nl::Weave::min(0, INT8_MIN);
    NL_TEST_ASSERT(inSuite, s8result == INT8_MIN);

    s8result = nl::Weave::min(0, INT8_MAX);
    NL_TEST_ASSERT(inSuite, s8result == 0);

    s8result = nl::Weave::min(INT8_MAX, 0);
    NL_TEST_ASSERT(inSuite, s8result == 0);

    s8result = nl::Weave::min(0, 0);
    NL_TEST_ASSERT(inSuite, s8result == 0);

    s8result = nl::Weave::min(INT8_MIN, INT8_MIN);
    NL_TEST_ASSERT(inSuite, s8result == INT8_MIN);

    s8result = nl::Weave::min(INT8_MAX, INT8_MAX);
    NL_TEST_ASSERT(inSuite, s8result == INT8_MAX);

    // Test 16-bit signed integers

    s16result = nl::Weave::min(INT16_MIN, INT16_MAX);
    NL_TEST_ASSERT(inSuite, s16result == INT16_MIN);

    s16result = nl::Weave::min(INT16_MAX, INT16_MIN);
    NL_TEST_ASSERT(inSuite, s16result == INT16_MIN);

    s16result = nl::Weave::min(INT16_MIN, 0);
    NL_TEST_ASSERT(inSuite, s16result == INT16_MIN);

    s16result = nl::Weave::min(0, INT16_MIN);
    NL_TEST_ASSERT(inSuite, s16result == INT16_MIN);

    s16result = nl::Weave::min(0, INT16_MAX);
    NL_TEST_ASSERT(inSuite, s16result == 0);

    s16result = nl::Weave::min(INT16_MAX, 0);
    NL_TEST_ASSERT(inSuite, s16result == 0);

    s16result = nl::Weave::min(0, 0);
    NL_TEST_ASSERT(inSuite, s16result == 0);

    s16result = nl::Weave::min(INT16_MIN, INT16_MIN);
    NL_TEST_ASSERT(inSuite, s16result == INT16_MIN);

    s16result = nl::Weave::min(INT16_MAX, INT16_MAX);
    NL_TEST_ASSERT(inSuite, s16result == INT16_MAX);

    // Test 32-bit signed integers

    s32result = nl::Weave::min(INT32_MIN, INT32_MAX);
    NL_TEST_ASSERT(inSuite, s32result == INT32_MIN);

    s32result = nl::Weave::min(INT32_MAX, INT32_MIN);
    NL_TEST_ASSERT(inSuite, s32result == INT32_MIN);

    s32result = nl::Weave::min(INT32_MIN, 0);
    NL_TEST_ASSERT(inSuite, s32result == INT32_MIN);

    s32result = nl::Weave::min(0, INT32_MIN);
    NL_TEST_ASSERT(inSuite, s32result == INT32_MIN);

    s32result = nl::Weave::min(0, INT32_MAX);
    NL_TEST_ASSERT(inSuite, s32result == 0);

    s32result = nl::Weave::min(INT32_MAX, 0);
    NL_TEST_ASSERT(inSuite, s32result == 0);

    s32result = nl::Weave::min(0, 0);
    NL_TEST_ASSERT(inSuite, s32result == 0);

    s32result = nl::Weave::min(INT32_MIN, INT32_MIN);
    NL_TEST_ASSERT(inSuite, s32result == INT32_MIN);

    s32result = nl::Weave::min(INT32_MAX, INT32_MAX);
    NL_TEST_ASSERT(inSuite, s32result == INT32_MAX);

    // Test 64-bit signed integers

    s64result = nl::Weave::min(INT64_MIN, INT64_MAX);
    NL_TEST_ASSERT(inSuite, s64result == INT64_MIN);

    s64result = nl::Weave::min(INT64_MAX, INT64_MIN);
    NL_TEST_ASSERT(inSuite, s64result == INT64_MIN);

    s64result = nl::Weave::min(INT64_MIN, static_cast<int64_t>(0));
    NL_TEST_ASSERT(inSuite, s64result == INT64_MIN);

    s64result = nl::Weave::min(static_cast<int64_t>(0), INT64_MIN);
    NL_TEST_ASSERT(inSuite, s64result == INT64_MIN);

    s64result = nl::Weave::min(static_cast<int64_t>(0), INT64_MAX);
    NL_TEST_ASSERT(inSuite, s64result == static_cast<int64_t>(0));

    s64result = nl::Weave::min(INT64_MAX, static_cast<int64_t>(0));
    NL_TEST_ASSERT(inSuite, s64result == static_cast<int64_t>(0));

    s64result = nl::Weave::min(static_cast<int64_t>(0), static_cast<int64_t>(0));
    NL_TEST_ASSERT(inSuite, s64result == static_cast<int64_t>(0));

    s64result = nl::Weave::min(INT64_MIN, INT64_MIN);
    NL_TEST_ASSERT(inSuite, s64result == INT64_MIN);

    s64result = nl::Weave::min(INT64_MAX, INT64_MAX);
    NL_TEST_ASSERT(inSuite, s64result == INT64_MAX);

    // Test 8-bit unsigned integers

    u8result = nl::Weave::min(UINT8_MAX, UINT8_MAX);
    NL_TEST_ASSERT(inSuite, u8result == UINT8_MAX);

    u8result = nl::Weave::min(0, UINT8_MAX);
    NL_TEST_ASSERT(inSuite, u8result == 0);

    u8result = nl::Weave::min(UINT8_MAX, 0);
    NL_TEST_ASSERT(inSuite, u8result == 0);

    u8result = nl::Weave::min(0, 0);
    NL_TEST_ASSERT(inSuite, u8result == 0);

    // Test 16-bit unsigned integers

    u16result = nl::Weave::min(UINT16_MAX, UINT16_MAX);
    NL_TEST_ASSERT(inSuite, u16result == UINT16_MAX);

    u16result = nl::Weave::min(0, UINT16_MAX);
    NL_TEST_ASSERT(inSuite, u16result == 0);

    u16result = nl::Weave::min(UINT16_MAX, 0);
    NL_TEST_ASSERT(inSuite, u16result == 0);

    u16result = nl::Weave::min(0, 0);
    NL_TEST_ASSERT(inSuite, u16result == 0);

    // Test 32-bit unsigned integers

    u32result = nl::Weave::min(UINT32_MAX, UINT32_MAX);
    NL_TEST_ASSERT(inSuite, u32result == UINT32_MAX);

    u32result = nl::Weave::min(static_cast<uint32_t>(0), UINT32_MAX);
    NL_TEST_ASSERT(inSuite, u32result == static_cast<uint32_t>(0));

    u32result = nl::Weave::min(UINT32_MAX, static_cast<uint32_t>(0));
    NL_TEST_ASSERT(inSuite, u32result == static_cast<uint32_t>(0));

    u32result = nl::Weave::min(static_cast<uint32_t>(0), static_cast<uint32_t>(0));
    NL_TEST_ASSERT(inSuite, u32result == static_cast<uint32_t>(0));

    // Test 64-bit unsigned integers

    u64result = nl::Weave::min(UINT64_MAX, UINT64_MAX);
    NL_TEST_ASSERT(inSuite, u64result == UINT64_MAX);

    u64result = nl::Weave::min(static_cast<uint64_t>(0), UINT64_MAX);
    NL_TEST_ASSERT(inSuite, u64result == static_cast<uint64_t>(0));

    u64result = nl::Weave::min(UINT64_MAX, static_cast<uint64_t>(0));
    NL_TEST_ASSERT(inSuite, u64result == static_cast<uint64_t>(0));

    u64result = nl::Weave::min(static_cast<uint64_t>(0), static_cast<uint64_t>(0));
    NL_TEST_ASSERT(inSuite, u64result == static_cast<uint64_t>(0));
}

static void CheckMax(nlTestSuite *inSuite, void *inContext)
{
    int8_t   s8result;
    int16_t  s16result;
    int32_t  s32result;
    int64_t  s64result;
    uint8_t  u8result;
    uint16_t u16result;
    uint32_t u32result;
    uint64_t u64result;

    // Test 8-bit signed integers

    s8result = nl::Weave::max(INT8_MIN, INT8_MAX);
    NL_TEST_ASSERT(inSuite, s8result == INT8_MAX);

    s8result = nl::Weave::max(INT8_MAX, INT8_MIN);
    NL_TEST_ASSERT(inSuite, s8result == INT8_MAX);

    s8result = nl::Weave::max(INT8_MIN, 0);
    NL_TEST_ASSERT(inSuite, s8result == 0);

    s8result = nl::Weave::max(0, INT8_MIN);
    NL_TEST_ASSERT(inSuite, s8result == 0);

    s8result = nl::Weave::max(0, INT8_MAX);
    NL_TEST_ASSERT(inSuite, s8result == INT8_MAX);

    s8result = nl::Weave::max(INT8_MAX, 0);
    NL_TEST_ASSERT(inSuite, s8result == INT8_MAX);

    s8result = nl::Weave::max(0, 0);
    NL_TEST_ASSERT(inSuite, s8result == 0);

    s8result = nl::Weave::max(INT8_MIN, INT8_MIN);
    NL_TEST_ASSERT(inSuite, s8result == INT8_MIN);

    s8result = nl::Weave::max(INT8_MAX, INT8_MAX);
    NL_TEST_ASSERT(inSuite, s8result == INT8_MAX);

    // Test 16-bit signed integers

    s16result = nl::Weave::max(INT16_MIN, INT16_MAX);
    NL_TEST_ASSERT(inSuite, s16result == INT16_MAX);

    s16result = nl::Weave::max(INT16_MAX, INT16_MIN);
    NL_TEST_ASSERT(inSuite, s16result == INT16_MAX);

    s16result = nl::Weave::max(INT16_MIN, 0);
    NL_TEST_ASSERT(inSuite, s16result == 0);

    s16result = nl::Weave::max(0, INT16_MIN);
    NL_TEST_ASSERT(inSuite, s16result == 0);

    s16result = nl::Weave::max(0, INT16_MAX);
    NL_TEST_ASSERT(inSuite, s16result == INT16_MAX);

    s16result = nl::Weave::max(INT16_MAX, 0);
    NL_TEST_ASSERT(inSuite, s16result == INT16_MAX);

    s16result = nl::Weave::max(0, 0);
    NL_TEST_ASSERT(inSuite, s16result == 0);

    s16result = nl::Weave::max(INT16_MIN, INT16_MIN);
    NL_TEST_ASSERT(inSuite, s16result == INT16_MIN);

    s16result = nl::Weave::max(INT16_MAX, INT16_MAX);
    NL_TEST_ASSERT(inSuite, s16result == INT16_MAX);

    // Test 32-bit signed integers

    s32result = nl::Weave::max(INT32_MIN, INT32_MAX);
    NL_TEST_ASSERT(inSuite, s32result == INT32_MAX);

    s32result = nl::Weave::max(INT32_MAX, INT32_MIN);
    NL_TEST_ASSERT(inSuite, s32result == INT32_MAX);

    s32result = nl::Weave::max(INT32_MIN, 0);
    NL_TEST_ASSERT(inSuite, s32result == 0);

    s32result = nl::Weave::max(0, INT32_MIN);
    NL_TEST_ASSERT(inSuite, s32result == 0);

    s32result = nl::Weave::max(0, INT32_MAX);
    NL_TEST_ASSERT(inSuite, s32result == INT32_MAX);

    s32result = nl::Weave::max(INT32_MAX, 0);
    NL_TEST_ASSERT(inSuite, s32result == INT32_MAX);

    s32result = nl::Weave::max(0, 0);
    NL_TEST_ASSERT(inSuite, s32result == 0);

    s32result = nl::Weave::max(INT32_MIN, INT32_MIN);
    NL_TEST_ASSERT(inSuite, s32result == INT32_MIN);

    s32result = nl::Weave::max(INT32_MAX, INT32_MAX);
    NL_TEST_ASSERT(inSuite, s32result == INT32_MAX);

    // Test 64-bit signed integers

    s64result = nl::Weave::max(INT64_MIN, INT64_MAX);
    NL_TEST_ASSERT(inSuite, s64result == INT64_MAX);

    s64result = nl::Weave::max(INT64_MAX, INT64_MIN);
    NL_TEST_ASSERT(inSuite, s64result == INT64_MAX);

    s64result = nl::Weave::max(INT64_MIN, static_cast<int64_t>(0));
    NL_TEST_ASSERT(inSuite, s64result == static_cast<int64_t>(0));

    s64result = nl::Weave::max(static_cast<int64_t>(0), INT64_MIN);
    NL_TEST_ASSERT(inSuite, s64result == static_cast<int64_t>(0));

    s64result = nl::Weave::max(static_cast<int64_t>(0), INT64_MAX);
    NL_TEST_ASSERT(inSuite, s64result == INT64_MAX);

    s64result = nl::Weave::max(INT64_MAX, static_cast<int64_t>(0));
    NL_TEST_ASSERT(inSuite, s64result == INT64_MAX);

    s64result = nl::Weave::max(static_cast<int64_t>(0), static_cast<int64_t>(0));
    NL_TEST_ASSERT(inSuite, s64result == static_cast<int64_t>(0));

    s64result = nl::Weave::max(INT64_MIN, INT64_MIN);
    NL_TEST_ASSERT(inSuite, s64result == INT64_MIN);

    s64result = nl::Weave::max(INT64_MAX, INT64_MAX);
    NL_TEST_ASSERT(inSuite, s64result == INT64_MAX);

    // Test 8-bit unsigned integers

    u8result = nl::Weave::max(UINT8_MAX, UINT8_MAX);
    NL_TEST_ASSERT(inSuite, u8result == UINT8_MAX);

    u8result = nl::Weave::max(0, UINT8_MAX);
    NL_TEST_ASSERT(inSuite, u8result == UINT8_MAX);

    u8result = nl::Weave::max(UINT8_MAX, 0);
    NL_TEST_ASSERT(inSuite, u8result == UINT8_MAX);

    u8result = nl::Weave::max(0, 0);
    NL_TEST_ASSERT(inSuite, u8result == 0);

    // Test 16-bit unsigned integers

    u16result = nl::Weave::max(UINT16_MAX, UINT16_MAX);
    NL_TEST_ASSERT(inSuite, u16result == UINT16_MAX);

    u16result = nl::Weave::max(0, UINT16_MAX);
    NL_TEST_ASSERT(inSuite, u16result == UINT16_MAX);

    u16result = nl::Weave::max(UINT16_MAX, 0);
    NL_TEST_ASSERT(inSuite, u16result == UINT16_MAX);

    u16result = nl::Weave::max(0, 0);
    NL_TEST_ASSERT(inSuite, u16result == 0);

    // Test 32-bit unsigned integers

    u32result = nl::Weave::max(UINT32_MAX, UINT32_MAX);
    NL_TEST_ASSERT(inSuite, u32result == UINT32_MAX);

    u32result = nl::Weave::max(static_cast<uint32_t>(0), UINT32_MAX);
    NL_TEST_ASSERT(inSuite, u32result == UINT32_MAX);

    u32result = nl::Weave::max(UINT32_MAX, static_cast<uint32_t>(0));
    NL_TEST_ASSERT(inSuite, u32result == UINT32_MAX);

    u32result = nl::Weave::max(static_cast<uint32_t>(0), static_cast<uint32_t>(0));
    NL_TEST_ASSERT(inSuite, u32result == static_cast<uint32_t>(0));

    // Test 64-bit unsigned integers

    u64result = nl::Weave::max(UINT64_MAX, UINT64_MAX);
    NL_TEST_ASSERT(inSuite, u64result == UINT64_MAX);

    u64result = nl::Weave::max(static_cast<uint64_t>(0), UINT64_MAX);
    NL_TEST_ASSERT(inSuite, u64result == UINT64_MAX);

    u64result = nl::Weave::max(UINT64_MAX, static_cast<uint64_t>(0));
    NL_TEST_ASSERT(inSuite, u64result == UINT64_MAX);

    u64result = nl::Weave::max(static_cast<uint64_t>(0), static_cast<uint64_t>(0));
    NL_TEST_ASSERT(inSuite, u64result == static_cast<uint64_t>(0));
}

static WEAVE_ERROR GetStatus(bool inFailure)
{
    return ((inFailure) ? -1 : WEAVE_NO_ERROR);
}

static void CheckSuccessOrExitFailure(nlTestSuite *inSuite, void *inContext)
{
    const bool failure = true;
    int        status;
    bool       sentinel;

    status = GetStatus(failure);
    sentinel = false;

    SuccessOrExit(status);
    sentinel = true;

 exit:
    NL_TEST_ASSERT(inSuite, sentinel == false);
}

static void CheckSuccessOrExitSuccess(nlTestSuite *inSuite, void *inContext)
{
    const bool failure = true;
    int        status;
    bool       sentinel;

    status = GetStatus(!failure);
    sentinel = false;

    SuccessOrExit(status);
    sentinel = true;

 exit:
    NL_TEST_ASSERT(inSuite, sentinel == true);
}

static void CheckSuccessOrExitAssignmentFailure(nlTestSuite *inSuite, void *inContext)
{
    const bool failure = true;
    int        status;
    bool       sentinel;

    sentinel = false;

    SuccessOrExit(status = GetStatus(failure));
    sentinel = true;

 exit:
    NL_TEST_ASSERT(inSuite, sentinel == false);
}

static void CheckSuccessOrExitAssignmentSuccess(nlTestSuite *inSuite, void *inContext)
{
    const bool failure = true;
    int        status;
    bool       sentinel;

    sentinel = false;

    SuccessOrExit(status = GetStatus(!failure));
    sentinel = true;

 exit:
    NL_TEST_ASSERT(inSuite, sentinel == true);
}

static void CheckSuccessOrExit(nlTestSuite *inSuite, void *inContext)
{
    CheckSuccessOrExitFailure(inSuite, inContext);
    CheckSuccessOrExitSuccess(inSuite, inContext);
    CheckSuccessOrExitAssignmentFailure(inSuite, inContext);
    CheckSuccessOrExitAssignmentSuccess(inSuite, inContext);
}

static void CheckVerifyOrExitFailure(nlTestSuite *inSuite, void *inContext)
{
    bool status;
    bool sentinel;
    bool action;

    status = false;
    sentinel = false;
    action = false;

    VerifyOrExit(status, action = true);
    sentinel = true;

 exit:
    NL_TEST_ASSERT(inSuite, sentinel == false);
    NL_TEST_ASSERT(inSuite, action == true);
}

static void CheckVerifyOrExitSuccess(nlTestSuite *inSuite, void *inContext)
{
    bool status;
    bool sentinel;
    bool action;

    status = true;
    sentinel = false;
    action = false;

    VerifyOrExit(status, action = true);
    sentinel = true;

 exit:
    NL_TEST_ASSERT(inSuite, sentinel == true);
    NL_TEST_ASSERT(inSuite, action == false);
}

static void CheckVerifyOrExit(nlTestSuite *inSuite, void *inContext)
{
    CheckVerifyOrExitFailure(inSuite, inContext);
    CheckVerifyOrExitSuccess(inSuite, inContext);
}

static void CheckExitNow(nlTestSuite *inSuite, void *inContext)
{
    bool action;
    bool sentinel;

    sentinel = false;
    action = false;

    ExitNow(action = true);
    sentinel = true;

 exit:
    NL_TEST_ASSERT(inSuite, sentinel == false);
    NL_TEST_ASSERT(inSuite, action == true);
}

static void TestWeaveDie(void)
{
    sContext.mDidDie = true;
}

static void CheckVerifyOrDieFailure(nlTestSuite *inSuite, void *inContext)
{
    struct TestVerifyOrDieContext *theContext = static_cast<struct TestVerifyOrDieContext *>(inContext);
    bool status;

    status = false;

    theContext->mShouldDie = true;
    theContext->mDidDie = false;

    VerifyOrDie(status);

    NL_TEST_ASSERT(inSuite, theContext->mShouldDie == theContext->mDidDie);
}

static void CheckVerifyOrDieSuccess(nlTestSuite *inSuite, void *inContext)
{
    struct TestVerifyOrDieContext *theContext = static_cast<struct TestVerifyOrDieContext *>(inContext);
    bool status;

    status = true;

    theContext->mShouldDie = false;
    theContext->mDidDie = false;

    VerifyOrDie(status);

    NL_TEST_ASSERT(inSuite, theContext->mShouldDie == theContext->mDidDie);
}

static void CheckVerifyOrDie(nlTestSuite *inSuite, void *inContext)
{
    CheckVerifyOrDieFailure(inSuite, inContext);
    CheckVerifyOrDieSuccess(inSuite, inContext);
}

static void CheckLogFileLineFunc(nlTestSuite *inSuite, void *inContext)
{

}

static const nlTest sTests[] = {
    NL_TEST_DEF("min",                CheckMin),
    NL_TEST_DEF("max",                CheckMax),
    NL_TEST_DEF("success-or-exit",    CheckSuccessOrExit),
    NL_TEST_DEF("verify-or-exit",     CheckVerifyOrExit),
    NL_TEST_DEF("exit-now",           CheckExitNow),
    NL_TEST_DEF("verify-or-die",      CheckVerifyOrDie),
    NL_TEST_DEF("log-file-line-func", CheckLogFileLineFunc),
    NL_TEST_SENTINEL()
};

int main(void)
{
    nlTestSuite theSuite = {
        "weave-code-utils",
        &sTests[0]
    };

    nl_test_set_output_style(OUTPUT_CSV);

    nlTestRunner(&theSuite, &sContext);

    return nlTestRunnerStats(&theSuite);
}
