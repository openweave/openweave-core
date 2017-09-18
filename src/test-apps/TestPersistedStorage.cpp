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
 *      Unit tests for the Weave Persisted Storage API.
 *
 */

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif
#define WEAVE_CONFIG_BDX_NAMESPACE kWeaveManagedNamespace_Development

#include <new>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <nltest.h>

#include <Weave/Support/platform/PersistedStorage.h>

#include "ToolCommon.h"

#include "TestPersistedStorageImplementation.h"

#define TOOL_NAME "TestPersistedStorage"

struct TestPersistedStorageContext
{
    TestPersistedStorageContext();
    bool mVerbose;
};

TestPersistedStorageContext::TestPersistedStorageContext() :
    mVerbose(false)
{
}

static void InitializePersistedStorage(TestPersistedStorageContext *context)
{
    sPersistentStore.clear();
}

static int TestSetup(void *inContext)
{
    return SUCCESS;
}

static int TestTeardown(void *inContext)
{
    sPersistentStore.clear();
    return SUCCESS;
}

static void CheckWriteEmptyKey(nlTestSuite *inSuite, void *inContext)
{
    TestPersistedStorageContext *context = static_cast<TestPersistedStorageContext *>(inContext);
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint32_t testValueWrite = 0x44445555;

    InitializePersistedStorage(context);

    // Test writing out a value with an empty key.

    err = nl::Weave::Platform::PersistedStorage::Write(NULL, testValueWrite);
    NL_TEST_ASSERT(inSuite, err == WEAVE_ERROR_INVALID_ARGUMENT);
}

static void CheckReadEmptyKey(nlTestSuite *inSuite, void *inContext)
{
    TestPersistedStorageContext *context = static_cast<TestPersistedStorageContext *>(inContext);
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint32_t testValueRead;

    InitializePersistedStorage(context);

    // Test reading in a value with an empty key.

    err = nl::Weave::Platform::PersistedStorage::Read(NULL, testValueRead);
    NL_TEST_ASSERT(inSuite, err == WEAVE_ERROR_INVALID_ARGUMENT);
}

static void CheckWriteNonexistentKey(nlTestSuite *inSuite, void *inContext)
{
    TestPersistedStorageContext *context = static_cast<TestPersistedStorageContext *>(inContext);
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint32_t testValueWrite = 0x12345678;
    char testKeyWrite[] = "nonexistentkey1";

    InitializePersistedStorage(context);

    // Test writing a value with a key not present.

    err = nl::Weave::Platform::PersistedStorage::Write(testKeyWrite, testValueWrite);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
}

static void CheckReadNonexistentKey(nlTestSuite *inSuite, void *inContext)
{
    TestPersistedStorageContext *context = static_cast<TestPersistedStorageContext *>(inContext);
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint32_t testValueRead;
    char testKeyRead[] = "nonexistentkey2";

    InitializePersistedStorage(context);

    // Test reading in a value with a key not present.

    err = nl::Weave::Platform::PersistedStorage::Read(testKeyRead, testValueRead);
    NL_TEST_ASSERT(inSuite, err == WEAVE_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND);
}

static void CheckWriteRead(nlTestSuite *inSuite, void *inContext)
{
    TestPersistedStorageContext *context = static_cast<TestPersistedStorageContext *>(inContext);
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    const char *testKey = "stinkbag";
    uint32_t testValueWrite = 0xBAADCAFE;
    uint32_t testValueRead;

    InitializePersistedStorage(context);

    // Test writing one value and then reading it out.

    err = nl::Weave::Platform::PersistedStorage::Write(testKey, testValueWrite);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    err = nl::Weave::Platform::PersistedStorage::Read(testKey, testValueRead);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    NL_TEST_ASSERT(inSuite, testValueRead == testValueWrite);
}

// Test Suite

/**
 *  Test Suite that lists all the test functions.
 */
static const nlTest sTests[] = {
    NL_TEST_DEF("Write Empty Key Test", CheckWriteEmptyKey),
    NL_TEST_DEF("Read Empty Key Test", CheckReadEmptyKey),
    NL_TEST_DEF("Write Nonexistent Key Test", CheckWriteNonexistentKey),
    NL_TEST_DEF("Read Nonexistent Key Test", CheckReadNonexistentKey),
    NL_TEST_DEF("Simple Write Read Test", CheckWriteRead),

    // Add cases for:
    //
    // Write with a key or value too long.
    // Read with a NULL value.
    // Write with a NULL value.

    NL_TEST_SENTINEL()
};

static HelpOptions gHelpOptions(
    TOOL_NAME,
    "Usage: " TOOL_NAME " [<options...>]\n",
    WEAVE_VERSION_STRING "\n" WEAVE_TOOL_COPYRIGHT,
    "Test persisted storage API.  Without any options, the program invokes a suite of local tests.\n"
);

static OptionSet *gOptionSets[] =
{
    &gHelpOptions,
    NULL
};

int main(int argc, char *argv[])
{
    TestPersistedStorageContext context;

    if (!ParseArgsFromEnvVar(TOOL_NAME, TOOL_OPTIONS_ENV_VAR_NAME, gOptionSets, NULL, true) ||
        !ParseArgs(TOOL_NAME, argc, argv, gOptionSets))
    {
        exit(EXIT_FAILURE);
    }

    nlTestSuite theSuite = {
        "weave-persisted-storage",
        &sTests[0],
        TestSetup,
        TestTeardown
    };

    // Generate machine-readable, comma-separated value (CSV) output.
    nl_test_set_output_style(OUTPUT_CSV);

    // Run test suit against one context
    nlTestRunner(&theSuite, &context);

    return nlTestRunnerStats(&theSuite);
}
