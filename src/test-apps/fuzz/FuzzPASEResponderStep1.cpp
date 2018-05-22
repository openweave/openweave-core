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
 *      This file implements a process to fuzz ResponderStep1 of
 *      the Weave Password Authenticated Session Establishment (PASE)
 *      protocol engine.
 *
 */

#include <stdio.h>
#include <vector>

#include "FuzzUtils.h"
#include "PASEEngineTest.h"
#include "ToolCommon.h"
#include <Weave/Support/ErrorStr.h>
#include <Weave/Profiles/security/WeaveSecurity.h>
#include <Weave/Profiles/security/WeavePASE.h>
#include <Weave/Support/RandUtils.h>

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
#include "lwip/tcpip.h"
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

using namespace nl::Weave::Profiles::Security;
using namespace nl::Weave::Profiles::Security::PASE;
using System::PacketBuffer;

#define SuccessOrQuit(ERR, MSG) \
do { \
    if ((ERR) != WEAVE_NO_ERROR) \
    { \
        fprintf(stderr, "%s FAILED: ", __FUNCTION__); \
        fputs(MSG, stderr); \
        fputs(ErrorStr(ERR), stderr); \
        fputs("\n", stderr); \
        exit(-1); \
    } \
} while (0)

#define SuccessOrFinish(ERR) \
do { \
    if ((ERR) != WEAVE_NO_ERROR) \
    { \
        goto cleanUp; \
    } \
} while (0)

#ifdef WEAVE_FUZZING_ENABLED
extern "C"
{
    int RAND_bytes(unsigned char *buf, int num)
    {
        memset(buf, 'A', num);
        return 1;
    }
}
#endif // WEAVE_FUZZING_ENABLED

static const char testName[] = "Message Substitution Fuzzing";
static const char testPassword[] = "TestPassword";

//Assume the user compiled with clang > 6.0
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size > 0) {
        MessageExternalFuzzer fuzzer = MessageExternalFuzzer(RESPONDER_STEP_1)
            .FuzzInput(data, size);

        PASEEngineTest(testName)
                .Mutator(&fuzzer)
                .InitiatorPassword(testPassword)
                .ResponderPassword(testPassword)
                .ProposedConfig(kPASEConfig_Config1)
                .ResponderAllowedConfigs(kPASEConfig_Config1)
                .ConfirmKey(true)
                .LogMessageData(false)
                .ExpectError(RESPONDER_STEP_1, WEAVE_ERROR_INVALID_PASE_PARAMETER)
                .ExpectError(RESPONDER_STEP_1, WEAVE_ERROR_INVALID_MESSAGE_LENGTH)
                .ExpectError(RESPONDER_STEP_1, WEAVE_ERROR_INVALID_ARGUMENT)
                .ExpectError(RESPONDER_STEP_1, WEAVE_ERROR_MESSAGE_INCOMPLETE)
                .ExpectError(RESPONDER_STEP_1, WEAVE_ERROR_NO_MEMORY)
                .Run();
    }
    return 0;
}

// When NOT building for fuzzing using libFuzzer, supply a main() function to satisfy
// the linker.  Even though the resultant application does nothing, being able to link
// it confirms that the fuzzing tests can be built successfully.
#ifndef WEAVE_FUZZING_ENABLED
int main(int argc, char *argv[])
{
    return 0;
}
#endif
