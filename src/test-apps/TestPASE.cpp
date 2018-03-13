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
 *      This file implements a process to effect a functional test for
 *      the Weave Password Authenticated Session Establishment (PASE)
 *      protocol engine.
 *
 */

#include <stdio.h>
#include <vector>

#include "ToolCommon.h"
#include "PASEEngineTest.h"
#include <Weave/Support/ErrorStr.h>
#include <Weave/Support/RandUtils.h>

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
#include "lwip/tcpip.h"
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

using namespace nl::Weave::Profiles::Security;
using namespace nl::Weave::Profiles::Security::PASE;
using System::PacketBuffer;

void PASEEngineTests_BasicTests()
{
    //Fails
    PASEEngineTest("Sanity")
            .Run();
}

void PASEEngine_ConfigTest1()
{
    PASEEngineTest("Config 1 Confirm Key")
            .InitiatorPassword("TestPassword")
            .ResponderPassword("TestPassword")
            .ProposedConfig(kPASEConfig_Config1)
            .ResponderAllowedConfigs(kPASEConfig_Config1)
            .ConfirmKey(true)
            .Run();

    PASEEngineTest("Config 1 No Confirm Key")
            .InitiatorPassword("TestPassword")
            .ResponderPassword("TestPassword")
            .ProposedConfig(kPASEConfig_Config1)
            .ResponderAllowedConfigs(kPASEConfig_Config1)
            .ConfirmKey(false)
            .Run();

    PASEEngineTest("Config 1 Test Bad Password")
             .InitiatorPassword("TestPassword")
             .ResponderPassword("TestwordPass")
             .ProposedConfig(kPASEConfig_Config1)
             .ResponderAllowedConfigs(kPASEConfig_Config1)
             .ConfirmKey(true)
             .Run();
}

void PASEEngine_ConfigTest4()
{
    PASEEngineTest("Config 4 Confirm Key")
            .InitiatorPassword("TestPassword")
            .ResponderPassword("TestPassword")
            .ProposedConfig(kPASEConfig_Config4)
            .ResponderAllowedConfigs(kPASEConfig_Config4)
            .ConfirmKey(true)
            .Run();

    PASEEngineTest("Config 4 No Confirm Key")
            .InitiatorPassword("TestPassword")
            .ResponderPassword("TestPassword")
            .ProposedConfig(kPASEConfig_Config4)
            .ResponderAllowedConfigs(kPASEConfig_Config4)
            .ConfirmKey(false)
            .Run();

    PASEEngineTest("Config 4 Test Bad Password")
             .InitiatorPassword("TestPassword")
             .ResponderPassword("TestwordPass")
             .ProposedConfig(kPASEConfig_Config4)
             .ResponderAllowedConfigs(kPASEConfig_Config4)
             .ConfirmKey(true)
             .Run();
}

void PASEEngineTest_MixedConfigs()
{
    PASEEngineTest("Different Configs 1/4")
            .InitiatorPassword("TestPassword")
            .ResponderPassword("TestPassword")
            .ProposedConfig(kPASEConfig_Config1)
            .ResponderAllowedConfigs(kPASEConfig_Config4)
            .ExpectReconfig(kPASEConfig_Config4)
            .ConfirmKey(true)
            .LogMessageData(false)
            .Run();

    PASEEngineTest("Different Config Force Reconfig 1/4")
            .InitiatorPassword("TestPassword")
            .ResponderPassword("TestPassword")
            .ProposedConfig(kPASEConfig_Config1)
            .ResponderAllowedConfigs(kPASEConfig_Config4)
            .ExpectReconfig(kPASEConfig_Config4)
            .ConfirmKey(true)
            .LogMessageData(false)
            .Run();

    PASEEngineTest("Different Configs 1/4")
            .InitiatorPassword("TestPassword")
            .ResponderPassword("TestPassword")
            .ProposedConfig(kPASEConfig_Config1)
            .ResponderAllowedConfigs(kPASEConfig_Config4)
            .ExpectReconfig(kPASEConfig_Config4)
            .ConfirmKey(true)
            .LogMessageData(false)
            .Run();

    PASEEngineTest("Different Config Force Reconfig 1/4")
            .InitiatorPassword("TestPassword")
            .ResponderPassword("TestPassword")
            .ProposedConfig(kPASEConfig_Config1)
            .ResponderAllowedConfigs(kPASEConfig_Config4)
            .ExpectReconfig(kPASEConfig_Config4)
            .ConfirmKey(true)
            .LogMessageData(false)
            .Run();
}

void PASEEngine_ExternalFuzzingEngine(const char *fuzzLocation, const uint8_t *fuzzInput, size_t fuzzInputSize)
{
    MessageExternalFuzzer fuzzer = MessageExternalFuzzer(fuzzLocation)
        .FuzzInput(fuzzInput, fuzzInputSize);

    PASEEngineTest("Message Substitution Fuzzing")
            .Mutator(&fuzzer)
            .InitiatorPassword("TestPassword")
            .ResponderPassword("TestPassword")
            .ProposedConfig(kPASEConfig_Config1)
            .ResponderAllowedConfigs(kPASEConfig_Config1)
            .ConfirmKey(true)
            .LogMessageData(false)
            .Run();

}

int main(int argc, char *argv[])
{
    WEAVE_ERROR err;
#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    tcpip_init(NULL, NULL);
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

    err = nl::Weave::Platform::Security::InitSecureRandomDataSource(NULL, 64, NULL, 0);
    FAIL_ERROR(err, "InitSecureRandomDataSource() failed");

    printf("Starting tests\n");
    PASEEngine_ConfigTest1();
    PASEEngine_ConfigTest4();
    PASEEngineTest_MixedConfigs();
    printf("All tests succeeded\n");
}
