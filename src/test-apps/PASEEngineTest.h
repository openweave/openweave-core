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

#ifndef PASEENGINETEST_H
#define PASEENGINETEST_H

extern const char * INITIATOR_STEP_1;
extern const char * RESPONDER_RECONFIGURE;
extern const char * RESPONDER_STEP_1;
extern const char * RESPONDER_STEP_2;
extern const char * INITIATOR_STEP_2;
extern const char * RESPONDER_KEY_CONFIRM;


class MessageMutator
{
public:
    virtual void MutateMessage(const char *msgName, PacketBuffer *msgBuf);
    virtual ~MessageMutator() { }
};

class MessageExternalFuzzer : public MessageMutator
{
public:
    MessageExternalFuzzer(const char *msgType);
    virtual void MutateMessage(const char *msgType, PacketBuffer *msgBuf);
    MessageExternalFuzzer& FuzzInput(const uint8_t *val, size_t size);
    MessageExternalFuzzer& SaveCorpusFile(bool val);

private:
    void SaveCorpus(const uint8_t *inBuf, size_t size, const char *fileName);
    const char *mMsgType;
    const uint8_t *mFuzzInput;
    size_t mFuzzInputSize;
    bool mSaveCorpus;
};

class PASEEngineTest
{
public:
    PASEEngineTest(const char *testName);
    const char *TestName() const;

    uint32_t ProposedConfig() const;
    PASEEngineTest& ProposedConfig(uint32_t val);

    uint32_t InitiatorAllowedConfigs() const;
    PASEEngineTest& InitiatorAllowedConfigs(uint32_t val);

    uint32_t ResponderAllowedConfigs() const;
    PASEEngineTest& ResponderAllowedConfigs(uint32_t val);

    const char* InitiatorPassword() const;
    PASEEngineTest& InitiatorPassword(const char* val);

    const char* ResponderPassword() const;
    PASEEngineTest& ResponderPassword(const char* val);

    uint32_t ExpectReconfig() const;
    PASEEngineTest& ExpectReconfig(uint32_t expectedConfig);
    uint32_t ExpectedConfig() const;

    bool PerformReconfig() const;
    PASEEngineTest& PerformReconfig(bool val);

    bool ConfirmKey() const;
    PASEEngineTest& ConfirmKey(bool val);

    PASEEngineTest& ExpectError(WEAVE_ERROR err);
    PASEEngineTest& ExpectError(const char *opName, WEAVE_ERROR err);

    bool IsExpectedError(const char *opName, WEAVE_ERROR err) const;
    bool IsSuccessExpected() const;

    PASEEngineTest& Mutator(MessageMutator *mutator);

    bool LogMessageData() const;
    PASEEngineTest& LogMessageData(bool val);

    void Run();

private:
    void setAllowedResponderConfigs(WeavePASEEngine &responderEng);
    enum
    {
        kMaxExpectedErrors = 32
    };

    struct ExpectedError
    {
        const char *OpName;
        WEAVE_ERROR Error;
    };
    const char *mTestName;
    uint32_t mProposedConfig;
    const char *mInitPW;
    const char *mRespPW;
    uint32_t mInitiatorAllowedConfigs;
    uint32_t mResponderAllowedConfigs;
    bool mExpectReconfig;
    uint32_t mExpectedConfig;
    bool mConfirmKey;
    bool mForceRepeatedReconfig;
    ExpectedError mExpectedErrors[kMaxExpectedErrors];
    MessageMutator *mMutator;
    bool mLogMessageData;
};
#endif // PASEENGINETEST_H
