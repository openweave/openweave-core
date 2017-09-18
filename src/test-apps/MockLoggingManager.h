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
 *      This file declares mock LoggingManager
 *
 */

#ifndef MOCKLOGGINGMANAGER_H
#define MOCKLOGGINGMANAGER_H
#include <stdint.h>
#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveMessageLayer.h>

class EventGenerator
{
public:
    virtual void Generate(void) = 0;
    virtual size_t GetNumStates(void);
protected:
    EventGenerator(size_t aNumStates, size_t aInitialState);
    size_t mNumStates;
    size_t mState;
};

void EnableMockEventTimestampInitialCounter(void);

void InitializeEventLogging(WeaveExchangeManager *inMgr);

class MockEventGenerator
{
public:
    static MockEventGenerator * GetInstance(void);
    virtual WEAVE_ERROR Init(nl::Weave::WeaveExchangeManager *aExchangeMgr, EventGenerator *aEventGenerator, int aDelayBetweenEvents, bool aWraparound) = 0;
    virtual void SetEventGeneratorStop() = 0;
    virtual bool IsEventGeneratorStopped() = 0;
};

EventGenerator * GetTestDebugGenerator(void);
EventGenerator * GetTestLivenessGenerator(void);
EventGenerator * GetTestSecurityGenerator(void);
EventGenerator * GetTestTelemetryGenerator(void);
EventGenerator * GetTestTraitGenerator(void);

#endif /* MOCKLOGGINGMANAGER_H */
